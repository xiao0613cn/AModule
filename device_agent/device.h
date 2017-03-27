#ifndef _AMODULE_DEVICE_H_
#define _AMODULE_DEVICE_H_


typedef struct ADevice {
	AObject          object;
	AObject_Status   status;
	struct rb_node   map_node;    // device map tree entry in DeviceManager
	struct list_head heart_entry; // checking heart list entry in DeviceManager

	DWORD      active;
	DWORDLONG  id;
	char       name[128];

	char       net_addr[128];
	int        net_port;
	char       login_user[32];
	char       login_pwd[32];

	int        channel_count;
	int        sensor_count;
	int        alarmout_count;
	char       private_sn[128];

	DWORDLONG  owner_id;
	char       owner_name[128];
	DWORDLONG  area_id;
	char       area_name[128];

	inline void init() {
		status = AObject_Invalid;
		RB_CLEAR_NODE(&map_node);
		INIT_LIST_HEAD(&heart_entry);
		memset(&this->active, 0, sizeof(ADevice) - offsetof(ADevice, active));
	}
} ADevice;



#endif
