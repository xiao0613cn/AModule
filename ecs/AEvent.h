#ifndef _AEVENT_H_
#define _AEVENT_H_

struct AObject;
typedef struct AEvent AEvent;
typedef struct AReceiver AReceiver, AReceiver2;
typedef struct AEventManager AEventManager;

struct AEvent {
	//AObject    *_self;
	const char *_name;
	long        _index;

	void  init(const char *n, long i) {
		_name = n; _index = i;
	}
	//void  exit();
};

struct AReceiver {
	AObject    *_self;
	AEventManager *_manager;
	struct rb_node _manager_node;
	list_node<AReceiver> _receiver_list;
	struct rb_node _manager_node2;
	list_node<AReceiver2> _receiver_list2;

	const char *_name;
	long        _index;
	int         _oneshot : 1;
	//int         _async : 1;
	int   (*receive)(AReceiver *r, void *p);

	void  init(AObject *o) {
		_self = o; _manager = NULL;
		RB_CLEAR_NODE(&_manager_node); _receiver_list.init(this);
		RB_CLEAR_NODE(&_manager_node2); _receiver_list2.init(this);
	}
	void  exit();
};

static inline int AReceiverCmp(const char *name, AReceiver *r) {
	return strcmp(name, r->_name);
}
rb_tree_declare(AReceiver, const char*)

static inline int AReceiver2Cmp(long index, AReceiver2 *r) {
	return int(index - r->_index);
}
rb_tree_declare(AReceiver2, long)

struct AEventManager {
	struct rb_root _receiver_map;
	long           _receiver_count;
	struct rb_root _receiver_map2;
	long           _receiver_count2;

	void  init() {
		INIT_RB_ROOT(&_receiver_map); _receiver_count = 0;
		INIT_RB_ROOT(&_receiver_map2); _receiver_count2 = 0;
	}
	bool  _subscribe(AReceiver *r) {
		bool valid = ((r->_manager == NULL) && RB_EMPTY_NODE(&r->_manager_node) && r->_receiver_list.empty());
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
	void  _erase(AReceiver *first, AReceiver *r) {
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
			r = list_entry(first->_receiver_list.front(), AReceiver, _receiver_list);
			rb_replace_node(&first->_manager_node, &r->_manager_node, &_receiver_map);
			first->_receiver_list.leave();
		}
	}
	bool  _unsubscribe(AReceiver *r) {
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
	int  _emit(const char *name, void *p) {
		AReceiver *first = rb_find_AReceiver(&_receiver_map, name);
		if (first == NULL)
			return 0;

		AReceiver *r = NULL;
		if (!first->_receiver_list.empty())
			r = list_entry(first->_receiver_list.front(), AReceiver, _receiver_list);

		int count = 0;
		while (r != NULL)
		{
			AReceiver *next = NULL;
			if (!first->_receiver_list.is_last(&r->_receiver_list))
				next = list_entry(r->_receiver_list.front(), AReceiver, _receiver_list);

			if (r->_oneshot)
				_erase(first, r);
			r->receive(r, p);
			count++;
			r = next;
		}

		if (first->_oneshot)
			_erase(first, first);
		first->receive(first, p);
		count++;
		return count;
	}
};

#ifdef _AMODULE_H_
struct AReceiver2 : public AObject, public AReceiver {
	void init(AModule *m = NULL) {
		AObject::init(m);
		AReceiver::init(this);
	}
};
#endif


#endif
