#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"

#ifdef _WIN32
#pragma comment(lib, "../bin/AModule.lib")
#endif

static int HttpInputHeadStatus(HttpConnection *p, HttpMsg *hm, int result)
{
	AInOutComponent *c = &p->_iocom;
	assert(c->_inmsg.data == p->_inbuf->ptr());
	assert(c->_inmsg.size == p->_inbuf->len());

	p->_inbuf->reset();
	c->_inmsg.done = &AInOutComponent::_inmsg_done;

	if ((result >= 0) && (hm->body_len() > 0)) {
		c->_inmsg.init(ioMsgType_Block, hm->body_ptr(), hm->body_len());
		result = c->_io->input(&c->_inmsg);
	}
	return result;
}

static int HttpInputHeadDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._inmsg);

	msg = list_first_entry(&p->_iocom._queue, AMessage, entry);
	assert(msg->type == HttpMsgType);
	HttpMsg *hm = (HttpMsg*)msg->data;

	result = HttpInputHeadStatus(p, hm, result);
	if (result != 0)
		result = p->_iocom._inmsg.done2(result);
	return result;
}

static int HttpConnInput(AInOutComponent *c, AMessage *msg)
{
	if (msg->type == ioMsgType_Block) {
		c->_inmsg.init(msg);
		assert(c->_inmsg.done == &AInOutComponent::_inmsg_done);
		return c->_io->input(&c->_inmsg);
	}
	if (msg->type != HttpMsgType)
		return -EINVAL;

	HttpConnection *p = container_of(c, HttpConnection, _iocom);
	HttpMsg *hm = (HttpMsg*)msg;
	str_t v;

	int result = 20;
	for (int ix = 0; ix < hm->_head_num; ++ix) {
		result += hm->at(ix, &v).len;
		result += v.len + 6;
	}
	if (hm->body_len() > 0)
		result += 64;

	result = ARefsBuf::reserve(p->_inbuf, result, 2048);
	if (result < 0)
		return result;

	v = hm->url();
	if (hm->_parser.type == HTTP_REQUEST) {
		p->_inbuf->strfmt("%s %.*s HTTP/%d.%d\r\n", http_method_str(hm->_parser.method),
			v.len, v.str, hm->_parser.http_major, hm->_parser.http_minor);
	} else {
		p->_inbuf->strfmt("HTTP/%d.%d %d %.*s\r\n", hm->_parser.http_major, hm->_parser.http_minor,
			hm->_parser.status_code, v.len, v.str);
	}
	if (hm->body_len() > 0) {
		p->_inbuf->strfmt("Content-Length: %lld\r\n", hm->body_len());
	}
	for (int ix = 1; ix < hm->_head_num; ++ix) {
		str_t f = hm->at(ix, &v);
		p->_inbuf->strfmt("%.*s: %.*s\r\n", f.len, f.str, v.len, v.str);
	}
	p->_inbuf->strfmt("\r\n");

	c->_inmsg.init(ioMsgType_Block, p->_inbuf->ptr(), p->_inbuf->len());
	c->_inmsg.done = &HttpInputHeadDone;

	result = c->_io->input(&c->_inmsg);
	if (result != 0)
		result = HttpInputHeadStatus(p, hm, result);
	return result;
}

static int HttpConnectionCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpConnection *p = (HttpConnection*)*object;
	p->init();
	p->_init_push(&p->_iocom);
	p->_iocom.do_input = &HttpConnInput;
	p->_inbuf = NULL;

	http_parser_init(&p->_parser, HTTP_BOTH, p);
	return 1;
}

static void HttpConnectionRelease(AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	p->_pop_exit(&p->_iocom);
	p->exit();
}

static int HttpProbe(AObject *other, AMessage *msg, AOption *option)
{
	if (msg->size < 4)
		return -1;
	if (strncmp_sz(msg->data, "GET ") == 0) {
		return strstr(msg->data, "HTTP/") ? 90 : 40;
	}
	return 0;
}

AModule HttpConnectionModule = {
	"HttpConnectionModule",
	"HttpConnectionModule",
	sizeof(HttpConnection),
	NULL, NULL,
	&HttpConnectionCreate,
	&HttpConnectionRelease,
	&HttpProbe,
};
static int reg_conn = AModuleRegister(&HttpConnectionModule);

