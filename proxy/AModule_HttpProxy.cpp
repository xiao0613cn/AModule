#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"


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

	p->recv_msg.init();
	p->recv_msg.done = &HttpProxySendDone;

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

	p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
	append_data("HTTP/%d.%d %d xxx\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor, status_code);

	p->send_status = s_send_private_header;
	int result = HttpClientDoSendRequest(p, &p->recv_msg);
	return result;
}

static int HttpProxyDispatch(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
	while (result > 0)
	{
		p->active = GetTickCount();

		const char *url = p->h_f_data(0);
		if (_strnicmp_c(url, "/rpc_user/") == 0) {
			//result = HttpRpcUser(p, url+sizeof("/rpc_user/"));
		} else if (_strnicmp_c(url, "/rpc_chan/") == 0) {
			//result = HttpRpcChan(p, url+sizeof("/rpc_user/"));
		} else {
			result = HttpGetFile(p, url+1);
		}
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

AModule HTTPProxyModule = {
	"proxy",
	"HttpProxy",
	sizeof(HttpClient),
	NULL, NULL,
	&HttpClientCreate,
	&HttpClientRelease,
	&HttpProxyProbe,
	0,
	&HttpProxyOpen,
	NULL, NULL,
	NULL, NULL,
	NULL,
};
