#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "http_parser.h"

enum status {
	s_invalid = 0,
	s_send_header,
	s_send_chunk_header,
	s_send_chunk_data,
	s_send_content_data,
};

struct HttpClient {
	AObject   object;
	AObject  *io;
	ARefsBuf *buf;
	struct list_head request_headers;
	char      url[BUFSIZ];
	char      version[32];
	int       send_chunk;

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
	p->send_chunk = 0;

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
			p->parser.method = ix;
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

	// method
	p->parser.method = HTTP_GET;
	AOption *opt = AOptionFind(option, "method");
	if (opt != NULL) {
		HttpClientSetMethod(p, opt);
	}

	// url
	const char *str = AOptionChild(option, "url");
	if ((str == NULL) || (str[0] == '\0'))
		str = "/";
	strcpy_sz(p->url, str);

	// version
	str = AOptionChild(option, "version");
	if ((str == NULL) || (str[0] == '\0'))
		str = "HTTP/1.1";
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
	&on_m_begin, /*&on_url*/NULL, &on_status, &on_h_field, &on_h_value,
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

int HttpClientOnSendContent(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	p->parser.data = p;
	http_parser_init(&p->parser, HTTP_RESPONSE);
	p->resp_size = 0;
	p->content_length = 0;

	AMsgInit(&p->msg, AMsgType_Unknown, p->buf->data, p->buf->size);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnResponse);

	result = ioOutput(p->io, &p->msg);
	if (result != 0)
		result = HttpClientOnResponse(p, result);
	return result;
}

static int HttpClientOnSendChunk(HttpClient *p, int result)
{
	if (p->from->size == 0) {
		p->send_chunk = 0;
		return result;
	}

	do {
		switch (p->send_chunk)
		{
		case 1:
			AMsgInit(&p->msg, ioMsgType_Block, p->from->data, p->from->size);

			p->send_chunk = 2;
			result = ioInput(p->io, &p->msg);
			break;
		case 2:
			AMsgInit(&p->msg, ioMsgType_Block, p->buf->data, 2);
			p->buf->data[0] = '\r';
			p->buf->data[1] = '\n';

			p->send_chunk = 3;
			result = ioInput(p->io, &p->msg);
			break;
		case 3:
			p->send_chunk = 1;
			return p->from->size;
		default:
			assert(FALSE);
			result = -EACCES;
			break;
		}
	} while (result > 0);
	return result;
}

int HttpClientOnSendRequest(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	if (p->send_chunk != 0)
		return HttpClientOnSendChunk(p, result);

	if (p->from->size != 0) {
		AMsgInit(&p->msg, ioMsgType_Block, p->from->data, p->from->size);
		p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnSendContent);

		result = ioInput(p->io, &p->msg);
		if (result == 0)
			return 0;
	}
	return HttpClientOnSendContent(p, result);
}

static int HttpClientDoSendChunk(HttpClient *p, AMessage *msg)
{
	assert(p->send_chunk == 1);
	if (msg->size < 0) {
		p->send_chunk = 0;
		return 1;
	}

	p->msg.size = snprintf(p->buf->data, p->buf->size, "%x\r\n", msg->size);
	if (msg->size == 0) {
		p->buf->data[p->msg.size++] = '\r';
		p->buf->data[p->msg.size++] = '\n';
	}

	p->send_chunk = 1;
	int result = ioInput(p->io, &p->msg);
	if (result != 0)
		result = HttpClientOnSendRequest(&p->msg, result);
	return result;
}

static int HttpClientDoSendRequest(HttpClient *p, AMessage *msg)
{
	if (p->buf == NULL) {
		p->buf = ARefsBufCreate(8*1024, NULL, NULL);
		if (p->buf == NULL)
			return -ENOMEM;
	}

	AMsgInit(&p->msg, ioMsgType_Block, p->buf->data, 0);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnSendRequest);
	p->from = msg;

	if (p->send_chunk != 0)
		return HttpClientDoSendChunk(p, msg);

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
			p->send_chunk = 1;

		p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size,
			"%s: %s\r\n", pos->name+1, pos->value);
	}

	if (p->send_chunk != 0) {
		p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size,
			"%x\r\n", msg->size);
	} else if (msg->size != 0) {
		p->msg.size += snprintf(p->buf->data+p->msg.size, p->buf->size-p->msg.size,
			"Content-Length: %d\r\n", msg->size);
	}
	p->buf->data[p->msg.size++] = '\r';
	p->buf->data[p->msg.size++] = '\n';

	int result = ioInput(p->io, &p->msg);
	if (result != 0)
		result = HttpClientOnSendRequest(p, result);
	return result;
}

static int HttpClientRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = to_http(object);

	if (reqix == Aio_Input) {
		if (msg->type == (AMsgType_Private|1)) {
			msg->type = ioMsgType_Block;
			return ioInput(p->io, msg);
		}
		return HttpClientDoSendRequest(p, msg);
	}

	if (reqix == Aio_Output) {
		return HttpClientDoRecvResponse(p, msg);
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