static int on_m_begin(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;

	p->recv_header_count = 0;
	p->h_f_pos() = 0;
	p->h_f_len() = 0;
	p->h_v_pos() = 0;
	p->h_v_len() = 0;

	release_s(p->recv_header_buffer);
	p->recv_header_pos = 0;
	return 0;
}

static int on_url_or_status(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_f_len() == 0) {
		p->h_v_pos() = p->h_f_pos() = (at - p->recv_buffer->ptr());
	}
	p->h_f_len() += length;
	p->h_v_len() += length;
	return 0;
}

static int on_h_field(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_v_len() != 0) {
		if (++p->recv_header_count >= max_head_count)
			return -EACCES;
		p->h_f_pos() = 0;
		p->h_f_len() = 0;
		p->h_v_pos() = 0;
		p->h_v_len() = 0;
	}

	if (p->h_f_len() == 0)
		p->h_f_pos() = (at - p->recv_buffer->ptr());
	p->h_f_len() += length;
	return 0;
}

static int on_h_value(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_v_len() == 0) {
		p->h_v_pos() = (at - p->recv_buffer->ptr());
	}
	p->h_v_len() += length;
	return 0;
}

static int on_h_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_v_len() != 0) {
		p->recv_header_count++;
	}

	p->recv_header_buffer = p->recv_buffer;
	p->recv_header_buffer->addref();
	p->recv_header_pos = p->recv_buffer->_bgn;
	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->recv_body_len == 0)
		p->recv_body_pos = (at - p->recv_buffer->ptr());
	p->recv_body_len += length;
	return 0;
}

static int on_m_done(http_parser *parser)
{
	//HttpClient *p = (HttpClient*)parser->data;
	http_parser_pause(parser, TRUE);
	return 0;
}

static int on_chunk_header(http_parser *parser)
{
	//HttpClient *p = (HttpClient*)parser->data;
	TRACE("chunk header, body = %lld.\n", parser->content_length);
	return 0;
}

static int on_chunk_complete(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->recv_body_len != 0)
		http_parser_pause(parser, TRUE);
	else
		; // last chunk size = 0
	return 0;
}

static const struct http_parser_settings cb_sets = {
	&on_m_begin, &on_url_or_status, &on_url_or_status, &on_h_field, &on_h_value,
	&on_h_done, &on_body, &on_m_done,
	&on_chunk_header, &on_chunk_complete
};

static int HttpConnCycle(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._outmsg);
	AInOutComponent *c = &p->_iocom;

	while (result >= 0) {
	switch (p->_status)
	{
	case s_recv_header:
		c->_outbuf->push(msg->size);

		result = http_parser_execute(&p->_parser, &cb_sets, p->r_p_ptr(), p->r_p_len());
		if (p->_parser.http_errno == HPE_PAUSED)
			http_parser_pause(&p->_parser, FALSE);

		if (p->_parser.http_errno != HPE_OK) {
			TRACE("http_errno_name = %s.\n", http_parser_error(&p->_parser));
			result = -p->_parser.http_errno;
			break;
		}
		result = http_next_chunk_is_incoming(&p->_parser);
		if (!result) {
	}
	}
	if (result < 0)
		p->release();
	return result;
}

static int HttpConnRun(AService *svc, AObject *object, AOption *option)
{
	HttpConnection *p = (HttpConnection*)object;
	AInOutComponent *c = &p->_iocom;
	int result = ARefsBuf::reserve(c->_outbuf, 1024, 2048);
	if (result < 0)
		return result;

	p->addref();
	p->_status = s_recv_header;
	c->_outmsg.init();
	c->_outmsg.done = &HttpConnCycle;

	if (c->_outbuf->len() != 0) {
		c->_outmsg.done2(0);
	} else {
		result = c->_io->output(&c->_outmsg, c->_outbuf);
		if (result != 0)
			c->_outmsg.done2(result);
	}
	return 0;
}

static int HttpServiceCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *svc = (AService*)*object;
	svc->peer_option = NULL;
	svc->peer_module = &HttpConnectionModule;
	svc->start = NULL;
	svc->stop = NULL;
	svc->run = &HttpConnRun;
	svc->abort = NULL;
	return 1;
}

AModule HttpServiceModule = {
	"AService",
	"HttpServiceModule",
	sizeof(AService),
	NULL, NULL,
	&HttpServiceCreate,
	NULL,
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