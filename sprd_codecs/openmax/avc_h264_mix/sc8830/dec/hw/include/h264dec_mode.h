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
#include "h264dec.h"
#include "h264dec_basic.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#ifndef WIN32
#define WORD_ALIGN //__align(32) //for or debug
#else
#define WORD_ALIGN
#endif

#define MAX_NUM_SLICE_GROUPS_MINUS1	8

#define DELAYED_PIC_REF 4

//! struct to characterize the state of the arithmetic coding engine
typedef struct
{
    uint32   Dlow, Drange;
    uint32   Dvalue;
//  uint32   Dbuffer;
//  int32     Dbits_to_go;
//  uint8     *Dcodestrm;
//  int32     *Dcodestrm_len;
} DecodingEnvironment;

typedef struct
{
    int8	valid;									//indicates the parameter set is valid
    uint8	pic_parameter_set_id;					//ue(v)
    uint8	seq_parameter_set_id;					//ue(v)
    uint8	pic_order_present_flag;					//u(1)

    uint8	num_slice_groups_minus1;				//ue(v)
    uint8	slice_group_map_type;					//ue(v)
    uint8	num_ref_idx_l0_active_minus1;			//ue(v)
    uint8	num_ref_idx_l1_active_minus1;			//uv(v)

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
    int8	pic_scaling_matrix_present_flag;		//u(1)

    int8	pic_scaling_list_present_flag[8];

    int8	ScalingList4x4[6][16];
    int8	ScalingList8x8[2][64];

    int8	UseDefaultScalingMatrix4x4Flag[16];
    int8	UseDefaultScalingMatrix8x8Flag[64];

    //fmo
    uint16	run_length_minus1[MAX_NUM_SLICE_GROUPS_MINUS1]; //ue(v)
    uint32	top_left[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
    uint32	bottom_right[MAX_NUM_SLICE_GROUPS_MINUS1];	//ue(v)
    int32	slice_group_change_direction_flag; //u(1)
    uint32	slice_group_change_rate_minus1;	//ue(v)
    uint	*slice_group_id;	//complete MBAmap u(v) for slice group map type 6 //weihu//uint8
    uint32	num_slice_group_map_units_minus1; //ue(v)
} DEC_PPS_T;

#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE	256
//weihu
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
} DEC_HRD_PARAM_T;

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
} DEC_VUI_T;

typedef struct
{
    int8	valid;									//indicates the parameter set is valid
    uint8	profile_idc;							//u(8)
    int8	constrained_set0_flag;					//u(1)
    int8	constrained_set1_flag;					//u(1)

    int8	constrained_set2_flag;					//u(1)
    int8	constrained_set3_flag;					//u(1)
#if _MVC_
    int8	constrained_set4_flag;					//u(1)
    int8	constrained_set5_flag;					//u(1)

    int8	max_dec_frame_buffering;
#endif
    uint8	level_idc;								//u(8)
    uint8	seq_parameter_set_id;					//ue(v)
    uint8	log2_max_frame_num_minus4;				//ue(v)

    uint8	pic_order_cnt_type;						//
    int8	delta_pic_order_always_zero_flag;		//u(1)
    uint8	num_ref_frames;							//ue(v)
    int8	gaps_in_frame_num_value_allowed_flag;	//u(1)

    int 	offset_for_non_ref_pic;					//se(v) //weihu

    int	    offset_for_top_to_bottom_field;			//se(v) //weihu




    uint8	pic_height_in_map_units_minus1;			//ue(v)
    int8	frame_mbs_only_flag;
    int8	mb_adaptive_frame_field_flag;           // u(1)
    int8	direct_8x8_reference_flag;

    uint8	pic_width_in_mbs_minus1;				//ue(v)
    int8	frame_cropping_flag;
    int8	vui_parameters_present_flag;
    int8	seq_scaling_matrix_present_flag; //for word aligned
    //int8	resv2;

    uint8	chroma_format_idc;
    uint8	bit_depth_luma_minus8;
    uint8	bit_depth_chroma_minus8;
    int8	qpprime_y_zero_transform_bypass_flag;

    int8	seq_scaling_list_present_flag[8];

    int8	ScalingList4x4[6][16];
    int8	ScalingList8x8[2][64];

    int8	UseDefaultScalingMatrix4x4Flag[16];
    int8	UseDefaultScalingMatrix8x8Flag[64];

    uint32	log2_max_pic_order_cnt_lsb_minus4;		//ue(v)
    uint32	num_ref_frames_in_pic_order_cnt_cycle;	//ue(v)
    int32	offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];	//se(v)
    DEC_VUI_T *vui_seq_parameters;

    /* if( frmae_cropping_flag) */
    uint32  frame_crop_left_offset;                /* ue(v) */
    uint32  frame_crop_right_offset;               /* ue(v) */
    uint32  frame_crop_top_offset;                 /* ue(v) */
    uint32  frame_crop_bottom_offset;              /* ue(v) */

} DEC_SPS_T;

