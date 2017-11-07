#ifndef _AMODULE_HTTP_MSG_H_
#define _AMODULE_HTTP_MSG_H_
extern "C" {
#include "http_parser.h"
};

struct HttpMsg {
	http_parser   _parser;
	int           _head_num;
	str_t       (*_head_at)(HttpMsg *p, int ix, str_t *value);
	str_t       (*_head_get)(HttpMsg *p, const char *field);
	int         (*_head_set)(HttpMsg *p, str_t field, str_t value);
	ARefsBuf     *_body_buf;
	char*          body_ptr() { return _body_buf->_data + _parser.nread; }
	uint32_t&      body_pos() { return _parser.nread; }
	int64_t&       body_len() { return *(int64_t*)&_parser.content_length; }

	void     reset()                       { set(str_t(), str_t()); release_s(_body_buf); }
	str_t    get_url()                     { return _head_get(this, ""); }
	int      set_url(str_t value)          { return _head_set(this, str_t("",0), value); }
	str_t    at(int ix, str_t *value)      { return _head_at(this, ix, value); }
	str_t    get(const char *field)        { return _head_get(this, field); }
	int      set(str_t field, str_t value) { return _head_set(this, field, value); }
};

#define httpMsgType_RawData       (AMsgType_Private|1)
#define httpMsgType_RawBlock      (AMsgType_Private|2)
#define httpMsgType_HttpMsg       (AMsgType_Private|3)


#if defined(_USE_HTTP_MSG_IMPL_) && (_USE_HTTP_MSG_IMPL_ != 0)
#include <map>
#include <string>

struct HttpMsgImpl : public HttpMsg {
	typedef std::map<std::string,std::string> HeaderMap;
	HeaderMap _headers;

	HttpMsgImpl() {
		memset(&_parser, 0, sizeof(_parser));
		_head_num = 0;
		_head_at = &head_at;
		_head_get = &head_get;
		_head_set = &head_set;
		_body_buf = NULL;
	}
	~HttpMsgImpl() {
		release_s(_body_buf);
	}
	static str_t head_at(HttpMsg *p, int ix, str_t *value) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		HeaderMap::iterator it = me->_headers.begin();
		while (ix > 0) { ++it; --ix; }
		if (value != NULL) {
			value->str = it->second.c_str();
			value->len = it->second.length();
		}
		return str_t(it->first.c_str(), it->first.length());
	}
	static str_t head_get(HttpMsg *p, const char *field) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		HeaderMap::iterator it = me->_headers.find(field);
		if (it == me->_headers.end())
			return str_t();
		return str_t(it->second.c_str(), it->second.length());
	}
	static int head_set(HttpMsg *p, str_t field, str_t value) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (field.str == NULL)
			me->_headers.clear();
		else
			me->_headers[std::string(field.str,field.len)].assign(value.str, value.len);
		return me->_head_num = me->_headers.size();
	}
};
#endif


#endif
