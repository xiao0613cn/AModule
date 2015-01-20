#ifndef _NETCMDINTELLIGENTSTRUCT_H_
#define _NETCMDINTELLIGENTSTRUCT_H_

#include "NetCmdStruct.h"

/************************************************************************/
/* 消息命令定义                                                                 */
/************************************************************************/
#define NET_SDVR_ALARM_CONNECT                 0xA0   //建立智能报警上报链路
#define NET_SDVR_ALARM_REPORT                  0xA1   //智能报警上报消息
#define NET_SDVR_INTELLECTCFG_SET              0xA2   //智能报警设置
#define NET_SDVR_INTELLECTCFG_GET              0xA3   //智能报警查询
#define NET_SDVR_INTELLECTCFG_ALGORITHM_RESET  0xA4   //智能算法复位
#define NET_SDVR_ALARM_HEART						0X9A			//智能报警心跳


#ifdef _MSC_VER
#pragma pack(push, 1)
#pragma warning(disable: 4244)
#pragma warning(disable: 4018)
#endif

/************************************************************************/
/* 上传智能报警消息                                                             */
/************************************************************************/
//矩形坐标
typedef  struct
{
	int  left;                      //矩形左坐标,CIF(0-352),D1(0-704)
	int  top;                       //矩形上坐标,CIF(0-352),D1(0-704)
	int  right;                     //矩形右坐标,CIF(0-352),D1(0-704)
	int  bottom;                    //矩形下坐标,CIF(0-352),D1(0-704)
}STRUCT_SDVR_ATMI_RECT, *PSTRUCT_SDVR_ATMI_RECT;

//报警类型及位置信息
typedef  struct
{
	int  alarm_type;                   //类型,NET_ATMI_ALARM_TYPE_E
	STRUCT_SDVR_ATMI_RECT  position;   //坐标位置
}STRUCT_SDVR_ATMI_ALARM_POSITION_S, *PSTRUCT_SDVR_ATMI_ALARM_POSITION_S;

// 1.人脸通道报警结构体
typedef  struct
{
	DWORD  alarm_num;                                  //报警个数
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  alarm_area[10]; //报警坐标值,一共有alarm_num个，后面的全为0
}STRUCT_SDVR_ATMI_FACE_ALARM_S, *PSTRUCT_SDVR_ATMI_FACE_ALARM_S;

// 2.面板通道报警结构体
typedef  struct
{
	DWORD  alarm_num;                                  //报警个数
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  alarm_area[10]; //报警坐标值,一共有alarm_num个，后面的全为0
}STRUCT_SDVR_ATMI_PANEL_ALARM_S, *PSTRUCT_SDVR_ATMI_PANEL_ALARM_S;

// 3.加钞间检测输出信息
typedef  struct
{
	DWORD  type;                    //是否有人闯入，0表示无，1表示有
}STRUCT_SDVR_ATMI_MONEY_ALARM_S, *PSTRUCT_SDVR_ATMI_MONEY_ALARM_S;

// 4.环境报警结构体,alarm_num所对应的区域在前，track_num所对应的区域紧跟在alarm_num区域后
typedef  struct
{
	DWORD  alarm_num;               //报警目标数量
	DWORD  track_num;               //跟踪目标数量
	STRUCT_SDVR_ATMI_ALARM_POSITION_S  envi_alarm_region[25];
}STRUCT_SDVR_ATMI_ENVI_ALARM_S, *PSTRUCT_SDVR_ATMI_ENVI_ALARM_S;

