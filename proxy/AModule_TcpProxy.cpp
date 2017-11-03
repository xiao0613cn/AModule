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

struct TCPServer : public AService {
	SOCKET     sock;
	AOption   *option;
	AOption   *services;
	TCPClient *prepare;
	AObject   *io_svc_data;

	u_short    port;
	int        family;
	BOOL       is_async;
	int        min_probe_size;
	int        max_probe_size;
#ifdef _WIN32
	LPFN_ACCEPTEX acceptex;
	AOperator  sysio;
#endif
	IOModule* io() { return (IOModule*)peer_module; }
};

static inline AService*&
m_ptr(AOption *m_opt) {
	return *(AService**)(m_opt->value + sizeof(m_opt->value) - sizeof(AService*));
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
	void release() {
		release_s(server);
		closesocket_s(sock);
		release_s(io);
		tobuf()->release();
	}
};

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
		result = AObject::create2(&client->io, server->io_svc_data, server->peer_option, server->peer_module);
		if (result < 0)
			break;

		client->status = tcp_io_attach;
		msg->init(AMsgType_Handle, (void*)client->sock, 0);
		result = server->io()->svc_accept(client->io, msg, server->io_svc_data, server->peer_option);
		break;

	case tcp_io_attach:
		client->sock = INVALID_SOCKET;
		client->status = tcp_probe_service;
		if (buf->len() == 0) {
			msg->init(0, buf->next(), buf->left()-1);
			result = client->io->output(msg);
			break;
		}
		msg->init();

	case tcp_probe_service:
		buf->push(msg->size);
		*buf->next() = '\0';
		msg->init(0, buf->ptr(), buf->len());
	{
		AService *service = NULL;
		AOption *option = NULL;
		int score = -1;
		list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
		{
			AService *&svc = m_ptr(m_opt);
			if (svc == NULL)
				continue;

			if (svc->_module->probe != NULL)
				result = svc->_module->probe(client->io, msg, m_opt);
			else if (svc->peer_module->probe != NULL)
				result = svc->peer_module->probe(client->io, msg, m_opt);
			else
				result = 0;
			if (result > score) {
				service = svc;
				option = m_opt;
				score = result;
			}
		}
		if (service == NULL) {
			if (msg->size < server->min_probe_size) {
				msg->init(0, buf->next(), buf->left()-1);
				result = client->io->output(msg);
				break;
			}
			TRACE("no found service: %d, %.*s.\n", msg->size,
				msg->size, msg->data);
			result = -EINVAL;
			break;
		}

		AEntity *e = NULL;
		result = AObject::create2(&e, service, option, service->peer_module);
		if (result < 0)
			break;

		AInOutComponent *c; e->_get(&c);
		assert(c != NULL);
		release_s(c->_io); c->_io = client->io; client->io = NULL;
		release_s(c->_outbuf); c->_outbuf = buf;
		release_s(client->server);

		service->run(service, e, option);
		e->release();
		return 1;
	}
	default: assert(0); result = -EACCES; break;
	}
	}
	if (result < 0) {
		TRACE("tcp error = %d.\n", result);
		release_s(client);
	}
	return result;
}

static int TCPClientProcess(AOperator *asop, int result)
{
	TCPClient *client = container_of(asop, TCPClient, sysop);
	client->msg.done = &TCPClientInmsgDone;
	return client->msg.done2((result>=0) ? 1 : result);
}

static void* TCPClientThread(void *p)
{
	TCPClient *client = (TCPClient*)p;
	client->msg.done = &TCPClientInmsgDone;
	return (void*)client->msg.done2(1);
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
		if (client == NULL) {
			//ENOMEM
			closesocket(sock);
		} else if (server->is_async) {
			client->sock = sock;
			client->sysop.post(NULL);
		} else {
			pthread_post(client, &TCPClientThread);
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
		buf->next(), /*buf->left()-result*2*/0,
		result, result, &tx, &server->sysio.ao_ovlp);
	if (!result) {
		result = -WSAGetLastError();
		if (result != -ERROR_IO_PENDING) {
			TRACE("AcceptEx(%d) = %d.\n", server->port, result);
			return -EIO;
		}
	}
	return 0;
}

static int TCPServerAcceptExDone(AOperator *sysop, int result)
{
	TCPServer *server = container_of(sysop, TCPServer, sysio);
	if (result >= 0) {
		server->prepare->tobuf()->push(result);
		result = setsockopt(server->prepare->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		                    (const char *)&server->sock, sizeof(server->sock));
	}
	if (result >= 0) {
		if (server->is_async) {
			server->prepare->sysop.post(NULL);
		} else {
			pthread_post(server->prepare, &TCPClientThread);
		}
		server->prepare = NULL;
	} else {
		result = -WSAGetLastError();
		TRACE("AcceptEx(%d) = %d.\n", server->port, result);
		release_s(server->prepare);
	}

	result = TCPServerDoAcceptEx(server);
	if (result < 0) {
		release_s(server->prepare);
		server->release();
	}
	return result;
}
#endif

