#include "stdafx.h"
#include <math.h> 
#ifndef AMODULE_API
#define AMODULE_API
#endif
#pragma warning(disable: 4189) //局部变量已初始化但不引用

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)  
{  
	//计算0bit的个数  
	UINT nZeroNum = 0;  
	while (nStartBit < nLen * 8)  
	{  
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余  
		{  
			break;  
		}  
		nZeroNum++;  
		nStartBit++;  
	}  
	nStartBit ++;  


	//计算结果  
	DWORD dwRet = 0;  
	for (UINT i=0; i<nZeroNum; i++)  
	{  
		dwRet <<= 1;  
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))  
		{  
			dwRet += 1;  
		}  
		nStartBit++;  
	}  
	return (1 << nZeroNum) - 1 + dwRet;  
}  


int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)  
{  
	int UeVal=Ue(pBuff,nLen,nStartBit);  
	double k=UeVal;  
	int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00  
	if (UeVal % 2==0)  
		nValue=-nValue;  
	return nValue;  
}  


DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit)  
{  
	DWORD dwRet = 0;  
	for (UINT i=0; i<BitCount; i++)  
	{  
		dwRet <<= 1;  
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))  
		{  
			dwRet += 1;  
		}  
		nStartBit++;  
	}  
	return dwRet;  
}  

AMODULE_API BOOL
h264_decode_sps(BYTE * buf,unsigned int nLen,int &width,int &height)  
{  
	UINT StartBit=0;   
	int forbidden_zero_bit=u(1,buf,StartBit);  
	int nal_ref_idc=u(2,buf,StartBit);  
	int nal_unit_type=u(5,buf,StartBit);  
	if (nal_unit_type == 7)  
	{  
		int profile_idc=u(8,buf,StartBit);  
		int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;  
		int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;  
		int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;  
		int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;  
		int reserved_zero_4bits=u(4,buf,StartBit);  
		int level_idc=u(8,buf,StartBit);  

		int seq_parameter_set_id=Ue(buf,nLen,StartBit);  

		if( profile_idc == 100 || profile_idc == 110 ||  
			profile_idc == 122 || profile_idc == 144 )  
		{  
			int chroma_format_idc=Ue(buf,nLen,StartBit);  
			if( chroma_format_idc == 3 )  
				int residual_colour_transform_flag=u(1,buf,StartBit);  
			int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);  
			int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);  
			int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);  
			int seq_scaling_matrix_present_flag=u(1,buf,StartBit);  

			int seq_scaling_list_present_flag[8];  
			if( seq_scaling_matrix_present_flag )  
			{  
				for( int i = 0; i < 8; i++ ) {  
					seq_scaling_list_present_flag[i]=u(1,buf,StartBit);  
				}  
			}  
		}  
		int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);  
		int pic_order_cnt_type=Ue(buf,nLen,StartBit);  
		if( pic_order_cnt_type == 0 )  
			int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);  
		else if( pic_order_cnt_type == 1 )  
		{  
			int delta_pic_order_always_zero_flag=u(1,buf,StartBit);  
			int offset_for_non_ref_pic=Se(buf,nLen,StartBit);  
			int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);  
			int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);  

			//int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];  
			for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )  
				/*offset_for_ref_frame[i]=*/Se(buf,nLen,StartBit);  
			//delete [] offset_for_ref_frame;  
		}  
		int num_ref_frames=Ue(buf,nLen,StartBit);  
		int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);  
		int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);  
		int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);  

		width=(pic_width_in_mbs_minus1+1)*16;  
		height=(pic_height_in_map_units_minus1+1)*16;  

		return TRUE;  
	}  
	else  
		return FALSE;  
}  

#if 0
#include "avconfig.h"
#include "get_bits.h"
#include "golomb.h"

