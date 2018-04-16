#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../ecs/AInOutComponent.h"
#include "../ecs/AServiceComponent.h"

struct EchoProxy : public AEntity {
	AInOutComponent _iocom;
};

static int EchoCreate(AObject **object, AObject *parent, AOption *option)
{
	EchoProxy *echo = (EchoProxy*)*object;
	echo->init();
	echo->init_push(&echo->_iocom);
	return 1;
}

static void EchoRelease(AObject *object)
{
	EchoProxy *echo = (EchoProxy*)object;
	echo->pop_exit(&echo->_iocom);
	echo->exit();
}

static int EchoProbe(AObject *object, AObject *other, AMessage *msg)
{
	if ((msg == NULL) || (msg->size < 4))
		return -1;
	return ((strncasecmp_sz(msg->data, "echo") == 0) ? 80 : -1);
}

static int EchoMsgRun(AMessage *msg, int result)
{
	EchoProxy *echo = container_of(msg, EchoProxy, _iocom._outmsg);
	ARefsBuf *buf = echo->_iocom._outbuf;

	int &status = echo->_iocom._inmsg.size;
	while (result > 0)
	{
		if (buf->len() != 0) {
			status = buf->len();
			msg->init(ioMsgType_Block, buf->ptr(), buf->len());
			buf->reset();
			result = echo->_iocom._io->input(msg);
		}
		else if (status != 0) {
			status = 0;
			result = echo->_iocom._io->output(msg, buf);
		}
		else {
			buf->push(msg->size);
		}
	}
	if (result < 0)
		echo->release();
	return result;
}

static int EchoRun(AServiceComponent *service, AObject *object)
{
	EchoProxy *echo = (EchoProxy*)object;
	echo->_iocom._inmsg.init();
	echo->_iocom._outmsg.init();
	echo->_iocom._outmsg.done = &EchoMsgRun;

	echo->addref();
	return echo->_iocom._outmsg.done2(1);
}

AModule EchoModule = {
	"AEntity",
	"EchoProxy",
	sizeof(EchoProxy),
	NULL, NULL,
	&EchoCreate,
	&EchoRelease,
	&EchoProbe,
};

static int reg_echo = AModuleRegister(&EchoModule);


//////////////////////////////////////////////////////////////////////////

struct EchoService : public AEntity {
	AServiceComponent _service;
};

static int EchoServiceCreate(AObject **object, AObject *parent, AOption *option)
{
	EchoService *echod = (EchoService*)*object;
	echod->init();
	echod->init_push(&echod->_service);
	echod->_service._peer_module = &EchoModule;
	echod->_service.run = &EchoRun;
	return 1;
}

static void EchoServiceRelease(AObject *object)
{
	EchoService *echod = (EchoService*)object;
	echod->pop_exit(&echod->_service);
	echod->exit();
}

AModule EchoServiceModule = {
	AServiceComponent::name(),
	"EchoService",
	sizeof(EchoService),
	NULL, NULL,
	&EchoServiceCreate,
	&EchoServiceRelease,
	&EchoProbe,
};

static int reg_svc = AModuleRegister(&EchoServiceModule);
