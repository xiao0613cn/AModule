#ifndef _AMODULE_H_
#define _AMODULE_H_

typedef struct AModule AModule;
typedef struct AObject AObject;

struct AModule {
	const char *class_name;
	const char *module_name;
	int         object_size;
	int   (*init)(AOption *global_option, AOption *module_option, BOOL first);
	void  (*exit)(int inited);

	int   (*create)(AObject **object, AObject *parent, AOption *option);
	void  (*release)(AObject *object);
	int   (*probe)(AObject *other, AMessage *msg, AOption *option);

	long      object_count;
	long      global_index;
	list_head global_entry;
	list_head class_entry;

	template <typename ASingleton>
	static ASingleton* singleton_data() {
		static ASingleton *s_m = (ASingleton*)(AModuleFind(
			ASingleton::name(), ASingleton::name()) + 1);
		return s_m;
	}
};

AMODULE_API int
AModuleRegister(AModule *module);

AMODULE_API int
AModuleInit(AOption *option);

AMODULE_API int
AModuleExit(void);

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name);

AMODULE_API AModule*
AModuleNext(AModule *m);

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param);

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg, AOption *option);

///////////////////////////////////////////////////////////////////////////////
struct AObject {
	long volatile _refcount;
	void        (*_release)(AObject *object);
	AModule      *_module;

#ifdef __cplusplus
	void init(AModule *m, void *release) {
		_refcount = 1;
		_release = (void(*)(AObject*))release;
		_module = m;
	}
	long addref() {
		return InterlockedAdd(&_refcount, 1);
	}
	long release() {
		long result = InterlockedAdd(&_refcount, -1);
		if (result <= 0)
			this->_release(this);
		return result;
	}
	template <typename AType>
	static int create(AType **object, AObject *parent, AOption *option, const char *default_module) {
		ACreateParam param(parent, option, NULL, NULL, default_module, 0);
		if (option == NULL) {
		} else if (option->value[0] != '\0') {
			param.class_name = option->name;
			param.module_name = option->value;
		} else if (option->name[0] != '\0') {
			param.module_name = option->name;
		}
		return AObjectCreate((AObject**)object, &param);
	}
	template <typename AType>
	static int create2(AType **object, AObject *parent, AOption *option, AModule *module) {
		ACreateParam param(parent, option, module, NULL, NULL, 0);
		return AObjectCreate((AObject**)object, &param);
	}
	template <typename AType>
	static int from(AType **object, AObject *parent, AOption *p_opt, const char *default_module) {
		AOption *o_opt = p_opt->find(AType::class_name());
		ACreateParam param(parent, o_opt, NULL, AType::class_name(),
			(o_opt && o_opt->value[0]) ? o_opt->value : default_module, 0);
		return AObjectCreate((AObject**)object, &param);
	}
#endif
};

typedef struct ACreateParam {
	AObject  *parent;
	AOption  *option;
	AModule  *module;
	const char *class_name;
	const char *module_name;
	int       ex_size;

#ifdef __cplusplus
	ACreateParam(AObject *p, AOption *o, AModule *m, const char *cn, const char *mn, int ex) {
		parent = p; option = o; module = m;
		class_name = cn; module_name = mn; ex_size = ex;
	}
#endif
} ACreateParam;

AMODULE_API int
AObjectCreate(AObject **object, ACreateParam *param);

AMODULE_API void
AObjectFree(AObject *object);



#endif
