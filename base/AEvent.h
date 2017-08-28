#ifndef _AEVENT_H_
#define _AEVENT_H_

struct AObject;
typedef struct AEvent AEvent;
typedef struct AReceiver AReceiver, AReceiver2;
typedef struct AEventManager AEventManager;

struct AEvent {
	AObject    *_self;
	const char *_name;
	long        _index;
	list_node<AEvent> _node;

	void    init(AObject *o);
	void    exit();
};

struct AReceiver {
	AObject    *_self;
	const char *_name;
	long        _index;
	AEventManager *_manager;
	struct rb_node _node;
	list_node<AReceiver> _list;

	int         _oneshot : 1;
	//int         _async : 1;
	void      (*on_event)(AReceiver *r, AEvent *v);

	void    init(AObject *o);
	void    exit();
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

	int  init() {
		INIT_RB_ROOT(&_receiver_map); _receiver_count = 0;
		INIT_RB_ROOT(&_receiver_map2); _receiver_count2 = 0;
	}
	bool _subscribe(AReceiver *r) {
		bool valid = ((r->_manager == NULL) && RB_EMPTY_NODE(&r->_node) && r->_list.empty());
		if (valid) {
			AReceiver *first = rb_insert_AReceiver(&_receiver_map, r, r->_name);
			if (first != NULL)
				first->_list.push_back(&r->_list);
			r->_manager = this;
			//r->r_object->addref();
		} else {
			assert(0);
		}
		return valid;
	}
	void _erase(AReceiver *first, AReceiver *r) {
		if (first != r) {
			assert(!first->_list.empty());
			assert(!r->_list.empty());
			assert(!RB_EMPTY_NODE(&first->_node));
			assert(RB_EMPTY_NODE(&r->_node));
			r->_list.leave();
		}
		else if (first->_list.empty()) {
			rb_erase(&first->_node, &_receiver_map);
			RB_CLEAR_NODE(&first->_node);
		}
		else {
			r = list_entry(first->_list.front(), AReceiver, _list);
			rb_replace_node(&first->_node, &r->_node, &_receiver_map);
			first->_list.leave();
		}
	}
	bool _unsubscribe(AReceiver *r) {
		bool valid = ((r->_manager == this) && (!RB_EMPTY_NODE(&r->_node) || !r->_list.empty()));
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
		//r->r_object->release2();
		return valid;
	}
	int  _emit(AEvent *v) {
		AReceiver *first = rb_find_AReceiver(&_receiver_map, v->_name);
		if (first == NULL)
			return 0;

		AReceiver *r = NULL;
		if (!first->_list.empty())
			r = list_entry(first->_list.front(), AReceiver, _list);

		int count = 0;
		while (r != NULL)
		{
			AReceiver *next = NULL;
			if (!first->_list.is_last(&r->_list))
				next = list_entry(r->_list.front(), AReceiver, _list);

			if (r->_oneshot)
				_erase(first, r);
			r->on_event(r, v);
			count++;
			r = next;
		}

		if (first->_oneshot)
			_erase(first, first);
		first->on_event(first, v);
		count++;
		return count;
	}
};


#endif
