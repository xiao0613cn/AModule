#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "PvdNetCmd.h"


struct PVDRTStream {
	AObject   object;
	AObject  *io;
	PVDStatus status;
	DWORD     userid;
	DWORD     version;

	AMessage    outmsg;
	AMessage   *outfrom;
	SliceBuffer outbuf;
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

	free(rt);
}

static long PVDRTCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDRTStream *rt = (PVDRTStream*)malloc(sizeof(PVDRTStream));
	if (rt == NULL)
		return -ENOMEM;

	extern AModule PVDRTModule;
	AObjectInit(&rt->object, &PVDRTModule);

	rt->io = NULL;
	rt->status = pvdnet_invalid;
	rt->userid = 0;
	rt->version = PROTOCOL_V3;

	rt->outfrom = NULL;
	SliceInit(&rt->outbuf);

	if (parent != NULL) {
		AOption opt;
		memset(&opt, 0, sizeof(opt));
		INIT_LIST_HEAD(&opt.children_list);
		INIT_LIST_HEAD(&opt.brother_entry);

		strcpy_s(opt.name, "session_id");
		if (parent->getopt(parent, &opt) >= 0)
			rt->userid = atol(opt.value);

		strcpy_s(opt.name, "version");
		if (parent->getopt(parent, &opt) >= 0)
			rt->version = atol(opt.value);
	}

	AOption *io_option = AOptionFindChild(option, "io");
	long result = AObjectCreate(&rt->io, &rt->object, io_option, "tcp");

	*object = &rt->object;
	return result;
}

static long PVDRTTryOutput(PVDRTStream *rt)
{
	rt->outmsg.data = SliceCurPtr(&rt->outbuf);
	rt->outmsg.size = SliceCurLen(&rt->outbuf);

	long result = MAKE_TAG(rt->outmsg.data);
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
			TRACE("session(%d): PVDCmdDecode(%d) error = %d.\n", rt->userid, rt->outmsg.size, result);
			return result;
		}
	} else {
		TRACE("session(%d): unsupport format: 0x%p.\n", rt->userid, result);
		assert(FALSE);
		return -EAGAIN;
	}

	if ((result == 0) || (result > rt->outmsg.size)) {
		if (max(result,8*1024) > SliceCapacity(&rt->outbuf)) {
			long error = SliceResize(&rt->outbuf, ((result/4096)+1)*4096);
			if (error < 0)
				return error;
			else if (error > 0)
				TRACE("session(%d): resize buffer(%d) for data(%d - %d).\n",
				rt->userid, SliceCapacity(&rt->outbuf), result, rt->outmsg.size);
			rt->outmsg.data = SliceCurPtr(&rt->outbuf);
		}
		if (!(rt->outmsg.type & AMsgType_Custom)) {
			return 0;
		}
		if (result == 0) {
			SliceReset(&rt->outbuf);
			return 0;
		}
		result = rt->outmsg.size;
	} else {
		rt->outmsg.size = result;
	}
	return result;
}

static inline long PVDRTDoOutput(PVDRTStream *rt)
{
	AMsgInit(&rt->outmsg, AMsgType_Unknown, SliceResPtr(&rt->outbuf), SliceResLen(&rt->outbuf));
	return rt->io->request(rt->io, ARequest_Output, &rt->outmsg);
}

static long PVDRTOpenStatus(PVDRTStream *rt, long result)
{
	do {
		if ((result < 0) && (rt->status != pvdnet_disconnected))
			rt->status = pvdnet_closing;
		switch (rt->status)
		{
		case pvdnet_connecting:
			SliceReset(&rt->outbuf);
			if (SliceResize(&rt->outbuf, 64*1024) < 0) {
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

			rt->outmsg.type = AMsgType_Custom|NET_SDVR_REAL_PLAY;
			rt->outmsg.data = SliceResPtr(&rt->outbuf);
			rt->outmsg.size = PVDCmdEncode(rt->userid, rt->outmsg.data, NET_SDVR_REAL_PLAY, result);

			memset(rt->outmsg.data+sizeof(pvdnet_head), 0, result);
			STRUCT_SDVR_REALPLAY *rp = (STRUCT_SDVR_REALPLAY*)(rt->outmsg.data+sizeof(pvdnet_head));

			AOption *channel = AOptionFindChild(option, "channel");
			if (channel != NULL)
				rp->byChannel = atol(channel->value);

			AOption *linkmode = AOptionFindChild(option, "linkmode");
			if (linkmode != NULL)
				rp->byLinkMode = atol(linkmode->value);
		}
			rt->status = pvdnet_syn_login;
			result = rt->io->request(rt->io, ARequest_Input, &rt->outmsg);
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

static long PVDRTOpenDone(AMessage *msg, long result)
{
	PVDRTStream *rt = from_outmsg(msg);

	result = PVDRTOpenStatus(rt, result);
	if (result != 0)
		result = rt->outfrom->done(rt->outfrom, result);
	return result;
}

static long PVDRTOpen(AObject *object, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	rt->outmsg.type = AMsgType_Option;
	rt->outmsg.data = (char*)AOptionFindChild((AOption*)msg->data, "io");
	rt->outmsg.size = sizeof(AOption);

	rt->outmsg.done = &PVDRTOpenDone;
	rt->outfrom = msg;

	rt->status = pvdnet_connecting;
	long result = rt->io->open(rt->io, &rt->outmsg);
	if (result != 0)
		result = PVDRTOpenStatus(rt, result);
	return result;
}

static long PVDRTOutputStatus(PVDRTStream *rt)
{
	long result;
	do {
		SlicePush(&rt->outbuf, rt->outmsg.size);
		result = PVDRTTryOutput(rt);
		if (result < 0) {
			SlicePop(&rt->outbuf, 1);
			break;
		}
		if (result > 0) {
			SlicePop(&rt->outbuf, rt->outmsg.size);
			AMsgCopy(rt->outfrom, AMsgType_Custom|rt->status, rt->outmsg.data, rt->outmsg.size);
			break;
		}
		result = PVDRTDoOutput(rt);
	} while (result > 0);
	return result;
}

static long PVDRTOutputDone(AMessage *msg, long result)
{
	PVDRTStream *rt = from_outmsg(msg);
	if (result >= 0)
		result = PVDRTOutputStatus(rt);
	if (result != 0)
		result = rt->outfrom->done(rt->outfrom, result);
	return result;
}

static long PVDRTRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if (reqix != 0)
		return -ENOSYS;

	rt->outmsg.done = &PVDRTOutputDone;
	rt->outfrom = msg;

	rt->outmsg.data = NULL;
	rt->outmsg.size = 0;
	return PVDRTOutputStatus(rt);
}

static long PVDRTClose(AObject *object, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	return rt->io->close(rt->io, msg);
}

AModule PVDRTModule = {
	"stream",
	"PVDRTStream",
	sizeof(PVDRTStream),
	NULL, NULL,
	&PVDRTCreate,
	&PVDRTRelease,
	1,

	&PVDRTOpen,
	NULL,
	NULL,
	&PVDRTRequest,
	NULL,
	&PVDRTClose,
};
