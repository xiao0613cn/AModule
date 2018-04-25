#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#include "../ecs/AInOutComponent.h"
#include "../http/http_msg.h"

#define send_bufsiz     2*1024
#define recv_bufsiz     64*1024

struct HttpParserCompenont : public AComponent {
	static const char* name() { return "HttpParserCompenont"; }

	HttpMsg    *_httpmsg;
	int     (*on_httpmsg)(HttpParserCompenont *c, int result);
	int         _max_body_size;

	http_parser _parser;
	int         _parsed_len;
	ARefsBlock<>_header_block;
	int         _header_count;
	int         _field_pos;
	int         _field_len;
	int         _value_pos;
	int         _value_len;
	int         _body_pos;
	int         _body_len;

	void init2() {
		_httpmsg = NULL;
		on_httpmsg = NULL;
		_max_body_size = 16*1024*1024;

		HttpConnectionModule::get()->parser_init(&_parser, HTTP_BOTH, NULL);
		_parsed_len = 0;
		_header_block.init();
		_header_count = 0;
		_field_pos = _field_len = 0;
		_value_pos = _value_len = 0;
		_body_pos = _body_len = 0;
	}
	void exit2() {
		_header_block.set(NULL, 0, 0);
	}
	int try_output(AInOutComponent *c, HttpMsg *hm, int (*done)(HttpParserCompenont*,int)) {
		_httpmsg = hm; on_httpmsg = done;

		HttpConnectionModule *HCM = HttpConnectionModule::get();
		assert((c->on_output == NULL) || (c->on_output == HCM->iocom_output));

		c->on_output = HCM->iocom_output;
		c->on_output_userdata = this;
		return c->_output_cycle(512, send_bufsiz);
	}
	//////////////////////////////////////////////////////////////////////////
	ARefsBuf *outbuf() { return (ARefsBuf*)_parser.data; }
	char*     p_next() { return outbuf()->ptr() + _parsed_len; }
	int       p_left() { return outbuf()->len() - _parsed_len; }
	str_t     _field() { return str_t(outbuf()->ptr() + _field_pos, _field_len); }
	str_t     _value() { return str_t(outbuf()->ptr() + _value_pos, _value_len); }
};


#endif
