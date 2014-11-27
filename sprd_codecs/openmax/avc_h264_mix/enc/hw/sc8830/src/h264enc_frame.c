/******************************************************************************
 ** File Name:    h264enc_frame.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/18/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC void H264Enc_reference_update (ENC_IMAGE_PARAMS_T *img_ptr)
{
    H264EncStorablePic *pTmp = PNULL;

    pTmp = img_ptr->pYUVRefFrame;
    img_ptr->pYUVRefFrame = img_ptr->pYUVRecFrame;
    img_ptr->pYUVRecFrame = pTmp;

    return;
}

PUBLIC void H264Enc_reference_build_list (ENC_IMAGE_PARAMS_T *img_ptr, /*int i_poc,*/ int32 i_slice_type)
{
//     int i;

    /* build ref list 0/1 */
    img_ptr->i_ref0 = 0;

    if( i_slice_type == SLICE_TYPE_P )
    {
        img_ptr->i_ref0++;
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

