#ifndef _AEVENT_H_
#define _AEVENT_H_

struct AObject;
typedef struct AReceiver AReceiver;
typedef struct AEventManager AEventManager;

struct AReceiver {
	AObject    *_self;
	AEventManager *_manager;
	struct rb_node _manager_node;
	list_head      _receiver_list;

	const char *_name;
	bool        _oneshot;
	bool        _preproc;
	int  (*on_event)(AReceiver *r, void *p, bool preproc);

	void init(AObject *o) {
		_self = o; _manager = NULL;
		RB_CLEAR_NODE(&_manager_node);
		_receiver_list.init();
	}
};


struct AEventManager {
	struct rb_root   _receiver_map;
	long             _receiver_count;
	pthread_mutex_t *_mutex;
	bool (*_subscribe)(AEventManager *em, AReceiver *r);
	bool (*_unsubscribe)(AEventManager *em, AReceiver *r);

	struct list_head _free_recvers;
	bool             _recycle_recvers;
	int  (*emit)(AEventManager *em, const char *name, void *p);

	void init();
	void lock() { _mutex ? pthread_mutex_lock(_mutex) : 0; }
	void unlock() { _mutex ? pthread_mutex_unlock(_mutex) : 0; }

	struct AReceiver2* _sub_const(const char *name, bool preproc, void *user,
		int (*f)(void *user, const char *name, void *p, bool preproc));
};

#ifdef _AMODULE_H_
struct AReceiver2 : public AObject, public AReceiver {
	void  *_user;
	int  (*_func)(void *user, const char *name, void *p, bool preproc);

	static AReceiver2* create() {
		AReceiver2 *r2 = gomake(AReceiver2);
		r2->AObject::init(NULL);
		r2->_release = (void(*)(AObject*))&free;

		r2->AReceiver::init(r2);
		r2->on_event = &AReceiver2::on_user_event;
		return r2;
	}
	static int on_user_event(AReceiver *r, void *p, bool preproc) {
		AReceiver2 *r2 = (AReceiver2*)r;
		return r2->_func(r2->_user, r2->_name, p, preproc);
	}
};

inline struct AReceiver2*
 AEventManager::_sub_const(const char *name, bool preproc, void *user,
                           int (*f)(void *user, const char *name, void *p, bool preproc)) {
	AReceiver2 *r2 = AReceiver2::create();
	r2->_name = name; r2->_oneshot = false; r2->_preproc = preproc;
	r2->_user = user; r2->_func = f;
	_subscribe(this, r2);
	return r2;
}
#endif


#endif