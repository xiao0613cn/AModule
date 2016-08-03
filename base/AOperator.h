#ifndef _AOPERATOR_H_
#define _AOPERATOR_H_

typedef struct AThread AThread;
typedef struct AOperator AOperator;

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AThreadBegin(AThread **at, AThread *pool);

AMODULE_API int
AThreadEnd(AThread *at);

AMODULE_API int
AThreadAbort(AThread *at);

#ifdef _WIN32
AMODULE_API int
AThreadBind(AThread *at, HANDLE file);
#else
AMODULE_API int
AThreadBind(AThread *at, AOperator *asop, uint32_t event);

#define AThreadUnbind(asop)  AThreadBind(NULL, asop, 0)
#endif

AMODULE_API AThread*
AThreadDefault(int ix);


//////////////////////////////////////////////////////////////////////////
#pragma warning(disable: 4201)
struct AOperator {
	void  (*callback)(AOperator *asop, int result);

	union {
	struct {
	DWORD            ao_tick;
	struct rb_node   ao_tree;
	struct list_head ao_list;
	};
#ifdef _WIN32
	OVERLAPPED       ao_ovlp;
#else
	struct {
	int              ao_fd;
	uint32_t         ao_events;
	AThread         *ao_thread;
	};
#endif
	};
};
#pragma warning(default: 4201)

AMODULE_API int
AOperatorPost(AOperator *asop, AThread *at, DWORD tick);

AMODULE_API int
AOperatorSignal(AOperator *asop, AThread *at, int cancel);

static inline int
AOperatorTimewait(AOperator *asop, AThread *at, DWORD timeout) {
	if ((timeout != 0) && (timeout != INFINITE)) {
		timeout += GetTickCount();
		if ((timeout == 0) || (timeout == INFINITE))
			timeout += 2;
	}
	return AOperatorPost(asop, at, timeout);
}



#endif
