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
#include "sc8810_video_header.h"
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

void Mp4Dec_motionCompY_fourMV(DEC_VOP_MODE_T *pVopmd, DEC_MB_BFR_T *pMBCache, MOTION_VECTOR_T *pmv,uint8* pDstFrameY)
{
	int32 iBlk;
	int32 xRef, yRef;
	uint8 *pframe_ref, * pRecBlk;
	uint8 *pframe_refY = pVopmd->YUVRefFrame0[0];
	int32 width = pVopmd->FrameExtendWidth;	
	int32 xMBPos = pVopmd->mb_x <<5;
	int32 yMBPos = pVopmd->mb_y <<5;
	MOTION_VECTOR_T * pmv8 = pmv;
	int32 dx, dy;
	
	int32 dstWidth = (pDstFrameY == pMBCache->pMBBfrY) ? MB_SIZE : width;

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

void Mp4Dec_motionCompY_oneMV(DEC_VOP_MODE_T *pVopmd, DEC_MB_BFR_T *pMBCache, MOTION_VECTOR_T *pmv,uint8* pDstFrameY)
{
	int32 xRef, yRef;
	uint8 * pframe_ref;
	int32 width = pVopmd->FrameExtendWidth;
	uint8 *pframe_refY = pVopmd->YUVRefFrame0[0];
	int32 xMBPos = pVopmd->mb_x <<5;
	int32 yMBPos = pVopmd->mb_y <<5;
	int32 dx, dy;
	int32 dstWidth = (pDstFrameY == pMBCache->pMBBfrY) ? MB_SIZE : width;
	
	xRef = xMBPos + pmv->x;
	yRef = yMBPos + pmv->y;
	dx = xRef & 1;
	dy = yRef & 1;

	pframe_ref = pframe_refY + ((yRef >> 1) + YEXTENTION_SIZE) * width + (xRef >> 1) + YEXTENTION_SIZE;
	g_mp4dec_mc_16x16[(dx<<1) | dy] (pframe_ref, pDstFrameY, width, dstWidth);
}

void Mp4Dec_motionCompensationUV(DEC_VOP_MODE_T *pVopmd, DEC_MB_BFR_T *pMBCache, int32 iMVUVx, int32 iMVUVy,uint8* pDstFrameU,uint8* pDstFrameV)
{
	int32 dx, dy;
	int32 xRef, yRef;
	uint8 *pframe_ref;
	int32 offset_ref;
	int32 width = pVopmd->FrameExtendWidth >> 1;
	int32 dstWidth = (pDstFrameU == pMBCache->pMBBfrU) ? BLOCK_SIZE : width;
	
	xRef = (pVopmd->mb_x <<4) + iMVUVx;
	yRef = (pVopmd->mb_y <<4) + iMVUVy; 
	dx = xRef & 1;
	dy = yRef & 1;
		
    	offset_ref = ((yRef >> 1) + UVEXTENTION_SIZE) * width + (xRef >> 1) + UVEXTENTION_SIZE;	
	
	pframe_ref = pVopmd->YUVRefFrame0 [1] + offset_ref;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pDstFrameU, width, dstWidth);

	pframe_ref = pVopmd->YUVRefFrame0 [2] + offset_ref;
	g_mp4dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pDstFrameV, width, dstWidth);
}

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

