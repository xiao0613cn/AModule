#include "stdafx.h"
#include "../base/AModule.h"


static struct HTTPRequest {
	const char *method;
	long        length;
} HTTPRequests[] = {
#define HTTPREQ(method) { method, sizeof(method)-1 }
	HTTPREQ("GET"), HTTPREQ("HEAD"), HTTPREQ("POST"), HTTPREQ("PUT"),
	HTTPREQ("DELETE"), HTTPREQ("TRACE"), HTTPREQ("OPTIONS"),
	HTTPREQ("CONNECT"), HTTPREQ("PATCH"), { NULL, 0 }
};

static long HTTPProxyProbe(AObject *object, const char *data, long size)
{
	if (size < 10)
		return -1;

	for (int ix = 0; HTTPRequests[ix].method != NULL; ++ix) {
		if ((_strnicmp(data,HTTPRequests[ix].method,HTTPRequests[ix].length) == 0)
		 && (data[HTTPRequests[ix].length] == ' '))
			return 0;
	}
	return -1;
}

struct HTTPProxy {
	AObject   object;
	AObject  *from;
	AObject  *to;
	AOption  *option;
	AMessage *openmsg;

	AMessage  inmsg;
	char      indata[2048];

	AMessage  outmsg;
	char      outdata[2048];
};
#define to_proxy(obj) container_of(obj, HTTPProxy, object)
#define from_inmsg(msg) container_of(msg, HTTPProxy, inmsg)
#define from_outmsg(msg) container_of(msg, HTTPProxy, outmsg)

static void HTTPProxyRelease(AObject *object)
{
	HTTPProxy *proxy = to_proxy(object);
	release_s(proxy->from, AObjectRelease, NULL);
	release_s(proxy->to, AObjectRelease, NULL);
	free(proxy);
}

static long HTTPProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	HTTPProxy *proxy = (HTTPProxy*)malloc(sizeof(HTTPProxy));
	if (proxy == NULL)
		return -ENOMEM;

	extern AModule HTTPProxyModule;
	AObjectInit(&proxy->object, &HTTPProxyModule);
	proxy->from = parent; AObjectAddRef(parent);
	proxy->to = NULL;
	proxy->option = AOptionFindChild(option, "proxy");
	*object = &proxy->object;

	AOption *io_opt = AOptionFindChild(option, "io");
	long result = AObjectCreate(&proxy->to, &proxy->object, io_opt, "tcp");
	return result;
}

static long HTTPProxyOutputTo(AMessage *msg, long result);
static long HTTPProxyInputFrom(AMessage *msg, long result)
{
	HTTPProxy *proxy = from_outmsg(msg);
	while (result > 0)
	{
		proxy->outmsg.done = HTTPProxyOutputTo;
		AMsgInit(&proxy->outmsg, AMsgType_Unknown, proxy->outdata, sizeof(proxy->outdata));
		result = proxy->to->request(proxy->to, ARequest_Output, &proxy->outmsg);
		if (result > 0)
		{
			proxy->outmsg.done = HTTPProxyInputFrom;
			proxy->outmsg.type = AMsgType_Custom;
			result = proxy->from->request(proxy->from, ARequest_Input, &proxy->outmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}
static long HTTPProxyOutputTo(AMessage *msg, long result)
{
	HTTPProxy *proxy = from_outmsg(msg);
	while (result > 0)
	{
		proxy->outmsg.done = HTTPProxyInputFrom;
		proxy->outmsg.type = AMsgType_Custom;
		result = proxy->from->request(proxy->from, ARequest_Input, &proxy->outmsg);
		if (result > 0)
		{
			proxy->outmsg.done = HTTPProxyOutputTo;
			AMsgInit(&proxy->outmsg, AMsgType_Unknown, proxy->outdata, sizeof(proxy->outdata));
			result = proxy->to->request(proxy->to, ARequest_Output, &proxy->outmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}

static DWORD WINAPI HTTPProxy_OutputTo_InputFrom(void *p)
{
	HTTPProxy *proxy = to_proxy(p);
	long result = HTTPProxyInputFrom(&proxy->outmsg, 1);
	return result;
}

static long HTTPProxyOutputFrom(AMessage *msg, long result);
static long HTTPProxyInputTo(AMessage *msg, long result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	while (result > 0)
	{
		proxy->inmsg.done = &HTTPProxyOutputFrom;
		AMsgInit(&proxy->inmsg, AMsgType_Unknown, proxy->indata, sizeof(proxy->indata));
		result = proxy->from->request(proxy->from, ARequest_Output, &proxy->inmsg);
		if (result > 0)
		{
			proxy->inmsg.done = &HTTPProxyInputTo;
			proxy->inmsg.type = AMsgType_Custom;
			result = proxy->to->request(proxy->to, ARequest_Input, &proxy->inmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}

static long HTTPProxyOutputFrom(AMessage *msg, long result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	while (result > 0)
	{
		proxy->inmsg.done = &HTTPProxyInputTo;
		proxy->inmsg.type = AMsgType_Custom;
		result = proxy->to->request(proxy->to, ARequest_Input, &proxy->inmsg);
		if (result > 0)
		{
			proxy->inmsg.done = &HTTPProxyOutputFrom;
			AMsgInit(&proxy->inmsg, AMsgType_Unknown, proxy->indata, sizeof(proxy->indata));
			result = proxy->from->request(proxy->from, ARequest_Output, &proxy->inmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}

static void HTTPProxy_OutputFrom_InputTo(HTTPProxy *proxy)
{
	AMessage *msg = proxy->openmsg;
	proxy->openmsg = NULL;

	AObjectAddRef(&proxy->object);
	QueueUserWorkItem(HTTPProxy_OutputTo_InputFrom, &proxy->object, 0);

	AMsgInit(&proxy->inmsg, AMsgType_Unknown, proxy->indata, sizeof(proxy->indata));
	AMsgCopy(&proxy->inmsg, msg->type, msg->data, msg->size);
	msg->data[128] = '\0';
	TRACE("request: %s\n", msg->data);

	AObjectAddRef(&proxy->object);
	HTTPProxyOutputFrom(&proxy->inmsg, 1);
}

static long HTTPProxyOpenDone(AMessage *msg, long result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	if (result > 0) {
		HTTPProxy_OutputFrom_InputTo(proxy);
	}
	result = proxy->openmsg->done(proxy->openmsg, result);
	return result;
}

static long HTTPProxyOpen(AObject *object, AMessage *msg)
{
	HTTPProxy *proxy = to_proxy(object);

	proxy->openmsg = msg;
	proxy->inmsg.done = &HTTPProxyOpenDone;

	AMsgInit(&proxy->inmsg, AMsgType_Option, (char*)proxy->option, sizeof(AOption));
	long result = proxy->to->open(proxy->to, &proxy->inmsg);
	if (result > 0) {
		HTTPProxy_OutputFrom_InputTo(proxy);
	}
	return result;
}

static long HTTPProxyClose(AObject *object, AMessage *msg)
{
	HTTPProxy *proxy = to_proxy(object);
	if (proxy->from != NULL)
		proxy->from->close(proxy->from, NULL);
	if (proxy->to != NULL)
		proxy->to->close(proxy->to, NULL);
	return 1;
}

AModule HTTPProxyModule = {
	"proxy",
	"HTTPProxy",
	sizeof(HTTPProxy),
	NULL, NULL,
	&HTTPProxyCreate,
	&HTTPProxyRelease,
	&HTTPProxyProbe,
	0,
	&HTTPProxyOpen,
	NULL, NULL,
	NULL, NULL,
	&HTTPProxyClose,
};
