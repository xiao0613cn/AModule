#ifndef _AMODULE_DEVICE_MANAGER_H_
#define _AMODULE_DEVICE_MANAGER_H_


typedef struct DeviceManager {
	void   init();

	struct rb_root    dev_map;
	pthread_mutex_t   dev_mutex;
	inline void       dev_lock() { pthread_mutex_lock(&dev_mutex); }
	inline void       dev_unlock() { pthread_mutex_unlock(&dev_mutex); }
	struct ADevice*   dev_get(DWORDLONG id);

	struct list_head  open_list;
	struct list_head  check_list;
	pthread_t         work_thread;
	int    start();
	void   run();

} DeviceManager;

extern DeviceManager DM;


#endif
