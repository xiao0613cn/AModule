#ifndef _AMODULE_IO_H_
#define _AMODULE_IO_H_


// class_name = "io"

enum AModule_ioRequest {
	Aio_RequestInput = 0,
	Aio_RequestOutput,
	Aio_RequestCount,

	Aiosync_IndexMask    = 0x00ffffff,
	Aiosync_RequestFront = 0x02000000,
	Aiosync_NotifyFront  = 0x03000000,
	Aiosync_NotifyBack   = 0x04000000,
};

static inline long
Aio_Input(AObject *obj, AMessage *msg) {
	if (obj->request == NULL)
		return -ENOSYS;
	return obj->request(obj, Aio_RequestInput, msg);
}

static inline long
Aio_Output(AObject *obj, AMessage *msg) {
	if (obj->request == NULL)
		return -ENOSYS;
	return obj->request(obj, Aio_RequestOutput, msg);
}



#endif
