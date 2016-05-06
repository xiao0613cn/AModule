#include "stdafx.h"
#include <process.h>
#include "AModule.h"

enum iocp_key {
	iocp_key_unknown = 0,
	iocp_key_signal,
	iocp_key_sysio,
};

static unsigned int __stdcall AThread_run(void *p)
{
	AThread *at = (AThread*)p;
	AOperator *asop;
	DWORD tx;
	ULONG_PTR key;
	OVERLAPPED *ovlp;

	struct list_head ao_timeout;
	INIT_LIST_HEAD(&ao_timeout);

	if (at->attach != NULL)
		at = at->attach;

	while (at->running) {
		EnterCriticalSection(&at->ao_lock);
		DWORD curtick = GetTickCount();
		long timewait = 20*1000;

		while (!list_empty(&at->ao_waiting))
		{
			asop = list_first_entry(&at->ao_waiting, AOperator, ao_entry);
			long diff = (long)(curtick - asop->ao_tick);
			if (diff < 0) {
				timewait = -diff;
				break;
			}
			asop->ao_tick = 0;
			list_move_tail(&asop->ao_entry, &ao_timeout);
		}
		LeaveCriticalSection(&at->ao_lock);

		while (!list_empty(&ao_timeout)) {
			asop = list_first_entry(&ao_timeout, AOperator, ao_entry);
			list_del_init(&asop->ao_entry);
			asop->callback(asop, 0);
		}

		for ( ; at->running; ) {
			if (timewait != 0) {
				DWORD newtick = GetTickCount();
				timewait -= (newtick - curtick);
				if (timewait < 0)
					timewait = 0;
				curtick = newtick;
			}

			GetQueuedCompletionStatus(at->iocp, &tx, &key, &ovlp, timewait);
			if (ovlp == NULL)
				break;

			asop = container_of(ovlp, AOperator, ao_ovlp);
			asop->callback(asop, tx);

			switch (key)
			{
			case iocp_key_signal: break;
			case iocp_key_sysio: break;
			default: TRACE("unknown iocp key = %d, tx = %d.\n", key, tx); break;
			}
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
static AThread work_thread[4];
static int work_thread_begin(AThread *pool)
{
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadBegin(&work_thread[ix], pool);
		if (pool == NULL)
			pool = &work_thread[0];
	}
	return 0;
}
static int work_thread_end(void)
{
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadAbort(&work_thread[ix]);
	}
	for (int ix = _countof(work_thread)-1; ix >= 0; --ix) {
		AThreadEnd(&work_thread[ix]);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
int AThreadBegin(AThread *at, AThread *pool)
{
	if (at == NULL)
		return work_thread_begin(pool);

	if (at->thread != NULL)
		return -1;

	at->attach = pool;
	if (pool != NULL) {
		at->iocp = pool->iocp;
		memset(&at->ao_lock, 0, sizeof(at->ao_lock));
	} else {
		at->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		InitializeCriticalSection(&at->ao_lock);
	}
	INIT_LIST_HEAD(&at->ao_waiting);
	INIT_LIST_HEAD(&at->ao_pending);

	at->running = TRUE;
	at->thread = (HANDLE)_beginthreadex(NULL, 0, &AThread_run, at, 0, NULL);
	return 0;
}

int AThreadEnd(AThread *at)
{
	if (at == NULL)
		return work_thread_end();

	at->running = FALSE;
	if (at->thread == NULL)
		return 0;

	PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
	WaitForSingleObjectEx(at->thread, INFINITE, FALSE);
	CloseHandle(at->thread); at->thread = NULL;

	if (at->attach) {
		at->attach = NULL;
		at->iocp = NULL;
		return 0;
	}
	CloseHandle(at->iocp);
	at->iocp = NULL;

	DeleteCriticalSection(&at->ao_lock);
	AOperator *asop;
	while (!list_empty(&at->ao_waiting)) {
		asop = list_first_entry(&at->ao_waiting, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, -1);
	}
	while (!list_empty(&at->ao_pending)) {
		asop = list_first_entry(&at->ao_pending, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, -1);
	}
	return 0;
}

int AThreadAbort(AThread *at)
{
	at->running = FALSE;
	PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
	return 0;
}

int AThreadBind(AThread *at, HANDLE file)
{
	if (at == NULL)
		at = work_thread;

	HANDLE iocp = CreateIoCompletionPort(file, at->iocp, iocp_key_sysio, 0);
	return (iocp == at->iocp) ? 1 : -GetLastError();
}

AThread* AThreadDefault(int ix)
{
	if (ix >= _countof(work_thread))
		return NULL;
	return &work_thread[ix];
}

//////////////////////////////////////////////////////////////////////////
int AOperatorPost(AOperator *asop, AThread *at, DWORD tick)
{
	if (at == NULL)
		at = work_thread;
	if (at->attach != NULL)
		at = at->attach;

	asop->ao_tick = tick;
	if (tick == 0) {
		PostQueuedCompletionStatus(at->iocp, TRUE, iocp_key_signal, &asop->ao_ovlp);
		return 0;
	}

	if (tick == INFINITE) {
		EnterCriticalSection(&at->ao_lock);
		list_add_tail(&asop->ao_entry, &at->ao_pending);
		LeaveCriticalSection(&at->ao_lock);
		return 0;
	}

	AOperator *pos;
	INIT_LIST_HEAD(&asop->ao_entry);

	EnterCriticalSection(&at->ao_lock);
	list_for_each_entry(pos, &at->ao_waiting, AOperator, ao_entry) {
		if (long(pos->ao_tick - tick) >= 0) {
			list_add_tail(&asop->ao_entry, &pos->ao_entry); //list_insert_front
			break;
		}
	}

	if (list_empty(&asop->ao_entry)) {
		if (list_empty(&at->ao_waiting))
			tick = 0;
		list_add_tail(&asop->ao_entry, &at->ao_waiting);
	}
	else if (list_is_last(&at->ao_waiting, &asop->ao_entry)) {
		tick = 0;
	}
	LeaveCriticalSection(&at->ao_lock);

	if (tick == 0) {
		PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
	}
	return 0;
}

int AOperatorSignal(AOperator *asop, AThread *at)
{
	if (at == NULL)
		at = work_thread;
	if (at->attach != NULL)
		at = at->attach;

	EnterCriticalSection(&at->ao_lock);
	BOOL signal = (asop->ao_tick != 0);
	if (signal) {
		asop->ao_tick = 0;
		list_del_init(&asop->ao_entry);
	}
	LeaveCriticalSection(&at->ao_lock);

	if (signal) {
		PostQueuedCompletionStatus(at->iocp, signal, iocp_key_signal, &asop->ao_ovlp);
	}
	return signal;
}


//////////////////////////////////////////////////////////////////////////
void async_test_callback(AOperator *asop, int result)
{
	TRACE("asop->timeout = %d, result = %d.\n", asop->userdata, result);
}

int async_test(void)
{
	AThread at;
	memset(&at, 0, sizeof(at));
	AThreadBegin(&at, NULL);

	AOperator asop[20];
	for (int ix = 0; ix < 20; ++ix) {
		asop[ix].userdata = (void*)(ix*1000);
		asop[ix].callback = async_test_callback;
		AOperatorTimewait(&asop[ix], &at, ix*1000);
	}

	Sleep(2*1000);
	AOperatorSignal(&asop[5], &at);
	Sleep(15*1000);

	AThreadEnd(&at);
	getchar();
	return 0;
}