//报警信息类型
typedef  enum
{
	NET_ATMI_FACE_BLOCK = 0,         //人脸遮挡
	NET_ATMI_FACE_NOSIGNAL,          //有脸通道视频丢失
	NET_ATMI_FACE_UNUSUAL,           //人脸异常
	NET_ATMI_FACE_NORMAL,            //人脸正常
	NET_ATMI_PANEL_BLOCK  = 40,      //面板遮挡
	NET_ATMI_PANEL_NOSIGNAL,         //面板通道视频丢失
	NET_ATMI_PANEL_PASTE,            //贴条
	NET_ATMI_PANEL_KEYBOARD,         //装假键盘
	NET_ATMI_PANEL_KEYMASK,          //破坏密码防护罩	
	NET_ATMI_PANEL_CARDREADER,       //破坏读卡器
	NET_ATMI_PANEL_TMIEOUT,          //超时报警
	NET_ATMI_ENTER,                  //有人进入
	NET_ATMI_EXIT,                   //人撤离
	NET_ATMI_MONEY_BLOCK = 80,       //加钞间视频遮挡
	NET_ATMI_MONEY_NOSIGNAL,         //加钞间通道视频丢失
	NET_ATMI_MONEY_UNUSUAL,          //加钞间异常,即有人闯入加钞间
	NET_ATMI_ENVI_BLOCK = 120,       //环境通道视频遮挡
	NET_ATMI_ENVI_NOSIGNAL,          //环境通道视频丢失
	NET_ATMI_ENVI_GATHER,            //多人聚集
	NET_ATMI_ENVI_MANYPEOPLEINREGION,//违规取款
	NET_ATMI_ENVI_LINGERING,         //人员徘徊
	NET_ATMI_ENVI_LIEDOWN,           //人员倒地
	NET_ATMI_ENVI_FORBIDREGION,      //非法进入警戒区
	NET_ATMI_ENVI_LEAVEOBJECT,       //物品遗留
}NET_ATMI_ALARM_TYPE_E;

typedef  struct
{
	DWORD  pic_alarm_type;          //NET_ATMI_ALARM_TYPE_E
	DWORD  pic_format;              //图片格式0-CIF  1-D1
	DWORD  pic_size;                //图片大小
}STRUCT_SDVR_ATMI_ALARM_PICINFO_S, *PSTRUCT_SDVR_ATMI_ALARM_PICINFO_S;

typedef  enum
{
	HBGK_HDCCOUNT_DIR1 = 0,         // 与标记方向相同
	HBGK_HDCCOUNT_DIR2,             // 与标记方向相反
}NET_HDCCOUNT_DIRECTION_E;

typedef struct 
{
	DWORD  dwResultType;            //输出结果总类型
	DWORD  dwSubType;               //输出结果子类型，表示人员流动统计的方向
	                                //见NET_HDCCOUNT_DIRECTION_E
	DWORD  dwTrackNum;              //当前输出统计的ID编号(已统计人数)
	STRUCT_SDVR_ATMI_RECT  rcPos;            //当前输出编号的外接矩形框
}STRUCT_SDVR_HDC_RESULT, *LPSTRUCT_SDVR_HDC_RESULT;

typedef  struct
{
	BYTE  byChn;
	BYTE  byReserver1;
	BYTE  byInfoType;               // 上传信息类型：0-STRUCT_SDVR_ATMI_FACE_ALARM_S，
	                                // 1-STRUCT_SDVR_ATMI_PANEL_ALARM_S，2-STRUCT_SDVR_ATMI_MONEY_ALARM_S
	                                // 3-STRUCT_SDVR_ATMI_ENVI_ALARM_S，4-STRUCT_SDVR_HDC_RESULT
	BYTE  byReserver2;
	union
	{
		STRUCT_SDVR_ATMI_FACE_ALARM_S   atmi_face_alarm;   // 1.人脸通道报警结构体
		STRUCT_SDVR_ATMI_PANEL_ALARM_S  atmi_panel_alarm;  // 2.面板通道报警结构体
		STRUCT_SDVR_ATMI_MONEY_ALARM_S  atmi_money_alarm;  // 3.加钞间通道报警结构体
		STRUCT_SDVR_ATMI_ENVI_ALARM_S   atmi_envi_alarm;   // 4.环境通道报警结构体
		STRUCT_SDVR_HDC_RESULT          hdc;
	}info;
	STRUCT_SDVR_ATMI_ALARM_PICINFO_S  alarm_picinfo;           //图片信息，如有则在结构体后跟抓拍图片数据
	SYSTIME  alarmtime;                                        //报警时间
}STRUCT_SDVR_ATMI_ALARM_INFO_S, *PSTRUCT_SDVR_ATMI_ALARM_INFO_S;

