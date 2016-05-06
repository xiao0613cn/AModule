#ifndef _AOPERATOR_H_
#define _AOPERATOR_H_


typedef struct AThread AThread;
struct AThread {
	BOOL volatile    running;
	HANDLE           thread;
	HANDLE           iocp;
	AThread         *attach;

	CRITICAL_SECTION ao_lock;
	struct list_head ao_waiting;
	struct list_head ao_pending;
};

AMODULE_API int
AThreadBegin(AThread *at, AThread *pool);

AMODULE_API int
AThreadEnd(AThread *at);

AMODULE_API int
AThreadAbort(AThread *at);

AMODULE_API int
AThreadBind(AThread *at, HANDLE file);

AMODULE_API AThread*
AThreadDefault(int ix);


//////////////////////////////////////////////////////////////////////////
typedef struct AOperator AOperator;
#pragma warning(disable: 4201)
struct AOperator {
	void   *userdata;
	void  (*callback)(AOperator *asop, int result);

	union {
	struct {
	DWORD            ao_tick;
	struct list_head ao_entry;
	};
	OVERLAPPED       ao_ovlp;
	};
};
#pragma warning(default: 4201)

AMODULE_API int
AOperatorPost(AOperator *asop, AThread *at, DWORD tick);

AMODULE_API int
AOperatorSignal(AOperator *asop, AThread *at);

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
