#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"

struct ASystem;
struct ASystemManager;
struct AEntity;
struct AEntityManager;
struct AEventManager;
struct AService;

struct ASystem {
	AModule module;
	static const char* class_name() { return "ASystem"; }

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

	int    (*regist)(AEntity *e);
	int    (*unregist)(AEntity *e);

	int    (*check_all)(list_head *results, DWORD cur_tick);
	Result* (*check_one)(AEntity *e, DWORD cur_tick);

	int    (*exec_run)(Result *r, int result);
	int    (*exec_abort)(Result *r);

	static ASystem* find(const char *sys_name) {
		AModule *m = AModuleFind(class_name(), sys_name);
		return m ? container_of(m, ASystem, module) : NULL;
	}
	void _exec(Result *r) {
		switch (r->status)
		{
		case Runnable: exec_run(r, 0); break;
		case Aborting: exec_abort(r); break;
		default: assert(0); break;
		}
	}
};

struct ASystemManager {
	ASystem  *_systems;
	AThread  *_exec_thread;
	AService *_services;
	pthread_mutex_t  *_mutex;
	void   lock()   { _mutex ? pthread_mutex_lock(_mutex) : 0; }
	void   unlock() { _mutex ? pthread_mutex_unlock(_mutex) : 0; }
	int  (*check_allsys)(ASystemManager *sm, DWORD cur_tick);

	AEntityManager  *_entity_manager;
	AEntity         *_last_check;
	void (*check_entity)(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick);

	AEventManager *_event_manager;
	int (*emit)(ASystemManager *sm, const char *name, void *p);

	void init();

	void _regist(AEntity *e) {
		_systems->regist ? _systems->regist(e) : 0;
		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry)
			s->regist ? s->regist(e) : 0;
	}
	void _unregist(AEntity *e) {
		_systems->unregist ? _systems->unregist(e) : 0;
		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry)
			s->unregist ? s->unregist(e) : 0;
	}
	void _exec_results(list_head &results) {
		while (!results.empty()) {
			ASystem::Result *r = list_pop_front(&results, ASystem::Result, node);
			r->manager = this;
			r->system->_exec(r);
		}
	}
};

#endif
