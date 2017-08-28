#ifndef __NETCMDSTRUCT_H__
#define __NETCMDSTRUCT_H__

#define NAME_LEN		32					//�û�������
#define PASSWD_LEN		16					//����
#define SERIALNO_LEN	48					//���к�
#define MAX_CHANNUM		16					//ͨ����
#define MAX_CHANNUM_EX	128					//ͨ����(��չ)
#define MAX_DISKNUM		16					//Ӳ����
#define MAX_ETHERNET	2
#define PATHNAME_LEN	128
#define MAX_DAYS		8
#define MAX_TIMESEGMENT	2
#define PRESETNUM		16					//Ԥ�õ�
#define MAX_SHELTERNUM	4					//ʱ��
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
#define MAX_REC_NUM		100					//��ʷ��Ƶ��־��
#define MAXPTZNUM		100					//��̨Э��
#define MAX_PRESET		128
#define MAX_LOGO_NUM	100

#define PROTOCOL_V3     32					//3.0Э��
#define MAX_DEV_NUM		32
#define MAX_FORMAT_DEV	16
#define MAX_PAT         15 

#define		ALARM_MD						0	//�ƶ���ⱨ��
#define		ALARM_IN							1	//̽ͷ����
#define		ALARM_VIDEOLOST			2	//��Ƶ��ʧ����
#define		ALARM_DISKERR				3	//�����쳣����
#define		ALARM_NET_DISCONN		4	//����Ͽ�����
#define		ALARM_TEMP_HIGH			5	//�¶ȹ���
#define		ALARM_VIDEOCOVER			6	//��Ƶ�ڵ�����
#define		ALARM_REC_NOTENOUGH	7	//¼���������㱨��

#define CMDNO(cmd, id)  (((id) << 16) | (cmd))
#define COMP_TEST               (911)            //�Զ�������ģ��
#define MAX_CH              64


#define MAX_HUMTMP_DEV		20   //��ʪ�Ȳɼ���������  //����ҩ��

//����ṹ
#pragma pack(push, 1)
#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif

//ʱ���ʽ
typedef struct
{
	uint8_t byEnable;							//����  0-���� 1-����	
    uint8_t byStartHour;
	uint8_t byStartMin;
	uint8_t byStopHour;
	uint8_t byStopMin;
}STRUCT_SDVR_SCHEDTIME, *LPSTRUCT_SDVR_SCHEDTIME;

