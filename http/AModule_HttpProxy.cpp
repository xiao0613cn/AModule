#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#define _USE_HTTP_MSG_IMPL_ 1
#include "AModule_HttpSession.h"

#ifdef _WIN32
#pragma comment(lib, "../bin/AModule.lib")
#endif

static int on_m_begin(http_parser *parser) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);

	p->_httpmsg->reset();
	p->_header_block.set(NULL, 0, 0);
	p->_header_count = 0;

	p->_field_pos = p->_field_len = 0;
	p->_value_pos = p->_value_len = 0;
	p->_body_pos = p->_body_len = 0;
	return 0;
}
static int on_url_or_status(http_parser *parser, const char *at, size_t length) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_field_len == 0) {
		p->_field_pos = p->_value_pos = (at - p->outbuf()->ptr());
	}
	p->_field_len += length;
	p->_value_len += length;
	return 0;
}
static int on_h_field(http_parser *parser, const char *at, size_t length) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_value_len != 0) {
		if (++p->_header_count == 1) {
			p->_httpmsg->uri_set(p->_value(), 1);
		} else {
			p->_httpmsg->header_set(p->_field(), p->_value());
		}
		p->_field_pos = p->_field_len = 0;
		p->_value_pos = p->_value_len = 0;
	}

	if (p->_field_len == 0)
		p->_field_pos = (at - p->outbuf()->ptr());
	p->_field_len += length;
	return 0;
}
static int on_h_value(http_parser *parser, const char *at, size_t length) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	assert(p->_field_len != 0);
	if (p->_value_len == 0)
		p->_value_pos = (at - p->outbuf()->ptr());
	p->_value_len += length;
	return 0;
}
static int on_h_done(http_parser *parser) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	if (p->_value_len != 0) {
		p->_header_count++;
		p->_httpmsg->header_set(p->_field(), p->_value());
	}
	assert(p->_header_block._buf == NULL);
	p->_header_block.set(p->outbuf(), p->outbuf()->_bgn, p->_parser.nread);
	return 0;
}
static int on_body(http_parser *parser, const char *at, size_t length) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_body_len == 0)
		p->_body_pos = (at - p->outbuf()->ptr());
	p->_body_len += length;
	return 0;
}
static int on_m_done(http_parser *parser) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	http_parser_pause(parser, TRUE);
	return 0;
}
static int on_chunk_header(http_parser *parser) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	TRACE("chunk header, body = %lld.\n", parser->content_length);
	return 0;
}
static int on_chunk_complete(http_parser *parser) {
	HttpCompenont *p = container_of(parser, HttpCompenont, _parser);
	if (p->_body_len != 0)
		http_parser_pause(parser, TRUE);
	else
		; // last chunk size = 0
	return 0;
}
static const struct http_parser_settings cb_sets = {
	&on_m_begin,
	&on_url_or_status,
	&on_url_or_status,
	&on_h_field,
	&on_h_value,
	&on_h_done,
	&on_body,
	&on_m_done,
	&on_chunk_header,
	&on_chunk_complete
};
int HttpCompenont::on_iocom_output(AInOutComponent *c, int result) {
	HttpCompenont *p = (HttpCompenont*)c->_outuser;
	if (result < 0)
		return p->on_httpmsg(p, result);

	p->_parser.data = c->_outbuf;
	if (p->p_left() == 0)
		return 1; // need more data

	result = http_parser_execute(&p->_parser, &cb_sets, p->p_next(), p->p_left());
	if (p->_parser.http_errno == HPE_PAUSED)
		http_parser_pause(&p->_parser, FALSE);

	if (p->_parser.http_errno != HPE_OK) {
		TRACE("http_parser_error(%d) = %s.\n",
			p->_parser.http_errno, http_errno_description(p->_parser.http_errno));
		return p->on_httpmsg(p, -AMsgType_Private|p->_parser.http_errno);
	}
	p->_parsed_len += result;

	result = http_next_chunk_is_incoming(&p->_parser);
	if (result == 0) {
		// new buffer
		assert(p->p_left() == 0);
		int reserve = 0;
		if (!http_header_is_complete(&p->_parser)) {
			reserve = send_bufsiz;
		}
		else if (p->_body_len < max_body_size) {
			if (p->_body_len == 0)
				p->_body_pos = p->_parsed_len;
			c->_outbuf->pop(p->_body_pos);
			p->_parsed_len -= p->_body_pos;
			p->_body_pos = 0;
			reserve = min(p->_parser.content_length, max_body_size-p->_body_len);
		}
		if (reserve != 0) { // need more data
			result = ARefsBuf::reserve(c->_outbuf, reserve, recv_bufsiz);
			TRACE2("resize buffer size = %d, left = %d, reserve = %d, result = %d.\n",
				c->_outbuf->len(), c->_outbuf->left(), reserve, result);
			return (result >= 0) ? 1 : p->on_httpmsg(p, result);
		}
	}
	p->_body_pos += c->_outbuf->_bgn;
	c->_outbuf->pop(p->_parsed_len);
	p->_parsed_len = 0;

	assert(p->_httpmsg->header_num()+1 == p->_header_count);
	p->_httpmsg->_parser = p->_parser;
	p->_httpmsg->body_set(c->_outbuf, p->_body_pos, p->_body_len);

	return p->on_httpmsg(p, 1); // return httpMsgType_HttpMsg; > AMsgType_Class
}

