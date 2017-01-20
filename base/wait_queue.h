#ifndef _WAIT_QUEUE_H_
#define _WAIT_QUEUE_H_


typedef struct WaitQueue
{
	struct list_head queue;
	pthread_mutex_t  mutex;
#ifdef _WIN32
	HANDLE           event;
#else
	pthread_cond_t   cond;
#endif
} WaitQueue;

static inline void WQ_Init(WaitQueue *wq)
{
	INIT_LIST_HEAD(&wq->queue);
	pthread_mutex_init(&wq->mutex, NULL);
#ifdef _WIN32
	wq->event = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	WinEvent_Init(&wq->ev, FALSE, FALSE);
#endif
}

static inline void WQ_Uninit(WaitQueue *wq)
{
	assert(list_empty(&wq->queue));
	pthread_mutex_destroy(&wq->mutex);
#ifdef _WIN32
	CloseHandle(wq->event);
#else
	WinEvent_Uninit(&wq->ev);
#endif
}

static inline void WQ_Lock(WaitQueue *wq) {
	pthread_mutex_lock(&wq->lock);
}

static inline void WQ_Unlock(WaitQueue *wq) {
	pthread_mutex_unlock(&wq->lock);
}

static inline BOOL WQ_Push(WaitQueue *wq, struct list_head *node, BOOL signal)
{
	pthread_mutex_lock(&wq->lock);
	BOOL first = list_empty(&wq->queue);
	list_add_tail(node, &wq->queue);
	pthread_mutex_unlock(&wq->lock);

	if (first && signal)
		SetEvent(wq->event);
	return first;
}

#endif
