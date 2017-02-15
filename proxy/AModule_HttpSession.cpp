#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"


static void HttpSessionRelease(AObject *object)
{
	HttpSession *s = to_sess(object);
	pthread_mutex_destroy(&s->mutex);
}

static int HttpSessionCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpSession *s = (HttpSession*)*object;
	s->active = GetTickCount();
	s->ssid[0] = '\0';

	INIT_LIST_HEAD(&s->msg_list);
	INIT_LIST_HEAD(&s->conn_list);
}

static inline int ssid_cmp(const char *key, HttpSession *s)
{
	return strcmp(key, s->ssid);
}

rb_tree_define(HttpSession, ss_node, const char*, ssid_cmp)

extern HttpSession* SessionGet(HttpSessionManager *sm, const char *ssid, BOOL create)
{
	sm->lock();
	HttpSession *s = rb_find_HttpSession(&sm->ss_map, ssid);
	if (s == NULL) {
		int result = AObjectCreate2(&s, NULL, NULL, &HttpSessionModule);
		if (result >= 0)
			strcpy_sz(s->ssid, ssid);
	}
	sm->unlock();
	return s;
}