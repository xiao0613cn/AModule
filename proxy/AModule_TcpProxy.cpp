#include "stdafx.h"
#ifdef _WIN32
#ifndef _MSWSOCK_
#include <MSWSock.h>
#endif
#pragma comment(lib, "ws2_32.lib")
#endif
#include "../base/AModule_API.h"
#include "../ecs/AInOutComponent.h"
#include "../ecs/AServiceComponent.h"


struct TCPServer : public AEntity {
	AServiceComponent svc;

	SOCKET     sock;
	AOption   *io_option;
	AObject   *io_svc_data;

	u_short    port;
	int        family;
	BOOL       is_async_io;
	int        min_probe_size;
	int        max_probe_size;
#ifdef _WIN32
	LPFN_ACCEPTEX acceptex;
	AOperator  sysio;
	struct TCPClient *prepare;
#endif
	IOModule* PM() { return (IOModule*)svc._peer_module; }
};

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
	TCPServer *tcpd;
	TCPStatus  status;
	SOCKET     sock;
	IOObject  *io;

	ARefsBuf*  tobuf() { return container_of(this, ARefsBuf, _data); }
	void release() {
		release_s(tcpd);
		closesocket_s(sock);
		release_s(io);
		tobuf()->release();
	}
};

static int TCPClientInmsgDone(AMessage *msg, int result)
{
	TCPClient *client = container_of(msg, TCPClient, msg);
	TCPServer *tcpd = client->tcpd;
	ARefsBuf  *buf = client->tobuf();

	while (result > 0)
	{
	switch (client->status)
	{
	case tcp_io_accept:
		result = AObject::create2(&client->io, tcpd->io_svc_data, tcpd->io_option, tcpd->svc._peer_module);
		if (result < 0)
			break;

		client->status = tcp_io_attach;
		msg->init(AMsgType_Handle, (void*)client->sock, 0);
		result = tcpd->PM()->svc_accept(client->io, msg, tcpd->io_svc_data, tcpd->io_option);
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
		AServiceComponent *service = AServiceComponent::get()->probe(&tcpd->svc, client->io, msg);
		if (service == NULL) {
			if (msg->size < tcpd->min_probe_size) {
				msg->init(0, buf->next(), buf->left()-1);
				result = client->io->output(msg);
				break;
			}
			TRACE("TCPServer(%d): no found service: %d, %.*s.\n",
				tcpd->port, msg->size, msg->size, msg->data);
			result = -EINVAL;
			break;
		}

		AEntity *e = NULL;
		result = AObject::create2(&e, service->_entity, service->_svc_option, service->_peer_module);
		if (result < 0)
			break;

		// TCPClient detach()...
		AInOutComponent *c; e->get(&c);
		assert(c != NULL);
		release_s(c->_io); c->_io = client->io; client->io = NULL;
		release_s(c->_outbuf); c->_outbuf = buf;
		release_s(client->tcpd);

		service->run(service, e);
		e->release();
		return 1;
	}
	default: assert(0); result = -EACCES; break;
	}
	}
	if (result < 0) {
		TRACE("TCPServer(%d): client(%d) error = %d.\n", tcpd->port, client->sock, result);
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

static TCPClient* TCPClientCreate(TCPServer *tcpd)
{
	ARefsBuf *buf = ARefsBuf::create(sizeof(TCPClient) + tcpd->max_probe_size);
	if (buf == NULL)
		return NULL;

	TCPClient *client = (TCPClient*)buf->ptr();
	buf->push(sizeof(TCPClient));
	buf->pop(sizeof(TCPClient));

	client->sysop.done = &TCPClientProcess;
	client->tcpd = tcpd; tcpd->addref();
	client->status = tcp_io_accept;
	client->sock = INVALID_SOCKET;
	client->io = NULL;
	return client;
}

static void* TCPServerProcess(void *p)
{
	TCPServer *tcpd = (TCPServer*)p;
	struct sockaddr addr;
	socklen_t addrlen;
	SOCKET sock;

	for (;;) {
		memzero(addr);
		addrlen = sizeof(addr);

		sock = accept(tcpd->sock, &addr, &addrlen);
		if (sock == INVALID_SOCKET) {
			TRACE("TCPServer(%d): accept() failed, errno = %d.\n", tcpd->port, errno);

			if ((errno == EINTR) || (errno == EAGAIN))
				continue;
			if ((errno == ENFILE) || (errno == EMFILE)) {
				Sleep(1000);
				continue;
			}
			break;
		}

		TCPClient *client = TCPClientCreate(tcpd);
		if (client == NULL) {
			//ENOMEM
			closesocket(sock);
		} else if (tcpd->is_async_io) {
			client->sock = sock;
			client->sysop.post(NULL);
		} else {
			client->sock = sock;
			pthread_post(client, &TCPClientThread);
		}
	}
	TRACE("TCPServer(%d): quit.\n", tcpd->port);
	tcpd->release();
	return 0;
}

#ifdef _WIN32
static int TCPServerDoAcceptEx(TCPServer *tcpd)
{
	tcpd->prepare = TCPClientCreate(tcpd);
	if (tcpd->prepare == NULL)
		return -ENOMEM;

	tcpd->prepare->sock = socket(tcpd->family, SOCK_STREAM, IPPROTO_TCP);
	if (tcpd->prepare->sock == INVALID_SOCKET)
		return -EFAULT;

	int result = sizeof(struct sockaddr_in)+16;
	if (tcpd->family == AF_INET6)
		result = sizeof(struct sockaddr_in6)+16;

	DWORD tx = 0;
	ARefsBuf *buf = tcpd->prepare->tobuf();
	result = tcpd->acceptex(tcpd->sock, tcpd->prepare->sock,
		buf->next(), /*buf->left()-result*2*/0,
		result, result, &tx, &tcpd->sysio.ao_ovlp);
	if (!result) {
		result = -WSAGetLastError();
		if (result != -ERROR_IO_PENDING) {
			TRACE("TCPServer(%d): AcceptEx() = %d.\n", tcpd->port, result);
			return -EIO;
		}
	}
	return 0;
}

static int TCPServerAcceptExDone(AOperator *sysop, int result)
{
	TCPServer *tcpd = container_of(sysop, TCPServer, sysio);
	if (result >= 0) {
		tcpd->prepare->tobuf()->push(result);
		result = setsockopt(tcpd->prepare->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		                    (const char *)&tcpd->sock, sizeof(tcpd->sock));
	}
	if (result >= 0) {
		if (tcpd->is_async_io) {
			tcpd->prepare->sysop.post(NULL);
		} else {
			pthread_post(tcpd->prepare, &TCPClientThread);
		}
		tcpd->prepare = NULL;
	} else {
		result = -WSAGetLastError();
		TRACE("TCPServer(%d): AcceptExDone() = %d.\n", tcpd->port, result);
		release_s(tcpd->prepare);
	}

	result = TCPServerDoAcceptEx(tcpd);
	if (result < 0) {
		release_s(tcpd->prepare);
		tcpd->release();
	}
	return result;
}
#endif

static int TCPServerStart(AServiceComponent *service, AOption *option)
{
	TCPServer *tcpd = container_of(service, TCPServer, svc);
	if (tcpd->sock != INVALID_SOCKET) {
		assert(0);
		return -EBUSY;
	}

	tcpd->port = (u_short)tcpd->svc._svc_option->getI64("port", 0);
	if (tcpd->port == 0) {
		TRACE("TCPServer(%d): require option: \"port\"\n", tcpd->port);
		return -EINVAL;
	}

	assert(tcpd->svc._svc_option == option);
	tcpd->io_option = tcpd->svc._svc_option->find("io");

	tcpd->svc._peer_module = AModuleFind("io", tcpd->io_option
	                     ? tcpd->io_option->value : "async_tcp");
	if ((tcpd->svc._peer_module == NULL) || (tcpd->PM()->svc_accept == NULL)) {
		TRACE("TCPServer(%d): require option: \"io\", function: io->svc_accept()\n", tcpd->port);
		return -ENOENT;
	}

	if (tcpd->PM()->svc_module != NULL) {
		int result = AObject::create2(&tcpd->io_svc_data,
			tcpd, tcpd->io_option, tcpd->PM()->svc_module);
		if (result < 0) {
			TRACE("TCPServer(%d): io(%s) create service data = %d.\n",
				tcpd->port, tcpd->svc._peer_module->module_name, result);
			return result;
		}
	}

	const char *af = tcpd->svc._svc_option->getStr("family", NULL);
	if ((af != NULL) && (strcasecmp(af, "inet6") == 0))
		tcpd->family = AF_INET6;
	else
		tcpd->family = AF_INET;

	tcpd->sock = socket_bind(tcpd->family, IPPROTO_TCP, tcpd->port);
	if (tcpd->sock == INVALID_SOCKET) {
		TRACE("TCPServer(%d): socket_bind(%d) failed, error = %d.\n",
			tcpd->port, tcpd->family, errno);
		return -EINVAL;
	}

	int backlog = tcpd->svc._svc_option->getInt("backlog", 8);
	int result = listen(tcpd->sock, backlog);
	if (result != 0) {
		TRACE("TCPServer(%d): listen(%d) failed, error = %d.\n",
			tcpd->port, backlog, errno);
		return -EIO;
	}

	tcpd->min_probe_size = tcpd->svc._svc_option->getInt("min_probe_size", 128);
	tcpd->max_probe_size = tcpd->svc._svc_option->getInt("max_probe_size", 1800);
	tcpd->is_async_io = tcpd->svc._svc_option->getInt("is_async_io", TRUE);

	tcpd->addref();
	int background = tcpd->svc._svc_option->getInt("background", TRUE);
	if (!background) {
		TCPServerProcess(tcpd);
		return 1;
	}
#ifndef _WIN32
	pthread_post(tcpd, &TCPServerProcess);
	return 0;
#else
	GUID ax_guid = WSAID_ACCEPTEX;
	DWORD tx = 0;
	result = WSAIoctl(tcpd->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &ax_guid, sizeof(ax_guid),
	                  &tcpd->acceptex, sizeof(tcpd->acceptex), &tx, NULL, NULL);
	if (result != 0) {
		TRACE("TCPServer(%d): require AcceptEx() funciton.\n", tcpd->port);
		tcpd->release();
		return -EIO;
	}

	result = AThreadBind(NULL, (HANDLE)tcpd->sock);
	memzero(tcpd->sysio).done = &TCPServerAcceptExDone;

	result = TCPServerDoAcceptEx(tcpd);
	if (result < 0) {
		release_s(tcpd->prepare);
		tcpd->release();
	}
	return result;
#endif
}

static void TCPServerStop(AServiceComponent *service)
{
	TCPServer *tcpd = container_of(service, TCPServer, svc);
	closesocket_s(tcpd->sock);
}

static int TCPServerRun(AServiceComponent *service, AObject *peer)
{
	TCPServer *tcpd = container_of(service, TCPServer, svc);
	if (peer->_module != tcpd->svc._peer_module) {
		TRACE("TCPServer(%d): invalid peer %s(%s), require %s(%s).\n",
			tcpd->port, peer->_module->class_name, peer->_module->module_name,
			tcpd->svc._peer_module->class_name, tcpd->svc._peer_module->module_name);
		//return -EINVAL;
	}

	TCPClient *client = TCPClientCreate(tcpd);
	if (client == NULL)
		return -ENOMEM;

	client->status = tcp_io_attach;
	client->io = (IOObject*)peer;
	client->msg.done = &TCPClientInmsgDone;
	return client->msg.done2(1);
}

static int TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *tcpd = (TCPServer*)*object;
	tcpd->init();
	tcpd->init_push(&tcpd->svc);
	tcpd->svc._save_option = TRUE; tcpd->svc._require_child = TRUE;
	tcpd->io_option = option->find("io");
	tcpd->svc._peer_module = AModuleFind("io", tcpd->io_option ? tcpd->io_option->value : "async_tcp");
	tcpd->svc.start = &TCPServerStart;
	tcpd->svc.stop = &TCPServerStop;
	tcpd->svc.run = &TCPServerRun;

	tcpd->sock = INVALID_SOCKET;
	tcpd->io_svc_data = NULL;
#ifdef _WIN32
	tcpd->prepare = NULL;
#endif
	return 1;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *tcpd = (TCPServer*)object;
	closesocket_s(tcpd->sock);
	release_s(tcpd->io_svc_data);
#ifdef _WIN32
	release_s(tcpd->prepare);
#endif
	tcpd->pop_exit(&tcpd->svc);
	tcpd->exit();
}

AModule TCPServerModule = {
	AServiceComponent::name(),
	"tcp_server",
	sizeof(TCPServer),
	NULL, NULL,
	&TCPServerCreate,
	&TCPServerRelease,
};
static int reg_svr = AModuleRegister(&TCPServerModule);
