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
	int    (*clear_all)(bool abort);

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

#define list_for_allsys(pos, allsys) \
	list_for_each2(pos, &(allsys)->module.class_entry, ASystem, module.class_entry)

typedef int (*ASelfFunc)(const char *name, bool preproc, void *user);

struct ASystemManagerMethod {
	int  (*start_checkall)(ASystemManager *sm);
	void (*stop_checkall)(ASystemManager *sm);
	int  (*check_by_allsys)(ASystemManager *sm, list_head *exec_list, DWORD cur_tick);
	int  (*check_by_entities)(ASystemManager *sm, list_head *exec_list, DWORD cur_tick);
	int  (*_check_one)(ASystemManager *sm, list_head *exec_list, AEntity *e, DWORD cur_tick);

	bool (*_subscribe)(ASystemManager *sm, AReceiver *r);
	bool (*_unsubscribe)(ASystemManager *sm, AReceiver *r);
	int  (*emit_event)(ASystemManager *sm, const char *name, void *p);
	int  (*emit_event2)(ASystemManager *sm, int index, void *p);
	AReceiver* (*_sub_self)(ASystemManager *sm, const char *name, bool preproc, void *user, ASelfFunc f);
	void (*clear_sub)(ASystemManager *sm);
};

struct ASystemManagerDefaultModule {
	AModule module;
	ASystemManagerMethod methods;

	static const char* name() { return "ASystemManagerDefaultModule"; }
	static ASystemManagerDefaultModule* get() { return (ASystemManagerDefaultModule*)AModuleFind(name(), name()); }
};

struct ASystemManager : public ASystemManagerMethod {
	ASystem         *_all_systems;
	AThread         *_thr_systems;
	AOperator        _asop_systems; // single operator execute
	DWORD            _tick_systems;
	bool             _idle_check_entities;
	pthread_mutex_t  _mutex;
	void   lock()   { pthread_mutex_lock(&_mutex); }
	void   unlock() { pthread_mutex_unlock(&_mutex); }

	AEntityManager  *_all_entities;
	AThread         *_thr_entities;
	AOperator        _asop_entities; // multiple operator execute
	DWORD            _tick_entities;
	AEntity         *_last_entity;

	AService        *_all_services;
	AEventManager   *_event_manager;

	void init(ASystemManagerMethod *m) {
		if (m != NULL) {
			_all_systems = ASystem::find(NULL);
			_thr_systems = NULL;
			_asop_systems.timer();
			_tick_systems = 1000;
			_idle_check_entities = false;
			pthread_mutex_init(&_mutex, NULL);

			_all_entities = NULL;
			_thr_entities = NULL;
			_asop_entities.timer();
			_tick_entities = 1000;
			_last_entity = NULL;

			_all_services = NULL;
			_event_manager = NULL;
			*(ASystemManagerMethod*)this = *m;
		} else {
			memzero(*this);
		}
	}
	void _regist(AEntity *e) {
		_all_systems->regist ? _all_systems->regist(e) : 0;
		list_for_allsys(s, _all_systems) {
			s->regist ? s->regist(e) : 0;
		}
	}
	void _unregist(AEntity *e) {
		_all_systems->unregist ? _all_systems->unregist(e) : 0;
		list_for_allsys(s, _all_systems) {
			s->unregist ? s->unregist(e) : 0;
		}
	}
	void _exec_results(list_head &results) {
		while (!results.empty()) {
			ASystem::Result *r = list_pop_front(&results, ASystem::Result, node);
			r->manager = this;
			r->system->_exec(r);
		}
	}
	void clear_allsys(bool abort) {
		lock();
		_all_systems->clear_all ? _all_systems->clear_all(abort) : 0;
		list_for_allsys(s, _all_systems) {
			s->clear_all ? s->clear_all(abort) : 0;
		}
		unlock();
	}
};

#endif
