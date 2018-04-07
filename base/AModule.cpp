#include "stdafx.h"
#include "AModule_API.h"

static LIST_HEAD(g_module);
static AOption *g_option = NULL;
static BOOL g_inited = FALSE;
static long g_index = 0;

#define list_for_all_AModule(pos) \
	list_for_each2(pos, &g_module, AModule, global_entry)

#define list_for_class_module(pos, class_module) \
	list_for_each2(pos, &(class_module)->class_entry, AModule, class_entry)

static int  AModuleInitNull(AOption *global_option, AOption *module_option, int first) { return 0; }
static void AModuleExitNull(int inited) { }
static int  AModuleCreateNull(AObject **object, AObject *parent, AOption *option) { return -ENOSYS; }
static void AModuleReleaseNull(AObject *object) { }
static int  AObjectProbeNull(AObject *object, AObject *other, AMessage *msg) { return -ENOSYS; }

AMODULE_API int
AModuleRegister(AModule *module)
{
	if (module->init == NULL) module->init = &AModuleInitNull;
	if (module->exit == NULL) module->exit = &AModuleExitNull;
	if (module->create == NULL) module->create = &AModuleCreateNull;
	if (module->release == NULL) module->release = &AModuleReleaseNull;
	if (module->probe == NULL) module->probe = &AObjectProbeNull;

	module->class_entry.init();
	list_for_all_AModule(pos)
	{
		if (strcasecmp(pos->class_name, module->class_name) == 0) {
			list_add_tail(&module->class_entry, &pos->class_entry);
			break;
		}
	}
	module->global_index = InterlockedAdd(&g_index, 1);
	g_module.push_back(&module->global_entry);
	TRACE("%2d: %s(%s): object_size = %d.\n", module->global_index,
		module->module_name, module->class_name, module->object_size);

	// delay init() in AModuleInit()
	if (!g_inited)
		return 0;

	AOption *option = g_option->find(module->module_name);
	if ((option == NULL) && (g_option != NULL))
		option = AOptionFind3(g_option, module->class_name, module->module_name);

	int result = module->init(g_option, option, TRUE);
	if (result < 0) {
		TRACE("%s(%s) init() = %d.\n", module->module_name, module->class_name, result);
		module->exit(result);

		list_del_init(&module->global_entry);
		if (!list_empty(&module->class_entry))
			list_del_init(&module->class_entry);
	}
	return result;
}

#ifdef _WIN32
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

static LPTOP_LEVEL_EXCEPTION_FILTER next_except_filter;

static LONG WINAPI except_filter(EXCEPTION_POINTERS *except_ptr)
{
	TCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);

	TCHAR exe_name[64];
	TCHAR *tstr = _tcsrchr(path, _T('\\'));
	if (tstr == NULL) {
		return next_except_filter ? next_except_filter(except_ptr)
			: EXCEPTION_EXECUTE_HANDLER;
	}

	_tcscpy_s(exe_name, tstr+1);
	time_t cur_time = time(NULL);
	struct tm *cur_tm = localtime(&cur_time);
	_stprintf_s(tstr, path+_countof(path)-tstr,
		_T("\\logs\\%s_")_T(tm_sfmt)_T(".dmp"), exe_name, tm_args(cur_tm));

	HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
	if (hFile == INVALID_HANDLE_VALUE)
	{
		TRACE("create dump file failed...\n");
		return next_except_filter ? next_except_filter(except_ptr)
			: EXCEPTION_EXECUTE_HANDLER;
	}

	MINIDUMP_EXCEPTION_INFORMATION except_info;
	except_info.ExceptionPointers = except_ptr;
	except_info.ThreadId = GetCurrentThreadId();
	except_info.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hFile, MiniDumpWithDataSegs, &except_info, NULL, NULL);  
	CloseHandle(hFile);

	return next_except_filter ? next_except_filter(except_ptr)
		: EXCEPTION_EXECUTE_HANDLER;
}
#endif

AMODULE_API int
AModuleInit(AOption *option)
{
	reset_s(g_option, option, release_s);
	BOOL first = !g_inited;
	if (!g_inited) {
		g_inited = TRUE;
#ifdef _WIN32
		next_except_filter = SetUnhandledExceptionFilter(&except_filter);
#endif
	}

	list_for_all_AModule(module)
	{
		option = g_option->find(module->module_name);
		if ((option == NULL) && (g_option != NULL))
			option = AOptionFind3(g_option, module->class_name, module->module_name);

		int result = module->init(g_option, option, first);
		if (result < 0) {
			TRACE("%s(%s) reload = %d.\n", module->module_name, module->class_name, result);
		}
	}
	return 1;
}

