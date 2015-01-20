#ifndef __NETCMDSTRUCT_H__
#define __NETCMDSTRUCT_H__

#define NAME_LEN		32					//用户名长度
#define PASSWD_LEN		16					//密码
#define SERIALNO_LEN	48					//序列号
#define MAX_CHANNUM		16					//通道数
#define MAX_CHANNUM_EX	128					//通道数(扩展)
#define MAX_DISKNUM		16					//硬盘数
#define MAX_ETHERNET	2
#define PATHNAME_LEN	128
#define MAX_DAYS		8
#define MAX_TIMESEGMENT	2
#define PRESETNUM		16					//预置点
#define MAX_SHELTERNUM	4					//时段
#define MACADDR_LEN		6
#define	INFO_LEN		32
#define	INFO_SEQ		4
#define MAX_SMTP_HOST   128
#define MAX_SMTP_ADDR   256
#define MAX_STRING      32
#define PTZ_SIZE		256
#define MAX_PRESET		128
#define MAX_RIGHT		32
#define MAX_USERNUM		16
#define PHONENUMBER_LEN	32
#define MAX_LINK		32
#define MAX_ALARMIN_EX	128
#define MAX_ALARMOUT_EX	128
#define MAX_REC_NUM		100					//历史视频日志数
#define MAXPTZNUM		100					//云台协议
#define MAX_PRESET		128
#define MAX_LOGO_NUM	100

#define PROTOCOL_V3     32					//3.0协议
#define MAX_DEV_NUM		32
#define MAX_FORMAT_DEV	16
#define MAX_PAT         15 

#define		ALARM_MD						0	//移动侦测报警
#define		ALARM_IN							1	//探头报警
#define		ALARM_VIDEOLOST			2	//视频丢失报警
#define		ALARM_DISKERR				3	//磁盘异常报警
#define		ALARM_NET_DISCONN		4	//网络断开报警
#define		ALARM_TEMP_HIGH			5	//温度过高
#define		ALARM_VIDEOCOVER			6	//视频遮挡报警
#define		ALARM_REC_NOTENOUGH	7	//录像天数不足报警

#define CMDNO(cmd, id)  (((id) << 16) | (cmd))
#define COMP_TEST               (911)            //自动化测试模块
#define MAX_CH              64


#define MAX_HUMTMP_DEV		20   //温湿度采集器最大个数  //网新药监

//网络结构
#pragma pack(push, 1)
#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif

//时间格式
typedef struct
{
	BYTE byEnable;							//激活  0-屏蔽 1-激活	
    BYTE byStartHour;
	BYTE byStartMin;
	BYTE byStopHour;
	BYTE byStopMin;
}STRUCT_SDVR_SCHEDTIME, *LPSTRUCT_SDVR_SCHEDTIME;

//登录设备
typedef struct
{
	char	szUserName[NAME_LEN];			//用户名
	char	szPassWord[PASSWD_LEN];			//密码
	DWORD	dwNamelen;						//有效的用户名长度
	DWORD	dwPWlen;						//有效的密码长度
}STRUCT_SDVR_LOGUSER, *LPSTRUCT_SDVR_LOGUSER;

typedef enum
{
	PT_DEVTYPE_7000T = 0, 
	PT_DEVTYPE_8000T,
	PT_DEVTYPE_8200T,
	PT_DEVTYPE_8000ATM,
	PT_DEVTYPE_8600T,//8600T
	PT_DEVTYPE_6200T,
	PT_DEVTYPE_8004AH,
	PT_DEVTYPE_8004AI,
	//PT_DEVTYPE_7000H,
	PT_DEVTYPE_7200X,
	PT_DEVTYPE_7200H,
	PT_DEVTYPE_7200L,
	PT_DEVTYPE_7000M = 12,
	PT_DEVTYPE_8000M,
	PT_DEVTYPE_8200M,
	PT_DEVTYPE_7000L,
	PT_DEVTYPE_2201TL = 16,
	PT_DEVTYPE_2600T,
	NET_DEVTYPE_2600TB,    //人流统计智能分析盒 
	NET_DEVTYPE_2600TC,    //车牌识别智能分析盒 
	NET_DEVTYPE_9300, 
	NET_DEVTYPE_9400, 

	HB9824N16H = 1000, 
	HB9832N16H, 
	HB9904, 
	HB9908, 
	HB9912, 
	HB9916, 
	HB9932, 
	HB7904, 
	HB7908, 
	HB7912, 
	HB7916, 
}NET_DEV_TYPE_E;

typedef enum
{
	DL_DEVTYPE_HB9304 = 170, /*机型:可唯一标识一种具体机型*/
	DL_DEVTYPE_HB9308,
	DL_DEVTYPE_HB9404,
	DL_DEVTYPE_HB9408,
	DL_DEVTYPE_HB7216XT3,
	DL_DEVTYPE_HB7016X3LH,  //HB7016X3LH
	DL_DEVTYPE_HB7208XT3,	
	DL_DEVTYPE_HB7216LL3,   //实际是DL_DEVTYPE_HB7116X3
	DL_DEVTYPE_HB7208LL3,   //实际是DL_DEVTYPE_HB7108X3
	DL_DEVTYPE_HB7008X3,
	DL_DEVTYPE_HB7008X3L,	
	DL_DEVTYPE_HB7016X3LC,  //HB7016X3LC
	DL_DEVTYPE_HB7116X3LC,  //HB7116X3LC 
	DL_DEVTYPE_HB7104X3LC, //3520D add
	DL_DEVTYPE_HB7108X3LC,
	DL_DEVTYPE_HB7104X3LH,
	DL_DEVTYPE_HB7108X3LH,
	DL_DEVTYPE_HB7204X3,
	DL_DEVTYPE_HB7116X3LH, 
	DL_DEVTYPE_HB8004AXL,
	DL_DEVTYPE_HB8004AX,
	DL_DEVTYPE_HB7016X3LE,
	DL_DEVTYPE_HB9012X3 = 200,	
	DL_DEVTYPE_HB9020X3,	
	DL_DEVTYPE_HB9212X3,
	DL_DEVTYPE_HB9220X3,	
	DL_DEVTYPE_HB9604X3,	
	DL_DEVTYPE_HB9608X3,
	DL_DEVTYPE_HB9212X3RD,
	DL_DEVTYPE_HB9220X3RD,
	DL_DEVTYPE_HB9016X3,	
	DL_DEVTYPE_HB9216X3RD,
	DL_DEVTYPE_HB9216X3,
	DL_DEVTYPE_HB8208X3 = 230,
	DL_DEVTYPE_HB8216X3,    
	DL_DEVTYPE_HB8608X3,  
	DL_DEVTYPE_HB8616X3,    
	DL_DEVTYPE_HB8808X3,
	DL_DEVTYPE_HB8816X3,    
	DL_DEVTYPE_HB8808X3RD,
	DL_DEVTYPE_HB8816X3RD,
	DL_DEVTYPE_HB8604X3,
	DL_DEVTYPE_HB7304X3L = 260,
	DL_DEVTYPE_HB7308X3L,
	DL_DEVTYPE_HB7316X3L,
	DL_DEVTYPE_HB7304X3,
	DL_DEVTYPE_HB7308X3,
	DL_DEVTYPE_HB7316X3,
	DL_DEVTYPE_HB7004X3LA,
	DL_DEVTYPE_HB7008X3LA,
	DL_DEVTYPE_HB7016X3LA,
	DL_DEVTYPE_HB7232X3,
	DL_DEVTYPE_HB7208X5,
	DL_DEVTYPE_HB7216X5,
	DL_DEVTYPE_HB7104X3LD,
	DL_DEVTYPE_HB7108X3LD,
	DL_DEVTYPE_HB7016X3LD,
	DL_DEVTYPE_HB7004X3HA,
	DL_DEVTYPE_HB7008X3HA,
	DL_DEVTYPE_HB7016X3HA,
	DL_DEVTYPE_HB7216X3H4,
	DL_DEVTYPE_HB7004X3HD,
	DL_DEVTYPE_HB7004X3HC,
	DL_DEVTYPE_HB7008X3HC,
	DL_DEVTYPE_HB7016X3HC,
	DL_DEVTPYE_HB_DVR5208 = 290,		//7308X3-L的机型改名成HB-DVR5208
	DL_DEVTYPE_HB_DVR5104,
	DL_DEVTYPE_HB_DVR5104S,
	DL_DEVTYPE_HB_DVR5108,
	DL_DEVTYPE_HB_DVR5204,
	DL_DEVTYPE_HB_DVR5204S,    
	DL_DEVTYPE_HB_DVR5208S,
	DL_DEVTYPE_HB_DVR5216,
	DL_DEVTYPE_HB_DVR5104HA,
	DL_DEVTYPE_HB_DVR5208HA,
	DL_DEVTYPE_HB_DVR5208A,
	DL_DEVTYPE_HB_DVR5216A,
	DL_DEVTYPE_HB_DVR5104A,
	DL_DEVTYPE_HB_DVR5104BH,
	DL_DEVTYPE_HB_DVR5104BC,
	DL_DEVTYPE_HB_DVR5108BH,
	DL_DEVTYPE_HB_DVR5108BC,
	DL_DEVTYPE_HB_DVR5116BH,
	DL_DEVTYPE_HB_DVR5116BC,
	DL_DEVTYPE_HB_NVR5804 = 320,
	DL_DEVTYPE_HB_NVR5808,
	DL_DEVTYPE_HB_NVR5816,
	DL_DEVTYPE_HB_NVR5832,
	DL_DEVTYPE_HB_NVR2208E_P = 350,
	DL_DEVTYPE_HB_NVR2216E_P,
	DL_DEVTYPE_HB_NVR2232E_P,
	DL_DEVTYPE_HB_NVR2416E_P,
	DL_DEVTYPE_HB_NVR2432E_P,
	DL_DEVTYPE_HB_NVR2208E,
	DL_DEVTYPE_HB_NVR2216E,
	DL_DEVTYPE_HB_NVR2232E,
	DL_DEVTYPE_HB_NVR2416E,
	DL_DEVTYPE_HB_NVR2432E,    
	DL_DEVTYPE_BUTT
} DL_DEVTYPE_E;

//设备主动连接
typedef struct
{
	char sDVRID[48]; //平台分配给DVR的登录ID
	char sSerialNumber[48]; //序列号：主机端必须填充，从前往后处理，其余补零
	//DVR设备本身的序列号
	BYTE byAlarmInPortNum; //DVR报警输入个数
	BYTE byAlarmOutPortNum; //DVR报警输出个数
	BYTE byDiskNum; //DVR 硬盘个数
	BYTE byProtocol; //新类型产品该值定为0x20，按协议二处理
	BYTE byChanNum; //DVR 通道个数
	BYTE byEncodeType; //主机编码格式：1为ANSI字符串，中文采用GB2312编码；2为UTF8
	BYTE reserve[26]; //保留
	char sDvrName[32]; //主机名
	char sChanName[128][32]; //通道名称
}STRUCT_SDVR_INITIATIVE_LOGIN;

//设备类型
typedef struct
{
	DWORD  dvrtype;							//7004 8004 2201 2004
	//DWORD  nreserve1;						//保留
	WORD   device_type;					    //设备类型		枚举NET_DEV_TYPE_E
	WORD   memory_size;					    //内存大小		枚举NET_MEMSIZE_E
	DWORD  nreserve2;						//保留
 }STRUCT_SDVR_INFO, *LPSTRUCT_SDVR_INFO;

//设备信息
typedef struct
{
	char szSerialNumber[SERIALNO_LEN];		//保留
	BYTE byAlarmInPortNum;					//DVR报警输入个数
	BYTE byAlarmOutPortNum;					//DVR报警输出个数
	BYTE byDiskNum;							//DVR硬盘个数
	BYTE byDVRType;							//DVR类型, 1:DVR 2:ATM DVR 3:DVS （建议使用NET_SDVR_GET_DVRTYPE命令）
	BYTE byChanNum;							//DVR通道个数
	BYTE byStartChan;						//保留
	char szDvrName[NAME_LEN];				//主机名
	char szChanName[MAX_CHANNUM][NAME_LEN];	//通道名称
}STRUCT_SDVR_DEVICE, *LPSTRUCT_SDVR_DEVICE;

//设备信息(扩展)
typedef struct
{
	char sSerialNumber[SERIALNO_LEN];		//保留
	BYTE byAlarmInPortNum;					//DVR报警输入个数
	BYTE byAlarmOutPortNum;					//DVR报警输出个数
	BYTE byDiskNum;							//DVR硬盘个数
	BYTE byDVRType;							//DVR类型, 1:DVR 2:ATM DVR 3:DVS （建议使用NET_SDVR_GET_DVRTYPE命令）
	BYTE byChanNum;							//DVR通道个数
	BYTE byStartChan;						//保留
	char szDvrName[NAME_LEN];				//主机名
	char szChanName[MAX_CHANNUM_EX][NAME_LEN];//通道名称
}STRUCT_SDVR_DEVICE_EX, *LPSTRUCT_SDVR_DEVICE_EX;

//硬件信息
typedef struct
{
	DWORD dwSize;
	BYTE sDVRName[NAME_LEN];				//DVR名称
	DWORD dwDVRID;							//保留
	DWORD dwRecycleRecord;					//保留
	BYTE sSerialNumber[SERIALNO_LEN];		//序列号
	BYTE sSoftwareVersion[16];				//软件版本号
	BYTE sSoftwareBuildDate[16];			//软件生成日期
	DWORD dwDSPSoftwareVersion;				//DSP软件版本
	BYTE sPanelVersion[16];					//前面板版本
	BYTE sHardwareVersion[16];				//保留
	BYTE byAlarmInPortNum;					//DVR报警输入个数
	BYTE byAlarmOutPortNum;					//DVR报警输出个数
	BYTE byRS232Num;						//保留
	BYTE byRS485Num;						//保留
	BYTE byNetworkPortNum;					//保留
	BYTE byDiskCtrlNum;						//保留
	BYTE byDiskNum;							//DVR 硬盘个数
	BYTE byDVRType;							//DVR类型, 1:DVR 2:ATM DVR 3:DVS （建议使用NET_SDVR_GET_DVRTYPE命令）
	BYTE byChanNum;							//DVR 通道个数
	BYTE byStartChan;						//保留
	BYTE byDecordChans;						//主机默认可接入的IP设备路数。混合DVR每一个机型都有一个默认可接入的IP设备路数，如8路机默认可接入4路IPC，16路机默认可接入8路IPC。
	//（默认路数不是最大路数，8路机最大可接入4+8=12个IPC,16路机最大可接入8+16=24个IPC）。获取时有效，设置时该值无效。
	BYTE byVGANum;							//保留
	BYTE byUSBNum;							//保留
 }STRUCT_SDVR_DEVICEINFO, *LPSTRUCT_SDVR_DEVICEINFO;

//报警信息
typedef struct
{
	WORD wAlarm;							//探头报警  按位：0-第一通道  1-第2通道 ...
	WORD wVlost;							//视频丢失  ...
	WORD wMotion;							//移动报警	...
	WORD wHide;								//保留		视频遮挡报警20110908...	
	BYTE byDisk[MAX_DISKNUM];				//保留		...
}STRUCT_SDVR_ALARM, *LPSTRUCT_SDVR_ALARM;

//报警信息(老8000)
typedef struct
{
	WORD wAlarm;							//探头报警  按位：0-第一通道  1-第2通道 ...
	WORD wVlost;							//视频丢失  ...
	WORD wMotion;							//移动报警	...
	WORD wHide;								//保留		...	
}STRUCT_SDVR_ALARM_OLD, *LPSTRUCT_SDVR_ALARM_OLD;

//报警信息(扩展)
typedef struct
{
	BYTE wAlarm[MAX_CHANNUM_EX];			//探头报警  0-无报警  1-有报警
	BYTE wVlost[MAX_CHANNUM_EX];			//视频丢失  ...
	BYTE wMotion[MAX_CHANNUM_EX];			//移动报警	...
	BYTE wHide[MAX_CHANNUM_EX];				//保留		视频遮挡报警20110908...	
	BYTE byDisk[MAX_DISKNUM];				//保留		...
}STRUCT_SDVR_ALARM_EX, *LPSTRUCT_SDVR_ALARM_EX;

//报警处理
typedef struct
{
	DWORD dwHandleType;				//按位(2-声音报警,5-监视器最大化)
	WORD wAlarmOut;					//报警输出触发通道 按位对应通道
}STRUCT_SDVR_HANDLEEXCEPTION, *LPSTRUCT_SDVR_HANDLEEXCEPTION;

//报警处理(扩展)
typedef struct
{
	DWORD dwHandleType;				//按位(2-声音报警,5-监视器最大化)
	BYTE szAlarmOut[MAX_CHANNUM_EX];//报警输出触发通道按位对应通道
}STRUCT_SDVR_HANDLEEXCEPTION_EX, *LPSTRUCT_SDVR_HANDLEEXCEPTION_EX;

//实时视频
typedef struct
{
	BYTE byChannel;						//通道号
	BYTE byLinkMode;					//0-主码流 1-子码流  
	BYTE byMultiCast;					//保留
	char szMultiCastIP[16];				//保留
	WORD wPort;							//保留 
}STRUCT_SDVR_REALPLAY, *LPSTRUCT_SDVR_REALPLAY;

//实时视频(扩展)
typedef struct	
{
	BYTE	byChannel;					//通道号
	BYTE	byLinkMode;					//0-主码流 1-子码流  
	BYTE	byMultiCast;				//是否多播（当主机设置了多播模式，使用UDP方式开视频的时候会自动选择多播）
	DWORD	sMultiCastIP;				//多播IP地址(大字节序)
	BYTE	OSDCharEncodingScheme;		//OSD字符的编码格式
	BYTE 	reserve[11];				//保留
	WORD 	wPort;						//多播端口(小字节序)
}STRUCT_SDVR_REALPLAY_EX, *LPSTRUCT_SDVR_REALPLAY_EX;

//主动连接获取实时视频
typedef struct
{
	DWORD msgid; //消息ID,该id 由平台生成，DVR 原封不动返回,msgid 用于平台区分主机新建立的socket 连接。使平台可以知道该socket,连接对应的命令。与byChannel 无关。
	BYTE byChannel; //通道号
	BYTE byLinkMode; // 0-主码流1-子码流
	BYTE byMultiCast; //是否多播(当主机设置了多播模式，使用UDP 方式开视频的时候会自动选择多播)
	BYTE OSDCharEncodingScheme; // OSD 字符的编码格式
	DWORD sMultiCastIP; //多播IP 地址(大字节序)
	BYTE reserve1[10]; //保留
	WORD wPort;//多播端口(小字节序)
	BYTE reserve2[16]; //保留
}STRUCT_SDVR_REALPLAY_INITIATIVE,*LPSTRUCT_SDVR_REALPLAY_INITIATIVE;

//关闭实时视频
typedef struct
{
	BYTE byChannel;						//通道号
	BYTE byLinkMode;					//位：0-主码流 1-子码流  
 	WORD uPort;							//本地端口
}STRUCT_SDVR_REALPLAY_STOP, *LPSTRUCT_SDVR_REALPLAY_STOP;

//视频参数
typedef struct
{
	DWORD	dwBrightValue;				//亮度(1-127)
	DWORD	dwContrastValue;			//对比度(1-127)
	DWORD	dwSaturationValue;			//饱和度(1-127)
	DWORD	dwHueValue;					//色度(1-127)
}STRUCT_SDVR_VIDEOPARAM, *LPSTRUCT_SDVR_VIDEOPARAM;

typedef struct
{	
	WORD wStartTime;					//高8位表示小时 低8位表示分钟
	WORD wEndTime;						//高8位表示小时 低8位表示分钟
	STRUCT_SDVR_VIDEOPARAM VideoParam;
}STRUCT_SDVR_SCHEDULE_VIDEOPARAM, *LPSTRUCT_SDVR_SCHEDULE_VIDEOPARAM;

typedef struct
{
	BYTE	byChannel;					//通道号
	STRUCT_SDVR_SCHEDULE_VIDEOPARAM Schedule_VideoParam[2];	//一天包含2个时间段
	STRUCT_SDVR_VIDEOPARAM Default_VideoParam;				//不在时间段内就使用默认
}STRUCT_SDVR_VIDEOEFFECT,*LPSTRUCT_SDVR_VIDEOEFFECT;

//云台控制
typedef struct
{
	BYTE	byPort;						//云台端口号
	DWORD	dwPTZCommand;				//云台控制命令
	DWORD   dwStop;						//是否停止
	DWORD	dwIndex;					//预制点号
	DWORD	dwSpeed;
}STRUCT_SDVR_PTZ_CTRL, *LPSTRUCT_SDVR_PTZ_CTRL;

typedef struct
{
	BYTE byPort;						//云台端口号
	DWORD dwSize;						//云台控制码有效长度
	BYTE szCtrlBuf[PTZ_SIZE];			//云台控制码数据
}STRUCT_SDVR_PTZ_TRANS;

//网络键盘
typedef struct
{
	DWORD dwKey;
	DWORD dwKeyCode;
}STRUCT_SDVR_NETKEY, *LPSTRUCT_SDVR_NETKEY;

//设备网络信息
typedef struct
{
	DWORD dwDVRIP;						//DVR IP地址
	DWORD dwDVRIPMask;					//DVR IP地址掩码
	DWORD dwNetInterface;				//网络接口 1-10MBase-T 2-10MBase-T全双工 3-100MBase-TX 4-100M全双工 5-10M/100M自适应
	WORD wDVRPort;						//端口号
	BYTE byMACAddr[MACADDR_LEN];		//服务器的物理地址
}STRUCT_SDVR_ETHERNET, *LPSTRUCT_SDVR_ETHERNET;
 
