#include "stdafx.h"
#include "AModule_API.h"

struct LogFile : public AObject {
	rb_node     _node;
	char        _name[32];
	FILE       *_file;
	AOperator   _asop;
	APool<char> _buf;
};

static int LogFileAsopDone(AOperator *asop, int result)
{
	LogFile *lf = container_of(asop, LogFile, _asop);

	return asop->delay(NULL, 2*1000, FALSE);
}

static int LogFileCmp(const char *key, LogFile *data) {
	return strcasecmp(key, data->_name);
}
rb_tree_define(LogFile,	_node, const char*, LogFileCmp)

static struct {
	struct rb_root  _file_map;
	pthread_mutex_t _mutex;
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }
} Log;

AMODULE_API int
ALog(const char *fname, const char *func, int line, const char fmt, va_list ap)
{
	Log.lock();
	LogFile *lf = rb_find_LogFile(&Log._file_map, fname);
	if (lf == NULL) {
		lf = gomake(LogFile);
		strcpy_sz(lf->_name, fname);
		lf->_file = NULL;
		lf->_asop.timer();
		lf->_buf.init(4*1024);
		rb_insert_LogFile(&Log._file_map, lf, lf->_name);
	}
	vsnprint
	Log.unlock();
}
