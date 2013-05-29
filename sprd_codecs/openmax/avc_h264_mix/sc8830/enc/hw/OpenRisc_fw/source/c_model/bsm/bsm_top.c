/******************************************************************************
 ** File Name:    bsm.c														  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

BSMR_T jpeg_bsmr;
uint32 g_destuffing_cnt;

//decoder
int32 g_bsm_stream_buf0_empty;
int32 g_bsm_stream_buf1_empty;

//encoder
int32 g_bsm_stream_buf0_full;
int32 g_bsm_stream_buf1_full;

uint8 *g_cur_bs_bfr_ptr;

uint8 *g_bs_pingpang_bfr0;
uint8 *g_bs_pingpang_bfr1;
uint32 g_bs_pingpang_bfr_len;

uint32 g_switch_cnt;
uint32 g_read_byte_num;

void init_bsm(/*JPEG_CODEC_T *jpeg_fw_codec*/)
{
	int i;
	uint32 BSM_CFG0 = g_bsm_reg_ptr->BSM_CFG0;
	uint32 BSM_CFG1 = g_bsm_reg_ptr->BSM_CFG1;
	int32 is_enc = (g_glb_reg_ptr->VSP_CFG0 >> 14) & 0x01;
		
//g_pf_debug = fopen("d:/new_jpeg_8801h.log", "wb");

	jpeg_bsmr.bs_bfr0 = g_bs_pingpang_bfr0;//jpeg_fw_codec->stream_0;
	jpeg_bsmr.bs_bfr1 = g_bs_pingpang_bfr1;//jpeg_fw_codec->stream_1;
	jpeg_bsmr.bs_pingpang_len = BSM_CFG0<<2; //byte
	jpeg_bsmr.bs_bfr0_valid = (uint8)(BSM_CFG0>>31);
	jpeg_bsmr.bs_bfr1_valid = (uint8)(BSM_CFG0>>30)&0x1;
	g_bsm_stream_buf0_empty = 0;
	g_bsm_stream_buf1_empty = 0;
	g_bsm_stream_buf0_full = 0;
	g_bsm_stream_buf1_full = 0;
	//set current bs buffer is pingping buffer0
	jpeg_bsmr.bs_bfr_idx = 0;
	jpeg_bsmr.bs_ptr = jpeg_bsmr.bs_bfr0; 

	//init de-stuffing module
	jpeg_bsmr.destuffing.byte_count = 0;
	jpeg_bsmr.destuffing.data[0] = 0;
	jpeg_bsmr.destuffing.data[1] = 0;
	jpeg_bsmr.destuffing.data[2] = 0;
	jpeg_bsmr.destuffing.data[3] = 0;
	jpeg_bsmr.destuffing.destuffing_eb = (uint8)(BSM_CFG1>>31);
	jpeg_bsmr.destuffing.destuffing_flag = 0;
	jpeg_bsmr.destuffing.input_counter = 0;
	jpeg_bsmr.destuffing.output_counter = 0;
	jpeg_bsmr.destuffing.h264_zero_num = 0;

 	jpeg_bsmr.fifo_depth = 0;

	g_destuffing_cnt = 0;
	g_read_byte_num = 0;

	//clear the bsm fifo
	for(i = 0; i < 15; i++)
	{
		jpeg_bsmr.data[i] = 0;
	}

	if(!is_enc)
	{
		//fill the bsm fifo
		for(i = 0; i < 15; i++)
		{
			get_one_word_from_destuffing_module();
		}
	}

	jpeg_bsmr.data_inner = &(jpeg_bsmr.data[2]);
	jpeg_bsmr.curr_data = &(jpeg_bsmr.data[0]);

	jpeg_bsmr.bit_left = 32;
	jpeg_bsmr.total_bit_cnt = 0;

	g_bsm_reg_ptr->TOTAL_BITS = 0;

	g_switch_cnt = 0;
}

void bsm_module()
{
	
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
