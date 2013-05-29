/******************************************************************************
 ** File Name:    mp4dec_datapartitioning.c                                   *
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

#ifdef _MP4CODEC_DATA_PARTITION_
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************
 **	Name : 			Mp4Dec_DecSecondPartitionIntraErrRes_vt
 ** Description:	get second part of data partition, IVOP. Software solution
 ** Author:			Xiaowei Luo, Simon Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecSecondPartitionIntraErrRes_vt(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no)
{
	int32 mb_num = start_mb_no;
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
	int32 NextMBLine_Offset;
	
	/*decode MB header in second partition*/
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

	#if _DEBUG_
		if(mb_num == 34)
		{
			mb_num = mb_num;
		}
	#endif //_DEBUG_
		
		Mp4Dec_GetSecondPartitionMBHeaderIVOP(vop_mode_ptr, mb_mode_ptr);
		
		mb_num++;		
	}
		
	/*decode MB texture in second partition*/
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;	
	
	mb_num = start_mb_no;
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

		NextMBLine_Offset = mb_y * vop_mode_ptr->FrameExtendWidth * MB_SIZE;

		mb_cache_ptr->mb_addr[0] = ppxlcRecGobY + mb_x * MB_SIZE + NextMBLine_Offset;
		mb_cache_ptr->mb_addr[1] = ppxlcRecGobU + mb_x * BLOCK_SIZE + NextMBLine_Offset/4;
		mb_cache_ptr->mb_addr[2] = ppxlcRecGobV + mb_x * BLOCK_SIZE + NextMBLine_Offset/4;


	#if _DEBUG_
		if((mb_y == 2)&&(mb_x == 0)&&(vop_mode_ptr->g_nFrame_dec == 0))
		{
			foo();
		}
	#endif //_DEBUG_

		Mp4Dec_DecIntraMBTexture_sw(vd, vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

		if(!vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->err_MB_num--;
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_NOT_IN_ERR_PKT;

#if 1
			if (vop_mode_ptr->err_left != 0)
			{
				int32 mb_end_pos;
				int32 err_start_pos;
				
				/*determine whether the mb is located in error domain*/
				mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;		

				err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;

				if (mb_end_pos >= err_start_pos)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_IN_ERR_PKT;
				}
			}
#endif			
		}else
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = NOT_DECODED;
			PRINTF ("decode intra mb coeff error!\n");
		}

		mb_num++;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecSecondPartitionInterErrRes_vt
 ** Description:	get second part of data partition, PVOP. Software solution
 ** Author:			Xiaowei Luol, Simon Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecSecondPartitionInterErrRes_vt(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no)
{
	int32 mb_num = start_mb_no;
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
	int32 NextMBLine_Offset;
	
#if _DEBUG_
	if((vop_mode_ptr->g_nFrame_dec == 242)&&(vop_mode_ptr->mbnumDec == 86))
	{
		foo();
	}
#endif //_DEBUG_

	/*decode MB header in second partition*/
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
	
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

		if(!mb_mode_ptr->bSkip)
		{
			Mp4Dec_GetSecondPartitionMBHeaderPVOP(vop_mode_ptr, mb_mode_ptr, mb_num);

			if(vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->return_pos |= (1<<9);
				return;
			}
		}
		mb_num++;
	}
		
	/*decode MB texture in second partition*/
	ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
	ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
	ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;	

	//ref frame 
    vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
    vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
	vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];
	
	mb_num = start_mb_no;
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
	
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

		NextMBLine_Offset = mb_y * vop_mode_ptr->FrameExtendWidth * MB_SIZE;

		mb_cache_ptr->mb_addr[0] = ppxlcRecGobY + mb_x * MB_SIZE + NextMBLine_Offset;
		mb_cache_ptr->mb_addr[1] = ppxlcRecGobU + mb_x * BLOCK_SIZE + NextMBLine_Offset/4;
		mb_cache_ptr->mb_addr[2] = ppxlcRecGobV + mb_x * BLOCK_SIZE + NextMBLine_Offset/4;

	#if _DEBUG_
		if((mb_y == 0)&&(mb_x == 0x2)&&(vop_mode_ptr->g_nFrame_dec == 1))
		{
			foo();
		}
	#endif //_DEBUG_
	#if _DEBUG_
		if((mb_num >= 81)&&(vop_mode_ptr->g_nFrame_dec == 9))
		{
			PRINTF("%mb bskip :%d, cbp = %d, bIntra = %d\n", mb_mode_ptr->bSkip, mb_mode_ptr->CBP, mb_mode_ptr->bIntra);
		}
	#endif //_DEBUG_

		if(mb_mode_ptr->bIntra)
		{	
			Mp4Dec_DecIntraMBTexture_sw(vd, vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
		}else
		{
			Mp4Dec_DecInterMBTexture_DP_vt(vd, vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);				
		}


		if(!vop_mode_ptr->error_flag)
		{
			int32 mb_end_pos;
			int32 err_start_pos;

			vop_mode_ptr->err_MB_num--;
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_NOT_IN_ERR_PKT;
#if 1
			if (vop_mode_ptr->err_left != 0)
			{
				/*determine whether the mb is located in error domain*/
				mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;	

				err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;

				if (mb_end_pos >= err_start_pos)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_IN_ERR_PKT;
				}
			}	
#endif			
		}else
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = NOT_DECODED;
			PRINTF ("\ndecode inter mb of pVop error!");
		}	

		mb_num++;	
	}	
}

