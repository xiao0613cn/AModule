#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AInOutComponent.h"


static int _inmsg_done(AMessage *msg, int result)
{
	AInOutComponent *c = container_of(msg, AInOutComponent, _inmsg);
	for (;;) {
		msg = list_first_entry(&c->_queue, AMessage, entry);
		if ((c->on_input != NULL) && (c->_inmsg.data != NULL))
			c->on_input(c, msg, result);

		AMessage *next = NULL;
		c->lock();
		msg->entry.leave();
		if (!c->_queue.empty()) {
			next = list_first_entry(&c->_queue, AMessage, entry);
		} else if (c->_abort) {
			c->on_input_end(c, result);
			c->on_input_end = NULL;
		}
		c->unlock();

		msg->done2(result);
		if (next == NULL) {
			c->_self->release();
			return result;
		}

		if (c->_abort) {
			c->_inmsg.init();
		} else {
			result = c->do_input(c, next);
			if (result == 0)
				return 0;
		}
	}
}

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

	c->_inmsg.done = _inmsg_done;
	c->_self->addref();

	int result = c->do_input(c, msg);
	if (result != 0)
		c->_inmsg.done2(result);
}

static int _outmsg_done(AMessage *msg, int result)
{
	AInOutComponent *c = container_of(msg, AInOutComponent, _outmsg);
	for (;;) {
		if (result >= 0)
			c->_outbuf->push(c->_outmsg.size);
		result = c->on_output(c, result);
		if (result < 0)
			return result;

		if ((result > 0) && (c->on_outmsg != NULL)) {
			c->on_outmsg(c, &c->_outmsg, result);
		}
		result = ARefsBuf::reserve(c->_outbuf, 2048, c->_outbuf->_size);
		if (result < 0)
			continue;

		result = c->_io->output(&c->_outmsg, c->_outbuf);
		if (result == 0)
			return 0;
	}
}

AInOutComponent::com_module InOutModule = { {
	AInOutComponent::name(),
	AInOutComponent::name(), },
	&_do_post,
	&_inmsg_done,
	&_outmsg_done,
};

static auto_reg_t reg(InOutModule.module);
