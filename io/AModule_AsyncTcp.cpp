#include "stdafx.h"
#include "../base/AModule.h"
#include "iocp_util.h"
#include "../base/async_operator.h"


struct AsyncTcp;
struct AsyncOvlp {
	sysio_operator sysio;
	AsyncTcp   *tcp;
	AMessage   *msg;
	WSABUF      buf;
};
struct AsyncTcp {
	AObject   object;
	SOCKET    sock;
	AsyncOvlp send_ovlp;
	AsyncOvlp recv_ovlp;
};
#define to_tcp(obj)  container_of(obj, AsyncTcp, object)

static void AsyncTcpRelease(AObject *object)
{
	AsyncTcp *tcp = to_tcp(object);
	release_s(tcp->sock, closesocket, INVALID_SOCKET);

	free(tcp);
}

static long AsyncTcpCreate(AObject **object, AObject *parent, AOption *option)
{
	AsyncTcp *tcp = (AsyncTcp*)malloc(sizeof(AsyncTcp));
	if (tcp == NULL)
		return -ENOMEM;

	extern AModule AsyncTcpModule;
	AObjectInit(&tcp->object, &AsyncTcpModule);
	tcp->sock = INVALID_SOCKET;
	memset(&tcp->send_ovlp, 0, sizeof(tcp->send_ovlp));
	memset(&tcp->recv_ovlp, 0, sizeof(tcp->recv_ovlp));
	tcp->send_ovlp.tcp = tcp;
	tcp->recv_ovlp.tcp = tcp;

	*object = &tcp->object;
	return 1;
}
/*
static void WINAPI AsyncTcpDone(DWORD error, DWORD tx, OVERLAPPED *op)
{
	AsyncOvlp *ovlp = container_of(op, AsyncOvlp, ovlp);
	long result;

	if (ovlp->msg->type == AMsgType_Option) {
		result = iocp_is_connected(ovlp->tcp->sock);
		if (result != 0)
			result = -EIO;
		else
			result = 1;
	} else if (tx == 0) {
		result = -EIO;
	} else if (ovlp->msg->type & AMsgType_Custom) {
		ovlp->buf.buf += tx;
		ovlp->buf.len -= tx;

		if (ovlp->buf.len == 0) {
			result = ovlp->msg->size;
		} else {
			if (ovlp == &ovlp->tcp->send_ovlp)
				result = iocp_send(ovlp->tcp->sock, &ovlp->buf, 1, &ovlp->ovlp);
			else
				result = iocp_recv(ovlp->tcp->sock, &ovlp->buf, 1, &ovlp->ovlp);
			if (result == 0)
				return;
			result = -EIO;
		}
	} else {
		result = tx;
		ovlp->msg->size = tx;
	}
	result = ovlp->msg->done(ovlp->msg, result);
}
*/
static void AsyncTcpRequestDone(sysio_operator *sysop, int result)
{
	AsyncOvlp *ovlp = container_of(sysop, AsyncOvlp, sysio);
	if (result <= 0) {
		result = -EIO;
	} else if (ovlp->msg->type & AMsgType_Custom) {
		ovlp->buf.buf += result;
		ovlp->buf.len -= result;

		if (ovlp->buf.len == 0) {
			result = ovlp->msg->size;
		} else {
			if (ovlp == &ovlp->tcp->send_ovlp)
				result = iocp_send(ovlp->tcp->sock, &ovlp->buf, 1, &ovlp->sysio.ovlp);
			else
				result = iocp_recv(ovlp->tcp->sock, &ovlp->buf, 1, &ovlp->sysio.ovlp);
			if (result == 0)
				return;
			result = -EIO;
		}
	} else {
		ovlp->msg->size = result;
	}
	result = ovlp->msg->done(ovlp->msg, result);
}

