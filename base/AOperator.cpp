#include "stdafx.h"
#include <map>
#include "AModule_API.h"

enum iocp_key {
	iocp_key_unknown = 0,
	iocp_key_signal,
	iocp_key_sysio,
};

struct AOperatorTimewaitCompare {
bool operator()(const DWORD left, const DWORD right) const
{
	return (int(left-right) < 0);
}
};
typedef std::map<DWORD, AOperator*, AOperatorTimewaitCompare> TimewaitMap;

struct AThread {
	int volatile     running;
	pthread_t        thread;
	AThread         *attach;
#ifdef _WIN32
	HANDLE           iocp;
#else
	int              epoll;
	int              signal[2];
	struct epoll_event events[64];
	char             sigbuf[128];
	long volatile    bind_count;
	struct list_head working_list;
#endif
	pthread_mutex_t  mutex;
	TimewaitMap      waiting_list;
	struct list_head pending_list;
};

static int AThreadCheckTimewait(AThread *at)
{
	int max_timewait = 20*1000;
	AOperator *asop;

	struct list_head timeout_list;
	INIT_LIST_HEAD(&timeout_list);
#ifndef _WIN32
	struct list_head working_list;
	INIT_LIST_HEAD(&working_list);
#endif
	pthread_mutex_lock(&at->mutex);
	DWORD curtick = GetTickCount();

	while (!at->waiting_list.empty()) {
		TimewaitMap::iterator it = at->waiting_list.begin();

		int diff = int(curtick - it->first);
		if (diff < 1) {
			max_timewait = -diff;
			break;
		}

		list_for_each_entry(asop, &it->second->ao_entry, AOperator, ao_entry) {
			asop->ao_tick = 0;
		}
		it->second->ao_tick = 0;

		struct list_head *last = it->second->ao_entry.prev;
		it->second->ao_entry.prev = timeout_list.prev;
		timeout_list.prev->next = &it->second->ao_entry;

		last->next = &timeout_list;
		timeout_list.prev = last;
		at->waiting_list.erase(it);
	}
#ifndef _WIN32
	list_splice_init(&at->working_list, &working_list);
#endif
	pthread_mutex_unlock(&at->mutex);

	while (!list_empty(&timeout_list)) {
		asop = list_first_entry(&timeout_list, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, 0);
	}
#ifndef _WIN32
	while (!list_empty(&working_list)) {
		asop = list_first_entry(&working_list, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, 1);
	}
#endif
	return max_timewait;
}

static void* AThreadRun(void *p)
{
	AThread *at = (AThread*)p;
	if (at->attach != NULL)
		at = at->attach;

	int max_timewait = 0;
	DWORD cur_timetick = GetTickCount();

	while (at->running)
	{
		if (max_timewait <= 0)
			max_timewait = AThreadCheckTimewait(at);

		DWORD new_timetick = GetTickCount();
		max_timewait -= (new_timetick - cur_timetick);
		cur_timetick = new_timetick;

		if (max_timewait < 0)
			max_timewait = 0;
#ifdef _WIN32
		DWORD tx = 0;
		ULONG_PTR key = iocp_key_unknown;
		OVERLAPPED *ovlp = NULL;
		GetQueuedCompletionStatus(at->iocp, &tx, &key, &ovlp, max_timewait);
		if (ovlp == NULL) {
			if (key == iocp_key_signal)
				max_timewait = 0;
		} else {
			AOperator *asop = container_of(ovlp, AOperator, ao_ovlp);
			asop->callback(asop, tx);
		}
#else
		char *sigbuf = ((AThread*)p)->sigbuf;
		struct epoll_event *events = ((AThread*)p)->events;

		int count = epoll_wait(at->epoll, events, _countof(at->events), max_timewait);
		for (int ix = 0; ix < count; ++ix)
		{
			AOperator *asop = (AOperator*)events[ix].data.ptr;
			if (asop != NULL) {
				asop->callback(asop, events[ix].events);
			} else {
				while (recv(at->signal[1], sigbuf, sizeof(at->sigbuf), 0) > 0)
					;
				max_timewait = 0;
			}
		}
#endif
	}
	return 0;
}

