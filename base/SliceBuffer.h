#ifndef _SLICE_BUFFER_H_
#define _SLICE_BUFFER_H_


struct SliceBuffer {
	char *buf;
	int   siz;
	int   bgn;
	int   end;

#ifdef __cplusplus
	inline void  Init(void)  { this->buf = NULL; this->siz = 0; Reset(); }
	inline void  Reset(void) { this->bgn = this->end = 0; }

	inline char* CurrentPtr(void)  { return this->buf + this->bgn; }
	inline int   CurrentLen(void)  { return this->end - this->bgn; }

	inline char* ReservedPtr(void) { return this->buf + this->end; }
	inline int   ReservedLen(void) { return this->siz - this->end; }

	inline void  Push(int len) { this->end += len; }
	inline void  Pop(int len)  { this->bgn += len; if (CurrentLen() == 0) Reset(); }

	inline int   CurrentCapacity(void)  { return this->siz - this->bgn; }
	inline int   ResizeCapacity(int len)
	{
		if (len == 0) {
			delete [] this->buf;
			Init();
			return 1;
		}

		if (len < this->siz) {
			memmove(this->buf, CurrentPtr(), CurrentLen());
			this->end = CurrentLen();
			this->bgn = 0;
			return 0;
		}

		char *ptr = this->buf;
		len = ((len+7) & ~7);

		this->buf = new char[len];
		if (this->buf == NULL) {
			delete [] ptr;
			return -ERROR_OUTOFMEMORY;
		}

		this->siz = len;
		memcpy(this->buf, ptr+this->bgn, CurrentLen());
		delete [] ptr;

		this->end = CurrentLen();
		this->bgn = 0;
		return 1;
	}
#endif
};

static inline void SliceInit(SliceBuffer *sb) {
	sb->buf = NULL;
	sb->siz = 0;
	sb->bgn = 0;
	sb->end = 0;
}

static inline void SliceReset(SliceBuffer *sb) {
	sb->bgn = 0;
	sb->end = 0;
}

static inline char* SliceCurPtr(SliceBuffer *sb) {
	return sb->buf + sb->bgn;
}

static inline int SliceCurLen(SliceBuffer *sb) {
	return sb->end - sb->bgn;
}

static inline char* SliceResPtr(SliceBuffer *sb) {
	return sb->buf + sb->end;
}

static inline int SliceResLen(SliceBuffer *sb) {
	return sb->siz - sb->end;
}

static inline void SlicePush(SliceBuffer *sb, int len) {
	sb->end += len;
}

static inline int SlicePop(SliceBuffer *sb, int len) {
	sb->bgn += len;
	len = SliceCurLen(sb);
	if (len == 0)
		SliceReset(sb);
	return len;
}

static inline int SliceCapacity(SliceBuffer *sb) {
	return sb->siz - sb->bgn;
}

static inline int SliceResize(SliceBuffer *sb, int len, int slice) {
	len = (len/slice+1)*slice;
	if (len <= sb->siz) {
		if (sb->bgn == 0)
			return 0;

		memmove(sb->buf, SliceCurPtr(sb), SliceCurLen(sb));
		sb->end = SliceCurLen(sb);
		sb->bgn = 0;
		return 0;
	}

	char *ptr = sb->buf;
	len = ((len+7) & ~7);

	sb->buf = (char*)malloc(len);
	if (sb->buf == NULL) {
		sb->buf = ptr;
		return -ENOMEM;
	}

	sb->siz = len;
	memcpy(sb->buf, ptr+sb->bgn, SliceCurLen(sb));
	if (ptr != NULL)
		free(ptr);

	sb->end = SliceCurLen(sb);
	sb->bgn = 0;
	return 1;
}

static inline int SliceReserve(SliceBuffer *sb, int res_len, int slice) {
	if (SliceResLen(sb) >= res_len)
		return 0;

	res_len += SliceCurLen(sb);
	return SliceResize(sb, res_len, slice);
}

static inline void SliceFree(SliceBuffer *sb) {
	if (sb->buf != NULL)
		free(sb->buf);
	SliceInit(sb);
}

//////////////////////////////////////////////////////////////////////////
struct RTBuffer {
	long    refs;
	long    size;
	void  (*free)(void*);
#pragma warning(disable:4200)
	char    data[0];
#pragma warning(default:4200)
};

static RTBuffer* RTBufferAlloc(int size) {
	RTBuffer *buffer = (RTBuffer*)malloc(sizeof(RTBuffer)+size+sizeof(long));
	buffer->refs = 1;
	buffer->size = size;
	buffer->free = &free;
	return buffer;
}

static long RTBufferAddRef(RTBuffer *buffer) {
	return InterlockedAdd(&buffer->refs, 1);
}

static void RTBufferFree(RTBuffer *buffer) {
	long ret = InterlockedAdd(&buffer->refs, -1);
	if (ret <= 0)
		(buffer->free)(buffer);
}

//
static void RTMsgSet(AMessage *msg, RTBuffer *buffer, long offset) {
	msg->type = offset;
	msg->data = buffer->data + offset;
	msg->size = buffer->size - offset;
}

static RTBuffer* RTMsgGet(AMessage *msg) {
	return (RTBuffer*)(msg->data - msg->type - sizeof(RTBuffer));
}

#endif
