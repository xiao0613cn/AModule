#ifndef _AOPERATOR_H_
#define _AOPERATOR_H_


//////////////////////////////////////////////////////////////////////////
typedef struct AThread AThread;

AMODULE_API int
AThreadBegin(AThread **at, AThread *pool);

AMODULE_API int
AThreadEnd(AThread *at);

AMODULE_API int
AThreadAbort(AThread *at);

AMODULE_API int
#ifdef _WIN32
AThreadBind(AThread *at, HANDLE file);
#else
AThreadBind(AThread *at, AOperator *asop, uint32_t event);
#endif

AMODULE_API AThread*
AThreadDefault(int ix);


//////////////////////////////////////////////////////////////////////////
#pragma warning(disable: 4201)
typedef struct AOperator AOperator;
struct AOperator {
	void  (*callback)(AOperator *asop, int result);

	union {
	struct {
	void            *ao_user;
	DWORD            ao_tick;
	struct list_head ao_entry;
	};
#ifdef _WIN32
	OVERLAPPED       ao_ovlp;
#else
	struct {
	int              ao_fd;
	uint32_t         ao_events;
	void           (*ao_recv)(AOperator *asop, int result);
	void           (*ao_send)(AOperator *asop, int result);
	void           (*ao_error)(AOperator *asop, int result);
	};
#endif
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
