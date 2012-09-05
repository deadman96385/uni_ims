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
	int16	resv; //for word aligned

	//fmo
	uint16	run_length_minus1[MAX_NUM_SLICE_GROUPS_MINUS1]; //ue(v)
	uint32	top_left[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
	uint32	bottom_right[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
	int32	slice_group_change_direction_flag; //u(1)
	uint32	slice_group_change_rate_minus1;	//ue(v)
	uint8	*slice_group_id;	//complete MBAmap u(v) for slice group map type 6
	uint32	num_slice_group_map_units_minus1; //ue(v)
}DEC_PPS_T;

#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE	256

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
	int8	direct_8x8_reference_flag;
	int8	frame_cropping_flag;
	
	int8	vui_parameters_present_flag;
	int8	resv1; //for word aligned
	int16	resv2;

	uint32	log2_max_pic_order_cnt_lsb_minus4;		//ue(v)
	uint32	num_ref_frames_in_pic_order_cnt_cycle;	//ue(v)
	int32	offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];	//se(v)
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
	int8	ref_pic[4];

	int8	mb_type;
	int8	is_skipped;
	int8	is_intra;
	int8	qp;

	int8	qp_c;
	int8	slice_nr;
	int16	resv1;	//for word align

	int8	LFDisableIdc;
	int8	LFAlphaC0Offset;
	int8	LFBetaOffset;
	int8	resv2;	//for word align

	int8	ref_idx_cache[6*5+2];	//used for mv predict and deblocking
	int16	mv_cache[6*5*2];
	int8	nnz_ref[6*8];
	int8	i4x4pred_mode_ref[6*5+2];  ///2 for word aligned
}DEC_MB_INFO_T;

typedef void (*mv_prediction_subMB)(DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8);
typedef struct mb_data_cache_tag
{
	int8	i16mode;
	int8	c_ipred_mode;
	int8	all_zero_ref;
	int8	cbp;

	int8	b8mode[4];

	int8	mb_avail_a;
	int8	mb_avail_b;
	int8	mb_avail_c;
	int8	mb_avail_d;

	uint8	cbp_uv;
	int8	top_edge_filter_flag;
	int8	left_edge_filter_flag;
	int8	resv; //for word-align
	
	int8	ref_pic_cache[6*5+2];

	uint32	BS[8];

	mv_prediction_subMB b8_mv_pred_func[4];

}DEC_MB_CACHE_T;

typedef struct 
{
	int8	ei_flag;			//!< 0 if the partArr[0] contains valid information
	int8	qp;
	int8	slice_qp_delta;
	int8	picture_type;		//!< picture type

	int8	next_header;
	int8	LFDisableIdc;		//!< Disable loop filter on slice
	int8	LFAlphaC0Offset;	//!< Alpha and C0 offset for filtering slice
	int8	LFBetaOffset;		//!< Beta offset for filtering slice

	uint16	start_mb_nr;		//!< MUST be set by NAL even in case of ei_flag == 1
	uint8	pic_parameter_set_id;	//!< the ID of the picture parameter set the slice is reffering to
	int8	ref_pic_list_reordering_flag_l0;

	int32	remapping_of_pic_nums_idc_l0[MAX_REF_FRAME_NUMBER+1];
	int32	abs_diff_pic_num_minus1_l0[MAX_REF_FRAME_NUMBER+1];
	int32	long_term_pic_idx_l0[MAX_REF_FRAME_NUMBER+1];
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

typedef struct img_parameter_tag
{
	int8	num_ref_idx_l0_active;	//!< number of forward reference
	uint8	frame_width_in_mbs;
	uint8	frame_height_in_mbs;
	int8	type;

	int8	qp;	//!< quant for the current frame
	int8	chroma_qp_offset;
	int8	constrained_intra_pred_flag;
	int8	disposable_flag;

	uint16	mb_x;
	uint16	mb_y;

	int32	cod_counter;
	
	int8	redundant_pic_cnt;
	int8	error_flag;
	uint16	num_dec_mb;

	uint16	width;
	uint16	height;

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

	uint32	pic_order_cnt_lsb;	// for poc mode 0
	int32	delta_pic_order_cnt_bottom;
	int32	delta_pic_order_cnt[3];	//for poc mode 1

	int32	pre_frame_num;	//!< store the frame_num in the last decoded slice, for detecting gap in frame_num
	int32	frame_num;		//frame_num for this frame;

	int32	is_new_pic;

	DEC_DEC_REF_PIC_MARKING_T	*dec_ref_pic_marking_buffer;	//!< stores the memory management control operation

	int32	last_has_mmco_5;	

	DEC_MB_INFO_T *mb_info;

	uint8	*ipred_top_line_buffer;

	//fmo
	int32	slice_group_change_cycle;

	int32	fmo_used;
	int32       not_supported;

    uint32 vsp_state;
    uint32 ahb_state;
    uint32 dbk_state;
    uint32 mbc_state;
    uint32 vld_state;
    uint32 return_pos;
    uint32 return_pos2;
	uint32	size_decode_flag;
		
}DEC_IMAGE_PARAMS_T;

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

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_MODE_H_