#if _MVC_
typedef struct mvcvui_tag
{
    int num_ops_minus1;
    char *temporal_id;
    int *num_target_output_views_minus1;
    int **view_id;
    char *timing_info_present_flag;
    int *num_units_in_tick;
    int *time_scale;
    char *fixed_frame_rate_flag;
    char *nal_hrd_parameters_present_flag;
    char *vcl_hrd_parameters_present_flag;
    char *low_delay_hrd_flag;
    char *pic_struct_present_flag;

    //hrd parameters;
    char cpb_cnt_minus1;
    char bit_rate_scale;
    char cpb_size_scale;
    char	resv;//weihu
    int bit_rate_value_minus1[32];
    int cpb_size_value_minus1[32];
    char cbr_flag[32];
    char initial_cpb_removal_delay_length_minus1;
    char cpb_removal_delay_length_minus1;
    char dpb_output_delay_length_minus1;
    char time_offset_length;
} MVCVUI_t;

typedef struct
{
    DEC_SPS_T sps;

    unsigned int bit_equal_to_one;
    int32 num_views_minus1;
    int32 *view_id;
    int32 *num_anchor_refs_l0;
    int32 **anchor_ref_l0;
    int32 *num_anchor_refs_l1;
    int32 **anchor_ref_l1;

    int32 *num_non_anchor_refs_l0;
    int32 **non_anchor_ref_l0;
    int32 *num_non_anchor_refs_l1;
    int32 **non_anchor_ref_l1;

    int32 num_level_values_signalled_minus1;
    int32 *level_idc;
    int32 *num_applicable_ops_minus1;
    int32 **applicable_op_temporal_id;
    int32 **applicable_op_num_target_views_minus1;
    int32 ***applicable_op_target_view_id;
    int32 **applicable_op_num_views_minus1;

    uint8 mvc_vui_parameters_present_flag;
    int8   Valid;                  // indicates the parameter set is valid
    char	resv;//weihu
    char	resv1;//weihu
    MVCVUI_t  MVCVUIParams;
} subset_seq_parameter_set_rbsp_t;

#endif

//! struct for context management
#if 0
typedef struct
{
    unsigned short state;         // index into state-table CP
    unsigned char  MPS;           // Least Probable Symbol 0/1 CP
    unsigned char rsv; //for word-aligned
} BiContextType;
#else
typedef 	uint8 BiContextType;	//(state<<1)|MPS
#endif

/**********************************************************************
 * C O N T E X T S   F O R   T M L   S Y N T A X   E L E M E N T S
 **********************************************************************
 */

#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  4//9
#define NUM_MV_RES_CTX   8//10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 3//4
#ifdef LUMA_8x8_CABAC
#define NUM_TRANSFORM_SIZE_CTX 3
#endif

typedef struct
{
    BiContextType mb_type_contexts [3][NUM_MB_TYPE_CTX];//4
    BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
    BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX];
    BiContextType ref_no_contexts  [1][NUM_REF_NO_CTX];//2
    BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
    BiContextType mb_aff_contexts  [NUM_MB_AFF_CTX];
#ifdef LUMA_8x8_CABAC
    BiContextType transform_size_contexts [NUM_TRANSFORM_SIZE_CTX];
#endif
} MotionInfoContexts;

#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5

typedef struct
{
    BiContextType  ipr_contexts [NUM_IPR_CTX];
    BiContextType  cipr_contexts[NUM_CIPR_CTX];
    BiContextType  cbp_contexts [3][NUM_CBP_CTX];
    BiContextType  bcbp_contexts[5][NUM_BCBP_CTX];//NUM_BLOCK_TYPES 5,12
    BiContextType  map_contexts [6][NUM_MAP_CTX];//NUM_BLOCK_TYPES 6,30
    BiContextType  last_contexts[6][NUM_LAST_CTX];//NUM_BLOCK_TYPES 6,30
    BiContextType  one_contexts [6][NUM_ONE_CTX];//NUM_BLOCK_TYPES 6,10
    BiContextType  abs_contexts [6][NUM_ABS_CTX];//NUM_BLOCK_TYPES 6,10
//BiContextType  fld_map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
//BiContextType  fld_last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
} TextureInfoContexts;

