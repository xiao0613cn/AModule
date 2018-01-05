#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


struct TCPObject : public IOObject {
	SOCKET  sock;
};

static void TCPRelease(AObject *object)
{
	TCPObject *tcp = (TCPObject*)object;
	closesocket_s(tcp->sock);
}

static int TCPCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPObject *tcp = (TCPObject*)*object;
	tcp->sock = INVALID_SOCKET;
	return 1;
}

static int TCPOpen(AObject *object, AMessage *msg)
{
	TCPObject *tcp = (TCPObject*)object;
	if (msg->type == AMsgType_Handle) {
		if (msg->size != 0)
			return -EINVAL;

		closesocket_s(tcp->sock);
		tcp->sock = (SOCKET)msg->data;
		return 1;
	}

	if ((msg->type != AMsgType_AOption)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	const char *addr = option->getStr("address", NULL);
	if (addr == NULL)
		return -EINVAL;

	const char *port = option->getStr("port", NULL);
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = net_getaddrinfo(addr, port);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr, port, errno);
		return -EIO;
	}
	godefer(addrinfo*, ai, freeaddrinfo(ai));

	struct addrinfo *ai_valid = ai;
	do {
		if (ai_valid->ai_protocol != IPPROTO_UDP)
			break;
	} while ((ai_valid = ai_valid->ai_next) != NULL);
	if (ai_valid == NULL) {
		TRACE("invalid address: %s:%s, ai_protocol = %d.\n",
			addr, port, ai->ai_protocol);
		return -EINVAL;
	}

	if (tcp->sock == INVALID_SOCKET) {
		tcp->sock = socket(ai_valid->ai_family, SOCK_STREAM, ai_valid->ai_protocol);
		if (tcp->sock == INVALID_SOCKET) {
			return -EIO;
		}
	}

	int timeout = option->getInt("timeout", 20);
	int result = tcp_connect(tcp->sock, ai_valid->ai_addr, ai_valid->ai_addrlen, timeout);

	if (result < 0) {
		closesocket_s(tcp->sock);
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
	TCPObject *tcp = (TCPObject*)object;
	if (strcasecmp(option->name, "socket") == 0) {
		closesocket_s(tcp->sock);
		tcp->sock = (SOCKET)option->extend;
		return 1;
	}
	return -ENOSYS;
}

static int TCPRequest(AObject *object, int reqix, AMessage *msg)
{
	TCPObject *tcp = (TCPObject*)object;
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
	TCPObject *tcp = (TCPObject*)object;
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
	TCPObject *tcp = (TCPObject*)object;
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (msg == NULL) {
		shutdown(tcp->sock, SD_BOTH);
	} else {
		closesocket_s(tcp->sock);
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

static int TCPSvcAccept(AObject *object, AMessage *msg, AObject *svc_data, AOption *svc_opt)
{
	TCPObject *tcp = (TCPObject*)object;
	if (msg->type != AMsgType_Handle)
		return -EINVAL;

	closesocket_s(tcp->sock);
	tcp->sock = (SOCKET)msg->data;
	return 1;
}

IOModule TCPModule = { {
	"io",
	"tcp",
	sizeof(TCPObject),
	&TCPInit, NULL,
	&TCPCreate,
	&TCPRelease, },
	&TCPOpen,
	&TCPSetOption,
	&IOModule::OptNull,
	&TCPRequest,
	&TcpCancel,
	&TCPClose,

	NULL, &TCPSvcAccept,
};

static int reg_code = AModuleRegister(&TCPModule.module);