/************************************************************************/
/* 智能报警配置设置                                                             */
/************************************************************************/
//矩形坐标
typedef  struct
{
	DWORD  left;                    //矩形左坐标
	DWORD  top;                     //矩形上坐标
	DWORD  right;                   //矩形右坐标
	DWORD  bottom;                  //矩形下坐标
}STRUCT_SDVR_ATMI_RECT_S;

//一个点的坐标
typedef  struct
{
	WORD  x;                        //横坐标
	WORD  y;                        //纵坐标
}STRUCT_SDVR_ATMI_POINT_S;

//区域类型
typedef enum
{
	NET_ATM_REGIONTYPE_WARN         = 1,   /* Warn region.     禁止粘贴区                       */
	NET_ATM_REGIONTYPE_NOWARN       = 0,   /* Indicate the last one in region array.区域结束标识*/
	NET_ATM_REGIONTYPE_SCREEN       = -1,  /* Screen region.屏蔽区                             */
	NET_ATM_REGIONTYPE_HUMAN        = -2,  /* Human region.面板通道人活动区                    */
	NET_ATM_REGIONTYPE_FACEHUMAN    = -3,  /* Human region in face channel.人脸通道人活动区    */
	NET_ATM_REGIONTYPE_OBJSIZE      = -4,  /* Object size region. 最小目标提取区               */
	NET_ATM_REGIONTYPE_KEYBOARD     = 2,   /* Keyboard region. 键盘区                          */
	NET_ATM_REGIONTYPE_CARDPORT     = 3,   /* Card port region.卡口区                          */
	NET_ATM_REGIONTYPE_KEYMASK      = 4,   /* Keyboard mask region.键盘罩区                    */
	NET_ATM_REGIONTYPE_FACE         = 5,   /* .人脸活动区                                      */
	NET_ATM_REGIONTYPE_PROCREGION   = 6,   /* .加钞间通道，处理区域                          */
	NET_ATM_REGIONTYPE_NOPROCREGION = 7,   /* .加钞间通道，不处理区域                     */
	NET_ATM_REGIONTYPE_WATCH        = 8,   /* .环境通道，大厅监控区                         */
	NET_ATM_REGIONTYPE_TAIL         = 9,   /* .环境通道，尾随取款区                         */
	NET_ATM_REGIONTYPE_FORBID       = 10,   /* .环境通道，禁止站立区                         */
} NET_ATM_REGIONTYPE_E;

//多边形表示结构体，带区域类型
typedef  struct
{
	STRUCT_SDVR_ATMI_POINT_S  point[10]; //多边形顶点坐标
	DWORD  point_num;                    //点的个数，(0-10),默认为0
	DWORD  region_type;                  //区域类型，见智能SDK中NET_ATM_REGIONTYPE_E定义
}STRUCT_SDVR_ATMI_POLYGON_S;

//矩形区域，带区域类型
typedef  struct
{
	STRUCT_SDVR_ATMI_RECT_S  region;   //矩形区域坐标
	DWORD  region_type;                //区域类型，见智能SDK中NET_ATM_REGIONTYPE_E定义
}STRUCT_SDVR_ATMI_RECT_TYPE_S;

//人脸感兴趣区域以及该区域中人脸的大小
typedef  struct
{
	STRUCT_SDVR_ATMI_RECT_TYPE_S  roi; //坐标
	DWORD  min_face;                   //最小尺寸,目前固定为60，后续会去掉此项
	DWORD  max_face;                   //最大尺寸,目前固定为288，后续会去掉此项
}STRUCT_SDVR_ATMI_FACE_ROI_S;

