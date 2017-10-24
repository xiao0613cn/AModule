#include "stdafx.h"
#include "AModule_HttpClient.h"


extern void HttpClientRelease(AObject *object)
{
	HttpClient *p = (HttpClient*)object;
	release_s(p->io);

	AOptionExit(&p->send_headers);
	release_s(p->recv_buffer);
	release_s(p->recv_header_buffer);
}

static void HttpClientResetStatus(HttpClient *p)
{
	p->send_status = s_invalid;

	http_parser_init(&p->recv_parser, HTTP_BOTH, p);
	p->recv_parser_pos = 0;
	p->recv_header_count = 0;

	release_s(p->recv_buffer);
	release_s(p->recv_header_buffer);
	p->recv_header_pos = 0;
	p->recv_body_pos = 0;
	p->recv_body_len = 0;
}

extern int HttpClientCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpClient *p = (HttpClient*)*object;
	p->io = (IOObject*)parent;
	if (parent != NULL)
		AObjectAddRef(parent);

	AOptionInit(&p->send_headers, NULL);
	strcpy_sz(p->method, "GET");
	strcpy_sz(p->url, "/");
	strcpy_sz(p->version, "HTTP/1.0");

	p->recv_buffer = NULL;
	p->recv_header_buffer = NULL;
	HttpClientResetStatus(p);

	AOption *io_opt = AOptionFind(option, "io");
	if ((p->io == NULL) && (io_opt != NULL))
		p->create(&p->io, p, io_opt, NULL);
		//AObjectCreate((AObject**)&p->io, p, io_opt, NULL);
	return 1;
}

static int HttpClientSetHeader(HttpClient *p, AOption *option)
{
	AOption *header = NULL;
	if (option->name[0] == ':')
		header = p->send_headers.find(option->name);

	if (header != NULL) {
		strcpy_sz(header->value, option->value);
	} else {
		header = AOptionClone(option, &p->send_headers);
		if (header == NULL)
			return -ENOMEM;
	}
	return 0;
}

static struct ObjKV kv_map[] = {
	ObjKV_S(HttpClient, method, "GET")
	ObjKV_S(HttpClient, url, "/")
	ObjKV_S(HttpClient, version, "HTTP/1.0")
	{ NULL }
};

int HttpClientOnOpen(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	AOption *option = (AOption*)p->send_from->data;

	// method
	AObjectSetKVMap(p, kv_map, option, TRUE);

	// send_headers
	AOption *pos;
	list_for_each_entry(pos, &option->children_list, AOption, brother_entry) {
		if ((pos->name[0] == ':') || (pos->name[0] == '+'))
			HttpClientSetHeader(p, pos);
	}
	return result;
}

static int HttpClientOpen(AObject *object, AMessage *msg)
{
	HttpClient *p = (HttpClient*)object;
	if ((msg->type == AMsgType_Object)
	 && (msg->size == 0)) {
		release_s(p->io);

		p->io = (IOObject*)msg->data;
		if (p->io != NULL)
			AObjectAddRef(p->io);

		HttpClientResetStatus(p);
		return 1;
	}

	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *io_opt = AOptionFind(option, "io");

	if (p->io == NULL) {
		//AObjectCreate((AObject**)&p->io, p, io_opt, "tcp");
		int result = p->create(&p->io, p, io_opt, "tcp");
		if (result < 0)
			return result;
	}

	p->send_msg.init(io_opt);
	p->send_msg.done = &TObjectDone(HttpClient, send_msg, send_from, HttpClientOnOpen);
	p->send_from = msg;

	int result = p->io->open(&p->send_msg);
	if (result != 0)
		result = HttpClientOnOpen(p, result);
	return result;
}

static int HttpClientSetOption(AObject *object, AOption *option)
{
	HttpClient *p = (HttpClient*)object;
	if ((option->name[0] == ':') || (option->name[0] == '+'))
		return HttpClientSetHeader(p, option);
	return AObjectSetOpt(object, option, kv_map);
}

