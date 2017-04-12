// libPSutil.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "libPSutil.h"
extern "C" {
#include "../crypto/crc.h"
};

//////////////////////////////////////////////////////////////////////////
// PS encode function
#define MPEGPS_STARTCODE_LEN  6
#define MARKER_BIT  1

extern LIBPSUTIL_API long MPEGPS_PackHeader_Init(MPEGPS_PackHeader *ph, long long timestamp)
{
	long long ullTmp = timestamp;
	unsigned short usTmp = 0;
	ph->psc = MAKEFOURCC(0,0,1,MPEGPS_PACK_START_CODE);

	usTmp |= 1;
	usTmp <<= 3;
	ullTmp >>= 30;
	ullTmp = ullTmp & 0x07;
	usTmp |= ullTmp;
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	ullTmp = timestamp;
	ullTmp = (ullTmp >> 20) & 0x03FF;
	usTmp <<= 10;
	usTmp |= ullTmp;
	ph->scrb1 = MAKETWOCC(usTmp>>8,usTmp);

	usTmp = 0;
	ullTmp = timestamp;
	ullTmp = (ullTmp >> 15) & 0x1F;
	usTmp |= ullTmp;
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	ullTmp = timestamp;
	ullTmp = (ullTmp >> 5) & 0x03FF;
	usTmp <<= 10;
	usTmp |= ullTmp;
	ph->scrb2 = MAKETWOCC(usTmp>>8,usTmp);

	usTmp = 0;
	ullTmp = timestamp;
	usTmp |= (ullTmp & 0x1F);
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	usTmp <<= 10;
	usTmp |= MARKER_BIT;
	ph->scre = MAKETWOCC(usTmp>>8,usTmp);

	ph->pmr = MAKETWOCC(0x01,0x99);
	ph->mb = 0x9F;
	ph->res = 0x1F;
	ph->psl = 0;
	return sizeof(*ph);
}

extern LIBPSUTIL_API long MPEGPS_SystemHeader_Init(MPEGPS_SystemHeader *sh)
{
	sh->shsc = MAKEFOURCC(0,0,1,MPEGPS_SYSTEM_HEADER_START_CODE);
	sh->hl = sizeof(*sh) - MPEGPS_STARTCODE_LEN;
	sh->hl = MAKETWOCC(sh->hl>>8, sh->hl);

	sh->m1 = 0x80;
	sh->rbd = MAKETWOCC(0xCC,0xF5);
	sh->uc1 = 0x04;
	sh->uc2 = 0xE1;
	sh->uc3 = 0x7F;
	sh->vsi = MPEGPS_PES_VIDEO_ID;
	sh->vpbsb = MAKETWOCC(0xE0,0xE8);
	sh->asi = MPEGPS_PES_AUDIO_ID;
	sh->apbsb = MAKETWOCC(0xC0,0x20);
	return sizeof(*sh);
}

extern LIBPSUTIL_API long MPEGPS_MapHeader_Init(MPEGPS_MapHeader *mh)
{
	mh->psmsc = MAKEFOURCC(0,0,1,MPEGPS_PROGRAM_STREAM_MAP);
	mh->psml = sizeof(*mh) - MPEGPS_STARTCODE_LEN - sizeof(mh->esm);

	mh->uc1 = 0xE1;
	mh->uc2 = 0xFF;
	mh->psil = MAKETWOCC(0,0);
	mh->esml = 0;
	return MPEGPS_STARTCODE_LEN + mh->psml;
}

extern LIBPSUTIL_API MPEGPS_ESMap* MPEGPS_MapHeader_NextElement(MPEGPS_MapHeader *mh)
{
	return (MPEGPS_ESMap*)((unsigned char*)mh->esm + mh->esml);
}

extern LIBPSUTIL_API long MPEGPS_MapHeader_PushElement(MPEGPS_MapHeader *mh)
{
	MPEGPS_ESMap *esm = (MPEGPS_ESMap*)((unsigned char*)mh->esm + mh->esml);

	mh->psml += sizeof(*esm) + esm->info_length;
	mh->esml += sizeof(*esm) + esm->info_length;
	esm->info_length = MAKETWOCC(esm->info_length>>8,esm->info_length);

	return MPEGPS_STARTCODE_LEN + mh->psml;
}

extern LIBPSUTIL_API long MPEGPS_MapHeader_EndElement(MPEGPS_MapHeader *mh)
{
	long psml = mh->psml;
	mh->psml = MAKETWOCC(mh->psml>>8,mh->psml);
	mh->esml = MAKETWOCC(mh->esml>>8,mh->esml);

	*(unsigned long*)((unsigned char*)mh+MPEGPS_STARTCODE_LEN+psml-4)
		= av_crc(NULL, 0, (unsigned char*)mh+MPEGPS_STARTCODE_LEN, psml-4);
	return MPEGPS_STARTCODE_LEN + psml;
}

