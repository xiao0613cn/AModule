#pragma once

struct srsw_count {
	size_t  add_count;
	size_t  del_count;

#ifdef __cplusplus
	srsw_count(void) { reset(); }
	~srsw_count(void) { }

	void reset(void) { add_count = del_count = 0; }
	operator size_t(void) { return (add_count - del_count); }
	void operator+=(size_t n) { add_count += n; }
	void operator-=(size_t n) { del_count += n; }
#endif
};

static inline void srsw_reset(srsw_count *n) {
	n->add_count = 0;
	n->del_count = 0;
}

static inline size_t srsw_get(srsw_count *n) {
	return (n->add_count - n->del_count);
}

static inline size_t srsw_add(srsw_count *n, size_t num) {
	n->add_count += num;
}

static inline void srsw_del(srsw_count *n, size_t num) {
	n->del_count += num;
}

template <typename item_t, size_t capacity>
class srsw_queue {
	size_t  put_index;
	size_t  get_index;
	srsw_count  item_count;
	item_t  item_array[capacity];
public:
	srsw_queue(void) { reset(); }
	~srsw_queue(void) { }

	void reset(void)
	{
		item_count.reset();
		put_index = 0;
		get_index = 0;
	}

	size_t _capacity(void)
	{
		return capacity;
	}
	size_t size(void)
	{
		return item_count;
	}

	item_t& end(void)
	{
		return item_array[put_index];
	}
	void put_end(void)
	{
		if (put_index == capacity-1)
			put_index = 0;
		else
			++put_index;
		item_count += 1;
	}
	void put_back(const item_t &i)
	{
		end() = i;
		put_end();
	}

	item_t& front(void)
	{
		return item_array[get_index];
	}
	void pop_front(void)
	{
		if (get_index == capacity-1)
			get_index = 0;
		else
			++get_index;
		item_count -= 1;
	}
	void get_front(item_t &i)
	{
		i = front();
		pop_front();
	}
};

template <typename byte_t>
class srsw_buffer_t {
	byte_t *buf_data;
	size_t  buf_size;

	size_t  write_siz;
	size_t  read_siz;
	size_t  write_pos;
	size_t  read_pos;
public:
	srsw_buffer_t(void)
	{
		buf_data = NULL;
		buf_size = 0;
		reset(NULL, 0);
	}
	~srsw_buffer_t(void) { }

	byte_t* reset(byte_t *ptr, size_t len)
	{
		byte_t *old_ptr = buf_data;
		if (ptr != NULL) {
			buf_data = ptr;
			buf_size = len;
		}

		write_siz = read_siz = 0;
		write_pos = read_pos = 0;
		return old_ptr;
	}

	size_t capacity(void)
	{
		return buf_size;
	}
	size_t size(void)
	{
		return write_siz - read_siz;
	}

	size_t write(byte_t *ptr, size_t len)
	{
		size_t tmp = buf_size - size();
		if (len > tmp)
			len = tmp;
		if (len == 0)
			return 0;

		// write in a single step
		if (write_pos+len <= buf_size) {
			memcpy(buf_data + write_pos, ptr, len);
			write_pos += len;

			if (write_pos == buf_size)
				write_pos = 0;
		}
		// write in two steps
		else {
			tmp = buf_size - write_pos;
			memcpy(buf_data + write_pos, ptr, tmp);

			write_pos = len - tmp;
			memcpy(buf_data, ptr + tmp, write_pos);
		}

		write_siz += len;
		return len;
	}

	size_t read(byte_t *ptr, size_t len)
	{
		size_t tmp = size();
		if (len > tmp)
			len = tmp;
		if (len == 0)
			return 0;

		// read in a single step
		if (read_pos+len <= buf_size) {
			memcpy(ptr, buf_data + read_pos, len);
			read_pos += len;

			if (read_pos == buf_size)
				read_pos = 0;
		}
		// read in two steps
		else {
			tmp = buf_size - read_pos;
			memcpy(ptr, buf_data + read_pos, tmp);

			read_pos = len - tmp;
			memcpy(ptr + tmp, buf_data, read_pos);
		}

		read_siz += len;
		return len;
	}

	// zero copy, next write pointer
	void reserve(byte_t *ptr[2], size_t len[2])
	{
		size_t tmp = buf_size - size();

		// reserve a single buffer
		if (write_pos+tmp <= buf_size) {
			ptr[0] = buf_data + write_pos;
			len[0] = tmp;

			ptr[1] = NULL;
			len[1] = 0;
		}
		// reserve two buffers
		else {
			ptr[0] = buf_data + write_pos;
			len[0] = buf_size - write_pos;

			ptr[1] = buf_data;
			len[1] = tmp - len[0];
		}
	}

	// zero copy, commit write data
	void commit(byte_t *ptr, size_t len)
	{
		if (ptr >= buf_data+write_pos) {
			len = (ptr + len) - (buf_data + write_pos);
			write_pos += len;
			write_siz += len;

			if (write_pos == buf_size)
				write_pos = 0;
		} else {
			// commit dummy data
			write_siz += buf_size - write_pos;

			write_pos = (ptr + len) - buf_data;
			write_siz += write_pos;
		}
	}

	void decommit(byte_t *ptr, size_t len)
	{
		if (ptr >= buf_data+read_pos) {
			len = (ptr + len) - (buf_data + read_pos);
			read_pos += len;
			read_siz += len;

			if (read_pos == buf_size)
				read_pos = 0;
		} else {
			// decommit dummy data
			read_siz += buf_size - read_pos;

			read_pos = (ptr + len) - buf_data;
			read_siz += read_pos;
		}
	}
};
typedef srsw_buffer_t<char> srsw_buffer;
