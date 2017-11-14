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
	IOModule* m() { return container_of(_module, IOModule, module); }

	int open(AMessage *msg)   { return m()->open(this, msg); }
	int getopt(AOption *opt)  { return m()->getopt(this, opt); }
	int setopt(AOption *opt)  { return m()->setopt(this, opt); }
	int request(int reqix, AMessage *msg) { return m()->request(this, reqix, msg); }
	int cancel(int reqix, AMessage *msg) { return m()->cancel(this, reqix, msg); }
	int shutdown()            { return m()->close(this, NULL); }
	int close(AMessage *msg)  { return m()->close(this, msg); }

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

//////////////////////////////////////////////////////////////////////////
struct AService : public AObject {
	static const char* class_name() { return "AService"; }

	struct ASystemManager *sysmng; // set by user
	struct AService *parent;
	struct list_head children_list;
	struct list_head brother_entry;

	unsigned int save_option : 1;
	unsigned int require_child : 1;
	unsigned int post_start : 1;

	AOption *svc_option;
	AModule *peer_module;
	int    (*start)(AService *service, AOption *option);
	void   (*stop)(AService *service);
	int    (*run)(AService *service, AObject *peer, AOption *option);
	int    (*abort)(AService *service, AObject *peer);

	void init() {
		sysmng = NULL; parent = NULL;
		children_list.init(); brother_entry.init();
		save_option = require_child = post_start = 0;
		svc_option = NULL; peer_module = NULL;
		start = NULL; stop = NULL; run = NULL; abort = NULL;
	}
};
#define list_for_AService(pos, service) \
	list_for_each2(pos, &(service)->children_list, AService, brother_entry)

AMODULE_API int
AServiceStart(AService *service, AOption *option, BOOL create_chains);

AMODULE_API void
AServiceStop(AService *service, BOOL clean_chains);

AMODULE_API AService*
AServiceProbe(AService *server, AObject *object, AMessage *msg);

#endif
