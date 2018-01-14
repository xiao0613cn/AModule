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

typedef int (*ASelfEventFunc)(const char *name, bool preproc, void *self);

struct ASystemManagerMethod {
	int  (*start_checkall)(ASystemManager *sm);
	void (*stop_checkall)(ASystemManager *sm);
	int  (*check_allsys)(ASystemManager *sm, list_head *exec_list, DWORD cur_tick);
	int  (*check_entities)(ASystemManager *sm, list_head *exec_list, DWORD cur_tick);
	int  (*_check_one)(ASystemManager *sm, list_head *exec_list, AEntity *e, DWORD cur_tick);

	// event by name
	bool (*_sub_by_name)(ASystemManager *sm, AReceiver *r);
	bool (*_unsub_by_name)(ASystemManager *sm, AReceiver *r);
	int  (*emit_by_name)(ASystemManager *sm, const char *name, void *p);
	AReceiver* (*_sub_self)(ASystemManager *sm, const char *name, bool oneshot, void *self, ASelfEventFunc f);
	void (*clear_sub)(ASystemManager *sm);

	// event by index
	bool (*_sub_by_index)(ASystemManager *sm, AReceiver *r);
	bool (*_unsub_by_index)(ASystemManager *sm, AReceiver *r);
	int  (*emit_by_index)(ASystemManager *sm, int index, void *p);
	void (*clear_sub2)(ASystemManager *sm);
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
	bool             _check_entities_when_idle;
	pthread_mutex_t  _mutex;
	void   lock()   { pthread_mutex_lock(&_mutex); }
	void   unlock() { pthread_mutex_unlock(&_mutex); }

	AEntityManager  *_all_entities;
	AService        *_all_services;
	AEventManager   *_event_manager;

	void init(ASystemManagerMethod *m) {
		if (m != NULL) {
			_all_systems = ASystem::find(NULL);
			_thr_systems = NULL;
			_asop_systems.timer();
			_tick_systems = 1000;
			_check_entities_when_idle = false;
			pthread_mutex_init(&_mutex, NULL);

			_all_entities = NULL;
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
