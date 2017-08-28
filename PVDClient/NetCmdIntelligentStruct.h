#ifndef _NETCMDINTELLIGENTSTRUCT_H_
#define _NETCMDINTELLIGENTSTRUCT_H_

#include "NetCmdStruct.h"

/************************************************************************/
/* ��Ϣ�����                                                                 */
/************************************************************************/
#define NET_SDVR_ALARM_CONNECT                 0xA0   //�������ܱ����ϱ���·
#define NET_SDVR_ALARM_REPORT                  0xA1   //���ܱ����ϱ���Ϣ
#define NET_SDVR_INTELLECTCFG_SET              0xA2   //���ܱ�������
#define NET_SDVR_INTELLECTCFG_GET              0xA3   //���ܱ�����ѯ
#define NET_SDVR_INTELLECTCFG_ALGORITHM_RESET  0xA4   //�����㷨��λ
#define NET_SDVR_ALARM_HEART						0X9A			//���ܱ�������


#ifdef _MSC_VER
#pragma pack(push, 1)
#pragma warning(disable: 4244)
#pragma warning(disable: 4018)
#endif

/************************************************************************/
/* �ϴ����ܱ�����Ϣ                                                             */
/************************************************************************/
//��������
typedef  struct
{
	int32_t  left;                      //����������,CIF(0-352),D1(0-704)
	int32_t  top;                       //����������,CIF(0-352),D1(0-704)
	int32_t  right;                     //����������,CIF(0-352),D1(0-704)
	int32_t  bottom;                    //����������,CIF(0-352),D1(0-704)
}STRUCT_SDVR_ATMI_RECT, *PSTRUCT_SDVR_ATMI_RECT;

//�������ͼ�λ����Ϣ
typedef  struct
{
	int32_t  alarm_type;                   //����,NET_ATMI_ALARM_TYPE_E
	STRUCT_SDVR_ATMI_RECT  position;   //����λ��
}STRUCT_SDVR_ATMI_ALARM_POSITION_S, *PSTRUCT_SDVR_ATMI_ALARM_POSITION_S;

// 1.����ͨ�������ṹ��
typedef  struct
{
	DWORD  alarm_num;                                  //��������
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  alarm_area[10]; //��������ֵ,һ����alarm_num���������ȫΪ0
}STRUCT_SDVR_ATMI_FACE_ALARM_S, *PSTRUCT_SDVR_ATMI_FACE_ALARM_S;

// 2.���ͨ�������ṹ��
typedef  struct
{
	DWORD  alarm_num;                                  //��������
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  alarm_area[10]; //��������ֵ,һ����alarm_num���������ȫΪ0
}STRUCT_SDVR_ATMI_PANEL_ALARM_S, *PSTRUCT_SDVR_ATMI_PANEL_ALARM_S;

// 3.�ӳ����������Ϣ
typedef  struct
{
	DWORD  type;                    //�Ƿ����˴��룬0��ʾ�ޣ�1��ʾ��
}STRUCT_SDVR_ATMI_MONEY_ALARM_S, *PSTRUCT_SDVR_ATMI_MONEY_ALARM_S;

// 4.���������ṹ��,alarm_num����Ӧ��������ǰ��track_num����Ӧ�����������alarm_num�����
typedef  struct
{
	DWORD  alarm_num;               //����Ŀ������
	DWORD  track_num;               //����Ŀ������
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  envi_alarm_region[25];
}STRUCT_SDVR_ATMI_ENVI_ALARM_S, *PSTRUCT_SDVR_ATMI_ENVI_ALARM_S;

