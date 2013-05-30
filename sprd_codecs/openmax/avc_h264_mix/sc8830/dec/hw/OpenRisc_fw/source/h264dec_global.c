/******************************************************************************
 ** File Name:    h264dec_global.c                                            *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
DEC_STORABLE_PICTURE_T g_rec_buf;
DEC_SPS_T	*g_sps_ptr;
DEC_PPS_T	*g_pps_ptr;
DEC_SPS_T	*g_active_sps_ptr;
DEC_PPS_T	*g_active_pps_ptr;
DEC_SPS_T	*g_sps_array_ptr;
DEC_PPS_T	*g_pps_array_ptr;
 uint32 direct_mb_info_addr[MAX_REF_FRAME_NUMBER+1];

#if _MVC_
  #define MAXSPS  32
  subset_seq_parameter_set_rbsp_t *g_active_subset_sps;
  //int svc_extension_flag;
  subset_seq_parameter_set_rbsp_t g_SubsetSeqParSet[MAXSPS];
  int8 last_pic_width_in_mbs_minus1;
  int8 last_pic_height_in_map_units_minus1;
  int8 last_max_dec_frame_buffering;
  uint8 last_profile_idc;//weihu
#endif

DEC_IMAGE_PARAMS_T		*g_image_ptr;
DEC_SLICE_T				*g_curr_slice_ptr;
DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;
DEC_MB_CACHE_T			*g_mb_cache_ptr;
#if _MVC_
DEC_DECODED_PICTURE_BUFFER_T	 *g_dpb_layer[2];
#else
DEC_DECODED_PICTURE_BUFFER_T	*g_dpb_ptr;
#endif

DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
DEC_STORABLE_PICTURE_T	*g_list0[MAX_REF_FRAME_NUMBER+MAX_REF_FRAME_NUMBER];//weihu//+1
DEC_STORABLE_PICTURE_T	*g_list1[MAX_REF_FRAME_NUMBER+1];
DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;
DEC_NALU_T	*g_nalu_ptr;

int32 g_wp_weight[2][MAX_REF_FRAME_NUMBER][3];  // weight in [list][index][component] order
int32 g_wp_offset[2][MAX_REF_FRAME_NUMBER][3];  // offset in [list][index][component] order
int32 g_wbp_weight[2][MAX_REF_FRAME_NUMBER][MAX_REF_FRAME_NUMBER][3]; //weight in [list][fw_index][bw_index][component] order
int32 g_list1_map_list0[MAX_REF_FRAME_NUMBER];//weihu0730
int8  g_list0_map_addr[32];//weihu
int8  g_list1_map_addr[16];//weihu

int8	*g_MbToSliceGroupMap;
int32	g_old_pps_id;
int32	g_list_size[2];
int32	g_dec_ref_pic_marking_buffer_size;
int32	g_ready_to_decode_slice;
int32	g_searching_IDR_pic;
int32	g_pre_mb_is_intra4;	
int32	g_nFrame_dec_h264;
int32 	g_dispFrmNum;
int32 g_firstBsm_init_h264;/*BSM init*/
volatile int32 	g_stream_offset;
volatile int32	g_slice_datalen;//weihu
//int8	load_vld_table_en=1;//weihu
void  *display_array_BH[16];
unsigned char *display_array_Y[16];
unsigned char *display_array_UV[16];
int32	display_array_len;
int   frame_dec_finish;

uint8 g_lengthSizeMinusOne;

int32 last_dquant;

char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
char weightscale8x8[2][8][8];////2 2*2+2=6*64=384B

char weightscale4x4_intra_default[16] = {
		6,13,20,28,
		13,20,28,32,
		20,28,32,37,
		28,32,37,42
};

char weightscale4x4_inter_default[16] = {
		10,14,20,24,
		14,20,24,27,
		20,24,27,30,
		24,27,30,34
};

char weightscale8x8_intra_default[64] = {
		6,10,13,16,18,23,25,27,
		10,11,16,18,23,25,27,29,
		13,16,18,23,25,27,29,31,
		16,18,23,25,27,29,31,33,
		18,23,25,27,29,31,33,36,
		23,25,27,29,31,33,36,38,
		25,27,29,31,33,36,38,40,
		27,29,31,33,36,38,40,42
};

char weightscale8x8_inter_default[64] = {
		9,13,15,17,19,21,22,24,
		13,13,17,19,21,22,24,25,
		15,17,19,21,22,24,25,27,
		17,19,21,22,24,25,27,28,
		19,21,22,24,25,27,28,30,
		21,22,24,25,27,28,30,32,
		22,24,25,27,28,30,32,33,
		24,25,27,28,30,32,33,35
};

const uint8 *g_totZero_Chroma_DC [3];
const uint8 * g_run_zeroLeft [6];

nal_startcode_follows_func nal_startcode_follows;
readRefFrame_func readRefFrame;
readMVD_xy_func readMVD_xy;
direct_mv_func direct_mv;
pred_skip_bslice_func pred_skip_bslice;
MC8x8_direct_func MC8x8_direct;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
