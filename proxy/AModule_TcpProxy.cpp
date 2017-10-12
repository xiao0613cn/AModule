#include "stdafx.h"
#ifdef _WIN32
#ifndef _MSWSOCK_
#include <MSWSock.h>
#endif
#pragma comment(lib, "ws2_32.lib")
#endif
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"


struct TCPServer {
	AObject   object;
	SOCKET    sock;
	u_short   port;
	pthread_t thread;
	AOption  *option;

	AModule  *io_module;
	int       async_tcp;
	AOption  *default_bridge;

	int       io_family;
	struct TCPClient *prepare;
#ifdef _WIN32
	LPFN_ACCEPTEX acceptex;
#endif
	AOperator sysio;
};
#define to_server(obj) container_of(obj, TCPServer, object)


enum TCPStatus {
	tcp_accept,
	tcp_client_open,
	tcp_recv_probe,
	tcp_proxy_open,
	tcp_client_output,
	tcp_proxy_input,

	tcp_bridge_open,
	tcp_bridge_input,
	tcp_bridge_output,
};
struct TCPClient {
	AOperator  sysop;
	TCPServer *server;
	TCPStatus  status;
	SOCKET     sock;
	IOObject  *client;
	IOObject  *proxy;
	int        probe_type;
	int        probe_size;
	AMessage   inmsg;
	char       indata[1800];
};

static void TCPClientRelease(TCPClient *client)
{
	if (client->proxy != NULL)
		client->proxy->cancel(Aio_Input, NULL);
	//TRACE("%p: result = %d.\n", client, result);
	release_s(client->proxy);
	release_s(client->client);
	closesocket_s(client->sock);
	client->server->object.release();
	free(client);
}

static int TCPClientInmsgDone(AMessage *msg, int result);
static int TCPClientProcess(AOperator *asop, int result)
{
	TCPClient *client = container_of(asop, TCPClient, sysop);
	return TCPClientInmsgDone(&client->inmsg, (result>=0) ? 1 : result);
}

static TCPClient* TCPClientCreate(TCPServer *server)
{
	TCPClient *client = gomake(TCPClient);
	if (client != NULL) {
		client->sysop.done = &TCPClientProcess;
		client->server = server; AObjectAddRef(&server->object);
		client->status = tcp_accept;
		client->sock = INVALID_SOCKET;
		client->client = NULL;
		client->proxy = NULL;
		client->probe_type = AMsgType_Unknown;
		client->probe_size = 0;
		client->inmsg.init();
		client->inmsg.done = &TCPClientInmsgDone;
	}
	return client;
}

