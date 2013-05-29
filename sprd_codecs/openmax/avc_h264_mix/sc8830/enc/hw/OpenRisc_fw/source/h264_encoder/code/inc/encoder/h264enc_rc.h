/******************************************************************************
** File Name:      h264enc_rc.h	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/16/2011    Shangwen Li     Create.                                     *
*****************************************************************************/
#ifndef _H264ENC_RC_H_
#define _H264ENC_RC_H_

#include "video_common.h"

#define TARGET_BITRATE 1000000

//#define SLICE_SIZE TARGET_BITRATE/INTRA_PERIOD/3
//#define SLICE_SIZE 5000
#ifndef SLICE_SIZE
	#define SLICE_MB 1
#endif

#define NO_BU_CHANGE	// slice level update, not BU update
#ifndef NO_BU_CHANGE
/*	#define CONST_QP_I	// not define: Use I/P ratio
	#define BU_SIZE 20 // currently 20 MBs in a basic unit
	#define PIPELINE_BU_RC// for the simulation of pipeline of BU rate control
	#ifdef PIPELINE_BU_RC
		#define PIPE_MB 4 // MBs in the hardware pipeline
	#endif*/
#endif

#define I_P_RATIO 4 // the bit allocation for I and P

uint32 BU_bit_stat;
uint32 prev_qp;

typedef struct  {
	long curr_buf_full;
	int32 rem_bits;
	int32 prev_pic_bits;
	int32 total_qp;
}RC_GOP_PARAS;

RC_GOP_PARAS rc_gop_paras;

typedef struct  {
	long delta_target_buf;
	int32 target_bits;
	int32 remain_bits;
	int32 pred_remain_bits;
	int32 curr_pic_qp;
	uint32 total_qp;
	uint32 slice_num;
}RC_PIC_PARAS;

RC_PIC_PARAS rc_pic_paras;

typedef struct  {
	int32 target_bits;
	int32 prev_target_bits;
	uint32 BU_skip_MB;
	uint32 curr_BU_QP;
	uint32 next_BU_QP;
}RC_BU_PARAS;

RC_BU_PARAS rc_bu_paras;

extern uint32 g_nFrame_enc;

int32 rc_init_GOP (RC_GOP_PARAS *rc_gop_paras);
int32 rc_init_pict(RC_GOP_PARAS *rc_gop_paras, RC_PIC_PARAS *rc_pic_paras);
uint8 rc_init_BU(RC_PIC_PARAS *rc_pic_paras, RC_BU_PARAS *rc_bu_paras, uint16 BU_bit_stat, uint32 BU_nr);
int32 rc_init_slice(ENC_IMAGE_PARAMS_T *img_ptr, RC_PIC_PARAS *rc_pic_paras);
void rc_update_pict(int32 bits, RC_GOP_PARAS *rc_gop_paras);

#endif