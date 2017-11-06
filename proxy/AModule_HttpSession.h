#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#include "../ecs/AEntity.h"
#include "../ecs/AInOutComponent.h"
#define _USE_HTTP_MSG_IMPL_ 1
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
	ARefsBuf   *_outbuf;
	char*       p_next() { return _outbuf->ptr() + _parsed_len; }
	int         p_left() { return _outbuf->len() - _parsed_len; }

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

		http_parser_init(&_parser, HTTP_BOTH, this);
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
	int try_output(HttpMsg *hm, int (*on)(HttpCompenont*,int)) {
		if (strcasecmp(_object->_module->class_name, "AEntity") != 0) {
			assert(0);
			return -EACCES;
		}
		AInOutComponent *c; ((AEntity*)_object)->_get(&c);
		if (c == NULL) {
			assert(0);
			return -EACCES;
		}
		_httpmsg = hm; on_httpmsg = on;
		_body_pos = _body_len = 0;

		assert((c->on_output == NULL) || (c->on_output == &_try_output));
		c->on_output = &_try_output;
		return c->_output_cycle(512, send_bufsiz);
	}
	static int _try_output(AInOutComponent *c, int result) {
		HttpCompenont *p; ((AEntity*)c->_object)->_get(&p);
		if (p == NULL) {
			assert(0);
			return -EACCES;
		}
		p->_outbuf = c->_outbuf;
		return p->on_io_output(result, c->_outbuf);
	}
	int on_io_output(int result, ARefsBuf *&buf) {
		assert(_outbuf == buf);
		if (result < 0)
			return on_httpmsg(this, result);
		if (p_left() == 0)
			return 1; // need more data

		result = http_parser_execute(&_parser, &cb_sets, p_next(), p_left());
		if (_parser.http_errno == HPE_PAUSED)
			http_parser_pause(&_parser, FALSE);

		if (_parser.http_errno != HPE_OK) {
			TRACE("http_parser_error(%d) = %s.\n",
				_parser.http_errno, http_parser_error(&_parser));
			return on_httpmsg(this, -AMsgType_Private|_parser.http_errno);
		}
		_parsed_len += result;

		result = http_next_chunk_is_incoming(&_parser);
		if (result == 0) {
			// new buffer
			assert(p_left() == 0);
			int reserve = 0;
			if (!http_header_is_complete(&_parser)) {
				reserve = send_bufsiz;
			}
			else if (_body_len < max_body_size) {
				if (_body_len == 0)
					_body_pos = _parsed_len;
				_outbuf->pop(_body_pos);
				_parsed_len -= _body_pos;
				_body_pos = 0;
				reserve = min(_parser.content_length, max_body_size-_body_len);
			}
			if (reserve != 0) { // need more data
				result = ARefsBuf::reserve(buf, reserve, recv_bufsiz);
				TRACE2("resize buffer size = %d, left = %d, reserve = %d, result = %d.\n",
					buf->len(), buf->left(), reserve, result);
				return (result >= 0) ? 1 : on_httpmsg(this, result);
			}
		}
		_body_pos += _outbuf->_bgn;
		_outbuf->pop(_parsed_len);
		_parsed_len = 0;

		assert(_httpmsg->_head_num == _header_count);
		_httpmsg->_parser = _parser;

		release_s(_httpmsg->_body_buf);
		_httpmsg->_body_buf = _outbuf; _outbuf->addref();
		_httpmsg->body_pos() = _body_pos;
		_httpmsg->body_len() = _body_len;

		return on_httpmsg(this, 1); // return httpMsgType_HttpMsg; > AMsgType_Class
	}
	//////////////////////////////////////////////////////////////////////////
	str_t _field() { return str_t(_outbuf->ptr() + _field_pos, _field_len); }
	str_t _value() { return str_t(_outbuf->ptr() + _value_pos, _value_len); }

	static int on_m_begin(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;

		p->_httpmsg->reset();
		p->_header_block.set(NULL, 0, 0);
		p->_header_count = 0;

		p->_field_pos = p->_field_len = 0;
		p->_value_pos = p->_value_len = 0;
		p->_body_pos = p->_body_len = 0;
		return 0;
	}
	static int on_url_or_status(http_parser *parser, const char *at, size_t length) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

		if (p->_field_len == 0) {
			p->_field_pos = p->_value_pos = (at - p->_outbuf->ptr());
		}
		p->_field_len += length;
		p->_value_len += length;
		return 0;
	}
	static int on_h_field(http_parser *parser, const char *at, size_t length) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

		if (p->_value_len != 0) {
			if (++p->_header_count == 1) {
				p->_httpmsg->set_url(p->_value());
			} else {
				p->_httpmsg->set(p->_field(), p->_value());
			}
			p->_field_pos = p->_field_len = 0;
			p->_value_pos = p->_value_len = 0;
		}

		if (p->_field_len == 0)
			p->_field_pos = (at - p->_outbuf->ptr());
		p->_field_len += length;
		return 0;
	}
	static int on_h_value(http_parser *parser, const char *at, size_t length) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

		assert(p->_field_len != 0);
		if (p->_value_len == 0)
			p->_value_pos = (at - p->_outbuf->ptr());
		p->_value_len += length;
		return 0;
	}
	static int on_h_done(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		if (p->_value_len != 0) {
			p->_header_count++;
			p->_httpmsg->set(p->_field(), p->_value());
		}
		assert(p->_header_block._buf == NULL);
		p->_header_block.set(p->_outbuf, p->_outbuf->_bgn, p->_parser.nread);
		return 0;
	}
	static int on_body(http_parser *parser, const char *at, size_t length) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

		if (p->_body_len == 0)
			p->_body_pos = (at - p->_outbuf->ptr());
		p->_body_len += length;
		return 0;
	}
	static int on_m_done(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		http_parser_pause(parser, TRUE);
		return 0;
	}
	static int on_chunk_header(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		TRACE("chunk header, body = %lld.\n", parser->content_length);
		return 0;
	}
	static int on_chunk_complete(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;
		if (p->_body_len != 0)
			http_parser_pause(parser, TRUE);
		else
			; // last chunk size = 0
		return 0;
	}
	static const struct http_parser_settings cb_sets;// = {
	//	&on_m_begin, &on_url_or_status, &on_url_or_status, &on_h_field, &on_h_value,
	//	&on_h_done, &on_body, &on_m_done, &on_chunk_header, &on_chunk_complete
	//};
	//////////////////////////////////////////////////////////////////////////
	static int encode(ARefsBuf *&buf, HttpMsg *hm) {
		str_t v;
		int result = 20;
		for (int ix = 0; ix < hm->_head_num; ++ix) {
			result += hm->at(ix, &v).len;
			result += v.len + 6;
		}
		if (hm->body_len() > 0)
			result += 64;

		result = ARefsBuf::reserve(buf, result, send_bufsiz);
		if (result < 0)
			return result;

		v = hm->get_url();
		if (hm->_parser.type == HTTP_REQUEST) {
			buf->strfmt("%s %.*s HTTP/%d.%d\r\n", http_method_str(hm->_parser.method),
				v.len, v.str, hm->_parser.http_major, hm->_parser.http_minor);
		} else {
			buf->strfmt("HTTP/%d.%d %d %.*s\r\n", hm->_parser.http_major, hm->_parser.http_minor,
				hm->_parser.status_code, v.len, v.str);
		}
		if (hm->body_len() > 0) {
			buf->strfmt("Content-Length: %lld\r\n", hm->body_len());
		}
		for (int ix = 1; ix < hm->_head_num; ++ix) {
			str_t f = hm->at(ix, &v);
			buf->strfmt("%.*s: %.*s\r\n", f.len, f.str, v.len, v.str);
		}
		buf->strfmt("\r\n");
		return buf->len();
	}
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

	ARefsBuf     *_inbuf;
	int (*raw_inmsg_done)(AMessage*,int);

	HttpMsgImpl  *_req;
	HttpMsgImpl  *_resp;
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
