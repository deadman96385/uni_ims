/******************************************************************************
 ** File Name:    mp4enc_vlc.c                                                *
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
#if _CMODEL_ //for RTL simulation
uint32 Mp4Enc_OutputMCBPC_Intra(ENC_VOP_MODE_T *pVop_mode, int32 cbpc, uint32 mode)
{
	uint32 index;
	uint32 length;
	int32 value;
	uint32 NumBits = 0;
	MCBPC_TABLE_CODE_LEN_T *tab;
	
	index = ((mode >> 1) & 3) | ((cbpc & 3) << 2);

	tab = (&pVop_mode->pMcbpc_intra_tab[index]);
	
	length = tab->len;
	value = tab->code;

	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputCBPY(ENC_VOP_MODE_T *pVop_mode,  uint32 cbpy)
{
	uint32 length;
	int32 value;
	uint32 NumBits = 0;
	MCBPC_TABLE_CODE_LEN_T *tab = (&pVop_mode->pCbpy_tab[cbpy]);
	
	length = tab->len;
	value = tab->code;

	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputMCBPC_Inter(ENC_VOP_MODE_T *pVop_mode, int32 cbpc, uint32 mode)
{
	uint32 index;
	uint32 length = 0;
	uint32 value;
	uint32 NumBits = 0;
	MCBPC_TABLE_CODE_LEN_T *tab;

	index = (mode & 7) | ((cbpc & 3) << 3);
	tab = (&pVop_mode->pMcbpc_inter_tab[index]);
	
	length = tab->len;
	value = tab->code;
	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputMV(ENC_VOP_MODE_T *pVop_mode, int32 mvint)
{
	uint32 sign = 0;
	uint32 abs_mv;
	uint32 length = 0;
	int32 value;	
	uint32 NumBits = 0;
	MV_TABLE_CODE_LEN_T *tab;
	uint32 data;
	BOOLEAN mvint_is_not_zero;
		
	if(mvint > 32)
	{
		abs_mv = -mvint + 65;
		sign = 1;
	}else
	{
		abs_mv = mvint;
	}

	tab = (&pVop_mode->pMvtab[abs_mv]);
	length = tab->len;
	value = tab->code;
	//NumBits += Mp4Enc_OutputBits(value,length);

	//if(mvint != 0)
	//{
	//	NumBits += Mp4Enc_OutputBits(sign,1);
	//}

	mvint_is_not_zero = (mvint != 0);
	data = (value << mvint_is_not_zero ) | sign;
	NumBits += Mp4Enc_OutputBits(data,length+mvint_is_not_zero);
	
	return NumBits;
}
#endif //#if _CMODEL_ 
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
