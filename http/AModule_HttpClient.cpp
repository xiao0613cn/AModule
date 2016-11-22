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

struct HttpClient {
	AObject   object;
	AObject  *io;

	ARefsBuf *buf;
	struct list_head request_headers;
	char      method[32];
	char      url[BUFSIZ];
	char      version[32];
	enum status send_status;

	//std::vector<ARefsMsg> response_headers;
	http_parser parser;
	AMessage  msg;
	AMessage *from;
	int       resp_size;
	int       content_length;
};
#define to_http(obj)   container_of(obj, HttpClient, object)

static void HttpClientRelease(AObject *object)
{
	HttpClient *p = to_http(object);
	release_s(p->io, AObjectRelease, NULL);
	release_s(p->buf, ARefsBufRelease, NULL);
	AOptionClear(&p->request_headers);
}

static int HttpClientCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpClient *p = (HttpClient*)*object;
	p->io = NULL;
	p->buf = NULL;
	INIT_LIST_HEAD(&p->request_headers);
	p->send_status = s_invalid;

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
		header = AOptionFind2(&p->request_headers, option->name);

	if (header != NULL) {
		strcpy_sz(header->value, option->value);
	} else {
		header = AOptionClone2(option, &p->request_headers);
		if (header == NULL)
			return -ENOMEM;
	}
	return 0;
}

int HttpClientOnOpen(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	AOption *option = (AOption*)p->from->data;
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

	// request_headers
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

	AMsgInit(&p->msg, AMsgType_Option, io_opt, 0);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnOpen);
	p->from = msg;

	int result = p->io->open(p->io, &p->msg);
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

static int on_m_begin(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
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
	return 0;
}

static int on_h_value(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static int on_h_done(http_parser *parser)
{
	HttpClient *p = (HttpClient*)parser->data;
	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t length)
{
	HttpClient *p = (HttpClient*)parser->data;
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
	return 0;
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

int HttpClientOnResponse(HttpClient *p, int result)
{
	if (result < 0)
		return result;
	do {
		p->resp_size += p->msg.size;
		p->msg.data[p->msg.size] = '\0';

		result = http_parser_execute(&p->parser, &cb_sets, p->msg.data, p->msg.size);
		if (p->parser.http_errno != HPE_OK) {
			TRACE("http_errno_name = %s.\n", http_parser_error(&p->parser));
			return -(AMsgType_Private|p->parser.http_errno);
		}

		if (p->resp_size+16*1024 >= p->buf->size) {
			ARefsBuf *buf = ARefsBufCreate(p->buf->size+16*1024, NULL, NULL);
			if (buf == NULL)
				return -ENOMEM;

			memcpy(buf->data, p->buf->data, p->resp_size);
			ARefsBufRelease(p->buf);
			p->buf = buf;
		}

		AMsgInit(&p->msg, AMsgType_Unknown, p->buf->data+p->resp_size, p->buf->size-p->resp_size);
		result = ioOutput(p->io, &p->msg);
	} while (result > 0);
	return result;
}

#define append_crlf() \
	p->buf->data[p->msg.size++] = '\r'; \
	p->buf->data[p->msg.size++] = '\n';

static int HttpClientSendStatus(HttpClient *p, int result)
{
	AMessage *msg = p->from;
	do {
		switch (p->send_status)
		{
		case s_send_header:
			p->msg.size = snprintf(p->buf->data, p->buf->size, "%s %s %s\r\n",
				http_parser_method(&p->parser), p->url, p->version);

			AOption *pos;
			list_for_each_entry(pos, &p->request_headers, AOption, brother_entry)
			{
				if ((pos->name[0] != ':') && (pos->name[0] != '+'))
					continue;
				if (pos->value[0] == '\0')
					continue;

				if ((strnicmp_c(pos->name+1, "Transfer-Encoding") == 0)
				 && (strnicmp_c(pos->value, "chunked") == 0))
					p->send_status = s_send_chunk_size;

				p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size,
					"%s: %s\r\n", pos->name+1, pos->value);
			}
			if (p->msg.size+64 >= p->buf->size)
				return -ENOMEM;

			if (p->send_status == s_send_chunk_size) {
				append_crlf();
				break;
			}

			if (msg->size != 0) {
				p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size,
					"Content-Length: %d\r\n\r\n", msg->size);
				p->send_status = s_send_content_data;
			} else {
				append_crlf();
				p->send_status = s_send_done;
			}
			result = ioInput(p->io, &p->msg);
			break;

		case s_send_chunk_size:
			p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size, "%x\r\n", msg->size);
			if (msg->size != 0) {
				p->send_status = s_send_chunk_data;
			} else {
				append_crlf();
				p->send_status = s_send_done;
			}
			result = ioInput(p->io, &p->msg);
			break;

		case s_send_chunk_data:
			AMsgInit(&p->msg, ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_chunk_tail;
			result = ioInput(p->io, &p->msg);
			break;

		case s_send_chunk_tail:
			AMsgInit(&p->msg, ioMsgType_Block, p->buf->data, 0);
			append_crlf();

			p->send_status = s_send_chunk_next;
			result = ioInput(p->io, &p->msg);
			break;

		case s_send_content_data:
			AMsgInit(&p->msg, ioMsgType_Block, msg->data, msg->size);

			p->send_status = s_send_done;
			result = ioInput(p->io, &p->msg);
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
	if (msg->type == httpMsgType_RawData) {
		msg->type = AMsgType_Unknown;
		return ioInput(p->io, msg);
	}

	if (msg->type == httpMsgType_RawBlock) {
		msg->type = ioMsgType_Block;
		return ioInput(p->io, msg);
	}

	if (p->buf == NULL) {
		p->buf = ARefsBufCreate(8*1024, NULL, NULL);
		if (p->buf == NULL)
			return -ENOMEM;
	}

	AMsgInit(&p->msg, ioMsgType_Block, p->buf->data, 0);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientSendStatus);
	p->from = msg;

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

static int HttpClientRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = to_http(object);

	if (reqix == Aio_Input) {
		return HttpClientDoSendRequest(p, msg);
	}

	if (reqix == Aio_Output) {
		//return HttpClientDoRecvResponse(p, msg);
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

