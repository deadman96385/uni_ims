/******************************************************************************
 ** File Name:    mp4dec_vop.c                                             *
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

#ifndef YUV_THREE_PLANE
#ifndef _NEON_OPT_
PUBLIC void write_display_frame(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame)
{
	int16 FrameWidth= vop_mode_ptr->FrameWidth;
	int16 FrameHeight= vop_mode_ptr->FrameHeight;
	int16 FrameExtendWidth= vop_mode_ptr->FrameExtendWidth;     
	int16 FrameExtendHeigth= vop_mode_ptr->FrameExtendHeigth;
	int16 iStartInFrameY= vop_mode_ptr->iStartInFrameY;	
	int16 iStartInFrameUV= vop_mode_ptr->iStartInFrameUV;
	
	uint8* src = pDecFrame->imgYUV[0] + iStartInFrameY;
	uint8* dst = pDecFrame->imgY;
	uint8*srcU,*srcV;
        int i,j;

	for(i = 0;i< FrameHeight;i++)
	{
	   memcpy(dst,src,FrameWidth);
	   src += FrameExtendWidth;
	   dst += FrameWidth;
	}

	srcU = pDecFrame->imgYUV[1] + iStartInFrameUV;
	srcV = pDecFrame->imgYUV[2] + iStartInFrameUV;

	dst = pDecFrame->imgU;

	for(i = 0;i< FrameHeight>>1;i++)
	{
	   	for(j = 0;j< FrameWidth>>1;j++)
	   	{
	   		*dst++ = srcU[j];
			*dst++ = srcV[j];
	   	}

		srcU += FrameExtendWidth>>1;
		srcV += FrameExtendWidth>>1;
	}
}
#else
void write_display_frame(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame);
#endif
#endif

PUBLIC void Mp4Dec_ExtendFrame(DEC_VOP_MODE_T *vop_mode_ptr )
{
	int32 i;
	int32 height, width, offset, extendWidth, copyWidth;
	uint8 *pSrc1, *pSrc2, *pDst1, *pDst2;
	uint8**Frame = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV;
#ifdef _NEON_OPT_	
	uint8x8_t vec64;
	uint8x16_t vec128;
#endif	
 
	height      = vop_mode_ptr->FrameHeight;
	width       = vop_mode_ptr->FrameWidth;
	extendWidth = vop_mode_ptr->FrameExtendWidth;
	offset      = vop_mode_ptr->iStartInFrameY;

	pSrc1 = Frame[0] + offset;
	pDst1 = pSrc1 - YEXTENTION_SIZE;
	pSrc2 = pSrc1 + width - 1;
	pDst2 = pSrc2 + 1;
	copyWidth = YEXTENTION_SIZE;	


	/*horizontal repeat Y*/
	for(i = 0; i < height; i++)
	{
#ifndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 * pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
		pIntDst[3] = intValue;

		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
		pIntDst[3] = intValue;

		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;
	#else
		//left
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		pSrc1 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst1, vec128);
		pDst1 += extendWidth;
		//right
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		pSrc2 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst2, vec128);
		pDst2 += extendWidth;
		
	#endif

	}

	/*horizontal repeat U*/
	extendWidth = extendWidth / 2;
	offset      = vop_mode_ptr->iStartInFrameUV;
	pSrc1       = Frame [1] + offset;
	pDst1       = pSrc1 - UVEXTENTION_SIZE;
	pSrc2 = pSrc1 + width / 2 - 1;
	pDst2 = pSrc2 + 1;
	copyWidth   = UVEXTENTION_SIZE;
	height = height / 2;
	
	for(i = 0; i < height; i++)
	{
#ifndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 *pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;

		
		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;

		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;
#else
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		pSrc1 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst1, vec64);
		pDst1 += extendWidth;
		
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		pSrc2 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst2, vec64);	
		pDst2 += extendWidth;
#endif		

	}

	/*horizontal repeat V*/
	pSrc1 = Frame [2] + offset;
	pDst1 = pSrc1 - UVEXTENTION_SIZE;
	pSrc2 = pSrc1 + width / 2 - 1;
	pDst2 = pSrc2 + 1;
	for (i = 0; i < height; i++)
	{
#ifndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 * pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;		
		
		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;

		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;	
#else
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		pSrc1 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst1, vec64);
		pDst1 += extendWidth;
		
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		pSrc2 += extendWidth;
		
		vec128 = vdupq_lane_u8(vec64, 0);
		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst2, vec64);
		pDst2 += extendWidth;	

