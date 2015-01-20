#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "PvdNetCmd.h"
#include "md5.h"

typedef struct PVDClient PVDClient;
typedef struct PVDNetCmd PVDNetCmd;

enum PVDStatus {
	pvdnet_invalid,

	pvdnet_connecting,
	pvdnet_syn_md5id,
	pvdnet_ack_md5id,

	pvdnet_fin_md5id,
	pvdnet_reconnecting,
	pvdnet_syn_login,
	pvdnet_ack_login,

	pvdnet_con_devinfo,
	pvdnet_con_devinfo2,
	pvdnet_con_devinfox,
	pvdnet_con_stream,

	pvdnet_closing,
	pvdnet_syn_logout,
	pvdnet_ack_logout,
	pvdnet_disconnected,
};

struct PVDClient {
	AObject     object;
	AObject    *io;
	PVDStatus   status;
	DWORD       userid;
	BYTE        md5id;
	STRUCT_SDVR_DEVICE_EX device2;

	AMessage    outmsg;
	AMessage   *outfrom;
	SliceBuffer outbuf;
};
#define to_pvd(obj) container_of(obj, PVDClient, object);
#define from_outmsg(msg) container_of(msg, PVDClient, outmsg)

static void PVDRelease(AObject *object)
{
	PVDClient *pvd = to_pvd(object);
	release_s(pvd->io, AObjectRelease, NULL);
	SliceFree(&pvd->outbuf);

	free(pvd);
}

static long PVDCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDClient *pvd = (PVDClient*)malloc(sizeof(PVDClient));
	if (pvd == NULL)
		return -ENOMEM;

	extern AModule PVDClientModule;
	AObjectInit(&pvd->object, &PVDClientModule);
	pvd->io = NULL;
	pvd->status = pvdnet_invalid;
	pvd->userid = 0;
	pvd->md5id = 0;
	SliceInit(&pvd->outbuf);

	AOption *io_option = AOptionFindChild(option, "io");
	long result = AObjectCreate(&pvd->io, &pvd->object, io_option, "tcp");

	*object = &pvd->object;
	return result;
}

static long PVDOutMsg(PVDClient *pvd, pvdnet_head **phead)
{
	*phead = NULL;
	long result = PVDCmdDecode(pvd->userid, SliceCurPtr(&pvd->outbuf), SliceCurLen(&pvd->outbuf));
	if (result < 0)
		return -EFAULT;

	if ((result == 0) || (result > SliceCurLen(&pvd->outbuf))) {
		if (result > SliceCapacity(&pvd->outbuf)) {
			result = SliceResize(&pvd->outbuf, result);
			if (result < 0)
				return -ENOMEM;
		}
		if ((result == 0) && (pvd->outmsg.type & AMsgType_Custom)) {
			SliceReset(&pvd->outbuf);
		}
		if ((result == 0) || !(pvd->outmsg.type & AMsgType_Custom)) {
			AMsgInit(&pvd->outmsg, AMsgType_Unknown, SliceResPtr(&pvd->outbuf), SliceResLen(&pvd->outbuf));
			result = pvd->io->request(pvd->io, ARequest_Output, &pvd->outmsg);
			return result;
		}
	}

	*phead = (pvdnet_head*)SliceCurPtr(&pvd->outbuf);
	SlicePop(&pvd->outbuf, result);
	return result;
}

static void PVDOpenLogin(PVDClient *pvd)
{
	SliceReset(&pvd->outbuf);
	pvd->outmsg.type = AMsgType_Custom|NET_SDVR_LOGIN;
	pvd->outmsg.data = SliceCurPtr(&pvd->outbuf);
	pvd->outmsg.size = PVDCmdEncode(pvd->userid, pvd->outmsg.data, NET_SDVR_LOGIN, sizeof(STRUCT_SDVR_LOGUSER));

	const char *usr = "admin";
	AOption *usr_opt = AOptionFindChild((AOption*)pvd->outfrom->data, "username");
	if (usr_opt != NULL)
		usr = usr_opt->value;

	const char *pwd = "888888";
	AOption *pwd_opt = AOptionFindChild((AOption*)pvd->outfrom->data, "password");
	if (pwd_opt != NULL)
		pwd = pwd_opt->value;

	STRUCT_SDVR_LOGUSER *login = (STRUCT_SDVR_LOGUSER*)(pvd->outmsg.data+sizeof(pvdnet_head));
	strcpy_s(login->szUserName, usr);
	MD5_enc(pvd->md5id, (BYTE*)pwd, strlen(pwd), (BYTE*)login->szPassWord);
	login->dwNamelen = strlen(usr);
	login->dwPWlen = PASSWD_LEN;
}

