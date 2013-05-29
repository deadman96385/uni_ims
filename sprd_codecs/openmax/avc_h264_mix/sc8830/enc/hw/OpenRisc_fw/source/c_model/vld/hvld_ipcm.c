/*hvld_ipcm.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "buffer_global.h"
#include "common_global.h"
#include "bsm_global.h"

#define BLK_LUMA	0
#define BLK_CB		1
#define BLK_CR		2


void H264_IpcmDec ()
{
	int		i;
	int		bit_cnt;
	int		flush_bits;
	int		left_bits_byte;
	int		bsm_ipcm_data;
	uint8	pix0;
	uint8	pix1;
	uint32	ipcm_dbuf_wdata;
	uint8	ipcm_dbuf_addr;
	int		blk_type;
	int		x_cor;
	int		y_cor;
	int		offset;
	int		blk4x4_x_id;
	int		blk4x4_y_id;
	int		blk8x8_x_id;
	int		blk8x8_y_id;
	int		blk8x8_id;
	int		blk4x4_id;
	int		x_cor_blk;		//x offset from block 4x4 start point
	int		y_cor_blk;		//y offset from block 4x4 start point

	bsm_ipcm_data = show_nbits (32);


	//byte align	
	bit_cnt			= g_bsm_reg_ptr->TOTAL_BITS; // g_bitstream->bitcnt;
	left_bits_byte	= 8 - (bit_cnt & 7);
	flush_bits		= (left_bits_byte == 8) ? 0 : left_bits_byte;

	flush_nbits (flush_bits);
	

	for (i = 0; i < 192; i++)
	{
		//read 16 bits from bitstream
		bsm_ipcm_data = show_nbits (32);
		pix0		  = (bsm_ipcm_data >> 24) & 0xff;
		pix1		  = (bsm_ipcm_data >> 16) & 0xff;

		flush_nbits (16);

		//write to dct/io buffer
		ipcm_dbuf_wdata = (pix1 << 16) | (pix0 << 0);

		if ((i & 0x80) == 0)
		{
			blk_type = BLK_LUMA;
			offset	= i;
		}
		else if((i & 0x20) == 0)
		{
			blk_type = BLK_CB;
			offset = i & 0x1f;
		}
		else
		{
			blk_type = BLK_CR;
			offset = i & 0x1f;
		}

		if (blk_type == BLK_LUMA)
		{
			x_cor = offset & 0x7;
			y_cor = offset >> 3;
		}
		else
		{
			x_cor = offset & 0x3;
			y_cor = offset >> 2;
		}

		blk4x4_x_id = x_cor >> 1;
		blk4x4_y_id = y_cor >> 2;

		blk8x8_x_id = blk4x4_x_id >> 1;
		blk8x8_y_id = blk4x4_y_id >> 1;

		blk8x8_id = (blk_type == BLK_LUMA) ? (blk8x8_y_id * 2 | blk8x8_x_id) :
					(blk_type == BLK_CB) ? 4 : 5;

		blk4x4_id = (blk8x8_id << 2) | ((blk4x4_y_id & 1) << 1) | (blk4x4_x_id & 1);

		x_cor_blk = x_cor & 0x1;
		y_cor_blk = y_cor & 0x3;

		ipcm_dbuf_addr = (blk4x4_id << 3) |  (y_cor_blk << 1) | x_cor_blk;
	
		vsp_dct_io_0[ipcm_dbuf_addr] = ipcm_dbuf_wdata;
	}
}