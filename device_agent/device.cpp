#include "../stdafx.h"
#include "device.h"
#include "../ecs/AEvent.h"
#pragma comment(lib, "../bin_win32/AModule.lib")

static int devcom_cmp(const char *key, ADeviceComponent *data) {
	return strcmp(key, data->_dev_id);
}
rb_tree_define(ADeviceComponent, _devmap_node, const char*, devcom_cmp)

extern ADeviceComponentModule DCM;

static int DCM_push(ADeviceComponent *dev, AEventManager *ev)
{
	ADeviceComponent *old_dev = DCM._find(dev->_dev_id);
	if (old_dev != NULL)
		DCM._pop(old_dev, ev);

	old_dev = rb_insert_ADeviceComponent(&DCM.dev_map, dev, dev->_dev_id);
	assert(old_dev == NULL);
	dev->_entity->addref();
	return 1;
}

static int DCM_pop(ADeviceComponent *dev, AEventManager *ev)
{
	if (RB_EMPTY_NODE(&dev->_devmap_node))
		return -1;

	rb_erase(&dev->_devmap_node, &DCM.dev_map);
	RB_CLEAR_NODE(&dev->_devmap_node);
	if (ev != NULL)
		ev->emit_by_name(ev, "on_dev_pop", dev);
	dev->_entity->release();
	return 1;
}

static ADeviceComponent* DCM_find(const char *devid)
{
	return rb_find_ADeviceComponent(&DCM.dev_map, devid);
}

static ADeviceComponent* DCM_upper(const char *devid)
{
	if (RB_EMPTY_ROOT(&DCM.dev_map))
		return NULL;
	if (devid == NULL)
		return rb_first_entry(&DCM.dev_map, ADeviceComponent, _devmap_node);
	return rb_upper_ADeviceComponent(&DCM.dev_map, devid);
}

static ADeviceComponent* DCM_next(ADeviceComponent *dev)
{
	rb_node *node = rb_next(&dev->_devmap_node);
	if (node == NULL)
		return NULL;
	return rb_entry(node, ADeviceComponent, _devmap_node);
}

static int DCM_init(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
		INIT_RB_ROOT(&DCM.dev_map);
		DCM.dev_count = 0;
		pthread_mutex_init(&DCM.dev_mutex, NULL);
	}
	return 1;
}

static void DCM_exit(int inited)
{
	if (inited > 0) {
		pthread_mutex_destroy(&DCM.dev_mutex);
	}
}

static int dev_com_ptz_ctrl(ADeviceComponent *dev, ADeviceComponent::ptz_req *req)
{
	AModule *m = dev->_entity->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int dev_com_get_rec(ADeviceComponent *dev, ADeviceComponent::rec_req *req)
{
	AModule *m = dev->_entity->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int dev_com_exctrl(ADeviceComponent *dev, const char *cmd, AOption *req, AOption *resp)
{
	AModule *m = dev->_entity->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int dev_com_create(AObject **object, AObject *parent, AOption *options)
{
	ADeviceComponent *dev = (ADeviceComponent*)*object;

	strcpy_sz(dev->_dev_id,     options->getStr("devid", NULL));
	strcpy_sz(dev->_net_addr,   options->getStr("net_addr", NULL));
	dev->_net_port            = options->getInt("net_port", 0);
	strcpy_sz(dev->_login_user, options->getStr("login_user", NULL));
	strcpy_sz(dev->_login_pwd,  options->getStr("login_pwd", NULL));

	dev->_chan_count = dev->_sensor_count = dev->_alarmout_count = 0;
	dev->_private_sn[0] = '\0';
	return 1;
}

static ADeviceComponent* dev_com_find(AEntityManager *em, const char *devid)
{
	ADeviceComponent *dev; em->upper_com(&dev, NULL);
	while (dev != NULL) {
		if (strcmp(dev->_dev_id, devid) == 0)
			return dev;
		dev = em->next_com(dev);
	}
	return NULL;
}

ADeviceComponentModule DCM = { {
	ADeviceComponent::name(),
	ADeviceComponent::name(),
	sizeof(ADeviceComponent),
	&DCM_init, &DCM_exit,
},
	{ }, 0, { },
	&DCM_push,
	&DCM_pop,
	&DCM_find,
	&DCM_upper,
	&DCM_next,
	&dev_com_find,
};
static int reg_dm = AModuleRegister(&DCM.module);	
