#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#ifndef _AMODULE_HTTPCLIENT_H_
#include "../http/AModule_HttpClient.h"
#endif

struct HttpSessionManager;

struct HttpSession {
	AObject object;
	DWORD   active;
	char    ssid[128];
	char    user[48];
	HttpSessionManager *manager;

	struct list_head msg_list;
	struct list_head user_list;
	AObject         *parent;
	pthread_mutex_t  mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	struct rb_node   sess_node;
	struct rb_node   user_node;
	struct list_head timeout_entry;
};
#define to_sess(obj)  container_of(obj, HttpSession, object)

struct HttpSessionManager {
	struct rb_root  sess_map;
	struct rb_root  user_map;
	struct list_head conn_list;
	pthread_mutex_t mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
	long volatile   genid;

	AOperator       asop;
	DWORD           check_timer;
	DWORD           max_sess_live;
	long volatile   sess_count;
	DWORD           max_conn_live;
	long volatile   conn_count;

	void push(HttpClient *p) {
		p->object.addref();
		lock();
		++conn_count;
		list_add_tail(&p->conn_entry, &conn_list);
		unlock();
	}
};

extern void         SessionInit(HttpSessionManager *sm);
extern HttpSession* SessionGet(HttpSessionManager *sm, const char *ssid, BOOL create);
extern HttpSession* SessionNew(HttpSessionManager *sm, const char *user, BOOL reuse);
extern void         SessionEnum(HttpSessionManager *sm, int(*cb)(HttpSession*,void*), int(*cb2)(HttpClient*,void*), void *arg);
extern void         SessionCheck(AOperator *asop, int result);

#endif