extern LIBPSUTIL_API long MPEGPS_PESHeader_Init(MPEGPS_PESHeader *pes, unsigned char stream_id, unsigned short stream_length)
{
	if (stream_length > MPEGPS_PES_MAX_PPL) {
		assert(FALSE);
		return -1;
	}

	pes->pessc = MAKEFOURCC(0,0,1,stream_id);
	pes->ppl = MAKETWOCC(stream_length>>8, stream_length);
	pes->ooc = 0;
	pes->ct = 0;
	pes->dai = 1;
	pes->pp = 0;
	pes->psc = 0;
	pes->ff = 2;
	pes->pef = 0;
	pes->pcf = 0;
	pes->acif = 0;
	pes->dtmf = 0;
	pes->erf = 0;
	pes->ef = 0;
	pes->pdf = MPEGPS_PTS_FLAG|MPEGPS_DTS_FLAG;
	pes->phdl = 10;
	return sizeof(*pes) + stream_length;
}

extern LIBPSUTIL_API void MPEGPS_PTS_Encode(unsigned char pdf, long long timestamp, unsigned char buf[5])
{
	buf[0] = (pdf<<4) | ((timestamp>>29)&0x0E) | MARKER_BIT; // 30~32: 3 bits
	buf[1] = (timestamp>>21)&0xFF;                           // 22~29: 8 bits
	buf[2] = ((timestamp>>14)&0xFE) | MARKER_BIT;            // 15~21: 7 bits
	buf[3] = (timestamp>>7)&0xFF;                            // 7~14: 8 bits
	buf[4] = ((timestamp<<1)&0xFE) | MARKER_BIT;             // 0~6: 7 bits
}

extern LIBPSUTIL_API unsigned char MPEGPS_PTS_Decode(long long *timestamp, const unsigned char buf[5])
{
	*timestamp = (long long(buf[0]&0x0E) << 29)
	           | ((MAKETWOCC(buf[2],buf[1])>>1) << 15)
	           | (MAKETWOCC(buf[4],buf[3])>>1);
	return (buf[0]>>4);
}

//////////////////////////////////////////////////////////////////////////
// PS decode function

inline long FillData(unsigned char *dest_ptr, long &dest_pos, long dest_len, const unsigned char *&src_ptr, long &src_len)
{
	long used = min(dest_len-dest_pos, src_len);
	memcpy(dest_ptr+dest_pos, src_ptr, used);

	dest_pos += used;
	src_ptr += used;
	src_len -= used;
	return used;
}

extern long LIBPSUTIL_API MPEGPS_ParserInit(MPEGPS_ParserInfo *info)
{
	memset(info, 0, sizeof(*info));
	return 0;
}

static inline long MPEGPS_ParserFillHeader(MPEGPS_ParserInfo *info, const unsigned char *&buf, long &len)
{
	long used = 0;
	if (info->header_pos < info->header_size)
	{
		used = min(info->header_size-info->header_pos, len);
		memcpy(info->header_data+info->header_pos, buf, used);

		info->header_pos += used;
		buf += used;
		len -= used;
	}
	return used;
}

#define MPEGPS_IsHeader(buf)  ((buf[0] == 0x00) && (buf[1] == 0x00) && (buf[2] == 0x01))

static BOOL MPEGPS_ParserHeaderBegin(MPEGPS_ParserInfo *info, const unsigned char *buf, long len)
{
	assert(len >= MPEGPS_STARTCODE_LEN);
	if (!MPEGPS_IsHeader(buf)) {
		if (info->unknown_header != NULL)
			return info->unknown_header(info, buf, len);
		return FALSE;
	}

	switch (buf[3])
	{
	case MPEGPS_PACK_START_CODE:
		info->header_size = sizeof(MPEGPS_PackHeader);
		break;

	case MPEGPS_SYSTEM_HEADER_START_CODE:
	case MPEGPS_PROGRAM_STREAM_MAP:
		info->header_size = MPEGPS_STARTCODE_LEN + MAKETWOCC(buf[5],buf[4]);
		break;

	case MPEGPS_PRIVATE_STREAM_1:
	case MPEGPS_PADDING_STREAM:
	case MPEGPS_PRIVATE_STREAM_2:
		info->header_size = MPEGPS_STARTCODE_LEN;
		info->content_length = MAKETWOCC(buf[5],buf[4]);
		break;

	case MPEGPS_PES_AUDIO_ID:
	case MPEGPS_PES_VIDEO_ID:
	case MPEGPS_PES_AC3_ID:
	case MPEGPS_PES_DTS_ID:
	case MPEGPS_PES_LPCM_ID:
	case MPEGPS_PES_SUB_ID:
		info->header_size = sizeof(MPEGPS_PESHeader)-10;
		break;

	default:
		// skip unknown header
		if (info->unknown_header != NULL)
			return info->unknown_header(info, buf, len);
		return FALSE;
	}
	return TRUE;
}

