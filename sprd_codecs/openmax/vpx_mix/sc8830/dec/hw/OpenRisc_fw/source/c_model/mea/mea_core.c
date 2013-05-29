/******************************************************************************
 ** File Name:    mea_core.c		                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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

int32 s_x_min[4];
int32 s_x_max[4];
int32 s_y_min[4];
int32 s_y_max[4];

#define MEA_FPRINTF	//fprintf

void GetSrcMB_y(int32 FrameWidth,
			  int32 FrameHeight, 
			  int32 mb_x, //mb index in x-direction
			  int32 mb_y  //mb index in y-direction
			  )
{
	int32 i;
	int32 blk_num;
	uint8 *pSrcMBy;
	uint8 *pSrc;
	int32 offset;
	uint8 *pBlock;

	pSrcMBy = (uint8 *)g_mea_src_frame_y + mb_y * MB_SIZE * FrameWidth + mb_x * MB_SIZE;
	
	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		pBlock = g_SrcMB[blk_num];
		offset = (blk_num >> 1) * BLOCK_SIZE * FrameWidth + (blk_num & 1) * BLOCK_SIZE;
		pSrc = pSrcMBy + offset;

		for(i = 0; i < BLOCK_SIZE; i++)
		{
			*pBlock++ = pSrc[0]; *pBlock++ = pSrc[1]; *pBlock++ = pSrc[2]; *pBlock++ = pSrc[3];
			*pBlock++ = pSrc[4]; *pBlock++ = pSrc[5]; *pBlock++ = pSrc[6]; *pBlock++ = pSrc[7];
			pSrc = pSrc + FrameWidth;
		}	
	}
}

void GetSrcMB_uv(int32 FrameWidth,
			  int32 FrameHeight, 
			  int32 mb_x, //mb index in x-direction
			  int32 mb_y  //mb index in y-direction
			  )
{
	int32 i;
	uint8 *pSrcMBuv;
	uint8 *pSrc;
	uint8 *pBlock;
	uint8 *pSrcFrameBfrU, *pSrcFrameBfrV;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 uv_pitch;

	pSrcFrameBfrU = (uint8 *)g_mea_src_frame_u;
	if (!uv_interleaved)
	{
		pSrcFrameBfrV = (uint8 *)g_mea_src_frame_v;
		uv_pitch = 1;
	}else
	{
		pSrcFrameBfrV = pSrcFrameBfrU+1;
		uv_pitch = 2;
	}

	pSrcMBuv = pSrcFrameBfrU + mb_y * BLOCK_SIZE * (FrameWidth/2)*uv_pitch + mb_x * BLOCK_SIZE*uv_pitch;

	pBlock = g_SrcMB[4];
	pSrc = pSrcMBuv;
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		*pBlock++ = pSrc[0*uv_pitch]; *pBlock++ = pSrc[1*uv_pitch]; 
		*pBlock++ = pSrc[2*uv_pitch]; *pBlock++ = pSrc[3*uv_pitch];
		*pBlock++ = pSrc[4*uv_pitch]; *pBlock++ = pSrc[5*uv_pitch]; 
		*pBlock++ = pSrc[6*uv_pitch]; *pBlock++ = pSrc[7*uv_pitch];
		pSrc = pSrc + (FrameWidth/2)*uv_pitch;
	}
	
	pSrcMBuv = pSrcFrameBfrV + mb_y * BLOCK_SIZE * (FrameWidth/2)*uv_pitch + mb_x * BLOCK_SIZE*uv_pitch;

	pBlock = g_SrcMB[5];
	pSrc = pSrcMBuv;
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		*pBlock++ = pSrc[0*uv_pitch]; *pBlock++ = pSrc[1*uv_pitch]; 
		*pBlock++ = pSrc[2*uv_pitch]; *pBlock++ = pSrc[3*uv_pitch];
		*pBlock++ = pSrc[4*uv_pitch]; *pBlock++ = pSrc[5*uv_pitch]; 
		*pBlock++ = pSrc[6*uv_pitch]; *pBlock++ = pSrc[7*uv_pitch];
		pSrc = pSrc + (FrameWidth/2)*uv_pitch;
	}
}

void GetRefBlk_y(uint8 *pRefFrameBfr,
			  int32 FrameWidth,
			  int32 FrameHeight, 
			  int32 ref_x,   //ref block top-left corner position 
			  int32 ref_y,
			  int32 blk_num)
{
	int32 i,j;
	uint8 *pBlock; 
	int32 iCorX, iCorY;
	
	pBlock = g_RefMB[blk_num];

	for(i = 0; i < BLOCK_SIZE; i++)
	{
		iCorY = ref_y + i;
		iCorY = IClip(0, FrameHeight - 1, iCorY);

		for(j = 0; j < BLOCK_SIZE; j++)
		{
			iCorX = ref_x + j;

			iCorX = IClip(0, FrameWidth - 1, iCorX);

			*pBlock++ = pRefFrameBfr[iCorY * FrameWidth + iCorX];
		}
	}
}

void GetErrBlk_y(int32 blk_num)
{
	int32 i;
	uint8 *pSrcBlk;
	uint8 *pRefBlk;
	int16 *pErrBlk;

	pSrcBlk = g_SrcMB[blk_num];
	pRefBlk = g_RefMB[blk_num];
	pErrBlk = g_ErrMB[blk_num];

	for(i= 0; i < (BLOCK_SIZE * BLOCK_SIZE); i++)
	{
		*pErrBlk++ = (int16)(*pSrcBlk++) - (int16)(*pRefBlk++);
	}
}

int32 ComputeBlkSad(int32 blk_num)
{
	int32 sad;
	int32 i;
	int16 *pErrBlk;

	sad = 0;

	pErrBlk = g_ErrMB[blk_num];

	for(i = 0; i <(BLOCK_SIZE * BLOCK_SIZE); i++)
	{
		sad += abs(*pErrBlk++);
	}

	return sad;
}
void GetRefMB_y(uint8 *pRefFrameBfr,
			  int32 FrameWidth,
			  int32 FrameHeight, 
			  int32 ref_x,   //ref mb top-left corner position 
			  int32 ref_y)
{
	int32 blk_num;
	int32 ref_blk_x, ref_blk_y;

	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		switch(blk_num)
		{
		case 0:
			ref_blk_x = ref_x;
			ref_blk_y = ref_y;
			break;
		case 1:
			ref_blk_x = ref_x + BLOCK_SIZE;
			ref_blk_y = ref_y;
			break;
		case 2:
			ref_blk_x = ref_x;
			ref_blk_y = ref_y + BLOCK_SIZE;
			break;
		case 3:
			ref_blk_x = ref_x + BLOCK_SIZE;
			ref_blk_y = ref_y + BLOCK_SIZE;
			break;
		default:
			printf(" illegal block number value! \n");
			break;
		}

		GetRefBlk_y(pRefFrameBfr, FrameWidth, FrameHeight, ref_blk_x, ref_blk_y, blk_num);
	}
}

void GetErrMB_y(void)
{
	int32 blk_num;	

	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		GetErrBlk_y(blk_num);
	}
}

void ConverRefMB_y(uint8 *pMB_y)
{
	int32 blk_num;
	const int32 s_BlkOffset[] = {0, 8, 128, 136};
	uint8 *pSrc;
	uint8 *pDst;
	int32 i;

	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		pSrc = pMB_y + s_BlkOffset[blk_num];
		pDst = g_RefMB[blk_num];

		for(i = 0; i < BLOCK_SIZE; i++)
		{
			pDst[0] = pSrc[0]; pDst[1] = pSrc[1]; pDst[2] = pSrc[2]; pDst[3] = pSrc[3];
			pDst[4] = pSrc[4]; pDst[5] = pSrc[5]; pDst[6] = pSrc[6]; pDst[7] = pSrc[7];

			pSrc += MB_SIZE;
			pDst += BLOCK_SIZE;
		}
	}
}

void ConverFinalRefMB_y(uint8 *pMB_y)
{
	int32 blk_num;
	const int32 s_BlkOffset[] = {0, 8, 128, 136};
	uint8 *pSrc;
	uint8 *pDst;
	int32 i;

	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		pSrc = pMB_y + s_BlkOffset[blk_num];
		pDst = g_FinalRefMB[blk_num];

		for(i = 0; i < BLOCK_SIZE; i++)
		{
			pDst[0] = pSrc[0]; pDst[1] = pSrc[1]; pDst[2] = pSrc[2]; pDst[3] = pSrc[3];
			pDst[4] = pSrc[4]; pDst[5] = pSrc[5]; pDst[6] = pSrc[6]; pDst[7] = pSrc[7];

			pSrc += MB_SIZE;
			pDst += BLOCK_SIZE;
		}
	}
}


int32 ComputeMBSad(void)
{
	int32 sad;
	int32 blk_num;	

	sad = 0;

	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		sad += ComputeBlkSad(blk_num);
	}

	return sad;
}

void SetSearchRange(int32 mb_x, //mb index in x-direction
				  int32 mb_y  //mb index in y-direction)
				  )
{
	
	int32 center_x;
	int32 center_y;
	int32 max_mv_x = ((g_mea_reg_ptr->MEA_CFG0 >> 0) & 0x1f);
	int32 max_mv_y = ((g_mea_reg_ptr->MEA_CFG0 >> 8) & 0x1f);

	center_x = (mb_x + 0) * MB_SIZE;
	center_y = (mb_y + 0) * MB_SIZE;

	//block 0
	s_x_min[0] = center_x - max_mv_x;
	s_x_max[0] = center_x + max_mv_x;
	s_y_min[0] = center_y - max_mv_y;
	s_y_max[0] = center_y + max_mv_y;

	//block 1
	s_x_min[1] = s_x_min[0] + BLOCK_SIZE;
	s_x_max[1] = s_x_max[0] + BLOCK_SIZE; 
	s_y_min[1] = s_y_min[0];
	s_y_max[1] = s_y_max[0];

	//block 2
	s_x_min[2] = s_x_min[0];
	s_x_max[2] = s_x_max[0]; 
	s_y_min[2] = s_y_min[0] + BLOCK_SIZE;
	s_y_max[2] = s_y_max[0] + BLOCK_SIZE;

	//block 3
	s_x_min[3] = s_x_min[0] + BLOCK_SIZE;
	s_x_max[3] = s_x_max[0] + BLOCK_SIZE; 
	s_y_min[3] = s_y_min[0] + BLOCK_SIZE;
	s_y_max[3] = s_y_max[0] + BLOCK_SIZE;
}

int32 ComputerMBA(void)
{
	int32 i;
	int32 blk_num;
	int32 mb_mean;
	uint8 *pSrcBlk;
	int32 A;

	A = 0;
	mb_mean = 0;

	for(blk_num = 0; blk_num < (BLOCK_CNT -2); blk_num++)
	{
		pSrcBlk = g_SrcMB[blk_num];

		for(i = 0; i < (BLOCK_SIZE * BLOCK_SIZE); i++)
		{
			mb_mean += (*pSrcBlk++);
		}
	}

	mb_mean /= 256;

	for(blk_num = 0; blk_num < (BLOCK_CNT -2); blk_num++)
	{
		pSrcBlk = g_SrcMB[blk_num];

		for(i = 0; i < (BLOCK_SIZE * BLOCK_SIZE); i++)
		{
			A += abs((*pSrcBlk++) - mb_mean);
		}
	}

	return A;
}

void GetFinalBlk_y(int32 blk_num)
{
	int32 i;
	uint8 *pRefBlk;
	uint8 *pFinalRefBlk;

	pRefBlk = g_RefMB[blk_num];
	pFinalRefBlk = g_FinalRefMB[blk_num];

	for(i= 0; i < (BLOCK_SIZE * BLOCK_SIZE); i++)
	{
		*pFinalRefBlk++ = (*pRefBlk++);
	}
}

void GetFinalRefMB_y(uint8 *pRefFrameBfr,
					int32 FrameWidth, 
					int32 FrameHeight,
					int32 mb_x, //mb index in x-direction
					int32 mb_y,  //mb index in y-direction
					MB_MODE_E MB_Mode,
					MOTION_VECTOR_T *pMv
					)
{
	int32 cor_x, cor_y;
	int32 dx, dy;
	int32 start_x;
	int32 start_y;
	int32 rounding_control; 
	uint8 *pBlock; 

	rounding_control = 0;

	if(MB_Mode == INTER)
	{
		uint8 MB_y[64*4]; //16x16 array

		cor_x = mb_x * MB_SIZE * 2 + pMv->x;
		cor_y = mb_y * MB_SIZE * 2 + pMv->y;

		dx = cor_x & 1;
		dy = cor_y & 1;
		start_x = cor_x >> 1;
		start_y = cor_y >> 1;

		g_dec_mc_16x16[(dx<<1) | dy](pRefFrameBfr, MB_y, rounding_control, FrameWidth, FrameHeight, start_x, start_y);

		//conver 16x16 array to MB[4][64]
		ConverFinalRefMB_y(MB_y);
	}else /*4MV*/
	{
		int32 blk_num;
		int32 xoffset, yoffset;
		
		for(blk_num = 0; blk_num < (BLOCK_CNT -2); blk_num++)
		{		
			xoffset = (blk_num & 1) * 8;
			yoffset = (blk_num >> 1) * 8;

			cor_x = mb_x * MB_SIZE * 2 + xoffset*2 + pMv[blk_num + 1].x;
			cor_y = mb_y * MB_SIZE * 2 + yoffset*2 + pMv[blk_num + 1].y;

			dx = cor_x & 1;
			dy = cor_y & 1;
			start_x = cor_x >> 1;
			start_y = cor_y >> 1;

			pBlock = g_FinalRefMB[blk_num];

			g_dec_mc_8x8_me[(dx<<1) | dy](pRefFrameBfr, pBlock, rounding_control, FrameWidth, FrameHeight, BLOCK_SIZE, start_x, start_y);	
		}
	}
}

