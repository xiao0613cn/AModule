#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"


struct AInOutComponent : public AComponent {
	static const char* name() { return "AInOutComponent"; }

	// used by outter
	void    post(AMessage *msg) { do_post(this, msg); }

	// implement by inner...
	bool volatile    _abort;
	struct list_head _queue;
	pthread_mutex_t *_mutex;
	void lock() { pthread_mutex_lock(_mutex); }
	void unlock() { pthread_mutex_unlock(_mutex); }

	IOObject *_io;
	AMessage  _inmsg;
	void  (*do_post)(AInOutComponent *c, AMessage *msg); // => _do_post()
	int   (*do_input)(AInOutComponent *c, AMessage *msg); // => _do_input()
	void  (*on_post_end)(AInOutComponent *c, int result);

	AMessage  _outmsg;
	ARefsBuf *_outbuf;
	int   (*on_output)(AInOutComponent *c, int result);
	void     *_outuser;

	struct com_module {
		AModule module;
		void  (*do_post)(AInOutComponent *c, AMessage *msg);
		int   (*inmsg_done)(AMessage *msg, int result);
		int   (*outmsg_done)(AMessage *msg, int result);
	};
	void init2() {
		//static com_module *impl = (com_module*)AModuleFind(name(), name());
		// set by inner module
		_abort = true; _queue.init(); _mutex = NULL;
		_io          = NULL;
		_inmsg.done  = &_inmsg_done;  // = impl.inmsg_done;
		do_post      = &_do_post;     // = impl.do_post;
		do_input     = &_do_input;    // = impl.do_input;
		on_post_end  = NULL;
		_outbuf      = NULL;
		_outmsg.done = &_outmsg_done; // = impl.outmsg_done;

		on_output = NULL;             // set by decoder
		_outuser = NULL;              // set by decoder
	}
	void exit2() {
		assert(_queue.empty());
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
		//assert(c->_inmsg.done == &_inmsg_done);

		c->_object->addref();
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
			AMessage *next = NULL;

			c->lock();
			msg = list_pop_front(&c->_queue, AMessage, entry);
			if (!c->_queue.empty()) {
				next = list_first_entry(&c->_queue, AMessage, entry);
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
	void _input_begin(void(*post_end)(AInOutComponent*,int)) {
		lock();
		assert(_queue.empty() && (on_post_end == NULL));
		_abort = false;
		on_post_end = post_end;
		unlock();
	}
	void _input_end(int result) {
		lock();
		_abort = true;
		if (_queue.empty() && (on_post_end != NULL)) {
			on_post_end(this, result);
			on_post_end = NULL;
		}
		unlock();
	}
	int _output_cycle(int ressiz, int bufsiz) {
		_object->addref();
		int result = ARefsBuf::reserve(_outbuf, ressiz, bufsiz);
		_outmsg.init();
		return _outmsg.done2(result);
	}
	static int _outmsg_done(AMessage *msg, int result) {
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
};


#endif
