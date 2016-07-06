#ifndef _BASE_UTIL_H_
#define _BASE_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#ifndef _tostring
#define _tostring(x) #x
#endif

#ifndef _align_8bytes
#define _align_8bytes(x) (((x)+7)&~7)
#endif

#ifdef _DEBUG
#include <assert.h>

#ifndef TRACE
static int
DTRACE(const char *f, int l, const char *fmt, ...)
{
	char outbuf[BUFSIZ];
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	int outpos = sprintf_s(
		outbuf, BUFSIZ,
		"[%04d-%02d-%02d %02d:%02d:%02d] %4d| [%s]: ",
		1900+tm->tm_year, 1+tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		l, f);

	va_list ap;
	va_start(ap, fmt);
	outpos += vsprintf_s(outbuf+outpos, BUFSIZ-outpos, fmt, ap);
	va_end(ap);

#ifdef _WIN32
	OutputDebugStringA(outbuf);
#endif
	fputs(outbuf, stdout);
	return outpos;
}
#define TRACE2(fmt, ...)   DTRACE(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define TRACE(fmt, ...)  DTRACE(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

#else //_DEBUG

#ifndef assert
#define assert(x)  (void)(0)
#endif

#ifndef TRACE
#define TRACE(fmt, ...) (void)(0)
#endif

#endif //_DEBUG

#ifdef _WIN32

#ifndef _INC_PROCESS
#include <process.h>
#endif

#ifndef PTHREAD_H
#define PTHREAD_H

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

typedef HANDLE pthread_t;
typedef struct pthread_attr_t pthread_attr_t;
#define pthread_null  NULL

static inline int 
pthread_create(pthread_t *tid, const pthread_attr_t *attr, void*(*start)(void*), void *arg) {
	*tid = (pthread_t)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))start, arg, 0, NULL);
	return (*tid ? 0 : errno);
}

static inline int 
pthread_detach(pthread_t tid) {
	CloseHandle(tid);
	return 0;
}

static inline int 
pthread_join(pthread_t tid, void **value_ptr) {
	WaitForSingleObject(tid, INFINITE);
	if (value_ptr != NULL)
		GetExitCodeThread(tid, (LPDWORD)value_ptr);
	CloseHandle(tid);
	return 0;
}
#endif

#else //_WIN32

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <pthread.h>
static const pthread_t pthread_null = { 0 };

#ifndef _stricmp
#define _stricmp     strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp    strncasecmp
#endif
#ifndef strcpy_s
#define strcpy_s(dest, src)  strncpy(dest, src, sizeof(dest)-1)
#endif
#ifndef max
#define max(a, b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)    (((a) < (b)) ? (a) : (b))
#endif
#ifndef SOCKET
typedef int SOCKET;
#define INVALID_SOCKET  -1
#define closesocket(fd) close(fd)
#endif
#ifndef INFINITE
#define INFINITE  -1
#endif
#ifndef _countof
#define _countof(a)  (sizeof(a)/sizeof(a[0]))
#endif

typedef unsigned long  u_long;
typedef unsigned long  DWORD;
typedef long long      LONGLONG;
typedef unsigned long long ULONGLONG;
typedef unsigned long long DWORDLONG;
typedef unsigned int   UINT;
typedef short          SHORT;
typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef int            BOOL;
#define TRUE      1
#define FALSE     0

static inline DWORD 
GetTickCount(void) {
	timespec ts;
	syscall(SYS_clock_gettime, CLOCK_MONOTONIC_RAW, &ts);
	return ts.tv_sec*1000 + ts.tv_nsec/(1000*1000);
}

static void 
Sleep(DWORD ms) {
	usleep(ms*1000);
}

static inline long
InterlockedIncrement(long volatile *count) {
	return __sync_add_and_fetch(count, 1);
}

static inline long
InterlockedDecrement(long volatile *count) {
	return __sync_add_and_fetch(count, -1);
}


#endif //_WIN32

#define strnicmp_c(ptr, c_str)  _strnicmp(ptr, c_str, sizeof(c_str)-1)


#endif

