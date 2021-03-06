#ifndef _AMODULE_API_H_
#define _AMODULE_API_H_

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C     extern "C"
#else
#define EXTERN_C     extern
#endif
#endif //!EXTERN_C

#if defined(AMODULE_API_EXPORTS)
#define AMODULE_API  EXTERN_C __declspec(dllexport)
#elif defined(_WIN32) && !defined(AMODULE_API_INNER)
#define AMODULE_API  EXTERN_C __declspec(dllimport)
#else
#define AMODULE_API  EXTERN_C
#endif


#ifdef _WIN32
#pragma warning(disable: 4100) // 未引用的形参
#pragma warning(disable: 4101) // 未引用的局部变量
#pragma warning(disable: 4127) // 条件表达式是常量
#pragma warning(disable: 4189) // 局部变量已初始化但不引用
#pragma warning(disable: 4200) // 使用了非标准扩展 : 结构/联合中的零大小数组
#pragma warning(disable: 4201) // 使用了非标准扩展 : 无名称的结构/联合
#pragma warning(disable: 4505) // 未引用的本地函数已移除
#pragma warning(disable: 4512) // 未能生成赋值运算符
#else //_WIN32
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //!_WIN32


//////////////////////////////////////////////////////////////////////////
// all api return value:
// > 0: completed with success
// = 0: pending with success, will callback msg->done()
// < 0: call failed, return error code

#include "base.h"
#include "str_util.h"
#include "func_util.h"
#include "thread_util.h"
#include "list.h"
#include "rbtree.h"
#include "iocp_util.h"
#include "buf_util.h"

#include "AOperator.h"
#include "AOption.h"
#include "AMessage.h"
#include "AModule.h"


AMODULE_API void*
dlload(const char *relative_path, const char *dll_name);

AMODULE_API const char*
getexepath(char *path, int size);

AMODULE_API int
ALog(const char *name, const char *func, int line, const char *fmt, va_list ap);

static inline int
ALog2(const char *name, const char *f, int l, const char *fmt, ...) {
	va_list ap; va_start(ap, fmt);
	int result = ALog(name, f, l, fmt, ap);
	va_end(ap); return result;
}


#endif
