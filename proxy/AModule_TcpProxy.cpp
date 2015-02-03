#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/iocp_util.h"


struct TCPServer {
	AObject  object;
	SOCKET   sock;
	HANDLE   listen_thread;
	AOption *option;

	AModule *io_module;
	AOption *default_bridge;
};
#define to_server(obj) container_of(obj, TCPServer, object)


struct TCPClient {
	TCPServer *server;
	SOCKET     sock;
	AObject   *client;
	AObject   *proxy;
	long       probe_type;
	long       probe_size;
	AMessage   inmsg;
	AMessage   outmsg;
	char       indata[2048];
	char       outdata[2048];
};
#define from_inmsg(msg) container_of(msg, TCPClient, inmsg)
#define from_outmsg(msg) container_of(msg, TCPClient, outmsg)


static long TCPProxyRelease(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	TRACE("%p: result = %d.\n", client, result);
	release_s(client->proxy, AObjectRelease, NULL);
	release_s(client->client, AObjectRelease, NULL);
	release_s(client->sock, closesocket, INVALID_SOCKET);
	AObjectRelease(&client->server->object);
	free(client);
	return result;
}
static void TCPBridgeRelease(AObject *object)
{
	TCPClient *client = (TCPClient*)object->extend;
	assert(object == client->client);
	TRACE("proxy = %d, client = %d, server = %d.\n", client->proxy->count,
		client->client->count, client->server->object.count);

	release_s(client->proxy, AObjectRelease, NULL);
	release_s(client->client, client->client->module->release, NULL);
	release_s(client->sock, closesocket, INVALID_SOCKET);
	AObjectRelease(&client->server->object);
	free(client);
}

static long TCPClientSendDone(AMessage *msg, long result);
static long TCPBridgeRecvDone(AMessage *msg, long result)
{
	TCPClient *client = from_outmsg(msg);
	while (result > 0)
	{
		msg->type |= AMsgType_Custom;
		msg->done = &TCPClientSendDone;
		result = client->client->request(client->client, ARequest_Input, msg);
		if (result > 0)
		{
			AMsgInit(msg, AMsgType_Unknown, client->outdata, sizeof(client->outdata));
			msg->done = &TCPBridgeRecvDone;
			result = client->proxy->request(client->proxy, ARequest_Output, msg);
		}
	}
	if (result < 0)
		AObjectRelease(client->client);
	return result;
}
static long TCPClientSendDone(AMessage *msg, long result)
{
	TCPClient *client = from_outmsg(msg);
	while (result > 0)
	{
		AMsgInit(msg, AMsgType_Unknown, client->outdata, sizeof(client->outdata));
		msg->done = &TCPBridgeRecvDone;
		result = client->proxy->request(client->proxy, ARequest_Output, msg);
		if (result > 0)
		{
			//msg->data[msg->size] = '\0';
			//OutputDebugStringA(msg->data);

			msg->type |= AMsgType_Custom;
			msg->done = &TCPClientSendDone;
			result = client->client->request(client->client, ARequest_Input, msg);
		}
	}
	if (result < 0)
		AObjectRelease(client->client);
	return result;
}
static DWORD WINAPI TCPBridgeProcess(void *p)
{
	TCPClient *client = (TCPClient*)p;
	long result = TCPClientSendDone(&client->outmsg, 1);
	return result;
}

static long TCPBridgeSendDone(AMessage *msg, long result);
static long TCPClientRecvDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	while (result > 0)
	{
		msg->type |= AMsgType_Custom;
		msg->done = &TCPBridgeSendDone;
		result = client->proxy->request(client->proxy, ARequest_Input, msg);
		if (result > 0)
		{
			AMsgInit(msg, AMsgType_Unknown, client->indata, sizeof(client->indata));
			msg->done = &TCPClientRecvDone;
			result = client->client->request(client->client, ARequest_Output, msg);
		}
	}
	if (result < 0)
		AObjectRelease(client->client);
	return result;
}
static long TCPBridgeSendDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	while (result > 0)
	{
		AMsgInit(msg, AMsgType_Unknown, client->indata, sizeof(client->indata));
		msg->done = &TCPClientRecvDone;
		result = client->client->request(client->client, ARequest_Output, msg);
		if (result > 0)
		{
			//msg->data[msg->size] = '\0';
			//OutputDebugStringA(msg->data);

			msg->type |= AMsgType_Custom;
			msg->done = &TCPBridgeSendDone;
			result = client->proxy->request(client->proxy, ARequest_Input, msg);
		}
	}
	if (result < 0)
		AObjectRelease(client->client);
	return result;
}

static long TCPBridgeOpenDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	if (result < 0) {
		result = TCPProxyRelease(msg, result);
		return result;
	}

	client->client->extend = client;
	client->client->release = TCPBridgeRelease;
	AObjectAddRef(client->client);
	QueueUserWorkItem(TCPBridgeProcess, client, 0);

	AMsgInit(msg, AMsgType_Custom|client->probe_type, client->indata, client->probe_size);
	msg->done = &TCPBridgeSendDone;
	result = client->proxy->request(client->proxy, ARequest_Input, msg);
	if (result != 0)
		result = msg->done(msg, result);
	return result;
}

