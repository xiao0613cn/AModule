#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "AModule_Session.h"


static void SessionRelease(AObject *object)
{
	SessionCtx *s = to_sess(object);
	AMsgListClear(&s->msg_list, -EINTR);
	assert(list_empty(&s->user_list));
	assert(list_empty(&s->conn_list));
	assert(s->parent == NULL);
	pthread_mutex_destroy(&s->mutex);

	assert(RB_EMPTY_NODE(&s->sm_sess_node));
	assert(RB_EMPTY_NODE(&s->sm_user_node));
}

static int SessionCreate(AObject **object, AObject *parent, AOption *option)
{
	SessionCtx *s = (SessionCtx*)*object;
	s->active = 0;
	s->ssid[0] = '\0';
	s->user[0] = '\0';
	s->sm = NULL;

	INIT_LIST_HEAD(&s->msg_list);
	INIT_LIST_HEAD(&s->user_list);
	INIT_LIST_HEAD(&s->conn_list);
	s->parent = NULL;
	pthread_mutex_init(&s->mutex, NULL);

	RB_CLEAR_NODE(&s->sm_sess_node);
	RB_CLEAR_NODE(&s->sm_user_node);
	INIT_LIST_HEAD(&s->sm_timeout_entry);
	return 1;
}

static int SessionRequest(AObject *object, int reqix, AMessage *msg)
{
	SessionCtx *s = to_sess(object);
	if (reqix == Aio_Input) {
	//	return HttpSessionInput(s, msg);
	}
	if (reqix == Aio_Output) {
	//	return HttpSessionOutput(s, msg);
	}
	return -ENOSYS;
}

static int SessionClose(AObject *object, AMessage *msg)
{
	SessionCtx *s = to_sess(object);
	RB_CLEAR_NODE(&s->sm_sess_node);
	RB_CLEAR_NODE(&s->sm_user_node);

	if (s->parent != NULL) {
		SessionCtx *parent = to_sess(s->parent);

		parent->lock();
		list_del_init(&s->user_list);
		parent->unlock();
		AObjectRelease(&s->object);

		//AObjectRelease(s->parent);
		//s->parent = NULL;
	/*} else {
		s->lock();
		struct list_head timeout_user = s->user_list;
		INIT_LIST_HEAD(&s->user_list);
		s->unlock();

		while (!list_empty(&timeout_user)) {
			SessionCtx *user = list_first_entry(&timeout_user, SessionCtx, user_list);
			list_del_init(&user->user_list);

			s->sm->on_session_timeout(user);
		}*/
	}
	return 1;
}

AModule DefSessionModule = {
	"session",
	"session",
	sizeof(SessionCtx),
	NULL, NULL,
	&SessionCreate,
	&SessionRelease,
	NULL,
	2,
	NULL, //&SessionOpen,
	NULL, NULL,
	&SessionRequest,
	NULL,
	&SessionClose,
};

//////////////////////////////////////////////////////////////////////////
static inline int ssid_cmp(const char *key, SessionCtx *s) {
	return strcmp(key, s->ssid);
}
rb_tree_define(SessionCtx, sm_sess_node, const char*, ssid_cmp)

typedef SessionCtx UserSessionCtx;
static inline int user_cmp(const char *key, UserSessionCtx *s) {
	return strcmp(key, s->user);
}
rb_tree_define(UserSessionCtx, sm_user_node, const char*, user_cmp)

AMODULE_API void
SessionInit(SessionManager *sm, AModule *sess_module)
{
	if (sess_module == NULL)
		sess_module = &DefSessionModule;
	sm->sess_module = sess_module;

	INIT_RB_ROOT(&sm->sess_map);
	INIT_RB_ROOT(&sm->user_map);
	INIT_LIST_HEAD(&sm->conn_list);
	pthread_mutex_init(&sm->mutex, NULL);

	sm->genid = 0;
	sm->check_timer.callback = NULL;
	sm->conn_tick_offset = 0;

	sm->max_sess_live = 30*60*1000;
	sm->sess_count = 0;

	sm->max_conn_live = 60*1000;
	sm->conn_count = 0;
	sm->on_connect_timeout = NULL;
}

static inline void
SessionAdd(SessionManager *sm, SessionCtx *s, DWORD tick, const char *ssid)
{
	AObjectAddRef(&s->object);
	s->active = tick;
	strcpy_sz(s->ssid, ssid);
	s->sm = sm;

	rb_insert_SessionCtx(&sm->sess_map, s, ssid);
	++sm->sess_count;
}

