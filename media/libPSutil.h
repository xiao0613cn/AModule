// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 LIBPSUTIL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// LIBPSUTIL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifndef LIBPSUTIL_API
#ifdef LIBPSUTIL_EXPORTS
#define LIBPSUTIL_API __declspec(dllexport)
#elif defined(_WIN32)
#define LIBPSUTIL_API __declspec(dllimport)
#else
#define LIBPSUTIL_API
#endif
#else
#undef LIBPSUTIL_API
#define LIBPSUTIL_API
#endif //LIBPSUTIL_API
#include <stdint.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
	(uint32_t)((unsigned char)(ch0) | ((unsigned char)(ch1)<<8) | \
	               ((unsigned char)(ch2)<<16) | ((unsigned char)(ch3)<<24))
#endif

#ifndef MAKETWOCC
#define MAKETWOCC(ch0, ch1) \
	(unsigned short)((unsigned char)(ch0) | ((unsigned char)(ch1)<<8))
#endif

//////////////////////////////////////////////////////////////////////////
// reference ffmpeg: libavformat/mpeg.h

// start code: 00 00 01 xx
#define MPEGPS_PACK_START_CODE             0xBA //(00 00 01 BA)
#define MPEGPS_SYSTEM_HEADER_START_CODE    0xBB //(00 00 01 BB)
#define MPEGPS_SEQUENCE_END_CODE           0xB7 //(00 00 01 B7)
#define MPEGPS_ISO_11172_END_CODE          0xB9 //(00 00 01 B9)

#define MPEGPS_PROGRAM_STREAM_MAP          0xBC
#define MPEGPS_PRIVATE_STREAM_1            0xBD
#define MPEGPS_PADDING_STREAM              0xBE
#define MPEGPS_PRIVATE_STREAM_2            0xBF

// pes stream id
#define MPEGPS_PES_AUDIO_ID                0xC0
#define MPEGPS_PES_VIDEO_ID                0xE0
#define MPEGPS_PES_AC3_ID                  0x80
#define MPEGPS_PES_DTS_ID                  0x8A
#define MPEGPS_PES_LPCM_ID                 0xA0
#define MPEGPS_PES_SUB_ID                  0x20

// pes stream type
#define MPEGPS_STREAM_TYPE_VIDEO_MPEG1     0x01
#define MPEGPS_STREAM_TYPE_VIDEO_MPEG2     0x02
#define MPEGPS_STREAM_TYPE_AUDIO_MPEG1     0x03
#define MPEGPS_STREAM_TYPE_AUDIO_MPEG2     0x04
#define MPEGPS_STREAM_TYPE_PRIVATE_SECTION 0x05
#define MPEGPS_STREAM_TYPE_PRIVATE_DATA    0x06
#define MPEGPS_STREAM_TYPE_AUDIO_AAC       0x0F
#define MPEGPS_STREAM_TYPE_VIDEO_MPEG4     0x10
#define MPEGPS_STREAM_TYPE_VIDEO_H264      0x1B
#define MPEGPS_STREAM_TYPE_AUDIO_AC3       0x81
#define MPEGPS_STREAM_TYPE_AUDIO_DTS       0x8A

// GB/T-28181 define
#define MPEGPS_STREAM_TYPE_VIDEO_SVAC      0x80
#define MPEGPS_STREAM_TYPE_AUDIO_G711      0x90
#define MPEGPS_STREAM_TYPE_AUDIO_G722_1    0x92
#define MPEGPS_STREAM_TYPE_AUDIO_G723_1    0x93
#define MPEGPS_STREAM_TYPE_AUDIO_G729      0x99
#define MPEGPS_STREAM_TYPE_AUDIO_SVAC      0x9B


//////////////////////////////////////////////////////////////////////////
//PS header defination
#pragma pack(push, 1)

typedef struct MPEGPS_PackHeader
{
	uint32_t   psc;           //pack_start_code
	unsigned short  scrb1;         //system_clock_reference_base1
	unsigned short  scrb2;         //system_clock_reference_base2
	unsigned short  scre;          //system_clock_reference_extension
	unsigned short  pmr;           //program_mux_rate
	unsigned char   mb;            //marker_bit
#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned char   psl :3;        //pack_stuffing_length
	unsigned char   res :5;        //reserved
#elif (BYTE_ORDER == BIG_ENDIAN)
	unsigned char   res :5;        //reserved
	unsigned char   psl :3;        //pack_stuffing_length
#endif
} MPEGPS_PackHeader;

typedef struct MPEGPS_SystemHeader
{
	uint32_t   shsc;           //system_header_start_code
	unsigned short  hl;             //head_length
	unsigned char   m1;             //fill marker_bit
	unsigned short  rbd;            //rate_bound
	unsigned char   uc1;
	unsigned char   uc2 ;
	unsigned char   uc3;
	unsigned char   vsi;            //video_stream_id
	unsigned short  vpbsb;          //video_P-STD_buffer_size_bound
	unsigned char   asi;            //audeo_stream_id
	unsigned short  apbsb;          //audeo_P-STD_buffer_size_bound    
} MPEGPS_SystemHeader;

typedef struct MPEGPS_ESMap
{
	unsigned char   stream_type;
	unsigned char   stream_id;
	unsigned short  info_length;
	//unsigned char   info[0];
} MPEGPS_ESMap;

