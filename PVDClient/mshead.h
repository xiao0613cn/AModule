#ifndef MSHEAD_H
#define MSHEAD_H 

//#include "common.h"

/*
MSHead内存存储结构示意图
+++++++++++++++++++++
+                   +
+     MSHEADINFO    +
+                   +
+                   +
+-------------------+
+-                 -+
+-    SEGINFO_0    -+
+-                 -+
+-------------------+
+      ......       +
+-------------------+
+-                 -+
+-    SEGINFO_n    -+
+-                 -+
+-------------------+
+++++++++++++++++++++
*/

#define MSHEAD_FLAG 0x534d4248 // "HBMS" - HanBang Media Stantard V1.0 
#define MSHDV2_FLAG 0x3256534d // "MSV2" - HanBang Media Stantard V2.0 

#define MSHEAD_MAXSIZE 511 // 媒体头最大长度

#define MSHEAD_KEYFRAME 1 // 关键帧标记
#define MSHEAD_DATAP(mshead) (((int)mshead)+((PMSHEAD)mshead)->mshsize) // 获取媒体数据指针
#define MSHEAD_SEGP(mshead) (((int)mshead)+sizeof(MSHEAD)) // 获取媒体数据指针

// Audio algorithm, 偶数
#define ISO_G722 0
#define ISO_G728 2
#define ISO_G729 4
#define ISO_PCM  6
#define ISO_G711A   8
#define ISO_G726    10

// Video algorithm, 奇数
#define ISO_MPEG4   1
#define ISO_H264    3 /* h24e算法,正常参考模式 */
#define ISO_H264_2X 5 /* h24e算法,间隔2的跳帧参考模式 */
#define ISO_H264_4X 7 /* h24e算法,间隔4的跳帧参考模式 */
#define ISO_PIC_JPG 9 /* 图片,JPG */

#define ISMSHDV1(fd)        (MSHEAD_FLAG == ((PMSHEAD)(fd))->flag)
#define ISMSHDV2(fd)        (MSHDV2_FLAG == ((PMSHEAD)(fd))->flag)
#define ISMSHEAD(fd)        (ISMSHDV1(fd) || ISMSHDV2(fd))
#define ISVIDEOFRAME(fd)    ((((PMSHEAD)(fd))->algorithm) & 1)
#define ISKEYFRAME(fd)      (MSHEAD_KEYFRAME == ((PMSHEAD)(fd))->type)

#define MUX(x, a, b)        ((x) ? (a) : (b))   /* x不为0则为a,为0则为b */

#define MSHEAD_GETMSHSIZE(fd)		(((PMSHEAD)(fd))->mshsize) /* 获取媒体头大小 */
#define MSHEAD_GETMSDSIZE(fd)		MUX(ISMSHDV2(fd), ((PMSHEAD)(fd))->msdsize << 2, ((PMSHEAD)(fd))->msdsize)
#define MSHEAD_GETFRAMESIZE(fd)		(MSHEAD_GETMSHSIZE(fd) + MSHEAD_GETMSDSIZE(fd)) /* 获取媒体数据帧大小(包含媒体头大小) */

typedef enum
{
	SEGID_OSD = 0,      /* OSD信息,OSD信息以字符串形式存放于data中,字符串包含了结束符'\0' */
	SEGID_TIME,         /* 时间信息,如果MSHEAD_SEG中data不存在(即size==sizeof(MSHEAD_SEG)),取MSHEAD中的time_sec和time_msec来显示时间,否则同OSD */
	SEGID_UTF8_OSD,     /* OSD信息,OSD信息以字符串形式存放于MSHEAD_SEG结构体之后,字符串包含了结束符'\0',字符串编码格式为UTF8 */
	SEGID_VENC_INFO,    /* 编码信息 */
	SEGID_USER = 8,     /* 用户自定义ID起始值,从此至SEGID_END都可由用户自定义使用,对于自定义段信息体,除size,id不能改变外,其余信息可根据需要自定义 */
	SEGID_END = 15      /* ID结束值 */
} SEG_ID;

#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif

typedef struct
{
	unsigned int size : 9;        /* 段信息长度 */
	unsigned int id : 4;          /* SEGID */
	unsigned int show : 1;        /* 显示属性1,0-不显示,1-显示 */
	unsigned int trans : 1;       /* 透明属性,0-不透明,1-透明 */
	unsigned int show_sec : 1;    /* 显示属性2,0-不显示,1-显示 */
	unsigned int reserve : 2;     /* 保留位 */
	unsigned int x : 7;           /* X轴坐标(单位为16像素) */
	unsigned int y : 7;           /* Y轴坐标(单位为16像素) */
	signed char data[];           /* 段信息数据内容,如为字符串则包含了结束符'\0' */
} MSHEAD_SEG, *PMSHEAD_SEG;
#define MSHEAD_SEG_LEN  sizeof(MSHEAD_SEG)

typedef struct {
  unsigned int flag;          // MSHEAD_FLAG
  unsigned int mshsize : 9;   // 媒体头信息大小, MAX size=511
  unsigned int msdsize : 19;  // 媒体数据流大小, Max size=512K-1
  unsigned int algorithm : 4; // 媒体编码标准，ISO_...
  unsigned int type : 2;      // 0-P frame, 1-I frame
  unsigned int width : 7;     // 如为视频，则表示宽度（单位16像素）；如为音频，则表示采样位宽（单位bit）
  unsigned int height : 7;    // 如为视频，则表示高度（单位16像素）；如为音频，则表示采样率（单位K[1000]）
  unsigned int serial_no : 9; // 0~511, 循环计数，用于码流的帧连续性判断，当间隔两帧序号不连续时，表示中间帧丢失
  unsigned int time_msec : 7; // 当前帧时间（单位10毫秒）
  unsigned int time_sec;      // 当前帧时间（单位1秒）；表示从公元1970年1月1日0时0分0秒算起至今的UTC时间所经过的秒数
  unsigned int fcount;        // 用于码流的帧计数
  unsigned char data[];       // 紧跟着为段信息或者媒体数据
} MSHEAD, *PMSHEAD;

#ifdef _MSC_VER
#pragma warning(default: 4200)
#endif

#endif