static long TCPProxyRecvDone(AMessage *msg, long result);
static long TCPProxySendDone(AMessage *msg, long result);
static DWORD WINAPI TCPProxyProcess(void *p)
{
	TCPClient *client = (TCPClient*)p;
	long result;
	do {
		AMsgInit(&client->inmsg, AMsgType_Unknown, NULL, 0);
		client->inmsg.done = NULL;
		result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
		if (result <= 0)
			break;

		client->inmsg.done = &TCPProxyRecvDone;
		result = client->client->request(client->client, ARequest_Output, &client->inmsg);
		if (result <= 0)
			break;

		if (client->server->sock == INVALID_SOCKET)
			break;

		client->inmsg.done = &TCPProxySendDone;
		result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
	} while (result > 0);
	if (result != 0) {
		result = TCPProxyRelease(&client->inmsg, result);
	}
	return result;
}
static long TCPProxySendDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	if (result > 0) {
		QueueUserWorkItem(TCPProxyProcess, client, 0);
	} else {
		result = TCPProxyRelease(&client->inmsg, result);
	}
	return result;
}
static long TCPProxyRecvDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	while (result > 0)
	{
		if (client->server->sock == INVALID_SOCKET)
			break;

		client->inmsg.done = &TCPProxySendDone;
		result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
		if (result <= 0)
			break;

		AMsgInit(&client->inmsg, AMsgType_Unknown, NULL, 0);
		client->inmsg.done = NULL;
		result = client->proxy->request(client->proxy, ARequest_Input, &client->inmsg);
		if (result <= 0)
			break;

		client->inmsg.done = &TCPProxyRecvDone;
		result = client->client->request(client->client, ARequest_Output, &client->inmsg);
	}
	if (result != 0) {
		result = TCPProxyRelease(&client->inmsg, result);
	}
	return result;
}
static long TCPProxyOpenDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	AMsgInit(&client->inmsg, client->probe_type, client->indata, client->probe_size);
	result = TCPProxyRecvDone(msg, result);
	return result;
}
static long TCPClientRecvProbe(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	if (result < 0) {
		result = TCPProxyRelease(&client->inmsg, result);
		return result;
	}

	client->probe_type = msg->type;
	client->probe_size = msg->size;
	//msg->data[msg->size] = '\0';
	//OutputDebugStringA(msg->data);

	msg->done = NULL;
	AModule *module = AModuleProbe("proxy", client->client, msg);

	AOption *proxy_opt = NULL;
	if (module != NULL)
		proxy_opt = AOptionFindChild(client->server->option, module->module_name);
	if ((proxy_opt == NULL) || (_stricmp(proxy_opt->value, "bridge") == 0))
	{
		if (proxy_opt == NULL)
			proxy_opt = client->server->default_bridge;
		if (proxy_opt == NULL) {
			result = -EFAULT;
		} else {
			result = client->server->io_module->create(&client->proxy, client->client, proxy_opt);
		}
		AMsgInit(msg, AMsgType_Option, (char*)proxy_opt, sizeof(*proxy_opt));
		msg->done = &TCPBridgeOpenDone;
	}
	else
	{
		result = module->create(&client->proxy, client->client, proxy_opt);
		AMsgInit(msg, AMsgType_Object, (char*)client->client, sizeof(AObject));
		msg->done = &TCPProxyOpenDone;
	}
	if (result >= 0) {
		result = client->proxy->open(client->proxy, msg);
	}
	if (result != 0)
		result = msg->done(msg, result);
	return result;
}

static long TCPClientOpenDone(AMessage *msg, long result)
{
	TCPClient *client = from_inmsg(msg);
	if (result >= 0) {
		client->sock = INVALID_SOCKET;

		AMsgInit(msg, AMsgType_Unknown, client->indata, sizeof(client->indata));
		msg->done = &TCPClientRecvProbe;
		result = client->client->request(client->client, ARequest_Output, msg);
	}
	if (result != 0) {
		result = TCPClientRecvProbe(msg, result);
	}
	return result;
}

static DWORD WINAPI TCPClientProcess(void *p)
{
	TCPClient *client = (TCPClient*)p;

	long result = client->server->io_module->create(&client->client, NULL, NULL);
	if (result >= 0) {
		AMsgInit(&client->inmsg, AMsgType_Object, (char*)client->sock, sizeof(client->sock));
		client->inmsg.done = &TCPClientOpenDone;
		result = client->client->open(client->client, &client->inmsg);
	}
	if (result != 0) {
		result = TCPClientOpenDone(&client->inmsg, result);
	}
	return result;
}

static DWORD WINAPI TCPServerProcess(void *p)
{
	TCPServer *server = to_server(p);
	struct sockaddr addr;
	int addrlen;
	SOCKET sock;

	AOption *opt = AOptionFindChild(server->option, "io");
	server->io_module = AModuleFind("io", opt?opt->value:"tcp");
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
		client->sock = sock;
		client->client = NULL;
		client->proxy = NULL;
		QueueUserWorkItem(&TCPClientProcess, client, 0);
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
	DWORD flag = 0;
	WT_SET_MAX_THREADPOOL_THREADS(flag, 8192);
	QueueUserWorkItem(&TCPServerProcess, &server->object, flag);
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
