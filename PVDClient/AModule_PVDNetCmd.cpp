#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "PvdNetCmd.h"
#include "md5.h"

typedef struct PVDClient PVDClient;
typedef struct PVDNetCmd PVDNetCmd;

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
	memset(&pvd->device2, 0, sizeof(pvd->device2));

	pvd->outfrom = NULL;
	SliceInit(&pvd->outbuf);

	AOption *io_option = AOptionFindChild(option, "io");
	long result = AObjectCreate(&pvd->io, &pvd->object, io_option, "tcp");

	*object = &pvd->object;
	return result;
}

static long PVDTryOutput(PVDClient *pvd)
{
	pvd->outmsg.data = SliceCurPtr(&pvd->outbuf);
	pvd->outmsg.size = SliceCurLen(&pvd->outbuf);

	long result = PVDCmdDecode(pvd->userid, pvd->outmsg.data, pvd->outmsg.size);
	if (result < 0)
		return result;

	if ((result == 0) || (result > pvd->outmsg.size)) {
		if (max(result,1024) > SliceCapacity(&pvd->outbuf)) {
			if (SliceResize(&pvd->outbuf, ((result/2048)+1)*2048) < 0)
				return -ENOMEM;
			pvd->outmsg.data = SliceCurPtr(&pvd->outbuf);
		}
		if (!(pvd->outmsg.type & AMsgType_Custom)) {
			return 0;
		}
		if (result == 0) {
			SliceReset(&pvd->outbuf);
			return 0;
		}
		result = pvd->outmsg.size;
	} else {
		pvd->outmsg.size = result;
	}

	SlicePop(&pvd->outbuf, pvd->outmsg.size);
	return result;
}

static inline long PVDDoOpen(PVDClient *pvd, PVDStatus status)
{
	pvd->outmsg.type = AMsgType_Option;
	pvd->outmsg.data = (char*)AOptionFindChild((AOption*)pvd->outfrom->data, "io");
	pvd->outmsg.size = sizeof(AOption);

	pvd->status = status;
	return pvd->io->open(pvd->io, &pvd->outmsg);
}

static inline long PVDDoClose(PVDClient *pvd, PVDStatus status)
{
	AMsgInit(&pvd->outmsg, AMsgType_Unknown, NULL, 0);

	pvd->status = status;
	return pvd->io->close(pvd->io, &pvd->outmsg);
}

static inline void PVDDoInput(PVDClient *pvd, PVDStatus status, long type, long body)
{
	pvd->outmsg.type = AMsgType_Custom|type;
	pvd->outmsg.data = SliceResPtr(&pvd->outbuf);
	pvd->outmsg.size = PVDCmdEncode(pvd->userid, pvd->outmsg.data, type, body);

	pvd->status = status;
}

static inline long PVDDoOutput(PVDClient *pvd)
{
	AMsgInit(&pvd->outmsg, AMsgType_Unknown, SliceResPtr(&pvd->outbuf), SliceResLen(&pvd->outbuf));
	return pvd->io->request(pvd->io, ARequest_Output, &pvd->outmsg);
}

static long PVDDoLogin(PVDClient *pvd, PVDStatus status)
{
	SliceReset(&pvd->outbuf);
	PVDDoInput(pvd, status, NET_SDVR_LOGIN, sizeof(STRUCT_SDVR_LOGUSER));

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

	return pvd->io->request(pvd->io, ARequest_Input, &pvd->outmsg);
}

