#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEvent.h"

static inline int AReceiverCmp(const char *name, AReceiver *r) {
	return strcmp(name, r->_name);
}
rb_tree_define(AReceiver, _manager_node, const char*, AReceiverCmp)

static bool _do_subscribe(AEventManager *em, AReceiver *r)
{
	bool valid = ((r->_manager == NULL)
	           && RB_EMPTY_NODE(&r->_manager_node)
	           && r->_receiver_list.empty());
	if (valid) {
		AReceiver *first = rb_insert_AReceiver(&em->_receiver_map, r, r->_name);
		if (first != NULL)
			first->_receiver_list.push_back(&r->_receiver_list);
		r->_manager = em;
		em->_receiver_count ++;
		//r->r_object->addref();
	} else {
		assert(0);
	}
	return valid;
}

static void _do_erase(AEventManager *em, AReceiver *first, AReceiver *r)
{
	if (first != r) {
		assert(!first->_receiver_list.empty());
		assert(!r->_receiver_list.empty());
		assert(!RB_EMPTY_NODE(&first->_manager_node));
		assert(RB_EMPTY_NODE(&r->_manager_node));
		r->_receiver_list.leave();
	}
	else if (first->_receiver_list.empty()) {
		rb_erase(&first->_manager_node, &em->_receiver_map);
		RB_CLEAR_NODE(&first->_manager_node);
	}
	else {
		r = list_first_entry(&first->_receiver_list, AReceiver, _receiver_list);
		rb_replace_node(&first->_manager_node, &r->_manager_node, &em->_receiver_map);
		first->_receiver_list.leave();
	}
	em->_receiver_count --;
}

static bool _do_unsubscribe(AEventManager *em, AReceiver *r)
{
	bool valid = ((r->_manager == em) && (!RB_EMPTY_NODE(&r->_manager_node) || !r->_receiver_list.empty()));
	if (!valid) {
		assert(0);
		return false;
	}

	AReceiver *first = rb_find_AReceiver(&em->_receiver_map, r->_name);
	if (first == NULL) {
		assert(0);
		return false;
	}
	_do_erase(em, first, r);
	//r->_object->release();
	return valid;
}

static int _do_emit(AEventManager *em, const char *name, void *p)
{
	APool<AReceiver*> recvers; recvers.init(128);

	em->lock();
	AReceiver *first = rb_find_AReceiver(&em->_receiver_map, name);
	if (first == NULL) {
		em->unlock();
		return 0;
	}

	if (!em->_free_recvers.empty()) {
		ASlice<AReceiver*> *slice = list_first_entry(&em->_free_recvers, ASlice<AReceiver*>, _node);
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
				_do_erase(em, first, r);
			else
				r->_self->addref();
			recvers.push_back(r);
		}
		r = next;
	}

	if (!first->_preproc || (first->on_event(first, p, true) >= 0)) {
		if (first->_oneshot)
			_do_erase(em, first, first);
		else
			first->_self->addref();
		recvers.push_back(first);
	}
	em->unlock();

	list_head free_list; free_list.init();
	int count = recvers.total_count();

	while (recvers.total_count() != 0) {
		AReceiver *r = recvers.front();
		r->on_event(r, p, false);
		r->_self->release();

		recvers.pop_front(1, em->_recycle_recvers ? &free_list : NULL);
	}

	if (em->_recycle_recvers) {
		recvers.reset();

		em->lock();
		list_splice_init(&recvers._slice_list, &em->_free_recvers);
		list_splice_init(&free_list, &em->_free_recvers);
		em->unlock();
	} else {
		recvers.exit();
	}
	return count;
}

void AEventManager::init()
{
	INIT_RB_ROOT(&_receiver_map);
	_receiver_count = 0;
	_mutex = NULL;
	_subscribe = &_do_subscribe;
	_unsubscribe = &_do_unsubscribe;

	_free_recvers.init();
	_recycle_recvers = true;
	emit = &_do_emit;
}
