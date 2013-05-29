/******************************************************************************
 ** File Name:    bsm_core.c	    										  *
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

uint8 readbyte_from_bs_pingpang_bfr()
{
	BSMR_T *bsmr = &jpeg_bsmr;

	if(bsmr->bs_bfr_idx == 0) //bfr0
	{
		if((uint32)(bsmr->bs_ptr - bsmr->bs_bfr0)>=bsmr->bs_pingpang_len)
		{
			bsmr->bs_bfr0_valid = 0;
			g_bsm_stream_buf0_empty = 1;

			if(!bsmr->bs_bfr1_valid)
			{
				//read data into pingpang buffer1
				SCI_MEMCPY(bsmr->bs_bfr1, g_cur_bs_bfr_ptr, bsmr->bs_pingpang_len);
				g_cur_bs_bfr_ptr += bsmr->bs_pingpang_len;

				bsmr->bs_bfr1_valid = 1;
				g_bsm_stream_buf1_empty = 0;

				g_bsm_reg_ptr->BSM_CFG1 &= 0xfff00000;
				g_switch_cnt++;
			}
			
			//set current bs buffer is pingping buffer1
			bsmr->bs_bfr_idx = 1;
			bsmr->bs_ptr = bsmr->bs_bfr1;
		}
	}else //bfr1
	{
		if((uint32)(bsmr->bs_ptr - bsmr->bs_bfr1)>=bsmr->bs_pingpang_len)
		{
			bsmr->bs_bfr1_valid = 0;
			g_bsm_stream_buf1_empty = 1;

			if(!bsmr->bs_bfr0_valid)
			{
				//read data into pingpang buffer1
				SCI_MEMCPY(bsmr->bs_bfr0, g_cur_bs_bfr_ptr, bsmr->bs_pingpang_len);
				g_cur_bs_bfr_ptr += bsmr->bs_pingpang_len;

				bsmr->bs_bfr0_valid = 1;
				g_bsm_stream_buf0_empty = 0;

				g_bsm_reg_ptr->BSM_CFG1 &= 0xfff00000;
				g_switch_cnt++;
			}
			
			//set current bs buffer is pingping buffer1
			bsmr->bs_bfr_idx = 0;
			bsmr->bs_ptr = bsmr->bs_bfr0;
		}
	}


	return *bsmr->bs_ptr++;
}

extern uint32 g_read_byte_num;
void get_one_word_from_destuffing_module()
{
	int i;
	DESTUFFING_T *pDestuffing = &(jpeg_bsmr.destuffing);
	int32 standard		= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;

	//de-stuffing module reads 4bytes form sdram pingpang buffer.
	for(i = 0; i < 4; i++)
	{
		uint8 data = readbyte_from_bs_pingpang_bfr();
			
		pDestuffing->input_counter++;
		if(standard == VSP_JPEG)
		{
			if((data == 0xff) && pDestuffing->destuffing_eb)
			{
				pDestuffing->destuffing_flag = 1;		
				pDestuffing->output_counter++;
					
				pDestuffing->data[pDestuffing->byte_count] = data;
				pDestuffing->byte_count++;
			}else if((data == 0x00) && (pDestuffing->destuffing_flag) && pDestuffing->destuffing_eb)
			{
				pDestuffing->destuffing_flag = 0;
				g_destuffing_cnt++;
			}else
			{
				pDestuffing->output_counter++;
				pDestuffing->destuffing_flag = 0;
					
				pDestuffing->data[pDestuffing->byte_count++] = data;
			}
		}else if ((standard == VSP_H264) && (pDestuffing->destuffing_eb))
		{
			g_read_byte_num++;

			if (g_read_byte_num == 0xc4)
			{
				g_read_byte_num = 0xc4;
			}

			if (g_read_byte_num >= g_bs_pingpang_bfr_len)
			{
				data = 0;
			}
			
			if (pDestuffing->h264_zero_num < 2)
			{
				pDestuffing->output_counter++;
					
				pDestuffing->data[pDestuffing->byte_count++] = data;

				pDestuffing->h264_zero_num++;

				if (data != 0)
				{
					pDestuffing->h264_zero_num = 0;
				}
			}else
			{
				if ((pDestuffing->h264_zero_num == 2) && (data == 0x03))
				{
					pDestuffing->h264_zero_num = 0;
					g_destuffing_cnt++;
			//		g_bsm_reg_ptr->TOTAL_BITS += 8;
				}else
				{
					pDestuffing->output_counter++;
					pDestuffing->data[pDestuffing->byte_count++] = data;

					if (data == 0)
					{
						pDestuffing->h264_zero_num++;
					}else
					{
						pDestuffing->h264_zero_num = 0;
					}
				}							
			}
		}else //MPEG-4, H.263, FLV1, or H.264(destuffing has been done in get unit
		{
			pDestuffing->output_counter++;
			pDestuffing->destuffing_flag = 0;
					
			pDestuffing->data[pDestuffing->byte_count++] = data;
		}
			
		if(pDestuffing->output_counter == 0x4)
		{
			jpeg_bsmr.data[jpeg_bsmr.fifo_depth++] = (pDestuffing->data[0]<<24) | 
													(pDestuffing->data[1]<<16) |
													(pDestuffing->data[2]<<8) |
													(pDestuffing->data[3]<<0) ;
			pDestuffing->output_counter = 0;
			pDestuffing->byte_count = 0;

		//		FPRINTF(g_pf_debug, "pDestuffing->data:%0x\n", jpeg_bsmr.data[jpeg_bsmr.fifo_depth-1]);
		}
	}
	
	pDestuffing->input_counter = 0; //reset

	if((jpeg_bsmr.data[jpeg_bsmr.fifo_depth-1] == 0xbdd712ca))
	{
		jpeg_bsmr = jpeg_bsmr;
	}

	//update interface
	g_bsm_reg_ptr->BSM_CFG1++; //update BSM_OFFSET_ADDR, word unit
}

const uint32 s_msk[33] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

void read_one_word_to_bsshifter_from_fifo()
{
	uint32 i;

	//move forward data[12]
	for(i = 0; i < 14; i++)
	{
		jpeg_bsmr.data[i] = jpeg_bsmr.data[i+1];
	}
	jpeg_bsmr.fifo_depth--;
	
	//read one word into fifo
	if(jpeg_bsmr.fifo_depth <= 11)
	{ //read 4 word
		get_one_word_from_destuffing_module();
		get_one_word_from_destuffing_module();
		get_one_word_from_destuffing_module();
		get_one_word_from_destuffing_module();
	}else
	{
		get_one_word_from_destuffing_module();
	}
}

uint32 show_nbits(uint32 nbits)
{
	uint32 val;
	uint32 bit_left = jpeg_bsmr.bit_left;
	uint32 curr_data = *(jpeg_bsmr.curr_data);

	if(nbits <= bit_left)
	{
		val = (curr_data>>(bit_left-nbits))&s_msk[nbits];
	}else
	{
		uint32 next_data = *(jpeg_bsmr.curr_data+1);

		val =((curr_data&s_msk[bit_left])<<(nbits-bit_left)) | (next_data>>(32-nbits+bit_left));
	}

	//update interface
	g_bsm_reg_ptr->BSM_RDATA = val;

	{
		uint32 bsm_fifo_depth = jpeg_bsmr.fifo_depth -3;
		uint32 remain_bytes_in_destuffing_module = jpeg_bsmr.destuffing.byte_count;
		uint32 remain_bits_in_bsshifter = jpeg_bsmr.bit_left;
		
		g_bsm_reg_ptr->BSM_DEBUG = ((32 - remain_bits_in_bsshifter) <<12)|(remain_bytes_in_destuffing_module<<8)| bsm_fifo_depth;	
	}

	return val;
}

void flush_nbits(uint32 nbits)
{
	uint32 bit_left = jpeg_bsmr.bit_left;

	if(nbits <= bit_left)
	{
		jpeg_bsmr.bit_left -= nbits;

		if(jpeg_bsmr.bit_left == 0) //bs shifter is empty now, then should read one word to bs-shifter from fifo
		{
			read_one_word_to_bsshifter_from_fifo();
			
			//update bit_left;
			jpeg_bsmr.bit_left = 32;
		}
	}else
	{
		//update bit_left;
		jpeg_bsmr.bit_left = (32 - (nbits - jpeg_bsmr.bit_left));
		
		read_one_word_to_bsshifter_from_fifo();
	}

	jpeg_bsmr.total_bit_cnt += nbits;

	//update interface
	g_bsm_reg_ptr->TOTAL_BITS += nbits;
	{
		uint32 bsm_fifo_depth = jpeg_bsmr.fifo_depth -3;
		uint32 remain_bytes_in_destuffing_module = jpeg_bsmr.destuffing.byte_count;
		uint32 remain_bits_in_bsshifter = jpeg_bsmr.bit_left;
		
		g_bsm_reg_ptr->BSM_DEBUG = ((32 - remain_bits_in_bsshifter) <<12)|(remain_bytes_in_destuffing_module<<8)| bsm_fifo_depth;	
	}
}

uint32 read_nbits(uint32 nbits)
{
	uint32 val;
	uint32 bit_left = jpeg_bsmr.bit_left;
	uint32 curr_data = *(jpeg_bsmr.curr_data);

	if(nbits <= bit_left)
	{
		val = (curr_data>>(bit_left-nbits))&s_msk[nbits];
		jpeg_bsmr.bit_left -= nbits;

		if(jpeg_bsmr.bit_left == 0) //bs shifter is empty now, then should read one word to bs-shifter from fifo
		{
			read_one_word_to_bsshifter_from_fifo();
			
			//update bit_left;
			jpeg_bsmr.bit_left = 32;
		}
	}else
	{
		uint32 next_data = *(jpeg_bsmr.curr_data+1);
		val =((curr_data&s_msk[bit_left])<<(nbits-bit_left)) | (next_data>>(32-nbits+bit_left));
		//update bit_left;
		jpeg_bsmr.bit_left = (32 - (nbits - jpeg_bsmr.bit_left));
		
		read_one_word_to_bsshifter_from_fifo();
	}

	jpeg_bsmr.total_bit_cnt += nbits;

	//update interface
	g_bsm_reg_ptr->BSM_RDATA = val;
	g_bsm_reg_ptr->TOTAL_BITS += nbits;
	{
		uint32 bsm_fifo_depth = jpeg_bsmr.fifo_depth -3;
		uint32 remain_bytes_in_destuffing_module = jpeg_bsmr.destuffing.byte_count;
		uint32 remain_bits_in_bsshifter = jpeg_bsmr.bit_left;
		
		g_bsm_reg_ptr->BSM_DEBUG = ((32 - remain_bits_in_bsshifter) <<12)|(remain_bytes_in_destuffing_module<<8)| bsm_fifo_depth;	
	}

	if((curr_data == 0xcb169171) && (jpeg_bsmr.bit_left == 32))
	{
		curr_data = curr_data;
	}

//	FPRINTF(g_pf_debug, "current bs32:%0x, bit_left:%d\n", curr_data, jpeg_bsmr.bit_left);

	return val;
}

/*****************************************************************************
**	Name : 			flush_read
**	Description:	Flush the Byte with FF
**	Author:			Yi.wang
**	Note:			
*****************************************************************************/
void flush_read(uint32 value)
{
	uint32 i = jpeg_bsmr.bit_left/8;
	uint32 flush_bits = jpeg_bsmr.bit_left - i*8;

	jpeg_bsmr.bit_left = i*8;
	
	//update interface
	g_bsm_reg_ptr->TOTAL_BITS+=flush_bits;
}

