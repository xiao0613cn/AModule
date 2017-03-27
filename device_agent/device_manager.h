#ifndef _AMODULE_DEVICE_MANAGER_H_
#define _AMODULE_DEVICE_MANAGER_H_


typedef struct DeviceManager {
	void   init();
	int    start();

	DWORD  timer_reconnect;
	DWORD  timer_offline;
	DWORD  timer_heart;

	struct rb_root    dev_map;
	pthread_mutex_t   dev_mutex;
	inline void       dev_lock() { pthread_mutex_lock(&dev_mutex); }
	inline void       dev_unlock() { pthread_mutex_unlock(&dev_mutex); }
	struct ADevice*   dev_get(DWORDLONG id);
	void              dev_put(struct ADevice *dev);

	struct list_head  open_list;
	pthread_t         open_thread;
	void              run_open();

	struct list_head  heart_list;
	pthread_t         heart_thread;
	void              run_heart();
} DeviceManager;

AMODULE_API DeviceManager DM;
AMODULE_API void DM_init();
AMODULE_API int  DM_start();
AMODULE_API struct ADevice* DM_get(DWORDLONG id);
AMODULE_API void DM_put(struct ADevice *dev);


#endif
