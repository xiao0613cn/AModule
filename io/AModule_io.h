#ifndef _AMODULE_IO_H_
#define _AMODULE_IO_H_


// class_name = "io"

enum AModule_ioRequest {
	Aio_Input = 0,
	Aio_Output,
	Aio_Error,
	Aio_InOutPair, // msg type: AMsgType_IOMsg

	Aiosync_IndexMask    = 0x00ffffff,
	Aiosync_RequestFront = 0x02000000,
	Aiosync_NotifyFront  = 0x03000000,
	Aiosync_NotifyBack   = 0x04000000,
	Aiosync_NotifyDispath = 0x05000000,
};

static inline int
ioInput(AObject *io, AMessage *msg) {
	return io->request(io, Aio_Input, msg);
}

static inline int
ioOutput(AObject *io, AMessage *msg) {
	return io->request(io, Aio_Output, msg);
}

#define ioMsgType_Block           (AMsgType_Class|0)
#define ioMsgType_isBlock(type)   ((type) & (AMsgType_Class|AMsgType_Private))


#endif
