#include "stdafx.h"
#include "../base/AModule.h"
#include "iocp_util.h"


struct TCPObject {
	AObject object;
	SOCKET  sock;
};
#define to_tcp(obj) CONTAINING_RECORD(obj, TCPObject, object);

static void TCPRelease(AObject *object)
{
	TCPObject *tcp = to_tcp(object);
	release_s(tcp->sock, closesocket, INVALID_SOCKET);

	free(tcp);
}

static long TCPCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPObject *tcp = (TCPObject*)malloc(sizeof(TCPObject));
	if (tcp == NULL)
		return -ENOMEM;

	extern AModule TCPModule;
	AObjectInit(&tcp->object, &TCPModule);
	tcp->sock = INVALID_SOCKET;

	*object = &tcp->object;
	return 1;
}

static long TCPOpen(AObject *object, AMessage *msg)
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
	AOption *addr = AOptionFindChild(option, "address");
	if (addr == NULL)
		return -EINVAL;

	AOption *port = AOptionFindChild(option, "port");
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = iocp_getaddrinfo(addr->value, port?port->value:NULL);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr->value, port?port->value:"", WSAGetLastError());
		return -EIO;
	}

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(ai->ai_family, SOCK_STREAM, ai->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			release_s(ai, freeaddrinfo, NULL);
			return -EIO;
		}
	}

	AOption *timeout = AOptionFindChild(option, "timeout");
	long result = tcp_connect(tcp->sock, ai->ai_addr, ai->ai_addrlen, (timeout?atol(timeout->value):20));
	release_s(ai, freeaddrinfo, NULL);

	if (result != 0) {
		result = -EIO;
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	} else {
		result = 1;
	}
	return result;
}

static long TCPSetOption(AObject *object, AOption *option)
{
	TCPObject *tcp = to_tcp(object);
	if (_stricmp(option->name, "socket") == 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		tcp->sock = (SOCKET)option->extend;
		return 1;
	}
	return -ENOSYS;
}

static long TCPRequest(AObject *object, long reqix, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	long result;

	assert(msg->size != 0);
	switch (reqix)
	{
	case ARequest_Input:
		if (msg->type & AMsgType_Custom)
			result = tcp_send(tcp->sock, msg->data, msg->size, 0);
		else
			result = send(tcp->sock, msg->data, msg->size, 0);
		break;
	case ARequest_Output:
		if (msg->type & AMsgType_Custom)
			result = tcp_recv(tcp->sock, msg->data, msg->size, 0);
		else
			result = recv(tcp->sock, msg->data, msg->size, 0);
		break;
	default:
		return -ENOSYS;
	}

	if (result <= 0)
		result = -EIO;
	else
		msg->size = result;
	return result;
}

static long TcpCancel(AObject *object, long reqix, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (reqix == ARequest_Input) {
		shutdown(tcp->sock, SD_SEND);
	} else if (reqix == ARequest_Output) {
		shutdown(tcp->sock, SD_RECEIVE);
	} else {
		return -ENOSYS;
	}
	return 1;
}

static long TCPClose(AObject *object, AMessage *msg)
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

static long TCPInit(AOption *option)
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);
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