typedef struct
{
	DWORD dwSize;
	STRUCT_SDVR_ETHERNET struEtherNet[MAX_ETHERNET]; /* 以太网口 */
	DWORD dwManageHostIP;				//远程管理主机地址		 
	WORD wManageHostPort;				//保存
	DWORD dwDNSIP;						//DNS服务器地址  
	DWORD dwMultiCastIP;				//多播组地址
	DWORD dwGatewayIP;       			//网关地址 
	DWORD dwNFSIP;						//保存
	BYTE szNFSDirectory[PATHNAME_LEN];	//保存
	DWORD dwPPPOE;						//0-不启用,1-启用
	BYTE szPPPoEUser[NAME_LEN];			//PPPoE用户名
	char szPPPoEPassword[PASSWD_LEN];	//PPPoE密码
	DWORD dwPPPoEIP;					//PPPoE IP地址(只读)
	WORD wHttpPort;						//HTTP端口号
}STRUCT_SDVR_NETINFO, *LPSTRUCT_SDVR_NETINFO;

//移动侦测
typedef struct
{
	BYTE byMotionScope[18][22];			//侦测区域,共有22*18个小宏块,为1表示该宏块是移动侦测区域,0-表示不是 
	BYTE byMotionSensitive;				//移动侦测灵敏度, 0 - 5,越高越灵敏,0xff关闭 
	BYTE byEnableHandleMotion;			//是否处理移动侦测
	STRUCT_SDVR_HANDLEEXCEPTION struMotionHandleType;	//处理方式 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
	DWORD dwRelRecord;					//保留
}STRUCT_SDVR_MOTION, *LPSTRUCT_SDVR_MOTION;

//移动侦测(扩展)
typedef struct
{
	BYTE byMotionScope[18][22];			//侦测区域,共有22*18个小宏块,为1表示该宏块是移动侦测区域,0-表示不是 
	BYTE byMotionSensitive;				//移动侦测灵敏度, 0 - 5,越高越灵敏,0xFF关闭 
	BYTE byEnableHandleMotion;			//是否处理移动侦测 
	STRUCT_SDVR_HANDLEEXCEPTION_EX struMotionHandleType;			//处理方式 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//布防时间
	BYTE dwRelRecord[MAX_CHANNUM_EX];	//保留
}STRUCT_SDVR_MOTION_EX, *LPSTRUCT_SDVR_MOTION_EX;

//遮挡报警区域为704*576
typedef struct
{
	DWORD dwEnableHideAlarm;										//保留
	WORD wHideAlarmAreaTopLeftX;									//保留
	WORD wHideAlarmAreaTopLeftY;									//保留
	WORD wHideAlarmAreaWidth;										//保留
	WORD wHideAlarmAreaHeight;										//保留
	STRUCT_SDVR_HANDLEEXCEPTION struHideAlarmHandleType;			//保留
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//保留
}STRUCT_SDVR_HIDEALARM, *LPSTRUCT_SDVR_HIDEALARM;

//遮挡报警区域为704*576(扩展)
typedef struct
{
	DWORD dwEnableHideAlarm;										//保留
	WORD wHideAlarmAreaTopLeftX;									//保留
	WORD wHideAlarmAreaTopLeftY;									//保留
	WORD wHideAlarmAreaWidth;										//保留
	WORD wHideAlarmAreaHeight;										//保留
	STRUCT_SDVR_HANDLEEXCEPTION_EX stHideAlarmHandleType;			//保留
	STRUCT_SDVR_SCHEDTIME stAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//保留
}STRUCT_SDVR_HIDEALARM_EX, *LPSTRUCT_SDVR_HIDEALARM_EX;

//信号丢失报警
typedef struct
{
	BYTE byEnableHandleVILost;										//是否处理信号丢失报警 
	STRUCT_SDVR_HANDLEEXCEPTION stVILostHandleType;					//处理方式 	
	STRUCT_SDVR_SCHEDTIME stAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//布防时间
}STRUCT_SDVR_VILOST;

//信号丢失报警
typedef struct
{
	BYTE byEnableHandleVILost;										//是否处理信号丢失报警 
	STRUCT_SDVR_HANDLEEXCEPTION_EX strVILostHandleType;				//处理方式 	
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//布防时间
}STRUCT_SDVR_VILOST_EX;

//遮盖区域
typedef struct
{
	WORD wHideAreaTopLeftX;						//遮盖区域的x坐标  
	WORD wHideAreaTopLeftY;						//遮盖区域的y坐标  
	WORD wHideAreaWidth;						//遮盖区域的宽 
	WORD wHideAreaHeight;						//遮盖区域的高 
}STRUCT_SDVR_SHELTER, *LPSTRUCT_SDVR_SHELTER;

//图像参数
typedef struct
{
	BYTE byChannel;
	DWORD dwSize;
	BYTE sChanName[NAME_LEN];						//通道名
	DWORD dwVideoFormat;							//保留 
	BYTE byBrightness;  							//保留
	BYTE byContrast;    							//保留	
	BYTE bySaturation;  							//保留
	BYTE byHue;    									//保留 	
	DWORD dwShowChanName;							//保留
	WORD wShowNameTopLeftX;							//通道名称显示位置的x坐标 
	WORD wShowNameTopLeftY;							//通道名称显示位置的y坐标  
	STRUCT_SDVR_VILOST struVILost;					//信号丢失报警
	STRUCT_SDVR_MOTION struMotion;					//移动侦测
	STRUCT_SDVR_HIDEALARM struHideAlarm;			//保留
	DWORD dwEnableHide;								//是否启动遮盖(0-禁用,1-实时屏蔽,2-回放屏蔽,3-全屏蔽)
	STRUCT_SDVR_SHELTER	struShelter[MAX_SHELTERNUM];//保留
	DWORD dwShowOsd;								//保留
	WORD wOSDTopLeftX;								//保留
	WORD wOSDTopLeftY;								//保留
	BYTE byOSDType;									//保留
	BYTE byDispWeek;								//保留	
	BYTE byOSDAttrib;								//通道名 1-不透明 2-透明
}STRUCT_SDVR_PICINFO, *LPSTRUCT_SDVR_PICINFO;

//图像参数(扩展)
typedef struct
{
	BYTE byChannel;
	DWORD dwSize;
	BYTE sChanName[NAME_LEN];						//通道名
	DWORD dwVideoFormat;							//保留 
	BYTE byBrightness;  							//保留
	BYTE byContrast;    							//保留	
	BYTE bySaturation;  							//保留
	BYTE byHue;										//保留 	
	DWORD dwShowChanName;							//保留
	WORD wShowNameTopLeftX;							//通道名称显示位置的x坐标 
	WORD wShowNameTopLeftY;							//通道名称显示位置的y坐标  
	STRUCT_SDVR_VILOST_EX struVILost;				//信号丢失报警
	STRUCT_SDVR_MOTION_EX struMotion;				//移动侦测
	STRUCT_SDVR_HIDEALARM_EX struHideAlarm;			//保留
	DWORD dwEnableHide;								//是否启动遮盖 ,0-否,1-是  
	STRUCT_SDVR_SHELTER	stShelter[MAX_SHELTERNUM];	//保留
	DWORD dwShowOsd;								//保留
	WORD wOSDTopLeftX;								//保留
	WORD wOSDTopLeftY;								//保留
	BYTE byOSDType;									//保留
	BYTE byDispWeek;								//保留	
	BYTE byOSDAttrib;								//通道名 1-不透明 2-透明
}STRUCT_SDVR_PICINFO_EX, *LPSTRUCT_SDVR_PICINFO_EX;


//编码压缩参数
typedef struct
{
	BYTE byStreamType;								//码流类型	0-无音频 ,1-有音频
	BYTE byResolution;								//分辨率	0-CIF 1-HD1, 2-D1, 3-QCIF
	BYTE byBitrateType;								//码率类型	0:变码率 1:定码率 2:定画质
	BYTE byPicQuality;								//图象质量	1-最好 2-次好 3-较好 4-一般 5-较差 6-差	 改成：最高，较高，高，中，低，最低
	DWORD dwVideoBitrate;							//视频码率 0-100K，1-128K，2-256K，3-512K，4-1M，5-2M，6-3M，7-4M
	DWORD dwVideoFrameRate;							//帧率		2 至 25
}STRUCT_SDVR_COMPRESSION, *LPSTRUCT_SDVR_COMPRESSION;

typedef struct
{
	BYTE byChannel;									//通道号   //BYTE
	DWORD dwSize;									//结构体大小
	BYTE byRecordType;								//0x0:手动录像，0x1:定时录象，0x2:移动侦测，0x3:报警，0x0f:所有类型
	STRUCT_SDVR_COMPRESSION stRecordPara;			//录像流（主码流）
	STRUCT_SDVR_COMPRESSION stNetPara;				//网传流（子码流）
}STRUCT_SDVR_COMPRESSINFO, *LPSTRUCT_SDVR_COMPRESSINFO;

//录像参数
typedef struct
{
	STRUCT_SDVR_SCHEDTIME stRecordTime;				//录像时段
	BYTE byRecordType;								//保留	
	char reservedData[3];
}STRUCT_SDVR_RECORDSCHED, *LPSTRUCT_SDVR_RECORDSCHED;

typedef struct
{
	WORD wAllDayRecord;								//保留	
	BYTE byRecordType;								//保留	
	char reservedData;
}STRUCT_SDVR_RECORDDAY, *LPSTRUCT_SDVR_RECORDDAY;

typedef struct
{
	BYTE byChannel;									//通道号
	DWORD dwSize;									//结构大小
	DWORD dwRecord;									//是否录像 0-否 1-是 
	STRUCT_SDVR_RECORDDAY stRecAllDay[MAX_DAYS];	//星期
	STRUCT_SDVR_RECORDSCHED stRecordSched[MAX_DAYS][MAX_TIMESEGMENT];//时间段
	DWORD dwPreRecordTime;							//保留	
}STRUCT_SDVR_RECORDINFO, *LPSTRUCT_SDVR_RECORDINFO;

//云台协议
typedef struct
{
	int nPTZNum;
	char szPTZProtocol[MAXPTZNUM][10];
}STRUCT_SDVR_PTZTYPE, *LPSTRUCT_SDVR_PTZTYPE;

//解码器参数(云台参数)
typedef struct	{
	BYTE byChannel;
	DWORD dwSize;
	DWORD dwBaudRate;								//波特率(bps)// 50 75 110 150 300 600 1200 2400 4800 9600 19200 38400 57600 76800 115.2k 
	BYTE byDataBit;									//数据位 5 6 7 8
	BYTE byStopBit;									//停止位 1 2 
	BYTE byParity;									//保留
	BYTE byFlowcontrol;								//保留
	WORD wDecoderType;								//0-unknow 1-RV800  2-TOTA120 3-S1601 4-CLT-168 5-TD-500  6-V1200 7-ZION 8-ANT 9-CBC 10-CS850A 
													//11-CONCORD 12-HD600 13-SAMSUNG 14-YAAN 15-PIH 16-MG-CS160 17-WISDOM 18-PELCOD1 19-PELCOD2 20-PELCOD3 
													//21-PELCOD4 22-PELCOP1 23-PELCOP2 24-PELCOP3 25-Philips 26-NEOCAM  27-ZHCD 28-DongTian 29-PELCOD5 30-PELCOD6
													//31-Emerson 32-TOTA160 33-PELCOP5
	WORD wDecoderAddress;							//解码器地址:0 - 255 
	long byUarType;   //串口类别RS-485串口0; sdi-485 1; TVI485 2；这里的TVI485不是串口，只是一种通讯类型 
	BYTE bySetPreset[MAX_PRESET - 4];					//保留
	BYTE bySetCruise[MAX_PRESET];					//保留
	BYTE bySetTrack[MAX_PRESET];					//保留
}STRUCT_SDVR_DECODERINFO, *LPSTRUCT_SDVR_DECODERINFO;

//报警输入参数
typedef struct
{
	BYTE byChannel;									//编号
	DWORD dwSize;									//结构体大小
	BYTE szAlarmInName[NAME_LEN];					//报警通道名
	BYTE byAlarmType;								//保留
	BYTE byAlarmInHandle;							//是否处理 0-1
	STRUCT_SDVR_HANDLEEXCEPTION struAlarmHandleType;//处理方式 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
	BYTE byRelRecordChan[MAX_CHANNUM];				//报警触发的录象通道,为1表示触发该通道
	BYTE byEnablePreset[MAX_CHANNUM];				//是否调用预置点 仅用byEnablePreset[0]来判断;
	BYTE byPresetNo[MAX_CHANNUM];					//调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点 
	BYTE byEnableCruise[MAX_CHANNUM];				//保留
	BYTE byCruiseNo[MAX_CHANNUM];					//保留
	BYTE byEnablePtzTrack[MAX_CHANNUM];				//保留 
	BYTE byPTZTrack[MAX_CHANNUM];					//保留
	BYTE byRecordTm;								//报警录像时间 1-99秒 
}STRUCT_SDVR_ALARMININFO, *LPSTRUCT_SDVR_ALARMININFO;

//报警输入参数(扩展)
typedef struct	{
	BYTE byChannel;									//编号
	DWORD dwSize;									//结构体大小
	BYTE sAlarmInName[NAME_LEN];					//报警通道名
	BYTE byAlarmType;								//保留
	BYTE byAlarmInHandle;							//是否处理 0-1
	STRUCT_SDVR_HANDLEEXCEPTION_EX struAlarmHandleType;//处理方式 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
	BYTE byRelRecordChan[MAX_CHANNUM_EX];			//报警触发的录象通道,为1表示触发该通道
	BYTE byEnablePreset[MAX_CHANNUM_EX];			//是否调用预置点 仅用byEnablePreset[0]来判断;
	BYTE byPresetNo[MAX_CHANNUM_EX];				//调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点 
	BYTE byEnableCruise[MAX_CHANNUM_EX];			//保留
	BYTE byCruiseNo[MAX_CHANNUM_EX];				//保留
	BYTE byEnablePtzTrack[MAX_CHANNUM_EX];			//保留 
	BYTE byPTZTrack[MAX_CHANNUM_EX];				//保留
	BYTE byRecordTm;								//报警录像时间 1-99秒 
}STRUCT_SDVR_ALARMININFO_EX, *LPSTRUCT_SDVR_ALARMININFO_EX;

//报警输出参数
typedef struct
{
	BYTE byChannel;									//编号
	DWORD dwSize;									//结构大小
	BYTE sAlarmOutName[NAME_LEN];					//名称 
	DWORD dwAlarmOutDelay;							//输出保持时间 单位秒
	BYTE byEnSchedule;								//报警输出布防时间激活 0-屏蔽 1-激活 
	STRUCT_SDVR_SCHEDTIME struAlarmOutTime[MAX_DAYS][MAX_TIMESEGMENT];// 报警输出激活时间段 				
}STRUCT_SDVR_ALARMOUTINFO, *LPSTRUCT_SDVR_ALARMOUTINFO;

//用户权限
typedef struct
{
	BYTE sUserName[NAME_LEN];						//用户名 
	BYTE sPassword[PASSWD_LEN];						//密码 
	DWORD dwLocalRight[MAX_RIGHT];					//本地权限 
	DWORD dwRemoteRight[MAX_RIGHT];					//远程权限 
	/*数组 0: 通道权限*/
	/*数组 1: 显示设置*/
	/*数组 2: 录像参数*/
	/*数组 3: 定时录像*/
	/*数组 4: 移动录像*/
	/*数组 5: 报警录像*/
	/*数组 6: 网络参数*/
	/*数组 7: 云台设置*/
	/*数组 8: 存储管理*/
	/*数组 9: 系统管理*/
	/*数组 10: 信息查询*/
	/*数组 11: 手动录像*/
	/*数组 12: 回放*/
	/*数组 13: 备份*/
	/*数组 14: 视频参数*/
	/*数组 15: 报警清除*/
	/*数组 16: 远程预览*/
	DWORD dwUserIP;									//用户IP地址(为0时表示允许任何地址) 
	BYTE byMACAddr[MACADDR_LEN];					//物理地址 	
}STRUCT_SDVR_USER_INFO,*LPSTRUCT_SDVR_USER_INFO;

//用户权限(扩展)
typedef struct
{
	BYTE sUserName[NAME_LEN];						//用户名
	BYTE sPassword[PASSWD_LEN];						//密码
	BYTE dwLocalRight[MAX_RIGHT];					//本地权限
	BYTE LocalChannel[MAX_CHANNUM_EX];				//本地通道权限
	BYTE dwRemoteRight[MAX_RIGHT];					//远程权限
	BYTE RemoteChannel[MAX_CHANNUM_EX];				//远程通道权限
	/*数组 0: 通道权限*/
	/*数组 1: 显示设置*/
	/*数组 2: 录像参数*/
	/*数组 3: 定时录像*/
	/*数组 4: 移动录像*/
	/*数组 5: 报警录像*/
	/*数组 6: 网络参数*/
	/*数组 7: 云台设置*/
	/*数组 8: 存储管理*/
	/*数组 9: 系统管理*/
	/*数组 10: 信息查询*/
	/*数组 11: 手动录像*/
	/*数组 12: 回放*/
	/*数组 13: 备份*/
	/*数组 14: 视频参数*/
	/*数组 15: 报警清除*/
	/*数组 16: 远程预览*/
	DWORD dwUserIP;									//用户IP地址(为0时表示允许任何地址) 
	BYTE byMACAddr[MACADDR_LEN];					//物理地址 	
}STRUCT_SDVR_USER_INFO_EX, *LPSTRUCT_SDVR_USER_INFO_EX;

typedef struct
{
	DWORD dwSize;
	STRUCT_SDVR_USER_INFO struUser[MAX_USERNUM];
}STRUCT_SDVR_USER, *LPSTRUCT_SDVR_USER;

typedef struct
{
	DWORD dwSize;
	STRUCT_SDVR_USER_INFO_EX struUser[MAX_USERNUM];
}STRUCT_SDVR_USER_EX, *LPSTRUCT_SDVR_USER_EX;

//DNS
typedef struct
{
	DWORD dwSize;
	char sDNSUser[INFO_LEN];						//DNS账号
	char sDNSPassword[INFO_LEN];					//DNS账号
	char sDNSAddress[INFO_SEQ][INFO_LEN];			//DNS解析地址
	BYTE sDNSALoginddress;							//DNS解析地址中sDNSAddress数组中的指定解析地址的行数
	BYTE sDNSAutoCon;								//DNS自动重连
	BYTE sDNSState;									//DNS登陆  0-注销 1-登陆
	BYTE sDNSSave;									//DNS信息保存
	WORD sDNServer;									//0-- hanbang.org.cn 1--oray.net 2--dyndns.com
	WORD reserve;									//1--立刻重启，0--不重启
}STRUCT_SDVR_DNS, *LPSTRUCT_SDVR_DNS;

//DNS(扩展)
typedef struct	{
	DWORD dwSize;
	char sDNSUser[INFO_LEN];							//DNS账号
	char sDNSPassword[INFO_LEN];					//DNS账号
	char sDNSAddress[INFO_SEQ][INFO_LEN];	//DNS解析地址
	BYTE sDNSALoginddress;							//DNS解析地址中sDNSAddress数组中的指定解析地址的行数
	BYTE sDNSAutoCon;									//DNS自动重连
	BYTE sDNSState;										//DNS登陆  0-注销 1-登陆
	BYTE sDNSSave;											//DNS信息保存
	WORD sDNServer;             // 0--hanbang.org.cn 1--oray.net 2--dyndns.com 3--no-ip.com
	// 4--ddns.hbgk.net (扩展) 5--www.meibu.com 6--freedns.afraid.org 7--multi.super-ddns.com
	BYTE  autoregist;				//用于表示DDNS自动注册时用户名是否默认，0-不默认，1-默认
	BYTE  revserse;					//保留
	BYTE sDNSname[128];		//域名服务器
}STRUCT_SDVR_DNS_EX, *LPSTRUCT_SDVR_DNS_EX;

//PPPoE
typedef struct
{
	DWORD dwSize;
	BYTE szPPPoEUser[INFO_LEN];						//PPPoE用户名
	char szPPPoEPassword[INFO_LEN];					//PPPoE密码
	BYTE byPPPoEAutoCon;							//PPPoE自动重连
	BYTE byPPPoEState;								//PPPoE登陆  0-注销 1-登陆
	BYTE byPPPoESave;								//DNS信息保存
	char reservedData;
}STRUCT_SDVR_PPPoE, *LPSTRUCT_SDVR_PPPoE;

//平台信息
typedef struct
{
	char	szServerIP[16];							//服务器IP地址
	DWORD	nPort;									//服务器端口号
	char	puId[NAME_LEN];							//主机ID号
	DWORD	nInternetIp ;
	DWORD	nVideoPort;
	DWORD	nTalkPort;
	DWORD	nCmdPort;
	DWORD	nVodPort;
	DWORD	tran_mode;								//1 子码流  0 主码流
	DWORD ftp_mode;									//以FTP方式进行中心存储 1 开启 0 关闭
	DWORD max_link;									//最大连接数 0 - 32
}STRUCT_SDVR_SERVERCFG, *LPSTRUCT_SDVR_SERVERCFG;

//平台信息(扩展)
typedef struct
{
	BYTE szRemoteIP[16];							//远端IP地址
	BYTE szLocalIP[16];								//本地IP地址
	BYTE szLocalIPMask[16];							//本地IP地址掩码
	BYTE szUsername[NAME_LEN];						//用户名
	BYTE szPassword[PASSWD_LEN];					//密码
	BYTE byPPPMode;									//PPP模式, 0－主动，1－被动	
	BYTE byRedial;									//是否回拨 ：0-否,1-是
	BYTE byRedialMode;								//回拨模式,0-由拨入者指定,1-预置回拨号码
	BYTE byDataEncrypt;								//数据加密,0-否,1-是
	DWORD dwMTU;									//MTU
	BYTE sTelephoneNumber[PHONENUMBER_LEN];			//电话号码
}STRUCT_SDVR_PPPCFG, *LPSTRUCT_SDVR_PPPCFG;

