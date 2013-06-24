
#ifndef _VP8DEC_REG_H_
#define _VP8DEC_REG_H_

#include "video_common.h"
#include "vp8dec_mode.h"
#include "vsp_drv_sc8830.h"

extern VSP_FH_REG_T   * g_fh_reg_ptr;


#define ORSC_SHARERAM_OFF	SHARE_RAM_BASE_ADDR
#define ORSC_VSP_GLB_OFF		GLB_REG_BASE_ADDR
#define ORSC_PPA_SINFO_OFF	PPA_SLICE_INFO_BASE_ADDR
#define ORSC_IQW_TBL_OFF		DCT_IQW_TABLE_BASE_ADDR
#define ORSC_FMADD_TBL_OFF	FRAME_ADDR_TABLE_BASE_ADDR
#define ORSC_VLC0_TBL_OFF		VLC_TABLE0_BASE_ADDR
#define ORSC_VLC1_TBL_OFF		VLC_TABLE1_BASE_ADDR
#define ORSC_BSM_CTRL_OFF	BSM_CTRL_REG_BASE_ADDR




void SharedRAM_Init();
void ORSC_Init(VP8D_COMP *pbi);
void BSM_Init(unsigned long size);
uint32 BitstreamReadBits (uint32 nbits);
uint32 BITSTREAMSHOWBYTE(vp8_reader *stream, uint32 nbytes);
void Write_tbuf_Probs(VP8D_COMP *pbi);

#endif //VP8DEC_REG_H