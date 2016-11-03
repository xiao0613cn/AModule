#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "http_parser.h"


struct HttpClient {
	AObject   object;
	AObject  *io;
	ARefsBuf *buf;
	AOption   request_headers;
	char      url[BUFSIZ];
	char      version[32];

	http_parser parser;
	http_parser_settings cbset;
	AMessage  msg;
	AMessage *from;
	int       reqix;
	int       resp_size;
	int       content_offset;
};
#define to_http(obj)   container_of(obj, HttpClient, object)

static void HttpClientRelease(AObject *object)
{
	HttpClient *p = to_http(object);
	release_s(p->io, AObjectRelease, NULL);
	release_s(p->buf, ARefsBufRelease, NULL);
	AOptionExit(&p->request_headers);
	free(p);
}

static int HttpClientCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpClient *p = (HttpClient*)*object;
	p->io = NULL;
	p->buf = ARefsBufCreate(1024*1024, NULL, NULL);
	AOptionInit(&p->request_headers, NULL);
	if (p->buf == NULL)
		return -ENOMEM;

	p->content_offset = 0;
	strcpy_s(p->request_headers.name, "request_headers");

	AOption *io_opt = AOptionFind(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&p->io, &p->object, io_opt, NULL);
	return 1;
}

int HttpClientOnOpen(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	p->parser.data = p;
	http_parser_init(&p->parser, HTTP_RESPONSE);
	http_parser_settings_init(&p->cbset);

	AOption *option = (AOption*)p->from->data;

	// method
	p->parser.method = HTTP_GET;
	const char *str = AOptionChild(option, "method");
	if (str != NULL)
	{
		int ix = 0;
		for (const char *method = NULL;
			((method = http_method_str((enum http_method)ix)) != NULL) && (method[0] != '<');
			++ix)
		{
			if (_stricmp(str, method) == 0) {
				p->parser.method = ix;
				break;
			}
		}
	}

	// url
	str = AOptionChild(option, "url");
	if ((str == NULL) || (str[0] == '\0'))
		str = "/";
	strcpy_s(p->url, str);

	// version
	str = AOptionChild(option, "version");
	if ((str == NULL) || (str[0] == '\0'))
		str = "HTTP/1.1";
	strcpy_s(p->version, str);

	// request_headers
	AOption *pos;
	list_for_each_entry(pos, &option->children_list, AOption, brother_entry) {
		if ((pos->name[0] != ':') && (pos->name[0] != '+'))
			continue;

		AOption *header = NULL;
		if (pos->name[0] == ':')
			header = AOptionFind(&p->request_headers, pos->name);

		if (header != NULL) {
			strcpy_s(header->value, pos->value);
		} else {
			header = AOptionClone(pos, &p->request_headers);
			if (header == NULL)
				return -ENOMEM;
		}
	}
	return result;
}

static int HttpClientOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	HttpClient *p = to_http(object);
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

int HttpClientOnResponse(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	do {
		p->resp_size += p->msg.size;
		do {
			result = http_parser_execute(&p->parser, &p->cbset, p->msg.data, p->msg.size);
			if (result == 0) {
				//return -EILSEQ;
				break;
			}

			p->msg.data += result;
			p->msg.size -= result;
		} while (p->msg.size > 0);

		if (p->parser.http_errno != HPE_OK)
			return -(AMsgType_Class|p->parser.http_errno);

		if (http_body_is_final(&p->parser)) {
			AMsgInit(p->from, AMsgType_Unknown, p->buf->data+p->content_offset, p->parser.content_length);
			return p->parser.status_code;
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

	AMsgInit(&p->msg, AMsgType_Unknown, p->buf->data, p->buf->size);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnResponse);
	p->resp_size = 0;

	result = ioOutput(p->io, &p->msg);
	if (result != 0)
		result = HttpClientOnResponse(p, result);
	return result;
}

int HttpClientOnRequest(HttpClient *p, int result)
{
	if (result < 0)
		return result;

	if ((p->reqix == Aio_Input) && (p->from->size != 0))
	{
		AMsgInit(&p->msg, ioMsgType_Block, p->from->data, p->from->size);
		p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnSendContent);

		result = ioInput(p->io, &p->msg);
		if (result == 0)
			return 0;
	}
	return HttpClientOnSendContent(p, result);
}

static int HttpClientRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpClient *p = to_http(object);
	if ((reqix != Aio_Input) && (reqix != Aio_Output))
		return -ENOSYS;

	int reqlen = snprintf(p->buf->data, p->buf->size, "%s %s %s\r\n",
		http_method_str((enum http_method)p->parser.method), p->url, p->version);

	AOption *pos;
	list_for_each_entry(pos, &p->request_headers.children_list, AOption, brother_entry) {
		if ((pos->name[0] != ':') && (pos->name[0] != '+'))
			continue;
		if (pos->value[0] == '\0')
			continue;

		reqlen += snprintf(p->buf->data+reqlen, p->buf->size-reqlen,
			"%s: %s\r\n", pos->name+1, pos->value);
	}

	if ((reqix == Aio_Input) && (msg->size != 0)) {
		reqlen += snprintf(p->buf->data+reqlen, p->buf->size-reqlen,
			"Content-Length: %d\r\n", msg->size);
	}
	if (reqlen+2 >= p->buf->size)
		return -ENOMEM;
	strcpy(p->buf->data+reqlen, "\r\n");
	reqlen += 2;

	AMsgInit(&p->msg, ioMsgType_Block, p->buf->data, reqlen);
	p->msg.done = &TObjectDone(HttpClient, msg, from, HttpClientOnRequest);
	p->from = msg;

	p->reqix = reqix;
	int result = ioInput(p->io, &p->msg);
	if (result != 0)
		result = HttpClientOnRequest(p, result);
	return result;
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

