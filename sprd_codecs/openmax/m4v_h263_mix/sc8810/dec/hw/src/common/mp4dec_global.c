/******************************************************************************
 ** File Name:    mp4dec_global_internal.c                                           *
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
int32 g_firstBsm_init;/*BSM init*/
DEC_VOP_MODE_T *g_dec_vop_mode_ptr;
DEC_FRM_BFR g_FrmYUVBfr[DEC_YUV_BUFFER_NUM];
DEC_FRM_BFR g_DispFrmYUVBfr[DISP_YUV_BUFFER_NUM];

BOOLEAN g_dec_is_first_frame;
BOOLEAN g_dec_is_stop_decode_vol;
BOOLEAN g_dec_is_changed_format;
VOP_PRED_TYPE_E g_dec_pre_vop_format;
int32 **g_dec_dc_store;
H263_PLUS_HEAD_INFO_T *g_h263_plus_head_info_ptr;
uint32 g_nFrame_dec;
uint32 g_dispFrmNum;
GetIntraBlkTCoef_sw_func g_Mp4Dec_GetIntraBlkTCoef_sw;
GetInterBlkTCoef_sw_func g_Mp4Dec_GetInterBlkTCoef_sw;
GetIntraBlkTCoef_hw_func g_Mp4Dec_GetIntraBlkTCoef_hw;
//GetInterBlkTCoef_hw_func g_Mp4Dec_GetInterBlkTCoef_hw;
DecIVOP_func g_Mp4Dec_IVOP;
DecPVOP_func g_Mp4Dec_PVOP;
DecBVOP_func g_Mp4Dec_BVOP;
MP4_MC_16x16 g_mp4dec_mc_16x16[4];
MP4_MC_8x8 g_mp4dec_mc_8x8[4];
//Dec_frm_level_sync_func g_Mp4Dec_Frm_Level_Sync;
uint8 * g_dbk_tmp_frm_ptr;

DEC_VOP_MODE_T *Mp4Dec_GetVopmode(void)
{
	return g_dec_vop_mode_ptr;
}

void Mp4Dec_SetVopmode(DEC_VOP_MODE_T *vop_mode_ptr)
{
	g_dec_vop_mode_ptr = vop_mode_ptr;
}

const int8 *Mp4Dec_GetDqTable(void)
{
	return g_dec_dq_tab;
}

H263_PLUS_HEAD_INFO_T *Mp4Dec_GetH263PlusHeadInfo(void)
{
	return g_h263_plus_head_info_ptr;
}

void Mp4Dec_SetH263PlusHeadInfo(H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr)
{
	g_h263_plus_head_info_ptr = h263_plus_head_info_ptr;
	g_h263_plus_head_info_ptr->long_vectors = 0;
}

int32 **Mp4Dec_GetDcStore(void)
{
	return g_dec_dc_store;
}

void Mp4Dec_SetDcStore(int32 **dc_store_pptr)
{
	g_dec_dc_store = dc_store_pptr;
}


#if _CMODEL_
	int32 g_stream_offset;
#endif //_CMODEL_
	
#if _DEBUG_
	void foo(void)
	{
		;
	}
#endif //_DEBUG_

#if defined(SIM_IN_WIN32)
	double  g_psnr[3] = {0, 0, 0};
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
