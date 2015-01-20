#include "stdafx.h"
#include "../base/AModule.h"
#include "iocp_util.h"


struct TCPObject {
	AObject object;
	SOCKET  sock;
};
#define to_tcp(object) CONTAINING_RECORD(object, TCPObject, object);

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
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *addr = AOptionFindChild(option, "address");
	if (addr == NULL)
		return -EINVAL;

	AOption *port = AOptionFindChild(option, "port");
	if (port == NULL)
		return -EINVAL;

	TCPObject *tcp = to_tcp(object);
	struct addrinfo *ai = iocp_getaddrinfo(addr->value, port->value);
	if (ai == NULL)
		return -EFAULT;

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(AF_INET, SOCK_STREAM, ai->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			release_s(ai, freeaddrinfo, NULL);
			return -EFAULT;
		}
	}

	AOption *timeout = AOptionFindChild(option, "timeout");
	long result = tcp_connect(tcp->sock, ai->ai_addr, ai->ai_addrlen, (timeout?atol(timeout->value):20));
	release_s(ai, freeaddrinfo, NULL);

	if (result != 0) {
		result = -EFAULT;
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	} else {
		result = 1;
	}
	return result;
}

static long TCPRequest(AObject *object, long reqix, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);

	long result;
	switch (reqix)
	{
	case ARequest_Input:
		if (msg->type == AMsgType_Unknown)
			result = send(tcp->sock, msg->data, msg->size, 0);
		else
			result = tcp_send(tcp->sock, msg->data, msg->size, 0);
		if (result <= 0)
			result = -EIO;
		break;

	case ARequest_Output:
		if (msg->type == AMsgType_Unknown)
			result = recv(tcp->sock, msg->data, msg->size, 0);
		else
			result = tcp_recv(tcp->sock, msg->data, msg->size, 0);
		if (result <= 0)
			result = -EIO;
		break;

	default:
		result = -ENOSYS;
		break;
	}
	return result;
}

static long TCPClose(AObject *object, AMessage *msg)
{
	TCPObject *tcp = to_tcp(object);
	if (msg == NULL) {
		if (tcp->sock != INVALID_SOCKET) {
			shutdown(tcp->sock, SD_BOTH);
			return 1;
		}
		return -ENOENT;
	}
	release_s(tcp->sock, closesocket, INVALID_SOCKET);
	return 1;
}

static long TCPInit(void)
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);
	return 1;
}

AModule TCPModule = {
	"io",
	"tcp",
	sizeof(TCPObject),
	TCPInit, NULL,
	&TCPCreate, &TCPRelease,
	2,

	&TCPOpen,
	NULL,
	NULL,
	&TCPRequest,
	NULL,
	&TCPClose,
};

//////////////////////////////////////////////////////////////////////////