//������Ϣ����
typedef  enum
{
	NET_ATMI_FACE_BLOCK = 0,         //�����ڵ�
	NET_ATMI_FACE_NOSIGNAL,          //����ͨ����Ƶ��ʧ
	NET_ATMI_FACE_UNUSUAL,           //�����쳣
	NET_ATMI_FACE_NORMAL,            //��������
	NET_ATMI_PANEL_BLOCK  = 40,      //����ڵ�
	NET_ATMI_PANEL_NOSIGNAL,         //���ͨ����Ƶ��ʧ
	NET_ATMI_PANEL_PASTE,            //����
	NET_ATMI_PANEL_KEYBOARD,         //װ�ټ���
	NET_ATMI_PANEL_KEYMASK,          //�ƻ����������	
	NET_ATMI_PANEL_CARDREADER,       //�ƻ�������
	NET_ATMI_PANEL_TMIEOUT,          //��ʱ����
	NET_ATMI_ENTER,                  //���˽���
	NET_ATMI_EXIT,                   //�˳���
	NET_ATMI_MONEY_BLOCK = 80,       //�ӳ�����Ƶ�ڵ�
	NET_ATMI_MONEY_NOSIGNAL,         //�ӳ���ͨ����Ƶ��ʧ
	NET_ATMI_MONEY_UNUSUAL,          //�ӳ����쳣,�����˴���ӳ���
	NET_ATMI_ENVI_BLOCK = 120,       //����ͨ����Ƶ�ڵ�
	NET_ATMI_ENVI_NOSIGNAL,          //����ͨ����Ƶ��ʧ
	NET_ATMI_ENVI_GATHER,            //���˾ۼ�
	NET_ATMI_ENVI_MANYPEOPLEINREGION,//Υ��ȡ��
	NET_ATMI_ENVI_LINGERING,         //��Ա�ǻ�
	NET_ATMI_ENVI_LIEDOWN,           //��Ա����
	NET_ATMI_ENVI_FORBIDREGION,      //�Ƿ����뾯����
	NET_ATMI_ENVI_LEAVEOBJECT,       //��Ʒ����
}NET_ATMI_ALARM_TYPE_E;

typedef  struct
{
	DWORD  pic_alarm_type;          //NET_ATMI_ALARM_TYPE_E
	DWORD  pic_format;              //ͼƬ��ʽ0-CIF  1-D1
	DWORD  pic_size;                //ͼƬ��С
}STRUCT_SDVR_ATMI_ALARM_PICINFO_S, *PSTRUCT_SDVR_ATMI_ALARM_PICINFO_S;

typedef  enum
{
	HBGK_HDCCOUNT_DIR1 = 0,         // ���Ƿ�����ͬ
	HBGK_HDCCOUNT_DIR2,             // ���Ƿ����෴
}NET_HDCCOUNT_DIRECTION_E;

typedef struct 
{
	DWORD  dwResultType;            //������������
	DWORD  dwSubType;               //�����������ͣ���ʾ��Ա����ͳ�Ƶķ���
	                                //��NET_HDCCOUNT_DIRECTION_E
	DWORD  dwTrackNum;              //��ǰ���ͳ�Ƶ�ID���(��ͳ������)
	STRUCT_SDVR_ATMI_RECT  rcPos;            //��ǰ�����ŵ���Ӿ��ο�
}STRUCT_SDVR_HDC_RESULT, *LPSTRUCT_SDVR_HDC_RESULT;

typedef  struct
{
	uint8_t  byChn;
	uint8_t  byReserver1;
	uint8_t  byInfoType;               // �ϴ���Ϣ���ͣ�0-STRUCT_SDVR_ATMI_FACE_ALARM_S��
	                                // 1-STRUCT_SDVR_ATMI_PANEL_ALARM_S��2-STRUCT_SDVR_ATMI_MONEY_ALARM_S
	                                // 3-STRUCT_SDVR_ATMI_ENVI_ALARM_S��4-STRUCT_SDVR_HDC_RESULT
	uint8_t  byReserver2;
	union
	{
		STRUCT_SDVR_ATMI_FACE_ALARM_S   atmi_face_alarm;   // 1.����ͨ�������ṹ��
		STRUCT_SDVR_ATMI_PANEL_ALARM_S  atmi_panel_alarm;  // 2.���ͨ�������ṹ��
		STRUCT_SDVR_ATMI_MONEY_ALARM_S  atmi_money_alarm;  // 3.�ӳ���ͨ�������ṹ��
		STRUCT_SDVR_ATMI_ENVI_ALARM_S   atmi_envi_alarm;   // 4.����ͨ�������ṹ��
		STRUCT_SDVR_HDC_RESULT          hdc;
	}info;
	STRUCT_SDVR_ATMI_ALARM_PICINFO_S  alarm_picinfo;           //ͼƬ��Ϣ���������ڽṹ����ץ��ͼƬ����
	SYSTIME  alarmtime;                                        //����ʱ��
}STRUCT_SDVR_ATMI_ALARM_INFO_S, *PSTRUCT_SDVR_ATMI_ALARM_INFO_S;

