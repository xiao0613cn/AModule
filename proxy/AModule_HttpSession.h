#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#include "../ecs/AEntity.h"
#include "../ecs/AInOutComponent.h"
#define _USE_HTTP_MSG_IMPL_ 1
#include "../http/http_msg.h"

enum HttpStatus {
	s_recv_header = 0,
	s_recv_body,
	s_send_header,
	s_send_body,
};

struct HttpConnection : public AEntity {
	AInOutComponent _iocom;
	http_parser     _parser;
	ARefsBuf       *_inbuf;
	HttpStatus      _status;
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
