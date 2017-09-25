#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"

typedef struct AInputQueueComponent AInputQueueComponent;

struct AInputQueueComponent : public AComponent {
	static const char* name() { return "AInputQueueComponent"; }

	bool    _abort;
	struct list_head _queue;
	pthread_mutex_t *_mutex;
	void  (*on_empty)(AInputQueueComponent *c);
	void  (*on_error)(AInputQueueComponent *c, AMessage *msg, int result);

	AMessage  _msg;
	IOObject *_io;
	void  (*do_input)(AInputQueueComponent *c, AMessage *msg); // => _input()

	void init(AObject *o) {
		AComponent::init(o, name());
		_abort = false; _queue.init(); //_mutex,
		on_empty = NULL; on_error = NULL;
		_io = NULL; do_input = &_do_input;
	}
	void post(AMessage *msg) {
		if (_abort) {
			msg->done2(-EINTR);
			return;
		}

		pthread_mutex_lock(_mutex);
		bool first = list_empty(&_queue);
		list_add_tail(&msg->entry, &_queue);
		pthread_mutex_unlock(_mutex);

		if (first)
			do_input(this, msg);
	}
	static void _do_input(AInputQueueComponent *c, AMessage *msg) {
		assert(&msg->entry == c->_queue.next);

		c->_msg.init(msg);
		c->_msg.done = _msg_done;
		c->_self->addref();

		int result = c->_io->input(&c->_msg);
		if (result != 0)
			c->_msg.done2(result);
	}
	static int _msg_done(AMessage *msg, int result) {
		AInputQueueComponent *c = container_of(msg, AInputQueueComponent, _msg);
		for (;;) {
			AMessage *next = NULL;

			pthread_mutex_lock(c->_mutex);
			msg = list_pop_front(&c->_queue, AMessage, entry);
			if (!list_empty(&c->_queue))
				next = list_first_entry(&c->_queue, AMessage, entry);
			else if (c->on_empty != NULL)
				c->on_empty(c);
			pthread_mutex_unlock(c->_mutex);

			if ((result < 0) && (c->on_error != NULL))
				c->on_error(c, msg, result);
			msg->done2(result);

			if (next == NULL) {
				c->_self->release();
				return result;
			}
			if (c->_abort) {
				result = -EINTR;
				continue;
			}

			c->_msg.init(next);
			result = c->_io->input(&c->_msg);
			if (result == 0)
				return 0;
		}
	}
};


#endif
