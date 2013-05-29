/******************************************************************************
 ** File Name:      bsm_global.h                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _BSM_GLOBAL_H_
#define _BSM_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

typedef struct stuffing_module_tag
{
	uint8 destuffing_eb;
	uint8 destuffing_flag;
	uint8 input_counter;
	uint8 output_counter;
	uint8 data[4];
	uint8 byte_count;
	uint8 h264_zero_num;
}DESTUFFING_T;

typedef struct bsmr_tag
{
	DESTUFFING_T destuffing;
	uint8 *bs_bfr0;
	uint8 *bs_bfr1;
	uint32 bs_pingpang_len;
	uint8 *bs_ptr;
	uint8 bs_bfr_idx;
	uint8 bs_bfr0_valid;
	uint8 bs_bfr1_valid;
	uint32 data[15];
	uint32 *data_inner; //the first 3byte in data[12];
	uint32 fifo_depth; //the word counter remain in fifo (total 9bytes)

	//for barral shift
	uint32 *curr_data; //the first word in data[12], it is current data used for vld decoding.
	uint32 bit_left; //the remain bit.

	uint32 total_bit_cnt; //total bit counter which has been consumed in vld decoding process.
}BSMR_T;

extern BSMR_T jpeg_bsmr;
extern uint32 g_destuffing_cnt;

//decoder
extern int32 g_bsm_stream_buf0_empty;
extern int32 g_bsm_stream_buf1_empty;

//encoder
extern int32 g_bsm_stream_buf0_full;
extern int32 g_bsm_stream_buf1_full;

extern uint8 *g_cur_bs_bfr_ptr;
extern uint8 *g_bs_pingpang_bfr0;
extern uint8 *g_bs_pingpang_bfr1;
extern uint32 g_bs_pingpang_bfr_len;

extern uint32 g_switch_cnt;
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void InitBitstream(void *pOneFrameBitstream, int32 length);

void init_bsm(/*JPEG_CODEC_T *jpeg_fw_codec*/);

void write_nbits(uint32 val, uint32 nbits, uint32 vlc_op);
uint32 show_nbits(uint32 nbits);
void flush_nbits(uint32 nbits);
uint32 read_nbits(uint32 nbits);
void flush_read(uint32 value);
uint32 ue_v(void);
int32 se_v(void);
void write_ue_v(uint32 val);

void get_one_word_from_destuffing_module();
void clear_bsm_fifo();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_BSMR_H_