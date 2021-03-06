#ifndef _AInputQUEUE_COMPONENT_H_
#define _AInputQUEUE_COMPONENT_H_

#include "AEntity.h"
#include "../io/AModule_io.h"

typedef struct AInOutComponent AInOutComponent;

struct AInOutModule {
	AModule module;
	void  (*do_post)(AInOutComponent *c, AMessage *msg);
	int   (*do_input)(AInOutComponent *c, AMessage *msg);
	int   (*inmsg_done)(AMessage *msg, int result);
	int   (*outmsg_done)(AMessage *msg, int result);
};

struct AInOutComponent : public AComponent {
	static const char* name() { return "AInOutComponent"; }
	AMODULE_GET(AInOutModule, name(), name())

	// implement by inner...
	bool volatile    _abort;
	struct list_head _queue;
	pthread_mutex_t *_mutex;
	void lock() { pthread_mutex_lock(_mutex); }
	void unlock() { pthread_mutex_unlock(_mutex); }

	IOObject *_io;
	AMessage  _inmsg;
	void  (*do_post)(AInOutComponent *c, AMessage *msg);
	int   (*do_input)(AInOutComponent *c, AMessage *msg);
	void  (*on_post_end)(AInOutComponent *c, int result);

	AMessage  _outmsg;
	ARefsBuf *_outbuf;
	int   (*on_output)(AInOutComponent *c, int result);
	void   *on_output_userdata;

	void init2() {
		// set by inner module
		_io = NULL;
		_abort = true; _queue.init(); _mutex = NULL;

		_inmsg.done  = get()->inmsg_done;
		do_post      = get()->do_post;
		do_input     = get()->do_input;
		on_post_end  = NULL;

		_outmsg.done = get()->outmsg_done;
		_outbuf      = NULL;
		on_output    = NULL;
		on_output_userdata = NULL;
	}
	void exit2() {
		assert(_queue.empty());
		release_s(_io);
		release_s(_outbuf);
	}
// public:
	void post(AMessage *msg) {
		do_post(this, msg);
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
		_entity->addref();
		int result = ARefsBuf::reserve(_outbuf, ressiz, bufsiz);
		_outmsg.init();
		return _outmsg.done2(result);
	}
};


#endif