//��¼�豸
typedef struct
{
	char	szUserName[NAME_LEN];			//�û���
	char	szPassWord[PASSWD_LEN];			//����
	DWORD	dwNamelen;						//��Ч���û�������
	DWORD	dwPWlen;						//��Ч�����볤��
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
	NET_DEVTYPE_2600TB,    //����ͳ�����ܷ����� 
	NET_DEVTYPE_2600TC,    //����ʶ�����ܷ����� 
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
	DL_DEVTYPE_HB9304 = 170, /*����:��Ψһ��ʶһ�־������*/
	DL_DEVTYPE_HB9308,
	DL_DEVTYPE_HB9404,
	DL_DEVTYPE_HB9408,
	DL_DEVTYPE_HB7216XT3,
	DL_DEVTYPE_HB7016X3LH,  //HB7016X3LH
	DL_DEVTYPE_HB7208XT3,	
	DL_DEVTYPE_HB7216LL3,   //ʵ����DL_DEVTYPE_HB7116X3
	DL_DEVTYPE_HB7208LL3,   //ʵ����DL_DEVTYPE_HB7108X3
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
	DL_DEVTPYE_HB_DVR5208 = 290,		//7308X3-L�Ļ��͸�����HB-DVR5208
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

//�豸��������
typedef struct
{
	char sDVRID[48]; //ƽ̨�����DVR�ĵ�¼ID
	char sSerialNumber[48]; //���кţ������˱�����䣬��ǰ���������ಹ��
	//DVR�豸��������к�
	uint8_t byAlarmInPortNum; //DVR�����������
	uint8_t byAlarmOutPortNum; //DVR�����������
	uint8_t byDiskNum; //DVR Ӳ�̸���
	uint8_t byProtocol; //�����Ͳ�Ʒ��ֵ��Ϊ0x20����Э�������
	uint8_t byChanNum; //DVR ͨ������
	uint8_t byEncodeType; //���������ʽ��1ΪANSI�ַ��������Ĳ���GB2312���룻2ΪUTF8
	uint8_t reserve[26]; //����
	char sDvrName[32]; //������
	char sChanName[128][32]; //ͨ������
}STRUCT_SDVR_INITIATIVE_LOGIN;

//�豸����
typedef struct
{
	DWORD  dvrtype;							//7004 8004 2201 2004
	//DWORD  nreserve1;						//����
	uint16_t   device_type;					    //�豸����		ö��NET_DEV_TYPE_E
	uint16_t   memory_size;					    //�ڴ��С		ö��NET_MEMSIZE_E
	DWORD  nreserve2;						//����
 }STRUCT_SDVR_INFO, *LPSTRUCT_SDVR_INFO;

//�豸��Ϣ
typedef struct
{
	char szSerialNumber[SERIALNO_LEN];		//����
	uint8_t byAlarmInPortNum;					//DVR�����������
	uint8_t byAlarmOutPortNum;					//DVR�����������
	uint8_t byDiskNum;							//DVRӲ�̸���
	uint8_t byDVRType;							//DVR����, 1:DVR 2:ATM DVR 3:DVS ������ʹ��NET_SDVR_GET_DVRTYPE���
	uint8_t byChanNum;							//DVRͨ������
	uint8_t byStartChan;						//����
	char szDvrName[NAME_LEN];				//������
	char szChanName[MAX_CHANNUM][NAME_LEN];	//ͨ������
}STRUCT_SDVR_DEVICE, *LPSTRUCT_SDVR_DEVICE;

//�豸��Ϣ(��չ)
typedef struct
{
	char sSerialNumber[SERIALNO_LEN];		//����
	uint8_t byAlarmInPortNum;					//DVR�����������
	uint8_t byAlarmOutPortNum;					//DVR�����������
	uint8_t byDiskNum;							//DVRӲ�̸���
	uint8_t byDVRType;							//DVR����, 1:DVR 2:ATM DVR 3:DVS ������ʹ��NET_SDVR_GET_DVRTYPE���
	uint8_t byChanNum;							//DVRͨ������
	uint8_t byStartChan;						//����
	char szDvrName[NAME_LEN];				//������
	char szChanName[MAX_CHANNUM_EX][NAME_LEN];//ͨ������
}STRUCT_SDVR_DEVICE_EX, *LPSTRUCT_SDVR_DEVICE_EX;

//Ӳ����Ϣ
typedef struct
{
	DWORD dwSize;
	uint8_t sDVRName[NAME_LEN];				//DVR����
	DWORD dwDVRID;							//����
	DWORD dwRecycleRecord;					//����
	uint8_t sSerialNumber[SERIALNO_LEN];		//���к�
	uint8_t sSoftwareVersion[16];				//����汾��
	uint8_t sSoftwareBuildDate[16];			//�����������
	DWORD dwDSPSoftwareVersion;				//DSP����汾
	uint8_t sPanelVersion[16];					//ǰ���汾
	uint8_t sHardwareVersion[16];				//����
	uint8_t byAlarmInPortNum;					//DVR�����������
	uint8_t byAlarmOutPortNum;					//DVR�����������
	uint8_t byRS232Num;						//����
	uint8_t byRS485Num;						//����
	uint8_t byNetworkPortNum;					//����
	uint8_t byDiskCtrlNum;						//����
	uint8_t byDiskNum;							//DVR Ӳ�̸���
	uint8_t byDVRType;							//DVR����, 1:DVR 2:ATM DVR 3:DVS ������ʹ��NET_SDVR_GET_DVRTYPE���
	uint8_t byChanNum;							//DVR ͨ������
	uint8_t byStartChan;						//����
	uint8_t byDecordChans;						//����Ĭ�Ͽɽ����IP�豸·�������DVRÿһ�����Ͷ���һ��Ĭ�Ͽɽ����IP�豸·������8·��Ĭ�Ͽɽ���4·IPC��16·��Ĭ�Ͽɽ���8·IPC��
	//��Ĭ��·���������·����8·�����ɽ���4+8=12��IPC,16·�����ɽ���8+16=24��IPC������ȡʱ��Ч������ʱ��ֵ��Ч��
	uint8_t byVGANum;							//����
	uint8_t byUSBNum;							//����
 }STRUCT_SDVR_DEVICEINFO, *LPSTRUCT_SDVR_DEVICEINFO;

//������Ϣ
typedef struct
{
	uint16_t wAlarm;							//̽ͷ����  ��λ��0-��һͨ��  1-��2ͨ�� ...
	uint16_t wVlost;							//��Ƶ��ʧ  ...
	uint16_t wMotion;							//�ƶ�����	...
	uint16_t wHide;								//����		��Ƶ�ڵ�����20110908...	
	uint8_t byDisk[MAX_DISKNUM];				//����		...
}STRUCT_SDVR_ALARM, *LPSTRUCT_SDVR_ALARM;

//������Ϣ(��8000)
typedef struct
{
	uint16_t wAlarm;							//̽ͷ����  ��λ��0-��һͨ��  1-��2ͨ�� ...
	uint16_t wVlost;							//��Ƶ��ʧ  ...
	uint16_t wMotion;							//�ƶ�����	...
	uint16_t wHide;								//����		...	
}STRUCT_SDVR_ALARM_OLD, *LPSTRUCT_SDVR_ALARM_OLD;

//������Ϣ(��չ)
typedef struct
{
	uint8_t wAlarm[MAX_CHANNUM_EX];			//̽ͷ����  0-�ޱ���  1-�б���
	uint8_t wVlost[MAX_CHANNUM_EX];			//��Ƶ��ʧ  ...
	uint8_t wMotion[MAX_CHANNUM_EX];			//�ƶ�����	...
	uint8_t wHide[MAX_CHANNUM_EX];				//����		��Ƶ�ڵ�����20110908...	
	uint8_t byDisk[MAX_DISKNUM];				//����		...
}STRUCT_SDVR_ALARM_EX, *LPSTRUCT_SDVR_ALARM_EX;

//��������
typedef struct
{
	DWORD dwHandleType;				//��λ(2-��������,5-���������)
	uint16_t wAlarmOut;					//�����������ͨ�� ��λ��Ӧͨ��
}STRUCT_SDVR_HANDLEEXCEPTION, *LPSTRUCT_SDVR_HANDLEEXCEPTION;

//��������(��չ)
typedef struct
{
	DWORD dwHandleType;				//��λ(2-��������,5-���������)
	uint8_t szAlarmOut[MAX_CHANNUM_EX];//�����������ͨ����λ��Ӧͨ��
}STRUCT_SDVR_HANDLEEXCEPTION_EX, *LPSTRUCT_SDVR_HANDLEEXCEPTION_EX;

//ʵʱ��Ƶ
typedef struct
{
	uint8_t byChannel;						//ͨ����
	uint8_t byLinkMode;					//0-������ 1-������  
	uint8_t byMultiCast;					//����
	char szMultiCastIP[16];				//����
	uint16_t wPort;							//���� 
}STRUCT_SDVR_REALPLAY, *LPSTRUCT_SDVR_REALPLAY;

//ʵʱ��Ƶ(��չ)
typedef struct	
{
	uint8_t	byChannel;					//ͨ����
	uint8_t	byLinkMode;					//0-������ 1-������  
	uint8_t	byMultiCast;				//�Ƿ�ಥ�������������˶ಥģʽ��ʹ��UDP��ʽ����Ƶ��ʱ����Զ�ѡ��ಥ��
	DWORD	sMultiCastIP;				//�ಥIP��ַ(���ֽ���)
	uint8_t	OSDCharEncodingScheme;		//OSD�ַ��ı����ʽ
	uint8_t 	reserve[11];				//����
	uint16_t 	wPort;						//�ಥ�˿�(С�ֽ���)
}STRUCT_SDVR_REALPLAY_EX, *LPSTRUCT_SDVR_REALPLAY_EX;

//�������ӻ�ȡʵʱ��Ƶ
typedef struct
{
	DWORD msgid; //��ϢID,��id ��ƽ̨���ɣ�DVR ԭ�ⲻ������,msgid ����ƽ̨���������½�����socket ���ӡ�ʹƽ̨����֪����socket,���Ӷ�Ӧ�������byChannel �޹ء�
	uint8_t byChannel; //ͨ����
	uint8_t byLinkMode; // 0-������1-������
	uint8_t byMultiCast; //�Ƿ�ಥ(�����������˶ಥģʽ��ʹ��UDP ��ʽ����Ƶ��ʱ����Զ�ѡ��ಥ)
	uint8_t OSDCharEncodingScheme; // OSD �ַ��ı����ʽ
	DWORD sMultiCastIP; //�ಥIP ��ַ(���ֽ���)
	uint8_t reserve1[10]; //����
	uint16_t wPort;//�ಥ�˿�(С�ֽ���)
	uint8_t reserve2[16]; //����
}STRUCT_SDVR_REALPLAY_INITIATIVE,*LPSTRUCT_SDVR_REALPLAY_INITIATIVE;

//�ر�ʵʱ��Ƶ
typedef struct
{
	uint8_t byChannel;						//ͨ����
	uint8_t byLinkMode;					//λ��0-������ 1-������  
 	uint16_t uPort;							//���ض˿�
}STRUCT_SDVR_REALPLAY_STOP, *LPSTRUCT_SDVR_REALPLAY_STOP;

//��Ƶ����
typedef struct
{
	DWORD	dwBrightValue;				//����(1-127)
	DWORD	dwContrastValue;			//�Աȶ�(1-127)
	DWORD	dwSaturationValue;			//���Ͷ�(1-127)
	DWORD	dwHueValue;					//ɫ��(1-127)
}STRUCT_SDVR_VIDEOPARAM, *LPSTRUCT_SDVR_VIDEOPARAM;

typedef struct
{	
	uint16_t wStartTime;					//��8λ��ʾСʱ ��8λ��ʾ����
	uint16_t wEndTime;						//��8λ��ʾСʱ ��8λ��ʾ����
	STRUCT_SDVR_VIDEOPARAM VideoParam;
}STRUCT_SDVR_SCHEDULE_VIDEOPARAM, *LPSTRUCT_SDVR_SCHEDULE_VIDEOPARAM;

typedef struct
{
	uint8_t	byChannel;					//ͨ����
	STRUCT_SDVR_SCHEDULE_VIDEOPARAM Schedule_VideoParam[2];	//һ�����2��ʱ���
	STRUCT_SDVR_VIDEOPARAM Default_VideoParam;				//����ʱ����ھ�ʹ��Ĭ��
}STRUCT_SDVR_VIDEOEFFECT,*LPSTRUCT_SDVR_VIDEOEFFECT;

//��̨����
typedef struct
{
	uint8_t	byPort;						//��̨�˿ں�
	DWORD	dwPTZCommand;				//��̨��������
	DWORD   dwStop;						//�Ƿ�ֹͣ
	DWORD	dwIndex;					//Ԥ�Ƶ��
	DWORD	dwSpeed;
}STRUCT_SDVR_PTZ_CTRL, *LPSTRUCT_SDVR_PTZ_CTRL;

typedef struct
{
	uint8_t byPort;						//��̨�˿ں�
	DWORD dwSize;						//��̨��������Ч����
	uint8_t szCtrlBuf[PTZ_SIZE];			//��̨����������
}STRUCT_SDVR_PTZ_TRANS;

//�������
typedef struct
{
	DWORD dwKey;
	DWORD dwKeyCode;
}STRUCT_SDVR_NETKEY, *LPSTRUCT_SDVR_NETKEY;

//�豸������Ϣ
typedef struct
{
	DWORD dwDVRIP;						//DVR IP��ַ
	DWORD dwDVRIPMask;					//DVR IP��ַ����
	DWORD dwNetInterface;				//����ӿ� 1-10MBase-T 2-10MBase-Tȫ˫�� 3-100MBase-TX 4-100Mȫ˫�� 5-10M/100M����Ӧ
	uint16_t wDVRPort;						//�˿ں�
	uint8_t byMACAddr[MACADDR_LEN];		//�������������ַ
}STRUCT_SDVR_ETHERNET, *LPSTRUCT_SDVR_ETHERNET;
 
typedef struct
{
	DWORD dwSize;
	STRUCT_SDVR_ETHERNET struEtherNet[MAX_ETHERNET]; /* ��̫���� */
	DWORD dwManageHostIP;				//Զ�̹���������ַ		 
	uint16_t wManageHostPort;				//����
	DWORD dwDNSIP;						//DNS��������ַ  
	DWORD dwMultiCastIP;				//�ಥ���ַ
	DWORD dwGatewayIP;       			//���ص�ַ 
	DWORD dwNFSIP;						//����
	uint8_t szNFSDirectory[PATHNAME_LEN];	//����
	DWORD dwPPPOE;						//0-������,1-����
	uint8_t szPPPoEUser[NAME_LEN];			//PPPoE�û���
	char szPPPoEPassword[PASSWD_LEN];	//PPPoE����
	DWORD dwPPPoEIP;					//PPPoE IP��ַ(ֻ��)
	uint16_t wHttpPort;						//HTTP�˿ں�
}STRUCT_SDVR_NETINFO, *LPSTRUCT_SDVR_NETINFO;

//�ƶ����
typedef struct
{
	uint8_t byMotionScope[18][22];			//�������,����22*18��С���,Ϊ1��ʾ�ú�����ƶ��������,0-��ʾ���� 
	uint8_t byMotionSensitive;				//�ƶ����������, 0 - 5,Խ��Խ����,0xff�ر� 
	uint8_t byEnableHandleMotion;			//�Ƿ����ƶ����
	STRUCT_SDVR_HANDLEEXCEPTION struMotionHandleType;	//����ʽ 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//����ʱ��
	DWORD dwRelRecord;					//����
}STRUCT_SDVR_MOTION, *LPSTRUCT_SDVR_MOTION;

//�ƶ����(��չ)
typedef struct
{
	uint8_t byMotionScope[18][22];			//�������,����22*18��С���,Ϊ1��ʾ�ú�����ƶ��������,0-��ʾ���� 
	uint8_t byMotionSensitive;				//�ƶ����������, 0 - 5,Խ��Խ����,0xFF�ر� 
	uint8_t byEnableHandleMotion;			//�Ƿ����ƶ���� 
	STRUCT_SDVR_HANDLEEXCEPTION_EX struMotionHandleType;			//����ʽ 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//����ʱ��
	uint8_t dwRelRecord[MAX_CHANNUM_EX];	//����
}STRUCT_SDVR_MOTION_EX, *LPSTRUCT_SDVR_MOTION_EX;

//�ڵ���������Ϊ704*576
typedef struct
{
	DWORD dwEnableHideAlarm;										//����
	uint16_t wHideAlarmAreaTopLeftX;									//����
	uint16_t wHideAlarmAreaTopLeftY;									//����
	uint16_t wHideAlarmAreaWidth;										//����
	uint16_t wHideAlarmAreaHeight;										//����
	STRUCT_SDVR_HANDLEEXCEPTION struHideAlarmHandleType;			//����
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//����
}STRUCT_SDVR_HIDEALARM, *LPSTRUCT_SDVR_HIDEALARM;

//�ڵ���������Ϊ704*576(��չ)
typedef struct
{
	DWORD dwEnableHideAlarm;										//����
	uint16_t wHideAlarmAreaTopLeftX;									//����
	uint16_t wHideAlarmAreaTopLeftY;									//����
	uint16_t wHideAlarmAreaWidth;										//����
	uint16_t wHideAlarmAreaHeight;										//����
	STRUCT_SDVR_HANDLEEXCEPTION_EX stHideAlarmHandleType;			//����
	STRUCT_SDVR_SCHEDTIME stAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//����
}STRUCT_SDVR_HIDEALARM_EX, *LPSTRUCT_SDVR_HIDEALARM_EX;

//�źŶ�ʧ����
typedef struct
{
	uint8_t byEnableHandleVILost;										//�Ƿ����źŶ�ʧ���� 
	STRUCT_SDVR_HANDLEEXCEPTION stVILostHandleType;					//����ʽ 	
	STRUCT_SDVR_SCHEDTIME stAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//����ʱ��
}STRUCT_SDVR_VILOST;

//�źŶ�ʧ����
typedef struct
{
	uint8_t byEnableHandleVILost;										//�Ƿ����źŶ�ʧ���� 
	STRUCT_SDVR_HANDLEEXCEPTION_EX strVILostHandleType;				//����ʽ 	
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//����ʱ��
}STRUCT_SDVR_VILOST_EX;

//�ڸ�����
typedef struct
{
	uint16_t wHideAreaTopLeftX;						//�ڸ������x����  
	uint16_t wHideAreaTopLeftY;						//�ڸ������y����  
	uint16_t wHideAreaWidth;						//�ڸ�����Ŀ� 
	uint16_t wHideAreaHeight;						//�ڸ�����ĸ� 
}STRUCT_SDVR_SHELTER, *LPSTRUCT_SDVR_SHELTER;

//ͼ�����
typedef struct
{
	uint8_t byChannel;
	DWORD dwSize;
	uint8_t sChanName[NAME_LEN];						//ͨ����
	DWORD dwVideoFormat;							//���� 
	uint8_t byBrightness;  							//����
	uint8_t byContrast;    							//����	
	uint8_t bySaturation;  							//����
	uint8_t byHue;    									//���� 	
	DWORD dwShowChanName;							//����
	uint16_t wShowNameTopLeftX;							//ͨ��������ʾλ�õ�x���� 
	uint16_t wShowNameTopLeftY;							//ͨ��������ʾλ�õ�y����  
	STRUCT_SDVR_VILOST struVILost;					//�źŶ�ʧ����
	STRUCT_SDVR_MOTION struMotion;					//�ƶ����
	STRUCT_SDVR_HIDEALARM struHideAlarm;			//����
	DWORD dwEnableHide;								//�Ƿ������ڸ�(0-����,1-ʵʱ����,2-�ط�����,3-ȫ����)
	STRUCT_SDVR_SHELTER	struShelter[MAX_SHELTERNUM];//����
	DWORD dwShowOsd;								//����
	uint16_t wOSDTopLeftX;								//����
	uint16_t wOSDTopLeftY;								//����
	uint8_t byOSDType;									//����
	uint8_t byDispWeek;								//����	
	uint8_t byOSDAttrib;								//ͨ���� 1-��͸�� 2-͸��
}STRUCT_SDVR_PICINFO, *LPSTRUCT_SDVR_PICINFO;

//ͼ�����(��չ)
typedef struct
{
	uint8_t byChannel;
	DWORD dwSize;
	uint8_t sChanName[NAME_LEN];						//ͨ����
	DWORD dwVideoFormat;							//���� 
	uint8_t byBrightness;  							//����
	uint8_t byContrast;    							//����	
	uint8_t bySaturation;  							//����
	uint8_t byHue;										//���� 	
	DWORD dwShowChanName;							//����
	uint16_t wShowNameTopLeftX;							//ͨ��������ʾλ�õ�x���� 
	uint16_t wShowNameTopLeftY;							//ͨ��������ʾλ�õ�y����  
	STRUCT_SDVR_VILOST_EX struVILost;				//�źŶ�ʧ����
	STRUCT_SDVR_MOTION_EX struMotion;				//�ƶ����
	STRUCT_SDVR_HIDEALARM_EX struHideAlarm;			//����
	DWORD dwEnableHide;								//�Ƿ������ڸ� ,0-��,1-��  
	STRUCT_SDVR_SHELTER	stShelter[MAX_SHELTERNUM];	//����
	DWORD dwShowOsd;								//����
	uint16_t wOSDTopLeftX;								//����
	uint16_t wOSDTopLeftY;								//����
	uint8_t byOSDType;									//����
	uint8_t byDispWeek;								//����	
	uint8_t byOSDAttrib;								//ͨ���� 1-��͸�� 2-͸��
}STRUCT_SDVR_PICINFO_EX, *LPSTRUCT_SDVR_PICINFO_EX;


//����ѹ������
typedef struct
{
	uint8_t byStreamType;								//��������	0-����Ƶ ,1-����Ƶ
	uint8_t byResolution;								//�ֱ���	0-CIF 1-HD1, 2-D1, 3-QCIF
	uint8_t byBitrateType;								//��������	0:������ 1:������ 2:������
	uint8_t byPicQuality;								//ͼ������	1-��� 2-�κ� 3-�Ϻ� 4-һ�� 5-�ϲ� 6-��	 �ĳɣ���ߣ��ϸߣ��ߣ��У��ͣ����
	DWORD dwVideoBitrate;							//��Ƶ���� 0-100K��1-128K��2-256K��3-512K��4-1M��5-2M��6-3M��7-4M
	DWORD dwVideoFrameRate;							//֡��		2 �� 25
}STRUCT_SDVR_COMPRESSION, *LPSTRUCT_SDVR_COMPRESSION;

typedef struct
{
	uint8_t byChannel;									//ͨ����   //uint8_t
	DWORD dwSize;									//�ṹ���С
	uint8_t byRecordType;								//0x0:�ֶ�¼��0x1:��ʱ¼��0x2:�ƶ���⣬0x3:������0x0f:��������
	STRUCT_SDVR_COMPRESSION stRecordPara;			//¼��������������
	STRUCT_SDVR_COMPRESSION stNetPara;				//����������������
}STRUCT_SDVR_COMPRESSINFO, *LPSTRUCT_SDVR_COMPRESSINFO;

//¼�����
typedef struct
{
	STRUCT_SDVR_SCHEDTIME stRecordTime;				//¼��ʱ��
	uint8_t byRecordType;								//����	
	char reservedData[3];
}STRUCT_SDVR_RECORDSCHED, *LPSTRUCT_SDVR_RECORDSCHED;

typedef struct
{
	uint16_t wAllDayRecord;								//����	
	uint8_t byRecordType;								//����	
	char reservedData;
}STRUCT_SDVR_RECORDDAY, *LPSTRUCT_SDVR_RECORDDAY;

typedef struct
{
	uint8_t byChannel;									//ͨ����
	DWORD dwSize;									//�ṹ��С
	DWORD dwRecord;									//�Ƿ�¼�� 0-�� 1-�� 
	STRUCT_SDVR_RECORDDAY stRecAllDay[MAX_DAYS];	//����
	STRUCT_SDVR_RECORDSCHED stRecordSched[MAX_DAYS][MAX_TIMESEGMENT];//ʱ���
	DWORD dwPreRecordTime;							//����	
}STRUCT_SDVR_RECORDINFO, *LPSTRUCT_SDVR_RECORDINFO;

//��̨Э��
typedef struct
{
	int32_t nPTZNum;
	char szPTZProtocol[MAXPTZNUM][10];
}STRUCT_SDVR_PTZTYPE, *LPSTRUCT_SDVR_PTZTYPE;

//����������(��̨����)
typedef struct	{
	uint8_t byChannel;
	DWORD dwSize;
	DWORD dwBaudRate;								//������(bps)// 50 75 110 150 300 600 1200 2400 4800 9600 19200 38400 57600 76800 115.2k 
	uint8_t byDataBit;									//����λ 5 6 7 8
	uint8_t byStopBit;									//ֹͣλ 1 2 
	uint8_t byParity;									//����
	uint8_t byFlowcontrol;								//����
	uint16_t wDecoderType;								//0-unknow 1-RV800  2-TOTA120 3-S1601 4-CLT-168 5-TD-500  6-V1200 7-ZION 8-ANT 9-CBC 10-CS850A 
													//11-CONCORD 12-HD600 13-SAMSUNG 14-YAAN 15-PIH 16-MG-CS160 17-WISDOM 18-PELCOD1 19-PELCOD2 20-PELCOD3 
													//21-PELCOD4 22-PELCOP1 23-PELCOP2 24-PELCOP3 25-Philips 26-NEOCAM  27-ZHCD 28-DongTian 29-PELCOD5 30-PELCOD6
													//31-Emerson 32-TOTA160 33-PELCOP5
	uint16_t wDecoderAddress;							//��������ַ:0 - 255 
	int32_t byUarType;   //�������RS-485����0; sdi-485 1; TVI485 2�������TVI485���Ǵ��ڣ�ֻ��һ��ͨѶ����
	uint8_t bySetPreset[MAX_PRESET - 4];					//����
	uint8_t bySetCruise[MAX_PRESET];					//����
	uint8_t bySetTrack[MAX_PRESET];					//����
}STRUCT_SDVR_DECODERINFO, *LPSTRUCT_SDVR_DECODERINFO;

//�����������
typedef struct
{
	uint8_t byChannel;									//���
	DWORD dwSize;									//�ṹ���С
	uint8_t szAlarmInName[NAME_LEN];					//����ͨ����
	uint8_t byAlarmType;								//����
	uint8_t byAlarmInHandle;							//�Ƿ��� 0-1
	STRUCT_SDVR_HANDLEEXCEPTION struAlarmHandleType;//����ʽ 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//����ʱ��
	uint8_t byRelRecordChan[MAX_CHANNUM];				//����������¼��ͨ��,Ϊ1��ʾ������ͨ��
	uint8_t byEnablePreset[MAX_CHANNUM];				//�Ƿ����Ԥ�õ� ����byEnablePreset[0]���ж�;
	uint8_t byPresetNo[MAX_CHANNUM];					//���õ���̨Ԥ�õ����,һ������������Ե��ö��ͨ������̨Ԥ�õ�, 0xff��ʾ������Ԥ�õ� 
	uint8_t byEnableCruise[MAX_CHANNUM];				//����
	uint8_t byCruiseNo[MAX_CHANNUM];					//����
	uint8_t byEnablePtzTrack[MAX_CHANNUM];				//���� 
	uint8_t byPTZTrack[MAX_CHANNUM];					//����
	uint8_t byRecordTm;								//����¼��ʱ�� 1-99�� 
}STRUCT_SDVR_ALARMININFO, *LPSTRUCT_SDVR_ALARMININFO;

//�����������(��չ)
typedef struct	{
	uint8_t byChannel;									//���
	DWORD dwSize;									//�ṹ���С
	uint8_t sAlarmInName[NAME_LEN];					//����ͨ����
	uint8_t byAlarmType;								//����
	uint8_t byAlarmInHandle;							//�Ƿ��� 0-1
	STRUCT_SDVR_HANDLEEXCEPTION_EX struAlarmHandleType;//����ʽ 
	STRUCT_SDVR_SCHEDTIME struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//����ʱ��
	uint8_t byRelRecordChan[MAX_CHANNUM_EX];			//����������¼��ͨ��,Ϊ1��ʾ������ͨ��
	uint8_t byEnablePreset[MAX_CHANNUM_EX];			//�Ƿ����Ԥ�õ� ����byEnablePreset[0]���ж�;
	uint8_t byPresetNo[MAX_CHANNUM_EX];				//���õ���̨Ԥ�õ����,һ������������Ե��ö��ͨ������̨Ԥ�õ�, 0xff��ʾ������Ԥ�õ� 
	uint8_t byEnableCruise[MAX_CHANNUM_EX];			//����
	uint8_t byCruiseNo[MAX_CHANNUM_EX];				//����
	uint8_t byEnablePtzTrack[MAX_CHANNUM_EX];			//���� 
	uint8_t byPTZTrack[MAX_CHANNUM_EX];				//����
	uint8_t byRecordTm;								//����¼��ʱ�� 1-99�� 
}STRUCT_SDVR_ALARMININFO_EX, *LPSTRUCT_SDVR_ALARMININFO_EX;

//�����������
typedef struct
{
	uint8_t byChannel;									//���
	DWORD dwSize;									//�ṹ��С
	uint8_t sAlarmOutName[NAME_LEN];					//���� 
	DWORD dwAlarmOutDelay;							//�������ʱ�� ��λ��
	uint8_t byEnSchedule;								//�����������ʱ�伤�� 0-���� 1-���� 
	STRUCT_SDVR_SCHEDTIME struAlarmOutTime[MAX_DAYS][MAX_TIMESEGMENT];// �����������ʱ��� 				
}STRUCT_SDVR_ALARMOUTINFO, *LPSTRUCT_SDVR_ALARMOUTINFO;

//�û�Ȩ��
typedef struct
{
	uint8_t sUserName[NAME_LEN];						//�û��� 
	uint8_t sPassword[PASSWD_LEN];						//���� 
	DWORD dwLocalRight[MAX_RIGHT];					//����Ȩ�� 
	DWORD dwRemoteRight[MAX_RIGHT];					//Զ��Ȩ�� 
	/*���� 0: ͨ��Ȩ��*/
	/*���� 1: ��ʾ����*/
	/*���� 2: ¼�����*/
	/*���� 3: ��ʱ¼��*/
	/*���� 4: �ƶ�¼��*/
	/*���� 5: ����¼��*/
	/*���� 6: �������*/
	/*���� 7: ��̨����*/
	/*���� 8: �洢����*/
	/*���� 9: ϵͳ����*/
	/*���� 10: ��Ϣ��ѯ*/
	/*���� 11: �ֶ�¼��*/
	/*���� 12: �ط�*/
	/*���� 13: ����*/
	/*���� 14: ��Ƶ����*/
	/*���� 15: �������*/
	/*���� 16: Զ��Ԥ��*/
	DWORD dwUserIP;									//�û�IP��ַ(Ϊ0ʱ��ʾ�����κε�ַ) 
	uint8_t byMACAddr[MACADDR_LEN];					//�����ַ 	
}STRUCT_SDVR_USER_INFO,*LPSTRUCT_SDVR_USER_INFO;

//�û�Ȩ��(��չ)
typedef struct
{
	uint8_t sUserName[NAME_LEN];						//�û���
	uint8_t sPassword[PASSWD_LEN];						//����
	uint8_t dwLocalRight[MAX_RIGHT];					//����Ȩ��
	uint8_t LocalChannel[MAX_CHANNUM_EX];				//����ͨ��Ȩ��
	uint8_t dwRemoteRight[MAX_RIGHT];					//Զ��Ȩ��
	uint8_t RemoteChannel[MAX_CHANNUM_EX];				//Զ��ͨ��Ȩ��
	/*���� 0: ͨ��Ȩ��*/
	/*���� 1: ��ʾ����*/
	/*���� 2: ¼�����*/
	/*���� 3: ��ʱ¼��*/
	/*���� 4: �ƶ�¼��*/
	/*���� 5: ����¼��*/
	/*���� 6: �������*/
	/*���� 7: ��̨����*/
	/*���� 8: �洢����*/
	/*���� 9: ϵͳ����*/
	/*���� 10: ��Ϣ��ѯ*/
	/*���� 11: �ֶ�¼��*/
	/*���� 12: �ط�*/
	/*���� 13: ����*/
	/*���� 14: ��Ƶ����*/
	/*���� 15: �������*/
	/*���� 16: Զ��Ԥ��*/
	DWORD dwUserIP;									//�û�IP��ַ(Ϊ0ʱ��ʾ�����κε�ַ) 
	uint8_t byMACAddr[MACADDR_LEN];					//�����ַ 	
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
	char sDNSUser[INFO_LEN];						//DNS�˺�
	char sDNSPassword[INFO_LEN];					//DNS�˺�
	char sDNSAddress[INFO_SEQ][INFO_LEN];			//DNS������ַ
	uint8_t sDNSALoginddress;							//DNS������ַ��sDNSAddress�����е�ָ��������ַ������
	uint8_t sDNSAutoCon;								//DNS�Զ�����
	uint8_t sDNSState;									//DNS��½  0-ע�� 1-��½
	uint8_t sDNSSave;									//DNS��Ϣ����
	uint16_t sDNServer;									//0-- hanbang.org.cn 1--oray.net 2--dyndns.com
	uint16_t reserve;									//1--����������0--������
}STRUCT_SDVR_DNS, *LPSTRUCT_SDVR_DNS;

//DNS(��չ)
typedef struct	{
	DWORD dwSize;
	char sDNSUser[INFO_LEN];							//DNS�˺�
	char sDNSPassword[INFO_LEN];					//DNS�˺�
	char sDNSAddress[INFO_SEQ][INFO_LEN];	//DNS������ַ
	uint8_t sDNSALoginddress;							//DNS������ַ��sDNSAddress�����е�ָ��������ַ������
	uint8_t sDNSAutoCon;									//DNS�Զ�����
	uint8_t sDNSState;										//DNS��½  0-ע�� 1-��½
	uint8_t sDNSSave;											//DNS��Ϣ����
	uint16_t sDNServer;             // 0--hanbang.org.cn 1--oray.net 2--dyndns.com 3--no-ip.com
	// 4--ddns.hbgk.net (��չ) 5--www.meibu.com 6--freedns.afraid.org 7--multi.super-ddns.com
	uint8_t  autoregist;				//���ڱ�ʾDDNS�Զ�ע��ʱ�û����Ƿ�Ĭ�ϣ�0-��Ĭ�ϣ�1-Ĭ��
	uint8_t  revserse;					//����
	uint8_t sDNSname[128];		//����������
}STRUCT_SDVR_DNS_EX, *LPSTRUCT_SDVR_DNS_EX;

//PPPoE
typedef struct
{
	DWORD dwSize;
	uint8_t szPPPoEUser[INFO_LEN];						//PPPoE�û���
	char szPPPoEPassword[INFO_LEN];					//PPPoE����
	uint8_t byPPPoEAutoCon;							//PPPoE�Զ�����
	uint8_t byPPPoEState;								//PPPoE��½  0-ע�� 1-��½
	uint8_t byPPPoESave;								//DNS��Ϣ����
	char reservedData;
}STRUCT_SDVR_PPPoE, *LPSTRUCT_SDVR_PPPoE;

//ƽ̨��Ϣ
typedef struct
{
	char	szServerIP[16];							//������IP��ַ
	DWORD	nPort;									//�������˿ں�
	char	puId[NAME_LEN];							//����ID��
	DWORD	nInternetIp ;
	DWORD	nVideoPort;
	DWORD	nTalkPort;
	DWORD	nCmdPort;
	DWORD	nVodPort;
	DWORD	tran_mode;								//1 ������  0 ������
	DWORD ftp_mode;									//��FTP��ʽ�������Ĵ洢 1 ���� 0 �ر�
	DWORD max_link;									//��������� 0 - 32
}STRUCT_SDVR_SERVERCFG, *LPSTRUCT_SDVR_SERVERCFG;

//ƽ̨��Ϣ(��չ)
typedef struct
{
	uint8_t szRemoteIP[16];							//Զ��IP��ַ
	uint8_t szLocalIP[16];								//����IP��ַ
	uint8_t szLocalIPMask[16];							//����IP��ַ����
	uint8_t szUsername[NAME_LEN];						//�û���
	uint8_t szPassword[PASSWD_LEN];					//����
	uint8_t byPPPMode;									//PPPģʽ, 0��������1������	
	uint8_t byRedial;									//�Ƿ�ز� ��0-��,1-��
	uint8_t byRedialMode;								//�ز�ģʽ,0-�ɲ�����ָ��,1-Ԥ�ûز�����
	uint8_t byDataEncrypt;								//���ݼ���,0-��,1-��
	DWORD dwMTU;									//MTU
	uint8_t sTelephoneNumber[PHONENUMBER_LEN];			//�绰����
}STRUCT_SDVR_PPPCFG, *LPSTRUCT_SDVR_PPPCFG;

//������Ϣ
typedef struct
{
	uint8_t byChannel;
	DWORD dwSize;
	DWORD dwBaudRate;								//������(bps)  
	uint8_t byDataBit;									//�����м�λ 5��8 
	uint8_t byStopBit;									//ֹͣλ 1-2 
	uint8_t byParity;									//У�� 0����У�飬1����У�飬2��żУ��;
	uint8_t byFlowcontrol;								//0���ޣ�1��������,2-Ӳ����
	DWORD dwWorkMode;								//����
	STRUCT_SDVR_PPPCFG struPPPConfig;				//����
}STRUCT_SDVR_SERIALINFO, *LPSTRUCT_SDVR_SERIALINFO;

//Զ��¼���ļ���ѯ�͵㲥����
typedef struct
{
	DWORD dwYear;									//��
	DWORD dwMonth;									//��
	DWORD dwDay;									//��
	DWORD dwHour;									//ʱ
	DWORD dwMinute;									//��
	DWORD dwSecond;									//��
}STRUCT_SDVR_TIME, *LPSTRUCT_SDVR_TIME;

//��ʷ��־��Ϣ
typedef struct
{
	uint8_t byChannel;									//ͨ����
	DWORD dwFileType;								//�ļ�����(0-ȫ��,1-��ʱ¼��,2-�ƶ����,3-��������,4-�����)	
	STRUCT_SDVR_TIME stStartTime;					//¼��ʼʱ��
	STRUCT_SDVR_TIME stStopTime;					//¼�����ʱ��
}STRUCT_SDVR_FILE_FIND, *LPSTRUCT_SDVR_FILE_FIND;

//��ʷ��־��Ϣ(��չ)
typedef struct
{
	char sFileName[100];							//�ļ���
	STRUCT_SDVR_TIME stStartTime;					//�ļ��Ŀ�ʼʱ��
	STRUCT_SDVR_TIME stStopTime;					//�ļ��Ľ���ʱ��
	DWORD dwFileSize;								//�ļ��Ĵ�С
}STRUCT_SDVR_FILE_DATA, *LPSTRUCT_SDVR_FILE_DATA;

//����
typedef struct 
{
	uint8_t year;										//��
	uint8_t month;										//��
	uint8_t day;										//��
	uint8_t channel;									//ͨ��
	uint8_t bgn_hour;									//��ʼʱ
	uint8_t bgn_minute;								//��ʼ��
	uint8_t bgn_second;								//��ʼ��
	uint8_t end_hour;									//����ʱ
	uint8_t end_minute;								//������
	uint8_t end_second;								//������
}TREC_BACKUPTIME, *LPTREC_BACKUPTIME;



//Զ�̷���������״̬
typedef struct
{
	DWORD dwVolume;				//Ӳ�̵�������MB��
	DWORD dwFreeSpace;			//Ӳ�̵�ʣ��ռ䣨MB��
	DWORD dwHardDiskState;		//Ӳ��״̬��dwVolume��ֵʱ��Ч�� 0-���� 1-���̴��� 2-�ļ�ϵͳ����
}STRUCT_SDVR_DISKSTATE, *LPSTRUCT_SDVR_DISKSTATE;

//ͨ��״̬
typedef struct
{
	uint8_t byRecordStatic;							//ͨ���Ƿ���¼��,0-��¼��,1-¼��
	uint8_t bySignalStatic;							//���ӵ��ź�״̬,0-����,1-�źŶ�ʧ
	uint8_t byHardwareStatic;							//����
	char reservedData;
	DWORD dwBitRate;								//ʵ������
	DWORD dwLinkNum;								//�ͻ������ӵĸ���
	DWORD dwClientIP[MAX_LINK];						//����
}STRUCT_SDVR_CHANNELSTATE, *LPSTRUCT_SDVR_CHANNELSTATE;

//����״̬
typedef struct
{
	DWORD dwDeviceStatic;									//����
	STRUCT_SDVR_DISKSTATE  stHardDiskStatic[MAX_DISKNUM];	//Ӳ��״̬
	STRUCT_SDVR_CHANNELSTATE stChanStatic[MAX_CHANNUM];		//ͨ����״̬
	DWORD byAlarmInStatic;									//��������˿ڵ�״̬,��λ��ʾ
	DWORD byAlarmOutStatic;									//��������˿ڵ�״̬,��λ��ʾ
	DWORD dwLocalDisplay;									//����
}STRUCT_SDVR_WORKSTATE, *LPSTRUCT_SDVR_WORKSTATE;

//����״̬(��չ)
typedef struct
{
	DWORD dwDeviceStatic;									//����
	STRUCT_SDVR_DISKSTATE stHardDiskStatic[MAX_DISKNUM];	//Ӳ��״̬
	STRUCT_SDVR_CHANNELSTATE stChanStatic[MAX_CHANNUM_EX];	//ͨ����״̬
	DWORD byAlarmInStatic[MAX_ALARMIN_EX];					//�����˿ڵ�״̬ ��λ��ʾ
	DWORD byAlarmOutStatic[MAX_ALARMOUT_EX];				//��������˿ڵ�״̬ ��λ��ʾ
	DWORD dwLocalDisplay;									//����
}STRUCT_SDVR_WORKSTATE_EX, *LPSTRUCT_SDVR_WORKSTATE_EX;

//�������״̬
typedef struct
{
	uint8_t byAlarm;											//�������״̬ 0-������ 1-����
	uint16_t wAlarm;											//�������״̬ bit0-15����16���������, 0-״̬���� 1-ִ��byAlarm����
}STRUCT_SDVR_ALARMOUT, *LPSTRUCT_SDVR_ALARMOUT;

//Ԥ�õ�
typedef struct
{
	DWORD  byChannel;										//����ͨ��
	uint16_t Preset[PRESETNUM];
	uint16_t PresetPoll;										//��Ԥ�õ���Ѳ������رձ�ʾ1-255
	uint16_t presettime;										//��Ԥ�õ���Ѳʱ��			1-99
}STRUCT_DVR_PRESETPOLL, *LPSTRUCT_DVR_PRESETPOLL;

//��ʽ��Ӳ��

typedef struct  
{
			DWORD	dwCurrentFormatDisk;	//���ڸ�ʽ����Ӳ�̺�[0, 15]
			DWORD	dwCurrentDiskPos;		//���ڸ�ʽ����Ӳ�̽���[0, 100]
			DWORD	dwFormatState;			//��ʽ��״̬ 0-���ڸ�ʽ�� 1-��ʽ����� 2-��ʽ������ 3-Ҫ��ʽ���Ĵ��̲����� 4-��ʽ����;���� 5-�������ڱ�ʹ��
}STRUCT_SDVR_FORMAT,*LPSTRUCT_SDVR_FORMAT;


typedef struct 
{
	char server[32];										//������
	DWORD port;												//�˿�
	DWORD auto_enbale;										//����ntp����,0-��ʾ�ֶ�,1-��ʾ�Զ�
	DWORD server_index;										//������������
	DWORD sync_period;										//ͬ�����ڣ�
	DWORD sync_unit;										//ͬ�����ڣ�0-���� 1-Сʱ 2-�� 3-���� 4-��
	DWORD sync_time;										//����
	DWORD time_zone;										//ʱ��
	DWORD reserve;											//����
}STRUCT_SDVR_NTPCONFIG, *LPSTRUCT_SDVR_NTPCONFIG;

typedef struct 
{
	char szHost[MAX_SMTP_HOST];								//�����ʼ���SMTP�����������磺126�������smtp.126.com
	DWORD dwPort;											//�������˿ڣ������ʼ�(SMTP)�˿ڣ�Ĭ��ֵ25
	char szUser[32];										//�ʼ��û���
	char szPwd[32];											//�ʼ��û�����
	char szSend_addr[MAX_SMTP_HOST];						//FROM���ʼ���ַ
	char szRecv_addr[MAX_SMTP_ADDR];						//TO���ʼ���ַ������Ƕ���ʼ���ַ����';'����
	DWORD dwSend_period;									//�ϴ�����,��λ(����)
	DWORD dwSnap_enable;									//�Ƿ�ץ���ϴ�
	char szReserve[MAX_STRING];								//����
}STRUCT_SDVR_SMTPCONFIG, *LPSTRUCT_SDVR_SMTPCONFIG;

typedef struct 
{
	DWORD  dwPoll_type;										//��ѵ���ͣ�0����ѵ��1��spot��Ѳ
	DWORD dwEnable;											//���ã�0-���ã�1-����
	DWORD dwPeriod;											//��ѵ�������λ��
	DWORD dwFormat;											//�����ʽ��0-0ff, 1-1,  4-2x2, 9-3x3, 16-4x4
	uint8_t byCh_list[MAX_CHANNUM_EX];
}STRUCT_SDVR_POLLCONFIG, *LPSTRUCT_SDVR_POLLCONFIG;

typedef struct 
{
	uint8_t byMatrix_channel[MAX_CHANNUM_EX];					//��Ƶ�����Ӧͨ�� ��1��ʼ��0xff��ʾ�ر�
	uint8_t byReserve[32];										//����λ
}STRUCT_SDVR_VIDEOMATRIX, *LPSTRUCT_SDVR_VIDEOMATRIX;

//��ʷ��Ƶ��־
typedef struct
{
	uint32_t uSecond : 6;			//��: 0~59
	uint32_t uMinute : 6;			//��: 0~59
	uint32_t uHour : 5;				//ʱ: 0~23
	uint32_t uDay : 5;				//��: 1~31
	uint32_t uMonth : 4;			//��: 1~12
	uint32_t nYear : 6;				//��: 2000~2063
}MFS_FIELD_TIME, *LPMFS_FIELD_TIME;

typedef union
{
	uint32_t nLogTime;
	MFS_FIELD_TIME stFieldTime;
}UNMFS_TIME, *LPUNMFS_TIME;

//��ѯ�����������Ƿ�ÿ�춼��¼���ļ�  
//0-��ѯʧ�ܣ�pc�˿����ٴβ�ѯ��1-��ѯ�ɹ�������ÿ�춼��¼�����ݣ�2-��ѯ�ɹ�������������һ��û��¼������
typedef struct
{
	uint8_t byChannel;
	uint8_t byType;		//�ֶ�¼��--0x01, ��ʱ¼��--0x02, �ƶ�¼��--0x03, ����¼��--0x04, ȫ��¼��--0xFF��0x00
	UNMFS_TIME unStartTime;
	UNMFS_TIME unEndTime;
	uint16_t dwStart;
	uint16_t dwNum;	//һ�β�ѯ�ĸ��������ڶ�����100 
	//Э����и����Ϊ�������ͣ�0-��������1-������ 
}STRUCT_SDVR_RECFIND, *LPSTRUCT_SDVR_RECFIND;

//��־��¼
typedef struct
{
	UNMFS_TIME unStartTime;
	UNMFS_TIME unEndTime;
	uint32_t uLenght;
	uint8_t byChannel;
	uint8_t byType;
	uint8_t byReserve1;
	uint8_t byReserve2;
}MFS_RECSEG_INFO, *LPMFS_RECSEG_INFO;

typedef struct
{
	DWORD dwTotalNum;
	MFS_RECSEG_INFO stRecSeg[MAX_REC_NUM];
}STRUCT_SDVR_RECINFO, *LPSTRUCT_SDVR_RECINFO;

//�㲥��ʷ��Ƶ
typedef struct
{
	uint8_t byChannel;					//ͨ����(0,1,2,3....)
	uint8_t byType;							//�㲥������	0x01-�ֶ�¼��,0x02-��ʱ¼��,0x04-�ƶ�¼��,0x08-����¼��,0x09-����¼��,0xff-����¼��
	UNMFS_TIME unBegTime;		//�㲥��ʼʱ��
	uint8_t byReserve1;					//����1
	uint8_t byReserve2;					//����2
}STRUCT_SDVR_VOD, *LPSTRUCT_SDVR_VOD;

//�㲥��ʷ��Ƶ��չ(ָ��ʱ���)
typedef struct
{
	uint8_t byChannel;					//ͨ����(0,1,2,3....)
	uint8_t byType;					//�㲥������
	UNMFS_TIME unBegTime;			//�㲥��ʼʱ��
	UNMFS_TIME unEndTime;			//�㲥����ʱ��
	uint8_t streamtype;				//�������ͣ�0-��������1-������ 
	uint8_t byReserve2;				//����2
}STRUCT_SDVR_VOD_EX, *LPSTRUCT_SDVR_VOD_EX;

//�������ӵ㲥��ʷ��Ƶ
typedef struct
{
	DWORD msgid; //��ϢID,��id ��ƽ̨���ɣ�DVR ԭ�ⲻ�����أ�msgid ����ƽ̨���������½�����socket ���ӡ�ʹƽ̨����֪����socket ���Ӷ�Ӧ�����
	uint8_t byChannel; //ͨ����[0, n-1], n:ͨ����
	uint8_t byType; //�㲥����
	uint8_t reserve1[2]; //����
	UNMFS_TIME unBegTime; //�㲥��ʼʱ�䣬ͬ¼���ѯ������Ϣ�ж��壬���-2000
	UNMFS_TIME unEndTime; //�㲥����ʱ�䣬ʱ����һ��֮��
	uint8_t streamtype;			//�������ͣ�0-��������1-������
	uint8_t reserve2[15];			//����
} STRUCT_SDVR_VOD_EX1;

typedef struct
{
	DWORD msgid; //��ϢID,��id ��ƽ̨���ɣ�DVR ԭ�ⲻ�����أ�msgid ����ƽ̨���������½�����socket ���ӡ�ʹƽ̨����֪����socket ���Ӷ�Ӧ�����
	DWORD dwVodID; //�㲥id
	uint8_t streamtype;      //�������ͣ�0-��������1-������
	uint8_t reserve[15];     //����
}STRUCT_SDVR_VOD_ANS_EX;

//�㲥����
typedef struct
{
	DWORD dwVodID;					//�طž��
	uint8_t byCtrl;					//0-���� 1-��ͣ 2-��� 3-���� 4-֡�� 5-����
	uint8_t bySpeed;					//������������ٶ�
	uint8_t bySeekFlag;				//����������־
	uint8_t byPecent;					//���Ȱٷֱ�
}STRUCT_SDVR_VODCTL, *LPSTRUCT_SDVR_VODCTL;

//������ʷ��Ƶ
typedef struct
{
	uint8_t byChannel;					//ͨ����
	uint8_t byType;							//��������	0x00-�ֶ�¼��,0x01-��ʱ¼��,0x02-�ƶ�¼��,0x03-����¼��,0x04-����¼��,0xff-����¼��
	UNMFS_TIME unBegTime;		//��ʼʱ��
	UNMFS_TIME unEndTime;		//����ʱ��
}STRUCT_SDVR_BACKINFO, *LPSTRUCT_SDVR_BACKINFO;

typedef struct
{
	DWORD msgid; //��ϢID,��id ��ƽ̨���ɣ�DVR ԭ�ⲻ�����أ�msgid ����ƽ̨���������½�����socket ���ӡ�ʹƽ̨����֪����socket ���Ӷ�Ӧ�����
	uint8_t byChannel; //ͨ����
	uint8_t byType; //�������ͣ�0-�ֶ���1-��ʱ��2-�ƶ���3-̽ͷ������0x0f-����¼��
	uint16_t file_index; //�ͻ������ص��ļ��б�������ţ���0 ��ʼ
	DWORD file_offset; //�ļ�ƫ�ƴ�С���տ�ʼΪ0
	UNMFS_TIME unBegTime; //��ʼʱ��
	UNMFS_TIME unEndTime; //����ʱ��
	uint8_t streamtype;   //�������ͣ�0-��������1-������
	uint8_t reserve[27];	//����
}STRUCT_SDVR_BACKINFO_EX2;

typedef struct
{
	DWORD msgid; //��ϢID,��id��ƽ̨���ɣ�DVRԭ�ⲻ�����أ�msgid����ƽ̨���������½�����socket���ӡ�ʹƽ̨����֪����socket���Ӷ�Ӧ�����
	uint8_t reserve[4]; //����
}STRUCT_SDVR_BACKINFO_RSP;

//�����ļ�����
typedef struct
{
	DWORD dwChannel;				//ͨ����
	DWORD dwFileLen;				//���ļ�����  ��BΪ��λ
	UNMFS_TIME unBegTime;			//¼���ļ���ʼʱ��
	UNMFS_TIME unEndTime;			//¼���ļ�����ʱ��
	DWORD dwTotalFileLen;			//���ļ�����  ��KBΪ��λ
}STRUCT_SDVR_FILEINFO, *LPSTRUCT_SDVR_FILEINFO;

//�������������Խ�
typedef struct
{
	DWORD msgid; //��ϢID,��id��ƽ̨���ɣ�DVRԭ�ⲻ�����أ�msgid����ƽ̨���������½�����
	//socket���ӡ�ʹƽ̨����֪����socket���Ӷ�Ӧ�����
	uint8_t reserve[4]; //����
}STRUCT_SDVR_VOICE;

typedef struct
{
	DWORD msgid; //��ϢID,��id��ƽ̨���ɣ�DVRԭ�ⲻ�����أ�msgid����ƽ̨���������½�����socket
	//���ӡ�ʹƽ̨����֪����socket���Ӷ�Ӧ�����
	uint8_t format; //��Ƶѹ����ʽ��������"��Ƶ����ѹ����ʽ"��,Ĭ�ϵı����ʽΪPCM
	uint8_t bitspersample; //��ʾ����λ��(��λbit)��ȡֵ��Χ(1~255)��Ĭ����16
	uint8_t samplerate; //��ʾ������(��λK[1000])
	uint8_t reserved[7]; //����
	unsigned short msglen; //����
} STUCT_SDVR_VIOCE_S;

//��������ץͼ
typedef struct
{
	DWORD msgid; //��ϢID����id ��ƽ̨���ɣ�DVR ԭ�ⲻ������
	uint8_t Channel; //ͨ����[0, n-1],n:ͨ����
	uint8_t pic_format; //ͼƬ��ʽ��0--jpg��1-bmp��Ŀǰֻ����jpg ��ʽ
	uint8_t reserve1[10]; //����
	DWORD len; //ͼƬ���ݳ���
	uint8_t reserve2[16]; //����
	char data[]; //ͼƬ����
}STRUCT_SDVR_GET_PHOTO;

//�豸��־
typedef struct
{
  uint16_t wYear;
  uint8_t byMonth;
  uint8_t byDay;
  uint16_t wStart;
  uint16_t wnum;  
}STRUCT_SDVR_REQLOG, *LPSTRUCT_SDVR_REQLOG;

//8000�豸��־��ʽ
typedef struct
{
  UNMFS_TIME		stTime;
  uint8_t				byType;			//����
  uint8_t				byOperate;		//������
  char				szAccount[18];	//�û�
  uint32_t				nIpAddr;		//0-Local other-ip
  uint32_t				nInput;			//�¼��������
  uint32_t				nOutput;
  char				szData[28];		//������Ϣ
}MFS_LOG_DAT, *PMFS_LOG_DAT;

typedef struct
{
  DWORD  dwTotalNum;
  MFS_LOG_DAT stDevLogData[MAX_LOGO_NUM];
}STRUCT_SDVR_LOGINFO, *LPSTRUCT_SDVR_LOGINFO;

//7000T�豸��־��ʽ
typedef MFS_FIELD_TIME SYSTIME;
typedef struct
{
	SYSTIME stTime;					//��¼ʱ��
	char PriType;					//������
	char SecType;					//������
	char Param;						//��������
	char Channel;					//ͨ����
	uint32_t nHost;				//����IP��0��ʾ����
	char szUser[17];				//�û���
	char HardDriver;				//Ӳ�̺�
	char AlarmIn;					//��������
	char AlarmOut;					//�������
	char Reserve[32];				//��������
}LOG_DAT, *PLOG_DAT;

typedef struct 
{
	DWORD dwTotalNum;
	LOG_DAT stLogData[MAX_LOGO_NUM];
}STRUCT_SDVR_LOGINFO_7KT, *LPSTRUCT_SDVR_LOGINFO_7KT;


//0ͨ��
typedef enum
{
    NET_AUSTREAM_DISABLE, //��Ƶ��
    NET_AUSTREAM_ENABLE,  //������
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
    NET_BITRATE_CHANGE,		//������
    NET_BITRATE_NOCHANGE,	//������
}NET_BITRATETYPE_E;

typedef enum
{
    NET_VQUALITY_BEST=0,	//���
    NET_VQUALITY_BETTER,	//�ϸ�
    NET_VQUALITY_GOOD,		//��
    NET_VQUALITY_NORMAL,	//��
    NET_VQUALITY_BAD,		//��
    NETT_VQUALITY_WORSE		//���
}NET_VQUALITY_E;

typedef struct
{
    NET_AUSTREAMADD_E byStreamType;		//��Ƶ������
    NET_RESOLUTION_E byResolution;		//��Ƶ�ֱ���
    NET_BITRATETYPE_E byBitrateType;	//��������
    NET_VQUALITY_E byPicQuality;		//ͼ������
    DWORD dwVideoBitrate;				//��Ƶ���� ʵ������
    uint16_t dwVideoFrameRate;				//֡�� PAL 2-30 N 2-30
    uint16_t quant;							//�������� 1-31
}STRUCT_DVR_VENC_CONFIG, *LPSTUCT_DVR_VENC;

typedef struct 
{
    DWORD 					enable;
    uint8_t 					chlist[MAX_CHANNUM_EX];
    STRUCT_DVR_VENC_CONFIG 	venc_conf; 
    DWORD 					reserve;
}STRUCT_DVR_ZERO_VENC_CONFIG, *LPSTRUCT_DVR_ZERO_VENC;

//��Ƶ�ڵ�����״̬
typedef struct
{
	uint8_t  byChannel;
	DWORD dwVCoverEnable;          //�ڵ�����ʹ�ܣ�1-���ã�0-������
	DWORD dwSensorOut;			   //���������������λ��ʾ��1-������0-������
}STRUCT_SDVR_VCOVER_ALM,*LPSTRUCT_SDVR_VCOVER_ALM;

typedef struct	
{
	uint8_t  cbStreamType;      	//�������� 1-���� 2-���� 3-��������
	uint8_t  cbReserve[3];			//����
}STRUCT_SDVR_REQIPCWORKPARAM,*LPSTRUCT_SDVR_REQIPCWORKPARAM;

typedef struct	
{
	uint8_t	cbStartHour;  		//��ʼСʱ 0-23
	uint8_t	cbStartMin;  		//��ʼ���� 0-59
	uint8_t	cbStopHour; 		//����Сʱ  0-23
	uint8_t	cbStopMin;  		//��������  0-59
}STRUCT_SDVR_IPCSCHEDTIME,*LPSTRUCT_SDVR_IPCSCHEDTIME; 

typedef struct
{
	uint16_t 	wLightRange;		//ICR�����л��ٽ�ֵ��ȡֵ��Χ[80,120];
	uint16_t 	wEnable;			//0-��֧��1--����ֵ��Ч  2--ʱ�����Ч
	STRUCT_SDVR_IPCSCHEDTIME stSchedTime[2];
}STRUCT_SDVR_ICRTIME,*LPSTRUCT_SDVR_ICRTIME;
//��wEnable = 1��wLightRange��Ч������������Ϊ��ת���ж�������
//��wEnable = 2��stSchedTime[2]��Ч������stSchedTime[2]����ʱ��ε�ʱ����IPC���óɺڰ�ģʽ��
//STRUCT_SDVR_SCHEDTIME stSchedTime[2];
//��������ʱ����ǿ��ǵ��û������Ϻ���������ʱ������Ҫ���óɺڰ�ģʽ�����Բ�������ʱ��Σ��û�������
//��һ��ʱ��Σ�cbStartHour��cbStartMin��cbStopHour��cbStopMin����0ʱ��ʱ�����Ч��;

typedef struct
{
	DWORD  dwShutterIndex; //��ǰ����ʱ������ֵ,��ʾ����dwShutterVal�е�λ�ã�����dwShutterIndex = 2��
	//��ǰ����ʱ��ΪdwShutterVal[2];
	DWORD  dwShutterVal[32];//��ȡ����ʱ���֧�ֲ����б����磺dwShutterVal[0]= 2048
	//dwShutterVal[1]=4096��dwShutterVal[2]=0���ʾ���ݹ���2��ѡ�1/4096,1/2048��
	//
	//��dwShutterVal[0]Ϊ����ֵ��dwShutterVal[1]=0 ��dwShutterIndex = 0ʱ����ʾֻ��һ��ѡ�����ʱ��ΪdwShutterVal[0]��ֵ����dwShutterVal[0]Ϊ����ֵ��dwShutterVal[1]=0 ��dwShutterIndex !=0ʱ����ʾһ��ȡֵ��Χ������ʱ��ΪdwShutterIndex��ֵ
	//���磺��dwShutterVal[0]= 2048��dwShutterVal[1]=0��dwShutterIndex = 0ʱ����ʾֻ��һ��ѡ�����ʱ��Ϊ2048��
	//��dwShutterVal[0]= 2048�� dwShutterVal[1]=0��dwShutterIndex != 0ʱ��dwShutterIndexȡֵ��ΧΪ[1,2048]������ʱ��Ϊ��ǰdwShutterIndex��ֵ��
	//��dwShutterVal[0]=0ʱ����ʾ��֧��
}STRUCT_SDVR_SHUTTERVAL,*LPSTRUCT_SDVR_SHUTTERVAL;

typedef struct
{
	DWORD dwSceneIndex;	//��ǰ��ͷ����ֵ����ʾ����cbSceneVal�е�λ�ã�����dwSceneIndex = 2��
	//��ǰ��ͷΪ��cbSceneVal[2] = ��JCD661 lens������cbSceneVal[x] =��\0����ʾ�ܹ���
	//x�������
	uint8_t  cbSceneVal[8][32];	//�û���֧�ֵľ�ͷ����,//0 - Full Maual lens,1 - DC Iris lens, //2 - JCD661 lens,
	//3 - Ricom NL3XZD lens,4 - Tamron 18X lens���������Աȫ��Ϊ0����ʾ��֧��
} STRUCT_SDVR_SCENEVAL,*LPSTRUCT_SDVR_SCENEVAL;

typedef struct
{
	DWORD	dwResoluIndex;	//��ǰ�ֱ�������ֵ����ʾ����dwResolution�е�λ�ã�����dwResoluIndex= 2��
	//��ǰ�ֱ���ΪdwResolution[2]����ָ���ķֱ���
	DWORD	dwResolution[16];	//�û���֧�ֵķֱ��ʣ���DWORD����ʾ֧�ֵķֱ��ʣ����磺
	//dwResolution[0]=0x07800438�������ֽڣ�0x0780=1920���������ֽڣ�0x0438=1080����
} STRUCT_SDVR_RESOLUTION,*LPSTRUCT_SDVR_RESOLUTION; //�������Աȫ��Ϊ0����ʾ��֧��

typedef struct
{
	DWORD dwAgcIndex;     //��ǰAGC������ֵ����ʾ����cbAgcVal�е�λ�ã�����cbAgcVal =2�����ʾAGC
		//ֵΪcbAgcVal[2]�е�ֵ��
	uint8_t  cbAgcVal[32];	  //AGC���Զ����棩��֧�ֲ����б�,��cbAgcVal[1]= 0ʱ��ʾcbAgcVal[0]�д洢����
	//һ��ȡֵ��Χ����cbAgcVal[0]= 128,���ʾȡֵ��ΧΪ��[1,128],��cbAgcVal[1]!=0
	//ʱ�����ʾcbAgcVal�����д洢���Ǿ����ֵ������ cbAgcVal[0]= 32��//cbAgcVal[1]=64�ȣ���cbAgcVal [x] =0��ʾ�ܹ���x�������
	//��cbAgcVal[0]Ϊ����ֵ��cbAgcVal[1]=0 ��dwAgcIndex = 0ʱ����ʾֻ��һ��ѡ�AGCʱ��ΪcbAgcVal [0]��ֵ����cbAgcVal[0]Ϊ����ֵ��cbAgcVal[1]=0 ��dwAgcIndex!=0ʱ����ʾһ��ȡֵ��Χ��AGCʱ��ΪdwAgcIndex��ֵ
	//���磺��cbAgcVal[0]= 32��cbAgcVal[1]=0��dwAgcIndex = 0ʱ����ʾֻ��һ��ѡ�AGCʱ��Ϊ32��
	//��cbAgcVal[0]= 32�� cbAgcVal[1]=0��dwAgcIndex!= 0ʱ��dwAgcIndexȡֵ��ΧΪ[1,32]��AGCʱ��Ϊ��ǰdwAgcIndex��ֵ��
	//��cbAgcVal [0]=0ʱ����ʾ��֧��

} STRUCT_SDVR_AGCVAL,*LPSTRUCT_SDVR_AGCVAL;

typedef struct
{
	uint8_t	cbMinFrameRate;	//�û���֧�ֵ���С����֡��ֵ;��ȡֵ��ΧΪ��1��2^8����ͬ��ֻ֧�ֻ�ȡ
	uint8_t	cbMaxFrameRate;	//�û���֧�ֵ�������֡��ֵ; ȡֵ��ΧΪ��1��2^8ֻ֧�ֻ�ȡ
	uint8_t	cbCurFrameRate;	//�û������õĵ�ǰ����֡��ֵ; ȡֵ��ΧΪ��1��2^8,֧���������ȡ
	uint8_t    cbreserve;		//����
} STRUCT_SDVR_FRAMERATE,*LPSTRUCT_SDVR_FRAMERATE;

typedef struct
{
	DWORD dwLength;			//�ṹ�峤��
	uint8_t  cbStreamEnable;   //�Ƿ�����ǰ����: 0-��֧��1-disable 2-enable
	uint8_t  cbStreamType;     //�������� 0-��֧��1-����2-���� 3-��������
	uint8_t  cbAudioEnable;    //��Ƶʹ�� 0-��֧�� 1-����Ƶ ,2-����Ƶ
	uint8_t  cbAntiFlicker;    //����˸����0-��֧��1-60HZ   2-50HZ
	STRUCT_SDVR_FRAMERATE  stFrameRate;		//����֡������;
	STRUCT_SDVR_SHUTTERVAL stShutterVal;	//������ز�����ȡ
	STRUCT_SDVR_SCENEVAL	stSceneVal;		//��ͷ��ز�����ȡ
	STRUCT_SDVR_RESOLUTION	stResolution;	//���������
	STRUCT_SDVR_AGCVAL		stAgcVal;		//Agc���
	DWORD	dwBitRateVal;		//��Ƶ����  0-��֧��1-100K 2-128K��3-256K��4-512K��5-1M��6-1.5M��7-2M��8-3M, 9-4M 10-5M��11-6M��12-7M��13-8M, 14-9M��15-10M��16-11 M��17-12M
	//����������ֵ��kbps����Ч��Χ 32~2^32,���ڵ���32����KΪ��λ��
	uint8_t	cbFoucusSpeedVal;	//��Ϊ0ʱ��ʾ��֧�ָù���
	uint8_t	cbDigitalFoucusVal;	// ��Ϊ0ʱ��ʾ��֧�ָù���
	uint8_t	cbImageTurnVal;		//��ǰͼ��ת���� //1-����ת,2-ˮƽ��ת 3-��ֱ��ת, 4-ˮƽ&��ֱ,0-��֧��
	uint8_t	cbBlackWhiteCtrlVal;//��ǰ�ڰ�ģʽ���� //1- Off, 2- On, 3�CAuto, 0-��֧��
	uint8_t	cbIRISCtrl;			//Iris control mode ��Ȧ����ģʽ���ã�1-Off,2-Basic, 3-Advanced,0-��֧��
	uint8_t	cbAutoFoucusVal;	//�Զ��Խ���1-�� 2-�� 0-��֧��
	uint8_t 	cbAWBVal;			//��ƽ�ⳡ��ģʽ���ã�1-auto_wide, 2-auto_normal, 3-sunny, 4-shadow, 5-indoor,
	//6-lamp, 7-FL1, 8-FL2,0-��֧��
	uint8_t 	cbA3Ctrl;			//3A����0-��֧��1-off; 2-Auto Exposure; 3-Auto White Balance; 4-both, (Auto Focus no support)

	STRUCT_SDVR_ICRTIME stICRTime;
	//��cbBlackWhiteCtrlVal = 3��3�CAuto����stICRTime���˹�Ƭ�л�ģʽ���ã����������ã���ȡ�������κ����
	//��֧�ֻ�ȡ��
	uint8_t	cbFNRSuppVal;		//��ǰ֡�������ã�1-��,2-��,0-��֧��
	uint8_t	cbStreamKindVal;	//��ǰ�������ͣ�0-��֧��1-������,2-������
	uint8_t	cbVideoOutKindVal;	//vout��Ƶ������ã�0-��֧��1-disable, 2-CVBS, 3-HDMI, 4-YPbPr�ȵ�
	uint8_t	cbWDRVal;			//��̬����, 1-�� 2-��, 0-��֧��
	uint8_t   cbColorMode;			//ɫ�ʷ������1-TV 2-PC��0-��֧��
	uint8_t   cbSharpNess;			//������ã�ȡֵ��ΧΪ��[1,255] ��0-��֧��
	uint8_t	cbPlatformType;		//Ĭ��Ϊ0-��֧��
	uint8_t	cbReserve[17];		//Ĭ��Ϊ0-��֧��
}STRUCT_SDVR_REIPCWORKPARAM,*LPSTRUCT_SDVR_REIPCWORKPARAM;

//�û�Ȩ��ģʽ
//typedef enum
//{
//	SYSNETAPI_USR_MODE_OLD = 1,			//���7000L��GUI���û�Ȩ��ģʽ
//	SYSNETAPI_USR_MODE_NEW,				//���7000L��GUI���û�Ȩ��ģʽ
//	SYSNETAPI_USR_MODE_HIGHRESOLUTION,	//��Ը��壬δ��
//}SYSNETAPI_USR_MODE;


//�û���Ϣ��չ1
//�ṹ��������1
//typedef struct 
//{
//	uint8_t dwLocalRight[32]; //����Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ�� 
//	/*����1-��������*/
//	/*����2-¼������*/
//	/*����3-�������*/
//	/*����4-��������*/
//	/*����5-��������*/
//	/*����6-��������*/
//	/*����7-¼��ط�*/
//	/*����8-ϵͳ����*/
//	/*����9-ϵͳ��Ϣ*/
//	/*����10-�������*/
//	/*����11-��̨����*/
//	/*����12-�ػ�����*/
//	/*����-13-USB����*/
//	/*����14-����*/
//	uint8_t LocalChannel[128]; //�����û���ͨ���Ĳ���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
//	uint8_t dwRemoteRight[32]; //Զ�̵�½�û����߱���Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ�� 
//	/*����1-Զ��Ԥ��*/
//	/*����2-��������*/
//	/*����3-Զ�̻ط�*/
//	/*����4-Զ�̱���*/
//	/*����5-�鿴��־*/
//	/*����6-�����Խ�*/
//	/*����7-Զ������*/
//	/*����8-Զ������*/
//	uint8_t RemoteChannel[128]; //�û�Զ�̵�½ʱ��ͨ�����߱���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
//}STRUCT_USERINFO, *LPSTRUCT_USERINFO;

typedef struct{
	uint8_t sUserName[NAME_LEN];		    //�û��� 
	uint8_t sPassword[32];					//����
	uint8_t dwLocalRight[MAX_RIGHT];	    //����Ȩ��
	uint8_t LocalChannel[MAX_CHANNUM];		//����ͨ��Ȩ��
	uint8_t dwRemoteRight[MAX_RIGHT];	    //Զ��Ȩ��
	uint8_t RemoteChannel[MAX_CHANNUM];	//Զ��ͨ��Ȩ��
	//���� 1: ��ʾ����
	//���� 2: ¼�����
	//���� 3: ��ʱ¼��
	//���� 4: �ƶ�¼��
	//���� 5: ����¼��
	//���� 6: �������
	//���� 7: ��̨����
	//���� 8: �洢����
	//���� 9: ϵͳ����
	//���� 10: ��Ϣ��ѯ
	//���� 11: �ֶ�¼��
	//���� 12: �ط�
	//���� 13: ����
	//���� 14: ��Ƶ����
	//���� 15: �������
	//���� 16: Զ��Ԥ��
	DWORD dwUserIP;				//�û�IP��ַ(Ϊ0ʱ��ʾ�����κε�ַ)	
	uint8_t byMACAddr[8];			//�����ַ	
}STRUCT_USERINFO, *LPSTRUCT_USERINFO;



//�û���Ϣ��չ1
//�ṹ��������2
//typedef struct 
//{
//	uint8_t dwLocalRight[32]; //����Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ��
//	/*���� 1: �ֶ�¼��*/
//	/*���� 2: �ֶ�����*/
//	/*���� 3: ¼��ط�*/
//	/*���� 4: ���ݹ���*/
//	/*���� 5: ���̹���*/
//	/*���� 6: ϵͳ�ػ�*/
//	/*���� 7: ϵͳ����*/
//	/*���� 8: ��̨����Ȩ��*/
//	/*���� 9: �������Ȩ��*/
//	/*���� 10: ��������*/ 
//	/*���� 11: �������*/ 
//	/*���� 12: ¼������*/
//	/*���� 13: ��ʱ¼��*/
//	/*���� 14����������*/ 
//	/*���� 15����������*/ 
//	/*���� 16����̨����*/ 
//	/*���� 17����������*/
//	/*���� 18��ϵͳ��Ϣ*/ 
//	/*���� 19��¼��״̬*/          
//	/*���� 20������״̬*/
//	/*���� 21������״̬*/
//	/*���� 22����־��ѯ*/ 
//	/*���� 23����������*/ 
//	/*���� 24���û�����*/ 
//	/*���� 25���ָ���������*/ 
//	/*���� 26������Ȩ��*/
//	/*���� 27����ʱ����*/ 
//	/*���� 28������¼��*/
//	uint8_t LocalChannel[128]; //�����û���ͨ���Ĳ���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
//	uint8_t dwRemoteRight[32]; //Զ�̵�½�û����߱���Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ��
//	/*���� 1: Զ��Ԥ��*/ 
//	/*���� 2: ��������*/
//	/*���� 3: Զ�̻ط�*/ 
//	/*���� 4: Զ�̱���*/
//	/*���� 5: �鿴��־*/ 
//	/*���� 6: �����Խ�*/
//	/*���� 7: Զ������*/
//	/*���� 8��Զ������*/
//	uint8_t RemoteChannel[128]; //�û�Զ�̵�½ʱ��ͨ�����߱���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
//}STRUCT_USERINFO_GUI, *LPSTRUCT_USERINFO_GUI;

typedef struct
{
	uint8_t sUserName[32];  		//�û��� �ԡ�\0�������ַ���
	uint8_t sPassword[32];  		//���� �ԡ�\0�������ַ���
	uint8_t dwLocalRight[32]; 		//����Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ��
	/*���� 1: �ֶ�¼��*//*���� 2: �ֶ�����*//*���� 3: ¼��ط�*//*���� 4: ���ݹ���*/
	/*���� 5: ���̹���*//*���� 6: ϵͳ�ػ�*//*���� 7: ϵͳ����*//*���� 8: ��̨����Ȩ��*//*���� 9: �������Ȩ��*//*���� 10: ��������*/ /*���� 11: �������*/ /*���� 12: ¼������*//*���� 13: ��ʱ¼��*/ 
	/*����14����������*/ /*���� 15����������*/ /*���� 16����̨����*/ /*���� 17����������*//*���� 18��ϵͳ��Ϣ*/ /*���� 19��¼��״̬*/          /*���� 20������״̬*/ /*���� 21������״̬*/
	/*���� 22����־��ѯ*/ /*���� 23����������*/ /*���� 24���û�����*/ /*���� 25���ָ���������*/ /*���� 26������Ȩ��*/ /*���� 27����ʱ����*/ /*���� 28������¼��*/
	uint8_t LocalChannel[128]; 	//�����û���ͨ���Ĳ���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
	uint8_t dwRemoteRight[32]; 	//Զ�̵�½�û����߱���Ȩ�� 1.����0δʹ�ã�2.ȡֵ��0-��Ȩ�ޣ�1-��Ȩ��
	/*���� 1: Զ��Ԥ��*/ /*���� 2: ��������*/ /*���� 3: Զ�̻ط�*/ /*���� 4: Զ�̱���*/ /*���� 5: �鿴��־*/ /*���� 6: �����Խ�*/ /*���� 7: Զ������*/ /*���� 8��Զ������*/
	uint8_t RemoteChannel[128]; 	//�û�Զ�̵�½ʱ��ͨ�����߱���Ȩ�ޣ����128��ͨ����0-��Ȩ�ޣ�1-��Ȩ��
	DWORD dwUserIP;				//�û���¼ʱpc����ip��ַ��Ϊ0��ʾ�κ�PC��������ʹ�ø��û���½��DVR�ϣ���Ϊ0��ʾֻ��ip��ַΪ�趨ֵ��pc���ſ���ʹ�ø��û���¼��DVR��
	uint8_t byMACAddr[8]; 			//�û���¼ʱPC����MAC��ַ��Ϊ0��ʾ�κ�PC��������ʹ�ø��û���½��DVR�ϣ���Ϊ0��ʾֻ��MAC��ַΪ�趨ֵ��PC���ſ���ʹ�ø��û���½��DVR��
}STRUCT_USERINFO_GUI, *LPSTRUCT_USERINFO_GUI;


//�û���Ϣ��չ1
//�ṹ��������3
//typedef struct 
//{
//	uint8_t grp_name[32]; //������
//	uint64_t local_authority[64]; // �����û�ʹ��Ȩ�ޣ�ÿλ����һ��ͨ��,bit0~bit63��ʾ0~63ͨ����ÿ���������һ��Ȩ�ޣ�
//	/*����0��ʵʱԤ��*/
//	/*����1���ֶ�¼��*/
//	/*����2��¼���ѯ�ط�*/
//	/*����3�����ݹ���*/
//	/*����4��¼�����*/
//	/*����5����̨����*/
//	/*����6����ͼ����*/
//	/*����7��ͨ������*/
//	/*����8����ʱ¼��*/
//	/*����9���ƶ����*/
//	/*����10����������*/
//	/*����11����������*/
//	/*����12����������*/
//	/*����13����������*/
//	/*����14����������*/
//	/*����15����Ϣ�鿴*/
//	/*����16����������*/
//	/*����17����������*/
//	/*����18����������*/
//	/*����19��ϵͳ�ػ�*/
//	/*����20������¼��*/
//	uint64_t remote_authority[64];//Զ��Ȩ�ޣ�ÿλ����һ��ͨ����bit0~bit63��ʾ0~63ͨ����ÿ���������һ��Ȩ�ޣ�
//	/*����0��Զ��Ԥ��*/
//	/*����1����������*/
//	/*����2��Զ�̻ط�*/
//	/*����3��Զ�̱���*/
//	/*����4���鿴��־*/
//	/*����5�������Խ�*/
//	/*����6��Զ������*/
//}STRUCT_USERINFO_9000, *LPSTRUCT_USERINFO_9000;

typedef struct 
{
	uint8_t user[32];				//�û��� �ԡ�\0�������ַ���
	uint8_t pwd[32];				//���� �ԡ�\0�������ַ���
	uint8_t grp_name[32]; 			//������
	uint64_t local_authority[64]; 		// �����û�ʹ��Ȩ�ޣ�ÿλ����һ��ͨ��,bit0~bit63��ʾ0~63ͨ����ÿ���������һ��Ȩ�ޣ�
	/*����0��ʵʱԤ��*//*����1���ֶ�¼��*//*����2��¼���ѯ�ط�*//*����3�����ݹ���*//*����4��¼�����*//*����5����̨����*//*����6����ͼ����*//*����7��ͨ������*//*����8����ʱ¼��*//*����9���ƶ����*//*����10����������*/
	/*����11����������*//*����12����������*//*����13����������*//*����14����������*//*����15����Ϣ�鿴*//*����16����������*//*����17����������*//*����18����������*//*����19��ϵͳ�ػ�*//*����20������¼��*/
	/*����21: ¼���ѯ */ /* ����22: ¼��ط� */ /* ����23: ¼��ɾ�� */ 
	uint64_t remote_authority[64];   	//Զ��Ȩ�ޣ�ÿλ����һ��ͨ����bit0~bit63��ʾ0~63ͨ����ÿ���������һ��Ȩ�ޣ�/*����0��Զ��Ԥ��*//*����1����������*//*����2��Զ�̻ط�*//*����3��Զ�̱���*//*����4���鿴��־*//*����5�������Խ�*//*����6��Զ������*/ 
	DWORD bind_ipaddr; 
	uint8_t  bind_macaddr[8];
}STRUCT_USERINFO_9000, *LPSTRUCT_USERINFO_9000;

//�û���Ϣ��չ1
//typedef struct
//{
//	DWORD dwSize;//�ṹ���С
//	uint16_t dwUserInfoMode;//�û�Ȩ��ģʽ��1-�ϵ�Ȩ��ģʽ��2-��GUIȨ��ģʽ��3-9000��ĿȨ��ģʽ
//	uint8_t reserve[2];//����
//	uint8_t user[32];//�û���
//	uint8_t pwd[32];//����
//	DWORD bind_ipaddr; //�û���¼ʱpc����ip��ַ��Ϊ0��ʾ�κ�PC��������ʹ�ø��û���½��DVR�ϣ���Ϊ0��ʾֻ��ip��ַΪ�趨ֵ��pc���ſ���ʹ�ø��û���¼��DVR��
//	uint8_t  bind_macaddr[8];//�û���¼ʱPC����MAC��ַ��Ϊ0��ʾ�κ�PC��������ʹ�ø��û���½��DVR�ϣ���Ϊ0��ʾֻ��MAC��ַΪ�趨ֵ��PC���ſ���ʹ�ø��û���½��DVR��
//	union
//	{
//		STRUCT_USERINFO userInfo;//��dwUserInfoMode=1ʱ��ʹ�øýṹ��
//		STRUCT_USERINFO_GUI userInfoGui;//��dwUserInfoMode=2ʱ��ʹ�øýṹ��
//		STRUCT_USERINFO_9000 userInfo9000;//��dwUserInfoMode=3ʱ��ʹ�øýṹ��
//	}info;
//}STRUCT_SDVR_USER_INFO_EX1, *LPSTRUCT_SDVR_USER_INFO_EX1;

typedef struct
{
	DWORD dwSize;
	uint16_t dwUserInfoMode;  //�û�Ȩ��ģʽ��1-�ϵ�Ȩ��ģʽ��2-��GUIȨ��ģʽ��3-9000��ĿȨ��ģʽ
	uint8_t right_ver;		/* �û�Ȩ�ް汾��־: 0-�ɰ汾��1-�°汾v1(��¼���ѯ�ط�Ȩ��ɾ�����ֱ�����¼���ѯ���طţ�ɾ������Ȩ�� */
	uint8_t reserve;      //����
	union 
	{
		STRUCT_USERINFO userInfo[MAX_USERNUM];			//��dwUserInfoMode=1ʱ��ʹ�øýṹ�� 
		STRUCT_USERINFO_GUI userInfoGui[MAX_USERNUM];	//��dwUserInfoMode=2ʱ��ʹ�øýṹ�� 
		STRUCT_USERINFO_9000 userInfo9000[MAX_USERNUM]; //��dwUserInfoMode=3ʱ��ʹ�øýṹ�� 
	}info;
}STRUCT_SDVR_USER_INFO_EX1,*LPSTRUCT_SDVR_USER_INFO_EX1;

//��ȡ����ͨ������֧�ַ�Χ
//��������
typedef struct { 
	uint8_t dwBitrateTypeIndex; //��ǰ������������ֵ����ʾ����dwBitrateType�е�λ�� 
	//����dwBitrateTypeIndex = 0����ǰ��������ΪdwBitrateType�ĵ�0λ��ָ�����������ͣ�����������
	uint8_t dwBitrateType; //֧�ֵ��������ͣ�ÿһλ����һ���������ͣ���λΪ1��ʾ֧�ָ��������ͣ��ӵ�λ��ʼ����0λ�Ǳ���������1λ�Ƕ�����
} STRUCT_SDVR_BITRATETYPE, *LPSTRUCT_SDVR_BITRATETYPE;

//��ȡ����ͨ������֧�ַ�Χ
//ͼƬ����
typedef struct { 
	uint8_t dwPicQualityIndex; //��ǰͼ����������ֵ����ʾ����dwPicQuality�е�λ�� 
	//����dwPicQualityIndex = 0����ǰͼ������ΪdwBitrateType�ĵ�0λ��ָ����ͼ������������������
	uint8_t dwPicQuality; //֧�ֵ�ͼ��������ÿһλ����һ��ͼ����������λΪ1��ʾ֧�ָ�ͼ���������ӵ�λ��ʼ����0λ����ߣ���1λ�ǽϸߣ���2λ�Ǹߣ���3λ���У���4λ�ǵͣ���5λ�����
} STRUCT_SDVR_PICQUALITY, *LPSTRUCT_SDVR_PICQUALITY;

//��ȡ����ͨ������֧�ַ�Χ
//��Ƶ����
typedef struct { 
	DWORD dwVideoBitrateIndex; //��ǰ��������ֵ����ʾ����dwVideoBitrate�е�λ�� 
	//����dwVideoBitrateIndex = 0����ǰ����ΪdwVideoBitrate[0]����ָ��������ֵ��
	DWORD dwVideoBitrate[32];//֧�ֵ����ʣ�ÿһ���������һ�����ʣ������ֵ���Ϊ0����ʾ������δ�õ�����Ϊ0����ʾ֧�ֵ�����ֵ����λΪKbit/s
} STRUCT_SDVR_VIDEOBITRATE, *LPSTRUCT_SDVR_VIDEOBITRATE;

//��ȡ����ͨ������֧�ַ�Χ
//֡��
typedef struct { 
	uint16_t VideoFrameRate;	   //��ǰ֡��ֵ��
	uint16_t dwVideoFrameRate_min; //֧�ֵ���С֡��ֵ
	uint16_t dwVideoFrameRate_max; //֧�ֵ����֡��ֵ
} STRUCT_SDVR_VIDEOFRAMERATE, *LPSTRUCT_SDVR_VIDEOFRAMERATE;

//��ȡ����ͨ������֧�ַ�Χ
//�ṹ��������
typedef struct {
	uint8_t byStream_support;//�Ƿ�֧�ָ������������ṹ���ֵ�Ƿ���Ч��1��ʾ��Ч��0��ʾ��Ч
	uint8_t byAudio_support; //�Ƿ�֧����Ƶ��0-��֧�֣�1-֧�� 
	uint8_t reserve[2]; //����
	STRUCT_SDVR_RESOLUTION byResolution_support;		//֧�ֵķֱ���  
	STRUCT_SDVR_BITRATETYPE byBitrateType_support;		//֧�ֵ���������
	STRUCT_SDVR_PICQUALITY byPicQuality_support;		//֧�ֵ�ͼ������
	STRUCT_SDVR_VIDEOBITRATE dwVideoBitrate_support;	//֧�ֵ�����        
	STRUCT_SDVR_VIDEOFRAMERATE dwVideoFrameRate_support; //֧�ֵ�֡��
}STRUCT_SDVR_COMPRESSION_SUPPORT, *LPSTRUCT_SDVR_COMPRESSION_SUPPORT;

//��ȡ����ͨ������֧�ַ�Χ
//������Ϣ�ṹ��
//typedef struct 
//{
//	DWORD dwSize;//�ṹ���С
//	uint8_t byChannel; //ͨ���� 
//	uint8_t byRecordType;  //0x0:�ֶ�¼��0x1:��ʱ¼��0x2:�ƶ���⣬0x3:������0x0f:�������� 
//	uint8_t byCompressionType; //������0-��������1-������1��������2����
//	uint8_t byReserv;//֧�ֵ�������ÿλ����һ����������λΪ1��ʾ֧�ָ��������ӵ�λ��ʼ����0λ��������������1λ����������1����2λ����������2������
//	STRUCT_SDVR_COMPRESSION_SUPPORT struRecordPara;//����֧�ֵĲ���
//	uint8_t  reserve[2];//����
//}STRUCT_SDVR_COMPRESSINFO_SUPPORT, *LPSTRUCT_SDVR_COMPRESSINFO_SUPPORT;

typedef struct { 
	DWORD dwResolution; //�ֱ���ֵ���磺0x07800438�������ֽڣ�0x0780=1920���������ֽڣ�0x0438=1080����
	DWORD dwVideoBitrate_support [32];//�÷ֱ�����֧�ֵ����ʷ�Χ��ÿһ���������һ�����ʣ������ֵ���Ϊ0����ʾ������δ�õ�����Ϊ0����ʾ֧�ֵ�����ֵ����λΪKbit/s
	uint16_t dwVideoFrameRate_min; //�÷ֱ����µ���С֡��
	uint16_t dwVideoFrameRate_max; //�÷ֱ����µ����֡��
	uint8_t  dwPicQuality_support[10];//�÷ֱ�����֧�ֵ�ͼ�������ȼ�,ÿ���������һ��ͼ�������ȼ���0��������ߣ� 1�����ǽϸߣ� 2�����Ǹߣ� 3�������У�4�����ǵͣ� 5��������ͣ�������Ϊ1����ʾ֧�ָ���ͼ������
	uint8_t  reserve[2];	//����
} STRUCT_SDVR_RESOLUTIONINFO;

//��ȡ����ͨ������֧�ַ�Χ
//������Ϣ�ṹ��
typedef struct 
{
	DWORD dwSize;				//�ṹ���С
	uint8_t byChannel; 			//ͨ����
	uint8_t byCompressionType; 	//������0-��������1-������1��������2..
	uint8_t byCompression_support;	//֧�ֵ�������ÿλ����һ����������λΪ1��ʾ֧�ָ������� �ӵ�λ��ʼ��0λ��������������1λ����������1����2λ����������2��..
	uint8_t dwBitrateTypeIndex;	//��ǰ������������ֵ����ʾ����dwBitrateType�е�λ��,����dwBitrateTypeIndex = 0����ǰ��������ΪdwBitrateType�ĵ�0λ,��ָ�����������ͣ�����������
	uint8_t byBitrateType_support; //֧�ֵ��������ͣ�ÿһλ����һ���������ͣ���λΪ1��ʾ֧�ָ��������ͣ��ӵ�λ��ʼ����0λ�Ǳ���������1λ�Ƕ�����
	uint8_t byRecordType_index;	//��ǰ¼����������ֵ
	uint8_t byRecordType_support;	//֧�ֵ�¼�����ͣ�ÿλ����һ��¼�����ͣ���λΪ1��ʾ֧�ָ����ͣ��ӵ�λ��ʼ����0λ�ֶ�¼�񣬵�1λ��ʱ¼�񣬵�2λ�ƶ�¼�񣬵�3λ����¼�񣬡�����15λ����¼��
	uint8_t byAudioflag;			//��ǰ�Ƿ�����Ƶ��0-����Ƶ��1-����Ƶ
	uint8_t byAudio_support;		//�Ƿ�֧����Ƶ��0-��֧�֣�1-֧�֣�����֧����Ƶʱ��byAudioflagֻ��Ϊ0
	uint8_t dwPicQuality;			//��ǰͼ�������� 0--��ߣ� 1-�ϸߣ� 2-�ߣ� 3-�У�4-�ͣ� 5-���
	uint16_t dwVideoFrameRate;		//��ǰ֡��ֵ
	DWORD dwVideoBitrate;		//��ǰ����ֵ����λΪKbit/s
	uint8_t  reserve[3];			//����
	uint8_t  dwResoluIndex;		//��ǰ�ֱ�������ֵ����ʾ����byResolution_support�е�λ��
	STRUCT_SDVR_RESOLUTIONINFO byResolution_support[16]; //֧�ֵķֱ��ʣ����16�ֱַ��ʣ�ÿ���ṹ�����һ�ֱַ��ʼ��÷ֱ�����֧�ֵ����ʣ�֡�ʣ�ͼ��������Χ���ýṹ���dwResolutionΪ0����ʾ�ýṹ��δ�õ�
}STRUCT_SDVR_COMPRESSINFO_SUPPORT,*LPSTRUCT_SDVR_COMPRESSINFO_SUPPORT;


//NVR��־��Ϣ��ѯ (NVR / IPC)
typedef struct
{
	uint16_t dwYear;			// ��: 2000~2063
	uint8_t dwMonth;			// ��: 1~12
	uint8_t dwDay;				// ��: 1~31
	uint16_t dwStart; 			// ��ѯ�ӵڼ�����ʼ��һ��Ϊ0��
	uint16_t dwnum;  			// һ�β�ѯ���������Ϊ100��
	uint8_t priType;    		// ������ ������չ���У�
	uint8_t secType;  			// ������
	uint8_t reserve[6];		// ����
} STRUCT_REQLOG_EX, *LPSTRUCT_REQLOG_EX;

//NVR��־��Ϣ��ѯ (NVR / IPC)���ؽṹ
typedef struct 
{
	DWORD	dwSize;					// �ṹ���С
	DWORD	totalLogNum;			// ��־������
	DWORD	curLogNum;				// ���β鵽������
	DWORD	dwStart; 				// ���β�ѯ������־����ʼ���
	uint8_t	encType;				// �����ʽ 1- UTF-8  2-gb2312   
	uint8_t	reserve[3];				// ����
	uint8_t	sigalLogData[100][128];  // ��־��Ϣ (ÿ�β�ѯ���֧��100����־����־����100��
	//ʱ����Ҫ��ε��ã�ÿ��128�ֽڣ�ÿ���ԡ�\0������)
} STRUCT_SDVR_LOGINFO_EX, *LPSTRUCT_SDVR_LOGINFO_EX;

//���ϣ��㣩ͨ�������������(��չ)
typedef struct	
{
	uint8_t 	byStreamType;		//��������	0-����Ƶ ,1-����Ƶ
	uint8_t 	byResolution; 		//�ֱ���	0-CIF 1-HD1, 2-D1��3-QCIF 4-720p 5-1080p
	uint8_t 	byBitrateType;		//��������	0:�����ʣ�1:������ 
	uint8_t 	byPicQuality;		//ͼ������	1-��� 2-�κ� 3-�Ϻ� 4-һ��5-�ϲ� 6-��
	DWORD 	dwVideoBitrate; 	//��Ƶ���� 0-100K 1-128K��2-256K��3-512K��4-1M��5-1.5M��6-2M��7-3M, 8-4M
	//����������ֵ��kbps����Ч��Χ 30~2^32//���ڵ���30����KΪ��λ
	short	nFrame;				//֡�ʣ�һ�������Ϊ 2-30����ʾ֡��ֵ��nTimebaseֵΪ1
	//��nFrameֵΪ1ʱ��֡��ȡ����nTimebase��ֵ
	short	nTimebase;     		//��nFrameֵΪ1ʱ��nTimebaseֵΪ8����ʾ֡��Ϊ1/8;
	//nTimebaseֵΪ16����ʾ֡��Ϊ1/16
	DWORD 	quant;				//�������� 1-31
}STRUCT_DVR_VENC_CONFIG_EX, *LPSTRUCT_DVR_VENC_CONFIG_EX;

typedef struct 
{
	DWORD 		enable;			//����ͨ���Ƿ����ã�0--�����ã�1--����
	uint8_t		chlist[128];	//ѡ���ͨ������enable=1ʱ��Ч
	STRUCT_DVR_VENC_CONFIG_EX	venc_conf;	 //����ͨ������Ƶ����
	uint8_t		format;			//�����ʽ��0��off��1��1���棬4��2*2���棬9---3*3���棬16---4*4���档
	uint8_t		SwitchTime;		//�л�ʱ��
	uint8_t		reserve[2];		//����
}STRUCT_DVR_ZERO_VENC_CONFIG_EX, *LPSTRUCT_DVR_ZERO_VENC_EX;

//����ʱ��������ʱ��
typedef struct
{
	uint8_t	month;		//����ʱ�������ã���[1��12]
	uint8_t	weeks;		//����ʱ�������ã���[1��5]
	uint8_t	week;		//����ʱ�������ã�����[0��6]
	uint8_t	hour;		//����ʱ�������ã�ʱ[0��23]
	uint8_t	min;		//����ʱ�������ã���[0��59]
	uint8_t	sec;		//����ʱ�������ã���[0��59]
} STRUCT_SDVR_DST_WEEK_TIME_S, *LPSTRUCT_SDVR_DST_WEEK_TIME_S;
//˵�����������õ�ʱ�䣬��ʾ�ڼ��µĵڼ������ڼ��ļ�ʱ���ּ��룬��month=5��weeks=2��week=1��hour=10��min=0��sec=0����ʾ5�·ݵĵ�2������1��10��00��00

//����ʱʱ������
typedef struct
{
	uint8_t	dst_en;			//����ʱʹ�ܼ���0-��ʹ�ܣ�1-ʹ��
	uint8_t	dsttype_en;		//��������Ϊ0, ����������Ϊ1
	SYSTIME	start_date;		//���������õĿ�ʼʱ��
	SYSTIME	end_date;		//���������õĽ���ʱ��
	STRUCT_SDVR_DST_WEEK_TIME_S	start_time;		//�������õĿ�ʼʱ��
	STRUCT_SDVR_DST_WEEK_TIME_S	end_time;		//�������õĽ���ʱ��
	uint8_t		reserve[4];	//����
}STRUCT_SDVR_DST_TIME_S, *LPSTRUCT_SDVR_DST_TIME_S;

// ����ɫ�����
typedef struct
{
	uint8_t brightness;					// ����      ȡֵ��Χ[0��255] ȱʡֵ128
	uint8_t constrast;						// �Աȶ�    ȡֵ��Χ[0��255] ȱʡֵ128
	uint8_t saturation;					// ���Ͷ�    ȡֵ��Χ[0��255] ȱʡֵ128
	uint8_t hue;							// ɫ��      ȡֵ��Χ[0��255] ȱʡֵ128
	uint8_t sharp;							// ���      ȡֵ��Χ[0��255]
	DWORD reserved;						// Ԥ��
}VIDEO_INFO, *LPVIDEO_INFO;

//NVRͨ������
typedef struct	
{
	DWORD dwSize; 					// ���ȣ��ṹ���С��
	// ͨ�������
	uint8_t sChanName[32];				// ͨ���� �ԡ�\0�������ַ���
	uint8_t byChannel; 				// ͨ���� [0, n��1] n:ͨ����
	DWORD dwShowChanName;			// �Ƿ���ʾͨ���� 0-��ʾ 1-����ʾ
	uint8_t byOSDAttrib;				// ͨ���� 1-��͸�� 2-͸����ֻ���PC����ʾ��
	uint16_t wShowNameTopLeftX;			// ͨ��������ʾλ�õ�x���� ��->�� 0~��Ƶʵ�ʿ�ȣ�ͨ��������
	uint16_t wShowNameTopLeftY;			// ͨ��������ʾλ�õ�y���� ��->�� 0~��Ƶʵ�ʸ߶ȣ�����߶�

	// �������
	DWORD dwShowTime;					// �Ƿ���ʾʱ�� 0-��ʾ 1-����ʾ
	uint16_t wOSDTopLeftX;					// ʱ��osd����X [0, ʵ�ʿ�ʱ�볤��]
	uint16_t wOSDTopLeftY;					// ʱ��osd����Y[0, ʵ�ʸߣ�����߶�]
	uint8_t byDateFormat;					// ���ڸ�ʽ
	//  0 - YYYY-MM-DD    ��ȱʡֵ��
	//  1 - MM-DD-YYYY
	//  2 - YYYY��MM��DD��
	//  3 - MM��DD��YYYY��

	// �������
	uint8_t byDispWeek;				    // �Ƿ���ʾ���� 0-��ʾ 1-����ʾ
	uint8_t byOSDLanguage;					// �������� 0-���� 1-Ӣ�� (����չ)

	// ����ɫ�����
	VIDEO_INFO  videoinfo;              // ��Ƶ��Ϣ

	// �ڵ��������
	DWORD dwEnableHide;		// ��Ƶ�ڵ�ʹ�� ,0-���ڵ�,1-�ڵ�(�ڵ�����ȫ��) 2-�ڵ�(�ڵ�����������)  
	STRUCT_SDVR_SHELTER	struShelter[16];	// ��Ƶ�ڵ�����	   
	DWORD dwOsdOverType; 					// osd�������� 1-ǰ�˵��� 2-��˵���
	DWORD reserve[32];						// ����
} STRUCT_NVR_CHN_ATTR_INFO, *LPSTRUCT_NVR_CHN_ATTR_INFO;

typedef struct
{
	uint8_t    streamtype;			// ������     0-��������ȱʡֵ�� 1-������
	uint8_t    quality;			// ��Ƶ����   1-��� 2-�ϸ� 3-�ߣ�ȱʡֵ�� 4-�� 5-�� 6-���
	uint8_t    resolution;			// ������     0-CIF 1-D1��ȱʡֵ�� 2-720P 3-1080P
	// ������     0-CIF 1-D1(ȱʡֵ)
	uint8_t    framerate;			// ֡��       ȡֵ��Χ[2,25] ȱʡֵ25
	uint8_t    maxbitrate;			//��Ƶ���� 0-100K 1-128K��2-256K��3-512K��4-1M��5-1.5M��6-2M��7-3M, 
	//8-4M ����������ֵ��kbps����Ч��Χ 30~2^32�����ڵ���32����KΪ��λ
	uint8_t    audio;				// ��Ƶ��ʶ   0-����Ƶ 1-����Ƶ��ȱʡֵ��
	DWORD   reserved;			// Ԥ��
}STRUCT_RECORD_PARAM,  *PSTRUCT_RECORD_PARAM;

typedef struct
{
	uint8_t starth;				// ��ʼʱ��-ʱ
	uint8_t startm;				// ��ʼʱ��-��
	uint8_t stoph;					// ����ʱ��-ʱ
	uint8_t stopm;					// ����ʱ��-��
	uint8_t recType;				// ¼������  0 - �� 1-�ֶ�(��Ч)  2-��ʱ  3-�ƶ�  4-���� 5-�ƶ� | ����  6 -�ƶ� & ����
	uint8_t reserve[3];			// ����
}STRUCT_REC_TIME_PERIOD, *PSTRUCT_REC_TIME_PERIOD;

typedef struct
{
	uint8_t Enable;  		// ������ʹ��  0-��ʹ��(ȱʡֵ) 1-ʹ��
	uint8_t recType; 		// �������Ӧ��¼������ 0 - �� 1-�ֶ�(��Ч)  2-��ʱ  3-�ƶ�  4-���� 5-�ƶ� | ����  6- �ƶ� & ����
	uint8_t reserve[2];	// ����
}STRUCT_FULL_DAY_S;

typedef struct
{
	uint8_t enable;				// ʹ��ʱ��� 0-��ʹ��(ȱʡֵ) 1-ʹ��    
	uint8_t weekEnable;			// ÿ��ʹ��λ 0-��ʹ�� 1-ʹ��(��ʹ��,ֻȡstruAlarmTime[0][0~7]��ÿ��������)  
	STRUCT_FULL_DAY_S	fullDayEnable[8];			// ������¼��
	STRUCT_REC_TIME_PERIOD struAlarmTime[8][8]; 	// [0-7][0]����ȫ��ʹ�ܵ�������

	DWORD  reserved;                 				// Ԥ��
}STRUCT_REC_TIME_SCHEDULE, *PSTRUCT_REC_TIME_SCHEDULE;

//NVR¼��������ϣ�
typedef struct
{
	DWORD   dwSize;					// �ṹ���С
	uint8_t	byChannel;				// ͨ����
	uint16_t    preRecTime;				// Ԥ¼ʱ��      ȡֵ��Χ[5,30]��  ȱʡֵ10
	DWORD   delayRecTime;			// ¼�����ʱ��  ȡֵ��Χ[0,180]��  ȱʡֵ30 
	//(��3-�ƶ�¼�� 4-����¼�� 5-�ƶ� | ����  6-�ƶ� & ���� ��Ч)
	STRUCT_REC_TIME_SCHEDULE timeschedule;		// ¼��ʱ�����¼����������

	STRUCT_RECORD_PARAM    timerecord;			// ��ʱ          ¼�����
	STRUCT_RECORD_PARAM    moverecord; 			// �ƶ�          ¼�����
	STRUCT_RECORD_PARAM    alarmrecord;			// ����          ¼�����
	STRUCT_RECORD_PARAM    moveOrAlarmrecord;	// �ƶ� | ����	¼����� 
	STRUCT_RECORD_PARAM    moveAndAlarmrecord;	// �ƶ� & ����	¼�����
	STRUCT_RECORD_PARAM    neRecParam[4];     	// ����

	DWORD  byLinkMode;							// �������� (0-������ 1-��һ������ 2-�ڶ������� ....)
	DWORD  reserved[31];						// Ԥ��
}STRUCT_RECORD_SET_OLD, *PSTRUCT_RECORD_SET_OLD;

//NVR¼��������£�
typedef struct
{
	DWORD   dwSize;					// �ṹ���С
	uint8_t	byChannel;				// ͨ����
	uint16_t    preRecTime;				// Ԥ¼ʱ��      ȡֵ��Χ[5,30]��  ȱʡֵ10
	DWORD   delayRecTime;			// ¼�����ʱ��  ȡֵ��Χ[0,180]��  ȱʡֵ30 
	//(��3-�ƶ�¼�� 4-����¼�� 5-�ƶ� | ����  6-�ƶ� & ���� ��Ч)
	STRUCT_REC_TIME_SCHEDULE timeschedule;		// ¼��ʱ�����¼����������

	STRUCT_RECORD_PARAM    manurecord;      	// �ֶ�          ¼�����
	STRUCT_RECORD_PARAM    timerecord;			// ��ʱ          ¼�����
	STRUCT_RECORD_PARAM    moverecord; 			// �ƶ�          ¼�����
	STRUCT_RECORD_PARAM    alarmrecord;			// ����          ¼�����
	STRUCT_RECORD_PARAM    moveOrAlarmrecord;	// �ƶ� | ����	¼����� 
	STRUCT_RECORD_PARAM    moveAndAlarmrecord;	// �ƶ� & ����	¼�����
	STRUCT_RECORD_PARAM    neRecParam[4];     	// ����

	DWORD  byLinkMode;							// �������� (0-������ 1-��һ������ 2-�ڶ������� ....)
	DWORD  reserved[31];						// Ԥ��
}STRUCT_RECORD_SET, *PSTRUCT_RECORD_SET; 

typedef struct	
{
	uint8_t	byStartHour;  			//��ʼСʱ 0-23
	uint8_t	byStartMin;  			//��ʼ���� 0-59
	uint8_t	byStopHour; 			//����Сʱ  0-23
	uint8_t	byStopMin;  			//��������  0-59
}STRUCT_NVR_SCHEDTIME;		

//�ƶ����(NVR)
typedef struct	
{
	DWORD dwSize;				//���ȣ��ṹ���С��
	uint8_t byChannel;				//ͨ���� [0, n��1] n:ͨ����

	uint8_t byMotionScope[18][22];	//�������,����22*18��С���,Ϊ1��ʾ�ú�����ƶ��������,0-��ʾ���� 
	uint8_t byMotionSensitive;		//�ƶ����������, 0 - 5,Խ��Խ���� 

	// ʱ������
	uint8_t byEnableHandleMotion;	// �ƶ���Ⲽ��ʹ�� 0-���� 1-����	
	uint8_t weekEnable;			// ����ÿ��0-��ʹ�� 1-ʹ��(��ʹ��,ֻȡstruAlarmTime[0][0~7]��ÿ��������)  
	uint8_t fullDayEnable[8];		// ������¼�� 0-��ʹ��(ȱʡֵ) 1-ʹ��,������ʹ��,���Ӧ����Ϊȫ�첼��,�����ж�ʱ���
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8];		//����ʱ���, 8��ʱ���
	DWORD	dwHandleType;							//��λ 2-��������5-��������� //6-�ʼ��ϴ�

	// �����������
	uint8_t	alarmOut_local[16];				//��������˿�(����)
	uint8_t	alarmOut_remote[128][16];		//��������˿�(ǰ���豸)

	// ����¼��    
	uint8_t record_channel[128];				// ������¼��ͨ����Ϊ0-������ 1-����

	// ��������  
	uint8_t byEnablePreset[128];			    // �Ƿ����Ԥ�õ� ����byEnablePreset[0]���ж�;
	uint8_t byPresetNo[128];				    // ���õ���̨Ԥ�õ����,һ������������Ե��ö��ͨ������̨Ԥ�õ�, 0xff��ʾ������Ԥ�õ� [1, 254]
	DWORD reserve[32];						// ����
}STRUCT_NVR_MOTION, *LPSTRUCT_NVR_MOTION;

