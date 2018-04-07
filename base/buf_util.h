#ifndef _BUF_UTIL_H_
#define _BUF_UTIL_H_


template <typename Item>
struct ASlice {
	typedef ASlice<Item> Slice;

	long volatile _refs;
	int     _size;
	void  (*_free)(void*);
	int     _bgn;
	int     _end;
	Item    _data[0];

	static Slice* create(int count, void*(*alloc_func)(size_t)=NULL, void(*free_func)(void*)=NULL) {
		if (alloc_func == NULL) alloc_func = &malloc;
		if (free_func == NULL) free_func = &free;

		Slice *slice = (Slice*)alloc_func(sizeof(Slice) + sizeof(Item)*count);
		if (slice != NULL)
			slice->init(count, free_func);
		return slice;
	}
	void  init(int count, void(*free_func)(void*)) {
		_refs = 1; _size = count; _free = free_func; reset();
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
	void  rewind() {
		memmove(_data, ptr(), len()*sizeof(Item));
		_end = len(); _bgn = 0;
	}
	int   strfmt(const char *fmt, ...) {
		va_list ap; va_start(ap, fmt);
		int len = vsnprintf(next(), left(), fmt, ap);
		va_end(ap);
		if (len > 0) { push(len); }
		return len;
	}
	static int reserve(Slice *&slice, int left, int size) {
		if (slice == NULL) {
			slice = create(max(left, size));
			return (slice == NULL) ? -ENOMEM : 1;
		}
		if (slice->left() >= left)
			return 0;

		Slice *s2 = create(max(slice->len()+left, size));
		if (s2 == NULL)
			return -ENOMEM;

		s2->mempush(slice->ptr(), slice->len());
		slice->release();
		slice = s2;
		return 1;
	}
};

typedef ASlice<char> ARefsBuf;

template <typename Item = char>
struct ARefsBlock {
	ASlice<Item> *_buf;
	Item         *_ptr;
	int           _len;
	list_head     _node;

	void init() {
		_buf = NULL; _ptr = NULL; _len = 0; _node.init();
	}
	void set(ASlice<Item> *p, int bgn, int len) {
		addref_s(_buf, p); _len = len;
		_ptr = (p ? p->_data+bgn : NULL);
	}
	static ARefsBlock<Item>* first(list_head &list) {
		return list_first_entry(&list, ARefsBlock<Item>, _node);
	}
	ARefsBlock<Item>* next() {
		return list_entry(_node.next, ARefsBlock<Item>, _node);
	}
};


template<typename Item>
struct APool {
	struct Slice {
		list_head _node;
		ASlice<Item> _slice;
	};
	typedef APool<Item> Pool;

	list_head _slice_list;
	Slice    *_slice_push;
	Slice    *_slice_pop() { return _first(_slice_list); }
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
		Pool::_clear(_slice_list);
	}
	static void free_slice(void *p) {
		Slice *slice = (Slice*)((char*)p - offsetof(Slice, _slice));
		free(slice);
	}
	void reserve(int count) {
		while (count > _item_left) {
			Slice *slice = (Slice*)malloc(sizeof(Slice) + sizeof(Item)*_chunk_size);
			slice->_node.init();
			slice->_slice.init(_chunk_size, &free_slice);
			_join(slice);
		}
	}
	void _join(Slice *slice) {
		if (slice->_node.empty())
			_slice_list.push_back(&slice->_node);
		else
			_slice_list.move_back(&slice->_node);
		if ((_slice_push == NULL) && (slice->_slice.left() != 0))
			_slice_push = slice;
		_slice_count ++;
		_item_count += slice->_slice.len();
		_item_left += slice->_slice.left();
	}
	void join(Pool &other) {
		while (!other._slice_list.empty()) {
			Slice *slice = other._slice_pop();
			_join(slice);
		}
		other.init(other._chunk_size);
	}
	void reset() {
		if (_item_count != 0) {
			list_for_each2(slice, &_slice_list, Slice, _node) {
				_item_left += slice->_slice._bgn;
				slice->_slice.reset();
			}
			if (!_slice_list.empty())
				_slice_push = _slice_pop();
			_item_count = 0;
		}
		else if (_slice_push != NULL) {
			_item_left += _slice_push->_slice._bgn;
			_slice_push->_slice.reset();
			assert(_slice_push == _slice_pop());
		}
	}

	void push_back(const Item &item) { push(&item, 1); }
	void push(const Item *item, int count) {
		if (item == NULL) {
			_slice_push->_slice.push(count);
			_move_push();
			_item_count += count; _item_left -= count;
			return;
		}
		reserve(count);
		while (count > 0) {
			int num = min(count, _slice_push->_slice.left());
			_slice_push->_slice.mempush(item, num);
			_move_push();
			_item_count += num; _item_left -= num;
			item += num; count -= num;
		}
	}

	Item& front() {
		return *_slice_pop()->_slice.ptr();
	}
	void  pop_front(int count = 1, list_head *free_list = NULL) {
		Slice *slice = _slice_pop();
		assert(count <= slice->_slice.len());
		slice->_slice.pop(count);
		_item_count -= count;
		_move_pop(slice, free_list);
	}
//protected:
	void _move_push(bool force = false) {
		if (force) {
			_item_left -= _slice_push->_slice.left();
			if (_slice_push->_slice.len() == 0) {
				_slice_push->_node.leave();
				_slice_push->_slice.release();
				_slice_push = NULL;
				return;
			}
			_slice_push->_slice._size = _slice_push->_slice._end; // left() = 0
		}
		while (_slice_push->_slice.left() == 0) {
			if (_slice_list.is_last(&_slice_push->_node)) {
				_slice_push = NULL;
				return;
			}
			_slice_push = list_entry(_slice_push->_node.next, Slice, _node);
		}
	}
	void _move_pop(Slice *slice, list_head *free_list) {
		if (slice->_slice.len() == 0) {
			if (slice == _slice_push) {
				assert(_slice_push->_slice.left() != 0);
				return;
			}
			if (free_list == NULL) {
				slice->_node.leave();
				slice->_slice.release();
			} else {
				slice->_slice.reset();
				free_list->move_back(&slice->_node);
			}
			_slice_count --;
		}
	}
// helper
	static Slice* _first(list_head &list) {
		return list_first_entry(&list, Slice, _node);
	}
	static void _clear(list_head &list) {
		while (!list.empty()) {
			Slice *slice = list_pop_front(&list, Slice, _node);
			slice->_slice.release();
		}
	}
};

typedef APool<void*> APtrPool;

#endif
