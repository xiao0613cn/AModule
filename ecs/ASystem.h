#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "../base/AModule_API.h"

struct ASystem;
struct ASystemManager;
struct AEntity;
struct AEntityManager;
struct AEventManager;
struct AReceiver;
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

#define list_for_allsys(pos, allsys) \
	list_for_each2(pos, &(allsys)->module.class_entry, ASystem, module.class_entry)

struct ASystemManagerMethod {
	int  (*_check_by_allsys)(ASystemManager *sm, list_head *results, DWORD cur_tick);
	int  (*_check_by_manager)(ASystemManager *sm, list_head *results_list, int max_count, DWORD cur_tick);
	int  (*_check_one)(ASystemManager *sm, list_head *results, AEntity *e, DWORD cur_tick);

	bool (*_subscribe)(ASystemManager *sm, AReceiver *r);
	bool (*_unsubscribe)(ASystemManager *sm, AReceiver *r);
	int  (*emit_event)(ASystemManager *sm, const char *name, void *p);
	int  (*emit_event2)(ASystemManager *sm, int index, void *p);
	AReceiver* (*_sub_const)(ASystemManager *sm, const char *name, bool preproc, void *user,
		int (*f)(void *user, const char *name, void *p, bool preproc));
};

struct ASystemManagerDefaultModule {
	static const char* name() { return "ASystemManagerDefaultModule"; }
	static ASystemManagerDefaultModule* get() { return (ASystemManagerDefaultModule*)AModuleFind(name(), name()); }

	AModule module;
	ASystemManagerMethod methods;
};

struct ASystemManager : public ASystemManagerMethod {
	ASystem         *_all_systems;
	AThread         *_exec_thread;
	AService        *_all_services;
	pthread_mutex_t *_mutex;
	void   lock()   { _mutex ? pthread_mutex_lock(_mutex) : 0; }
	void   unlock() { _mutex ? pthread_mutex_unlock(_mutex) : 0; }

	AEntityManager  *_entity_manager;
	AEntity         *_last_check;
	AEventManager   *_event_manager;

	void init(ASystemManagerDefaultModule *m) {
		if (m != NULL) {
			_all_systems = ASystem::find(NULL);
			_exec_thread = NULL;
			_all_services = NULL;
			_mutex = NULL;

			_entity_manager = NULL;
			_last_check = NULL;
			_event_manager = NULL;
			*(ASystemManagerMethod*)this = m->methods;
		} else {
			memset(this, 0, sizeof(*this));
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
};

#endif
