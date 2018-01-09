#ifndef _AMODULE_HTTP_MSG_H_
#define _AMODULE_HTTP_MSG_H_
extern "C" {
#include "http_parser.h"
};

struct HttpMsg {
	enum KVTypes {
		KV_Header = 0,
		KV_UrlParam,
		KV_PostParam,
		KV_Cookie,
		KV_MaxTypes
	};
	void        (*_reset)(HttpMsg *p);
	http_parser   _parser;
	int         (*_kv_num)(HttpMsg *p, int type);
	str_t       (*_kv_at)(HttpMsg *p, int type, int ix, str_t *value);
	str_t       (*_kv_get)(HttpMsg *p, int type, const char *field);
	int         (*_kv_set)(HttpMsg *p, int type, str_t field, str_t value);
	ARefsBuf     *_body_buf;
	char*          body_ptr() { return _body_buf->_data + _parser.nread; }
	uint32_t&      body_pos() { return _parser.nread; }
	int64_t&       body_len() { return *(int64_t*)&_parser.content_length; }

	void  reset() { _reset(this); }
	str_t get_url()                     { return _head_get(this, ""); }
	int   set_url(str_t value)          { return _head_set(this, str_t("",0), value); }
	str_t at(int ix, str_t *value)      { return _head_at(this, ix, value); }
	str_t get(const char *field)        { return _head_get(this, field); }
	int   set(str_t field, str_t value) { return _head_set(this, field, value); }
	void  set_body(ARefsBuf *p, uint32_t pos, int64_t len) {
		addref_set(_body_buf, p); body_pos() = pos; body_len() = len;
	}
};

enum { httpMsgType_HttpMsg = (AMsgType_Private|3) };


#if defined(_USE_HTTP_MSG_IMPL_) && (_USE_HTTP_MSG_IMPL_ != 0)
#include <map>

struct HttpMsgImpl : public HttpMsg {
	typedef std::map<std::string, std::string> KVMap;
	KVMap  _headers;
	void  *_user;

	HttpMsgImpl() {
		_reset = &_reset_;
		http_parser_init(&_parser, HTTP_BOTH, 0);
		_kv_num = &_kv_num_;
		_kv_at = &_kv_at_;
		_kv_get = &_kv_get_;
		_kv_set = &_kv_set_;
		_body_buf = NULL;
		_user = NULL;
	}
	~HttpMsgImpl() {
		release_s(_body_buf);
	}
	static void _reset_(HttpMsg *p) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		http_parser_init(&me->_parser, HTTP_BOTH, me->_parser.data);
		me->_head_num = 0;
		me->_headers.clear();
		release_s(me->_body_buf);
	}
	static int _kv_num_(HttpMsg *p, int type) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		KVVec *kvv;
		switch (type) {
		case KV_Header: kvv = &me->_headers; break;
		case KV_UrlParam: kvv = &me->_urlparams; break;
		default: return -1;
		}
	}
	static str_t _kv_at_(HttpMsg *p, int ix, str_t *value) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		HeaderVec::iterator it = me->_headers.begin() + ix;
		if (value != NULL) {
			value->str = it->second.c_str();
			value->len = it->second.length();
		}
		return str_t(it->first.c_str(), it->first.length());
	}
	static str_t _kv_get_(HttpMsg *p, const char *field) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		for (HeaderVec::iterator it = me->_headers.begin();
			it != me->_headers.end(); ++it)
		{
			if (it->first != field) continue;
			return str_t(it->second.c_str(), it->second.length());
		}
		return str_t();
	}
	static int _kv_set_(HttpMsg *p, str_t field, str_t value) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (field.str == NULL) {
			me->_headers.clear();
			return me->_head_num = 0;
		}
		for (HeaderVec::iterator it = me->_headers.begin();
			it != me->_headers.end(); ++it)
		{
			if ((strncmp(it->first.c_str(), field.str, field.len) != 0)
			 || (it->first.c_str()[field.len] != '\0'))
				continue;

			if (value.str == NULL) {
				me->_headers.erase(it);
				me->_head_num = me->_headers.size();
			} else {
				it->second = std::string(value.str,value.len);
			}
			return me->_head_num;
		}
		if (value.str != NULL) {
			me->_headers.push_back(HeaderItem(
				std::string(field.str, field.len),
				std::string(value.str, value.len)));
			me->_head_num = me->_headers.size();
		}
		return me->_head_num;
	}
};
#endif


#endif
