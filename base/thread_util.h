#ifndef _THREAD_UTIL_H_
#define _THREAD_UTIL_H_

#ifdef _WIN32

#ifndef InterlockedAdd
#define InterlockedAdd(count, value)   (InterlockedExchangeAdd(count,value) + value)
#endif

#ifndef _INC_PROCESS
#include <process.h>
#endif

#define pthread_null  NULL
#ifndef PTHREAD_H
//////////////////////////////////////////////////////////////////////////
// pthread_mutex_t
typedef CRITICAL_SECTION pthread_mutex_t;
typedef struct pthread_mutexattr_t pthread_mutexattr_t;

static inline int 
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	InitializeCriticalSection(mutex);
	return 0;
}

static inline int 
pthread_mutex_lock(pthread_mutex_t *mutex) {
	EnterCriticalSection(mutex);
	return 0;
}

static inline int 
pthread_mutex_trylock(pthread_mutex_t *mutex) {
	return !TryEnterCriticalSection(mutex);
}

static inline int 
pthread_mutex_unlock(pthread_mutex_t *mutex) {
	LeaveCriticalSection(mutex);
	return 0;
}

static inline int 
pthread_mutex_destroy(pthread_mutex_t *mutex) {
	DeleteCriticalSection(mutex);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// pthread_t
typedef HANDLE pthread_t;
typedef struct pthread_attr_t pthread_attr_t;

static inline int 
pthread_create(pthread_t *tid, const pthread_attr_t *attr, void*(*start)(void*), void *arg) {
	*tid = (pthread_t)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))start, arg, 0, NULL);
	return (*tid ? 0 : errno);
}

static inline int 
pthread_detach(pthread_t tid) {
	return !CloseHandle(tid);
}

static inline int 
pthread_join(pthread_t tid, void **value_ptr) {
	WaitForSingleObject(tid, INFINITE);
	if (value_ptr != NULL)
		GetExitCodeThread(tid, (LPDWORD)value_ptr);
	return !CloseHandle(tid);
}

static inline pthread_t
pthread_self() {
	return GetCurrentThread();
}

static inline unsigned long
gettid() {
	return GetCurrentThreadId();
}

//////////////////////////////////////////////////////////////////////////
// pthread_cond_t
typedef HANDLE pthread_cond_t;

static inline int
pthread_cond_init_mono(pthread_cond_t *cond, int broadcast) {
	*cond = CreateEvent(NULL, broadcast, FALSE, NULL);
	return (*cond == NULL);
}

static inline int
pthread_cond_wait_mono(pthread_cond_t *cond, pthread_mutex_t *mutex, unsigned long msec) {
	pthread_mutex_unlock(mutex);
	DWORD ret = WaitForSingleObject(*cond, msec);
	pthread_mutex_lock(mutex);
	return (ret != WAIT_OBJECT_0);
}

static inline int
pthread_cond_signal(pthread_cond_t *cond) {
	return !SetEvent(*cond);
}

static inline int
pthread_cond_broadcast(pthread_cond_t *cond) {
	return !SetEvent(*cond);
}

static inline int
pthread_cond_destroy(pthread_cond_t *cond) {
	return !CloseHandle(*cond);
}
#endif //PTHREAD_H

static inline DWORD
WaitEvent(HANDLE ev, DWORD ms) {
	return WaitForSingleObject(ev, ms);
}

static inline void
CloseEvent(HANDLE ev) {
	CloseHandle(ev);
}

#else //_WIN32

static inline DWORD 
GetTickCount(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec*1000 + ts.tv_nsec/(1000*1000);
}

static inline void 
Sleep(DWORD ms) {
	usleep(ms*1000);
}

#include <pthread.h>
#define  pthread_null  0

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
static inline int
pthread_cond_init_mono(pthread_cond_t *cond, int broadcast) {
	return pthread_cond_init(cond, NULL);
}

static inline int
pthread_cond_wait_mono(pthread_cond_t *cond, pthread_mutex_t *mutex, unsigned long msec) {
	if (msec == INFINITE)
		return pthread_cond_wait(cond, mutex);

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	long nsec = ts.tv_nsec + (msec%1000)*(1000*1000);
	ts.tv_sec += msec/1000 + nsec/(1000*1000*1000);
	ts.tv_nsec = nsec%(1000*1000*1000);

	return pthread_cond_timedwait_monotonic_np(cond, mutex, &ts);
}
#else
static inline int
pthread_cond_init_mono(pthread_cond_t *cond, int broadcast) {
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

	int ret = pthread_cond_init(cond, &attr);
	pthread_condattr_destroy(&attr);
	return ret;
}

