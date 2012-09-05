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
#include "vsp_bsm.h"
#include "vsp_drv_sc8800g.h"
#if _CMODEL_
#include "bsm_global.h"
#include "common_global.h"
#endif //_CMODEL_
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif
#ifdef _VSP_LINUX_
uint32 H264Dec_ByteConsumed();
#endif
#ifdef _VSP_LINUX_
static  inline void H264Dec_FlushBits(uint32 nbits)
#else
__inline void H264Dec_FlushBits(uint32 nbits)
#endif
{
#if _CMODEL_
	flush_nbits(nbits);
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (1<<0), "BSM_CFG2: configure flush n bits");
}

PUBLIC MMDecRet H264Dec_InitBitstream(void *pOneFrameBitstream, int32 length);
PUBLIC uint32 H264Dec_ShowBits(uint32 nbits);
PUBLIC uint32 READ_UE_V (void);
PUBLIC int32 READ_SE_V (void);
PUBLIC int32 SHOW_FLC(int32 nbits);
PUBLIC int32 READ_FLC(int32 nbits);
PUBLIC int32 H264Dec_Long_SEV (void);
PUBLIC void H264Dec_flush_left_byte (void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BITSTREAM_H_
