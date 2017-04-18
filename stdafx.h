// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#ifdef _WIN32
#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料

#ifdef _DEBUG
#pragma warning(disable: 4985)
#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW
#include <crtdbg.h>
#pragma warning(default: 4985)
#endif //_DEBUG

#include <tchar.h>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>

#pragma warning(disable: 4200) // 使用了非标准扩展 : 结构/联合中的零大小数组
#endif //_WIN32

// TODO: 在此处引用程序需要的其他头文件
#include "base/AModule_API.h"