//for h264 
uint32 ue_v(void)
{
	int32 info;
	uint32 tmp;
	int32 leading_zero = 0;
	uint32 val;

	tmp = (uint32)show_nbits(32);

	//find the leading zero number
#if defined(SIM_IN_ADS)
	__asm{
		clz leading_zero, tmp
	}
#elif defined(SIM_IN_WIN)
	while ((tmp & (1<<(31-leading_zero))) == 0)
	{
		leading_zero++;
	}
#else
#endif

	info = (tmp >> (32 - 2*leading_zero - 1)) & s_msk[leading_zero];

	val = (1<<leading_zero) + info - 1;
	flush_nbits(leading_zero*2+1);
	
	g_bsm_reg_ptr->BSM_GLO_RESULT = val;

	return val;
}

int32 se_v(void)
{
	int32 info;
	uint32 tmp;
	int32 leading_zero = 0;
	int32 val;

	tmp = (uint32)show_nbits(32);

	//find the leading zero number
#if defined(SIM_IN_ADS)
	__asm{
		clz leading_zero, tmp
	}
#elif defined(SIM_IN_WIN)
	while ((tmp & (1<<(31-leading_zero))) == 0)
	{
		leading_zero++;
	}
#else
#endif

	info = (tmp >> (32 - 2*leading_zero - 1)) & s_msk[leading_zero];

	tmp = (1<<leading_zero) + info - 1;

	val = (tmp + 1)/2;
	
	if ((tmp & 1) == 0)
	{
		val = -val;
	}
	
	flush_nbits(leading_zero*2+1);
	
	g_bsm_reg_ptr->BSM_GLO_RESULT = val;

	return val;
}