static long PVDOpenStatus(PVDClient *pvd, long result)
{
	pvdnet_head *phead;
	do {
		if ((result < 0) && (pvd->status != pvdnet_disconnected))
			pvd->status = pvdnet_closing;
		switch (pvd->status)
		{
		case pvdnet_connecting:
			SliceReset(&pvd->outbuf);
			if (SliceResize(&pvd->outbuf, 8*1024) < 0) {
				result = -ENOMEM;
			} else {
				PVDDoInput(pvd, pvdnet_syn_md5id, NET_SDVR_MD5ID_GET, 0);
				result = pvd->io->request(pvd->io, ARequest_Input, &pvd->outmsg);
			}
			break;

		case pvdnet_syn_md5id:
			pvd->status = pvdnet_ack_md5id;
			result = PVDDoOutput(pvd);
			break;

		case pvdnet_ack_md5id:
			SlicePush(&pvd->outbuf, pvd->outmsg.size);
			result = PVDTryOutput(pvd);
			if (result < 0)
				break;
			if (result == 0) {
				result = PVDDoOutput(pvd);
				break;
			}
			phead = (pvdnet_head*)pvd->outmsg.data;
			if ((phead->uCmd != NET_SDVR_MD5ID_GET) || (phead->uResult == 0)) {
				result = -EFAULT;
				break;
			}
			pvd->md5id = *(BYTE*)(phead+1);
			result = PVDDoClose(pvd, pvdnet_fin_md5id);
			if (result == 0)
				break;

		case pvdnet_fin_md5id:
			result = PVDDoOpen(pvd, pvdnet_reconnecting);
			break;

		case pvdnet_reconnecting:
			result = PVDDoLogin(pvd, pvdnet_syn_login);
			break;

		case pvdnet_syn_login:
			pvd->status = pvdnet_ack_login;
			result = PVDDoOutput(pvd);
			break;

		case pvdnet_ack_login:
			SlicePush(&pvd->outbuf, pvd->outmsg.size);
			result = PVDTryOutput(pvd);
			if (result < 0)
				break;
			if (result == 0) {
				result = PVDDoOutput(pvd);
				break;
			}
			phead = (pvdnet_head*)pvd->outmsg.data;
			if ((phead->uCmd != NET_SDVR_LOGIN) || (phead->uResult == 0)) {
				result = -EFAULT;
				break;
			}

			result -= sizeof(pvdnet_head);
			memcpy(&pvd->device2, phead+1, min(sizeof(pvd->device2), result));
			pvd->userid = phead->uUserId;

			if (result == sizeof(STRUCT_SDVR_DEVICE_EX))
				pvd->status = pvdnet_con_devinfo2;
			else if (result == sizeof(STRUCT_SDVR_DEVICE))
				pvd->status = pvdnet_con_devinfo;
			else
				pvd->status = pvdnet_con_devinfox;
			return result;

		case pvdnet_closing:
			result = PVDDoClose(pvd, pvdnet_disconnected);
			if (result == 0)
				break;

		case pvdnet_disconnected:
			pvd->status = pvdnet_invalid;
			return -EFAULT;

		default:
			assert(FALSE);
			return -EACCES;
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

	long result = PVDDoOpen(pvd, pvdnet_connecting);
	if (result != 0)
		result = PVDOpenStatus(pvd, result);
	return result;
}

static long PVDGetOption(AObject *object, AOption *option)
{
	PVDClient *pvd = to_pvd(object);
	if (_stricmp(option->name, "version") == 0) {
		_itoa_s(pvd->device2.byDVRType, option->value, 10);
		return 1;
	}
	if (_stricmp(option->name, "session_id") == 0) {
		_itoa_s(pvd->userid, option->value, 10);
		return 1;
	}
	return -ENOSYS;
}

static long PVDOutputStatus(PVDClient *pvd)
{
	long result;
	do {
		SlicePush(&pvd->outbuf, pvd->outmsg.size);
		result = PVDTryOutput(pvd);
		if (result < 0) {
			break;
		}
		if (result > 0) {
			pvdnet_head *phead = (pvdnet_head*)pvd->outmsg.data;
			AMsgCopy(pvd->outfrom, AMsgType_Custom|phead->uCmd, pvd->outmsg.data, pvd->outmsg.size);
			break;
		}
		result = PVDDoOutput(pvd);
	} while (result > 0);
	return result;
}

static long PVDOutputDone(AMessage *msg, long result)
{
	PVDClient *pvd = from_outmsg(msg);
	if (result >= 0)
		result = PVDOutputStatus(pvd);
	if (result != 0)
		result = pvd->outfrom->done(pvd->outfrom, result);
	return result;
}

static long PVDRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDClient *pvd = to_pvd(object);
	if (reqix != ARequest_Output)
	{
		if ((reqix == ARequest_Input) && (msg->type & AMsgType_Custom)) {
			pvdnet_head *phead = (pvdnet_head*)msg->data;
			phead->uUserId = pvd->userid;
		}
		return pvd->io->request(pvd->io, reqix, msg);
	}

	pvd->outmsg.done = &PVDOutputDone;
	pvd->outfrom = msg;

	pvd->outmsg.data = NULL;
	pvd->outmsg.size = 0;
	return PVDOutputStatus(pvd);
}

static long PVDCloseStatus(PVDClient *pvd, long result)
{
	pvdnet_head *phead;
	do {
		if ((result < 0) && (pvd->status != pvdnet_disconnected))
			pvd->status = pvdnet_closing;
		switch (pvd->status)
		{
		case pvdnet_syn_logout:
			pvd->status = pvdnet_ack_logout;
			AMsgInit(&pvd->outmsg, AMsgType_Unknown, NULL, 0);

		case pvdnet_ack_logout:
			SlicePush(&pvd->outbuf, pvd->outmsg.size);
			result = PVDTryOutput(pvd);
			if (result < 0)
				break;
			if (result == 0) {
				result = PVDDoOutput(pvd);
				break;
			}
			phead = (pvdnet_head*)pvd->outmsg.data;
			if (phead->uCmd != NET_SDVR_LOGOUT) {
				pvd->outmsg.data = NULL;
				pvd->outmsg.size = 0;
				break;
			}
			if ((pvd->outfrom->type == AMsgType_Unknown) || (pvd->outfrom->type & AMsgType_Custom)) {
				AMsgCopy(pvd->outfrom, AMsgType_Custom|phead->uCmd, pvd->outmsg.data, pvd->outmsg.size);
			}
		case pvdnet_closing:
			result = PVDDoClose(pvd, pvdnet_disconnected);
			if (result == 0)
				break;

		case pvdnet_disconnected:
			pvd->status = pvdnet_invalid;
			return (pvd->outfrom->type&AMsgType_Custom) ? result : -EFAULT;

		default:
			assert(FALSE);
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
		result = pvd->outfrom->done(pvd->outfrom, result);
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
		SliceResize(&pvd->outbuf, 1024);
		PVDDoInput(pvd, pvdnet_syn_logout, NET_SDVR_LOGOUT, 0);
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
	&PVDCreate,
	&PVDRelease,
	2,

	&PVDOpen,
	NULL,
	&PVDGetOption,
	&PVDRequest,
	NULL,
	&PVDClose,
};