//��ȡNVR��̨Э���б�
typedef struct
{
	uint8_t  Type; 		//  0-NVR������̨��1-ǰ���豸��̨ (Ϊ1ʱbyChannel��Ч)
	uint8_t  byChannel;   	//  [0, n-1],n:ͨ����
	uint8_t  reserve[2];	//	����
}STRUCT_NVR_PTZLIST, *LPSTRUCT_NVR_PTZLIST;

typedef struct 
{
	DWORD	ptznum;					// Э�����������Ϊ���100����
	uint8_t	reserve[4];				// ����
	uint8_t	ptztype[100][10];		// Э�����б�D�D�D0��unknow;
}STRUCT_NVR_PTZLIST_INFO, *LPSTRUCT_NVR_PTZLIST_INFO;

//�������������NVR��
typedef struct
{
	DWORD dwSize;						// �ṹ���С
	uint8_t opType;                        // 0-���� 1-ǰ�� (Ϊ1ʱ,byChannel��Ч)
	uint8_t byChannel;                     // ����ǰ��ĳͨ���豸 [0, n-1], n:ͨ������
	uint8_t byAlarmInPort;				    // ��������˿ں�[0, n-1], n:�����������
	uint8_t sAlarmInName[32];			    // ��������˿����� �ԡ�\0�������ַ���
	uint8_t byAlarmType;				    // ̽ͷ���� 0-����1-����
	uint8_t byEnSchedule;					// �������벼��ʱ�伤�� 0-���� 1-���� 
	uint8_t weekEnable;               		// ÿ��ʹ��λ0-��ʹ�� 1-ʹ��(��ʹ��,ֻȡstruAlarmTime[0][0~7]������ÿһ��) 
	uint8_t allDayEnable[8];						// ȫ��ʹ�� ,0-��ʹ�� 1-ʹ��������ʹ��,���Ӧ����Ϊȫ�첼��,�����ж�ʱ���	
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8];	// ����ʱ���
	DWORD	dwHandleType;	           			// ��λ 2-�������� 5-��������� //6-�ʼ��ϴ�

	// �����������
	uint8_t  alarmOut_local[16];		    // ��������˿�(����)
	uint8_t  alarmOut_remote[128][16];		// ��������˿�(ǰ���豸)

	// ����¼��
	uint8_t byRelRecordChan[128];		    // ����������¼��ͨ��,Ϊ1��ʾ������ͨ�� 

	// ��������
	uint8_t byEnablePreset[128];			// �Ƿ����Ԥ�õ� ����byEnablePreset[0]���ж�
	uint8_t byPresetNo[128];				// ���õ���̨Ԥ�õ����,һ������������Ե��ö��ͨ������̨Ԥ�õ�, 0xff��ʾ������Ԥ�õ� [1, 254]
	uint8_t reserve[32];			        // ����
} STRUCT_NVR_ALRMIN, *LPSTRUCT_NVR_ALRMIN;

