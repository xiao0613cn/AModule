#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule.h"
#include "AEntity.h"


typedef struct ASystem ASystem;
typedef struct ASystemManager ASystemManager;

struct ASystem : public AModule {
	enum Status {
		NotNeed = 0,
		Pending,
		Aborting,
		Runnable,
		EndExit,
	};
	struct Result {
		list_head node;
		ASystem *system;
		Status status;
	};
	AThread  *_exec_thread;

	Result* (*exec_check)(ASystem *s, AEntity *e, DWORD cur_tick);
	int    (*exec_abort)(ASystem *s, Result *r);
	int    (*exec_run)(ASystem *s, Result *r, int result);
	int    (*exec_exit)(ASystem *s, Result *r);

#ifdef _AMODULE_H_
	static ASystem* create(const char *name) {
		AObject *o = NULL;
		int result = AObjectCreate(&o, NULL, NULL, name);
		if ((result < 0) || (o == NULL))
			return NULL;
		return (ASystem*)(AEntity*)(AEntity2*)o;
	}
#endif
};

struct ASystemManager {
	list_head       *_system_list;
	AEntityManager  *_entity_manager;
	pthread_mutex_t *_entity_mutex;
	AEntity         *_last_check;

	struct check_item {
		AEntity *entity;
		list_head results;
	}
	bool _check_one(check_item &c, AEntity *e, DWORD cur_tick) {
		c.entity = e;
		INIT_LIST_HEAD(&c.results);

		ASystem *s;
		list_for_each_entry(s, _system_list, ASystem, class_entry) {
			ASystem::Result *r = s->exec_check(s, c.entity, cur_tick);
			if (r != NULL)
				list_add_tail(&r->node, &c.results);
		}
		return !list_empty(&c.results);
	}
	template <int max_count = 16>
	void exec_check() {
		check_item check_list[max_count];
		int check_count = 0;
		DWORD cur_tick = GetTickCount();

		pthread_mutex_lock(_entity_mutex);
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
		pthread_mutex_unlock(_entity_mutex);

		while (check_count > 0) {
			check_item &c = check_list[--check_count];

			while (!list_empty(&c.results)) {
				ASystem::Result *r = list_pop_front(&c.results, ASystem::Result, node);
				switch (r->result)
				{
				case ASystem::Aborting: r->system->exec_abort(r->system, r); break;
				case ASystem::Runnable: r->system->exec_run(r->system, r, 0); break;
				case ASystem::EndExit:  r->system->exec_exit(r->system, r); break;
				}
			}
			c.entity->_self->release2();
		}
	}
};

template <typename TSystem, typename TComponent, int com_index>
ASystem::Result ExecCheck(ASystem *s, AEntity *e, DWORD cur_tick) {
	TSystem *cs = (TSystem*)s;
	TComponent *cc = e->_get<TComponent>(com_index);
	if (cc == NULL)
		return ASystem::NotNeed;
	return cs->_exec_check(cc, cur_tick);
}

template <typename TSystem, typename TComponent, int com_index>
int ExecOne(ASystem *s, AEntity *e, int result) {
	TSystem *cs = (TSystem*)s;
	TComponent *cc = e->_get<TComponent>(com_index);
	assert(cc != NULL);
	cs->_exec_one(cc, result);
}

template <typename TSystem, typename TComponent, int com_index>
void ExecPost(ASystem *s, AEntity *e, bool addref) {
	TSystem *cs = (TSystem*)s;
	TComponent *cc = (TComponent*)e->_get(TComponent::name(), com_index);
	assert(cc != NULL);
	cc->_system_asop.post(cs->_exec_thread);
}

#endif
