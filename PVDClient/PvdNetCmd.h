#ifndef _PVDNETCMD_H_
#define _PVDNETCMD_H_


#include "NetCmdDef.h"
#include "NetCmdStruct.h"
#include "NetCmdIntelligentStruct.h"
#include "mshead.h"
#include "HBStreamHeaderV30.h"


#define NET_CMD_HEAD_FLAG    ('S'|('D'<<8)|('V'<<16)|('R'<<24))

typedef struct ST_NET_CMD_HEAD {
	uint32_t  uFlag;    //固定'SDVR'
	uint32_t  uUserId;  //用户ID
	uint16_t  uCmd;     //网络命令
	uint16_t  uLen;     //数据长度
	uint16_t  uResult;  //返回结果(0-失败,1-成功)
	uint16_t  uReserve; //保留
	//unsigned char  data[0];  //数据
} pvdnet_head;

inline int PVDCmdEncode(uint32_t userid, void *ptr, int type, int body)
{
	pvdnet_head *phead = (pvdnet_head*)ptr;
	phead->uFlag    = NET_CMD_HEAD_FLAG;
	phead->uUserId  = userid;
	phead->uCmd     = (unsigned short)type;
	phead->uLen     = (unsigned short)body;
	phead->uResult  = 0;
	phead->uReserve = (unsigned short)(body>>16);
	return sizeof(pvdnet_head)+body;
}

inline int PVDCmdDecode(uint32_t user_id, void *data, int size)
{
	if (size < sizeof(pvdnet_head)) {
		return 0;
	}

	pvdnet_head *phead = (pvdnet_head*)data;
	int cmdlen = sizeof(pvdnet_head) + phead->uLen;
	if (phead->uResult
	 && (phead->uCmd == NET_SDVR_GET_PHOTO
	  || phead->uCmd == NET_SDVR_ALARM_REPORT
	  || phead->uCmd == NET_SDVR_GET_PHOTO_EX)) {
		cmdlen += long(phead->uReserve)<<16;
	}

	if (phead->uFlag != NET_CMD_HEAD_FLAG) {
		return -EINVAL;
	}

	if ((user_id != 0) && (user_id != phead->uUserId)) {
		//return -ERROR_INVALID_OWNER;
		TRACE("user_id(%d): error: command(%02x) userid = %d.\n",
			user_id, phead->uCmd, phead->uUserId);
	}
	return cmdlen;
}

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

struct HBNetCmdComponent : public AComponent {
	static const char* name() { return "PvdNetCmdComponent"; }

	uint32_t    _userid;
	STRUCT_SDVR_DEVICE_EX _device2;

	void  (*on_msg)(HBNetCmdComponent *c, pvdnet_head *phead, char *pbody);
	void   *on_msg_userdata;
};


#endif
