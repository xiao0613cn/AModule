#ifndef _BASE_UTIL_H_
#define _BASE_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "str_util.h"


#ifndef container_of
#define container_of(ptr, type, member)   ((type*)((char*)(ptr) - (char*)(&((type*)0)->member)))
#endif

#ifdef _WIN32
#ifndef __attribute__
#define __attribute__(x) 
#endif

static inline void*
dlsym(void *handle, const char *symbol) {
	return GetProcAddress((HMODULE)handle, symbol);
}

#else //_WIN32

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/in.h>

#ifndef __stdcall
#define __stdcall  
#endif
#ifndef __cdecl
#define __cdecl  
#endif
#ifndef __fastcall
#define __fastcall  
#endif

#ifndef max
#define max(a, b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef SOCKET
typedef int             SOCKET;
#define INVALID_SOCKET  -1
#define closesocket(fd) close(fd)
#define SD_RECEIVE      SHUT_RD
#define SD_SEND         SHUT_WR
#define SD_BOTH         SHUT_RDWR
#endif

#ifndef INFINITE
#define INFINITE  -1u
#endif
#ifndef _countof
#define _countof(a)  (sizeof(a)/sizeof(a[0]))
#endif

typedef void *         HANDLE;
typedef uint32_t       DWORD;
typedef int            BOOL;
#define TRUE      1
#define FALSE     0

#endif //!_WIN32

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL  0
#endif

#include <stdarg.h>
#ifdef ANDROID
#include <android/log.h>
#endif

#ifndef TRACE
static int
DTRACE(const char *f, int l, const char *fmt, ...)
{
	char outbuf[BUFSIZ];
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	int outpos = snprintf(outbuf, sizeof(outbuf),
		tm_fmt" %4d | %s():\t", tm_args(tm), l, f);

	va_list ap; va_start(ap, fmt);
	int len = vsnprintf(outbuf+outpos, sizeof(outbuf)-outpos, fmt, ap);
	va_end(ap);
	if (len < 0) {
		outpos = BUFSIZ-1;
		outbuf[outpos] = '\0';
	} else {
		outpos += len;
	}

#ifdef _WIN32
	OutputDebugStringA(outbuf);
#elif defined(ANDROID)
	__android_log_write(ANDROID_LOG_DEBUG, "AModule", outbuf);
#endif
	fputs(outbuf, stdout);
	return outpos;
}
#define TRACE(fmt, ...)  ALog2(NULL, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#endif //!TRACE

#ifdef _DEBUG
#include <assert.h>
#define TRACE2     TRACE
#else //_DEBUG
#ifndef assert
#define assert(x)  (void)(0)
#endif
#ifndef TRACE2
#define TRACE2(fmt, ...)  (void)(0)
#endif
#endif //!_DEBUG


#endif

