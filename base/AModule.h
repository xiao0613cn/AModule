#ifndef _AMODULE_H_
#define _AMODULE_H_


typedef struct AModule AModule;
typedef struct AObject AObject;

struct AObject {
	long volatile count;
	void  (*release)(AObject *object);
	void   *extend;
	AModule *module;
	long    reqix_count;

	long  (*open)(AObject *object, AMessage *msg);
	long  (*setopt)(AObject *object, AOption *option);
	long  (*getopt)(AObject *object, AOption *option);
	long  (*request)(AObject *object, long reqix, AMessage *msg);
	long  (*cancel)(AObject *object, long reqix, AMessage *msg);
	long  (*close)(AObject *object, AMessage *msg);
};

AMODULE_API void
AObjectInit(AObject *object, AModule *module);

AMODULE_API long
AObjectCreate(AObject **object, AObject *parent, AOption *option, const char *default_module);

static inline long
AObjectAddRef(AObject *object) {
	return InterlockedIncrement(&object->count);
}

static inline long
AObjectRelease(AObject *object) {
	long result = InterlockedDecrement(&object->count);
	if (result <= 0)
		object->release(object);
	return result;
}

static inline long
AObjectOpen(AObject *object, AMessage *msg) {
	if (object->open == NULL)
		return -ENOSYS;
	return object->open(object, msg);
}

static inline long
AObjectRequest(AObject *object, long reqix, AMessage *msg) {
	if (object->request == NULL)
		return -ENOSYS;
	return object->request(object, reqix, msg);
}

static inline long
AObjectCancel(AObject *object, long reqix, AMessage *msg) {
	if (object->cancel == NULL)
		return -ENOSYS;
	return object->cancel(object, reqix, msg);
}

static inline long
AObjectClose(AObject *object, AMessage *msg) {
	if (object->close == NULL)
		return -ENOSYS;
	return object->close(object, msg);
}


struct AModule {
	const char *class_name;
	const char *module_name;
	long        object_size;
	long  (*init)(AOption *option);
	void  (*exit)(void);
	long  (*create)(AObject **object, AObject *parent, AOption *option);
	void  (*release)(AObject *object);
	long  (*probe)(AObject *other, AMessage *msg);
	long    reqix_count;

	long  (*open)(AObject *object, AMessage *msg);
	long  (*setopt)(AObject *object, AOption *option);
	long  (*getopt)(AObject *object, AOption *option);
	long  (*request)(AObject *object, long reqix, AMessage *msg);
	long  (*cancel)(AObject *object, long reqix, AMessage *msg);
	long  (*close)(AObject *object, AMessage *msg);

	struct list_head global_entry;
	struct list_head class_list;
};

AMODULE_API long
AModuleRegister(AModule *module);

AMODULE_API long
AModuleInitAll(AOption *option);

AMODULE_API long
AModuleExitAll(void);

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name);

AMODULE_API AModule*
AModuleEnum(const char *class_name, long(*comp)(void*,AModule*), void *param);

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg);


#endif