//encoder
void get_current_byte_addr()
{
	BSMR_T *bsmr = &jpeg_bsmr;

	if(bsmr->bs_bfr_idx == 0) //bfr0
	{
		if((uint32)(bsmr->bs_ptr - bsmr->bs_bfr0)>=bsmr->bs_pingpang_len)
		{
			//send data from pingpang buffer0 to whole bs buffer
			SCI_MEMCPY(g_cur_bs_bfr_ptr, bsmr->bs_bfr0, bsmr->bs_pingpang_len);
			g_cur_bs_bfr_ptr += bsmr->bs_pingpang_len;
			
			bsmr->bs_bfr0_valid = 0;
			g_bsm_stream_buf0_full = 1;
			
			if(!bsmr->bs_bfr1_valid)
			{
				bsmr->bs_bfr1_valid = 1;
				g_bsm_stream_buf1_full = 0;
				
				g_bsm_reg_ptr->BSM_CFG1 &= 0xfff0000;
			}
			
			//set current bs buffer is pingping buffer1
			bsmr->bs_bfr_idx = 1;
			bsmr->bs_ptr = bsmr->bs_bfr1;
		}
	}else //bfr1
	{
		if((uint32)(bsmr->bs_ptr - bsmr->bs_bfr1)>=bsmr->bs_pingpang_len)
		{
			//send data from pingpang buffer0 to whole bs buffer
			SCI_MEMCPY(g_cur_bs_bfr_ptr, bsmr->bs_bfr1, bsmr->bs_pingpang_len);
			g_cur_bs_bfr_ptr += bsmr->bs_pingpang_len;
			
			bsmr->bs_bfr1_valid = 0;
			g_bsm_stream_buf1_full = 1;
			
			if(!bsmr->bs_bfr0_valid)
			{
				bsmr->bs_bfr0_valid = 1;
				g_bsm_stream_buf0_full = 0;
				
				g_bsm_reg_ptr->BSM_CFG1 &= 0xfff0000;
			}
			
			//set current bs buffer is pingping buffer1
			bsmr->bs_bfr_idx = 0;
			bsmr->bs_ptr = bsmr->bs_bfr0;
		}
	}
}

