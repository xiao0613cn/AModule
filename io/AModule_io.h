#ifndef _AMODULE_IO_H_
#define _AMODULE_IO_H_


// class_name = "io"

enum AModule_ioRequest {
	Aio_Input = 0,
	Aio_Output,
	Aio_AppendOutput,
	Aio_InOutPair, // msg type: AMsgType_InOutMsg

	Aiosync_IndexMask    = 0x00ffffff,
	Aiosync_RequestFront = 0x02000000,
	Aiosync_NotifyFront  = 0x03000000,
	Aiosync_NotifyBack   = 0x04000000,
	Aiosync_NotifyDispath = 0x05000000,
};
/*
static inline int
ioInput(AObject *io, AMessage *msg) {
	return io->request(Aio_Input, msg);
}

static inline int
ioOutput(AObject *io, AMessage *msg) {
	return io->request(Aio_Output, msg);
}

static inline int
ioOutput(AObject *io, AMessage *msg, void *data, int size, int type = AMsgType_Unknown) {
	AMsgInit(msg, type, data, size);
	return io->request(Aio_Output, msg);
}

static inline int
ioOutput(AObject *io, AMessage *msg, ARefsBuf *buf, int type = AMsgType_Unknown) {
	AMsgInit(msg, type, buf->next(), buf->left());
	return io->request(Aio_Output, msg);
}
*/
#define ioMsgType_Block           (AMsgType_Class|0)
#define ioMsgType_isBlock(type)   ((type) & (AMsgType_Class|AMsgType_Private))

#define httpMsgType_RawData       (AMsgType_Private|1)
#define httpMsgType_RawBlock      (AMsgType_Private|2)

//<<<<<<<<<< C style <<<<<<<<<
struct IOModule {
	AModule module;
	int (*open)(AObject *object, AMessage *msg);
	int (*setopt)(AObject *object, AOption *option);
	int (*getopt)(AObject *object, AOption *option);
	int (*request)(AObject *object, int reqix, AMessage *msg);
	int (*cancel)(AObject *object, int reqix, AMessage *msg);
	int (*close)(AObject *object, AMessage *msg);

	static int MsgNull(AObject *other, AMessage *msg) { return -ENOSYS; }
	static int OptNull(AObject *object, AOption *option) { return -ENOSYS; }
	static int ReqNull(AObject *other, int reqix, AMessage *msg) { return -ENOSYS; }
};

struct IOObject : public AObject {
	static const char* class_name() { return "io"; }
	IOModule* operator->() { return container_of(_module, IOModule, module); }

	int open(AMessage *msg)   { return (*this)->open(this, msg); }
	int getopt(AOption *opt)  { return (*this)->getopt(this, opt); }
	int setopt(AOption *opt)  { return (*this)->setopt(this, opt); }
	int request(int reqix, AMessage *msg) { return (*this)->request(this, reqix, msg); }
	int cancel(int reqix, AMessage *msg) { return (*this)->cancel(this, reqix, msg); }
	int close(AMessage *msg)  { return (*this)->close(this, msg); }

	int input(AMessage *msg)  { return request(Aio_Input, msg); }
	int input(AMessage *msg, ARefsBuf *buf, int type = ioMsgType_Block) {
		msg->init(type, buf->ptr(), buf->len());
		return input(msg);
	}
	int output(AMessage *msg) { return request(Aio_Output, msg); }
	int output(AMessage *msg, ARefsBuf *buf, int type = AMsgType_Unknown) {
		msg->init(type, buf->next(), buf->left());
		return output(msg);
	}
	int shutdown()            { return (*this)->close(this, NULL); }
};

struct AService {
	AModule module;
	int  (*svr_init)(AObject *server, AOption *option); // option
	int  (*svr_exit)(AObject *server, AOption *option); // option
	int  (*svc_run)(AObject *object, AOption *option);
	int  (*svc_abort)(AObject *object); // option
};
//>>>>>>>>>> C Style >>>>>>>>>>
//<<<<<<<<<< C++ Style <<<<<<<<<<
struct IOObject2 : public AObject {
	static const char* name() { return "io"; }

	virtual int open(AMessage *msg) = NULL;
	virtual int input(AMessage *msg) = NULL;
	virtual int output(AMessage *msg) = NULL;
	virtual int close(AMessage *msg) = NULL;
};
//>>>>>>>>>> C++ Style >>>>>>>>>>

#endif
