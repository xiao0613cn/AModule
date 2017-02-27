#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_HttpSession.h"


static void HttpSessionRelease(AObject *object)
{
	HttpSession *s = to_sess(object);
	AMsgListClear(&s->msg_list, -EINTR);

	assert(s->parent == NULL);
	assert(list_empty(&s->user_list));
	pthread_mutex_destroy(&s->mutex);

	assert(RB_EMPTY_NODE(&s->sess_node));
	assert(RB_EMPTY_NODE(&s->user_node));
}

static int HttpSessionCreate(AObject **object, AObject *parent, AOption *option)
{
	HttpSession *s = (HttpSession*)*object;
	s->active = GetTickCount();
	s->ssid[0] = '\0';
	s->user[0] = '\0';
	s->manager = NULL;

	INIT_LIST_HEAD(&s->msg_list);
	INIT_LIST_HEAD(&s->user_list);
	s->parent = NULL;
	pthread_mutex_init(&s->mutex, NULL);

	RB_CLEAR_NODE(&s->sess_node);
	RB_CLEAR_NODE(&s->user_node);
	INIT_LIST_HEAD(&s->timeout_entry);
	return 1;
}

static int HttpSessionRequest(AObject *object, int reqix, AMessage *msg)
{
	HttpSession *s = to_sess(object);
	if (reqix == Aio_Input) {
	//	return HttpSessionInput(s, msg);
	}
	if (reqix == Aio_Output) {
	//	return HttpSessionOutput(s, msg);
	}
	return -ENOSYS;
}

AModule HttpSessionModule = {
	"session",
	"http_session",
	sizeof(HttpSession),
	NULL, NULL,
	&HttpSessionCreate,
	&HttpSessionRelease,
	NULL,
	2,
	NULL, //&HttpSessionOpen,
	NULL, NULL,
	&HttpSessionRequest,
	NULL,
	NULL, //&HttpSessionClose,
};

//////////////////////////////////////////////////////////////////////////
static inline int ssid_cmp(const char *key, HttpSession *s) {
	return strcmp(key, s->ssid);
}
rb_tree_define(HttpSession, sess_node, const char*, ssid_cmp)

typedef HttpSession HttpUserSession;
static inline int user_cmp(const char *key, HttpUserSession *s) {
	return strcmp(key, s->user);
}
rb_tree_define(HttpUserSession, user_node, const char*, user_cmp)

extern void SessionInit(HttpSessionManager *sm)
{
	INIT_RB_ROOT(&sm->sess_map);
	INIT_RB_ROOT(&sm->user_map);
	INIT_LIST_HEAD(&sm->conn_list);
	pthread_mutex_init(&sm->mutex, NULL);

	sm->genid = 0;
	sm->max_live = 30*60*1000;
}

extern HttpSession* SessionGet(HttpSessionManager *sm, const char *ssid, BOOL create)
{
	sm->lock();
	HttpSession *s = rb_find_HttpSession(&sm->sess_map, ssid);
	if (s != NULL) {
		AObjectAddRef(&s->object);
	} else if (create) {
		int result = AObjectCreate2((AObject**)&s, NULL, NULL, &HttpSessionModule);
		if (result >= 0) {
			AObjectAddRef(&s->object);

			strcpy_sz(s->ssid, ssid);
			s->manager = sm;
			rb_insert_HttpSession(&sm->sess_map, s, ssid);
		}
	}
	sm->unlock();
	return s;
}

