#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#include "../ecs/AEntity.h"
#include "../ecs/AInOutComponent.h"
#include "../http/http_msg.h"

#define send_bufsiz     2*1024
#define recv_bufsiz     64*1024
#define max_body_size   16*1024*1024

struct HttpCompenont : public AComponent {
	static const char* name() { return "HttpCompenont"; }

	HttpMsg    *_httpmsg;
	int       (*on_httpmsg)(HttpCompenont *c, int result);

	http_parser _parser;
	int         _parsed_len;
	ARefsBuf   *outbuf() { return (ARefsBuf*)_parser.data; }
	char*       p_next() { return outbuf()->ptr() + _parsed_len; }
	int         p_left() { return outbuf()->len() - _parsed_len; }

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

		http_parser_init(&_parser, HTTP_BOTH, NULL);
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
	int try_output(AInOutComponent *c, HttpMsg *hm, int (*on)(HttpCompenont*,int)) {
		_httpmsg = hm; on_httpmsg = on;
		_body_pos = _body_len = 0;

		assert((c->on_output == NULL) || (c->on_output == &on_iocom_output));
		c->on_output = &on_iocom_output;
		c->_outuser = this;
		return c->_output_cycle(512, send_bufsiz);
	}
	//////////////////////////////////////////////////////////////////////////
	str_t _field() { return str_t(outbuf()->ptr() + _field_pos, _field_len); }
	str_t _value() { return str_t(outbuf()->ptr() + _value_pos, _value_len); }
	static int on_iocom_output(AInOutComponent *c, int result);
	struct com_module {
		AModule module;
		int (*on_iocom_output)(AInOutComponent *c, int result);
		int (*encode)(ARefsBuf *&buf, HttpMsg *hm);
		int (*input)(struct HttpConnection *p, AMessage *msg, HttpMsg *hm, int result);
	};
};

enum HttpStatus {
	s_recv_header = 0,
	s_recv_body,
	s_send_header,
	s_send_body,
};

struct HttpConnection : public AEntity {
	AInOutComponent _iocom;
	HttpCompenont _http;
	AService     *_svc;

	ARefsBuf     *_inbuf;
	int (*raw_inmsg_done)(AMessage*,int);

	struct HttpMsgImpl  *_req;
	struct HttpMsgImpl  *_resp;
	int (*raw_outmsg_done)(AMessage*,int);
};

#if 0
#ifndef _AMODULE_HTTPCLIENT_H_
#include "../http/AModule_HttpClient.h"
#endif
#ifndef _AMODULE_SESSION_H_
#include "AModule_Session.h"
#endif

// HttpClient::url used by SessionCtx
struct HttpCtxExt {
	HttpClient* p() { return container_of(this, HttpClient, url); }
	SessionManager  *sm;
	struct list_head sm_conn_entry;

	DWORD     active;
	AObject  *session;
	struct list_head sess_conn_entry;
	ARefsBuf *proc_buf;

	uint8_t   recv_param_list[25][4];
	int       recv_param_count;
	char*     p_n_ptr(int ix) { return recv_param_list[ix][0] + p()->h_f_ptr(0); }
	uint8_t&  p_n_pos(int ix) { return recv_param_list[ix][0]; }
	uint8_t&  p_n_len(int ix) { return recv_param_list[ix][1]; }
	char*     p_v_ptr(int ix) { return recv_param_list[ix][2] + p()->recv_header_buffer->data + p()->recv_header_pos; }
	uint8_t&  p_v_pos(int ix) { return recv_param_list[ix][2]; }
	uint8_t&  p_v_len(int ix) { return recv_param_list[ix][3]; }

	AOperator asop;
	int       segix;
};
#endif

#endif