static inline void AThreadWakeup(AThread *at, AOperator *asop)
{
#ifdef _WIN32
	PostQueuedCompletionStatus(at->iocp, !!asop, iocp_key_signal, (asop ? &asop->ao_ovlp : NULL));
#else
	if (asop != NULL) {
		pthread_mutex_lock(&at->mutex);
		int first = list_empty(&at->working_list);
		list_add_tail(&asop->ao_entry, &at->working_list);
		pthread_mutex_unlock(&at->mutex);
		if (!first)
			return;
	}
	send(at->signal[0], "a", 1, 0);
#endif
}

//////////////////////////////////////////////////////////////////////////
static AThread *work_thread[4];
static int work_thread_begin(void)
{
#ifdef _WIN32
	AThread *pool = work_thread[0];
#else
	AThread *poll = NULL;
#endif
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadBegin(&work_thread[ix], pool);
	}
	return 1;
}
static int work_thread_end(void)
{
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadAbort(work_thread[ix]);
	}
	for (int ix = _countof(work_thread)-1; ix >= 0; --ix) {
		AThreadEnd(work_thread[ix]);
		work_thread[ix] = NULL;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AThreadBegin(AThread **p, AThread *pool)
{
	if (p == NULL)
		return work_thread_begin();

	AThread *at = new AThread();
	*p = at;
	at->attach = pool;

#ifdef _WIN32
	if (pool != NULL) {
		at->iocp = pool->iocp;
	} else {
		at->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	}
#else
	if (pool != NULL) {
		at->epoll = pool->epoll;
		at->signal[0] = pool->signal[0];
		at->signal[1] = pool->signal[1];
	} else {
		at->epoll = epoll_create(32000);
		socketpair(AF_UNIX, SOCK_STREAM, 0, at->signal);
		at->bind_count = 0;
		INIT_LIST_HEAD(&at->working_list);

		tcp_nonblock(at->signal[0], 1);
		tcp_nonblock(at->signal[1], 1);

		struct epoll_event epev;
		epev.events = EPOLLIN|EPOLLET|EPOLLHUP|EPOLLERR;
		epev.data.ptr = NULL;
		epoll_ctl(at->epoll, EPOLL_CTL_ADD, at->signal[1], &epev);
	}
#endif
	if (pool == NULL) {
		pthread_mutex_init(&at->mutex, NULL);
		INIT_LIST_HEAD(&at->pending_list);
	}

	at->running = 1;
	pthread_create(&at->thread, NULL, &AThreadRun, at);
	return 1;
}

AMODULE_API int
AThreadEnd(AThread *at)
{
	if (at == NULL)
		return work_thread_end();

	AThreadAbort(at);
	pthread_join(at->thread, NULL);

	if (at->attach != NULL) {
		delete at;
		return 1;
	}
#ifdef _WIN32
	CloseHandle(at->iocp);
#else
	close(at->signal[0]);
	close(at->signal[1]);
	close(at->epoll);
#endif
	pthread_mutex_destroy(&at->mutex);

	AOperator *asop;
	while (!at->waiting_list.empty()) {
		TimewaitMap::iterator it = at->waiting_list.begin();

		while (!list_empty(&it->second->ao_entry)) {
			asop = list_first_entry(&it->second->ao_entry, AOperator, ao_entry);
			list_del_init(&asop->ao_entry);
			asop->callback(asop, -EINTR);
		}

		it->second->callback(it->second, -EINTR);
		at->waiting_list.erase(it);
	}
	while (!list_empty(&at->pending_list)) {
		asop = list_first_entry(&at->pending_list, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, -EINTR);
	}
	delete at;
	return 1;
}

int AThreadAbort(AThread *at)
{
	if (at == NULL)
		at = work_thread[0];
	at->running = 0;
	if (at->attach != NULL)
		at = at->attach;
	AThreadWakeup(at, NULL);
	return 1;
}
#ifdef _WIN32
AMODULE_API int
AThreadBind(AThread *at, HANDLE file)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	HANDLE iocp = CreateIoCompletionPort(file, at->iocp, iocp_key_sysio, 0);
	return (iocp == at->iocp) ? 1 : -int(GetLastError());
}
#else
AMODULE_API int
AThreadBind(AThread *at, AOperator *asop, uint32_t event)
{
	int op;
	if (event == 0) {
		if (asop->ao_events == 0)
			return -1;
		op = EPOLL_CTL_DEL;
		at = asop->ao_thread;
	} else if (asop->ao_events != 0) {
		op = EPOLL_CTL_MOD;
		at = asop->ao_thread;
	} else {
		op = EPOLL_CTL_ADD;
		if (at == NULL) {
			at = work_thread[0];
			for (int ix = 1; ix < _countof(work_thread); ++ix) {
				if (work_thread[ix]->bind_count < at->bind_count)
					at = work_thread[ix];
			}
		}
		asop->ao_thread = at;
	}
	asop->ao_events = event;

	struct epoll_event epev;
	epev.events = asop->ao_events;
	epev.data.ptr = asop;

	int result = epoll_ctl(at->epoll, op, asop->ao_fd, &epev);
	if (result == 0) {
		if (op == EPOLL_CTL_ADD) {
			InterlockedAdd(&at->bind_count, 1);
		} else if (op == EPOLL_CTL_DEL) {
			InterlockedAdd(&at->bind_count, -1);
		}
	}
	return result;
}
#endif