// 1.人脸通道中所设置的区域
typedef  struct
{
	DWORD num;                      //实际设置区域个数(0-10)，默认0
	STRUCT_SDVR_ATMI_FACE_ROI_S face_roi[10];
}STRUCT_SDVR_ATMI_FACEROI_ALL_S;

// 2.面板通道中所设置的区域
typedef struct
{
	DWORD  num;                     //区域个数(0-20)，默认为0
	STRUCT_SDVR_ATMI_POLYGON_S  atmi_panel_region[20];
}STRUCT_SDVR_ATMI_PANEL_REGION_S;

// 3.加钞间通道中所设置的区域及参数
typedef  struct
{
	STRUCT_SDVR_ATMI_POLYGON_S  pol_proc_region;  //处理区域，默认4个点，包含全图
	STRUCT_SDVR_ATMI_RECT_TYPE_S  no_process[10]; //不处理区域
	DWORD  no_process_num;                        //不处理区域个数 (默认0，最大10)
	DWORD  warn_interval;                         //两次报警时间间隔，(默认100 秒，最小为0，最大不限)
}STRUCT_SDVR_ATMI_DISTRICTPARA_S;	

// 4.场景通道中所设置的区域
typedef	struct
{
	STRUCT_SDVR_ATMI_POLYGON_S  pol_proc_region;  //图像中的处理区域，多边形表示
	                                              //用于ATM机前尾随取款检测的参数，标识ATM前人站立的区域
	STRUCT_SDVR_ATMI_POLYGON_S  tail_region[10];  // Region in polygon.
	DWORD  tail_num;                              // Region number. default: 0
	                                              //用于禁止区域进入报警，标识选定的禁止进入的区域
	STRUCT_SDVR_ATMI_POLYGON_S  forbid_region[10];// Region in polygon.
	DWORD  forbid_num;                            // Region number.	default: 0 (0-10)
	STRUCT_SDVR_ATMI_POLYGON_S  obj_height;       // 目标（人）在图像中的高度，默认85
}STRUCT_SDVR_ATMI_SCENE_COMMONPARA_S;

// 5.环境通道设置的参数,以下以帧为单位的，我们在界面上做为秒，然后在内部再转化为帧数
typedef  struct
{
	//物品遗留算法相关参数
	DWORD  objlv_frames_th;         //物品遗留时间阈值(帧) (默认30，最小为0，最大不限)

	//人员徘徊算法相关参数
	DWORD  mv_block_cnt;            //移动距离(20，0表示此规则无效，最小为0，最大不限)
	WORD  mv_stay_frames;           //场景中出现时间阈值(帧),存在总时间(0表示此规则无效，最小为0，最大不限)
	WORD  mv_stay_valid_frames;     // ATM区域停留时间阈值(帧),
	                                //ATM区域前停留时间(200, 0表示此规则无效，最小为0，最大不限)

	//多人聚集算法相关参数
	WORD  gather_cnt;               //最多聚集人数(默认4，最小为0，最大不限)
	WORD  gather_interval_frames;   //报警间隔时间(帧)(1000 frames,约100秒，最小为0，最大不限)
	DWORD  gather_frame_cnt;        //多人聚集时间阈值(帧) (默认100，最小为0，最大不限)

	//人员躺卧算法相关参数
	DWORD liedown_frame_cnt;        //躺卧时间阈值(帧).(默认20 frames，，最小为0，最大不限)

	//尾随取款算法相关参数
	WORD  after_frame_cnt;          //可疑行为时间阈值(帧)(默认20 frames，最小为0，最大不限)
	WORD  after_interval_frames;    //报警间隔时间(帧)(1000 frames，约100秒，最小为0，最大不限)

	//禁止进入算法相关参数
	WORD  forbid_frame_cnt;         //禁止站立时间阈值(帧)(20 frames，最小为0，最大不限)
	WORD  reserve;                  //保留
}STRUCT_SDVR_ATMI_SCENE_WARN_PARAM_S;

