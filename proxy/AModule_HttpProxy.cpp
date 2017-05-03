#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"


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
	AObjectRelease(&p->object);
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
	append_proc("<html><body>connect count=%d</body></html>",
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
				ctx->asop.callback = &HttpProxySendTS_wait;
				AOperatorTimewait(&ctx->asop, NULL, 20);
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
			if (_strnicmp(url, d->url, d->len) == 0)
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
	p->io->close(p->io, NULL);
	p->object.release2();
}

static int HttpProxyOpen(AObject *object, AMessage *msg)
{
	HttpClient *p = to_http(object);
	int result = HttpClientAppendOutput(p, msg);
	if (result < 0)
		return result;

	AObjectAddRef(&p->object);
	HttpCtxExt *ctx = (HttpCtxExt*)p->url;
	ctx->active = GetTickCount();
	ctx->session = NULL;
	ctx->proc_buf = NULL;
	ctx->recv_param_count = 0;

	ctx->sm = &sm;
	p->object.addref();
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

static void HttpProxyCheck(AOperator *asop, int result)
{
	SessionManager *sm = container_of(asop, SessionManager, check_timer);
	if (result >= 0) {
		SessionCheck(sm);

		AOperatorTimewait(&sm->check_timer, NULL, 5000);
	} else {
		//shutdown
	}
}

static int HttpProxyInit(AOption *global_option, AOption *module_option)
{
	SessionInit(&sm, NULL);

	sm.conn_tick_offset = offsetof(HttpCtxExt, active) - offsetof(HttpCtxExt, sm_conn_entry);
	sm.on_connect_timeout = &on_http_timeout;

	sm.check_timer.callback = &HttpProxyCheck;
	AOperatorTimewait(&sm.check_timer, NULL, 5000);
	return 0;
}

AModule HTTPProxyModule = {
	"proxy",
	"HttpProxy",
	sizeof(HttpClient),
	&HttpProxyInit, NULL,
	&HttpClientCreate,
	&HttpClientRelease,
	&HttpProxyProbe,
	0,
	&HttpProxyOpen,
	NULL, NULL,
	NULL, NULL,
	NULL,
};
