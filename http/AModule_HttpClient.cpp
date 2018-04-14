#include "../stdafx.h"
#include "AModule_HttpClient.h"


static int HttpReqRecvDone(HttpParserCompenont *c, int result)
{
	HttpConnection *p = container_of(c, HttpConnection, _http);
	int(*on_resp)(HttpConnection*,HttpMsg*,HttpMsg*,int) =
		(int(*)(HttpConnection*,HttpMsg*,HttpMsg*,int))p->raw_outmsg_done;

	result = on_resp(p, p->_req, p->_resp, result);
	if ((result < 0) || (result >= AMsgType_Class)) {
		p->_req = NULL;
		p->release();
	}
	return result;
}

static int HttpReqSendDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._inmsg);
	result = p->M()->input_status(p, msg, p->_req, result);

	if ((result > 0) && (p->_resp == NULL)) {
		p->_resp = p->M()->hm_create();
		if (p->_resp == NULL)
			result = -ENOMEM;
	}
	if (result > 0) {
		if (p->_iocom._outbuf == NULL) {
			addref_s(p->_iocom._outbuf, p->_inbuf);
		}
		result = p->_http.try_output(&p->_iocom, p->_resp, &HttpReqRecvDone);
	} else if (result < 0) {
		result = HttpReqRecvDone(&p->_http, result);
	}
	return result;
}

static int HttpReqOpenDone(AMessage *msg, int result)
{
	HttpConnection *p = container_of(msg, HttpConnection, _iocom._inmsg);
	assert(msg->type == AMsgType_AOption);
	((AOption*)msg->data)->release();

	msg->init();
	msg->done = &HttpReqSendDone;
	return msg->done2(result);
}

extern int HttpRequest(HttpConnection *p, HttpMsg *req, int(*on_resp)(HttpConnection*,HttpMsg*,HttpMsg*,int))
{
	if (p == NULL) {
		int result = AObject::create(&p, NULL, NULL, "HttpConnection");
		if (result < 0)
			return result;
	} else {
		p->addref();
	}

	AMessage *msg = &p->_iocom._inmsg;
	if (p->_iocom._io != NULL) {
		p->_req = req;
		p->raw_outmsg_done = (int(*)(AMessage*,int))on_resp;

		msg->init();
		msg->done = &HttpReqSendDone;
		msg->done2(1);
		return 0;
	}

	str_t host = req->header_get(sz_t("Proxy"));
	if (host.str == NULL)
		host = req->header_get(sz_t("Host"));
	if (host.str == NULL)
		return -EINVAL;

	str_t port(strnchr(host.str, host.len, ':'), 0);
	if (port.str == NULL) {
		port.str = "80"; port.len = 2;
	} else {
		port.len = int(host.str + host.len - port.str - 1);
		host.len = int(port.str - host.str);
		port.str += 1;
	}

	char opt_str[256];
	snprintf(opt_str, 256, "{address:'%.*s',port:%.*s}",
		host.len, host.str, port.len, port.str);

	AOption *io_opt = NULL;
	int result = AOptionDecode(&io_opt, opt_str, -1);
	if (result < 0) {
		p->release();
		return result;
	}

	msg->init(io_opt);
	msg->done = &HttpReqOpenDone;
	p->_req = req;
	p->raw_outmsg_done = (int(*)(AMessage*,int))on_resp;

	result = AObject::create(&p->_iocom._io, p, io_opt, "async_tcp");
	if (result >= 0) {
		result = p->_iocom._io->open(msg);
	}
	if (result != 0) {
		msg->done2(result);
		result = 0;
	}
	return result;
}
