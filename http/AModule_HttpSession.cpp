#include "../stdafx.h"
#define _USE_HTTP_MSG_IMPL_ 1
#include "AModule_HttpSession.h"


static int on_m_begin(http_parser *parser) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);

	p->_httpmsg->reset();
	p->_header_block.set(NULL, 0, 0);
	p->_header_count = 0;

	p->_field_pos = p->_field_len = 0;
	p->_value_pos = p->_value_len = 0;
	p->_body_pos = p->_body_len = 0;
	return 0;
}
static int on_url_or_status(http_parser *parser, const char *at, size_t length) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_field_len == 0) {
		p->_field_pos = p->_value_pos = (at - p->outbuf()->ptr());
	}
	p->_field_len += length;
	p->_value_len += length;
	return 0;
}
static int on_h_field(http_parser *parser, const char *at, size_t length) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_value_len != 0) {
		if (++p->_header_count == 1) {
			p->_httpmsg->uri_set(p->_value(), 1);
		} else {
			p->_httpmsg->header_set(p->_field(), p->_value());
		}
		p->_field_pos = p->_field_len = 0;
		p->_value_pos = p->_value_len = 0;
	}

	if (p->_field_len == 0)
		p->_field_pos = (at - p->outbuf()->ptr());
	p->_field_len += length;
	return 0;
}
static int on_h_value(http_parser *parser, const char *at, size_t length) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	assert(p->_field_len != 0);
	if (p->_value_len == 0)
		p->_value_pos = (at - p->outbuf()->ptr());
	p->_value_len += length;
	return 0;
}
static int on_h_done(http_parser *parser) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	if (p->_value_len != 0) {
		p->_header_count++;
		p->_httpmsg->header_set(p->_field(), p->_value());
	}
	assert(p->_header_block._buf == NULL);
	p->_header_block.set(p->outbuf(), p->outbuf()->_bgn, p->_parser.nread);
	return 0;
}
static int on_body(http_parser *parser, const char *at, size_t length) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	assert((at >= p->p_next()) && (at < p->p_next()+p->p_left()));

	if (p->_body_len == 0)
		p->_body_pos = (at - p->outbuf()->ptr());
	p->_body_len += length;
	return 0;
}
static int on_m_done(http_parser *parser) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	http_parser_pause(parser, TRUE);
	return 0;
}
static int on_chunk_header(http_parser *parser) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	TRACE("chunk header, body = %lld.\n", parser->content_length);
	return 0;
}
static int on_chunk_complete(http_parser *parser) {
	HttpParserCompenont *p = container_of(parser, HttpParserCompenont, _parser);
	if (p->_body_len != 0)
		http_parser_pause(parser, TRUE);
	else
		; // last chunk size = 0
	return 0;
}
static const struct http_parser_settings cb_sets = {
	&on_m_begin,
	&on_url_or_status,
	&on_url_or_status,
	&on_h_field,
	&on_h_value,
	&on_h_done,
	&on_body,
	&on_m_done,
	&on_chunk_header,
	&on_chunk_complete
};

// 0: need more data
// <0: error
// >0: p->_httpmsg valid
static int hm_decode(HttpParserCompenont *p, HttpMsg *hm, ARefsBuf *&_outbuf) {
	p->_httpmsg = hm;
	p->_parser.data = _outbuf;
	if (p->p_left() == 0)
		return 0;

	int result = http_parser_execute(&p->_parser, &cb_sets, p->p_next(), p->p_left());
	if (p->_parser.http_errno == HPE_PAUSED)
		http_parser_pause(&p->_parser, FALSE);

	if (p->_parser.http_errno != HPE_OK) {
		TRACE("http_parser_error(%d) = %s.\n", p->_parser.http_errno,
			http_errno_description(p->_parser.http_errno));
		return -AMsgType_Private|p->_parser.http_errno;
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
		else if (p->_body_len < p->_max_body_size) {
			if (p->_body_len == 0)
				p->_body_pos = p->_parsed_len;
			_outbuf->pop(p->_body_pos);
			p->_parsed_len -= p->_body_pos;
			p->_body_pos = 0;
			reserve = min(p->_parser.content_length, p->_max_body_size-p->_body_len);
		}
		if (reserve != 0) { // need more data
			result = ARefsBuf::reserve(_outbuf, reserve, recv_bufsiz);
			TRACE2("resize buffer size = %d, left = %d, reserve = %d, result = %d.\n",
				_outbuf->len(), _outbuf->left(), reserve, result);
			return (result < 0) ? result : 0;
		}
	}
	p->_body_pos += _outbuf->_bgn;
	_outbuf->pop(p->_parsed_len);
	p->_parsed_len = 0;

	assert(hm->header_num()+1 == p->_header_count);
	hm->_parser = p->_parser;
	hm->body_set(_outbuf, p->_body_pos, p->_body_len);
	return 1;
}

