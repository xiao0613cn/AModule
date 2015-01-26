#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/iocp_util.h"
#include "../io/AModule_TCP.h"


struct TCPServer {
	AObject  object;
	SOCKET   sock;
	HANDLE   listen_thread;
	AOption *proxy_option;
};
#define to_tcpsvr(obj) container_of(obj, TCPServer, object)


static long TCPClientFindProxy(void *p, AModule *module)
{
	TCPObject *tcp = to_tcp(p);
	if (module->probe == NULL)
		return -1;

	if (_stricmp(module->class_name,"proxy") != 0)
		return -1;

	return module->probe(&tcp->object, tcp->recvbuf, tcp->recvsiz);
}

static DWORD WINAPI TCPClientProcess(void *p)
{
	SOCKET sock = (SOCKET)p;

	AObject *io = NULL;
	long result = TCPModule.create(&io, NULL, NULL);
	if (result < 0) {
		closesocket(sock);
		return result;
	}

	TCPObject *tcp = to_tcp(io);
	tcp->sock = sock;
	result = recv(sock, tcp->recvbuf, sizeof(tcp->recvbuf), 0);
	if (result <= 0) {
		AObjectRelease(&tcp->object);
		return -EIO;
	}

	tcp->recvsiz = result;
	tcp->peekpos = 0;
	AModule *module = AModuleEnum(TCPClientFindProxy, &tcp->object);
	if (module == NULL) {
		AObjectRelease(&tcp->object);
		return -EFAULT;
	}

	AObject *proxy = NULL;
	result = module->create(&proxy, &tcp->object, NULL);
	if (result >= 0) {
		result = proxy->open(proxy, NULL);
		AObjectRelease(proxy);
	}
	AObjectRelease(&tcp->object);
	return result;
}

static DWORD WINAPI TCPServerProcess(void *p)
{
	TCPServer *svr = to_tcpsvr(p);
	struct sockaddr addr;
	int addrlen;
	SOCKET sock;

	do {
		memset(&addr, 0, sizeof(addr));
		addrlen = sizeof(addr);

		sock = accept(svr->sock, &addr, &addrlen);
		if (sock == INVALID_SOCKET)
			break;

		QueueUserWorkItem(&TCPClientProcess, (void*)sock, 0);
	} while (1);
	AObjectRelease(&svr->object);
	return 0;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *svr = to_tcpsvr(object);
	release_s(svr->sock, closesocket, INVALID_SOCKET);
	release_s(svr->listen_thread, CloseHandle, NULL);
	release_s(svr->proxy_option, AOptionRelease, NULL);
	free(svr);
}

static long TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *svr = (TCPServer*)malloc(sizeof(TCPServer));
	if (svr == NULL)
		return -ENOMEM;

	extern AModule TCPServerModule;
	AObjectInit(&svr->object, &TCPServerModule);
	svr->sock = INVALID_SOCKET;
	svr->listen_thread = NULL;
	svr->proxy_option = NULL;

	*object = &svr->object;
	return 1;
}

static long TCPServerOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *port_opt = AOptionFindChild(option, "port");
	if (port_opt == NULL)
		return -EINVAL;

	TCPServer *svr = to_tcpsvr(object);
	svr->sock = bind_socket(IPPROTO_TCP, (u_short)atol(port_opt->value));
	if (svr->sock == INVALID_SOCKET)
		return -EINVAL;

	svr->proxy_option = AOptionFindChild(option, "proxy");
	if (svr->proxy_option != NULL)
		svr->proxy_option = AOptionClone(svr->proxy_option);

	AObjectAddRef(&svr->object);
	QueueUserWorkItem(&TCPServerProcess, &svr->object, 0);
	return 1;
}

static long TCPServerClose(AObject *object, AMessage *msg)
{
	TCPServer *svr = to_tcpsvr(object);
	release_s(svr->sock, closesocket, INVALID_SOCKET);
	return 1;
}

AModule TCPServerModule = {
	"server",
	"tcp_server",
	sizeof(TCPServer),
	NULL, NULL,
	&TCPServerCreate,
	&TCPServerRelease,
	NULL,
	0,

	&TCPServerOpen,
	NULL,
	NULL,
	NULL,
	NULL,
	&TCPServerClose,
};