#endif	

	}

	/*copy first row and last row*/
	/*vertical repeat Y*/
	height = vop_mode_ptr->FrameHeight;
	extendWidth  = vop_mode_ptr->FrameExtendWidth;
	offset = extendWidth * YEXTENTION_SIZE;
	pSrc1  = Frame[0] + offset;
	pDst1  = Frame[0];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;

	for(i = 0; i < YEXTENTION_SIZE; i++)
	{
		memcpy(pDst1, pSrc1, extendWidth);
		memcpy(pDst2, pSrc2, extendWidth);		
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}

   	 /*vertical repeat U*/
    	height = height / 2;
	extendWidth  = extendWidth / 2;
	offset = offset / 4;
	pSrc1  = Frame[1] + offset;
	pDst1  = Frame[1];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;

	for(i = 0; i < UVEXTENTION_SIZE; i++)
	{
		memcpy (pDst1, pSrc1, extendWidth);
		memcpy (pDst2, pSrc2, extendWidth);
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}

	/*vertical repeat V*/
	pSrc1  = Frame[2] + offset;
	pDst1  = Frame[2];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;
	
	for(i = 0; i < UVEXTENTION_SIZE; i++)
	{
		memcpy (pDst1, pSrc1, extendWidth);
		memcpy (pDst2, pSrc2, extendWidth);
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOP_sw
 ** Description:	Decode the IVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOP_sw(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (VSP_ITU_H263 == vop_mode_ptr->video_std)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon
	
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;
	
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{
	        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
		mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
		mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
		vop_mode_ptr->mb_y = (int8)pos_y;

		if(is_itu_h263)
		{
			ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
		}

		/*decode one MB line*/		
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{		
		  	vop_mode_ptr->mb_x = (int8)pos_x;	

			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				 mb_mode_ptr->bFirstMB_in_VP = FALSE;

				if(Mp4Dec_CheckResyncMarker(0))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,  0);
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}
			}

			if((0 == pos_y) && (0 == pos_x))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);	
			Mp4Dec_DecIntraMBHeader(vop_mode_ptr, mb_mode_ptr);

			Mp4Dec_DecIntraMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
			
			((int*)mb_mode_ptr->mv)[0] = 0;   //set to zero for B frame'mv prediction
			((int*)mb_mode_ptr->mv)[1] = 0;
			((int*)mb_mode_ptr->mv)[2] = 0;
			((int*)mb_mode_ptr->mv)[3] = 0;
			mb_mode_ptr++;

			//updated for next mb
			mb_cache_ptr->mb_addr[0] += MB_SIZE;
			mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
			mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
			vop_mode_ptr->mbnumDec++;
		}

		ppxlcRecGobY += vop_mode_ptr->FrameExtendWidth * MB_SIZE;
		ppxlcRecGobU += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;
		ppxlcRecGobV += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;
		
 		vop_mode_ptr->GobNum++;
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOP_sw
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
#ifdef _DEBUG_TIME_
long long DecPVOP_time = 0;
#endif
PUBLIC MMDecRet Mp4Dec_DecPVOP_sw(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	DEC_MB_MODE_T * mb_mode_ptr = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == VSP_ITU_H263)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon
#ifdef _DEBUG_TIME_
	struct timeval tpstart,tpend;
       long long  cur_time;
	gettimeofday(&tpstart,NULL);
#endif
  
	Mp4Dec_ExchangeMBMode (vop_mode_ptr);

        //ref frame 
       vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
       vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];
		
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
	        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
		mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
		mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
		vop_mode_ptr->mb_y = (int8)pos_y;

		if(is_itu_h263)
		{
			/*decode GOB header*/
			Mp4Dec_DecGobHeader(vop_mode_ptr);
		}
			
		/*decode one MB line*/
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{	
		#if _DEBUG_
			if((pos_x == 0)&&(pos_y == 0)&&(g_nFrame_dec == 1))
			{
				foo();
			}
		#endif //_DEBUG_
		  	vop_mode_ptr->mb_x = (int8)pos_x;

			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				mb_mode_ptr->bFirstMB_in_VP = FALSE;
				
				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
							
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}	
			}

			if((0 == pos_y) && (0 == pos_x))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
			Mp4Dec_DecInterMBHeader(vop_mode_ptr, mb_mode_ptr);
			Mp4Dec_DecMV_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

			if(mb_mode_ptr->bIntra)
			{	
				Mp4Dec_DecIntraMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
			}else if (mb_mode_ptr->CBP)
			{
				Mp4Dec_DecInterMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);				
			}

			//updated for next mb
			mb_cache_ptr->mb_addr[0] += MB_SIZE;
			mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
			mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
			vop_mode_ptr->mbnumDec++;
			mb_mode_ptr++;	
		}
		
		ppxlcRecGobY += (vop_mode_ptr->FrameExtendWidth <<4);//* MB_SIZE;
		ppxlcRecGobU += (vop_mode_ptr->FrameExtendWidth <<2);//* MB_SIZE / 4;
		ppxlcRecGobV += (vop_mode_ptr->FrameExtendWidth <<2);//MB_SIZE / 4;
			
		vop_mode_ptr->GobNum++;
	}

