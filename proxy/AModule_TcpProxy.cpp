#include "stdafx.h"
#include <process.h>
#include "../base/AModule.h"
#include "../io/iocp_util.h"


struct TCPServer {
	AObject  object;
	SOCKET   sock;
	HANDLE   listen_thread;
	AOption *option;

	AModule *io_module;
	long     async_tcp;
	AOption *default_bridge;
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
	TCPServer *server;
	TCPStatus  status;
	SOCKET     sock;
	AObject   *client;
	AObject   *proxy;
	long       probe_type;
	long       probe_size;
	AMessage   inmsg;
	char       indata[1800];
};
#define from_inmsg(msg) container_of(msg, TCPClient, inmsg)

static long TCPClientRelease(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	//TRACE("%p: result = %d.\n", client, result);
	release_s(client->proxy, AObjectRelease, NULL);
	release_s(client->client, AObjectRelease, NULL);
	release_s(client->sock, closesocket, INVALID_SOCKET);
	AObjectRelease(&client->server->object);
	free(client);
	return result;
}

static DWORD WINAPI TCPClientProcess(void *p);
static long TCPClientInmsgDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	while (result > 0)
	{
		switch (client->status)
		{
		case tcp_accept:
			result = client->server->io_module->create(&client->client, NULL, NULL);
			if (result < 0)
				break;

			client->status = tcp_client_open;
			client->inmsg.done = &TCPClientInmsgDone;

			AMsgInit(&client->inmsg, AMsgType_Object, (char*)client->sock, sizeof(client->sock));
			result = client->client->open(client->client, &client->inmsg);
			break;

		case tcp_client_open:
			client->sock = INVALID_SOCKET;
			client->status = tcp_recv_probe;

			AMsgInit(&client->inmsg, AMsgType_Unknown, client->indata, sizeof(client->indata)-1);
			result = client->client->request(client->client, ARequest_Output, &client->inmsg);
			break;

		case tcp_recv_probe:
			client->probe_type = client->inmsg.type;
			client->probe_size = client->inmsg.size;

			AModule *module;
			module = AModuleProbe("proxy", client->client, &client->inmsg);

			AOption *proxy_opt;
			if (module != NULL)
				proxy_opt = AOptionFindChild(client->server->option, module->module_name);
			else
				proxy_opt = NULL;
			if ((proxy_opt == NULL) || (_stricmp(proxy_opt->value, "bridge") == 0))
			{
				if (proxy_opt == NULL)
					proxy_opt = client->server->default_bridge;
				if (proxy_opt == NULL) {
					result = -EFAULT;
				} else {
					result = client->server->io_module->create(&client->proxy, client->client, proxy_opt);
				}
				AMsgInit(&client->inmsg, AMsgType_Option, (char*)proxy_opt, sizeof(*proxy_opt));
				client->status = tcp_bridge_open;
			}
			else
			{
				result = module->create(&client->proxy, client->client, proxy_opt);
				AMsgInit(&client->inmsg, AMsgType_Object, (char*)client->client, sizeof(AObject));
				client->status = tcp_proxy_open;
			}
			if (result >= 0) {
				result = client->proxy->open(client->proxy, &client->inmsg);
			}
			break;

		case tcp_proxy_open:
			AMsgInit(&client->inmsg, client->probe_type, client->indata, client->probe_size);

		case tcp_client_output:
			if (client->server->sock == INVALID_SOCKET) {
				result = -EINTR;
			} else {
				client->status = tcp_proxy_input;
				result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
			}
			break;

		case tcp_proxy_input:
			if (client->inmsg.data != NULL) {
				AMsgInit(&client->inmsg, AMsgType_Unknown, NULL, 0);
				if (!client->server->async_tcp) {
					QueueUserWorkItem(&TCPClientProcess, client, 0);
					return 0;
				}
			}
			result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
			if (result > 0) {
				client->status = tcp_client_output;
				result = client->client->request(client->client, ARequest_Output, &client->inmsg);
			}
			break;

		case tcp_bridge_open:
			TCPClient *bridge;
			bridge = (TCPClient*)malloc(sizeof(TCPClient));
			if (bridge == NULL) {
				result = -ENOMEM;
				break;
			}

			bridge->server = client->server; AObjectAddRef(&client->server->object);
			bridge->status = tcp_bridge_input;
			bridge->sock = INVALID_SOCKET;
			bridge->client = client->proxy; AObjectAddRef(client->proxy);
			bridge->proxy = client->client; AObjectAddRef(client->client);
			bridge->inmsg.done = &TCPClientInmsgDone;
			if (client->server->async_tcp) {
				TCPClientProcess(bridge);
			} else {
				QueueUserWorkItem(&TCPClientProcess, bridge, 0);
			}

			AMsgInit(&client->inmsg, client->probe_type, client->indata, client->probe_size);
			msg->data[msg->size] = '\0';
			OutputDebugStringA(msg->data);
			fputs(msg->data, stdout);

		case tcp_bridge_output:
			if (client->server->sock == INVALID_SOCKET) {
				result = -EINTR;
			} else {
				client->inmsg.type |= AMsgType_Custom;
				client->status = tcp_bridge_input;
				result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
			}
			break;

		case tcp_bridge_input:
			AMsgInit(&client->inmsg, AMsgType_Unknown, client->indata, sizeof(client->indata)-1);
			client->status = tcp_bridge_output;
			result = client->client->request(client->client, ARequest_Output, &client->inmsg);
			break;

		default:
			assert(FALSE);
			result = -EACCES;
			break;
		}
	}
	if (result < 0) {
		result = TCPClientRelease(&client->inmsg, result);
	}
	return result;
}
static DWORD WINAPI TCPClientProcess(void *p)
{
	TCPClient *client = (TCPClient*)p;
	return TCPClientInmsgDone(&client->inmsg, 1);
}

static unsigned __stdcall TCPServerProcess(void *p)
{
	TCPServer *server = to_server(p);
	struct sockaddr addr;
	int addrlen;
	SOCKET sock;

	AOption *opt = AOptionFindChild(server->option, "io");
	server->io_module = AModuleFind("io", opt?opt->value:"tcp");
	server->async_tcp = (_stricmp(server->io_module->module_name, "async_tcp") == 0);
	server->default_bridge = AOptionFindChild(server->option, "default_bridge");

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
		client->status = tcp_accept;
		client->sock = sock;
		client->client = NULL;
		client->proxy = NULL;
		if (server->async_tcp) {
			TCPClientProcess(client);
		} else {
			QueueUserWorkItem(&TCPClientProcess, client, 0);
		}
	} while (1);
	TRACE("%p: quit.\n", &server->object);
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
	/*DWORD flag = 0;
	WT_SET_MAX_THREADPOOL_THREADS(flag, 8192);
	QueueUserWorkItem(&TCPServerProcess, &server->object, flag);*/
	server->listen_thread = (HANDLE)_beginthreadex(NULL, 0, &TCPServerProcess, server, 0, NULL);
	return 1;
}

static long TCPServerClose(AObject *object, AMessage *msg)
{
	TCPServer *server = to_server(object);
	release_s(server->sock, closesocket, INVALID_SOCKET);
	if (server->listen_thread != NULL) {
		WaitForSingleObject(server->listen_thread, INFINITE);
		CloseHandle(server->listen_thread);
		server->listen_thread = NULL;
	}
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
