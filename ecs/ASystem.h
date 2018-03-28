#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"

struct ASystem;
struct ASystemManager;
struct AEntity;
struct AEntityManager;
struct AReceiver;
struct AEventManager;
struct AService;

struct ASystem {
	AModule module;

	enum Status {
		NotNeed = 0,
		Pending,
		Runnable,
		Aborting,
	};
	struct Result {
		list_head node;
		Status status;
		ASystem *system;
		ASystemManager *manager;
	};

	int    (*_regist)(AEntity *e);
	int    (*_unregist)(AEntity *e);
	int    (*_clear_all)(bool abort);

	int    (*_check_all)(list_head *results, DWORD cur_tick);
	Result* (*_check_one)(AEntity *e, DWORD cur_tick);

	int    (*exec_run)(Result *r, int result);
	int    (*exec_abort)(Result *r);

	static ASystem* find(const char *sys_name) {
		AModule *m = AModuleFind("ASystem", sys_name);
		return m ? container_of(m, ASystem, module) : NULL;
	}
	int _exec(Result *r) {
		switch (r->status)
		{
		case Runnable: return exec_run(r, 0);
		case Aborting: return exec_abort(r);
		default: assert(0); return -EINVAL;
		}
	}
};

#define list_for_allsys(pos, allsys) \
	list_for_each2(pos, &(allsys)->module.class_entry, ASystem, module.class_entry)


struct ASystemManagerMethod {
	int  (*start_checkall)(ASystemManager *sm);
	void (*stop_checkall)(ASystemManager *sm);
	int  (*check_allsys)(ASystemManager *sm, list_head *exec_list, DWORD cur_tick);
	int  (*check_entities)(ASystemManager *sm, AEntityManager *em, list_head *exec_list, DWORD cur_tick);
	int  (*_check_one)(ASystemManager *sm, list_head *exec_list, AEntity *e, DWORD cur_tick);
};

struct ASystemManager : public ASystemManagerMethod {
	static const char* name() { return "ASystemManagerDefaultModule"; }
	static ASystemManager* get() { return AModule::singleton_data<ASystemManager>(); }

	ASystem         *_all_systems;
	AThread         *_thr_systems;
	AOperator        _asop_systems; // single operator execute
	DWORD            _tick_systems;
	bool             _check_entities_when_idle;
	pthread_mutex_t  _mutex;
	void   lock()   { pthread_mutex_lock(&_mutex); }
	void   unlock() { pthread_mutex_unlock(&_mutex); }

	AEntityManager  *_all_entities;
	AService        *_all_services;
	AEventManager   *_event_manager;

	void init(ASystemManagerMethod *m) {
		if (m != this)
			*(ASystemManagerMethod*)this = *m;

		_all_systems = ASystem::find(NULL);
		_thr_systems = NULL;
		_asop_systems.timer();
		_tick_systems = 1000;
		_check_entities_when_idle = false;
		pthread_mutex_init(&_mutex, NULL);

		_all_entities = NULL;
		_all_services = NULL;
		_event_manager = NULL;
	}
	void exit() {
		pthread_mutex_destroy(&_mutex);
	}
	void _regist(AEntity *e) {
		_all_systems->_regist ? _all_systems->_regist(e) : 0;
		list_for_allsys(s, _all_systems) {
			s->_regist ? s->_regist(e) : 0;
		}
	}
	void _unregist(AEntity *e) {
		_all_systems->_unregist ? _all_systems->_unregist(e) : 0;
		list_for_allsys(s, _all_systems) {
			s->_unregist ? s->_unregist(e) : 0;
		}
	}
	void _exec_results(list_head &results) {
		while (!results.empty()) {
			ASystem::Result *r = list_pop_front(&results, ASystem::Result, node);
			r->manager = this;
			r->system->_exec(r);
		}
	}
#ifdef _AENTITY_H_
	void clear_allsys(bool abort) {
		lock();
		if (_all_entities) _all_entities->lock();
		_all_systems->_clear_all ? _all_systems->_clear_all(abort) : 0;
		list_for_allsys(s, _all_systems) {
			s->_clear_all ? s->_clear_all(abort) : 0;
		}
		if (_all_entities) _all_entities->unlock();
		unlock();
	}
#endif
#ifdef _AEVENT_H_
	int emit_by_name(const char *name, void *p) {
		if (_event_manager == NULL)
			return -EINVAL;
		return _event_manager->emit_by_name(_event_manager, name, p);
	}
#endif
};

#endif
