#ifndef _AMESSAGE_H_
#define _AMESSAGE_H_


enum AMsgType {
	AMsgType_Unknown = 0,
	AMsgType_Handle,   /* void*   (data = void*,    size = 0) */
	AMsgType_Option,   /* AOption (data = AOption*, size = 0) */
	AMsgType_Object,   /* AObject (data = AObject*, size = 0) */
	AMsgType_Module,   /* AModule (data = AModule*, size = 0) */
	AMsgType_OtherMsg, /* AMessage (data = AMessage*, size = 0) */
	AMsgType_InOutMsg, /* AIOMsg  (data = AIOMsg*,  size = 0) */
	AMsgType_RefsMsg,  /* ARefMsg (data = ARefMsg*, size = 0) */
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
amsg_init(AMessage *msg, long type, char *data, long size) {
	msg->type = type;
	msg->data = data;
	msg->size = size;
}

static inline long
amsg_done(AMessage *msg, long result) {
	if (msg->done == NULL)
		return result;
	return msg->done(msg, result);
}

static inline void
amsg_copy(AMessage *msg, long type, char *data, long size) {
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
amsg_list_clear(struct list_head *head, long result) {
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
amsg_iom_init(AIOMsg *iom, long type, char *indata, long insize) {
	amsg_init(&iom->msg, AMsgType_InOutMsg, (char*)iom, 0);
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
arb_create(long size) {
	ARefsBuf *buf = (ARefsBuf*)malloc(sizeof(ARefsBuf)+size+4);
	if (buf != NULL) {
		buf->refs = 1;
		buf->size = size;
		buf->free = &free;
	}
	return buf;
}

static inline long
arb_addref(ARefsBuf *buf) {
	return InterlockedIncrement(&buf->refs);
}

static inline long
arb_release(ARefsBuf *buf) {
	long result = InterlockedDecrement(&buf->refs);
	if (result <= 0)
		(buf->free)(buf);
	return result;
}

static inline void
amsg_rm_init(ARefsMsg *rm, long type, ARefsBuf *buf, long offset, long size) {
	amsg_init(&rm->msg, AMsgType_RefsMsg, (char*)rm, 0);
	rm->buf = buf; // arb_addref(buf);
	rm->pos = offset;
	rm->type = type;
	rm->size = size;
}

//////////////////////////////////////////////////////////////////////////






#endif
