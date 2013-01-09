/******************************************************************************
 ** File Name:    mp4dec_mc.c                                              *
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
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_LookupUVMV
 ** Description:	Get motion vector of u/v component. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
#define CLIP(x) ((x) > 255) ? 255 : ((x) < 0 ? 0 : (x))

/************************************************************
16x16, only for luma, 1mv
************************************************************/
#ifndef _NEON_OPT_  
void mc_xyfull_16x16(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;

	for (i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = pframe_ref[j];
		}

		pRecMB += dstWidth;
		pframe_ref += width;
	}	
}

void mc_xfullyhalf_16x16_rnd0(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 *pbot_ref = pframe_ref + width;
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pbot_ref[ j] + 1)>>1;
		}
		
		pRecMB += dstWidth;
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
	}
}

void mc_xfullyhalf_16x16_rnd1(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 *pbot_ref = pframe_ref + width;
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pbot_ref[ j])>>1;
		}
		
		pRecMB += dstWidth;
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
	}
}

void mc_xhalfyfull_16x16_rnd0(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{	
	int32 i, j;	
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + 1)>>1;
		}
		
		pframe_ref  += width;
		pRecMB += dstWidth;
	}		
}

void mc_xhalfyfull_16x16_rnd1(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{	
	int32 i, j;	
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1])>>1;
		}
		
		pframe_ref  += width;
		pRecMB += dstWidth;
	}		
}

void mc_xyhalf_16x16_rnd0(uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 * pbot_ref = pframe_ref + width;
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + pbot_ref[j] + pbot_ref[j+1] + 2)>>2;
		}
		
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB += dstWidth;
	}				
}

void mc_xyhalf_16x16_rnd1(uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 * pbot_ref = pframe_ref + width;
	
	for(i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + pbot_ref[j] + pbot_ref[j+1] + 1)>>2;
		}
		
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB += dstWidth;
	}				
}

/***********************************************************
motion compensation for 8x8 block; 1: 4mv for y, and for uv 
************************************************************/
void mc_xyfull_8x8(uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;

	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = pframe_ref[j];
		}

		pRecMB += dstWidth;
		pframe_ref += width;
	}
}

void mc_xfullyhalf_8x8_rnd0 (uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 *pbot_ref = pframe_ref + width;
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pbot_ref[j] + 1) >> 1;	
		}
		
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB += dstWidth;
	}	
}

void mc_xfullyhalf_8x8_rnd1 (uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth)
{
	int32 i, j;
	uint8 *pbot_ref = pframe_ref + width;
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pbot_ref[j]) >> 1;	
		}
		
		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB += dstWidth;
	}	
}

void mc_xhalfyfull_8x8_rnd0(uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{
	int32 i, j;	
	
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + 1) >> 1;	
		}

		pRecMB += dstWidth;
		pframe_ref  += width;
	}
}

void mc_xhalfyfull_8x8_rnd1(uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{
	int32 i, j;	
	
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1]) >> 1;	
		}

		pRecMB += dstWidth;
		pframe_ref  += width;
	}
}

void mc_xyhalf_8x8_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{	
	int32 i, j;
	uint8 * pbot_ref = pframe_ref + width;

	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + pbot_ref[j] + pbot_ref[j+1] + 2) >> 2;	
		}

		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB      += dstWidth;
	}		
}

void mc_xyhalf_8x8_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth)
{	
	int32 i, j;
	uint8 * pbot_ref = pframe_ref + width;

	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			pRecMB[j] = (pframe_ref[j] + pframe_ref[j+1] + pbot_ref[j] + pbot_ref[j+1] + 1) >> 2;	
		}

		pframe_ref   = pbot_ref;
		pbot_ref    += width;
		pRecMB      += dstWidth;
	}		
}
#endif //_NEON_OPT_

