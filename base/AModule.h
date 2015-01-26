#ifndef _INTERFACE_METHOD_H_
#define _INTERFACE_METHOD_H_

#ifndef _AOPTION_H_
#include "AOption.h"
#endif

typedef struct AMessage AMessage;
typedef struct AObject AObject;
typedef struct AModule AModule;

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
enum AMsgType {
	AMsgType_Unknown = 0,
	AMsgType_Option,
	AMsgType_Object,
	AMsgType_Module,
	AMsgType_Packet,
	AMsgType_Custom = 0x80000000,
};

struct AMessage {
	long    type;
	char   *data;
	long    size;
	long  (*done)(AMessage *msg, long result);
	struct list_head entry;
};

static inline void
AMsgInit(AMessage *msg, long type, char *data, long size) {
	msg->type = type;
	msg->data = data;
	msg->size = size;
}

static inline void
AMsgCopy(AMessage *msg, long type, char *data, long size) {
	msg->type = type;
	if ((msg->data == NULL) || (msg->size == 0)) {
		msg->data = data;
		msg->size = size;
	} else {
		if (msg->size > size)
			msg->size = size;
		memcpy(msg->data, data, msg->size);
	}
}

static inline void
MsgListClear(struct list_head *head, long result) {
	while (!list_empty(head)) {
		AMessage *msg = list_first_entry(head, AMessage, entry);
		list_del_init(&msg->entry);
		msg->done(msg, result);
	}
}

//////////////////////////////////////////////////////////////////////////
enum ARequestIndex {
	ARequest_Input = 0,
	ARequest_Output,
	ARequest_IndexMask = 0x00ffffff,

	ARequest_MsgLoop      = 0x01000000,
	ARequest_InQueueFront = 0x02000000,
	ANotify_InQueueFront  = 0x03000000,
	ANotify_InQueueBack   = 0x04000000,
};

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


struct AModule {
	const char *class_name;
	const char *module_name;
	long        object_size;
	long  (*init)(void);
	void  (*exit)(void);
	long  (*create)(AObject **object, AObject *parent, AOption *option);
	void  (*release)(AObject *object);
	long  (*probe)(AObject *other, const char *data, long size);
	long    reqix_count;

	long  (*open)(AObject *object, AMessage *msg);
	long  (*setopt)(AObject *object, AOption *option);
	long  (*getopt)(AObject *object, AOption *option);
	long  (*request)(AObject *object, long reqix, AMessage *msg);
	long  (*cancel)(AObject *object, long reqix, AMessage *msg);
	long  (*close)(AObject *object, AMessage *msg);

	struct list_head global_entry;
	struct list_head children_list;
	struct list_head brother_entry;
	struct AModule  *parent;
};

extern void
AModuleRegister(AModule *module);

extern long
AModuleInitAll(void);

extern AModule*
AModuleFind(const char *class_name, const char *module_name);

extern AModule*
AModuleEnum(long(*comp)(void*,AModule*), void *param);

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
struct IObject {
	AObject *object;
	void init(AObject *object, bool ref) { this->object = object; if (ref) AObjectAddRef(object); }
	long addref(void)                    { return AObjectAddRef(this->object); }
	void release(void)                   { release_s(this->object, AObjectRelease, NULL); }

	long create(AObject *parent, AOption *option, const char *default_module)
	{ release(); return AObjectCreate(&this->object, parent, option, default_module); }

	IObject(void)                  { init(NULL, false); }
	IObject(AObject *other)        { init(other, false); }
	IObject(const IObject &other)  { init(other.object, !!other.object); }
	~IObject(void)                 { release(); }

	IObject& operator=(AObject *other)       { release(); init(other, false); return *this; }
	IObject& operator=(const IObject &other) { release(); init(other.object, !!other.object); return *this; }

	AObject* operator->(void) { return this->object; }
	operator AObject* (void)  { return this->object; }
	operator AObject** (void) { return &this->object; }
	operator bool (void)      { return (this->object != NULL); }
};
#endif

#endif
