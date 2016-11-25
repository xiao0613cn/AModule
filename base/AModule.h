#ifndef _AMODULE_H_
#define _AMODULE_H_


typedef struct AModule AModule;
typedef struct AObject AObject;

struct AObject {
	long volatile refcount;
	void  (*release)(AObject *object);
	void   *extend;
	AModule *module;
	int     reqix_count;

	int   (*open)(AObject *object, AMessage *msg);
	int   (*setopt)(AObject *object, AOption *option);
	int   (*getopt)(AObject *object, AOption *option);
	int   (*request)(AObject *object, int reqix, AMessage *msg);
	int   (*cancel)(AObject *object, int reqix, AMessage *msg);
	int   (*close)(AObject *object, AMessage *msg);
};

AMODULE_API void
AObjectInit(AObject *object, AModule *module);

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


struct AModule {
	const char *class_name;
	const char *module_name;
	int         object_size;
	int   (*init)(AOption *option);
	void  (*exit)(void);
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

	struct list_head global_entry;
	struct list_head class_entry;
};

AMODULE_API int
AModuleRegister(AModule *module);

AMODULE_API int
AModuleInitOption(AOption *option);

AMODULE_API int
AModuleExit(void);

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name);

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param);

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg);


#endif
