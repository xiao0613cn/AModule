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
#ifdef _WIN32
	HANDLE           iocp;
#else
	int              epoll;
	int              signal[2];
	struct epoll_event events[32];
	char             ao_sigbuf[128];
	struct list_head ao_working;
#endif
	AThread         *attach;

	pthread_mutex_t  ao_mutex;
	TimewaitMap      ao_waiting;
	struct list_head ao_pending;
};

static int AThreadCheckTimewait(AThread *at)
{
	int max_timewait = 20*1000;
	AOperator *asop;

	struct list_head ao_timeout;
	INIT_LIST_HEAD(&ao_timeout);
#ifndef _WIN32
	struct list_head ao_working;
	INIT_LIST_HEAD(&ao_working);
#endif
	pthread_mutex_lock(&at->ao_mutex);
	DWORD curtick = GetTickCount();

	while (!at->ao_waiting.empty()) {
		TimewaitMap::iterator it = at->ao_waiting.begin();

		int diff = int(curtick - it->first);
		if (diff < 0) {
			max_timewait = -diff;
			break;
		}

		list_for_each_entry(asop, &it->second->ao_entry, AOperator, ao_entry) {
			asop->ao_tick = 0;
		}
		it->second->ao_tick = 0;

		list_splice_tail(&it->second->ao_entry, &ao_timeout);
		list_add_tail(&it->second->ao_entry, &ao_timeout);
		at->ao_waiting.erase(it);
	}
#ifndef _WIN32
	list_splice_init(&at->ao_working, &ao_working);
#endif
	pthread_mutex_unlock(&at->ao_mutex);

	while (!list_empty(&ao_timeout)) {
		asop = list_first_entry(&ao_timeout, AOperator, ao_entry);
		list_del_init(&asop->ao_entry);
		asop->callback(asop, 0);
	}
#ifndef _WIN32
	while (!list_empty(&ao_working)) {
		asop = list_first_entry(&ao_working, AOperator, ao_entry);
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
		char *ao_sigbuf = ((AThread*)p)->ao_sigbuf;
		struct epoll_event *events = ((AThread*)p)->events;

		int count = epoll_wait(at->epoll, events, _countof(at->events), max_timewait);
		for (int ix = 0; ix < count; ++ix)
		{
			AOperator *asop = (AOperator*)events[ix].data.ptr;
			if (asop == NULL) {
				while (recv(at->signal[1], ao_sigbuf, sizeof(at->ao_sigbuf), 0) > 0)
					;
				max_timewait = 0;
				continue;
			}

			if (asop->callback != NULL) {
				asop->callback(asop, events[ix].events);
			} else {
				if ((events[ix].events & (EPOLLHUP|EPOLLERR)) && (asop->ao_error != NULL))
					asop->ao_error(asop, -EIO);

				if (events[ix].events & EPOLLIN)
					asop->ao_recv(asop, 0);

				if (events[ix].events & EPOLLOUT)
					asop->ao_send(asop, 0);
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
		pthread_mutex_lock(&at->ao_mutex);
		int first = list_empty(&at->ao_working);
		list_add_tail(&asop->ao_entry, &at->ao_working);
		pthread_mutex_unlock(&at->ao_mutex);
		if (!first)
			return;
	}
	send(at->signal[0], "a", 1, 0);
#endif
}

//////////////////////////////////////////////////////////////////////////
static AThread *work_thread[4];
static int work_thread_begin(AThread *pool)
{
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadBegin(&work_thread[ix], pool);
		if (pool == NULL)
			pool = work_thread[0];
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
int AThreadBegin(AThread **p, AThread *pool)
{
	if (p == NULL)
		return work_thread_begin(pool);

	AThread *at = new AThread();
	*p = at;

	at->attach = pool;
	if (pool != NULL) {
#ifdef _WIN32
		at->iocp = pool->iocp;
#else
		at->epoll = pool->epoll;
		at->signal[0] = pool->signal[0];
		at->signal[1] = pool->signal[1];
#endif
		memset(&at->ao_mutex, 0, sizeof(at->ao_mutex));
	} else {
#ifdef _WIN32
		at->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
#else
		at->epoll = epoll_create(32000);
		socketpair(AF_UNIX, SOCK_STREAM, 0, at->signal);
		INIT_LIST_HEAD(&at->ao_working);

		tcp_nonblock(at->signal[0], 1);
		tcp_nonblock(at->signal[1], 1);

		struct epoll_event epev;
		epev.events = EPOLLIN|EPOLLET|EPOLLHUP|EPOLLERR;
		epev.data.ptr = NULL;
		epoll_ctl(at->epoll, EPOLL_CTL_ADD, at->signal[1], &epev);
#endif
		pthread_mutex_init(&at->ao_mutex, NULL);
	}
	INIT_LIST_HEAD(&at->ao_pending);

	at->running = 1;
	pthread_create(&at->thread, NULL, &AThreadRun, at);
	return 1;
}

int AThreadEnd(AThread *at)
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
	pthread_mutex_destroy(&at->ao_mutex);

	AOperator *asop;
	while (!at->ao_waiting.empty()) {
		TimewaitMap::iterator it = at->ao_waiting.begin();

		while (!list_empty(&it->second->ao_entry)) {
			asop = list_first_entry(&it->second->ao_entry, AOperator, ao_entry);
			list_del_init(&asop->ao_entry);
			asop->callback(asop, -EINTR);
		}

		it->second->callback(it->second, -EINTR);
		at->ao_waiting.erase(it);
	}
	while (!list_empty(&at->ao_pending)) {
		asop = list_first_entry(&at->ao_pending, AOperator, ao_entry);
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
int AThreadBind(AThread *at, HANDLE file)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	HANDLE iocp = CreateIoCompletionPort(file, at->iocp, iocp_key_sysio, 0);
	return (iocp == at->iocp) ? 1 : -int(GetLastError());
}
#else
int AThreadBind(AThread *at, AOperator *asop, uint32_t event)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	int op;
	pthread_mutex_lock(&at->ao_mutex);
	if (event == 0)
		op = (asop->ao_events ? EPOLL_CTL_DEL : -1);
	else if (asop->ao_events != 0)
		op = EPOLL_CTL_MOD;
	else
		op = EPOLL_CTL_ADD;

	int result = -1;
	if (op != -1) {
		asop->ao_events |= event;

		struct epoll_event epev;
		epev.events = asop->ao_events;
		epev.data.ptr = asop;
		result = epoll_ctl(at->epoll, op, asop->ao_fd, &epev);
	}

	pthread_mutex_unlock(&at->ao_mutex);
	return result;
}
#endif

AThread* AThreadDefault(int ix)
{
	if (ix >= _countof(work_thread))
		return NULL;
	return work_thread[ix];
}

//////////////////////////////////////////////////////////////////////////
int AOperatorPost(AOperator *asop, AThread *at, DWORD tick)
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
		pthread_mutex_lock(&at->ao_mutex);
		list_add_tail(&asop->ao_entry, &at->ao_pending);
		pthread_mutex_unlock(&at->ao_mutex);
		return 0;
	}

	pthread_mutex_lock(&at->ao_mutex);
	TimewaitMap::iterator it = at->ao_waiting.find(asop->ao_tick);
	if (at->ao_waiting.empty()
	 || ((it == at->ao_waiting.end()) && int(asop->ao_tick-at->ao_waiting.begin()->first) < 0))
		tick = 0;

	if ((it == at->ao_waiting.end()) || (it->first != asop->ao_tick)) {
		at->ao_waiting.insert(std::make_pair(asop->ao_tick, asop));
		INIT_LIST_HEAD(&asop->ao_entry);
	} else {
		list_add_tail(&asop->ao_entry, &it->second->ao_entry);
		//TRACE("combine list timewait = %d.\n", tick);
	}
	pthread_mutex_unlock(&at->ao_mutex);

	if (tick == 0) {
		AThreadWakeup(at, NULL);
	}
	return 0;
}

int AOperatorSignal(AOperator *asop, AThread *at)
{
	if (at == NULL)
		at = work_thread[0];
	if (at->attach != NULL)
		at = at->attach;

	pthread_mutex_lock(&at->ao_mutex);
	int signal = (asop->ao_tick != 0);
	if (asop->ao_tick == INFINITE) {
		asop->ao_tick = 0;
		list_del_init(&asop->ao_entry);
	}
	else if (asop->ao_tick != 0) {
		TimewaitMap::iterator it = at->ao_waiting.find(asop->ao_tick);
		if (it == at->ao_waiting.end()) {
			assert(0);
			signal = 0;
		} else if (asop != it->second) {
			assert(!list_empty(&asop->ao_entry));
			list_del_init(&asop->ao_entry);
		} else if (list_empty(&asop->ao_entry)) {
			at->ao_waiting.erase(it);
		} else {
			it->second = list_first_entry(&asop->ao_entry, AOperator, ao_entry);
			list_del_init(&asop->ao_entry);
		}
		asop->ao_tick = 0;
	}
	pthread_mutex_unlock(&at->ao_mutex);

	if (signal) {
		AThreadWakeup(at, asop);
	}
	return signal;
}

