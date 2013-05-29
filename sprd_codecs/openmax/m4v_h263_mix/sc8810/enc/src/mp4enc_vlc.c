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

uint32 Mp4Enc_OutputMCBPC_Intra(ENC_VOP_MODE_T *vop_mode_ptr, int32 cbpc, uint32 mode)
{
	uint32 index;
	uint32 length;
	int32 value;
	uint32 NumBits = 0;
	const MCBPC_TABLE_CODE_LEN_T *tab;
	
	index = ((mode >> 1) & 3) | ((cbpc & 3) << 2);

	tab = (&vop_mode_ptr->pMcbpc_intra_tab[index]);
	
	length = tab->len;
	value = tab->code;

	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputCBPY(ENC_VOP_MODE_T *vop_mode_ptr,  uint32 cbpy)
{
	uint32 length;
	int32 value;
	uint32 NumBits = 0;
	const MCBPC_TABLE_CODE_LEN_T *tab = (&vop_mode_ptr->pCbpy_tab[cbpy]);
	
	length = tab->len;
	value = tab->code;

	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputMCBPC_Inter(ENC_VOP_MODE_T *vop_mode_ptr, int32 cbpc, uint32 mode)
{
	uint32 index;
	uint32 length = 0;
	uint32 value;
	uint32 NumBits = 0;
	const MCBPC_TABLE_CODE_LEN_T *tab;

	index = (mode & 7) | ((cbpc & 3) << 3);
	tab = (&vop_mode_ptr->pMcbpc_inter_tab[index]);
	
	length = tab->len;
	value = tab->code;
	NumBits += Mp4Enc_OutputBits(value,length);
	
	return NumBits;
}

uint32 Mp4Enc_OutputMV(ENC_VOP_MODE_T *vop_mode_ptr, int32 mvint)
{
	uint32 sign = 0;
	uint32 abs_mv;
	uint32 length = 0;
	int32 value;	
	uint32 NumBits = 0;
	const MV_TABLE_CODE_LEN_T *tab;
		
	if(mvint > 32)
	{
		abs_mv = -mvint + 65;
		sign = 1;
	}else
	{
		abs_mv = mvint;
	}

	tab = (&vop_mode_ptr->pMvtab[abs_mv]);
	length = tab->len;
	value = tab->code;
	NumBits += Mp4Enc_OutputBits(value,length);
	
	if(mvint != 0)
	{
		NumBits += Mp4Enc_OutputBits(sign,1);
	}
	
	return NumBits;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
