#ifndef _AREFSBUF_H_
#define _AREFSBUF_H_


typedef struct ARefsBuf ARefsBuf;
struct ARefsBuf {
	long    refs;
	int     size;
	void  (*free)(void*p);
	void   *user;
	int     bgn;
	int     end;
	char    data[1];

#ifdef __cplusplus
	long  addref() { return InterlockedAdd(&refs, 1); }
	long  release() {
		long result = InterlockedAdd(&refs, -1);
		if (result <= 0)
			(this->free)(this);
		return result;
	}
	void  reset() { bgn = end = 0; }
	int   caps() { return (size - bgn); }

	int   len() { return (end - bgn); }
	char* ptr() { return (data + bgn); }
	void  pop(int len) { bgn += len; }

	int   left() { return (size - end); }
	char* next() { return (data + end); }
	void  push(int len) { end += len; }

	void  mempush(const void *p, int n) { memcpy(next(), p, n); push(n); }
	int   strpush(const char *str) {
		int len = 0;
		while (str[len] != '\0' && end != size) {
			data[end++] = str[len++];
		}
		return len;
	}
#endif
};

static inline ARefsBuf*
ARefsBufCreate(int size, void*(*alloc_func)(size_t), void(*free_func)(void*))
{
	if (alloc_func == NULL) alloc_func = &malloc;
	if (free_func == NULL) free_func = &free;

	ARefsBuf *buf = (ARefsBuf*)alloc_func(sizeof(ARefsBuf) + size);
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
ARefsBufAddRef(ARefsBuf *buf) {
	return InterlockedAdd(&buf->refs, 1);
}

static inline long
ARefsBufRelease(ARefsBuf *buf) {
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


template <typename Item>
struct ASlice {
	typedef ASlice<Item> this_type;

	long    refs;
	int     size;
	void  (*free)(void*);
	void   *user;
	int     bgn;
	int     end;
	list_head node;
	Item    data[1];

	static this_type* create(int count) {
		this_type *slice = (this_type*)malloc(sizeof(this_type) + sizeof(Item)*(count-1));
		slice->refs = 1;
		slice->size = count;
		slice->free = &::free;
		slice->reset();
		slice->node.init();
		return slice;
	}
	long  addref() { return InterlockedAdd(&refs, 1); }
	long  release() {
		long result = InterlockedAdd(&refs, -1);
		if (result <= 0) (this->free)(this);
		return result;
	}
	void  reset() { bgn = end = 0; }
	int   len()   { return (end - bgn); }
	Item* ptr()   { return (data + bgn); }

	int   caps() { return (size - bgn); }
	int   left() { return (size - end); }
	Item* next() { return (data + end); }

	void  pop(int len) { bgn += len; }
	void  push(int len) { end += len; }
	void  mempush(const void *p, int n) { memcpy(next(), p, n); push(n); }
	/*int   strpush(const char *str) {
		int len = 0;
		while (str[len] != '\0' && end != size) {
			data[end++] = str[len++];
		}
		return len;
	}*/
};

template<typename Item>
struct APool {
	typedef ASlice<Item> Slice;

	list_head slice_list;
	Slice    *slice_push;
	int       slice_count;
	int       chunk_size;
	int       item_count;
	int       item_left;

	void init() {
		slice_list.init();
		slice_push = NULL;
		slice_count = 0;
		chunk_size = (sizeof(Item) > 16 ? 512 : 1024);
		item_count = 0;
		item_left = 0;
	}
	void reserve(int count) {
		while (count > item_left) {
			Slice *slice = Slice::create(chunk_size);
			_join(slice);
		}
	}
	void _join(Slice *slice) {
		if (slice->node.empty())
			slice_list.push_back(&slice->node);
		else
			slice_list.move_back(&slice->node);
		if ((slice_push == NULL) && (slice->left() != 0))
			slice_push = slice;
		slice_count ++;
		item_count += slice->len();
		item_left += slice->left();
	}
	void join(list_head &other) {
		while (!other.empty()) {
			Slice *slice = list_entry(other.next, Slice, node);
			_join(slice);
		}
	}

	int total_left() { return item_left; }
	int left() {
		if (slice_push == NULL) return 0;
		return slice_push->left();
	}
	Item* next() {
		return slice_push->next();
	}
	void push(int count) {
		slice_push->push(count);
		item_count += count;
		item_left -= count;
		_move_push();
	}
	void push_back(Item &item) {
		reserve(1);
		*next() = item;
		push(1);
	}
	int total_len() { return item_count; }
	int len() {
		if (slice_list.empty()) return 0;
		Slice *slice = list_entry(slice_list.next, Slice, node);
		return slice->len();
	}
	Item* ptr() {
		Slice *slice = list_entry(slice_list.next, Slice, node);
		return slice->ptr();
	}
	void pop(int count, list_head *free_list = NULL) {
		Slice *slice = list_entry(slice_list.next, Slice, node);
		assert(count <= slice->len());

		slice->pop(count);
		item_count -= count;
		_move_pop(slice, free_list);
	}
	void pop_front() {
		pop(1);
	}
	void reset() {
		list_for_each2(slice, &slice_list, Slice, node) {
			item_left += slice->len();
			slice->reset();
		}
		if (!slice_list.empty())
			slice_push = list_entry(slice_list.next, Slice, node);
		item_count = 0;
	}
	void quick_reset() {
		assert(item_count == 0);
		if (slice_push != NULL) {
			item_left += slice_push->bgn;
			slice_push->reset();
			slice_push = list_entry(slice_list.next, Slice, node);
		}
	}

	void _move_push() {
		while (slice_push->left() == 0) {
			if (slice_list.is_last(&slice_push->node)) {
				slice_push = NULL;
				return;
			}
			slice_push = list_entry(slice_push->node.next, Slice, node);
		}
	}
	void _move_pop(Slice *slice, list_head *free_list) {
		if (slice->len() == 0) {
			if (slice == slice_push) {
				if (slice->left() != 0)
					return;
				slice_push = NULL;
			}
			if (free_list == NULL) {
				slice->node.leave();
				slice->release();
			} else {
				slice->reset();
				free_list->move_back(&slice->node);
			}
			slice_count --;
		}
	}
};


#endif
