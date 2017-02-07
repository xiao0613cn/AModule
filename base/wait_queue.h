#ifndef _WAIT_QUEUE_H_
#define _WAIT_QUEUE_H_


typedef struct WaitQueue
{
	unsigned int     count;
	struct list_head queue;
	pthread_mutex_t  mutex;
#ifdef _WIN32
	HANDLE           event;
#else
	pthread_cond_t   cond;
#endif
#ifdef __cplusplus
	inline void init();
	inline void uninit();
	inline BOOL push(struct list_head *node, BOOL signal);
	inline struct list_head* pop(DWORD msec);
#endif
} WaitQueue;

static inline void
WQ_Init(WaitQueue *wq)
{
	wq->count = 0;
	INIT_LIST_HEAD(&wq->queue);
	pthread_mutex_init(&wq->mutex, NULL);
#ifdef _WIN32
	wq->event = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	pthread_cond_init_mono(&wq->cond);
#endif
}

static inline void
WQ_Uninit(WaitQueue *wq)
{
	assert(list_empty(&wq->queue));
	pthread_mutex_destroy(&wq->mutex);
#ifdef _WIN32
	CloseHandle(wq->event);
#else
	pthread_cond_destroy(&wq->cond);
#endif
}

static inline BOOL
WQ_Push(WaitQueue *wq, struct list_head *node, BOOL signal)
{
	pthread_mutex_lock(&wq->mutex);
	BOOL first = list_empty(&wq->queue);
	wq->count++;
	list_add_tail(node, &wq->queue);
	pthread_mutex_unlock(&wq->mutex);

	if (first && signal) {
#ifdef _WIN32
		SetEvent(wq->event);
#else
		pthread_cond_signal(&wq->cond);
#endif
	}
	return first;
}

static inline struct list_head*
WQ_Pop(WaitQueue *wq, DWORD msec)
{
	struct list_head *node = NULL;
#ifdef _WIN32
	if ((msec != 0) && list_empty(&wq->queue)) {
		WaitForSingleObject(wq->event, msec);
	}
	pthread_mutex_lock(&wq->mutex);
#else
	pthread_mutex_lock(&wq->mutex);
	if (list_empty(&wq->queue)) {
		if (msec == INFINITE)
			pthread_cond_wait(&wq->cond, &wq->mutex);
		else if (msec != 0)
			pthread_cond_wait_mono(&wq->cond, &wq->mutex, msec);
	}
#endif
	if (!list_empty(&wq->queue)) {
		node = wq->queue.next;
		list_del_init(node);
		wq->count--;
	}
	pthread_mutex_unlock(&wq->mutex);
	return node;
}

#define WQ_PopNode(p, wq, msec, type, member) \
{ \
	struct list_head *node = WQ_Pop(wq, msec); \
	if (node != NULL) \
		p = list_entry(node, type, member); \
	else \
		p = NULL; \
}

#ifdef __cplusplus
inline void WaitQueue::init() { WQ_Init(this); }
inline void WaitQueue::uninit() { WQ_Uninit(this); }
inline BOOL WaitQueue::push(struct list_head *node, BOOL signal) { return WQ_Push(this, node, signal); }
inline struct list_head* WaitQueue::pop(DWORD msec) { return WQ_Pop(this, msec); }
#endif
#endif
