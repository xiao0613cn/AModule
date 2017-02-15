#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"


static struct HTTPRequest {
	const char *method;
	int         length;
} HTTPRequests[] = {
#define HTTPREQ(method) { method, sizeof(method)-1 }
	HTTPREQ("GET"), HTTPREQ("HEAD"), HTTPREQ("POST"), HTTPREQ("PUT"),
	HTTPREQ("DELETE"), HTTPREQ("TRACE"), HTTPREQ("OPTIONS"),
	HTTPREQ("CONNECT"), HTTPREQ("PATCH"), { NULL, 0 }
};

static int HTTPProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->size < 10)
		return -1;

	for (int ix = 0; HTTPRequests[ix].method != NULL; ++ix) {
		if ((_strnicmp(msg->data,HTTPRequests[ix].method,HTTPRequests[ix].length) == 0)
		 && (msg->data[HTTPRequests[ix].length] == ' '))
			return 60;
	}
	return -1;
}

static void HttpConnectCreate(AObject *object)
{
	HttpConnect *conn = to_conn(object);
	release_s(conn->io, AObjectRelease, NULL);
}

static int HTTPProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpConnect *conn = (HttpConnect*)*object;
	conn->io = NULL;
	conn->session = NULL;

	int result = AObjectCreate(&conn->io, parent, option);
	if (result < 0)
		return result;
	return result;
}

static int HTTPProxyOutputTo(AMessage *msg, int result);
static int HTTPProxyInputFrom(AMessage *msg, int result)
{
	HTTPProxy *proxy = from_outmsg(msg);
	while (result > 0)
	{
		proxy->outmsg.done = &HTTPProxyOutputTo;
		result = ioOutput(proxy->to, &proxy->outmsg, proxy->outdata, sizeof(proxy->outdata));
		if (result > 0)
		{
			//proxy->outmsg.data[proxy->outmsg.size] = '\0';
			//OutputDebugStringA(proxy->outmsg.data);

			proxy->outmsg.type |= AMsgType_Private;
			proxy->outmsg.done = &HTTPProxyInputFrom;
			result = ioInput(proxy->from, &proxy->outmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}
static int HTTPProxyOutputTo(AMessage *msg, int result)
{
	HTTPProxy *proxy = from_outmsg(msg);
	while (result > 0)
	{
		proxy->outmsg.type |= AMsgType_Private;
		proxy->outmsg.done = &HTTPProxyInputFrom;
		result = ioInput(proxy->from, &proxy->outmsg);
		if (result > 0)
		{
			proxy->outmsg.done = &HTTPProxyOutputTo;
			result = ioOutput(proxy->to, &proxy->outmsg, proxy->outdata, sizeof(proxy->outdata));
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}

static void HTTPProxy_OutputTo_InputFrom(AOperator *asop, int result)
{
	HTTPProxy *proxy = container_of(asop, HTTPProxy, asop);
	result = HTTPProxyInputFrom(&proxy->outmsg, 1);
}

static int HTTPProxyOutputFrom(AMessage *msg, int result);
static int HTTPProxyInputTo(AMessage *msg, int result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	while (result > 0)
	{
		proxy->inmsg.done = &HTTPProxyOutputFrom;
		result = ioOutput(proxy->from, &proxy->inmsg, proxy->indata, sizeof(proxy->indata));
		if (result > 0)
		{
			proxy->inmsg.type |= AMsgType_Private;
			proxy->inmsg.done = &HTTPProxyInputTo;
			result = ioInput(proxy->to, &proxy->inmsg);
		}
	}
	if (result != 0)
		AObjectRelease(&proxy->object);
	return result;
}

static int HTTPProxyOutputFrom(AMessage *msg, int result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	while (result > 0)
	{
		//proxy->inmsg.data[proxy->inmsg.size] = '\0';
		//OutputDebugStringA(proxy->inmsg.data);

		proxy->inmsg.type |= AMsgType_Private;
		proxy->inmsg.done = &HTTPProxyInputTo;
		result = ioInput(proxy->to, &proxy->inmsg);
		if (result > 0)
		{
			proxy->inmsg.done = &HTTPProxyOutputFrom;
			result = ioOutput(proxy->from, &proxy->inmsg, proxy->indata, sizeof(proxy->indata));
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
	proxy->asop.callback = &HTTPProxy_OutputTo_InputFrom;
	AOperatorTimewait(&proxy->asop, NULL, 0);

	AMsgInit(&proxy->inmsg, AMsgType_Unknown, proxy->indata, sizeof(proxy->indata));
	AMsgCopy(&proxy->inmsg, msg->type, msg->data, msg->size);

	proxy->inmsg.data[proxy->inmsg.size] = '\0';
	fputs(proxy->inmsg.data, stdout);

	AObjectAddRef(&proxy->object);
	HTTPProxyOutputFrom(&proxy->inmsg, 1);
}

static int HTTPProxyOpenDone(AMessage *msg, int result)
{
	HTTPProxy *proxy = from_inmsg(msg);
	if (result > 0) {
		HTTPProxy_OutputFrom_InputTo(proxy);
	}
	result = proxy->openmsg->done(proxy->openmsg, result);
	return result;
}

static int HTTPProxyOpen(AObject *object, AMessage *msg)
{
	HTTPProxy *proxy = to_proxy(object);

	proxy->openmsg = msg;
	proxy->inmsg.init(proxy->option);
	proxy->inmsg.done = &HTTPProxyOpenDone;

	int result = proxy->to->open(proxy->to, &proxy->inmsg);
	if (result > 0) {
		HTTPProxy_OutputFrom_InputTo(proxy);
	}
	return result;
}

static int HTTPProxyClose(AObject *object, AMessage *msg)
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
