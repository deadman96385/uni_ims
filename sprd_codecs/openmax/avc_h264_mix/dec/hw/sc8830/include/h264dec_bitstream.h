/******************************************************************************
** File Name:      h264dec_bitstream.h                                       *
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
#ifndef _H264DEC_BITSTREAM_H_
#define _H264DEC_BITSTREAM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

uint32 show_bits (H264DecObject *vo, int32 nbits);
uint32 read_bits (H264DecObject *vo, int32 nbits);
uint32 ue_v (H264DecObject *vo);
int32 se_v (H264DecObject *vo);
int32 long_ue_v (H264DecObject *vo);
int32 long_se_v (H264DecObject *vo);

#define SHOW_FLC(nbits)    show_bits(vo, nbits)
#define READ_FLC(nbits)	read_bits(vo, nbits)
#define UE_V()    ue_v(vo)
#define SE_V()   se_v(vo)
#define Long_SE_V()    long_se_v(vo)
#define Long_UE_V()    long_ue_v(vo)

void H264Dec_flush_left_byte (H264DecObject *vo);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_BITSTREAM_H_