static int encode_headers(ARefsBuf *&buf, HttpMsg *hm) {
	str_t f, v;
	int result = 20;
	while ((f = hm->_kv_next(hm, HttpMsg::KV_Header, f, &v)).str != NULL) {
		result += f.len;
		result += v.len + 6;
	}
	if (hm->body_len() > 0)
		result += 64;

	result = ARefsBuf::reserve(buf, result, send_bufsiz);
	if (result < 0)
		return result;

	v = hm->uri_get(1);
	if (hm->_parser.type == HTTP_REQUEST) {
		buf->strfmt("%s %.*s HTTP/%d.%d\r\n", http_method_str(hm->_parser.method),
			v.len, v.str, hm->_parser.http_major, hm->_parser.http_minor);
	} else {
		buf->strfmt("HTTP/%d.%d %d %.*s\r\n", hm->_parser.http_major, hm->_parser.http_minor,
			hm->_parser.status_code, v.len, v.str);
	}
	if (hm->body_len() > 0) {
		buf->strfmt("Content-Length: %lld\r\n", hm->body_len());
	}
	f.str = NULL;
	while ((f = hm->_kv_next(hm, HttpMsg::KV_Header, f, &v)).str != NULL) {
		buf->strfmt("%.*s: %.*s\r\n", f.len, f.str, v.len, v.str);
	}
	buf->strfmt("\r\n");
	return buf->len();
}

static int HttpMsgInputStatus(HttpConnection *p, AMessage *msg, HttpMsg *hm, int result)
{
	while (result > 0) {
		if (msg->data == NULL) { // do input header
			assert(msg->done != &AInOutComponent::_inmsg_done
			    && msg->done != &AInOutComponent::_outmsg_done);
			result = encode_headers(p->_inbuf, hm);
			if (result < 0)
				break;

			msg->init(ioMsgType_Block, p->_inbuf->ptr(), p->_inbuf->len());
			result = p->_iocom._io->input(msg);
			continue;
		}
		if (msg->data == p->_inbuf->ptr()) { // do input body
			assert(msg->size == p->_inbuf->len());

			p->_inbuf->reset();
			msg->init(ioMsgType_Block, hm->body_ptr(), hm->body_len());

			if (msg->size > 0) {
				result = p->_iocom._io->input(msg);
				continue;
			}
		}
		assert(msg->data == hm->body_ptr());
		assert(msg->size == hm->body_len());
		break; // input done
	}
	if (result != 0)
		msg->done = ((msg == &p->_iocom._inmsg) ? p->raw_inmsg_done : p->raw_outmsg_done);
	return result;
}