//�������������NVR��
typedef struct	
{
	DWORD dwSize;					// �ṹ���С
	uint8_t opType;                    // 0-���� 1-ǰ�� (Ϊ1ʱ,byChannel��Ч)
	uint8_t byChannel;                 // ����ǰ��ĳͨ���豸 [0, n-1], n:ͨ������
	uint8_t byALarmoutPort;			// �������ͨ���� [0, n-1], n:��������˿���
	uint8_t sAlarmOutName[32];			// ���� �ԡ�\0�������ַ���
	DWORD dwAlarmOutDelay;			// �������ʱ�� ��λ�� [2, 300]
	uint8_t byAlarmType;				// ̽ͷ���� 0-����1-���� (����)    
	uint8_t byEnSchedule;				// �����������ʱ�伤�� 0-���� 1-���� 
	uint8_t weekEnable;				// ÿ��ʹ��λ0-��ʹ�� 1-ʹ��(��ʹ��,ֻȡstruAlarmTime[0][0~7]��ÿ��������)  
	uint8_t fullDayEnable[8];     		// ������¼�� 0-��ʹ��(ȱʡֵ) 1-ʹ��
	STRUCT_NVR_SCHEDTIME struAlarmTime[8][8]; //����ʱ���, 8��ʱ���

	uint8_t  reserve[32]; 				// ����
}STRUCT_NVR_ALARMOUTINFO, *LPSTRUCT_NVR_ALARMOUTINFO;

