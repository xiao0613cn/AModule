#ifndef _AMODULE_HTTP_MSG_H_
#define _AMODULE_HTTP_MSG_H_
extern "C" {
#include "http_parser.h"
};

struct HttpMsg {
	enum KVTypes {
		KV_UriInfo = 0, // "0": uri, "1": raw uri (with param)
		KV_Header,
		KV_Param,
		KV_Cookie,
		KV_MaxTypes
	};
	void        (*_reset)(HttpMsg *p);
	http_parser   _parser;
	int         (*_kv_num)(HttpMsg *p, int type);
	str_t       (*_kv_get)(HttpMsg *p, int type, str_t field);
	int         (*_kv_set)(HttpMsg *p, int type, str_t field, str_t value);
	str_t       (*_kv_next)(HttpMsg *p, int type, str_t cur_field, str_t *next_value);
	ARefsBuf     *_body_buf;
	char*          body_ptr() { return _body_buf->_data + _parser.nread; }
	uint32_t&      body_pos() { return _parser.nread; }
	int64_t&       body_len() { return *(int64_t*)&_parser.content_length; }

	void  reset() { _reset(this); }
	str_t uri_get(int isRaw) { return _kv_get(this, KV_UriInfo, str_t(isRaw?"1":"0",1)); }
	int   uri_set(str_t value, int isRaw) { return _kv_set(this, KV_UriInfo, str_t(isRaw?"1":"0",1), value); }

	str_t header_get(str_t field) { return _kv_get(this, KV_Header, field); }
	int   header_set(str_t field, str_t value) { return _kv_set(this, KV_Header, field, value); }
	int   header_num() { return _kv_num(this, KV_Header); }
	int   header_clear() { return _kv_set(this, KV_Header, str_t(), str_t()); }

	str_t param_get(str_t field) { return _kv_get(this, KV_Param, field); }
	int   param_set(str_t field, str_t value) { return _kv_set(this, KV_Param, field, value); }
	int   param_num() { return _kv_num(this, KV_Param); }
	int   param_clear() { return _kv_set(this, KV_Param, str_t(), str_t()); }

	str_t cookie_get(str_t field) { return _kv_get(this, KV_Cookie, field); }
	int   cookie_set(str_t field, str_t value) { return _kv_set(this, KV_Cookie, field, value); }
	int   cookie_num() { return _kv_num(this, KV_Cookie); }
	int   cookie_clear() { return _kv_set(this, KV_Cookie, str_t(), str_t()); }

	void  set_body(ARefsBuf *p, uint32_t pos, int64_t len) {
		addref_set(_body_buf, p); body_pos() = pos; body_len() = len;
	}
};

enum { httpMsgType_HttpMsg = (AMsgType_Private|3) };


#if defined(_USE_HTTP_MSG_IMPL_) && (_USE_HTTP_MSG_IMPL_ != 0)
#include <string>
#include <map>

struct HttpMsgImpl : public HttpMsg {
	typedef std::map<std::string, std::string> KVMap;

	KVMap   _maps[KV_MaxTypes];
	void   *_user;

	HttpMsgImpl() {
		_reset = &_reset_;
		http_parser_init(&_parser, HTTP_BOTH, 0);
		_kv_num = &_kv_num_;
		_kv_get = &_kv_get_;
		_kv_set = &_kv_set_;
		_kv_next = &_kv_next_;
		_body_buf = NULL;
		_user = NULL;
	}
	~HttpMsgImpl() {
		release_s(_body_buf);
	}
	static void _reset_(HttpMsg *p) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		http_parser_init(&me->_parser, HTTP_BOTH, me->_parser.data);
		for (int type = 0; type < KV_MaxTypes; ++type)
			me->_maps[type].clear();
		release_s(me->_body_buf);
	}
	static int _kv_num_(HttpMsg *p, int type) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type > KV_MaxTypes)
			return -1;
		return me->_maps[type].size();
	}
	static str_t _kv_next_(HttpMsg *p, int type, str_t f, str_t *v) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type > KV_MaxTypes)
			return str_t();
		KVMap::iterator it;
		if (f.str == NULL) {
			it = me->_maps[type].begin();
		} else {
			it = me->_maps[type].find(std::string(f.str, f.len));
			if (it == me->_maps[type].end())
				return str_t();
			++it;
		}
		if (it == me->_maps[type].end())
			return str_t();
		if (v != NULL) {
			v->str = (char*)it->second.c_str();
			v->len = it->second.length();
		}
		return str_t(it->first.c_str(), it->first.length());
	}
	static str_t _kv_get_(HttpMsg *p, int type, str_t f) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type > KV_MaxTypes)
			return str_t();
		KVMap::iterator it = me->_maps[type].find(std::string(f.str, f.len));
		if (it == me->_maps[type].end())
			return str_t();
		return str_t(it->second.c_str(), it->second.length());
	}
	static int _kv_set_(HttpMsg *p, int type, str_t f, str_t v) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type > KV_MaxTypes)
			return -1;
		if (f.str == NULL) {
			me->_maps[type].clear();
		} else if (v.str == NULL) {
			me->_maps[type].erase(std::string(f.str, f.len));
		} else {
			me->_maps[type][std::string(f.str, f.len)] = std::string(v.str, v.len);
		}
		return me->_maps[type].size();
	}
};
#endif


#endif
