#include "stdafx.h"
#include "ASystem.h"
#include "AEntity.h"
#include "AEvent.h"


void ASystemManager::_do_check_entity(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick)
{
	int check_count = 0;

	sm->entity_lock();
	sm->_last_check = sm->_entity_manager->_upper(sm->_last_check);
	while (sm->_last_check != NULL)
	{
		list_head &results = results_list[check_count];
		results.init();
		if (sm->_check_one(results, sm->_last_check, cur_tick) > 0) {
			if (++check_count >= max_count)
				break;
		}
		sm->_last_check = sm->_entity_manager->_next(sm->_last_check);
	}
	sm->entity_unlock();

	while (check_count > 0) {
		sm->_exec(results_list[--check_count]);
	}
}

int ASystemManager::_do_check_allsys(ASystemManager *sm, DWORD cur_tick)
{
	list_head results; results.init();

	sm->entity_lock();
	int count = sm->_systems->check_all(&results, cur_tick);
	list_for_each2(s, &sm->_systems->module.class_entry, ASystem, module.class_entry)
		count += s->check_all(&results, cur_tick);
	sm->entity_unlock();

	sm->_exec(results);
	return count;
}

int ASystemManager::_do_emit(ASystemManager *sm, const char *name, void *p)
{
	APool<AReceiver*> recvers; recvers.init();

	sm->event_lock();
	AReceiver *first = rb_find_AReceiver(&sm->_event_manager->_receiver_map, name);
	if (first == NULL) {
		sm->event_unlock();
		return 0;
	}

	if (!sm->_free_recvers.empty()) {
		ASlice<AReceiver*> *slice = list_entry(sm->_free_recvers.next, ASlice<AReceiver*>, node);
		recvers._join(slice);
	}

	AReceiver *r = NULL;
	if (!first->_receiver_list.empty())
		r = list_entry(first->_receiver_list.next, AReceiver, _receiver_list);

	int count = 0;
	while (r != NULL)
	{
		AReceiver *next = NULL;
		if (!first->_receiver_list.is_last(&r->_receiver_list))
			next = list_entry(r->_receiver_list.next, AReceiver, _receiver_list);

		if (r->_oneshot) {
			sm->_event_manager->_erase(first, r);
			recvers.push_back(r);
		}
		if (!r->_async) {
			r->on_event(r, p);
		} else if (!r->_oneshot) {
			r->_self->addref();
			recvers.push_back(r);
		}
		count ++;
		r = next;
	}

	if (first->_oneshot) {
		sm->_event_manager->_erase(first, first);
		recvers.push_back(first);
	}
	if (!first->_async) {
		first->on_event(first, p);
	} else if (!first->_oneshot) {
		first->_self->addref();
		recvers.push_back(first);
	}
	count ++;
	sm->event_unlock();

	if (recvers.total_len() == 0)
		return count;

	list_head free_list; free_list.init();
	do {
		AReceiver *r = *recvers.ptr();
		if (r->_async)
			r->on_event(r, p);
		r->_self->release();

		recvers.pop(1, &free_list);
	} while (recvers.total_len() != 0);

	recvers.quick_reset();

	sm->event_lock();
	list_splice_init(&recvers.slice_list, &sm->_free_recvers);
	list_splice_init(&free_list, &sm->_free_recvers);
	sm->event_unlock();
	return count;
}
