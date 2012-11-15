/******************************************************************************
 ** File Name:    mp4dec_vld.c                                                *
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

//#define FAST_VLD
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#ifdef WIN32
/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecIntraTCOEF_sw
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:			vld, inverse scan, for MPEG-4
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecIntraTCOEF_sw(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iCoefQ, int32 iCoefStart,char *pNonCoeffPos)
{
	int32 len;
	int32 temp;	
	uint32 code;
	int32 nonCoeffNum = 0;
	int32 flush_bits;
	int32 i = iCoefStart, index;
	int32 level, run, sign, last = 0;	
	const VLC_TABLE_CODE_LEN_T *tab = PNULL;
	const uint8 * rgiZigzag = vop_mode_ptr->pZigzag; 
	DEC_BS_T *pBitstrm = vop_mode_ptr->bitstrm_ptr;
		
	while(!last)
	{		
		//code = Mp4Dec_ShowBits_sw(pBitstrm, 32);
		code = Mp4Dec_Show32Bits(pBitstrm);
		temp = code >> 20;
		
		if(temp >= 512)
		{
			tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
		}else if(temp >= 128)
		{
			tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
		}else if(temp >= 8) 
		{
			tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
		}else
		{
			PRINTF("Invalid Huffman code 1\n");
			vop_mode_ptr->error_flag = TRUE;	
			vop_mode_ptr->return_pos2 |= (1<<21);
			
			return 0;
		}
		
		//Mp4Dec_FlushBits (pBitstrm, tab->len);
		len = tab->len;
		flush_bits = len;
		code = code << len;		
				
		if(ESCAPE == tab->code)
		{	
			int32 level_offset;
			
			//level_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC level offset"
			level_offset = code >> 31;
			flush_bits += 1;
			code = code << 1;
			
			/*first escape mode, level is offset*/
			if(!level_offset) 
			{
				//code = Mp4Dec_ShowBits (pBitstrm, 12);
				temp = code >> 20;
				
				if(temp >= 512) 
				{
					tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
				}else if(temp >= 128)
				{
					tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
				}else if(temp >= 8)
				{
					tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
				}else
				{					 
					PRINTF ("Invalid Huffman code 2\n");
					vop_mode_ptr->error_flag = TRUE;	
					vop_mode_ptr->return_pos2 |= (1<<22);
					return 0;
				}
				
				//Mp4Dec_FlushBits (pBitstrm, tab->len);
				len = tab->len;
				flush_bits += len;
				code = code << len;
				
				run = (tab->code >> 8) & 63;
				level = tab->code & 255;
				last = (tab->code >> 14) & 1;
				
				level += vop_mode_ptr->pIntra_max_level[(last <<6) + run];
				//sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"

				sign = code >> 31;
				flush_bits += 1;
				code = code << 1;
			}else
			{
				/* second escape mode. run is offset */
				int32 run_offset;
				
				//run_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC run offset"
				run_offset = code >> 31;
				flush_bits += 1;
				code = code << 1;
				
				if(!run_offset) 
				{
					//code = Mp4Dec_ShowBits (pBitstrm, 12);
					temp = code >> 20;
					
					if(temp >= 512) 
					{
						tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
					}else if(temp >= 128)
					{
						tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
					}else if(temp >= 8)
					{
						tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
					}else
					{					 
						PRINTF("Invalid Huffman code 3\n");
						vop_mode_ptr->error_flag = TRUE;	
						vop_mode_ptr->return_pos2 |= (1<<23);
						return 0;
					}
					
					//Mp4Dec_FlushBits (pBitstrm, tab->len);
					len = tab->len;
					flush_bits += len; 
					code = code << len;
					
					run = (tab->code >> 8) & 63;
					level = tab->code & 255;
					last = (tab->code >> 14) & 1;

						run = run + vop_mode_ptr->pIntra_max_run [(last <<5) + level] + 1;
					
					//sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
					sign = code >> 31;
					flush_bits += 1;
					code = code << 1;
				}else
				{
					int32 run_levle_last;
					
					/* third escape mode*/			
					//run_levle_last = Mp4Dec_ReadBits (pBitstrm, 21);
					run_levle_last = code >> 11;
					flush_bits += 21;
					code = code << 21;

				    last = run_levle_last >> 20;
					run = (run_levle_last >> 14) & 0x3f;
					level = (run_levle_last >> 1) & 0xfff;
					
					if(level >= 2048)
					{
						sign = 1;
						level = 4096 - level;
					}else
					{
						sign = 0;
					}
				}
			}		
		}else
		{
			run = (tab->code >> 8) & 63;
			level = tab->code & 255;
			last = (tab->code >> 14) & 1;
			
			//tcoef.sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
			sign = code >> 31;
			flush_bits += 1;
			code = code << 1;
		}

		Mp4Dec_FlushBits(pBitstrm, flush_bits);

		i += run;

		if(i >= 64)
		{
			vop_mode_ptr->error_flag = TRUE;
		} 
	    index = rgiZigzag[i];
		//statistics which coefficients are non-zero for inverse quantization
		pNonCoeffPos[nonCoeffNum] = index;  

		if(sign)
		{
			iCoefQ [index] = -level;
		}else
		{
			iCoefQ [index] = level;	
		}
		
		i++;
		nonCoeffNum++;	
	}
	
	//return -1;
	return nonCoeffNum;
}
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecInterTCOEF_H263
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:     
 *****************************************************************************/
