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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOP_vt
 ** Description:	Decode the IVOP with error resilience enabled. 
 ** Author:			Simon.Wang
 **	Note:			Based on 8800G
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOP_vt(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T *mb_mode_ptr =vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (VSP_ITU_H263 == vop_mode_ptr->video_std)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
	
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

	SCI_TRACE_LOW("Mp4Dec_DecIVOP_vt: E\n");
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{
		vop_mode_ptr->mb_y = (int8)pos_y;

		if(is_itu_h263)
		{
			ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
		}

		/*decode one MB line*/		
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{		
		  	vop_mode_ptr->mb_x = (int8)pos_x;	
			
			vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
			mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;

			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;

				rsc_found = Mp4Dec_SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return MMDEC_STREAM_ERROR;
				}
				else
				{					
					if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
					{
						return MMDEC_ERROR;
					}

					if(is_itu_h263)
					{
						/*decode GOB header*/	
						ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
					}else
					{
						ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, 0);
					}

                    if (vop_mode_ptr->error_flag)
					{
						PRINTF ("decode resync header error!\n");
						continue;
					}
									
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}
			}
			else if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				 mb_mode_ptr->bFirstMB_in_VP = FALSE;

				if(Mp4Dec_CheckResyncMarker(0))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,  0);
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}

				if (vop_mode_ptr->error_flag)
				{
					PRINTF ("decode resync header error!\n");
					continue;
				}
			}

			if((0 == pos_y) && (0 == pos_x))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}
			
			mb_cache_ptr->mb_addr[0] = ppxlcRecGobY + pos_y*(vop_mode_ptr->FrameExtendWidth <<4) + pos_x*MB_SIZE;
			mb_cache_ptr->mb_addr[1] = ppxlcRecGobU + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x*BLOCK_SIZE;
			mb_cache_ptr->mb_addr[2] = ppxlcRecGobV + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x*BLOCK_SIZE;

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);	
			vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
			
			Mp4Dec_DecIntraMBHeader(vop_mode_ptr, mb_mode_ptr);
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF("decode intra mb header error!\n");
				continue;
			}

			Mp4Dec_DecIntraMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;

				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;		
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
			}else
			{
				//Simon.Wang @20120822. The previous MB in the same Gob may be error.
				int k;
				for(k=vop_mode_ptr->mbnumDec-1; k>=pos_y * total_mb_num_x; k--)
				{
					vop_mode_ptr->mbdec_stat_ptr[k] = DECODED_IN_ERR_PKT;	
				}	
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
				PRINTF ("decode intra mb coeff error!\n");
				continue;
			}			
 			
		}
 		vop_mode_ptr->GobNum++;
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOP_vt
 ** Description:	Decode the PVOP with error resilience is enabled. 
 ** Author:			Simon.Wang
 **	Note:			Based on 8800G
 *****************************************************************************/
#ifdef _DEBUG_TIME_
long long DecPVOP_time = 0;
#endif
PUBLIC MMDecRet Mp4Dec_DecPVOP_vt(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T * mb_mode_ptr;
	DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == VSP_ITU_H263)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
#ifdef _DEBUG_TIME_
	struct timeval tpstart,tpend;
       long long  cur_time;
	gettimeofday(&tpstart,NULL);