//��ȡ�豸��Ϣ��չ(NVR)
typedef struct	
{
	DWORD dwSize;						// �ṹ���С
	uint8_t sDVRName[32];					// �豸, �ԡ�\0�������ַ���
	DWORD dwDVRID;						// ����
	DWORD dwRecycleRecord;				// Э���: //¼�񸲸ǲ��� 0-ѭ������ 1-��ʾ����
	uint8_t sSerialNumber[48];				// ���к�
	uint8_t sSoftwareVersion[64];			// ����汾���ԡ�\0�������ַ���Э���: �������ͺ� ����汾�ţ�
	uint8_t sSoftwareBuildDate[32];		// ������������ԡ�\0�������ַ���Э���:��Build 100112��
	DWORD dwDSPSoftwareVersion;			// DSP����汾
	uint8_t sPanelVersion[32];				// ǰ���汾���ԡ�\0�������ַ�����IPC��
	uint8_t sHardwareVersion[32];	        // (����)Э���: ������汾�ų���16�ֽ�ʱ��ʹ����Ϊ�����ͺ���ʾ
	uint8_t byAlarmInPortNum;		        // �����������, NVRֻȡ���ر�������
	uint8_t byAlarmOutPortNum;	  	    	// �����������, NVRֻȡ���ر������
	uint8_t byRS232Num;			        // ����
	uint8_t byRS485Num;					// ����
	uint8_t byNetworkPortNum;				// ����
	uint8_t byDiskCtrlNum;					// ����
	uint8_t byDiskNum;						// Ӳ�̸���
	uint8_t byDVRType;						// DVR����, 1:NVR 2:ATM NVR 3:DVS 4:IPC 5:NVR ������ʹ��//NET_SDVR_GET_DVRTYPE���
	uint8_t byChanNum;						// ͨ������[0, 128]
	uint8_t byStartChan;					// ����
	uint8_t byDecordChans;					// ����
	uint8_t byVGANum;						// ����
	uint8_t byUSBNum;						// ����
	uint8_t bySDI485Num;					// ����
	uint8_t reserve[2];					// ����
}STRUCT_SDVR_DEVICEINFO_EX, *LPSTRUCT_SDVR_DEVICEINFO_EX;