static long PVDOpenStatus(PVDClient *pvd, long result)
{
	pvdnet_head *phead;
	do {
	if ((result < 0) && (result != pvdnet_disconnected))
		pvd->status = pvdnet_closing;

	switch (pvd->status)
	{
	case pvdnet_connecting:
		SliceReset(&pvd->outbuf);
		if (SliceResize(&pvd->outbuf, 8*1024) < 0) {
			result = -ENOMEM;
			break;
		}

		pvd->outmsg.type = AMsgType_Custom|NET_SDVR_MD5ID_GET;
		pvd->outmsg.data = SliceCurPtr(&pvd->outbuf);
		pvd->outmsg.size = PVDCmdEncode(pvd->userid, pvd->outmsg.data, NET_SDVR_MD5ID_GET, 0);

		pvd->status = pvdnet_syn_md5id;
		result = pvd->io->request(pvd->io, ARequest_Input, &pvd->outmsg);
		break;

	case pvdnet_syn_md5id:
		SliceReset(&pvd->outbuf);
		result = 0;
		pvd->status = pvdnet_ack_md5id;

	case pvdnet_ack_md5id:
		SlicePush(&pvd->outbuf, result);
		result = PVDOutMsg(pvd, &phead);
		if (phead == NULL)
			break;
		if ((phead->uCmd != NET_SDVR_MD5ID_GET) || (phead->uResult == 0)) {
			result = -EFAULT;
			break;
		}

		pvd->md5id = *(BYTE*)(phead+1);
		AMsgInit(&pvd->outmsg, AMsgType_Unknown, NULL, 0);

		pvd->status = pvdnet_fin_md5id;
		result = pvd->io->close(pvd->io, &pvd->outmsg);
		break;

	case pvdnet_fin_md5id:
		pvd->outmsg.type = AMsgType_Option;
		pvd->outmsg.data = (char*)AOptionFindChild((AOption*)pvd->outfrom->data, "io");
		pvd->outmsg.size = sizeof(AOption);

		pvd->status = pvdnet_reconnecting;
		result = pvd->io->open(pvd->io, &pvd->outmsg);
		break;

	case pvdnet_reconnecting:
		PVDOpenLogin(pvd);

		pvd->status = pvdnet_syn_login;
		result = pvd->io->request(pvd->io, ARequest_Input, &pvd->outmsg);
		break;

	case pvdnet_syn_login:
		SliceReset(&pvd->outbuf);
		result = 0;
		pvd->status = pvdnet_ack_login;

	case pvdnet_ack_login:
		SlicePush(&pvd->outbuf, result);
		result = PVDOutMsg(pvd, &phead);
		if (phead == NULL)
			break;
		if ((phead->uCmd != NET_SDVR_LOGIN) || (phead->uResult == 0)) {
			result = -EFAULT;
			break;
		}

		pvd->userid = phead->uUserId;
		memcpy(&pvd->device2, phead+1, min(sizeof(STRUCT_SDVR_DEVICE_EX), result-sizeof(pvdnet_head)));
		pvd->status = pvdnet_con_devinfo2;
		return result;

	case pvdnet_closing:
		AMsgInit(&pvd->outmsg, AMsgType_Unknown, NULL, 0);
		pvd->status = pvdnet_disconnected;
		result = pvd->io->close(pvd->io, &pvd->outmsg);
		if (result == 0)
			break;

	case pvdnet_disconnected:
		pvd->status = pvdnet_invalid;
		return -EFAULT;
	}
	} while (result != 0);
	return result;
}

static long PVDOpenDone(AMessage *msg, long result)
{
	PVDClient *pvd = from_outmsg(msg);
	result = PVDOpenStatus(pvd, result);
	if (result != 0)
		result = pvd->outfrom->done(pvd->outfrom, result);
	return result;
}

static long PVDOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	PVDClient *pvd = to_pvd(object);
	pvd->outmsg.done = &PVDOpenDone;
	pvd->outfrom = msg;

	pvd->outmsg.type = AMsgType_Option;
	pvd->outmsg.data = (char*)AOptionFindChild((AOption*)msg->data, "io");
	pvd->outmsg.size = sizeof(AOption);

	pvd->status = pvdnet_connecting;
	long result = pvd->io->open(pvd->io, &pvd->outmsg);
	if (result != 0)
		result = PVDOpenStatus(pvd, result);
	return result;
}

