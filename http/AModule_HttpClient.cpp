#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "http_parser.h"

enum status {
	s_invalid = 0,
	s_send_header,
	s_send_chunk_size,
	s_send_chunk_data,
	s_send_chunk_tail,
	s_send_chunk_next,
	s_send_content_data,
	s_send_done,
};

#define send_bufsiz     2*1024
#define recv_bufsiz     64*1024
#define max_body_size   4*1024*1024

struct HttpClient {
	AObject   object;
	AObject  *io;

	// send
	struct list_head send_headers;
	char      send_buffer[send_bufsiz];
	char      method[32];
	char      url[BUFSIZ];
	char      version[32];

	enum status send_status;
	AMessage  send_msg;
	AMessage *send_from;

	// recv
	struct http_parser recv_parser;
	int       recv_parser_pos;
	char*     r_p_ptr() { return (recv_buffer->ptr() + recv_parser_pos); }
	int       r_p_len() { return (recv_buffer->len() - recv_parser_pos); }

	int       recv_header_list[50][4];
	int       recv_header_count;
	int&      h_f_pos() { return recv_header_list[recv_header_count][0]; }
	int&      h_f_len() { return recv_header_list[recv_header_count][1]; }
	int&      h_v_pos() { return recv_header_list[recv_header_count][2]; }
	int&      h_v_len() { return recv_header_list[recv_header_count][3]; }

	ARefsBuf *recv_header_buffer;
	int       recv_header_pos;
	char*     h_f_data(int ix) { return recv_header_list[ix][0] + recv_header_buffer->data + recv_header_pos; }
	int&      h_f_size(int ix) { return recv_header_list[ix][1]; }
	char*     h_v_data(int ix) { return recv_header_list[ix][2] + recv_header_buffer->data + recv_header_pos; }
	int&      h_v_size(int ix) { return recv_header_list[ix][3]; }

	ARefsBuf *recv_buffer;
	int       recv_body_pos;
	int       recv_body_len;

	AMessage  recv_msg;
	AMessage *recv_from;
};
#define to_http(obj)   container_of(obj, HttpClient, object)

static void HttpClientRelease(AObject *object)
{
	HttpClient *p = to_http(object);
	release_s(p->io, AObjectRelease, NULL);

	AOptionClear(&p->send_headers);
	release_s(p->recv_buffer, ARefsBufRelease, NULL);
	release_s(p->recv_header_buffer, ARefsBufRelease, NULL);
}

static void HttpClientResetStatus(HttpClient *p)
{
	p->send_status = s_invalid;

	http_parser_init(&p->recv_parser, HTTP_BOTH, p);
	p->recv_parser_pos = 0;
	p->recv_header_count = 0;

	release_s(p->recv_buffer, ARefsBufRelease, NULL);
	release_s(p->recv_header_buffer, ARefsBufRelease, NULL);
	p->recv_header_pos = 0;
	p->recv_body_pos = 0;
	p->recv_body_len = 0;
}

static int HttpClientCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpClient *p = (HttpClient*)*object;
	p->io = NULL;

	INIT_LIST_HEAD(&p->send_headers);
	p->method[0] = '\0';
	p->url[0] = '\0';
	p->version[0] = '\0';

	p->recv_buffer = NULL;
	p->recv_header_buffer = NULL;
	HttpClientResetStatus(p);

	AOption *io_opt = AOptionFind(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&p->io, &p->object, io_opt, NULL);
	return 1;
}

static int HttpClientSetMethod(HttpClient *p, AOption *option)
{
	int ix = 0;
	for (const char *m = NULL;
		((m = http_method_str((enum http_method)ix)) != NULL) && (m[0] != '<');
		++ix)
	{
		if (_stricmp(option->value, m) == 0) {
			strcpy_sz(p->method, m);
			return ix;
		}
	}
	return -1;
}

static int HttpClientSetHeader(HttpClient *p, AOption *option)
{
	AOption *header = NULL;
	if (option->name[0] == ':')
		header = AOptionFind2(&p->send_headers, option->name);

	if (header != NULL) {
		strcpy_sz(header->value, option->value);
	} else {
		header = AOptionClone2(option, &p->send_headers);
		if (header == NULL)
			return -ENOMEM;
	}
	return 0;
}

int HttpClientOnOpen(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	AOption *option = (AOption*)p->send_from->data;
	const char *str;

	// method
	str = AOptionChild(option, "method", http_method_str(HTTP_GET));
	strcpy_sz(p->method, str);

	// url
	str = AOptionChild(option, "url", "/");
	strcpy_sz(p->url, str);

	// version
	str = AOptionChild(option, "version", "HTTP/1.0");
	strcpy_sz(p->version, str);

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
	HttpClient *p = to_http(object);

	if ((msg->type == AMsgType_Object)
	 && (msg->size == 0)) {
		release_s(p->io, AObjectRelease, NULL);

		p->io = (AObject*)msg->data;
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
		int result = AObjectCreate(&p->io, &p->object, io_opt, "tcp");
		if (result < 0)
			return result;
	}

	AMsgInit(&p->send_msg, AMsgType_Option, io_opt, 0);
	p->send_msg.done = &TObjectDone(HttpClient, send_msg, send_from, HttpClientOnOpen);
	p->send_from = msg;

	int result = p->io->open(p->io, &p->send_msg);
	if (result != 0)
		result = HttpClientOnOpen(p, result);
	return result;
}

