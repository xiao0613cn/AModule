#include "stdafx.h"
#include "ASystem.h"
#include "AEntity.h"
#include "AEvent.h"

static inline int AEntityCmp(AEntity *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_define(AEntity, _map_node, AEntity*, AEntityCmp)

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

static int check_entities(ASystemManager *sm, list_head *exec_list, DWORD cur_tick)
{
	AEntityManager *em = sm->_all_entities;
	if (em == NULL) {
		TRACE("no manager for checking...\n");
		return 0;
	}

	int result = 0;
	em->lock();
	em->_last_entity = em->_upper(em->_last_entity);
	while (em->_last_entity != NULL)
	{
		result = sm->_check_one(sm, exec_list, em->_last_entity, cur_tick);
		if (result > 0)
			break;
		em->_last_entity = em->_next(em->_last_entity);
	}
	em->unlock();
	return result;
}

static int check_allsys(ASystemManager *sm, list_head *exec_list, DWORD cur_tick)
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

template <bool by_name>
static bool _subscribe(ASystemManager *sm, AReceiver *r)
{
	if (sm->_event_manager == NULL)
		return false;
	return by_name ? sm->_event_manager->_sub_by_name(r) : sm->_event_manager->_sub_by_index(r);
}

template <bool by_name>
static bool _unsubscribe(ASystemManager *sm, AReceiver *r)
{
	if (sm->_event_manager == NULL)
		return false;
	return by_name ? sm->_event_manager->_unsub_by_name(r) : sm->_event_manager->_unsub_by_index(r);
}

static int emit_by_name(ASystemManager *sm, const char *name, void *p)
{
	if (sm->_event_manager == NULL)
		return -1;
	return sm->_event_manager->emit_by_name(name, p);
}

static int emit_by_index(ASystemManager *sm, int index, void *p)
{
	if (sm->_event_manager == NULL)
		return -1;
	return sm->_event_manager->emit_by_index(index, p);
}

struct AReceiver2 : public AReceiver {
	void     *_self;
	ASelfEventFunc _func;
};

static int on_self_event(AReceiver *r, void *p, bool preproc)
{
	AReceiver2 *r2 = (AReceiver2*)r;
	if (preproc) {
		return (r2->_self != p) ? -1 : 1;
	}
	return r2->_func(r2->_name, preproc, p);
}

static AReceiver* _sub_self(ASystemManager *sm, const char *name, bool oneshot, void *self, ASelfEventFunc f)
{
	if (sm->_event_manager == NULL)
		return NULL;

	AReceiver2 *r2 = gomake(AReceiver2);
	r2->AReceiver::init(NULL, &free);
	r2->on_event = &on_self_event;

	r2->_name = name; r2->_oneshot = oneshot; r2->_preproc = true;
	r2->_self = self; r2->_func = f;
	sm->_event_manager->_sub_by_name(r2);
	return r2;
}

static int check_allsys_asop(AOperator *asop, int result)
{
	ASystemManager *sm = container_of(asop, ASystemManager, _asop_systems);
	if ((result < 0) || (sm->_tick_systems == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_allsys(sm, &exec_list, GetTickCount());

	if (exec_list.empty() && sm->_check_entities_when_idle) {
		result = sm->check_entities(sm, &exec_list, GetTickCount());
	}

	sm->_exec_results(exec_list);
	asop->delay(sm->_thr_systems, sm->_tick_systems);
	return result;
}

static int check_entities_asop(AOperator *asop, int result)
{
	AEntityManager *em = container_of(asop, AEntityManager, _asop_entities);
	ASystemManager *sm = em->_sysmng;
	if ((result < 0) || (em->_tick_entities == 0)) {
		asop->done = NULL;
		return result;
	}

	struct list_head exec_list; exec_list.init();
	result = sm->check_entities(sm, &exec_list, GetTickCount());

	if (exec_list.empty()) {
		asop->delay(em->_thr_entities, em->_tick_entities);
	} else {
		asop->post(em->_thr_entities);
		sm->_exec_results(exec_list);
	}
	return result;
}

static int start_checkall(ASystemManager *sm)
{
	if (sm->_asop_systems.done == NULL)
		sm->_asop_systems.done = &check_allsys_asop;
	sm->_asop_systems.post(sm->_thr_systems);

	AEntityManager *em = sm->_all_entities;
	if (em != NULL) {
		em->_sysmng = sm;

		if (em->_asop_entities.done == NULL)
			em->_asop_entities.done = &check_entities_asop;
		em->_asop_entities.post(em->_thr_entities);
	}
	return 0;
}

static void stop_checkall(ASystemManager *sm)
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

template <bool by_name>
static void clear_sub(ASystemManager *sm)
{
	if (sm->_event_manager == NULL)
		return;
	by_name ? sm->_event_manager->clear_sub() : sm->_event_manager->clear_sub2();
}

ASystemManagerDefaultModule SysMngModule = { {
	ASystemManagerDefaultModule::name(),
	ASystemManagerDefaultModule::name(),
	0, NULL, NULL,
}, {
	&start_checkall,
	&stop_checkall,
	&check_allsys,
	&check_entities,
	&_check_one,

	&_subscribe<true>,
	&_unsubscribe<true>,
	&emit_by_name,
	&_sub_self,
	&clear_sub<true>,

	&_subscribe<false>,
	&_unsubscribe<false>,
	&emit_by_index,
	&clear_sub<false>
} };
static int reg_sys = AModuleRegister(&SysMngModule.module);
