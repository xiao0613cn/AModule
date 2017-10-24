#ifndef _AMODULE_HTTPCLIENT_H_
#define _AMODULE_HTTPCLIENT_H_

#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "http_parser.h"

enum status {
	s_invalid = 0,
	s_send_header,
	s_send_private_header,
	s_send_chunk_size,
	s_send_chunk_data,
	s_send_chunk_tail,
	s_send_chunk_next,
	s_send_content_data,
	s_send_done,
};

#define send_bufsiz     2*1024
#define recv_bufsiz     64*1024
#define max_body_size   16*1024*1024
#define max_head_count  50

struct HttpClient : public IOObject {
	IOObject  *io;

	// send
	AOption   send_headers;
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

	int       recv_header_list[max_head_count][4];
	int       recv_header_count;
	int&      h_f_pos() { return recv_header_list[recv_header_count][0]; }
	int&      h_f_len() { return recv_header_list[recv_header_count][1]; }
	int&      h_v_pos() { return recv_header_list[recv_header_count][2]; }
	int&      h_v_len() { return recv_header_list[recv_header_count][3]; }

	ARefsBuf *recv_header_buffer;
	int       recv_header_pos;
	char*     h_f_ptr(int ix) { return recv_header_list[ix][0] + recv_header_buffer->_data + recv_header_pos; }
	int&      h_f_pos(int ix) { return recv_header_list[ix][0]; }
	int&      h_f_len(int ix) { return recv_header_list[ix][1]; }
	char*     h_v_ptr(int ix) { return recv_header_list[ix][2] + recv_header_buffer->_data + recv_header_pos; }
	int&      h_v_pos(int ix) { return recv_header_list[ix][2]; }
	int&      h_v_len(int ix) { return recv_header_list[ix][3]; }

	ARefsBuf *recv_buffer;
	int       recv_body_pos;
	int       recv_body_len;

	AMessage  recv_msg;
	AMessage *recv_from;
};

#define append_data(fmt, ...) \
	p->send_msg.size += snprintf(p->send_buffer+p->send_msg.size, send_bufsiz-p->send_msg.size, fmt, ##__VA_ARGS__)

#define append_crlf() \
	p->send_buffer[p->send_msg.size++] = '\r'; \
	p->send_buffer[p->send_msg.size++] = '\n';

static inline const char*
HeaderGet(HttpClient *p, const char *header, int &len)
{
	for (int ix = 1; ix < p->recv_header_count; ++ix) {
		if ((strncasecmp(header, p->h_f_ptr(ix), p->h_f_len(ix)) == 0)
		 && (header[p->h_f_len(ix)] == '\0'))
		{
			len = p->h_v_len(ix);
			return p->h_v_ptr(ix);
		}
	}
	return NULL;
}

extern int HttpClientCreate(AObject **object, AObject *parent, AOption *option);
extern void HttpClientRelease(AObject *object);
extern int HttpClientAppendOutput(HttpClient *p, AMessage *msg);
extern int HttpClientDoRecv(HttpClient *p, AMessage *msg);
extern int HttpClientDoSend(HttpClient *p, AMessage *msg);


#endif
