#include "../stdafx.h"
#include "test.h"
#include "../http/AModule_HttpSession.h"

static int on_resp(HttpConnection *p, HttpMsg *req, HttpMsg *resp, int result)
{
	AModule::get<HttpConnectionModule>()->hm_release(req);
	TRACE("result = %d.\n", result);
	if (resp == NULL)
		return result;

	TRACE("body len = %lld, body data = %.*s.\n", resp->body_len(),
		(int)resp->body_len(), resp->body_ptr());
	return result;
}

CU_TEST(test_http)
{
	dlload(NULL, "service_http", FALSE);
	HttpConnectionModule *m = AModule::get<HttpConnectionModule>();
	HttpMsg *msg = m->hm_create();

	msg->_parser.type = HTTP_REQUEST;
	msg->_parser.method = HTTP_GET;
	msg->uri_set(sz_t("/index.html"), 1);
	msg->header_set(sz_t("Host"), sz_t("www.sina.com.cn"));
	msg->header_set(sz_t("Accept"), sz_t("*/*"));

	int result = m->request(NULL, msg, on_resp);
	TRACE("http request = %d.\n", result);
	if (result < 0)
		m->hm_release(msg);
}
