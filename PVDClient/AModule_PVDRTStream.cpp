#include "stdafx.h"
#include "../base/AModule.h"


struct PVDRTStream {
	AObject object;
	AObject *io;
};
#define to_rt(obj) container_of(obj, PVDRTStream, object)