static int HttpMsgInputDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._inmsg);

	msg = list_first_entry(&p->_iocom._queue, AMessage, entry);
	assert(msg->type == httpMsgType_HttpMsg);

	result = HttpMsgInputStatus(p, &p->_iocom._inmsg, (HttpMsg*)msg->data, result);
	if (result != 0)
		result = p->_iocom._inmsg.done2(result);
	return result;
}

static int HttpSvcResp(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._outmsg);
	result = HttpMsgInputStatus(p, msg, p->_resp, result);
	if (result != 0) {
		msg->init();
		msg->done2(result);
	}
	return result;
}

static int HttpConnectionInput(AInOutComponent *c, AMessage *msg)
{
	HttpConnection *p = container_of(c, HttpConnection, _iocom);
	assert(c->_inmsg.done == &AInOutComponent::_inmsg_done);

	switch (msg->type)
	{
	case AMsgType_Unknown:
	case ioMsgType_Block:
		c->_inmsg.init(msg);
		return c->_io->input(&c->_inmsg);

	case httpMsgType_HttpMsg:
		if (msg == &c->_outmsg) {
			assert(msg->data == (char*)p->_resp);
			p->raw_outmsg_done = msg->done;
			msg->init();
			msg->done = &HttpSvcResp;
			return HttpMsgInputStatus(p, msg, (HttpMsg*)msg->data, 1);
		}

		p->raw_inmsg_done = c->_inmsg.done;
		c->_inmsg.init();
		c->_inmsg.done = &HttpMsgInputDone;
		return HttpMsgInputStatus(p, &c->_inmsg, (HttpMsg*)msg->data, 1);

	default: assert(0);
		return -EINVAL;
	}
}

static int HttpConnectionCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpConnection *p = (HttpConnection*)*object;
	p->init();
	p->_init_push(&p->_http);
	p->_init_push(&p->_iocom); p->_iocom.do_input = &HttpConnectionInput;

	p->_svc = NULL;
	p->_inbuf = NULL;
	p->_req = NULL;
	p->_resp = NULL;
	return 1;
}

static void HttpConnectionRelease(AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	reset_nif(p->_resp, NULL, delete p->_resp);
	reset_nif(p->_req, NULL, delete p->_req);
	release_s(p->_svc);

	release_s(p->_inbuf);
	p->_pop_exit(&p->_http);
	p->_pop_exit(&p->_iocom);
	p->exit();
}

static const str_t HttpMethodStrs[] = {
#define XX(num, name, string)  str_t(#string, sizeof(#string)-1),
	HTTP_METHOD_MAP(XX)
#undef XX
	str_t()
};

static int HttpProbe(AObject *other, AMessage *msg, AOption *option)
{
	if (msg->size < 4)
		return -1;
	for (const str_t *m = HttpMethodStrs; m->str != NULL; ++m) {
		if ((msg->size > m->len) && (msg->data[m->len] == ' ')
		 && (strncmp(msg->data, m->str, m->len) == 0))
			return 60;
	}
	return 0;
}

AModule HttpConnectionModule = {
	"AEntity",
	"HttpConnection",
	sizeof(HttpConnection),
	NULL, NULL,
	&HttpConnectionCreate,
	&HttpConnectionRelease,
	&HttpProbe,
};
static int reg_conn = AModuleRegister(&HttpConnectionModule);


