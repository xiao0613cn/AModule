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
		tcp->sock = (SOCKET)(long)msg->data;
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

	struct addrinfo *ai_valid = ai;
	do {
		if (ai_valid->ai_protocol != IPPROTO_UDP)
			break;
	} while ((ai_valid = ai_valid->ai_next) != NULL);
	if (ai_valid == NULL) {
		TRACE("invalid address: %s, ai_protocol = %d.\n", addr->value, ai->ai_protocol);
		release_s(ai, freeaddrinfo, NULL);
		return -EINVAL;
	}

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(ai_valid->ai_family, SOCK_STREAM, ai_valid->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			release_s(ai, freeaddrinfo, NULL);
			return -EIO;
		}
	}

	int timeout = AOptionGetInt(option, "timeout", 20);
	int result = tcp_connect(tcp->sock, ai_valid->ai_addr, ai_valid->ai_addrlen, timeout);
	release_s(ai, freeaddrinfo, NULL);

	if (result < 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
	} else {
#ifdef _WIN32
		int tv = timeout*1000;
#else
		struct timeval tv = { timeout, 0 };
#endif
		setsockopt(tcp->sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
		setsockopt(tcp->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		result = 1;
	}
	return result;
}

static int TCPSetOption(AObject *object, AOption *option)
{
	TCPObject *tcp = to_tcp(object);
	if (strcasecmp(option->name, "socket") == 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		tcp->sock = (SOCKET)(long)option->extend;
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

static int TCPInit(AOption *global_option, AOption *module_option, BOOL first)
{
#ifdef _WIN32
	if (first) {
		WSADATA wsadata;
		WSAStartup(WINSOCK_VERSION, &wsadata);
	}
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

static auto_reg_t<TCPModule> auto_reg;

//////////////////////////////////////////////////////////////////////////