/*设置面板报警参数*/
typedef struct 
{
	int AlphaVal;						//检测库alpha值(5)
	int BetaVal;						//检测库Beta值(3)
	int MetinThVal;						//前景融背景阈值(4500)
	int LBTWTriggerVal;					//取走遗留报警阈值(75)

	int AppearCntThdVal;				//区域入侵报警基数(40)		//活动区灵敏度基数
	int AppearCntTriggerVal;			//区域入侵报警阈值(40)		//活动区灵敏度阈值
	int LBTWCntThdVal;					//取走遗留报警基数(75)		//粘贴区控制基数
	int LBTWCntTriggerVal;				//取走遗留报警阈值(75)		//粘贴区报警阈值

	int PanelTimeOutTriggerVal;			//超时报警阈值(1500)

	int OpenLightTriggerVal;			//进变化报警阈值(45)		//遮挡开始灵敏度
	int CloseLightTriggerVal;			//出变化报警阈值(55)		//遮挡退出灵敏度

	int AppearMinWidth;					//区域入侵最小目标宽度(10)
	int AppearMinHeight;				//区域入侵最小目标高度(10)
	int AppearMaxWidth;					//区域入侵最大目标宽度(200)
	int AppearMaxHeight;				//区域入侵最大目标高度(200)

	int LBTWMinWidth;					//取走遗留最小目标宽度(10)
	int LBTWMinHeight;					//取走遗留最小目标高度(10)
	int LBTWMaxWidth;					//取走遗留最大目标宽度(200)
	int LBTWMaxHeight;					//取走遗留最大目标高度(200)

}STRUCT_SDVR_ATMI_PANEL_PARAM_S;

// 1.人脸通道设置结构体
typedef  struct
{
	WORD  face_unusual;                       //是否打开异常人脸（戴口罩、蒙面等）检测功能，1 为打开，0为关闭。默认为0
	WORD  output_oneface;                     //设置人脸只输出一次与否，0为否，1为是，默认为1
	DWORD  fd_rate;                           //设置人脸检测跟踪间隔，从0开始，最大值不限，默认5
	STRUCT_SDVR_ATMI_FACEROI_ALL_S face_roi;  //人脸通道的区域及其它参数
	DWORD abnormalface_alarmtime;		//异常人脸触发报警时间阈值		//***新增***2013-3-13
}STRUCT_SDVR_ATMI_SET_FACE_S;

// 2.面板通道设置结构体
typedef  struct
{
	DWORD  timeout_enable;                         //超时时间，0为不开启，非0为超时时间(3\5\10\15\20\30分)，默认为0
	STRUCT_SDVR_ATMI_PANEL_REGION_S panel_region;  //面板通道区域及其它参数
	STRUCT_SDVR_ATMI_PANEL_PARAM_S panel_alarm_param;	//面板报警参数   //***新增***2013-3-13
}STRUCT_SDVR_ATMI_SET_PANEL_S;

// 3.加钞间通道设置结构体
typedef  struct
{
	STRUCT_SDVR_ATMI_DISTRICTPARA_S  money_para;   //加钞间通道区域及其它参数
}STRUCT_SDVR_ATMI_SET_MONEY_S;

// 4.环境通道设置结构体
typedef  struct
{
	STRUCT_SDVR_ATMI_SCENE_WARN_PARAM_S  envi_para;   //环境通道参数
	STRUCT_SDVR_ATMI_SCENE_COMMONPARA_S  envi_region; //环境通道区域
}STRUCT_SDVR_ATMI_SET_ENVI_S;

