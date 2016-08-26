#ifndef _AMODULE_API_H_
#define _AMODULE_API_H_

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C     extern "C"
#else
#define EXTERN_C     extern
#endif
#endif

#if defined(AMODULE_API_EXPORTS)
#define AMODULE_API  EXTERN_C __declspec(dllexport)
#elif defined(_WIN32) && !defined(AMODULE_API_INNER)
#define AMODULE_API  EXTERN_C __declspec(dllimport)
#else
#define AMODULE_API  EXTERN_C
#endif

#pragma warning(disable: 4100) //未引用的形参
#pragma warning(disable: 4505) //未引用的本地函数已移除


//////////////////////////////////////////////////////////////////////////
// all api return value:
// > 0: completed with success
// = 0: pending with success, will callback msg->done()
// < 0: call failed, return error code

#ifndef _BASE_UTIL_H_
#include "base.h"
#endif

#ifndef _LIST_HEAD_H_
#include "list.h"
#endif

#ifndef _LINUX_RBTREE_H
#include "rbtree.h"
#endif

#ifndef _IOCP_UTIL_H_
#include "iocp_util.h"
#endif

#ifndef _AOPERATOR_H_
#include "AOperator.h"
#endif

#ifndef _AOPTION_H_
#include "AOption.h"
#endif

#ifndef _AMESSAGE_H_
#include "AMessage.h"
#endif

#ifndef _AMODULE_H_
#include "AModule.h"
#endif

#ifndef release_s
#define release_s(ptr, release, null) \
	if ((ptr) != null) { \
		release(ptr); \
		(ptr) = null; \
	} else { }
#endif


#ifdef __cplusplus
template <typename TObject, size_t offset_msg, size_t offset_from, int OnMsgDone(TObject*,int)>
int TObjectMsgDone(AMessage *msg, int result)
{
	TObject *p = (TObject*)((char*)msg - offset_msg);
	if (result >= 0)
		result = OnMsgDone(p, result);

	if (result != 0) {
		msg = *(AMessage**)((char*)p + offset_from);
		result = msg->done(msg, result);
	}
	return result;
}
#define TObjectDone(type, msg, from, done) \
	TObjectMsgDone<type, offsetof(type, msg), offsetof(type, from), done>

template <typename TObject, size_t offset_msg, size_t offset_from, int OnMsgDone(TObject*,int)>
int TObjectMsgDone2(AMessage *msg, int result)
{
	TObject *p = (TObject*)((char*)msg - offset_msg);
	result = OnMsgDone(p, result);

	if (result != 0) {
		msg = (AMessage*)((char*)msg - offset_from);
		result = msg->done(msg, result);
	}
	return result;
}
#define TObjectDone2(type, msg, from, done) \
	TObjectMsgDone2<type, offsetof(type, msg), offsetof(type, from), done>

template <typename TObject>
int TObjectNullOpen(AObject *object, AMessage *msg) {
	return -ENOSYS;
}

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
	int create(AObject *parent, AOption *option, const char *default_module) {
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