void Mp4Dec_motionCompY_fourMV(DEC_VOP_MODE_T *pVopmd,  MOTION_VECTOR_T *pmv,uint8* pDstFrameY, int32 dstWidth, int32 bck_direction)
{
	int32 iBlk;
	int32 xRef, yRef;
	uint8 *pframe_ref, * pRecBlk;
	uint8 *pframe_refY = bck_direction ? pVopmd->YUVRefFrame0[0] : pVopmd->YUVRefFrame2[0];
	int32 width = pVopmd->FrameExtendWidth;	
	int32 xMBPos = pVopmd->mb_x <<5;
	int32 yMBPos = pVopmd->mb_y <<5;
	MOTION_VECTOR_T * pmv8 = pmv;
	int32 dx, dy;
	

	for(iBlk = 0; iBlk < 4; iBlk++)
	{
		int32 xoffset, yoffset;

		xoffset = (iBlk & 1) <<3;
		yoffset = (iBlk >> 1) <<3;

		xRef = xMBPos + xoffset+ xoffset + pmv8->x;
		yRef = yMBPos + yoffset+yoffset + pmv8->y;
		dx = xRef & 1;
		dy = yRef & 1;

		pframe_ref = pframe_refY + ((yRef >> 1) + YEXTENTION_SIZE) * width + (xRef >> 1) + YEXTENTION_SIZE;
		pRecBlk = pDstFrameY + yoffset*dstWidth + xoffset;
		g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecBlk, width, dstWidth);
		
		pmv8++;
	}
}

void Mp4Dec_motionCompY_oneMV(DEC_VOP_MODE_T *pVopmd,  MOTION_VECTOR_T *pmv,uint8* pDstFrameY, int32 dstWidth , int32 bck_direction)
{
	int32 xRef, yRef;
	uint8 * pframe_ref;
	int32 width = pVopmd->FrameExtendWidth;
	uint8 *pframe_refY = bck_direction ? pVopmd->YUVRefFrame0[0] : pVopmd->YUVRefFrame2[0];
	int32 xMBPos = pVopmd->mb_x <<5;
	int32 yMBPos = pVopmd->mb_y <<5;
	int32 dx, dy;
	
	xRef = xMBPos + pmv->x;
	yRef = yMBPos + pmv->y;
	dx = xRef & 1;
	dy = yRef & 1;

	pframe_ref = pframe_refY + ((yRef >> 1) + YEXTENTION_SIZE) * width + (xRef >> 1) + YEXTENTION_SIZE;
	g_mp4dec_mc_16x16[(dx<<1) | dy] (pframe_ref, pDstFrameY, width, dstWidth);
}

void Mp4Dec_motionCompensationUV(DEC_VOP_MODE_T *pVopmd, int32 iMVUVx, int32 iMVUVy,uint8* pDstFrameU,uint8* pDstFrameV, int32 dstWidth , int32 bck_direction)
{
	int32 dx, dy;
	int32 xRef, yRef;
	uint8 *pframe_ref;
	int32 offset_ref;
	int32 width = pVopmd->FrameExtendWidth >> 1;
	
	xRef = (pVopmd->mb_x <<4) + iMVUVx;
	yRef = (pVopmd->mb_y <<4) + iMVUVy; 
	dx = xRef & 1;
	dy = yRef & 1;
		
    	offset_ref = ((yRef >> 1) + UVEXTENTION_SIZE) * width + (xRef >> 1) + UVEXTENTION_SIZE;	
	
	pframe_ref =(bck_direction ? pVopmd->YUVRefFrame0[1] : pVopmd->YUVRefFrame2[1])+ offset_ref;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pDstFrameU, width, dstWidth);

	pframe_ref = (bck_direction ? pVopmd->YUVRefFrame0[2] : pVopmd->YUVRefFrame2[2])+ offset_ref;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pDstFrameV, width, dstWidth);
}
#if 0
//for B Frames
void Mp4Dec_motionCompY_fourMV_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[],\
		uint8* pBlkY256, MOTION_VECTOR_T *pmv)
{
	int32 iBlk;
	int32 xRef, yRef;
	uint8 *pframe_ref;
	uint8 *pRecMBY, * pRecBlk;
	//uint8 *pframe_refY = pVopmd->YUVRefFrame0[0];
	uint8 *pframe_refY = pRefFrame[0];
	int32 width = pVopmd->FrameExtendWidth;	
	int32 xMBPos = pVopmd->mb_x * MB_SIZE * 2;
	int32 yMBPos = pVopmd->mb_y * MB_SIZE * 2;
	MOTION_VECTOR_T * pmv8 = pmv;
	int32 dx, dy;
		
	//pRecMBY = pMBCache->pMBBfrY;
	pRecMBY = pBlkY256;

	for(iBlk = 0; iBlk < 4; iBlk++)
	{
		int32 xoffset, yoffset;

		xoffset = (iBlk & 1) * 8;
		yoffset = (iBlk >> 1) * 8;

		xRef = xMBPos + xoffset*2 + pmv8->x;
		yRef = yMBPos + yoffset*2 + pmv8->y;
		dx = xRef & 1;
		dy = yRef & 1;

		pframe_ref = pframe_refY + ((yRef >> 1) + YEXTENTION_SIZE) * width + (xRef >> 1) + YEXTENTION_SIZE;
		pRecBlk = pRecMBY + yoffset * MB_SIZE + xoffset;
		g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecBlk, width, MB_SIZE);

		pmv8++;
	}
}