#if _MVC_
typedef struct nalunitHeadermvcext_tag
{
    int8 non_idr_flag;
    int8 priority_id;
    int8 view_id;
    int8 temporal_id;
    int8 anchor_pic_flag;
    int8 inter_view_flag;
    int8 reserved_one_bit;
    int8 iPrefixNALU;
} NALUnitHeaderMVCExt_t;
#endif

typedef struct dec_ref_pic_marking_tag
{
    int32	memory_management_control_operation;
    int32	difference_of_pic_nums_minus1;
    int32	long_term_pic_num;
    int32	long_term_frame_idx;
    int32	max_long_term_frame_idx_plus1;
} DEC_DEC_REF_PIC_MARKING_T;

typedef struct storable_picture
{
    int32	poc;
    int32	frame_poc;
    int32	frame_num;
    int32	pic_num;
    int32	long_term_pic_num;
    int32	long_term_frame_idx;

    int8	is_long_term;
    int8	used_for_reference;
    int8	chroma_vector_adjustment;
    int8	coded_frame;

    int8	is_output;
    int8	slice_type;
    int8	idr_flag;
    int8	no_output_of_prior_pics_flag;

    int32	non_existing;

    int8	adaptive_ref_pic_buffering_flag;
#if _MVC_
    int8    view_id;
    int8    inter_view_flag;
    int8    anchor_pic_flag;
#endif

    DEC_DEC_REF_PIC_MARKING_T *dec_ref_pic_marking_buffer; //<! stores the memory management control operations

    uint8	*imgY;		//should be 64 word alignment
    uint8	*imgU;
    uint8	*imgV;
    int32   *direct_mb_info;
    uint32	imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
    uint32	imgUAddr;	//imgUAddr = ((uint32)imgU>>8)
    uint32	imgVAddr;	//imgVAddr = ((uint32)imgV>>8)
    uint32  direct_mb_info_Addr;

    int32   DPB_addr_index;//weihu

    void 	*pBufferHeader;
    int32   mPicId;  // Which output picture is for which input buffer?
} DEC_STORABLE_PICTURE_T;

typedef struct frame_store_tag
{
    int8	is_reference;
    int8	is_long_term;
    int8	is_short_term;
    int8	disp_status;

    int32	pic_num;
    int32	frame_num;
    int32	frame_num_wrap;
    int32	long_term_frame_idx;
    int32	poc;

#if _MVC_
    int8       view_id;
    int8       inter_view_flag[2];
    int8       anchor_pic_flag[2];
    int8       layer_id;
#endif
    DEC_STORABLE_PICTURE_T *frame;
} DEC_FRAME_STORE_T;

#define MAX_DELAYED_PIC_NUM	10 //5


//decoded picture buffer
typedef struct decoded_picture_buffer
{
    DEC_FRAME_STORE_T	**fs;
    int32				size;
    int32				used_size;

    //ref buffer
    DEC_FRAME_STORE_T	**fs_ref;
    int32				ref_frames_in_buffer;

    DEC_FRAME_STORE_T	**fs_ltref;
    int32				ltref_frames_in_buffer;

    int32				max_long_term_pic_idx;
    int32				num_ref_frames;
#if _MVC_
    DEC_FRAME_STORE_T	**fs_ilref; // inter-layer reference (for multi-layered codecs)
    int8				last_output_view_id;
    int8				init_done;
    int8                resv;
    int8                resv1;
#endif

    DEC_STORABLE_PICTURE_T	*delayed_pic[MAX_DELAYED_PIC_NUM];
    DEC_STORABLE_PICTURE_T *delayed_pic_ptr;
    int32				delayed_pic_num;
} DEC_DECODED_PICTURE_BUFFER_T;

