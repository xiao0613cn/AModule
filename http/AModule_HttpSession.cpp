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

static int hm_decode(HttpParserCompenont *p, ARefsBuf *&_outbuf) {
	p->_parser.data = _outbuf;
	if (p->p_left() == 0)
		return 1; // need more data

	int result = http_parser_execute(&p->_parser, &cb_sets, p->p_next(), p->p_left());
	if (p->_parser.http_errno == HPE_PAUSED)
		http_parser_pause(&p->_parser, FALSE);

	if (p->_parser.http_errno != HPE_OK) {
		TRACE("http_parser_error(%d) = %s.\n",
			p->_parser.http_errno, http_errno_description(p->_parser.http_errno));
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
			return (result >= 0) ? 1 : p->on_httpmsg(p, result);
		}
	}
	p->_body_pos += _outbuf->_bgn;
	_outbuf->pop(p->_parsed_len);
	p->_parsed_len = 0;

	assert(p->_httpmsg->header_num()+1 == p->_header_count);
	p->_httpmsg->_parser = p->_parser;
	p->_httpmsg->body_set(_outbuf, p->_body_pos, p->_body_len);

	return p->on_httpmsg(p, 1); // return httpMsgType_HttpMsg; > AMsgType_Class
}

static int iocom_output(AInOutComponent *c, int result) {
	HttpParserCompenont *p = (HttpParserCompenont*)c->_outuser;
	if (result < 0)
		return p->on_httpmsg(p, result);

	return hm_decode(p, c->_outbuf);
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

extern int HttpMsgInputStatus(HttpConnection *p, AMessage *msg, HttpMsg *hm, int result)
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

static HttpMsg* hm_create() {
	return new HttpMsgImpl();
}
static void hm_release(HttpMsg *hm) {
	delete (HttpMsgImpl*)hm;
}

HttpParserModule HPM = { {
	HttpParserCompenont::name(),
	HttpParserCompenont::name(),
	0, NULL, NULL,
},
	&iocom_output,
	&HttpMsgInputStatus,
	&hm_create,
	&hm_release,
	&hm_encode,
	&hm_decode,
};
static int reg_hpm = AModuleRegister(&HPM.module);
