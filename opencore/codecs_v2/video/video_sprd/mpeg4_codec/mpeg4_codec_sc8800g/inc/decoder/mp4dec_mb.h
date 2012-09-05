/******************************************************************************
 ** File Name:      mp4dec_mb.h                                            *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of macroblock  *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_MB_H_
#define _MP4DEC_MB_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_DecIntraMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr );
PUBLIC void Mp4Dec_DecInterMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecMBHeaderBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecIntraMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecInterMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC BOOLEAN Mp4Dec_IsIntraDCSwitch(DEC_MB_MODE_T *mb_mode_ptr, int32 intra_dc_vlc_thr);
PUBLIC void Mp4Dec_MBC_DBK_Cmd(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);

#ifdef _VSP_LINUX_
static  inline void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y)
#else
PUBLIC __inline void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y)
#endif
{
	uint32 cmd;

#if _CMODEL_
	#include "buffer_global.h"
	memset(vsp_fw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_bw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));		
#endif //_CMODEL_

	cmd  = (mb_y << 8) | (mb_x << 0);
	VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
}

#ifdef _VSP_LINUX_
static  inline void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr)
#else
PUBLIC __inline void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr)
#endif
{
#if _CMODEL_
// 	#include "hdbk_global.h"
	mbc_module();
#endif //_CMODEL_

	//check the mbc done flag, or time out flag; if time out, error occur then reset the vsp	
	if(READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return;
	}
	
	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");

#if _CMODEL_
	dbk_module();
#endif	
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_DEC_MB_H_