void GetFinalRefMB_uv(uint8 *pRefFrameBfr,
					int32 FrameWidth, 
					int32 FrameHeight,
					int32 mb_x, //mb index in x-direction
					int32 mb_y,  //mb index in y-direction
					MOTION_VECTOR_T *pMv
					)
{
	int32 ref_blk_center_x, ref_blk_center_y;
	int32 dx, dy;
	int32 start_x;
	int32 start_y;
	int32 rounding_control; 
	uint8 *pBlock; 
	uint8 *pRefFrameBfrU, *pRefFrameBfrV;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);

	pRefFrameBfrU = (uint8 *)g_mea_ref_frame_u;

	if (!uv_interleaved)
	{
		pRefFrameBfrV = (uint8 *)g_mea_ref_frame_v;
	}else
	{
		pRefFrameBfrV = pRefFrameBfrU + 1;
	}
	
	rounding_control = 0;

	ref_blk_center_x = mb_x * BLOCK_SIZE * 2 + pMv->x;
	ref_blk_center_y = mb_y * BLOCK_SIZE * 2 + pMv->y;

	dx = ref_blk_center_x & 1;
	dy = ref_blk_center_y & 1;
	start_x = ref_blk_center_x >> 1;
	start_y = ref_blk_center_y >> 1;

	pBlock = g_FinalRefMB[4];
	g_dec_mc_8x8[(dx<<1) | dy](pRefFrameBfrU, pBlock, rounding_control, FrameWidth/2, FrameHeight/2, BLOCK_SIZE, start_x, start_y);	

	if (!uv_interleaved)
	{
		pBlock = g_FinalRefMB[5];
		g_dec_mc_8x8[(dx<<1) | dy](pRefFrameBfrV, pBlock, rounding_control, FrameWidth/2, FrameHeight/2, BLOCK_SIZE, start_x, start_y);	
	}
	return;
}

void Get12x9UVBlockInRefFrame(uint8 *pRefFrameBfr,
					int32 FrameWidth, 
					int32 FrameHeight,
					int32 mb_x, //mb index in x-direction
					int32 mb_y,  //mb index in y-direction
					MOTION_VECTOR_T *pMv)
{
	int i, j;
	int iXcor, iYcor;
	int xRef, yRef;
	int min_x = 0, max_x = FrameWidth/2 - 1;
	int min_y = 0, max_y = FrameHeight/2 - 1;
	uint8 *pRefFrameBfrU, *pRefFrameBfrV;
	int32 size;
	int32 mb_total_num_x;
	int k;
	uint8 *pRefFrame = NULL;
					
	iXcor = mb_x * BLOCK_SIZE * 2 + pMv->x;
	iYcor = mb_y * BLOCK_SIZE * 2 + pMv->y;
	
	size = FrameWidth * FrameHeight;
	pRefFrameBfrU = pRefFrameBfr + size;
	pRefFrameBfrV = pRefFrameBfrU + (size>>2);

	mb_total_num_x = (FrameWidth + 15)/MB_SIZE;

	MEA_FPRINTF(g_p12x9UVBlockFile, "------No.%d (mb_x = %d, mb_y = %d)----\n", mb_x + mb_y * mb_total_num_x, mb_x, mb_y);

	for(k = 0; k < 2; k++)
	{			
		pRefFrame = pRefFrameBfrU;

		if(k == 0)
		{
			MEA_FPRINTF(g_p12x9UVBlockFile, "------U block----\n" );
		}else
		{
			MEA_FPRINTF(g_p12x9UVBlockFile, "------V block----\n" );
			pRefFrame = pRefFrameBfrV;
		}

		yRef = iYcor >> 1;

		if((yRef + 9) < min_y)
		{
			for(i = 0; i < 8; i++)
			{
				for(j = 0; j < 12; j++)
				{
					MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");	
				}
				MEA_FPRINTF(g_p12x9UVBlockFile, "\n");
			}

			xRef = iXcor >> 1;
			xRef = (xRef >> 2) << 2;   //word align

			for(j = 0; j < 12; j++)
			{
				xRef = IClip(min_x, max_x, xRef);
				MEA_FPRINTF(g_p12x9UVBlockFile, "%02x\t", pRefFrame[FrameWidth/2 * min_y + xRef]);
				xRef++;
			}
			MEA_FPRINTF(g_p12x9UVBlockFile, "\n");		
		}else if(yRef > max_y)
		{
			xRef = iXcor >> 1;
			xRef = (xRef >> 2) << 2;   //word align

			for(j = 0; j < 12; j++)
			{
				xRef = IClip(min_x, max_x, xRef);
				MEA_FPRINTF(g_p12x9UVBlockFile, "%02x\t", pRefFrame[FrameWidth/2 * max_y + xRef]);
				xRef++;
			}
			MEA_FPRINTF(g_p12x9UVBlockFile, "\n");	
			
			for(i = 0; i < 8; i++)
			{
				for(j = 0; j < 12; j++)
				{
					MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");	
				}
				MEA_FPRINTF(g_p12x9UVBlockFile, "\n");
			}
		}else
		{
			xRef = iXcor >> 1;
			xRef = (xRef >> 2) << 2;   //word align

			if((xRef + 12) < min_x)
			{
				for(i = 0; i < 9; i++)
				{
					for(j = 0; j < 11; j++)
					{
						MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");	
					}
					yRef = IClip(min_y,max_y, yRef);
					MEA_FPRINTF(g_p12x9UVBlockFile, "%02x\t", pRefFrame[FrameWidth/2 * yRef + min_x]);
					yRef++;
				}
			}else if(xRef > max_x)
			{
				for(i = 0; i < 9; i++)
				{				
					yRef = IClip(min_y,max_y, yRef);
					MEA_FPRINTF(g_p12x9UVBlockFile, "%02x\t", pRefFrame[FrameWidth/2 * yRef + max_x]);

					for(j = 0; j < 11; j++)
					{
						MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");	
					}
					yRef++;
				}
			}else
			{
				for(i = 0; i < 9; i++)
				{
					if((yRef < min_y) ||(yRef > max_y)) //outrange at y-direction
					{
						for(j = 0; j < 12; j++)
						{
							MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");					
						}
						MEA_FPRINTF(g_p12x9UVBlockFile, "\n");
					}else
					{
						xRef = iXcor >> 1;
						xRef = (xRef >> 2) << 2;   //word align

						for(j = 0; j < 12; j++)
						{
							if((xRef < min_x) ||(xRef > max_x)) //outrange at x-direction
							{
								MEA_FPRINTF(g_p12x9UVBlockFile, "X \t");					
							}else
							{
								MEA_FPRINTF(g_p12x9UVBlockFile, "%02x\t", pRefFrame[FrameWidth/2 * yRef + xRef]);					
							}

							xRef++;
						}
						MEA_FPRINTF(g_p12x9UVBlockFile, "\n");
					}

					yRef++;
				}
			}			
		}
	}
}