PUBLIC int32 Mp4Dec_RvlcIntraTCOEF_vt(DEC_VOP_MODE_T *vop_mode_ptr, int16 * iDCTCoefQ, int32 iCoefStart,char *pNonCoeffPos)
{
	int32 i = iCoefStart;
	int32 last = 0;
	TCOEF_T run_level = {0, 0, 0, 0};
	const uint8 *rgiZigzag = vop_mode_ptr->pZigzag; 
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	int32 nonCoeffNum = 0;
	int32 index;
	
	do 
	{
		Mp4Dec_GetIntraRvlcTcoef(vop_mode_ptr, bitstrm_ptr, &run_level);
		
		i += run_level.run;
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos |= (1<<5);
			return nonCoeffNum;
		}
		
		if(i >= 64)
		{
			vop_mode_ptr->return_pos |= (1<<6);
			vop_mode_ptr->error_flag = TRUE;
			PRINTF ("TOO MUCH COEFF !!");
			return nonCoeffNum;
		}

		index = rgiZigzag[i];
		pNonCoeffPos[nonCoeffNum] = index;
		if(run_level.sign == 1)
		{
			iDCTCoefQ [index] = - run_level.level;
		}else
		{
			iDCTCoefQ [index] = run_level.level;
		}
		
		last = run_level.last;
		nonCoeffNum++;
		i++;	
	}while(!last);
	return nonCoeffNum;
}


PUBLIC void Mp4Dec_RvlcInterTCOEF_vt(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoefQ, int32 iQP, DEC_BS_T *pBitstrm)
{
	int32 i = 0, index;
	int32 last = 0;
	TCOEF_T run_level = {0, 0, 0, 0};
	int32 fQuantizer = vop_mode_ptr->QuantizerType;
	int32 qadd, qmul;
	const uint8 *rgiZigzag = vop_mode_ptr->pZigzag; 
	char *piQuantizerMatrix;
	int32 iquant;
	int32 iSum = 0;
	BOOLEAN bCoefQAllZero = TRUE;

	if(fQuantizer == Q_H263)
	{
		qadd = (iQP - 1) | 0x1;
		qmul = iQP << 1;
	}else
	{
		piQuantizerMatrix = vop_mode_ptr->InterQuantizerMatrix;
	}
	
	do 
	{
		Mp4Dec_GetInterRvlcTcoef(vop_mode_ptr, pBitstrm, &run_level);
		
		i += run_level.run;
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos |= (1<<7);
			return;
		}
		
		if(i >= 64)
		{
			vop_mode_ptr->return_pos |= (1<<8);
			vop_mode_ptr->error_flag = TRUE;
			PRINTF ("TOO MUCH COEFF !\n");
			return;
		}

		/*inverse quantization*/
		index = rgiZigzag[i];
		if(fQuantizer == Q_H263)
		{
			iquant = run_level.level * qmul + qadd;
		}else
		{
			iquant =  iQP * (run_level.level * 2 + 1) * piQuantizerMatrix[index] >> 4;

			iSum ^= iquant;
			bCoefQAllZero = FALSE;
		}

		if(run_level.sign == 1)
		{
			iquant = -iquant;
		}

		iDCTCoefQ[index] = iquant;
		
		last = run_level.last;
		i++;		
	}while(!last);

	if(fQuantizer != Q_H263)
	{
		if(!bCoefQAllZero)
		{
			if((iSum & 0x00000001) == 0)
			{
				iDCTCoefQ[63] ^= 0x00000001;
			}
		}
	}
}


#endif //_MP4CODEC_DATA_PARTITION_
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