static int HttpClientSetOption(AObject *object, AOption *option)
{
	HttpClient *p = to_http(object);

	if ((option->name[0] == ':') || (option->name[0] == '+'))
		return HttpClientSetHeader(p, option);

	if (_stricmp(option->name, "method") == 0)
		return HttpClientSetMethod(p, option);

	return -ENOSYS;
}

#define append_data(fmt, ...) \
	p->send_msg.size += snprintf(p->send_buffer+p->send_msg.size, send_bufsiz-p->send_msg.size, fmt, ##__VA_ARGS__)

#define append_crlf() \
	p->send_buffer[p->send_msg.size++] = '\r'; \
	p->send_buffer[p->send_msg.size++] = '\n';

int HttpClientOnSendStatus(HttpClient *p, int result)
{
	AMessage *msg = p->send_from;
	do {
		switch (p->send_status)
		{
		case s_send_header:
			append_data("%s %s %s\r\n", p->method, p->url, p->version);

			AOption *pos;
			list_for_each_entry(pos, &p->send_headers, AOption, brother_entry)
			{
				if ((pos->name[0] != ':') && (pos->name[0] != '+'))
					continue;
				if (pos->value[0] == '\0')
					continue;

				if ((_stricmp(pos->name+1, "Transfer-Encoding") == 0)
				 && (_stricmp(pos->value, "chunked") == 0))
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
			result = ioInput(p->io, &p->send_msg);
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
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_chunk_data:
			AMsgInit(&p->send_msg, ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_chunk_tail;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_chunk_tail:
			AMsgInit(&p->send_msg, ioMsgType_Block, p->send_buffer, 0);
			append_crlf();

			p->send_status = s_send_chunk_next;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_content_data:
			AMsgInit(&p->send_msg, ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_done;
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_done:
			p->send_status = s_invalid;
		case s_send_chunk_next:
			return result;

		default:
			assert(FALSE);
			return -EACCES;
		}
	} while (result > 0);
	return result;
}

static int HttpClientDoSendRequest(HttpClient *p, AMessage *msg)
{
	AMsgInit(&p->send_msg, ioMsgType_Block, p->send_buffer, 0);
	p->send_msg.done = &TObjectDone(HttpClient, send_msg, send_from, HttpClientOnSendStatus);
	p->send_from = msg;

	if (p->send_status == s_invalid) {
		p->send_status = s_send_header;
	} else if (p->send_status == s_send_chunk_next) {
		p->send_status = s_send_chunk_size;
	} else {
		assert(FALSE);
		return -EACCES;
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

	release_s(p->recv_header_buffer, ARefsBufRelease, NULL);
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
		if (++p->recv_header_count >= _countof(p->recv_header_list))
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

	if (p->h_v_len() == 0)
		p->h_v_pos() = (at - p->recv_buffer->ptr());
	p->h_v_len() += length;
	return 0;
}

static int on_h_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_f_len() != 0)
		p->recv_header_count++;

	p->recv_header_buffer = p->recv_buffer;
	ARefsBufAddRef(p->recv_header_buffer);
	p->recv_header_pos = p->recv_buffer->bgn;
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

		result = ioOutput(p->io, &p->recv_msg, p->recv_buffer);
		if (result <= 0)
			return result;
		p->recv_buffer->push(p->recv_msg.size);
	}

	result = http_parser_execute(&p->recv_parser, &cb_sets, p->r_p_ptr(), p->r_p_len());
	if (p->recv_parser.http_errno == HPE_PAUSED)
		http_parser_pause(&p->recv_parser, FALSE);

	if (p->recv_parser.http_errno != HPE_OK) {
		TRACE("http_errno_name = %s.\n", http_parser_error(&p->recv_parser));
		return -(AMsgType_Private|p->recv_parser.http_errno);
	}
	p->recv_parser_pos += result;

	result = http_next_chunk_is_incoming(&p->recv_parser);
	if (!result) {
		// new buffer
		assert(p->r_p_len() == 0);
		char *buf_ptr = p->recv_buffer->ptr();
		int buf_size = 0;

		if (!http_header_is_complete(&p->recv_parser)) {
			if (p->recv_buffer->left() >= send_bufsiz)
				goto _continue;

			buf_size = p->recv_buffer->len() + recv_bufsiz;
		}
		else if (p->recv_body_len+p->recv_parser.content_length < max_body_size) {
			if (p->recv_buffer->left() >= p->recv_parser.content_length + 32)
				goto _continue;

			buf_size = p->recv_body_len + p->recv_parser.content_length + send_bufsiz;
			if (buf_size < recv_bufsiz)
				buf_size = recv_bufsiz;

			if (p->recv_body_len == 0)
				p->recv_body_pos = p->recv_parser_pos;
			buf_ptr += p->recv_body_pos;
			p->recv_parser_pos -= p->recv_body_pos;
			p->recv_body_pos = 0;
		}
		if (buf_size != 0) {
			TRACE("resize buffer, %d => %d.\n", p->recv_buffer->len(), buf_size);
			ARefsBuf *buf = ARefsBufCreate(buf_size, NULL, NULL);
			if (buf == NULL)
				return -ENOMEM;

			buf->mempush(buf_ptr, p->recv_buffer->next()-buf_ptr);
			ARefsBufRelease(p->recv_buffer);
			p->recv_buffer = buf;
			goto _continue;
		}
	}

	p->recv_body_pos += p->recv_buffer->bgn;
	p->recv_buffer->pop(p->recv_parser_pos);
	p->recv_parser_pos = 0;

	if (p->recv_from->type == AMsgType_RefsMsg) {
		ARefsMsg *rm = (ARefsMsg*)p->recv_from->data;

		ARefsMsgInit(rm, (result==1?ioMsgType_Block:AMsgType_Unknown),
			p->recv_buffer, p->recv_body_pos, p->recv_body_len);
	} else {
		AMsgInit(p->recv_from, (result==1?ioMsgType_Block:AMsgType_Unknown),
			p->recv_buffer->data+p->recv_body_pos, p->recv_body_len);
	}

	TRACE2("status code = %d, header count = %d, body = %lld.\n",
		p->recv_parser.status_code, p->recv_header_count, p->recv_parser.content_length);
#ifdef _DEBUG
	char h_f[64];
	char h_v[BUFSIZ];
	for (int ix = 0; ix < p->recv_header_count; ++ix)
	{
		strncpy_sz(h_f, p->h_f_data(ix), p->h_f_size(ix));
		strncpy_sz(h_v, p->h_v_data(ix), p->h_v_size(ix));
		TRACE2("http header: %s: %s\r\n", h_f, h_v);
	}
#endif
	return AMsgType_Private|((p->recv_parser.type == HTTP_REQUEST) ? p->recv_parser.method : p->recv_parser.status_code);
}

static int HttpClientDoRecvResponse(HttpClient *p, AMessage *msg)
{
	if (ARefsBufCheck(p->recv_buffer, send_bufsiz, recv_bufsiz) < 0)
		return -ENOMEM;

	p->recv_body_pos = 0;
	p->recv_body_len = 0;

	AMsgInit(&p->recv_msg, AMsgType_Unknown, NULL, 0);
	p->recv_msg.done = &TObjectDone(HttpClient, recv_msg, recv_from, HttpClientOnRecvStatus);
	p->recv_from = msg;

	return HttpClientOnRecvStatus(p, 0);
}

static int HttpClientAppendOutput(HttpClient *p, AMessage *msg)
{
	if (ARefsBufCheck(p->recv_buffer, msg->size, recv_bufsiz) < 0)
		return -ENOMEM;

	p->recv_buffer->mempush(msg->data, msg->size);
	return 1;
}

static int HttpClientRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = to_http(object);

	if (msg->type == httpMsgType_RawData) {
		msg->type = AMsgType_Unknown;
		return p->io->request(p->io, reqix, msg);
	}

	if (msg->type == httpMsgType_RawBlock) {
		msg->type = ioMsgType_Block;
		return p->io->request(p->io, reqix, msg);
	}

	if (reqix == Aio_Input) {
		return HttpClientDoSendRequest(p, msg);
	}

	if (reqix == Aio_Output) {
		return HttpClientDoRecvResponse(p, msg);
	}

	if (reqix == Aio_AppendOutput) {
		return HttpClientAppendOutput(p, msg);
	}
	return -ENOSYS;
}

static int HttpClientCancel(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = to_http(object);
	if (p->io == NULL)
		return -ENOENT;

	return p->io->cancel(p->io, reqix, msg);
}

static int HttpClientClose(AObject *object, AMessage *msg)
{
	HttpClient *p = to_http(object);
	if (p->io == NULL)
		return -ENOENT;

	return p->io->close(p->io, msg);
}

AModule HttpClientModule = {
	"io",
	"http_client",
	sizeof(HttpClient),
	NULL, NULL,
	&HttpClientCreate,
	&HttpClientRelease,
	NULL,
	2,

	&HttpClientOpen,
	NULL,
	NULL,
	&HttpClientRequest,
	&HttpClientCancel,
	&HttpClientClose,
};

