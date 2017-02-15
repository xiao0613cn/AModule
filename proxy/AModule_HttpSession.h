#ifndef _AMODULE_HTTPSESSION_H_
#define _AMODULE_HTTPSESSION_H_

#ifndef _AMODULE_HTTPCLIENT_H_
#include "../http/AModule_HttpClient.h"
#endif

struct HttpSession {
	AObject object;
	DWORD   active;
	char    ssid[48];

	struct list_head msg_list;
	struct list_head conn_list;
	pthread_mutex_t  mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	struct rb_node   ss_node;
};
#define to_sess(obj)  container_of(obj, HttpSession, object)

struct HttpSessionManager {
	struct rb_root  ss_map;
	pthread_mutex_t mutex;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	long volatile   genid;
	char            prefix[16];
};

extern HttpSession* SessionGet(HttpSessionManager *sm, HttpClient *c, BOOL create);

#endif
