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

enum { ioMsgType_Block = (AMsgType_Class|0) };
#define ioMsgType_isBlock(type)   ((type) & (AMsgType_Class|AMsgType_Private))

//<<<<<<<<<< C style <<<<<<<<<
struct IOModule {
	AModule module;
	int (*open)(AObject *object, AMessage *msg);
	int (*setopt)(AObject *object, AOption *option);
	int (*getopt)(AObject *object, AOption *option);
	int (*request)(AObject *object, int reqix, AMessage *msg);
	int (*cancel)(AObject *object, int reqix, AMessage *msg);
	int (*close)(AObject *object, AMessage *msg);

	AModule *svc_module;
	int  (*svc_accept)(AObject *object, AMessage *msg, AObject *svc_data, AOption *svc_opt);

	static int MsgNull(AObject *other, AMessage *msg) { return -ENOSYS; }
	static int OptNull(AObject *object, AOption *option) { return -ENOSYS; }
	static int ReqNull(AObject *other, int reqix, AMessage *msg) { return -ENOSYS; }
};

struct IOObject : public AObject {
	static const char* class_name() { return "io"; }
	IOModule* M() { return container_of(this->_module, IOModule, module); }

	int open(AMessage *msg)   { return M()->open(this, msg); }
	int getopt(AOption *opt)  { return M()->getopt(this, opt); }
	int setopt(AOption *opt)  { return M()->setopt(this, opt); }
	int request(int reqix, AMessage *msg) { return M()->request(this, reqix, msg); }
	int cancel(int reqix, AMessage *msg) { return M()->cancel(this, reqix, msg); }
	int shutdown()            { return M()->close(this, NULL); }
	int close(AMessage *msg)  { return M()->close(this, msg); }

	int input(AMessage *msg)  { return request(Aio_Input, msg); }
	int output(AMessage *msg) { return request(Aio_Output, msg); }

#ifdef _BUF_UTIL_H_
	int input(AMessage *msg, ARefsBuf *buf, int type = ioMsgType_Block) {
		msg->init(type, buf->ptr(), buf->len());
		return input(msg);
	}
	int output(AMessage *msg, ARefsBuf *buf, int type = AMsgType_Unknown) {
		msg->init(type, buf->next(), buf->left());
		return output(msg);
	}
#endif
};
//>>>>>>>>>> C Style >>>>>>>>>>

//<<<<<<<<<< C++ Style <<<<<<<<<<
struct IOObject2 : public AObject {
	static const char* class_name() { return "io"; }

	virtual int open(AMessage *msg) = NULL;
	virtual int input(AMessage *msg) = NULL;
	virtual int output(AMessage *msg) = NULL;
	virtual int close(AMessage *msg) = NULL;
};
//>>>>>>>>>> C++ Style >>>>>>>>>>


#endif
