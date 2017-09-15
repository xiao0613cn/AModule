#ifndef _ASYSTEM_H_
#define _ASYSTEM_H_

#include "AEntity.h"

struct AObject;
typedef struct ASystem ASystem;
typedef struct ASystemManager ASystemManager;

struct ASystem : public AEntity {
	AThread  *_exec_thread;

	enum Result {
		Invalid = 0,
		Abort,
		Success,
	};
	Result (*exec_check)(ASystem *s, AEntity *e, DWORD cur_tick);
	int    (*exec_one)(ASystem *s, AEntity *e, int result);
	void   (*exec_post)(ASystem *s, AEntity *e, bool addref);
};

struct ASystemManager : public AEntityManager {
	AEntity *_exec_lastone;
};


#endif