//串口信息
typedef struct
{
	BYTE byChannel;
	DWORD dwSize;
	DWORD dwBaudRate;								//波特率(bps)  
	BYTE byDataBit;									//数据有几位 5－8 
	BYTE byStopBit;									//停止位 1-2 
	BYTE byParity;									//校验 0－无校验，1－奇校验，2－偶校验;
	BYTE byFlowcontrol;								//0－无，1－软流控,2-硬流控
	DWORD dwWorkMode;								//保留
	STRUCT_SDVR_PPPCFG struPPPConfig;				//保留
}STRUCT_SDVR_SERIALINFO, *LPSTRUCT_SDVR_SERIALINFO;

//远程录像文件查询和点播下载
typedef struct
{
	DWORD dwYear;									//年
	DWORD dwMonth;									//月
	DWORD dwDay;									//日
	DWORD dwHour;									//时
	DWORD dwMinute;									//分
	DWORD dwSecond;									//秒
}STRUCT_SDVR_TIME, *LPSTRUCT_SDVR_TIME;

//历史日志信息
typedef struct
{
	BYTE byChannel;									//通道号
	DWORD dwFileType;								//文件类型(0-全部,1-定时录像,2-移动侦测,3-报警触发,4-命令触发)	
	STRUCT_SDVR_TIME stStartTime;					//录像开始时间
	STRUCT_SDVR_TIME stStopTime;					//录像结束时间
}STRUCT_SDVR_FILE_FIND, *LPSTRUCT_SDVR_FILE_FIND;

//历史日志信息(扩展)
typedef struct
{
	char sFileName[100];							//文件名
	STRUCT_SDVR_TIME stStartTime;					//文件的开始时间
	STRUCT_SDVR_TIME stStopTime;					//文件的结束时间
	DWORD dwFileSize;								//文件的大小
}STRUCT_SDVR_FILE_DATA, *LPSTRUCT_SDVR_FILE_DATA;

//下载
typedef struct 
{
	BYTE year;										//年
	BYTE month;										//月
	BYTE day;										//日
	BYTE channel;									//通道
	BYTE bgn_hour;									//开始时
	BYTE bgn_minute;								//开始分
	BYTE bgn_second;								//开始秒
	BYTE end_hour;									//结束时
	BYTE end_minute;								//结束分
	BYTE end_second;								//结束秒
}TREC_BACKUPTIME, *LPTREC_BACKUPTIME;



//远程服务器工作状态
typedef struct
{
	DWORD dwVolume;				//硬盘的容量（MB）
	DWORD dwFreeSpace;			//硬盘的剩余空间（MB）
	DWORD dwHardDiskState;		//硬盘状态（dwVolume有值时有效） 0-正常 1-磁盘错误 2-文件系统出错
}STRUCT_SDVR_DISKSTATE, *LPSTRUCT_SDVR_DISKSTATE;

//通道状态
typedef struct
{
	BYTE byRecordStatic;							//通道是否在录像,0-不录像,1-录像
	BYTE bySignalStatic;							//连接的信号状态,0-正常,1-信号丢失
	BYTE byHardwareStatic;							//保留
	char reservedData;
	DWORD dwBitRate;								//实际码率
	DWORD dwLinkNum;								//客户端连接的个数
	DWORD dwClientIP[MAX_LINK];						//保留
}STRUCT_SDVR_CHANNELSTATE, *LPSTRUCT_SDVR_CHANNELSTATE;

//其它状态
typedef struct
{
	DWORD dwDeviceStatic;									//保留
	STRUCT_SDVR_DISKSTATE  stHardDiskStatic[MAX_DISKNUM];	//硬盘状态
	STRUCT_SDVR_CHANNELSTATE stChanStatic[MAX_CHANNUM];		//通道的状态
	DWORD byAlarmInStatic;									//报警输入端口的状态,按位表示
	DWORD byAlarmOutStatic;									//报警输出端口的状态,按位表示
	DWORD dwLocalDisplay;									//保留
}STRUCT_SDVR_WORKSTATE, *LPSTRUCT_SDVR_WORKSTATE;

//其它状态(扩展)
typedef struct
{
	DWORD dwDeviceStatic;									//保留
	STRUCT_SDVR_DISKSTATE stHardDiskStatic[MAX_DISKNUM];	//硬盘状态
	STRUCT_SDVR_CHANNELSTATE stChanStatic[MAX_CHANNUM_EX];	//通道的状态
	DWORD byAlarmInStatic[MAX_ALARMIN_EX];					//报警端口的状态 按位表示
	DWORD byAlarmOutStatic[MAX_ALARMOUT_EX];				//报警输出端口的状态 按位表示
	DWORD dwLocalDisplay;									//保留
}STRUCT_SDVR_WORKSTATE_EX, *LPSTRUCT_SDVR_WORKSTATE_EX;

//报警输出状态
typedef struct
{
	BYTE byAlarm;											//报警输出状态 0-不报警 1-报警
	WORD wAlarm;											//报警输出状态 bit0-15代表16个报警输出, 0-状态不变 1-执行byAlarm操作
}STRUCT_SDVR_ALARMOUT, *LPSTRUCT_SDVR_ALARMOUT;

//预置点
typedef struct
{
	DWORD  byChannel;										//设置通道
	WORD Preset[PRESETNUM];
	WORD PresetPoll;										//多预置点轮巡开启或关闭表示1-255
	WORD presettime;										//多预置点轮巡时间			1-99
}STRUCT_DVR_PRESETPOLL, *LPSTRUCT_DVR_PRESETPOLL;

//格式化硬盘

typedef struct  
{
			DWORD	dwCurrentFormatDisk;	//正在格式化的硬盘号[0, 15]
			DWORD	dwCurrentDiskPos;		//正在格式化的硬盘进度[0, 100]
			DWORD	dwFormatState;			//格式化状态 0-正在格式化 1-格式化完成 2-格式化错误 3-要格式化的磁盘不存在 4-格式化中途错误 5-磁盘正在被使用
}STRUCT_SDVR_FORMAT,*LPSTRUCT_SDVR_FORMAT;


typedef struct 
{
	char server[32];										//服务器
	DWORD port;												//端口
	DWORD auto_enbale;										//开启ntp服务,0-表示手动,1-表示自动
	DWORD server_index;										//服务器索引号
	DWORD sync_period;										//同步周期，
	DWORD sync_unit;										//同步周期，0-分钟 1-小时 2-天 3-星期 4-月
	DWORD sync_time;										//保留
	DWORD time_zone;										//时区
	DWORD reserve;											//保留
}STRUCT_SDVR_NTPCONFIG, *LPSTRUCT_SDVR_NTPCONFIG;

typedef struct 
{
	char szHost[MAX_SMTP_HOST];								//发送邮件的SMTP服务器，例如：126信箱的是smtp.126.com
	DWORD dwPort;											//服务器端口，发送邮件(SMTP)端口：默认值25
	char szUser[32];										//邮件用户名
	char szPwd[32];											//邮件用户密码
	char szSend_addr[MAX_SMTP_HOST];						//FROM：邮件地址
	char szRecv_addr[MAX_SMTP_ADDR];						//TO：邮件地址，如果是多个邮件地址，以';'隔开
	DWORD dwSend_period;									//上传周期,单位(分钟)
	DWORD dwSnap_enable;									//是否抓拍上传
	char szReserve[MAX_STRING];								//保留
}STRUCT_SDVR_SMTPCONFIG, *LPSTRUCT_SDVR_SMTPCONFIG;

typedef struct 
{
	DWORD  dwPoll_type;										//轮训类型：0：轮训；1：spot轮巡
	DWORD dwEnable;											//启用？0-禁用，1-启用
	DWORD dwPeriod;											//轮训间隔，单位秒
	DWORD dwFormat;											//画面格式：0-0ff, 1-1,  4-2x2, 9-3x3, 16-4x4
	BYTE byCh_list[MAX_CHANNUM_EX];
}STRUCT_SDVR_POLLCONFIG, *LPSTRUCT_SDVR_POLLCONFIG;

typedef struct 
{
	BYTE byMatrix_channel[MAX_CHANNUM_EX];					//视频矩阵对应通道 从1开始，0xff表示关闭
	BYTE byReserve[32];										//保留位
}STRUCT_SDVR_VIDEOMATRIX, *LPSTRUCT_SDVR_VIDEOMATRIX;

//历史视频日志
typedef struct
{
	unsigned uSecond : 6;			//秒: 0~59
	unsigned uMinute : 6;			//分: 0~59
	unsigned uHour : 5;				//时: 0~23
	unsigned uDay : 5;				//日: 1~31
	unsigned uMonth : 4;			//月: 1~12
	unsigned nYear : 6;				//年: 2000~2063
}MFS_FIELD_TIME, *LPMFS_FIELD_TIME;

typedef union
{
	unsigned int nLogTime;
	MFS_FIELD_TIME stFieldTime;
}UNMFS_TIME, *LPUNMFS_TIME;

//查询给定天数内是否每天都有录像文件  
//0-查询失败，pc端可以再次查询；1-查询成功，并且每天都有录像数据；2-查询成功，并且至少有一天没有录像数据
typedef struct
{
	BYTE byChannel;
	BYTE byType;		//手动录像--0x01, 定时录像--0x02, 移动录像--0x03, 报警录像--0x04, 全部录像--0xFF或0x00
	UNMFS_TIME unStartTime;
	UNMFS_TIME unEndTime;
	WORD dwStart;
	WORD dwNum;	//一次查询的个数，现在定义是100 
	//协议二中该项定义为码流类型，0-主码流，1-子码流 
}STRUCT_SDVR_RECFIND, *LPSTRUCT_SDVR_RECFIND;

//日志记录
typedef struct
{
	UNMFS_TIME unStartTime;
	UNMFS_TIME unEndTime;
	unsigned int uLenght;
	BYTE byChannel;
	BYTE byType;
	BYTE byReserve1;
	BYTE byReserve2;
}MFS_RECSEG_INFO, *LPMFS_RECSEG_INFO;

typedef struct
{
	DWORD dwTotalNum;
	MFS_RECSEG_INFO stRecSeg[MAX_REC_NUM];
}STRUCT_SDVR_RECINFO, *LPSTRUCT_SDVR_RECINFO;

//点播历史视频
typedef struct
{
	BYTE byChannel;					//通道号(0,1,2,3....)
	BYTE byType;							//点播放类型	0x01-手动录像,0x02-定时录像,0x04-移动录像,0x08-报警录像,0x09-卡号录像,0xff-所有录像
	UNMFS_TIME unBegTime;		//点播开始时间
	BYTE byReserve1;					//保留1
	BYTE byReserve2;					//保留2
}STRUCT_SDVR_VOD, *LPSTRUCT_SDVR_VOD;

//点播历史视频扩展(指定时间段)
typedef struct
{
	BYTE byChannel;					//通道号(0,1,2,3....)
	BYTE byType;					//点播放类型
	UNMFS_TIME unBegTime;			//点播开始时间
	UNMFS_TIME unEndTime;			//点播结束时间
	BYTE streamtype;				//码流类型，0-主码流，1-子码流 
	BYTE byReserve2;				//保留2
}STRUCT_SDVR_VOD_EX, *LPSTRUCT_SDVR_VOD_EX;

//主动连接点播历史视频
typedef struct
{
	DWORD msgid; //消息ID,该id 由平台生成，DVR 原封不动返回，msgid 用于平台区分主机新建立的socket 连接。使平台可以知道该socket 连接对应的命令。
	BYTE byChannel; //通道号[0, n-1], n:通道数
	BYTE byType; //点播类型
	BYTE reserve1[2]; //保留
	UNMFS_TIME unBegTime; //点播开始时间，同录像查询请求消息中定义，年份-2000
	UNMFS_TIME unEndTime; //点播结束时间，时间跨度一天之内
	BYTE streamtype;			//码流类型，0-主码流，1-子码流
	BYTE reserve2[15];			//保留
} STRUCT_SDVR_VOD_EX1;

typedef struct
{
	DWORD msgid; //消息ID,该id 由平台生成，DVR 原封不动返回，msgid 用于平台区分主机新建立的socket 连接。使平台可以知道该socket 连接对应的命令。
	DWORD dwVodID; //点播id
	BYTE streamtype;      //码流类型，0-主码流，1-子码流
	BYTE reserve[15];     //保留
}STRUCT_SDVR_VOD_ANS_EX;

//点播控制
typedef struct
{
	DWORD dwVodID;					//回放句柄
	BYTE byCtrl;					//0-正常 1-暂停 2-快进 3-快退 4-帧进 5-慢放
	BYTE bySpeed;					//快进快退慢放速度
	BYTE bySeekFlag;				//拉进度条标志
	BYTE byPecent;					//进度百分比
}STRUCT_SDVR_VODCTL, *LPSTRUCT_SDVR_VODCTL;

//下载历史视频
typedef struct
{
	BYTE byChannel;					//通道号
	BYTE byType;							//备份类型	0x00-手动录像,0x01-定时录像,0x02-移动录像,0x03-报警录像,0x04-卡号录像,0xff-所有录像
	UNMFS_TIME unBegTime;		//开始时间
	UNMFS_TIME unEndTime;		//结束时间
}STRUCT_SDVR_BACKINFO, *LPSTRUCT_SDVR_BACKINFO;

typedef struct
{
	DWORD msgid; //消息ID,该id 由平台生成，DVR 原封不动返回，msgid 用于平台区分主机新建立的socket 连接。使平台可以知道该socket 连接对应的命令。
	BYTE byChannel; //通道号
	BYTE byType; //备份类型：0-手动；1-定时；2-移动；3-探头报警；0x0f-所有录像
	WORD file_index; //客户端下载的文件列表的索引号，从0 开始
	DWORD file_offset; //文件偏移大小，刚开始为0
	UNMFS_TIME unBegTime; //开始时间
	UNMFS_TIME unEndTime; //结束时间
	BYTE streamtype;   //码流类型，0-主码流，1-子码流
	BYTE reserve[27];	//保留
}STRUCT_SDVR_BACKINFO_EX2;

typedef struct
{
	DWORD msgid; //消息ID,该id由平台生成，DVR原封不动返回，msgid用于平台区分主机新建立的socket连接。使平台可以知道该socket连接对应的命令。
	BYTE reserve[4]; //保留
}STRUCT_SDVR_BACKINFO_RSP;

//下载文件返回
typedef struct
{
	DWORD dwChannel;				//通道号
	DWORD dwFileLen;				//本文件长度  以B为单位
	UNMFS_TIME unBegTime;			//录像文件开始时间
	UNMFS_TIME unEndTime;			//录像文件结束时间
	DWORD dwTotalFileLen;			//总文件长度  以KB为单位
}STRUCT_SDVR_FILEINFO, *LPSTRUCT_SDVR_FILEINFO;

//主动连接语音对讲
typedef struct
{
	DWORD msgid; //消息ID,该id由平台生成，DVR原封不动返回，msgid用于平台区分主机新建立的
	//socket连接。使平台可以知道该socket连接对应的命令。
	BYTE reserve[4]; //保留
}STRUCT_SDVR_VOICE;

typedef struct
{
	DWORD msgid; //消息ID,该id由平台生成，DVR原封不动返回，msgid用于平台区分主机新建立的socket
	//连接。使平台可以知道该socket连接对应的命令。
	BYTE format; //音频压缩格式，见附件"音频数据压缩格式"，,默认的编码格式为PCM
	BYTE bitspersample; //表示采样位宽(单位bit)，取值范围(1~255)，默认是16
	BYTE samplerate; //表示采样率(单位K[1000])
	BYTE reserved[7]; //保留
	unsigned short msglen; //保留
} STUCT_SDVR_VIOCE_S;

//主动连接抓图
typedef struct
{
	DWORD msgid; //消息ID，该id 由平台生成，DVR 原封不动返回
	BYTE Channel; //通道号[0, n-1],n:通道数
	BYTE pic_format; //图片格式，0--jpg，1-bmp；目前只能是jpg 格式
	BYTE reserve1[10]; //保留
	DWORD len; //图片数据长度
	BYTE reserve2[16]; //保留
	char data[]; //图片数据
}STRUCT_SDVR_GET_PHOTO;

//设备日志
typedef struct
{
  WORD wYear;
  BYTE byMonth;
  BYTE byDay;
  WORD wStart;
  WORD wnum;  
}STRUCT_SDVR_REQLOG, *LPSTRUCT_SDVR_REQLOG;

//8000设备日志格式
typedef struct
{
  UNMFS_TIME		stTime;
  BYTE				byType;			//类型
  BYTE				byOperate;		//操作码
  char				szAccount[18];	//用户
  UINT				nIpAddr;		//0-Local other-ip
  UINT				nInput;			//事件输入输出
  UINT				nOutput;
  char				szData[28];		//其他信息
}MFS_LOG_DAT, *PMFS_LOG_DAT;

typedef struct
{
  DWORD  dwTotalNum;
  MFS_LOG_DAT stDevLogData[MAX_LOGO_NUM];
}STRUCT_SDVR_LOGINFO, *LPSTRUCT_SDVR_LOGINFO;

//7000T设备日志格式
typedef MFS_FIELD_TIME SYSTIME;
typedef struct
{
	SYSTIME stTime;					//记录时间
	char PriType;					//主类型
	char SecType;					//次类型
	char Param;						//参数类型
	char Channel;					//通道号
	unsigned int nHost;				//主机IP，0表示本机
	char szUser[17];				//用户名
	char HardDriver;				//硬盘号
	char AlarmIn;					//报警输入
	char AlarmOut;					//报警输出
	char Reserve[32];				//保留数据
}LOG_DAT, *PLOG_DAT;

typedef struct 
{
	DWORD dwTotalNum;
	LOG_DAT stLogData[MAX_LOGO_NUM];
}STRUCT_SDVR_LOGINFO_7KT, *LPSTRUCT_SDVR_LOGINFO_7KT;


//0通道
typedef enum
{
    NET_AUSTREAM_DISABLE, //视频流
    NET_AUSTREAM_ENABLE,  //复合流
}NET_AUSTREAMADD_E;

typedef enum
{
    NET_QCIF, //QCIF
    NET_QVGA, //QVGA
    NET_CIF,  //CIF
    NET_DCIF, //DCIF
    NET_HD1,  //HD1
    NET_VGA,  //VGA
    NET_FD1,  //FD1
    NET_SD,   //SD
    NET_HD    //HD
}NET_RESOLUTION_E;

typedef enum
{
    NET_BITRATE_CHANGE,		//变码率
    NET_BITRATE_NOCHANGE,	//定码率
}NET_BITRATETYPE_E;

typedef enum
{
    NET_VQUALITY_BEST=0,	//最高
    NET_VQUALITY_BETTER,	//较高
    NET_VQUALITY_GOOD,		//高
    NET_VQUALITY_NORMAL,	//中
    NET_VQUALITY_BAD,		//低
    NETT_VQUALITY_WORSE		//最低
}NET_VQUALITY_E;

typedef struct
{
    NET_AUSTREAMADD_E byStreamType;		//视频流类型
    NET_RESOLUTION_E byResolution;		//视频分辨率
    NET_BITRATETYPE_E byBitrateType;	//码率类型
    NET_VQUALITY_E byPicQuality;		//图像质量
    DWORD dwVideoBitrate;				//视频码率 实际码率
    WORD dwVideoFrameRate;				//帧率 PAL 2-30 N 2-30
    WORD quant;							//量化因子 1-31
}STRUCT_DVR_VENC_CONFIG, *LPSTUCT_DVR_VENC;

typedef struct 
{
    DWORD 					enable;
    BYTE 					chlist[MAX_CHANNUM_EX];
    STRUCT_DVR_VENC_CONFIG 	venc_conf; 
    DWORD 					reserve;
}STRUCT_DVR_ZERO_VENC_CONFIG, *LPSTRUCT_DVR_ZERO_VENC;

//视频遮挡报警状态
typedef struct
{
	BYTE  byChannel;
	DWORD dwVCoverEnable;          //遮挡报警使能，1-启用，0-不启用
	DWORD dwSensorOut;			   //联动报警输出，按位表示，1-联动，0-不联动
}STRUCT_SDVR_VCOVER_ALM,*LPSTRUCT_SDVR_VCOVER_ALM;

typedef struct	
{
	BYTE  cbStreamType;      	//码流类型 1-主流 2-子流 3-第三码流
	BYTE  cbReserve[3];			//保留
}STRUCT_SDVR_REQIPCWORKPARAM,*LPSTRUCT_SDVR_REQIPCWORKPARAM;

typedef struct	
{
	BYTE	cbStartHour;  		//开始小时 0-23
	BYTE	cbStartMin;  		//开始分钟 0-59
	BYTE	cbStopHour; 		//结束小时  0-23
	BYTE	cbStopMin;  		//结束分钟  0-59
}STRUCT_SDVR_IPCSCHEDTIME,*LPSTRUCT_SDVR_IPCSCHEDTIME; 

typedef struct
{
	WORD 	wLightRange;		//ICR亮度切换临界值，取值范围[80,120];
	WORD 	wEnable;			//0-不支持1--亮度值有效  2--时间段有效
	STRUCT_SDVR_IPCSCHEDTIME stSchedTime[2];
}STRUCT_SDVR_ICRTIME,*LPSTRUCT_SDVR_ICRTIME;
//当wEnable = 1，wLightRange有效，采用亮度作为彩转黑判断条件；
//当wEnable = 2，stSchedTime[2]有效，采用stSchedTime[2]两个时间段的时间内IPC设置成黑白模式；
//STRUCT_SDVR_SCHEDTIME stSchedTime[2];
//采用两个时间段是考虑到用户的早上和晚上两个时间区域要设置成黑白模式，所以采用两个时间段，用户可以设
//置一个时间段（cbStartHour、cbStartMin、cbStopHour、cbStopMin都是0时该时间段无效）;

