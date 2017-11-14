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

static int _check_one(ASystemManager *sm, list_head *results, AEntity *e, DWORD cur_tick)
{
	int count = 0;
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	ASystem::Result *r = sm->_all_systems->check_one ? sm->_all_systems->check_one(e, cur_tick) : NULL;
	if (r != NULL) {
		r->system = sm->_all_systems;
		results->push_back(&r->node);
		count ++;
	}

	list_for_allsys(s, sm->_all_systems) {
		r = s->check_one ? s->check_one(e, cur_tick) : NULL;
		if (r != NULL) {
			r->system = s;
			results->push_back(&r->node);
			count ++;
		}
	}
	return count;
}

static int _check_by_manager(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick)
{
	int check_count = 0;
	if (sm->_entity_manager == NULL) {
		TRACE("no manager for checking...\n");
		return 0;
	}

	//sm->_entity_manager->lock();
	sm->_last_check = sm->_entity_manager->_upper(sm->_last_check);
	while (sm->_last_check != NULL)
	{
		list_head &results = results_list[check_count];
		results.init();
		if (sm->_check_one(sm, &results, sm->_last_check, cur_tick) > 0) {
			if (++check_count >= max_count)
				break;
		}
		sm->_last_check = sm->_entity_manager->_next(sm->_last_check);
	}
	//sm->_entity_manager->unlock();

	return check_count;
}

static int _check_by_allsys(ASystemManager *sm, list_head *results, DWORD cur_tick)
{
	if (sm->_all_systems == NULL) {
		TRACE("no systems for checking entity...\n");
		return 0;
	}

	//sm->lock();
	int count = sm->_all_systems->check_all ? sm->_all_systems->check_all(results, cur_tick) : 0;
	list_for_allsys(s, sm->_all_systems) {
		count += s->check_all ? s->check_all(results, cur_tick) : 0;
	}
	//sm->unlock();
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
	//if (sm->_event_manager == NULL)
		return -1;
	//return sm->_event_manager->emit(name, p);
}

struct AReceiver2 : public AReceiver {
	void  *_user;
	int  (*_func)(void *user, const char *name, void *p, bool preproc);

	static AReceiver2* create() {
		AReceiver2 *r2 = gomake(AReceiver2);
		r2->AObject::init(NULL, &free);

		r2->AReceiver::init();
		r2->on_event = &AReceiver2::on_user_event;
		return r2;
	}
	static int on_user_event(AReceiver *r, void *p, bool preproc) {
		AReceiver2 *r2 = (AReceiver2*)r;
		return r2->_func(r2->_user, r2->_name, p, preproc);
	}
};

static AReceiver* _sub_const(ASystemManager *sm, const char *name, bool preproc, void *user,
		int (*f)(void *user, const char *name, void *p, bool preproc))
{
	if (sm->_event_manager == NULL)
		return NULL;

	AReceiver2 *r2 = AReceiver2::create();
	r2->_name = name; r2->_oneshot = false; r2->_preproc = preproc;
	r2->_user = user; r2->_func = f;
	sm->_event_manager->_subscribe(r2);
	return r2;
}

ASystemManagerDefaultModule SysMngModule = {
{
	ASystemManagerDefaultModule::name(),
	ASystemManagerDefaultModule::name(),
	0, NULL, NULL,
}, {
	&_check_by_allsys,
	&_check_by_manager,
	&_check_one,
	&_subscribe,
	&_unsubscribe,
	&emit_event,
	&emit_event2,
	&_sub_const,
} };
static int reg_sys = AModuleRegister(&SysMngModule.module);