typedef struct
{
    int8	ei_flag;			//!< 0 if the partArr[0] contains valid information
    int8	qp;
    int8	slice_qp_delta;
    int8	picture_type;		//!< picture type

#if _MVC_
    int8	svc_extension_flag;
    int8	layer_id;
    int8	inter_view_flag;
    int8	anchor_pic_flag;

    DEC_DECODED_PICTURE_BUFFER_T *p_Dpb;
    NALUnitHeaderMVCExt_t	NaluHeaderMVCExt;

    int	abs_diff_view_idx_minus1[2][MAX_REF_FRAME_NUMBER+1];

    int listinterviewidx0;
    int listinterviewidx1;
    struct frame_store_tag **fs_listinterview0;
    struct frame_store_tag **fs_listinterview1;
#endif

    int8	next_header;
    int8	LFDisableIdc;		//!< Disable loop filter on slice
    int8	LFAlphaC0Offset;	//!< Alpha and C0 offset for filtering slice
    int8	LFBetaOffset;		//!< Beta offset for filtering slice

    uint	start_mb_nr;		//!< MUST be set by NAL even in case of ei_flag == 1 //weihu //uint16

    uint8	pic_parameter_set_id;	//!< the ID of the picture parameter set the slice is reffering to
    int8	ref_pic_list_reordering_flag_l0;
    int8	ref_pic_list_reordering_flag_l1;
    int8	view_id;

    int32	remapping_of_pic_nums_idc_l0[MAX_REF_FRAME_NUMBER+1];
    int32	abs_diff_pic_num_minus1_l0[MAX_REF_FRAME_NUMBER+1];
    int32	long_term_pic_idx_l0[MAX_REF_FRAME_NUMBER+1];

    int32	remapping_of_pic_nums_idc_l1[MAX_REF_FRAME_NUMBER+1];
    int32	abs_diff_pic_num_minus1_l1[MAX_REF_FRAME_NUMBER+1];
    int32	long_term_pic_idx_l1[MAX_REF_FRAME_NUMBER+1];

    MotionInfoContexts  *mot_ctx;      //!< pointer to struct of context models for use in CABAC
    TextureInfoContexts *tex_ctx;      //!< pointer to struct of context models for use in CABAC
} DEC_SLICE_T;

typedef struct old_slice_par_tag
{
    uint8	field_pic_flag;
    uint8	bottom_field_flag;
    int8	nal_ref_idc;
    int8	idr_flag;

    uint16	idr_pic_id;
    uint8	pps_id;
    uint8	resv;//for word-align

#if _MVC_
    int8	view_id;
    int8	inter_view_flag;
    int8	anchor_pic_flag;
    uint8	resv1;//for word-align
#endif

    int32	frame_num;
    uint32	pic_order_cnt_lsb;
    int32	delta_pic_order_cnt_bottom;
    int32	delta_pic_order_cnt[2];
} DEC_OLD_SLICE_PARAMS_T;

typedef struct img_parameter_tag
{
    uint8	num_ref_idx_l0_active;	//!< number of forward reference
    uint8	num_ref_idx_l1_active;	//!< number of backward reference
    uint8	frame_width_in_mbs;
    uint8	frame_height_in_mbs;

    int8	type;
    int8	direct_type;                            //!< 1 for Spatial Direct, 0 for Temporal
    int16	model_number;

    int8	qp;	//!< quant for the current frame
    int8	chroma_qp_offset;
    int8	constrained_intra_pred_flag;
    int8	disposable_flag;

    int8	redundant_pic_cnt;
    int8 rsv0;
    int16 rsv1;


    int32	curr_mb_nr;
    int32	slice_nr;

    uint16	frame_size_in_mbs;
    uint16	old_frame_size_in_mbs;

    uint32	pic_height_in_map_units;

    //weighted prediction
    uint32 luma_log2_weight_denom;
    uint32 chroma_log2_weight_denom;
    int32 wp_round_luma;
    int32 wp_round_chroma;
    uint32 apply_weights;

    int8	idr_flag;
    int8	nal_reference_idc;	//!< nal_reference_idc from NAL unit
    uint16	idr_pic_id;

    int32	max_frame_num;

    DEC_SLICE_T	*curr_slice_ptr;	//!< pointer to current slice data struct

    int8	no_output_of_prior_pics_flag;
    int8	long_term_reference_flag;
    int8	adaptive_ref_pic_buffering_flag;
    uint8	profile_idc;

    // POC200301: from unsigned int to int
    int32 toppoc;      //poc for this top field // POC200301
    int32 bottompoc;   //poc of bottom field of frame
    int32 framepoc;    //poc of this frame // POC200301
    int32 frame_num;   //frame_num for this frame	//++ 帧号：参考帧帧号按解码顺序递增，非参考帧与后续参考帧使用同一帧号
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

    int32	is_new_pic;
    uint32   DPB_addr_index;//weihu//当前图像在帧缓存中的地址索引

    DEC_DEC_REF_PIC_MARKING_T	*dec_ref_pic_marking_buffer;	//!< stores the memory management control operation

    int32	last_has_mmco_5;
    int32 	last_pic_bottom_field;

    uint8	*ipred_top_line_buffer;

    //fmo
    int32	slice_group_change_cycle;
    int32	fmo_used;
} DEC_IMAGE_PARAMS_T;