void send_one_word_to_bs_pingpang_bfr()
{
	int i;
	BSMR_T *bsmr = &jpeg_bsmr;

	for(i = 0; i < 4; i++)
	{
		get_current_byte_addr();

		*bsmr->bs_ptr++ = (jpeg_bsmr.data[jpeg_bsmr.fifo_depth] >> (24 - i*8))&0xff;
	}

	fprintf_oneWord_hex(g_fp_bsm_tv, jpeg_bsmr.data[jpeg_bsmr.fifo_depth]);
	jpeg_bsmr.fifo_depth--;

	//update interface
	g_bsm_reg_ptr->BSM_CFG1++; //update BSM_OFFSET_ADDR, word unit
}

void write_one_word_to_fifo_from_bsshifter()
{
 	uint32 i;
	//move forward data[12]
	for(i = 14; i > 0 ; i--)
	{
		jpeg_bsmr.data[i] = jpeg_bsmr.data[i-1];
	}
	jpeg_bsmr.data[0] = 0;
	jpeg_bsmr.fifo_depth++;	

	//send one word into fifo
	if(jpeg_bsmr.fifo_depth > 11)
	{ //send 4 word
		send_one_word_to_bs_pingpang_bfr();
		send_one_word_to_bs_pingpang_bfr();
		send_one_word_to_bs_pingpang_bfr();
		send_one_word_to_bs_pingpang_bfr();
	}
}

