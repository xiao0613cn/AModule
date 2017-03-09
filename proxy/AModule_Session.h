#ifndef _AMODULE_SESSION_H_
#define _AMODULE_SESSION_H_

struct SessionManager;

struct SessionCtx {
	AObject object;
	DWORD   active;
	char    ssid[128];
	char    user[48];

	struct list_head msg_list;
	struct list_head user_list;
	struct list_head conn_list;
	SessionCtx      *parent;
	pthread_mutex_t  mutex;
	void lock()    { pthread_mutex_lock(&mutex); }
	void unlock()  { pthread_mutex_unlock(&mutex); }

	SessionManager  *sm;
	struct rb_node   sm_sess_node;
	struct rb_node   sm_user_node;
	struct list_head sm_timeout_entry;
};
#define to_sess(obj)  container_of(obj, SessionCtx, object)

struct SessionManager {
	AModule        *sess_module;
	struct rb_root  sess_map;
	struct rb_root  user_map;
	struct list_head conn_list;
	pthread_mutex_t mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	long volatile   genid;
	AOperator       check_timer;

	DWORD           max_sess_live;
	long volatile   sess_count;
	//void          (*on_session_timeout)(SessionCtx*); => sess->AObject::close(sess, NULL)

	DWORD           max_conn_live;
	long volatile   conn_count;
	int             conn_tick_offset;
	void          (*on_connect_timeout)(struct list_head *sm_conn_entry);

	void push(struct list_head *conn) {
		// do: conn->addref();
		lock();
		++conn_count;
		list_add_tail(conn, &conn_list);
		unlock();
	}
};

AMODULE_API void        SessionInit(SessionManager *sm, AModule *sess_module);
AMODULE_API SessionCtx* SessionGet(SessionManager *sm, const char *ssid, BOOL create);
AMODULE_API SessionCtx* SessionNew(SessionManager *sm, const char *user, BOOL reuse);
AMODULE_API void        SessionEnum(SessionManager *sm, int(*cb)(SessionCtx*,void*), int(*cb2)(struct list_head *conn,void*), void *arg);
AMODULE_API void        SessionCheck(SessionManager *sm);

#endif
