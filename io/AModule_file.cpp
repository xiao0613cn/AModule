#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


struct FileObject : public IOObject {
	FILE   *fp;
};

static void FileRelease(AObject *object)
{
	FileObject *fo = (FileObject*)object;
	if_not(fo->fp, NULL, fclose);
}

static int FileCreate(AObject **object, AObject *parent, AOption *option)
{
	FileObject *fo = (FileObject*)*object;
	fo->fp = NULL;
	return 1;
}

static int FileOpen(AObject *object, AMessage *msg)
{
	FileObject *fo = (FileObject*)object;
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	const char *mode = AOptionGet(option, "mode", "rwb");

	const char *path = AOptionGet(option, "path", NULL);
	if (path == NULL)
		return -EINVAL;

	fo->fp = fopen(path, mode);
	if (fo->fp == NULL)
		return -EIO;
	return 1;
}

static int FileRequest(AObject *object, int reqix, AMessage *msg)
{
	FileObject *fo = (FileObject*)object;
	int result = 0;

	if (reqix == Aio_Input) {
		do {
			int len = fwrite(msg->data+result, msg->size-result, 1, fo->fp);
			if (len <= 0)
				return -EIO;

			result += len;
		} while (ioMsgType_isBlock(msg->type) && (result < msg->size));

		msg->size = result;
		return result;
	}
	if (reqix == Aio_Output) {
		do {
			int len = fread(msg->data+result, msg->size-result, 1, fo->fp);
			if (len <= 0)
				return -EIO;

			result += len;
		} while (ioMsgType_isBlock(msg->type) && (result < msg->size));

		msg->size = result;
		return result;
	}
	return -ENOSYS;
}

static int FileClose(AObject *object, AMessage *msg)
{
	FileObject *fo = (FileObject*)object;
	if (msg == NULL)
		return -ENOSYS;
	if (fo->fp == NULL)
		return -ENOENT;

	fclose(fo->fp);
	fo->fp = NULL;
	return 1;
}

IOModule FileModule = { {
	"io",
	"file",
	sizeof(FileObject),
	NULL, NULL,
	&FileCreate,
	&FileRelease,
	NULL, },
	&FileOpen,
	&IOModule::OptNull,
	&IOModule::OptNull,
	&FileRequest,
	&IOModule::ReqNull,
	&FileClose,
};

static auto_reg_t reg(FileModule.module);
