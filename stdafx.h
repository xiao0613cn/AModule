// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#ifdef _WIN32
#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
#pragma warning(disable: 4985)
#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW
#include <crtdbg.h>
#pragma warning(default: 4985)
#endif

#include <tchar.h>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>

#pragma warning(disable: 4200)
#endif

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include "base/AModule_API.h"
