#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../base/SliceBuffer.h"
#include "../io/AModule_io.h"
#include "PvdNetCmd.h"


struct PVDRTStream {
	AObject   object;
	AObject  *io;
	PVDStatus status;
	DWORD     userid;
	DWORD     version;

	AMessage  outmsg;
	AMessage *outfrom;
	SliceBuffer outbuf;
	int       retry_count;
};
#define to_rt(obj) container_of(obj, PVDRTStream, object)
#define from_outmsg(msg) container_of(msg, PVDRTStream, outmsg)

#define TAG_SIZE  4
#define MAKE_TAG(ptr)  (BYTE(ptr[0])|(BYTE(ptr[1])<<8)|(BYTE(ptr[2])<<16)|(BYTE(ptr[3])<<24))

static void PVDRTRelease(AObject *object)
{
	PVDRTStream *rt = to_rt(object);
	release_s(rt->io, AObjectRelease, NULL);
	SliceFree(&rt->outbuf);
}

static int PVDRTCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDRTStream *rt = (PVDRTStream*)*object;
	rt->io = NULL;
	rt->status = pvdnet_invalid;
	rt->userid = 0;
	rt->version = PROTOCOL_V3;

	rt->outfrom = NULL;
	SliceInit(&rt->outbuf);
	rt->retry_count = 0;

	if (parent != NULL) {
		AOption opt;
		AOptionInit(&opt, NULL);

		strcpy_sz(opt.name, "session_id");
		if (parent->getopt(parent, &opt) >= 0)
			rt->userid = atol(opt.value);

		strcpy_sz(opt.name, "version");
		if (parent->getopt(parent, &opt) >= 0)
			rt->version = atol(opt.value);
	}

	AOption *io_option = AOptionFind(option, "io");
	AObjectCreate(&rt->io, &rt->object, io_option, NULL);
	return 1;;//result;
}

static int PVDRTTryOutput(PVDRTStream *rt)
{
	rt->outmsg.data = SliceCurPtr(&rt->outbuf);
	rt->outmsg.size = SliceCurLen(&rt->outbuf);

	int result = MAKE_TAG(rt->outmsg.data);
	if (rt->outmsg.size < TAG_SIZE) {
		result = 0;
	} else if (ISMSHEAD(rt->outmsg.data)) {
		if (result == MSHDV2_FLAG) {
			rt->status = pvdnet_con_devinfo2;
		} else {
			rt->status = pvdnet_con_devinfo;
		}
		if (rt->outmsg.size < sizeof(MSHEAD)) {
			result = 0;
		} else {
			result = MSHEAD_GETFRAMESIZE(rt->outmsg.data);
		}
	} else if (result == STREAM_HEADER_FLAG) {
		rt->status = pvdnet_con_devinfox;
		if (rt->outmsg.size < sizeof(STREAM_HEADER)) {
			result = 0;
		} else {
			STREAM_HEADER *p = (STREAM_HEADER*)rt->outmsg.data;
			result = p->nHeaderSize + p->nEncodeDataSize;
		}
	} else if (result == NET_CMD_HEAD_FLAG) {
		rt->status = pvdnet_con_stream;
		result = PVDCmdDecode(rt->userid, rt->outmsg.data, rt->outmsg.size);
		if (result < 0) {
			TRACE("%p: PVDCmdDecode(%d) error = %d.\n", rt, rt->outmsg.size, result);
			return result;
		}
	} else {
		if (rt->retry_count == 0)
			TRACE2("%p: unsupport format: 0x%p, left size = %d.\n", rt, result, rt->outmsg.size);
		//assert(FALSE);
		return -EAGAIN;
	}

	if ((result == 0) || (result > rt->outmsg.size)) {
		if (max(result,8*1024) > SliceCapacity(&rt->outbuf)) {
			int error = SliceResize(&rt->outbuf, result, 8*1024);
			if (error < 0)
				return error;
			else if (error > 0)
				TRACE("%p: %s buffer(%d) for data(%d - %d).\n",
					rt, (error?"resize":"rewind"), rt->outbuf.siz, result, rt->outmsg.size);
			rt->outmsg.data = SliceCurPtr(&rt->outbuf);
		}
		if (!ioMsgType_isBlock(rt->outmsg.type)) {
			return 0;
		}
		if (result == 0) {
			TRACE("%p: reset buffer(%d), drop data(%d).\n", rt, rt->outbuf.siz, rt->outmsg.size);
			SliceReset(&rt->outbuf);
			return 0;
		}
		result = rt->outmsg.size;
	} else {
		rt->outmsg.size = result;
	}
	return result;
}

static inline int PVDRTDoOutput(PVDRTStream *rt)
{
	AMsgInit(&rt->outmsg, AMsgType_Unknown, SliceResPtr(&rt->outbuf), SliceResLen(&rt->outbuf));
	return ioOutput(rt->io, &rt->outmsg);
}