int ff_h264_decode_seq_parameter_set(SPS *sps, unsigned char *data, int size, int ignore_truncation)
{
    int profile_idc, level_idc, constraint_set_flags = 0;
    unsigned int sps_id;
    int i, log2_max_frame_num_minus4;
    GetBitContext gb2, *gb = &gb2;
    init_get_bits8(gb, data, size);

    profile_idc           = get_bits(gb, 8);
    constraint_set_flags |= get_bits1(gb) << 0;   // constraint_set0_flag
    constraint_set_flags |= get_bits1(gb) << 1;   // constraint_set1_flag
    constraint_set_flags |= get_bits1(gb) << 2;   // constraint_set2_flag
    constraint_set_flags |= get_bits1(gb) << 3;   // constraint_set3_flag
    constraint_set_flags |= get_bits1(gb) << 4;   // constraint_set4_flag
    constraint_set_flags |= get_bits1(gb) << 5;   // constraint_set5_flag
    skip_bits(gb, 2);                             // reserved_zero_2bits
    level_idc = get_bits(gb, 8);
    sps_id    = get_ue_golomb_31(gb);

    if (sps_id >= MAX_SPS_COUNT) {
        av_log(avctx, AV_LOG_ERROR, "sps_id %u out of range\n", sps_id);
        //goto fail;
    }

    sps->sps_id               = sps_id;
    sps->time_offset_length   = 24;
    sps->profile_idc          = profile_idc;
    sps->constraint_set_flags = constraint_set_flags;
    sps->level_idc            = level_idc;
    sps->full_range           = -1;

    memset(sps->scaling_matrix4, 16, sizeof(sps->scaling_matrix4));
    memset(sps->scaling_matrix8, 16, sizeof(sps->scaling_matrix8));
    sps->scaling_matrix_present = 0;
    sps->colorspace = 2; //AVCOL_SPC_UNSPECIFIED

    if (sps->profile_idc == 100 ||  // High profile
        sps->profile_idc == 110 ||  // High10 profile
        sps->profile_idc == 122 ||  // High422 profile
        sps->profile_idc == 244 ||  // High444 Predictive profile
        sps->profile_idc ==  44 ||  // Cavlc444 profile
        sps->profile_idc ==  83 ||  // Scalable Constrained High profile (SVC)
        sps->profile_idc ==  86 ||  // Scalable High Intra profile (SVC)
        sps->profile_idc == 118 ||  // Stereo High profile (MVC)
        sps->profile_idc == 128 ||  // Multiview High profile (MVC)
        sps->profile_idc == 138 ||  // Multiview Depth High profile (MVCD)
        sps->profile_idc == 144) {  // old High444 profile
        sps->chroma_format_idc = get_ue_golomb_31(gb);
        if (sps->chroma_format_idc > 3U) {
            avpriv_request_sample(avctx, "chroma_format_idc %u",
                                  sps->chroma_format_idc);
            goto fail;
        } else if (sps->chroma_format_idc == 3) {
            sps->residual_color_transform_flag = get_bits1(gb);
            if (sps->residual_color_transform_flag) {
                av_log(avctx, AV_LOG_ERROR, "separate color planes are not supported\n");
                goto fail;
            }
        }
        sps->bit_depth_luma   = get_ue_golomb(gb) + 8;
        sps->bit_depth_chroma = get_ue_golomb(gb) + 8;
        if (sps->bit_depth_chroma != sps->bit_depth_luma) {
            avpriv_request_sample(avctx,
                                  "Different chroma and luma bit depth");
            goto fail;
        }
        if (sps->bit_depth_luma   < 8 || sps->bit_depth_luma   > 14 ||
            sps->bit_depth_chroma < 8 || sps->bit_depth_chroma > 14) {
            av_log(avctx, AV_LOG_ERROR, "illegal bit depth value (%d, %d)\n",
                   sps->bit_depth_luma, sps->bit_depth_chroma);
            goto fail;
        }
        sps->transform_bypass = get_bits1(gb);
        sps->scaling_matrix_present |= decode_scaling_matrices(gb, sps, NULL, 1,
                                sps->scaling_matrix4, sps->scaling_matrix8);
    } else {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma    = 8;
        sps->bit_depth_chroma  = 8;
    }

    log2_max_frame_num_minus4 = get_ue_golomb(gb);
    if (log2_max_frame_num_minus4 < MIN_LOG2_MAX_FRAME_NUM - 4 ||
        log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM - 4) {
        av_log(avctx, AV_LOG_ERROR,
               "log2_max_frame_num_minus4 out of range (0-12): %d\n",
               log2_max_frame_num_minus4);
        goto fail;
    }
    sps->log2_max_frame_num = log2_max_frame_num_minus4 + 4;

    sps->poc_type = get_ue_golomb_31(gb);

    if (sps->poc_type == 0) { // FIXME #define
        unsigned t = get_ue_golomb(gb);
        if (t>12) {
            av_log(avctx, AV_LOG_ERROR, "log2_max_poc_lsb (%d) is out of range\n", t);
            goto fail;
        }
        sps->log2_max_poc_lsb = t + 4;
    } else if (sps->poc_type == 1) { // FIXME #define
        sps->delta_pic_order_always_zero_flag = get_bits1(gb);
        sps->offset_for_non_ref_pic           = get_se_golomb(gb);
        sps->offset_for_top_to_bottom_field   = get_se_golomb(gb);
        sps->poc_cycle_length                 = get_ue_golomb(gb);

        if ((unsigned)sps->poc_cycle_length >=
            FF_ARRAY_ELEMS(sps->offset_for_ref_frame)) {
            av_log(avctx, AV_LOG_ERROR,
                   "poc_cycle_length overflow %d\n", sps->poc_cycle_length);
            goto fail;
        }

        for (i = 0; i < sps->poc_cycle_length; i++)
            sps->offset_for_ref_frame[i] = get_se_golomb(gb);
    } else if (sps->poc_type != 2) {
        av_log(avctx, AV_LOG_ERROR, "illegal POC type %d\n", sps->poc_type);
        goto fail;
    }

    sps->ref_frame_count = get_ue_golomb_31(gb);
    if (avctx->codec_tag == MKTAG('S', 'M', 'V', '2'))
        sps->ref_frame_count = FFMAX(2, sps->ref_frame_count);
    if (sps->ref_frame_count > MAX_DELAYED_PIC_COUNT) {
        av_log(avctx, AV_LOG_ERROR,
               "too many reference frames %d\n", sps->ref_frame_count);
        goto fail;
    }
    sps->gaps_in_frame_num_allowed_flag = get_bits1(gb);
    sps->mb_width                       = get_ue_golomb(gb) + 1;
    sps->mb_height                      = get_ue_golomb(gb) + 1;

    sps->frame_mbs_only_flag = get_bits1(gb);
    if (!sps->frame_mbs_only_flag)
        sps->mb_aff = get_bits1(gb);
    else
        sps->mb_aff = 0;

    if ((unsigned)sps->mb_width  >= INT_MAX / 16 ||
        (unsigned)sps->mb_height >= INT_MAX / (16 * (2 - sps->frame_mbs_only_flag)) ||
        av_image_check_size(16 * sps->mb_width,
                            16 * sps->mb_height * (2 - sps->frame_mbs_only_flag), 0, avctx)) {
        av_log(avctx, AV_LOG_ERROR, "mb_width/height overflow\n");
        goto fail;
    }

    sps->direct_8x8_inference_flag = get_bits1(gb);

#ifndef ALLOW_INTERLACE
    if (sps->mb_aff)
        av_log(avctx, AV_LOG_ERROR,
               "MBAFF support not included; enable it at compile-time.\n");
#endif
    sps->crop = get_bits1(gb);
    if (sps->crop) {
        unsigned int crop_left   = get_ue_golomb(gb);
        unsigned int crop_right  = get_ue_golomb(gb);
        unsigned int crop_top    = get_ue_golomb(gb);
        unsigned int crop_bottom = get_ue_golomb(gb);
        int width  = 16 * sps->mb_width;
        int height = 16 * sps->mb_height * (2 - sps->frame_mbs_only_flag);

        if (avctx->flags2 & AV_CODEC_FLAG2_IGNORE_CROP) {
            av_log(avctx, AV_LOG_DEBUG, "discarding sps cropping, original "
                                           "values are l:%d r:%d t:%d b:%d\n",
                   crop_left, crop_right, crop_top, crop_bottom);

            sps->crop_left   =
            sps->crop_right  =
            sps->crop_top    =
            sps->crop_bottom = 0;
        } else {
            int vsub   = (sps->chroma_format_idc == 1) ? 1 : 0;
            int hsub   = (sps->chroma_format_idc == 1 ||
                          sps->chroma_format_idc == 2) ? 1 : 0;
            int step_x = 1 << hsub;
            int step_y = (2 - sps->frame_mbs_only_flag) << vsub;

            if (crop_left & (0x1F >> (sps->bit_depth_luma > 8)) &&
                !(avctx->flags & AV_CODEC_FLAG_UNALIGNED)) {
                crop_left &= ~(0x1F >> (sps->bit_depth_luma > 8));
                av_log(avctx, AV_LOG_WARNING,
                       "Reducing left cropping to %d "
                       "chroma samples to preserve alignment.\n",
                       crop_left);
            }

            if (crop_left  > (unsigned)INT_MAX / 4 / step_x ||
                crop_right > (unsigned)INT_MAX / 4 / step_x ||
                crop_top   > (unsigned)INT_MAX / 4 / step_y ||
                crop_bottom> (unsigned)INT_MAX / 4 / step_y ||
                (crop_left + crop_right ) * step_x >= width ||
                (crop_top  + crop_bottom) * step_y >= height
            ) {
                av_log(avctx, AV_LOG_ERROR, "crop values invalid %d %d %d %d / %d %d\n", crop_left, crop_right, crop_top, crop_bottom, width, height);
                goto fail;
            }

            sps->crop_left   = crop_left   * step_x;
            sps->crop_right  = crop_right  * step_x;
            sps->crop_top    = crop_top    * step_y;
            sps->crop_bottom = crop_bottom * step_y;
        }
    } else {
        sps->crop_left   =
        sps->crop_right  =
        sps->crop_top    =
        sps->crop_bottom =
        sps->crop        = 0;
    }

    sps->vui_parameters_present_flag = get_bits1(gb);
    if (sps->vui_parameters_present_flag) {
        int ret = decode_vui_parameters(gb, avctx, sps);
        if (ret < 0)
            goto fail;
    }

    if (get_bits_left(gb) < 0) {
        av_log(avctx, ignore_truncation ? AV_LOG_WARNING : AV_LOG_ERROR,
               "Overread %s by %d bits\n", sps->vui_parameters_present_flag ? "VUI" : "SPS", -get_bits_left(gb));
        if (!ignore_truncation)
            goto fail;
    }

    /* if the maximum delay is not stored in the SPS, derive it based on the
     * level */
    if (!sps->bitstream_restriction_flag &&
        (sps->ref_frame_count || avctx->strict_std_compliance >= FF_COMPLIANCE_STRICT)) {
        sps->num_reorder_frames = MAX_DELAYED_PIC_COUNT - 1;
        for (i = 0; i < FF_ARRAY_ELEMS(level_max_dpb_mbs); i++) {
            if (level_max_dpb_mbs[i][0] == sps->level_idc) {
                sps->num_reorder_frames = FFMIN(level_max_dpb_mbs[i][1] / (sps->mb_width * sps->mb_height),
                                                sps->num_reorder_frames);
                break;
            }
        }
    }

    if (!sps->sar.den)
        sps->sar.den = 1;

    if (avctx->debug & FF_DEBUG_PICT_INFO) {
        static const char csp[4][5] = { "Gray", "420", "422", "444" };
        av_log(avctx, AV_LOG_DEBUG,
               "sps:%u profile:%d/%d poc:%d ref:%d %dx%d %s %s crop:%u/%u/%u/%u %s %s %"PRId32"/%"PRId32" b%d reo:%d\n",
               sps_id, sps->profile_idc, sps->level_idc,
               sps->poc_type,
               sps->ref_frame_count,
               sps->mb_width, sps->mb_height,
               sps->frame_mbs_only_flag ? "FRM" : (sps->mb_aff ? "MB-AFF" : "PIC-AFF"),
               sps->direct_8x8_inference_flag ? "8B8" : "",
               sps->crop_left, sps->crop_right,
               sps->crop_top, sps->crop_bottom,
               sps->vui_parameters_present_flag ? "VUI" : "",
               csp[sps->chroma_format_idc],
               sps->timing_info_present_flag ? sps->num_units_in_tick : 0,
               sps->timing_info_present_flag ? sps->time_scale : 0,
               sps->bit_depth_luma,
               sps->bitstream_restriction_flag ? sps->num_reorder_frames : -1
               );
    }

    /* check if this is a repeat of an already parsed SPS, then keep the
     * original one.
     * otherwise drop all PPSes that depend on it */
    if (ps->sps_list[sps_id] &&
        !memcmp(ps->sps_list[sps_id]->data, sps_buf->data, sps_buf->size)) {
        av_buffer_unref(&sps_buf);
    } else {
        remove_sps(ps, sps_id);
        ps->sps_list[sps_id] = sps_buf;
    }

    return 0;

fail:
    av_buffer_unref(&sps_buf);
    return AVERROR_INVALIDDATA;
}
#endif
