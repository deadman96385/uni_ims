/******************************************************************************
 ** File Name:      mp4enc_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of header      *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_HERDER_H_
#define _MP4ENC_HERDER_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 Mp4Enc_EncH263PicHeader(Mp4EncObject*vo);
uint32 Mp4Enc_EncSequenceHeader(Mp4EncObject*vo);
uint32 Mp4Enc_EncVOHeader(Mp4EncObject*vo);
uint32 Mp4Enc_EncVOLHeader(Mp4EncObject*vo);
uint32 Mp4Enc_EncVOPHeader(Mp4EncObject*vo, int32 time_stamp);
uint32 Mp4Enc_EncGOBHeader(Mp4EncObject*vo, int32 gob_quant);
uint32 Mp4Enc_ReSyncHeader(Mp4EncObject*vo, int32 time_stamp);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

#endif //_MP4ENC_ENCHERDER_H_