extern HttpSession* SessionNew(HttpSessionManager *sm, const char *user, BOOL reuse)
{
	HttpSession *s;
	char ssid[128];
	for (int ix = 0; ix < 10; ++ix)
	{
		long genid = InterlockedAdd(&sm->genid, 1);
		int len = snprintf(ssid, sizeof(ssid)-1, "%s%08x%08x", user, GetTickCount(), genid);
		ssid[len] = '\0';

		sm->lock();
		HttpUserSession *parent = rb_find_HttpUserSession(&sm->user_map, user);

		if (reuse && (parent != NULL)) {
			s = parent;
			AObjectAddRef(&s->object);
		}
		else if ((s = rb_find_HttpSession(&sm->sess_map, ssid)) != NULL) {
			s = NULL; //ssid conflict
		}
		else {
			int result = AObjectCreate2((AObject**)&s, NULL, NULL, &HttpSessionModule);
			if (result >= 0) {
				AObjectAddRef(&s->object);
				strcpy_sz(s->ssid, ssid);
				strcpy_sz(s->user, user);
				s->manager = sm;

				rb_insert_HttpSession(&sm->sess_map, s, ssid);
				if (parent == NULL) {
					rb_insert_HttpUserSession(&sm->user_map, s, user);
				} else {
					s->parent = &parent->object;
					AObjectAddRef(&parent->object);
				}
			}
		}
		sm->unlock();
		if (s != NULL) {
			if (!reuse && (parent != NULL)) {
				AObjectAddRef(&s->object);
				parent->lock();
				list_add_tail(&s->user_list, &parent->user_list);
				parent->unlock();
			}
			return s;
		}
	}
	return NULL;
}

extern void SessionEnum(HttpSessionManager *sm, int(*cb)(HttpSession*,void*), int(*cb2)(HttpClient*,void*), void *arg)
{
	sm->lock();
	if (cb != NULL) {
		cb(NULL, arg);

		struct rb_node *node = rb_first(&sm->sess_map);
		while (node != NULL)
		{
			HttpSession *s = rb_entry(node, HttpSession, sess_node);
			node = rb_next(node);

			if (cb(s, arg) < 0) {
				rb_erase(&s->sess_node, &sm->sess_map);
				if ((s->parent == NULL) && !RB_EMPTY_NODE(&s->user_node)) {
					rb_erase(&s->user_node, &sm->user_map);
				}
			}
		}
	}
	if (cb2 != NULL) {
		cb2(NULL, arg);

		HttpClient *conn = list_first_entry(&sm->conn_list, HttpClient, conn_entry);
		while (&conn->conn_entry != &sm->conn_list)
		{
			HttpClient *next = list_entry(conn->conn_entry.next, HttpClient, conn_entry);
			if (cb2(conn, arg) < 0)
				list_del_init(&conn->conn_entry);
			conn = next;
		}
	}
	sm->unlock();
}

extern void SessionCheck(HttpSessionManager *sm)
{
	struct list_head timeout_sess;
	INIT_LIST_HEAD(&timeout_sess);

	struct list_head timeout_conn;
	INIT_LIST_HEAD(&timeout_conn);

	sm->lock();
	DWORD tick = GetTickCount();

	struct rb_node *node = rb_first(&sm->sess_map);
	while (node != NULL)
	{
		HttpSession *s = rb_entry(node, HttpSession, sess_node);
		node = rb_next(node);

		if ((tick-s->active) < sm->max_live)
			continue;

		rb_erase(&s->sess_node, &sm->sess_map);
		if ((s->parent == NULL) && !RB_EMPTY_NODE(&s->user_node)) {
			rb_erase(&s->user_node, &sm->user_map);
		}

		list_add_tail(&s->timeout_entry, &timeout_sess);
	}

	HttpClient *conn;
	list_for_each_entry(conn, &sm->conn_list, HttpClient, conn_entry) {
		if ((tick-conn->active) < sm->max_live)
			continue;
		list_move_tail(&conn->conn_entry, &timeout_conn);
		AObjectAddRef(&conn->object);
	}
	sm->unlock();

	while (!list_empty(&timeout_sess)) {
		HttpSession *s = list_first_entry(&timeout_sess, HttpSession, timeout_entry);
		list_del_init(&s->timeout_entry);

		if (s->parent != NULL) {
			HttpSession *parent = to_sess(s->parent);

			parent->lock();
			list_del_init(&s->user_list);
			parent->unlock();

			//AObjectRelease(s->parent);
			//s->parent = NULL;
		}
		AObjectRelease(&s->object);
	}
	while (!list_empty(&timeout_conn)) {
		HttpClient *conn = list_first_entry(&timeout_conn, HttpClient, conn_entry);
		conn->io->close(conn->io, NULL);
		AObjectRelease(&conn->object);
	}
}
