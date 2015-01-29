#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/iocp_util.h"


struct TCPServer {
	AObject  object;
	SOCKET   sock;
	HANDLE   listen_thread;
	AOption *option;
};
#define to_server(obj) container_of(obj, TCPServer, object)


struct TCPClient {
	TCPServer *server;
	AModule   *module;
	SOCKET     sock;
	AObject   *tcp;
	AObject   *proxy;
	AMessage   probe_msg;
	char       probe_data[BUFSIZ];
};
#define to_client(msg)  container_of(msg, TCPClient, probe_msg)


static long TCPProxyOpenDone(AMessage *msg, long result)
{
	TCPClient *client = to_client(msg);
	release_s(client->proxy, AObjectRelease, NULL);
	release_s(client->tcp, AObjectRelease, NULL);
	release_s(client->sock, closesocket, INVALID_SOCKET);
	AObjectRelease(&client->server->object);
	free(client);
	return result;
}

static long TCPClientRecvDone(AMessage *msg, long result)
{
	TCPClient *client = to_client(msg);
	if (result >= 0) {
		client->probe_msg.done = NULL;

		AModule *module = AModuleProbe(client->tcp, &client->probe_msg, "proxy");
		if (module == NULL) {
			result = -EFAULT;
		} else {
			AOption *proxy_opt = AOptionFindChild(client->server->option, module->module_name);
			result = module->create(&client->proxy, client->tcp, proxy_opt);
		}
	}
	if (result >= 0) {
		client->probe_msg.done = &TCPProxyOpenDone;
		result = client->proxy->open(client->proxy, &client->probe_msg);
	}
	if (result != 0)
		result = TCPProxyOpenDone(&client->probe_msg, result);
	return result;
}

static long TCPClientOpenDone(AMessage *msg, long result)
{
	TCPClient *client = to_client(msg);
	if (result >= 0) {
		client->sock = INVALID_SOCKET;

		AMsgInit(&client->probe_msg, AMsgType_Unknown, client->probe_data, sizeof(client->probe_data));
		client->probe_msg.done = &TCPClientRecvDone;
		result = client->tcp->request(client->tcp, ARequest_Output, &client->probe_msg);
	}
	if (result != 0) {
		result = TCPClientRecvDone(&client->probe_msg, result);
	}
	return result;
}

static DWORD WINAPI TCPClientProcess(void *p)
{
	TCPClient *client = to_client(p);

	long result = client->module->create(&client->tcp, NULL, NULL);
	if (result >= 0) {
		AMsgInit(&client->probe_msg, AMsgType_Object, (char*)client->sock, sizeof(client->sock));
		client->probe_msg.done = &TCPClientOpenDone;
		result = client->tcp->open(client->tcp, &client->probe_msg);
	}
	if (result != 0) {
		result = TCPClientOpenDone(&client->probe_msg, result);
	}
	return result;
}

static DWORD WINAPI TCPServerProcess(void *p)
{
	TCPServer *server = to_server(p);
	struct sockaddr addr;
	int addrlen;
	SOCKET sock;

	AModule *io_module = NULL;
	AOption *io_option = AOptionFindChild(server->option, "io");
	if (io_option != NULL)
		io_module = AModuleFind(NULL, io_option->value);
	if (io_module == NULL)
		io_module = AModuleFind(NULL, "tcp");

	do {
		memset(&addr, 0, sizeof(addr));
		addrlen = sizeof(addr);

		sock = accept(server->sock, &addr, &addrlen);
		if (sock == INVALID_SOCKET)
			break;

		TCPClient *client = (TCPClient*)malloc(sizeof(TCPClient));
		if (client == NULL) {
			closesocket(sock);
			continue;
		}

		client->server = server; AObjectAddRef(&server->object);
		client->module = io_module;
		client->sock = sock;
		client->tcp = NULL;
		QueueUserWorkItem(&TCPClientProcess, &client->probe_msg, 0);
	} while (1);
	TRACE("quit.\n");
	AObjectRelease(&server->object);
	return 0;
}

static void TCPServerRelease(AObject *object)
{
	TCPServer *server = to_server(object);
	release_s(server->sock, closesocket, INVALID_SOCKET);
	release_s(server->listen_thread, CloseHandle, NULL);
	release_s(server->option, AOptionRelease, NULL);
	free(server);
}

static long TCPServerCreate(AObject **object, AObject *parent, AOption *option)
{
	TCPServer *server = (TCPServer*)malloc(sizeof(TCPServer));
	if (server == NULL)
		return -ENOMEM;

	extern AModule TCPServerModule;
	AObjectInit(&server->object, &TCPServerModule);
	server->sock = INVALID_SOCKET;
	server->listen_thread = NULL;
	server->option = NULL;

	*object = &server->object;
	return 1;
}

static long TCPServerOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	TCPServer *server = to_server(object);
	release_s(server->option, AOptionRelease, NULL);
	server->option = AOptionClone((AOption*)msg->data);
	if (server->option == NULL)
		return -ENOMEM;

	AOption *port_opt = AOptionFindChild(server->option, "port");
	if (port_opt == NULL)
		return -EINVAL;

	server->sock = bind_socket(IPPROTO_TCP, (u_short)atol(port_opt->value));
	if (server->sock == INVALID_SOCKET)
		return -EINVAL;

	long backlog = 8;
	AOption *backlog_opt = AOptionFindChild(server->option, "backlog");
	if (backlog_opt != NULL)
		backlog = atol(backlog_opt->value);
	long result = listen(server->sock, backlog);
	if (result != 0)
		return -EIO;

	AObjectAddRef(&server->object);
	QueueUserWorkItem(&TCPServerProcess, &server->object, 0);
	return 1;
}

static long TCPServerClose(AObject *object, AMessage *msg)
{
	TCPServer *server = to_server(object);
	release_s(server->sock, closesocket, INVALID_SOCKET);
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
