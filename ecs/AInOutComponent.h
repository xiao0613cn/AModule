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
	void  (*do_post)(AInOutComponent *c, AMessage *msg); // => _do_post()
	int   (*do_input)(AInOutComponent *c, AMessage *msg); // => _do_input()
	void  (*on_input)(AInOutComponent *c, AMessage *msg, int result); // = NULL
	void  (*on_input_end)(AInOutComponent *c, int result);

	AMessage  _outmsg;
	ARefsBuf *_outbuf;
	int   (*on_output)(AInOutComponent *c, int result);

	// using by outter
	void    post(AMessage *msg) { do_post(this, msg); }
	void  (*on_outmsg)(AInOutComponent *c, AMessage *msg, int result);

	// implement by inner...
	struct com_module {
		AModule module;
		void  (*do_post)(AInOutComponent *c, AMessage *msg);
		int   (*inmsg_done)(AMessage *msg, int result);
		int   (*outmsg_done)(AMessage *msg, int result);
	};
	void init(AEntity2 *e, pthread_mutex_t *l) {
		AComponent::init2(e, name());
		//static com_module *impl = (com_module*)AModuleFind(name(), name());

		_abort = true; _queue.init(); _mutex = l;
		_io          = NULL;
		_inmsg.done  = &_inmsg_done;
		do_post      = &_do_post;
		_outbuf      = NULL;
		_outmsg.done = &_outmsg_done;

		// set by inner module
		do_input = &_do_input;
		on_input = NULL; on_input_end = NULL; on_output = NULL;
		// set by outter using
		on_outmsg = NULL;
	}
	void exit() {
		assert(_queue.empty());
		_entity->_pop(this);
		release_s(_io);
		release_s(_outbuf);
	}
	static void _do_post(AInOutComponent *c, AMessage *msg) {
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
	static int _do_input(AInOutComponent *c, AMessage *msg) {
		assert(msg->size != 0);
		c->_inmsg.init(msg);
		return c->_io->input(&c->_inmsg);
	}
	static int _inmsg_done(AMessage *msg, int result) {
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
	void _input_begin(void  (*input_end)(AInOutComponent *c, int result)) {
		lock();
		assert(_queue.empty());
		_abort = false;
		on_input_end = input_end;
		unlock();
	}
	void _input_end(int result) {
		lock();
		_abort = true;
		if (_queue.empty() && (on_input_end != NULL)) {
			on_input_end(this, result);
			on_input_end = NULL;
		}
		unlock();
	}
	void _output_begin(int ressiz, int bufsiz) {
		_self->addref();
		int result = ARefsBuf::reserve(_outbuf, ressiz, bufsiz);
		_outmsg.init();
		_outmsg.done2(result);
	}
	void _output_end() {
		// called by on_output()
		_self->release();
	}
	static int _outmsg_done(AMessage *msg, int result) {
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
};


#endif