//��ȡ�����豸����״̬��չ(NVR)
typedef struct 
{
	uint8_t byRecordState;				// ͨ���Ƿ���¼��,0-��¼��,1-¼��
	uint8_t bySignalState;				// ���ӵ��ź�״̬,0-����,1-�źŶ�ʧ
	uint8_t byHardwareState;			// ����
	uint8_t byLinkNum; 				// �ͻ������ӵĸ�����ͬһͨ����ǰʱ���ʵʱ��������������������������ͬһIP���������������
	DWORD dwBitRate;				// ʵ������
}STRUCT_SDVR_CHANNELSTATE_EX, LPSTRUCT_SDVR_CHANNELSTATE_EX;

typedef struct	
{
	DWORD dwSize ; 										// �ṹ���С
	STRUCT_SDVR_DISKSTATE  struHardDiskState[16];		// Ӳ��״̬
	STRUCT_SDVR_CHANNELSTATE_EX struChanState[128];		// ͨ����״̬
	uint8_t alarminStatusLocal[128];						// ���ر�������˿ڵ�״̬
	uint8_t alarmoutStatusLocal[128];						// ���ر�������˿ڵ�״̬
	DWORD reserve[4];									// ����
}STRUCT_SDVR_WORKSTATE_EX_NVR, *LPSTRUCT_SDVR_WORKSTATE_EX_NVR;

/*******************��ȡ�洢�豸��Ϣ�ṹ��***********************/
typedef enum
{
	NET_RAID_LEVEL_NULL,		 /*δ���raidģʽ*/    
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
	NET_DEV_RAID_NORMAL,            // raid ����
	NET_DEV_RAID_DEGRED,            // raid ����
	NET_DEV_RAID_REBUILDING,        // raid �ؽ�
	NET_DEV_RAID_BROKEN,            // raid ��
	NET_DEV_RAID_NO_RAID,           // raid û�н���
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
	uint8_t	  Name[64];                  //�ͺ�
	uint64_t dev_capabilty;             //����                          
//	uint8_t      reserve[4];                //����	                                                                                                                                     
}STRUCT_SDVR_RAIDDEVINFO;

typedef struct
{
	uint64_t serial_num;              //�������к�
	uint8_t      rec_port;                //¼��ͨ����4;8;16
	uint8_t      rec_format;              //¼��ֱ���
	uint8_t      rec_colorsystem;         //¼����ʽNtsc;Pal
	uint8_t      rec_standard;            //¼��ѹ���㷨H264;MPG4
	MFS_FIELD_TIME sys_time;           //ϵͳ�����ʴ���ʱ��
	uint8_t      log_size;                //32M;64M;128M;256M;512M
	uint8_t      esata_enable;            //esata�豸λ��ʹ��
	uint8_t      sys_hd_num;              //��ǰϵͳ֧�ֵ����Ӳ����
	uint8_t      reserve1[5];             //����	  
}STRUCT_SDVR_RECORD_SYSINFO;

typedef struct
{
	NET_RAIDSTATUS_E  raid_status;               //�豸raid״̬
	NET_Raid_Level_E  raid_level;                //�豸raid ģʽ�ļ���     
	DWORD  raid_dev_table;                       //ÿ��raid���ϵ��豸����λ�����
	STRUCT_SDVR_RAIDDEVINFO raid_dev_info[10];   //ÿ�����̵��ͺź�����    
//	uint8_t reserve[4];							 //����	                                                                                                                     
}STRUCT_SDVR_RAIDINFO;

typedef struct
{
	uint16_t dev_num;                    //�洢�豸����ͨ��
	uint16_t dev_type;                   //�豸���ͣ�Ĭ��Ӳ�̣�, NET _DEVTYPE_E
	DWORD removable;                 //�豸�Ƿ���ƶ����ã�Ĭ�Ϸ� 0-��1-��
	DWORD backup;                    //�豸�Ƿ񱸷����ã�Ĭ�Ϸ�  0-��1-��
	DWORD dev_status;                //�豸״̬   0-������1-���̴���2-�ļ�ϵͳ����
	DWORD is_raid_dev;               //�豸�Ƿ�raid װ��(Ĭ�Ϸ�)   0-��1-��
	STRUCT_SDVR_RAIDINFO raid_info;  //��ǰ�豸raidװ�õ���ϸ��Ϣ
	DWORD active;                    //�豸����״̬�����߻��߻��
	uint64_t device_capabilty;      //�豸����(�ֽ�) ƽ̨���ݸ�����Ҫ������Ӧת��
	DWORD mfs_fs_active;             /*�豸�ļ�ϵͳ�Ǹ�ʽ��*/
	uint64_t mfs_capability;        //�ļ�ϵͳ������������ʽ����������ֽڣ�ƽ//̨���ݸ�����//Ҫ������Ӧת��
	uint64_t mfs_free;              //��ǰ��������(�ֽ�)ƽ̨���ݸ�����Ҫ������Ӧת��
	DWORD device_handle;             //�豸�������
	STRUCT_SDVR_RECORD_SYSINFO device_sys_info;     //������¼����ص�ϵͳ��Ϣ 
	uint8_t reserve[4];                                //����	                                                                                                                
}STRUCT_SDVR_DEVICEINF;

typedef struct
{
	DWORD cycle_overlay;                             //ѭ�����Ƿ�ʽ
	STRUCT_SDVR_DEVICEINF device_info[MAX_DEV_NUM];  //�洢�豸��Ϣ    
	uint8_t reserve[4];                                 //����	                                                                                                           
}STRUCT_SDVR_STORDEVINFO, *LPSTRUCT_SDVR_STORDEVINFO;
/******************************************************************/

/**************************��ʽ���洢�豸��ͷ��*****************************/
typedef struct
{
	DWORD dev_num;              //�洢�豸����ͨ��
	DWORD dev_type;             //�洢�豸���ͣ�NET_DEVTYPE_E
	DWORD reserve;              //����
}STRUCT_SDVR_FORMAT_DEV;

typedef struct
{
	DWORD format_num;				 //��Ҫ��ʽ���豸�ĸ���
	DWORD format_log_flg;            //�Ƿ��ʽ����־��־��0-����ʽ����1-��ʽ��
	STRUCT_SDVR_FORMAT_DEV dev_info[MAX_FORMAT_DEV];  //ÿ��Ҫ��ʽ���豸����Ϣ                                                                                                                                                         
	uint8_t reserve[4];								  //����	                                                                                 
}STRUCT_SDVR_FORMATINFO;
/****************************************************************************/
//UPNP
typedef struct
{
	uint8_t  servicename[32];/*��������*/
	int32_t  iport;/*/DVR�˿ں�*/
	int32_t  eport;/*·�����˿�*/
	int32_t  proto;/*Э�鼴TCP����UDP*/
	uint8_t  leaseDuration[20];
}STRUCT_SDVR_UPNP_PROTINFO;
typedef struct
{
	uint8_t  upnp_en;                 // UPNP ʹ��
	int32_t   port_num;             // ת���˿���Ϣ����
	STRUCT_SDVR_UPNP_PROTINFO portinfo[MAX_PAT];   // ת���˿���Ϣ  MAX_PAT=15�������ͬʱ������15���˿���Ϣ
	uint8_t  reserve[10];   //����
}STRUCT_SDVR_UPNPINFO;

//ǰ��ͨ���ַ�������Ϣ����
typedef   struct 
{ 
	uint8_t     id;					// ͨ���ַ�������Ϣ�� [0, n��1] n: �����ַ���Ϣ����
	uint8_t     byLinkMode;			// 0-������  1-������  
	uint8_t     byChanOSDStrSize;		// �����ַ���Ϣ���ַ������ݵĳ��ȣ������ַ���������'\0'
	uint8_t     byOSDAttrib;			// ͨ���ַ�������Ϣ  1-��͸��  2-͸��(ֻ��� PC ����ʾ)
	uint8_t     byOSDType;				//��ʽ�����ԣ����λΪ 0 ��ʾ�������ӣ�Ϊ 1 ��ʾǰ�˵���
	//��Ϊ 0x80 ʱ��ʾ�� osd ��Ϊǰ�˵���
	char     reservedData[3];
	DWORD	 dwShowChanOSDInfo;		// �Ƿ���ʾͨ���ַ�������Ϣ  0-��ʾ  1-����ʾ 
	uint16_t     wShowOSDInfoTopLeftX;	// ͨ���ַ�������Ϣ��ʾλ�õ� x ����
									// [0,  ʵ�ʿ������ַ����ݳ���]
	uint16_t     wShowOSDInfoTopLeftY;	// ͨ���ַ�������Ϣ��ʾλ�õ� y ���� 
									// [0,  ʵ�ʸߣ�����߶�] 
	char     data[];				// �����ַ���Ϣ����ַ������ݣ������ַ���������'\0'
} STRUCT_SDVR_OSDINFO, *LPSTRUCT_SDVR_OSDINFO; 

typedef   struct 
{ 
	uint8_t    byChannel;				// ͨ���� [0, n��1] n:ͨ����
	uint8_t    byOSDInfoNum;			//�����ĵ����ַ���Ϣ������ÿ��ṹΪSTRUCT_SDVR_OSDINFO
	uint16_t	byChanOSDInfoSize;      // �����ַ���Ϣ�����ݰ���С
	char    data[];					// �����ŵ����ַ���Ϣ��������
} STRUCT_SDVR_OSDCFG, *LPSTRUCT_SDVR_OSDCFG; 



//������������
typedef   struct 
{ 
	DWORD dwMsgid;
	DWORD dwFileType;
	DWORD dwFileSize;
	DWORD dwFileCRC32;
	DWORD dwReserve[4];
}STRUCT_SDVR_UPDATE, *LPSTRUCT_SDVR_UPDATE; 

//Զ�̲�ѯ���ĳ�µ�¼���¼
typedef struct  
{
	uint8_t  byChannel;		//ͨ����[0, n��1]��n:ͨ���� 
	uint8_t  byType;			//��ѯ��¼�����ͣ��±���ֵ��¼�����Ͷ�Ӧ��ϵ 
	uint8_t  byYear;			//��ѯ����  [0-63] 2000 ��Ϊ 0��2063 ��Ϊ 63 
	uint8_t  byMonth;			//��ѯ����  [1-12] 
	uint8_t  Reserve[32];		//����  
}STRUCT_SDVR_QUERY_MONTH, *LPSTRUCT_SDVR_QUERY_MONTH;

typedef struct 
{ 
	uint8_t  byDate[31];  //������¼�����ݵ�����,����[n]����ĳ�µĵ�(n+1)�죬0-��¼��  1-��¼�� 
	uint8_t  Reserve[9];  //���� 
}STRUCT_SDVR_MONTHINFO,  *LPSTRUCT_SDVR_MONTHINFO; 

//��ȡ�����¶�
typedef struct 
{ 
	DWORD    mainboard_temperature;			//�����¶� 
	DWORD    chip_temperature;						//оƬ�¶� 
	DWORD    harddisk_temperature[MAX_DISKNUM]; //Ӳ���¶� 
	DWORD    cpu_temperature;					//CPU �¶� 
	DWORD    humidity;								//ʪ�� 
	uint8_t			reserve[4];								//���� 
}STRUCT_SDVR_TEMPERATURE, *LPSTRUCT_SDVR_TEMPERATURE; 

//��ȡ¼��ͳ��
typedef struct
{
	DWORD totalDateNumber_record;        		 //¼����������Ӳ������¼������������ܺ�
	UNMFS_TIME beginDate_record;		//Ӳ���ϼ�¼�����翪ʼ¼���ʱ��
	UNMFS_TIME endDate_record;			//Ӳ���ϼ�¼���������¼���ʱ��
	DWORD type_record;					//¼�����ͣ�0x01-�ֶ�¼��,0x02-��ʱ¼��,0x04-�ƶ�¼��,0x08-����¼��,0xff-����¼��
	unsigned long long totalTime;		//��ʱ�䣬Ӳ��������¼���ʱ���ܺͣ�����Ϊ��λͳ��
	unsigned long long totalFileLength;		//���ļ����ȣ�Ӳ��������¼���ļ����ܳ��ȣ����ֽڱ�ʾ
	uint8_t reserve[4];				//����
}STRUCT_SDVR_RECORDSTATISTICS, *LPSTRUCT_SDVR_RECORDSTATISTICS;   

//���ø���¼�������Ľṹ��
typedef struct{
	unsigned short notEnoughDate;	 //ָ��Ӳ���ϸ���¼��ʱ��¼������������������ֵ
	uint8_t reserve[2];           				 //����                            
}STRUCT_SDVR_RECORDNOTENOUGHDATE_ALARM;

typedef struct 
{ 
	DWORD    dwAlarmDayNum;     //Ԥ�豨������ 
	DWORD    dwCurDayNum;        // ��ǰ���� 
	uint8_t  reserve[8];            //���� 
}STRUCT_SDVR_RECORD_ALARM; 

typedef struct 
{ 
	uint8_t  byChannel;			//����ͨ���� 
	uint8_t  reserve[11];			//���� 
}STRUCT_SDVR_ALARM_STAT; 

//���������ϴ�
//����������dwAlarmTypeΪ0-6ʱ��union����STRUCT_SDVR_ALARM_STAT�ṹ�壬��dwAlarmType=7ʱ��union����STRUCT_SDVR_RECORD_ALARM�ṹ��
typedef struct 
{ 
	uint8_t byAlarmStat;			//����״̬��0������������1��������ʼ 
	uint8_t reserve[3];				//���� 
	DWORD dwAlarmType;        //�������ͣ�0-�ƶ�������1-̽ͷ������2-��Ƶ��ʧ������3-���̴��󱨾��� 
	//4-����Ͽ�������5-�¶ȹ��ߣ�6-��Ƶ�ڵ����� 7-¼���������㱨�� 
	UNMFS_TIME dwAlarmTime;     //����ʱ�� 
	union 
	{ 
		STRUCT_SDVR_ALARM_STAT     stat; 
		STRUCT_SDVR_RECORD_ALARM      rec;  //¼���������㱨�� 
	}alarm; 
}STRUCT_SDVR_ALARM_REQ, *LPSTRUCT_SDVR_ALARM_REQ; 

//���������ļ�����
typedef struct 
{ 
	DWORD   dwFileSize;	//�����ļ��Ĵ�С 
	DWORD   reserve;		//���� 
	char    pFileData[]; // �����ļ�����
} STRUCT_EXPT_REQ, *LPSTRUCT_EXPT_REQ; 

//���������ļ�����
typedef struct 
{ 
	DWORD    dwFileSize; //�����ļ��Ĵ�С����Ϊ0���򲻵����ļ����ָ�����Ĭ�ϲ��� 
	DWORD    reserve;		//���� 
} STRUCT_IMPT_REQ, *LPSTRUCT_IMPT_REQ; 

//Զ��¼�񱸷�(ʵ�ֶϵ�����)
typedef struct
{
	uint8_t byChannel;			//ͨ����
	uint8_t type;						//�������ͣ�0-�ֶ���1-��ʱ��2-�ƶ���3-̽ͷ������0x0f-����¼��
	uint16_t file_index;			//�ͻ������ص��ļ��б�������ţ���0��ʼ
	DWORD file_offset;			//�ļ�ƫ�ƴ�С���տ�ʼΪ0
	UNMFS_TIME starttime;  //��ʼʱ��
	UNMFS_TIME endtime;   //����ʱ��
	uint8_t streamtype;			//�������ͣ�0-��������1-������
	uint8_t reserve[31];			//����
}STRUCT_SDVR_BACKINFO_EX;
/*
//Զ��¼��㲥��ʼ
typedef struct TAG_VOD_PARAM 
{ 
	uint8_t   byChannel;					// ͨ����[0, n-1],n:ͨ���� 
	uint8_t   byType;						// ¼������: 1-�ֶ���-��ʱ��-�ƶ���-������xFF-ȫ�� 
	uint16_t    wLoadMode;           // �ط�����ģʽ 1-��ʱ�䣬2-������ 
	union 
	{ 
		struct 
		{ 
			UNMFS_TIME   struStartTime;	 // ���һ����Ȼ�� 
			UNMFS_TIME   struStopTime;  // ����ʱ����ൽ23:59:59, 
			// ����ʾ�ӿ�ʼʱ�俪ʼһֱ���� 
			char cReserve[16]; 
		}byTime; 

		uint8_t   byFile[64];            // �Ƿ񹻳��� 
	}mode; 

	uint8_t   streamtype;                 //�������ͣ�0-��������1-������ 
	uint8_t   byReserve[15];             //���� 
}VOD_PARAM, *LPVOD_PARAM; 

typedef struct TAG_VOD_ANS 
{ 
	DWORD    dwVodID;		//�������� 
	uint8_t   streamtype;			//�������ͣ�0-��������1-������ 
	uint8_t   byReserve[15];		//���� 
}VOD_ANSWER, *LPVOD_ANSWER; 
*/

//Զ��¼���ļ������ļ�ͷ��־��Ϣ(��չ) ���ϵ���������ʹ�øñ�־
typedef struct 
{ 
	DWORD   port;        //ͨ���� 
	DWORD   filelen;     //���ļ����� ��B Ϊ��λ 
	UNMFS_TIME  filestarttime; //��¼���ļ���ʼʱ�� 
	UNMFS_TIME  fileendtime; //��¼���ļ�����ʱ�� 
	DWORD   totalFileLen; //���ļ����ȣ���ʾNETCOM_BACKUP_RECFILE_REQ_EX ���������� 
	//ʱ����ڵ��ܳ���(ע����ʱ������п����ж���ļ�) ����KB Ϊ��λ 
	uint16_t   file_index;   //�ļ��б�������� 
	DWORD   file_offset; //�ļ�ƫ�ƴ�С�����ͱ��ļ�����ʼƫ�ƴ�С 
}STRUCT_SDVR_FILEINFO_EX,*LPSTRUCT_SDVR_FILEINFO_EX; 

//��ȡ������֧�ֵĹ���
typedef struct 
{ 
	uint8_t funtions[256]; //����Ĺ��ܣ������ÿ��Ԫ�ش���һ�ֹ��ܣ�Ԫ�ص�ֵΪ0-��ʾ��֧�ָù��ܣ�Ϊ1-��ʾ֧�ָù��ܣ��±��0��ʼ��
	//Ԫ��1-�ϵ���������,Ԫ��2-Զ�̽�ͼ���ܣ����֧����0xE0��0xE1��0xE2��0xE3��0xE4��0xE5������Ч��Ԫ��3-��Ƶ��ʽȴ�����������ܣ�Ԫ��4-��չOSD���ܣ�Ԫ��5-ʣ��֡�ʼ���
	//��˵������Щ�����������ޣ���������֡�ʣ���������֡��ʱ�ῼ�����õ�֡���Ƿ���ʣ��֡�ʷ�Χ�ڣ���Ԫ��6-˫�����طŹ���
	uint8_t reserve1[8];	//����
} STRUCT_SDVR_SUPPORT_FUNC, *LPSTRUCT_SDVR_SUPPORT_FUNC;

//ͨ������¼��״̬
typedef struct 
{ 
	uint8_t   channel[128];	//ͨ������¼��״̬ 1.���п���¼�� 0.ֹͣ����¼�� 
	uint8_t   reserve[4];		//���� 
}STRUCT_SDVR_CHAN_CARDCOR, *LPSTRUCT_SDVR_CHAN_CARDCOR;

//���öԽ��������ݸ�ʽ
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

#define HB_TEST_MAX_HD          32           //�������Ĵ�����
#define HB_TEST_LOSTINFO        5            //��¼��֡��Ϣ����
#define HB_TEST_DEVNAME       64           //�豸�ͺ���󳤶�

typedef struct
{
	uint16_t year ;       //�� ��2009
	uint16_t month ;      //�� 1-12
	uint8_t   day ;       //�� 1-31
	uint8_t   hour ;      //Сʱ 0-23
	uint8_t   minute ;    //���� 0-59
	uint8_t   second ;    //�� 0-59
	DWORD msec;        //����
} PT_TIME_S, *PPT_TIME_S;

typedef struct
{
	DWORD          enable;                     //�Ƿ�ִ�д������ʹ�ܣ�0-��ִ�У�1-ִ��
	DWORD          times;                      //��ʱ���ã� ���������ɿͻ���������
} HB_TEST_REBOOT_IN_S;

typedef struct
{
	DWORD          flag;                       //�����ɹ�����ʧ�ܱ�־��0Ϊʧ�ܣ�1Ϊ�ɹ�, 2Ϊδ������
	DWORD          reserve;
} HB_TEST_REBOOT_OUT_S;

typedef struct
{
	uint8_t           flag;                       //���̼��״̬��0Ϊδ��⵽���̣�1Ϊ��⵽����
	uint8_t           dev_no;                     //�豸��(������Ŷ�Ӧ����0��ʼ)
	uint8_t           devtype;                    //�豸����
	uint8_t           reserve[5];
	uint8_t           dev_model[HB_TEST_DEVNAME]; //�豸�ͺ�
	uint64_t       dev_capability;             //�豸����(�ֽ�)
} HB_DEVINFO_OUT_S;