typedef struct MPEGPS_MapHeader
{
	uint32_t   psmsc;         //program_stream_map_start_code
	unsigned short  psml;          //program_stream_map_length
	unsigned char   uc1;
	unsigned char   uc2;
	unsigned short  psil;          //program_stream_info_length
	unsigned short  esml;          //elementary_stream_map_length
	MPEGPS_ESMap    esm[1];        //elementary_stream_map(at least one element)
	uint32_t   s32crc;        //crc32 checksum
} MPEGPS_MapHeader;

typedef struct MPEGPS_PESHeader
{
	uint32_t   pessc           ;   //PES_start_code
	unsigned short  ppl             ;   //PES_packet_length
#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned short  ooc           :1;   //original_or_copy
	unsigned short  ct            :1;   //copyright
	unsigned short  dai           :1;   //data_alignment_indicator
	unsigned short  pp            :1;   //PES_priority
	unsigned short  psc           :2;   //PES_scrambling_control
	unsigned short  ff            :2;   //fixed_field
	unsigned short  pef           :1;   //PES_extion_flag
	unsigned short  pcf           :1;   //PES_crc_flag
	unsigned short  acif          :1;   //additional_copy_info_flag
	unsigned short  dtmf          :1;   //DSM_trick_mode_flag
	unsigned short  erf           :1;   //ES_rate_flag
	unsigned short  ef            :1;   //ESCR_flag
	unsigned short  pdf           :2;   //PTS_DTS_flag  2:只有PTS, 3:有PTS和DTS
#elif(BYTE_ORDER == BIG_ENDIAN)
	unsigned short  ff            :2;   //fixed_field
	unsigned short  psc           :2;   //PES_scrambling_control
	unsigned short  pp            :1;   //PES_priority
	unsigned short  dai           :1;   //data_alignment_indicator
	unsigned short  ct            :1;   //copyright
	unsigned short  ooc           :1;   //original_or_copy
	unsigned short  pdf           :2;   //PTS_DTS_flag
	unsigned short  ef            :1;   //ESCR_flag
	unsigned short  erf           :1;   //ES_rate_flag
	unsigned short  dtmf          :1;   //DSM_trick_mode_flag
	unsigned short  acif          :1;   //additional_copy_info_flag
	unsigned short  pcf           :1;   //PES_crc_flag
	unsigned short  pef           :1;   //PES_extion_flag
#endif
	unsigned char   phdl            ;   //PES_header_data_length    
	unsigned char   pts[5]          ;   //PTS
	unsigned char   dts[5]          ;   //DTS
} MPEGPS_PESHeader;

#pragma pack(pop)


//////////////////////////////////////////////////////////////////////////

// PS encode
extern LIBPSUTIL_API int MPEGPS_PackHeader_Init(MPEGPS_PackHeader *ph, long long timestamp);
extern LIBPSUTIL_API int MPEGPS_SystemHeader_Init(MPEGPS_SystemHeader *sh);

extern LIBPSUTIL_API int           MPEGPS_MapHeader_Init(MPEGPS_MapHeader *mh);
extern LIBPSUTIL_API MPEGPS_ESMap* MPEGPS_MapHeader_NextElement(MPEGPS_MapHeader *mh);
extern LIBPSUTIL_API int           MPEGPS_MapHeader_PushElement(MPEGPS_MapHeader *mh);
extern LIBPSUTIL_API int           MPEGPS_MapHeader_EndElement(MPEGPS_MapHeader *mh);

#define MPEGPS_PES_MAX_PPL   (0xFFFF - sizeof(MPEGPS_PESHeader) + MPEGPS_STARTCODE_LEN)
extern LIBPSUTIL_API int MPEGPS_PESHeader_Init(MPEGPS_PESHeader *pes, unsigned char stream_id, unsigned short stream_length);


#define MPEGPS_PTS_FLAG       2
#define MPEGPS_DTS_FLAG       1
extern LIBPSUTIL_API void          MPEGPS_PTS_Encode(unsigned char pdf, long long timestamp, unsigned char buf[5]);
extern LIBPSUTIL_API unsigned char MPEGPS_PTS_Decode(long long *timestamp, const unsigned char buf[5]);


// PS decode
enum MPEGPS_ParserStatus
{
	MPEGPS_ParerInvalid = 0,
	MPEGPS_ParserHead,
	MPEGPS_ParserContent,
};

struct MPEGPS_ParserInfo
{
	void           *user_data;
	BOOL (__stdcall*unknown_header)(MPEGPS_ParserInfo *info, const unsigned char *buf, int len);
	void (__stdcall*parse_header)(MPEGPS_ParserInfo *info);
	void (__stdcall*header_callback)(MPEGPS_ParserInfo *info);
	void (__stdcall*content_callback)(MPEGPS_ParserInfo *info, const unsigned char *buf, int len);

	unsigned char   header_data[256];
	int             header_pos;
	int             header_size;

	int             content_length;
	long long       pts;
	long long       dts;
};

extern int LIBPSUTIL_API MPEGPS_ParserInit(MPEGPS_ParserInfo *info);
extern int LIBPSUTIL_API MPEGPS_ParserInput(MPEGPS_ParserInfo *info, const unsigned char *buf, int len);
extern int LIBPSUTIL_API MPEGPS_ParserRun(MPEGPS_ParserInfo *info, const unsigned char *buf, int len);


// util function
extern uint32_t crc32_checksum(unsigned char *data, unsigned int len);
