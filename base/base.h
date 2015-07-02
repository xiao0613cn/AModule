#pragma once

#ifdef _DEBUG
#pragma warning(disable: 4985)
#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW
#include <crtdbg.h>
#pragma warning(default: 4985)
#endif


#include <errno.h>
#include <Windows.h>


#if 1

#ifndef TRACE
#include <stdio.h>
#include <time.h>
static __inline int
DTRACE(const char *f, int l, const char *fmt, ...)
{
	char outbuf[BUFSIZ];
	int  outpos;

	struct tm tm;
	time_t t = time(NULL);
	localtime_s(&tm, &t);

	outpos = sprintf_s(
		outbuf, BUFSIZ,
		"[%04d-%02d-%02d %02d:%02d:%02d] %4d| %s: ",
		1900+tm.tm_year, 1+tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		l, f);

	va_list ap;
	va_start(ap, fmt);
	outpos += vsprintf_s(outbuf+outpos, BUFSIZ-outpos, fmt, ap);
	va_end(ap);

	OutputDebugStringA(outbuf);
	fputs(outbuf, stdout);
	return outpos;
}
#define TRACE2(fmt, ...)   DTRACE(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define TRACE(fmt, ...)  DTRACE(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

#ifndef ASSERT
#include <assert.h>
#define ASSERT   assert
#define VERIFY   assert
#endif

#else // _DEBUG

#ifndef TRACE
#define TRACE(fmt, ...) (void)0
#endif

#ifndef ASSERT
#define ASSERT(x) (void)(0)
#define VERIFY(x) (void)(x)
#endif

#ifndef assert
#define assert(x) (void)(0)
#endif

#endif // _RELEASE


#ifndef _tostring
#define _tostring(x) #x
#endif


#ifndef _align_8bytes
#define _align_8bytes(x) (((x)+7)&~7)
#endif


