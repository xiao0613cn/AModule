#ifndef _AEVENT_H_
#define _AEVENT_H_
#include "../base/AModule_API.h"

typedef struct AReceiver AReceiver;
typedef struct AEventManager AEventManager;

struct AReceiver : public AObject {
	AEventManager *_manager;
	struct rb_node _manager_node;
	list_head      _receiver_list;

	union {
	const char *_name;
	int         _index;
	};
	bool        _oneshot;
	bool        _preproc;
	int  (*on_event)(AReceiver *r, void *p, bool preproc);

	void init() {
		_manager = NULL;
		RB_CLEAR_NODE(&_manager_node);
		_receiver_list.init();
	}
};


struct AEventManager {
	struct rb_root   _receiver_map;
	long             _receiver_count;
	struct rb_root   _receiver_map2;
	long             _receiver_count2;
	pthread_mutex_t *_mutex;
	void lock() { _mutex ? pthread_mutex_lock(_mutex) : 0; }
	void unlock() { _mutex ? pthread_mutex_unlock(_mutex) : 0; }

	struct list_head _free_recvers;
	bool             _recycle_recvers;

	void init() {
		INIT_RB_ROOT(&_receiver_map);
		_receiver_count = 0;
		INIT_RB_ROOT(&_receiver_map2);
		_receiver_count2 = 0;
		_mutex = NULL;
		_free_recvers.init();
		_recycle_recvers = true;
	}
	bool _subscribe(AReceiver *r);
	void _erase(AReceiver *first, AReceiver *r);
	bool _unsubscribe(AReceiver *r);
	int  emit(const char *name, void *p);
	int  emit2(int index, void *p);
};


#endif
