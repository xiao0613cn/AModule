#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/AInOutComponent.h"

struct EchoProxy : public AEntity {
	AInOutComponent _iocom;
};

static int EchoCreate(AObject **object, AObject *parent, AOption *option)
{
	EchoProxy *echo = (EchoProxy*)*object;
	echo->init();
	echo->_init_push(&echo->_iocom);
	return 1;
}

static void EchoRelease(AObject *object)
{
	EchoProxy *echo = (EchoProxy*)object;
	echo->_pop_exit(&echo->_iocom);
	echo->exit();
}

static int EchoProbe(AObject *object, AMessage *msg, AOption *option)
{
	if (msg->size < 4)
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
			msg->init(0, buf->next(), buf->left());
			result = echo->_iocom._io->output(msg);
		}
		else {
			buf->push(msg->size);
		}
	}
	if (result < 0)
		echo->release();
	return result;
}

static int EchoRun(AObject *object, AOption *option)
{
	EchoProxy *echo = (EchoProxy*)object;
	echo->_iocom._inmsg.init();
	echo->_iocom._outmsg.init();
	echo->_iocom._outmsg.done = &EchoMsgRun;

	echo->addref();
	return echo->_iocom._outmsg.done2(1);
}

AService EchoService = { {
	"AService",
	"EchoProxy",
	sizeof(EchoProxy),
	NULL, NULL,
	&EchoCreate,
	&EchoRelease,
	&EchoProbe, },
	NULL, NULL,
	&EchoRun,
};

static auto_reg_t reg(EchoService.module);
