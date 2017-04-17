#include "stdafx.h"
#include "device.h"
#include "device_manager.h"


static inline int dev_cmp(DWORDLONG key, ADevice *dev) {
	return int(key - dev->id);
}
rb_tree_define(ADevice, map_node, DWORDLONG, dev_cmp);


DeviceManager DM;

void DeviceManager::init()
{
	timer_reconnect = 30*1000;
	timer_offline = 30*1000;
	timer_heart = 10*1000;

	INIT_RB_ROOT(&dev_map);
	pthread_mutex_init(&dev_mutex, NULL);

	INIT_LIST_HEAD(&open_list);
	open_thread = pthread_null;

	INIT_LIST_HEAD(&heart_list);
	heart_thread = pthread_null;
}
AMODULE_API void DM_init() { DM.init(); }

int DeviceManager::start()
{
	int ret = pthread_create(&open_thread, NULL,
		&pthread_object_run<DeviceManager, &DeviceManager::run_open>, this);
	return ret;
}
AMODULE_API int DM_start() { return DM.start(); }

static inline void check_abort(ADevice *&dev) {
	if (dev->object.refcount > 1) {
		dev = NULL;
	} else {
		dev->status = AObject_Closing;
		dev->object.addref();
	}
}

void DeviceManager::run_open()
{
	DWORDLONG last_dev = 0;
	for ( ; ; Sleep(100))
	{
		dev_lock();
		ADevice *dev = rb_upper_ADevice(&dev_map, last_dev);
		if (dev == NULL) {
			last_dev = 0;
			dev_unlock();

			Sleep(10*1000);
			continue;
		}

		last_dev = dev->id;
		DWORD tick = GetTickCount();

		switch (dev->status)
		{
		case AObject_Opened:
			if (!list_empty(&dev->heart_entry)) {
				dev = NULL;
				break;
			}
			if ((tick-dev->active) > timer_offline) {
				dev->status = AObject_Abort;
				check_abort(dev);
			} else {
				//list_add_tail(&dev->heart_entry, &heart_list);
				dev = NULL;
			}
			break;

		case AObject_Opening:
		case AObject_Closing:
			dev = NULL;
			break;

		case AObject_Abort:
			check_abort(dev);
			break;

		case AObject_Invalid:
		case AObject_Closed:
			if ((tick-dev->active) < timer_reconnect) {
				dev = NULL;
			} else {
				dev->status = AObject_Opening;
				dev->object.addref();
			}
			break;

		default: assert(FALSE); dev = NULL; break;
		}
		dev_unlock();
		if (dev == NULL)
			continue;

		dev->active = tick;
		if (dev->status == AObject_Opening) {
			int result = dev->object.open(&dev->object, NULL);
			TRACE("dev(%lld): open(%s:%d) = %d.\n", dev->id,
				dev->net_addr, dev->net_port, result);
			dev->status = (result >= 0 ? AObject_Opened : AObject_Abort);
		} else {
			dev->object.close(&dev->object, NULL);
			dev->status = AObject_Closed;
		}
		dev->object.release2();
	}
}

ADevice* DeviceManager::dev_get(DWORDLONG id)
{
	dev_lock();
	ADevice *dev = rb_find_ADevice(&dev_map, id);
	if (dev != NULL) {
		if (dev->status == AObject_Opened)
			dev->object.addref();
		else
			dev = NULL;
	}
	dev_unlock();
	return dev;
}
AMODULE_API ADevice* DM_get(DWORDLONG id) { return DM.dev_get(id); }

void DeviceManager::dev_put(ADevice *dev)
{
	dev->object.addref();

	dev_lock();
	ADevice *old_dev = rb_find_ADevice(&dev_map, dev->id);
	if (old_dev != NULL) {
		rb_replace_node(&old_dev->map_node, &dev->map_node, &dev_map);
	} else {
		rb_insert_ADevice(&dev_map, dev, dev->id);
	}
	dev_unlock();

	if (old_dev != NULL) {
		TRACE("dev(%lld): kick old dev(%s).\n", dev->id, old_dev->net_addr);
		old_dev->status = AObject_Abort;
		old_dev->object.release2();
	}
}
AMODULE_API void DM_put(ADevice *dev) { DM.dev_put(dev); }