void Mp4Dec_motionCompY_oneMV_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[], uint8* pBlkY256, MOTION_VECTOR_T *pmv)
{
	int32 xRef, yRef;
	uint8 * pframe_ref;
	uint8 * pRecMBY;
	int32 mb_x = pVopmd->mb_x;
	int32 mb_y = pVopmd->mb_y;
	int32 width = pVopmd->FrameExtendWidth;
	uint8 *pframe_refY = pRefFrame[0];
	int32 xMBPos = mb_x * MB_SIZE * 2;
	int32 yMBPos = mb_y * MB_SIZE * 2;
	int32 dx, dy;
		
	pRecMBY = pBlkY256;
	
	xRef = xMBPos + pmv->x;
	yRef = yMBPos + pmv->y;
	dx = xRef & 1;
	dy = yRef & 1;

	pframe_ref = pframe_refY + ((yRef >> 1) + YEXTENTION_SIZE) * width + (xRef >> 1) + YEXTENTION_SIZE;
	g_mp4dec_mc_16x16[(dx<<1) | dy] (pframe_ref, pRecMBY, width, MB_SIZE);
}

void Mp4Dec_motionCompensationUV_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[], uint8* pBlkU64,uint8* pBlkV64, int32 iMVUVx, int32 iMVUVy)
{
	int32 dx, dy;
	int32 xRef, yRef;
	uint8 *pframe_ref;
	uint8 *pRecMB;
	int32 offset_ref;
	int32 width = pVopmd->FrameExtendWidth >> 1;
	
	xRef = pVopmd->mb_x * BLOCK_SIZE * 2 + iMVUVx;
	yRef = pVopmd->mb_y * BLOCK_SIZE * 2 + iMVUVy; 
	dx = xRef & 1;
	dy = yRef & 1;
	
    	offset_ref = ((yRef >> 1) + UVEXTENTION_SIZE) * width + (xRef >> 1) + UVEXTENTION_SIZE;	
	
        pframe_ref = pRefFrame[1] + offset_ref;
	pRecMB = pBlkU64;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecMB, width, BLOCK_SIZE);

	pframe_ref = pRefFrame[2] + offset_ref;
	pRecMB = pBlkV64;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecMB, width, BLOCK_SIZE);
}
#endif
void Mp4Dec_MC_GetAverage(uint8 * pSrc0, uint8 * pSrc1, uint8 *pDst, int dst_width, int iYUVFlag)
{
	int i;

	if(iYUVFlag)
	{
		for(i = 0; i <16; i++)
		{
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			pDst += (dst_width - MB_SIZE);
		}
	}else
	{
		for(i = 0; i <8; i++)
		{
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;
			* pDst ++ = ((*pSrc0 ++) + (*pSrc1 ++) +1) >>1;

			pDst += (dst_width - MB_CHROMA_SIZE);
		}		
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_MCA_BVOP_sw
 ** Description:		MC the MB in BVOP
 ** Author:			Simon.Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_MCA_BVOP_sw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv, 
					   MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv)
{
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdMv, BckMv;
	int32 mbmode = mb_mode_ptr->dctMd;
	MOTION_VECTOR_T mv, zeroMv = {0, 0};
	int32 blk_num;
	int32 dmvcx_fwd,dmvcy_fwd, dmvcx_bck, dmvcy_bck;
	int mvc_x_fwd, mvc_y_fwd, mvc_x_bck, mvc_y_bck;
        int32 pos_x = vop_mode_ptr->mb_x;
 	int32 pos_y = vop_mode_ptr->mb_y;	
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	uint8 * pDstFrameY, * pDstFrameU,* pDstFrameV;
	MOTION_VECTOR_T FwdMvCliped[4] = {0};
	MOTION_VECTOR_T BckMvCliped[4] = {0};
	int	dst_width;
	
	mv = zeroMv;
//for MV CLIP
	vop_mode_ptr->mv_x_max = ((total_mb_num_x - pos_x) << 5) -1;
	vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
	vop_mode_ptr->mv_y_max = ((vop_mode_ptr->MBNumY - pos_y) << 5)-1; 
	vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;

	if(mb_mode_ptr->CBP)
	{
		pDstFrameY = mb_cache_ptr->pMBBfrY;
		pDstFrameU = mb_cache_ptr->pMBBfrU;
		pDstFrameV = mb_cache_ptr->pMBBfrV;
		dst_width	= MB_SIZE;
	}else
	{
		pDstFrameY = mb_cache_ptr->mb_addr[0];
		pDstFrameU = mb_cache_ptr->mb_addr[1];
		pDstFrameV = mb_cache_ptr->mb_addr[2];
		dst_width	= vop_mode_ptr->FrameExtendWidth;		
	}


	switch(mbmode)
	{
	case MODE_DIRECT:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, &mv, &zeroMv, 1);
		//Noted: do not add "break" here.xiaoweiluo@20081204
	case MODE_DIRECT_NONE_MV: //lint !e825
		dmvcx_fwd = dmvcy_fwd = dmvcx_bck = dmvcy_bck = 0;
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			int32 tmp;
			int32 comv_x, comv_y;

			comv_x = pCoMv[blk_num].x;
			comv_y = pCoMv[blk_num].y;

			FwdMv.x = comv_x * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.x;
			FwdMv.y = comv_y * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.y;
			
			tmp = (vop_mode_ptr->time_bp - vop_mode_ptr->time_pp);
			BckMv.x = (mv.x) ? (FwdMv.x - (int16)comv_x) : (int16)(comv_x*tmp/vop_mode_ptr->time_pp);
			BckMv.y = (mv.y) ? (FwdMv.y - (int16)comv_y) : (int16)(comv_y*tmp/vop_mode_ptr->time_pp);

			//fwd
			CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);
		       FwdMvCliped[blk_num] = FwdMv;

			//bck
			CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
			BckMvCliped[blk_num] = BckMv;

			dmvcx_fwd += FwdMv.x;
			dmvcy_fwd += FwdMv.y;
			dmvcx_bck += BckMv.x;
			dmvcy_bck += BckMv.y;
		}


		//uv mv_fwd
		mvc_x_fwd = (dmvcx_fwd >> 3) + g_MvRound16[dmvcx_fwd & 0xf];
		mvc_y_fwd = (dmvcy_fwd >> 3) + g_MvRound16[dmvcy_fwd & 0xf];	

		//uv mv_bck
		mvc_x_bck = (dmvcx_bck >> 3) + g_MvRound16[dmvcx_bck & 0xf];
		mvc_y_bck = (dmvcy_bck >> 3) + g_MvRound16[dmvcy_bck & 0xf];	

		Mp4Dec_motionCompY_fourMV(vop_mode_ptr,FwdMvCliped,mb_cache_ptr->pMBBfrY, MB_SIZE, 0);
		Mp4Dec_motionCompY_fourMV(vop_mode_ptr,BckMvCliped,(uint8 *)(vop_mode_ptr->coef_block[0]), MB_SIZE,1 );

		Mp4Dec_motionCompensationUV(vop_mode_ptr, mvc_x_fwd, mvc_y_fwd ,mb_cache_ptr->pMBBfrU, mb_cache_ptr->pMBBfrV,MB_CHROMA_SIZE,0);
		Mp4Dec_motionCompensationUV(vop_mode_ptr, mvc_x_bck, mvc_y_bck,(uint8 *)(vop_mode_ptr->coef_block[4]), (uint8 *)(vop_mode_ptr->coef_block[5]),MB_CHROMA_SIZE,1);

		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrY, (uint8 *)(vop_mode_ptr->coef_block[0]),pDstFrameY,dst_width,1);
		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrU, (uint8 *)(vop_mode_ptr->coef_block[4]),pDstFrameU,dst_width/2,0);
		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrV, (uint8 *)(vop_mode_ptr->coef_block[5]),pDstFrameV,dst_width/2,0);
		break;
	case MODE_INTERPOLATE:

	//fwd
		Mp4Dec_DecodeOneMV(vop_mode_ptr, &FwdMv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);

		* pFwdPredMv = FwdMv;

		CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);

	//bck
		Mp4Dec_DecodeOneMV(vop_mode_ptr,&BckMv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);

		*pBckPredMv = BckMv;

		CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);

	/*compute mv of uv*/		
		mvc_x_fwd = (FwdMv.x >> 1) + g_MvRound4[FwdMv.x & 0x3];
		mvc_y_fwd = (FwdMv.y >> 1) + g_MvRound4[FwdMv.y & 0x3];

		mvc_x_bck = (BckMv.x >> 1) + g_MvRound4[BckMv.x & 0x3];
		mvc_y_bck = (BckMv.y >> 1) + g_MvRound4[BckMv.y & 0x3];

		Mp4Dec_motionCompY_oneMV(vop_mode_ptr,&FwdMv,mb_cache_ptr->pMBBfrY,MB_SIZE,0 );
		Mp4Dec_motionCompY_oneMV(vop_mode_ptr,&BckMv,(uint8 *)(vop_mode_ptr->coef_block[0]), MB_SIZE,1);

		Mp4Dec_motionCompensationUV(vop_mode_ptr,mvc_x_fwd, mvc_y_fwd,mb_cache_ptr->pMBBfrU, mb_cache_ptr->pMBBfrV,MB_CHROMA_SIZE,0 );
		Mp4Dec_motionCompensationUV(vop_mode_ptr, mvc_x_bck, mvc_y_bck ,(uint8 *)(vop_mode_ptr->coef_block[4]), (uint8 *)(vop_mode_ptr->coef_block[5]),MB_CHROMA_SIZE,1);

		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrY, (uint8 *)(vop_mode_ptr->coef_block[0]),pDstFrameY,dst_width,1);
		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrU, (uint8 *)(vop_mode_ptr->coef_block[4]),pDstFrameU,dst_width/2,0);
		Mp4Dec_MC_GetAverage(mb_cache_ptr->pMBBfrV, (uint8 *)(vop_mode_ptr->coef_block[5]),pDstFrameV,dst_width/2,0);

		break;
	case MODE_BACKWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr,&BckMv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);

		*pBckPredMv = BckMv;
		CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
		
		mvc_x_bck = (BckMv.x >> 1) + g_MvRound4[BckMv.x & 0x3];
		mvc_y_bck = (BckMv.y >> 1) + g_MvRound4[BckMv.y & 0x3];
		
		Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  &BckMv,  pDstFrameY,dst_width, 1);
		Mp4Dec_motionCompensationUV(vop_mode_ptr, mvc_x_bck,mvc_y_bck,  pDstFrameU,  pDstFrameV, dst_width/2, 1);

		break;
		
	case MODE_FORWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, &FwdMv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);

		*pFwdPredMv = FwdMv;
		CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);

		mvc_x_fwd = (FwdMv.x >> 1) + g_MvRound4[FwdMv.x & 0x3];
		mvc_y_fwd = (FwdMv.y >> 1) + g_MvRound4[FwdMv.y & 0x3];
		
		Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  &FwdMv,  pDstFrameY,dst_width, 0);
		Mp4Dec_motionCompensationUV(vop_mode_ptr, mvc_x_fwd,mvc_y_fwd,  pDstFrameU,  pDstFrameV, dst_width/2, 0);
		
		break;
	default:
		break;
	}

}
                                              



/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