void WriteFinalRefMB(uint8 *pFinalRefFrameBfr, 
					 int32 FrameWidth, 
					 int32 FrameHeight, 
					 int32 mb_x, 
					 int32 mb_y
					 )
{
	int32 i;
	int32 blk_num;
	uint8 *pFinalRefMBy;
	uint8 *pRef;
	int32 offset;
	uint8 *pBlock;
	uint8 *pFinalRefBfrU, *pFinalRefBfrV;
	uint8 *pFinalRefMBuv;
	int32 size;

	pFinalRefMBy = pFinalRefFrameBfr + mb_y * MB_SIZE * FrameWidth + mb_x * MB_SIZE;
	
	for(blk_num = 0; blk_num < (BLOCK_CNT - 2); blk_num++)
	{
		pBlock = g_FinalRefMB[blk_num];
		offset = (blk_num >> 1) * BLOCK_SIZE * FrameWidth + (blk_num & 1) * BLOCK_SIZE;
		pRef = pFinalRefMBy + offset;

		for(i = 0; i < BLOCK_SIZE; i++)
		{
			pRef[0] = *pBlock++; pRef[1] = *pBlock++; pRef[2] = *pBlock++; pRef[3] = *pBlock++;
			pRef[4] = *pBlock++; pRef[5] = *pBlock++; pRef[6] = *pBlock++; pRef[7] = *pBlock++;
			pRef = pRef + FrameWidth;
		}	
	}

	size = FrameWidth * FrameHeight;
	pFinalRefBfrU = pFinalRefFrameBfr + size;
	pFinalRefBfrV = pFinalRefBfrU + (size>>2);

	pFinalRefMBuv = pFinalRefBfrU + mb_y * BLOCK_SIZE * (FrameWidth/2) + mb_x * BLOCK_SIZE;

	pBlock = g_FinalRefMB[4];
	pRef = pFinalRefMBuv;
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		pRef[0] = *pBlock++; pRef[1] = *pBlock++; pRef[2] = *pBlock++; pRef[3] = *pBlock++;
		pRef[4] = *pBlock++; pRef[5] = *pBlock++; pRef[6] = *pBlock++; pRef[7] = *pBlock++;
		pRef = pRef + (FrameWidth/2);
	}
	
	pFinalRefMBuv = pFinalRefBfrV + mb_y * BLOCK_SIZE * (FrameWidth/2) + mb_x * BLOCK_SIZE;

	pBlock = g_FinalRefMB[5];
	pRef = pFinalRefMBuv;
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		pRef[0] = *pBlock++; pRef[1] = *pBlock++; pRef[2] = *pBlock++; pRef[3] = *pBlock++;
		pRef[4] = *pBlock++; pRef[5] = *pBlock++; pRef[6] = *pBlock++; pRef[7] = *pBlock++;
		pRef = pRef + (FrameWidth/2);
	}
}

int32 GetMinSad_LDSP(SAD_SEARCH_T sad_search_array[9])
{
	int32 min_direction;
	
	//compare step 1
	if(sad_search_array[8].sad > sad_search_array[7].sad)
	{
		sad_search_array[8].x_offset = sad_search_array[7].x_offset;
		sad_search_array[8].y_offset = sad_search_array[7].y_offset;
		sad_search_array[8].sad = sad_search_array[7].sad;
	}

	if(sad_search_array[6].sad > sad_search_array[5].sad)
	{
		sad_search_array[6].x_offset = sad_search_array[5].x_offset;
		sad_search_array[6].y_offset = sad_search_array[5].y_offset;
		sad_search_array[6].sad = sad_search_array[5].sad;
	}
		
	if(sad_search_array[4].sad > sad_search_array[3].sad)
	{
		sad_search_array[4].x_offset = sad_search_array[3].x_offset;
		sad_search_array[4].y_offset = sad_search_array[3].y_offset;
		sad_search_array[4].sad = sad_search_array[3].sad;
	}

	if(sad_search_array[2].sad > sad_search_array[1].sad)
	{
		sad_search_array[2].x_offset = sad_search_array[1].x_offset;
		sad_search_array[2].y_offset = sad_search_array[1].y_offset;
		sad_search_array[2].sad = sad_search_array[1].sad;
	}

	//compare step 2
	if(sad_search_array[8].sad > sad_search_array[6].sad)
	{
		sad_search_array[8].x_offset = sad_search_array[6].x_offset;
		sad_search_array[8].y_offset = sad_search_array[6].y_offset;
		sad_search_array[8].sad = sad_search_array[6].sad;
	}

	if(sad_search_array[4].sad > sad_search_array[2].sad)
	{
		sad_search_array[4].x_offset = sad_search_array[2].x_offset;
		sad_search_array[4].y_offset = sad_search_array[2].y_offset;
		sad_search_array[4].sad = sad_search_array[2].sad;
	}

	//compare step 3
	if(sad_search_array[8].sad > sad_search_array[4].sad)
	{
		sad_search_array[8].x_offset = sad_search_array[4].x_offset;
		sad_search_array[8].y_offset = sad_search_array[4].y_offset;
		sad_search_array[8].sad = sad_search_array[4].sad;
	}

	//compare step 4
	if(sad_search_array[8].sad >= sad_search_array[0].sad)
	{//中心点最小
		sad_search_array[8].x_offset = sad_search_array[0].x_offset;
		sad_search_array[8].y_offset = sad_search_array[0].y_offset;
		sad_search_array[8].sad = sad_search_array[0].sad;

		min_direction = 0;
	}else
	{
		min_direction = -1; //不可能的数
	}

	return min_direction;
}

void GetMinSad_SDSP(SAD_SEARCH_T sad_search_array[5])
{
	//compare step 1
	if(sad_search_array[4].sad > sad_search_array[3].sad)
	{
		sad_search_array[4].x_offset = sad_search_array[3].x_offset;
		sad_search_array[4].y_offset = sad_search_array[3].y_offset;
		sad_search_array[4].sad = sad_search_array[3].sad;
	}

	if(sad_search_array[2].sad > sad_search_array[1].sad)
	{
		sad_search_array[2].x_offset = sad_search_array[1].x_offset;
		sad_search_array[2].y_offset = sad_search_array[1].y_offset;
		sad_search_array[2].sad = sad_search_array[1].sad;
	}

	//compare step 2
	if(sad_search_array[4].sad > sad_search_array[2].sad)
	{
		sad_search_array[4].x_offset = sad_search_array[2].x_offset;
		sad_search_array[4].y_offset = sad_search_array[2].y_offset;
		sad_search_array[4].sad = sad_search_array[2].sad;
	}

	//compare step 3
	if(sad_search_array[0].sad > sad_search_array[4].sad)
	{
		sad_search_array[0].x_offset = sad_search_array[4].x_offset;
		sad_search_array[0].y_offset = sad_search_array[4].y_offset;
		sad_search_array[0].sad = sad_search_array[4].sad;
	}
}

int8 ChangChar2Num(char ch)
{
	int8 num;

	if((ch >= 'a') && (ch <= 'f'))
	{
		ch = ch - 0x20; //转换成大写字母
	}

	if((ch >= '0') && (ch <= '9'))
	{
		num = ch - '0';
	}else
	{
		num = ch - 'A' + 10;
	}

	return num;
}

