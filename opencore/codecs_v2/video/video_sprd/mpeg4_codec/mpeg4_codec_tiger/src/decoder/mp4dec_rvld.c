/******************************************************************************
 ** File Name:    mp4dec_rvld.c                                               *
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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#ifdef _MP4CODEC_DATA_PARTITION_
LOCAL int32 Mp4Dec_FastSearch(uint16 *code_table, uint16 code);
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_FastSearch
 ** Description:	fast binary search function to find the index from code_table.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int32 Mp4Dec_FastSearch(uint16 *code_table, uint16 code)
{
	uint32 low = 0;
	uint32 high = 169;
	uint32 mid;
	uint16 mid_code;
	
	while(low <= high)
	{
		mid=(low+high)>>1;
		mid_code = code_table[mid];
		if(code == mid_code)
		{
			return mid;
		}else if(code < mid_code)
		{
			high = mid - 1;
		}else
		{
			low=mid + 1 ;
		}
	}
	
	return -1;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraRvlcTcoef
 ** Description:	Get the intra rvlc tcoef.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC TCOEF_T Mp4Dec_GetIntraRvlcTcoef(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr)
{
	uint16 code;
	uint16 mask;
	uint32 count;
	uint32 len;	
	int32 index;
	TCOEF_T tcoef =	{0, 0, 0, 0};
	RVLC_TABLE_CODE_LEN_T *rvlc_code_len = PNULL;
	RVLC_TABLE_CODE_LEN_T *rvlc_code_len_table = g_dec_rvlc_dct3d_tab_intra;
	uint16 *code_tab = g_dec_rvlc_code_tab_intra;
	uint16 *index_tab = g_dec_rvlc_index_tab_intra;
	uint32 tmp_var;
	
	mask = 0x4000;      /* mask  100000000000000   */
	code = (uint16)Mp4Dec_ShowBits(bitstrm_ptr, 15);
	len = 1;	
	
	if(code & mask)
	{
		count = 1;
		while(count > 0) 
		{
			mask = mask >> 1;
			if(code & mask) 
			{
				count--;
			}
			len++;
		}
	}else
	{
		count = 2;
		while (count > 0)
		{
			mask = mask >> 1;
			if (!(code & mask))
			{
				count--;
			}
			len++;
		}
	}
	
	code = code & 0x7fff;
	code = code >> (15 - (len + 1));
	
	index = Mp4Dec_FastSearch(code_tab, code);
	
	if((-1) == index)
	{
		PRINTF( "Invalid Huffman code in RvlcDecTCOEF().\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos2 |= (1<<14);
		return tcoef;
	}
	
	rvlc_code_len = &rvlc_code_len_table[index_tab[index]];
	
	Mp4Dec_FlushBits(bitstrm_ptr, (uint32)rvlc_code_len->len);	
	
	tcoef.run = (rvlc_code_len->code >> 8) & 0xff;
	tcoef.level = rvlc_code_len->code & 0xff;
	tcoef.last = (rvlc_code_len->code >> 16) & 1;		
	
	if(ESCAPE == rvlc_code_len->code)	/* ESCAPE */
	{		
		Mp4Dec_FlushBits(bitstrm_ptr, 1);
		
		//tmp_var
		//bit[6], "LAST"
		//bit[5:0],"RUN"
		tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 7);
		tcoef.last = (int16)(tmp_var>>6); 
		tcoef.run = (int16)(tmp_var & 0x3F);
		
		//tmp_var
		//bit[17], "marker_bit"
		//bit[16:6],"LEVEL"
		//bit[5],"marker_bit"
		//bit[4:0],"SIGN"
		tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 18);
		tcoef.level = (int16)((tmp_var & 0x1FFFF)>>6);
		tcoef.sign = (int16)(tmp_var & 0x1F);
	}else
	{
		tcoef.sign = (int16)Mp4Dec_ReadBits(bitstrm_ptr, 1); //, "SIGN");
	}	
	
	return tcoef;	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetInterRvlcTcoef
 ** Description:	Get the inter rvlc tcoef.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC TCOEF_T Mp4Dec_GetInterRvlcTcoef(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr)
{
	uint16 code;
	int32 mask;
	uint32 count;
	uint32 len;	
	TCOEF_T tcoef = {0, 0, 0, 0};	
	int32 index;
	RVLC_TABLE_CODE_LEN_T *rvlc_code_len;
	RVLC_TABLE_CODE_LEN_T *rvlc_code_len_table = g_dec_rvlc_dct3d_tab_inter;
	uint16 *code_tab = g_dec_rvlc_code_tab_inter;
	uint16 *index_tab = g_dec_rvlc_index_tab_inter;
	uint32 tmp_var;
	
	mask = 0x4000;      /* mask  100000000000000   */
	code = (uint16)Mp4Dec_ShowBits(bitstrm_ptr, 15);
	len = 1;	
	
	if(code & mask)
	{
		count = 1;
		while (count > 0)
		{
			mask = mask >> 1;
			if (code & mask) 
				count--;
			len++;
		}
	}else 
	{
		count = 2;
		while(count > 0) 
		{
			mask = mask >> 1;
			if (!(code & mask))
				count--;
			len++;
		}
	}
	
	code = code & 0x7fff;
	code = code >> (15 - (len + 1));
	
	index = Mp4Dec_FastSearch(code_tab,code);
	if((-1) == index)
	{
		PRINTF ( "Invalid Huffman code in RvlcDecTCOEF().\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos2 |= (1<<15);
		return tcoef;
	}
	
	rvlc_code_len = &rvlc_code_len_table[index_tab[index]];
	
	Mp4Dec_FlushBits(bitstrm_ptr, (uint32)rvlc_code_len->len);	
  		
	tcoef.run = (rvlc_code_len->code >> 8) & 255;
	tcoef.level = rvlc_code_len->code & 255;
	tcoef.last = (rvlc_code_len->code >> 16) & 1;  
	
	if (rvlc_code_len->code == ESCAPE)	/* ESCAPE */
	{
		Mp4Dec_FlushBits(bitstrm_ptr, 1);
		//tmp_var
		//bit[6], "LAST"
		//bit[5:0],"RUN"
		tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 7);
		tcoef.last = (int16)(tmp_var>>6); 
		tcoef.run = (int16)(tmp_var & 0x3F);
		
		//tmp_var
		//bit[17], "marker_bit"
		//bit[16:6],"LEVEL"
		//bit[5],"marker_bit"
		//bit[4:0],"SIGN"
		tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 18);
		tcoef.level = (int16)((tmp_var & 0x1FFFF)>>6);
		tcoef.sign = (int16)(tmp_var & 0x1F);
	}else
	{
       	tcoef.sign = (int16)Mp4Dec_ReadBits(bitstrm_ptr, 1); //, "SIGN");
	}
	
	return tcoef;		
}
#endif //DATA_PARTITION
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
