#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


struct TCPObject {
	AObject object;
	SOCKET  sock;
};
#define to_tcp(obj) container_of(obj, TCPObject, object);

static void TCPRelease(AObject *object)
{
	TCPObject *tcp = to_tcp(object);
	release_s(tcp->sock, closesocket, INVALID_SOCKET);
}

static int TCPCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPObject *tcp = (TCPObject*)*object;
	tcp->sock = INVALID_SOCKET;
	return 1;
}

static int TCPOpen(AObject *object, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	if (msg->type == AMsgType_Handle) {
		if (msg->size != 0)
			return -EINVAL;

		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		tcp->sock = (SOCKET)msg->data;
		return 1;
	}

	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *addr = AOptionFind(option, "address");
	if (addr == NULL)
		return -EINVAL;

	AOption *port = AOptionFind(option, "port");
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = tcp_getaddrinfo(addr->value, port?port->value:NULL);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr->value, port?port->value:"", errno);
		return -EIO;
	}

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(ai->ai_family, SOCK_STREAM, ai->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			release_s(ai, freeaddrinfo, NULL);
			return -EIO;
		}
	}

	AOption *timeout = AOptionFind(option, "timeout");
	int result = tcp_connect(tcp->sock, ai->ai_addr, ai->ai_addrlen, (timeout?atol(timeout->value):20));
	release_s(ai, freeaddrinfo, NULL);

	if (result < 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	} else {
		result = 1;
	}
	return result;
}

static int TCPSetOption(AObject *object, AOption *option)
{
	TCPObject *tcp = to_tcp(object);
	if (_stricmp(option->name, "socket") == 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		tcp->sock = (SOCKET)option->extend;
		return 1;
	}
	return -ENOSYS;
}

static int TCPRequest(AObject *object, int reqix, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	int result;

	assert(msg->size != 0);
	switch (reqix)
	{
	case Aio_Input:
		if (ioMsgType_isBlock(msg->type))
			return tcp_send(tcp->sock, msg->data, msg->size, MSG_NOSIGNAL);

		result = send(tcp->sock, msg->data, msg->size, MSG_NOSIGNAL);
		break;

	case Aio_Output:
		if (ioMsgType_isBlock(msg->type))
			return tcp_recv(tcp->sock, msg->data, msg->size, 0);

		result = recv(tcp->sock, msg->data, msg->size, 0);
		break;

	default:
		assert(FALSE);
		return -ENOSYS;
	}

	if (result <= 0)
		result = -EIO;
	else
		msg->size = result;
	return result;
}

static int TcpCancel(AObject *object, int reqix, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (reqix == Aio_Input) {
		shutdown(tcp->sock, SD_SEND);
	} else if (reqix == Aio_Output) {
		shutdown(tcp->sock, SD_RECEIVE);
	} else {
		assert(FALSE);
		return -ENOSYS;
	}
	return 1;
}

static int TCPClose(AObject *object, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (msg == NULL) {
		shutdown(tcp->sock, SD_BOTH);
	} else {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	}
	return 1;
}

static int TCPInit(AOption *option)
{
#ifdef _WIN32
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);
#endif
	return 1;
}

AModule TCPModule = {
	"io",
	"tcp",
	sizeof(TCPObject),
	&TCPInit, NULL,
	&TCPCreate,
	&TCPRelease,
	NULL,
	2,

	&TCPOpen,
	&TCPSetOption,
	NULL,
	&TCPRequest,
	&TcpCancel,
	&TCPClose,
};

//////////////////////////////////////////////////////////////////////////
