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

	void  reset()                         { _reset(this); }
	str_t uri_get(int isRaw)              { return _kv_get(this, KV_UriInfo, str_t(isRaw?"1":"0",1)); }
	int   uri_set(str_t value, int isRaw) { return _kv_set(this, KV_UriInfo, str_t(isRaw?"1":"0",1), value); }

	str_t header_get(str_t field)              { return _kv_get(this, KV_Header, field); }
	int   header_set(str_t field, str_t value) { return _kv_set(this, KV_Header, field, value); }
	int   header_num()                         { return _kv_num(this, KV_Header); }
	int   header_clear()                       { return _kv_set(this, KV_Header, str_t(), str_t()); }

	str_t param_get(str_t field)              { return _kv_get(this, KV_Param, field); }
	int   param_set(str_t field, str_t value) { return _kv_set(this, KV_Param, field, value); }
	int   param_num()                         { return _kv_num(this, KV_Param); }
	int   param_clear()                       { return _kv_set(this, KV_Param, str_t(), str_t()); }

	str_t cookie_get(str_t field)              { return _kv_get(this, KV_Cookie, field); }
	int   cookie_set(str_t field, str_t value) { return _kv_set(this, KV_Cookie, field, value); }
	int   cookie_num()                         { return _kv_num(this, KV_Cookie); }
	int   cookie_clear()                       { return _kv_set(this, KV_Cookie, str_t(), str_t()); }

	char*     body_ptr() { return _body_buf->_data + body_pos(); }
	uint32_t& body_pos() { return _parser.nread; }
	int64_t&  body_len() { return *(int64_t*)&_parser.content_length; }
	void      body_set(ARefsBuf *p, uint32_t pos, int64_t len) {
		addref_s(_body_buf, p); body_pos() = pos; body_len() = len;
	}
};

enum { httpMsgType_HttpMsg = (AMsgType_Private|3) };


struct HttpConnectionModule {
	AModule module;
	AMODULE_GET(HttpConnectionModule ,"AEntity", "HttpConnection")

	const str_t *http_method_str; // end with str_t{ NULL, 0 }
	int (*iocom_output)(struct AInOutComponent *c, int result);
	int (*input_status)(struct HttpConnection *p, AMessage *msg, HttpMsg *hm, int result);

	HttpMsg* (*hm_create)();
	void     (*hm_release)(HttpMsg *hm);
	int      (*hm_encode)(HttpMsg *hm, ARefsBuf *&buf);
	int      (*hm_decode)(struct HttpParserCompenont *p, HttpMsg *hm, ARefsBuf *&buf);

	int (*request)(struct HttpConnection *p, HttpMsg *req, int(*on_resp)
		(struct HttpConnection *p, HttpMsg *req, HttpMsg *resp, int result));
};


#endif
