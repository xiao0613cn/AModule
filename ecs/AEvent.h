#ifndef _AEVENT_H_
#define _AEVENT_H_
#include "../base/AModule_API.h"

typedef struct AReceiver AReceiver;
typedef ASlice<AReceiver*> ARecvSlice;
typedef struct AEventManager AEventManager;

typedef int  (*AEventFunc)(AReceiver *r, void *p, bool preproc);

struct AReceiver : public AObject {
	AEventManager *_manager;
	struct rb_node _map_node;
	list_head      _recv_list;
	void          *_user;

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
		_user = NULL;
	}
};


struct AEventManager {
	struct rb_root   _recv_map;
	long             _recv_count;
	struct rb_root   _recv_map2;
	long             _recv_count2;
	pthread_mutex_t  _mutex;
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }

	struct list_head _free_recvers;
	bool             _recycle_recvers;

	void init() {
		INIT_RB_ROOT(&_recv_map); _recv_count = 0;
		INIT_RB_ROOT(&_recv_map2); _recv_count2 = 0;
		pthread_mutex_init(&_mutex, NULL);
		_free_recvers.init();
		_recycle_recvers = true;
	}
	void exit() {
		assert(RB_EMPTY_ROOT(&_recv_map));
		assert(RB_EMPTY_ROOT(&_recv_map2));
		assert(_free_recvers.empty());
		pthread_mutex_destroy(&_mutex);
	}
	// event by name
	bool _sub_by_name(AReceiver *r);
	bool _unsub_by_name(AReceiver *r);
	int  emit_by_name(const char *name, void *p);
	void clear_sub();

	// event by index
	bool _sub_by_index(AReceiver *r);
	bool _unsub_by_index(AReceiver *r);
	int  emit_by_index(int index, void *p);
	void clear_sub2();
};


#endif