typedef struct
{
	DWORD  dwShutterIndex; //当前快门时间索引值,表示基于dwShutterVal中的位置，例如dwShutterIndex = 2，
	//则当前快门时间为dwShutterVal[2];
	DWORD  dwShutterVal[32];//获取快门时间的支持参数列表：比如：dwShutterVal[0]= 2048
	//dwShutterVal[1]=4096，dwShutterVal[2]=0则表示数据共有2个选项：1/4096,1/2048。
	//
	//当dwShutterVal[0]为非零值，dwShutterVal[1]=0 且dwShutterIndex = 0时，表示只有一个选项，快门时间为dwShutterVal[0]的值；当dwShutterVal[0]为非零值，dwShutterVal[1]=0 且dwShutterIndex !=0时，表示一个取值范围，快门时间为dwShutterIndex的值
	//例如：当dwShutterVal[0]= 2048，dwShutterVal[1]=0且dwShutterIndex = 0时，表示只有一个选项，快门时间为2048；
	//当dwShutterVal[0]= 2048， dwShutterVal[1]=0且dwShutterIndex != 0时，dwShutterIndex取值范围为[1,2048]，快门时间为当前dwShutterIndex的值；
	//当dwShutterVal[0]=0时，表示不支持
}STRUCT_SDVR_SHUTTERVAL,*LPSTRUCT_SDVR_SHUTTERVAL;

typedef struct
{
	DWORD dwSceneIndex;	//当前镜头索引值，表示基于cbSceneVal中的位置，例如dwSceneIndex = 2，
	//则当前镜头为：cbSceneVal[2] = “JCD661 lens”，当cbSceneVal[x] =”\0”表示总共有
	//x个数据项；
	BYTE  cbSceneVal[8][32];	//该机型支持的镜头种类,//0 - Full Maual lens,1 - DC Iris lens, //2 - JCD661 lens,
	//3 - Ricom NL3XZD lens,4 - Tamron 18X lens，当数组成员全部为0，表示不支持
} STRUCT_SDVR_SCENEVAL,*LPSTRUCT_SDVR_SCENEVAL;

typedef struct
{
	DWORD	dwResoluIndex;	//当前分辨率索引值，表示基于dwResolution中的位置，例如dwResoluIndex= 2，
	//则当前分辨率为dwResolution[2]中所指定的分辨率
	DWORD	dwResolution[16];	//该机型支持的分辨率，用DWORD来表示支持的分辨率，例如：
	//dwResolution[0]=0x07800438，高两字节（0x0780=1920）、低两字节（0x0438=1080）；
} STRUCT_SDVR_RESOLUTION,*LPSTRUCT_SDVR_RESOLUTION; //当数组成员全部为0，表示不支持

typedef struct
{
	DWORD dwAgcIndex;     //当前AGC的索引值，表示基于cbAgcVal中的位置，例如cbAgcVal =2，则表示AGC
		//值为cbAgcVal[2]中的值；
	BYTE  cbAgcVal[32];	  //AGC（自动增益）的支持参数列表,当cbAgcVal[1]= 0时表示cbAgcVal[0]中存储的是
	//一个取值范围，如cbAgcVal[0]= 128,则表示取值范围为：[1,128],当cbAgcVal[1]!=0
	//时，则表示cbAgcVal数组中存储的是具体的值，例如 cbAgcVal[0]= 32，//cbAgcVal[1]=64等，当cbAgcVal [x] =0表示总共有x个数据项。
	//当cbAgcVal[0]为非零值，cbAgcVal[1]=0 且dwAgcIndex = 0时，表示只有一个选项，AGC时间为cbAgcVal [0]的值；当cbAgcVal[0]为非零值，cbAgcVal[1]=0 且dwAgcIndex!=0时，表示一个取值范围，AGC时间为dwAgcIndex的值
	//例如：当cbAgcVal[0]= 32，cbAgcVal[1]=0且dwAgcIndex = 0时，表示只有一个选项，AGC时间为32；
	//当cbAgcVal[0]= 32， cbAgcVal[1]=0且dwAgcIndex!= 0时，dwAgcIndex取值范围为[1,32]，AGC时间为当前dwAgcIndex的值；
	//当cbAgcVal [0]=0时，表示不支持

} STRUCT_SDVR_AGCVAL,*LPSTRUCT_SDVR_AGCVAL;

typedef struct
{
	BYTE	cbMinFrameRate;	//该机型支持的最小编码帧率值;，取值范围为：1—2^8，下同。只支持获取
	BYTE	cbMaxFrameRate;	//该机型支持的最大编码帧率值; 取值范围为：1—2^8只支持获取
	BYTE	cbCurFrameRate;	//该机型设置的当前编码帧率值; 取值范围为：1—2^8,支持设置与获取
	BYTE    cbreserve;		//保留
} STRUCT_SDVR_FRAMERATE,*LPSTRUCT_SDVR_FRAMERATE;

typedef struct
{
	DWORD dwLength;			//结构体长度
	BYTE  cbStreamEnable;   //是否开启当前码流: 0-不支持1-disable 2-enable
	BYTE  cbStreamType;     //码流类型 0-不支持1-主流2-子流 3-第三码流
	BYTE  cbAudioEnable;    //音频使能 0-不支持 1-无音频 ,2-有音频
	BYTE  cbAntiFlicker;    //抗闪烁设置0-不支持1-60HZ   2-50HZ
	STRUCT_SDVR_FRAMERATE  stFrameRate;		//编码帧率设置;
	STRUCT_SDVR_SHUTTERVAL stShutterVal;	//快门相关参数获取
	STRUCT_SDVR_SCENEVAL	stSceneVal;		//镜头相关参数获取
	STRUCT_SDVR_RESOLUTION	stResolution;	//解析度相关
	STRUCT_SDVR_AGCVAL		stAgcVal;		//Agc相关
	DWORD	dwBitRateVal;		//视频码率  0-不支持1-100K 2-128K，3-256K，4-512K，5-1M，6-1.5M，7-2M，8-3M, 9-4M 10-5M，11-6M，12-7M，13-8M, 14-9M，15-10M，16-11 M，17-12M
	//其他：码率值（kbps）有效范围 32~2^32,大于等于32，以K为单位；
	BYTE	cbFoucusSpeedVal;	//当为0时表示不支持该功能
	BYTE	cbDigitalFoucusVal;	// 当为0时表示不支持该功能
	BYTE	cbImageTurnVal;		//当前图像翻转设置 //1-不翻转,2-水平翻转 3-垂直翻转, 4-水平&垂直,0-不支持
	BYTE	cbBlackWhiteCtrlVal;//当前黑白模式设置 //1- Off, 2- On, 3–Auto, 0-不支持
	BYTE	cbIRISCtrl;			//Iris control mode 光圈控制模式设置，1-Off,2-Basic, 3-Advanced,0-不支持
	BYTE	cbAutoFoucusVal;	//自动对焦，1-开 2-关 0-不支持
	BYTE 	cbAWBVal;			//白平衡场景模式设置，1-auto_wide, 2-auto_normal, 3-sunny, 4-shadow, 5-indoor,
	//6-lamp, 7-FL1, 8-FL2,0-不支持
	BYTE 	cbA3Ctrl;			//3A控制0-不支持1-off; 2-Auto Exposure; 3-Auto White Balance; 4-both, (Auto Focus no support)

	STRUCT_SDVR_ICRTIME stICRTime;
	//当cbBlackWhiteCtrlVal = 3（3–Auto），stICRTime（滤光片切换模式设置）才允许设置，获取可以在任何情况
	//下支持获取；
	BYTE	cbFNRSuppVal;		//当前帧降噪设置，1-开,2-关,0-不支持
	BYTE	cbStreamKindVal;	//当前码流类型，0-不支持1-定码流,2-变码流
	BYTE	cbVideoOutKindVal;	//vout视频输出设置：0-不支持1-disable, 2-CVBS, 3-HDMI, 4-YPbPr等等
	BYTE	cbWDRVal;			//宽动态设置, 1-开 2-关, 0-不支持
	BYTE   cbColorMode;			//色彩风格设置1-TV 2-PC，0-不支持
	BYTE   cbSharpNess;			//锐度设置，取值范围为：[1,255] ，0-不支持
	BYTE	cbPlatformType;		//默认为0-不支持
	BYTE	cbReserve[17];		//默认为0-不支持
}STRUCT_SDVR_REIPCWORKPARAM,*LPSTRUCT_SDVR_REIPCWORKPARAM;

//用户权限模式
//typedef enum
//{
//	SYSNETAPI_USR_MODE_OLD = 1,			//针对7000L老GUI的用户权限模式
//	SYSNETAPI_USR_MODE_NEW,				//针对7000L新GUI的用户权限模式
//	SYSNETAPI_USR_MODE_HIGHRESOLUTION,	//针对高清，未用
//}SYSNETAPI_USR_MODE;


//用户信息扩展1
//结构体数据项1
//typedef struct 
//{
//	BYTE dwLocalRight[32]; //本地权限 1.数组0未使用；2.取值：0-无权限，1-有权限 
//	/*数组1-常规设置*/
//	/*数组2-录像设置*/
//	/*数组3-输出设置*/
//	/*数组4-报警设置*/
//	/*数组5-串口设置*/
//	/*数组6-网络设置*/
//	/*数组7-录像回放*/
//	/*数组8-系统管理*/
//	/*数组9-系统信息*/
//	/*数组10-报警清除*/
//	/*数组11-云台控制*/
//	/*数组12-关机重启*/
//	/*数组-13-USB升级*/
//	/*数组14-备份*/
//	BYTE LocalChannel[128]; //本地用户对通道的操作权限，最大128个通道，0-无权限，1-有权限
//	BYTE dwRemoteRight[32]; //远程登陆用户所具备的权限 1.数组0未使用；2.取值：0-无权限，1-有权限 
//	/*数组1-远程预览*/
//	/*数组2-参数设置*/
//	/*数组3-远程回放*/
//	/*数组4-远程备份*/
//	/*数组5-查看日志*/
//	/*数组6-语音对讲*/
//	/*数组7-远程升级*/
//	/*数组8-远程重启*/
//	BYTE RemoteChannel[128]; //用户远程登陆时对通道所具备的权限，最大128个通道，0-无权限，1-有权限
//}STRUCT_USERINFO, *LPSTRUCT_USERINFO;

typedef struct{
	BYTE sUserName[NAME_LEN];		    //用户名 
	BYTE sPassword[32];					//密码
	BYTE dwLocalRight[MAX_RIGHT];	    //本地权限
	BYTE LocalChannel[MAX_CHANNUM];		//本地通道权限
	BYTE dwRemoteRight[MAX_RIGHT];	    //远程权限
	BYTE RemoteChannel[MAX_CHANNUM];	//远程通道权限
	//数组 1: 显示设置
	//数组 2: 录像参数
	//数组 3: 定时录像
	//数组 4: 移动录像
	//数组 5: 报警录像
	//数组 6: 网络参数
	//数组 7: 云台设置
	//数组 8: 存储管理
	//数组 9: 系统管理
	//数组 10: 信息查询
	//数组 11: 手动录像
	//数组 12: 回放
	//数组 13: 备份
	//数组 14: 视频参数
	//数组 15: 报警清除
	//数组 16: 远程预览
	DWORD dwUserIP;				//用户IP地址(为0时表示允许任何地址)	
	BYTE byMACAddr[8];			//物理地址	
}STRUCT_USERINFO, *LPSTRUCT_USERINFO;



//用户信息扩展1
//结构体数据项2
//typedef struct 
//{
//	BYTE dwLocalRight[32]; //本地权限 1.数组0未使用；2.取值：0-无权限，1-有权限
//	/*数组 1: 手动录像*/
//	/*数组 2: 手动报警*/
//	/*数组 3: 录像回放*/
//	/*数组 4: 备份管理*/
//	/*数组 5: 磁盘管理*/
//	/*数组 6: 系统关机*/
//	/*数组 7: 系统重启*/
//	/*数组 8: 云台控制权限*/
//	/*数组 9: 报警清除权限*/
//	/*数组 10: 常规设置*/ 
//	/*数组 11: 输出设置*/ 
//	/*数组 12: 录像设置*/
//	/*数组 13: 定时录像*/
//	/*数组 14：报警设置*/ 
//	/*数组 15：串口设置*/ 
//	/*数组 16：云台设置*/ 
//	/*数组 17：网络设置*/
//	/*数组 18：系统信息*/ 
//	/*数组 19：录像状态*/          
//	/*数组 20：报警状态*/
//	/*数组 21：在线状态*/
//	/*数组 22：日志查询*/ 
//	/*数组 23：快速设置*/ 
//	/*数组 24：用户管理*/ 
//	/*数组 25：恢复出厂设置*/ 
//	/*数组 26：升级权限*/
//	/*数组 27：定时重启*/ 
//	/*数组 28：卡号录像*/
//	BYTE LocalChannel[128]; //本地用户对通道的操作权限，最大128个通道，0-无权限，1-有权限
//	BYTE dwRemoteRight[32]; //远程登陆用户所具备的权限 1.数组0未使用；2.取值：0-无权限，1-有权限
//	/*数组 1: 远程预览*/ 
//	/*数组 2: 参数设置*/
//	/*数组 3: 远程回放*/ 
//	/*数组 4: 远程备份*/
//	/*数组 5: 查看日志*/ 
//	/*数组 6: 语音对讲*/
//	/*数组 7: 远程升级*/
//	/*数组 8：远程重启*/
//	BYTE RemoteChannel[128]; //用户远程登陆时对通道所具备的权限，最大128个通道，0-无权限，1-有权限
//}STRUCT_USERINFO_GUI, *LPSTRUCT_USERINFO_GUI;

typedef struct
{
	BYTE sUserName[32];  		//用户名 以’\0’结束字符串
	BYTE sPassword[32];  		//密码 以’\0’结束字符串
	BYTE dwLocalRight[32]; 		//本地权限 1.数组0未使用；2.取值：0-无权限，1-有权限
	/*数组 1: 手动录像*//*数组 2: 手动报警*//*数组 3: 录像回放*//*数组 4: 备份管理*/
	/*数组 5: 磁盘管理*//*数组 6: 系统关机*//*数组 7: 系统重启*//*数组 8: 云台控制权限*//*数组 9: 报警清除权限*//*数组 10: 常规设置*/ /*数组 11: 输出设置*/ /*数组 12: 录像设置*//*数组 13: 定时录像*/ 
	/*数组14：报警设置*/ /*数组 15：串口设置*/ /*数组 16：云台设置*/ /*数组 17：网络设置*//*数组 18：系统信息*/ /*数组 19：录像状态*/          /*数组 20：报警状态*/ /*数组 21：在线状态*/
	/*数组 22：日志查询*/ /*数组 23：快速设置*/ /*数组 24：用户管理*/ /*数组 25：恢复出厂设置*/ /*数组 26：升级权限*/ /*数组 27：定时重启*/ /*数组 28：卡号录像*/
	BYTE LocalChannel[128]; 	//本地用户对通道的操作权限，最大128个通道，0-无权限，1-有权限
	BYTE dwRemoteRight[32]; 	//远程登陆用户所具备的权限 1.数组0未使用；2.取值：0-无权限，1-有权限
	/*数组 1: 远程预览*/ /*数组 2: 参数设置*/ /*数组 3: 远程回放*/ /*数组 4: 远程备份*/ /*数组 5: 查看日志*/ /*数组 6: 语音对讲*/ /*数组 7: 远程升级*/ /*数组 8：远程重启*/
	BYTE RemoteChannel[128]; 	//用户远程登陆时对通道所具备的权限，最大128个通道，0-无权限，1-有权限
	DWORD dwUserIP;				//用户登录时pc机的ip地址，为0表示任何PC机都可以使用该用户登陆到DVR上，不为0表示只有ip地址为设定值的pc机才可以使用该用户登录到DVR上
	BYTE byMACAddr[8]; 			//用户登录时PC机的MAC地址，为0表示任何PC机都可以使用该用户登陆到DVR上，不为0表示只有MAC地址为设定值的PC机才可以使用该用户登陆到DVR上
}STRUCT_USERINFO_GUI, *LPSTRUCT_USERINFO_GUI;


//用户信息扩展1
//结构体数据项3
//typedef struct 
//{
//	BYTE grp_name[32]; //分组名
//	ULONGLONG local_authority[64]; // 本地用户使用权限，每位代表一个通道,bit0~bit63表示0~63通道，每个数组代表一种权限，
//	/*数组0：实时预览*/
//	/*数组1：手动录像*/
//	/*数组2：录像查询回放*/
//	/*数组3：备份管理*/
//	/*数组4：录像参数*/
//	/*数组5：云台设置*/
//	/*数组6：截图设置*/
//	/*数组7：通道设置*/
//	/*数组8：定时录像*/
//	/*数组9：移动检测*/
//	/*数组10：报警管理*/
//	/*数组11：常规设置*/
//	/*数组12：串口设置*/
//	/*数组13：磁盘设置*/
//	/*数组14：网络设置*/
//	/*数组15：信息查看*/
//	/*数组16：升级管理*/
//	/*数组17：快速设置*/
//	/*数组18：出厂设置*/
//	/*数组19：系统关机*/
//	/*数组20：卡号录像*/
//	ULONGLONG remote_authority[64];//远程权限，每位代表一个通道，bit0~bit63表示0~63通道，每个数组代表一种权限，
//	/*数组0：远程预览*/
//	/*数组1：参数设置*/
//	/*数组2：远程回放*/
//	/*数组3：远程备份*/
//	/*数组4：查看日志*/
//	/*数组5：语音对讲*/
//	/*数组6：远程升级*/
//}STRUCT_USERINFO_9000, *LPSTRUCT_USERINFO_9000;

typedef struct 
{
	BYTE user[32];				//用户名 以’\0’结束字符串
	BYTE pwd[32];				//密码 以’\0’结束字符串
	BYTE grp_name[32]; 			//分组名
	ULONGLONG local_authority[64]; 		// 本地用户使用权限，每位代表一个通道,bit0~bit63表示0~63通道，每个数组代表一种权限，
	/*数组0：实时预览*//*数组1：手动录像*//*数组2：录像查询回放*//*数组3：备份管理*//*数组4：录像参数*//*数组5：云台设置*//*数组6：截图设置*//*数组7：通道设置*//*数组8：定时录像*//*数组9：移动检测*//*数组10：报警管理*/
	/*数组11：常规设置*//*数组12：串口设置*//*数组13：磁盘设置*//*数组14：网络设置*//*数组15：信息查看*//*数组16：升级管理*//*数组17：快速设置*//*数组18：出厂设置*//*数组19：系统关机*//*数组20：卡号录像*/
	/*数组21: 录像查询 */ /* 数组22: 录像回放 */ /* 数组23: 录像删除 */ 
	ULONGLONG remote_authority[64];   	//远程权限，每位代表一个通道，bit0~bit63表示0~63通道，每个数组代表一种权限，/*数组0：远程预览*//*数组1：参数设置*//*数组2：远程回放*//*数组3：远程备份*//*数组4：查看日志*//*数组5：语音对讲*//*数组6：远程升级*/ 
	DWORD bind_ipaddr; 
	BYTE  bind_macaddr[8];
}STRUCT_USERINFO_9000, *LPSTRUCT_USERINFO_9000;

//用户信息扩展1
//typedef struct
//{
//	DWORD dwSize;//结构体大小
//	WORD dwUserInfoMode;//用户权限模式，1-老的权限模式，2-新GUI权限模式，3-9000项目权限模式
//	BYTE reserve[2];//保留
//	BYTE user[32];//用户名
//	BYTE pwd[32];//密码
//	DWORD bind_ipaddr; //用户登录时pc机的ip地址，为0表示任何PC机都可以使用该用户登陆到DVR上，不为0表示只有ip地址为设定值的pc机才可以使用该用户登录到DVR上
//	BYTE  bind_macaddr[8];//用户登录时PC机的MAC地址，为0表示任何PC机都可以使用该用户登陆到DVR上，不为0表示只有MAC地址为设定值的PC机才可以使用该用户登陆到DVR上
//	union
//	{
//		STRUCT_USERINFO userInfo;//当dwUserInfoMode=1时，使用该结构体
//		STRUCT_USERINFO_GUI userInfoGui;//当dwUserInfoMode=2时，使用该结构体
//		STRUCT_USERINFO_9000 userInfo9000;//当dwUserInfoMode=3时，使用该结构体
//	}info;
//}STRUCT_SDVR_USER_INFO_EX1, *LPSTRUCT_SDVR_USER_INFO_EX1;

typedef struct
{
	DWORD dwSize;
	WORD dwUserInfoMode;  //用户权限模式，1-老的权限模式，2-新GUI权限模式，3-9000项目权限模式
	BYTE right_ver;		/* 用户权限版本标志: 0-旧版本，1-新版本v1(将录像查询回放权限删除，分别增加录像查询，回放，删除三个权限 */
	BYTE reserve;      //保留
	union 
	{
		STRUCT_USERINFO userInfo[MAX_USERNUM];			//当dwUserInfoMode=1时，使用该结构体 
		STRUCT_USERINFO_GUI userInfoGui[MAX_USERNUM];	//当dwUserInfoMode=2时，使用该结构体 
		STRUCT_USERINFO_9000 userInfo9000[MAX_USERNUM]; //当dwUserInfoMode=3时，使用该结构体 
	}info;
}STRUCT_SDVR_USER_INFO_EX1,*LPSTRUCT_SDVR_USER_INFO_EX1;

//获取主机通道参数支持范围
//码流类型
typedef struct { 
	BYTE dwBitrateTypeIndex; //当前码流类型索引值，表示基于dwBitrateType中的位置 
	//例如dwBitrateTypeIndex = 0，则当前码流类型为dwBitrateType的第0位所指定的码流类型，即变码流。
	BYTE dwBitrateType; //支持的码流类型，每一位代表一种码流类型，该位为1表示支持该码流类型，从低位开始，第0位是变码流，第1位是定码流
} STRUCT_SDVR_BITRATETYPE, *LPSTRUCT_SDVR_BITRATETYPE;