int HttpClientOnSendStatus(HttpClient *p, int result)
{
	AMessage *msg = p->send_from;
	while (result > 0)
	{
		switch (p->send_status)
		{
		case s_send_header:
			append_data("%s %s %s\r\n", p->method, p->url, p->version);

		case s_send_private_header:
			AOption *pos;
			list_for_each_entry(pos, &p->send_headers.children_list, AOption, brother_entry)
			{
				if ((pos->name[0] != ':') && (pos->name[0] != '+'))
					continue;
				if (pos->value[0] == '\0')
					continue;

				if ((strcasecmp(pos->name+1, "Transfer-Encoding") == 0)
				 && (strcasecmp(pos->value, "chunked") == 0))
					p->send_status = s_send_chunk_data;

				append_data("%s: %s\r\n", pos->name+1, pos->value);
			}
			if (p->send_msg.size+64 >= send_bufsiz)
				return -ENOMEM;

			if (msg->size == 0) {
				append_crlf();
				if (p->send_status == s_send_chunk_data)
					p->send_status = s_send_chunk_next;
				else
					p->send_status = s_send_done;
			} else {
				if (p->send_status == s_send_chunk_data) {
					append_data("\r\n%x\r\n", msg->size);
				} else {
					append_data("Content-Length: %d\r\n\r\n", msg->size);
					p->send_status = s_send_content_data;
				}
			}
			result = p->io->input(&p->send_msg);
			break;

		case s_send_chunk_size:
			append_data("%x\r\n", msg->size);
			if (msg->size == 0) {
				append_crlf();
				p->send_status = s_send_done;
			} else if (p->send_msg.size+msg->size+2 <= send_bufsiz) {
				memcpy(p->send_buffer+p->send_msg.size, msg->data, msg->size);
				p->send_msg.size += msg->size;

				append_crlf();
				p->send_status = s_send_chunk_next;
			} else {
				p->send_status = s_send_chunk_data;
			}
			result = p->io->input(&p->send_msg);
			break;

		case s_send_chunk_data:
			p->send_msg.init(ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_chunk_tail;
			result = p->io->input(&p->send_msg);
			break;

		case s_send_chunk_tail:
			p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
			append_crlf();

			p->send_status = s_send_chunk_next;
			result = p->io->input(&p->send_msg);
			break;

		case s_send_content_data:
			p->send_msg.init(ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_done;
			result = p->io->input(&p->send_msg);
			break;

		case s_send_done:
			p->send_status = s_invalid;
		case s_send_chunk_next:
			return result;

		default:
			assert(FALSE);
			return -EACCES;
		}
	}
	return result;
}

extern int HttpClientDoSend(HttpClient *p, AMessage *msg)
{
	p->send_msg.done = &TObjectDone(HttpClient, send_msg, send_from, HttpClientOnSendStatus);
	p->send_from = msg;

	switch (p->send_status)
	{
	case s_invalid:
		p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
		p->send_status = s_send_header;
		break;

	case s_send_private_header:
		break;

	case s_send_chunk_next:
		p->send_msg.init(ioMsgType_Block, p->send_buffer, 0);
		p->send_status = s_send_chunk_size;
		break;

	default: assert(FALSE); return -EACCES;
	}

	int result = HttpClientOnSendStatus(p, 1);
	return result;
}

static int on_m_begin(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;

	p->recv_header_count = 0;
	p->h_f_pos() = 0;
	p->h_f_len() = 0;
	p->h_v_pos() = 0;
	p->h_v_len() = 0;

	release_s(p->recv_header_buffer);
	p->recv_header_pos = 0;
	return 0;
}

static int on_url_or_status(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_f_len() == 0) {
		p->h_v_pos() = p->h_f_pos() = (at - p->recv_buffer->ptr());
	}
	p->h_f_len() += length;
	p->h_v_len() += length;
	return 0;
}

static int on_h_field(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_v_len() != 0) {
		if (++p->recv_header_count >= max_head_count)
			return -EACCES;
		p->h_f_pos() = 0;
		p->h_f_len() = 0;
		p->h_v_pos() = 0;
		p->h_v_len() = 0;
	}

	if (p->h_f_len() == 0)
		p->h_f_pos() = (at - p->recv_buffer->ptr());
	p->h_f_len() += length;
	return 0;
}

