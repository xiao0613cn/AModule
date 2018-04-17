#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "test.h"
#include "../http/http_msg.h"

static int on_resp(HttpConnection *p, HttpMsg *req, HttpMsg *resp, int result)
{
	if (result < 0)
		HttpConnectionModule::get()->hm_release(req);
	TRACE("result = %d.\n", result);
	if (resp == NULL)
		return result;

	TRACE("body len = %lld, body data = %.*s.\n", resp->body_len(),
		(int)resp->body_len(), resp->body_ptr());
	return result;
}

CU_TEST(test_http)
{
	dlload(NULL, "service_http");
	HttpConnectionModule *m = HttpConnectionModule::get();
	HttpMsg *msg = m->hm_create();
	//AObject *p = NULL;
	//AObject::create2(&p, NULL, NULL, &m->module);
	//p->_http._max_body_size = 64*1024;

	msg->_parser.type = HTTP_REQUEST;
	msg->_parser.method = HTTP_GET;
	msg->_parser.http_major = 1;
	msg->_parser.http_minor = 1;
	msg->uri_set(sz_t("/"), 1);
	msg->_kv_set(msg, HttpMsg::KV_UriInfo, sz_t("Schema"), sz_t("tcp"));
	msg->header_set(sz_t("Host"), sz_t("www.sina.com.cn"));
	msg->header_set(sz_t("Accept"), sz_t("*/*"));

	int result = m->request(NULL/*p*/, msg, on_resp);
	//p->release();
	TRACE("http request = %d.\n", result);
	if (result < 0)
		m->hm_release(msg);
}
