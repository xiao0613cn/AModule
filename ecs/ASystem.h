#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"
#include "AEntity.h"

typedef struct ASystem ASystem;
typedef struct ASystemManager ASystemManager;
typedef struct AEventManager AEventManager;

struct ASystem {
	AModule module;
	static const char* name() { return "ASystem"; }

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
		AModule *m = AModuleFind(name(), sys_name);
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
	void event_lock() { _event_mutex ? pthread_mutex_lock(_event_mutex) : 0; }
	void event_unlock() { _event_mutex ? pthread_mutex_unlock(_event_mutex) : 0; }

	void init() {
		_systems = ASystem::find(NULL);
		_exec_thread = NULL;
		_entity_mutex = NULL;
		_event_mutex = NULL;
	}
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
	void check_entity(list_head *results_list, int max_count, DWORD cur_tick) {
		int check_count = 0;

		entity_lock();
		_last_check = _entity_manager->_upper(_last_check);
		while (_last_check != NULL)
		{
			list_head &results = results_list[check_count];
			results.init();
			if (_check_one(results, _last_check, cur_tick) > 0) {
				if (++check_count >= max_count)
					break;
			}
			_last_check = _entity_manager->_next(_last_check);
		}
		entity_unlock();

		while (check_count > 0) {
			_exec(results_list[--check_count]);
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
	int check_allsys(DWORD cur_tick) {
		list_head results; results.init();

		entity_lock();
		int count = _systems->check_all(&results, cur_tick);
		list_for_each2(s, &_systems->module.class_entry, ASystem, module.class_entry)
			count += s->check_all(&results, cur_tick);
		entity_unlock();

		_exec(results);
		return count;
	}
};

#endif