static int on_h_value(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->h_v_len() == 0) {
		p->h_v_pos() = (at - p->recv_buffer->ptr());
	}
	p->h_v_len() += length;
	return 0;
}

static int on_h_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_v_len() != 0) {
		p->recv_header_count++;
	}

	p->recv_header_buffer = p->recv_buffer;
	p->recv_header_buffer->addref();
	p->recv_header_pos = p->recv_buffer->_bgn;
	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	assert((at >= p->r_p_ptr()) && (at < p->r_p_ptr()+p->r_p_len()));

	if (p->recv_body_len == 0)
		p->recv_body_pos = (at - p->recv_buffer->ptr());
	p->recv_body_len += length;
	return 0;
}

static int on_m_done(http_parser *parser)
{
	//HttpClient *p = (HttpClient*)parser->data;
	http_parser_pause(parser, TRUE);
	return 0;
}

static int on_chunk_header(http_parser *parser)
{
	//HttpClient *p = (HttpClient*)parser->data;
	TRACE("chunk header, body = %lld.\n", parser->content_length);
	return 0;
}

static int on_chunk_complete(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->recv_body_len != 0)
		http_parser_pause(parser, TRUE);
	else
		; // last chunk size = 0
	return 0;
}

static const struct http_parser_settings cb_sets = {
	&on_m_begin, &on_url_or_status, &on_url_or_status, &on_h_field, &on_h_value,
	&on_h_done, &on_body, &on_m_done,
	&on_chunk_header, &on_chunk_complete
};

int HttpClientOnRecvStatus(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	p->recv_buffer->push(p->recv_msg.size);
_continue:
	if (p->r_p_len() == 0) {
		if (p->recv_buffer->left() == 0) {
			assert(FALSE);
			return -EACCES;
		}

		result = p->io->output(&p->recv_msg, p->recv_buffer);
		if (result <= 0)
			return result;
		p->recv_buffer->push(p->recv_msg.size);
	}

	result = http_parser_execute(&p->recv_parser, &cb_sets, p->r_p_ptr(), p->r_p_len());
	if (p->recv_parser.http_errno == HPE_PAUSED)
		http_parser_pause(&p->recv_parser, FALSE);

	if (p->recv_parser.http_errno != HPE_OK) {
		TRACE("http_errno_name = %s.\n", http_parser_error(&p->recv_parser));
		return -int(AMsgType_Private|p->recv_parser.http_errno);
	}
	p->recv_parser_pos += result;

	result = http_next_chunk_is_incoming(&p->recv_parser);
	if (!result) {
		// new buffer
		assert(p->r_p_len() == 0);
		int reserve = 0;

		if (!http_header_is_complete(&p->recv_parser)) {
			reserve = send_bufsiz;
		}
		else if (p->recv_body_len < max_body_size) {
			if (p->recv_body_len == 0)
				p->recv_body_pos = p->recv_parser_pos;

			p->recv_buffer->pop(p->recv_body_pos);
			p->recv_parser_pos -= p->recv_body_pos;
			p->recv_body_pos = 0;

			reserve = min(p->recv_parser.content_length, max_body_size-p->recv_body_len);
		}
		if (reserve != 0) {
			result = ARefsBuf::reserve(p->recv_buffer, reserve, recv_bufsiz);
			TRACE2("resize buffer size = %d, left = %d, reserve = %d, result = %d.\n",
				p->recv_buffer->len(), p->recv_buffer->left(), reserve, result);
			if (result < 0)
				return result;
			goto _continue;
		}
	}

	p->recv_body_pos += p->recv_buffer->_bgn;
	p->recv_buffer->pop(p->recv_parser_pos);
	p->recv_parser_pos = 0;

	if (p->recv_from->type == AMsgType_RefsMsg) {
		ARefsMsg *rm = (ARefsMsg*)p->recv_from->data;

		ARefsMsgInit(rm, (result==1?ioMsgType_Block:AMsgType_Unknown),
			p->recv_buffer, p->recv_body_pos, p->recv_body_len);
	} else {
		AMsgInit(p->recv_from, (result==1?ioMsgType_Block:AMsgType_Unknown),
			p->recv_buffer->_data+p->recv_body_pos, p->recv_body_len);
	}

	TRACE("status code = %d, header count = %d, body = %d.\n",
		p->recv_parser.status_code, p->recv_header_count, p->recv_body_len);
	if (result == 1) {
	for (int ix = 0; ix < p->recv_header_count; ++ix) {
		TRACE2("http header: %.*s: %.*s\r\n",
			p->h_f_len(ix), p->h_f_ptr(ix), p->h_v_len(ix), p->h_v_ptr(ix));
	}
	}
	return AMsgType_Private|((p->recv_parser.type == HTTP_REQUEST) ? p->recv_parser.method : p->recv_parser.status_code);
}

