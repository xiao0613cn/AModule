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

	void     reset_head()                  { set(str_t(), str_t()); }
	void     reset_body()                  { if (_body_buf) _body_buf->reset(); body_pos() = 0; body_len() = 0; }
	str_t    get_url()                     { return _head_get(this, ""); }
	int      set_url(str_t value)          { return _head_set(this, str_t("",0), value); }
	str_t    at(int ix, str_t *value)      { return _head_at(this, ix, value); }
	str_t    get(const char *field)        { return _head_get(this, field); }
	int      set(str_t field, str_t value) { return _head_set(this, field, value); }
};

enum { httpMsgType_HttpMsg = (AMsgType_Private|3) };


#if defined(_USE_HTTP_MSG_IMPL_) && (_USE_HTTP_MSG_IMPL_ != 0)
#include <vector>
#include <string>

struct HttpMsgImpl : public HttpMsg {
	typedef std::pair<std::string, std::string> HeaderItem;
	typedef std::vector<HeaderItem> HeaderVec;
	HeaderVec _headers;
	void     *_user;

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
		HeaderVec::iterator it = me->_headers.begin() + ix;
		if (value != NULL) {
			value->str = it->second.c_str();
			value->len = it->second.length();
		}
		return str_t(it->first.c_str(), it->first.length());
	}
	static str_t head_get(HttpMsg *p, const char *field) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		for (HeaderVec::iterator it = me->_headers.begin();
			it != me->_headers.end(); ++it)
		{
			if (it->first != field) continue;
			return str_t(it->second.c_str(), it->second.length());
		}
		return str_t();
	}
	static int head_set(HttpMsg *p, str_t field, str_t value) {
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
			it->second = std::string(value.str,value.len);
			return me->_headers.size();
		}
		me->_headers.push_back(HeaderItem(
			std::string(field.str,field.len), std::string(value.str,value.len)));
		return me->_head_num = me->_headers.size();
	}
};
#endif


#endif