static int iocom_output(AInOutComponent *c, int result) {
	HttpParserCompenont *p = (HttpParserCompenont*)c->_outuser;
	if (result >= 0)
		result = hm_decode(p, p->_httpmsg, c->_outbuf);

	if (result != 0)
		result = p->on_httpmsg(p, result);
	else
		result = 1; // continue for c->_output_cycle().
	return result;
}

static int hm_encode(HttpMsg *hm, ARefsBuf *&buf) {
	str_t f, v;
	int result = 20;
	while ((f = hm->_kv_next(hm, HttpMsg::KV_Header, f, &v)).str != NULL) {
		result += f.len;
		result += v.len + 6;
	}
	if (hm->body_len() > 0)
		result += 64;

	result = ARefsBuf::reserve(buf, result, send_bufsiz);
	if (result < 0)
		return result;

	v = hm->uri_get(1);
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
	f.str = NULL;
	while ((f = hm->_kv_next(hm, HttpMsg::KV_Header, f, &v)).str != NULL) {
		buf->strfmt("%.*s: %.*s\r\n", f.len, f.str, v.len, v.str);
	}
	buf->strfmt("\r\n");
	return buf->len();
}

static int HttpMsgInputStatus(HttpConnection *p, AMessage *msg, HttpMsg *hm, int result)
{
	while (result > 0) {
		if (msg->data == NULL) { // do input header
			//assert(msg->done != &AInOutComponent::_inmsg_done
			//    && msg->done != &AInOutComponent::_outmsg_done);
			result = hm_encode(hm, p->_inbuf);
			if (result < 0)
				break;

			msg->init(ioMsgType_Block, p->_inbuf->ptr(), p->_inbuf->len());
			result = p->_iocom._io->input(msg);
			continue;
		}
		if (msg->data == p->_inbuf->ptr()) { // do input body
			assert(msg->size == p->_inbuf->len());
			p->_inbuf->pop(msg->size);

			if (hm->body_len() > 0) {
				msg->init(ioMsgType_Block, hm->body_ptr(), hm->body_len());
				result = p->_iocom._io->input(msg);
				continue;
			}
		} else {
			assert(msg->data == hm->body_ptr());
			assert(msg->size == hm->body_len());
		}
		break; // input done
	}
	if (result != 0)
		msg->done = ((msg == &p->_iocom._inmsg) ? p->raw_inmsg_done : p->raw_outmsg_done);
	return result;
}

#include <string>
#include <map>

struct HttpMsgImpl : public HttpMsg {
	typedef std::map<std::string, std::string> KVMap;
	KVMap   _maps[KV_MaxTypes];
	unsigned _parsed_url : 1;
	unsigned _parsed_param : 1;
	unsigned _parsed_cookie : 1;

	HttpMsgImpl() {
		_reset = &_reset_;
		http_parser_init(&_parser, HTTP_BOTH, NULL);
		_kv_num = &_kv_num_;
		_kv_get = &_kv_get_;
		_kv_set = &_kv_set_;
		_kv_next = &_kv_next_;
		_body_buf = NULL;
		_parsed_url = _parsed_param = _parsed_cookie = 0;
	}
	~HttpMsgImpl() {
		release_s(_body_buf);
	}
	static void _reset_(HttpMsg *p) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		http_parser_init(&me->_parser, HTTP_BOTH, me->_parser.data);

