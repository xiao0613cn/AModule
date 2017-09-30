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
	int         _oneshot : 1;
	int         _async : 1;
	void (*on_event)(AReceiver *r, void *p);

	void init(AObject *o) {
		_self = o; _manager = NULL;
		RB_CLEAR_NODE(&_manager_node);
		_receiver_list.init();
	}
};

static inline int AReceiverCmp(const char *name, AReceiver *r) {
	return strcmp(name, r->_name);
}
rb_tree_declare(AReceiver, const char*)

struct AEventManager {
	struct rb_root _receiver_map;
	long           _receiver_count;

	void init() {
		INIT_RB_ROOT(&_receiver_map);
		_receiver_count = 0;
	}
	bool _subscribe(AReceiver *r) {
		bool valid = ((r->_manager == NULL)
		           && RB_EMPTY_NODE(&r->_manager_node)
		           && r->_receiver_list.empty());
		if (valid) {
			AReceiver *first = rb_insert_AReceiver(&_receiver_map, r, r->_name);
			if (first != NULL)
				first->_receiver_list.push_back(&r->_receiver_list);
			r->_manager = this;
			//r->r_object->addref();
		} else {
			assert(0);
		}
		return valid;
	}
	void _sub_const(const char *name, bool async, void *user,
	                void (*f)(void *user, const char *name, void *p));
	void _erase(AReceiver *first, AReceiver *r) {
		if (first != r) {
			assert(!first->_receiver_list.empty());
			assert(!r->_receiver_list.empty());
			assert(!RB_EMPTY_NODE(&first->_manager_node));
			assert(RB_EMPTY_NODE(&r->_manager_node));
			r->_receiver_list.leave();
		}
		else if (first->_receiver_list.empty()) {
			rb_erase(&first->_manager_node, &_receiver_map);
			RB_CLEAR_NODE(&first->_manager_node);
		}
		else {
			r = list_entry(first->_receiver_list.next, AReceiver, _receiver_list);
			rb_replace_node(&first->_manager_node, &r->_manager_node, &_receiver_map);
			first->_receiver_list.leave();
		}
	}
	bool _unsubscribe(AReceiver *r) {
		bool valid = ((r->_manager == this) && (!RB_EMPTY_NODE(&r->_manager_node) || !r->_receiver_list.empty()));
		if (!valid) {
			assert(0);
			return false;
		}

		AReceiver *first = rb_find_AReceiver(&_receiver_map, r->_name);
		if (first == NULL) {
			assert(0);
			return false;
		}
		_erase(first, r);
		//r->_object->release();
		return valid;
	}
	int _emit(const char *name, void *p) {
		AReceiver *first = rb_find_AReceiver(&_receiver_map, name);
		if (first == NULL)
			return 0;

		AReceiver *r = NULL;
		if (!first->_receiver_list.empty())
			r = list_entry(first->_receiver_list.next, AReceiver, _receiver_list);

		int count = 0;
		while (r != NULL)
		{
			AReceiver *next = NULL;
			if (!first->_receiver_list.is_last(&r->_receiver_list))
				next = list_entry(r->_receiver_list.next, AReceiver, _receiver_list);

			if (r->_oneshot)
				_erase(first, r);
			r->on_event(r, p);
			count ++;
			r = next;
		}

		if (first->_oneshot)
			_erase(first, first);
		first->on_event(first, p);
		count ++;
		return count;
	}
};

#ifdef _AMODULE_H_
struct AReceiver2 : public AObject, public AReceiver {
	void  *_user;
	void (*_func)(void *user, const char *name, void *p);

	static AReceiver2* create() {
		AReceiver2 *r2 = make(AReceiver2);
		r2->AObject::init(NULL);
		r2->_release = (void(*)(AObject*))&free;

		r2->AReceiver::init(r2);
		r2->on_event = &AReceiver2::on_user_event;
		return r2;
	}
	static void on_user_event(AReceiver *r, void *p) {
		AReceiver2 *r2 = (AReceiver2*)r;
		r2->_func(r2->_user, r2->_name, p);
	}
};

inline void AEventManager::_sub_const(const char *name, bool async, void *user,
                                      void (*f)(void *user, const char *name, void *p)) {
	AReceiver2 *r2 = AReceiver2::create();
	r2->_name = name; r2->_oneshot = false; r2->_async = async;
	r2->_user = user; r2->_func = f;
	_subscribe(r2);
}
#endif


#endif
