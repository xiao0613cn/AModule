#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"


static int HttpProxyClose(HttpClient *p, int result)
{
	release_s(p->io, AObjectRelease, NULL);

	if (p->session != NULL) {
		HttpSession *s = to_sess(p->session);

		s->lock();
		list_del_init(&p->conn_entry);
		s->unlock();

		AObjectRelease(p->session);
		p->session = NULL;
	}
	AObjectRelease(&p->object);
	return result;
}

static int HttpProxySendDone(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, send_msg);
}

static int HttpGetFile(HttpClient *p)
{
	int status_code = 200;
	long file_size = 0;

	FILE *fp = fopen(p->h_f_data(0), "rb");
	if (fp != NULL) {
		status_code = 404;
	} else {
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	ARefsBuf *buf = NULL;
	if (file_size > 0) {
		if (ARefsBufCheck(buf, file_size+sizeof(ARefsMsg)) < 0)
			status_code = 501;
		else {
			buf->push(sizeof(ARefsMsg));
			buf->pop(sizeof(ARefsMsg));
		}
	} else {
		status_code = 404;
	}
	if (status_code == 200) {
		if (fread(buf->next(), file_size, 1, fp) != file_size)
			status_code = 502;
		else
			buf->push(file_size);
	}

	if (status_code == 200) {
		ARefsMsg *rm = (ARefsMsg*)buf->data;
		rm->buf = NULL;
		ARefsMsgInit(rm, ioMsgType_Block, buf, buf->bgn, file_size);
	}
	release_s(fp, fclose, NULL);

	p->send_msg.init(ioMsgType_Block, p->send_buffer);
	append_data("HTTP/%d.%d %d xxx\r\n"
		"Content-Length: %ld\r\n"
		"\r\n",
		p->recv_parser.http_major, p->recv_parser.http_minor, status_code,
		file_size);

	p->send_msg.done = &HttpProxySendDone;
	int result = p->io->request(p->io, Aio_Input, &p->send_msg);
	if (result > 0)
		HttpProxySendDone(&p->send_msg, result);
	return result;
}

static int HttpProxyDispatch(HttpClient *p, int result)
{
	const char *url = p->h_f_data(0);
	if (_strnicmp_c(url, "/rpc_user/") == 0) {
		//return HttpRpcUser(p, url+sizeof("/rpc_user/")-1);
	}
	if (_strnicmp_c(url, "/rpc_chan/") == 0) {
		//return HttpRpcChan(p, url+sizeof("/rpc_user/")-1);
	}
	return HttpGetFile(p);
}

static int HttpProxyOnRecv(AMessage *msg, int result)
{
	HttpClient *p = container_of(msg, HttpClient, recv_msg);
	if (result > 0) {
		result = HttpClientOnRecvStatus(p, result);
	}
	while (result > 0) {
		result = HttpProxyDispatch(p, result);
		if (result <= 0)
			break;

		result = HttpClientDoRecvResponse(p, &p->recv_msg);
	}
	if (result < 0) {
		result = HttpProxyClose(p, result);
	}
	return result;
}

static int HttpProxyOpen(AObject *object, AMessage *msg)
{
	HttpClient *p = to_http(object);

	int result = HttpClientAppendOutput(p, msg);
	if (result < 0)
		return result;

	p->recv_msg.done = &HttpProxyOnRecv;
	do {
		result = HttpClientDoRecvResponse(p, &p->recv_msg);
		if (result <= 0)
			break;

		result = HttpProxyDispatch(p, result);
	} while (result > 0);

	if (result < 0) {
		result = HttpProxyClose(p, result);
	}
	return -EBUSY;
}

static int HttpProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->size < 10)
		return -1;

	for (int ix = 0; ix < HTTP_TRACE; ++ix) {
		if ((_strnicmp(msg->data,HTTPRequests[ix].method,HTTPRequests[ix].length) == 0)
		 && (msg->data[HTTPRequests[ix].length] == ' '))
			return 60;
	}
	return -1;
}

AModule HttpProxyModule = {
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
