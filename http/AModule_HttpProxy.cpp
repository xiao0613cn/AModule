#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/ASystem.h"
#include "AModule_HttpSession.h"

#ifdef _WIN32
#pragma comment(lib, "../bin_win32/AModule.lib")
#endif

extern HttpConnectionModule HCM;

static int HttpSvcHandle(HttpConnection *p, HttpMsg *req, HttpMsg *resp)
{
	if (req->_parser.type != HTTP_REQUEST) {
		TRACE("invalid http type: %d.\n", req->_parser.type);
		return -EINVAL;
	}
	resp->reset();
	resp->_parser = p->_http._parser;
	resp->_parser.type = HTTP_RESPONSE;

	AService *svc = AServiceProbe(p->_svc, p, NULL);
	if (svc != NULL) {
		return svc->run(svc, p);
	}

	resp->_parser.status_code = 400;
	resp->uri_set(sz_t("Bad Request"), 1);
	return p->svc_resp();
}

static int HttpSvcRecvMsg(HttpParserCompenont *c, int result)
{
	HttpConnection *p = container_of(c, HttpConnection, _http);
	if (result >= 0)
		result = HttpSvcHandle(p, p->_req, p->_resp);
	if (result < 0) {
		TRACE("http connection down, result = %d.\n", result);
		if (!RB_EMPTY_NODE(&p->_map_node)) {
			AEntityManager *em = p->_svc->_sysmng->_all_entities;
			em->lock();
			em->_pop(em, p);
			em->unlock();
		}
	}
	return result;
}

static int HttpSvcRun(AService *svc, AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	addref_s(p->_svc, svc);

	AEntityManager *em = svc->_sysmng->_all_entities;
	p->_iocom._mutex = &em->_mutex;
	p->_iocom._abort = false;

	em->lock();
	em->_push(em, p);
	em->unlock();

	ARefsBuf::reserve(p->_inbuf, 512, recv_bufsiz);
	if (p->_req == NULL) p->_req = HCM.hm_create();
	if (p->_resp == NULL) p->_resp = HCM.hm_create();

	return p->_http.try_output(&HCM, &p->_iocom, p->_req, &HttpSvcRecvMsg);
}

static void HttpSvcStop(AService *svc)
{
	AEntityManager *em = svc->_sysmng->_all_entities;
	HttpConnection *p = NULL;

	em->lock();
	AComponent *c = em->_upper_com(em, p, HttpParserCompenont::name(), -1);
	while (c != NULL) {
		p = (HttpConnection*)c->_entity;
		c = em->_next_com(em, p, HttpParserCompenont::name(), -1);

		if ((p->_module == &HCM.module) && (p->_svc == svc)) {
			p->_iocom._io->shutdown();
			em->_pop(em, p);
		}
	}
	em->unlock();
}

static int HttpSvcCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *svc = (AService*)*object;
	svc->init();
	svc->_peer_module = &HCM.module;
	svc->stop = &HttpSvcStop;
	svc->run = &HttpSvcRun;
	return 1;
}

static void HttpSvcRelease(AObject *object)
{
	AService *svc = (AService*)object;
	svc->exit();
}

AModule HttpServiceModule = {
	AService::class_name(),
	"HttpService",
	sizeof(AService),
	NULL, NULL,
	&HttpSvcCreate,
	&HttpSvcRelease,
	HCM.module.probe,
};
static int reg_svc = AModuleRegister(&HttpServiceModule);


//////////////////////////////////////////////////////////////////////////
// Http File Service
static int HttpFileSvcRun(AService *hfs, AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	HttpMsg *req = p->_req;
	HttpMsg *resp = p->_resp;

	assert(p->_inbuf->len() == 0);
	int body_len = 0;

	FILE *fp = fopen(req->uri_get(1).str+1, "rb");
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		body_len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	if (body_len <= 0) {
		resp->_parser.status_code = 404;
		resp->uri_set(sz_t("File Not Found"), 1);
	} else if (ARefsBuf::reserve(p->_inbuf, body_len, recv_bufsiz) < 0) {
		resp->_parser.status_code = 500;
		resp->uri_set(sz_t("Out Of Memory"), 1);
	} else if (fread(p->_inbuf->next(), body_len, 1, fp) != 1) {
		resp->_parser.status_code = 420;
		resp->uri_set(sz_t("Read File Error"), 1);
	} else {
		p->_inbuf->push(body_len);
		resp->body_set(p->_inbuf, p->_inbuf->_bgn, p->_inbuf->len());
		p->_inbuf->pop(p->_inbuf->len());

		resp->_parser.status_code = 200;
		resp->uri_set(sz_t("OK"), 1);
		resp->header_set(sz_t("Content-Type"), sz_t("text/html"));
	}
	reset_s(fp, NULL, fclose);

	return p->svc_resp();
}

static int HttpFileSvcCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *hfs = (AService*)*object;
	hfs->init();
	hfs->run = &HttpFileSvcRun;
	return 1;
}

static void HttpFileSvcRelease(AObject *object)
{
	AService *hfs = (AService*)object;
	hfs->exit();
}

static int HttpFileSvcProbe(AObject *object, AObject *other, AMessage *msg)
{
	if ((object == NULL) || (other == NULL) || (other->_module != &HCM.module))
		return -1;
	AService *hfs = (AService*)object;
	HttpConnection *p = (HttpConnection*)other;
	if (hfs->_parent == p->_svc)
		return 40;
	return -1;
}

AModule HttpFileServiceModule = {
	AService::class_name(),
	"HttpFileService",
	sizeof(AService),
	NULL, NULL,
	&HttpFileSvcCreate,
	&HttpFileSvcRelease,
	&HttpFileSvcProbe,
};
static int reg_hfs = AModuleRegister(&HttpFileServiceModule);

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