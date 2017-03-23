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
	INIT_RB_ROOT(&dev_map);
	pthread_mutex_init(&dev_mutex, NULL);

	INIT_LIST_HEAD(&open_list);
	INIT_LIST_HEAD(&check_list);
	work_thread = 0;
}

int DeviceManager::start()
{
	int ret = pthread_create(&work_thread, NULL,
		&TObjectRun<DeviceManager, &DeviceManager::run>, this);
	return ret;
}

void DeviceManager::run()
{
	DWORDLONG last_dev = 0;
	for ( ; ; Sleep(100))
	{
		dev_lock();
		ADevice *dev = rb_upper_ADevice(&dev_map, last_dev);
		if (dev == NULL) {
			dev_unlock();
			last_dev = 0;
			Sleep(10*1000);
			continue;
		}

		last_dev = dev->id;
		DWORD tick = GetTickCount();

		switch (dev->status)
		{
		case AObject_Opened:
			if (list_empty(&dev->check_entry) && ((tick-dev->active) > 10*1000))
				list_add_tail(&dev->check_entry, &check_list);

		case AObject_Opening:
		case AObject_Closing:
			dev = NULL;
			break;

		case AObject_Abort:
			if (dev->object.refcount > 1) {
				dev = NULL;
			} else {
				dev->status = AObject_Closing;
				dev->object.addref();
			}
			break;

		case AObject_Invalid:
		case AObject_Closed:
			if ((tick-dev->active) < 10*1000) {
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