static void MPEGPS_ParserHeaderEnd(MPEGPS_ParserInfo *info)
{
	switch (info->header_data[3])
	{
	case MPEGPS_PACK_START_CODE:
		info->header_size = sizeof(MPEGPS_PackHeader) + ((MPEGPS_PackHeader*)info->header_data)->psl;
		break;

	case MPEGPS_PES_AUDIO_ID:
	case MPEGPS_PES_VIDEO_ID:
	case MPEGPS_PES_AC3_ID:
	case MPEGPS_PES_DTS_ID:
	case MPEGPS_PES_LPCM_ID:
	case MPEGPS_PES_SUB_ID:
		MPEGPS_PESHeader *pes; pes = (MPEGPS_PESHeader*)info->header_data;
		if (!(pes->ff & 2)) {
			assert(FALSE);
			break;
		}

		info->header_size = sizeof(MPEGPS_PESHeader)-10+pes->phdl;
		info->content_length = MPEGPS_STARTCODE_LEN
		                     + MAKETWOCC(info->header_data[5], info->header_data[4])
		                     - info->header_size;
		if (info->content_length < 0) {
			TCHAR buf[128];
			_stprintf_s(buf, _T("pes[%02x]: parse error, header(%d): %02x, content(%d): %02x %02x.\n"),
				info->header_data[3], info->header_size, pes->phdl,
				info->content_length, info->header_data[4], info->header_data[5]);
			::OutputDebugString(buf);
			info->header_size = 0;
			info->header_pos = 0;
			info->content_length = 0;
			return;
		}
		if (info->header_pos < info->header_size)
			break;

		unsigned char *buf; buf = pes->pts;
		if (pes->pdf & MPEGPS_PTS_FLAG) {
			MPEGPS_PTS_Decode(&info->pts, buf);
			buf += 5;
		}
		if (pes->pdf == (MPEGPS_PTS_FLAG|MPEGPS_DTS_FLAG)) {
			MPEGPS_PTS_Decode(&info->dts, buf);
			buf += 5;
		}
		break;

	default:
		// find next header
		break;
	}

	if (info->parse_header != NULL)
		info->parse_header(info);

	if (info->header_size > sizeof(info->header_data)) {
		assert(FALSE);
		info->header_size = 0;
		info->header_pos = 0;
		info->content_length = 0;
	}
}

extern long LIBPSUTIL_API MPEGPS_ParserInput(MPEGPS_ParserInfo *info, const unsigned char *buf, long len)
{
	// fill header data
	if (info->header_pos < info->header_size)
	{
		long used = FillData(info->header_data, info->header_pos, info->header_size, buf, len);
		if (info->header_pos == info->header_size)
			MPEGPS_ParserHeaderEnd(info);
		return used;
	}

	// skip content data
	if (info->content_length != 0)
	{
		long used = min(info->content_length, len);
		info->content_length -= used;

		if (info->content_callback != NULL)
			info->content_callback(info, buf, used);

		if (info->content_length == 0) {
			info->header_size = 0;
			info->header_pos = 0;
		}
		return used;
	}

	// find next header
	if (info->header_size != 0)
	{
		if (info->header_callback != NULL)
			info->header_callback(info);
		info->header_pos = 0;
		info->header_size = 0;
		return 0;
	}

	// probe cache data
	if (info->header_pos != 0)
	{
		long used = 0;
		if (info->header_pos < MPEGPS_STARTCODE_LEN)
			used = FillData(info->header_data, info->header_pos, MPEGPS_STARTCODE_LEN, buf, len);
		if (info->header_pos >= MPEGPS_STARTCODE_LEN)
		{
			if (!MPEGPS_ParserHeaderBegin(info, info->header_data, info->header_pos)) {
				info->header_pos -= used + 1;
				memmove(info->header_data, info->header_data+1, info->header_pos);
				used = 0;
			}
		}
		return used;
	}

	// probe header
	long pos = 0;
	while (len >= MPEGPS_STARTCODE_LEN)
	{
		if (MPEGPS_ParserHeaderBegin(info, buf, len)) {
			;
			return pos;
		}
		pos += 1;
		buf += 1;
		len -= 1;
	}

	// cache left data
	memcpy(info->header_data, buf, len);
	info->header_pos = len;
	return pos+len;
}

extern long LIBPSUTIL_API MPEGPS_ParserRun(MPEGPS_ParserInfo *info, const unsigned char *buf, long len)
{
	long pos = 0;
	while (pos < len)
	{
		long used = MPEGPS_ParserInput(info, buf+pos, len-pos);
		if ((info->content_callback == NULL)
		 && (info->header_size != 0)
		 && (info->header_size == info->header_pos))
			break;

		pos += used;
	}
	return pos;
}