static void AsyncTcpOpenDone(sysio_operator *sysop, int result)
{
	AsyncOvlp *ovlp = container_of(sysop, AsyncOvlp, sysio);
	if (result >= 0)
		result = iocp_is_connected(ovlp->tcp->sock);
	if (result != 0)
		result = -EIO;
	else
		result = 1;
	ovlp->tcp->send_ovlp.sysio.callback = &AsyncTcpRequestDone;
	ovlp->tcp->recv_ovlp.sysio.callback = &AsyncTcpRequestDone;
	ovlp->msg->done(ovlp->msg, result);
}

static long AsyncTcpOpen(AObject *object, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	if (msg->type == AMsgType_Object) {
		if (msg->size != sizeof(SOCKET))
			return -EINVAL;

		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		tcp->sock = (SOCKET)msg->data;
		if (tcp->sock != INVALID_SOCKET) {
			//BindIoCompletionCallback((HANDLE)tcp->sock, &AsyncTcpDone, 0);
			sysio_bind(NULL, (HANDLE)tcp->sock);
			tcp->send_ovlp.sysio.callback = &AsyncTcpRequestDone;
			tcp->recv_ovlp.sysio.callback = &AsyncTcpRequestDone;
		}
		return 1;
	}

	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *addr = AOptionFindChild(option, "address");
	if (addr == NULL)
		return -EINVAL;

	AOption *port = AOptionFindChild(option, "port");
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = iocp_getaddrinfo(addr->value, port?port->value:NULL);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr->value, port?port->value:"", WSAGetLastError());
		return -EFAULT;
	}

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(AF_INET, SOCK_STREAM, ai->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			release_s(ai, freeaddrinfo, NULL);
			return -EFAULT;
		}
	}
	//BindIoCompletionCallback((HANDLE)tcp->sock, &AsyncTcpDone, 0);
	sysio_bind(NULL, (HANDLE)tcp->sock);

	tcp->send_ovlp.msg = msg;
	tcp->send_ovlp.sysio.callback = &AsyncTcpOpenDone;
	long result = iocp_connect(tcp->sock, ai->ai_addr, ai->ai_addrlen, &tcp->send_ovlp.sysio.ovlp);
	release_s(ai, freeaddrinfo, NULL);

	if (result != 0) {
		result = -EIO;
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	}
	return result;
}

static long AsyncTcpRequest(AObject *object, long reqix, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	long result;

	if (reqix == ARequest_Input) {
		tcp->send_ovlp.msg = msg;
		tcp->send_ovlp.buf.buf = msg->data;
		tcp->send_ovlp.buf.len = msg->size;
		result = iocp_send(tcp->sock, &tcp->send_ovlp.buf, 1, &tcp->send_ovlp.sysio.ovlp);
	} else if (reqix == ARequest_Output) {
		tcp->recv_ovlp.msg = msg;
		tcp->recv_ovlp.buf.buf = msg->data;
		tcp->recv_ovlp.buf.len = msg->size;
		result = iocp_recv(tcp->sock, &tcp->recv_ovlp.buf, 1, &tcp->recv_ovlp.sysio.ovlp);
	} else {
		result = -ENOSYS;
	}
	if (result != 0)
		result = -EIO;
	return result;
}

static long AsyncTcpClose(AObject *object, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	if (msg == NULL) {
		if (tcp->sock != INVALID_SOCKET) {
			shutdown(tcp->sock, SD_BOTH);
			CancelIoEx((HANDLE)tcp->sock, NULL);
			return 1;
		}
		return -ENOENT;
	}
	release_s(tcp->sock, closesocket, INVALID_SOCKET);
	return 1;
}

static long AsyncTcpInit(AOption *option)
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);
	return 1;
}

AModule AsyncTcpModule = {
	"io",
	"async_tcp",
	sizeof(AsyncTcp),
	AsyncTcpInit, NULL,
	&AsyncTcpCreate,
	&AsyncTcpRelease,
	NULL,
	2,

	&AsyncTcpOpen,
	NULL,
	NULL,
	&AsyncTcpRequest,
	NULL,
	&AsyncTcpClose,
};