typedef struct
{
	uint8_t           dev_no;                     //�豸��(������Ŷ�Ӧ����0��ʼ)
	uint8_t           devtype;                    //�豸����
	uint16_t          reserve;
} HB_DEVINFO_IN_S;

typedef struct
{
	DWORD          enable;                     //�Ƿ�ִ�д������ʹ�ܣ�0-��ִ�У�1-ִ��
	DWORD          dev_num;                    //������������ô�����豸��������devinfo��Ӧ
	HB_DEVINFO_IN_S devinfo[HB_TEST_MAX_HD];    //�������(��ѡ����λ��ʾ)����ֵ���ʾ��֪ʵ�����ļ����ڽ���Ӳ�̣���Ϊ���Գ���Ա�ʹ�ã�ûֵ����Գ������Ƚϣ�ֱ����������
} HB_TEST_DISK_IN_S;

typedef struct
{
	uint8_t          test_flag;                   //�����������ʾ����״̬��0-û��ִ�в���(�������ݴ�ʱ��Ч)��1-��ִ�в���(�������ݴ�ʱ��Ч)
	uint8_t           dev_nun;                    //���������0-δ��⵽���̣�>0�����̸�����������Ϣ�����devinfo��
	uint8_t           reserve[2];
	HB_DEVINFO_OUT_S    devinfo[HB_TEST_MAX_HD];    //����������洢����ÿ�����̵���Ϣ����dev_nun��0ʱ��Ч
} HB_TEST_DISK_OUT_S;


typedef struct
{
	DWORD          enable;                     //�Ƿ�ִ�д������ʹ�ܣ�0-��ִ�У�1-ִ��
	DWORD          ch_mask_in[2];              //�������(��ѡ����λ��ʾ)������¼��ͨ������Ϊ�������(��0)����֪����������Щͨ��������Ĭ�ϰ�����ͨ�����
} HB_TEST_REC_IN_S;

typedef struct
{
	uint8_t           result;                     //�����������Ӧͨ��¼������, 1Ϊ�ɹ���0Ϊʧ��, 2Ϊδ������ 
	DWORD          delay_time;                 //�����������¼¼�����������ӳ�ʱ��(��Ҫ�ǿ���¼��ʵ��д�����������ʱ��)��ֻ��¼����ͨ�������ʱ��    
}HB_TEST_REC_CHINFO;

typedef struct
{
	HB_TEST_REC_CHINFO  ch_check_info[MAX_CH];  //�����������ͨ��¼��������Ϣ
} HB_TEST_REC_OUT_S;

typedef struct
{
	uint16_t          format_devno;               //��ʽ�����̺�
	uint16_t          format_devtype;             //��ʽ����������(PT_DEVTYPE_E)
} HB_FORMAT_ININFO_S;

typedef struct
{
	uint16_t            format_devno;               //��ʽ�����̺�
	uint16_t			format_devtype;             //��ʽ����������
	uint16_t            format_time;                //��ʽ������ʱ�䣬��λΪ��
	DWORD          format_flag;                //��ʽ���ɹ���ʧ�ܣ�0-ʧ�ܣ�1-�ɹ���2-δ��⵽����
} HB_FORMATINFO_S;

typedef struct
{
	uint16_t          enable;                     //�Ƿ�ִ�д������ʹ�ܣ�0-��ִ�У�1-ִ��
	uint16_t          format_num;                  //��Ҫ��ʽ�����̸���
	HB_FORMAT_ININFO_S formatinfo[HB_TEST_MAX_HD];//��Ҫ��ʽ��������Ϣ��format_num��0ʱ��Ч
	DWORD          format_times;               //ִ�и�ʽ�����Դ�����û��ѡ����������ʱ��Ч������Ĭ��ִֻ��һ��
} HB_TEST_FORMAT_IN_S;

typedef struct
{
	uint8_t          test_flag;                   //�����������ʾ����״̬��0-û��ִ�в���(�������ݴ�ʱ��Ч)��1-��ִ�в���(�������ݴ�ʱ��Ч)
	uint8_t          format_num;                  //����������Ѹ�ʽ��Ӳ�̸���
	uint8_t          reserve[2];
	HB_FORMATINFO_S formatinfo[HB_TEST_MAX_HD]; //���������ÿ��Ӳ�̸�ʽ����Ϣ�������ɹ���ʧ�ܼ�ÿ��Ӳ�̸�ʽ��ʱ��
} HB_TEST_FORMAT_OUT_S;

typedef struct
{
	DWORD          lost_framecount;            //��ʧ֡��������ʵ�ʱ���֡�ʵĲ�ֵ
	DWORD          lost_fcount;                //��ʧ֡����������0��ʾ��ʧ��һ��֡��(��֡�Ų�����)
	PT_TIME_S       lost_time;                  //��ʧ֡����ʱ��(��ȷ����)
} HB_RECDATA_INFO_S;

typedef struct
{
	DWORD          enable;                     //�Ƿ�ִ�д������ʹ�ܣ�0-��ִ�У�1-ִ��
	DWORD          channel;                    //�������(��ѡ)��ָ������ͨ������֪����������Щͨ��(��0)������Ĭ�ϵ�һͨ��
	DWORD          frame_deviation;            //������������ö�֡�ĺ���ƫ�Χ�����û�����ã�Ĭ��ƫ��Ϊ2���������֡������2֡����Ϊ����
	PT_TIME_S       rec_time_start;             //���������Ҫ����¼�����ݶε���ʼʱ��
	PT_TIME_S       rec_time_stop;              //���������Ҫ����¼�����ݶεĽ���ʱ��
} HB_TEST_REC_PB_IN_S;

typedef struct
{
	DWORD          test_flag;                  //�����������ʾ����״̬��0-û��ִ�в���(�������ݴ�ʱ��Ч)��1-��ִ�в���(�������ݴ�ʱ��Ч)
	DWORD          check_result;               //���������Ϊ0��ʾû�ж�֡��>0��ʾ���ֶ�֡����
	DWORD          frame_venc;                //����֡��
	DWORD          a_framecount;             //��Ƶ֡��
	DWORD          v_framecount;             //��Ƶ֡��
	DWORD          i_framecount;              //��ƵI֡��
	HB_RECDATA_INFO_S lost_info[HB_TEST_LOSTINFO];//������������������֡��ʱ��㼰��ʧ��֡��������¼HB_TEST_LOSTINFO��
} HB_TEST_REC_PB_OUT_S;

typedef struct
{
	HB_TEST_REBOOT_IN_S    reboot_para;         //������������ɿͻ��������ƣ�����������Ч
	HB_TEST_DISK_IN_S      disk_para;           //Ӳ��ʶ�������ز���
	HB_TEST_REC_IN_S       record_para;         //¼�������رղ�����ز���(Ĭ���ֶ�¼��)����ʵ���Ƿ�д����Ϊ׼
	HB_TEST_FORMAT_IN_S    format_para;         //Ӳ�̸�ʽ���������
	HB_TEST_REC_PB_IN_S    analyse_para;        //¼������֡������ز���

} HB_TEST_DISK_AUTO_IN_S;

typedef struct
{
	HB_TEST_REBOOT_OUT_S    reboot_test;        //������������ɿͻ��������ƣ�����������Ч
	HB_TEST_DISK_OUT_S      disk_test;          //Ӳ��ʶ�������Ϣ��������
	HB_TEST_REC_OUT_S       record_test;        //¼�������رղ�����Ϣ����
	HB_TEST_FORMAT_OUT_S    format_test;        //Ӳ�̸�ʽ�����Խ������
	HB_TEST_REC_PB_OUT_S    data_analyse_test;  //¼������֡�����������
} HB_TEST_DISK_AUTO_OUT_S;

typedef struct
{
	DWORD ptz_type;           //��̨Э������
	DWORD ptz_baudrate;    //������
	DWORD reserve[4];        //�����ֶ�
} PTZ_PARAM, *PPTZ_PARAM;

typedef struct
{
	uint8_t stop_flag;               //ֹͣλ 1ֹͣ 0����
	uint8_t show_split_time;         //��ʾ����ָ�ʱ��
	uint8_t show_page_time;          //��ʾҳ��ʱ��
	uint8_t switch_screen_time;      //����ʱ��
	uint8_t playback_channel[64];    //�ط�ͨ��
	DWORD playback_time;          //�ط�ʱ��
	DWORD alarm_out_time;         //�������ʱ��
	MFS_FIELD_TIME sys_time;         //ϵͳʱ��
	PTZ_PARAM ptz_param;           //������̨����
	DWORD show_disk_info_time;     //��ʾ������Ϣ�������ʱ��+
	DWORD show_logo_time;          //��ʾlogo��ʱ��+
	DWORD show_system_info_time;   //��ʾϵͳ��Ϣ��ʱ��+
	DWORD reserve[5];             //�����ֶ�
} PT_AUTO_TEST, *PPT_AUTO_TEST;    //�����Զ�������

typedef struct
{
	uint8_t   test_type;                          //���ò�������(HB_TEST_TYPE_E)����union��Ա��Ӧ
	uint8_t   reserve[15];

	/*���������ͷֱ�ȡ����Ľṹ��ÿ��ֻ�ܶ�Ӧһ��*/
	union
	{
		PT_AUTO_TEST      product_test_para;    //�������Զ������Բ���
		HB_TEST_DISK_AUTO_IN_S disk_test_para;    //����ʶ���Զ������Բ���
	};

	DWORD  reserve2[4];
} HB_TEST_AUTO_IN_S, *LPHB_TEST_AUTO_IN_S;

typedef struct
{
	uint8_t   test_type;                          //�ϴ���������(HB_TEST_TYPE_E)����union��Ա��Ӧ
	uint8_t   reserve[15];

	/*���������ͷֱ�ȡ����Ľṹ��ÿ��ֻ�ܶ�Ӧһ��*/
	union
	{
		HB_TEST_DISK_AUTO_OUT_S  disk_test_out;  //����ʶ���Զ������Խ�����
	};

	DWORD  reserve2[4];
} HB_TEST_AUTO_OUT_S, *LPHB_TEST_AUTO_OUT_S;


//IPC���߲���
typedef struct
{
	uint8_t safeoption;		//��ȫѡ�����ã�ȡֵ��Χ[0,2]  0:�Զ�ѡ��  1������ϵͳ   2��������Կ
	uint8_t pswformat;		//��Կ��ʽ���ã�ȡֵ��Χ[0,1]  0��16����   1��ASCII��
	uint8_t pswtype;			//�� Կ �� �����ã�ȡֵ��Χ[0,3]   0������  1��64λ  2:128λ   3:152λ
	uint8_t pswword[62];		//���룬�ԡ�\0����β������62byte��Ϊ����STRUCT_SDVR_IPCWPAPSK�ȴ�С��
	//��ע�����볤��˵����ѡ��64λ��Կ������16�������ַ�10��������ASCII���ַ�
	//5����ѡ��128λ��Կ������16�������ַ�26��������ASCII���ַ�13����
	//ѡ��152λ��Կ������16�������ַ�32��������ASCII���ַ�16����
	uint8_t reserve[3];		//����
}STRUCT_SDVR_IPCWEP,*LPSTRUCT_SDVR_IPCWEP;

typedef struct
{
	uint8_t safeoption;		//��ȫѡ�����ã�ȡֵ��Χ[0,2] 0���Զ�ѡ��   1��WPA-PSK    2:WPA2-PSK
	uint8_t pswmod;			//���ܷ�������,ȡֵ��Χ[0,2]  0���Զ�ѡ��   1��TKIP   2:AES
	uint8_t pawword[64];	//psk���룬8��63���ַ����ԡ�\0����β
	uint8_t reserve[2];			//����
}STRUCT_SDVR_IPCWPAPSK,*LPSTRUCT_SDVR_IPCWPAPSK;

typedef struct
{
	DWORD nSize;			//������ӣ��ṹ�峤�ȡ�
	uint8_t ssid[50];			//SSID���ԡ�\0����β
	uint8_t wirelessIP[16];	//����ip�ԡ�\0����β
	uint8_t safetype; 			//��ȫ�������ã� 0��WEB��1��WPA-PSK/WPA2-PSK��2���޼���
	uint8_t reserve[3];			//����  	
	union{
	//��Ϊ���������ṹ�岻����ͬʱʹ�ã������������塣
	STRUCT_SDVR_IPCWEP ipcwep;			//��ȫ����ΪWEPʱ�����ṹ��
	STRUCT_SDVR_IPCWPAPSK ipcwpapsk;		//��ȫ����ΪWPA-PSK/WPA2-PSKʱ�����ṹ��
	}u;
}STRUCT_SDVR_IPCWIRELESS,*LPSTRUCT_SDVR_IPCWIRELESS;

typedef struct                           //key�ļ���Ϣ
{
	char   internal_ver[16]; //�ڲ��汾��
	char   external_ver7004t[16]; //�ⲿ�汾��7004t
	char   external_ver7008t[16]; //�ⲿ�汾��7008t
	char   external_ver7016t[16]; //�ⲿ�汾��7016t
	char   external_ver8004t[16]; //�ⲿ�汾��8004t
	char   external_ver8008t[16]; //�ⲿ�汾��8008t
	char   external_ver8016t[16]; //�ⲿ�汾��8016t
	char   external_ver700xt[16]; //�ⲿ�汾��700xt
	char   external_ver800xt[16]; //�ⲿ�汾��800xt
	uint32_t lang_ver;     //���԰汾  0�����ģ�1��Ӣ��
	uint32_t logo_ver;     //logo�汾 0�����1������
	uint32_t lang_maskl;  //֧�ֵ����������32 λ����32������
	uint32_t lang_maskh;  //֧�ֵ����������32 λ��������32������
	unsigned short oem_type;  //OEM ���ͣ��������ֲ�ͬ�Ŀͻ�
	char   full_pb_flag;      //�Ƿ�֧��ȫ�ط� 0��֧��,1֧��
	char   reserve[21];
}key_field ;

typedef  struct                             //logo��Ϣ
{
	char      file_path[256];             //������·����ͼƬ����
	uint32_t   logo_support;          //�Ƿ�֧�֣�1-֧��, 0-��֧��
	uint32_t   file_format;          //bit:0-֧�ֵ�ͼƬ��ʽΪjpg ,1-֧�ֵ�ͼƬ��ʽΪbmp , 2-֧�ֵ�ͼƬ��ʽΪYUV
	uint32_t   max_width;            //ͼƬ����;
	uint32_t   max_height;           //ͼƬ����;
	uint32_t   min_width;            //ͼƬ����
	uint32_t   min_height;           //ͼƬ��С��
	uint32_t   logo_bitdepth;        //ͼƬλ��ȣ�����λ��Ϊ8����7λΪ1��λ��Ϊ12����11λΪ1����������
	uint32_t   logo_size;            //ͼƬ��С;
	uint32_t   reserver;             //����;
}LOGO;

typedef struct
{
	char       dev_name[64];                //������������
	char       alias_name[64];              //�޸ĺ��������
	key_field  key_info;                    //key�ļ���Ϣ
	char       key_path[256];               //����key, devinfo.ini�����ļ�
	LOGO       logo[10];                    //logo��Ϣ  0-uboot 1-״̬ 2-����Ƶ�ź� 3-ˮӡ 4-osd���Ͻ� 5-IE  
	uint32_t reserve;            //������Ϣ
} LOGO_UPDATE_INFO, *LPLOGO_UPDATE_INFO;

typedef struct 
{ 
	int32_t local_ch;     //��������ͨ����
	int32_t dev_type;     //�豸���ͣ�0-IPC ��1-NVR
	int32_t dev_chnum;    //�豸ͨ������Ŀǰ����������Ϊ1����ʾ��IP �豸ֻ��һ��ͨ��
	int32_t dev_ch;       //�豸ͨ������dev_chnum ����1 ʱ����ֵ��ʾѡ���IP �豸���ĸ�ͨ����
	//����IP �豸ͨ����ϵ�ж�����ʵʱ����ͨ�������ȶ������IP �豸�Ĵ�ͨ�����Եġ� 
	int32_t proto_type;   //�豸Э������ѡ��0-HB,1-ONVIF
	int32_t port;         //�˿ںţ�HB Э���ʾ����˿ڣ�ONVIF ��ʾHTTP �˿�
	char ip[128];     //�豸IP 
	char usr_name[32]; //�û��� 
	char pass_wd[32]; //���� 
	int32_t stream_statue; //IP �豸ͨ����״̬��0-�Ͽ���1-���ӡ���ȡʱ���ã����ʱ��ֵ��ʹ��
	char reserve[4];  //���� 
}STRUCT_SDVR_IPDEV_INFO; 

//���IP�豸
typedef struct 
{ 
	int32_t add_num;              //���IP �豸����
	STRUCT_SDVR_IPDEV_INFO    add_info[64];  //��ӵ�IP �豸��Ϣ��64 ��ʾ������ӵ�IP �豸���� 
	//������ֵ��add_num ��������add_num Ϊ8���������ֻ��ǰ8 ��ֵ��Ч�� 
	//����Ķ�����Чֵ��ǰ8 ��ֵÿһ����ʾһ��IP �豸 
	//char reserve[4];          //����
	char edit_flag;//0-�Ǳ༭ģʽ��ӣ�1-�༭ģʽ��� 
	char reserve[3];
}STRUCT_SDVR_ADD_IPDEV, *LPSTRUCT_SDVR_ADD_IPDEV; 

//ɾ��IP�豸
typedef struct 
{ 
	int32_t del_num;      //Ҫɾ����IP �豸����
	int32_t local_ch[64]; //Ҫɾ���ĵ�IP �豸�ı���ͨ������Ϣ��64 ��ʾ������ӵ�IP �豸����
	//������ֵ��del_num ��������del_num Ϊ8���������ֻ��ǰ8 ��ֵ��Ч�� 
	//����Ķ�����Чֵ��ǰ8 ��ֵÿһ����ʾһ��IP �豸 
	char reserve[4];  //���� 
}STRUCT_SDVR_DEL_IPDEV, *LPSTRUCT_SDVR_DEL_IPDEV;

//������ӵ�IP�豸��Ϣ
typedef struct 
{ 
	int32_t total_num; //��ӵ�IP ͨ������
	STRUCT_SDVR_IPDEV_INFO   ipdev[64]; //IP ͨ������Ϣ��64 ��ʾ������ӵ�IP ͨ������ 
	//������ֵ��total_num ��������total_num Ϊ16���������ֻ�� 
	//ǰ16 ��ֵ��Ч������Ķ�����Чֵ�� 
	//ǰ16 ��ֵÿһ����ʾһ��IP ͨ���� 
	char reserve[4]; //���� 
}STRUCT_SDVR_ALLIPCH_INFO, *LPSTRUCT_SDVR_ALLIPCH_INFO; 

//��ӵ�ĳ��IP�豸��Ϣ
typedef struct 
{ 
	STRUCT_SDVR_IPDEV_INFO     ipdev; //������Ϣ���� 
	char reserve[4]; //���� 
}STRUCT_SDVR_IPCH_INFO, *LPSTRUCT_SDVR_IPCH_INFO; 

//IP�豸�������
typedef struct 
{ 
	int32_t local_ch;                  //����ͨ����
	STRUCT_SDVR_NETINFO   netinfo; //���������STRUCT_SDVR_NETINFO ΪЭ�����нṹ�� 
	char reserve[4];                //���� 
}STRUCT_SDVR_SET_IPDEV_NETPARAM, *LPSTRUCT_SDVR_SET_IPDEV_NETPARAM; 

//IP�豸ϵͳʱ��
typedef struct 
{ 
	int32_t local_ch;            //����ͨ����
	STRUCT_SDVR_TIME  time; //����ʱ�䣬STRUCT_SDVR_TIME ΪЭ�������нṹ�� 
	char reserve[4];         //���� 
}STRUCT_SDVR_SET_IPDEV_TIME, *LPSTRUCT_SDVR_SET_IPDEV_TIME; 

//����ͨ������״̬
typedef struct 
{ 
	char localch_status[256]; //����ͨ��״̬���Ƿ����ã�0-δ���ã����ã���1-����,2-IPC����ʾ�ѽ�IP�豸,��ֵֻ�����ڻ�ȡ�������������ã���256 Ϊ���ͨ���� 
	char reserve[4];     //���� 
}STRUCT_SDVR_LOCH_STATUE, *LPSTRUCT_SDVR_LOCH_STATUE; 

//�������Ͳ���
typedef enum
{
	HBTEST_CMD_TEST_START = CMDNO(0 , COMP_TEST),//��ʼ�Զ������ԣ���Ӧ����HB_TEST_AUTO_IN_S
	HBTEST_CMD_TEST_STOP,                       //ֹͣ����, ��Ӧ����HB_TEST_AUTO_IN_S, �ṹ���е�test_type����Ч����
	HBTEST_CMD_REGIST_CALLBACK,                 //ע��ص�(���ղ��Խ������,������ΪHB_TEST_AUTO_OUT_S),channelΪע������(��ʱ��Ч),�ص���������ΪPNETAPICALLBACK
	HBTEST_CMD_DATA_CALLBACK,                   //���ݻص������ʱû������

} HB_TEST_CMD_E;

typedef struct
{
	char support_capability[64];   //�豸֧������(���������һһ��Ӧ,64Ϊ֧�ֵ����������) 
	char support_protocol[3]; //�豸Э��֧�֣�8Ϊ֧�ֵ����Э������Ŀǰ��������������0-����Э�飬1-ONVIFЭ��
}STRUCT_SDVR_IPC_SUPPORTINFO;

typedef struct
{
	int32_t port;     //�˿ڣ�����Э���ʾ����˿ںţ�ONVIFЭ���ʾHTTP�˿ں�
	char ip[128];    //IP��ַ
	char mac_addr[8];     //MAC��ַ
	char vendor_name[32];     //������
	int32_t protocol_type;     //��ǰ��Э�����ͣ�0-HB,1-ONVIF,2-HBGK_EXT,3-AVIPC,4-SAMSUNG,5-WAYULINK
	char reserve[16];   //����
}STRUCT_SDVR_DEVSEACHCONTENT;

typedef struct
{
	int32_t num;   //�ѵ���IP�豸����
	STRUCT_SDVR_DEVSEACHCONTENT ipc_devsearch_content[256];   //�ѵ���IP�豸���ݣ�256Ϊ֧�ֵ����IP�豸������
	char reserve[16];     //����
}STRUCT_SDVR_SEACH_IPDEV, *LPSTRUCT_SDVR_SEACH_IPDEV;

