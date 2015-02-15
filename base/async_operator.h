#ifndef _ASYNC_OPERATOR_H_
#define _ASYNC_OPERATOR_H_

#ifndef _LIST_HEAD_H_
#include "list.h"
#endif

typedef struct async_thread async_thread;
typedef struct async_operator async_operator;
typedef struct sysio_operator sysio_operator;


//////////////////////////////////////////////////////////////////////////
struct async_thread {
	BOOL volatile    running;
	HANDLE           thread;
	HANDLE           iocp;
	BOOL             attach;

	CRITICAL_SECTION ao_lock;
	struct list_head ao_waiting;
	struct list_head ao_pending;
};

int async_thread_begin(async_thread *at, HANDLE iocp);
int async_thread_end(async_thread *at);
int async_thread_abort(async_thread *at);


//////////////////////////////////////////////////////////////////////////
struct async_operator {
	void   *userdata;
	void  (*callback)(async_operator *asop, int result);

	// private
	DWORD            ao_tick;
	struct list_head ao_entry;
};

int async_operator_post(async_operator *asop, async_thread *at, DWORD tick);
int async_operator_signal(async_operator *asop, async_thread *at);


//////////////////////////////////////////////////////////////////////////
struct sysio_operator {
	// private
	OVERLAPPED ovlp;

	void   *userdata;
	void  (*callback)(sysio_operator *sysop, int result);
};

int sysio_bind(async_thread *at, HANDLE file);
int sysio_operator_connect(sysio_operator *sysop, SOCKET sd, const char *netaddr, const char *port);
int sysio_is_connected(SOCKET sd);
int sysio_operator_send(sysio_operator *sysop, SOCKET sd, const char *data, int size);
int sysio_operator_recv(sysio_operator *sysop, SOCKET sd, char *data, int size);


//////////////////////////////////////////////////////////////////////////
static inline int async_operator_run(async_operator *asop, async_thread *at)
{
	return async_operator_post(asop, at, 0);
}

static inline int async_operator_timewait(async_operator *asop, async_thread *at, DWORD timeout)
{
	if ((timeout != 0) && (timeout != INFINITE)) {
		timeout += GetTickCount();
		if ((timeout == 0) || (timeout == INFINITE))
			timeout += 2;
	}
	return async_operator_post(asop, at, timeout);
}

static inline int async_opeartor_pending(async_operator *asop, async_thread *at)
{
	return async_operator_post(asop, at, INFINITE);
}


#endif
