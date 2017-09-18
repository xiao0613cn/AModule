#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"

typedef struct AInputQueueComponent AInputQueueComponent;

struct AInputQueueComponent : public AComponent {
	static const char* name() { return "AInputQueueComponent"; }

	struct list_head _queue;
	pthread_mutex_t *_mutex;
	AMessage _msg;
	AObject *_io;
	bool     _abort;

	void post(AMessage *msg) {
		if (_abort) {
			msg->done2(-EINTR);
			return;
		}

		pthread_mutex_lock(_mutex);
		bool first = list_empty(&_queue);
		list_add_tail(&msg->entry, &_queue);
		pthread_mutex_unlock(_mutex);
		if (!first)
			return;

		_msg.init(msg);
		_msg.done = &MsgDone(AInputQueueComponent, _msg, done);

		_self->addref();
		int result = _io->request(Aio_Input, &_msg);
		if (result != 0)
			_msg.done2(result);
	}
	int  done(int result) {
		for (;;) {
			AMessage *next = NULL;

			pthread_mutex_lock(_mutex);
			AMessage *msg = list_pop_front(&_queue, AMessage, entry);
			if (!list_empty(&_queue))
				next = list_first_entry(&_queue, AMessage, entry);
			pthread_mutex_unlock(_mutex);

			msg->done2(result);
			if (next == NULL) {
				_self->release2();
				return result;
			}
			if (_abort) {
				result = -EINTR;
				continue;
			}

			_msg.init(next);
			result = _io->request(Aio_Input, &_msg);
			if (result == 0)
				return 0;
		}
	}
};


#endif
