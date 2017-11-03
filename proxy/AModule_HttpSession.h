#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#include "../ecs/AEntity.h"
#include "../ecs/AInOutComponent.h"
#define _USE_HTTP_MSG_IMPL_ 1
#include "../http/http_msg.h"

#define send_bufsiz     2*1024
#define recv_bufsiz     64*1024
#define max_body_size   16*1024*1024

struct HttpCompenont : public AInOutComponent {
	HttpMsg    *_httpmsg;
	int       (*on_httpmsg)(HttpCompenont *c, int result);

	http_parser _parser;
	int         _parsed_len;
	char*       p_next() { return _outbuf->ptr() + _parsed_len; }
	int         p_left() { return _outbuf->len() - _parsed_len; }

	ARefsBlock<>_header_block;
	int         _header_count;
	int         _header_pos;
	int         _header_len;

	int         _field_pos;
	int         _field_len;
	int         _value_pos;
	int         _value_len;
	int         _body_pos;
	int         _body_len;

	void init2() {
		AInOutComponent::init2();
		AInOutComponent::on_output = &_try_output;
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
		AInOutComponent::exit2();
	}
	int try_output(HttpMsg *hm, int (*on)(HttpCompenont*,int)) {
		_httpmsg = hm; on_httpmsg = on;
		_body_pos = _body_len = 0;
		return _output_cycle(512, send_bufsiz);
	}
	static int _try_output(AInOutComponent *c, int result) {
		HttpCompenont *p = (HttpCompenont*)c;
		if (result < 0)
			return p->on_httpmsg(p, result);
		if (p->p_left() == 0)
			return 1; // need more data

		result = http_parser_execute(&p->_parser, &cb_sets, p->p_next(), p->p_left());
		if (p->_parser.http_errno == HPE_PAUSED)
			http_parser_pause(&p->_parser, FALSE);

		if (p->_parser.http_errno != HPE_OK) {
			TRACE("http_parser_error(%d) = %s.\n",
				p->_parser.http_errno, http_parser_error(&p->_parser));
			return p->on_httpmsg(p, -AMsgType_Private|p->_parser.http_errno);
		}
		p->_parsed_len += result;

		result = http_next_chunk_is_incoming(&p->_parser);
		if (result == 0) {
			// new buffer
			assert(p->p_left() == 0);
			int reserve = 0;
			if (!http_header_is_complete(&p->_parser)) {
				reserve = send_bufsiz;
			}
			else if (p->_body_len < max_body_size) {
				if (p->_body_len == 0)
					p->_body_pos = p->_parsed_len;

				p->_outbuf->pop(p->_body_pos);
				p->_parsed_len -= p->_body_pos;
				p->_body_pos = 0;

				reserve = min(p->_parser.content_length, max_body_size-p->_body_len);
			}
			if (reserve != 0) { // need more data
				result = ARefsBuf::reserve(p->_outbuf, reserve, recv_bufsiz);
				TRACE2("resize buffer size = %d, left = %d, reserve = %d, result = %d.\n",
					p->_outbuf->len(), p->_outbuf->left(), reserve, result);
				return (result >= 0) ? 1 : p->on_httpmsg(p, result);
			}
		}
		p->_body_pos += p->_outbuf->_bgn;
		p->_outbuf->pop(p->_parsed_len);
		p->_parsed_len = 0;

		assert(p->_httpmsg->_head_num == p->_header_count);
		p->_httpmsg->_parser = p->_parser;

		release_s(p->_httpmsg->_body_buf);
		p->_httpmsg->_body_buf = p->_outbuf; p->_outbuf->addref();
		p->_httpmsg->_parser.nread = p->_body_pos;
		p->_httpmsg->_parser.content_length = p->_body_len;

		return p->on_httpmsg(p, 1); // httpMsgType_HttpMsg > AMsgType_Class
	}
	//////////////////////////////////////////////////////////////////////////
	str_t _field() { return str_t(_outbuf->ptr() + _field_pos, _field_len); }
	str_t _value() { return str_t(_outbuf->ptr() + _value_pos, _value_len); }

	static int on_m_begin(http_parser *parser) {
		HttpCompenont *p = (HttpCompenont*)parser->data;

		p->_httpmsg->set(str_t(), str_t());
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