//#define MV_COST_TEST

#if defined(MV_COST_TEST)
#define LAMBDA 0.85
#define	FCODE1

int  GetMVD_len(int32 diff_mv_component)
{
	int    vlc, residual, len;
	int    mvtab[33][2] =
	{
		{1,1}, {1,2}, {1,3}, {1,4}, {3,6}, {5,7}, {4,7}, {3,7},	{11,9}, {10,9}, 
		{9,9}, {17,10}, {16,10}, {15,10}, {14,10}, {13,10},	{12,10}, {11,10}, 
		{10,10}, {9,10}, {8,10}, {7,10}, {6,10}, {5,10},{4,10}, {7,11}, {6,11}, 
	{5,11}, {4,11}, {3,11}, {2,11}, {3,12},	{2,12}
	};
#ifndef  FCODE1
	int abs_diff_mv_component, vlc_magnitude;
	if(diff_mv_component == 0)	
	{									
		vlc = 0;
		residual = 0;
	}else
	{														
		abs_diff_mv_component = abs(diff_mv_component);
		residual = (abs_diff_mv_component - 1) % 2;																	
		vlc_magnitude = (abs_diff_mv_component - residual + 1) / 2;	
		vlc = vlc_magnitude * ssign(diff_mv_component);
	}
	
	len = mvtab[abs(vlc)][1];
	len += (vlc != 0) ? 2 : 0; //sign + residual
	return len;
#else
								
	vlc = diff_mv_component;
	residual = 0;

	if(vlc < -32)
	{
		vlc += 64;
	}else if(vlc > 31)
	{
		vlc -= 64;
	}


	len = mvtab[abs(vlc)][1];
	len += (vlc != 0) ? 1 : 0; //sign
	return len;
#endif	
}
#endif //MV_COST_TEST


//    *1
//  *2 | *3
//* 4--*0--*5
//  *6 | *7
//    *8
static int32 s_LDSP_OffsetXY[9][2] = { {0, 0}, {0, -2}, {-1, -1}, {1, -1}, {-2, 0}, {2, 0}, {-1, 1}, {1, 1}, {0, 2}};

//   *1
//*2 *0 *3
//   *4
static int32 s_SDSP_OffsetXY[5][2] = { {0, 0}, {0, -1}, {-1, 0}, {1, 0}, {0, 1}};

// *1   *2
//   *0   
// *3   *4
static int32  s_SDSP_Fake_OffsetXY[5][2] = { {0, 0}, {-1, -1}, {1, -1}, {-1, }, {1, 1}};

int32 GetSad16x16_FullPixel_DSMethod(uint8 *pRefFrameBfr,
									int32 FrameWidth, 
									int32 FrameHeight,
									int32 center_x,
									int32 center_y,
									int32 min_sad,
									MOTION_VECTOR_T *pMv16x16,
									BOOLEAN *pbLittle_than_thres
								)
{
	int32 cor_x, cor_y;
	int32 sad;
	int32 i;
	int32 min_direction;
	int32 curr_center_x, curr_center_y;
	int32 next_center_x, next_center_y;
	int32 dst_x, dst_y;
	int32 search_cycle;
	SAD_SEARCH_T sad_search_array[9];
	int32 max_search_cycle	= (g_mea_reg_ptr->MEA_CFG1 >> 8)  & 0xff;
	int32 mb_sad_threshold	= (g_mea_reg_ptr->MEA_CFG1 >> 16) & 0xffff;
#if defined(MV_COST_TEST)
	int32 MEA_CFG3 = (int32)(g_mea_reg_ptr->MEA_CFG3); 
	MOTION_VECTOR_T predMV;
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
	int32 mb_qp = vop_mode_ptr->StepSize;
	int32 mvd_cost = (int)sqrt(LAMBDA*mb_qp*mb_qp);

	predMV.x = (int16)((MEA_CFG3 << 24)>>24);
	predMV.y = (int16)((MEA_CFG3 <<16)>>24);

	predMV.x = (predMV.x&0xFFFE);
	predMV.y = (predMV.y&0xFFFE);
#endif

	//step 1, initialize current center and dst
	curr_center_x = center_x;
	curr_center_y = center_y;
	dst_x = curr_center_x;
	dst_y = curr_center_y;
	next_center_x = curr_center_x;
	next_center_y = curr_center_y;

	//step 2, large diamond search pattern
	search_cycle = 0;
	MEA_FPRINTF(g_fp_trace_mea_sad, "16x16 large diamond search pattern\n");
	while(1)
	{
		if(search_cycle >= max_search_cycle)
		{
			MEA_FPRINTF(g_fp_trace_mea_sad, "Didnot find the center-point whose sad is minimum!\n");
			goto DiamondSearchEnd;
		}
		
		min_direction = 0;

		sad_search_array[0].x_offset = 0;
		sad_search_array[0].y_offset = 0;
		sad_search_array[0].sad = min_sad;
			
		for(i = 1; i < 9; i++)
		{
			cor_x = curr_center_x + s_LDSP_OffsetXY[i][0];
			cor_y = curr_center_y + s_LDSP_OffsetXY[i][1];
			g_search_point_cnt++;
		
			if((cor_x < s_x_min[0]) ||
			   (cor_x > s_x_max[0]) ||
			   (cor_y < s_y_min[0]) ||
			   (cor_y > s_y_max[0]))
			{
				sad = MAX_SAD;
			}else
			{
				GetRefMB_y(pRefFrameBfr, FrameWidth, FrameHeight, cor_x, cor_y);
				GetErrMB_y();
				sad = ComputeMBSad();
				
			#if defined(MV_COST_TEST)
				{
					int32 mvdx = (cor_x - vop_mode_ptr->mb_x*MB_SIZE) * 2 - predMV.x;
					int32 mvdy = (cor_y - vop_mode_ptr->mb_y*MB_SIZE) * 2 - predMV.y;
					sad += mvd_cost * (GetMVD_len(mvdx) + GetMVD_len(mvdy));
				}	
			#endif
			}

			sad_search_array[i].x_offset = s_LDSP_OffsetXY[i][0];
			sad_search_array[i].y_offset = s_LDSP_OffsetXY[i][1];
			sad_search_array[i].sad = sad;

			MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_LDSP_OffsetXY[i][0], s_LDSP_OffsetXY[i][1], sad);
			MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad);
						
			if(sad < mb_sad_threshold)
			{
				dst_x = cor_x;
				dst_y = cor_y; 
				min_sad = sad;
				*pbLittle_than_thres = TRUE;

				MEA_FPRINTF(g_fp_trace_mea_sad, "sad is little than threshold!\n");
				
				goto DiamondSearchEnd;
			}
		}
		
		search_cycle++;

		min_direction = GetMinSad_LDSP(sad_search_array);

		min_sad = sad_search_array[8].sad;
		next_center_x = curr_center_x + sad_search_array[8].x_offset;
		next_center_y = curr_center_y + sad_search_array[8].y_offset;

		/*compute the new start address*/
		curr_center_x = next_center_x;
		curr_center_y = next_center_y;	

#if 1 //for out_frame_clip of h.263 baseline
		{
			int32 standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
			if (standard == ITU_H263 || standard == FLV_H263)
			{
				int32 mb_pos_x = (g_mea_reg_ptr->MEA_CFG2 >> 0) & 0xff;
				int32 mb_pos_y = (g_mea_reg_ptr->MEA_CFG2 >> 16) & 0xff;
				int32 mb_num_x = ((g_glb_reg_ptr->VSP_CFG1 >> 0) & 0x1ff);
				int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);

				/* checking in integral-pixel precise */
				//modify mv
				if ((curr_center_x < 0) || (curr_center_x > ((mb_num_x - 1) * MB_SIZE)))
				{
					curr_center_x = mb_pos_x * MB_SIZE;
				}

				if ((curr_center_y < 0) || (curr_center_y > ((mb_num_y - 1) * MB_SIZE)))
				{
					curr_center_y = mb_pos_y * MB_SIZE;
				}
			}
		}
#endif

		/*the position of min sad in current layer */
		dst_x = curr_center_x;
		dst_y = curr_center_y;

		/*if the center has minimum sad, then do small diamond search*/
		if((min_direction == 0))
		{
			goto LDSPEnd;
		}		
	}

