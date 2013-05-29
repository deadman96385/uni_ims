/******************************************************************************
 ** File Name:    mp4enc_global.c											  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
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

int32 g_enc_last_modula_time_base;
int32 g_enc_tr;
BOOLEAN g_enc_is_prev_frame_encoded_success;
int32 g_re_enc_frame_number;

VOL_MODE_T *g_enc_vol_mode_ptr;
ENC_VOP_MODE_T *g_enc_vop_mode_ptr;

uint8 *g_enc_yuv_src_frame[3];   // current source frame
int32 g_is_yuv_frm_malloced;

VOL_MODE_T *Mp4Enc_GetVolmode(void)
{
	return g_enc_vol_mode_ptr;
}

void Mp4Enc_SetVolmode(VOL_MODE_T *vop_mode_ptr)
{
	g_enc_vol_mode_ptr = vop_mode_ptr;
}

ENC_VOP_MODE_T *Mp4Enc_GetVopmode(void)
{
	return g_enc_vop_mode_ptr;
}

void Mp4Enc_SetVopmode(ENC_VOP_MODE_T *vop_mode_ptr)
{
	g_enc_vop_mode_ptr = vop_mode_ptr;
}

//rate control
RCMode	g_stat_rc;		// Rate control mode status
int		g_stat;
RateCtrlPara g_rc_par;

#if defined(SIM_IN_WIN)
FILE *	g_rgstat_fp;
FILE *	g_buf_full_pf;
FILE *  g_psnr_pf;
#endif

MEA_FETCH_REF g_mea_fetch;

uint32 g_nFrame_enc;

uint32 g_enc_frame_skip_number;

//for mpeg-4 time
uint32 g_enc_first_frame;
uint32 g_enc_last_frame; //encoder first and last frame number
int32 g_enc_vop_time_incr;	
uint32 g_enc_bits_modulo_base;
int32 g_enc_modulo_base_disp;		//of the most recently displayed I/Pvop
int32 g_enc_modulo_base_decd;		//of the most recently decoded I/Pvop

uint8 *g_pEnc_output_bs_buffer;  //the pointer to the output encoded bistream buffer.

uint16 g_enc_p_frame_count;

static uint32 g_ME_SearchCount;

EncPVOP_func g_EncPVOP;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