//获取主机通道参数支持范围
//图片质量
typedef struct { 
	BYTE dwPicQualityIndex; //当前图像质量索引值，表示基于dwPicQuality中的位置 
	//例如dwPicQualityIndex = 0，则当前图像质量为dwBitrateType的第0位所指定的图像质量，即变码流。
	BYTE dwPicQuality; //支持的图像质量，每一位代表一种图像质量，该位为1表示支持该图像质量，从低位开始，第0位是最高，第1位是较高，第2位是高，第3位是中，第4位是低，第5位是最低
} STRUCT_SDVR_PICQUALITY, *LPSTRUCT_SDVR_PICQUALITY;

//获取主机通道参数支持范围
//视频码率
typedef struct { 
	DWORD dwVideoBitrateIndex; //当前码率索引值，表示基于dwVideoBitrate中的位置 
	//例如dwVideoBitrateIndex = 0，则当前码率为dwVideoBitrate[0]中所指定的码率值。
	DWORD dwVideoBitrate[32];//支持的码率，每一个数组代表一种码率，数组的值如果为0，表示该数组未用到，不为0，表示支持的码率值，单位为Kbit/s
} STRUCT_SDVR_VIDEOBITRATE, *LPSTRUCT_SDVR_VIDEOBITRATE;

//获取主机通道参数支持范围
//帧率
typedef struct { 
	WORD VideoFrameRate;	   //当前帧率值。
	WORD dwVideoFrameRate_min; //支持的最小帧率值
	WORD dwVideoFrameRate_max; //支持的最大帧率值
} STRUCT_SDVR_VIDEOFRAMERATE, *LPSTRUCT_SDVR_VIDEOFRAMERATE;

//获取主机通道参数支持范围
//结构体数据项
typedef struct {
	BYTE byStream_support;//是否支持该码流，即本结构体的值是否有效，1表示有效，0表示无效
	BYTE byAudio_support; //是否支持音频，0-不支持，1-支持 
	BYTE reserve[2]; //保留
	STRUCT_SDVR_RESOLUTION byResolution_support;		//支持的分辨率  
	STRUCT_SDVR_BITRATETYPE byBitrateType_support;		//支持的码流类型
	STRUCT_SDVR_PICQUALITY byPicQuality_support;		//支持的图像质量
	STRUCT_SDVR_VIDEOBITRATE dwVideoBitrate_support;	//支持的码率        
	STRUCT_SDVR_VIDEOFRAMERATE dwVideoFrameRate_support; //支持的帧率
}STRUCT_SDVR_COMPRESSION_SUPPORT, *LPSTRUCT_SDVR_COMPRESSION_SUPPORT;

//获取主机通道参数支持范围
//请求消息结构体
//typedef struct 
//{
//	DWORD dwSize;//结构体大小
//	BYTE byChannel; //通道号 
//	BYTE byRecordType;  //0x0:手动录像，0x1:定时录象，0x2:移动侦测，0x3:报警，0x0f:所有类型 
//	BYTE byCompressionType; //码流，0-主码流，1-字码流1，字码流2……
//	BYTE byReserv;//支持的码流，每位代表一种码流，该位为1表示支持该码流，从低位开始，第0位代表主码流，第1位代表子码流1，第2位代表子码流2，……
//	STRUCT_SDVR_COMPRESSION_SUPPORT struRecordPara;//码流支持的参数
//	BYTE  reserve[2];//保留
//}STRUCT_SDVR_COMPRESSINFO_SUPPORT, *LPSTRUCT_SDVR_COMPRESSINFO_SUPPORT;

typedef struct { 
	DWORD dwResolution; //分辨率值，如：0x07800438，高两字节（0x0780=1920）、低两字节（0x0438=1080）；
	DWORD dwVideoBitrate_support [32];//该分辨率下支持的码率范围，每一个数组代表一种码率，数组的值如果为0，表示该数组未用到，不为0，表示支持的码率值，单位为Kbit/s
	WORD dwVideoFrameRate_min; //该分辨率下的最小帧率
	WORD dwVideoFrameRate_max; //该分辨率下的最大帧率
	BYTE  dwPicQuality_support[10];//该分辨率下支持的图像质量等级,每个数组代表一种图像质量等级，0数组是最高， 1数组是较高， 2数组是高， 3数组是中，4数组是低， 5数组是最低，该数组为1，表示支持该种图像质量
	BYTE  reserve[2];	//保留
} STRUCT_SDVR_RESOLUTIONINFO;

//获取主机通道参数支持范围
//请求消息结构体
typedef struct 
{
	DWORD dwSize;				//结构体大小
	BYTE byChannel; 			//通道号
	BYTE byCompressionType; 	//码流，0-主码流，1-子码流1，子码流2..
	BYTE byCompression_support;	//支持的码流，每位代表一种码流，该位为1表示支持该码流， 从低位开始，0位代表主码流，第1位代表子码流1，第2位代表子码流2，..
	BYTE dwBitrateTypeIndex;	//当前码流类型索引值，表示基于dwBitrateType中的位置,例如dwBitrateTypeIndex = 0，则当前码流类型为dwBitrateType的第0位,所指定的码流类型，即变码流。
	BYTE byBitrateType_support; //支持的码流类型，每一位代表一种码流类型，该位为1表示支持该码流类型，从低位开始，第0位是变码流，第1位是定码流
	BYTE byRecordType_index;	//当前录像类型索引值
	BYTE byRecordType_support;	//支持的录像类型，每位代表一种录像类型，该位为1表示支持该类型，从低位开始，第0位手动录像，第1位定时录像，第2位移动录像，第3位报警录像，……第15位所有录像
	BYTE byAudioflag;			//当前是否有音频，0-无音频，1-有音频
	BYTE byAudio_support;		//是否支持音频，0-不支持，1-支持，当不支持音频时，byAudioflag只能为0
	BYTE dwPicQuality;			//当前图像质量， 0--最高， 1-较高， 2-高， 3-中，4-低， 5-最低
	WORD dwVideoFrameRate;		//当前帧率值
	DWORD dwVideoBitrate;		//当前码率值，单位为Kbit/s
	BYTE  reserve[3];			//保留
	BYTE  dwResoluIndex;		//当前分辨率索引值，表示基于byResolution_support中的位置
	STRUCT_SDVR_RESOLUTIONINFO byResolution_support[16]; //支持的分辨率，最大16种分辨率，每个结构体代表一种分辨率及该分辨率下支持的码率，帧率，图像质量范围，该结构体的dwResolution为0，表示该结构体未用到
}STRUCT_SDVR_COMPRESSINFO_SUPPORT,*LPSTRUCT_SDVR_COMPRESSINFO_SUPPORT;


//NVR日志信息查询 (NVR / IPC)
typedef struct
{
	WORD dwYear;			// 年: 2000~2063
	BYTE dwMonth;			// 月: 1~12
	BYTE dwDay;				// 日: 1~31
	WORD dwStart; 			// 查询从第几条开始，一般为0。
	WORD dwnum;  			// 一次查询个数，最多为100。
	BYTE priType;    		// 主类型 （需扩展所有）
	BYTE secType;  			// 次类型
	BYTE reserve[6];		// 保留
} STRUCT_REQLOG_EX, *LPSTRUCT_REQLOG_EX;

//NVR日志信息查询 (NVR / IPC)返回结构
typedef struct 
{
	DWORD	dwSize;					// 结构体大小
	DWORD	totalLogNum;			// 日志总条数
	DWORD	curLogNum;				// 本次查到的条数
	DWORD	dwStart; 				// 本次查询到的日志的起始编号
	BYTE	encType;				// 编码格式 1- UTF-8  2-gb2312   
	BYTE	reserve[3];				// 保留
	BYTE	sigalLogData[100][128];  // 日志信息 (每次查询最多支持100条日志，日志多于100条
	//时，需要多次调用，每条128字节，每条以‘\0’结束)
} STRUCT_SDVR_LOGINFO_EX, *LPSTRUCT_SDVR_LOGINFO_EX;

//复合（零）通道编码参数配置(扩展)
typedef struct	
{
	BYTE 	byStreamType;		//码流类型	0-无音频 ,1-有音频
	BYTE 	byResolution; 		//分辨率	0-CIF 1-HD1, 2-D1，3-QCIF 4-720p 5-1080p
	BYTE 	byBitrateType;		//码率类型	0:变码率，1:定码率 
	BYTE 	byPicQuality;		//图象质量	1-最好 2-次好 3-较好 4-一般5-较差 6-差
	DWORD 	dwVideoBitrate; 	//视频码率 0-100K 1-128K，2-256K，3-512K，4-1M，5-1.5M，6-2M，7-3M, 8-4M
	//其他：码率值（kbps）有效范围 30~2^32//大于等于30，以K为单位
	short	nFrame;				//帧率：一般情况下为 2-30，表示帧率值，nTimebase值为1
	//当nFrame值为1时，帧率取决于nTimebase的值
	short	nTimebase;     		//当nFrame值为1时，nTimebase值为8，表示帧率为1/8;
	//nTimebase值为16，表示帧率为1/16
	DWORD 	quant;				//量化因子 1-31
}STRUCT_DVR_VENC_CONFIG_EX, *LPSTRUCT_DVR_VENC_CONFIG_EX;

typedef struct 
{
	DWORD 		enable;			//复合通道是否启用，0--不启用，1--启用
	BYTE		chlist[128];	//选择的通道，当enable=1时有效
	STRUCT_DVR_VENC_CONFIG_EX	venc_conf;	 //复合通道的视频参数
	BYTE		format;			//画面格式，0—off，1—1画面，4—2*2画面，9---3*3画面，16---4*4画面。
	BYTE		SwitchTime;		//切换时间
	BYTE		reserve[2];		//保留
}STRUCT_DVR_ZERO_VENC_CONFIG_EX, *LPSTRUCT_DVR_ZERO_VENC_EX;

//夏令时按周设置时间
typedef struct
{
	BYTE	month;		//夏令时按周设置，月[1，12]
	BYTE	weeks;		//夏令时按周设置，周[1，5]
	BYTE	week;		//夏令时按周设置，星期[0，6]
	BYTE	hour;		//夏令时按周设置，时[0，23]
	BYTE	min;		//夏令时按周设置，分[0，59]
	BYTE	sec;		//夏令时按周设置，秒[0，59]
} STRUCT_SDVR_DST_WEEK_TIME_S, *LPSTRUCT_SDVR_DST_WEEK_TIME_S;
//说明：按周设置的时间，表示第几月的第几个星期几的几时几分几秒，如month=5，weeks=2，week=1，hour=10，min=0，sec=0，表示5月份的第2个星期1的10：00：00

//夏令时时间设置
typedef struct
{
	BYTE	dst_en;			//夏令时使能键，0-不使能，1-使能
	BYTE	dsttype_en;		//按周设置为0, 按日期设置为1
	SYSTIME	start_date;		//按日期设置的开始时间
	SYSTIME	end_date;		//按日期设置的结束时间
	STRUCT_SDVR_DST_WEEK_TIME_S	start_time;		//按周设置的开始时间
	STRUCT_SDVR_DST_WEEK_TIME_S	end_time;		//按周设置的结束时间
	BYTE		reserve[4];	//保留
}STRUCT_SDVR_DST_TIME_S, *LPSTRUCT_SDVR_DST_TIME_S;

// 亮度色度相关
typedef struct
{
	BYTE brightness;					// 亮度      取值范围[0，255] 缺省值128
	BYTE constrast;						// 对比度    取值范围[0，255] 缺省值128
	BYTE saturation;					// 饱和度    取值范围[0，255] 缺省值128
	BYTE hue;							// 色度      取值范围[0，255] 缺省值128
	BYTE sharp;							// 锐度      取值范围[0，255]
	DWORD reserved;						// 预留
}VIDEO_INFO, *LPVIDEO_INFO;

//NVR通道参数
typedef struct	
{
	DWORD dwSize; 					// 长度（结构体大小）
	// 通道名相关
	BYTE sChanName[32];				// 通道名 以’\0’结束字符串
	BYTE byChannel; 				// 通道号 [0, n－1] n:通道数
	DWORD dwShowChanName;			// 是否显示通道名 0-显示 1-不显示
	BYTE byOSDAttrib;				// 通道名 1-不透明 2-透明（只针对PC端显示）
	WORD wShowNameTopLeftX;			// 通道名称显示位置的x坐标 左->右 0~视频实际宽度－通道名长度
	WORD wShowNameTopLeftY;			// 通道名称显示位置的y坐标 上->下 0~视频实际高度－字体高度

	// 日期相关
	DWORD dwShowTime;					// 是否显示时间 0-显示 1-不显示
	WORD wOSDTopLeftX;					// 时间osd坐标X [0, 实际宽－时码长度]
	WORD wOSDTopLeftY;					// 时间osd坐标Y[0, 实际高－字体高度]
	BYTE byDateFormat;					// 日期格式
	//  0 - YYYY-MM-DD    （缺省值）
	//  1 - MM-DD-YYYY
	//  2 - YYYY年MM月DD日
	//  3 - MM月DD日YYYY年

	// 星期相关
	BYTE byDispWeek;				    // 是否显示星期 0-显示 1-不显示
	BYTE byOSDLanguage;					// 星期语言 0-中文 1-英文 (可扩展)

	// 亮度色度相关
	VIDEO_INFO  videoinfo;              // 视频信息

	// 遮挡区域相关
	DWORD dwEnableHide;		// 视频遮挡使能 ,0-不遮挡,1-遮挡(遮挡区域全黑) 2-遮挡(遮挡区域马赛克)  
	STRUCT_SDVR_SHELTER	struShelter[16];	// 视频遮挡区域	   
	DWORD dwOsdOverType; 					// osd叠加类型 1-前端叠加 2-后端叠加
	DWORD reserve[32];						// 保留
} STRUCT_NVR_CHN_ATTR_INFO, *LPSTRUCT_NVR_CHN_ATTR_INFO;

typedef struct
{
	BYTE    streamtype;			// 流类型     0-变码流（缺省值） 1-定码流
	BYTE    quality;			// 视频质量   1-最高 2-较高 3-高（缺省值） 4-中 5-低 6-最低
	BYTE    resolution;			// 主码流     0-CIF 1-D1（缺省值） 2-720P 3-1080P
	// 子码流     0-CIF 1-D1(缺省值)
	BYTE    framerate;			// 帧率       取值范围[2,25] 缺省值25
	BYTE    maxbitrate;			//视频码率 0-100K 1-128K，2-256K，3-512K，4-1M，5-1.5M，6-2M，7-3M, 
	//8-4M 其他：码率值（kbps）有效范围 30~2^32，大于等于32，以K为单位
	BYTE    audio;				// 音频标识   0-无音频 1-有音频（缺省值）
	DWORD   reserved;			// 预留
}STRUCT_RECORD_PARAM,  *PSTRUCT_RECORD_PARAM;

typedef struct
{
	BYTE starth;				// 起始时间-时
	BYTE startm;				// 起始时间-分
	BYTE stoph;					// 结束时间-时
	BYTE stopm;					// 结束时间-分
	BYTE recType;				// 录像类型  0 - 无 1-手动(无效)  2-定时  3-移动  4-报警 5-移动 | 报警  6 -移动 & 报警
	BYTE reserve[3];			// 保留
}STRUCT_REC_TIME_PERIOD, *PSTRUCT_REC_TIME_PERIOD;

typedef struct
{
	BYTE Enable;  		// 完整天使能  0-不使能(缺省值) 1-使能
	BYTE recType; 		// 完整天对应的录像类型 0 - 无 1-手动(无效)  2-定时  3-移动  4-报警 5-移动 | 报警  6- 移动 & 报警
	BYTE reserve[2];	// 保留
}STRUCT_FULL_DAY_S;

typedef struct
{
	BYTE enable;				// 使能时间表 0-不使能(缺省值) 1-使能    
	BYTE weekEnable;			// 每天使能位 0-不使能 1-使能(若使能,只取struAlarmTime[0][0~7]对每天做设置)  
	STRUCT_FULL_DAY_S	fullDayEnable[8];			// 完整天录像
	STRUCT_REC_TIME_PERIOD struAlarmTime[8][8]; 	// [0-7][0]代表全天使能的设置项

	DWORD  reserved;                 				// 预留
}STRUCT_REC_TIME_SCHEDULE, *PSTRUCT_REC_TIME_SCHEDULE;

//NVR录像参数（老）
typedef struct
{
	DWORD   dwSize;					// 结构体大小
	BYTE	byChannel;				// 通道号
	WORD    preRecTime;				// 预录时间      取值范围[5,30]秒  缺省值10
	DWORD   delayRecTime;			// 录像持续时间  取值范围[0,180]秒  缺省值30 
	//(对3-移动录像 4-报警录像 5-移动 | 报警  6-移动 & 报警 有效)
	STRUCT_REC_TIME_SCHEDULE timeschedule;		// 录像时间表与录像类型设置

	STRUCT_RECORD_PARAM    timerecord;			// 定时          录像参数
	STRUCT_RECORD_PARAM    moverecord; 			// 移动          录像参数
	STRUCT_RECORD_PARAM    alarmrecord;			// 报警          录像参数
	STRUCT_RECORD_PARAM    moveOrAlarmrecord;	// 移动 | 报警	录像参数 
	STRUCT_RECORD_PARAM    moveAndAlarmrecord;	// 移动 & 报警	录像参数
	STRUCT_RECORD_PARAM    neRecParam[4];     	// 保留

	DWORD  byLinkMode;							// 码流类型 (0-主码流 1-第一子码流 2-第二子码流 ....)
	DWORD  reserved[31];						// 预留
}STRUCT_RECORD_SET_OLD, *PSTRUCT_RECORD_SET_OLD;

//NVR录像参数（新）
typedef struct
{
	DWORD   dwSize;					// 结构体大小
	BYTE	byChannel;				// 通道号
	WORD    preRecTime;				// 预录时间      取值范围[5,30]秒  缺省值10
	DWORD   delayRecTime;			// 录像持续时间  取值范围[0,180]秒  缺省值30 
	//(对3-移动录像 4-报警录像 5-移动 | 报警  6-移动 & 报警 有效)
	STRUCT_REC_TIME_SCHEDULE timeschedule;		// 录像时间表与录像类型设置

	STRUCT_RECORD_PARAM    manurecord;      	// 手动          录像参数
	STRUCT_RECORD_PARAM    timerecord;			// 定时          录像参数
	STRUCT_RECORD_PARAM    moverecord; 			// 移动          录像参数
	STRUCT_RECORD_PARAM    alarmrecord;			// 报警          录像参数
	STRUCT_RECORD_PARAM    moveOrAlarmrecord;	// 移动 | 报警	录像参数 
	STRUCT_RECORD_PARAM    moveAndAlarmrecord;	// 移动 & 报警	录像参数
	STRUCT_RECORD_PARAM    neRecParam[4];     	// 保留

	DWORD  byLinkMode;							// 码流类型 (0-主码流 1-第一子码流 2-第二子码流 ....)
	DWORD  reserved[31];						// 预留
}STRUCT_RECORD_SET, *PSTRUCT_RECORD_SET; 

typedef struct	
{
	BYTE	byStartHour;  			//开始小时 0-23
	BYTE	byStartMin;  			//开始分钟 0-59
	BYTE	byStopHour; 			//结束小时  0-23
	BYTE	byStopMin;  			//结束分钟  0-59
}STRUCT_NVR_SCHEDTIME;		

//移动侦测(NVR)
typedef struct	
{
	DWORD dwSize;				//长度（结构体大小）
	BYTE byChannel;				//通道号 [0, n－1] n:通道数

	BYTE byMotionScope[18][22];	//侦测区域,共有22*18个小宏块,为1表示该宏块是移动侦测区域,0-表示不是 
	BYTE byMotionSensitive;		//移动侦测灵敏度, 0 - 5,越高越灵敏 

	// 时间表相关
	BYTE byEnableHandleMotion;	// 移动侦测布防使能 0-撤防 1-布防	
	BYTE weekEnable;			// 设置每天0-不使能 1-使能(若使能,只取struAlarmTime[0][0~7]对每天做设置)  
	BYTE fullDayEnable[8];		// 完整天录像 0-不使能(缺省值) 1-使能,若此项使能,则对应的天为全天布防,不用判断时间段
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8];		//布防时间段, 8个时间段
	DWORD	dwHandleType;							//按位 2-声音报警5-监视器最大化 //6-邮件上传

	// 联动报警输出
	BYTE	alarmOut_local[16];				//报警输出端口(本地)
	BYTE	alarmOut_remote[128][16];		//报警输出端口(前端设备)

	// 联动录像    
	BYTE record_channel[128];				// 联动的录像通道，为0-不联动 1-联动

	// 联动其他  
	BYTE byEnablePreset[128];			    // 是否调用预置点 仅用byEnablePreset[0]来判断;
	BYTE byPresetNo[128];				    // 调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点 [1, 254]
	DWORD reserve[32];						// 保留
}STRUCT_NVR_MOTION, *LPSTRUCT_NVR_MOTION;

//获取NVR云台协议列表
typedef struct
{
	BYTE  Type; 		//  0-NVR本地云台，1-前端设备云台 (为1时byChannel生效)
	BYTE  byChannel;   	//  [0, n-1],n:通道数
	BYTE  reserve[2];	//	保留
}STRUCT_NVR_PTZLIST, *LPSTRUCT_NVR_PTZLIST;

typedef struct 
{
	DWORD	ptznum;					// 协议个数（限制为最多100个）
	BYTE	reserve[4];				// 保留
	BYTE	ptztype[100][10];		// 协议名列表―――0，unknow;
}STRUCT_NVR_PTZLIST_INFO, *LPSTRUCT_NVR_PTZLIST_INFO;

