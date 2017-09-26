#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"

typedef struct AInputQueueComponent AInputQueueComponent;

struct AInputQueueComponent : public AComponent {
	static const char* name() { return "AInputQueueComponent"; }

	bool volatile    _abort;
	struct list_head _queue;
	pthread_mutex_t *_mutex;
	void  (*on_empty)(AInputQueueComponent *c);
	void  (*on_error)(AInputQueueComponent *c, AMessage *msg, int result);

	AMessage  _msg;
	IOObject *_io;
	void  (*do_post)(AInputQueueComponent *c, AMessage *msg); // => _do_post()

	void init(AObject *o) {
		AComponent::init(o, name());
		_abort = true; _queue.init(); //_mutex,

		on_empty = NULL; on_error = NULL;
		_io = NULL; do_post = &_do_post;
	}

	void post(AMessage *msg) {
		do_post(this, msg);
	}
	static void _do_post(AInputQueueComponent *c, AMessage *msg) {
		if (c->_abort) {
			msg->done2(-EINTR);
			return;
		}

		pthread_mutex_lock(c->_mutex);
		bool first = list_empty(&c->_queue);
		list_add_tail(&msg->entry, &c->_queue);
		pthread_mutex_unlock(c->_mutex);
		if (!first)
			return;

		c->_msg.init(msg);
		c->_msg.done = _msg_done;
		c->_self->addref();

		int result = c->_io->input(&c->_msg);
		if (result != 0)
			_msg_done(&_msg, result);
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
