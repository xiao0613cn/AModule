// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#ifdef _DEBUG
#pragma warning(disable: 4985)
#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW
#include <crtdbg.h>
#pragma warning(default: 4985)
#endif

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <errno.h>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>

// TODO: 在此处引用程序需要的其他头文件
#include "base/base.h"