static int TCPClientInmsgDone(AMessage *msg, int result)
{
	TCPClient *client = container_of(msg, TCPClient, inmsg);
	TCPServer *server = client->server;
	while (result > 0)
	{
		switch (client->status)
		{
		case tcp_accept:
			result = AObject::create2(&client->client, NULL, NULL, server->io_module);
			if (result < 0)
				break;

			client->status = tcp_client_open;
			client->inmsg.init(AMsgType_Handle, (void*)client->sock, 0);
			result = client->client->open(&client->inmsg);
			break;

		case tcp_client_open:
			client->sock = INVALID_SOCKET;
			client->status = tcp_recv_probe;
			if (client->probe_size != 0) {
				AMsgInit(&client->inmsg, client->probe_type, NULL, 0);
				break;
			}

			client->inmsg.init(0, client->indata, sizeof(client->indata)-1);
			result = client->client->output(&client->inmsg);
			break;

		case tcp_recv_probe:
			client->probe_type = client->inmsg.type;
			client->probe_size += client->inmsg.size;
			client->indata[client->probe_size] = '\0';
			client->inmsg.init(client->probe_type, client->indata, client->probe_size);

			AModule *module;
			module = AModuleProbe("proxy", client->client, &client->inmsg);
			if ((module == NULL) && (client->inmsg.size < 8))
			{
				TRACE("retry probe: %s\n", client->indata);
				client->inmsg.init(0, client->indata+client->probe_size,
					sizeof(client->indata)-1-client->probe_size);
				result = client->client->output(&client->inmsg);
				break;
			}

			AOption *proxy_opt;
			if (module != NULL)
				proxy_opt = server->option->find(module->module_name);
			else
				proxy_opt = NULL;
			if ((module == NULL) || (proxy_opt != NULL && strcasecmp(proxy_opt->value, "bridge") == 0))
			{
				if (proxy_opt == NULL)
					proxy_opt = server->default_bridge;
				if (proxy_opt == NULL) {
					result = -EFAULT;
				} else {
					result = AObject::create2(&client->proxy, client->client, proxy_opt, server->io_module);
				}
				client->inmsg.init(proxy_opt);
				client->status = tcp_bridge_open;
			}
			else
			{
				result = AObject::create2(&client->proxy, client->client, proxy_opt, module);
				//AMsgInit(&client->inmsg, AMsgType_Object, client->client, 0);
				client->status = tcp_proxy_input; //tcp_proxy_open;
			}
			if (result >= 0) {
				result = client->proxy->open(&client->inmsg);
			}
			break;

		//case tcp_proxy_open:
		//	AMsgInit(&client->inmsg, client->probe_type, client->indata, client->probe_size);

		case tcp_client_output:
			if (server->sock == INVALID_SOCKET) {
				result = -EINTR;
			} else {
				client->status = tcp_proxy_input;
				result = client->proxy->input(&client->inmsg);
			}
			break;

		case tcp_proxy_input:
			if (client->inmsg.data != NULL) {
				client->inmsg.init();
				if (!server->async_tcp) {
					client->sysop.post(NULL);
					return 0;
				}
			}
			result = client->proxy->input(&client->inmsg);
			if (result < 0) {
				client->inmsg.init(0, client->indata, sizeof(client->indata)-1);
				result = 1;
			}
			if (result > 0) {
				client->status = tcp_client_output;
				result = client->client->output(&client->inmsg);
			}
			break;

		case tcp_bridge_open:
			TCPClient *bridge;
			bridge = TCPClientCreate(server);
			if (bridge == NULL) {
				result = -ENOMEM;
				break;
			}

			bridge->status = tcp_bridge_input;
			bridge->client = client->proxy; client->proxy->addref();
			bridge->proxy = client->client; client->client->addref();
			bridge->sysop.post(NULL);

			client->inmsg.init(ioMsgType_Block, client->indata, client->probe_size);
#ifdef _DEBUG
			msg->data[msg->size] = '\0';
			OutputDebugStringA(msg->data);
			fputs(msg->data, stdout);
#endif
		case tcp_bridge_output:
			if (server->sock == INVALID_SOCKET) {
				result = -EINTR;
			} else {
				client->status = tcp_bridge_input;
				result = client->proxy->input(&client->inmsg);
			}
			break;

		case tcp_bridge_input:
			client->status = tcp_bridge_output;
			client->inmsg.init(0, client->indata, sizeof(client->indata)-1);
			result = client->client->output(&client->inmsg);
			break;

		default:
			assert(FALSE);
			result = -EACCES;
			break;
		}
	}
	if (result < 0) {
		TCPClientRelease(client);
	}
	return result;
}

static void* TCPServerProcess(void *p)
{
	TCPServer *server = to_server(p);
	struct sockaddr addr;
	socklen_t addrlen;
	SOCKET sock;

	for ( ; ; ) {
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
			closesocket(sock);
			continue;
		}

		client->sock = sock;
		client->sysop.post(NULL);
	}
	TRACE("%p: quit.\n", &server->object);
	AObjectRelease(&server->object);
	return 0;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *server = to_server(object);
	closesocket_s(server->sock);
	if_not(server->thread, pthread_null, pthread_detach);
	release_s(server->option);
	if_not(server->prepare, NULL, TCPClientRelease);
}

static int TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *server = (TCPServer*)*object;
	server->sock = INVALID_SOCKET;
	server->thread = pthread_null;
	server->option = NULL;
	server->prepare = NULL;
	return 1;
}