void write_nbits(uint32 val, uint32 nbits, uint32 vlc_op)
{
	uint32 bit_left = jpeg_bsmr.bit_left;
	uint32 curr_data = *(jpeg_bsmr.curr_data);
	uint32 msk = (1<<nbits) - 1;
	uint32 prev_byte_num = (32 - bit_left)>>3;
	uint32 curr_byte_num = prev_byte_num;
	uint32 i;
	int32 standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;
	int32 byte_out = 0, out_byte_num = 0; //for vlc out
	DESTUFFING_T *pDestuffing = &(jpeg_bsmr.destuffing);
	
	jpeg_bsmr.destuffing.destuffing_eb = (uint8)(g_bsm_reg_ptr->BSM_CFG1>>31);

	val &= msk;

	for(i = 1; i <= nbits; i++)
	{
		*(jpeg_bsmr.curr_data) |= ((val>>(nbits-i)&0x01)<<(jpeg_bsmr.bit_left-1));

		jpeg_bsmr.bit_left--;
		jpeg_bsmr.total_bit_cnt++;
		g_bsm_reg_ptr->TOTAL_BITS++;

		/*if(g_bsm_reg_ptr->TOTAL_BITS == 31118*8)
		{
			g_bsm_reg_ptr->TOTAL_BITS = g_bsm_reg_ptr->TOTAL_BITS;
		}*/

		curr_data = *(jpeg_bsmr.curr_data);

		if((VSP_JPEG ==standard) && vlc_op)
		{
			if((jpeg_bsmr.bit_left&7) == 0) //byte align, bit_left = 24 or 16 or 8 or 0
			{
				curr_data = (curr_data>>jpeg_bsmr.bit_left)&0xff;

				byte_out <<= (8);
				byte_out |= curr_data;
				out_byte_num++;	
				
				if(curr_data == 0xff)
				{
					out_byte_num++;
					byte_out <<= 8;
					if(jpeg_bsmr.bit_left)//24, 16 or 8
					{
						jpeg_bsmr.bit_left -= 8; //stuffing.
					}else //bit left = 0;
					{
						write_one_word_to_fifo_from_bsshifter();
			
						//update bit_left;
						jpeg_bsmr.bit_left = 24;
					}

					jpeg_bsmr.total_bit_cnt+=8;
					g_bsm_reg_ptr->TOTAL_BITS+=8;
				}
			}
		}else if ((VSP_H264 == standard) && pDestuffing->destuffing_eb) //for stuffing 0x03
		{
#ifndef NO_STUFFING
			/*if((jpeg_bsmr.bit_left&7) == 0) //byte align, bit_left = 24 or 16 or 8 or 0
			{
				curr_data = (curr_data>>jpeg_bsmr.bit_left)&0xff;

				if (curr_data <= 0x3 && pDestuffing->h264_zero_num == 2)
				{
					if (jpeg_bsmr.bit_left) //24, 16 or 8
					{
						if (jpeg_bsmr.bit_left == 24)
						{
							*(jpeg_bsmr.curr_data) = (((0x3<<8)|curr_data)<<16);
						}else if (jpeg_bsmr.bit_left == 16)
						{
							*(jpeg_bsmr.curr_data) &= 0xff000000;
							*(jpeg_bsmr.curr_data) |= (((0x3<<8)|curr_data)<<8);
						}
						else if(jpeg_bsmr.bit_left == 8)
						{
							*(jpeg_bsmr.curr_data) &= 0xffff0000;
							*(jpeg_bsmr.curr_data) |= (((0x3<<8)|curr_data)<<0);		
						}
						
						jpeg_bsmr.bit_left -= 8; //stuffing.
					}else //bit left = 0;
					{
						//add 0x3
						*(jpeg_bsmr.curr_data) &= 0xffffff00;
						*(jpeg_bsmr.curr_data) |= 0x3;

						write_one_word_to_fifo_from_bsshifter();
			
						*(jpeg_bsmr.curr_data) = curr_data << 24;
						
						//update bit_left;
						jpeg_bsmr.bit_left = 24;
					}

					jpeg_bsmr.total_bit_cnt+=8;
					g_bsm_reg_ptr->TOTAL_BITS+=8;

					pDestuffing->h264_zero_num = 0; //reset
				}

				if (curr_data == 0)
				{
					pDestuffing->h264_zero_num++;
				}else
				{
					pDestuffing->h264_zero_num = 0;
				}
			}*/
#endif
		}

		if(jpeg_bsmr.bit_left == 0) //bs shifter is full now, then should output one word to fifo from bs-shifter.
		{
			write_one_word_to_fifo_from_bsshifter();
			
			//update bit_left;
			jpeg_bsmr.bit_left = 32;
		}				
	}

	if (vlc_op)
	{
		if(VSP_JPEG ==standard)
		{
			if (out_byte_num)
			{
				PrintfBsmOut (byte_out, 8*out_byte_num);
			}
		}else //video
		{
			PrintfBsmOut (val, nbits);
		}
	}
	
	return;
}

