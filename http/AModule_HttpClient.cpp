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

#define header_bufsiz   4*1024

struct HttpClient {
	AObject   object;
	AObject  *io;

	// send
	struct list_head send_headers;
	char      send_buffer[header_bufsiz];
	char      method[32];
	char      url[BUFSIZ];
	char      version[32];

	enum status send_status;
	AMessage  send_msg;
	AMessage *send_from;

	// recv
	struct http_parser recv_parser;
	char      recv_buffer[header_bufsiz];
	int       recv_parser_pos;
	int       recv_parser_size;

	int       recv_headers[50][4];
	int       recv_header_count;
	int&      h_f_pos() { return recv_headers[recv_header_count][0]; }
	int&      h_f_len() { return recv_headers[recv_header_count][1]; }
	int&      h_v_pos() { return recv_headers[recv_header_count][2]; }
	int&      h_v_len() { return recv_headers[recv_header_count][3]; }
	ARefsBuf *recv_content;

	AMessage  recv_msg;
	AMessage *recv_from;
};
#define to_http(obj)   container_of(obj, HttpClient, object)

static void HttpClientRelease(AObject *object)
{
	HttpClient *p = to_http(object);
	release_s(p->io, AObjectRelease, NULL);
	AOptionClear(&p->send_headers);
	release_s(p->recv_content, ARefsBufRelease, NULL);
}

static int HttpClientCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpClient *p = (HttpClient*)*object;
	p->io = NULL;

	INIT_LIST_HEAD(&p->send_headers);
	p->method[0] = '\0';
	p->url[0] = '\0';
	p->version[0] = '\0';
	p->send_status = s_invalid;

	p->recv_parser.data = p;
	http_parser_init(&p->recv_parser, HTTP_BOTH);
	p->recv_parser_pos = 0;
	p->recv_parser_size = 0;

	p->recv_header_count = 0;
	p->recv_content = NULL;

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
	str = AOptionChild(option, "method");
	if ((str == NULL) || (str[0] == '\0'))
		str = http_method_str(HTTP_GET);
	strcpy_sz(p->method, str);

	// url
	str = AOptionChild(option, "url");
	if ((str == NULL) || (str[0] == '\0'))
		str = "/";
	strcpy_sz(p->url, str);

	// version
	str = AOptionChild(option, "version");
	if ((str == NULL) || (str[0] == '\0'))
		str = "HTTP/1.0";
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
	p->send_msg.size += snprintf(p->send_buffer+p->send_msg.size, header_bufsiz-p->send_msg.size, fmt, ##__VA_ARGS__)

#define append_crlf() \
	p->send_buffer[p->send_msg.size++] = '\r'; \
	p->send_buffer[p->send_msg.size++] = '\n';

static int HttpClientSendStatus(HttpClient *p, int result)
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

				if ((strnicmp_c(pos->name+1, "Transfer-Encoding") == 0)
				 && (strnicmp_c(pos->value, "chunked") == 0))
					p->send_status = s_send_chunk_size;

				append_data("%s: %s\r\n", pos->name+1, pos->value);
			}
			if (p->send_msg.size+64 >= header_bufsiz)
				return -ENOMEM;

			if (p->send_status == s_send_chunk_size) {
				append_crlf();
				break;
			}

			if (msg->size != 0) {
				append_data("Content-Length: %d\r\n\r\n", msg->size);
				p->send_status = s_send_content_data;
			} else {
				append_crlf();
				p->send_status = s_send_done;
			}
			result = ioInput(p->io, &p->send_msg);
			break;

		case s_send_chunk_size:
			append_data("%x\r\n", msg->size);
			if (msg->size != 0) {
				p->send_status = s_send_chunk_data;
			} else {
				append_crlf();
				p->send_status = s_send_done;
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
	p->send_msg.done = &TObjectDone(HttpClient, send_msg, send_from, HttpClientSendStatus);
	p->send_from = msg;

	if (p->send_status == s_invalid) {
		p->send_status = s_send_header;
	} else if (p->send_status == s_send_chunk_next) {
		p->send_status = s_send_chunk_size;
	} else {
		assert(FALSE);
		return -EACCES;
	}

	int result = HttpClientSendStatus(p, 1);
	return result;
}

static int HttpClientResetContent(HttpClient *p, int content_length)
{
	if ((p->recv_content == NULL) || (p->recv_content->size < content_length)) {
		release_s(p->recv_content, ARefsBufRelease, NULL);

		p->recv_content = ARefsBufCreate(content_length, NULL, NULL);
		if (p->recv_content == NULL)
			return -ENOMEM;
	}

	p->recv_content->bgn = 0;
	p->recv_content->end = content_length;
	return 0;
}

