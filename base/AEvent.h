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
	struct rb_node _node;
	AEventManager *_manager;

	int         _oneshot : 1;
	int         _async : 1;
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
	int  _subscribe(AReceiver *r) {
		int valid = ((r->vm_manager == NULL) && RB_EMPTY_NODE(&r->vm_node) && r->r_node.empty());
		if (valid) {
			AReceiver *first = rb_insert_AReceiver(&_receiver_map, r, r->r_name);
			if (first != NULL) first->r_node.push_back(&r->r_node);
			r->vm_manager = this;
			//r->r_object->addref();
		} else {
			assert(0);
		}
		return valid;
	}
	void _erase(AReceiver *first, AReceiver *r) {
		if (first != r) {
			assert(!first->r_node.empty());
			assert(!r->r_node.empty());
			assert(RB_EMPTY_NODE(&r->vm_node));
			r->r_node.leave();
		}
		else if (first->r_node.empty()) {
			rb_erase(&first->vm_node, &_receiver_map);
			RB_CLEAR_NODE(&first->vm_node);
		}
		else {
			r = list_entry(first->r_node.front(), AReceiver, r_node);
			rb_replace_node(&first->vm_node, &r->vm_node, &_receiver_map);
			first->r_node.leave();
		}
	}
	int  _unsubscribe(AReceiver *r) {
		int valid = ((r->vm_manager == this) && (!RB_EMPTY_NODE(&r->vm_node) || !r->r_node.empty()));
		if (!valid) {
			assert(0);
			return 0;
		}

		AReceiver *first = rb_find_AReceiver(&_receiver_map, r->r_name);
		if (first == NULL) {
			assert(0);
			return 0;
		}
		_erase(first, r);
		//r->r_object->release2();
		return valid;
	}
	int  _emit(AEvent *v) {
		AReceiver *first = rb_find_AReceiver(&_receiver_map, v->v_name);
		if (first == NULL)
			return 0;

		AReceiver *r = NULL;
		if (!first->r_node.empty())
			r = list_entry(first->r_node.front(), AReceiver, r_node);

		int count = 0;
		while (r != NULL)
		{
			AReceiver *next = NULL;
			if (!first->r_node.is_last(r))
				r = list_entry(r->r_node.front(), AReceiver, r_node);

			if (r->r_oneshot)
				_erase(first, r);
			r->on_event(r, v);
			count++;
			r = next;
		}

		if (first->r_oneshot)
			_erase(first, first);
		first->on_event(first, v);
		count++;
		return count;
	}
};


#endif