static inline int
pthread_cond_wait_mono(pthread_cond_t *cond, pthread_mutex_t *mutex, unsigned long msec) {
	if (msec == INFINITE)
		return pthread_cond_wait(cond, mutex);

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	long nsec = ts.tv_nsec + (msec%1000)*(1000*1000);
	ts.tv_sec += msec/1000 + nsec/(1000*1000*1000);
	ts.tv_nsec = nsec%(1000*1000*1000);

	return pthread_cond_timedwait(cond, mutex, &ts);
}
#endif
#ifndef ANDROID
static inline pid_t
gettid() {
	return syscall(SYS_gettid);
}
#endif

static inline long
InterlockedAdd(long volatile *count, long value) {
	return __sync_add_and_fetch(count, value);
}

static inline long
InterlockedExchangeAdd(long volatile *count, long value) {
	return __sync_fetch_and_add(count, value);
}

static inline long
InterlockedCompareExchange(long volatile *count, long change, long compare) {
	return __sync_val_compare_and_swap(count, compare, change);
}

static inline long
InterlockedExchange(long volatile *count, long value) {
	return __sync_lock_test_and_set(count, value);
}

//////////////////////////////////////////////////////////////////////////
typedef struct WinEvent {
	int  signal_state : 1;
	int  manual_reset : 1;
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
} WinEvent;

static inline void
WinEvent_Init(WinEvent *ev, int manual, int state) {
	ev->signal_state = state;
	ev->manual_reset = manual;
	pthread_mutex_init(&ev->mutex, NULL);
	pthread_cond_init_mono(&ev->cond, manual);
}

static inline HANDLE
CreateEvent(const void *attr, int manual, int state, const char *name) {
	if (name != NULL) // not support!!
		return NULL;

	WinEvent *ev = gomake(WinEvent);
	if (ev != NULL)
		WinEvent_Init(ev, manual, state);
	return ev;
}

#define WAIT_OBJECT_0   0
#define WAIT_ABANDONED  0x00000080L
#define WAIT_TIMEOUT    0x00000102L

static inline int
WaitEvent(HANDLE h, DWORD msec) {
	WinEvent *ev = (WinEvent*)h;

	pthread_mutex_lock(&ev->mutex);
	if (msec == INFINITE) {
		while (!ev->signal_state) {
			pthread_cond_wait(&ev->cond, &ev->mutex);
		}
	} else if (!ev->signal_state && (msec != 0)) {
		pthread_cond_wait_mono(&ev->cond, &ev->mutex, msec);
	}

	int result = (ev->signal_state ? WAIT_OBJECT_0 : WAIT_TIMEOUT);
	if (ev->signal_state && !ev->manual_reset)
		ev->signal_state = 0;
	pthread_mutex_unlock(&ev->mutex);
	return result;
}

static inline BOOL
SetEvent(HANDLE h) {
	WinEvent *ev = (WinEvent*)h;

	pthread_mutex_lock(&ev->mutex);
	int set = ev->signal_state;
	ev->signal_state = 1;
	pthread_mutex_unlock(&ev->mutex);

	if (!set) {
		if (ev->manual_reset)
			pthread_cond_broadcast(&ev->cond);
		else
			pthread_cond_signal(&ev->cond);
	}
	return 1;
}

static inline BOOL
ResetEvent(HANDLE h) {
	WinEvent *ev = (WinEvent*)h;

	pthread_mutex_lock(&ev->mutex);
	int set = ev->signal_state;
	ev->signal_state = 0;
	pthread_mutex_unlock(&ev->mutex);
	return 1;
}

static inline void
WinEvent_Uninit(WinEvent *ev) {
	pthread_mutex_destroy(&ev->mutex);
	pthread_cond_destroy(&ev->cond);
}

static inline void
CloseEvent(HANDLE h) {
	WinEvent *ev = (WinEvent*)h;
	WinEvent_Uninit(ev);
	free(ev);
}
#endif //!_WIN32

static inline void
pthread_post(void *arg, void*(*func)(void*)) {
	pthread_t tid;
	pthread_create(&tid, NULL, func, arg);
	pthread_detach(tid);
}

#ifdef __cplusplus
template <typename object_t, void(object_t::*run)()>
void* pthread_object_run(void *p) {
	(((object_t*)p)->*run)();
	return NULL;
}

struct pthread_auto_lock {
	pthread_mutex_t *_mutex;

	pthread_auto_lock(pthread_mutex_t *m) {
		_mutex = m;
		pthread_mutex_lock(_mutex);
	}
	~pthread_auto_lock() {
		pthread_mutex_unlock(_mutex);
	}
};
#endif //__cplusplus

#endif
