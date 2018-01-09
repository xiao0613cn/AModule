#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEvent.h"


// event by name
static inline int ARecvCmpName(const char *name, AReceiver *r) {
	return strcmp(name, r->_name);
}
rb_tree_define(AReceiver, _map_node, const char*, ARecvCmpName)
struct HelperByName {
	typedef const char* KeyType;
	static KeyType key(AReceiver *r) {
		return r->_name;
	}
	static AReceiver* find(rb_root *map, KeyType name) {
		return rb_find_AReceiver(map, name);
	}
	static AReceiver* insert(rb_root *map, AReceiver *r, KeyType name) {
		return rb_insert_AReceiver(map, r, name);
	}
};

// event by index
typedef struct AReceiver ARecvByIndex;
static inline int ARecvCmpIndex(int64_t index, ARecvByIndex *r) {
	return int(index - r->_index);
}
rb_tree_define(ARecvByIndex, _map_node, int64_t, ARecvCmpIndex)
struct HelperByIndex {
	typedef int64_t KeyType;
	static KeyType key(AReceiver *r) {
		return r->_index;
	}
	static AReceiver* find(rb_root *map, KeyType index) {
		return rb_find_ARecvByIndex(map, index);
	}
	static AReceiver* insert(rb_root *map, AReceiver *r, KeyType index) {
		return rb_insert_ARecvByIndex(map, r, index);
	}
};

//
template <typename helper>
static bool _subscribe(AEventManager *em, AReceiver *r, rb_root &map, long &count)
{
	bool valid = ((r->_manager == NULL)
	           && RB_EMPTY_NODE(&r->_map_node)
	           && r->_recv_list.empty());
	if (valid) {
		AReceiver *first = helper::insert(&map, r, helper::key(r));
		if (first != NULL)
			first->_recv_list.push_back(&r->_recv_list);
		r->_manager = em;
		count ++;
		//r->_self->addref();
	} else {
		assert(0);
	}
	return valid;
}

bool AEventManager::_sub_by_name(AReceiver *r) {
	return _subscribe<HelperByName>(this, r, _recv_map, _recv_count);
}
bool AEventManager::_sub_by_index(AReceiver *r) {
	return _subscribe<HelperByIndex>(this, r, _recv_map2, _recv_count2);
}


static void _erase(AReceiver *first, AReceiver *r, rb_root &map, long &count)
{
	if (first != r) {
		assert(!first->_recv_list.empty());
		assert(!r->_recv_list.empty());
		assert(!RB_EMPTY_NODE(&first->_map_node));
		assert(RB_EMPTY_NODE(&r->_map_node));
		r->_recv_list.leave();
	}
	else if (first->_recv_list.empty()) {
		rb_erase(&first->_map_node, &map);
		RB_CLEAR_NODE(&first->_map_node);
	}
	else {
		r = list_first_entry(&first->_recv_list, AReceiver, _recv_list);
		rb_replace_node(&first->_map_node, &r->_map_node, &map);
		first->_recv_list.leave();
	}
	count --;
}

template <typename helper>
static bool _unsubscribe(AEventManager *em, AReceiver *r, rb_root &map, long &count)
{
	bool valid = ((r->_manager == em) && (!RB_EMPTY_NODE(&r->_map_node) || !r->_recv_list.empty()));
	if (!valid) {
		assert(0);
		return false;
	}

	AReceiver *first = helper::find(&map, helper::key(r));
	if (first == NULL) {
		assert(0);
		return false;
	}
	_erase(first, r, map, count);
	//r->_self->release();
	return valid;
}

bool AEventManager::_unsub_by_name(AReceiver *r) {
	return _unsubscribe<HelperByName>(this, r, _recv_map, _recv_count);
}
bool AEventManager::_unsub_by_index(AReceiver *r) {
	return _unsubscribe<HelperByIndex>(this, r, _recv_map2, _recv_count2);
}

typedef APool<AReceiver*> ARecvPool;

template <typename helper>
static int emit_event(AEventManager *em, typename helper::KeyType key, void *p, rb_root &map, long &count)
{
	ARecvPool recvers; recvers.init(128);

	em->lock();
	AReceiver *first = helper::find(&map, key);
	if (first == NULL) {
		em->unlock();
		return 0;
	}

	if (!em->_free_recvers.empty()) {
		ARecvSlice *slice = list_first_entry(&em->_free_recvers, ARecvSlice, _node);
		recvers._join(slice);
	}

	AReceiver *r = NULL;
	if (!first->_recv_list.empty())
		r = list_first_entry(&first->_recv_list, AReceiver, _recv_list);
	while (r != NULL)
	{
		AReceiver *next = NULL;
		if (!first->_recv_list.is_last(&r->_recv_list))
			next = list_entry(r->_recv_list.next, AReceiver, _recv_list);

		if (!r->_preproc || (r->on_event(r, p, true) >= 0)) {
			if (r->_oneshot)
				_erase(first, r, map, count);
			else
				r->addref();
			recvers.push_back(r);
		}
		r = next;
	}

	if (!first->_preproc || (first->on_event(first, p, true) >= 0)) {
		if (first->_oneshot)
			_erase(first, first, map, count);
		else
			first->addref();
		recvers.push_back(first);
	}
	em->unlock();

	list_head free_list; free_list.init();
	int recver_count = recvers.total_count();

	while (recvers.total_count() != 0) {
		AReceiver *r = recvers.front();
		r->on_event(r, p, false);
		r->release();

		recvers.pop_front(1, em->_recycle_recvers ? &free_list : NULL);
	}

	if (em->_recycle_recvers && (!free_list.empty() || !recvers._slice_list.empty())) {
		recvers.reset();

		em->lock();
		list_splice_init(&recvers._slice_list, &em->_free_recvers);
		list_splice_init(&free_list, &em->_free_recvers);
		em->unlock();
	} else {
		recvers.exit();
	}
	return recver_count;
}

int AEventManager::emit_by_name(const char *name, void *p) {
	return emit_event<HelperByName>(this, name, p, _recv_map, _recv_count);
}
int AEventManager::emit_by_index(int index, void *p) {
	return emit_event<HelperByIndex>(this, index, p, _recv_map2, _recv_count2);
}


static void clear_sub_map(AEventManager *em, rb_root &map, long &count)
{
	struct list_head recvers; recvers.init();

	em->lock();
	list_splice_init(&em->_free_recvers, &recvers);

	while (!RB_EMPTY_ROOT(&map)) {
		AReceiver *first = rb_first_entry(&map, AReceiver, _map_node);

		while (!first->_recv_list.empty()) {
			AReceiver *r = list_first_entry(&first->_recv_list, AReceiver, _recv_list);
			_erase(first, r, map, count);
			recvers.push_back(&r->_recv_list);
		}
		_erase(first, first, map, count);
		recvers.push_back(&first->_recv_list);
	}
	em->unlock();

	ARecvSlice::_clear(recvers);
}

void AEventManager::clear_sub() {
	return clear_sub_map(this, _recv_map, _recv_count);
}
void AEventManager::clear_sub2() {
	return clear_sub_map(this, _recv_map2, _recv_count2);
}