AMODULE_API AThread*
AThreadDefault(int ix)
{
	if (ix >= _countof(work_thread))
		return NULL;
	return work_thread[ix];
}

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AOperatorPost(AOperator *asop, AThread *at, DWORD tick)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	asop->ao_tick = tick;
	if (tick == 0) {
		AThreadWakeup(at, asop);
		return 0;
	}

	if (tick == INFINITE) {
		pthread_mutex_lock(&at->mutex);
		list_add_tail(&asop->ao_entry, &at->pending_list);
		pthread_mutex_unlock(&at->mutex);
		return 0;
	}

	pthread_mutex_lock(&at->mutex);
	TimewaitMap::iterator it = at->waiting_list.find(asop->ao_tick);
	if (at->waiting_list.empty()
	 || ((it == at->waiting_list.end()) && int(asop->ao_tick-at->waiting_list.begin()->first) < 0))
		tick = 0;

	if ((it == at->waiting_list.end()) || (it->first != asop->ao_tick)) {
		at->waiting_list.insert(std::make_pair(asop->ao_tick, asop));
		INIT_LIST_HEAD(&asop->ao_entry);
	} else {
		list_add_tail(&asop->ao_entry, &it->second->ao_entry);
		//TRACE("combine list timewait = %d.\n", tick);
	}
	pthread_mutex_unlock(&at->mutex);

	if (tick == 0) {
		AThreadWakeup(at, NULL);
	}
	return 0;
}

AMODULE_API int
AOperatorSignal(AOperator *asop, AThread *at, int cancel)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	pthread_mutex_lock(&at->mutex);
	int signal = (asop->ao_tick != 0);
	if (asop->ao_tick == INFINITE) {
		asop->ao_tick = 0;
		list_del_init(&asop->ao_entry);
	}
	else if (asop->ao_tick != 0) {
		TimewaitMap::iterator it = at->waiting_list.find(asop->ao_tick);
		if (it == at->waiting_list.end()) {
			assert(0);
			signal = 0;
		} else if (asop != it->second) {
			assert(!list_empty(&asop->ao_entry));
			list_del_init(&asop->ao_entry);
		} else if (list_empty(&asop->ao_entry)) {
			at->waiting_list.erase(it);
		} else {
			it->second = list_first_entry(&asop->ao_entry, AOperator, ao_entry);
			list_del_init(&asop->ao_entry);
		}
		asop->ao_tick = 0;
	}
	pthread_mutex_unlock(&at->mutex);

	if (signal && !cancel) {
		AThreadWakeup(at, asop);
	}
	return signal;
}