LDSPEnd:
	
	sad_search_array[0].x_offset = 0;
	sad_search_array[0].y_offset = 0;
	sad_search_array[0].sad = sad_search_array[8].sad;

	//step 2, small diamond search pattern
	MEA_FPRINTF(g_fp_trace_mea_sad, "16x16 small diamond search pattern\n");
	for(i = 1; i < 5; i++)
	{
		cor_x = curr_center_x + s_SDSP_OffsetXY[i][0];
		cor_y = curr_center_y + s_SDSP_OffsetXY[i][1];
		g_search_point_cnt++;

		if((cor_x < s_x_min[0]) ||
		   (cor_x > s_x_max[0]) ||
		   (cor_y < s_y_min[0]) ||
		   (cor_y > s_y_max[0]))
		{
			sad = MAX_SAD;
		}else
		{
			GetRefMB_y(pRefFrameBfr, FrameWidth, FrameHeight, cor_x, cor_y);
			GetErrMB_y();
			sad = ComputeMBSad();
		#if defined(MV_COST_TEST)
			{
				int32 mvdx = (cor_x - vop_mode_ptr->mb_x*MB_SIZE) * 2 - predMV.x;
				int32 mvdy = (cor_y - vop_mode_ptr->mb_y*MB_SIZE) * 2 - predMV.y;
				sad += mvd_cost * (GetMVD_len(mvdx) + GetMVD_len(mvdy));
			}	
		#endif
		}
	
		sad_search_array[i].x_offset = s_SDSP_OffsetXY[i][0];
		sad_search_array[i].y_offset = s_SDSP_OffsetXY[i][1];
		sad_search_array[i].sad = sad;

		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_SDSP_OffsetXY[i][0], s_SDSP_OffsetXY[i][1], sad);
		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_SDSP_Fake_OffsetXY[i][0], s_SDSP_Fake_OffsetXY[i][1], 0xffff);
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad);
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", 0xffff);
		
		if(sad < mb_sad_threshold)
		{
			dst_x = cor_x;
			dst_y = cor_y; 
			min_sad = sad;
			*pbLittle_than_thres = TRUE;

			MEA_FPRINTF(g_fp_trace_mea_sad, "sad is little than threshold!\n");
			
			goto DiamondSearchEnd;
		}
	}

	GetMinSad_SDSP(sad_search_array);

	min_sad = sad_search_array[0].sad;
	dst_x = curr_center_x + sad_search_array[0].x_offset;
	dst_y = curr_center_y + sad_search_array[0].y_offset;
	
DiamondSearchEnd:
	pMv16x16->x = (dst_x - center_x);
	pMv16x16->y = (dst_y - center_y);	

// 	g_ME_SearchCount += (search_cycle * 3) + 4;
	
	return min_sad;
}

int32 GetSad8x8_FullPixel_DSMethod(uint8 *pRefFrameBfr,
									int32 FrameWidth, 
									int32 FrameHeight,
									int32 blk_center_x,
									int32 blk_center_y,
									MOTION_VECTOR_T *pMv8x8,
									int32 blk_num
								)
{
	int32 cor_x, cor_y;
	int32 sad8x8;
	int32 i;
	int32 dst_x, dst_y;
	int32 min_sad8x8;
	SAD_SEARCH_T sad_search_array[9];

	min_sad8x8 = 0x7FFFFFFF;

	//step 1, large diamond search pattern
	MEA_FPRINTF(g_fp_trace_mea_sad, "8*8 large diamond search pattern\n");
					
	for(i = 0; i < 9; i++)
	{
		cor_x = blk_center_x + s_LDSP_OffsetXY[i][0];
		cor_y = blk_center_y + s_LDSP_OffsetXY[i][1];
		g_search_point_cnt++;
		
		if((cor_x < s_x_min[blk_num]) ||
		   (cor_x > s_x_max[blk_num]) ||
		   (cor_y < s_y_min[blk_num]) ||
		   (cor_y > s_y_max[blk_num]))
		{
			sad8x8 = MAX_SAD;
		}else
		{
			GetRefBlk_y(pRefFrameBfr, FrameWidth, FrameHeight, cor_x, cor_y, blk_num);
			GetErrBlk_y(blk_num);
			sad8x8 = ComputeBlkSad(blk_num);	
		}

		sad_search_array[i].x_offset = s_LDSP_OffsetXY[i][0];
		sad_search_array[i].y_offset = s_LDSP_OffsetXY[i][1];
		sad_search_array[i].sad = sad8x8;

		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_LDSP_OffsetXY[i][0], s_LDSP_OffsetXY[i][1], sad8x8);
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad8x8);
	}

	GetMinSad_LDSP(sad_search_array);

	sad_search_array[0].x_offset = sad_search_array[8].x_offset;
	sad_search_array[0].y_offset = sad_search_array[8].y_offset;
	sad_search_array[0].sad = sad_search_array[8].sad;

	//step 2, small diamond search pattern
	MEA_FPRINTF(g_fp_trace_mea_sad, "8*8 small diamond search pattern\n");
	for(i = 1; i < 5; i++)
	{
		cor_x = blk_center_x + s_SDSP_OffsetXY[i][0];
		cor_y = blk_center_y + s_SDSP_OffsetXY[i][1];
		g_search_point_cnt++;

		if((cor_x < s_x_min[blk_num]) ||
		   (cor_x > s_x_max[blk_num]) ||
		   (cor_y < s_y_min[blk_num]) ||
		   (cor_y > s_y_max[blk_num]))
		{
			sad8x8 = MAX_SAD;
		}else
		{
			GetRefBlk_y(pRefFrameBfr, FrameWidth, FrameHeight, cor_x, cor_y, blk_num);
			GetErrBlk_y(blk_num);
			sad8x8 = ComputeBlkSad(blk_num);
		}

		sad_search_array[i].x_offset = s_SDSP_OffsetXY[i][0];
		sad_search_array[i].y_offset = s_SDSP_OffsetXY[i][1];
		sad_search_array[i].sad = sad8x8;
	
		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_SDSP_OffsetXY[i][0], s_SDSP_OffsetXY[i][1], sad8x8);
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad8x8);
	}

	GetMinSad_SDSP(sad_search_array);

	min_sad8x8 = sad_search_array[0].sad;
	dst_x = blk_center_x + sad_search_array[0].x_offset;
	dst_y = blk_center_y + sad_search_array[0].y_offset;
	
//DiamondSearchEnd:
	pMv8x8->x = (dst_x - blk_center_x);
	pMv8x8->y = (dst_y - blk_center_y);	
	
	return min_sad8x8;
}

// *2 *1 *3
// *5 *0 *1
// *6 *7 *8
static int32 s_Blk8x8_OffsetXY[9][2] = { {0, 0}, {0, -1}, {-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}, {0, 1}};

// *2 *1 *3
// *4 *0 *5
// *6 *8 *7
static int32 s_HalfME_OffsetXY[9][2] = { {0, 0}, {0, -1}, {-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}, {0, 1}};

//get one block's mv.
int32 MotionEstimation_8x8(uint8 *pRefFrameBfr,
				  int32 FrameWidth, 
				  int32 FrameHeight,
				  int32 mb_x, //mb index in x-direction
				  int32 mb_y,  //mb index in y-direction
				  MOTION_VECTOR_T *pMv16x16,
				  MOTION_VECTOR_T *pMv8x8,
				  int32 blk_num
				  )
{
	int32 ref_center_x, ref_center_y;
	int32 ref_blk_center_x, ref_blk_center_y;
	int32 min_sad8x8;
	int32 sad8x8;

	ref_center_x = (mb_x + 0) * MB_SIZE + pMv16x16->x;
	ref_center_y = (mb_y + 0) * MB_SIZE + pMv16x16->y;

	min_sad8x8 = MAX_SAD;
	sad8x8 = 0;

	switch(blk_num)
	{
	case 0:
		ref_blk_center_x = ref_center_x;
		ref_blk_center_y = ref_center_y;
		break;
	case 1:
		ref_blk_center_x = ref_center_x + BLOCK_SIZE;
		ref_blk_center_y = ref_center_y;
		break;
	case 2:
		ref_blk_center_x = ref_center_x;
		ref_blk_center_y = ref_center_y + BLOCK_SIZE;
		break;
	case 3:
		ref_blk_center_x = ref_center_x + BLOCK_SIZE;
		ref_blk_center_y = ref_center_y + BLOCK_SIZE;
		break;
	default:
		printf(" illegal block number value! \n");
		break;
	}
	
	min_sad8x8 = GetSad8x8_FullPixel_DSMethod(pRefFrameBfr, FrameWidth, FrameHeight, ref_blk_center_x, 
						ref_blk_center_y, pMv8x8, blk_num);
	
	return min_sad8x8;	
}

int32 MotionEstimation_16x16(uint8 *pRefFrameBfr,
				  int32 FrameWidth, 
				  int32 FrameHeight,
				  int32 mb_x, //mb index in x-direction
				  int32 mb_y,  //mb index in y-direction
				  MOTION_VECTOR_T *pMv16x16,
				  BOOLEAN *pb16x16SadLittle_than_thres
				  )
{
	int32 ref_center_x, ref_center_y;
	int32 sad_at_0;
	int32 min_sad;
	int32 sad;
	MOTION_VECTOR_T predMV;
	int32 sad_at_predMv;
	int32 total_mb_num_x;
	int32 MEA_CFG3 = (int32)(g_mea_reg_ptr->MEA_CFG3); 
	int32 MEA_CFG4 = (int32)(g_mea_reg_ptr->MEA_CFG4); 
	int32 mb_sad_threshold	= (g_mea_reg_ptr->MEA_CFG1 >> 16) & 0xffff;
	int32 predEn		    = (g_mea_reg_ptr->MEA_CFG1 >> 4) & 0x01;
#if defined(MV_COST_TEST)
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
	int32 mb_qp = vop_mode_ptr->StepSize;
	int32 mvd_cost = (int32)sqrt(LAMBDA*mb_qp*mb_qp);
#endif

	ref_center_x = mb_x * MB_SIZE;
	ref_center_y = mb_y * MB_SIZE;

	total_mb_num_x = (FrameWidth + 15)/16;

	/*compute sad at mv(0, 0) and prediction position, get the best start point*/
	GetRefMB_y(pRefFrameBfr, FrameWidth, FrameHeight, ref_center_x, ref_center_y);
	GetErrMB_y();
	sad_at_0 = (ComputeMBSad());
	sad_at_0 -= (MEA_CFG4&0xffff); //(MB_NB/2 + 1);
	g_search_point_cnt++;

#if defined(MV_COST_TEST)
	predMV.x = (int16)((MEA_CFG3 << 24)>>24);
	predMV.y = (int16)((MEA_CFG3 <<16)>>24);

	predMV.x = (predMV.x&0xFFFE);
	predMV.y = (predMV.y&0xFFFE);
{
	int32 mvdx = 0 - predMV.x;
	int32 mvdy = 0 - predMV.y;
	sad_at_0 += mvd_cost * (GetMVD_len(mvdx) + GetMVD_len(mvdy));
}	
#endif

	if(sad_at_0 < 0)
	{
		sad_at_0 = 0;
	}

	MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", 0, 0, sad_at_0);
	MEA_FPRINTF(g_pPureSadFile, "MB16X16\n");
	MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad_at_0);