/************************************************************************/
/* ���ܱ�����������                                                             */
/************************************************************************/
//��������
typedef  struct
{
	DWORD  left;                    //����������
	DWORD  top;                     //����������
	DWORD  right;                   //����������
	DWORD  bottom;                  //����������
}STRUCT_SDVR_ATMI_RECT_S;

//һ���������
typedef  struct
{
	uint16_t  x;                        //������
	uint16_t  y;                        //������
}STRUCT_SDVR_ATMI_POINT_S;

//��������
typedef enum
{
	NET_ATM_REGIONTYPE_WARN         = 1,   /* Warn region.     ��ֹճ����                       */
	NET_ATM_REGIONTYPE_NOWARN       = 0,   /* Indicate the last one in region array.���������ʶ*/
	NET_ATM_REGIONTYPE_SCREEN       = -1,  /* Screen region.������                             */
	NET_ATM_REGIONTYPE_HUMAN        = -2,  /* Human region.���ͨ���˻��                    */
	NET_ATM_REGIONTYPE_FACEHUMAN    = -3,  /* Human region in face channel.����ͨ���˻��    */
	NET_ATM_REGIONTYPE_OBJSIZE      = -4,  /* Object size region. ��СĿ����ȡ��               */
	NET_ATM_REGIONTYPE_KEYBOARD     = 2,   /* Keyboard region. ������                          */
	NET_ATM_REGIONTYPE_CARDPORT     = 3,   /* Card port region.������                          */
	NET_ATM_REGIONTYPE_KEYMASK      = 4,   /* Keyboard mask region.��������                    */
	NET_ATM_REGIONTYPE_FACE         = 5,   /* .�������                                      */
	NET_ATM_REGIONTYPE_PROCREGION   = 6,   /* .�ӳ���ͨ������������                          */
	NET_ATM_REGIONTYPE_NOPROCREGION = 7,   /* .�ӳ���ͨ��������������                     */
	NET_ATM_REGIONTYPE_WATCH        = 8,   /* .����ͨ�������������                         */
	NET_ATM_REGIONTYPE_TAIL         = 9,   /* .����ͨ����β��ȡ����                         */
	NET_ATM_REGIONTYPE_FORBID       = 10,   /* .����ͨ������ֹվ����                         */
} NET_ATM_REGIONTYPE_E;

//����α�ʾ�ṹ�壬����������
typedef  struct
{
	STRUCT_SDVR_ATMI_POINT_S  point[10]; //����ζ�������
	DWORD  point_num;                    //��ĸ�����(0-10),Ĭ��Ϊ0
	DWORD  region_type;                  //�������ͣ�������SDK��NET_ATM_REGIONTYPE_E����
}STRUCT_SDVR_ATMI_POLYGON_S;

//�������򣬴���������
typedef  struct
{
	STRUCT_SDVR_ATMI_RECT_S  region;   //������������
	DWORD  region_type;                //�������ͣ�������SDK��NET_ATM_REGIONTYPE_E����
}STRUCT_SDVR_ATMI_RECT_TYPE_S;

//��������Ȥ�����Լ��������������Ĵ�С
typedef  struct
{
	STRUCT_SDVR_ATMI_RECT_TYPE_S  roi; //����
	DWORD  min_face;                   //��С�ߴ�,Ŀǰ�̶�Ϊ60��������ȥ������
	DWORD  max_face;                   //���ߴ�,Ŀǰ�̶�Ϊ288��������ȥ������
}STRUCT_SDVR_ATMI_FACE_ROI_S;

// 1.����ͨ���������õ�����
typedef  struct
{
	DWORD num;                      //ʵ�������������(0-10)��Ĭ��0
	STRUCT_SDVR_ATMI_FACE_ROI_S face_roi[10];
}STRUCT_SDVR_ATMI_FACEROI_ALL_S;

// 2.���ͨ���������õ�����
typedef struct
{
	DWORD  num;                     //�������(0-20)��Ĭ��Ϊ0
	STRUCT_SDVR_ATMI_POLYGON_S  atmi_panel_region[20];
}STRUCT_SDVR_ATMI_PANEL_REGION_S;

