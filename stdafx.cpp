// stdafx.cpp : 只包括标准包含文件的源文件
// InterfaceMethod.pch 将作为预编译头
// stdafx.object 将包含预编译类型信息

#include "stdafx.h"

// TODO: 在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用
#if defined(_WIN32) && defined(_DEBUG)
static int dbg_flag = _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
