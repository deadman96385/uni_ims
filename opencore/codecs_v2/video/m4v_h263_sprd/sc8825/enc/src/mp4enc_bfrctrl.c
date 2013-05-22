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
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
PUBLIC int32 Mp4Enc_InitYUVBfr(MP4EncHandle* mp4Handle)
{
	Mp4EncObject*vd = (Mp4EncObject *) mp4Handle->videoEncoderData;
	ENC_VOP_MODE_T *vop_mode_ptr = vd->g_enc_vop_mode_ptr;
	uint32 size;
//	uint32 uv_offset;
// 	uint8 *bfr_gap_ptr;

	//uv_offset = ((VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "readout uv_offset")>>4)<<2);

	/*backward reference frame and forward reference frame after world-aligned*/
	size = (vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight);

	vop_mode_ptr->pYUVRecFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size);
// 	bfr_gap_ptr = (uint8 *)Mp4Enc_ExtraMemAlloc(uv_offset - size);
	if (!vop_mode_ptr->uv_interleaved)
	{
		vop_mode_ptr->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>2);
		vop_mode_ptr->pYUVRecFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>2);
	}else
	{
		vop_mode_ptr->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>1);
		vop_mode_ptr->pYUVRecFrame->imgV = NULL;
	}

	vop_mode_ptr->pYUVRefFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size);
// 	bfr_gap_ptr = (uint8 *)Mp4Enc_ExtraMemAlloc(uv_offset - size);
	if (!vop_mode_ptr->uv_interleaved)
	{
		vop_mode_ptr->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>2);
		vop_mode_ptr->pYUVRefFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>2);
	}else
	{
		vop_mode_ptr->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(mp4Handle,size>>1);
		vop_mode_ptr->pYUVRefFrame->imgV = NULL;
	}
#if defined(_SIMULATION_) 
	vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgU ;//>> 8;
	vop_mode_ptr->pYUVSrcFrame->imgVAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgV;// >> 8;
#endif


#ifdef _VSP_LINUX_	
	vop_mode_ptr->pYUVRecFrame->imgYAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRecFrame->imgY) ;//>> 8;
	vop_mode_ptr->pYUVRecFrame->imgUAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRecFrame->imgU) ;//>> 8;
	vop_mode_ptr->pYUVRecFrame->imgVAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRecFrame->imgV) ;//>> 8;

	vop_mode_ptr->pYUVRefFrame->imgYAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRefFrame->imgY) ;//>> 8;
	vop_mode_ptr->pYUVRefFrame->imgUAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRefFrame->imgU) ;//>> 8;
	vop_mode_ptr->pYUVRefFrame->imgVAddr = (uint32)Mp4Enc_ExtraMem_V2Phy(mp4Handle,vop_mode_ptr->pYUVRefFrame->imgV) ;//>> 8;
#else
	vop_mode_ptr->pYUVRecFrame->imgYAddr = (uint32)vop_mode_ptr->pYUVRecFrame->imgY ;//>> 8;
	vop_mode_ptr->pYUVRecFrame->imgUAddr = (uint32)vop_mode_ptr->pYUVRecFrame->imgU ;//>> 8;
	vop_mode_ptr->pYUVRecFrame->imgVAddr = (uint32)vop_mode_ptr->pYUVRecFrame->imgV ;//>> 8;

	vop_mode_ptr->pYUVRefFrame->imgYAddr = (uint32)vop_mode_ptr->pYUVRefFrame->imgY;// >> 8;
	vop_mode_ptr->pYUVRefFrame->imgUAddr = (uint32)vop_mode_ptr->pYUVRefFrame->imgU ;//>> 8;
	vop_mode_ptr->pYUVRefFrame->imgVAddr = (uint32)vop_mode_ptr->pYUVRefFrame->imgV ;//>> 8;
#endif

#if _CMODEL_
	vop_mode_ptr->pYUVRecFrame->imgYAddr = REC_FRAME0_Y_ADDR ;//>> 8;
	vop_mode_ptr->pYUVRefFrame->imgYAddr = REC_FRAME1_Y_ADDR ;//>> 8;
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