#ifdef PRED_MIN
	sad_at_0 = MAX_SAD;
	MEA_FPRINTF(g_fp_trace_mea_sad, "we set the PRED_MIN param!\n");
#endif //PRED_MIN

//	predMV.x = (int16)(((g_PredMV[mb_y * total_mb_num_x + mb_x] & 0xff00)<<16)>>24);
//	predMV.y = (int16)(((g_PredMV[mb_y * total_mb_num_x + mb_x] & 0x00ff)<<24)>>24);
	predMV.x = (int16)((MEA_CFG3 << 24)>>24);
	predMV.y = (int16)((MEA_CFG3 <<16)>>24);

	predMV.x = (predMV.x&0xFFFE);
	predMV.y = (predMV.y&0xFFFE);

	/*compute sad at mv(0, 0) and prediction position, get the best start point*/
	GetRefMB_y(pRefFrameBfr, FrameWidth, FrameHeight, ref_center_x + predMV.x/2, ref_center_y + predMV.y/2);
	GetErrMB_y();
	sad_at_predMv = (ComputeMBSad());
	g_search_point_cnt++;
#if defined(MV_COST_TEST)
	{
		int32 mvdx = 0;
		int32 mvdy = 0;
		sad_at_predMv += mvd_cost * (GetMVD_len(mvdx) + GetMVD_len(mvdy));
	}	
#endif
	MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", predMV.x/2, predMV.y/2, sad_at_predMv);
	MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad_at_predMv);

	if(predEn && (sad_at_0 > sad_at_predMv))
	{
		MEA_FPRINTF(g_fp_trace_mea_sad, "SAD in prediction mv position is small than in (0,0) position\n");
		min_sad = sad_at_predMv;
		ref_center_x = ref_center_x + predMV.x/2;
		ref_center_y = ref_center_y + predMV.y/2;
	}else
	{
		predMV.x = 0;
		predMV.y = 0;
		min_sad = sad_at_0;
	}
	
	if (min_sad < mb_sad_threshold)
	{
		pMv16x16->x = 0;
		pMv16x16->y = 0;
		sad = min_sad;
		*pb16x16SadLittle_than_thres = TRUE;
	}else
	{
		if( min_sad < 0)
		{
			min_sad = 0;
		}

		sad = GetSad16x16_FullPixel_DSMethod(pRefFrameBfr, FrameWidth, FrameHeight, ref_center_x, ref_center_y, min_sad, pMv16x16, pb16x16SadLittle_than_thres);
	}

	pMv16x16->x = pMv16x16->x +(predMV.x/2);
	pMv16x16->y = pMv16x16->y +(predMV.y/2);

#if defined(MV_COST_TEST)
	predMV.x = (int16)((MEA_CFG3 << 24)>>24);
	predMV.y = (int16)((MEA_CFG3 <<16)>>24);

	predMV.x = (predMV.x&0xFFFE);
	predMV.y = (predMV.y&0xFFFE);
{
	int32 mvdx = (pMv16x16->x) * 2 - predMV.x;
	int32 mvdy = (pMv16x16->y) * 2 - predMV.y;
	sad -= mvd_cost * (GetMVD_len(mvdx) + GetMVD_len(mvdy));
}	
#endif

	return sad;	
}

int32 HalfPixMotionEstimation_8x8(uint8 *pRefFrameBfr,
						int32 FrameWidth, 
						int32 FrameHeight,
						int32 mb_x, //mb index in x-direction
						int32 mb_y,  //mb index in y-direction
						MOTION_VECTOR_T *pMv,
						int32 blk_num
						)
{
	int32 ref_center_x, ref_center_y;
	int32 ref_blk_center_x, ref_blk_center_y;
	int32 min_sad8x8;
	int32 sad8x8;
	int32 cor_x, cor_y;
	int32 dst_x, dst_y;
	int32 i;
	int32 dx, dy;
	int32 start_x;
	int32 start_y;
	int32 rounding_control; 
	uint8 *pBlock; 
	SAD_SEARCH_T sad_search_array[9];

	rounding_control = 0;

	ref_center_x = mb_x * MB_SIZE + pMv[blk_num + 1].x;
	ref_center_y = mb_y * MB_SIZE + pMv[blk_num + 1].y;

	min_sad8x8 = MAX_SAD;
	sad8x8 = 0;

	switch(blk_num)
	{
	case 0:
		ref_blk_center_x = ref_center_x;
		ref_blk_center_y = ref_center_y;
		break;
	case 1:
		ref_blk_center_x = ref_center_x + BLOCK_SIZE;
		ref_blk_center_y = ref_center_y;
		break;
	case 2:
		ref_blk_center_x = ref_center_x;
		ref_blk_center_y = ref_center_y + BLOCK_SIZE;
		break;
	case 3:
		ref_blk_center_x = ref_center_x + BLOCK_SIZE;
		ref_blk_center_y = ref_center_y + BLOCK_SIZE;
		break;
	default:
		printf(" illegal block number value! \n");
		break;
	}
	
	//multiply 2.
	ref_blk_center_x = ref_blk_center_x * 2;
	ref_blk_center_y = ref_blk_center_y * 2;

	dst_x = ref_blk_center_x;
	dst_y = ref_blk_center_y;
	
	for(i = 0; i < 9; i++)
	{
		cor_x = ref_blk_center_x + s_Blk8x8_OffsetXY[i][0];
		cor_y = ref_blk_center_y + s_Blk8x8_OffsetXY[i][1];
		g_search_point_cnt++;

		if((cor_x < (2*s_x_min[blk_num])) ||
		   (cor_x > (2*s_x_max[blk_num])) ||
		   (cor_y < (2*s_y_min[blk_num])) ||
		   (cor_y > (2*s_y_max[blk_num])))
		{
			MEA_FPRINTF(g_fp_trace_mea_sad, "half me 8x8 out of range \n");		
			sad8x8 = MAX_SAD;
		}else
		{
			dx = cor_x & 1;
			dy = cor_y & 1;
			start_x = cor_x >> 1;
			start_y = cor_y >> 1;

			pBlock = g_RefMB[blk_num];

			g_dec_mc_8x8_me[(dx<<1) | dy](pRefFrameBfr, pBlock, rounding_control, FrameWidth, FrameHeight, BLOCK_SIZE, start_x, start_y);	

			GetErrBlk_y(blk_num);
			sad8x8 = ComputeBlkSad(blk_num);
		}
		
		sad_search_array[i].x_offset = s_Blk8x8_OffsetXY[i][0];
		sad_search_array[i].y_offset = s_Blk8x8_OffsetXY[i][1];
		sad_search_array[i].sad = sad8x8;

		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_Blk8x8_OffsetXY[i][0], s_Blk8x8_OffsetXY[i][1], sad8x8);		
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad8x8);
	}
	
	GetMinSad_LDSP(sad_search_array);

	min_sad8x8 = sad_search_array[8].sad;

//HalfPixelBlockMEEnd:
	pMv[blk_num + 1].x = (pMv[blk_num + 1].x * 2) + sad_search_array[8].x_offset;
	pMv[blk_num + 1].y = (pMv[blk_num + 1].y * 2) + sad_search_array[8].y_offset;	
	
	return min_sad8x8;	
}

