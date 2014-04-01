/******************************************************************************
 ** File Name:      h264enc_mode.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/18/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                                                               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_MODE_H_
#define _H264ENC_MODE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264enc.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

enum profile_e
{
    PROFILE_BASELINE = 66,
    PROFILE_MAIN	= 77,
       PROFILE_ETENTED	= 88
};

/*****************************************************************
//NAL structure and functions:
*****************************************************************/
//nal
enum nal_unit_type_e
{
    NAL_UNKNOWN	= 0,
    NAL_SLICE	= 1,
    NAL_SLICE_DPA	= 2,
    NAL_SLICE_DPB	= 3,
    NAL_SLICE_DPC 	= 4,
    NAL_SLICE_IDR	= 5,	/*ref_idc != 0*/
    NAL_SEI		= 6,	/*ref_idc == 0*/
    NAL_SPS		= 7,
    NAL_PPS		= 8,
    NAL_AUD		= 9,
    /* ref_idc == 0 for 6,9,10,11,12*/
};

enum nal_priority_e
{
    NAL_PRIORITY_DISPOSABLE	= 0,
    NAL_PRIORITY_LOW	= 1,
    NAL_PRIORITY_HIGH	= 2,
    NAL_PRIORITY_HIGHEST	= 3,
};

enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,
    SLICE_TYPE_SI = 4
};

enum mb_class_e
{
    I_4x4           = 0,
//     I_8x8           = 1,
    I_16x16         = 2,
    I_PCM           = 3,

    P_L0            = 4,
    P_8x8           = 5,
    P_SKIP          = 6,
};

enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only */
//    D_L1_4x4        = 4,
//    D_L1_8x4        = 5,
//    D_L1_4x8        = 6,
//    D_L1_8x8        = 7,
//
//    D_BI_4x4        = 8,
//    D_BI_8x4        = 9,
//    D_BI_4x8        = 10,
//    D_BI_8x8        = 11,
//    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

enum intra4x4_pred_e
{
    I_PRED_4x4_V  = 0,
    I_PRED_4x4_H  = 1,
    I_PRED_4x4_DC = 2,
    I_PRED_4x4_DDL= 3,
    I_PRED_4x4_DDR= 4,
    I_PRED_4x4_VR = 5,
    I_PRED_4x4_HD = 6,
    I_PRED_4x4_VL = 7,
    I_PRED_4x4_HU = 8,
};

enum intra_chroma_pred_e
{
    I_PRED_CHROMA_DC = 0,
    I_PRED_CHROMA_H  = 1,
    I_PRED_CHROMA_V  = 2,
    I_PRED_CHROMA_P  = 3,
};

typedef struct
{
    int32	i_id;

    int32	i_profile_idc;
    int32	i_level_idc;

    int32	b_constraint_set0;
    int32	b_constraint_set1;
    int32	b_constraint_set2;

    int32	i_log2_max_frame_num;

    int32	i_poc_type;
    /*poc 0*/
    int32 	i_log2_max_poc_lsb;
    /*poc 1*/
    int32	b_delta_pic_order_always_zero;
    int32 	i_offset_for_non_ref_pic;
    int32	i_offset_for_top_to_bottom_field;
    int32	i_num_ref_frames_in_poc_cycle;
//	int32	i_offset_for_ref_frame[256];

    int32	i_num_ref_frames;
    int32	b_gaps_in_frame_num_value_allowed;
    int32	i_mb_width;
    int32	i_mb_height;
    int32	b_frame_mbs_only;
    int32 	b_mb_adaptive_frame_field;
    int32	b_direct8x8_inference;

    int32	b_crop;
    int32	b_vui;

    /* if( frmae_cropping_flag) */
    uint32  frame_crop_left_offset;                /* ue(v) */
    uint32  frame_crop_right_offset;               /* ue(v) */
    uint32  frame_crop_top_offset;                 /* ue(v) */
    uint32  frame_crop_bottom_offset;              /* ue(v) */
} ENC_SPS_T;

typedef struct
{
    int32 	i_id;
    int32	i_sps_id;

    int32	b_pic_order;
    int32 	i_num_slice_groups;

    int32	i_num_ref_idx_l0_active;
    int32	i_num_ref_idx_l1_active;

    int32	i_pic_init_qp;
    int32	i_pic_init_qs;

    int32	i_chroma_qp_index_offset;

    int32	b_deblocking_filter_control;
    int32	b_constrained_intra_pred;
    int32	b_redundant_pic_cnt;

//	int32	i_cqm_preset;
//	uint8	*scaling_list[6]; /* could be 8, but we don't allow separate Cb/Cr lists */
} ENC_PPS_T;

typedef struct
{
    int32 idc;
    int32 arg;
} REF_PIC_LIST_ORDER_T;

typedef struct
{
    ENC_SPS_T *sps;
    ENC_PPS_T *pps;

    int32 	i_type;
    int32 	i_first_mb;
    int32	i_last_mb;

    int32	i_pps_id;
    int32	i_frame_num;
    int32	i_idr_pic_id;	/*-1 if nal_type != 5*/

    int32	i_poc_lsb;
    int32	i_delta_poc_bottom;

    int32	i_delta_poc[2];
    int32 	i_redundant_pic_cnt;

    int32	b_num_ref_idx_override;
    int32	i_num_ref_idx_l0_active;
    int32	i_num_ref_idx_l1_active;

    int32	b_ref_pic_list_reordering_l0;
//	int32	b_ref_pic_list_reordering_l1;

//	REF_PIC_LIST_ORDER_T ref_pic_list_order[2][16];

    int32	i_qp;
    int32	i_qp_delta;
    int32	i_qs_delta;

    //deblocking filter
    int32	i_disable_deblocking_filter_idc;
    int32	i_alpha_c0_offset;
    int32	i_beta_offset;
} ENC_SLICE_HEADER_T;

