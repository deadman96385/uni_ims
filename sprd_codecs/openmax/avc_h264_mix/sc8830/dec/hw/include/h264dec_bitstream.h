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
#include "vsp_drv_sc8830.h"
 #include "h264dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

uint32 BitstreamReadBits (DEC_BS_T * stream, int32 nbits);
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream);
PUBLIC int32 READ_SE_V (DEC_BS_T * stream);
PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream);
PUBLIC int32 H264Dec_Long_UEV (DEC_BS_T * stream);
PUBLIC void H264Dec_flush_left_byte (void);
void fillStreamBfr (DEC_BS_T * stream);

#define BITSTREAMSHOWBITS(bitstream, nbits) OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara") 
#define BITSTREAMFLUSHBITS(stream, nbits) \
	    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_flush n bits")



#define SHOW_FLC(stream, nbits)	BITSTREAMSHOWBITS(stream, nbits)
#define READ_FLC(stream, nbits)	BitstreamReadBits(stream, nbits)


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BITSTREAM_H_
