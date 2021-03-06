#ifndef _AMODULE_DEVICE_H_
#define _AMODULE_DEVICE_H_

#include "../ecs/AEntity.h"
struct AEventManager;

struct ADeviceComponent : public AComponent {
	static const char* name() { return "ADeviceComponent"; }
	AMODULE_GET(struct ADeviceComponentModule, name(), name())
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
	};
	struct rec_req {
		int        chan_id;
		struct tm  begin_tm;
		struct tm  end_tm;
		AOption   *opts;
	};
};

struct ADeviceComponentModule {
	AModule module;
	AMODULE_GET(ADeviceComponentModule, ADeviceComponent::name(), ADeviceComponent::name())

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
	int   (*extra_ctrl)(ADeviceComponent *dev, const char *cmd, void *req, void *resp);
};

struct ACaptureComponent : public AComponent {
	static const char* name() { return "ACaptureComponent"; }

	int       (*do_capture)(ACaptureComponent *c, void *req);

	int       (*on_capture_done)(ACaptureComponent *c);
	void       *on_capture_userdata;
	uint8_t    *on_capture_data;
	int         on_capture_size;
	const char *on_capture_fmt;
};

#endif