#if 0
void Mp4Dec_motionCompY_oneMV_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[], uint8* pBlkY256, MOTION_VECTOR_T *pmv)
{
	int32 xRef, yRef;
	uint8 * pframe_ref;
	uint8 * pRecMBY;
	int32 mv_x, mv_y;
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
	g_dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecMB, width, BLOCK_SIZE);

	pframe_ref = pRefFrame[2] + offset_ref;
	pRecMB = pBlkV64;
	g_dec_mc_8x8[(dx<<1) | dy] (pframe_ref, pRecMB, width, BLOCK_SIZE);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_MCA_BVOP
 ** Description:	Get the motion vector of current macroblock from bitstream. BVOP
 store the mv after CLIP,to be used for fetch RefFrame pixel.
 ** Author:			leon li
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_MCA_BVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv, 
					   MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv,\
					   MOTION_VECTOR_T*pFwdMvCliped,MOTION_VECTOR_T*pBckMvCliped,\
					   MOTION_VECTOR_T*pFwdMvUV,MOTION_VECTOR_T*pBckMvUV)
{
	uint32 cmd;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdMv, BckMv;
	int32 mbmode = mb_mode_ptr->dctMd;
	MOTION_VECTOR_T mv, zeroMv = {0, 0};
	int32 ref_blk_id, ref_blk_size;
	int32 ref_blk_end, ref_bir_blk, ref_cmd_type;
	int32 ref_bw_frame_id, ref_fw_frame_id;
//	int32 mca_type = mb_cache_ptr->mca_type;
	int32 blk_num;
	int32 dmvcx_fwd,dmvcy_fwd, dmvcx_bck, dmvcy_bck;
	int mvc_x_fwd, mvc_y_fwd, mvc_x_bck, mvc_y_bck;
	//@LEON
        int32 pos_x = vop_mode_ptr->mb_x;
 	int32 pos_y = vop_mode_ptr->mb_y;	
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	
	mv = zeroMv;
//for MV CLIP
	vop_mode_ptr->mv_x_max = (total_mb_num_x - pos_x) << 5;
	vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
	vop_mode_ptr->mv_y_max = (vop_mode_ptr->MBNumY - pos_y) << 5; 
	vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;
	
#if _TRACE_	
	FPRINTF(g_fp_trace_fw, "pfpmv_x: %d, pfpmv_y: %d, pbpmv_x:%d, pbpmv_y: %d\n", pFwdPredMv->x, pFwdPredMv->y, pBckPredMv->x, pBckPredMv->y);
#endif //_TRACE_

	switch(mbmode)
	{
	case MODE_DIRECT:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, &mv, &zeroMv, 1);
		//Noted: do not add "break" here.xiaoweiluo@20081204
	case MODE_DIRECT_NONE_MV: //lint !e825
		dmvcx_fwd = dmvcy_fwd = dmvcx_bck = dmvcy_bck = 0;
		ref_blk_size = MC_BLKSIZE_8x8;
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			int32 tmp;
			int32 comv_x, comv_y;

			ref_blk_id = blk_num; 

			comv_x = pCoMv[blk_num].x;
			comv_y = pCoMv[blk_num].y;

			FwdMv.x = comv_x * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.x;
			FwdMv.y = comv_y * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.y;
			
			tmp = (vop_mode_ptr->time_bp - vop_mode_ptr->time_pp);
			BckMv.x = (mv.x) ? (FwdMv.x - (int16)comv_x) : (int16)(comv_x*tmp/vop_mode_ptr->time_pp);
			BckMv.y = (mv.y) ? (FwdMv.y - (int16)comv_y) : (int16)(comv_y*tmp/vop_mode_ptr->time_pp);


			((int32 *)(&(mb_cache_ptr->fmv[blk_num])))[0] = ((int32 *)(&FwdMv))[0];
			((int32 *)(&(mb_cache_ptr->bmv[blk_num])))[0] = ((int32 *)(&BckMv))[0];


			//fwd
			CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);
		       *(  (uint32*)(pFwdMvCliped+blk_num)  )= ((uint32*)(&FwdMv))[0];

			//bck
			CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
                        *(  (uint32*)(pBckMvCliped+blk_num)  )= ((uint32*)(&BckMv))[0];

			dmvcx_fwd += FwdMv.x;
			dmvcy_fwd += FwdMv.y;
			dmvcx_bck += BckMv.x;
			dmvcy_bck += BckMv.y;
			
		#if _TRACE_
			FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d, bmv_x:%d, bmv_y: %d\n", FwdMv.x, FwdMv.y, BckMv.x, BckMv.y);
		#endif //_TRACE_	
		}


		//uv mv_fwd
		mvc_x_fwd = (dmvcx_fwd >> 3) + g_MvRound16[dmvcx_fwd & 0xf];
		mvc_y_fwd = (dmvcy_fwd >> 3) + g_MvRound16[dmvcy_fwd & 0xf];	
		pFwdMvUV->x = mvc_x_fwd;
		pFwdMvUV->y = mvc_y_fwd;

		//uv mv_bck
		mvc_x_bck = (dmvcx_bck >> 3) + g_MvRound16[dmvcx_bck & 0xf];
		mvc_y_bck = (dmvcy_bck >> 3) + g_MvRound16[dmvcy_bck & 0xf];	
		pBckMvUV->x = mvc_x_bck;
		pBckMvUV->y = mvc_y_bck;

		break;
	case MODE_INTERPOLATE:

	//fwd
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->fmv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);

		((int32 *)pFwdPredMv)[0] = ((int32 *)mb_cache_ptr->fmv)[0];

		FwdMv = mb_cache_ptr->fmv[0];

		CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);
		//*(  (uint32*)(pFwdMvCliped+blk_num)  )= ((uint32*)(&FwdMv))[0];
		*(  (uint32*)(pFwdMvCliped+0)  )= ((uint32*)(&FwdMv))[0];

	//bck
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->bmv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);
		((int32 *)pBckPredMv)[0] = ((int32 *)mb_cache_ptr->bmv)[0];

		BckMv = mb_cache_ptr->bmv[0];
		CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
		//*(  (uint32*)(pBckMvCliped+blk_num)  )= ((uint32*)(&BckMv))[0];
		*(  (uint32*)(pBckMvCliped+0)  )= ((uint32*)(&BckMv))[0];



	/*compute mv of uv*/		
		mvc_x_fwd = (FwdMv.x >> 1) + g_MvRound4[FwdMv.x & 0x3];
		mvc_y_fwd = (FwdMv.y >> 1) + g_MvRound4[FwdMv.y & 0x3];

		pFwdMvUV->x = mvc_x_fwd;
		pFwdMvUV->y = mvc_y_fwd;

		mvc_x_bck = (BckMv.x >> 1) + g_MvRound4[BckMv.x & 0x3];
		mvc_y_bck = (BckMv.y >> 1) + g_MvRound4[BckMv.y & 0x3];

		pBckMvUV->x = mvc_x_bck;
		pBckMvUV->y = mvc_y_bck;


	#if _TRACE_	
		FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d, bmv_x:%d, bmv_y: %d\n", mb_cache_ptr->fmv->x, mb_cache_ptr->fmv->y, mb_cache_ptr->bmv->x, mb_cache_ptr->bmv->y);
	#endif //_TRACE_
		break;
	case MODE_BACKWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->bmv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);
		((int32 *)pBckPredMv)[0] = ((int32 *)mb_cache_ptr->bmv)[0];
               // *(  (uint32*)(pBckMvCliped+blk_num)  )= ((uint32*)(&BckMv))[0];//@leon???
               *(  (uint32*)(pBckMvCliped+0)  )= ((uint32*)(mb_cache_ptr->bmv))[0];//@leon???
		Mp4Dec_StartMcaOneDir_sw(vop_mode_ptr, mb_cache_ptr, mb_cache_ptr->bmv,&(pBckMvUV->x),&(pBckMvUV->y) );
	#if _TRACE_
		FPRINTF(g_fp_trace_fw, "bmv_x: %d, bmv_y: %d\n", mb_cache_ptr->bmv->x, mb_cache_ptr->bmv->y);
	#endif //_TRACE_
		break;
	case MODE_FORWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->fmv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);
		((int32 *)pFwdPredMv)[0] = ((int32 *)mb_cache_ptr->fmv)[0];
		//*(  (uint32*)(pFwdMvCliped+blk_num)  )= ((uint32*)(&FwdMv))[0];//@leon ???
                *(  (uint32*)(pFwdMvCliped+0)  )= ((uint32*)(mb_cache_ptr->fmv))[0];//@leon ???
		Mp4Dec_StartMcaOneDir_sw (vop_mode_ptr, mb_cache_ptr, mb_cache_ptr->fmv,&(pFwdMvUV->x),&(pFwdMvUV->y));
	#if _TRACE_	
		FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d\n", mb_cache_ptr->fmv->x, mb_cache_ptr->fmv->y);
	#endif //_TRACE_	
		break;
	default:
		break;
	}

}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetRefMB
 ** Description:	Get the ref pixel of current macroblock from bitstream. BVOP
 ** Author:			Leon Li
 **	Note:
 *****************************************************************************/