// 客流统计参数
typedef struct 
{
	DWORD  dwWidth;                 //处理视频的宽度，默认值352 CIF(0-352) D1(0-704)
	DWORD  dwHeight;                //处理视频高度，默认值288 CIF(0-288) D1(0-576)
	DWORD  objWidth;                //单个目标的宽度，单位为像素，默认值55 CIF(0-288) D1(0-576)
	STRUCT_SDVR_ATMI_POINT_S  ptStart;       //检测线起点，默认值(5,216)
	STRUCT_SDVR_ATMI_POINT_S  ptEnd;         //检测线终点，默认值(347,216)
	STRUCT_SDVR_ATMI_POINT_S  ptDirection;   //检测线的方向，默认值(290, 205)
	DWORD  dwPassFrames;            // 初始化的单目标在合成图中的高度，即目标通过检测线的帧数，默认值15
	DWORD  dwMutiObjWidth;          //三个以上目标并行时矩形框的宽度，默认值110
	DWORD  dwMutiObjWidthEdge;      //与dwMutiObjWidth有关，
	                                //dwMutiObjWidthEdge = （dwMutiObjWidth / 2 - 5）/ 2，默认值25
	DWORD  dwThreshBackDiff;        //背景差阀值，默认值50，比较敏感，(0-不限)
	DWORD  dwThreshFrameDiff;       //帧间差阀值，默认值20，(0-不限)
	BYTE  byStartPtLabel;           //起点检测标记，0表示任何目标均计数，1表示小于阀值的目标不计数，默认值为0
	BYTE  byEndPtLable;             //终点检测标记，0表示任何目标均计数，1表示小于阀值的目标不计数，默认值为0
	BYTE  byReserver[2];            //保留
	DWORD  dwHalfObjW;              //阀值，与前两项相关，宽度小于该阀值不计数，默认值为20。(objWidth/2)
}STRUCT_SDVR_HDC_CTRLPARAM, *LPSTRUCT_SDVR_HDC_CTRLPARAM;

//客户端设置或获取到主机四路智能总的结构体
typedef  struct
{
	// int  chn;                    //通道号(0-MAXCH)
	BYTE  byChn;                    //通道号
	BYTE  byReserver1;              //保留
	BYTE  bySetInfoType;            //设置参数类型：0- STRUCT_SDVR_ATMI_SET_FACE_S，
	                                //1-STRUCT_SDVR_ATMI_SET_PANEL_S，2-STRUCT_SDVR_ATMI_SET_MONEY_S， 
	                                //3-STRUCT_SDVR_ATMI_SET_ENVI_S，4-STRUCT_SDVR_HDC_CTRLPARAM
	BYTE  byReserver2;              //保留

	int  chn_attri;                 //通道属性(人脸、面板、加钞、环境)，目前未用，防止以后用
	SHORT  channel_enable;          //通道开关，0-关闭，1-打开，默认0
	SHORT  if_pic;                  //是否需要抓取图片,0-需要，1-不需要，默认0
	SHORT  enc_type;                //抓取图片的格式，0-CIF，1-D1，2路默认1，4路默认0
	SHORT  email_linkage;           //联动email，0-不联动，1-联动，默认0
	UINT  sensor_num;               //联动探头输出,按位表示，0-不联动，1-联动，默认1
	UINT  rec_linkage;              //联动录像，按位表示，0-不联动，1-联动，默认0

	union
	{
		STRUCT_SDVR_ATMI_SET_FACE_S   face_set_para;  //人脸通道设置结构体
		STRUCT_SDVR_ATMI_SET_PANEL_S  panel_set_para; //面板通道设置结构体
		STRUCT_SDVR_ATMI_SET_MONEY_S  money_set_para; //加钞间通道设置结构体
		STRUCT_SDVR_ATMI_SET_ENVI_S   envi_set_para;  //环境通道设置结构体
		STRUCT_SDVR_HDC_CTRLPARAM     hdc;            //人流统计参数设置
	}setInfo;
}STRUCT_SDVR_INTELLECTCFG;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif
