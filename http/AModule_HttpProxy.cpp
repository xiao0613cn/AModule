#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/ASystem.h"
#include "AModule_HttpClient.h"

#ifdef _WIN32
#pragma comment(lib, "../bin_win32/AModule.lib")
#endif

extern HttpConnectionModule HCM;

static int HttpSvcHandle(HttpConnection *p, HttpMsg *req, HttpMsg *resp)
{
	if (req->_parser.type != HTTP_REQUEST) {
		TRACE("invalid http type: %d.\n", req->_parser.type);
		return -EINVAL;
	}
	resp->reset();
	resp->_parser = p->_http._parser;
	resp->_parser.type = HTTP_RESPONSE;

	AService *svc = AServiceProbe(p->_svc, p, NULL);
	if (svc != NULL) {
		return svc->run(svc, p);
	}

	resp->_parser.status_code = 400;
	resp->uri_set(sz_t("Bad Request"), 1);
	return p->svc_resp();
}

static int HttpSvcRecvMsg(HttpParserCompenont *c, int result)
{
	HttpConnection *p = container_of(c, HttpConnection, _http);
	if (result >= 0)
		result = HttpSvcHandle(p, p->_req, p->_resp);
	if (result < 0) {
		TRACE("http connection down, result = %d.\n", result);
		if (p->valid()) {
			AEntityManager *em = p->_svc->_sysmng->_all_entities;
			em->lock();
			em->_pop(em, p);
			em->unlock();
		}
	}
	return result;
}

static int HttpSvcRun(AService *svc, AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	addref_s(p->_svc, svc);

	AEntityManager *em = svc->_sysmng->_all_entities;
	p->_iocom._mutex = &em->_mutex;
	p->_iocom._abort = false;

	em->lock();
	em->_push(em, p);
	em->unlock();

	ARefsBuf::reserve(p->_inbuf, 512, recv_bufsiz);
	if (p->_req == NULL) p->_req = HCM.hm_create();
	if (p->_resp == NULL) p->_resp = HCM.hm_create();

	return p->_http.try_output(&p->_iocom, p->_req, &HttpSvcRecvMsg);
}

static void HttpSvcStop(AService *svc)
{
	AEntityManager *em = svc->_sysmng->_all_entities;
	HttpConnection *p = NULL;

	em->lock();
	AComponent *c = em->_upper_com(em, p, HttpParserCompenont::name(), -1);
	while (c != NULL) {
		p = (HttpConnection*)c->_entity;
		c = em->_next_com(em, p, HttpParserCompenont::name(), -1);

		if ((p->_module == &HCM.module) && (p->_svc == svc)) {
			p->_iocom._io->shutdown();
			em->_pop(em, p);
		}
	}
	em->unlock();
}

static int HttpSvcCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *svc = (AService*)*object;
	svc->init();
	svc->_peer_module = &HCM.module;
	svc->stop = &HttpSvcStop;
	svc->run = &HttpSvcRun;
	return 1;
}

static void HttpSvcRelease(AObject *object)
{
	AService *svc = (AService*)object;
	svc->exit();
}

AModule HttpServiceModule = {
	AService::class_name(),
	"HttpService",
	sizeof(AService),
	NULL, NULL,
	&HttpSvcCreate,
	&HttpSvcRelease,
	HCM.module.probe,
};
static int reg_svc = AModuleRegister(&HttpServiceModule);


//////////////////////////////////////////////////////////////////////////
// Http File Service
static int HttpFileSvcRun(AService *hfs, AObject *object)
{
	HttpConnection *p = (HttpConnection*)object;
	HttpMsg *req = p->_req;
	HttpMsg *resp = p->_resp;

	assert(p->_inbuf->len() == 0);
	int body_len = 0;

	FILE *fp = fopen(req->uri_get(1).str+1, "rb");
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		body_len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	if (body_len <= 0) {
		resp->_parser.status_code = 404;
		resp->uri_set(sz_t("File Not Found"), 1);
	} else if (ARefsBuf::reserve(p->_inbuf, body_len, recv_bufsiz) < 0) {
		resp->_parser.status_code = 500;
		resp->uri_set(sz_t("Out Of Memory"), 1);
	} else if (fread(p->_inbuf->next(), body_len, 1, fp) != 1) {
		resp->_parser.status_code = 420;
		resp->uri_set(sz_t("Read File Error"), 1);
	} else {
		p->_inbuf->push(body_len);
		resp->body_set(p->_inbuf, p->_inbuf->_bgn, p->_inbuf->len());
		p->_inbuf->pop(p->_inbuf->len());

		resp->_parser.status_code = 200;
		resp->uri_set(sz_t("OK"), 1);
		resp->header_set(sz_t("Content-Type"), sz_t("text/html"));
	}
	reset_s(fp, NULL, fclose);

	return p->svc_resp();
}

static int HttpFileSvcCreate(AObject **object, AObject *parent, AOption *option)
{
	AService *hfs = (AService*)*object;
	hfs->init();
	hfs->run = &HttpFileSvcRun;
	return 1;
}

static void HttpFileSvcRelease(AObject *object)
{
	AService *hfs = (AService*)object;
	hfs->exit();
}

static int HttpFileSvcProbe(AObject *object, AObject *other, AMessage *msg)
{
	if ((object == NULL) || (other == NULL) || (other->_module != &HCM.module))
		return -1;
	AService *hfs = (AService*)object;
	HttpConnection *p = (HttpConnection*)other;
	if (hfs->_parent == p->_svc)
		return 40;
	return -1;
}

AModule HttpFileServiceModule = {
	AService::class_name(),
	"HttpFileService",
	sizeof(AService),
	NULL, NULL,
	&HttpFileSvcCreate,
	&HttpFileSvcRelease,
	&HttpFileSvcProbe,
};
static int reg_hfs = AModuleRegister(&HttpFileServiceModule);
