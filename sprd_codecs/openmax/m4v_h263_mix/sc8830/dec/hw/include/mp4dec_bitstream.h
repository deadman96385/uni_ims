/******************************************************************************
 ** File Name:      mp4dec_bitstream.h                                        *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of bitstream   *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_BITSTREAM_H_
#define _MP4DEC_BITSTREAM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_global.h"
#include "mp4dec_mode.h"
#include "vsp_drv_sc8830.h"



#if SIM_IN_WIN
#include "bsm_global.h"
#include "common_global.h"
#include "vsp_bsm.h"
#endif //_CMODEL_
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/*----------------------------------------------------------------------------*
**                            Mcaro Definitions                               *
**---------------------------------------------------------------------------*/





PUBLIC void Mp4Dec_FlushBits(uint32 nbits);
PUBLIC uint32 Mp4Dec_ShowBits(uint32 nbits);
PUBLIC void Mp4Dec_InitBitstream(void *pOneFrameBitstream, int32 length);
PUBLIC uint32 Mp4Dec_ByteAlign_Mp4(void);
PUBLIC uint32 Mp4Dec_ReadBits(uint32 nbits);
PUBLIC uint32 Mp4Dec_ShowBitsByteAlign(int32 nbits);
PUBLIC uint32 Mp4Dec_ByteAlign_Startcode(void);
PUBLIC uint32 Mp4Dec_ShowBitsByteAlign_H263(int32 nbits);
PUBLIC void Mp4Dec_VerifyBitstrm(uint8 *pStream, int32 strmLen);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_BITSTREAM_H_
