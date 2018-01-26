#ifndef _AMODULE_DEVICE_MANAGER_H_
#define _AMODULE_DEVICE_MANAGER_H_
#include "../ecs/AEntity.h"
#include "../ecs/ASystem.h"


struct ADeviceSystem {
	ASystem system;

	static const char* name() { return "ADeviceSystem"; }
	static ADeviceSystem* get() {
		static ADeviceSystem *sys = (ADeviceSystem*)AModuleFind(name(), name());
		return sys;
	}
};


#endif
