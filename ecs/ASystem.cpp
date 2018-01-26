#include "stdafx.h"
#include "ASystem.h"
#include "AEntity.h"
#include "AEvent.h"


static int SM_check_one(ASystemManager *sm, list_head *exec_list, AEntity *e, DWORD cur_tick)
{
	int count = 0;
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	ASystem::Result *r = sm->_all_systems->_check_one ? sm->_all_systems->_check_one(e, cur_tick) : NULL;
	if (r != NULL) {
		r->system = sm->_all_systems;
		exec_list->push_back(&r->node);
		count ++;
	}

	list_for_allsys(s, sm->_all_systems) {
		r = s->_check_one ? s->_check_one(e, cur_tick) : NULL;
		if (r != NULL) {
			r->system = s;
			exec_list->push_back(&r->node);
			count ++;
		}
	}
	return count;
}

static int SM_check_entities(ASystemManager *sm, AEntityManager *em, list_head *exec_list, DWORD cur_tick)
{
	int result = 0;
	em->lock();
	em->_last_entity = em->_upper(em, em->_last_entity);
	while (em->_last_entity != NULL)
	{
		result = sm->_check_one(sm, exec_list, em->_last_entity, cur_tick);
		if (result > 0)
			break;
		em->_last_entity = em->_next(em, em->_last_entity);
	}
	em->unlock();
	return result;
}

static int SM_check_allsys(ASystemManager *sm, list_head *exec_list, DWORD cur_tick)
{
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	sm->lock();
	if (sm->_all_entities) sm->_all_entities->lock();
	int count = sm->_all_systems->_check_all ? sm->_all_systems->_check_all(exec_list, cur_tick) : 0;

	list_for_allsys(s, sm->_all_systems) {
		count += s->_check_all ? s->_check_all(exec_list, cur_tick) : 0;
	}
	if (sm->_all_entities) sm->_all_entities->unlock();
	sm->unlock();
	return count;
}

static int SM_check_allsys_asop(AOperator *asop, int result)
{
	ASystemManager *sm = container_of(asop, ASystemManager, _asop_systems);
	if ((result < 0) || (sm->_tick_systems == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_allsys(sm, &exec_list, GetTickCount());

	if (exec_list.empty() && sm->_check_entities_when_idle && sm->_all_entities) {
		result = sm->check_entities(sm, sm->_all_entities, &exec_list, GetTickCount());
	}

	sm->_exec_results(exec_list);
	asop->delay(sm->_thr_systems, sm->_tick_systems);
	return result;
}

static int SM_check_entities_asop(AOperator *asop, int result)
{
	AEntityManager *em = container_of(asop, AEntityManager, _asop_entities);
	ASystemManager *sm = em->_sysmng;
	if ((result < 0) || (em->_tick_entities == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_entities(sm, em, &exec_list, GetTickCount());

	if (exec_list.empty()) {
		asop->delay(em->_thr_entities, em->_tick_entities);
	} else {
		asop->post(em->_thr_entities);
		sm->_exec_results(exec_list);
	}
	return result;
}

static int SM_start_checkall(ASystemManager *sm)
{
	if (sm->_asop_systems.done == NULL)
		sm->_asop_systems.done = &SM_check_allsys_asop;
	sm->_asop_systems.post(sm->_thr_systems);

	AEntityManager *em = sm->_all_entities;
	if (em != NULL) {
		em->_sysmng = sm;

		if (em->_asop_entities.done == NULL)
			em->_asop_entities.done = &SM_check_entities_asop;
		em->_asop_entities.post(em->_thr_entities);
	}
	return 0;
}

static void SM_stop_checkall(ASystemManager *sm)
{
	sm->_tick_systems = 0;
	sm->_asop_systems.signal(sm->_thr_systems, TRUE);

	AEntityManager *em = sm->_all_entities;
	if (em != NULL) {
		em->_tick_entities = 0;
		em->_asop_entities.signal(em->_thr_entities, TRUE);
	}
	while ((sm->_asop_systems.done != NULL) || (em && em->_asop_entities.done != NULL))
		Sleep(20);
}

extern struct SM_m {
	AModule module;
	union {
		ASystemManagerMethod method;
		ASystemManager manager;
	};
} SM_default;

static int SM_init(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
		SM_default.manager.init(&SM_default.method);
		SM_default.manager._all_entities = AEntityManager::get();
		SM_default.manager._event_manager = AEventManager::get();
	}
	return 1;
}

static void SM_exit(int inited)
{
	if (inited > 0) {
		SM_default.manager.exit();
	}
}

static SM_m SM_default = { {
	ASystemManager::name(),
	ASystemManager::name(),
	0, NULL, NULL,
}, {
	&SM_start_checkall,
	&SM_stop_checkall,
	&SM_check_allsys,
	&SM_check_entities,
	&SM_check_one,
} };
static int reg_sys = AModuleRegister(&SM_default.module);
