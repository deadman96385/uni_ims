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

//PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(uint8 *bitstrm_ptr, uint32 bitstrm_len);
PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(H264DecObject *vo);//uint8 *bitstrm_ptr, uint32 bitstrm_len);//weihu
PUBLIC void H264Dec_FirstPartOfSliceHeader (H264DecObject *vo);
PUBLIC void H264Dec_RestSliceHeader (H264DecObject *vo);

#if _MVC_
extern int GetViewIdx(H264DecObject *vo, int iVOIdx);
extern int GetVOIdx(H264DecObject *vo, int iViewId);
extern int get_maxViewIdx(H264DecObject *vo, int view_id, int anchor_pic_flag, int listidx);
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_HEADER_H_