#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "AEntity.h"

struct AObject;
typedef struct ASystem ASystem;
typedef struct ASystemManager ASystemManager;

struct ASystem : public AEntity {
	AThread  *_exec_thread;

	enum Result {
		NotNeed = 0,
		Pending,
		Aborting,
		Executable,
		EndExit,
	};
	Result (*exec_check)(ASystem *s, AEntity *e, DWORD cur_tick);
	int    (*exec_one)(ASystem *s, AEntity *e, int result);
	void   (*exec_post)(ASystem *s, AEntity *e, bool addref);

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

struct ASystemManager : public AEntityManager {
	AEntity *_exec_lastone;
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