typedef struct nalu_tag
{
    int32	len;

    int8	nal_unit_type;	//!< NALU_TYPE_xxxx
    int8	nal_reference_idc;	//!< NALU_PRIORITY_xxxx
    int8	frobidden_bit;	//!< should be always FALSE
    int8	resv; //for world aligned
} DEC_NALU_T;

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint8* v_base;  //virtual address
    uint32 p_base;  //physical address
} CODEC_BUF_T;

typedef struct H264DecObject_tag
{
    uint32 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 ddr_bandwidth_req_cnt;
    uint32 vsp_freq_div;
    int32	error_flag;

    AVCHandle  *avcHandle;

    DEC_STORABLE_PICTURE_T g_rec_buf;

    CODEC_BUF_T mem[MAX_MEM_TYPE];

    uint16	width;
    uint16	height;

    DEC_SPS_T	*g_sps_ptr;
    DEC_PPS_T	*g_pps_ptr;
    DEC_SPS_T	*g_active_sps_ptr;
    DEC_PPS_T	*g_active_pps_ptr;
    DEC_SPS_T	*g_sps_array_ptr;
    DEC_PPS_T	*g_pps_array_ptr;
    uint32 direct_mb_info_addr[MAX_REF_FRAME_NUMBER+1];

    DEC_IMAGE_PARAMS_T		*g_image_ptr;
    DEC_SLICE_T 			*g_curr_slice_ptr;
    DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;

    uint32					*g_cavlc_tbl_ptr;
    int32 is_need_init_vsp_hufftab;

    DEC_DECODED_PICTURE_BUFFER_T	 *g_dpb_layer[2];

    DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
    DEC_STORABLE_PICTURE_T	*g_list0[MAX_REF_FRAME_NUMBER+MAX_REF_FRAME_NUMBER];//weihu//+1
    DEC_STORABLE_PICTURE_T	*g_list1[MAX_REF_FRAME_NUMBER+1];
    DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
    DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;
    DEC_NALU_T	*g_nalu_ptr;

    int32 g_wp_weight[2][MAX_REF_FRAME_NUMBER][3];	// weight in [list][index][component] order
    int32 g_wp_offset[2][MAX_REF_FRAME_NUMBER][3];	// offset in [list][index][component] order
    int32 g_list1_map_list0[MAX_REF_FRAME_NUMBER];//weihu0730
    int8  g_list0_map_addr[32];//weihu
    int8  g_list1_map_addr[16];//weihu

    int8	*g_MbToSliceGroupMap;
    int32	g_old_pps_id;
    int32	g_list_size[2];
    int32	g_dec_ref_pic_marking_buffer_size;
    int32	g_ready_to_decode_slice;
    int32	g_searching_IDR_pic;
    int32	g_nFrame_dec_h264;
    int32	g_dispFrmNum;
    int32	g_stream_offset;
    int32	g_slice_datalen;//weihu
    void  *display_array_BH[16];
    unsigned char *display_array_Y[16];
    unsigned char *display_array_UV[16];
    uint32 display_array_mPicId[16];
    int32	display_array_len;
    int   frame_dec_finish;

    char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
    char weightscale8x8[2][8][8];////2 2*2+2=6*64=384B

#if _MVC_
    subset_seq_parameter_set_rbsp_t *g_active_subset_sps;
    //int svc_extension_flag;
    subset_seq_parameter_set_rbsp_t g_SubsetSeqParSet[MAX_SPS];
    int8 last_pic_width_in_mbs_minus1;
    int8 last_pic_height_in_map_units_minus1;
    int8 last_max_dec_frame_buffering;
    uint8 last_profile_idc;//weihu

    uint32 DecodeAllLayers;		  //for MVC
#endif

    int ref_list_buf[33];
    int slice_info[50];
    uint8 * pStream;
    int memory_error;
} H264DecObject;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_MODE_H_
