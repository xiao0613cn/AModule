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
	int     type;
	char   *data;
	int     size;
	int   (*done)(AMessage *msg, int result);
	struct list_head entry;
};

// util function
static inline void
AMsgInit(AMessage *msg, int type, char *data, int size) {
	msg->type = type;
	msg->data = data;
	msg->size = size;
}

static inline int
AMsgDone(AMessage *msg, int result) {
	if (msg->done == NULL)
		return result;
	return msg->done(msg, result);
}

static inline void
AMsgCopy(AMessage *msg, int type, char *data, int size) {
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
AMsgListClear(struct list_head *head, int result) {
	while (!list_empty(head)) {
		AMessage *msg = list_first_entry(head, AMessage, entry);
		list_del_init(&msg->entry);
		msg->done(msg, result);
	}
}


// extented struct AMessage
//////////////////////////////////////////////////////////////////////////
struct AIOMsg {
	AMessage msg;
	int      type;
	char    *indata;
	int      insize;
	char    *outdata;
	int      outsize;
};
// AIOMsg::iomsg.type = AMsgType_InOutMsg
// AIOMsg::iomsg.data = AIOMsg*;
// AIOMsg::iomsg.size = 0;

static inline void
AIOMsgInit(AIOMsg *iom, int type, char *indata, int insize) {
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
	int     size;
	void  (*free)(void*);
#pragma warning(disable:4200)
	char    data[0];
#pragma warning(default:4200)
};

static inline ARefsBuf*
ARefsBufCreate(int size) {
	ARefsBuf *buf = (ARefsBuf*)malloc(sizeof(ARefsBuf)+size+4);
	if (buf != NULL) {
		buf->refs = 1;
		buf->size = size;
		buf->free = &free;
	}
	return buf;
}

static inline int
ARefsBufAddRef(ARefsBuf *buf) {
	return InterlockedIncrement(&buf->refs);
}

static inline int
ARefsBufRelease(ARefsBuf *buf) {
	int result = InterlockedDecrement(&buf->refs);
	if (result <= 0)
		(buf->free)(buf);
	return result;
}

// ARefsMsg::msg.type = AMsgType_RefsMsg
// ARefsMsg::msg.data = ARefsMsg::buf->data + ARefsMsg::pos;
struct ARefsMsg {
	AMessage msg;
	ARefsBuf *buf;
	int     pos;
	int     type;
	int     size;
};

static inline void
ARefsMsgInit(ARefsMsg *rm, int type, ARefsBuf *buf, int offset, int size) {
	AMsgInit(&rm->msg, AMsgType_RefsMsg, (char*)rm, 0);
	rm->buf = buf; // ARefsBufAddRef(buf);
	rm->pos = offset;
	rm->type = type;
	rm->size = size;
}

static inline int
ARefsMsgType(AMessage *msg) {
	assert(msg->type == AMsgType_RefsMsg);
	return ((ARefsMsg*)msg->data)->type;
}

static inline char*
ARefsMsgData(AMessage *msg) {
	assert(msg->type == AMsgType_RefsMsg);
	ARefsMsg *rm = (ARefsMsg*)msg->data;
	return (rm->buf->data + rm->pos);
}

static inline int
ARefsMsgSize(AMessage *msg) {
	assert(msg->type == AMsgType_RefsMsg);
	return ((ARefsMsg*)msg->data)->size;
}

//////////////////////////////////////////////////////////////////////////






#endif