static int TCPServerStart(AService *service, AOption *option)
{
	TCPServer *server = (TCPServer*)service;
	if (server->sock != INVALID_SOCKET) {
		assert(0);
		return -EACCES;
	}

	server->option = AOptionClone(option, NULL);
	if (server->option == NULL)
		return -ENOMEM;

	server->peer_option = server->option->find("io");
	server->peer_module = AModuleFind("io", server->peer_option ? server->peer_option->value : "async_tcp");
	if ((server->peer_module == NULL) || (server->io()->svc_accept == NULL)) {
		TRACE("require option: \"io\", function: io->svc_accept()\n");
		return -ENOENT;
	}

	server->port = (u_short)server->option->getI64("port", 0);
	if (server->port == 0) {
		TRACE("require option: \"port\"\n");
		return -EINVAL;
	}

	if (server->io()->svc_module != NULL) {
		int result = AObjectCreate2(&server->io_svc_data, server, server->peer_option, server->io()->svc_module);
		if (result < 0) {
			TRACE("io(%s) create service data = %d.\n", server->peer_module->module_name, result);
			return result;
		}
	}

	server->services = server->option->find("services");
	if ((server->services == NULL) || server->services->children_list.empty()) {
		TRACE("require option: \"services\"\n");
		return -EINVAL;
	}
	//list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry) {
	//	m_ptr(m_opt) = NULL;
	//}
	list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry) {
		AService *&svc = m_ptr(m_opt);

		int result = AObject::create(&svc, server, m_opt, NULL);
		if (result == NULL) {
			TRACE("create services(%s,%s) = %d.\n", m_opt->name, m_opt->value, result);
			continue;
		}
		svc->sysmng = server->sysmng;
		if (strcasecmp(svc->peer_module->class_name, "AEntity") != 0) {
			TRACE("service(%s,%s) peer type(%s,%s) maybe error, require AEntity!.\n",
				svc->_module->class_name, svc->_module->module_name,
				svc->peer_module->class_name, svc->peer_module->module_name);
		}
		if ((svc->start != NULL) && ((result = svc->start(svc, m_opt)) < 0)) {
			TRACE("start services(%s,%s) = %d.\n", m_opt->name, m_opt->value, result);
			release_s(svc);
		}
	}

	const char *af = server->option->getStr("family", NULL);
	if ((af != NULL) && (strcasecmp(af, "inet6") == 0))
		server->family = AF_INET6;
	else
		server->family = AF_INET;

	server->sock = tcp_bind(server->family, IPPROTO_TCP, server->port);
	if (server->sock == INVALID_SOCKET) {
		TRACE("tcp_bind(%d, %d) failed, error = %d.\n", server->family, server->port, errno);
		return -EINVAL;
	}

	int backlog = server->option->getInt("backlog", 8);
	int result = listen(server->sock, backlog);
	if (result != 0) {
		TRACE("listen(%d, %d) failed, error = %d.\n", server->sock, backlog, errno);
		return -EIO;
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
	pthread_post(server, &TCPServerProcess, server);
	return 0;
#else
	GUID ax_guid = WSAID_ACCEPTEX;
	DWORD tx = 0;
	result = WSAIoctl(server->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &ax_guid, sizeof(ax_guid),
	                  &server->acceptex, sizeof(server->acceptex), &tx, NULL, NULL);
	if (result != 0) {
		TRACE("server(%d,%d) require AcceptEx() funciton.\n", server->sock, server->port);
		server->release();
		return -EIO;
	}

	result = AThreadBind(NULL, (HANDLE)server->sock);
	memset(&server->sysio, 0, sizeof(server->sysio));
	server->sysio.done = &TCPServerAcceptExDone;

	result = TCPServerDoAcceptEx(server);
	if (result < 0)
		server->release();
	return result;
#endif
}

static void TCPServerStop(AService *service)
{
	TCPServer *server = (TCPServer*)service;
	closesocket_s(server->sock);

	if (server->services != NULL)
		list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
	{
		AService *svc = m_ptr(m_opt);
		if ((svc == NULL) && (svc->stop != NULL)) {
			svc->stop(svc);
		}
	}
}

static int TCPServerRun(AService *service, AObject *peer, AOption *option)
{
	TCPServer *server = (TCPServer*)service;
	if (peer->_module != server->peer_module) {
		TRACE("invalid peer %s(%s), require %s(%s).\n",
			peer->_module->class_name, peer->_module->module_name,
			server->peer_module->class_name, server->peer_module->module_name);
		//return -EINVAL;
	}

	TCPClient *client = TCPClientCreate(server);
	if (client == NULL)
		return -ENOMEM;

	client->status = tcp_io_attach;
	client->io = (IOObject*)peer;
	return TCPClientInmsgDone(&client->msg, 1);
}

static int TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *server = (TCPServer*)*object;
	server->peer_option = option->find("io");
	server->peer_module = AModuleFind("io", server->peer_option ? server->peer_option->value : "async_tcp");
	server->start = &TCPServerStart;
	server->stop = &TCPServerStop;
	server->run = &TCPServerRun;
	server->abort = NULL;

	server->sock = INVALID_SOCKET;
	server->option = NULL;
	server->services = NULL;
	server->prepare = NULL;
	server->io_svc_data = NULL;
	return 1;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *server = (TCPServer*)object;
	closesocket_s(server->sock);
	if_not2(server->services, NULL,
		list_for_each2(m_opt, &server->services->children_list, AOption, brother_entry)
	{
		AService *&svc = m_ptr(m_opt);
		if ((svc != NULL) && (svc->stop != NULL)) {
			svc->stop(svc);
		}
		release_s(svc);
	});
	release_s(server->option);
	release_s(server->prepare);
	release_s(server->io_svc_data);
}

AModule TCPServerModule = {
	"AService",
	"tcp_server",
	sizeof(TCPServer),
	NULL, NULL,
	&TCPServerCreate,
	&TCPServerRelease,
};

static int reg_svr = AModuleRegister(&TCPServerModule);
