#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/AModule_io.h"

//struct EchoProxy {
//	AObject   object;
//};

static long EchoCreate(AObject **object, AObject *parent, AOption *option)
{
	AObject *echo = (AObject*)malloc(sizeof(AObject));
	if (echo == NULL)
		return -ENOMEM;

	extern AModule EchoModule;
	AObjectInit(echo, &EchoModule);

	*object = echo;
	return 1;
}

static void EchoRelease(AObject *echo)
{
	free(echo);
}

static long EchoProbe(AObject *object, AMessage *msg)
{
	if (msg->size < 4)
		return -1;
	return ((_strnicmp(msg->data, "echo", 4) == 0) ? 100 : 0);
}

static long EchoOpen(AObject *echo, AMessage *msg)
{
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AObjectAddRef((AObject*)msg->data);
	echo->extend = msg->data;
	return 1;
}

static long EchoRequest(AObject *echo, long reqix, AMessage *msg)
{
	if (reqix != Aio_Input)
		return -ENOSYS;
	if (msg->data == NULL)
		return -ENOSYS;
	return ((AObject*)echo->extend)->request((AObject*)echo->extend, reqix, msg);
}

static long EchoCancel(AObject *echo, long reqix, AMessage *msg)
{
	if (reqix != Aio_Input)
		return -ENOSYS;
	release_s(*((AObject**)&echo->extend), AObjectRelease, NULL);
	return 1;
}

static long EchoClose(AObject *echo, AMessage *msg)
{
	release_s(*((AObject**)&echo->extend), AObjectRelease, NULL);
	return 1;
}

AModule EchoModule = {
	"proxy",
	"EchoProxy",
	sizeof(AObject),
	NULL, NULL,
	&EchoCreate,
	&EchoRelease,
	&EchoProbe,
	0,
	&EchoOpen,
	NULL, NULL,
	&EchoRequest,
	&EchoCancel,
	&EchoClose,
};