#ifdef _WIN32
static int TCPServerDoAcceptEx(TCPServer *server)
{
	server->prepare = TCPClientCreate(server);
	if (server->prepare == NULL)
		return -ENOMEM;

	server->prepare->sock = socket(server->io_family, SOCK_STREAM, IPPROTO_TCP);
	if (server->prepare->sock == INVALID_SOCKET)
		return -EFAULT;

	int result = sizeof(struct sockaddr_in)+16;
	if (server->io_family == AF_INET6)
		result = sizeof(struct sockaddr_in6)+16;

	DWORD tx = 0;
	result = server->acceptex(server->sock, server->prepare->sock, server->prepare->indata,
	                          sizeof(server->prepare->indata)-1-result*2, result, result, &tx, &server->sysio.ao_ovlp);
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
		server->prepare->probe_type = AMsgType_Unknown;
		server->prepare->probe_size = result;
		result = setsockopt(server->prepare->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		                    (const char *)&server->sock, sizeof(server->sock));
	}
	if (result >= 0) {
		server->prepare->sysop.post(NULL);
		server->prepare = NULL;
	} else {
		result = -WSAGetLastError();
		if_not(server->prepare, NULL, TCPClientRelease);
	}

	result = TCPServerDoAcceptEx(server);
	if (result < 0) {
		if_not(server->prepare, NULL, TCPClientRelease);
		server->object.release();
	}
	return result;
}
#endif

static int TCPServerOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	TCPServer *server = to_server(object);
	release_s(server->option);

	server->option = AOptionClone((AOption*)msg->data, NULL);
	if (server->option == NULL)
		return -ENOMEM;

	const char *af = AOptionGet(server->option, "family");
	if ((af != NULL) && (strcasecmp(af, "inet6") == 0))
		server->io_family = AF_INET6;
	else
		server->io_family = AF_INET;

	server->port = (u_short)AOptionGetInt(server->option, "port", 0);
	if (server->port == 0)
		return -EINVAL;

	server->sock = tcp_bind(server->io_family, IPPROTO_TCP, server->port);
	if (server->sock == INVALID_SOCKET)
		return -EINVAL;

	int backlog = AOptionGetInt(server->option, "backlog", 8);
	int result = listen(server->sock, backlog);
	if (result != 0)
		return -EIO;

	server->io_module = AModuleFind("io", AOptionGet(server->option, "io", "tcp"));
	server->async_tcp = (strcasecmp(server->io_module->module_name, "async_tcp") == 0);
	server->default_bridge = AOptionFind(server->option, "default_bridge");

	if (!AOptionGetInt(server->option, "background", TRUE))
		return 1;

	AObjectAddRef(&server->object);
#ifndef _WIN32
	pthread_create(&server->thread, NULL, &TCPServerProcess, server);
	return 1;
#else
	if (!server->async_tcp) {
		pthread_create(&server->thread, NULL, &TCPServerProcess, server);
		return 1;
	}

	GUID ax_guid = WSAID_ACCEPTEX;
	DWORD tx = 0;
	result = WSAIoctl(server->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &ax_guid, sizeof(ax_guid),
	                  &server->acceptex, sizeof(server->acceptex), &tx, NULL, NULL);
	if (result != 0)
		return -EIO;

	result = AThreadBind(NULL, (HANDLE)server->sock);
	memset(&server->sysio.ao_ovlp, 0, sizeof(server->sysio.ao_ovlp));
	server->sysio.done = &TCPServerAcceptExDone;

	if_not(server->prepare, NULL, TCPClientRelease);
	result = TCPServerDoAcceptEx(server);
	return (result >= 0) ? 1 : result;
#endif
}

static int TCPServerRequest(AObject *object, int reqix, AMessage *msg)
{
	TCPServer *server = to_server(object);
	if (reqix != Aio_Output) {
		return -ENOSYS;
	}

	struct sockaddr addr;
	memset(&addr, 0, sizeof(addr));

	socklen_t addrlen = sizeof(addr);
	SOCKET sock = accept(server->sock, &addr, &addrlen);
	if (sock == INVALID_SOCKET)
		return -EIO;

	msg->init((HANDLE)sock);
	return 1;
}

static int TCPServerCancel(AObject *object, int reqix, AMessage *msg)
{
	TCPServer *server = to_server(object);
	if (server->sock != INVALID_SOCKET)
		shutdown(server->sock, SD_BOTH);
	return 1;
}

static int TCPServerClose(AObject *object, AMessage *msg)
{
	TCPServer *server = to_server(object);
	closesocket_s(server->sock);
	if_not2(server->thread, pthread_null, pthread_join(server->thread, NULL));
	return 1;
}

IOModule TCPServerModule = { {
	"server",
	"tcp_server",
	sizeof(TCPServer),
	NULL, NULL,
	&TCPServerCreate,
	&TCPServerRelease,
	NULL, },

	&TCPServerOpen,
	NULL,
	NULL,
	&TCPServerRequest,
	&TCPServerCancel,
	&TCPServerClose,
};

static auto_reg_t reg(TCPServerModule.module);
