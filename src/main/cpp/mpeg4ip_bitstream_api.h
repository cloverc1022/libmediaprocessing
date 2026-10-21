
#ifndef __BITSTREAM_API_H__
#define __BITSTREAM_API_H__ 

//#include "common.h"
#include <libavutil/common.h>

//C wrapper API for C++
#ifdef __cplusplus
extern "C" {
#endif

	#ifndef MIN
		#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
	#endif

	typedef struct h264_decode_t {
		uint8_t profile;
		uint8_t level;
		uint32_t chroma_format_idc;
		uint8_t residual_colour_transform_flag;
		uint32_t bit_depth_luma_minus8;
		uint32_t bit_depth_chroma_minus8;
		uint8_t qpprime_y_zero_transform_bypass_flag;
		uint8_t seq_scaling_matrix_present_flag;
		uint32_t log2_max_frame_num_minus4;
		uint32_t log2_max_pic_order_cnt_lsb_minus4;
		uint32_t pic_order_cnt_type;
		uint8_t frame_mbs_only_flag;
		uint8_t pic_order_present_flag;
		uint8_t delta_pic_order_always_zero_flag;
		int32_t offset_for_non_ref_pic;
		int32_t offset_for_top_to_bottom_field;
		uint32_t pic_order_cnt_cycle_length;
		int16_t offset_for_ref_frame[256];

		uint8_t nal_ref_idc;
		uint8_t nal_unit_type;

		uint8_t field_pic_flag;
		uint8_t bottom_field_flag;
		uint32_t frame_num;
		uint32_t idr_pic_id;
		uint32_t pic_order_cnt_lsb;
		int32_t delta_pic_order_cnt_bottom;
		int32_t delta_pic_order_cnt[2];

		uint32_t pic_width, pic_height;
		uint32_t slice_type;

		/* POC state */
		int32_t  pic_order_cnt;        /* can be < 0 */

		uint32_t  pic_order_cnt_msb;
		uint32_t  pic_order_cnt_msb_prev;
		uint32_t  pic_order_cnt_lsb_prev;
		uint32_t  frame_num_prev;
		int32_t  frame_num_offset;
		int32_t  frame_num_offset_prev;

		uint8_t NalHrdBpPresentFlag;
		uint8_t VclHrdBpPresentFlag;
		uint8_t CpbDpbDelaysPresentFlag;
		uint8_t pic_struct_present_flag;
		uint8_t cpb_removal_delay_length_minus1;
		uint8_t time_offset_length;
		uint32_t cpb_cnt_minus1;
		uint8_t initial_cpb_removal_delay_length_minus1;
	}h264_decode_t;


	void* C_BitstreamCreate(const uint8_t *buffer, uint32_t bit_len);
	void  C_BitstreamDestroy(void* thisC);
	uint32_t C_Bitstream_GetBits(void* thisC, uint32_t numBits);
	int   C_Bitstream_bits_remain(void* thisC);
	uint32_t C_Bitstream_PeekBits(void* thisC, uint32_t bits);

#ifdef __cplusplus
}
#endif


#endif