int PVDRTOpenStatus(PVDRTStream *rt, int result)
{
	do {
		if ((result < 0) && (rt->status != pvdnet_disconnected))
			rt->status = pvdnet_closing;
		switch (rt->status)
		{
		case pvdnet_connecting:
			SliceReset(&rt->outbuf);
			if (SliceResize(&rt->outbuf, 64*1024, 8*1024) < 0) {
				result = -ENOMEM;
				break;
			}
		{
			AOption *option = (AOption*)rt->outfrom->data;
			if (rt->version != PROTOCOL_V3) {
				result = sizeof(STRUCT_SDVR_REALPLAY);
			} else {
				result = sizeof(STRUCT_SDVR_REALPLAY_EX);
			}

			rt->outmsg.type = ioMsgType_Block;
			rt->outmsg.data = SliceResPtr(&rt->outbuf);
			rt->outmsg.size = PVDCmdEncode(rt->userid, rt->outmsg.data, NET_SDVR_REAL_PLAY, result);

			memset(rt->outmsg.data+sizeof(pvdnet_head), 0, result);
			STRUCT_SDVR_REALPLAY *rp = (STRUCT_SDVR_REALPLAY*)(rt->outmsg.data+sizeof(pvdnet_head));

			AOption *channel = AOptionFind(option, "channel");
			if (channel != NULL)
				rp->byChannel = atol(channel->value);

			AOption *linkmode = AOptionFind(option, "linkmode");
			if (linkmode != NULL)
				rp->byLinkMode = atol(linkmode->value);
		}
			rt->status = pvdnet_syn_login;
			result = ioInput(rt->io, &rt->outmsg);
			break;

		case pvdnet_syn_login:
			rt->status = pvdnet_ack_login;
			result = PVDRTDoOutput(rt);
			break;

		case pvdnet_ack_login:
			SlicePush(&rt->outbuf, rt->outmsg.size);
			result = PVDRTTryOutput(rt);
			if (result < 0)
				break;
			if (result == 0) {
				rt->status = pvdnet_ack_login;
				result = PVDRTDoOutput(rt);
				break;
			}
			assert(rt->status != pvdnet_ack_login);
			if (rt->status == pvdnet_con_stream) {
				pvdnet_head *phead = (pvdnet_head*)rt->outmsg.data;
				if (phead->uResult == 0)
					return -EFAULT;

				SlicePop(&rt->outbuf, rt->outmsg.size);
				rt->outmsg.data = NULL;
				rt->outmsg.size = 0;
				rt->status = pvdnet_ack_login;
				break;
			}
			return result;

		case pvdnet_closing:
			AMsgInit(&rt->outmsg, AMsgType_Unknown, NULL, 0);
			rt->status = pvdnet_disconnected;
			result = rt->io->close(rt->io, &rt->outmsg);
			if (result == 0)
				return 0;

		case pvdnet_disconnected:
			rt->status = pvdnet_invalid;
			return -EFAULT;

		default:
			assert(FALSE);
			return -EACCES;
		}
	} while (result != 0);
	return result;
}

static int PVDRTOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	PVDRTStream *rt = to_rt(object);
	rt->outmsg.done = &TObjectDone(PVDRTStream, outmsg, outfrom, PVDRTOpenStatus);
	rt->outfrom = msg;

	if (rt->io == NULL) {
		AOption *io_opt = AOptionFind((AOption*)rt->outfrom->data, "io");
		if (io_opt == NULL)
			return -EINVAL;

		int result = AObjectCreate(&rt->io, &rt->object, io_opt, NULL);
		if (result < 0)
			return result;
	}

	rt->outmsg.type = AMsgType_Option;
	rt->outmsg.data = (char*)AOptionFind((AOption*)msg->data, "io");
	rt->outmsg.size = 0;

	rt->status = pvdnet_connecting;
	int result = rt->io->open(rt->io, &rt->outmsg);
	if (result != 0)
		result = PVDRTOpenStatus(rt, result);
	return result;
}

static int PVDRTSetOption(AObject *object, AOption *option)
{
	PVDRTStream *rt = to_rt(object);
	if (_stricmp(option->name, "version") == 0) {
		rt->version = atol(option->value);
		return 1;
	}
	if (_stricmp(option->name, "session_id") == 0) {
		rt->userid = atol(option->value);
		return 1;
	}
	return -ENOSYS;
}

int PVDRTOutputStatus(PVDRTStream *rt, int result)
{
	if (result < 0)
		rt->outmsg.size = 0;
	do {
		SlicePush(&rt->outbuf, rt->outmsg.size);
		result = PVDRTTryOutput(rt);
		if (result < 0) {
			if (result != -EAGAIN)
				break;

			rt->retry_count++;
			SlicePop(&rt->outbuf, 1);
			rt->outmsg.size = 0;
			result = 1;
			continue;
		}
		if (result > 0) {
			if (rt->retry_count != 0) {
				TRACE("%p: re-probe stream head, count = %d.\n", rt, rt->retry_count);
				rt->retry_count = 0;
			}
			SlicePop(&rt->outbuf, rt->outmsg.size);
			AMsgCopy(rt->outfrom, AMsgType_Private|rt->status, rt->outmsg.data, rt->outmsg.size);
			break;
		}
		result = PVDRTDoOutput(rt);
	} while (result > 0);
	return result;
}

static int PVDRTRequest(AObject *object, int reqix, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if (reqix != 0)
		return -ENOSYS;

	rt->outmsg.done = &TObjectDone(PVDRTStream, outmsg, outfrom, PVDRTOutputStatus);
	rt->outfrom = msg;

	rt->outmsg.data = NULL;
	rt->outmsg.size = 0;
	return PVDRTOutputStatus(rt, 0);
}

static int PVDRTClose(AObject *object, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if (rt->io == NULL)
		return -ENOSYS;
	return rt->io->close(rt->io, msg);
}

AModule PVDRTModule = {
	"stream",
	"PVDRTStream",
	sizeof(PVDRTStream),
	NULL, NULL,
	&PVDRTCreate,
	&PVDRTRelease,
	NULL,
	1,

	&PVDRTOpen,
	&PVDRTSetOption,
	NULL,
	&PVDRTRequest,
	NULL,
	&PVDRTClose,
};
