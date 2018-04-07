#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "AModule_API.h"

static struct {
	BOOL            _inited;
	struct rb_root  _file_map;
	pthread_mutex_t _mutex;
	//char            _path[BUFSIZ];
	int             _max_count;
	int             _max_fsize;
	int             _delay_first;
	int             _delay_tick;
	int             _delay_count;
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }
} Log = { 0 };

static int LogInit()
{
	if (Log._inited) return 1;
	Log._inited = TRUE;

	INIT_RB_ROOT(&Log._file_map);
	pthread_mutex_init(&Log._mutex, NULL);
	//snprintf(Log._path, sizeof(Log._path), "%slogs/" getexepath(NULL,0)));
	Log._max_count = 3;
	Log._max_fsize = 4*1024*1024; // 4 MB
	Log._delay_first = 100;       // 100 milli-sec
	Log._delay_tick = 10*1000;    // 10 seconds
	Log._delay_count = 3;
	return 0;
}

struct LogFile : public AObject {
	rb_node     _node;
	AOperator   _asop;
	char        _name[32];
	int         _fd;
	int         _fsize;
	int         _index;
	APool<char> _buf;
	unsigned    _delay_close;
};

static int LogFileCmp(const char *key, LogFile *data) {
	return strcasecmp(key, data->_name);
}
rb_tree_define(LogFile,	_node, const char*, LogFileCmp)

static int LogFileOpen(LogFile *lf)
{
	char fpath[BUFSIZ];
	snprintf(fpath, BUFSIZ, "%slogs/%s-%d.log", getexepath(NULL,0), lf->_name, lf->_index);

	lf->_fd = open(fpath, O_RDWR|O_CREAT|O_APPEND|O_BINARY, S_IREAD|S_IWRITE);
	if (lf->_fd == -1)
		return -errno;

	lseek(lf->_fd, 0, SEEK_END);
	lf->_fsize = tell(lf->_fd);

	if (lf->_fsize >= Log._max_fsize) {
		close(lf->_fd);
		remove(fpath);

		lf->_fd = open(fpath, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
		if (lf->_fd == -1)
			return -errno;
		lf->_fsize = 0;
	}
	return 1;
}

static int LogFileWrite(LogFile *lf)
{
	int total_len = 0;
	do {
		ASlice<char> *slice = NULL;
		const char *ptr = NULL;
		int len = 0;

		Log.lock();
		if (lf->_buf._item_count != 0) {
			slice = &lf->_buf._slice_pop()->_slice;
			slice->addref();
			ptr = slice->ptr();
			len = slice->len();

			lf->_buf.pop_front(len);
			lf->_delay_close = 0;
		} else {
			++lf->_delay_close;
		}
		Log.unlock();

		if (slice == NULL) {
			break;
		}
		if (len == 0) {
			slice->release();
			continue;
		}
		if (lf->_fd == -1) {
			int result = LogFileOpen(lf);
			if (result < 0) {
				slice->release();
				return result;
			}
		}
		write(lf->_fd, ptr, len);
		slice->release();
		total_len += len;

		lf->_fsize += len;
		if (lf->_fsize >= Log._max_fsize) {
			close(lf->_fd);
			lf->_fd = -1;
			if (++lf->_index >= Log._max_count)
				lf->_index = 0;
		}
	} while (lf->_buf._item_count != 0);
	return total_len;
}

static int LogFileAsopDone(AOperator *asop, int result)
{
	LogFile *lf = container_of(asop, LogFile, _asop);
	LogFileWrite(lf);

	if (lf->_delay_close > Log._delay_count) {
		reset_nif(lf->_fd, -1, close(lf->_fd));
	}
	if (result >= 0) {
		asop->delay(NULL, Log._delay_tick, FALSE);
	} else {
		Log.lock();
		rb_erase(&lf->_node, &Log._file_map);
		Log.unlock();
		lf->release();
	}
	return 0;
}

static void LogFileRelease(AObject *object)
{
	LogFile *lf = (LogFile*)object;
	reset_nif(lf->_fd, -1, close(lf->_fd));
	lf->_buf.exit();
	free(lf);
}

static LogFile* LogFileCreate(const char *name)
{
	LogFile *lf = gomake(LogFile);
	lf->init(NULL, &LogFileRelease);

	strcpy_sz(lf->_name, name);
	lf->_fd = -1;
	lf->_fsize = 0;
	lf->_index = 0;
	lf->_asop.timer();
	lf->_buf.init(4*1024);
	lf->_delay_close = 0;

	rb_insert_LogFile(&Log._file_map, lf, lf->_name);
	return lf;
}

AMODULE_API int
ALog(const char *name, const char *func, int line, const char *fmt, va_list ap)
{
	if (!Log._inited) LogInit();
	if (name == NULL) name = "AModule";
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	Log.lock();
	LogFile *lf = rb_find_LogFile(&Log._file_map, name);
	if (lf == NULL) {
		lf = LogFileCreate(name);
	}
	if ((lf->_buf._slice_push == NULL) || (lf->_buf._slice_push->_slice.left() < 100)) {
		if (lf->_buf._slice_push != NULL)
			lf->_buf._move_push(true);
		lf->_buf.reserve(lf->_buf._chunk_size);
	}

	ASlice<char> *s = &lf->_buf._slice_push->_slice;
	char *ptr = s->next();
	int len = snprintf(s->next(), min(BUFSIZ,s->left()),
		tm_fmt" %4d | %s():\t", tm_args(tm), line, func);
	if (len < 0) {
		assert(0);
		Log.unlock();
		return len;
	}
	lf->_buf.push(NULL, len);

	len = vsnprintf(s->next(), min(BUFSIZ,s->left()), fmt, ap);
	if (len < 0) {
		if (s->left() < BUFSIZ) {
			lf->_buf._move_push(true);
			lf->_buf.reserve(lf->_buf._chunk_size);

			s = &lf->_buf._slice_push->_slice;
			ptr = s->next();
			len = vsnprintf(s->next(), min(BUFSIZ,s->left()), fmt, ap);
		}
		if (len < 0) {
			len = min(BUFSIZ,s->left()) - 1;
			s->next()[len] = '\0';
		}
	}
	lf->_buf.push(NULL, len);

	s->addref();
	lf->addref();
	Log.unlock();

#ifdef _WIN32
	OutputDebugStringA(ptr);
#elif defined(ANDROID)
	__android_log_write(ANDROID_LOG_DEBUG, name, ptr);
#endif
	fputs(ptr, stdout);
	s->release();

	if (AThreadDefault(0) == NULL) {
		LogFileWrite(lf);
	} else if (lf->_asop.done == NULL) {
		lf->_asop.done = &LogFileAsopDone;
		lf->_asop.delay(NULL, Log._delay_first, FALSE);
	} else {
		lf->_asop.signal(NULL, TRUE);
	}
	lf->release();
	return len;
}
