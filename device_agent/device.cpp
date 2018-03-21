#include "../stdafx.h"
#include "device.h"
#include "../ecs/AEvent.h"
#pragma comment(lib, "../bin_win32/AModule.lib")

static int devcom_cmp(const char *key, ADeviceComponent *data) {
	return strcmp(key, data->_dev_id);
}
rb_tree_define(ADeviceComponent, _devmap_node, const char*, devcom_cmp)

extern ADeviceModule DM;

static int DM_push(ADeviceComponent *dev, AEventManager *ev)
{
	ADeviceComponent *old_dev = DM._find(dev->_dev_id);
	if (old_dev != NULL)
		DM._pop(old_dev, ev);

	old_dev = rb_insert_ADeviceComponent(&DM.dev_map, dev, dev->_dev_id);
	assert(old_dev == NULL);
	return 1;
}

static int DM_pop(ADeviceComponent *dev, AEventManager *ev)
{
	if (RB_EMPTY_NODE(&dev->_devmap_node))
		return -1;

	rb_erase(&dev->_devmap_node, &DM.dev_map);
	RB_CLEAR_NODE(&dev->_devmap_node);
	if (ev != NULL)
		ev->emit_by_name(ev, "on_dev_pop", dev);
	return 1;
}

static ADeviceComponent* DM_find(const char *devid)
{
	return rb_find_ADeviceComponent(&DM.dev_map, devid);
}

static ADeviceComponent* DM_upper(const char *devid)
{
	return rb_upper_ADeviceComponent(&DM.dev_map, devid);
}

static ADeviceComponent* DM_next(ADeviceComponent *dev)
{
	rb_node *node = rb_next(&dev->_devmap_node);
	if (node == NULL)
		return NULL;
	return rb_entry(node, ADeviceComponent, _devmap_node);
}

static int DM_init(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
		INIT_RB_ROOT(&DM.dev_map);
		DM.dev_count = 0;
		pthread_mutex_init(&DM.dev_mutex, NULL);
	}
	return 1;
}

static void DM_exit(int inited)
{
	if (inited > 0) {
		pthread_mutex_destroy(&DM.dev_mutex);
	}
}

static int devcom_ptz_ctrl(ADeviceComponent *dev, ADeviceComponent::ptz_req *req)
{
	AModule *m = dev->_object->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int devcom_get_rec(ADeviceComponent *dev, ADeviceComponent::rec_req *req)
{
	AModule *m = dev->_object->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int devcom_exctrl(ADeviceComponent *dev, const char *cmd, AOption *req, AOption *resp)
{
	AModule *m = dev->_object->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int devcom_create(AObject **object, AObject *parent, AOption *options)
{
	ADeviceComponent *dev = (ADeviceComponent*)*object;

	strcpy_sz(dev->_dev_id,     options->getStr("devid", NULL));
	strcpy_sz(dev->_net_addr,   options->getStr("net_addr", NULL));
	dev->_net_port            = options->getInt("net_port", 0);
	strcpy_sz(dev->_login_user, options->getStr("login_user", NULL));
	strcpy_sz(dev->_login_pwd,  options->getStr("login_pwd", NULL));

	dev->_chan_count = dev->_sensor_count = dev->_alarmout_count = 0;
	dev->_private_sn[0] = '\0';
	dev->ptz_ctrl = &devcom_ptz_ctrl;
	dev->get_record_list = &devcom_get_rec;
	dev->extra_ctrl = &devcom_exctrl;
	return 1;
}

static ADeviceComponent* devcom_find(AEntityManager *em, const char *devid)
{
	ADeviceComponent *dev; em->upper_com(&dev, NULL);
	while (dev != NULL) {
		if (strcmp(dev->_dev_id, devid) == 0)
			return dev;
		dev = em->next_com(dev);
	}
	return NULL;
}

ADeviceModule DM = { {
	ADeviceComponent::name(),
	ADeviceComponent::name(),
	sizeof(ADeviceComponent),
	&DM_init, &DM_exit,
},
	{ }, 0, { },
	&DM_push,
	&DM_pop,
	&DM_find,
	&DM_upper,
	&DM_next,
	&devcom_find,
};
static int reg_dm = AModuleRegister(&DM.module);	
