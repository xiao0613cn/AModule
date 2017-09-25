#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"

struct EchoProxy : public IOObject {
	IOObject  *client;
};

static int EchoCreate(AObject **object, AObject *parent, AOption *option)
{
	EchoProxy *echo = (EchoProxy*)*object;
	echo->client = NULL;
	return 1;
}

static void EchoRelease(AObject *object)
{
	EchoProxy *echo = (EchoProxy*)object;
	release_s(echo->client, AObjectRelease, NULL);
}

static int EchoProbe(AObject *object, AMessage *msg)
{
	if (msg->size < 4)
		return -1;
	return ((strncasecmp_sz(msg->data, "echo") == 0) ? 100 : 0);
}

static int EchoOpen(AObject *object, AMessage *msg)
{
	EchoProxy *echo = (EchoProxy*)object;
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	echo->client = (IOObject*)msg->data;
	echo->client->addref();
	return 1;
}

static int EchoRequest(AObject *object, int reqix, AMessage *msg)
{
	EchoProxy *echo = (EchoProxy*)object;
	if (reqix != Aio_Input)
		return -ENOSYS;
	if (msg->size == 0)
		return -ENOSYS;
	return (*echo->client)->request(echo->client, reqix, msg);
}

static int EchoCancel(AObject *object, int reqix, AMessage *msg)
{
	EchoProxy *echo = (EchoProxy*)object;
	int result = 1;

	if (echo->client != NULL)
		result = (*echo->client)->cancel(echo->client, reqix, msg);
	return result;
}

static int EchoClose(AObject *object, AMessage *msg)
{
	EchoProxy *echo = (EchoProxy*)object;
	release_s(echo->client, AObjectRelease, NULL);
	return 1;
}

IOModule EchoModule = { {
	"proxy",
	"EchoProxy",
	sizeof(AObject),
	NULL, NULL,
	&EchoCreate,
	&EchoRelease,
	&EchoProbe, },
	&EchoOpen,
	NULL, NULL,
	&EchoRequest,
	&EchoCancel,
	&EchoClose,
};

static auto_reg_t reg(EchoModule.module);
