#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"


struct AInOutComponent : public AComponent {
	static const char* name() { return "AInOutComponent"; }

	bool volatile    _abort;
	struct list_head _queue;
	pthread_mutex_t *_mutex;
	void lock() { pthread_mutex_lock(_mutex); }
	void unlock() { pthread_mutex_unlock(_mutex); }

	IOObject *_io;
	AMessage  _inmsg;
	void  (*do_input)(AInOutComponent *c, AMessage *msg); // => _do_input()
	void  (*on_endqu)(AInOutComponent *c, int result);

	AMessage  _outmsg;
	ARefsBuf *_outbuf;
	int   (*on_output)(AInOutComponent *c, int result);

	void init(AObject *o) {
		AComponent::init(o, name());
		_abort = true;   _queue.init();
		//_mutex         on_endqu
		_io = NULL;      do_input = &_do_input;
		_outbuf = NULL;  //on_output = NULL;
	}
	void post(AMessage *msg) {
		do_input(this, msg);
	}

	static void _do_input(AInOutComponent *c, AMessage *msg) {
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

		c->_inmsg.init(msg);
		c->_inmsg.done = _inmsg_done;
		c->_self->addref();

		int result = c->_io->input(&c->_inmsg);
		if (result != 0)
			_inmsg_done(&c->_inmsg, result);
	}
	static int _inmsg_done(AMessage *msg, int result) {
		AInOutComponent *c = container_of(msg, AInOutComponent, _inmsg);
		for (;;) {
			AMessage *next = NULL;
			if (result < 0)
				c->_abort = true;

			c->lock();
			msg = list_pop_front(&c->_queue, AMessage, entry);

			if (!c->_queue.empty())
				next = list_entry(c->_queue.next, AMessage, entry);
			else if (c->_abort)
				c->on_endqu(c, result);
			c->unlock();

			msg->done2(result);
			if (next == NULL) {
				c->_self->release();
				return result;
			}

			c->_inmsg.init(next);
			result = c->_io->input(&c->_inmsg);
			if (result == 0)
				return 0;
		}
	}
	void _outmsg_cycle(int ressiz, int bufsiz) {
		int result = ARefsBufCheck(_outbuf, ressiz, bufsiz);
		_outmsg.init();
		_outmsg.done = &_outmsg_done;
		_outmsg_done(&_outmsg, result);
	}
	static int _outmsg_done(AMessage *msg, int result) {
		AInOutComponent *c = container_of(msg, AInOutComponent, _outmsg);
		for (;;) {
			if (result >= 0)
				c->_outbuf->push(c->_outmsg.size);
			result = c->on_output(c, result);
			if (result < 0)
				return result;

			if (result > 0) {
				c->_outbuf->pop(result);
				result = ARefsBufCheck(c->_outbuf, 2048, c->_outbuf->size);
				if (result < 0)
					continue;
			}

			result = c->_io->output(&c->_outmsg, c->_outbuf);
			if (result == 0)
				return 0;
		}
	}
};


#endif