#endif
  	SCI_TRACE_LOW("Mp4Dec_DecPVOP_vt: E\n");
	Mp4Dec_ExchangeMBMode (vop_mode_ptr);
	mb_mode_ptr = vop_mode_ptr->pMbMode;

    //ref frame 
    vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
    vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];
		
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
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

			vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
			mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
			
			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;
			
				rsc_found = Mp4Dec_SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return MMDEC_STREAM_ERROR;
				}
				else
				{		
					if(is_itu_h263)
					{
						/*decode GOB header*/	
						ret = Mp4Dec_DecGobHeader(vop_mode_ptr);	
					}else
					{
						ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
					}
			
					if (vop_mode_ptr->error_flag)
					{
						PRINTF ("decode resync header error!\n");
						continue;
					}
		
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}
			}
			else if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				mb_mode_ptr->bFirstMB_in_VP = FALSE;
				
				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
							
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					vop_mode_ptr->mbnumDec = pos_y * total_mb_num_x + pos_x;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}	

				if (vop_mode_ptr->error_flag)
				{
					PRINTF ("decode resync header error!\n");
					continue;
				}				
			}

			if((0 == pos_y) && (0 == pos_x))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}
			mb_cache_ptr->mb_addr[0] = ppxlcRecGobY + pos_y*(vop_mode_ptr->FrameExtendWidth <<4) + pos_x*MB_SIZE;
			mb_cache_ptr->mb_addr[1] = ppxlcRecGobU + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x*BLOCK_SIZE;
			mb_cache_ptr->mb_addr[2] = ppxlcRecGobV + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x*BLOCK_SIZE;

			

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
			vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
			
			Mp4Dec_DecInterMBHeader(vop_mode_ptr, mb_mode_ptr);
			Mp4Dec_DecMV_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

			if(mb_mode_ptr->bIntra)
			{	
				Mp4Dec_DecIntraMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
			}else if (mb_mode_ptr->CBP)
			{
				Mp4Dec_DecInterMBTexture_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);				
			}

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;

				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;	
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
			}else
			{
				//Simon.Wang @20120822. The previous MB in the same Gob may be error.
				int k;
				for(k=vop_mode_ptr->mbnumDec-1; k>=pos_y * total_mb_num_x; k--)
				{
					vop_mode_ptr->mbdec_stat_ptr[k] = DECODED_IN_ERR_PKT;	
				}	
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
				PRINTF ("\ndecode inter mb of pVop error!");
				continue;
			}	
			
		}	
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


PUBLIC MMDecRet Mp4Dec_DecBVOP_vt(DEC_VOP_MODE_T * vop_mode_ptr)
{
	MMDecRet ret = MMDEC_OK;
	int32 pos_y, pos_x;
	DEC_MB_MODE_T *mb_mode_bvop_ptr = vop_mode_ptr->pMbMode_B;
	DEC_MB_MODE_T *pCoMb_mode = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdPredMv, BckPredMv, zeroMv = {0, 0};
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon
	SCI_TRACE_LOW("Mp4Dec_DecBVOP_vt: E\n");
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
	

	FwdPredMv = zeroMv;
	BckPredMv = zeroMv;
	
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
		vop_mode_ptr->mb_y = (int8)pos_y;

		//reset prediction mv
		FwdPredMv = zeroMv;
		BckPredMv = zeroMv;

		/*decode one MB line*/
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{
			vop_mode_ptr->mb_x = (int8)pos_x;

			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

					//reset prediction mv
					FwdPredMv = zeroMv; 
					BckPredMv = zeroMv;
				}			
			}

			mb_mode_bvop_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);

			mb_cache_ptr->mb_addr[0] = ppxlcRecGobY + pos_y*(vop_mode_ptr->FrameExtendWidth <<4) + pos_x * MB_SIZE;
			mb_cache_ptr->mb_addr[1] = ppxlcRecGobU + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x * BLOCK_SIZE;
			mb_cache_ptr->mb_addr[2] = ppxlcRecGobV + pos_y*(vop_mode_ptr->FrameExtendWidth <<2) + pos_x * BLOCK_SIZE;
			
			if(MODE_NOT_CODED == pCoMb_mode->dctMd)
			{
				mb_mode_bvop_ptr->bSkip = TRUE;
				Mp4Dec_DecMV_sw(vop_mode_ptr, mb_mode_bvop_ptr,mb_cache_ptr);

			}else
			{
				Mp4Dec_DecMBHeaderBVOP(vop_mode_ptr,  mb_mode_bvop_ptr);

				Mp4Dec_MCA_BVOP_sw(vop_mode_ptr, mb_mode_bvop_ptr, &FwdPredMv, &BckPredMv, pCoMb_mode->mv);

				if(vop_mode_ptr->error_flag)
				{
					PRINTF("decode mv of B-VOP error!\n");
					vop_mode_ptr->return_pos2 |= (1<<0);
					return MMDEC_STREAM_ERROR;
				}
				
				if(mb_mode_bvop_ptr->CBP)
				{
					Mp4Dec_DecInterMBTexture_sw(vop_mode_ptr, mb_mode_bvop_ptr,mb_cache_ptr);
				}
				if(vop_mode_ptr->error_flag)
				{
					PRINTF ("decode inter mb of B-Vop error!\n");
					vop_mode_ptr->return_pos |= (1<<0);
					return MMDEC_STREAM_ERROR;
				}				
			}
				
			pCoMb_mode++;
			vop_mode_ptr->mbnumDec++;	
		}

	}

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