static int HttpSvcRecvMsg(HttpCompenont *c, int result)
{
	HttpConnection *p = container_of(c, HttpConnection, _http);
	if (result < 0) {
		TRACE("http connection down, result = %d.\n", result);
		return result;
	}
	if (p->_req->_parser.type != HTTP_REQUEST) {
		TRACE("invalid http type: %d.\n", p->_req->_parser.type);
		return -EINVAL;
	}
	{
		ARefsBuf *tmp = p->_resp->_body_buf;
		if (tmp) tmp->reset();
		p->_resp->_body_buf = NULL;

		p->_resp->reset();
		p->_resp->_parser = p->_http._parser;
		p->_resp->_parser.type = HTTP_RESPONSE;
		p->_resp->_body_buf = tmp;
	}

	AService *svc = AServiceProbe(p->_svc, p, NULL);
	if (svc != NULL) {
		return svc->run(svc, p, svc->_svc_option);
	}

	FILE *fp = fopen(p->_req->uri_get(1).str+1, "rb");
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		p->_resp->body_len() = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	if (p->_resp->body_len() <= 0) {
		p->_resp->_parser.status_code = 404;
		p->_resp->uri_set(str_sz("File Invalid"), 1);
	} else if (ARefsBuf::reserve(p->_resp->_body_buf, p->_resp->body_len(), recv_bufsiz) < 0) {
		p->_resp->_parser.status_code = 501;
		p->_resp->uri_set(str_sz("Out Of Memory"), 1);
	} else if (fread(p->_resp->_body_buf->next(), p->_resp->body_len(), 1, fp) != 1) {
		p->_resp->_parser.status_code = 502;
		p->_resp->uri_set(str_sz("Read File Error"), 1);
	} else {
		p->_resp->_body_buf->push(p->_resp->body_len());
		p->_resp->_parser.status_code = 200;
		p->_resp->uri_set(str_sz("OK"), 1);
	}
	reset_s(fp, NULL, fclose);
	p->_resp->header_set(str_sz("Content-Type"), str_sz("text/html"));

	p->raw_outmsg_done = p->_iocom._outmsg.done;
	p->_iocom._outmsg.init();
	p->_iocom._outmsg.done = &HttpSvcResp;

	result = HttpMsgInputStatus(p, &p->_iocom._outmsg, p->_resp, 1);
	return result; // continue recv next request
}

static int HttpSvcRun(AService *svc, AObject *object, AOption *option)
{
	HttpConnection *p = (HttpConnection*)object;
	addref_set(p->_svc, svc);

	if (!p->_req) p->_req = new HttpMsgImpl();
	if (!p->_resp) p->_resp = new HttpMsgImpl();

	return p->_http.try_output(&p->_iocom, p->_req, &HttpSvcRecvMsg);
}

static int HttpServiceCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *svc = (AService*)*object;
	svc->init();
	svc->_peer_module = &HttpConnectionModule;
	svc->run = &HttpSvcRun;
	return 1;
}

static void HttpServiceRelease(AObject *object)
{
	AService *svc = (AService*)object;
	svc->exit();
}

AModule HttpServiceModule = {
	AService::class_name(),
	"HttpService",
	sizeof(AService),
	NULL, NULL,
	&HttpServiceCreate,
	&HttpServiceRelease,
	&HttpProbe,
};
static int reg_svc = AModuleRegister(&HttpServiceModule);

#if 0
static SessionManager sm;

static void HttpProxyClose(HttpClient *p)
{
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	if (ctx->session != NULL) {
		SessionCtx *s = to_sess(ctx->session);

		s->lock();
		list_del_init(&ctx->sess_conn_entry);
		s->unlock();

		AObjectRelease(ctx->session);
		ctx->session = NULL;
	}
	ctx->active = 0;
	release_s(ctx->proc_buf, ARefsBufRelease, NULL);
	AObjectRelease(p);
}

static int HttpProxyDispatch(AMessage *msg, int result);
static int HttpProxySendDone(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, recv_msg);
	if (result >= 0) {
		p->send_msg.init();
		p->send_msg.done = &HttpProxyDispatch;
		result = HttpClientDoRecv(p, &p->send_msg);
	}
	if (result != 0)
		result = HttpProxyDispatch(&p->send_msg, result);
	return result;
}

