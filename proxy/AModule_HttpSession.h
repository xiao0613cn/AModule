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
	struct list_head conn_list;
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
	pthread_mutex_t mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	long volatile   genid;
	DWORD           max_live;
};

extern void         SessionInit(HttpSessionManager *sm);
extern HttpSession* SessionGet(HttpSessionManager *sm, const char *ssid, BOOL create);
extern HttpSession* SessionNew(HttpSessionManager *sm, const char *user, BOOL reuse);
extern void         SessionEnum(HttpSessionManager *sm, int(*cb)(HttpSession*,void*), void *arg);
extern void         SessionCheck(HttpSessionManager *sm);

#endif
