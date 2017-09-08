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
	int   (*probe)(AObject *other, AMessage *msg);
	int     reqix_count;

	int   (*open)(AObject *object, AMessage *msg);
	int   (*setopt)(AObject *object, AOption *option);
	int   (*getopt)(AObject *object, AOption *option);
	int   (*request)(AObject *object, int reqix, AMessage *msg);
	int   (*cancel)(AObject *object, int reqix, AMessage *msg);
	int   (*close)(AObject *object, AMessage *msg);

	struct ObjKV *kv_map;
	struct list_head global_entry;
	long             global_index;
	struct list_head class_entry;
	long             class_index;
	long             class_count;
};

struct AObject {
	long volatile refcount;
	void        (*release)(AObject *object);
	AModule      *module;

#ifdef __cplusplus
	void  init(AModule *m) {
		refcount = 1;
		release = m ? m->release : NULL;
		module = m;
	}
	long  addref() {
		return InterlockedAdd(&refcount, 1);
	}
	long  release2() {
		long result = InterlockedAdd(&refcount, -1);
		if (result <= 0)
			this->release(this);
		return result;
	}

	long  open(AMessage *msg)               { return module->open(this, msg); }
	long  getopt(AOption *opt)              { return module->getopt(this, opt); }
	long  setopt(AOption *opt)              { return module->setopt(this, opt); }
	long  request(int reqix, AMessage *msg) { return module->request(this, reqix, msg); }
	long  cancel(int reqix, AMessage *msg)  { return module->cancel(this, reqix, msg); }
	long  close(AMessage *msg)              { return module->close(this, msg); }
};

defer2(IObject, AObject*, if (_value)_value->release2());
#else
};
#endif

AMODULE_API int
AObjectCreate(AObject **object, AObject *parent, AOption *option, const char *default_module);

AMODULE_API int
AObjectCreate2(AObject **object, AObject *parent, AOption *option, AModule *module);

static inline long
AObjectAddRef(AObject *object) {
	return InterlockedAdd(&object->refcount, 1);
}

static inline long
AObjectRelease(AObject *object) {
	long result = InterlockedAdd(&object->refcount, -1);
	if (result <= 0)
		object->release(object);
	return result;
}

AMODULE_API void
AObjectFree(AObject *object);



AMODULE_API int
AModuleRegister(AModule *module);

AMODULE_API int
AModuleInit(AOption *option);

AMODULE_API int
AModuleExit(void);

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name);

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param);

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg);

#ifdef __cplusplus
template <AModule &module>
struct auto_reg_t {
	auto_reg_t() { AModuleRegister(&module); }
};
#endif //__cplusplus

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
