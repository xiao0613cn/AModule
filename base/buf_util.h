#ifndef _BUF_UTIL_H_
#define _BUF_UTIL_H_

/*
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
*/

template <typename Item>
struct ASlice {
	typedef ASlice<Item> Slice;

	long    _refs;
	int     _size;
	void  (*_free)(void*);
	void   *_user;
	int     _bgn;
	int     _end;
	list_head _node;
	Item    _data[1];

	static Slice* create(int count, void*(*alloc_func)(size_t)=NULL, void(*free_func)(void*)=NULL) {
		if (alloc_func == NULL) alloc_func = &malloc;
		if (free_func == NULL) free_func = &free;

		Slice *slice = (Slice*)alloc_func(sizeof(Slice) + sizeof(Item)*(count-1));
		if (slice != NULL) {
			slice->_refs = 1;
			slice->_size = count;
			slice->_free = free_func;
			slice->_user = NULL;
			slice->reset();
			slice->_node.init();
		}
		return slice;
	}
	long  addref() { return InterlockedAdd(&_refs, 1); }
	long  release() {
		long result = InterlockedAdd(&_refs, -1);
		if (result <= 0) (this->_free)(this);
		return result;
	}
	void  reset() { _bgn = _end = 0; }
	int   len()   { return (_end - _bgn); }
	Item* ptr()   { return (_data + _bgn); }

	int   caps()  { return (_size - _bgn); }
	int   left()  { return (_size - _end); }
	Item* next()  { return (_data + _end); }

	void  pop(int len) { _bgn += len; }
	void  push(int len) { _end += len; }
	void  mempush(const void *ptr, int count) {
		memcpy(next(), ptr, count*sizeof(Item));
		push(count);
	}
	static int reserve(Slice *&slice, int left, int size) {
		if (slice == NULL) {
			slice = create(max(left, size));
			return (slice == NULL) ? -ENOMEM : 1;
		}
		if (slice->left() > left)
			return 0;

		Slice *s2 = create(max(slice->len()+left, size));
		if (s2 == NULL)
			return -ENOMEM;

		s2->mempush(slice->ptr(), slice->len());
		slice->release();
		slice = s2;
		return 1;
	}
	static void _clear(list_head &list) {
		while (!list.empty()) {
			Slice *slice = list_pop_front(&list, Slice, _node);
			slice->release();
		}
	}
};

typedef ASlice<char> ARefsBuf;

template<typename Item>
struct APool {
	typedef ASlice<Item> Slice;
	typedef APool<Item> Pool;

	list_head _slice_list;
	Slice    *_slice_push;
	Slice    *_slice_pop() { return list_first_entry(&_slice_list, Slice, _node); }
	int       _slice_count;
	int       _chunk_size;
	int       _item_count;
	int       _item_left;

	void init(int chunk) {
		_slice_list.init();
		_slice_push = NULL;
		_slice_count = 0;
		_chunk_size = chunk ? chunk : (sizeof(Item) > 16 ? 128 : 512);
		_item_count = 0;
		_item_left = 0;
	}
	void exit() {
		Slice::_clear(_slice_list);
	}
	void reserve(int count) {
		while (count > _item_left) {
			Slice *slice = Slice::create(_chunk_size);
			_join(slice);
		}
	}
	void _join(Slice *slice) {
		if (slice->_node.empty())
			_slice_list.push_back(&slice->_node);
		else
			_slice_list.move_back(&slice->_node);
		if ((_slice_push == NULL) && (slice->left() != 0))
			_slice_push = slice;
		_slice_count ++;
		_item_count += slice->len();
		_item_left += slice->left();
	}
	void join(Pool &other) {
		while (!other._slice_list.empty()) {
			Slice *slice = list_first_entry(&other._slice_list, Slice, _node);
			_join(slice);
		}
		other.init(other._chunk_size);
	}
	void reset() {
		if (_item_count != 0) {
			list_for_each2(slice, &_slice_list, Slice, _node) {
				_item_left += slice->_bgn;
				slice->reset();
			}
			if (!_slice_list.empty())
				_slice_push = _slice_pop();
			_item_count = 0;
		}
		else if (_slice_push != NULL) {
			_item_left += _slice_push->_bgn;
			_slice_push->reset();
			assert(_slice_push == _slice_pop());
		}
	}

	int  total_left()          { return _item_left; }
	void push_back(Item &item) { push(&item, 1); }
	void push(Item *item, int count) {
		reserve(count);
		while (count > 0) {
			int num = min(count, _slice_push->left());
			_slice_push->mempush(item, num);
			_move_push();
			_item_count += num; _item_left -= num;
			item += num; count -= num;
		}
	}

	int   total_count() { return _item_count; }
	Item& front()       { return *_slice_pop()->ptr(); }
	void  pop_front(int count = 1, list_head *free_list = NULL) {
		Slice *slice = _slice_pop();
		assert(count <= slice->len());
		slice->pop(count);
		_item_count -= count;
		_move_pop(slice, free_list);
	}
//protected:
	void _move_push() {
		while (_slice_push->left() == 0) {
			if (_slice_list.is_last(&_slice_push->_node)) {
				_slice_push = NULL;
				return;
			}
			_slice_push = list_entry(_slice_push->_node.next, Slice, _node);
		}
	}
	void _move_pop(Slice *slice, list_head *free_list) {
		if (slice->len() == 0) {
			if (slice == _slice_push) {
				assert(_slice_push->left() != 0);
				return;
			}
			if (free_list == NULL) {
				slice->_node.leave();
				slice->release();
			} else {
				slice->reset();
				free_list->move_back(&slice->_node);
			}
			_slice_count --;
		}
	}
};


#endif
