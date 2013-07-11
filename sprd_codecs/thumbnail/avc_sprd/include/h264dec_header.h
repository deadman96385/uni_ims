/******************************************************************************
** File Name:      h264dec_header.h                                          *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_HEADER_H_
#define _H264DEC_HEADER_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "sci_types.h"
#include "h264dec_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 H264Dec_Read_SPS_PPS_SliceHeader(H264DecContext *img_ptr, uint8 *bitstrm_ptr, uint32 bitstrm_len, MMDecOutput *dec_output_ptr);
MMDecRet H264Dec_FirstPartOfSliceHeader (DEC_SLICE_T *curr_slice_ptr, H264DecContext *img_ptr);
MMDecRet H264Dec_RestSliceHeader (H264DecContext *img_ptr, DEC_SLICE_T *curr_slice_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_HEADER_H_