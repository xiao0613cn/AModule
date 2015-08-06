#ifndef _AMESSAGE_H_
#define _AMESSAGE_H_


enum AMsgType {
	AMsgType_Unknown = 0,
	AMsgType_Handle,   /* void*   (data = void*, size = 0) */
	AMsgType_Option,   /* AOption (data = AOption*, size = 0 */
	AMsgType_Object,   /* AObject (data = AObject*, size = 0 */
	AMsgType_Module,   /* AModule (data = AModule*, size = 0 */
	AMsgType_InOutMsg, /* contain_of(msg, AIOMsg, msg) */
	AMsgType_RefsMsg,  /* contain_of(msg, ARefMsg, msg) */
	AMsgType_Custom = 0x80000000,
};

typedef struct AMessage AMessage;
struct AMessage {
	long    type;
	char   *data;
	long    size;
	long  (*done)(AMessage *msg, long result);
	struct list_head entry;
};

// util function
static inline void
AMsgInit(AMessage *msg, long type, char *data, long size) {
	msg->type = type;
	msg->data = data;
	msg->size = size;
}

static inline long
AMsgDone(AMessage *msg, long result) {
	if (msg->done == NULL)
		return result;
	return msg->done(msg, result);
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


// extented struct AMessage
//////////////////////////////////////////////////////////////////////////
struct AIOMsg {
	AMessage  msg;
	long      type;
	char     *indata;
	long      insize;
	char     *outdata;
	long      outsize;
};
// AIOMsg::iomsg.type = AMsgType_InOutMsg
// AIOMsg::iomsg.data = AIOMsg*;
// AIOMsg::iomsg.size = 0;

static inline void
AIOMsgInit(AIOMsg *iom, long type, char *indata, long insize) {
	AMsgInit(&iom->msg, AMsgType_InOutMsg, (char*)iom, 0);
	iom->type = type;
	iom->indata = indata;
	iom->insize = insize;
	iom->outdata = NULL;
	iom->outsize = 0;
}

//////////////////////////////////////////////////////////////////////////
struct ARefsBuf {
	long    refs;
	long    size;
	void  (*free)(void*);
#pragma warning(disable:4200)
	char    data[0];
#pragma warning(default:4200)
};

struct ARefsMsg {
	AMessage   msg;
	ARefsBuf  *buf;
	long       pos;
	long       type;
	long       size;
#ifdef __cplusplus
	char*      data(void) { return this->buf->data + this->pos; }
#endif
};
// ARefsMsg::msg.type = AMsgType_RefsMsg
// ARefsMsg::msg.data = ARefsMsg::buf->data + ARefsMsg::pos;

static inline ARefsBuf*
ARefsBufCreate(long size) {
	ARefsBuf *buf = (ARefsBuf*)malloc(sizeof(ARefsBuf)+size);
	if (buf != NULL) {
		buf->refs = 1;
		buf->size = size;
		buf->free = &free;
	}
	return buf;
}

static inline long
ARefsBufAddRef(ARefsBuf *buf) {
	return InterlockedIncrement(&buf->refs);
}

static inline long
ARefsBufRelease(ARefsBuf *buf) {
	long result = InterlockedDecrement(&buf->refs);
	if (result <= 0)
		(buf->free)(buf);
	return result;
}

static inline void
ARefsMsgInit(ARefsMsg *rm, long type, ARefsBuf *buf, long offset, long size) {
	AMsgInit(&rm->msg, AMsgType_RefsMsg, (char*)rm, 0);
	rm->buf = buf; // ARefsBufAddRef(buf);
	rm->pos = offset;
	rm->type = type;
	rm->size = size;
}

//////////////////////////////////////////////////////////////////////////






#endif