PUBLIC void Mp4Dec_VlcDecInterTCOEF_H263_sw(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP, DEC_BS_T *pBitstrm)
{
	uint32 code;
	uint32 temp;
	const VLC_TABLE_CODE_LEN_T *tab = PNULL;
	int32 run, level, sign = 0,last = 0;
	int32 flush_bits = 0;
	int32 index;
	int32 i = vop_mode_ptr->iCoefStart;
	const uint8 * rgiZigzag = vop_mode_ptr->pStandardZigzag; 
    int32 iquant;
	int32 QPModify = (iQP & 0x01) - 1;
	int32 qadd = (iQP - 1) | 0x1;
	int32 qmul = iQP << 1;

	last = 0;

	while(last != 1)
	{
		flush_bits = 0;
		
		code = Mp4Dec_Show32Bits(pBitstrm);
		temp = code >> 20;
		
		if(temp >= 512)
		{
			tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
		}else if(temp >= 128)
		{
			tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
		}else if(temp >= 8) 
		{
			tab = &vop_mode_ptr->pDCT3Dtab2[(temp >> 0) - 8];
		}else
		{
			PRINTF("Invalid Huffman code\n");
			vop_mode_ptr->error_flag = TRUE;	
			vop_mode_ptr->return_pos2 |= (1<<24);
			return;
		}
		
		//Mp4Dec_FlushBits (pBitstrm, tab->len);
		code = code << tab->len;
		flush_bits += tab->len;		
		
		if(ESCAPE == tab->code)
		{
			int32 run_level_last;
			
			if(VSP_ITU_H263 == vop_mode_ptr->video_std)
			{
				//run_level_last = Mp4Dec_ReadBits (pBitstrm, 15);
				run_level_last = code >> 17;
				code = code << 15;
				flush_bits += 15;
						
				last = run_level_last >> 14;
				run = (run_level_last >> 8) & 0x3f;
				level = run_level_last & 0xff;
				
				if(level > 128)
				{ 
					sign = 1; 
					level = 256 - level; 
				}else if(level == 128)
				{ 
					PRINTF ("Illegal LEVEL for ESCAPE mode 4: 128\n");
					vop_mode_ptr->error_flag = TRUE;
					vop_mode_ptr->return_pos2 |= (1<<25);
					return;
				}else if(level > 0)
				{
					sign = 0;
				}else if(level == 0)
				{
					PRINTF ("Illegal LEVEL for ESCAPE mode 4: 0\n");
					vop_mode_ptr->error_flag = TRUE;
					vop_mode_ptr->return_pos2 |= (1<<26);
					return;
				}
			}else //sorenson h.263
			{
				int isl1;

				//run_level_last = Mp4Dec_ReadBits (pBitstrm, 8);
				run_level_last = code >> 24;
				code = code << 8;
				flush_bits += 8;

				isl1 = (run_level_last >> 7);
				last = (run_level_last >> 6) & 0x01;
				run = run_level_last & 0x3f;

				if(isl1)
				{
					flush_bits += 11;
					level = code >>21;					
					if(level >= 1024)
					{
						sign = 1;
						level = 2048 - level;
					}else
					{
						sign = 0;
					}
				}else
				{
					flush_bits += 7;
					level = code >> 25;
					if(level > 64)
					{
						sign = 1;
						level = 128 - level;
					}else
					{
						sign = 0;
					}
				}
			}	
		}else
		{
			run = (tab->code >> 4) & 255;
			last = (tab->code >> 12) & 1;
			level = tab->code & 15;
		
			/*sign = Mp4Dec_ReadBits (pBitstrm, 1); */
			sign = code >> 31;
			code = code << 1;
			flush_bits += 1;
		}		
		
		Mp4Dec_FlushBits(pBitstrm, flush_bits);

		i += run;
		if(i >= 64)
		{
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos2 |= (1<<27);
			return;
		} 
		index = rgiZigzag[i];
		
		iquant = level * qmul + qadd;
		
		if(sign == 1)
		{
			iquant = -iquant;
		}
		
		iDCTCoef[index] = iquant;
		i++;		
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecInterTCOEF_Mpeg
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:     vld do deScan.
 *****************************************************************************/
#ifdef WIN32
PUBLIC void Mp4Dec_VlcDecInterTCOEF_Mpeg_sw(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP,DEC_BS_T * pBitstrm)
{
	uint32 code;
	int32 i = 0, index;
	const VLC_TABLE_CODE_LEN_T *tab = PNULL;
	int32 run, level, sign, last = 0;
	int32 flush_bits;
	int32 temp;
	const uint8 * rgiZigzag = vop_mode_ptr->pStandardZigzag; 

	/*for iquantization*/
	int32 iquant;
	int32 fQuantizer = vop_mode_ptr->QuantizerType;
	char *piQuantizerMatrix;
	int32 iSum = 0;
	BOOLEAN bCoefQAllZero = TRUE;
	int32 qadd, qmul;

	if(fQuantizer == Q_H263)
	{
		qadd = (iQP - 1) | 0x1;
		qmul = iQP << 1;
	}else
	{
		piQuantizerMatrix = vop_mode_ptr->InterQuantizerMatrix;
	}
	
	while(last == 0)
	{	
		code = Mp4Dec_Show32Bits(pBitstrm);
		
		temp = code >> 20;
		if(temp >= 512)
		{
			tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
		}else if(temp >= 128)
		{
			tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
		}else if(temp >= 8) 
		{
			tab = &vop_mode_ptr->pDCT3Dtab2[(temp >> 0) - 8];
		}else
		{
			PRINTF("Invalid Huffman code\n");
			vop_mode_ptr->error_flag = TRUE;	
			vop_mode_ptr->return_pos2 |= (1<<28);
			return;
		}
		
		//Mp4Dec_FlushBits (pBitstrm, tab->len);
		code = code << tab->len;
		flush_bits = tab->len;	
		
		if(ESCAPE == tab->code)
		{
			int32 level_offset;
			
			//level_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC level offset"
			level_offset = code >> 31;
			code = code << 1;
			flush_bits += 1;
			
			/*first escape mode, level is offset*/
			if(!level_offset) 
			{
				//code = Mp4Dec_ShowBits (pBitstrm, 12);
				temp = code >> 20;
				
				if(temp >= 512) 
				{
					tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
				}else if (temp >= 128)
				{
					tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
				}else if (temp >= 8)
				{
					tab = &vop_mode_ptr->pDCT3Dtab2[temp - 8];
				}else
				{					 
					PRINTF("Invalid Huffman code\n");
					vop_mode_ptr->error_flag = TRUE;	
					vop_mode_ptr->return_pos2 |= (1<<29);
					return;
				}
				
				//Mp4Dec_FlushBits (pBitstrm, tab->len);
				code = code << tab->len;
				flush_bits += tab->len;
				
				run = (tab->code >> 4) & 255;
				last = (tab->code >> 12) & 1;
				level = tab->code & 15;
				
				level += vop_mode_ptr->pInter_max_level[(last <<6) + run];
				//sign = Mp4Dec_ReadBits (pBitstrm, 1); 	//, "SIGN"	
				sign = code >> 31;
				code = code << 1;
				flush_bits += 1;			
			}else
			{
				/* second escape mode. run is offset */
				int32 run_offset;
				
				//run_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC run offset"
				run_offset = code >> 31;
				code = code << 1;
				flush_bits += 1;
				
				if(!run_offset) 
				{
					//code = Mp4Dec_ShowBits (pBitstrm, 12);
					temp = code >> 20;
					
					if(temp >= 512) 
					{
						tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
					}else if(temp >= 128)
					{
						tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
					}else if(temp >= 8)
					{
						tab = &vop_mode_ptr->pDCT3Dtab2[temp  - 8];
					}else
					{					 
						PRINTF("Invalid Huffman code\n");
						vop_mode_ptr->error_flag = TRUE;	
						vop_mode_ptr->return_pos2 |= (1<<30);
						return;
					}
					
					//Mp4Dec_FlushBits (pBitstrm, tab->len);
					code = code << tab->len;
					flush_bits += tab->len;
					
					run = (tab->code >> 4) & 255;					
					last = (tab->code >> 12) & 1;
					level = tab->code & 15;
					run = run + vop_mode_ptr->pInter_max_run[(last <<4) + level] + 1;

					//sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
					sign = code >> 31;
					code = code << 1;
					flush_bits += 1;
				}else
				{
					//int32_t mark;
					int32 run_levle_last;//
					
					/* third escape mode*/			
					//run_levle_last = Mp4Dec_ReadBits (pBitstrm, 21);
					run_levle_last = code >> 11;
					code = code << 21;
					flush_bits += 21;

					last = run_levle_last >> 20;
					run = (run_levle_last >> 14) & 0x3f;
					level = (run_levle_last >> 1) & 0xfff;
					
					if(level >= 2048)
					{
						sign = 1;
						level = 4096 - level;
					}else
					{
						sign = 0;
					}
				}
			}
		}else
		{
			run = (tab->code >> 4) & 255;
			level = tab->code & 15;  
			last = (tab->code >> 12) & 1;

			//tcoef.sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
			sign = code >> 31; 
			code = code << 1;
			flush_bits += 1;
		}

		Mp4Dec_FlushBits(pBitstrm, flush_bits);

		/*inverse quantization*/
		i += run;
		if(i >= 64)
		{
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos2 |= (1<<31);
			return;
		} 
		index = rgiZigzag[i];

		if(fQuantizer == Q_H263)
		{
		//	iquant = (iQP * ((level << 1) + 1) + QPModify);
			iquant = level * qmul + qadd;
		}else
		{
			iquant = iQP * (level * 2  + 1) * piQuantizerMatrix [index] >> 4;
			
			iSum ^= iquant;
			bCoefQAllZero = FALSE;
		}

		if(sign == 1)
		{
			iquant = -iquant;
		}

		iDCTCoef[index] = iquant;	
		i++;	
	}

	if(fQuantizer != Q_H263)
	{
		if(!bCoefQAllZero)
		{
			if((iSum & 0x00000001) == 0)
			{
				iDCTCoef [63] ^= 0x00000001;
			}
		}
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