typedef enum
{
	GET_ZDWX_PT_CONFIG = 0xF010, //��ȡ�������ҩ��ƽ̨��������
	SET_ZDWX_PT_CONFIG,          //�����������ҩ��ƽ̨��������
	GET_ZDWX_TMP_HUMI_CONFIG,    //��ȡ��ʪ���豸��������
	SET_ZDWX_TMP_HUMI_CONFIG,    //������ʪ���豸��������
	GET_ZDWX_GLOBAL_CONFIG,      //��ȡ�����豸��ʼID����Χ���������
	SET_ZDWX_GLOBAL_CONFIG,      //���������豸��ʼID����Χ���������
	SET_ZDWX_FINGLE_TMP_CONFIG,      //����ָ��ģ��
	CLEAR_ZDWX_FINGLE_TMP_CONFIG,    //���ָ��ģ��
	RETURN_ZDWX_FINGER_PRINT,         //�û���ָ�ƺ�Ľ��
	SET_ZDWX_CALLBACK_UPSEND,          //��ʾ����Ѿ�������DVR���Ѿ�׼��   ���˽��ջص�����
	UPLOAD_ZDWX_FINGLE_COMPARE_CONFIG,  //�ϴ�ָ�ƱȶԽ��
	UPLOAD_ZDWX_FINGLE_IMAGE_CONFIG,  //�ϴ�ָ��ͼ������
	UPLOAD_ZDWX_TMP_HUMI_DEVINFO_CONFIG,  //�ϴ���ʪ���豸�����Ϣ
	UPLOAD_ZDWX_TMP_HUMI_CONFIG,  //������ʪ���豸�ɼ���Ϣ
	GET_ZDWX_HISTORY_DATA,      //��ʷ���ݲ�ѯ
	GET_ZDWX_FINGLE_CONPARE_TYPE,      //ƽ̨��ȡ����ָ�ƱȶԷ�ʽ  1���ضԱ� 2���������жԱ�
	SET_ZDWX_FINGLE_CONPARE_TYPE,      //ƽ̨��������ָ�ƱȶԷ�ʽ
	GET_ZDWX_DATA_SAVE_TYPE,      //ƽ̨��ȡ�������ݲɼ��洢��ʽ   1����ת�� 2���ر��� 3����ת��������
	SET_ZDWX_DATA_SAVE_TYPE,      //ƽ̨�����������ݲɼ��洢��ʽ
	UPLOAD_ZDWX_TMP_HUMI_ONLINE_INFO,  //�ϴ���ʪ���豸����״̬
	UPLOAD_ZDWX_FINGLE_ONLINE_INFO,  //�ϴ�ָ�����豸����״̬
	GET_ZDWX_MEDICINE_INTERVAL_TM,      //��ȡҩ���豸״̬�ϱ����ʱ�� 
	SET_ZDWX_MEDICINE_INTERVAL_TM,      //����ҩ���豸״̬�ϱ����ʱ�� 
} ZDWX_PT_CMD_E;

//ҩ��ƽ̨����
typedef struct 
{
	char    platform_addr[128];
	int32_t    platform_port;
	char    dvr_register_id[64];        //ע�����к�ID
}WXYJ_platform_config, *PWXYJ_platform_config;

//��ʪ�ȴ���������
typedef struct 
{
	int32_t dev_id;             // �������豸��� 1-999

	/* �������ɼ�������Ϣ */
	int32_t tmp;                 //�¶�
	int32_t hui;                 //ʪ��
	int32_t baty;                //����
	/* ��ȡ�����ô�����������Ϣ */
	int32_t tempr_up;           // �¶�����
	int32_t tempr_down;	       // �¶�����
	int32_t hunty_up;           // ʪ������
	int32_t hunty_down;         // ʪ������
	int32_t collect_time;       // ��ʪ�ȴ������ɼ����ʱ�� ��
}WXYJ_LOCAL_PARAM, *PWXYJ_LOCAL_PARAM;

//��ʪ����Ϣ
typedef struct 
{
	int32_t first_search_id;    // �豸������ʼID    ֵ1-999   Ĭ��955
	int32_t search_range;	   // �豸������Χ      ֵ1-50    Ĭ��20
	int32_t search_time;        // dvr�����豸ʱ��   ֵ1-1800  Ĭ��300��
}WXYJ_GLOBAL_PARAM, *PWXYJ_GLOBAL_PARAM;

//ָ��ģ��
typedef struct
{
	char   finger_feature[360];   // ָ��������
	char   usr_name[48];          // �û��� 
	char   ID_card[32];           // ���֤��
	char   reserve[32];
} WXYJ_FINGER_CONFIG, *PWXYJ_FINGER_CONFIG;

//�ɼ���Ϣ
typedef struct
{
	int32_t total_count;
	WXYJ_LOCAL_PARAM tmp_humi_dev[MAX_HUMTMP_DEV];
}WXYJ_TMP_HUMI_DEV_INFO, *PWXYJ_TMP_HUMI_DEV_INFO;

/* δ�ȶԵ�ָ��������Ϣ */
typedef struct
{
	uint32_t  size;  // ָ�����ݳ���
	char   outbuf[152 * 200 + 16];  // ָ��ͼ�����ݱ���buffer
} WXYJ_FINGER_UNCOMPARE_DATA, *PWXYJ_FINGER_UNCOMPARE_DATA;

/* �ȶԺ��ָ��������Ϣ, ƽ̨���ص�ָ��ģ�� */
typedef struct
{
	char   finger_feature[344];   // ָ��������
	char   ID_card[20];          // �û��� 
	char   usr_name[45];           // ���֤��
	char   reserve[3];
}WXYJ_FINGER_COMPARE_DATA, *PWXYJ_FINGER_COMPARE_DATA;

/* �豸����״̬ö�� */
typedef enum
{
	PERIPHERA_WORKING_OFFLINE = 0,  //�豸�����ߣ������쳣
	PERIPHERA_WORKING_ONLINE,       //�豸���ߣ���������
}PT_PERIPHERA_WORKING_E;


/* �������豸״̬*/
typedef struct 
{
	int32_t dev_id;              //�������豸��� 1-500
	int32_t work_state;          //����״̬,PT_PERIPHERA_WORKING_E
	int32_t baty;                //��ص���
} PT_HUMTR_SENSOR_WORK_S, *PPT_HUMTR_SENSOR_WORK_S;

/* ��ʪ���豸����״̬�ϱ� */
typedef struct 
{
	int32_t                   work_state;       //��ʪ�Ƚ���������״̬,PT_PERIPHERA_WORKING_E
	PT_HUMTR_SENSOR_WORK_S   sensor_work[MAX_HUMTMP_DEV];  //����������״̬
} PT_HUMTR_WORK_S, *PPT_HUMTR_WORK_S;

/* ҩ���豸����״̬�ϱ�ʱ���� */
typedef struct 
{
	DWORD						dev_type;       //ҩ���豸����
	DWORD						interval_time;  //�ϱ����ʱ�䣬��λ�룬��С5*60�룬���24*60*60��
} PT_MED_WORKING_INTERVAL_TM_S, *PPT_MED_WORKING_INTERVAL_TM_S;

//ͨ��¼��Ԥ¼����¼ʱ��
typedef struct 
{ 
	DWORD    channel;        // ͨ���� 
	DWORD    type;           // ¼�����ͣ�0x01-�ֶ�¼��,0x02-��ʱ¼��, 
	// 0x04-�ƶ�¼��,0x08-����¼��,0xff-����¼�� 
	DWORD    prerecord_time; // Ԥ¼ʱ��,5-30 �� 
	DWORD    delayrecord_time; // ��¼ʱ�䣬0-180 �� 
	uint8_t   reserver[4];      // ���� 
}STRUCT_SDVR_RECTIME, *LPSTRUCT_SDVR_RECTIME;

//Զ��ͼƬ��ѯ
typedef struct 
{ 
	uint8_t  byChannel;    //ͨ����[0, n��1]��n:ͨ���� 
	uint8_t  dwType;       //��ѯ��ͼƬ���ͣ��±���ֵ��ͼƬ���Ͷ�Ӧ��ϵ 
	uint8_t  pic_format;  // ͼƬ��ʽ��0--jpg ��1-bmp ��Ŀǰֻ����jpg ��ʽ
	uint8_t  reverse;		//����
	UNMFS_TIME  starttime; //��ѯ�Ŀ�ʼʱ�� 
	UNMFS_TIME  endtime;  //��ѯ�Ľ���ʱ�� 
	DWORD   dwStart;     //�ӵڼ����ļ���ʼ��ѯ��һ��Ϊ0(��������ʱ������ļ���������ֶ� 
	//�β�ѯ���������һ�β�ѯΪ0���Ժ�ݼӳɹ���ѯ�����ļ���) 
	uint16_t   dwNum;       //һ�β�ѯ�ĸ��������ڶ�����100 
	uint8_t   reverse1[6]; //����
}STRUCT_SDVR_SNAPSHOTFIND;

typedef struct 
{ 
	UNMFS_TIME  time;     // ͼƬ��ץ��ʱ�䣬�ṹͬ�ϣ����-2000 
	uint32_t lengh; // ͼƬ��С
	uint8_t  ch;           //ץ�ĵ�ͨ����
	uint8_t   dwType;         // ͼƬ���ͣ�1--�ֶ���2--��ʱ��4--�ƶ���8--̽ͷ��0xff--ȫ�� 
	uint8_t   pic_format;     // ͼƬ��ʽ��0--jpg ��1-bmp ��Ŀǰֻ����jpg ��ʽ 
	uint8_t   cRes[5];          //���� 
}Mfs_SnapShotSeg_Info, *PMfs_SnapShotSeg_Info; 

typedef struct 
{ 
	DWORD    Totalnum;     //���ҵ����ļ����� 
	Mfs_SnapShotSeg_Info recdata[100]; 
}STRUCT_SDVR_SNAPSHOTINFO, *LPSTRUCT_SDVR_SNAPSHOTINFO;

//IPV6�������
typedef struct	
{
	DWORD ip_mode;                        //���ڱ�ʾ����ģʽ��0��ʾIPV4ģʽ��1��ʾIPV6ģʽ
	DWORD dwNetInterface;		     //����ӿ� 1-10MBase-T 2-10MBase-Tȫ˫�� 3-100MBase-TX 4-100Mȫ˫�� 5-10M/100M����Ӧ
	uint16_t wDVRPort;		            //�˿ں� 
	uint8_t byMACAddr[MACADDR_LEN];          //�����������ַ
	uint8_t strDVRdefgatewayIP[64];          //DVR Ĭ������IP��ַ����'\0'��β
	uint8_t  strDNSIP[64];	                   //DNS��������ַ ����'\0'��β
	uint8_t  strSECDNSIP[64];	            //DNS��������ѡ��ַ����'\0'��β
	DWORD dhcp_enable;                    //����DHCP�Ƿ����ã�0��ʾδ���ã�1��ʾ���� 
	uint8_t strDVRIP[64];	                   //DVR IP��ַ ����'\0'��β  
	uint8_t strDVRMASKIP[64];		     //DVR ���������ַ������'\0'��β��IPV6���������ע  
	uint8_t strDVRLOCALIP[64];	            //DVR ��·����IP��ַ  ����'\0'��β
	DWORD dwDVRIPnetpre;	            //DVR IP����ǰ׺λ��,IPV4���������ע
	uint8_t  reserved[64];                   //����	 
}STRUCT_SDVR_ETHERNET_6,*LPSTRUCT_SDVR_ETHERNET_6;

typedef struct	 
{
	DWORD dwSize;          
	DWORD default_network;                        //���ڱ�ʾĬ��������ţ��磺0��ʾeth0,1��ʾeth1,2��ʾeth2����������
	STRUCT_SDVR_ETHERNET_6 struEtherNet[MAX_ETHERNET]; //��̫���� 

	uint8_t  strManageHostIP[64];				//Զ�̹���������ַ����'\0'��β	 
	uint16_t wManageHostPort;				//����

	uint8_t  strMultiCastIP[64];				//�ಥ���ַ����'\0'��β
	uint8_t  strNFSIP[64];				//���棬��'\0'��β
	uint8_t sNFSDirectory[PATHNAME_LEN];	                    //����
	DWORD dwPPPOE;						//0-������,1-����
	uint8_t sPPPoEUser[NAME_LEN];			//PPPoE�û���	 
	char sPPPoEPassword[PASSWD_LEN];	//PPPoE����
	uint8_t  strPPPoEIP[64];					//PPPoE IP��ַ(ֻ��)����'\0'��β
	uint16_t wHttpPort;					//HTTP�˿ں�
	uint8_t  reserve[128];                                    //����
}STRUCT_SDVR_NETINFO_6,*LPSTRUCT_SDVR_NETINFO_6;

//��չOSDЭ��
typedef struct
{
	DWORD type;//0-�������ƣ�1-ͨ�����ƣ�2-��չOSD1��3-��չOSD2��4-��չOSD3��5-��չOSD4��6-��չOSD5
	DWORD  channel;//ͨ���ţ���0��ʼ����type=0ʱ��������Ч
	uint8_t  reverser[16];//����
}STRUCT_SDVR_OSD_CONFIG_REQ;

typedef struct
{
	uint16_t x, y; // xy����
	uint16_t w, h; // ��Ⱥ͸߶ȣ�����Ŀǰδʵ�֣�
} SDVR_RECT;

typedef struct
{
	DWORD type;//0-�������ƣ�1-ͨ�����ƣ�2-��չOSD1��3-��չOSD2��4-��չOSD3��5-��չOSD4��6-��չOSD5
	DWORD  channel;//ͨ���ţ���0��ʼ����type=0ʱ��������Ч     
	SDVR_RECT coordinate;//OSD���꣬��type=0ʱ��������Ч    
	uint8_t  show_enable;//�Ƿ���ʾ��0-����ʾ��1-��ʾ   
	uint8_t  style;//OSD��ʽ��1-͸����˸��2-͸������˸��3-��˸��͸����4-��͸������˸ ������Ŀǰδʵ�֣�   
	uint8_t  alpha;//OSD͸����  0-��͸����1-͸��  
	uint8_t  reverser1; //����    
	DWORD typemaxlen;//type����֧�ֵ�����ֽ�������ֵ������ȷ��������ʱ�ַ����ĳ��Ȳ��ܴ������ֵ  
	DWORD typelen;//type���͵�ǰ��ռ�õ��ֽ���
	uint8_t  reverser2[16];//����
	uint8_t  data[0];//typelen���ȵ��ַ�������'\0'����
}STRUCT_SDVR_OSD_CONFIG_RSP, *LPSTRUCT_SDVR_OSD_CONFIG_RSP;

//��֡�ʺ�ʣ��֡��
typedef struct
{
	DWORD stream_type;//0-¼�����������������1-��������Ŀǰδʵ�֣���2-���루Ŀǰδʵ�֣�
	DWORD reverse[4];//����
}STRUCT_SDVR_FRAMERATE_REQ;

typedef struct
{
	DWORD stream_type;//0-¼�����������������1-��������Ŀǰδʵ�֣���2-���루Ŀǰδʵ�֣�
	DWORD total_freme;//����ͨ������֡��
	DWORD residual_frame;//ʣ��֡��
	DWORD reverse[4];//����
}STRUCT_SDVR_FRAMERATE_TOL, *LPSTRUCT_SDVR_FRAMERATE_TOL;

//Զ��ͼƬ����
typedef struct 
{ 
	uint8_t   byChannel;      //ͨ���� [0, n-1], n:ͨ����
	uint8_t  type;        // ͼƬ���ͣ�1-�ֶ���2-��ʱ��4-�ƶ���8-̽ͷ������0xff-����¼�� 
	uint8_t  pic_format;  // ͼƬ��ʽ��0--jpg ��1-bmp ��Ŀǰֻ����jpg ��ʽ
	uint8_t reverse1;//����
	DWORD   file_index; //�ͻ������ص��ļ��б�������ţ���0 ��ʼ 
	DWORD   file_offset; //�ļ�ƫ�ƴ�С���տ�ʼΪ0 
	UNMFS_TIME  starttime; //��ʼʱ�䣬�ṹ��ͬ�ϣ�ע�����-2000 
	UNMFS_TIME  endtime; //����ʱ��
	uint8_t reverse2[4];//����
}STRUCT_SDVR_BACK_SNAPSHOTINFO;

//Զ��ͼƬ�����ļ�ͷ��־
typedef struct 
{ 
	DWORD   port;      //ͨ���� 
	DWORD   filelen;   //���ļ����� ��B Ϊ��λ 
	UNMFS_TIME  time;    //��ͼƬ�ļ���ץ��ʱ�� 
	DWORD   totalFileLen; //���ļ����ȣ���ʾNETCOM_BACKUP_SNAPSHOT_REQ ���������� 
	//ʱ����ڵ�ץ��ͼƬ�ܳ��ȣ���KB Ϊ��λ 
	DWORD   file_index; //�ļ��б�������ţ���0 ��ʼ
	DWORD   file_offset; //�ļ�ƫ�ƴ�С���տ�ʼΪ0
	uint8_t  reverse[4];//����
}STRUCT_SDVR_SNAPSHOT_FILEINFO, *LPSTRUCT_SDVR_SNAPSHOT_FILEINFO;

//��̨��Ԥ�õ���Ѳ
typedef struct 
{ 
	DWORD    byChannel;  //����ͨ�� [0, n-1] n:ͨ���� 
	uint16_t   Preset[16];   //Ԥ�õ�[1��254]��255 ����Чֵ�����е����0 ��Ч���е����0 ��Ч�� 
	//���Ԥ�õ�����16 �����������255 
	uint16_t   PresetPoll;   //��Ԥ�õ���Ѳ������رձ�ʾ(0, �رգ�1������) 
	uint16_t   presettime;   //��Ԥ�õ���Ѳ���ʱ��(��λ��) [1, 99] 
}HB_SDVR_PRESETPOLL,*LPHB_SDVR_PRESETPOLL; 

//һ��ͨ ����
typedef struct 
{ 
	char enable;            //����һ��ͨʹ��-1-��֧�֣�0-off, 1-on 
	uint8_t   reserve[31];      //���� 
}STRUCT_SDVR_DYT_STATUE,*LPSTRUCT_SDVR_DYT_STATUE; 

//
typedef struct 
{ 
	int32_t          support_alarmtype;      //���������б��������Ƿ�֧���ϴ�������������ӵ͵��߰�λ��ʾ//STRUCT_SDVR_ALMUPLOADTYPE_E, // λ����: 1-֧�֣�  0-��֧��
	uint8_t           reserve[16];                  //����λ 
} STRUCT_SDVR_ALMUPLOADSUPT_S, *PSTRUCT_SDVR_ALMUPLOADSUPT_S;     

typedef enum 
{ 
	NET_ALM_UP_MOTION,                      // �ƶ���ⱨ�� 
	NET_ALM_UP_VIDEOLOST,                //  ��Ƶ��ʧ���� 
	NET_ALM_UP_VIDEO_COVER,          //  ��Ƶ�ڵ����� 
	NET_ALM_UP_SENSOR,                      //  ̽ͷ���� 
	NET_ALM_UP_TEMP_HI,                    //  �¶ȹ��߱��� 
	NET_ALM_UP_HD_ERROR,                //  ���̴��󱨾� 
	NET_ALM_UP_HD_FULL,           //  ���������� 
	NET_ALM_UP_TYPE_MAX, 
} STRUCT_SDVR_ALMUPLOADTYPE_E; 

typedef struct 
{     
	uint16_t     alm_type;              //  �������ͣ�STRUCT_SDV_ALMUPLOADTYPE_E 
	uint16_t     upload_enable;      //  �����ϴ�ʹ�ܣ�����: 1-�����ϴ���  0-���ϴ� 
	STRUCT_SDVR_SCHEDTIME     upload_time[8][4];        //  �ϴ�����ʱ�������, [0][0]-[0][3]��ÿ�죬������4��ʱ��Ρ�[1��7][0]-[1��7][3],����һ�������죬������ 4��ʱ��� 
	uint8_t          reserve[16];          //����λ 
} STRUCT_SDVR_ALMUPLOADPARAM_S, *PSTRUCT_SDVR_ALMUPLOADPARAM_S;  

typedef struct
{
	int32_t channel;    //ͨ����
	int32_t command;    //���CAMERA_PARAM_SET_CMD_E
	uint8_t reserve[16];   //����
}STRUCT_SDVR_CAMERA_PARAM_SET_CRTL,*LPSTRUCT_SDVR_CAMERA_PARAM_SET_CRTL;

typedef struct 
{ 
	uint8_t enable;        //ʹ�� 
	uint8_t hour;            //ʱ 
	uint8_t min;            //�� 
	uint8_t sec;            //�� 
	uint8_t reserve[4];      //���� 
}STRUCT_SDVR_REBOOT_TIME; 

typedef struct 
{ 
	uint8_t en_time_reboot;                                          //��ʱ����ʹ�ܣ�1-���ã�0-������ 
	uint8_t reserve1[3];                //���� 
	STRUCT_SDVR_REBOOT_TIME week[8];      //����ʱ��([0]-ÿ�죬[1-7]��Ӧ��һ������)��[0]���б�ʹ��
	//ÿ����Ч����ÿ�촦������ע���ڼ�                             
	uint8_t reserve2[16];            //���� 
} STRUCT_SDVR_TIMING_REBOOT_PARAM,*LPSTRUCT_SDVR_TIMING_REBOOT_PARAM; 




//�»ط�Э��
typedef struct TAG_VOD_PARAM
{
	uint8_t byChannel; // ͨ����[0, n-1],n:ͨ����
	uint8_t byType; // ¼������: 1-�ֶ���-��ʱ��-�ƶ���-������xFF-ȫ��
	uint16_t wLoadMode; // �ط�����ģʽ 1-��ʱ�䣬2-������
	union
	{
		struct
		{
			UNMFS_TIME struStartTime; // ���һ����Ȼ��
			UNMFS_TIME struStopTime; // ����ʱ����ൽ23:59:59,
			// ����ʾ�ӿ�ʼʱ�俪ʼһֱ����
			char cReserve[16];
		}byTime;
		uint8_t byFile[64]; // �Ƿ񹻳���
	}mode;
	uint8_t streamtype; //�������ͣ�0-��������1-������
	uint8_t byReserve[15]; //����
}VOD_PARAM, *LPVOD_PARAM;

typedef struct TAG_VOD_ANS
{
	DWORD dwVodID; //��������
	uint8_t streamtype; //�������ͣ�0-��������1-������
	uint8_t byReserve[15]; //����
}VOD_ANSWER, *LPVOD_ANSWER;

typedef struct TAG_VOD_GETDATA
{
	DWORD dwVodID;
	DWORD dwCon; // �Ƿ���������, 0,����; 1,����(struStartTime��Ч);
	UNMFS_TIME struStartTime; // ��ʼʱ��(dwConΪ0ʱ��Ч)
	DWORD dwFrameNum; // ȡ֡����150֡(��)
	DWORD dwFrameType; // 0,����(����Ƶ)֡; 1,��I֡
	char cReserve[16]; // ����
}VOD_GETDATA, *LPVOD_GETDATA;

typedef struct TAG_VOD_GETDATA_ANS
{
	DWORD dwVodID;
	DWORD dwReserve[2]; // ����
	DWORD dwLen; // ���ݳ���
}VOD_GETDATA_ANSWER, *LPVOD_GETDATA_ANSWER;

typedef struct TAG_VOD_PAUSE
{
	DWORD dwVodID; // �㲥ID
	DWORD dwStat; // ����0,��ͣ; 1,��ͣ�ָ���
	// Ӧ��0,��ͣ; 1,������
	DWORD dwReserve[4]; // ����
}VOD_PAUSE, *LPVOD_PAUSE;

typedef struct TAG_VOD_END
{
	DWORD dwVodID;
	char cReserve[16];
}VOD_END, *LPVOD_END;

typedef struct	{
	uint8_t	byChannel;					//ͨ����
	uint8_t	byLinkMode;					// 0-��������socket  1-������ ��socket  
	uint8_t	OSDCharEncodingScheme;		// OSD�ַ��ı����ʽ 
	uint8_t	reserve[9];					//����
}ST_SDVR_REALPLAY_MULTI,*LPST_SDVR_REALPLAY_MULTI;

#ifdef _MSC_VER
#pragma warning(default: 4200)
#endif
#pragma pack(pop)

#endif