void clear_bsm_fifo()
{
	int32 i;
	int32 left_byte = (32-jpeg_bsmr.bit_left)>>3;
	BSMR_T *bsmr = &jpeg_bsmr;

	//send one word into fifo
	while(jpeg_bsmr.fifo_depth)
	{ 
		send_one_word_to_bs_pingpang_bfr();
	}

	for(i = 0; i < left_byte; i++)
	{
		get_current_byte_addr();

		*bsmr->bs_ptr++ = (jpeg_bsmr.data[0] >> 24)&0xff;

		jpeg_bsmr.data[0] <<= 8;
		jpeg_bsmr.bit_left +=8;
	}

	if (left_byte)
	{
		fprintf_oneWord_hex(g_fp_bsm_tv, jpeg_bsmr.data[0]);
	}
	return;
}


int32 i_size0_255[256] = 
{
	1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
};

void write_ue_v(uint32 val)
{
	int32 i_size = 0;

	if (val == 0)
	{
#ifdef SIM_IN_WIN
		write_nbits(1, 1, 1);
		if(or1200_print)
#endif
		{
			OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, (1&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, 1, "ORSC: BSM_WDATA");
		}
#ifdef TV_OUT
//		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
#endif
	}else
	{
		uint32 tmp = ++val;
		
		if (tmp >= 0x00010000)
		{
			i_size += 16;
			tmp >>= 16;
		}

		if (tmp >= 0x100)
		{
			i_size += 8;
			tmp >>= 8;
		}

		i_size += i_size0_255[tmp];

#ifdef SIM_IN_WIN
		write_nbits (val, 2 * i_size-1, 1);
		if(or1200_print)
#endif
		{
			OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, ((2*i_size-1)&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, val, "ORSC: BSM_WDATA");
		}
#ifdef TV_OUT
//		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
#endif
	}
}

void write_se_v (int32 val)
{
	write_ue_v(val <= 0 ? -val*2 : val*2-1);
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
