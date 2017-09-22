#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"
#include "AEntity.h"


typedef struct ASystem ASystem;
typedef struct ASystemManager ASystemManager;

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
		ASystem *system;
		Status status;
	};

	Result* (*exec_check)(AEntity *e, DWORD cur_tick);
	int    (*exec_run)(AEntity *e, Result *r, int result);
	int    (*exec_abort)(AEntity *e, Result *r);

	ASystemManager *_manager;
	AThread  *_exec_thread;

#ifdef _AMODULE_H_
	static ASystem* find(const char *name) {
		AModule *m = AModuleFind("ASystem", name);
		return m ? container_of(m, ASystem, module) : NULL;
	}
#endif
};

struct ASystemManager {
	list_head       *_system_list; // AModuleFind("ASystem", NULL);

	AEntityManager  *_entity_manager;
	pthread_mutex_t *_entity_mutex;
	AEntity         *_last_check;
	void entity_lock() { pthread_mutex_lock(_entity_mutex); }
	void entity_unlock() { pthread_mutex_unlock(_entity_mutex); }

	struct AEventManager *_event_manager;
	pthread_mutex_t *_event_mutex;
	void event_lock() { pthread_mutex_lock(_event_mutex); }
	void event_unlock() { pthread_mutex_unlock(_event_mutex); }

	struct check_item {
		AEntity *entity;
		list_head results;
	};
	bool _check_one(check_item &c, AEntity *e, DWORD cur_tick) {
		c.entity = e;
		INIT_LIST_HEAD(&c.results);

		list_for_each2(s, _system_list, ASystem, module.class_entry) {
			ASystem::Result *r = s->exec_check(c.entity, cur_tick);
			if (r != NULL) {
				r->system = s;
				list_add_tail(&r->node, &c.results);
			}
		}
		return !list_empty(&c.results);
	}
	void exec_check(check_item *check_list, int max_count) {
		int check_count = 0;
		DWORD cur_tick = GetTickCount();

		entity_lock();
		_last_check = _entity_manager->_upper(_last_check);
		while (_last_check != NULL)
		{
			check_item &c = check_list[check_count];
			if (_check_one(c, _last_check, cur_tick)) {
				c.entity->_self->addref();
				if (++check_count >= max_count)
					break;
			}
			_last_check = _entity_manager->_next(_last_check);
		}
		entity_unlock();

		while (check_count > 0) {
			check_item &c = check_list[--check_count];

			while (!list_empty(&c.results)) {
				ASystem::Result *r = list_pop_front(&c.results, ASystem::Result, node);
				switch (r->status)
				{
				case ASystem::Aborting: r->system->exec_abort(c.entity, r); break;
				case ASystem::Runnable: r->system->exec_run(c.entity, r, 0); break;
				}
			}
			c.entity->_self->release();
		}
	}
};

#endif
