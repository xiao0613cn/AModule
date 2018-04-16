#ifndef _AMODULE_HTTPCLIENT_H_
#define _AMODULE_HTTPCLIENT_H_

#include "AModule_HttpSession.h"
#include "../ecs/AServiceComponent.h"


struct HttpConnection : public AEntity {
	AInOutComponent     _iocom;
	HttpParserCompenont _http;
	AServiceComponent  *_svc;

	ARefsBuf     *_inbuf;
	int (*raw_inmsg_done)(AMessage*,int);

	HttpMsg  *_req;
	HttpMsg  *_resp;
	int (*raw_outmsg_done)(AMessage*,int);

	HttpConnectionModule* M() {
		return (HttpConnectionModule*)_module;
	}
	int svc_resp() {
		return _iocom.do_input(&_iocom, &_iocom._outmsg);
	}
};


#endif
