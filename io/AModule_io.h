#ifndef _AMODULE_IO_H_
#define _AMODULE_IO_H_


// class_name = "io"

enum AModule_ioRequest {
	Aio_RequestInput = 0,
	Aio_RequestOutput,
	Aio_RequestSeek,
	Aio_RequestCount,
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