#ifdef _DEBUG_TIME_
	gettimeofday(&tpend,NULL);
	//recCrop_time +=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
	cur_time = tpend.tv_usec-tpstart.tv_usec;
	if(cur_time < 0)
	{	
		cur_time += 1000000;
	}
	SCI_TRACE_LOW("cur frame % dec time %lld",g_nFrame_dec,cur_time);
#endif
	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecBVOP_sw
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecBVOP_sw(DEC_VOP_MODE_T *vop_mode_ptr)
{
	MMDecRet ret = MMDEC_OK;
#if 0
	int32 pos_y, pos_x;
	DEC_MB_MODE_T *mb_mode_bvop_ptr = vop_mode_ptr->pMbMode_B;
	DEC_MB_MODE_T *pCoMb_mode = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdPredMv, BckPredMv, zeroMv = {0, 0};
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MOTION_VECTOR_T FwdMvCliped[4] = {0};
	MOTION_VECTOR_T BckMvCliped[4] = {0};
	MOTION_VECTOR_T FwdMvClipedUV = {0};
	MOTION_VECTOR_T BckMvClipedUV = {0};


	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon
        uint8*ppxlcRecMBY, *ppxlcRecMBU, *ppxlcRecMBV;
	MOTION_VECTOR_T *pmv;
	DEC_MB_MODE_T * mb_mode_ptr;
	
	mb_mode_ptr		= vop_mode_ptr->pMbMode;
        //backward ref frame 
        vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
        vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];

       //forward ref frame 
        vop_mode_ptr->YUVRefFrame2[0] = vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYUV[0];
        vop_mode_ptr->YUVRefFrame2[1] = vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame2[2] = vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYUV[2];
         
        //rec frame
        vop_mode_ptr->YUVRefFrame1[0] = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0];
        vop_mode_ptr->YUVRefFrame1[1] = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame1[2] = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2];
		
	ppxlcRecGobY = vop_mode_ptr->YUVRefFrame1[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->YUVRefFrame1[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->YUVRefFrame1[2] + vop_mode_ptr->iStartInFrameUV;
	
#ifdef DECODE_BVOP
	FwdPredMv = zeroMv;
	BckPredMv = zeroMv;
	
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
		vop_mode_ptr->mb_y = (int8)pos_y;

		ppxlcRecMBY = ppxlcRecGobY;
		ppxlcRecMBU = ppxlcRecGobU;
		ppxlcRecMBV = ppxlcRecGobV;

		//reset prediction mv
		FwdPredMv = zeroMv;
		BckPredMv = zeroMv;

		/*decode one MB line*/
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{

           		((int32 *)(&(FwdMvCliped[0])))[0]    = 0;
          		((int32 *)(&(FwdMvCliped[1])))[0]    = 0;
          		 ((int32 *)(&(FwdMvCliped[2])))[0]    = 0;
           		((int32 *)(&(FwdMvCliped[3])))[0]    = 0;

           		((int32 *)(&(BckMvCliped[0])))[0]    = 0;
           		((int32 *)(&(BckMvCliped[1])))[0]    = 0;
           		((int32 *)(&(BckMvCliped[2])))[0]    = 0;
           		((int32 *)(&(BckMvCliped[3])))[0]    = 0;

          		((int32 *)(&(FwdMvClipedUV)))[0]    = 0;
	  		((int32 *)(&(BckMvClipedUV)))[0]    = 0;
				
		#if _DEBUG_
			if((pos_y == 0)&&(pos_x == 7)&&(g_nFrame_dec == 2))
			{
				foo();
			}
		#endif //_DEBUG_	
			
		#if _TRACE_	
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_
		
			vop_mode_ptr->mb_x = (int8)pos_x;

			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				if(Mp4Dec_CheckResyncMarker_sw(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader_sw(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

					//reset prediction mv
					FwdPredMv = zeroMv; 
					BckPredMv = zeroMv;
				}			
			}

			mb_mode_bvop_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
			
			if(MODE_NOT_CODED == pCoMb_mode->dctMd)
			{
			#if _TRACE_
				FPRINTF (g_fp_trace_fw, "co-located MB skipped!\n");
			#endif //_TRACE_
				mb_mode_bvop_ptr->bIntra = FALSE; //for dbk.
				mb_mode_bvop_ptr->StepSize = vop_mode_ptr->StepSize; //for dbk.
				mb_mode_bvop_ptr->dctMd = FORWARD;
				mb_mode_bvop_ptr->CBP = 0x00;
				mb_cache_ptr->mca_type = MCA_FORWARD;
				//memset(mb_cache_ptr->fmv, 0, sizeof(mb_cache_ptr->fmv));
				//memset(mb_cache_ptr->bmv, 0, sizeof(mb_cache_ptr->bmv));
				
	           		((int32 *)(&(mb_cache_ptr->fmv[0])))[0]    = 0;
          			((int32 *)(&(mb_cache_ptr->fmv[1])))[0]    = 0;
          		 	((int32 *)(&(mb_cache_ptr->fmv[2])))[0]    = 0;
           			((int32 *)(&(mb_cache_ptr->fmv[3])))[0]    = 0;

				((int32 *)(&(mb_cache_ptr->bmv[0])))[0]    = 0;
          			((int32 *)(&(mb_cache_ptr->bmv[1])))[0]    = 0;
          		 	((int32 *)(&(mb_cache_ptr->bmv[2])))[0]    = 0;
           			((int32 *)(&(mb_cache_ptr->bmv[3])))[0]    = 0;

				Mp4Dec_StartMcaOneDir_sw(vop_mode_ptr, mb_cache_ptr, &zeroMv,&(FwdMvClipedUV.x),&(FwdMvClipedUV.y));

			}else
			{
				Mp4Dec_DecMBHeaderBVOP_sw(vop_mode_ptr,  mb_mode_bvop_ptr);

				Mp4Dec_MCA_BVOP(vop_mode_ptr, mb_mode_bvop_ptr, &FwdPredMv, &BckPredMv, pCoMb_mode->mv,\
				FwdMvCliped,BckMvCliped,&FwdMvClipedUV,&BckMvClipedUV);

				if(vop_mode_ptr->error_flag)
				{
					PRINTF("decode mv of B-VOP error!\n");
					vop_mode_ptr->return_pos2 |= (1<<0);
					return MMDEC_STREAM_ERROR;
				}
			}

		        Mp4Dec_RecOneMbBvop(vop_mode_ptr,mb_mode_bvop_ptr,FwdMvCliped,BckMvCliped,&FwdMvClipedUV,&BckMvClipedUV);
			if(mb_mode_bvop_ptr->bSkip)
			{	
				mb_mode_bvop_ptr->CBP = 0x00;
			}

			/*vld*/
			if(0x00 != (mb_mode_bvop_ptr->CBP & 0x3f))
			{	
				//vop_mode_ptr->B_CBP1_cnt++;
				Mp4Dec_DecInterMBTexture_sw(vop_mode_ptr, mb_mode_bvop_ptr,mb_cache_ptr);
			}
			else
			{
				//vop_mode_ptr->B_CBP0_cnt++;
			
				//write pMBBfrY[256] pMBBfrU[64] pMBBfrV[64] to REC Frame  @leon
#ifndef _PUT_ASSEMBLY_

				Mp4Dec_WriteRecMB2Frame(vop_mode_ptr, mb_cache_ptr, ppxlcRecMBY, ppxlcRecMBU, ppxlcRecMBV);

#else
				arm_put_recMB_to_frame(vop_mode_ptr, mb_cache_ptr, ppxlcRecMBY, ppxlcRecMBU, ppxlcRecMBV);
#endif
			}
			
			if(!vop_mode_ptr->error_flag)
			{
			}else
			{
				PRINTF ("decode inter mb of B-Vop error!\n");
				vop_mode_ptr->return_pos |= (1<<0);
				return MMDEC_STREAM_ERROR;
			}
				
			pCoMb_mode++;

			ppxlcRecMBY += MB_SIZE;
			ppxlcRecMBU += BLOCK_SIZE;
			ppxlcRecMBV += BLOCK_SIZE;	
			mb_mode_ptr++;	
			vop_mode_ptr->mbnumDec++;	
			pmv += 4;
		}

		ppxlcRecGobY += vop_mode_ptr->FrameExtendWidth * MB_SIZE;
		ppxlcRecGobU += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;
		ppxlcRecGobV += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;
	}
      //  SCI_TRACE_LOW("mb cbp0 :%d, cbp1:%d, total mb %d",vop_mode_ptr->B_CBP0_cnt,\
	//SSS		vop_mode_ptr->B_CBP1_cnt,vop_mode_ptr->B_CBP0_cnt+vop_mode_ptr->B_CBP1_cnt);
#endif

#endif

	  return ret;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