void Mp4Dec_GetRefMB(DEC_VOP_MODE_T* vop_mode_ptr, \
	                                                  DEC_MB_MODE_T* mb_mode_ptr,\
	                                                  MOTION_VECTOR_T *pMv ,\
	                                                  MOTION_VECTOR_T *pMvUV,\
	                                                  unsigned char* pMblockY,\
	                                                  unsigned char* pMblockU,\
	                                                  unsigned char* pMblockV,\
	                                                   unsigned char* pRefFrame[],\
	                                                   int32 round_control)
	                                                  

{
//to add 
        int blk_num = 0;
        int32  iMVUVx,  iMVUVy;
        DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	iMVUVx = pMvUV->x;
	iMVUVy = pMvUV->y;

	if(mb_mode_ptr->bSkip)
	{
	//direct copy
		Mp4Dec_CopyRef2RecMB_BFrame(vop_mode_ptr, pRefFrame,pMblockY,pMblockU,pMblockV);
	}

	//if(INTER4V == mb_mode_ptr->dctMd)
	if(MCA_BI_DRT_4V == mb_cache_ptr->mca_type)
	{
		//MC_Y;
		Mp4Dec_motionCompY_fourMV_BFrame(vop_mode_ptr, pRefFrame,\
		pMblockY, pMv);
		//MC_UV;
		Mp4Dec_motionCompensationUV_BFrame(vop_mode_ptr, pRefFrame,\
                pMblockU, pMblockV,  iMVUVx,  iMVUVy);
	}
	else//INTER
	{
		//MC_Y;
		Mp4Dec_motionCompY_oneMV_BFrame(vop_mode_ptr, pRefFrame,\
		pMblockY, pMv);
		//MC_UV;
		Mp4Dec_motionCompensationUV_BFrame(vop_mode_ptr, pRefFrame,\
                pMblockU, pMblockV,  iMVUVx,  iMVUVy);
		
	}
		
}

