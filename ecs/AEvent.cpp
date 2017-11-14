#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEvent.h"

// event by name
static inline int AReceiverCmp(const char *name, AReceiver *r) {
	return strcmp(name, r->_name);
}
rb_tree_define(AReceiver, _manager_node, const char*, AReceiverCmp)

bool AEventManager::_subscribe(AReceiver *r)
{
	bool valid = ((r->_manager == NULL)
	           && RB_EMPTY_NODE(&r->_manager_node)
	           && r->_receiver_list.empty());
	if (valid) {
		AReceiver *first = rb_insert_AReceiver(&_receiver_map, r, r->_name);
		if (first != NULL)
			first->_receiver_list.push_back(&r->_receiver_list);
		r->_manager = this;
		_receiver_count ++;
		//r->_self->addref();
	} else {
		assert(0);
	}
	return valid;
}

void AEventManager::_erase(AReceiver *first, AReceiver *r)
{
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
		r = list_first_entry(&first->_receiver_list, AReceiver, _receiver_list);
		rb_replace_node(&first->_manager_node, &r->_manager_node, &_receiver_map);
		first->_receiver_list.leave();
	}
	_receiver_count --;
}

bool AEventManager::_unsubscribe(AReceiver *r)
{
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
	//r->_self->release();
	return valid;
}

int AEventManager::emit(const char *name, void *p)
{
	APool<AReceiver*> recvers; recvers.init(128);

	lock();
	AReceiver *first = rb_find_AReceiver(&_receiver_map, name);
	if (first == NULL) {
		unlock();
		return 0;
	}

	if (!_free_recvers.empty()) {
		ASlice<AReceiver*> *slice = list_first_entry(&_free_recvers, ASlice<AReceiver*>, _node);
		recvers._join(slice);
	}

	AReceiver *r = NULL;
	if (!first->_receiver_list.empty())
		r = list_first_entry(&first->_receiver_list, AReceiver, _receiver_list);
	while (r != NULL)
	{
		AReceiver *next = NULL;
		if (!first->_receiver_list.is_last(&r->_receiver_list))
			next = list_entry(r->_receiver_list.next, AReceiver, _receiver_list);

		if (!r->_preproc || (r->on_event(r, p, true) >= 0)) {
			if (r->_oneshot)
				_erase(first, r);
			else
				r->addref();
			recvers.push_back(r);
		}
		r = next;
	}

	if (!first->_preproc || (first->on_event(first, p, true) >= 0)) {
		if (first->_oneshot)
			_erase(first, first);
		else
			first->addref();
		recvers.push_back(first);
	}
	unlock();

	list_head free_list; free_list.init();
	int count = recvers.total_count();

	while (recvers.total_count() != 0) {
		AReceiver *r = recvers.front();
		r->on_event(r, p, false);
		r->release();

		recvers.pop_front(1, _recycle_recvers ? &free_list : NULL);
	}

	if (_recycle_recvers && (!free_list.empty() || !recvers._slice_list.empty())) {
		recvers.reset();

		lock();
		list_splice_init(&recvers._slice_list, &_free_recvers);
		list_splice_init(&free_list, &_free_recvers);
		unlock();
	} else {
		recvers.exit();
	}
	return count;
}

// event by index
typedef struct AReceiver ARecvByIndex;
static inline int ARecvByIndexCmp(int index, ARecvByIndex *r) {
	return index - r->_index;
}
rb_tree_define(ARecvByIndex, _manager_node, int, ARecvByIndexCmp)
