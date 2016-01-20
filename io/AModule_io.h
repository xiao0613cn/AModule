#ifndef _AMODULE_IO_H_
#define _AMODULE_IO_H_


// class_name = "io"

enum AModule_ioRequest {
	Aio_Input = 0,
	Aio_Output,

	Aiosync_IndexMask    = 0x00ffffff,
	Aiosync_RequestFront = 0x02000000,
	Aiosync_NotifyFront  = 0x03000000,
	Aiosync_NotifyBack   = 0x04000000,
	Aiosync_NotifyDispath = 0x05000000,
};

static inline long
AioInput(AObject *obj, AMessage *msg) {
	if (obj->request == NULL)
		return -ENOSYS;
	return obj->request(obj, Aio_Input, msg);
}

static inline long
AioOutput(AObject *obj, AMessage *msg) {
	if (obj->request == NULL)
		return -ENOSYS;
	return obj->request(obj, Aio_Output, msg);
}



#endif
