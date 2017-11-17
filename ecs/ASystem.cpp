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

static int _check_one(ASystemManager *sm, list_head *exec_list, AEntity *e, DWORD cur_tick)
{
	int count = 0;
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	ASystem::Result *r = sm->_all_systems->check_one ? sm->_all_systems->check_one(e, cur_tick) : NULL;
	if (r != NULL) {
		r->system = sm->_all_systems;
		exec_list->push_back(&r->node);
		count ++;
	}

	list_for_allsys(s, sm->_all_systems) {
		r = s->check_one ? s->check_one(e, cur_tick) : NULL;
		if (r != NULL) {
			r->system = s;
			exec_list->push_back(&r->node);
			count ++;
		}
	}
	return count;
}

static int check_by_entities(ASystemManager *sm, list_head *exec_list, DWORD cur_tick)
{
	AEntityManager *em = sm->_all_entities;
	if (em == NULL) {
		TRACE("no manager for checking...\n");
		return 0;
	}

	int result = 0;
	em->lock();
	sm->_last_entity = em->_upper(sm->_last_entity);
	while (sm->_last_entity != NULL)
	{
		result = sm->_check_one(sm, exec_list, sm->_last_entity, cur_tick);
		if (result > 0)
			break;
		sm->_last_entity = em->_next(sm->_last_entity);
	}
	em->unlock();
	return result;
}

static int check_by_allsys(ASystemManager *sm, list_head *exec_list, DWORD cur_tick)
{
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	sm->lock();
	int count = sm->_all_systems->check_all ? sm->_all_systems->check_all(exec_list, cur_tick) : 0;
	list_for_allsys(s, sm->_all_systems) {
		count += s->check_all ? s->check_all(exec_list, cur_tick) : 0;
	}
	sm->unlock();
	return count;
}

static bool _subscribe(ASystemManager *sm, AReceiver *r)
{
	if (sm->_event_manager == NULL)
		return false;
	return sm->_event_manager->_subscribe(r);
}

static bool _unsubscribe(ASystemManager *sm, AReceiver *r)
{
	if (sm->_event_manager == NULL)
		return false;
	return sm->_event_manager->_unsubscribe(r);
}

static int emit_event(ASystemManager *sm, const char *name, void *p)
{
	if (sm->_event_manager == NULL)
		return -1;
	return sm->_event_manager->emit(name, p);
}

static int emit_event2(ASystemManager *sm, int index, void *p)
{
	//TODO:
	//if (sm->_event_manager == NULL)
		return -1;
	//return sm->_event_manager->emit(name, p);
}

struct AReceiver2 : public AReceiver {
	void     *_user;
	ASelfFunc _func;
};

static int on_self_event(AReceiver *r, void *p, bool preproc)
{
	AReceiver2 *r2 = (AReceiver2*)r;
	if (r2->_user != p)
		return -1;
	return r2->_func(r2->_name, preproc, p);
}

static AReceiver* _sub_self(ASystemManager *sm, const char *name, bool preproc, void *user, ASelfFunc f)
{
	if (sm->_event_manager == NULL)
		return NULL;

	AReceiver2 *r2 = gomake(AReceiver2);
	r2->AObject::init(NULL, &free);
	r2->AReceiver::init();
	r2->on_event = &on_self_event;

	r2->_name = name; r2->_oneshot = false; r2->_preproc = preproc;
	r2->_user = user; r2->_func = f;
	sm->_event_manager->_subscribe(r2);
	return r2;
}

static int check_allsys(AOperator *asop, int result)
{
	ASystemManager *sm = container_of(asop, ASystemManager, _asop_systems);
	if ((result < 0) || (sm->_tick_systems == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_by_allsys(sm, &exec_list, GetTickCount());

	if (exec_list.empty() && sm->_idle_check_entities) {
		result = sm->check_by_entities(sm, &exec_list, GetTickCount());
	}

	sm->_exec_results(exec_list);
	asop->delay(sm->_thr_systems, sm->_tick_systems);
	return result;
}

static int check_entities(AOperator *asop, int result)
{
	ASystemManager *sm = container_of(asop, ASystemManager, _asop_entities);
	if ((result < 0) || (sm->_tick_entities == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_by_entities(sm, &exec_list, GetTickCount());

	if (exec_list.empty()) {
		asop->delay(sm->_thr_entities, sm->_tick_entities);
	} else {
		asop->post(sm->_thr_entities);
		sm->_exec_results(exec_list);
	}
	return result;
}

static int start_checkall(ASystemManager *sm)
{
	if (sm->_asop_systems.done == NULL)
		sm->_asop_systems.done = &check_allsys;
	sm->_asop_systems.post(sm->_thr_systems);

	if (sm->_all_entities != NULL) {
		if (sm->_asop_entities.done == NULL)
			sm->_asop_entities.done = &check_entities;
		sm->_asop_entities.post(sm->_thr_entities);
	}
	return 0;
}

static void stop_checkall(ASystemManager *sm)
{
	sm->_tick_systems = 0;
	sm->_asop_systems.signal(sm->_thr_systems, TRUE);

	sm->_tick_entities = 0;
	sm->_asop_entities.signal(sm->_thr_entities, TRUE);

	while ((sm->_asop_systems.done != NULL) || (sm->_asop_entities.done != NULL))
		Sleep(20);
}

rb_tree_declare(AReceiver, const char*)

static void clear_sub(ASystemManager *sm)
{
	if (sm->_event_manager == NULL)
		return;

	sm->_event_manager->lock();
	while (!RB_EMPTY_ROOT(&sm->_event_manager->_receiver_map)) {
		AReceiver *first = rb_first_entry(&sm->_event_manager->_receiver_map, AReceiver, _manager_node);

		while (!first->_receiver_list.empty()) {
			AReceiver *r = list_first_entry(&first->_receiver_list, AReceiver, _receiver_list);
			sm->_event_manager->_erase(first, r);
			r->release();
		}
		sm->_event_manager->_erase(first, first);
		first->release();
	}
	ASlice<AReceiver>::_clear(sm->_event_manager->_free_recvers);
	sm->_event_manager->unlock();
}

ASystemManagerDefaultModule SysMngModule = {
{
	ASystemManagerDefaultModule::name(),
	ASystemManagerDefaultModule::name(),
	0, NULL, NULL,
}, {
	&start_checkall,
	&stop_checkall,
	&check_by_allsys,
	&check_by_entities,
	&_check_one,
	&_subscribe,
	&_unsubscribe,
	&emit_event,
	&emit_event2,
	&_sub_self,
	&clear_sub,
} };
static int reg_sys = AModuleRegister(&SysMngModule.module);
