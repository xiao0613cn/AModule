#include "stdafx.h"
#ifdef _WIN32
#ifndef _MSWSOCK_
#include <MSWSock.h>
#endif
#pragma comment(lib, "ws2_32.lib")
#endif
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/AInOutComponent.h"

struct TCPClient;

struct TCPServer : public AObject {
	SOCKET     sock;
	AOption   *option;
	AOption   *services;
	TCPClient *prepare;

	u_short    port;
	int        family;
	AOption   *io_option;
	AModule   *io_module;
	BOOL       is_async;
	int        min_probe_size;
	int        max_probe_size;
#ifdef _WIN32
	LPFN_ACCEPTEX acceptex;
	AOperator  sysio;
#endif
};

static inline AService**
m_ptr(AOption *m_opt) {
	return (AService**)(m_opt->value + sizeof(m_opt->value) - sizeof(AService*));
}

enum TCPStatus {
	tcp_io_accept = 0,
	tcp_io_attach,
	tcp_recv_data,
	tcp_probe_service,
};
struct TCPClient {
	union {
	AOperator  sysop;
	AMessage   msg;
	};
	TCPServer *server;
	TCPStatus  status;
	SOCKET     sock;
	IOObject  *io;

	ARefsBuf*  tobuf() { return container_of(this, ARefsBuf, _data); }
};

static void TCPClientRelease(TCPClient *client)
{
	client->server->release();
	closesocket_s(client->sock);
	release_s(client->io);
	client->tobuf()->release();
}

static int TCPClientInmsgDone(AMessage *msg, int result)
{
	TCPClient *client = container_of(msg, TCPClient, msg);
	TCPServer *server = client->server;
	ARefsBuf  *buf = client->tobuf();

	while (result > 0)
	{
	switch (client->status)
	{
	case tcp_io_accept:
		result = AObject::create2(&client->io, server, server->io_option, server->io_module);
		if (result < 0)
			break;

		client->status = tcp_io_attach;
		client->msg.init(AMsgType_Handle, (void*)client->sock, 0);
		result = client->io->open(&client->msg);
		break;

	case tcp_io_attach:
		client->sock = INVALID_SOCKET;
		client->status = tcp_probe_service;
		if (buf->len() == 0) {
			client->msg.init(0, buf->next(), buf->left());
			result = client->io->output(&client->msg);
			break;
		}
		client->msg.init();

	case tcp_probe_service:
		buf->push(client->msg.size);
		client->msg.init(0, buf->ptr(), buf->len());
	{
		AService *service = NULL;
		AOption *option = NULL;
		int score = -1;
		list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
		{
			AService *svc = *m_ptr(m_opt);
			if (svc == NULL)
				continue;

			if (svc->module.probe != NULL)
				result = svc->module.probe(client->io, &client->msg, m_opt);
			else
				result = 1;
			if (result > score) {
				service = svc;
				option = m_opt;
				score = result;
			}
		}
		if (service == NULL) {
			if (client->msg.size < server->min_probe_size) {
				//TRACE("retry probe: %d, %.*s.\n", client->msg.size,
				//	client->msg.size, client->msg.data);
				client->msg.init(0, buf->next(), buf->left());
				result = client->io->output(&client->msg);
				break;
			}
			TRACE("no found service: %d, %.*s.\n", client->msg.size,
				client->msg.size, client->msg.data);
			result = -EINVAL;
			break;
		}

		AEntity *e = NULL;
		result = AObject::create2(&e, server, option, &service->module);
		if (result < 0)
			break;

		AInOutComponent *c; e->_get(&c);
		assert(c != NULL);
		release_s(c->_io); c->_io = client->io; client->io = NULL;
		release_s(c->_outbuf); c->_outbuf = buf;
		release_s(client->server);

		service->svc_run(e, option);
		e->release();
		return 1;
	}
	default: assert(0); result = -EACCES; break;
	}
	}
	if (result < 0) {
		TRACE("tcp error = %d.\n", result);
		TCPClientRelease(client);
	}
	return result;
}

static int TCPClientProcess(AOperator *asop, int result)
{
	TCPClient *client = container_of(asop, TCPClient, sysop);
	client->msg.done = &TCPClientInmsgDone;
	return client->msg.done2((result>=0) ? 1 : result);
}

static TCPClient* TCPClientCreate(TCPServer *server)
{
	ARefsBuf *buf = ARefsBuf::create(sizeof(TCPClient) + server->max_probe_size);
	if (buf == NULL)
		return NULL;

	TCPClient *client = (TCPClient*)buf->ptr();
	buf->push(sizeof(TCPClient));
	buf->pop(sizeof(TCPClient));

	client->sysop.done = &TCPClientProcess;
	client->server = server; server->addref();
	client->status = tcp_io_accept;
	client->sock = INVALID_SOCKET;
	client->io = NULL;
	return client;
}

static void* TCPServerProcess(void *p)
{
	TCPServer *server = (TCPServer*)p;
	struct sockaddr addr;
	socklen_t addrlen;
	SOCKET sock;

	for (;;) {
		memset(&addr, 0, sizeof(addr));
		addrlen = sizeof(addr);

		sock = accept(server->sock, &addr, &addrlen);
		if (sock == INVALID_SOCKET) {
			TRACE("accept(%d) failed, errno = %d.\n", server->port, errno);

			if ((errno == EINTR) || (errno == EAGAIN))
				continue;
			if ((errno == ENFILE) || (errno == EMFILE)) {
				Sleep(1000);
				continue;
			}
			break;
		}

		TCPClient *client = TCPClientCreate(server);
		if (client != NULL) {
			client->sock = sock;
			client->sysop.post(NULL);
		} else {
			//ENOMEM
			closesocket(sock);
		}
	}
	TRACE("%p: quit.\n", server);
	server->release();
	return 0;
}

