#include "stdafx.h"
#include "ASystem.h"
#include "AEntity.h"
#include "AEvent.h"

static inline int
AEntityCmp(AEntity *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_define(AEntity, _manager_node, AEntity*, AEntityCmp)

static int _do_check_one(ASystemManager *sm, list_head &results, AEntity *e, DWORD cur_tick)
{
	int count = 0;

	ASystem::Result *r = sm->_all_systems->check_one ? sm->_all_systems->check_one(e, cur_tick) : NULL;
	if (r != NULL) {
		r->system = sm->_all_systems;
		results.push_back(&r->node);
		count ++;
	}

	list_for_each2(s, &sm->_all_systems->module.class_entry, ASystem, module.class_entry)
	{
		r = s->check_one ? s->check_one(e, cur_tick) : NULL;
		if (r != NULL) {
			r->system = s;
			results.push_back(&r->node);
			count ++;
		}
	}
	return count;
}

static void _do_check_entity(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick)
{
	int check_count = 0;

	sm->_entity_manager->lock();
	sm->_last_check = sm->_entity_manager->_upper(sm->_last_check);
	while (sm->_last_check != NULL)
	{
		list_head &results = results_list[check_count];
		results.init();
		if (_do_check_one(sm, results, sm->_last_check, cur_tick) > 0) {
			if (++check_count >= max_count)
				break;
		}
		sm->_last_check = sm->_entity_manager->_next(sm->_last_check);
	}
	sm->_entity_manager->unlock();

	while (check_count > 0) {
		sm->_exec_results(results_list[--check_count]);
	}
}

static int _do_check_allsys(ASystemManager *sm, DWORD cur_tick)
{
	list_head results; results.init();

	sm->lock();
	int count = sm->_all_systems->check_all ? sm->_all_systems->check_all(&results, cur_tick) : 0;

	list_for_each2(s, &sm->_all_systems->module.class_entry, ASystem, module.class_entry)
	{
		count += s->check_all ? s->check_all(&results, cur_tick) : 0;
	}
	sm->unlock();

	sm->_exec_results(results);
	return count;
}

static int _do_emit(ASystemManager *sm, const char *name, void *p)
{
	return sm->_event_manager->emit(sm->_event_manager, name, p);
}

void ASystemManager::init()
{
	_all_systems = ASystem::find(NULL);
	_exec_thread = NULL;
	_all_services = NULL;
	_mutex = NULL;
	check_allsys = &_do_check_allsys;

	_entity_manager = NULL;
	_last_check = NULL;
	check_entity = &_do_check_entity;

	_event_manager = NULL;
	emit = &_do_emit;
}