typedef struct H264Enc_storable_pic
{
    uint8 * imgY;
    uint8 * imgUV;
//	uint8 * imgV;

    uint32 imgYAddr;
    uint32 imgUVAddr;
//	uint32 imgVAddr;

    int32	i_poc;
    int32	i_type;
//	int32	i_pts;
    int32	i_frame;	//presentation frame number
//	int32	i_frame_num;	//coded frame number
    int32	b_kept_as_ref;
    int32	addr_idx;
} H264EncStorablePic;

typedef struct enc_img_parameter_tag
{
    uint32	frame_width_in_mbs;
    uint32	frame_height_in_mbs;
    uint32	frame_size_in_mbs;

    int8	type;
    int8	qp;	//!< quant for the current frame
    int8	chroma_qp_offset;
    int8	constrained_intra_pred_flag;

    uint16	mb_x;
    uint16	mb_y;

//	uint16	i_mb_count;

    uint16      orig_width;
    uint16      orig_height;

    uint16	width;
    uint16	height;

    int32	curr_mb_nr;

//	int32	max_frame_num;

//	int		me_flag;

// 	ENC_SLICE_T	*curr_slice_ptr;	//!< pointer to current slice data struct

    int32	frame_num;		//frame_num for this frame;


    int32	i_nal_type;		//threads only
    int32	i_nal_ref_idc;	//threads only

    //we use only one SPS and PPS
    ENC_SPS_T	sps_array[1];
    ENC_SPS_T	*sps;
    ENC_PPS_T	pps_array[1];
    ENC_PPS_T	*pps;
    int32	i_idr_pic_id;
    int32	i_sps_id;	//SPS and PPS id number

    ENC_SLICE_HEADER_T sh;

    /*bitstream parameters*/
//	int32 	i_frame_reference_num;	//maximum number of reference frames
    int32	i_keyint_max;	//force an IDR keyframe at this interval
//	int32	i_keyint_min;	//scenecuts closer together than this are coded as I, not IDR
//	int32	i_scenecut_threshold;	//how aggressively to insert extra I frames

    int32	b_deblocking_filter;
    int32 	i_deblocking_filter_alphac0;	//[-6,6] -6 light filter, 6 strong
    int32 	i_deblocking_filter_beta;		//[-6,6] idem
    int32	i_chroma_qp_offset;

    H264EncStorablePic *pYUVSrcFrame;  //frame to be encoded
    H264EncStorablePic *pYUVRecFrame; //store forward reference frame
    H264EncStorablePic *pYUVRefFrame; //store backward reference frame

//	int32 i_last_idr;	//frame number of the last IDR
//	int32 i_input;	//number of input frames already accepted
//	int32 i_max_dpb;	//number of frames allocated in the decoded picture buffer
//	int32 i_max_ref0;
    int32 i_ref0;

    uint8 *pOneFrameBitstream;
    uint32 OneFrameBitstream_addr_phy;
    uint32 OneframeStreamLen;

    int32 i_level_idc;
    int32 pic_sz;
    uint32 stm_offset;
    uint32 slice_mb;
    uint32 slice_nr;
    uint32 slice_sz[5];
    uint32 slice_end;
    uint32 prev_slice_bits;
    uint32 crop_x;
    uint32 crop_y;
} ENC_IMAGE_PARAMS_T;

typedef enum
{
    INTER_MEM = 0, /*physical continuous and no-cachable, constant length */
    EXTRA_MEM,   /*physical continuous and no-cachable, variable length, need allocated according to image resolution */
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint8* v_base;  //virtual address
    uint8* p_base;  //physical address
} CODEC_BUF_T;

typedef struct  {
    long curr_buf_full;
    int32 rem_bits;
    int32 prev_pic_bits;
    int32 total_qp;
    int32 intra_period;
    int32 I_P_ratio;
    int32 gop_num_per_sec;
    int32 rem_frame_num;
    int32 last_I_frame_bits;
    int32 last_I_QP;
    int32 last_P_frame_bits;
    int32 last_P_QP;
    int32 frames;
    int32 bit_rate;
    int32 sv;
} RC_GOP_PARAS;


typedef struct  {
    long delta_target_buf;
    int32 target_bits;
    int32 remain_bits;
    int32 pred_remain_bits;
    int32 curr_pic_qp;
    uint32 total_qp;
    uint32 slice_num;
} RC_PIC_PARAS;


typedef struct  {
    int32 target_bits;
    int32 prev_target_bits;
    uint32 BU_skip_MB;
    uint32 curr_BU_QP;
    uint32 next_BU_QP;
} RC_BU_PARAS;

typedef struct
{
    uint32 enable_anti_shake;
    uint32 input_width;
    uint32 input_height;
    uint32 shift_x;
    uint32 shift_y;
} ENC_ANTI_SHAKE_T;

typedef struct tagH264EncObject
{
    uint32 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 vsp_freq_div;
    int32	error_flag;
    int32   vsp_capability;

    AVCHandle  *avcHandle;

    uint32 g_nFrame_enc;
    ENC_IMAGE_PARAMS_T *g_enc_image_ptr;
    MMEncConfig * g_h264_enc_config;
    uint8 * g_vlc_hw_ptr;

    RC_BU_PARAS rc_bu_paras;
    RC_GOP_PARAS rc_gop_paras;
    RC_PIC_PARAS rc_pic_paras;
    uint32 BU_bit_stat;
    uint32 prev_qp;

    CODEC_BUF_T mem[MAX_MEM_TYPE];

    ENC_ANTI_SHAKE_T g_anti_shake;

    uint32 b_previous_frame_failed;
    int32 yuv_format;
} H264EncObject;
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264ENC_MODE_H_

