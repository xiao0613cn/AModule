#ifndef _AOPERATOR_H_
#define _AOPERATOR_H_

typedef struct AThread AThread;
typedef struct AOperator AOperator;

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AThreadBegin(AThread **at, AThread *pool, int max_timewait);

AMODULE_API int
AThreadEnd(AThread *at);

AMODULE_API int
AThreadPost(AThread *at, AOperator *asop);

AMODULE_API int
AThreadAbort(AThread *at);

#ifdef _WIN32
AMODULE_API int
AThreadBind(AThread *at, HANDLE file);
#else
AMODULE_API int
AThreadBind(AThread *at, AOperator *asop, uint32_t event);

static inline int
AThreadUnbind(AOperator *asop) {
	return AThreadBind(NULL, asop, 0);
}
#endif

AMODULE_API AThread*
AThreadDefault(int ix);


//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AOperatorPost(AOperator *asop, AThread *at, DWORD tick, BOOL wakeup = TRUE);

AMODULE_API int
AOperatorSignal(AOperator *asop, AThread *at, BOOL wakeup_or_cancel);

struct AOperator {
	int  (*done)(AOperator *asop, int result);

	AThread         *ao_thread;
	union {
	struct {         // post timer or runnable
	DWORD            ao_tick;
	struct rb_node   ao_tree;
	struct list_head ao_list;
	};
#ifdef _WIN32            // win32 IOCP
	OVERLAPPED       ao_ovlp;
#else
	struct {         // linux epoll
	int              ao_fd;
	uint32_t         ao_events;
	};
#endif
	void            *ao_user[4];
	};
#ifdef __cplusplus
	void timer() {
		memzero(*this);
		RB_CLEAR_NODE(&ao_tree);
		INIT_LIST_HEAD(&ao_list);
	}
	int delay(AThread *at, DWORD timeout, BOOL wakeup = TRUE) {
		if ((timeout != 0) && (timeout != INFINITE)) {
			timeout += GetTickCount();
			if ((timeout == 0) || (timeout == INFINITE))
				timeout += 2;
		}
		return AOperatorPost(this, at, timeout, wakeup);
	}
	int signal(AThread *at, BOOL wakeup_or_cancel) {
		return AOperatorSignal(this, at, wakeup_or_cancel);
	}
	int post(AThread *at) {
		return AThreadPost(at, this);
	}
	int done2(int result) {
		return done(this, result);
	}
#endif
};

template <typename AType, size_t offset, int(AType::*run)(int)>
int AsopDoneT(AOperator *asop, int result) {
	AType *p = (AType*)((char*)asop - offset);
	return (p->*run)(result);
}
#define AsopDone(type, member, run) \
	AsopDoneT<type, offsetof(type,member), &type::run>



#endif
