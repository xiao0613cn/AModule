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
	AMsgType_RefsMsg,  /* data = ARefMsg*, size = 0 */
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
};

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


// extented struct AMessage
//////////////////////////////////////////////////////////////////////////
// AIOMsg::msg.type = AMsgType_InOutMsg
typedef struct AInOutMsg AInOutMsg;
struct AInOutMsg
{
	AMessage msg;
	int     intype;
	int     insize;
	char   *indata;
	int     outtype;
	int     outsize;
	char   *outdata;
};

static inline void
AIOMsgInit(AInOutMsg *iom, int type, char *indata, int insize)
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
typedef struct ARefsBuf ARefsBuf;
struct ARefsBuf
{
	long    refs;
	int     size;
	void  (*free)(void*);
	int     bgn;
	int     end;
#pragma warning(disable:4200)
	char    data[0];
#pragma warning(default:4200)
#ifdef __cplusplus
	int   push(int len) { end += len; }
	void  pop(int len) { bgn += len; if (bgn == end) bgn = end = 0; }
	int   len() { return (end - bgn); }
	char* ptr() { return (data + bgn); }

	int   caps() { return (size - bgn); }
	int   left() { return (size - end); }
	char* next() { return (data + end); }
#endif
};

static inline ARefsBuf*
ARefsBufCreate(int size, void*(*alloc_func)(size_t), void(*free_func)(void*))
{
	ARefsBuf *buf;
	if (alloc_func == NULL) alloc_func = &malloc;
	if (free_func == NULL) free_func = &free;

	buf = (ARefsBuf*)alloc_func(sizeof(ARefsBuf)+_align_8bytes(size));
	if (buf != NULL) {
		buf->refs = 1;
		buf->size = size;
		buf->free = free_func;
		buf->bgn = 0;
		buf->end = 0;
	}
	return buf;
}

static inline long
ARefsBufAddRef(ARefsBuf *buf)
{
	return InterlockedAdd(&buf->refs, 1);
}

static inline long
ARefsBufRelease(ARefsBuf *buf)
{
	long result = InterlockedAdd(&buf->refs, -1);
	if (result <= 0)
		(buf->free)(buf);
	return result;
}

// ARefsMsg::msg.type = AMsgType_RefsMsg
typedef struct ARefsMsg ARefsMsg;
struct ARefsMsg
{
	AMessage msg;
	ARefsBuf *buf;
	int     pos;
	int     type;
	int     size;
};

static inline void
ARefsMsgInit(ARefsMsg *rm, int type, ARefsBuf *buf, int offset, int size)
{
	AMsgInit(&rm->msg, AMsgType_RefsMsg, rm, 0);
	rm->buf = buf; // ARefsBufAddRef(buf);
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
