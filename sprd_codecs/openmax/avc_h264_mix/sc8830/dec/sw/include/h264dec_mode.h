/******************************************************************************
** File Name:      h264dec_mode.h                                            *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_MODE_H_
#define _H264DEC_MODE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "sci_types.h"
#include "h264dec_basic.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

//#define YUV_THREE_PLANE


/**
 * Value of Picture.reference when Picture is not a reference picture, but
 * is held for delayed output.
 */
#define DELAYED_PIC_REF 4

	
#define MAX_NUM_SLICE_GROUPS_MINUS1	8
	
typedef struct 
{
	int8	valid;									//indicates the parameter set is valid
	uint8	pic_parameter_set_id;					//ue(v)
	int8	seq_parameter_set_id;					//ue(v)
	uint8	pic_order_present_flag;					//u(1)

	uint8	num_slice_groups_minus1;				//ue(v)
	int8	slice_group_map_type;					//ue(v)
	int8	num_ref_idx_l0_active_minus1;			//ue(v)
	int8	num_ref_idx_l1_active_minus1;			//uv(v)
	
	int8	weighted_pred_flag;						//u(1)
	int8	weighted_bipred_idc;					//u(2)
	int8	pic_init_qp_minus26;					//se(v)
	int8	pic_init_qs_minus26;					//se(v)
	
	int8	chroma_qp_index_offset;					//se(v)
	int8	second_chroma_qp_index_offset;			//se(v)
	int8	deblocking_filter_control_present_flag;	//u(1)
	int8	constrained_intra_pred_flag;			//u(1)

	int8	redundant_pic_cnt_present_flag;			//u(1)
	int8	entropy_coding_mode_flag;				//u(1)
	int8	transform_8x8_mode_flag;				//u(1)
	int8	pic_scaling_matrix_present_flag;			//u(1)

	//fmo
	uint16	run_length_minus1[MAX_NUM_SLICE_GROUPS_MINUS1]; //ue(v)
	uint32	top_left[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
	uint32	bottom_right[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
	int32	slice_group_change_direction_flag; //u(1)
	uint32	slice_group_change_rate_minus1;	//ue(v)
	uint32	num_slice_group_map_units_minus1; //ue(v)
	
	uint8	ScalingList4x4[6][16];
	uint8	ScalingList8x8[2][64];
	
	uint8	*slice_group_id;	//complete MBAmap u(v) for slice group map type 6
}DEC_PPS_T;

#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE	256
#define MAXIMUMVALUEOFcpb_cnt   32

typedef struct hrd_parameters_tag
{
	uint32 cpb_cnt_minus1;                                   // ue(v)
    	uint32 bit_rate_value_minus1[MAXIMUMVALUEOFcpb_cnt];  // ue(v)
    	uint32 cpb_size_value_minus1[MAXIMUMVALUEOFcpb_cnt];  // ue(v)

  	uint8 bit_rate_scale;                                   // u(4)
  	uint8 cpb_size_scale;                                   // u(4)
  	uint8 initial_cpb_removal_delay_length_minus1;      // u(5)
  	uint8 cpb_removal_delay_length_minus1;                  // u(5)

    	uint8 cbr_flag[MAXIMUMVALUEOFcpb_cnt];  // u(1)

  	uint8 dpb_output_delay_length_minus1;   // u(5)
  	uint8 time_offset_length;                               // u(5)
  	uint16 rsv;
}DEC_HRD_PARAM_T; 

typedef struct
{
	BOOLEAN aspect_ratio_info_present_flag;                   // u(1)
  	BOOLEAN	 overscan_info_present_flag;                       // u(1)
  	BOOLEAN	 overscan_appropriate_flag;                      // u(1)
  	BOOLEAN video_signal_type_present_flag;                   // u(1)

    	uint8 aspect_ratio_idc;                               // u(8)
	uint8 video_format;                                   // u(3)
	uint8 matrix_coefficients;                          // u(8)
  	BOOLEAN      chroma_location_info_present_flag;                // u(1)

	uint16 sar_width;                                    // u(16)
    	uint16 sar_height;                                   // u(16)
    	
    	BOOLEAN      video_full_range_flag;                          // u(1)
    	BOOLEAN      colour_description_present_flag;                // u(1)
      	uint8 colour_primaries;                             // u(8)
      	uint8 transfer_characteristics;                     // u(8)

    	uint32  chroma_sample_loc_type_top_field;               // ue(v)
    	uint32  chroma_sample_loc_type_bottom_field;            // ue(v)

	BOOLEAN      timing_info_present_flag;                         // u(1)
    	BOOLEAN      fixed_frame_rate_flag;                          // u(1)
  	BOOLEAN      nal_hrd_parameters_present_flag;                  // u(1)
  	BOOLEAN      vcl_hrd_parameters_present_flag;                  // u(1)
  	
    	uint32 num_units_in_tick;                              // u(32)
    	uint32 time_scale;                                     // u(32)
    	
    	DEC_HRD_PARAM_T nal_hrd_parameters;                      // hrd_paramters_t
    	DEC_HRD_PARAM_T vcl_hrd_parameters;                      // hrd_paramters_t

    	BOOLEAN      low_delay_hrd_flag;                             // u(1)
  	BOOLEAN      pic_struct_present_flag;                        // u(1)
  	BOOLEAN      bitstream_restriction_flag;                       // u(1)
    	BOOLEAN      motion_vectors_over_pic_boundaries_flag;        // u(1)

	uint32 max_bytes_per_pic_denom;                        // ue(v)
    	uint32 max_bits_per_mb_denom;                          // ue(v)
    	uint32 log2_max_mv_length_vertical;                    // ue(v)
    	uint32 log2_max_mv_length_horizontal;                  // ue(v)
    	uint32 num_reorder_frames;                             // ue(v)
    	uint32 max_dec_frame_buffering;                        // ue(v)
}DEC_VUI_T;

typedef struct 
{
	int8	valid;									//indicates the parameter set is valid
	int8	profile_idc;							//u(8)
	int8	constrained_set0_flag;					//u(1)
	int8	constrained_set1_flag;					//u(1)

	int8	constrained_set2_flag;					//u(1)
	int8	constrained_set3_flag;					//u(1)
	int8	level_idc;								//u(8)
	int8	seq_parameter_set_id;					//ue(v)
	
	int8	log2_max_frame_num_minus4;				//ue(v)
	int8	pic_order_cnt_type;						//
	int8	delta_pic_order_always_zero_flag;		//u(1)
	int8	offset_for_non_ref_pic;					//se(v)
	
	int8	offset_for_top_to_bottom_field;			//se(v)
	int8	num_ref_frames;							//ue(v)
	int8	gaps_in_frame_num_value_allowed_flag;	//u(1)
	int8	pic_width_in_mbs_minus1;				//ue(v)
	
	int8	pic_height_in_map_units_minus1;			//ue(v)
	int8	frame_mbs_only_flag;
	int8	mb_adaptive_frame_field_flag;           // u(1)
	int8	direct_8x8_inference_flag;

	int8	frame_cropping_flag;	
	int8	vui_parameters_present_flag;
	int8	seq_scaling_matrix_present_flag;	
	int8	resv2; //for word aligned

	int8	chroma_format_idc;
	int8	bit_depth_luma_minus8;
	int8	bit_depth_chroma_minus8;
	int8	qpprime_y_zero_transform_bypass_flag;

	uint32	log2_max_pic_order_cnt_lsb_minus4;		//ue(v)
	uint32	num_ref_frames_in_pic_order_cnt_cycle;	//ue(v)
	int32	offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];	//se(v)

    /* if( frmae_cropping_flag) */
    uint32  frame_crop_left_offset;                /* ue(v) */
    uint32  frame_crop_right_offset;               /* ue(v) */
    uint32  frame_crop_top_offset;                 /* ue(v) */
    uint32  frame_crop_bottom_offset;              /* ue(v) */

	
	uint8	ScalingList4x4[6][16];
	uint8	ScalingList8x8[2][64];
	
	DEC_VUI_T *vui_seq_parameters;
}DEC_SPS_T;

/********************************************
			|          |          |
			|	D	   |       B  |   C
			-----------|----------|------
			|          |
				A	   |  curr    |
			|          |		  |
			|          |		  |
			--------------------------
********************************************/

/***********************************************************
*: denote the top or the left or top right reference
?: denote no data
-2: denote the reference is not available

nnz_ref[6x8] layout
the up 6x5 used for Y, and the bottom 3x6 used for U and V

  ? * * * * ? 
  *         
  *         
  *         
  *         
  ? * * ? * *
  *     *
  *     *

  i4x4Pred_mode_ref[6x5] lay out: need top and left prediction
  ? * * * * ?
  *         
  *         
  *         
  *         

  refIdxCache[6x5] lay out: used for mv predict(need top, left, and up right), 
                            and deblocking(need top, or left)
  ? * * * * * ?
  *          -2
  *          -2
  *          -2
  *          -2

  mv_cache[6x5x2] lay out: used for mv predict, need top, left, (up right/up left)
                           motion compensation: need current block's mv
						   deblocking: need top or left block's motion vector
  * * * * * * ? 
  *          -2
  *          -2
  *          -2
  *          -2
  ***********************************************************/
typedef struct mb_info_tag
{
	int8	mb_type;
	int8	is_intra;
	int8	qp;
	int8 transform_size_8x8_flag;
	
	int8	c_ipred_mode;
	int8	cbp;
	int8	dc_coded_flag;
	int8	skip_flag;
}DEC_MB_INFO_T;

typedef struct mb_pred_cache_tag
{
	uint8 pred_Y[16*16];
	uint8 pred_UV[2][8*8];
}DEC_MB_PRED_CACHE_T;

typedef struct mb_data_cache_tag
{
	int8	i16mode;
	int8	all_zero_ref;
	int8	is_direct;	
	uint8	cbp_uv;
	
	uint32 	cbp_luma_iqt;
	uint32	cbp_iqt;			//[25:0]: cbp for iqt
	uint32	cbp_mbc;			//[23:0]: cbp for mbc

	uint32 read_ref_id_flag[2];
	
	int8	top_edge_filter_flag;
	int8	left_edge_filter_flag;
	int8	is_skipped;
	int8	noSubMbPartSizeLess8x8;
	
	int8	mb_avail_a;
	int8	mb_avail_b;
	int8	mb_avail_c;
	int8	mb_avail_d;

	int8	b8mode[4];
	int8	b8pdir[4];

	uint16	BS[8];

	//for cabac decoding, (curr << 8) | (abv << 4) | left, b'210: vuy
	uint32		vld_dc_coded_flag;		

	int8 i4x4pred_mode_cache[12*5];
	int8 nnz_cache[12*8];
	int8  ref_idx_cache[2][12*5];	//used for mv predict and deblocking lsw move this array into mb_cache_ptr is a better way of this 
	int32 ref_pic_id_cache[2][12*5];
	int16 mv_cache[2][12*5*2];
	int16 mvd_cache[2][12*5*2];

	//for direct mb
	int8  direct_cache[12*5];
	int32 direct_pdir;
	int32 fw_rFrame;
	int32 fw_mv_xy;
	int32 bw_rFrame;
	int32 bw_mv_xy;
	int8 moving_block[16];//b-slice, direct type = 1	
	int8	ref_idx_cache_direct[2][16];	//used for mv predict and deblocking
	int16 mv_cache_direct[2][16*2];
	int32 ref_pic_id_cache_direct[2][16];
	
	//for software decoding
	int16	coff_Y[16 * 4*4];		//coff_Y[16 * 4*4];
	int16   coff_UV[2][4 * 4*4];

	DEC_MB_PRED_CACHE_T pred_cache[2];
	
	uint8	leftPredPix_Y[16];
	uint8	leftPredPix_UV[2][8];

	uint16	rsv0;		//for word-align
	uint8        topleftNoFilteredPix;//for 8x8 blk  to store topleft pixel
	uint8   	topFilteredPix[17];

	int8	mb_avail_a_8x8_ipred;
	int8	mb_avail_b_8x8_ipred;
	int8	mb_avail_c_8x8_ipred;
	int8	mb_avail_d_8x8_ipred;
	
	//sw
	uint8 *mb_addr[3];
	uint32 blk4x4_offset[16+4];

	//hw	
	uint32	mbc_mb_mode; 
	uint8 	mbc_ipred_cmd[8];
	uint8	mbc_b8pdir[4];
}DEC_MB_CACHE_T;

typedef struct short_VLC_tag
{
	const int16 (*table)[2];	///<code, bits
//	uint32	bits;
}SHORT_VLC_SYMBOL_T;

typedef struct byte_VLC_tag
{
	const int8 (*table)[2];	///<code, bits
//	uint32	bits;
}BYTE_VLC_SYMBOL_T;

typedef struct CABACContext{
    int32 low;
    int32 range;
}CABACContext;

typedef struct 
{
//	int8	ei_flag;			//!< 0 if the partArr[0] contains valid information
	int8	qp;
	int8	slice_qp_delta;
	int8	picture_type;		//!< picture type
	uint8	pic_parameter_set_id;	//!< the ID of the picture parameter set the slice is reffering to

	int8	next_header;
	int8	LFDisableIdc;		//!< Disable loop filter on slice
	int8	LFAlphaC0Offset;	//!< Alpha and C0 offset for filtering slice
	int8	LFBetaOffset;		//!< Beta offset for filtering slice

	uint16	start_mb_nr;		//!< MUST be set by NAL even in case of ei_flag == 1
	int8	ref_pic_list_reordering_flag_l0;
	int8	ref_pic_list_reordering_flag_l1;

	int32	remapping_of_pic_nums_idc_l0[MAX_REF_FRAME_NUMBER+1];
	int32	abs_diff_pic_num_minus1_l0[MAX_REF_FRAME_NUMBER+1];
	int32	long_term_pic_idx_l0[MAX_REF_FRAME_NUMBER+1];

	int32	remapping_of_pic_nums_idc_l1[MAX_REF_FRAME_NUMBER+1];
	int32	abs_diff_pic_num_minus1_l1[MAX_REF_FRAME_NUMBER+1];
	int32	long_term_pic_idx_l1[MAX_REF_FRAME_NUMBER+1];
}DEC_SLICE_T;

typedef struct old_slice_par_tag
{
	uint8	field_pic_flag;
	uint8	bottom_field_flag;
	int8	nal_ref_idc;
	int8	idr_flag;
	
	uint16	idr_pic_id;
	uint8	pps_id;
	uint8	resv;//for word-align

	int32	frame_num;
	uint32	pic_order_cnt_lsb;
	int32	delta_pic_order_cnt_bottom;
	int32	delta_pic_order_cnt[2];
}DEC_OLD_SLICE_PARAMS_T;

typedef struct dec_ref_pic_marking_tag
{
	int32	memory_management_control_operation;
	int32	difference_of_pic_nums_minus1;
	int32	long_term_pic_num;
	int32	long_term_frame_idx;
	int32	max_long_term_frame_idx_plus1;
}DEC_DEC_REF_PIC_MARKING_T;

typedef int32 (*mv_prediction_subMB)(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);

typedef struct img_parameter_tag
{
	uint8	ref_count[2];	//!< number of forward/backward reference
	uint8	frame_width_in_mbs;
	uint8	frame_height_in_mbs;
	
	int8	type;	//slice_type
	int8	direct_spatial_mv_pred_flag;                            //!< 1 for Spatial Direct, 0 for Temporal

	/*Cabac    */
	int16	model_number;
	

	int8	qp;	//!< quant for the current frame
	int8	chroma_qp_offset;
	int8	constrained_intra_pred_flag;
	int8	disposable_flag;

	uint16	mb_x;
	uint16	mb_y;

	int32 b4_offset;
	int32 b4_offset_nnz;
	int32 b4_pitch;

	int32	cod_counter;
	
	int8	redundant_pic_cnt;
	int8 	rsv;
	uint16	num_dec_mb;

	uint16	width;
	uint16	height;

	uint16 	ext_width;
	uint16 	ext_height;

	uint16 	start_in_frameY;
	uint16 	start_in_frameUV;

	int32	error_flag;
	
	int32	curr_mb_nr;
	int32	slice_nr;
	
	uint16	frame_size_in_mbs;
	uint16	old_frame_size_in_mbs;

	uint32	pic_height_in_map_units;

	int8	idr_flag;
	int8	nal_reference_idc;	//!< nal_reference_idc from NAL unit
	uint16	idr_pic_id;

	int32	max_frame_num;

	DEC_SLICE_T	*curr_slice_ptr;	//!< pointer to current slice data struct

	int8	no_output_of_prior_pics_flag;
	int8	long_term_reference_flag;
	int8	adaptive_ref_pic_buffering_flag;
	int8	profile_idc;

	// POC200301: from unsigned int to int
	int32 toppoc;      //poc for this top field // POC200301
    int32 bottompoc;   //poc of bottom field of frame
    int32 framepoc;    //poc of this frame // POC200301
	int32 frame_num;   //frame_num for this frame	//++ Ö¡ºÅ£º²Î¿¼Ö¡Ö¡ºÅ°´½âÂëË³ÐòµÝÔö£¬·Ç²Î¿¼Ö¡ÓëºóÐø²Î¿¼Ö¡Ê¹ÓÃÍ¬Ò»Ö¡ºÅ
	uint32 field_pic_flag;
	uint32 bottom_field_flag;

	uint32	pic_order_cnt_lsb;	// for poc mode 0
	int32	delta_pic_order_cnt_bottom;
	int32	delta_pic_order_cnt[3];	//for poc mode 1

	 // ////////////////////////
  // for POC mode 0:
    int32 PrevPicOrderCntMsb;
	uint32 PrevPicOrderCntLsb;
	int32 PicOrderCntMsb;

  // for POC mode 1:
	int32 AbsFrameNum;
	int32 ExpectedPicOrderCnt, PicOrderCntCycleCnt, FrameNumInPicOrderCntCycle;
	int32 PreviousFrameNum, FrameNumOffset;
	int32 ExpectedDeltaPerPicOrderCntCycle;
    int32 PreviousPOC, ThisPOC;
	int32 PreviousFrameNumOffset;
  // /////////////////////////
	int32	pre_frame_num;	//!< store the frame_num in the last decoded slice, for detecting gap in frame_num
	uint32 has_b_frames;
	int32 low_delay;

	int8 list_count;
	int8	last_has_mmco_5;	
//	int8 	last_pic_bottom_field;
	int8	is_new_pic;
	int8 is_cabac;
    CABACContext cabac;
    uint8      cabac_state[460];

	DEC_DEC_REF_PIC_MARKING_T	*dec_ref_pic_marking_buffer;	//!< stores the memory management control operation

	DEC_MB_INFO_T *mb_info;	// address of the first mb_info
	DEC_MB_INFO_T *abv_mb_info; // address of the above mb_info
	int8 *i4x4pred_mode_ptr;	//address of the first 4x4 sub-block.
	int8 *nnz_ptr; // address of the first 4x4 sub-block. which indicate the non-zero-number.
	int16 *mvd_ptr[2];	// address of the first 4x4 sub-block.

	int8 *direct_ptr; // address of the first 4x4 sub-block, which indicate the sublock is direct mode or not, 
	int32 corner_map[16];//for direct_8x8_inference_flag = 1
	DEC_BS_T *bitstrm_ptr;
	mv_prediction_subMB b8_mv_pred_func[8];	//0: for direct8x8 mode

	//weighted prediction
  	uint32 luma_log2_weight_denom;
  	uint32 chroma_log2_weight_denom;
//  	int32 wp_weight[2][MAX_REF_FRAME_NUMBER][3];  // weight in [list][index][component] order
//  	int32 wp_offset[2][MAX_REF_FRAME_NUMBER][3];  // offset in [list][index][component] order
//  	int32 wbp_weight[2][MAX_REF_FRAME_NUMBER][MAX_REF_FRAME_NUMBER][3]; //weight in [list][fw_index][bw_index][component] order
	int32 wp_round_luma;
	int32 wp_round_chroma;
	uint32 apply_weights;

	//fmo
	int32	slice_group_change_cycle;
	int32	fmo_used;
//hw	
	int32 	is_need_init_vsp_hufftab;
	uint32 	mbc_cfg_cmd;
	uint8	*ipred_top_line_buffer;
	uint32 	*vld_cabac_table_ptr;
	uint8 	*frame_bistrm_ptr;	
	
	int32       not_supported;

	uint32	VSP_used;
    uint32 vsp_state;
    uint32 ahb_state;
    uint32 dbk_state;
    uint32 mbc_state;
    uint32 vld_state;

	uint32 return_pos;
	uint32 return_pos1;	
   	uint32 return_pos2;
	
//sw	
	//mv range
	int16	mv_x_min;
	int16	mv_x_max;
	int16	mv_y_min;
	int16	mv_y_max;
	
	//for motion compensation
	int32 xpos;
	int32 ypos;

	int8	second_chroma_qp_index_offset;
	int8	scaling_list_present_flag;
	int16	rsv2;

    uint32 dequant4_buffer[6][52][16];
    uint32 dequant8_buffer[2][52][64];
    uint32 (*dequant4_coeff[6])[16];
    uint32 (*dequant8_coeff[2])[64];

	int32 dist_scale_factor[16];
	int32 *slice_nr_ptr;

	uint32 is_first_frame;

	
	uint32 *cmd_data_buf[2];
	uint32 * cmd_info_buf[2];
	uint32 cmd_buf_idx; 

	uint8 *frame_bistrm_buf[2];
}DEC_IMAGE_PARAMS_T;

#define H264DEC_FRM_STRM_BUF_SIZE (500*1024)
#define NALU_BUFFER_SIZE	(10 * 1024)

typedef struct nalu_tag
{
	uint8	*buf; //can hold a whole nalu (except start code)
	int32	len;
	
	int8	nal_unit_type;	//!< NALU_TYPE_xxxx
	int8	nal_reference_idc;	//!< NALU_PRIORITY_xxxx
	int8	frobidden_bit;	//!< should be always FALSE
	int8	resv; //for world aligned
}DEC_NALU_T;

typedef uint32 (*nal_startcode_follows_func)(DEC_IMAGE_PARAMS_T *img_ptr);
typedef int32 (*readRefFrame_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list);
typedef int32 (*decode_mvd_xy_func)(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list);
typedef void (*direct_mv_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
typedef int8 (*pred_skip_bslice_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
typedef int8 (*MC8x8_direct_func)(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, uint8 b8);
typedef void (*decode_mb_coeff_func) (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

typedef int32 (*read_mb_type_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
typedef uint32 (*readB8_typeInfo_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr);

typedef void (*BS_and_Para_interMB_func)(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
typedef int32 (*readMB_typeInfo_CABAC_func) (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

typedef void (*Intra4x4Pred)(DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
typedef void (*Intra8x8Pred)(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);	
typedef void (*Intra16x16Pred)(DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr,  uint8 * pPred, uint8 * pRec, int32 pitch);
typedef void (*IntraChromPred)(DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr,uint8 * pPredMB, uint8 * pRec, int32 pitch);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_MODE_H_
