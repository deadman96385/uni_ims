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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

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

	PrintfVLCOut(value, length, "MCBPC_Intra");
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

	PrintfVLCOut(value, length, "CBPY");
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
	PrintfVLCOut(value, length, "MCBPC_Inter");
	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}
#ifdef OUTPUT_TEST_VECTOR
uint32 Mp4Enc_OutputMV(ENC_VOP_MODE_T *pVop_mode, int32 mvint, int32 residual, uint8 Fcode, int32 vlc)
{
	uint32 sign = 0;
	uint32 abs_mv;
	uint32 length = 0;
	int32 value;	
	uint32 NumBits = 0;
	MV_TABLE_CODE_LEN_T *tab;
	uint32 temp = 0;
	uint32 temp_bits = 0;
	uint8 comment[128];
	
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
	NumBits += Mp4Enc_OutputBits(value,length);

	if(mvint != 0)
	{
		NumBits += Mp4Enc_OutputBits(sign,1);
	}

	temp |= (value<<(32-length));
	temp_bits += length;
	if (mvint != 0)
	{
		temp_bits += 1;
		temp |= (sign<<(32-temp_bits));
	}
	if((Fcode != 1) && (vlc != 0))
	{
		temp_bits += (Fcode-1);
		temp |= (residual<<(32-temp_bits));
	}

	temp >>= (32-temp_bits);


	sprintf(comment, "MV %d mvint %d residual %d", length, (mvint!=0)?1:0, ((Fcode != 1) && (vlc != 0))?(Fcode-1):0);

	PrintfVLCOut(temp, temp_bits, comment);
	
	return NumBits;
}
#else
uint32 Mp4Enc_OutputMV(ENC_VOP_MODE_T *pVop_mode, int32 mvint)
{
	uint32 sign = 0;
	uint32 abs_mv;
	uint32 length = 0;
	int32 value;	
	uint32 NumBits = 0;
	MV_TABLE_CODE_LEN_T *tab;
		
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
	NumBits += Mp4Enc_OutputBits(value,length);
	
	if(mvint != 0)
	{
		NumBits += Mp4Enc_OutputBits(sign,1);
	}
	
	return NumBits;
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
