#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


struct FileObject {
	AObject object;
	FILE   *fp;
};
#define to_file(obj)  container_of(obj, FileObject, object)

static void FileRelease(AObject *object)
{
	FileObject *fo = to_file(object);
	fclose(fo->fp);
}

static int FileCreate(AObject **object, AObject *parent, AOption *option)
{
	FileObject *fo = (FileObject*)*object;
	fo->fp = NULL;
	return 1;
}

static int FileOpen(AObject *object, AMessage *msg)
{
	FileObject *fo = to_file(object);
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	const char *mode = AOptionChild(option, "mode", "rwb");

	const char *path = AOptionChild(option, "path", NULL);
	if (path == NULL)
		return -EINVAL;

	fo->fp = fopen(path, mode);
	if (fo->fp == NULL)
		return -EIO;
	return 1;
}

static int FileRequest(AObject *object, int reqix, AMessage *msg)
{
	FileObject *fo = to_file(object);
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
	FileObject *fo = to_file(object);
	if (fo->fp == NULL)
		return -ENOENT;

	fclose(fo->fp);
	fo->fp = NULL;
	return 1;
}

AModule FileModule = {
	"io",
	"file",
	sizeof(FileObject),
	NULL, NULL,
	&FileCreate,
	&FileRelease,
	NULL,
	2,

	&FileOpen,
	NULL, NULL,
	&FileRequest,
	NULL,
	&FileClose,
};
