/******************************************************************************
 ** File Name:      h264dec_biaridecod.c                                      *
 ** Author:         Xiaowei.Luo                                               *
 ** DATE:           03/29/2010                                                *
 ** Copyright:      2010 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                      *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 03/29/2010     Xiaowei.Luo      Create.                                   *
 ******************************************************************************/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void init_contexts (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int i;

        
	/* calculate pre-state */
    for( i= 0; i < 460; i++ ) 
	{
		int pre;
			
		if( img_ptr->type == I_SLICE )
			pre = IClip (1, 126 , ((cabac_context_init_I[i][0] * img_ptr->qp) >>4 ) + cabac_context_init_I[i][1]);
		else
			pre = IClip (1, 126 ,  ((cabac_context_init_PB[img_ptr->model_number][i][0] * img_ptr->qp) >>4 ) + cabac_context_init_PB[img_ptr->model_number][i][1]);

		if( pre <= 63 )
			img_ptr->cabac_state[i] = 2 * ( 63 - pre ) + 0;
		else
			img_ptr->cabac_state[i] = 2 * ( pre - 64 ) + 1;
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
