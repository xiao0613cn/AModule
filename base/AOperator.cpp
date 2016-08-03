#include "stdafx.h"
#include <map>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include "AModule_API.h"

enum iocp_key {
	iocp_key_unknown = 0,
	iocp_key_signal,
	iocp_key_sysio,
};

static inline int AOperatorTimewaitCompare(DWORD tick, AOperator *asop)
{
	return int(tick - asop->ao_tick);
}
rb_tree_define(AOperator, ao_tree, DWORD, AOperatorTimewaitCompare)

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
	struct rb_root   waiting_tree;
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

	while (!RB_EMPTY_ROOT(&at->waiting_tree)) {
		asop = rb_entry(rb_first(&at->waiting_tree), AOperator, ao_tree);

		int diff = AOperatorTimewaitCompare(curtick, asop);
		if (diff < -1) {
			max_timewait = -diff;
			break;
		}

		AOperator *pos;
		list_for_each_entry(pos, &asop->ao_list, AOperator, ao_list) {
			pos->ao_tick = 0;
		}
		asop->ao_tick = 0;

		struct list_head *last = asop->ao_list.prev;
		asop->ao_list.prev = timeout_list.prev;
		timeout_list.prev->next = &asop->ao_list;

		last->next = &timeout_list;
		timeout_list.prev = last;
		rb_erase(&asop->ao_tree, &at->waiting_tree);
	}
#ifndef _WIN32
	list_splice_init(&at->working_list, &working_list);
#endif
	pthread_mutex_unlock(&at->mutex);

	while (!list_empty(&timeout_list)) {
		asop = list_first_entry(&timeout_list, AOperator, ao_list);
		list_del_init(&asop->ao_list);
		asop->callback(asop, 0);
	}
#ifndef _WIN32
	while (!list_empty(&working_list)) {
		asop = list_first_entry(&working_list, AOperator, ao_list);
		list_del_init(&asop->ao_list);
		asop->callback(asop, 1);
	}
#endif
	return max_timewait;
}

static void* AThreadRun(void *p)
{
	AThread *at = (AThread*)p;
	AThread *pool = (at->attach ? at->attach : at);

	int max_timewait = 0;
	DWORD cur_timetick = GetTickCount();

	while (at->running)
	{
		if (max_timewait <= 0)
			max_timewait = AThreadCheckTimewait(pool);

		DWORD new_timetick = GetTickCount();
		max_timewait -= (new_timetick - cur_timetick);

		cur_timetick = new_timetick;
		if (max_timewait < 0)
			continue;
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
		int count = epoll_wait(at->epoll, at->events, _countof(at->events), max_timewait);
		for (int ix = 0; ix < count; ++ix)
		{
			AOperator *asop = (AOperator*)at->events[ix].data.ptr;
			if (asop != NULL) {
				asop->callback(asop, at->events[ix].events);
			} else {
				while (recv(at->signal[1], at->sigbuf, sizeof(at->sigbuf), 0) == sizeof(at->sigbuf))
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
		list_add_tail(&asop->ao_list, &at->working_list);
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
	AThread *&pool = work_thread[0];
#else
	AThread *pool = NULL;

	//SIGPIPE ignore
	struct sigaction act;
	act.sa_handler = SIG_IGN;

	int result = sigaction(SIGPIPE, &act, NULL);
	TRACE("SIGPIPE ignore, result = %d.\n", result);

	//
	struct rlimit rl;
	result = getrlimit(RLIMIT_NOFILE, &rl);

	if (result == 0) {
		rl.rlim_cur = rl.rlim_max;

		result = setrlimit(RLIMIT_NOFILE, &rl);
		TRACE("setrlimit(%d, %d) = %d.\n", RLIMIT_NOFILE, rl.rlim_cur, result);
	}
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

	AThread *at = (AThread*)malloc(sizeof(AThread));
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
		if (at->epoll < 0) {
			delete at;
			return -EFAULT;
		}

		if (socketpair(AF_UNIX, SOCK_STREAM, 0, at->signal) != 0) {
			close(at->epoll);
			delete at;
			return -EFAULT;
		}

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
		INIT_RB_ROOT(&at->waiting_tree);
		INIT_LIST_HEAD(&at->pending_list);
	}

	*p = at;
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
		free(at);
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
	while (!RB_EMPTY_ROOT(&at->waiting_tree)) {
		asop = rb_entry(rb_first(&at->waiting_tree), AOperator, ao_tree);

		while (!list_empty(&asop->ao_list)) {
			AOperator *asop2 = list_first_entry(&asop->ao_list, AOperator, ao_list);
			list_del_init(&asop2->ao_list);
			asop2->callback(asop2, -EINTR);
		}

		rb_erase(&asop->ao_tree, &at->waiting_tree);
		asop->callback(asop, -EINTR);
	}
	while (!list_empty(&at->pending_list)) {
		asop = list_first_entry(&at->pending_list, AOperator, ao_list);
		list_del_init(&asop->ao_list);
		asop->callback(asop, -EINTR);
	}
	free(at);
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
			int ix = 1;
			if (_countof(work_thread) <= 1)
				ix = 0;

			at = work_thread[ix];
			while (++ix < _countof(work_thread)) {
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
		list_add_tail(&asop->ao_list, &at->pending_list);
		pthread_mutex_unlock(&at->mutex);
		return 0;
	}

	pthread_mutex_lock(&at->mutex);
	AOperator *node = rb_search_AOperator(&at->waiting_tree, asop->ao_tick);

	if (RB_EMPTY_ROOT(&at->waiting_tree)
	 || ((node == NULL) && (AOperatorTimewaitCompare(asop->ao_tick,
	                        rb_entry(rb_first(&at->waiting_tree), AOperator, ao_tree)) < 0)))
		tick = 0;

	if ((node == NULL) || (node->ao_tick != asop->ao_tick)) {
		rb_insert_AOperator(&at->waiting_tree, asop, asop->ao_tick);
		INIT_LIST_HEAD(&asop->ao_list);
	} else {
		list_add_tail(&asop->ao_list, &node->ao_list);
		//TRACE2("combine list timewait = %d.\n", tick);
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
		list_del_init(&asop->ao_list);
	}
	else if (asop->ao_tick != 0) {
		AOperator *node = rb_search_AOperator(&at->waiting_tree, asop->ao_tick);
		if (node == NULL) {
			assert(0);
			signal = 0;
		}
		else if (asop != node) {
			assert(!list_empty(&node->ao_list));
			assert(!list_empty(&asop->ao_list));
			list_del_init(&asop->ao_list);
		}
		else if (list_empty(&asop->ao_list)) {
			rb_erase(&asop->ao_tree, &at->waiting_tree);
		}
		else {
			node = list_first_entry(&asop->ao_list, AOperator, ao_list);
			rb_replace_node(&asop->ao_tree, &node->ao_tree, &at->waiting_tree);
			list_del_init(&asop->ao_list);
		}
		asop->ao_tick = 0;
	}
	pthread_mutex_unlock(&at->mutex);

	if (signal && !cancel) {
		AThreadWakeup(at, asop);
	}
	return signal;
}

