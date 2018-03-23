#ifndef _H264_DECODE_SPS_H_
#define _H264_DECODE_SPS_H_


AMODULE_API BOOL
h264_decode_sps(unsigned char *buf, unsigned int nLen, int &width, int &height);

#define is_es4_header(buf) ((buf)[0] == 0 && (buf)[1] == 0 && (buf)[2] == 0 && (buf)[3] == 1)
#define is_es3_header(buf) ((buf)[0] == 0 && (buf)[1] == 0 && (buf)[2] == 1)

#define get_es_type(buf, pos) ((buf)[pos] & 0x1f)

static inline int
next_es_header(unsigned char *data, int size, int type) {
	assert(is_es4_header(data));
	for (unsigned char *ptr = data+3, *end = data+size-3; ptr < end; ++ptr) {
		if (is_es4_header(ptr) && ((type == -1) || (get_es_type(ptr, 4) == type)))
			return (int)(ptr - data);
	}
	return -1;
}

/* NAL unit types */
enum H264_NAL_Types {
	H264_NAL_SLICE           = 1,
	H264_NAL_DPA             = 2,
	H264_NAL_DPB             = 3,
	H264_NAL_DPC             = 4,
	H264_NAL_IDR_SLICE       = 5,
	H264_NAL_SEI             = 6,
	H264_NAL_SPS             = 7,
	H264_NAL_PPS             = 8,
	H264_NAL_AUD             = 9,
	H264_NAL_END_SEQUENCE    = 10,
	H264_NAL_END_STREAM      = 11,
	H264_NAL_FILLER_DATA     = 12,
	H264_NAL_SPS_EXT         = 13,
	H264_NAL_AUXILIARY_SLICE = 19,
};

#if 0
/**
 * Sequence parameter set
 */
typedef struct SPS {
    unsigned int sps_id;
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int poc_type;                      ///< pic_order_cnt_type
    int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
    int ref_frame_count;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
    int mb_height;                     ///< pic_height_in_map_units_minus1 + 1
    int frame_mbs_only_flag;
    int mb_aff;                        ///< mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int crop;                          ///< frame_cropping_flag

    /* those 4 are already in luma samples */
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    AVRational sar;
    int video_signal_type_present_flag;
    int full_range;
    int colour_description_present_flag;
    enum AVColorPrimaries color_primaries;
    enum AVColorTransferCharacteristic color_trc;
    enum AVColorSpace colorspace;
    int timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    int fixed_frame_rate_flag;
    short offset_for_ref_frame[256]; // FIXME dyn aloc?
    int bitstream_restriction_flag;
    int num_reorder_frames;
    int scaling_matrix_present;
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int pic_struct_present_flag;
    int time_offset_length;
    int cpb_cnt;                          ///< See H.264 E.1.2
    int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
    int cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
    int dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
    int bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
    int residual_color_transform_flag;    ///< residual_colour_transform_flag
    int constraint_set_flags;             ///< constraint_set[0-3]_flag
    uint8_t data[4096];
    size_t data_size;
} SPS;
#endif

#endif
