// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#ifdef _WIN32
#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN		// �� Windows ͷ���ų�����ʹ�õ�����

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

#pragma warning(disable: 4200) // ʹ���˷Ǳ�׼��չ : �ṹ/�����е����С����
#endif //_WIN32

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include "base/AModule_API.h"
