#ifndef _AREFSBUF_H_
#define _AREFSBUF_H_


typedef struct ARefsBuf ARefsBuf;
struct ARefsBuf
{
	long    refs;
	int     size;
	void  (*free)(void*p);
	void   *user;
	int     bgn;
	int     end;
	char    data[0];

#ifdef __cplusplus
	long  addref() { return InterlockedAdd(&refs, 1); }
	long  release2() {
		long result = InterlockedAdd(&refs, -1);
		if (result <= 0)
			(this->free)(this);
		return result;
	}
	void  reset() { bgn = end = 0; }
	int   len() { return (end - bgn); }
	char* ptr() { return (data + bgn); }

	int   caps() { return (size - bgn); }
	int   left() { return (size - end); }
	char* next() { return (data + end); }

	void  pop(int len) { bgn += len; }
	void  push(int len) { end += len; }
	void  mempush(const void *p, int n) { memcpy(next(), p, n); push(n); }
	int   strpush(const char *str) {
		int len = 0;
		while (str[len] != '\0' && end != size) {
			data[end++] = str[len++];
		}
		return len;
	}
};

defer2(IRefsBuf, ARefsBuf*, if(_value)_value->release2());
#else
};
#endif

static inline ARefsBuf*
ARefsBufCreate(int size, void*(*alloc_func)(size_t), void(*free_func)(void*))
{
	if (alloc_func == NULL) alloc_func = &malloc;
	if (free_func == NULL) free_func = &free;

	ARefsBuf *buf = (ARefsBuf*)alloc_func(sizeof(ARefsBuf)+_align_8bytes(size));
	if (buf != NULL) {
		buf->refs = 1;
		buf->size = size;
		buf->free = free_func;
		buf->user = NULL;
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

static inline int
ARefsBufCheck(ARefsBuf *&buf, int left, int size,
              void*(*alloc_func)(size_t) = NULL, void(*free_func)(void*) = NULL)
{
	if (buf == NULL) {
		buf = ARefsBufCreate(max(left,size), alloc_func, free_func);
		return (buf == NULL) ? -ENOMEM : 1;
	}

	if (buf->left() >= left)
		return 0;

	ARefsBuf *b2 = ARefsBufCreate(max(buf->len()+left,size), alloc_func, free_func);
	if (b2 == NULL)
		return -ENOMEM;

	b2->mempush(buf->ptr(), buf->len());
	ARefsBufRelease(buf);
	buf = b2;
	return 1;
}

typedef struct ARefsChain
{
	ARefsBuf *buf;
	int     type;
	int     pos;
	int     len;
	struct list_head entry;

#ifdef __cplusplus
	void    init(int t, ARefsBuf *b, int p, int n) {
		buf = b; if (b != NULL) ARefsBufAddRef(b);
		type = t; pos = p; len = n;
	}
	char*   ptr() { return buf->data + pos; }
#endif
} ARefsChain;

static inline int
ARefsChainCheckBuf(ARefsChain *&chain, int left, int size, void*(*alloc_func)(size_t) = NULL, void(*free_func)(void*) = NULL)
{
	return 0;
}


#endif
