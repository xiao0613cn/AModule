#include "stdafx.h"
#include <process.h>
#include "async_operator.h"
#include "../io/iocp_util.h"

enum iocp_key {
	iocp_key_signal = 0,
	iocp_key_sysio,
};

unsigned int __stdcall async_thread_run(void *p)
{
	async_thread *at = (async_thread*)p;
	async_operator *asop;
	sysio_operator *sysop;
	DWORD tx;
	ULONG_PTR key;
	OVERLAPPED *ovlp;

	struct list_head ao_timeout;
	INIT_LIST_HEAD(&ao_timeout);

	while (at->running) {
		DWORD curtick = GetTickCount();
		long timewait = 20*1000;

		EnterCriticalSection(&at->ao_lock);
		while (!list_empty(&at->ao_waiting))
		{
			asop = list_first_entry(&at->ao_waiting, async_operator, ao_entry);
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
			asop = list_first_entry(&ao_timeout, async_operator, ao_entry);
			list_del_init(&asop->ao_entry);
			asop->callback(asop, 0);
		}

		for ( ; ; ) {
			timewait -= (GetTickCount() - curtick);
			if (timewait < 0) {
				timewait = 0;
			}

			GetQueuedCompletionStatus(at->iocp, &tx, &key, &ovlp, timewait);
			if (ovlp == NULL)
				break;

			switch (key)
			{
			case iocp_key_signal:
				asop = (async_operator*)ovlp;
				asop->callback(asop, tx);
				break;
			case iocp_key_sysio:
				sysop = container_of(ovlp, sysio_operator, ovlp);
				sysop->callback(sysop, tx);
				break;
			default:
				TRACE("unknown iocp key = %d, tx = %d.\n", key, tx);
				break;
			}
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
int async_thread_begin(async_thread *at, HANDLE iocp)
{
	if (at->thread != NULL)
		return -1;

	InitializeCriticalSectionEx(&at->ao_lock, 1000, 0);
	INIT_LIST_HEAD(&at->ao_waiting);
	INIT_LIST_HEAD(&at->ao_pending);

	at->attach = (iocp != NULL);
	if (iocp == NULL)
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	at->iocp = iocp;

	at->running = TRUE;
	at->thread = (HANDLE)_beginthreadex(NULL, 0, &async_thread_run, at, 0, NULL);
	return 0;
}

int async_thread_end(async_thread *at)
{
	at->running = FALSE;
	if (at->thread == NULL)
		return 0;

	PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
	WaitForSingleObjectEx(at->thread, INFINITE, FALSE);
	CloseHandle(at->thread); at->thread = NULL;

	if (!at->attach)
		CloseHandle(at->iocp);
	at->iocp = NULL;

	DeleteCriticalSection(&at->ao_lock);
	async_operator *asop;
	while (!list_empty(&at->ao_waiting)) {
		asop = list_first_entry(&at->ao_waiting, async_operator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, -1);
	}
	while (!list_empty(&at->ao_pending)) {
		asop = list_first_entry(&at->ao_pending, async_operator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, -1);
	}
	return 0;
}

int async_thread_abort(async_thread *at)
{
	at->running = FALSE;
	PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
int async_operator_post(async_operator *asop, async_thread *at, DWORD tick)
{
	OVERLAPPED *ovlp = NULL;

	asop->ao_tick = tick;
	if (tick == 0) {
		ovlp = (OVERLAPPED*)asop;
	}
	else if (tick == INFINITE) {
		EnterCriticalSection(&at->ao_lock);
		list_add_tail(&asop->ao_entry, &at->ao_pending);
		LeaveCriticalSection(&at->ao_lock);
	}
	else {
		async_operator *pos;
		INIT_LIST_HEAD(&asop->ao_entry);

		EnterCriticalSection(&at->ao_lock);
		list_for_each_entry(pos, &at->ao_waiting, async_operator, ao_entry) {
			if (long(pos->ao_tick - tick) >= 0) {
				list_add_tail(&asop->ao_entry, &pos->ao_entry); //list_insert_front
				break;
			}
		}

		if (list_empty(&asop->ao_entry)) {
			list_add_tail(&asop->ao_entry, &at->ao_waiting);
		}
		else if (list_is_last(&at->ao_waiting, &asop->ao_entry)) {
			tick = 0;
		}
		LeaveCriticalSection(&at->ao_lock);
	}

	if (tick == 0) {
		PostQueuedCompletionStatus(at->iocp, (ovlp!=NULL), iocp_key_signal, ovlp);
	}
	return 0;
}

int async_operator_signal(async_operator *asop, async_thread *at)
{
	EnterCriticalSection(&at->ao_lock);
	int set = (asop->ao_tick != 0);
	if (set) {
		asop->ao_tick = 0;
		list_del_init(&asop->ao_entry);
	}
	LeaveCriticalSection(&at->ao_lock);

	if (set) {
		PostQueuedCompletionStatus(at->iocp, set, iocp_key_signal, (OVERLAPPED*)asop);
	}
	return set;
}


//////////////////////////////////////////////////////////////////////////
int sysio_bind(async_thread *at, HANDLE file)
{
	HANDLE iocp = CreateIoCompletionPort(file, at->iocp, iocp_key_sysio, 0);
	assert(iocp == at->iocp);
	return 0;
}

int sysio_operator_connect(sysio_operator *sysop, SOCKET sd, const char *netaddr, const char *port)
{
	struct addrinfo *ai = iocp_getaddrinfo(netaddr, port);
	if (ai == NULL)
		return -1;

	int ret = iocp_connect(sd, ai->ai_addr, ai->ai_addrlen, &sysop->ovlp);
	freeaddrinfo(ai);
	return ret;
}

int sysio_is_connected(SOCKET sd)
{
	return iocp_is_connected(sd);
}

int sysio_operator_send(sysio_operator *sysop, SOCKET sd, const char *data, int size)
{
	return iocp_send(sd, data, size, &sysop->ovlp);
}

int sysio_operator_recv(sysio_operator *sysop, SOCKET sd, char *data, int size)
{
	return iocp_recv(sd, data, size, &sysop->ovlp);
}


//////////////////////////////////////////////////////////////////////////
void async_test_callback(async_operator *asop, int result)
{
	TRACE("asop->timeout = %d, result = %d.\n", asop->userdata, result);
}

int async_test(void)
{
	async_thread at;
	memset(&at, 0, sizeof(at));
	async_thread_begin(&at, NULL);

	async_operator asop[20];
	for (int ix = 0; ix < 20; ++ix) {
		asop[ix].userdata = (void*)(ix*1000);
		asop[ix].callback = async_test_callback;
		async_operator_timewait(&asop[ix], &at, ix*1000);
	}

	Sleep(2*1000);
	async_operator_signal(&asop[5], &at);
	Sleep(15*1000);

	async_thread_end(&at);
	getchar();
	return 0;
}
