#ifndef _AMODULE_ACLIENTMANAGER_H_
#define _AMODULE_ACLIENTMANAGER_H_


typedef struct AClientManager
{
	struct rb_root  client_map;
	pthread_mutex_t client_mutex;
	inline void     client_lock() { pthread_mutex_lock(&client_mutex); }
	inline void     client_unlock() { pthread_mutex_unlock(&client_mutex); }

	int        timer_reconnect;
	int        timer_offline;
	int        timer_heart;
	int        timer_check;

	AOperator  check_asop;
	AThread   *check_thread;
	int        check_post_num;
	struct AClient *check_last_client;
	BOOL       check_running;
	int        check_run(int result);
	void       check_status();
	void       check_abort();
} CM;

AMODULE_API void CM_init(AClientManager *cm);
AMODULE_API void CM_start(AClientManager *cm);
AMODULE_API void CM_stop(AClientManager *cm);
AMODULE_API void CM_exit(AClientManager *cm);

AMODULE_API void CM_check_status(AClientManager *cm);
AMODULE_API void CM_check_abort(AClientManager *cm);
AMODULE_API void CM_check_post(AClientManager *cm, struct AClient *c, BOOL addref);

AMODULE_API BOOL CM_add_internal(AClientManager *cm, struct AClient *c);
AMODULE_API int  CM_del_internal(AClientManager *cm, struct AClient *c);

#endif
