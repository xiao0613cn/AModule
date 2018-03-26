#ifndef _AMODULE_DEVICE_H_
#define _AMODULE_DEVICE_H_

#include "../ecs/AEntity.h"
struct AEventManager;

struct ADeviceComponent : public AComponent {
	static const char* name() { return "ADeviceComponent"; }
	rb_node _devmap_node;

	char    _dev_id[48];
	char    _net_addr[256];
	int     _net_port;
	char    _login_user[64];
	char    _login_pwd[64];

	int     _chan_count;
	int     _sensor_count;
	int     _alarmout_count;
	char    _private_sn[128];

	struct ptz_req {
		int         chan_id;
		const char *control;
		AOption    *opts;

		AOption    *resp;
	};
	int   (*ptz_ctrl)(ADeviceComponent *dev, ptz_req *req);

	struct rec_req {
		int        chan_id;
		struct tm  begin_tm;
		struct tm  end_tm;
		AOption   *opts;

		AOption   *resp;
	};
	int   (*get_record_list)(ADeviceComponent *dev, rec_req *req);

	int   (*extra_ctrl)(ADeviceComponent *dev, const char *cmd, AOption *req, AOption *resp);
};

struct ADeviceModule {
	AModule module;
	static ADeviceModule* get() {
		static ADeviceModule *s_m = (ADeviceModule*)AModuleFind(
			ADeviceComponent::name(), ADeviceComponent::name());
		return s_m;
	}

	// ADeviceComponent._devmap_node
	rb_root dev_map;
	int     dev_count;
	pthread_mutex_t dev_mutex;
	void lock() { pthread_mutex_lock(&dev_mutex); }
	void unlock() { pthread_mutex_unlock(&dev_mutex); }

	int      (*_push)(ADeviceComponent *dev, AEventManager *ev); // include dev->_object->addref()
	int      (*_pop)(ADeviceComponent *dev, AEventManager *ev);  // include dev->_object->release()
	ADeviceComponent* (*_find)(const char *devid);
	ADeviceComponent* (*_upper)(const char *devid);
	ADeviceComponent* (*_next)(ADeviceComponent *dev);

	ADeviceComponent* (*_find2)(AEntityManager *em, const char *devid);
};

struct ADeviceImplement {
	AModule module;
	// class_name: device

	int   (*ptz_ctrl)(AEntity *e, void *req, void *resp);
	int   (*get_record_list)(AEntity *e, void *req, void *resp);
	//int   (*extra_ctrl)(ADeviceComponent *dev, const char *cmd, AOption *req, AOption *resp);
};


#endif
