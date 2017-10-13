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
		void  (*do_post)(AInOutComponent *c, AMessage *msg); // => _do_post()
		int   (*inmsg_done)(AMessage *msg, int result);
		int   (*outmsg_done)(AMessage *msg, int result);
	};
	void init(AObject *o, pthread_mutex_t *l) {
		AComponent::init(o, name());
		static com_module *impl = (com_module*)AModuleFind(name(), name());

		_abort = true; _queue.init(); _mutex = l;
		_io          = NULL;
		_inmsg.done  = impl->inmsg_done;
		do_post      = impl->do_post;
		_outbuf      = NULL;
		_outmsg.done = impl->outmsg_done;

		// set by inner module
		do_input = &_do_input;
		on_input = NULL; on_input_end = NULL; on_output = NULL;
		// set by outter using
		on_outmsg = NULL;
	}
	static int _do_input(AInOutComponent *c, AMessage *msg) {
		assert(msg->size != 0);
		c->_inmsg.init(ioMsgType_Block, msg->data, msg->size);
		return c->_io->input(&c->_inmsg);
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
	void _output_end() { _self->release(); }
};


#endif