static int HttpGetFile(HttpClient *p, const char *url)
{
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	int status_code = 200;
	long file_size = 0;

	p->h_f_ptr(0)[p->h_f_len(0)] = '\0';
	FILE *fp = fopen(url, "rb");
	if (fp == NULL) {
		status_code = 404;
	} else {
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	if (file_size <= 0) {
		status_code = 404;
	} else if (ARefsBufCheck(ctx->proc_buf, file_size, recv_bufsiz, NULL, NULL) < 0) {
		status_code = 501;
	} else if (fread(ctx->proc_buf->data, file_size, 1, fp) != 1) {
		status_code = 502;
	} else {
		p->recv_msg.init(ioMsgType_Block, ctx->proc_buf->data, file_size);
	}
	release_s(fp, fclose, NULL);

	append_data("HTTP/%d.%d %d xxx\r\n"
		"Content-Type: text/html\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor, status_code);

	p->send_status = s_send_private_header;
	int result = HttpClientDoSend(p, &p->recv_msg);
	return result;
}

#define append_proc(fmt, ...) \
	p->recv_msg.size += snprintf(ctx->proc_buf->data+p->recv_msg.size, ctx->proc_buf->size-p->recv_msg.size, fmt, ##__VA_ARGS__)

static int HttpProxyGetStatus(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;

	result = ARefsBufCheck(ctx->proc_buf, 4096, 0, NULL, NULL);
	if (result < 0)
		return result;

	p->recv_msg.init(ioMsgType_Block, ctx->proc_buf->data, 0);
	append_proc("<html><body>connect count=%ld</body></html>",
		ctx->sm->conn_count);

	append_data("HTTP/%d.%d 200 OK\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor);

	p->send_status = s_send_private_header;
	result = HttpClientDoSend(p, &p->recv_msg);
	return result;
}

struct media_file_t {
	DWORD media_sequence;
	DWORD target_duration;
	//struct tm program_datetime;
	int   content_length;
	int   nb_buffers;
	ARefsBuf *buffers[16];
};
extern DWORD media_file_get(media_file_t *seg, DWORD cseq);
extern void media_file_release(media_file_t *mf);

static void HttpProxySendTS_wait(AOperator *asop, int result)
{
	HttpCtxExt *ctx = container_of(asop, HttpCtxExt, asop);
	if (result >= 0) {
		result = (ctx->active != 0) ? 1 : -EIO;
	}
	ctx->p()->send_msg.done(&ctx->p()->send_msg, result);
}
static int HttpProxyGetTS(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	media_file_t *mf = (media_file_t*)(ctx+1);

#if 1
	{
#else
	while (result > 0)
	{
		switch (p->send_status)
		{
		case s_invalid:
			append_data("HTTP/1.1 200 OK\r\n"
				"Content-Type: video/mp2t\r\n"
				"Cache-Control: no-cache\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n");
			memset(mf, 0, sizeof(*mf));

			p->send_status = s_send_header;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_header:
			if (media_file_get(mf, mf->media_sequence+1) == 0) {
				ctx->asop.done = &HttpProxySendTS_wait;
				ctx->asop.delay(NULL, 20);
				return 0;
			}

			ctx->active = GetTickCount();
			p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
			append_data("%x\r\n", mf->content_length);
			ctx->segix = 0;

			p->send_status = s_send_content_data;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_content_data:
			p->send_msg.init(ioMsgType_Block, mf->buffers[ctx->segix]->ptr(), mf->buffers[ctx->segix]->len());
			if (++ctx->segix == mf->nb_buffers)
				p->send_status = s_send_chunk_tail;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_chunk_tail:
			media_file_release(mf);
			p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
			append_crlf();
			p->send_status = s_send_header;
			result = ioInput(p->io, &p->send_msg);
			break;

		default:
			assert(FALSE);
			result = -EACCES;
			break;
		}
	}
	if (result < 0) {
		media_file_release(mf);
#endif
		HttpProxyClose(p);
	}
	return 0;
}

static const struct HttpDispatch {
	const char *url;
	int         len;
	int       (*get)(AMessage*, int);
	int       (*post)(AMessage*, int);
}
HttpDispatchMap[] = {
#define XX(url, get, post) \
	{ url, sizeof(url)-1, get, post }

	XX("/status.html", &HttpProxyGetStatus, NULL ),
	XX("/video.ts", &HttpProxyGetTS, NULL ),
#undef XX
	{ NULL }
};

static int HttpProxyDispatch(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	while (result > 0)
	{
		ctx->active = GetTickCount();
		assert(p->send_status == s_invalid);
		p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
		p->recv_msg.init();
		p->recv_msg.done = &HttpProxySendDone;

		const char *url = p->h_f_ptr(0);
		for (const struct HttpDispatch *d = HttpDispatchMap; d->url != NULL; ++d) {
			if (strncasecmp(url, d->url, d->len) == 0)
			{
				p->send_msg.done = NULL;
				switch (p->recv_parser.method)
				{
				case HTTP_GET: p->send_msg.done = d->get; break;
				case HTTP_POST: p->send_msg.done = d->post; break;
				}
				if (p->send_msg.done != NULL) {
					result = p->send_msg.done(&p->send_msg, result);
					goto _check;
				}
				break;
			}
		}
		result = HttpGetFile(p, url+1);
_check:
		if (result <= 0)
			break;

		p->send_msg.init();
		p->send_msg.done = &HttpProxyDispatch;
		result = HttpClientDoRecv(p, &p->send_msg);
	}
	if (result < 0)
		HttpProxyClose(p);
	return result;
}

static void on_http_timeout(struct list_head *conn)
{
	HttpCtxExt *ctx = container_of(conn, HttpCtxExt, sm_conn_entry);
	HttpClient *p = ctx->p();
	ctx->active = 0;
	p->io->close(NULL);
	p->release();
}

static int HttpProxyOpen(AObject *object, AMessage *msg)
{
	HttpClient *p = (HttpClient*)object;
	int result = HttpClientAppendOutput(p, msg);
	if (result < 0)
		return result;

	p->addref();
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	ctx->active = GetTickCount();
	ctx->session = NULL;
	ctx->proc_buf = NULL;
	ctx->recv_param_count = 0;

	ctx->sm = &sm;
	p->addref();
	sm.push(&ctx->sm_conn_entry);

	p->send_msg.init();
	p->send_msg.done = &HttpProxyDispatch;
	result = HttpClientDoRecv(p, &p->send_msg);
	if (result != 0)
		result = HttpProxyDispatch(&p->send_msg, result);
	return -EBUSY;
}

static int HttpProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->size < 10)
		return -1;

	for (int ix = 0; ix < HTTP_TRACE; ++ix) {
		if (strncmp(msg->data, http_method_str((enum http_method)ix), 3) == 0)
			return 60;
	}
	return -1;
}

static int HttpProxyCheck(AOperator *asop, int result)
{
	SessionManager *sm = container_of(asop, SessionManager, check_timer);
	if (result >= 0) {
		SessionCheck(sm);

		sm->check_timer.delay(NULL, 5000);
	} else {
		//shutdown
	}
	return result;
}

static int HttpProxyInit(AOption *global_option, AOption *module_option, BOOL first)
{
	if ((module_option == NULL) || !first)
		return 0;

	SessionInit(&sm, NULL);

	sm.conn_tick_offset = offsetof(HttpCtxExt, active) - offsetof(HttpCtxExt, sm_conn_entry);
	sm.on_connect_timeout = &on_http_timeout;

	sm.check_timer.timer();
	sm.check_timer.done = &HttpProxyCheck;
	sm.check_timer.delay(NULL, 5000);
	return 1;
}

IOModule HTTPProxyModule = { {
	"proxy",
	"HttpProxy",
	sizeof(HttpClient),
	&HttpProxyInit, NULL,
	&HttpClientCreate,
	&HttpClientRelease,
	&HttpProxyProbe, },
	&HttpProxyOpen,
	NULL, NULL,
	NULL, NULL,
	NULL,
};

static auto_reg_t reg(HTTPProxyModule.module);
#endif