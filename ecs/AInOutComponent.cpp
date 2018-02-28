#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AInOutComponent.h"


static void _do_post(AInOutComponent *c, AMessage *msg)
{
	c->lock();
	if (c->_abort) {
		c->unlock();
		msg->done2(-EINTR);
		return;
	}

	bool first = c->_queue.empty();
	c->_queue.push_back(&msg->entry);
	c->unlock();
	if (!first)
		return;
	//assert(c->_inmsg.done == &_inmsg_done);

	c->_object->addref();
	int result = c->do_input(c, msg);
	if (result != 0)
		c->_inmsg.done2(result);
}

static int _do_input(AInOutComponent *c, AMessage *msg)
{
	assert(msg->size != 0);
	c->_inmsg.init(msg);
	return c->_io->input(&c->_inmsg);
}

static int _inmsg_done(AMessage *msg, int result)
{
	AInOutComponent *c = container_of(msg, AInOutComponent, _inmsg);
	for (;;) {
		AMessage *next = NULL;

		c->lock();
		msg = AMessage::first(c->_queue);
		msg->entry.leave();

		if (!c->_queue.empty()) {
			next = AMessage::first(c->_queue);
		} else if (c->_abort && (c->on_post_end != NULL)) {
			c->on_post_end(c, result);
			c->on_post_end = NULL;
		}
		c->unlock();

		msg->done2(result);
		if (next == NULL) {
			c->_object->release();
			return result;
		}

		if (c->_abort) {
			c->_inmsg.init();
			result = -EINTR;
		} else {
			result = c->do_input(c, next);
			if (result == 0)
				return 0;
		}
	}
}

static int _outmsg_done(AMessage *msg, int result)
{
	AInOutComponent *c = container_of(msg, AInOutComponent, _outmsg);
	for (;;) {
		if (result >= 0)
			c->_outbuf->push(c->_outmsg.size);
		if ((result < 0) || (c->_outbuf->len() != 0)) {
			result = c->on_output(c, result);
			if (result == 0)
				return 0;
		}
		if ((result < 0) || (result >= AMsgType_Class)) {
			c->_object->release();
			return result;
		}
		result = ARefsBuf::reserve(c->_outbuf, 512, c->_outbuf->_size);
		if (result >= 0) {
			result = c->_io->output(&c->_outmsg, c->_outbuf);
			if (result == 0)
				return 0;
		}
	}
}

AInOutModule InOutModule = { {
	AInOutComponent::name(),
	AInOutComponent::name(),
	0, NULL, NULL,
},
	&_do_post,
	&_do_input,
	&_inmsg_done,
	&_outmsg_done,
};
static int reg_iocom = AModuleRegister(&InOutModule.module);