// 3.�ӳ���ͨ���������õ����򼰲���
typedef  struct
{
	STRUCT_SDVR_ATMI_POLYGON_S  pol_proc_region;  //��������Ĭ��4���㣬����ȫͼ
	STRUCT_SDVR_ATMI_RECT_TYPE_S  no_process[10]; //����������
	DWORD  no_process_num;                        //������������� (Ĭ��0�����10)
	DWORD  warn_interval;                         //���α���ʱ������(Ĭ��100 �룬��СΪ0�������)
}STRUCT_SDVR_ATMI_DISTRICTPARA_S;	

// 4.����ͨ���������õ�����
typedef	struct
{
	STRUCT_SDVR_ATMI_POLYGON_S  pol_proc_region;  //ͼ���еĴ������򣬶���α�ʾ
	                                              //����ATM��ǰβ��ȡ����Ĳ�������ʶATMǰ��վ��������
	STRUCT_SDVR_ATMI_POLYGON_S  tail_region[10];  // Region in polygon.
	DWORD  tail_num;                              // Region number. default: 0
	                                              //���ڽ�ֹ������뱨������ʶѡ���Ľ�ֹ���������
	STRUCT_SDVR_ATMI_POLYGON_S  forbid_region[10];// Region in polygon.
	DWORD  forbid_num;                            // Region number.	default: 0 (0-10)
	STRUCT_SDVR_ATMI_POLYGON_S  obj_height;       // Ŀ�꣨�ˣ���ͼ���еĸ߶ȣ�Ĭ��85
}STRUCT_SDVR_ATMI_SCENE_COMMONPARA_S;

// 5.����ͨ�����õĲ���,������֡Ϊ��λ�ģ������ڽ�������Ϊ�룬Ȼ�����ڲ���ת��Ϊ֡��
typedef  struct
{
	//��Ʒ�����㷨��ز���
	DWORD  objlv_frames_th;         //��Ʒ����ʱ����ֵ(֡) (Ĭ��30����СΪ0�������)

	//��Ա�ǻ��㷨��ز���
	DWORD  mv_block_cnt;            //�ƶ�����(20��0��ʾ�˹�����Ч����СΪ0�������)
	uint16_t  mv_stay_frames;           //�����г���ʱ����ֵ(֡),������ʱ��(0��ʾ�˹�����Ч����СΪ0�������)
	uint16_t  mv_stay_valid_frames;     // ATM����ͣ��ʱ����ֵ(֡),
	                                //ATM����ǰͣ��ʱ��(200, 0��ʾ�˹�����Ч����СΪ0�������)

	//���˾ۼ��㷨��ز���
	uint16_t  gather_cnt;               //���ۼ�����(Ĭ��4����СΪ0�������)
	uint16_t  gather_interval_frames;   //�������ʱ��(֡)(1000 frames,Լ100�룬��СΪ0�������)
	DWORD  gather_frame_cnt;        //���˾ۼ�ʱ����ֵ(֡) (Ĭ��100����СΪ0�������)

	//��Ա�����㷨��ز���
	DWORD liedown_frame_cnt;        //����ʱ����ֵ(֡).(Ĭ��20 frames������СΪ0�������)

	//β��ȡ���㷨��ز���
	uint16_t  after_frame_cnt;          //������Ϊʱ����ֵ(֡)(Ĭ��20 frames����СΪ0�������)
	uint16_t  after_interval_frames;    //�������ʱ��(֡)(1000 frames��Լ100�룬��СΪ0�������)

	//��ֹ�����㷨��ز���
	uint16_t  forbid_frame_cnt;         //��ֹվ��ʱ����ֵ(֡)(20 frames����СΪ0�������)
	uint16_t  reserve;                  //����
}STRUCT_SDVR_ATMI_SCENE_WARN_PARAM_S;