#ifndef _NEON_OPT_
void Mp4Dec_MC_GetAverage(uint8* pOriBlk,uint8* pRefBlk,int iYUVFlag)
{
	int i=0;
	uint8* pSrc;
	uint8*pDst;

	pDst = pOriBlk;
	pSrc = pRefBlk;
	
	if(iYUVFlag)//Y component
	{
		for(i = 0 ;i < 16;i++)
		{
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;

			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;

			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			  
			 
		}
	}
	else//UV
	{
		for(i = 0 ;i < 8;i++)
		{
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;
			*pDst = ((*pDst) + (*pSrc++) + 1)>>1;
			pDst++;

		}
	
	}

}
#else
void Mp4Dec_MC_GetAverage(uint8* pOriBlk,uint8* pRefBlk,int iYUVFlag)
{
	int i=0;
	uint8* pSrc;
	uint8*pDst;

	//uint8 *pbot_ref = pframe_ref + width;
	uint8x8_t i8byte_top;
	uint8x8_t i8byte_bot;
	uint8x8_t i8byte_aft_shift;
	uint16x8_t i8ushort,i8ushort_sum;
	uint8x8_t i8byte_RndCtrl;
	uint8 iRndCtrl = 1;
	
	i8byte_RndCtrl = vdup_n_u8(iRndCtrl);
	pDst = pOriBlk;
	pSrc = pRefBlk;
	
	if(iYUVFlag)//Y component
	{
		for(i = 0 ;i < 32;i++)
		{
			  i8byte_bot = vld1_u8(pSrc);
			  i8byte_top = vld1_u8(pDst);
			  pSrc += 8;

			  i8ushort = vaddl_u8(i8byte_top,i8byte_bot);

			i8ushort_sum = vaddw_u8(i8ushort,i8byte_RndCtrl);
                	i8byte_aft_shift = vshrn_n_u16(i8ushort_sum,1);
			vst1_u8(pDst,i8byte_aft_shift);

			pDst += 8;
			
			 
		}
	}
	else//UV
	{
		for(i = 0 ;i < 8;i++)
		{
			 
			  i8byte_bot = vld1_u8(pSrc);
      		          i8byte_top = vld1_u8(pDst);
			  pSrc += 8;
			  
			  i8ushort = vaddl_u8(i8byte_top,i8byte_bot);

			i8ushort_sum = vaddw_u8(i8ushort,i8byte_RndCtrl);
                	i8byte_aft_shift = vshrn_n_u16(i8ushort_sum,1);
			vst1_u8(pDst,i8byte_aft_shift);

			pDst += 8;
		}
	
	}

}
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_RecOneMbBvop
 ** Description:	Get the motion vector of current macroblock from bitstream. BVOP
 ** Author:			Leon Li
 **	Note:
 *****************************************************************************/
