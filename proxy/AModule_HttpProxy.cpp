#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"

static HttpSessionManager sm;

static void HttpProxyClose(HttpClient *p)
{
	if (p->session != NULL) {
		HttpSession *s = to_sess(p->session);

		s->lock();
		list_del_init(&p->conn_entry);
		s->unlock();

		AObjectRelease(p->session);
		p->session = NULL;
	}
	p->active = 0;
	AObjectRelease(&p->object);
}

static int HttpProxyDispatch(AMessage *msg, int result);
static int HttpProxySendDone(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, recv_msg);
	if (result >= 0) {
		p->send_msg.init();
		p->send_msg.done = &HttpProxyDispatch;
		result = HttpClientDoRecvResponse(p, &p->send_msg);
	}
	if (result != 0)
		result = HttpProxyDispatch(&p->send_msg, result);
	return result;
}

static int HttpGetFile(HttpClient *p, const char *url)
{
	int status_code = 200;
	long file_size = 0;

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
	} else if (ARefsBufCheck(p->proc_buf, file_size, recv_bufsiz, NULL, NULL) < 0) {
		status_code = 501;
	} else if (fread(p->proc_buf->data, file_size, 1, fp) != 1) {
		status_code = 502;
	} else {
		p->recv_msg.init(ioMsgType_Block, p->proc_buf->data, file_size);
	}
	release_s(fp, fclose, NULL);

	append_data("HTTP/%d.%d %d xxx\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor, status_code);

	p->send_status = s_send_private_header;
	int result = HttpClientDoSendRequest(p, &p->recv_msg);
	return result;
}

#define append_proc(fmt, ...) \
	p->recv_msg.size += snprintf(p->proc_buf->data+p->recv_msg.size, p->proc_buf->size-p->recv_msg.size, fmt, ##__VA_ARGS__)

static int HttpProxyGetStatus(HttpClient *p)
{
	int result = ARefsBufCheck(p->proc_buf, 4096, 0, NULL, NULL);
	if (result < 0)
		return result;

	p->recv_msg.init(ioMsgType_Block, p->proc_buf->data, 0);
	append_proc("<html><body>connect count=%d</body></html>",
		sm.conn_count);

	append_data("HTTP/%d.%d 200 OK\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor);

	p->send_status = s_send_private_header;
	result = HttpClientDoSendRequest(p, &p->recv_msg);
	return result;
}

static const struct HttpDispatch {
	const char *url;
	int         len;
	enum http_method method;
	int       (*proc)(HttpClient*);
}
HttpDispatchMap[] = {
#define XX(url, method, proc) \
	{ url, sizeof(url-1), method, proc }

	XX("/status.html", HTTP_GET, &HttpProxyGetStatus ),
#undef XX
	{ NULL }
};

static int HttpProxyDispatch(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
	while (result > 0)
	{
		p->active = GetTickCount();
		p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
		p->recv_msg.init();
		p->recv_msg.done = &HttpProxySendDone;

		const char *url = p->h_f_data(0);
		for (const struct HttpDispatch *d = HttpDispatchMap; d->url != NULL; ++d)
		{
			if ((_strnicmp(url, d->url, d->len) == 0)
			 && (unsigned(d->method) == p->recv_parser.method))
			{
				result = d->proc(p);
				goto _check;
			}
		}
		result = HttpGetFile(p, url+1);
_check:
		if (result <= 0)
			break;

		p->send_msg.init();
		p->send_msg.done = &HttpProxyDispatch;
		result = HttpClientDoRecvResponse(p, &p->send_msg);
	}
	if (result < 0)
		HttpProxyClose(p);
	return result;
}

static int HttpProxyOpen(AObject *object, AMessage *msg)
{
	HttpClient *p = to_http(object);
	int result = HttpClientAppendOutput(p, msg);
	if (result < 0)
		return result;

	AObjectAddRef(&p->object);
	p->active = GetTickCount();
	sm.push(p);

	p->send_msg.init();
	p->send_msg.done = &HttpProxyDispatch;
	result = HttpClientDoRecvResponse(p, &p->send_msg);
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

int HttpProxyInit(AOption *global_option, AOption *module_option)
{
	SessionInit(&sm);
	AOperatorTimewait(&sm.asop, NULL, sm.check_timer);
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
