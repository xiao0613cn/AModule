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
	AThread         *pool;
	int              max_timewait;
#ifdef _WIN32
	HANDLE           iocp;
#else
	int              epoll;
	int              signal[2];
	long volatile    bind_count;
	struct list_head working_list;
	struct list_head private_list;
#endif
	pthread_mutex_t  mutex;
	struct rb_root   waiting_tree;
	struct list_head pending_list;
};

static int AThreadCheckTimewait(AThread *pool, AThread *at)
{
	int max_timewait = at->max_timewait;
	AOperator *asop;

	struct list_head timeout_list;
	INIT_LIST_HEAD(&timeout_list);
#ifndef _WIN32
	struct list_head working_list;
	INIT_LIST_HEAD(&working_list);
#endif
	DWORD curtick = GetTickCount();
	pthread_mutex_lock(&pool->mutex);

	for (struct rb_node *node = rb_first(&pool->waiting_tree); node != NULL; ) {
		asop = rb_entry(node, AOperator, ao_tree);

		int diff = AOperatorTimewaitCompare(curtick, asop);
		if (diff < -1) {
			if (asop->ao_thread == NULL) {
				asop->ao_thread = at;
			} else if (asop->ao_thread == at) {
				;
			} else {
				node = rb_next(node);
				continue;
			}
			max_timewait = std::min(at->max_timewait, -diff);
			break;
		}

		/*AOperator *pos;
		list_for_each_entry(pos, &asop->ao_list, AOperator, ao_list) {
			pos->ao_tick = 0;
		}
		asop->ao_tick = 0;*/

		struct list_head *last = asop->ao_list.prev;
		asop->ao_list.prev = timeout_list.prev;
		timeout_list.prev->next = &asop->ao_list;

		last->next = &timeout_list;
		timeout_list.prev = last;
		rb_erase(&asop->ao_tree, &pool->waiting_tree);
		node = rb_first(&pool->waiting_tree);
	}
#ifndef _WIN32
	list_splice_init(&pool->working_list, &working_list);
	list_splice_init(&at->private_list, &working_list);
#endif
	pthread_mutex_unlock(&pool->mutex);

	while (!list_empty(&timeout_list)) {
		asop = list_first_entry(&timeout_list, AOperator, ao_list);
		list_del_init(&asop->ao_list);
		asop->ao_tick = 0;
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
	AThread *pool = (at->pool ? at->pool : at);

	int max_timewait = 0;
	DWORD cur_timetick = GetTickCount();
#ifdef _WIN32
#else
	struct epoll_event events[64];
	char sigbuf[128];
#endif
	while (at->running)
	{
		if (max_timewait <= 0)
			max_timewait = AThreadCheckTimewait(pool, at);

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
		int count = epoll_wait(at->epoll, events, _countof(events), max_timewait);
		for (int ix = 0; ix < count; ++ix)
		{
			if (events[ix].data.u64 == 0) {
				while (recv(pool->signal[1], sigbuf, sizeof(sigbuf), 0) == sizeof(sigbuf))
					;
				//TRACE2("%d: wakeup for working_list...\n", syscall(__NR_gettid));
				max_timewait = 0;
			}
			else if (events[ix].data.u64 == 1) {
				while (recv(at->signal[0], sigbuf, sizeof(sigbuf), 0) == sizeof(sigbuf))
					;
				TRACE2("%d: wakeup for private_list...\n", syscall(__NR_gettid));
				max_timewait = 0;
			}
			else {
				AOperator *asop = (AOperator*)events[ix].data.ptr;
				asop->callback(asop, events[ix].events);
			}
		}
#endif
	}
	return 0;
}

static inline void
AThreadWakeup(AThread *at, BOOL is_pool)
{
#ifdef _WIN32
	PostQueuedCompletionStatus(at->iocp, 0, iocp_key_signal, NULL);
#else
	send(at->signal[!is_pool], "a", 1, MSG_NOSIGNAL);
#endif
}

//////////////////////////////////////////////////////////////////////////
static AThread *work_thread[4];
static int work_thread_begin(int max_timewait)
{
#ifndef _WIN32
	//SIGPIPE ignore
	//struct sigaction act;
	//act.sa_handler = SIG_IGN;

	//int result = sigaction(SIGPIPE, &act, NULL);
	//TRACE("SIGPIPE ignore, result = %d.\n", result);

	//
	struct rlimit rl;
	int result = getrlimit(RLIMIT_NOFILE, &rl);

	if (result == 0) {
		rl.rlim_cur = rl.rlim_max;

		result = setrlimit(RLIMIT_NOFILE, &rl);
		TRACE("setrlimit(%d, %d) = %d.\n", RLIMIT_NOFILE, rl.rlim_cur, result);
	}
#endif
	AThread *&pool = work_thread[0];
	for (int ix = 0; ix < _countof(work_thread); ++ix) {
		AThreadBegin(&work_thread[ix], pool, max_timewait);
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
AThreadBegin(AThread **p, AThread *pool, int max_timewait)
{
	if (p == NULL)
		return work_thread_begin(max_timewait);

	AThread *at = (AThread*)malloc(sizeof(AThread));
	at->pool = pool;
	at->max_timewait = max_timewait;

#ifdef _WIN32
	if (pool != NULL) {
		at->iocp = pool->iocp;
	} else {
		at->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	}
#else
	at->epoll = epoll_create(32000);
	if (at->epoll < 0) {
		free(at);
		return -EFAULT;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, at->signal) != 0) {
		close(at->epoll);
		free(at);
		return -EFAULT;
	}

	tcp_nonblock(at->signal[0], 1);
	tcp_nonblock(at->signal[1], 1);

	//private_list signal
	struct epoll_event epev;
	epev.events = EPOLLIN|EPOLLET|EPOLLHUP|EPOLLERR;
	epev.data.u64 = 1;
	epoll_ctl(at->epoll, EPOLL_CTL_ADD, at->signal[0], &epev);

	//working_list signal
	epev.events = EPOLLIN|EPOLLET|EPOLLHUP|EPOLLERR;
	epev.data.u64 = 0;
	if (pool != NULL) {
		epoll_ctl(at->epoll, EPOLL_CTL_ADD, pool->signal[1], &epev);
	} else {
		epoll_ctl(at->epoll, EPOLL_CTL_ADD, at->signal[1], &epev);
		INIT_LIST_HEAD(&at->working_list);
	}

	at->bind_count = 0;
	INIT_LIST_HEAD(&at->private_list);
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

#ifdef _WIN32
	if (at->pool != NULL) {
		free(at);
		return 1;
	}
	CloseHandle(at->iocp);
#else
	close(at->signal[0]);
	close(at->signal[1]);
	close(at->epoll);

	while (!list_empty(&at->private_list)) {
		AOperator *asop = list_first_entry(&at->private_list, AOperator, ao_list);
		list_del_init(&asop->ao_list);
		asop->callback(asop, -EINTR);
	}
	if (at->pool != NULL) {
		free(at);
		return 1;
	}

	list_splice_init(&at->working_list, &at->pending_list);
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

AMODULE_API int
AThreadPost(AThread *at, AOperator *asop, BOOL signal)
{
	if (at == NULL)
		at = work_thread[0];
	AThread *pool = (at->pool ? at->pool : at);

	if (asop == NULL) {
		AThreadWakeup(pool, TRUE);
		return 0;
	}
#ifdef _WIN32
	PostQueuedCompletionStatus(at->iocp, 1, iocp_key_signal, &asop->ao_ovlp);
#else
	int first;
	pthread_mutex_lock(&pool->mutex);
	if (asop->ao_thread == at) {
		first = (list_empty(&at->private_list) ? 2 : 0);
		list_add_tail(&asop->ao_list, &at->private_list);
	} else {
		first = (list_empty(&pool->working_list) ? 1 : 0);
		list_add_tail(&asop->ao_list, &pool->working_list);
		at = pool;
	}
	pthread_mutex_unlock(&pool->mutex);

	if ((first != 0) && signal)
		AThreadWakeup(at, (first==1));
#endif
	return 0;
}

int AThreadAbort(AThread *at)
{
	if (at == NULL)
		at = work_thread[0];

	at->running = 0;
	AThreadWakeup(at, FALSE);
	return 1;
}
#ifdef _WIN32
AMODULE_API int
AThreadBind(AThread *at, HANDLE file)
{
	if (at == NULL)
		at = work_thread[0];

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
	asop->ao_tick = tick;
	if (tick == 0) {
		AThreadPost(at, asop, TRUE);
		return 0;
	}

	if (at == NULL)
		at = work_thread[0];
	AThread *pool = (at->pool ? at->pool : at);

	if (tick == INFINITE) {
		pthread_mutex_lock(&pool->mutex);
		list_add_tail(&asop->ao_list, &pool->pending_list);
		pthread_mutex_unlock(&pool->mutex);
		return 0;
	}

	asop->ao_thread = NULL;
	INIT_LIST_HEAD(&asop->ao_list);

	pthread_mutex_lock(&pool->mutex);
	AOperator *node = rb_insert_AOperator(&pool->waiting_tree, asop, tick);
	if (node != NULL) {
		list_add_tail(&asop->ao_list, &node->ao_list);
	} else {
		if (&asop->ao_tree == rb_first(&pool->waiting_tree))
			tick = 0;
	}
	pthread_mutex_unlock(&pool->mutex);

	if (tick == 0) {
		AThreadWakeup(pool, TRUE);
	}
	return 0;
}

AMODULE_API int
AOperatorSignal(AOperator *asop, AThread *at, BOOL cancel)
{
	if (at == NULL)
		at = work_thread[0];
	AThread *pool = (at->pool ? at->pool : at);

	pthread_mutex_lock(&pool->mutex);
	int signal = (asop->ao_tick ? 1 : -1);

	if (asop->ao_tick == INFINITE) {
		asop->ao_tick = 0;
		list_del_init(&asop->ao_list);
	}
	else if (asop->ao_tick != 0) {
		AOperator *node = rb_search_AOperator(&pool->waiting_tree, asop->ao_tick);
		asop->ao_tick = 0;

		if (node == NULL) {
			assert(0);
			signal = -1;
		}
		else if (asop != node) {
			assert(!list_empty(&node->ao_list));
			assert(!list_empty(&asop->ao_list));
			list_del_init(&asop->ao_list);
		}
		else if (list_empty(&asop->ao_list)) {
			rb_erase(&asop->ao_tree, &pool->waiting_tree);
		}
		else {
			node = list_first_entry(&asop->ao_list, AOperator, ao_list);
			rb_replace_node(&asop->ao_tree, &node->ao_tree, &pool->waiting_tree);
			list_del_init(&asop->ao_list);
		}
	}
#ifndef _WIN32
	if ((signal > 0) && !cancel) {
		if (asop->ao_thread == at) {
			signal = (list_empty(&at->private_list) ? 2 : 0);
			list_add_tail(&asop->ao_list, &at->private_list);
		} else {
			signal = (list_empty(&pool->working_list) ? 1 : 0);
			at = pool;
			list_add_tail(&asop->ao_list, &pool->working_list);
		}
	}
#endif
	pthread_mutex_unlock(&pool->mutex);

	if ((signal > 0) && !cancel) {
#ifdef _WIN32
		PostQueuedCompletionStatus(pool->iocp, 1, iocp_key_signal, &asop->ao_ovlp);
#else
		AThreadWakeup(at, (signal==1));
#endif
		signal = 0;
	}
	return signal;
}

