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


#endif
