#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"

struct ASystem;
struct ASystemManager;
struct AEntity;
struct AEntityManager;
struct AEventManager;

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

	AEntityManager  *_entity_manager;
	pthread_mutex_t *_entity_mutex;
	AEntity         *_last_check;
	void entity_lock() { _entity_mutex ? pthread_mutex_lock(_entity_mutex) : 0; }
	void entity_unlock() { _entity_mutex ? pthread_mutex_unlock(_entity_mutex) : 0; }

	AEventManager *_event_manager;
	pthread_mutex_t *_event_mutex;
	list_head _free_recvers;
	void event_lock() { _event_mutex ? pthread_mutex_lock(_event_mutex) : 0; }
	void event_unlock() { _event_mutex ? pthread_mutex_unlock(_event_mutex) : 0; }

	void (*check_entity)(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick);
	int  (*check_allsys)(ASystemManager *sm, DWORD cur_tick);
	int  (*emit)(ASystemManager *sm, const char *name, void *p);

	void init() {
		_systems = ASystem::find(NULL);
		_exec_thread = NULL;
		_entity_mutex = NULL;
		_event_mutex = NULL;
		_free_recvers.init();

		check_entity = &_do_check_entity;
		check_allsys = &_do_check_allsys;
		emit = &_do_emit;
	}
	static void _do_check_entity(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick);
	static int  _do_check_allsys(ASystemManager *sm, DWORD cur_tick);
	static int  _do_emit(ASystemManager *sm, const char *name, void *p);

	int _check_one(list_head &results, AEntity *e, DWORD cur_tick) {
		int count = 0;

		ASystem::Result *r = _systems->check_one(e, cur_tick);
		if (r != NULL) {
			r->system = _systems;
			results.push_back(&r->node);
			count ++;
		}

		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry) {
			r = s->check_one(e, cur_tick);
			if (r != NULL) {
				r->system = s;
				results.push_back(&r->node);
				count ++;
			}
		}
		return count;
	}
	void _exec(list_head &results) {
		while (!results.empty()) {
			ASystem::Result *r = list_pop_front(&results, ASystem::Result, node);
			r->manager = this;
			r->system->_exec(r);
		}
	}
	void _regist(AEntity *e) {
		_systems->regist(e);
		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry)
			s->regist(e);
	}
	void _unregist(AEntity *e) {
		_systems->unregist(e);
		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry)
			s->unregist(e);
	}
};

#endif
