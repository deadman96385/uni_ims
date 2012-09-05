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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMCBPC_com_intra
 ** Description:	Get MCBPC of intra macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecMCBPC_com_intra(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint16 tmpval;
	const MCBPC_TABLE_CODE_LEN_T *tab = NULL;
	
	tmpval = (uint16)Mp4Dec_ShowBits(9);
	
	if(1 == tmpval)
    {
		/* macroblock stuffing */
		Mp4Dec_FlushBits(9);
		return 7; 
    }
	
	if(tmpval < 8)
    {
		PRINTF("Invalid MCBPCintra code\n");
		vop_mode_ptr->error_flag = TRUE;
		return -1;
    }
	
	tmpval >>= 3;
	
	if(tmpval >= 32)
    {
		Mp4Dec_FlushBits(1);
		return 3;
    }
	
	tab = &(vop_mode_ptr->pMCBPCtabintra[tmpval]);
	Mp4Dec_FlushBits((uint32)(tab->len));	
	
	return (int32)tab->code;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMCBPC_com_inter
 ** Description:	Get MCBPC of inter macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int16 Mp4Dec_VlcDecMCBPC_com_inter(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 code;

	code = Mp4Dec_ShowBits(9);
	
	if(code == 1)
    {
		/* macroblock stuffing */
		Mp4Dec_FlushBits(9);    
		return 7; /* HHI for Macroblock stuffing */
    }
	
	if(code == 0)
    {	
		PRINTF ("Invalid MCBPC code\n");
		vop_mode_ptr->error_flag = TRUE;
		return -1;
    }
	
	if(code >= 256)
    {
		Mp4Dec_FlushBits(1);	   
		return 0;
    }
	
	Mp4Dec_FlushBits((uint8)vop_mode_ptr->pMCBPCtab[code].len);	
	
	return vop_mode_ptr->pMCBPCtab[code].code;	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecCBPY
 ** Description:	Get CBPY of macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecCBPY(DEC_VOP_MODE_T *vop_mode_ptr, BOOLEAN is_intra_mb)
{
	int32 tmpval;
	int32  cbpy;
	const CBPY_TABLE_CODE_LEN_T *tab = NULL;
	
	tmpval = (int32)Mp4Dec_ShowBits(6);
	
	if(tmpval < 2)
	{	
		PRINTF("Invalid CBPY4 code\n");
		vop_mode_ptr->error_flag = TRUE;
		return -1;
	}
	
	if(tmpval >= 48)
	{
		Mp4Dec_FlushBits(2);
		cbpy = 15;
	}else 
	{		
		tab = &(vop_mode_ptr->pCBPYtab[tmpval]);
		Mp4Dec_FlushBits((uint32)(tab->len));
		cbpy = tab->code;
	}
	
	if(!is_intra_mb) 
	{
		cbpy = 15-cbpy;
	}
	
	return cbpy;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecPredIntraDCSize
 ** Description:	Get DC size of prediction intra macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL uint8 Mp4Dec_VlcDecPredIntraDCSize(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
{
	uint8 dc_size=0;
	uint16 code;
	uint8 code_len;/*add by zhang zheng*/
	
	if(blk_num < 4) /* luminance block */
    {
		code = (uint16)Mp4Dec_ShowBits(11);
		code_len = 11;

		while((1 != code) && (code_len > 3))
		{
			code >>= 1;
			code_len--;
		}

		if(code_len > 3)
		{
			dc_size = code_len + 1;
			Mp4Dec_FlushBits(code_len);
			return dc_size;
		}
		
		if(3 == code_len)
		{
			if(1 == code)
			{
				dc_size = 4;
				Mp4Dec_FlushBits(3);
				
				return dc_size;
			}else if(2 == code)
			{
				dc_size = 3;
				Mp4Dec_FlushBits(3);

				return dc_size;
			}else if(3 == code)
			{
				dc_size = 0;
				Mp4Dec_FlushBits(3);

				return dc_size;
			}
			code_len--;
			code >>= 1;
		}
		
		if(2 == code_len)
		{
			if(2 == code)
			{
				dc_size = 2;
			}else if(3 == code)
			{
				dc_size =1;
			}

			Mp4Dec_FlushBits(2);
		}

		return dc_size;		
    }else /* chrominance block */
    {
		code = (uint16)Mp4Dec_ShowBits(12);
		code_len = 12;

		while( (code_len > 2) && ( 1 != code ))
		{
			code>>=1;
			code_len--;
		}

		dc_size = code_len;

		if(2 == code_len)
		{
			dc_size = 3 - code;
		}

		Mp4Dec_FlushBits(code_len);

		return dc_size;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecPredIntraDC
 ** Description:	Get DC of prediction intra macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecPredIntraDC(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
{
	uint8 dc_size;
	uint32 tmp_var;
	int32 intra_dc_delta;	
	int first_bit;

	if(READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
		"polling bsm fifo depth >= 8 words for mpeg4 intra DC"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return 0;
	}
	
	/* read DC size 2 - 8 bits */
	dc_size = Mp4Dec_VlcDecPredIntraDCSize(vop_mode_ptr, blk_num);
	
	if(vop_mode_ptr->error_flag)
    {
		PRINTF("Error decoding INTRADC pred. size\n");
		return -1;
    }
	
	if(0 == dc_size)
    {
		intra_dc_delta = 0;
	}else
    {
		/* read delta DC 0 - 8 bits */
		tmp_var = Mp4Dec_ReadBits(dc_size);//"DC coeff"
		
		first_bit = (tmp_var >> (dc_size-1));
		
		if(0 == first_bit)
       	{ 
       		/* negative delta INTRA DC */   
			intra_dc_delta  = (-1) * (int32)(tmp_var ^ ((1 << dc_size) - 1));
        }else
        { 
        	/* positive delta INTRA DC */
			intra_dc_delta = tmp_var;
        }

		if(dc_size > 8)
		{
			 Mp4Dec_ReadBits(1);//"Marker bit"
		}
    }
	
	return intra_dc_delta;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMV
 ** Description:	Get motion vector. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int16 Mp4Dec_VlcDecMV(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 code;
	int32  tmp;	
	const MV_TABLE_CODE_LEN_T *tab = PNULL;
	
	if(Mp4Dec_ReadBits(1))//"motion_code"
    {
		return 0; /* Vector difference = 0 */
    }
	
	if((code = Mp4Dec_ShowBits(12)) >= 512)
    {
		code = (code >> 8) - 2;
		tab = &vop_mode_ptr->pTMNMVtab0[code];

		Mp4Dec_FlushBits((uint8)(tab->len));		
		return tab->code;
    }

	if(code >= 128)
    {
		code = (code >> 2) - 32;
		tab  = &vop_mode_ptr->pTMNMVtab1[code];

		Mp4Dec_FlushBits((uint8)(tab->len));
		return tab->code;
    }
	
	tmp = code - 4;
	
	if(tmp < 0)
    {	
		PRINTF("Invalid motion_vector code\n");
		vop_mode_ptr->error_flag = TRUE;
		return -1;
    }
	
	code -= 4;

	tab = &vop_mode_ptr->pTMNMVtab2[code];
	
	Mp4Dec_FlushBits((uint8)(tab->len));

	return tab->code;	
}	

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecInterTCOEF_H263
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:     			only for sorenson h.263
 *****************************************************************************/
PUBLIC void Mp4Dec_VlcDecInterTCOEF_H263(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iCoefStart)
{
	uint32 code;
	uint32 temp;
	const VLC_TABLE_CODE_LEN_T *tab = PNULL;
	int32 run = 0, level = 0, sign = 0, last = 0;
	int32 flush_bits = 0;
	int32 index;
	int32 i = iCoefStart;
	const uint8 * rgiZigzag = g_standard_zigzag; 

	while(last != 1)
	{
		flush_bits = 0;
		
		code = Mp4Dec_ShowBits(32);
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
			return;
		}
		
		code = code << tab->len;
		flush_bits += tab->len;		
		
		if(ESCAPE == tab->code)
		{
			int32 run_level_last;
			
			if(ITU_H263 == vop_mode_ptr->video_std)
			{
			}else //sorenson h.263
			{
				int isl1;

				run_level_last = code >> 24;
				code = code << 8;
				flush_bits += 8;

				isl1 = (run_level_last >> 7);
				last = (run_level_last >> 6) & 0x01;
				run = run_level_last & 0x3f;

				if(isl1)
				{
					level = code >>21;
					flush_bits += 11;
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
					level = code >> 25;
					flush_bits += 7;
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
			level = tab->code & 15;
			last = (tab->code >> 12) & 1;
			
			/*sign = Mp4Dec_ReadBits (pBitstrm, 1); */
			sign = code >> 31;
			code = code << 1;
			flush_bits += 1;
		}		
		
		Mp4Dec_FlushBits(flush_bits);

		i += run;
		if(i >= 64)
		{
			vop_mode_ptr->error_flag = TRUE;
			return;
		} 
		
		index = rgiZigzag[i];
		
		if(sign == 1)
		{
			level = -level;
		}
		
		iDCTCoef[index] = (int16)level;
		
		i++;
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
