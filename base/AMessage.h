#ifndef _AMESSAGE_H_
#define _AMESSAGE_H_


enum AMsgTypes
{
	AMsgType_Unknown = 0,
	AMsgType_Handle,   /* data = void*,    size = 0 */
	AMsgType_Option,   /* data = AOption*, size = 0 */
	AMsgType_Object,   /* data = AObject*, size = 0 */
	AMsgType_Module,   /* data = AModule*, size = 0 */
	AMsgType_OtherMsg, /* data = AMessage*, size = 0 */
	AMsgType_InOutMsg, /* data = AInOutMsg*, size = 0 */
	AMsgType_RefsMsg,  /* data = ARefsMsg*, size = 0 */
	AMsgType_RefsChain,/* data = ARefsChain*, size = 0 */
	AMsgType_Class   = 0x20000000, /* class defined */
	AMsgType_Private = 0x40000000, /* module defined */
};

typedef struct AMessage AMessage;
struct AMessage
{
	int     type;
	int     size;
	char   *data;
	int   (*done)(AMessage *msg, int result);
	struct list_head entry;

#ifdef __cplusplus
	void  init(int t = 0, const void *p = 0, int n = 0) { type = t; size = n; data = (char*)p; }
	void  init(HANDLE handle)          { init(AMsgType_Handle, handle, 0); }
	void  init(struct AOption *option) { init(AMsgType_Option, option, 0); }
	void  init(struct AObject *object) { init(AMsgType_Object, object, 0); }
	void  init(struct AModule *module) { init(AMsgType_Module, module, 0); }
	void  init(AMessage *msg)          { init(msg->type, msg->data, msg->size); }
	void  init(AMessage &msg)          { init(msg.type, msg.data, msg.size); }
	int   done2(int result)            { return done(this, result); }
};
template <typename AType, size_t offset, int(AType::*run)(int)>
int MsgDoneT(AMessage *msg, int result) {
	AType *p = (AType*)((char*)msg - offset);
	return (p->*run)(result);
}
#define MsgDone(type, member, run) \
	MsgDoneT<type, offsetof(type,member), &type::run>
#else
};
#endif

// util function
static inline void
AMsgInit(AMessage *msg, int type, const void *data, int size)
{
	msg->type = type;
	msg->data = (char*)data;
	msg->size = size;
}

static inline void
AMsgCopy(AMessage *msg, int type, const void *data, int size)
{
	msg->type = type;
	if ((msg->data == NULL) || (msg->size == 0)) {
		msg->data = (char*)data;
		msg->size = size;
	} else {
		if (msg->size > size)
			msg->size = size;
		memcpy(msg->data, data, msg->size);
	}
}

static inline void
AMsgListClear(struct list_head *head, int result)
{
	while (!list_empty(head)) {
		AMessage *msg = list_first_entry(head, AMessage, entry);
		list_del_init(&msg->entry);
		msg->done(msg, result);
	}
}

static inline int
AMsgListPush(pthread_mutex_t *mutex, struct list_head *list, AMessage *msg)
{
	pthread_mutex_lock(mutex);
	int first = list_empty(list);
	list_add_tail(&msg->entry, list);
	pthread_mutex_unlock(mutex);
	return first;
}

AMODULE_API AMessage*
AMsgDispatch(struct list_head *notify_list, AMessage *from, struct list_head *quit_list);

AMODULE_API int
AMsgDispatch2(pthread_mutex_t *mutex, struct list_head *notify_list, AMessage *from);

// extented struct AMessage
//////////////////////////////////////////////////////////////////////////
// AIOMsg::msg.type = AMsgType_InOutMsg
typedef struct AInOutMsg
{
	AMessage msg;
	int     intype;
	int     insize;
	char   *indata;
	int     outtype;
	int     outsize;
	char   *outdata;
} AInOutMsg;

static inline void
AInOutMsgInit(AInOutMsg *iom, int type, char *indata, int insize)
{
	AMsgInit(&iom->msg, AMsgType_InOutMsg, iom, 0);
	iom->intype = type;
	iom->insize = insize;
	iom->indata = indata;
	iom->outtype = AMsgType_Unknown;
	iom->outsize = 0;
	iom->outdata = NULL;
}

//////////////////////////////////////////////////////////////////////////
#ifndef _AREFSBUF_H_
#include "ARefsBuf.h"
#endif
// ARefsMsg::msg.type = AMsgType_RefsMsg
typedef struct ARefsMsg
{
	AMessage msg;
	ARefsBuf *buf;
	int     pos;
	int     type;
	int     size;
#ifdef __cplusplus
	char*   ptr() { return (buf->data + pos); }
#endif
} ARefsMsg;

static inline void
ARefsMsgInit(ARefsMsg *rm, int type, ARefsBuf *buf, int offset, int size)
{
	AMsgInit(&rm->msg, AMsgType_RefsMsg, rm, 0);
	if (rm->buf != NULL)
		ARefsBufRelease(rm->buf);

	rm->buf = buf;
	if (buf != NULL)
		ARefsBufAddRef(buf);

	rm->pos = offset;
	rm->type = type;
	rm->size = size;
}

//////////////////////////////////////////////////////////////////////////
static inline int
AMsgData(AMessage *msg, int *type_p, int *size_p, char **data_p)
{
	int   type = msg->type;
	int   size = msg->size;
	char *data = msg->data;
_retry:
	switch (type)
	{
	case AMsgType_OtherMsg:
		assert(size == 0);
		type = ((AMessage*)data)->type;
		size = ((AMessage*)data)->size;
		data = ((AMessage*)data)->data;
		goto _retry;

	case AMsgType_InOutMsg:
		assert(size == 0);
		type = ((AInOutMsg*)data)->intype;
		size = ((AInOutMsg*)data)->insize;
		data = ((AInOutMsg*)data)->indata;
		goto _retry;

	case AMsgType_RefsMsg:
		assert(size == 0);
		type = ((ARefsMsg*)data)->type;
		size = ((ARefsMsg*)data)->size;
		data = ((ARefsMsg*)data)->buf->data + ((ARefsMsg*)data)->pos;
		goto _retry;

	default:
		if (type_p != NULL) *type_p = type;
		if (size_p != NULL) *size_p = size;
		if (data_p != NULL) *data_p = data;
		return type;
	}
}


#endif