static int on_m_begin(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	p->recv_header_count = 0;
	p->h_f_pos() = 0;
	p->h_f_len() = 0;
	p->h_v_pos() = 0;
	p->h_v_len() = 0;
	return 0;
}

static int on_url(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static int on_status(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static int on_h_field(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_v_len() != 0) {
		if (++p->recv_header_count >= _countof(p->recv_headers))
			return -EACCES;
	}

	if (p->h_f_pos() == 0)
		p->h_f_pos() = (at - p->recv_buffer);
	p->h_f_len() += length;

	ASSERT(at >= p->recv_buffer+p->h_f_pos());
	ASSERT(at < p->recv_buffer+p->h_f_pos()+p->h_f_len())
	return 0;
}

static int on_h_value(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_v_pos() == 0)
		p->h_v_pos() = (at - p->recv_buffer);
	p->h_v_len() += length;

	ASSERT(at >= p->recv_buffer+p->h_v_pos());
	ASSERT(at < p->recv_buffer+p->h_v_pos()+p->h_v_len())
	return 0;
}

static int on_h_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	if (p->h_f_len() != 0)
		p->recv_header_count++;

	if ((parser->content_length > 0) && (parser->content_length != ULLONG_MAX)) {
		HttpClientResetContent(p, parser->content_length);
	} else if (p->recv_content != NULL) {
		p->recv_content->reset();
	}
	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	if ((p->recv_content == NULL) || (p->recv_content->bgn+length >= p->recv_content->end))
		return -EACCES;

	if ((at < p->recv_content->data) || (at >= p->recv_content->next()))
		memcpy(p->recv_content->data, at, length);

	p->recv_content->push(length);
	return 0;
}

static int on_m_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static int on_chunk_header(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	return HttpClientResetContent(p, parser->content_length);
}

static int on_chunk_complete(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static const struct http_parser_settings cb_sets = {
	&on_m_begin, &on_url, &on_status, &on_h_field, &on_h_value,
	&on_h_done, &on_body, &on_m_done,
	&on_chunk_header, &on_chunk_complete
};

static int HttpClientDoRecvResponse(HttpClient *p, int result)
{
	if (result < 0)
		return result;
	do {
		p->recv_parser_size += p->recv_msg.size;
		if (p->recv_parser_pos == p->recv_parser_size) {
			AMsgInit(&p->recv_msg, AMsgType_Unknown, p->recv_buffer+p->recv_parser_size, sizeof(p->recv_buffer)-p->recv_parser_size);

			result = ioOutput(p->io, &p->recv_msg);
			continue;
		}

		result = http_parser_execute(&p->recv_parser, &cb_sets, p->recv_buffer+p->recv_parser_pos, p->recv_parser_size-p->recv_parser_pos);
		if (p->recv_parser.http_errno != HPE_OK) {
			TRACE("http_errno_name = %s.\n", http_parser_error(&p->recv_parser));
			return -(AMsgType_Private|p->recv_parser.http_errno);
		}

		p->recv_parser_pos += result;
		if (!p->recv_response_completed) {
			p->recv_msg.size = 0;
			continue;
		}

		if (p->recv_from->type == AMsgType_RefsMsg) {
			ARefsMsg *rm = (ARefsMsg*)p->recv_from;

			rm->buf = p->recv_content;
			if (p->recv_content != NULL)
				ARefsBufAddRef(p->recv_content);

			rm->type = ioMsgType_Block;
			rm->pos = (p->recv_content ? p->recv_content->bgn : 0);
			rm->size = (p->recv_content ? p->recv_content->end : 0);
		} else if (p->recv_content != NULL) {
			AMsgCopy(p->recv_from, ioMsgType_Block, p->recv_content->ptr(), p->recv_content->len());
		} else {
			AMsgInit(p->recv_from, AMsgType_Unknown, NULL, 0);
		}
		return p->recv_parser.status_code;
	} while (result > 0);
	return result;
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
		p->recv_msg.size = 0;
		p->recv_msg.done = &TObjectDone(HttpClient, recv_msg, recv_from, HttpClientDoRecvResponse);
		p->recv_from = msg;
		return HttpClientDoRecvResponse(p, 0);
	}

	/*if (reqix == Aio_InOutPair) {
		if (msg->type != AMsgType_InOutMsg)
			return -EINVAL;
		return HttpClientDoSendRequest(p, msg);
	}*/
	return -ENOSYS;
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
	NULL,
	&HttpClientClose,
};

