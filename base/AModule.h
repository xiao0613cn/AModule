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

	long volatile    object_count;
	long             global_index;
	struct list_head global_entry;
	struct list_head class_entry;
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
	template <typename TObject>
	static int create(TObject **object, AObject *parent, AOption *option, const char *default_module) {
		ACreateParam param(parent, option, NULL,
			(option && option->value[0]) ? option->name : NULL,
			(option && !option->value[0]) ? option->name : default_module, 0);
		return AObjectCreate((AObject**)object, &param);
	}
	template <typename TObject>
	static int create2(TObject **object, AObject *parent, AOption *option, AModule *module) {
		ACreateParam param(parent, option, module, NULL, NULL, 0);
		return AObjectCreate((AObject**)object, &param);
	}
	template <typename TObject>
	static int from(TObject **object, AObject *parent, AOption *p_opt, const char *def_mn) {
		AOption *o_opt = p_opt->find(TObject::class_name());
		ACreateParam param(parent, o_opt, NULL, TObject::class_name(),
			(o_opt && o_opt->value[0]) ? o_opt->value : def_mn, 0);
		return AObjectCreate((AObject**)object, &param);
	}
#endif
};

struct ACreateParam {
	AObject  *parent;
	AOption  *option;
	AModule  *module;
	const char *class_name;
	const char *module_name;
	int       ex_size;

	ACreateParam(AObject *p, AOption *o, AModule *m, const char *cn, const char *mn, int ex) {
		parent = p; option = o; module = m;
		class_name = cn; module_name = mn; ex_size = ex;
	}
};

AMODULE_API int
AObjectCreate(AObject **object, ACreateParam *p);

AMODULE_API void
AObjectFree(AObject *object);



#if 0
typedef enum AObject_Status {
	AObject_Invalid = 0,
	AObject_Opening,
	AObject_Opened,
	AObject_Abort,
	AObject_Closing,
	AObject_Closed,
} AObject_Status;

static inline const char*
StatusName(AObject_Status status)
{
	switch (status)
	{
	case AObject_Invalid: return "invalid";
	case AObject_Opening: return "opening";
	case AObject_Opened:  return "opened";
	case AObject_Abort:   return "abort";
	case AObject_Closing: return "closing";
	case AObject_Closed:  return "closed";
	default: return "unknown";
	}
}


enum ObjKV_Types {
	ObjKV_string = 0,
	ObjKV_char   = 1,
	ObjKV_int8   = 8*sizeof(int8_t),
	ObjKV_int16  = 8*sizeof(int16_t),
	ObjKV_int32  = 8*sizeof(int32_t),
	ObjKV_int64  = 8*sizeof(int64_t),
	ObjKV_object = 256,
	ObjKV_get,  // int get(AObject *obj, void *ptr, int len);
	ObjKV_set,  // int set(AObject *obj, const void *ptr, int len);
};

typedef struct ObjKV {
	const char *name;
	int         offset;
	ObjKV_Types type;
	int         size;
	union {
	int64_t     defnum;
	const char *defstr;
	};
} ObjKV;

#define ObjKV_T(type, member, kv, def) \
	{ #member, offsetof(type, member), kv, sizeof(((type*)0)->member), {(int64_t)def} },

#define ObjKV_S(type, member, defstr) \
	ObjKV_T(type, member, ObjKV_string, defstr)

#define ObjKV_N(type, member, defnum) \
	ObjKV_T(type, member, (ObjKV_Types)(8*sizeof(((type*)0)->member)), defnum)

#define ObjKV_O(type, member, defstr) \
	ObjKV_T(type, member, ObjKV_object, defstr)

AMODULE_API int
AObjectSetKVOpt(AObject *object, const ObjKV *kv, AOption *opt);

AMODULE_API int
AObjectSetKVMap(AObject *object, const ObjKV *kv_map, AOption *option, BOOL skip_unfound);

AMODULE_API int
AObjectSetOpt(AObject *object, AOption *opt, const ObjKV *kv_map);
#endif

#endif
