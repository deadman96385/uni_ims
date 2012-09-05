/******************************************************************************
 ** File Name:    mp4enc_bfrctrl.c                                            *
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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int32 Mp4Enc_InitYUVBfr(ENC_VOP_MODE_T *pVop_mode)
{
	uint32 size;
	uint32 uv_offset;
	uint8 *bfr_gap_ptr;

	uv_offset = ((VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "readout uv_offset")>>4)<<2);

	/*backward reference frame and forward reference frame after world-aligned*/
	size = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight);

	pVop_mode->pYUVRecFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size);
    if( uv_offset!= size)
    {
	    bfr_gap_ptr = (uint8 *)Mp4Enc_ExtraMemAlloc(uv_offset - size);
    }
	if (!pVop_mode->uv_interleaved)
	{
		pVop_mode->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
		pVop_mode->pYUVRecFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
	}else
	{
		pVop_mode->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>1);
		pVop_mode->pYUVRecFrame->imgV = NULL;
	}

	pVop_mode->pYUVRefFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size);
    if( uv_offset!= size)
    {
	    bfr_gap_ptr = (uint8 *)Mp4Enc_ExtraMemAlloc(uv_offset - size);
    }
	if (!pVop_mode->uv_interleaved)
	{
		pVop_mode->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
		pVop_mode->pYUVRefFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
	}else
	{
		pVop_mode->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>1);
		pVop_mode->pYUVRefFrame->imgV = NULL;
	}
#if defined(_SIMULATION_) 
	pVop_mode->pYUVSrcFrame->imgUAddr = (uint32)pVop_mode->pYUVSrcFrame->imgU >> 8;
	pVop_mode->pYUVSrcFrame->imgVAddr = (uint32)pVop_mode->pYUVSrcFrame->imgV >> 8;
#endif

#ifdef _VSP_LINUX_	
	pVop_mode->pYUVRecFrame->imgYAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRecFrame->imgY) >> 8;
	pVop_mode->pYUVRecFrame->imgUAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRecFrame->imgU) >> 8;
	pVop_mode->pYUVRecFrame->imgVAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRecFrame->imgV) >> 8;

	pVop_mode->pYUVRefFrame->imgYAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRefFrame->imgY) >> 8;
	pVop_mode->pYUVRefFrame->imgUAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRefFrame->imgU) >> 8;
	pVop_mode->pYUVRefFrame->imgVAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pYUVRefFrame->imgV) >> 8;
#else
	pVop_mode->pYUVRecFrame->imgYAddr = (uint32)pVop_mode->pYUVRecFrame->imgY >> 8;
	pVop_mode->pYUVRecFrame->imgUAddr = (uint32)pVop_mode->pYUVRecFrame->imgU >> 8;
	pVop_mode->pYUVRecFrame->imgVAddr = (uint32)pVop_mode->pYUVRecFrame->imgV >> 8;

	pVop_mode->pYUVRefFrame->imgYAddr = (uint32)pVop_mode->pYUVRefFrame->imgY >> 8;
	pVop_mode->pYUVRefFrame->imgUAddr = (uint32)pVop_mode->pYUVRefFrame->imgU >> 8;
	pVop_mode->pYUVRefFrame->imgVAddr = (uint32)pVop_mode->pYUVRefFrame->imgV >> 8;
#endif

#if _CMODEL_
	pVop_mode->pYUVRecFrame->imgYAddr = REC_FRAME0_Y_ADDR >> 8;
	pVop_mode->pYUVRefFrame->imgYAddr = REC_FRAME1_Y_ADDR >> 8;
#endif

	return TRUE;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