//报警输入参数（NVR）
typedef struct
{
	DWORD dwSize;						// 结构体大小
	BYTE opType;                        // 0-本地 1-前端 (为1时,byChannel有效)
	BYTE byChannel;                     // 操作前端某通道设备 [0, n-1], n:通道个数
	BYTE byAlarmInPort;				    // 报警输入端口号[0, n-1], n:报警输入个数
	BYTE sAlarmInName[32];			    // 报警输入端口名， 以’\0’结束字符串
	BYTE byAlarmType;				    // 探头类型 0-常闭1-常开
	BYTE byEnSchedule;					// 报警输入布防时间激活 0-屏蔽 1-激活 
	BYTE weekEnable;               		// 每天使能位0-不使能 1-使能(若使能,只取struAlarmTime[0][0~7]来设置每一天) 
	BYTE allDayEnable[8];						// 全天使能 ,0-不使能 1-使能若此项使能,则对应的天为全天布防,不用判断时间段	
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8];	// 布防时间段
	DWORD	dwHandleType;	           			// 按位 2-声音报警 5-监视器最大化 //6-邮件上传

	// 联动报警输出
	BYTE  alarmOut_local[16];		    // 报警输出端口(本地)
	BYTE  alarmOut_remote[128][16];		// 报警输出端口(前端设备)

	// 联动录像
	BYTE byRelRecordChan[128];		    // 报警触发的录象通道,为1表示触发该通道 

	// 联动其他
	BYTE byEnablePreset[128];			// 是否调用预置点 仅用byEnablePreset[0]来判断
	BYTE byPresetNo[128];				// 调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点 [1, 254]
	BYTE reserve[32];			        // 保留
} STRUCT_NVR_ALRMIN, *LPSTRUCT_NVR_ALRMIN;

//报警输出参数（NVR）
typedef struct	
{
	DWORD dwSize;					// 结构体大小
	BYTE opType;                    // 0-本地 1-前端 (为1时,byChannel有效)
	BYTE byChannel;                 // 操作前端某通道设备 [0, n-1], n:通道个数
	BYTE byALarmoutPort;			// 报警输出通道号 [0, n-1], n:报警输出端口数
	BYTE sAlarmOutName[32];			// 名称 以’\0’结束字符串
	DWORD dwAlarmOutDelay;			// 输出保持时间 单位秒 [2, 300]
	BYTE byAlarmType;				// 探头类型 0-常闭1-常开 (保留)    
	BYTE byEnSchedule;				// 报警输出布防时间激活 0-屏蔽 1-激活 
	BYTE weekEnable;				// 每天使能位0-不使能 1-使能(若使能,只取struAlarmTime[0][0~7]对每天做设置)  
	BYTE fullDayEnable[8];     		// 完整天录像 0-不使能(缺省值) 1-使能
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8]; //布防时间段, 8个时间段

	BYTE  reserve[32]; 				// 保留
}STRUCT_NVR_ALARMOUTINFO, *LPSTRUCT_NVR_ALARMOUTINFO;

//获取设备信息扩展(NVR)
typedef struct	
{
	DWORD dwSize;						// 结构体大小
	BYTE sDVRName[32];					// 设备, 以’\0’结束字符串
	DWORD dwDVRID;						// 保留
	DWORD dwRecycleRecord;				// 协议二: //录像覆盖策略 0-循环覆盖 1-提示覆盖
	BYTE sSerialNumber[48];				// 序列号
	BYTE sSoftwareVersion[64];			// 软件版本号以’\0’结束字符串协议二: （主机型号 软件版本号）
	BYTE sSoftwareBuildDate[32];		// 软件生成日期以’\0’结束字符串协议二:（Build 100112）
	DWORD dwDSPSoftwareVersion;			// DSP软件版本
	BYTE sPanelVersion[32];				// 前面板版本，以’\0’结束字符串，IPC无
	BYTE sHardwareVersion[32];	        // (保留)协议二: 当软件版本号超过16字节时会使用作为主机型号显示
	BYTE byAlarmInPortNum;		        // 报警输入个数, NVR只取本地报警输入
	BYTE byAlarmOutPortNum;	  	    	// 报警输出个数, NVR只取本地报警输出
	BYTE byRS232Num;			        // 保留
	BYTE byRS485Num;					// 保留
	BYTE byNetworkPortNum;				// 保留
	BYTE byDiskCtrlNum;					// 保留
	BYTE byDiskNum;						// 硬盘个数
	BYTE byDVRType;						// DVR类型, 1:NVR 2:ATM NVR 3:DVS 4:IPC 5:NVR （建议使用//NET_SDVR_GET_DVRTYPE命令）
	BYTE byChanNum;						// 通道个数[0, 128]
	BYTE byStartChan;					// 保留
	BYTE byDecordChans;					// 保留
	BYTE byVGANum;						// 保留
	BYTE byUSBNum;						// 保留
	BYTE bySDI485Num;					// 保留
	BYTE reserve[2];					// 保留
}STRUCT_SDVR_DEVICEINFO_EX, *LPSTRUCT_SDVR_DEVICEINFO_EX;


//获取主机设备工作状态扩展(NVR)
typedef struct 
{
	BYTE byRecordState;				// 通道是否在录像,0-不录像,1-录像
	BYTE bySignalState;				// 连接的信号状态,0-正常,1-信号丢失
	BYTE byHardwareState;			// 保留
	BYTE byLinkNum; 				// 客户端连接的个数：同一通道当前时间的实时流的连接数。不分主子码流，同一IP多个连接算多个连接
	DWORD dwBitRate;				// 实际码率
}STRUCT_SDVR_CHANNELSTATE_EX, LPSTRUCT_SDVR_CHANNELSTATE_EX;

typedef struct	
{
	DWORD dwSize ; 										// 结构体大小
	STRUCT_SDVR_DISKSTATE  struHardDiskState[16];		// 硬盘状态
	STRUCT_SDVR_CHANNELSTATE_EX struChanState[128];		// 通道的状态
	BYTE alarminStatusLocal[128];						// 本地报警输入端口的状态
	BYTE alarmoutStatusLocal[128];						// 本地报警输出端口的状态
	DWORD reserve[4];									// 保留
}STRUCT_SDVR_WORKSTATE_EX_NVR, *LPSTRUCT_SDVR_WORKSTATE_EX_NVR;

/*******************获取存储设备信息结构体***********************/
typedef enum
{
	NET_RAID_LEVEL_NULL,		 /*未组成raid模式*/    
	NET_RAID_LEVEL_0,            /*RAID 0*/  
	NET_RAID_LEVEL_1,            /*RAID 1*/  
	NET_RAID_LEVEL_2,            /*RAID 2*/  
	NET_RAID_LEVEL_3,            /*RAID 3*/  
	NET_RAID_LEVEL_4,            /*RAID 4*/ 
	NET_RAID_LEVEL_5,            /*RAID 5*/  
	NET_RAID_LEVEL_10            /*RAID 10*/
}NET_Raid_Level_E;

typedef enum
{
	NET_DEV_RAID_NORMAL,            // raid 正常
	NET_DEV_RAID_DEGRED,            // raid 降级
	NET_DEV_RAID_REBUILDING,        // raid 重建
	NET_DEV_RAID_BROKEN,            // raid 损坏
	NET_DEV_RAID_NO_RAID,           // raid 没有建立
	NET_DEV_RAID_UNKOWN             //NO USE STAUS NOW
}NET_RAIDSTATUS_E;

typedef enum
{
	NET_DEVTYPE_NULL,               /* Null device */
	NET_DEVTYPE_IDE,                /* IDE hard or SATA hard disk */ 
	NET_DEVTYPE_RAM,                /* RAM disk */
	NET_DEVTYPE_FLASH,              /* Flash memory */
	NET_DEVTYPE_SD,                 /* SD Card */  
	NET_DEVTYPE_USB,                /* USB host device */  
	NET_DEVTYPE_CDDVD,              /* CDDVD Rom */
	NET_DEVTYPE_ESATA,              /* ESATA device */     
	NET_DEVTYPE_NETWORK,            /* network device */
}NET_DEVTYPE_E;

typedef struct
{
	BYTE	  Name[64];                  //型号
	ULONGLONG dev_capabilty;             //容量                          
//	BYTE      reserve[4];                //保留	                                                                                                                                     
}STRUCT_SDVR_RAIDDEVINFO;

typedef struct
{
	ULONGLONG serial_num;              //主板序列号
	BYTE      rec_port;                //录像通道数4;8;16
	BYTE      rec_format;              //录像分辨率
	BYTE      rec_colorsystem;         //录像制式Ntsc;Pal
	BYTE      rec_standard;            //录像压缩算法H264;MPG4
	MFS_FIELD_TIME sys_time;           //系统最后访问磁盘时间
	BYTE      log_size;                //32M;64M;128M;256M;512M
	BYTE      esata_enable;            //esata设备位置使能
	BYTE      sys_hd_num;              //当前系统支持的最大硬盘数
	BYTE      reserve1[5];             //保留	  
}STRUCT_SDVR_RECORD_SYSINFO;

typedef struct
{
	NET_RAIDSTATUS_E  raid_status;               //设备raid状态
	NET_Raid_Level_E  raid_level;                //设备raid 模式的级别     
	DWORD  raid_dev_table;                       //每个raid组上的设备存在位置情况
	STRUCT_SDVR_RAIDDEVINFO raid_dev_info[10];   //每个磁盘的型号和容量    
//	BYTE reserve[4];							 //保留	                                                                                                                     
}STRUCT_SDVR_RAIDINFO;

typedef struct
{
	WORD dev_num;                    //存储设备挂载通道
	WORD dev_type;                   //设备类型（默认硬盘）, NET _DEVTYPE_E
	DWORD removable;                 //设备是否可移动设置（默认否） 0-否，1-是
	DWORD backup;                    //设备是否备份设置（默认否）  0-否，1-是
	DWORD dev_status;                //设备状态   0-正常，1-磁盘错误，2-文件系统出错
	DWORD is_raid_dev;               //设备是否raid 装置(默认否)   0-否，1-是
	STRUCT_SDVR_RAIDINFO raid_info;  //当前设备raid装置的详细信息
	DWORD active;                    //设备工作状态（休眠或者活动）
	ULONGLONG device_capabilty;      //设备容量(字节) 平台根据各自需要进行相应转换
	DWORD mfs_fs_active;             /*设备文件系统是格式化*/
	ULONGLONG mfs_capability;        //文件系统可用容量（格式化后的容量字节）平//台根据各自需//要进行相应转换
	ULONGLONG mfs_free;              //当前可用容量(字节)平台根据各自需要进行相应转换
	DWORD device_handle;             //设备操作句柄
	STRUCT_SDVR_RECORD_SYSINFO device_sys_info;     //磁盘上录像相关的系统信息 
	BYTE reserve[4];                                //保留	                                                                                                                
}STRUCT_SDVR_DEVICEINF;

typedef struct
{
	DWORD cycle_overlay;                             //循环覆盖方式
	STRUCT_SDVR_DEVICEINF device_info[MAX_DEV_NUM];  //存储设备信息    
	BYTE reserve[4];                                 //保留	                                                                                                           
}STRUCT_SDVR_STORDEVINFO, *LPSTRUCT_SDVR_STORDEVINFO;
/******************************************************************/

/**************************格式化存储设备接头体*****************************/
typedef struct
{
	DWORD dev_num;              //存储设备挂载通道
	DWORD dev_type;             //存储设备类型，NET_DEVTYPE_E
	DWORD reserve;              //保留
}STRUCT_SDVR_FORMAT_DEV;

typedef struct
{
	DWORD format_num;				 //需要格式化设备的个数
	DWORD format_log_flg;            //是否格式化日志标志，0-不格式化，1-格式化
	STRUCT_SDVR_FORMAT_DEV dev_info[MAX_FORMAT_DEV];  //每个要格式化设备的信息                                                                                                                                                         
	BYTE reserve[4];								  //保留	                                                                                 
}STRUCT_SDVR_FORMATINFO;
/****************************************************************************/
//UPNP
typedef struct
{
	BYTE  servicename[32];/*服务名称*/
	int  iport;/*/DVR端口号*/
	int  eport;/*路由器端口*/
	int  proto;/*协议即TCP或者UDP*/
	BYTE  leaseDuration[20];
}STRUCT_SDVR_UPNP_PROTINFO;
typedef struct
{
	BYTE  upnp_en;                 // UPNP 使能
	int   port_num;             // 转换端口信息个数
	STRUCT_SDVR_UPNP_PROTINFO portinfo[MAX_PAT];   // 转换端口信息  MAX_PAT=15，即最多同时可增加15个端口信息
	BYTE  reserve[10];   //保留
}STRUCT_SDVR_UPNPINFO;

//前端通道字符叠加信息参数
typedef   struct 
{ 
	BYTE     id;					// 通道字符叠加信息号 [0, n－1] n: 叠加字符信息组数
	BYTE     byLinkMode;			// 0-主码流  1-子码流  
	BYTE     byChanOSDStrSize;		// 叠加字符信息里字符串数据的长度，包含字符串结束符'\0'
	BYTE     byOSDAttrib;			// 通道字符叠加信息  1-不透明  2-透明(只针对 PC 端显示)
	BYTE     byOSDType;				//格式及语言，最高位为 0 表示解码后叠加，为 1 表示前端叠加
	//设为 0x80 时表示将 osd 设为前端叠加
	char     reservedData[3];
	DWORD	 dwShowChanOSDInfo;		// 是否显示通道字符叠加信息  0-显示  1-不显示 
	WORD     wShowOSDInfoTopLeftX;	// 通道字符叠加信息显示位置的 x 坐标
									// [0,  实际宽－叠加字符数据长度]
	WORD     wShowOSDInfoTopLeftY;	// 通道字符叠加信息显示位置的 y 坐标 
									// [0,  实际高－字体高度] 
	char     data[];				// 叠加字符信息里的字符串数据，包含字符串结束符'\0'
} STRUCT_SDVR_OSDINFO, *LPSTRUCT_SDVR_OSDINFO; 

typedef   struct 
{ 
	BYTE    byChannel;				// 通道号 [0, n－1] n:通道数
	BYTE    byOSDInfoNum;			//包含的叠加字符信息组数，每组结构为STRUCT_SDVR_OSDINFO
	WORD	byChanOSDInfoSize;      // 叠加字符信息的数据包大小
	char    data[];					// 紧跟着叠加字符信息数据内容
} STRUCT_SDVR_OSDCFG, *LPSTRUCT_SDVR_OSDCFG; 



//主动连接升级
typedef   struct 
{ 
	DWORD dwMsgid;
	DWORD dwFileType;
	DWORD dwFileSize;
	DWORD dwFileCRC32;
	DWORD dwReserve[4];
}STRUCT_SDVR_UPDATE, *LPSTRUCT_SDVR_UPDATE; 

//远程查询标记某月的录像记录
typedef struct  
{
	BYTE  byChannel;		//通道号[0, n－1]，n:通道数 
	BYTE  byType;			//查询的录像类型，下表是值和录像类型对应关系 
	BYTE  byYear;			//查询的年  [0-63] 2000 年为 0，2063 年为 63 
	BYTE  byMonth;			//查询的月  [1-12] 
	BYTE  Reserve[32];		//保留  
}STRUCT_SDVR_QUERY_MONTH, *LPSTRUCT_SDVR_QUERY_MONTH;

typedef struct 
{ 
	BYTE  byDate[31];  //返回有录像数据的日期,数组[n]代表某月的第(n+1)天，0-无录像  1-有录像 
	BYTE  Reserve[9];  //保留 
}STRUCT_SDVR_MONTHINFO,  *LPSTRUCT_SDVR_MONTHINFO; 

//获取主机温度
typedef struct 
{ 
	DWORD    mainboard_temperature;			//主板温度 
	DWORD    chip_temperature;						//芯片温度 
	DWORD    harddisk_temperature[MAX_DISKNUM]; //硬盘温度 
	DWORD    cpu_temperature;					//CPU 温度 
	DWORD    humidity;								//湿度 
	BYTE			reserve[4];								//保留 
}STRUCT_SDVR_TEMPERATURE, *LPSTRUCT_SDVR_TEMPERATURE; 

//获取录像统计
typedef struct
{
	DWORD totalDateNumber_record;        		 //录像总天数，硬盘上有录像的所有天数总和
	UNMFS_TIME beginDate_record;		//硬盘上记录的最早开始录像的时间
	UNMFS_TIME endDate_record;			//硬盘上记录的最晚结束录像的时间
	DWORD type_record;					//录像类型，0x01-手动录像,0x02-定时录像,0x04-移动录像,0x08-报警录像,0xff-所有录像
	unsigned long long totalTime;		//总时间，硬盘上所有录像的时间总和，以秒为单位统计
	unsigned long long totalFileLength;		//总文件长度，硬盘上所有录像文件的总长度，按字节表示
	BYTE reserve[4];				//保留
}STRUCT_SDVR_RECORDSTATISTICS, *LPSTRUCT_SDVR_RECORDSTATISTICS;   

//设置覆盖录像天数的结构体
typedef struct{
	unsigned short notEnoughDate;	 //指定硬盘上覆盖录像时，录像不足天数报警的天数值
	BYTE reserve[2];           				 //保留                            
}STRUCT_SDVR_RECORDNOTENOUGHDATE_ALARM;

typedef struct 
{ 
	DWORD    dwAlarmDayNum;     //预设报警天数 
	DWORD    dwCurDayNum;        // 当前天数 
	BYTE  reserve[8];            //保留 
}STRUCT_SDVR_RECORD_ALARM; 

typedef struct 
{ 
	BYTE  byChannel;			//报警通道号 
	BYTE  reserve[11];			//保留 
}STRUCT_SDVR_ALARM_STAT; 

//报警主动上传
//当报警类型dwAlarmType为0-6时，union等于STRUCT_SDVR_ALARM_STAT结构体，当dwAlarmType=7时，union等于STRUCT_SDVR_RECORD_ALARM结构体
typedef struct 
{ 
	BYTE byAlarmStat;			//报警状态，0—报警结束，1—报警开始 
	BYTE reserve[3];				//保留 
	DWORD dwAlarmType;        //报警类型：0-移动报警，1-探头报警，2-视频丢失报警，3-磁盘错误报警， 
	//4-网络断开报警，5-温度过高，6-视频遮挡报警 7-录像天数不足报警 
	UNMFS_TIME dwAlarmTime;     //报警时间 
	union 
	{ 
		STRUCT_SDVR_ALARM_STAT     stat; 
		STRUCT_SDVR_RECORD_ALARM      rec;  //录像天数不足报警 
	}alarm; 
}STRUCT_SDVR_ALARM_REQ, *LPSTRUCT_SDVR_ALARM_REQ; 

//主机参数文件导出
typedef struct 
{ 
	DWORD   dwFileSize;	//导出文件的大小 
	DWORD   reserve;		//保留 
	char    pFileData[]; // 参数文件数据
} STRUCT_EXPT_REQ, *LPSTRUCT_EXPT_REQ; 

//主机参数文件导入
typedef struct 
{ 
	DWORD    dwFileSize; //导入文件的大小，若为0，则不导入文件，恢复主机默认参数 
	DWORD    reserve;		//保留 
} STRUCT_IMPT_REQ, *LPSTRUCT_IMPT_REQ; 

//远程录像备份(实现断点续传)
typedef struct
{
	BYTE byChannel;			//通道号
	BYTE type;						//备份类型：0-手动；1-定时；2-移动；3-探头报警；0x0f-所有录像
	WORD file_index;			//客户端下载的文件列表的索引号，从0开始
	DWORD file_offset;			//文件偏移大小，刚开始为0
	UNMFS_TIME starttime;  //开始时间
	UNMFS_TIME endtime;   //结束时间
	BYTE streamtype;			//码流类型，0-主码流，1-子码流
	BYTE reserve[31];			//保留
}STRUCT_SDVR_BACKINFO_EX;
/*
//远程录像点播开始
typedef struct TAG_VOD_PARAM 
{ 
	BYTE   byChannel;					// 通道号[0, n-1],n:通道数 
	BYTE   byType;						// 录像类型: 1-手动，-定时，-移动，-报警，xFF-全部 
	WORD    wLoadMode;           // 回放下载模式 1-按时间，2-按名字 
	union 
	{ 
		struct 
		{ 
			UNMFS_TIME   struStartTime;	 // 最多一个自然天 
			UNMFS_TIME   struStopTime;  // 结束时间最多到23:59:59, 
			// 即表示从开始时间开始一直播放 
			char cReserve[16]; 
		}byTime; 

		BYTE   byFile[64];            // 是否够长？ 
	}mode; 

	BYTE   streamtype;                 //码流类型，0-主码流，1-子码流 
	BYTE   byReserve[15];             //保留 
}VOD_PARAM, *LPVOD_PARAM; 

typedef struct TAG_VOD_ANS 
{ 
	DWORD    dwVodID;		//主机分配 
	BYTE   streamtype;			//码流类型，0-主码流，1-子码流 
	BYTE   byReserve[15];		//保留 
}VOD_ANSWER, *LPVOD_ANSWER; 
*/

//远程录像文件备份文件头标志消息(扩展) ，断点续传命令使用该标志
typedef struct 
{ 
	DWORD   port;        //通道号 
	DWORD   filelen;     //本文件长度 以B 为单位 
	UNMFS_TIME  filestarttime; //本录像文件开始时间 
	UNMFS_TIME  fileendtime; //本录像文件结束时间 
	DWORD   totalFileLen; //总文件长度，表示NETCOM_BACKUP_RECFILE_REQ_EX 命令所给的 
	//时间段内的总长度(注：该时间段内有可能有多个文件) ，以KB 为单位 
	WORD   file_index;   //文件列表的索引号 
	DWORD   file_offset; //文件偏移大小，发送本文件的起始偏移大小 
}STRUCT_SDVR_FILEINFO_EX,*LPSTRUCT_SDVR_FILEINFO_EX; 