extern int HttpClientDoRecv(HttpClient *p, AMessage *msg)
{
	if (ARefsBuf::reserve(p->recv_buffer, send_bufsiz, recv_bufsiz) < 0)
		return -ENOMEM;

	p->recv_body_pos = 0;
	p->recv_body_len = 0;

	p->recv_msg.init();
	p->recv_msg.done = &TObjectDone(HttpClient, recv_msg, recv_from, HttpClientOnRecvStatus);
	p->recv_from = msg;

	return HttpClientOnRecvStatus(p, 0);
}

extern int HttpClientAppendOutput(HttpClient *p, AMessage *msg)
{
	if (ARefsBuf::reserve(p->recv_buffer, msg->size, recv_bufsiz) < 0)
		return -ENOMEM;

	p->recv_buffer->mempush(msg->data, msg->size);
	return 1;
}

static int HttpClientRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = (HttpClient*)object;

	if (msg->type == httpMsgType_RawData) {
		msg->type = AMsgType_Unknown;
		return (*p->io)->request(p->io, reqix, msg);
	}

	if (msg->type == httpMsgType_RawBlock) {
		msg->type = ioMsgType_Block;
		return (*p->io)->request(p->io, reqix, msg);
	}

	if (reqix == Aio_Input) {
		return HttpClientDoSend(p, msg);
	}

	if (reqix == Aio_Output) {
		return HttpClientDoRecv(p, msg);
	}

	if (reqix == Aio_AppendOutput) {
		return HttpClientAppendOutput(p, msg);
	}
	return -ENOSYS;
}

static int HttpClientCancel(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = (HttpClient*)object;
	if (p->io == NULL)
		return -ENOENT;
	return (*p->io)->cancel(p->io, reqix, msg);
}

static int HttpClientClose(AObject *object, AMessage *msg)
{
	HttpClient *p = (HttpClient*)object;
	if (p->io == NULL)
		return -ENOENT;
	return p->io->close(msg);
}

IOModule HttpClientModule = { {
	"io",
	"http_client",
	sizeof(HttpClient),
	NULL, NULL,
	&HttpClientCreate,
	&HttpClientRelease, },

	&HttpClientOpen,
	&HttpClientSetOption,
	&IOModule::OptNull,
	&HttpClientRequest,
	&HttpClientCancel,
	&HttpClientClose,
};

static auto_reg_t reg(HttpClientModule.module);