/*������屨������*/
typedef struct 
{
	int32_t AlphaVal;						//����alphaֵ(5)
	int32_t BetaVal;						//����Betaֵ(3)
	int32_t MetinThVal;						//ǰ���ڱ�����ֵ(4500)
	int32_t LBTWTriggerVal;					//ȡ������������ֵ(75)

	int32_t AppearCntThdVal;				//�������ֱ�������(40)		//��������Ȼ���
	int32_t AppearCntTriggerVal;			//�������ֱ�����ֵ(40)		//�����������ֵ
	int32_t LBTWCntThdVal;					//ȡ��������������(75)		//ճ�������ƻ���
	int32_t LBTWCntTriggerVal;				//ȡ������������ֵ(75)		//ճ����������ֵ

	int32_t PanelTimeOutTriggerVal;			//��ʱ������ֵ(1500)

	int32_t OpenLightTriggerVal;			//���仯������ֵ(45)		//�ڵ���ʼ������
	int32_t CloseLightTriggerVal;			//���仯������ֵ(55)		//�ڵ��˳�������

	int32_t AppearMinWidth;					//����������СĿ����(10)
	int32_t AppearMinHeight;				//����������СĿ��߶�(10)
	int32_t AppearMaxWidth;					//�����������Ŀ����(200)
	int32_t AppearMaxHeight;				//�����������Ŀ��߶�(200)

	int32_t LBTWMinWidth;					//ȡ��������СĿ����(10)
	int32_t LBTWMinHeight;					//ȡ��������СĿ��߶�(10)
	int32_t LBTWMaxWidth;					//ȡ���������Ŀ����(200)
	int32_t LBTWMaxHeight;					//ȡ���������Ŀ��߶�(200)

}STRUCT_SDVR_ATMI_PANEL_PARAM_S;

// 1.����ͨ�����ýṹ��
typedef  struct
{
	uint16_t  face_unusual;                       //�Ƿ���쳣�����������֡�����ȣ���⹦�ܣ�1 Ϊ�򿪣�0Ϊ�رա�Ĭ��Ϊ0
	uint16_t  output_oneface;                     //��������ֻ���һ�����0Ϊ��1Ϊ�ǣ�Ĭ��Ϊ1
	DWORD  fd_rate;                           //�������������ټ������0��ʼ�����ֵ���ޣ�Ĭ��5
	STRUCT_SDVR_ATMI_FACEROI_ALL_S face_roi;  //����ͨ����������������
	DWORD abnormalface_alarmtime;		//�쳣������������ʱ����ֵ		//***����***2013-3-13
}STRUCT_SDVR_ATMI_SET_FACE_S;

// 2.���ͨ�����ýṹ��
typedef  struct
{
	DWORD  timeout_enable;                         //��ʱʱ�䣬0Ϊ����������0Ϊ��ʱʱ��(3\5\10\15\20\30��)��Ĭ��Ϊ0
	STRUCT_SDVR_ATMI_PANEL_REGION_S panel_region;  //���ͨ��������������
	STRUCT_SDVR_ATMI_PANEL_PARAM_S panel_alarm_param;	//��屨������   //***����***2013-3-13
}STRUCT_SDVR_ATMI_SET_PANEL_S;

// 3.�ӳ���ͨ�����ýṹ��
typedef  struct
{
	STRUCT_SDVR_ATMI_DISTRICTPARA_S  money_para;   //�ӳ���ͨ��������������
}STRUCT_SDVR_ATMI_SET_MONEY_S;

// 4.����ͨ�����ýṹ��
typedef  struct
{
	STRUCT_SDVR_ATMI_SCENE_WARN_PARAM_S  envi_para;   //����ͨ������
	STRUCT_SDVR_ATMI_SCENE_COMMONPARA_S  envi_region; //����ͨ������
}STRUCT_SDVR_ATMI_SET_ENVI_S;