int32 HalfPixMotionEstimation_16x16(uint8 *pRefFrameBfr,
						int32 FrameWidth, 
						int32 FrameHeight,
						int32 mb_x, //mb index in x-direction
						int32 mb_y,  //mb index in y-direction
						MOTION_VECTOR_T *pMv,
						int32 sad16x16_center
						)
{
	int32 ref_center_x, ref_center_y;
	int32 min_sad16x16;
	int32 sad16x16;
	int32 cor_x, cor_y;
	int32 dst_x, dst_y;
	int32 i;
	int32 dx, dy;
	int32 start_x;
	int32 start_y;
	int32 rounding_control; 
	uint8 MB_y[64*4]; //16x16 array
	SAD_SEARCH_T sad_search_array[9];
	int32 MEA_CFG4 = (int32)(g_mea_reg_ptr->MEA_CFG4); 
	int32 mb_sad_threshold	= (g_mea_reg_ptr->MEA_CFG1 >> 16) & 0xffff;

	rounding_control = 0;

	ref_center_x = mb_x * MB_SIZE + pMv->x;
	ref_center_y = mb_y * MB_SIZE + pMv->y;

	min_sad16x16 = MAX_SAD;
	sad16x16 = 0;
	
	//multiply 2.
	ref_center_x = ref_center_x * 2;
	ref_center_y = ref_center_y * 2;

	dst_x = ref_center_x;
	dst_y = ref_center_y;
	
	for(i = 0; i < 9; i++)
	{
		cor_x = ref_center_x + s_HalfME_OffsetXY[i][0];
		cor_y = ref_center_y + s_HalfME_OffsetXY[i][1];
		g_search_point_cnt++;

		if((cor_x < (2*s_x_min[0])) ||
		   (cor_x > (2*s_x_max[0])) ||
		   (cor_y < (2*s_y_min[0])) ||
		   (cor_y > (2*s_y_max[0])))
		{
			MEA_FPRINTF(g_fp_trace_mea_sad, "half me 16x16 out of range \n");		
			sad16x16 = MAX_SAD;
		}else
		{
			dx = cor_x & 1;
			dy = cor_y & 1;
			start_x = cor_x >> 1;
			start_y = cor_y >> 1;

			g_dec_mc_16x16[(dx<<1) | dy](pRefFrameBfr, MB_y, rounding_control, FrameWidth, FrameHeight, start_x, start_y);

			//conver 16x16 array to MB[4][64]
			ConverRefMB_y(MB_y);
		
			GetErrMB_y();
			sad16x16 = ComputeMBSad();	
				
			if( (cor_x ==(2 *mb_x * MB_SIZE)) &&(cor_y == (2*mb_y * MB_SIZE)) )
			{
				sad16x16 = sad16x16 - (MEA_CFG4&0xffff); //(MB_NB/2 + 1); //sad16(0,0)
				if( sad16x16 < 0)
				{
					sad16x16 = 0;
				}
			}
		}

		sad_search_array[i].x_offset = s_HalfME_OffsetXY[i][0];
		sad_search_array[i].y_offset = s_HalfME_OffsetXY[i][1];
		sad_search_array[i].sad = sad16x16;
		if (i == 0) //added by xw, @20100524, //for out_frame_clip of h.263 baseline
		{
			sad_search_array[i].sad = sad16x16_center;
		}

		MEA_FPRINTF(g_fp_trace_mea_sad, "(%d, %d) %0x\n", s_HalfME_OffsetXY[i][0], s_HalfME_OffsetXY[i][1], sad16x16);
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad16x16);

		if(min_sad16x16 < mb_sad_threshold)
		{
			MEA_FPRINTF(g_fp_trace_mea_sad, "little than Threshold (%d, %d) %0x\n", cor_x, cor_y, sad16x16);		
			goto HalfPixelMBMEEnd;
		}
	}

	GetMinSad_LDSP(sad_search_array);

	min_sad16x16 = sad_search_array[8].sad;
		
HalfPixelMBMEEnd:
	pMv->x = (pMv->x * 2) + sad_search_array[8].x_offset;
	pMv->y = (pMv->y * 2) + sad_search_array[8].y_offset;
	
	return min_sad16x16;	
}

#define ssign(x)		((x) > 0 ? 1 : (-1))		

void LookupUVMV(MB_MODE_E MB_Mode, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pMv_uv)
{
	int32 dx = 0, dy = 0;
	int32 grgiMvRound16[16] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
	int32 grgiMvRound12[12] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2};
	int32 grgiMvRound8[8] = {0, 0, 1, 1, 1, 1, 1, 2};
	int32 grgiMvRound4[4] = {0, 1, 1, 1};
	
	if(INTER4V == MB_Mode)
	{
		int32 i;

		pMv++;
		for(i = 0; i < 4; i++)
		{
			dx += pMv->x;
			dy += pMv->y;
			pMv++;
		}

		pMv_uv->x = ssign(dx) * (grgiMvRound16[abs(dx)%16] + (abs(dx)/16) * 2);
		pMv_uv->y = ssign(dy) * (grgiMvRound16[abs(dy)%16] + (abs(dy)/16) * 2);
	}else
	{
		dx = pMv->x;
		dy = pMv->y;

		pMv_uv->x = ssign(dx) * (grgiMvRound4[abs(dx)%4] + (abs(dx)/4) * 2);
		pMv_uv->y = ssign(dy) * (grgiMvRound4[abs(dy)%4] + (abs(dy)/4) * 2);		
	}
}

/*set MB mode to inter16x16, software only encoding inter16x16 MB now*/
void SoftMotionEstimation(int32 mb_x, int32 mb_y)
{
	int32 predEn;
	int32 fourMVEn;
	int32 intraEn;
	int32 max_search_cycle;
	int32 mb_sad_threshold;
	int32 mea_cfg1, mea_cfg2, mea_cfg3, mea_cfg4, mea_cfg5;

	MOTION_VECTOR_T mv16x16;
	MOTION_VECTOR_T mv8x8[4];	
	int32 blk_num;
	int32 sad8x8[4];
	MOTION_VECTOR_T mv[5];
	MOTION_VECTOR_T mv_uv;
	int32 sad16x16;
	int32 A;
	MB_MODE_E MB_Mode;
	int32 sad_inter;
	int32 sad8x8_sum;
	BOOLEAN b4MV_mode;
	BOOLEAN b16x16SadLittle_than_thres;
	uint8 *pSrcFrameBfr;
	uint8 *pRefFrameBfr;
	uint32 FrameWidth;
	uint32 FrameHeight;
	int32 mb_num_x;

	mea_cfg1 = g_mea_reg_ptr->MEA_CFG1;
	mea_cfg2 = g_mea_reg_ptr->MEA_CFG2;
	mea_cfg3 = g_mea_reg_ptr->MEA_CFG3;
	mea_cfg4 = g_mea_reg_ptr->MEA_CFG4;
	mea_cfg5 = g_mea_reg_ptr->MEA_CFG5;

	intraEn		= (mea_cfg1 >> 1) & 0x01;
	fourMVEn	= (mea_cfg1 >> 2) & 0x01;
	predEn		= (mea_cfg1 >> 4) & 0x01;
	max_search_cycle	= (mea_cfg1 >> 8)  & 0xff;
	mb_sad_threshold	= (mea_cfg1 >> 16) & 0xffff;


//	mb_mode_ptr->dctMd = INTER;
//	mb_mode_ptr->bIntra = FALSE;
//	mb_mode_ptr->bSkip = FALSE;

	pSrcFrameBfr = (uint8 *)g_mea_src_frame_y;
	pRefFrameBfr = (uint8 *)g_mea_ref_frame_y;
	FrameWidth = g_mea_frame_width <<2; //byte
	FrameHeight = g_mea_frame_height;
	mb_num_x = ((g_glb_reg_ptr->VSP_CFG1 >> 0) & 0x1ff);

	MEA_FPRINTF(g_fp_trace_mea_sad, "------No.%d (mb_x = %d, mb_y = %d)----\n", mb_x + mb_y * mb_num_x, mb_x, mb_y);
#ifdef USING_PREV_MV_FILE
	ReadoutPredMVfromFile(g_pPredMVFile);
	fclose(g_pPredMVFile);
#else
	{
/*
		for(i = 0; i< MB_NUM; i++)
		{
	//		g_PredMV[i] = PRED_MV;
		}
		*/
//#ifdef PRED_EN
//		g_PredMV [mb_y*vop_mode_ptr->MBNumX + mb_x] = (mb_mode_ptr->mvPred.x << 8) | (mb_mode_ptr->mvPred.y & 0xff);
//#else
//		g_PredMV [mb_y*vop_mode_ptr->MBNumX + mb_x] = 0;
//#endif
	}
#endif //USING_PREV_MV_FILE
						
	//step 1, get curr srcmb
	GetSrcMB_y(FrameWidth, FrameHeight, mb_x, mb_y);
	GetSrcMB_uv(FrameWidth, FrameHeight, mb_x, mb_y);

	//step 2, get curr src mb's A parameter

	if(intraEn)
	{
		A = ComputerMBA(); 
		MEA_FPRINTF(g_fp_trace_mea_sad, "MB's A = %0x\n", A);
		MEA_FPRINTF(g_pPureSadFile, "MBA\n");
		MEA_FPRINTF(g_pPureSadFile, "%04x\n", A);
	}else
	{
		MEA_FPRINTF(g_fp_trace_mea_sad, "Disable intra mb mode!\n");
		A = MAX_SAD;
	}
	FormatPrintHexNum(A, g_pFinalResultFile);

	//step 3, set the search area
	SetSearchRange(mb_x, mb_y);

	//step 4, do 16x16 me
	b16x16SadLittle_than_thres = FALSE;
	sad16x16 = MotionEstimation_16x16(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, &mv16x16, &b16x16SadLittle_than_thres);
	MEA_FPRINTF(g_fp_trace_mea_sad, "MV16x16: (%d, %d), Sad16X16 = %0x\n", mv16x16.x, mv16x16.y, sad16x16);
	MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad16x16);
			
	//step 5, do 8*8 me
	if(b16x16SadLittle_than_thres == FALSE)
	{
		if(fourMVEn)
		{
			sad8x8_sum = 0;
			for(blk_num = 0; blk_num < (BLOCK_CNT -2); blk_num++)
			{
				MEA_FPRINTF(g_fp_trace_mea_sad, "block %d\n", blk_num);
				MEA_FPRINTF(g_pPureSadFile, "block %d\n", blk_num);
				sad8x8[blk_num] = MotionEstimation_8x8(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, &mv16x16, &mv8x8[blk_num], blk_num);
				MEA_FPRINTF(g_fp_trace_mea_sad, "MV8x8: (%d, %d), Sad8x8 = %0x\n", mv8x8[blk_num].x, mv8x8[blk_num].y, sad8x8[blk_num]);
				MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad8x8[blk_num]);
				sad8x8_sum += sad8x8[blk_num];
				mv8x8[blk_num].x = mv8x8[blk_num].x + mv16x16.x;
				mv8x8[blk_num].y = mv8x8[blk_num].y + mv16x16.y;		
			}
			sad8x8_sum += ((mea_cfg4>>16)&0xffff); //(MB_NB/2 + 1);
			MEA_FPRINTF(g_fp_trace_mea_sad, "MV8x8 sum: %0x\n", sad8x8_sum);
		}else
		{
			sad8x8_sum = MAX_SAD;
		}
				
		b4MV_mode = FALSE;
		if(sad8x8_sum < sad16x16)
		{
			sad_inter = sad8x8_sum;
			b4MV_mode = TRUE;
		}else
		{
			sad_inter = sad16x16;
		}

		if(intraEn && ((A+(mea_cfg5&0xffff)) < sad_inter))
		{
			MB_Mode = INTRA;
		}else
		{
			MB_Mode = INTER;

			if(b4MV_mode)
			{
				MB_Mode = INTER4V;
			}
		}
	}else
	{
		MB_Mode = INTER;
		mv16x16.x = mv16x16.x * 2;
		mv16x16.y = mv16x16.y * 2;
	}

