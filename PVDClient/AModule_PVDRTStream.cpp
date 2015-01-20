#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "PvdNetCmd.h"


struct PVDRTStream {
	AObject   object;
	AObject  *io;
	PVDStatus status;
	DWORD     userid;

	AMessage    outmsg;
	AMessage   *outfrom;
	SliceBuffer outbuf;
};
#define to_rt(obj) container_of(obj, PVDRTStream, object)
#define from_outmsg(msg) container_of(msg, PVDRTStream, outmsg)

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

	rt->outfrom = NULL;
	SliceInit(&rt->outbuf);

	AOption *io_option = AOptionFindChild(option, "io");
	long result = AObjectCreate(&rt->io, &rt->object, io_option, "tcp");

	*object = &rt->object;
	return result;
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
			AOption *session_id = AOptionFindChild(option, "session_id");
			AOption *channel = AOptionFindChild(option, "channel");
			AOption *linkmode = AOptionFindChild(option, "linkmode");
			AOption *version = AOptionFindChild(option, "version");

			if ((version != NULL) && (strcmp(version->value, "2.0") == 0)) {
				result = sizeof(STRUCT_SDVR_REALPLAY);
			} else {
				result = sizeof(STRUCT_SDVR_REALPLAY_EX);
			}
			if (session_id != NULL)
				rt->userid = atol(session_id->value);

			rt->outmsg.type = AMsgType_Custom|NET_SDVR_REAL_PLAY;
			rt->outmsg.data = SliceCurPtr(&rt->outbuf);
			rt->outmsg.size = PVDCmdEncode(rt->userid, rt->outmsg.data, NET_SDVR_REAL_PLAY, result);

			memset(rt->outmsg.data+sizeof(pvdnet_head), 0, result);
			STRUCT_SDVR_REALPLAY *rp = (STRUCT_SDVR_REALPLAY*)(rt->outmsg.data+sizeof(pvdnet_head));
			if (channel != NULL)
				rp->byChannel = atol(channel->value);
			if (linkmode != NULL)
				rp->byLinkMode = atol(linkmode->value);
		}
			rt->status = pvdnet_syn_login;
			result = rt->io->request(rt->io, ARequest_Input, &rt->outmsg);
			break;

		case pvdnet_syn_login:
			SliceReset(&rt->outbuf);
			result = 0;
			rt->status = pvdnet_ack_login;

		case pvdnet_ack_login:
			SlicePush(&rt->outbuf, result);
			result = PVDCmdDecode(rt->userid, SliceCurPtr(&rt->outbuf), SliceCurLen(&rt->outbuf));
			if (result < 0)
				break;
			if (((result == 0) || (result > SliceCurLen(&rt->outbuf))) && !(rt->outmsg.type & AMsgType_Custom)) {
				AMsgInit(&rt->outmsg, AMsgType_Unknown, SliceResPtr(&rt->outbuf), SliceResLen(&rt->outbuf));
				result = rt->io->request(rt->io, ARequest_Output, &rt->outmsg);
				break;
			}
		}
	} while (result != 0);
	return result;
}

static long PVDRTOpenDone(AMessage *msg, long result)
{
	PVDRTStream *rt = from_outmsg(msg);
	msg = rt->outfrom;

	result = PVDRTOpenStatus(rt, result);
	if (result != 0)
		result = msg->done(msg, result);
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