AMODULE_API int
AModuleExit(void)
{
	while (!list_empty(&g_module)) {
		AModule *pos = list_pop_front(&g_module, AModule, global_entry);
		pos->exit(g_inited);

		if (pos->object_count != 0) {
			TRACE("%2d: %s(%s): left object_count = %d.\n", pos->global_index,
				pos->module_name, pos->class_name, pos->object_count);
		}
	}
	g_inited = FALSE;
	release_s(g_option);
	return 1;
}

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name)
{
	if ((class_name == NULL) && (module_name == NULL))
		return NULL;

	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;
		if ((module_name == NULL) || (strcasecmp(module_name, pos->module_name) == 0))
			return pos;

		if (class_name == NULL)
			continue;
		list_for_class_module(class_pos, pos)
		{
			if (strcasecmp(module_name, class_pos->module_name) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleNext(AModule *m)
{
	if (m == NULL) {
		if (g_module.empty())
			return NULL;
		return list_first_entry(&g_module, AModule, global_entry);
	}

	if (g_module.is_last(&m->global_entry))
		return NULL;
	return list_entry(m->global_entry.next, AModule, global_entry);
}

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param)
{
	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;

		int result = comp(param, pos);
		if (result > 0)
			return pos;
		if (result < 0)
			return NULL;

		if (class_name == NULL)
			continue;
		list_for_class_module(class_pos, pos)
		{
			result = comp(param, class_pos);
			if (result > 0)
				return class_pos;
			if (result < 0)
				return NULL;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *object, AObject *other, AMessage *msg)
{
	AModule *module = NULL;
	int score = 0;
	int ret;

	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;

		ret = pos->probe(object, other, msg);
		if (ret > score) {
			score = ret;
			module = pos;
		}

		if (class_name == NULL)
			continue;
		list_for_class_module(class_pos, pos)
		{
			ret = class_pos->probe(object, other, msg);
			if (ret > score) {
				score = ret;
				module = class_pos;
			}
		}
		break;
	}
	return module;
}

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AObjectCreate(AObject **object, ACreateParam *param)
{
	*object = NULL;
	if (param->module == NULL) {
		param->module = AModuleFind(param->class_name, param->module_name);
		if (param->module == NULL)
			return -EINVAL;
	}

	if (param->module->object_size > 0) {
		*object = (AObject*)malloc(param->module->object_size + param->ex_size);
		if (*object == NULL)
			return -ENOMEM;

		InterlockedAdd(&param->module->object_count, 1);
		(*object)->init(param->module, &AObjectFree);
	}

	int result = param->module->create(object, param->parent, param->option);
	if (result < 0)
		release_s(*object);
	return result;
}

AMODULE_API void
AObjectFree(AObject *object)
{
	AModule *m = object->_module;
	m->release(object);
	InterlockedAdd(&m->object_count, -1);

	TRACE2("%s(%s): free one, left object_count = %d.\n",
		m->module_name, m->class_name, m->object_count);
	free(object);
}

#ifdef _WIN32
#include <direct.h>
#ifdef _WIN64
	#define DLL_BIN_OS    "Win64"
#else
	#define DLL_BIN_OS    "Win32"
#endif
#define DLL_BIN_LIB   ""
#define DLL_BIN_NAME  "dll"

#define RTLD_NOW  0
static inline void*
dlopen(const char *filename, int flag) {
	return LoadLibraryExA(filename, NULL, flag);
}
#else //_WIN32
#include <dlfcn.h>
#if defined(__LP64__) && (__LP64__)
	#define DLL_BIN_OS    "linux64"
#else
	#define DLL_BIN_OS    "linux32"
#endif
#define DLL_BIN_LIB   "lib"
#define DLL_BIN_NAME  "so"
#endif //!_WIN32

static long volatile dlload_tid = 0;

AMODULE_API void*
dlload(const char *relative_path, const char *dll_name/*, BOOL relative_os_name*/)
{
	long cur_tid = gettid();
	long last_tid = 0;

	for (;;) {
		last_tid = InterlockedCompareExchange(&dlload_tid, cur_tid, 0);
		if ((last_tid == 0) || (last_tid == cur_tid))
			break;

		TRACE("%s: current(%d) wait other(%d) completed...\n",
			dll_name, cur_tid, last_tid);
		Sleep(10);
	}

	char cur_path[BUFSIZ];
	getcwd(cur_path, sizeof(cur_path));

	char abs_path[BUFSIZ];
	getexepath(abs_path, sizeof(abs_path));

	int len = strlen(abs_path);
	if (relative_path != NULL) {
		len += snprintf(abs_path+len, sizeof(abs_path)-len, "%s/", relative_path);
		chdir(abs_path);
	}
	snprintf(abs_path+len, sizeof(abs_path)-len, "%s%s.%s",
		DLL_BIN_LIB, dll_name, DLL_BIN_NAME);

	void *module = dlopen(abs_path, RTLD_NOW);
	if ((module == NULL) && (relative_path != NULL)) {
		module = dlopen(abs_path+len, RTLD_NOW);
		if (module == NULL) {
			TRACE("%s, dlopen(%s) failed.\n", dll_name, abs_path);
		}
	}

	if (relative_path != NULL) {
		chdir(cur_path);
	}
	if (last_tid == 0) {
		InterlockedExchange(&dlload_tid, 0);
	}
	return module;
}

AMODULE_API const char*
getexepath(char *path, int size)
{
	static char exe_path[BUFSIZ] = { 0 };
	if (exe_path[0] == '\0') {
#ifdef _WIN32
		DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
		const char *pos = strrchr(exe_path, '\\');
#else
		size_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path));
		const char *pos = strrchr(exe_path, '/');
#endif
		len = pos - exe_path + 1;
		exe_path[len] = '\0';
	}
	if (path != NULL) {
		strcpy_sz(path, size, exe_path);
	} else {
		path = exe_path;
	}
	return path;
}