#ifdef _WIN32
static int TCPServerDoAcceptEx(TCPServer *server)
{
	server->prepare = TCPClientCreate(server);
	if (server->prepare == NULL)
		return -ENOMEM;

	server->prepare->sock = socket(server->family, SOCK_STREAM, IPPROTO_TCP);
	if (server->prepare->sock == INVALID_SOCKET)
		return -EFAULT;

	int result = sizeof(struct sockaddr_in)+16;
	if (server->family == AF_INET6)
		result = sizeof(struct sockaddr_in6)+16;

	DWORD tx = 0;
	ARefsBuf *buf = server->prepare->tobuf();
	result = server->acceptex(server->sock, server->prepare->sock,
		buf->next(), buf->left()-result*2,
		result, result, &tx, &server->sysio.ao_ovlp);
	if (!result) {
		if (WSAGetLastError() != ERROR_IO_PENDING)
			return -EIO;
	}
	return 0;
}

static int TCPServerAcceptExDone(AOperator *sysop, int result)
{
	TCPServer *server = container_of(sysop, TCPServer, sysio);
	if (result > 0) {
		server->prepare->tobuf()->push(result);
		result = setsockopt(server->prepare->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		                    (const char *)&server->sock, sizeof(server->sock));
	}
	if (result >= 0) {
		server->prepare->sysop.post(NULL);
		server->prepare = NULL;
	} else {
		result = -WSAGetLastError();
		TRACE("%p: AcceptEx() = %d.\n", server, result);
		if_not(server->prepare, NULL, TCPClientRelease);
	}

	result = TCPServerDoAcceptEx(server);
	if (result < 0)
		server->release();
	return result;
}
#endif

static int TCPServerRun(AObject *object, AOption *option)
{
	TCPServer *server = (TCPServer*)object;

	server->option = AOptionClone(option, NULL);
	if (server->option == NULL)
		return -ENOMEM;

	server->port = (u_short)server->option->getI64("port", 0);
	if (server->port == 0)
		return -EINVAL;

	const char *af = server->option->getStr("family", NULL);
	if ((af != NULL) && (strcasecmp(af, "inet6") == 0))
		server->family = AF_INET6;
	else
		server->family = AF_INET;

	server->io_option = server->option->find("io");
	if ((server->io_option == NULL) || (server->io_option->value[0] == '\0'))
		return -EINVAL;

	server->io_module = AModuleFind("io", server->io_option->value);
	if (server->io_module == NULL)
		return -ENOENT;

	server->services = server->option->find("services");
	if ((server->services == NULL) || server->services->children_list.empty())
		return -EINVAL;

	server->sock = tcp_bind(server->family, IPPROTO_TCP, server->port);
	if (server->sock == INVALID_SOCKET)
		return -EINVAL;

	int backlog = server->option->getInt("backlog", 8);
	int result = listen(server->sock, backlog);
	if (result != 0)
		return -EIO;

	list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
	{
		AService *svc = NULL;
		if (m_opt->value[0] == '\0')
			svc = (AService*)AModuleFind(NULL, m_opt->name);
		else
			svc = (AService*)AModuleFind(m_opt->name, m_opt->value);
		*m_ptr(m_opt) = svc;
		if ((svc != NULL) && (svc->svr_init != NULL)) {
			svc->svr_init(server, m_opt);
		}
	}

	server->min_probe_size = server->option->getInt("min_probe_size", 128);
	server->max_probe_size = server->option->getInt("max_probe_size", 1800);
	server->is_async = server->option->getInt("is_async", TRUE);

	server->addref();
	int background = server->option->getInt("background", TRUE);
	if (!background) {
		TCPServerProcess(server);
		return 1;
	}
#ifndef _WIN32
	pthread_t thread = pthread_null;
	pthread_create(&thread, NULL, &TCPServerProcess, server);
	pthread_detach(thread);
	return 0;
#else
	GUID ax_guid = WSAID_ACCEPTEX;
	DWORD tx = 0;
	result = WSAIoctl(server->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &ax_guid, sizeof(ax_guid),
	                  &server->acceptex, sizeof(server->acceptex), &tx, NULL, NULL);
	if (result != 0) {
		server->release();
		return -EIO;
	}

	result = AThreadBind(NULL, (HANDLE)server->sock);
	memset(&server->sysio, 0, sizeof(server->sysio));
	server->sysio.done = &TCPServerAcceptExDone;

	if_not(server->prepare, NULL, TCPClientRelease);
	result = TCPServerDoAcceptEx(server);
	if (result < 0)
		server->release();
	return result;
#endif
}

static int TCPServerAbort(AObject *object)
{
	TCPServer *server = (TCPServer*)object;
	if (server->sock != INVALID_SOCKET)
		shutdown(server->sock, SD_BOTH);
	return 1;
}

static int TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *server = (TCPServer*)*object;
	server->sock = INVALID_SOCKET;
	server->option = NULL;
	server->services = NULL;
	server->prepare = NULL;
	return 1;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *server = (TCPServer*)object;
	closesocket_s(server->sock);
	if_not2(server->services, NULL,
		list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
		{
			AService *svc = *m_ptr(m_opt);
			if ((svc != NULL) && (svc->svr_exit != NULL)) {
				svc->svr_exit(server, m_opt);
			}
		});
	release_s(server->option);
	if_not(server->prepare, NULL, TCPClientRelease);
}

AService TCPServerModule = { {
	"AService",
	"tcp_server",
	sizeof(TCPServer),
	NULL, NULL,
	&TCPServerCreate,
	&TCPServerRelease,
	NULL, },
	NULL, NULL,
	&TCPServerRun,
	&TCPServerAbort,
};

static auto_reg_t reg(TCPServerModule.module);
