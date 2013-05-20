/******************************************************************************
 ** File Name:      mp4enc_global.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic interfaces of mp4 codec       *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_GLOBAL_H_
#define _MP4ENC_GLOBAL_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "mp4_basic.h"
#include "mp4enc_mode.h"
#include "mpeg4enc.h"
#include "mp4enc_me.h"
#include "mp4enc_ratecontrol.h"

#if defined(SIM_IN_WIN)
#include <stdio.h>
#endif //SIM_IN_WIN
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//for mp4 encoder
//extern int32 g_enc_last_modula_time_base;
//extern int32 g_enc_tr;
//extern BOOLEAN g_enc_is_prev_frame_encoded_success;
//extern uint32 g_enc_frame_skip_number;
//extern int32 g_re_enc_frame_number;

//extern VOL_MODE_T* g_enc_vol_mode_ptr;
//extern ENC_VOP_MODE_T* g_enc_vop_mode_ptr;

//for mpeg-4 time
//extern uint32 g_enc_first_frame;
//extern uint32 g_enc_last_frame; //encoder first and last frame number
//extern int32  g_enc_vop_time_incr;	
//extern uint32 g_enc_bits_modulo_base;
//extern int32  g_enc_modulo_base_disp;		//of the most recently displayed I/Pvop
//extern int32  g_enc_modulo_base_decd;		//of the most recently decoded I/Pvop

//for mp4 encoder
//extern uint8 *g_enc_yuv_src_frame[3]; // current source frame
//extern int32 g_is_yuv_frm_malloced;

//extern VopInfo g_enc_vop_info;
//extern uint16 g_enc_p_frame_count;

//extern uint8 *g_pEnc_output_bs_buffer;

#if 0
#ifdef _ARM_
extern uint8 *g_pEnc_input_yuv_buffer;	
extern uint8 *g_pEnc_recon_yuv_buffer;  //the pointer to the one reconstructure frame buffer.
#else
extern FILE * g_bitstream_vrf;
extern FILE * g_sourcFrmY_vrf;
extern FILE * g_sourcFrmU_vrf;
extern FILE * g_sourcFrmV_vrf;
extern FILE * g_recFrame_MBorder_vrf;


extern FILE * g_DCTCoeff_Q;
extern FILE * g_sourceMB;
extern FILE * g_IDCTResult;
extern FILE * g_referenceMB;

extern FILE * g_sourceMB_SoftModule;
extern FILE * g_refMB_softModule;
extern FILE * g_DCTCoeffQ_softModule;
extern FILE * pf_Coeff; 
extern FILE * g_pfMeaFetchRef;
#endif
#endif
/*table*/
extern const uint32 g_msk[33];
extern uint16 g_quant_pa[64];  //[32][2]
extern uint16 g_DC_scaler[94]; //[47][2];

extern MCBPC_TABLE_CODE_LEN_T g_mcbpc_intra_tab[15];
extern MCBPC_TABLE_CODE_LEN_T g_cbpy_tab[16];
extern MCBPC_TABLE_CODE_LEN_T g_mcbpc_inter_tab[29];
extern MV_TABLE_CODE_LEN_T g_mv_tab[33];

//rate control
//extern RCMode	g_stat_rc;		// Rate control mode status
//extern int		g_stat;
//extern RateCtrlPara g_rc_par;
//extern int g_stat_rc_nvop_cnt;
#if defined(SIM_IN_WIN)
extern FILE *	g_rgstat_fp;
extern FILE *	g_buf_full_pf;
extern FILE *  g_psnr_pf;
#endif

extern uint32 g_mp4_enc_huff_tbl[128];

extern MEA_FETCH_REF g_mea_fetch;

//extern uint32 g_nFrame_enc;
//extern uint32 g_HW_CMD_START;
/*
VOL_MODE_T *Mp4Enc_GetVolmode(void);
void Mp4Enc_SetVolmode(VOL_MODE_T *vop_mode_ptr);
ENC_VOP_MODE_T *Mp4Enc_GetVopmode(void);
void Mp4Enc_SetVopmode(ENC_VOP_MODE_T *vop_mode_ptr);
*/
//extern rc_single_t * g_rc_ptr;
//extern xvid_plg_data_t * g_rc_data_ptr;
//extern uint32 *g_cmd_data_ptr;
//extern uint32 *g_cmd_info_ptr;
//extern uint32 *g_cmd_data_base;
//extern uint32 *g_cmd_info_base;
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4ENC_GLOBAL_H_