		for (int type = 0; type < KV_MaxTypes; ++type) {
			me->_maps[type].clear();
		}
		release_s(me->_body_buf);
		me->_parsed_url = me->_parsed_param = me->_parsed_cookie = 0;
	}
	static int _kv_num_(HttpMsg *p, int type) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type >= KV_MaxTypes)
			return -1;

		if ((type == KV_Param) && (me->_parsed_param == 0)) {
			_url_parse(me);
			me->_parsed_param = 1;
		}
		//if ((type == KV_Cookie) && !_parsed_cookie)) {
		//	_cookie_parse(me);
		//	_parsed_cookie = 1;
		//};
		return me->_maps[type].size();
	}
	static str_t _kv_next_(HttpMsg *p, int type, str_t f, str_t *v) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type >= KV_MaxTypes)
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
		if (type < 0 || type >= KV_MaxTypes)
			return str_t();

		KVMap::iterator it = me->_maps[type].find(std::string(f.str, f.len));
		if (it == me->_maps[type].end())
			return str_t();

		return str_t(it->second.c_str(), it->second.length());
	}
	static int _kv_set_(HttpMsg *p, int type, str_t f, str_t v) {
		HttpMsgImpl *me = (HttpMsgImpl*)p;
		if (type < 0 || type >= KV_MaxTypes)
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
	static int _url_parse(HttpMsgImpl *me) {
		str_t s = me->uri_get(1);
		http_parser_url u; http_parser_url_init(&u);
		if (http_parser_parse_url(s.str, s.len, 1, &u) != 0)
			return -2;

		if (u.field_set & UF_PATH) {
			me->uri_set(str_t(s.str+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len), 0);
		}
		if (u.field_set & UF_QUERY) {
			const char *end = s.str + u.field_data[UF_QUERY].off + u.field_data[UF_QUERY].len;
			str_t f(s.str+u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
			while (f.len != 0) {
				const char *sep = strnchr(f.str, '&', f.len);
				const char *sep2 = NULL;
				if (sep != NULL)
					sep2 = strnchr(f.str, '=', sep-f.str);
				else
					sep = end;
				if (sep2 != NULL) {
					me->param_set(str_t(f.str, sep2-f.str), str_t(sep2+1, sep-sep2-1));
				} else {
					me->param_set(str_t(f.str, sep-f.str), str_t());
				}
				if (sep == end)
					break;
				f = str_t(sep+1, end-sep-1);
			}
		}
	}
};
static HttpMsg* hm_create() {
	return new HttpMsgImpl;
}
static void hm_release(HttpMsg *hm) {
	delete (HttpMsgImpl*)hm;
}

static int HttpConnectionInputDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._inmsg);

	msg = AMessage::first(p->_iocom._queue);
	assert(msg->type == httpMsgType_HttpMsg);

	result = HttpMsgInputStatus(p, &p->_iocom._inmsg, (HttpMsg*)msg->data, result);
	if (result != 0)
		result = p->_iocom._inmsg.done2(result);
	return result;
}

static int HttpSvcRespDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._outmsg);
	result = HttpMsgInputStatus(p, msg, p->_resp, result);
	if (result != 0) {
		msg->init();
		msg->done2(result);
	}
	return result;
}

static int HttpConnectionInput(AInOutComponent *c, AMessage *msg)
{
	HttpConnection *p = container_of(c, HttpConnection, _iocom);
	if (msg == &c->_outmsg) {
		//assert(msg->data == (char*)p->_resp);
		p->raw_outmsg_done = msg->done;
		msg->init();
		msg->done = &HttpSvcRespDone;
		return HttpMsgInputStatus(p, msg, p->_resp, 1);
	}

	//assert(c->_inmsg.done == AModule::find<AInOutModule>(c->name(), c->name())->inmsg_done);
	switch (msg->type)
	{
	case AMsgType_Unknown:
	case ioMsgType_Block:
		c->_inmsg.init(msg);
		return c->_io->input(&c->_inmsg);

	case httpMsgType_HttpMsg:
		p->raw_inmsg_done = c->_inmsg.done;
		c->_inmsg.init();
		c->_inmsg.done = &HttpConnectionInputDone;
		return HttpMsgInputStatus(p, &c->_inmsg, (HttpMsg*)msg->data, 1);

	default: assert(0);
		return -EINVAL;
	}
}

static int HttpConnectionCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpConnection *p = (HttpConnection*)*object;
	p->init();
	p->_init_push(&p->_http);
	p->_init_push(&p->_iocom); p->_iocom.do_input = &HttpConnectionInput;

	p->_svc = NULL;
	p->_inbuf = NULL;
	p->_req = NULL;
	p->_resp = NULL;
	return 1;
}

static void HttpConnectionRelease(AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	reset_s(p->_resp, NULL, hm_release);
	reset_s(p->_req, NULL, hm_release);
	release_s(p->_svc);

	release_s(p->_inbuf);
	p->_pop_exit(&p->_http);
	p->_pop_exit(&p->_iocom);
	p->exit();
}

static const str_t HttpMethodStrs[] = {
#define XX(num, name, string)  str_t(#string, sizeof(#string)-1),
	HTTP_METHOD_MAP(XX)
#undef XX
	str_t()
};

static int HttpProbe(AObject *object, AObject *other, AMessage *msg)
{
	if ((msg == NULL) || (msg->size < 4))
		return -1;
	for (const str_t *m = HttpMethodStrs; m->str != NULL; ++m) {
		if ((msg->size > m->len) && (msg->data[m->len] == ' ')
		 && (strncmp(msg->data, m->str, m->len) == 0))
			return 60;
	}
	return 0;
}

HttpConnectionModule HCM = { {
	HttpConnectionModule::class_name(),
	HttpConnectionModule::module_name(),
	sizeof(HttpConnection),
	NULL, NULL,
	&HttpConnectionCreate,
	&HttpConnectionRelease,
	&HttpProbe,
},
	&iocom_output,
	&HttpMsgInputStatus,
	&hm_create,
	&hm_release,
	&hm_encode,
	&hm_decode,
	HttpMethodStrs,
};
static int reg_conn = AModuleRegister(&HCM.module);