//获取主机端支持的功能
typedef struct 
{ 
	BYTE funtions[256]; //请求的功能，数组的每个元素代表一种功能，元素的值为0-表示不支持该功能，为1-表示支持该功能，下标从0开始。
	//元素1-断点续传功能,元素2-远程截图功能，如果支持则0xE0，0xE1，0xE2，0xE3，0xE4，0xE5命令有效，元素3-视频制式却换不重启功能，元素4-扩展OSD功能，元素5-剩余帧率计算
	//（说明：有些机型性能有限，会限制总帧率，这样设置帧率时会考虑设置的帧率是否在剩余帧率范围内），元素6-双码流回放功能
	BYTE reserve1[8];	//保留
} STRUCT_SDVR_SUPPORT_FUNC, *LPSTRUCT_SDVR_SUPPORT_FUNC;

//通道卡号录像状态
typedef struct 
{ 
	BYTE   channel[128];	//通道卡号录像状态 1.进行卡号录像 0.停止卡号录像 
	BYTE   reserve[4];		//保留 
}STRUCT_SDVR_CHAN_CARDCOR, *LPSTRUCT_SDVR_CHAN_CARDCOR;

//设置对讲接收数据格式
typedef enum 
{ 
	NET_VOICE_DECODE_G711 = 0, 
	NET_VOICE_DECODE_G722, 
	NET_VOICE_DECODE_PCM, 
	NET_VOICE_DECODE_ADPCM, 
	NET_VOICE_DECODE_G726, 
	NET_VOICE_DECODE_G721, 
	NET_VOICE_DECODE_G723, 
	NET_VOICE_DECODE_DPCM, 
	NET_VOICE_DECODE_MAX 
}NET_VOICE_DECODE_PARAM; 

#define HB_TEST_MAX_HD          32           //测试最大的磁盘数
#define HB_TEST_LOSTINFO        5            //记录丢帧信息个数
#define HB_TEST_DEVNAME       64           //设备型号最大长度

typedef struct
{
	WORD year ;       //年 如2009
	WORD month ;      //月 1-12
	BYTE   day ;       //日 1-31
	BYTE   hour ;      //小时 0-23
	BYTE   minute ;    //分钟 0-59
	BYTE   second ;    //秒 0-59
	DWORD msec;        //毫秒
} PT_TIME_S, *PPT_TIME_S;

typedef struct
{
	DWORD          enable;                     //是否执行此项测试使能，0-不执行，1-执行
	DWORD          times;                      //暂时不用， 重启计数由客户端来控制
} HB_TEST_REBOOT_IN_S;

typedef struct
{
	DWORD          flag;                       //重启成功还是失败标志，0为失败，1为成功, 2为未做测试
	DWORD          reserve;
} HB_TEST_REBOOT_OUT_S;

typedef struct
{
	BYTE           flag;                       //磁盘检测状态，0为未检测到磁盘，1为检测到磁盘
	BYTE           dev_no;                     //设备号(和物理号对应，从0开始)
	BYTE           devtype;                    //设备类型
	BYTE           reserve[5];
	BYTE           dev_model[HB_TEST_DEVNAME]; //设备型号
	DWORDLONG          dev_capability;             //设备容量(字节)
} HB_DEVINFO_OUT_S;

typedef struct
{
	BYTE           dev_no;                     //设备号(和物理号对应，从0开始)
	BYTE           devtype;                    //设备类型
	WORD          reserve;
} HB_DEVINFO_IN_S;

typedef struct
{
	DWORD          enable;                     //是否执行此项测试使能，0-不执行，1-执行
	DWORD          dev_num;                    //输入参数，设置待检测设备个数，和devinfo对应
	HB_DEVINFO_IN_S devinfo[HB_TEST_MAX_HD];    //输入参数(可选，按位表示)，有值则表示告知实际上哪几个口接了硬盘，作为测试程序对比使用，没值则测试程序不做比较，直接输出检测结果
} HB_TEST_DISK_IN_S;

typedef struct
{
	BYTE          test_flag;                   //输出参数，表示测试状态，0-没有执行测试(下面数据此时无效)，1-有执行测试(下面数据此时有效)
	BYTE           dev_nun;                    //输出参数，0-未检测到磁盘，>0检测磁盘个数，具体信息存放在devinfo中
	BYTE           reserve[2];
	HB_DEVINFO_OUT_S    devinfo[HB_TEST_MAX_HD];    //输出参数，存储具体每个磁盘的信息，当dev_nun非0时有效
} HB_TEST_DISK_OUT_S;


typedef struct
{
	DWORD          enable;                     //是否执行此项测试使能，0-不执行，1-执行
	DWORD          ch_mask_in[2];              //输入参数(可选，按位表示)，测试录像通道，作为输入参数(非0)，告知主机测试哪些通道，否则默认按所有通道检测
} HB_TEST_REC_IN_S;

typedef struct
{
	BYTE           result;                     //输出参数，对应通道录像检测结果, 1为成功，0为失败, 2为未做测试 
	DWORD          delay_time;                 //输出参数，记录录像开启操作的延迟时间(主要是开启录像到实际写数据所间隔的时间)，只记录所有通道中最大时间    
}HB_TEST_REC_CHINFO;

typedef struct
{
	HB_TEST_REC_CHINFO  ch_check_info[MAX_CH];  //输出参数，各通道录像检测结果信息
} HB_TEST_REC_OUT_S;

typedef struct
{
	WORD          format_devno;               //格式化磁盘号
	WORD          format_devtype;             //格式化磁盘类型(PT_DEVTYPE_E)
} HB_FORMAT_ININFO_S;

typedef struct
{
	WORD            format_devno;               //格式化磁盘号
	WORD			format_devtype;             //格式化磁盘类型
	WORD            format_time;                //格式化所需时间，单位为秒
	DWORD          format_flag;                //格式化成功或失败，0-失败，1-成功，2-未检测到磁盘
} HB_FORMATINFO_S;

typedef struct
{
	WORD          enable;                     //是否执行此项测试使能，0-不执行，1-执行
	WORD          format_num;                  //需要格式化磁盘个数
	HB_FORMAT_ININFO_S formatinfo[HB_TEST_MAX_HD];//需要格式化磁盘信息，format_num非0时有效
	DWORD          format_times;               //执行格式化测试次数，没有选择重启测试时有效，否则默认只执行一次
} HB_TEST_FORMAT_IN_S;

typedef struct
{
	BYTE          test_flag;                   //输出参数，表示测试状态，0-没有执行测试(下面数据此时无效)，1-有执行测试(下面数据此时有效)
	BYTE          format_num;                  //输出参数，已格式化硬盘个数
	BYTE          reserve[2];
	HB_FORMATINFO_S formatinfo[HB_TEST_MAX_HD]; //输出参数，每个硬盘格式化信息，包括成功与失败及每个硬盘格式化时间
} HB_TEST_FORMAT_OUT_S;

typedef struct
{
	DWORD          lost_framecount;            //丢失帧总数，和实际编码帧率的差值
	DWORD          lost_fcount;                //丢失帧号数，大于0表示丢失的一个帧号(即帧号不连续)
	PT_TIME_S       lost_time;                  //丢失帧数的时间(精确到秒)
} HB_RECDATA_INFO_S;

typedef struct
{
	DWORD          enable;                     //是否执行此项测试使能，0-不执行，1-执行
	DWORD          channel;                    //输入参数(可选)，指定测试通道，告知主机测试哪些通道(非0)，否则默认第一通道
	DWORD          frame_deviation;            //输入参数，设置丢帧的合理偏差范围，如果没有设置，默认偏差为2，即如果丢帧不超过2帧，认为正常
	PT_TIME_S       rec_time_start;             //输入参数，要分析录像数据段的起始时间
	PT_TIME_S       rec_time_stop;              //输入参数，要分析录像数据段的结束时间
} HB_TEST_REC_PB_IN_S;

typedef struct
{
	DWORD          test_flag;                  //输出参数，表示测试状态，0-没有执行测试(下面数据此时无效)，1-有执行测试(下面数据此时有效)
	DWORD          check_result;               //输出参数，为0表示没有丢帧，>0表示发现丢帧总数
	DWORD          frame_venc;                //编码帧率
	DWORD          a_framecount;             //音频帧数
	DWORD          v_framecount;             //视频帧数
	DWORD          i_framecount;              //视频I帧数
	HB_RECDATA_INFO_S lost_info[HB_TEST_LOSTINFO];//输出参数，输出发生丢帧的时间点及丢失的帧数，最多记录HB_TEST_LOSTINFO个
} HB_TEST_REC_PB_OUT_S;

typedef struct
{
	HB_TEST_REBOOT_IN_S    reboot_para;         //如果重启测试由客户端来控制，则此项基本无效
	HB_TEST_DISK_IN_S      disk_para;           //硬盘识别测试相关参数
	HB_TEST_REC_IN_S       record_para;         //录像开启、关闭测试相关参数(默认手动录像)，以实际是否写数据为准
	HB_TEST_FORMAT_IN_S    format_para;         //硬盘格式化测试相关
	HB_TEST_REC_PB_IN_S    analyse_para;        //录像数据帧分析相关参数

} HB_TEST_DISK_AUTO_IN_S;

typedef struct
{
	HB_TEST_REBOOT_OUT_S    reboot_test;        //如果重启测试由客户端来控制，则此项基本无效
	HB_TEST_DISK_OUT_S      disk_test;          //硬盘识别测试信息反馈测试
	HB_TEST_REC_OUT_S       record_test;        //录像开启、关闭测试信息反馈
	HB_TEST_FORMAT_OUT_S    format_test;        //硬盘格式化测试结果反馈
	HB_TEST_REC_PB_OUT_S    data_analyse_test;  //录像数据帧分析结果反馈
} HB_TEST_DISK_AUTO_OUT_S;

typedef struct
{
	DWORD ptz_type;           //云台协议类型
	DWORD ptz_baudrate;    //波特率
	DWORD reserve[4];        //保留字段
} PTZ_PARAM, *PPTZ_PARAM;

typedef struct
{
	BYTE stop_flag;               //停止位 1停止 0运行
	BYTE show_split_time;         //显示画面分割时间
	BYTE show_page_time;          //显示页面时间
	BYTE switch_screen_time;      //切屏时间
	BYTE playback_channel[64];    //回放通道
	DWORD playback_time;          //回放时间
	DWORD alarm_out_time;         //报警输出时间
	MFS_FIELD_TIME sys_time;         //系统时间
	PTZ_PARAM ptz_param;           //设置云台参数
	DWORD show_disk_info_time;     //显示磁盘信息管理界面时间+
	DWORD show_logo_time;          //显示logo的时间+
	DWORD show_system_info_time;   //显示系统信息的时间+
	DWORD reserve[5];             //保留字段
} PT_AUTO_TEST, *PPT_AUTO_TEST;    //生产自动化测试

typedef struct
{
	BYTE   test_type;                          //设置测试类型(HB_TEST_TYPE_E)，与union成员对应
	BYTE   reserve[15];

	/*按测试类型分别取下面的结构，每次只能对应一个*/
	union
	{
		PT_AUTO_TEST      product_test_para;    //生产线自动化测试参数
		HB_TEST_DISK_AUTO_IN_S disk_test_para;    //磁盘识别自动化测试参数
	};

	DWORD  reserve2[4];
} HB_TEST_AUTO_IN_S, *LPHB_TEST_AUTO_IN_S;

typedef struct
{
	BYTE   test_type;                          //上传测试类型(HB_TEST_TYPE_E)，与union成员对应
	BYTE   reserve[15];

	/*按测试类型分别取下面的结构，每次只能对应一个*/
	union
	{
		HB_TEST_DISK_AUTO_OUT_S  disk_test_out;  //磁盘识别自动化测试结果输出
	};

	DWORD  reserve2[4];
} HB_TEST_AUTO_OUT_S, *LPHB_TEST_AUTO_OUT_S;


//IPC无线参数
typedef struct
{
	BYTE safeoption;		//安全选项设置，取值范围[0,2]  0:自动选择  1：开放系统   2：共享密钥
	BYTE pswformat;		//密钥格式设置，取值范围[0,1]  0：16进制   1：ASCII码
	BYTE pswtype;			//密 钥 类 型设置，取值范围[0,3]   0：禁用  1：64位  2:128位   3:152位
	BYTE pswword[62];		//密码，以’\0’结尾，定义62byte是为了与STRUCT_SDVR_IPCWPAPSK等大小。
	//备注：密码长度说明，选择64位密钥需输入16进制数字符10个，或者ASCII码字符
	//5个。选择128位密钥需输入16进制数字符26个，或者ASCII码字符13个。
	//选择152位密钥需输入16进制数字符32个，或者ASCII码字符16个。
	BYTE reserve[3];		//保留
}STRUCT_SDVR_IPCWEP,*LPSTRUCT_SDVR_IPCWEP;

typedef struct
{
	BYTE safeoption;		//安全选项设置，取值范围[0,2] 0：自动选择   1：WPA-PSK    2:WPA2-PSK
	BYTE pswmod;			//加密方法设置,取值范围[0,2]  0：自动选择   1：TKIP   2:AES
	BYTE pawword[64];	//psk密码，8到63个字符，以’\0’结尾
	BYTE reserve[2];			//保留
}STRUCT_SDVR_IPCWPAPSK,*LPSTRUCT_SDVR_IPCWPAPSK;

typedef struct
{
	DWORD nSize;			//建议添加，结构体长度。
	BYTE ssid[50];			//SSID号以’\0’结尾
	BYTE wirelessIP[16];	//无线ip以’\0’结尾
	BYTE safetype; 			//安全类型设置， 0：WEB、1：WPA-PSK/WPA2-PSK、2：无加密
	BYTE reserve[3];			//保留  	
	union{
	//因为以下两个结构体不可能同时使用，建议用联合体。
	STRUCT_SDVR_IPCWEP ipcwep;			//安全类型为WEP时参数结构体
	STRUCT_SDVR_IPCWPAPSK ipcwpapsk;		//安全类型为WPA-PSK/WPA2-PSK时参数结构体
	}u;
}STRUCT_SDVR_IPCWIRELESS,*LPSTRUCT_SDVR_IPCWIRELESS;

typedef struct                           //key文件信息
{
	char   internal_ver[16]; //内部版本号
	char   external_ver7004t[16]; //外部版本号7004t
	char   external_ver7008t[16]; //外部版本号7008t
	char   external_ver7016t[16]; //外部版本号7016t
	char   external_ver8004t[16]; //外部版本号8004t
	char   external_ver8008t[16]; //外部版本号8008t
	char   external_ver8016t[16]; //外部版本号8016t
	char   external_ver700xt[16]; //外部版本号700xt
	char   external_ver800xt[16]; //外部版本号800xt
	unsigned int lang_ver;     //语言版本  0：中文；1：英文
	unsigned int logo_ver;     //logo版本 0：汉邦；1：中性
	unsigned int lang_maskl;  //支持的语言掩码低32 位代表32种语言
	unsigned int lang_maskh;  //支持的语言掩码高32 位代表另外32种语言
	unsigned short oem_type;  //OEM 类型，用来区分不同的客户
	char   full_pb_flag;      //是否支持全回放 0不支持,1支持
	char   reserve[21];
}key_field ;

typedef  struct                             //logo信息
{
	char      file_path[256];             //带完整路径的图片名称
	unsigned  int   logo_support;          //是否支持：1-支持, 0-不支持
	unsigned  int   file_format;          //bit:0-支持的图片格式为jpg ,1-支持的图片格式为bmp , 2-支持的图片格式为YUV
	unsigned  int   max_width;            //图片最大宽;
	unsigned  int   max_height;           //图片最大高;   
	unsigned  int   min_width;            //图片最大宽
	unsigned  int   min_height;           //图片最小高
	unsigned  int   logo_bitdepth;        //图片位深度，例：位深为8，第7位为1，位深为12，第11位为1，依次类推
	unsigned  int   logo_size;            //图片大小;
	unsigned  int   reserver;             //保留;  
}LOGO;

typedef struct
{
	char       dev_name[64];                //机器机型名称
	char       alias_name[64];              //修改后机型名称
	key_field  key_info;                    //key文件信息
	char       key_path[256];               //包含key, devinfo.ini两个文件
	LOGO       logo[10];                    //logo信息  0-uboot 1-状态 2-无视频信号 3-水印 4-osd右上角 5-IE  
	unsigned int reserve;            //保留信息
} LOGO_UPDATE_INFO, *LPLOGO_UPDATE_INFO;

typedef struct 
{ 
	int local_ch;     //所属本地通道号 
	int dev_type;     //设备类型，0-IPC ，1-NVR 
	int dev_chnum;    //设备通道数，目前大多数情况下为1，表示该IP 设备只有一个通道 
	int dev_ch;       //设备通道，当dev_chnum 大于1 时，该值表示选择该IP 设备的哪个通道， 
	//后续IP 设备通道的系列动作如实时流、通道参数等都是针对IP 设备的此通道而言的。 
	int proto_type;   //设备协议类型选择，0-HB,1-ONVIF 
	int port;         //端口号，HB 协议表示命令端口，ONVIF 表示HTTP 端口 
	char ip[128];     //设备IP 
	char usr_name[32]; //用户名 
	char pass_wd[32]; //密码 
	int stream_statue; //IP 设备通道流状态，0-断开，1-连接。获取时有用，添加时该值不使用 
	char reserve[4];  //保留 
}STRUCT_SDVR_IPDEV_INFO; 

//添加IP设备
typedef struct 
{ 
	int add_num;              //添加IP 设备总数 
	STRUCT_SDVR_IPDEV_INFO    add_info[64];  //添加的IP 设备信息，64 表示最大可添加的IP 设备数。 
	//该数组值由add_num 决定，如add_num 为8，则该数组只有前8 个值有效， 
	//后面的都是无效值，前8 个值每一个表示一个IP 设备 
	//char reserve[4];          //保留
	char edit_flag;//0-非编辑模式添加，1-编辑模式添加 
	char reserve[3];
}STRUCT_SDVR_ADD_IPDEV, *LPSTRUCT_SDVR_ADD_IPDEV; 

//删除IP设备
typedef struct 
{ 
	int del_num;      //要删除的IP 设备总数 
	int local_ch[64]; //要删除的的IP 设备的本地通道号信息，64 表示最大可添加的IP 设备数。 
	//该数组值由del_num 决定，如del_num 为8，则该数组只有前8 个值有效， 
	//后面的都是无效值，前8 个值每一个表示一个IP 设备 
	char reserve[4];  //保留 
}STRUCT_SDVR_DEL_IPDEV, *LPSTRUCT_SDVR_DEL_IPDEV;

//所有添加的IP设备信息
typedef struct 
{ 
	int total_num; //添加的IP 通道总数 
	STRUCT_SDVR_IPDEV_INFO   ipdev[64]; //IP 通道号信息，64 表示最大可添加的IP 通道数。 
	//该数组值由total_num 决定，如total_num 为16，则该数组只有 
	//前16 个值有效，后面的都是无效值， 
	//前16 个值每一个表示一个IP 通道号 
	char reserve[4]; //保留 
}STRUCT_SDVR_ALLIPCH_INFO, *LPSTRUCT_SDVR_ALLIPCH_INFO; 

//添加的某个IP设备信息
typedef struct 
{ 
	STRUCT_SDVR_IPDEV_INFO     ipdev; //具体信息参数 
	char reserve[4]; //保留 
}STRUCT_SDVR_IPCH_INFO, *LPSTRUCT_SDVR_IPCH_INFO; 

//IP设备网络参数
typedef struct 
{ 
	int local_ch;                  //本地通道号 
	STRUCT_SDVR_NETINFO   netinfo; //具体参数，STRUCT_SDVR_NETINFO 为协议已有结构体 
	char reserve[4];                //保留 
}STRUCT_SDVR_SET_IPDEV_NETPARAM, *LPSTRUCT_SDVR_SET_IPDEV_NETPARAM; 

//IP设备系统时间
typedef struct 
{ 
	int local_ch;            //本地通道号 
	STRUCT_SDVR_TIME  time; //具体时间，STRUCT_SDVR_TIME 为协议中已有结构体 
	char reserve[4];         //保留 
}STRUCT_SDVR_SET_IPDEV_TIME, *LPSTRUCT_SDVR_SET_IPDEV_TIME; 

//本地通道启用状态
typedef struct 
{ 
	char localch_status[256]; //本地通道状态，是否启用，0-未启用（禁用），1-启用,2-IPC（表示已接IP设备,该值只能用于获取，不能用于设置）。256 为最大通道数 
	char reserve[4];     //保留 
}STRUCT_SDVR_LOCH_STATUE, *LPSTRUCT_SDVR_LOCH_STATUE; 

//测试类型参数
typedef enum
{
	HBTEST_CMD_TEST_START = CMDNO(0 , COMP_TEST),//开始自动化测试，对应参数HB_TEST_AUTO_IN_S
	HBTEST_CMD_TEST_STOP,                       //停止测试, 对应参数HB_TEST_AUTO_IN_S, 结构体中的test_type是有效参数
	HBTEST_CMD_REGIST_CALLBACK,                 //注册回调(接收测试结果反馈,输出结果为HB_TEST_AUTO_OUT_S),channel为注册类型(暂时无效),回调函数类型为PNETAPICALLBACK
	HBTEST_CMD_DATA_CALLBACK,                   //数据回调命令，暂时没有意义

} HB_TEST_CMD_E;

typedef struct
{
	char support_capability[64];   //设备支持能力(与操作命令一一对应,64为支持的最大命令数) 
	char support_protocol[3]; //设备协议支持，8为支持的最大协议数，目前仅适用了两个，0-汉邦协议，1-ONVIF协议
}STRUCT_SDVR_IPC_SUPPORTINFO;

typedef struct
{
	int port;     //端口，汉邦协议表示命令端口号，ONVIF协议表示HTTP端口号
	char ip[128];    //IP地址
	char mac_addr[8];     //MAC地址
	char vendor_name[32];     //厂商名
	int protocol_type;     //当前的协议类型，0-HB,1-ONVIF,2-HBGK_EXT,3-AVIPC,4-SAMSUNG,5-WAYULINK
	char reserve[16];   //保留
}STRUCT_SDVR_DEVSEACHCONTENT;