static long PVDOutputStatus(PVDClient *pvd, long result)
{
	pvdnet_head *phead;
	do {
		SlicePush(&pvd->outbuf, result);
		result = PVDOutMsg(pvd, &phead);
		if (phead == NULL)
			continue;

		AMessage *msg = pvd->outfrom;
		msg->type = AMsgType_Custom|phead->uCmd;
		if ((msg->data != NULL) && (msg->size != 0)) {
			memcpy(msg->data, phead, min(msg->size,result));
		} else {
			msg->data = (char*)phead;
			msg->size = result;
		}
		break;
	} while (result > 0);
	return result;
}

static long PVDOutputDone(AMessage *msg, long result)
{
	PVDClient *pvd = from_outmsg(msg);
	if (result >= 0)
		result = PVDOutputStatus(pvd, result);
	if (result != 0)
		result = pvd->outfrom->done(pvd->outfrom, result);
	return result;
}

static long PVDRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDClient *pvd = to_pvd(object);
	if (reqix != ARequest_Output) {
		if ((reqix == ARequest_Input) && (msg->type & AMsgType_Custom)) {
			pvdnet_head *phead = (pvdnet_head*)msg->data;
			phead->uUserId = pvd->userid;
		}
		return pvd->io->request(pvd->io, reqix, msg);
	}

	pvd->outmsg.done = &PVDOutputDone;
	pvd->outfrom = msg;
	return PVDOutputStatus(pvd, 0);
}

static long PVDCloseStatus(PVDClient *pvd, long result)
{
	pvdnet_head *phead = NULL;
	do {
	if ((result < 0) && (pvd->status != pvdnet_disconnected))
		pvd->status = pvdnet_closing;

	switch (pvd->status)
	{
	case pvdnet_syn_logout:
		pvd->status = pvdnet_ack_logout;
		result = 0;

	case pvdnet_ack_logout:
		if (phead == NULL)
			SlicePush(&pvd->outbuf, result);
		result = PVDOutMsg(pvd, &phead);
		if (phead == NULL)
			break;
		if (phead->uCmd != NET_SDVR_LOGOUT)
			break;
		pvd->outfrom->type = AMsgType_Custom|NET_SDVR_LOGOUT;
		pvd->outfrom->data = (char*)phead;
		pvd->outfrom->size = result;

	case pvdnet_closing:
		AMsgInit(&pvd->outmsg, AMsgType_Unknown, NULL, 0);
		pvd->status = pvdnet_disconnected;
		result = pvd->io->close(pvd->io, &pvd->outmsg);
		if (result == 0)
			break;

	case pvdnet_disconnected:
		pvd->status = pvdnet_invalid;
		return result;

	default:
		return -EACCES;
	}
	} while (result != 0);
	return result;
}

static long PVDCloseDone(AMessage *msg, long result)
{
	PVDClient *pvd = from_outmsg(msg);
	result = PVDCloseStatus(pvd, result);
	if (result != 0)
		result = pvd->outfrom->done(pvd->outfrom, 1);
	return result;
}

static long PVDClose(AObject *object, AMessage *msg)
{
	PVDClient *pvd = to_pvd(object);
	if (msg == NULL)
		return pvd->io->close(pvd->io, NULL);

	pvd->outmsg.done = &PVDCloseDone;
	pvd->outfrom = msg;

	long result;
	if (pvd->status < pvdnet_con_devinfo) {
		result = -ENOENT;
	} else {
		SliceReset(&pvd->outbuf);
		pvd->outmsg.type = AMsgType_Custom|NET_SDVR_LOGOUT;
		pvd->outmsg.data = SliceCurPtr(&pvd->outbuf);
		pvd->outmsg.size = PVDCmdEncode(pvd->userid, pvd->outmsg.data, NET_SDVR_LOGOUT, 0);

		pvd->status = pvdnet_syn_logout;
		result = pvd->io->request(pvd->io, ARequest_Input, &pvd->outmsg);
	}
	if (result != 0)
		result = PVDCloseStatus(pvd, result);
	return result;
}

AModule PVDClientModule = {
	"stream",
	"PVDClient",
	sizeof(PVDClient),
	NULL, NULL,
	&PVDCreate, &PVDRelease,
	2,

	&PVDOpen,
	NULL, NULL,
	&PVDRequest,
	NULL,
	&PVDClose,
};
