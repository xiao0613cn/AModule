#ifndef _AEVENT_H_
#define _AEVENT_H_
#include "../base/AModule_API.h"

typedef struct AReceiver AReceiver;
typedef struct AEventManager AEventManager;

typedef int  (*AEventFunc)(AReceiver *r, void *p, bool preproc);

struct AReceiver : public AObject {
	AEventManager *_manager;
	struct rb_node _map_node;
	list_head      _recv_list;
	void          *_userdata;

	union {
	const char *_name;
	int64_t     _index;
	};
	bool        _oneshot;
	bool        _preproc;
	AEventFunc  on_event;

	void init(AModule *m, void *release) {
		AObject::init(m, release);
		_manager = NULL;
		RB_CLEAR_NODE(&_map_node);
		_recv_list.init();
		_userdata = NULL;
	}
	static AReceiver* first(list_head &list) {
		return list_first_entry(&list, AReceiver, _recv_list);
	}
	AReceiver* next() {
		return list_entry(_recv_list.next, AReceiver, _recv_list);
	}
	bool valid() {
		return !RB_EMPTY_NODE(&_map_node);
	}
};

typedef int (*ASelfEventFunc)(AReceiver *r, bool preproc, void *self);

struct AEventManagerMethod {
	// event by name
	bool (*_sub_by_name)(AEventManager *em, AReceiver *r);   // include r->addref()
	bool (*_unsub_by_name)(AEventManager *em, AReceiver *r); // include r->release()
	int  (*emit_by_name)(AEventManager *em, const char *name, void *p); // include lock(), unlock()
	int  (*clear_sub_by_name)(AEventManager *em);                       // include lock(), unlock()
	AReceiver* (*_sub_self)(AEventManager *em, const char *name, void *self, ASelfEventFunc f); // include r->addref()

	// event by index
	bool (*_sub_by_index)(AEventManager *em, AReceiver *r);   // include r->addref()
	bool (*_unsub_by_index)(AEventManager *em, AReceiver *r); // include r->release()
	int  (*emit_by_index)(AEventManager *em, int64_t index, void *p); // include lock(), unlock()
	int  (*clear_sub_by_index)(AEventManager *em);                    // include lock(), unlock()
};

struct AEventManager : public AEventManagerMethod {
	static const char* name() { return "AEventManagerDefaultModule"; }
	static AEventManager* get() { return AModule::singleton_data<AEventManager>(); }

	struct rb_root   _name_map;
	int              _name_count;
	struct rb_root   _index_map;
	int              _index_count;
	pthread_mutex_t  _mutex;
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }

	struct list_head _free_ptrslice;
	bool             _reuse_ptrslice;

	void init(AEventManagerMethod *m) {
		if (m != this)
			*(AEventManagerMethod*)this = *m;

		INIT_RB_ROOT(&_name_map); _name_count = 0;
		INIT_RB_ROOT(&_index_map); _index_count = 0;
		pthread_mutex_init(&_mutex, NULL);
		_free_ptrslice.init();
		_reuse_ptrslice = true;
	}
	void exit() {
		assert(RB_EMPTY_ROOT(&_name_map));
		assert(RB_EMPTY_ROOT(&_index_map));
		APtrPool::_clear(_free_ptrslice);
		pthread_mutex_destroy(&_mutex);
	}
	void sub(AReceiver *r, bool by_name) {
		lock();
		by_name ? _sub_by_name(this, r) : _sub_by_index(this, r);
		unlock();
	}
	void unsub(AReceiver *r, bool by_name) {
		lock();
		by_name ? _unsub_by_name(this, r) : _unsub_by_index(this, r);
		unlock();
	}
	int clear_sub() {
		return clear_sub_by_name(this) + clear_sub_by_index(this);
	}
};


#endif