// ����ͳ�Ʋ���
typedef struct 
{
	DWORD  dwWidth;                 //������Ƶ�Ŀ�ȣ�Ĭ��ֵ352 CIF(0-352) D1(0-704)
	DWORD  dwHeight;                //������Ƶ�߶ȣ�Ĭ��ֵ288 CIF(0-288) D1(0-576)
	DWORD  objWidth;                //����Ŀ��Ŀ�ȣ���λΪ���أ�Ĭ��ֵ55 CIF(0-288) D1(0-576)
	STRUCT_SDVR_ATMI_POINT_S  ptStart;       //�������㣬Ĭ��ֵ(5,216)
	STRUCT_SDVR_ATMI_POINT_S  ptEnd;         //������յ㣬Ĭ��ֵ(347,216)
	STRUCT_SDVR_ATMI_POINT_S  ptDirection;   //����ߵķ���Ĭ��ֵ(290, 205)
	DWORD  dwPassFrames;            // ��ʼ���ĵ�Ŀ���ںϳ�ͼ�еĸ߶ȣ���Ŀ��ͨ������ߵ�֡����Ĭ��ֵ15
	DWORD  dwMutiObjWidth;          //��������Ŀ�겢��ʱ���ο�Ŀ�ȣ�Ĭ��ֵ110
	DWORD  dwMutiObjWidthEdge;      //��dwMutiObjWidth�йأ�
	                                //dwMutiObjWidthEdge = ��dwMutiObjWidth / 2 - 5��/ 2��Ĭ��ֵ25
	DWORD  dwThreshBackDiff;        //�����ֵ��Ĭ��ֵ50���Ƚ����У�(0-����)
	DWORD  dwThreshFrameDiff;       //֡��ֵ��Ĭ��ֵ20��(0-����)
	uint8_t  byStartPtLabel;           //������ǣ�0��ʾ�κ�Ŀ���������1��ʾС�ڷ�ֵ��Ŀ�겻������Ĭ��ֵΪ0
	uint8_t  byEndPtLable;             //�յ����ǣ�0��ʾ�κ�Ŀ���������1��ʾС�ڷ�ֵ��Ŀ�겻������Ĭ��ֵΪ0
	uint8_t  byReserver[2];            //����
	DWORD  dwHalfObjW;              //��ֵ����ǰ������أ����С�ڸ÷�ֵ��������Ĭ��ֵΪ20��(objWidth/2)
}STRUCT_SDVR_HDC_CTRLPARAM, *LPSTRUCT_SDVR_HDC_CTRLPARAM;

//�ͻ������û��ȡ��������·�����ܵĽṹ��
typedef  struct
{
	// int32_t  chn;                    //ͨ����(0-MAXCH)
	uint8_t  byChn;                    //ͨ����
	uint8_t  byReserver1;              //����
	uint8_t  bySetInfoType;            //���ò������ͣ�0- STRUCT_SDVR_ATMI_SET_FACE_S��
	                                //1-STRUCT_SDVR_ATMI_SET_PANEL_S��2-STRUCT_SDVR_ATMI_SET_MONEY_S�� 
	                                //3-STRUCT_SDVR_ATMI_SET_ENVI_S��4-STRUCT_SDVR_HDC_CTRLPARAM
	uint8_t  byReserver2;              //����

	int32_t  chn_attri;                 //ͨ������(��������塢�ӳ�������)��Ŀǰδ�ã���ֹ�Ժ���
	int16_t  channel_enable;          //ͨ�����أ�0-�رգ�1-�򿪣�Ĭ��0
	int16_t  if_pic;                  //�Ƿ���ҪץȡͼƬ,0-��Ҫ��1-����Ҫ��Ĭ��0
	int16_t  enc_type;                //ץȡͼƬ�ĸ�ʽ��0-CIF��1-D1��2·Ĭ��1��4·Ĭ��0
	int16_t  email_linkage;           //����email��0-��������1-������Ĭ��0
	uint32_t  sensor_num;               //����̽ͷ���,��λ��ʾ��0-��������1-������Ĭ��1
	uint32_t  rec_linkage;              //����¼�񣬰�λ��ʾ��0-��������1-������Ĭ��0

	union
	{
		STRUCT_SDVR_ATMI_SET_FACE_S   face_set_para;  //����ͨ�����ýṹ��
		STRUCT_SDVR_ATMI_SET_PANEL_S  panel_set_para; //���ͨ�����ýṹ��
		STRUCT_SDVR_ATMI_SET_MONEY_S  money_set_para; //�ӳ���ͨ�����ýṹ��
		STRUCT_SDVR_ATMI_SET_ENVI_S   envi_set_para;  //����ͨ�����ýṹ��
		STRUCT_SDVR_HDC_CTRLPARAM     hdc;            //����ͳ�Ʋ�������
	}setInfo;
}STRUCT_SDVR_INTELLECTCFG;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif
