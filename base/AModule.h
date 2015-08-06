#ifndef _AMODULE_H_
#define _AMODULE_H_

#ifndef _LIST_HEAD_H_
#include "list.h"
#endif

#ifndef _AOPTION_H_
#include "AOption.h"
#endif

#ifndef _AMESSAGE_H_
#include "AMessage.h"
#endif

#ifndef release_s
#define release_s(ptr, release, null) \
	do { \
		if ((ptr) != null) { \
			release(ptr); \
			(ptr) = null; \
		} \
	} while (0)
#endif

//////////////////////////////////////////////////////////////////////////
// all api return value:
// > 0: completed with success
// = 0: pending with success, will callback msg->done()
// < 0: call failed, return error code
typedef struct AModule AModule;
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

extern void
AObjectInit(AObject *object, AModule *module);

extern long
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

//////////////////////////////////////////////////////////////////////////
typedef struct AModule AModule;
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

extern long
AModuleRegister(AModule *module);

extern long
AModuleInitAll(AOption *option);

extern long
AModuleExitAll(void);

extern AModule*
AModuleFind(const char *class_name, const char *module_name);

extern AModule*
AModuleEnum(const char *class_name, long(*comp)(void*,AModule*), void *param);

extern AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg);

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
struct IObject {
public:
	AObject *object;
	IObject(void)                  { init(NULL, false); }
	IObject(AObject *other)        { init(other, false); }
	IObject(const IObject &other)  { init(other.object, (other.object != NULL)); }
	~IObject(void)                 { release(); }

	void init(AObject *obj, bool ref) {
		this->object = obj;
		if (ref)
			AObjectAddRef(obj);
	}
	long addref(void) {
		return AObjectAddRef(this->object);
	}
	void release(void) {
		release_s(this->object, AObjectRelease, NULL);
	}
	long create(AObject *parent, AOption *option, const char *default_module) {
		release();
		return AObjectCreate(&this->object, parent, option, default_module);
	}

	IObject& operator=(AObject *other) {
		release();
		init(other, false);
		return *this;
	}
	IObject& operator=(const IObject &other) {
		release();
		init(other.object, (other.object != NULL));
		return *this;
	}

	AObject* operator->(void) { return this->object; }
	operator AObject* (void)  { return this->object; }
	operator AObject** (void) { return &this->object; }
	operator bool (void)      { return (this->object != NULL); }
};
#endif

#endif