void Mp4Dec_RecOneMbBvop(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr,\
 					   MOTION_VECTOR_T*pFwdMvCliped,MOTION_VECTOR_T*pBckMvCliped,\
					   MOTION_VECTOR_T*pFwdMvUV,MOTION_VECTOR_T*pBckMvUV)

{
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
	DEC_MB_BFR_T *pMBCache = vop_mode_ptr->mb_cache_ptr;
	int32 mbmode = mb_mode_ptr->dctMd;
	
	switch(mbmode)
	{
		case MODE_DIRECT:
		case MODE_DIRECT_NONE_MV:
		case MODE_INTERPOLATE:
		{
			Mp4Dec_GetRefMB(vop_mode_ptr,mb_mode_ptr,pFwdMvCliped,pFwdMvUV,pMBCache->pMBBfrY,\
				pMBCache->pMBBfrU,pMBCache->pMBBfrV,vop_mode_ptr->YUVRefFrame2,0);//frd
			Mp4Dec_GetRefMB(vop_mode_ptr,mb_mode_ptr,pBckMvCliped,pBckMvUV,(uint8 *)(vop_mode_ptr->coef_block[0]),\
				(uint8 *)(vop_mode_ptr->coef_block[4]),(uint8 *)(vop_mode_ptr->coef_block[5]),vop_mode_ptr->YUVRefFrame0,0);//bck
                         //to add
			   Mp4Dec_MC_GetAverage(pMBCache->pMBBfrY,vop_mode_ptr->coef_block[0],1);//@leon
			   Mp4Dec_MC_GetAverage(pMBCache->pMBBfrU,vop_mode_ptr->coef_block[4],0);//@leon
			   Mp4Dec_MC_GetAverage(pMBCache->pMBBfrV,vop_mode_ptr->coef_block[5],0);//@leon
			break;
		}
		
		case MODE_FORWARD:
		{
			Mp4Dec_GetRefMB(vop_mode_ptr,mb_mode_ptr,pFwdMvCliped,pFwdMvUV,pMBCache->pMBBfrY,\
				pMBCache->pMBBfrU,pMBCache->pMBBfrV,vop_mode_ptr->YUVRefFrame2,0);//frd
			break;
		}
		
		case MODE_BACKWARD:
		{
			Mp4Dec_GetRefMB(vop_mode_ptr,mb_mode_ptr,pBckMvCliped,pBckMvUV,pMBCache->pMBBfrY,\
				pMBCache->pMBBfrU,pMBCache->pMBBfrV,vop_mode_ptr->YUVRefFrame0,0);//bck
			break;
		}
		
		default :
			break;
	}	
}
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