AMODULE_API SessionCtx*
SessionGet(SessionManager *sm, const char *ssid, BOOL create)
{
	DWORD tick = GetTickCount();
	sm->lock();
	SessionCtx *s = rb_find_SessionCtx(&sm->sess_map, ssid);
	if (s != NULL) {
		AObjectAddRef(&s->object);
	} else if (create != NULL) {
		int result = AObjectCreate2((AObject**)&s, NULL, NULL, sm->sess_module);
		if (result >= 0)
			SessionAdd(sm, s, tick, ssid);
	}
	sm->unlock();
	return s;
}

AMODULE_API SessionCtx*
SessionNew(SessionManager *sm, const char *user, BOOL reuse)
{
	SessionCtx *s;
	char ssid[128];
	for (int ix = 0; ix < 10; ++ix)
	{
		long genid = InterlockedAdd(&sm->genid, 1);
		DWORD tick = GetTickCount();
		int len = snprintf(ssid, sizeof(ssid)-1, "%s%08x%08x", user, tick, genid);
		ssid[len] = '\0';

		sm->lock();
		UserSessionCtx *parent = rb_find_UserSessionCtx(&sm->user_map, user);

		if (reuse && (parent != NULL)) {
			s = parent;
			AObjectAddRef(&s->object);
		}
		else if ((s = rb_find_SessionCtx(&sm->sess_map, ssid)) != NULL) {
			s = NULL; //ssid conflict
		}
		else {
			int result = AObjectCreate2((AObject**)&s, NULL, NULL, sm->sess_module);
			if (result >= 0) {
				SessionAdd(sm, s, tick, ssid);

				strcpy_sz(s->user, user);
				if (parent == NULL) {
					rb_insert_UserSessionCtx(&sm->user_map, s, user);
				} else {
					s->parent = parent;
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

AMODULE_API void
SessionEnum(SessionManager *sm, int(*cb)(SessionCtx*,void*), int(*cb2)(struct list_head*,void*), void *arg)
{
	sm->lock();
	if (cb != NULL) {
		cb(NULL, arg);

		struct rb_node *node = rb_first(&sm->sess_map);
		while (node != NULL)
		{
			SessionCtx *s = rb_entry(node, SessionCtx, sm_sess_node);
			node = rb_next(node);

			if (cb(s, arg) < 0) {
				--sm->sess_count;

				rb_erase(&s->sm_sess_node, &sm->sess_map);
				if ((s->parent == NULL) && !RB_EMPTY_NODE(&s->sm_user_node)) {
					rb_erase(&s->sm_user_node, &sm->user_map);
				}
			}
		}
	}
	if (cb2 != NULL) {
		cb2(NULL, arg);

		struct list_head *sm_conn_entry = sm->conn_list.next;
		while (sm_conn_entry != &sm->conn_list)
		{
			struct list_head *next = sm_conn_entry->next;
			if (cb2(sm_conn_entry, arg) < 0) {
				--sm->conn_count;
				if (sm_conn_entry->next == next)
					list_del_init(sm_conn_entry);
			}
			sm_conn_entry = next;
		}
	}
	sm->unlock();
}

AMODULE_API void
SessionCheck(SessionManager *sm)
{
	struct list_head timeout_sess;
	INIT_LIST_HEAD(&timeout_sess);

	struct list_head timeout_conn;
	INIT_LIST_HEAD(&timeout_conn);

	DWORD tick = GetTickCount();

	sm->lock();
	struct rb_node *node = rb_first(&sm->sess_map);
	while (node != NULL)
	{
		SessionCtx *s = rb_entry(node, SessionCtx, sm_sess_node);
		node = rb_next(node);

		if ((tick-s->active) < sm->max_sess_live)
			continue;

		rb_erase(&s->sm_sess_node, &sm->sess_map);
		if ((s->parent == NULL) && !RB_EMPTY_NODE(&s->sm_user_node))
			rb_erase(&s->sm_user_node, &sm->user_map);

		list_add_tail(&s->sm_timeout_entry, &timeout_sess);
		--sm->sess_count;
	}
	struct list_head *sm_conn_entry = sm->conn_list.next;
	while (sm_conn_entry != &sm->conn_list)
	{
		struct list_head *next = sm_conn_entry->next;
		if ((tick-*(DWORD*)((char*)sm_conn_entry+sm->conn_tick_offset)) > sm->max_conn_live) {
			list_move_tail(sm_conn_entry, &timeout_conn);
			--sm->conn_count;
		}
		sm_conn_entry = next;
	}
	sm->unlock();

	while (!list_empty(&timeout_sess)) {
		SessionCtx *s = list_first_entry(&timeout_sess, SessionCtx, sm_timeout_entry);
		list_del_init(&s->sm_timeout_entry);

		s->object.close(&s->object, NULL);
		AObjectRelease(&s->object);
	}
	while (!list_empty(&timeout_conn)) {
		sm_conn_entry = timeout_conn.next;
		list_del_init(sm_conn_entry);
		sm->on_connect_timeout(sm_conn_entry);
	}
}