///	mb_mode_ptr->dctMd = MB_Mode;

	MEA_FPRINTF(g_fp_trace_mea_sad, "MB's mode: %d, [0->INTRA, 2->INTER, 4->INTER4V]\n", MB_Mode);
			
	if(MB_Mode != INTRA)
	{
		#if 1//for out_frame_clip of h.263 baseline
		{
			int32 standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
			if (standard == ITU_H263 || standard == FLV_H263)
			{
				int32 ref_pos_x = ((mb_x<<5) + (mv16x16.x));
				int32 ref_pos_y = ((mb_y<<5) + (mv16x16.y));

				/* checking in half-pixel precise */
				//modify mv
				if ((ref_pos_x < 0) || (ref_pos_x > ((int32)(FrameWidth  - MB_SIZE)*2)))
				{
					mv16x16.x = 0;
				}

				if ((ref_pos_y < 0) || (ref_pos_y > ((int32)(FrameHeight - MB_SIZE)*2)))
				{
					mv16x16.y = 0;
				}
			}
		}
		#endif		

		mv[0].x = mv16x16.x;    mv[0].y = mv16x16.y;
			
		if(MB_Mode == INTER)
		{
			/*	*/	
			mv[1].x = mv16x16.x;   mv[1].y = mv16x16.y;
			mv[2].x = mv16x16.x;   mv[2].y = mv16x16.y;
			mv[3].x = mv16x16.x;   mv[3].y = mv16x16.y;
			mv[4].x = mv16x16.x;   mv[4].y = mv16x16.y;
		}
		else   //INTER4V
		{
			mv[1].x = mv8x8[0].x;   mv[1].y = mv8x8[0].y;
			mv[2].x = mv8x8[1].x;	mv[2].y = mv8x8[1].y;
			mv[3].x = mv8x8[2].x;   mv[3].y = mv8x8[2].y;
			mv[4].x = mv8x8[3].x;   mv[4].y = mv8x8[3].y;
		}

		//step 6, do half-pixel me
		if(b16x16SadLittle_than_thres == FALSE)
		{
			MEA_FPRINTF(g_fp_trace_mea_sad, "-----Do half pixel precision motion estimation-----\n", MB_Mode);
		}

		if(MB_Mode == INTER4V) ///4MV
		{
 			FormatPrintHexNum(0, g_pFinalResultFile);
			for(blk_num = 0; blk_num < (BLOCK_CNT -2); blk_num++)
			{
 				MEA_FPRINTF(g_fp_trace_mea_sad, "block %d\n", blk_num);
				sad8x8[blk_num] = HalfPixMotionEstimation_8x8(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, mv, blk_num); 

				MEA_FPRINTF(g_fp_trace_mea_sad, "half pixel MV8x8: (%d, %d), Sad8x8 = %0x\n", mv[blk_num+1].x, mv[blk_num+1].y, sad8x8[blk_num]);
				MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad8x8[blk_num]);					
				FormatPrintHexNum(sad8x8[blk_num], g_pFinalResultFile);
				sad8x8_sum += sad8x8[blk_num];
			}	
		}else ///1MV
		{
			if(!b16x16SadLittle_than_thres)
			{
				sad16x16 = HalfPixMotionEstimation_16x16(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, mv, sad16x16); 
				MEA_FPRINTF(g_fp_trace_mea_sad, "half pixel MV16x16: (%d, %d), Sad16x16 = %0x\n", mv[0].x, mv[0].y, sad16x16);	
				MEA_FPRINTF(g_pPureSadFile, "%04x\n", sad16x16);	
			}
			FormatPrintHexNum(sad16x16, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
		}

		if(MB_Mode == INTER4V) ///4MV
		{
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum((mv[1].y<<8)|(mv[1].x&0xff), g_pFinalResultFile);
			FormatPrintHexNum((mv[2].y<<8)|(mv[2].x&0xff), g_pFinalResultFile);
			FormatPrintHexNum((mv[3].y<<8)|(mv[3].x&0xff), g_pFinalResultFile);
			FormatPrintHexNum((mv[4].y<<8)|(mv[4].x&0xff), g_pFinalResultFile);

		}else
		{
			FormatPrintHexNum((mv[0].y<<8)|(mv[0].x&0xff), g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);
			FormatPrintHexNum(0, g_pFinalResultFile);			
		}
#if 1 //for out_frame_clip of h.263 baseline
	{
		int32 standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
		if (standard == ITU_H263 || standard == FLV_H263)
		{
			int32 ref_pos_x = ((mb_x<<5) + (mv[0].x));
			int32 ref_pos_y = ((mb_y<<5) + (mv[0].y));

			/* checking in half-pixel precise */
			//modify mv
			if ((ref_pos_x < 0) || (ref_pos_x > ((int32)(FrameWidth  - MB_SIZE)*2)))
			{
				mv[0].x = 0;
			}

			if ((ref_pos_y < 0) || (ref_pos_y > ((int32)(FrameHeight - MB_SIZE)*2)))
			{
				mv[0].y = 0;
			}
		}
	}
#endif		
		//step 7, get final ref mb
		GetFinalRefMB_y(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, MB_Mode, mv);
		LookupUVMV(MB_Mode, mv, &mv_uv);

		MEA_FPRINTF(g_fp_trace_mea_sad, "u/v's mv: (%d, %d)\n", mv_uv.x, mv_uv.y);
	// 	Get12x9UVBlockInRefFrame(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y, &mv_uv);

		GetFinalRefMB_uv(pRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y,	 &mv_uv);
	} /*if(MB_Mode != INTRA)*/else
	{
		memcpy((void *)g_FinalRefMB, (void *)g_SrcMB, 64*6);
	}

	//step 7, write final ref mb to final-ref-buffer
//	WriteFinalRefMB(pFinalRefFrameBfr, FrameWidth, FrameHeight, mb_x, mb_y);

	g_mea_reg_ptr->MEA_CTL = (1<<31) | (((MB_Mode == INTRA)?0:((MB_Mode == INTER)?1:2))<<MEA_RESULT_BIT);

	if(MB_Mode == INTER)
	{
		g_mea_reg_ptr->mea_result_mv_16x16 = (sad16x16<<16) | ((mv[0].y<<8)&0xff00) | (mv[0].x&0xff);
	}
	else if(MB_Mode == INTER4V)
	{
		g_mea_reg_ptr->mea_result_mv_8x8[0] = (sad8x8[0]<<16) | ((mv[1].y<<8)&0xff00) | (mv[1].x&0xff);
		g_mea_reg_ptr->mea_result_mv_8x8[1] = (sad8x8[1]<<16) | ((mv[2].y<<8)&0xff00) | (mv[2].x&0xff);
		g_mea_reg_ptr->mea_result_mv_8x8[2] = (sad8x8[2]<<16) | ((mv[3].y<<8)&0xff00) | (mv[3].x&0xff);
		g_mea_reg_ptr->mea_result_mv_8x8[3] = (sad8x8[3]<<16) | ((mv[4].y<<8)&0xff00) | (mv[4].x&0xff);
	}
		
	//step 8
//	memcpy(vop_mode_ptr->pMBCache->pSrcMb[0], g_SrcMB[0], 64*6);
//	memcpy(vop_mode_ptr->pMBCache->pRefMb[0], g_FinalRefMB[0], 64*6);


//	Mp4Enc_ReadErrMB2DCTIOBfr(vop_mode_ptr);
	
// 	Mp4Enc_Trace_MEA_result_one_macroblock(vop_mode_ptr);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