typedef struct
{
	int num;   //搜到的IP设备数量
	STRUCT_SDVR_DEVSEACHCONTENT ipc_devsearch_content[256];   //搜到的IP设备内容，256为支持的最大IP设备搜索数
	char reserve[16];     //保留
}STRUCT_SDVR_SEACH_IPDEV, *LPSTRUCT_SDVR_SEACH_IPDEV;

typedef enum
{
	GET_ZDWX_PT_CONFIG = 0xF010, //获取浙大网新药监平台参数设置
	SET_ZDWX_PT_CONFIG,          //设置浙大网新药监平台参数设置
	GET_ZDWX_TMP_HUMI_CONFIG,    //获取温湿度设备参数设置
	SET_ZDWX_TMP_HUMI_CONFIG,    //设置温湿度设备参数设置
	GET_ZDWX_GLOBAL_CONFIG,      //获取搜索设备起始ID，范围，搜索间隔
	SET_ZDWX_GLOBAL_CONFIG,      //设置搜索设备起始ID，范围，搜索间隔
	SET_ZDWX_FINGLE_TMP_CONFIG,      //设置指纹模板
	CLEAR_ZDWX_FINGLE_TMP_CONFIG,    //清除指纹模板
	RETURN_ZDWX_FINGER_PRINT,         //用户按指纹后的结果
	SET_ZDWX_CALLBACK_UPSEND,          //标示插件已经连接上DVR，已经准备   好了接收回调数据
	UPLOAD_ZDWX_FINGLE_COMPARE_CONFIG,  //上传指纹比对结果
	UPLOAD_ZDWX_FINGLE_IMAGE_CONFIG,  //上传指纹图像数据
	UPLOAD_ZDWX_TMP_HUMI_DEVINFO_CONFIG,  //上传温湿度设备编号信息
	UPLOAD_ZDWX_TMP_HUMI_CONFIG,  //反馈温湿度设备采集信息
	GET_ZDWX_HISTORY_DATA,      //历史数据查询
	GET_ZDWX_FINGLE_CONPARE_TYPE,      //平台获取主机指纹比对方式  1本地对比 2服务器集中对比
	SET_ZDWX_FINGLE_CONPARE_TYPE,      //平台设置主机指纹比对方式
	GET_ZDWX_DATA_SAVE_TYPE,      //平台获取主机数据采集存储方式   1网络转发 2本地保存 3网络转发并保存
	SET_ZDWX_DATA_SAVE_TYPE,      //平台设置主机数据采集存储方式
	UPLOAD_ZDWX_TMP_HUMI_ONLINE_INFO,  //上传温湿度设备在线状态
	UPLOAD_ZDWX_FINGLE_ONLINE_INFO,  //上传指纹仪设备在线状态
	GET_ZDWX_MEDICINE_INTERVAL_TM,      //获取药监设备状态上报间隔时间 
	SET_ZDWX_MEDICINE_INTERVAL_TM,      //设置药监设备状态上报间隔时间 
} ZDWX_PT_CMD_E;

//药监平台参数
typedef struct 
{
	char    platform_addr[128];
	long    platform_port;
	char    dvr_register_id[64];        //注册序列号ID
}WXYJ_platform_config, *PWXYJ_platform_config;

//温湿度传感器参数
typedef struct 
{
	long dev_id;             // 传感器设备编号 1-999

	/* 传感器采集数据信息 */
	long tmp;                 //温度
	long hui;                 //湿度
	long baty;                //电量
	/* 获取或设置传感器配置信息 */
	long tempr_up;           // 温度上限
	long tempr_down;	       // 温度下限
	long hunty_up;           // 湿度上限
	long hunty_down;         // 湿度下限
	long collect_time;       // 温湿度传感器采集间隔时间 秒
}WXYJ_LOCAL_PARAM, *PWXYJ_LOCAL_PARAM;

//温湿度信息
typedef struct 
{
	long first_search_id;    // 设备搜索起始ID    值1-999   默认955
	long search_range;	   // 设备搜索范围      值1-50    默认20
	long search_time;        // dvr搜索设备时间   值1-1800  默认300秒
}WXYJ_GLOBAL_PARAM, *PWXYJ_GLOBAL_PARAM;

//指纹模板
typedef struct
{
	char   finger_feature[360];   // 指纹特征码
	char   usr_name[48];          // 用户名 
	char   ID_card[32];           // 身份证号
	char   reserve[32];
} WXYJ_FINGER_CONFIG, *PWXYJ_FINGER_CONFIG;

//采集信息
typedef struct
{
	long total_count;
	WXYJ_LOCAL_PARAM tmp_humi_dev[MAX_HUMTMP_DEV];
}WXYJ_TMP_HUMI_DEV_INFO, *PWXYJ_TMP_HUMI_DEV_INFO;

/* 未比对的指纹数据信息 */
typedef struct
{
	unsigned long  size;  // 指纹数据长度
	char   outbuf[152 * 200 + 16];  // 指纹图像数据保存buffer
} WXYJ_FINGER_UNCOMPARE_DATA, *PWXYJ_FINGER_UNCOMPARE_DATA;

/* 比对后的指纹数据信息, 平台下载的指纹模板 */
typedef struct
{
	char   finger_feature[344];   // 指纹特征码
	char   ID_card[20];          // 用户名 
	char   usr_name[45];           // 身份证号
	char   reserve[3];
}WXYJ_FINGER_COMPARE_DATA, *PWXYJ_FINGER_COMPARE_DATA;

/* 设备工作状态枚举 */
typedef enum
{
	PERIPHERA_WORKING_OFFLINE = 0,  //设备不在线，工作异常
	PERIPHERA_WORKING_ONLINE,       //设备在线，工作正常
}PT_PERIPHERA_WORKING_E;


/* 传感器设备状态*/
typedef struct 
{
	long dev_id;              //传感器设备编号 1-500
	long work_state;          //在线状态,PT_PERIPHERA_WORKING_E
	long baty;                //电池电量
} PT_HUMTR_SENSOR_WORK_S, *PPT_HUMTR_SENSOR_WORK_S;

/* 温湿度设备工作状态上报 */
typedef struct 
{
	long                   work_state;       //温湿度接收器工作状态,PT_PERIPHERA_WORKING_E
	PT_HUMTR_SENSOR_WORK_S   sensor_work[MAX_HUMTMP_DEV];  //传感器工作状态
} PT_HUMTR_WORK_S, *PPT_HUMTR_WORK_S;

/* 药监设备工作状态上报时间间隔 */
typedef struct 
{
	DWORD						dev_type;       //药监设备类型
	DWORD						interval_time;  //上报间隔时间，单位秒，最小5*60秒，最大24*60*60秒
} PT_MED_WORKING_INTERVAL_TM_S, *PPT_MED_WORKING_INTERVAL_TM_S;

//通道录像预录及延录时间
typedef struct 
{ 
	DWORD    channel;        // 通道号 
	DWORD    type;           // 录像类型，0x01-手动录像,0x02-定时录像, 
	// 0x04-移动录像,0x08-报警录像,0xff-所有录像 
	DWORD    prerecord_time; // 预录时间,5-30 秒 
	DWORD    delayrecord_time; // 延录时间，0-180 秒 
	BYTE   reserver[4];      // 保留 
}STRUCT_SDVR_RECTIME, *LPSTRUCT_SDVR_RECTIME;

//远程图片查询
typedef struct 
{ 
	BYTE  byChannel;    //通道号[0, n－1]，n:通道数 
	BYTE  dwType;       //查询的图片类型，下表是值和图片类型对应关系 
	BYTE  pic_format;  // 图片格式，0--jpg ，1-bmp ；目前只能是jpg 格式
	BYTE  reverse;		//保留
	UNMFS_TIME  starttime; //查询的开始时间 
	UNMFS_TIME  endtime;  //查询的结束时间 
	DWORD   dwStart;     //从第几个文件开始查询，一般为0(用于由于时间段内文件数过多而分多 
	//次查询的情况，第一次查询为0，以后递加成功查询出的文件数) 
	WORD   dwNum;       //一次查询的个数，现在定义是100 
	BYTE   reverse1[6]; //保留
}STRUCT_SDVR_SNAPSHOTFIND;

typedef struct 
{ 
	UNMFS_TIME  time;     // 图片的抓拍时间，结构同上，年份-2000 
	unsigned int lengh; // 图片大小 
	BYTE  ch;           //抓拍的通道号
	BYTE   dwType;         // 图片类型，1--手动，2--定时，4--移动，8--探头，0xff--全部 
	BYTE   pic_format;     // 图片格式，0--jpg ，1-bmp ；目前只能是jpg 格式 
	BYTE   cRes[5];          //保留 
}Mfs_SnapShotSeg_Info, *PMfs_SnapShotSeg_Info; 

typedef struct 
{ 
	DWORD    Totalnum;     //查找到的文件个数 
	Mfs_SnapShotSeg_Info recdata[100]; 
}STRUCT_SDVR_SNAPSHOTINFO, *LPSTRUCT_SDVR_SNAPSHOTINFO;

//IPV6网络参数
typedef struct	
{
	DWORD ip_mode;                        //用于表示网络模式，0表示IPV4模式，1表示IPV6模式
	DWORD dwNetInterface;		     //网络接口 1-10MBase-T 2-10MBase-T全双工 3-100MBase-TX 4-100M全双工 5-10M/100M自适应
	WORD wDVRPort;		            //端口号 
	BYTE byMACAddr[MACADDR_LEN];          //网卡的物理地址
	BYTE strDVRdefgatewayIP[64];          //DVR 默认网关IP地址，以'\0'结尾
	BYTE  strDNSIP[64];	                   //DNS服务器地址 ，以'\0'结尾
	BYTE  strSECDNSIP[64];	            //DNS服务器备选地址，以'\0'结尾
	DWORD dhcp_enable;                    //用于DHCP是否启用，0表示未启用，1表示启用 
	BYTE strDVRIP[64];	                   //DVR IP地址 ，以'\0'结尾  
	BYTE strDVRMASKIP[64];		     //DVR 子网掩码地址，，以'\0'结尾，IPV6网络无需关注  
	BYTE strDVRLOCALIP[64];	            //DVR 链路本地IP地址  ，以'\0'结尾
	DWORD dwDVRIPnetpre;	            //DVR IP子网前缀位数,IPV4网络无需关注
	BYTE  reserved[64];                   //保留	 
}STRUCT_SDVR_ETHERNET_6,*LPSTRUCT_SDVR_ETHERNET_6;

typedef struct	 
{
	DWORD dwSize;          
	DWORD default_network;                        //用于表示默认网卡序号，如：0表示eth0,1表示eth1,2表示eth2，依次类推
	STRUCT_SDVR_ETHERNET_6 struEtherNet[MAX_ETHERNET]; //以太网口 

	BYTE  strManageHostIP[64];				//远程管理主机地址，以'\0'结尾	 
	WORD wManageHostPort;				//保存

	BYTE  strMultiCastIP[64];				//多播组地址，以'\0'结尾
	BYTE  strNFSIP[64];				//保存，以'\0'结尾
	BYTE sNFSDirectory[PATHNAME_LEN];	                    //保存
	DWORD dwPPPOE;						//0-不启用,1-启用
	BYTE sPPPoEUser[NAME_LEN];			//PPPoE用户名	 
	char sPPPoEPassword[PASSWD_LEN];	//PPPoE密码
	BYTE  strPPPoEIP[64];					//PPPoE IP地址(只读)，以'\0'结尾
	WORD wHttpPort;					//HTTP端口号
	BYTE  reserve[128];                                    //保留
}STRUCT_SDVR_NETINFO_6,*LPSTRUCT_SDVR_NETINFO_6;

//扩展OSD协议
typedef struct
{
	DWORD type;//0-主机名称，1-通道名称，2-扩展OSD1，3-扩展OSD2，4-扩展OSD3，5-扩展OSD4，6-扩展OSD5
	DWORD  channel;//通道号，从0开始，当type=0时，该项无效
	BYTE  reverser[16];//保留
}STRUCT_SDVR_OSD_CONFIG_REQ;

typedef struct
{
	WORD x, y; // xy坐标
	WORD w, h; // 宽度和高度（该项目前未实现）
} SDVR_RECT;

typedef struct
{
	DWORD type;//0-主机名称，1-通道名称，2-扩展OSD1，3-扩展OSD2，4-扩展OSD3，5-扩展OSD4，6-扩展OSD5
	DWORD  channel;//通道号，从0开始，当type=0时，该项无效     
	SDVR_RECT coordinate;//OSD坐标，当type=0时，该项无效    
	BYTE  show_enable;//是否显示，0-不显示，1-显示   
	BYTE  style;//OSD样式，1-透明闪烁，2-透明不闪烁，3-闪烁不透明，4-不透明不闪烁 （该项目前未实现）   
	BYTE  alpha;//OSD透明度  0-不透明，1-透明  
	BYTE  reverser1; //保留    
	DWORD typemaxlen;//type类型支持的最大字节数，该值由主机确定，设置时字符串的长度不能大于这个值  
	DWORD typelen;//type类型当前所占用的字节数
	BYTE  reverser2[16];//保留
	BYTE  data[0];//typelen长度的字符串，以'\0'结束
}STRUCT_SDVR_OSD_CONFIG_RSP, *LPSTRUCT_SDVR_OSD_CONFIG_RSP;

//总帧率和剩余帧率
typedef struct
{
	DWORD stream_type;//0-录像参数（主码流），1-子码流（目前未实现），2-解码（目前未实现）
	DWORD reverse[4];//保留
}STRUCT_SDVR_FRAMERATE_REQ;

typedef struct
{
	DWORD stream_type;//0-录像参数（主码流），1-子码流（目前未实现），2-解码（目前未实现）
	DWORD total_freme;//所有通道的总帧率
	DWORD residual_frame;//剩余帧率
	DWORD reverse[4];//保留
}STRUCT_SDVR_FRAMERATE_TOL, *LPSTRUCT_SDVR_FRAMERATE_TOL;

//远程图片备份
typedef struct 
{ 
	BYTE   byChannel;      //通道号 [0, n-1], n:通道数
	BYTE  type;        // 图片类型：1-手动；2-定时；4-移动；8-探头报警；0xff-所有录像 
	BYTE  pic_format;  // 图片格式，0--jpg ，1-bmp ；目前只能是jpg 格式
	BYTE reverse1;//保留
	DWORD   file_index; //客户端下载的文件列表的索引号，从0 开始 
	DWORD   file_offset; //文件偏移大小，刚开始为0 
	UNMFS_TIME  starttime; //开始时间，结构体同上，注意年份-2000 
	UNMFS_TIME  endtime; //结束时间
	BYTE reverse2[4];//保留
}STRUCT_SDVR_BACK_SNAPSHOTINFO;

//远程图片备份文件头标志
typedef struct 
{ 
	DWORD   port;      //通道号 
	DWORD   filelen;   //本文件长度 以B 为单位 
	UNMFS_TIME  time;    //本图片文件的抓拍时间 
	DWORD   totalFileLen; //总文件长度，表示NETCOM_BACKUP_SNAPSHOT_REQ 命令所给的 
	//时间段内的抓拍图片总长度，以KB 为单位 
	DWORD   file_index; //文件列表的索引号，从0 开始
	DWORD   file_offset; //文件偏移大小，刚开始为0
	BYTE  reverse[4];//保留
}STRUCT_SDVR_SNAPSHOT_FILEINFO, *LPSTRUCT_SDVR_SNAPSHOT_FILEINFO;

//云台多预置点轮巡
typedef struct 
{ 
	DWORD    byChannel;  //设置通道 [0, n-1] n:通道数 
	WORD   Preset[16];   //预置点[1，254]，255 是无效值，而有的球机0 有效，有的球机0 无效。 
	//如果预置点少于16 个，多余的填255 
	WORD   PresetPoll;   //多预置点轮巡开启或关闭表示(0, 关闭；1，开启) 
	WORD   presettime;   //多预置点轮巡间隔时间(单位秒) [1, 99] 
}HB_SDVR_PRESETPOLL,*LPHB_SDVR_PRESETPOLL; 

//一点通 参数
typedef struct 
{ 
	char enable;            //启用一点通使能-1-不支持，0-off, 1-on 
	BYTE   reserve[31];      //保留 
}STRUCT_SDVR_DYT_STATUE,*LPSTRUCT_SDVR_DYT_STATUE; 

//
typedef struct 
{ 
	long          support_alarmtype;      //主机对所有报警类型是否支持上传参数的情况，从低到高按位表示//STRUCT_SDVR_ALMUPLOADTYPE_E, // 位参数: 1-支持，  0-不支持 
	BYTE           reserve[16];                  //保留位 
} STRUCT_SDVR_ALMUPLOADSUPT_S, *PSTRUCT_SDVR_ALMUPLOADSUPT_S;     

typedef enum 
{ 
	NET_ALM_UP_MOTION,                      // 移动检测报警 
	NET_ALM_UP_VIDEOLOST,                //  视频丢失报警 
	NET_ALM_UP_VIDEO_COVER,          //  视频遮挡报警 
	NET_ALM_UP_SENSOR,                      //  探头报警 
	NET_ALM_UP_TEMP_HI,                    //  温度过高报警 
	NET_ALM_UP_HD_ERROR,                //  磁盘错误报警 
	NET_ALM_UP_HD_FULL,           //  磁盘满报警 
	NET_ALM_UP_TYPE_MAX, 
} STRUCT_SDVR_ALMUPLOADTYPE_E; 

typedef struct 
{     
	WORD     alm_type;              //  报警类型，STRUCT_SDV_ALMUPLOADTYPE_E 
	WORD     upload_enable;      //  报警上传使能，参数: 1-开启上传，  0-不上传 
	STRUCT_SDVR_SCHEDTIME     upload_time[8][4];        //  上传报警时间段设置, [0][0]-[0][3]，每天，可设置4个时间段。[1～7][0]-[1～7][3],星期一到星期天，可设置 4个时间段 
	BYTE          reserve[16];          //保留位 
} STRUCT_SDVR_ALMUPLOADPARAM_S, *PSTRUCT_SDVR_ALMUPLOADPARAM_S;  

typedef struct
{
	long channel;    //通道号
	long command;    //命令，CAMERA_PARAM_SET_CMD_E
	BYTE reserve[16];   //保留
}STRUCT_SDVR_CAMERA_PARAM_SET_CRTL,*LPSTRUCT_SDVR_CAMERA_PARAM_SET_CRTL;

typedef struct 
{ 
	BYTE enable;        //使能 
	BYTE hour;            //时 
	BYTE min;            //分 
	BYTE sec;            //秒 
	BYTE reserve[4];      //保留 
}STRUCT_SDVR_REBOOT_TIME; 

typedef struct 
{ 
	BYTE en_time_reboot;                                          //定时重启使能，1-启用，0-不启用 
	BYTE reserve1[3];                //保留 
	STRUCT_SDVR_REBOOT_TIME week[8];      //重启时间([0]-每天，[1-7]对应周一到周日)。[0]如有被使能
	//每天有效，则按每天处理，不关注星期几                             
	BYTE reserve2[16];            //保留 
} STRUCT_SDVR_TIMING_REBOOT_PARAM,*LPSTRUCT_SDVR_TIMING_REBOOT_PARAM; 




//新回放协议
typedef struct TAG_VOD_PARAM
{
	BYTE byChannel; // 通道号[0, n-1],n:通道数
	BYTE byType; // 录像类型: 1-手动，-定时，-移动，-报警，xFF-全部
	WORD wLoadMode; // 回放下载模式 1-按时间，2-按名字
	union
	{
		struct
		{
			UNMFS_TIME struStartTime; // 最多一个自然天
			UNMFS_TIME struStopTime; // 结束时间最多到23:59:59,
			// 即表示从开始时间开始一直播放
			char cReserve[16];
		}byTime;
		BYTE byFile[64]; // 是否够长？
	}mode;
	BYTE streamtype; //码流类型，0-主码流，1-子码流
	BYTE byReserve[15]; //保留
}VOD_PARAM, *LPVOD_PARAM;

typedef struct TAG_VOD_ANS
{
	DWORD dwVodID; //主机分配
	BYTE streamtype; //码流类型，0-主码流，1-子码流
	BYTE byReserve[15]; //保留
}VOD_ANSWER, *LPVOD_ANSWER;

typedef struct TAG_VOD_GETDATA
{
	DWORD dwVodID;
	DWORD dwCon; // 是否连续请求, 0,重新; 1,连续(struStartTime无效);
	UNMFS_TIME struStartTime; // 开始时间(dwCon为0时有效)
	DWORD dwFrameNum; // 取帧数，150帧(内)
	DWORD dwFrameType; // 0,所有(音视频)帧; 1,仅I帧
	char cReserve[16]; // 保留
}VOD_GETDATA, *LPVOD_GETDATA;

typedef struct TAG_VOD_GETDATA_ANS
{
	DWORD dwVodID;
	DWORD dwReserve[2]; // 保留
	DWORD dwLen; // 数据长度
}VOD_GETDATA_ANSWER, *LPVOD_GETDATA_ANSWER;

typedef struct TAG_VOD_PAUSE
{
	DWORD dwVodID; // 点播ID
	DWORD dwStat; // 请求（0,暂停; 1,暂停恢复）
	// 应答（0,暂停; 1,正常）
	DWORD dwReserve[4]; // 保留
}VOD_PAUSE, *LPVOD_PAUSE;

typedef struct TAG_VOD_END
{
	DWORD dwVodID;
	char cReserve[16];
}VOD_END, *LPVOD_END;

typedef struct	{
	BYTE	byChannel;					//通道号
	BYTE	byLinkMode;					// 0-主码流单socket  1-子码流 单socket  
	BYTE	OSDCharEncodingScheme;		// OSD字符的编码格式 
	BYTE	reserve[9];					//保留
}ST_SDVR_REALPLAY_MULTI,*LPST_SDVR_REALPLAY_MULTI;

#ifdef _MSC_VER
#pragma warning(default: 4200)
#endif
#pragma pack(pop)

#endif
