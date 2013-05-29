#include "sc8810_video_header.h"

int run_reg[64];
/*register definition*/
static int index_reg; 
static int zeros_left_reg;

int32 g_inverse_scan_ord_tbl [16] = //哈达吗的要转置，普通的只要倒序而不用转置了，所以下面两个表可能要重新排列一下。
{
	 0,  1,  4,  5, 
	 2,  3,  6,  7, 
	 8,  9, 12, 13,
	10, 11, 14, 15
};

char g_is_dctio_4x4_order [16] =
{	
	//0, 8, 2, 1, 10, 4, 12, 6, 9, 3, 11, 5, 14, 13, 7, 15
	  0, 2, 8 ,4, 10, 1, 3, 9, 6, 12, 14, 5, 11, 7, 13, 15
};

char g_is_dctio_8x8_order [4][16] =
{	
//	{ 0, 36, 34, 18, 12,  5, 54, 56, 49, 39, 46, 30, 23, 25, 43, 61},
	{ 0, 36, 20, 18, 33, 40, 54,  7, 14, 60, 53, 51, 58, 11, 29, 47},
//	{32, 16,  6, 52, 50,  3, 10, 28, 21, 19, 26, 41, 55, 62, 15, 59},
	{ 4,  2, 48, 38, 22, 24, 17, 35, 42, 26, 19, 13, 62, 55, 57, 31},
//	{ 4, 48,  1,  8, 22, 37, 44, 42, 35, 53, 60, 13, 11, 57, 47, 31},
	{32,  6,  8,  1, 50, 44, 37, 21, 28, 46, 39, 41, 25, 15, 61, 59},
//	{ 2, 20, 38, 40, 33, 17, 24, 14,  7,  9, 58, 51, 45, 29, 27, 63},
	{16, 34, 52,  5, 12, 10,  3, 49, 56,  9, 23, 30, 45, 43, 27, 63},
};

char g_is_dctio_8x8_order_cabac [64] =
{	
	 0,  4, 32, 16, 36,  2,  6, 34, 20, 48,  8, 52, 18, 38,  1,  5,
    33, 22, 50, 12, 40, 24, 44, 10, 54, 17, 37,  3,  7, 35, 21, 49,
	14, 42, 28, 56, 60, 26, 46,  9, 53, 19, 39, 23, 51, 13, 41, 30, 
	58, 62, 25, 45, 11, 55, 15, 43, 29, 57, 61, 27, 47, 31, 59, 63
};

char g_is_dctio_chroma_dc_order [4] = {0, 3, 2, 1};//{0, 3, 1, 2};//weihu


char g_is_dctio_4x4_order_ori [16] =
{	
	//0, 8, 2, 1, 10, 4, 12, 6, 9, 3, 11, 5, 14, 13, 7, 15
	//0, 2, 8 ,4, 10, 1, 3, 9, 6, 12, 14, 5, 11, 7, 13, 15
	0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
};
/*
char g_is_dctio_8x8_order_ori [4][16] =
{	
	{ 0, 9,17,18},

	{1,2,24,11,},
	
	{ 8,3,32,4},
	
	{16,10,25,5,},
	
};
*/
char g_is_dctio_8x8_order_ori [64] =
{	
	0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63,
};

#define COEFF_LUMA_AC_BASE_HIGH		0
#define COEFF_CHROMA_AC_BASE_HIGH	128
#define COEFF_LUMA_DC_BASE_HIGH		192
#define COEFF_CHROMA_DC_BASE_HIGH	200 //CHROMA 的系数位置估计要改，问虎哥

#define NZFLAG_LUMA_BASE_HIGH	204
#define NZFLAG_CHROMA_BASE_HIGH	212
#define NZFLAG_LUMA_DC_BASE_HIGH	216//weihu
#define NZFLAG_CHROMA_DC_BASE_HIGH	217


void coeff_vld_module()
{
	int		cbpLuma;
	int		cbpChroma;
	
	int		mb_x;
	int		mb_type;
	int		slice_type;
	int		cbp;
	int		lmb_avail;
	int		tmb_avail;
	int		transform_size_8x8_flag;

	int		i8x8, i4x4, iCbCr;
	int		is_cabac	= g_active_pps_ptr->entropy_coding_mode_flag;//(g_hvld_reg_ptr->mb_info >> 31) & 1;
	LINE_BUF_T *MBLineBuf;
	int i;
	int tmp2;

	mb_x		= currMBBuf->mb_x;
	mb_type		= currMBBuf->mb_type;
	slice_type	= currMBBuf->slice_type;
	cbp			= currMBBuf->cbp;
	lmb_avail	= currMBBuf->lmb_avail;
	tmb_avail	= currMBBuf->tmb_avail;
	transform_size_8x8_flag = currMBBuf->transform_size_8x8_flag;

	MBLineBuf= vldLineBuf + mb_x;

	cbpLuma	= cbp & 0xf;
	cbpChroma	= cbp >> 4;

	//clear dct/io buffer only for verification
	memset (vsp_dct_io_0, 0, 256*sizeof(uint32));
	memset (vsp_dct_io_1, 0, 256*sizeof(uint32)); //czzheng added @20120630

	currMBBuf->nnz_y0 = 0;
	currMBBuf->nnz_y1 = 0;
	currMBBuf->nnz_y2 = 0;
	currMBBuf->nnz_y3 = 0;
	currMBBuf->nnz_cb = 0;
	currMBBuf->nnz_cr = 0;

	if (!g_active_pps_ptr->entropy_coding_mode_flag)
	{
		residual_block = residual_block_cavlc;
	}
	else
	{
		residual_block = residual_block_cabac;
	}

	if (IS_I_16x16)
	{
		residual_block(LUMA_DC, 0, 0, 16);
	}
	
	for (i8x8=0; i8x8<4; i8x8++)
	{
		if (!transform_size_8x8_flag || !is_cabac)
		{
			if (cbpLuma&(1<<i8x8))
			{
				for (i4x4=0; i4x4<4; i4x4++)
				{
					if (IS_I_16x16)
					{
						residual_block(LUMA_AC_I16, i8x8*4+i4x4, 1, is_cabac ? 16 : 15);
					}
					else
					{
						residual_block(LUMA_AC, i8x8*4+i4x4, 0, 16);
					}
				}
			}
			else if (i8x8 == 2)
			{
				MBLineBuf->nnz_y &= 0xffff;
			}
			else if (i8x8 == 3)
			{
				MBLineBuf->nnz_y &= 0xffff0000; 
			}		
		}
		else if (cbpLuma&(1<<i8x8))//8x8 transform CABAC
		{
			residual_block(LUMA_AC_8x8, i8x8, 0, 64);
		}
		else if (i8x8 == 2)
		{
			MBLineBuf->nnz_y &= 0xffff;
		}
		else if (i8x8 == 3)
		{
			MBLineBuf->nnz_y &= 0xffff0000; 
		}		
	}
	
	//CHROMA DECODING
	for (iCbCr=0; iCbCr<2; iCbCr++)
	{
		if (cbpChroma&3)
			residual_block (CHROMA_DC, iCbCr, 0, 4);
		else
		{
			WriteBackTotalCoeffHigh (CHROMA_DC, 0, 0);
			WriteBackTotalCoeffHigh (CHROMA_DC, 1, 0);
		}
	}

		/*decoding chroma AC*/
	if (cbpChroma & 2)
	{
		for (i4x4 = 0; i4x4 < 8; i4x4++)
		{
			residual_block (CHROMA_AC, i4x4, 1, 15);
		}
	}
	else
	{
		MBLineBuf->nnz_c = 0;
	}
	
//	inbuf[1] = (currMBBuf->nnz_y0) | 
//			   (currMBBuf->nnz_y1 << 4) | 
//			   (currMBBuf->nnz_y2 << 8) | 
//			   (currMBBuf->nnz_y3 << 12);
//	| 
//			   (currMBBuf->nnz_cb << 16) | 
// 			   (currMBBuf->nnz_cr << 20);

	inbuf[1] = ((((currMBBuf->nnz_y3 >>  0) & 0x1f) != 0) << 15) |
			   ((((currMBBuf->nnz_y3 >>  8) & 0x1f) != 0) << 14) |
			   ((((currMBBuf->nnz_y3 >> 16) & 0x1f) != 0) << 13) |
			   ((((currMBBuf->nnz_y3 >> 24) & 0x1f) != 0) << 12) |
			   ((((currMBBuf->nnz_y2 >>  0) & 0x1f) != 0) << 11) |
			   ((((currMBBuf->nnz_y2 >>  8) & 0x1f) != 0) << 10) |
			   ((((currMBBuf->nnz_y2 >> 16) & 0x1f) != 0) <<  9) |
			   ((((currMBBuf->nnz_y2 >> 24) & 0x1f) != 0) <<  8) |
			   ((((currMBBuf->nnz_y1 >>  0) & 0x1f) != 0) <<  7) |
			   ((((currMBBuf->nnz_y1 >>  8) & 0x1f) != 0) <<  6) |
			   ((((currMBBuf->nnz_y1 >> 16) & 0x1f) != 0) <<  5) |
			   ((((currMBBuf->nnz_y1 >> 24) & 0x1f) != 0) <<  4) |
			   ((((currMBBuf->nnz_y0 >>  0) & 0x1f) != 0) <<  3) |
			   ((((currMBBuf->nnz_y0 >>  8) & 0x1f) != 0) <<  2) |
			   ((((currMBBuf->nnz_y0 >> 16) & 0x1f) != 0) <<  1) |
			   ((((currMBBuf->nnz_y0 >> 24) & 0x1f) != 0) <<  0);
	
	if (((currMBBuf->coded_dc_flag >> 1) & 1) != 0)
	{
		inbuf[1] |= (0xf << 16);
	}
	if (((currMBBuf->coded_dc_flag >> 2) & 1) != 0)
	{
		inbuf[1] |= (0xf << 20);
	}

    inbuf[1] |= (cbpChroma&2|((cbpChroma&3) != 0)) << 24;//cbpChroma//weihu change cbp to 26bit
	
	{
		inbuf[1] |= ((((currMBBuf->nnz_cr >>  0) & 0x1f) != 0) <<  23) |
				    ((((currMBBuf->nnz_cr >>  8) & 0x1f) != 0) <<  22) |
				    ((((currMBBuf->nnz_cr >> 16) & 0x1f) != 0) <<  21) |
				    ((((currMBBuf->nnz_cr >> 24) & 0x1f) != 0) <<  20) |
				    ((((currMBBuf->nnz_cb >>  0) & 0x1f) != 0) <<  19) |
				    ((((currMBBuf->nnz_cb >>  8) & 0x1f) != 0) <<  18) |
				    ((((currMBBuf->nnz_cb >> 16) & 0x1f) != 0) <<  17) |
				    ((((currMBBuf->nnz_cb >> 24) & 0x1f) != 0) <<  16);
	}

	MBLineBuf->coded_dc_flag = currMBBuf->coded_dc_flag;

	//Switch the currMBBuf to leftMBBuf
// 	memcpy(&leftMBBuf, &currMBBuf, sizeof(MB_BUF_T));
	{
		MB_BUF_T *tempMBBuf;	
		tempMBBuf = leftMBBuf;
		leftMBBuf = currMBBuf;
		currMBBuf = tempMBBuf;	
	}

	//set the top line buffer


	for(i=0;i<109;i++)  //czzheng added @20120630
	   {
		tmp2= (vsp_dct_io_1[2*i+1]>>16)&0xffff;
		fprintf(g_fp_idct_tv,"%04x", tmp2);
		tmp2= vsp_dct_io_1[2*i+1]&0xffff;
		fprintf(g_fp_idct_tv,"%04x", tmp2);
		tmp2= (vsp_dct_io_1[2*i]>>16)&0xffff;
		fprintf(g_fp_idct_tv,"%04x", tmp2);
		tmp2= vsp_dct_io_1[2*i]&0xffff;
		fprintf(g_fp_idct_tv,"%04x", tmp2);
			
		fprintf(g_fp_idct_tv,"\n");	
	}  

	
	return;
}

void residual_block_cavlc(int	blk_type, int blk_id, int startIdx, int maxNumCoeff)
{
	int			i;
	int			nc;		//for choose huffman table
	int			trailing_one;
	int			total_coeff;
	int			level;
	int			run;
	int			hvld_dbuf_addr;
	int			blk_coeff_base_addr;
	int			blk_nzf_base_addr;
	int			offset_coeff;
	int			offset_nzf;
	int			blk_tbuf_addr;
	int			index;				//register
	int			position;
	int			err_ptr;
	int			position1,position2,hvld_dbuf_addr1,blk_nzf_base_addr1;//weihu for testvector

	GetnC(blk_type, blk_id, &nc);

	err_ptr = CoeffTokenDecHigh (nc, &trailing_one, &total_coeff);
	
	if (err_ptr)
	{
		printf ("coeff token error!\n");
		return;
	}

	if (total_coeff == 0)
	{
		WriteBackTotalCoeffHigh(blk_type, blk_id, total_coeff);
		return;
	}

	if (blk_type == CHROMA_DC)
	{
		int shift_bits;
		int cdf;
		
		shift_bits = (blk_id == 0) ? 1 : 2;
		cdf = (currMBBuf->coded_dc_flag & (~(1<<shift_bits)));
		cdf = cdf | ((total_coeff != 0) << shift_bits);
		currMBBuf->coded_dc_flag = cdf;
	}

	err_ptr = H264VldLevDecHigh (trailing_one, total_coeff);
	if (err_ptr)
	{
		printf ("level error!\n");
		return;
	}

	for (i = 0; i < 64; i++)
	{
		run_reg[i] = 0;
	}

	err_ptr = H264VldRunDecHigh (total_coeff, blk_type, maxNumCoeff);

	if (err_ptr)
	{
		printf ("run error!\n");
		return;
	}

	/*derive the block base address, and coefficient start position*///需要考虑8x8变换
	if (blk_type == LUMA_DC)//当采用的是luma16x16模式的时候，第一个word的前两个字节用来存储non_zero_flag
	{
		offset_coeff		= 0;
		offset_nzf			= 0;
		blk_coeff_base_addr = COEFF_LUMA_DC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_LUMA_DC_BASE_HIGH; //如果模式是luma_DC的话，那么luma_ac中的第一个系数，也就是第一个word的前16个比特肯定是用不到的。
	}
	else if ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16))
	{
		if (currMBBuf->transform_size_8x8_flag && blk_type == LUMA_AC)//CAVLC的时候8x8还是按照4个4x4块来做，所以还是先遍历16个块，但是当是CABAC的时候，这个需要重新改正了，因为8x8就是一个8x8的系数来做CABAC的。
		{
			offset_coeff = (blk_id/4) * 8 * 4;			//为了将各个标准的统一起来，按行扫描来做（第0，1，2，3个4x4块从0开始，然后4，5，6，7从第二个8x8块起）
			offset_nzf	 = (blk_id/4) * 2;				//start from 0 2 4 6
		}
		else
		{
			offset_coeff = blk_id * 8;//g_inverse_scan_ord_tbl[blk_id] * 8;			//为了将各个标准的统一起来，按行扫描来做
			offset_nzf	 = blk_id / 2;//g_inverse_scan_ord_tbl[blk_id] / 2; //weihu
		}

		blk_coeff_base_addr = COEFF_LUMA_AC_BASE_HIGH;
		blk_nzf_base_addr	= NZFLAG_LUMA_BASE_HIGH;
	}
	else if (blk_type == CHROMA_DC)
	{
		offset_coeff		= blk_id * 2;//四个系数，占两个word
		offset_nzf			= 0;
		blk_coeff_base_addr = COEFF_CHROMA_DC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_CHROMA_DC_BASE_HIGH;	//chroma AC 的第一个系数位置肯定用不到。
	}
	else
	{
		offset_coeff		= blk_id * 8;//4个块，所以不需要倒顺序
		offset_nzf			= blk_id / 2;
		blk_coeff_base_addr = COEFF_CHROMA_AC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_CHROMA_BASE_HIGH;
	}

	blk_coeff_base_addr += offset_coeff;
	blk_nzf_base_addr   += offset_nzf;
	
	/*assemble the coefficient block, including trailing_one and other coefficient*/
	index = startIdx - 1;

	for (i = 0; i < total_coeff; i++)
	{
		blk_tbuf_addr = LEV_TBUF_BASE_ADDR + i;
		level = g_hvld_huff_tab[blk_tbuf_addr];

		run = (i == 15) ? 0 : run_reg[i];

		index += run + 1;

		if (currMBBuf->transform_size_8x8_flag && blk_type == LUMA_AC)
		{
			position = g_is_dctio_8x8_order[blk_id%4][index];//weihu
			position1 = g_is_dctio_8x8_order_ori[blk_id%4+index*4];//weihu
			position2= position1%8+((blk_id/4)&1)*8+(position1/8+(blk_id/8)*8)*16;
			hvld_dbuf_addr1 = COEFF_LUMA_AC_BASE_HIGH*2 + position2;
			blk_nzf_base_addr1=NZFLAG_LUMA_BASE_HIGH+(position2/32);
		}
		else
		{
			position = (blk_type == CHROMA_DC) ? g_is_dctio_chroma_dc_order[index] : g_is_dctio_4x4_order[index];//因为chroma_DC只有四个系数，所以不需要倒顺序
		    position1 = (blk_type == CHROMA_DC) ? index : g_is_dctio_4x4_order_ori[index];
		
		    if (blk_type == CHROMA_DC) 
			{
				position2= position1+blk_id * 4;
				hvld_dbuf_addr1 = COEFF_CHROMA_DC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_CHROMA_DC_BASE_HIGH+(position2/32);
			}
			else if (blk_type == LUMA_DC)
			{
				position2= position1;
				hvld_dbuf_addr1 = COEFF_LUMA_DC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_LUMA_DC_BASE_HIGH+(position2/32);
			}
			else if ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16))
			{
				position2= position1%4+(g_inverse_scan_ord_tbl[blk_id]%4)*4+(position1/4+(g_inverse_scan_ord_tbl[blk_id]/4)*4)*16;
				hvld_dbuf_addr1 = COEFF_LUMA_AC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_LUMA_BASE_HIGH+(position2/32);
			}
			else//chroma_ac
			{
                position2= position1%4+(blk_id%2)*4+(position1/4+(blk_id/2)*4)*8;
				hvld_dbuf_addr1 = COEFF_CHROMA_AC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_CHROMA_BASE_HIGH+(position2/32);
			}


		}
		
		hvld_dbuf_addr = blk_coeff_base_addr*2 + position;
		

		((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] = (int16)level;//level 应该没有问题，不用改了，但是nz肯定还是需要改的。
        ((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] = (int16)level;//

		/*nz_flag |= (1 << position);*/
		if (currMBBuf->transform_size_8x8_flag && blk_type == LUMA_AC)
		{
			if (position < 32)
			{
				hvld_dbuf_addr = blk_nzf_base_addr;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << position);
				hvld_dbuf_addr1 = blk_nzf_base_addr1;//
				vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
			}
			else
			{
				hvld_dbuf_addr = blk_nzf_base_addr + 1;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << (position - 32));
				hvld_dbuf_addr1 = blk_nzf_base_addr1;//
				vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
			}	
		}
		else if (blk_type == LUMA_AC || blk_type == LUMA_AC_I16 || blk_type == CHROMA_AC)
		{
			if (blk_id & 1)
			{
				hvld_dbuf_addr = blk_nzf_base_addr;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << (position + 16));
				hvld_dbuf_addr1 = blk_nzf_base_addr1;//
				vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
			}
			else
			{
				hvld_dbuf_addr = blk_nzf_base_addr;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << position);
				hvld_dbuf_addr1 = blk_nzf_base_addr1;
				vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);
			}
		}
		else if (blk_type == LUMA_DC)
		{
			hvld_dbuf_addr = blk_nzf_base_addr * 2;
			((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << position);
			hvld_dbuf_addr1 = blk_nzf_base_addr1 * 2;
			((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] |= (1 << position2%32);
		}
		else if (blk_type == CHROMA_DC)
		{
			if (blk_id & 1)
			{
				hvld_dbuf_addr = blk_nzf_base_addr * 2;
				((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << (position + 4));
				hvld_dbuf_addr1 = blk_nzf_base_addr1 * 2+1;
				((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] |= (1 << (index));//weihu
			}
			else
			{
				hvld_dbuf_addr = blk_nzf_base_addr * 2;
				((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << position);
				hvld_dbuf_addr1 = blk_nzf_base_addr1 * 2;
				((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] |= (1 << index);//
			}	
		}//weihu tmp
	}

	if (blk_type != CHROMA_DC)
	{
		WriteBackTotalCoeffHigh(blk_type, blk_id, total_coeff);
	}
	

	/*write non-zero coeff flag into dct/io buffer*/
//	hvld_dbuf_addr = blk_nzf_base_addr;
//
//	vsp_dct_io_0[hvld_dbuf_addr] = nz_flag;

}

void WriteBackTotalCoeffHigh (
						  int	blk_type, 
						  int	blk_id, 
						  int	total_coeff
						  )
{
	int			mb_x;
	int			nnz_4blk;
	LINE_BUF_T *MBLineBuf;
	
	mb_x		= currMBBuf->mb_x;
	MBLineBuf	= vldLineBuf + mb_x;

	if ((blk_type == LUMA_AC) | (blk_type == LUMA_AC_I16))
	{
		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				nnz_4blk = currMBBuf->nnz_y0;

				if (blk_id == 0)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 1)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 2)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}

				currMBBuf->nnz_y0 = nnz_4blk;
			}
			else
			{
				nnz_4blk = currMBBuf->nnz_y1;

				if (blk_id == 4)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 5)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 6)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				currMBBuf->nnz_y1 = nnz_4blk;
			}
		}
		else
		{
			if (blk_id < 12)
			{
				nnz_4blk = currMBBuf->nnz_y2;
				
				if (blk_id == 8)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 9)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 10)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
					MBLineBuf->nnz_y = (MBLineBuf->nnz_y & 0xffffff) | (total_coeff << 24);
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
					MBLineBuf->nnz_y = (MBLineBuf->nnz_y & 0xff00ffff) | (total_coeff << 16);
				}
				
				currMBBuf->nnz_y2 = nnz_4blk;
			}
			else
			{
				nnz_4blk = currMBBuf->nnz_y3;
				
				if (blk_id == 12)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 13)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 14)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
					MBLineBuf->nnz_y = (MBLineBuf->nnz_y & 0xffff00ff) | (total_coeff << 8);
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
					MBLineBuf->nnz_y = (MBLineBuf->nnz_y & 0xffffff00) | (total_coeff << 0);
				}
				
				currMBBuf->nnz_y3 = nnz_4blk;
			}
		}
	}
	else if(blk_type == CHROMA_AC)
	{
		if (blk_id < 4)
		{
			nnz_4blk = currMBBuf->nnz_cb;
			
			if (blk_id == 0)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 1)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 2)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				MBLineBuf->nnz_c = (MBLineBuf->nnz_c & 0xffffff) | (total_coeff << 24);
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				MBLineBuf->nnz_c = (MBLineBuf->nnz_c & 0xff00ffff) | (total_coeff << 16);
			}
			
			currMBBuf->nnz_cb = nnz_4blk;
		}
		else
		{			
			nnz_4blk = currMBBuf->nnz_cr;
			
			if (blk_id == 4)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 5)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 6)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				MBLineBuf->nnz_c = (MBLineBuf->nnz_c & 0xffff00ff) | (total_coeff << 8);
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				MBLineBuf->nnz_c = (MBLineBuf->nnz_c & 0xffffff00) | (total_coeff << 0);
			}
			
			currMBBuf->nnz_cr = nnz_4blk;
		}
	}
	else if (blk_type == LUMA_DC || blk_type == CHROMA_DC)
	{
		int shift_bits;
		int cdf;
		
		shift_bits = (blk_type == LUMA_DC) ? 0 : ((blk_id == 0) ? 1 : 2);
		cdf = (currMBBuf->coded_dc_flag & (~(1<<shift_bits)));
		cdf = cdf | (total_coeff << shift_bits);
		currMBBuf->coded_dc_flag = cdf;
	}
}

void residual_block_cabac(int	blk_type, int blk_id, int startIdx, int maxNumCoeff)
{
	int		coded_blk_flag;
	DecodingEnvironment* dep = &g_image_ptr->de_cabac;

	g_offset = dep->Dvalue;
	g_range = dep->Drange;

	/*decode coded_block_flag*/
	if (maxNumCoeff != 64)
	{
		coded_blk_flag = readBlkCodedFlag_CABAC_High (blk_type, blk_id);
	}
	else
	{
		coded_blk_flag = 1;
		WriteBackTotalCoeffHigh (LUMA_AC, blk_id*4+0, 1);
		WriteBackTotalCoeffHigh (LUMA_AC, blk_id*4+1, 1);
		WriteBackTotalCoeffHigh (LUMA_AC, blk_id*4+2, 1);
		WriteBackTotalCoeffHigh (LUMA_AC, blk_id*4+3, 1);
	}
	
	if (coded_blk_flag == 0)
	{
		dep->Dvalue = g_offset;
		dep->Drange = g_range;
		return;
	}

	/*if block coded, decode significant map*/
	ArithSigMapDecoderHigh (blk_type, blk_id, maxNumCoeff);
	
	/*decode level information*/
	ArithLevInforDecHigh (blk_type, blk_id);

	dep->Dvalue = g_offset;
	dep->Drange = g_range;
}

/*each 15 bits for significant map and last coefficient flag*/
int		g_sig_map_reg;
int		g_sig_map_reg2;

uint32	sigctx_reg_dec;
uint32	sigctx_reg_wb;

static const int  pos2ctx_map8x8 [] = { 0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
                                        4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
                                        7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
                                       12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX

static const int  pos2ctx_last8x8 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
                                         2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
                                         3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
                                         5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX

void ArithSigMapDecoderHigh (int blk_cat, int blk_id, int	maxNumCoeff)
{
	int			index;
	int			coeff_pos;
	int			start_pos;
	uint32	*	ctx_model_ptr;
	uint16		ctx_sig_last;
	uint8		sig_ctx;
	uint8		last_ctx;
	int			is_sig;
	int			is_last;
	int			syn_type;
	int			cat_offset;

	/*reset the register before one block decoding*/
	g_sig_map_reg = 0;
	g_sig_map_reg2 = 0;

	if (blk_cat == CHROMA_AC)
	{
		maxNumCoeff++;
	}

	start_pos = ((blk_cat == LUMA_AC_I16) || (blk_cat == CHROMA_AC)) ? 1 : 0;

	cat_offset = (blk_cat == 0) ? 0 :
				 (blk_cat == 1) ? 8 : 
				 (blk_cat == 2) ? 16 :
				 (blk_cat == 3) ? 24 : 
				 (blk_cat == 4) ? 26 : 34;
	
	for (index = 0; index < maxNumCoeff-1; index++)
	{
		coeff_pos = index + start_pos;//从低频到高频

		//at even clock, read out two context model
		if (blk_cat == LUMA_AC_8x8)
		{
			uint32	*	map_ctx_model_ptr;
			uint32	*	lst_ctx_model_ptr;

			map_ctx_model_ptr  = (g_hvld_huff_tab + 4) + cat_offset + (pos2ctx_map8x8[index] >> 2);
			lst_ctx_model_ptr  = (g_hvld_huff_tab + 4) + cat_offset + (pos2ctx_last8x8[index] >> 2) + 4;
			
			sigctx_reg_dec = *map_ctx_model_ptr;
			sig_ctx = (sigctx_reg_dec >> ((pos2ctx_map8x8[index] & 0x3) << 3)) & 0xff;//((index & 4) << 3)) & 0xff;//weihu

			sigctx_reg_dec = *lst_ctx_model_ptr;
			last_ctx = (sigctx_reg_dec >> ((pos2ctx_last8x8[index] & 0x3) << 3)) & 0xff;//((index & 4) << 3)) & 0xff;

			syn_type = CABAC_SIG_LAST;
			TwoBinArithDecHigh (syn_type, &sig_ctx, &last_ctx, &is_sig, &is_last, 0);

			/*update significant register*/
			if (coeff_pos <= 31)
			{
				g_sig_map_reg = g_sig_map_reg | (is_sig << coeff_pos);//一个寄存器还不够啊
			}
			else
			{
				g_sig_map_reg2 = g_sig_map_reg2 | (is_sig << (coeff_pos-32));//一个寄存器还不够啊
			}		
#if 0
			sigctx_reg_dec = (sigctx_reg_dec | (0xff << ((index & 4) << 3)));//(sigctx_reg_dec | (0xff << ((index & 4) << 3)));
			sigctx_reg_dec = (sigctx_reg_dec & (last_ctx << ((index & 4) << 3)));
			*lst_ctx_model_ptr = sigctx_reg_dec;

			sigctx_reg_dec = *map_ctx_model_ptr;
			sigctx_reg_dec = (sigctx_reg_dec | (0xff << ((index & 4) << 3)));
			sigctx_reg_dec = (sigctx_reg_dec & (sig_ctx << ((index & 4) << 3)));
			*map_ctx_model_ptr = sigctx_reg_dec;
#else
			sigctx_reg_wb = (0xffffffff ^ (0xff << ((pos2ctx_last8x8[index] & 0x3) << 3)));
			sigctx_reg_dec = sigctx_reg_dec & sigctx_reg_wb;
			sigctx_reg_dec = (sigctx_reg_dec | (last_ctx << ((pos2ctx_last8x8[index] & 0x3) << 3)));
			*lst_ctx_model_ptr = sigctx_reg_dec;

			sigctx_reg_dec = *map_ctx_model_ptr;
			sigctx_reg_wb = (0xffffffff ^ (0xff << ((pos2ctx_map8x8[index] & 0x3) << 3)));
			sigctx_reg_dec = sigctx_reg_dec & sigctx_reg_wb;
			sigctx_reg_dec = (sigctx_reg_dec | (sig_ctx << ((pos2ctx_map8x8[index] & 0x3) << 3)));
			*map_ctx_model_ptr = sigctx_reg_dec;
#endif
			if ((is_last) || (coeff_pos == maxNumCoeff-2))//结束的系数如果正好是even clock，也需要回写更新ctx
			{
				break;
			}
		}
		else
		{
			if ((index & 1) == 0)
			{
				ctx_model_ptr  = (g_hvld_huff_tab + SIG_MAP_CTX_BASE) + cat_offset + (index >> 1);
				sigctx_reg_dec = *ctx_model_ptr;
			}

			ctx_sig_last = (index & 1) ? (sigctx_reg_dec >> 16) : (sigctx_reg_dec & 0xffff);

			sig_ctx  = ctx_sig_last & 0xff;
			last_ctx = (ctx_sig_last >> 8) & 0xff;

			syn_type = CABAC_SIG_LAST;
			TwoBinArithDecHigh (syn_type, &sig_ctx, &last_ctx, &is_sig, &is_last, 0);

			/*update significant register*/
			g_sig_map_reg = g_sig_map_reg | (is_sig << coeff_pos);		

			ctx_sig_last = (last_ctx << 8) | sig_ctx;

			/*store the context into register sigctx_reg_dec*/
			if ((index & 1) == 0)
			{
				sigctx_reg_dec = (sigctx_reg_dec & 0xffff0000) | ctx_sig_last;
			}
			else
			{
				//at odd clock, write back two context mode to memory
				sigctx_reg_wb = (sigctx_reg_dec & 0xffff) | (ctx_sig_last << 16);
				*ctx_model_ptr = sigctx_reg_wb;			
			}

			if ((is_last) || (coeff_pos == maxNumCoeff-2))//结束的系数如果正好是even clock，也需要回写更新ctx
			{
				if ((index & 1) == 0 )
				{
					*ctx_model_ptr = sigctx_reg_dec;			
				}

				break;
			}
		}

	}	

	/*--- last coefficient must be significant if no last symbol was received ---*/
	
	if (!is_last)
	{
		if (blk_cat == LUMA_AC_8x8)
		{
			g_sig_map_reg2 = g_sig_map_reg2 | (1 << 31);	
		}
		else
		{
			g_sig_map_reg = g_sig_map_reg | (1 << (maxNumCoeff-1));			
		}
	}	
}

int GetPrefix0Num8x8 (int sig_map_reg, int sig_map_reg2)
{
	int prefix0_num;

	if (sig_map_reg2 & 0x80000000)
		prefix0_num = 0;
	else if (sig_map_reg2 & 0x40000000)
		prefix0_num = 1;
	else if (sig_map_reg2 & 0x20000000)
		prefix0_num = 2;
	else if (sig_map_reg2 & 0x10000000)
		prefix0_num = 3;
	else if (sig_map_reg2 & 0x8000000)
		prefix0_num = 4;
	else if (sig_map_reg2 & 0x4000000)
		prefix0_num = 5;
	else if (sig_map_reg2 & 0x2000000)
		prefix0_num = 6;
	else if (sig_map_reg2 & 0x1000000)
		prefix0_num = 7;
	else if (sig_map_reg2 & 0x800000)
		prefix0_num = 8;
	else if (sig_map_reg2 & 0x400000)
		prefix0_num = 9;
	else if (sig_map_reg2 & 0x200000)
		prefix0_num = 10;
	else if (sig_map_reg2 & 0x100000)
		prefix0_num = 11;
	else if (sig_map_reg2 & 0x80000)
		prefix0_num = 12;
	else if (sig_map_reg2 & 0x40000)
		prefix0_num = 13;
	else if (sig_map_reg2 & 0x20000)
		prefix0_num = 14;
	else if (sig_map_reg2 & 0x10000)
		prefix0_num = 15;
	else if (sig_map_reg2 & 0x8000)
		prefix0_num = 16;
	else if (sig_map_reg2 & 0x4000)
		prefix0_num = 17;
	else if (sig_map_reg2 & 0x2000)
		prefix0_num = 18;
	else if (sig_map_reg2 & 0x1000)
		prefix0_num = 19;
	else if (sig_map_reg2 & 0x800)
		prefix0_num = 20;
	else if (sig_map_reg2 & 0x400)
		prefix0_num = 21;
	else if (sig_map_reg2 & 0x200)
		prefix0_num = 22;
	else if (sig_map_reg2 & 0x100)
		prefix0_num = 23;
	else if (sig_map_reg2 & 0x80)
		prefix0_num = 24;
	else if (sig_map_reg2 & 0x40)
		prefix0_num = 25;
	else if (sig_map_reg2 & 0x20)
		prefix0_num = 26;
	else if (sig_map_reg2 & 0x10)
		prefix0_num = 27;
	else if (sig_map_reg2 & 0x8)
		prefix0_num = 28;
	else if (sig_map_reg2 & 0x4)
		prefix0_num = 29;
	else if (sig_map_reg2 & 0x2)
		prefix0_num = 30;
	else if (sig_map_reg2 & 0x1)
		prefix0_num = 31;
	else if (sig_map_reg & 0x80000000)
		prefix0_num = 32;
	else if (sig_map_reg & 0x40000000)
		prefix0_num = 33;
	else if (sig_map_reg & 0x20000000)
		prefix0_num = 34;
	else if (sig_map_reg & 0x10000000)
		prefix0_num = 35;
	else if (sig_map_reg & 0x8000000)
		prefix0_num = 36;
	else if (sig_map_reg & 0x4000000)
		prefix0_num = 37;
	else if (sig_map_reg & 0x2000000)
		prefix0_num = 38;
	else if (sig_map_reg & 0x1000000)
		prefix0_num = 39;
	else if (sig_map_reg & 0x800000)
		prefix0_num = 40;
	else if (sig_map_reg & 0x400000)
		prefix0_num = 41;
	else if (sig_map_reg & 0x200000)
		prefix0_num = 42;
	else if (sig_map_reg & 0x100000)
		prefix0_num = 43;
	else if (sig_map_reg & 0x80000)
		prefix0_num = 44;
	else if (sig_map_reg & 0x40000)
		prefix0_num = 45;
	else if (sig_map_reg & 0x20000)
		prefix0_num = 46;
	else if (sig_map_reg & 0x10000)
		prefix0_num = 47;
	else if (sig_map_reg & 0x8000)
		prefix0_num = 48;
	else if (sig_map_reg & 0x4000)
		prefix0_num = 49;
	else if (sig_map_reg & 0x2000)
		prefix0_num = 50;
	else if (sig_map_reg & 0x1000)
		prefix0_num = 51;
	else if (sig_map_reg & 0x800)
		prefix0_num = 52;
	else if (sig_map_reg & 0x400)
		prefix0_num = 53;
	else if (sig_map_reg & 0x200)
		prefix0_num = 54;
	else if (sig_map_reg & 0x100)
		prefix0_num = 55;
	else if (sig_map_reg & 0x80)
		prefix0_num = 56;
	else if (sig_map_reg & 0x40)
		prefix0_num = 57;
	else if (sig_map_reg & 0x20)
		prefix0_num = 58;
	else if (sig_map_reg & 0x10)
		prefix0_num = 59;
	else if (sig_map_reg & 0x8)
		prefix0_num = 60;
	else if (sig_map_reg & 0x4)
		prefix0_num = 61;
	else if (sig_map_reg & 0x2)
		prefix0_num = 62;
	else
		prefix0_num = 63;

	return prefix0_num;
}

void UpdateSignMap8x8 ( int * sig_map_ptr, int *sig_map_ptr2, int prefix0_num)
{
	int sig_map = *sig_map_ptr;
	int sig_map2 = *sig_map_ptr2;

	switch (prefix0_num)
	{
	case 0: 
		sig_map2  = (sig_map2 <<  1) & 0xfffffffe;
		sig_map2 |= (sig_map  >> 31) & 0x1;
		sig_map   = (sig_map  <<  1) & 0xffffffff;
		break;
	case 1: 
		sig_map2  = (sig_map2 <<  2) & 0xfffffffc;
		sig_map2 |= (sig_map  >> 30) & 0x3;
		sig_map   = (sig_map  <<  2) & 0xffffffff;
		break;
	case 2: 
		sig_map2  = (sig_map2 <<  3) & 0xfffffff8;
		sig_map2 |= (sig_map  >> 29) & 0x7;
		sig_map   = (sig_map  <<  3) & 0xffffffff;
		break;
	case 3: 
		sig_map2  = (sig_map2 <<  4) & 0xfffffff0;
		sig_map2 |= (sig_map  >> 28) & 0xf;
		sig_map   = (sig_map  <<  4) & 0xffffffff;
		break;
	case 4: 
		sig_map2  = (sig_map2 <<  5) & 0xffffffe0;
		sig_map2 |= (sig_map  >> 27) & 0x1f;
		sig_map   = (sig_map  <<  5) & 0xffffffff;
		break;
	case 5: 
		sig_map2  = (sig_map2 <<  6) & 0xffffffc0;
		sig_map2 |= (sig_map  >> 26) & 0x3f;
		sig_map   = (sig_map  <<  6) & 0xffffffff;
		break;
	case 6: 
		sig_map2  = (sig_map2 <<  7) & 0xffffff80;
		sig_map2 |= (sig_map  >> 25) & 0x7f;
		sig_map   = (sig_map  <<  7) & 0xffffffff;
		break;
	case 7: 
		sig_map2  = (sig_map2 <<  8) & 0xffffff00;
		sig_map2 |= (sig_map  >> 24) & 0xff;
		sig_map   = (sig_map  <<  8) & 0xffffffff;
		break;
	case 8: 
		sig_map2  = (sig_map2 <<  9) & 0xfffffe00;
		sig_map2 |= (sig_map  >> 23) & 0x1ff;
		sig_map   = (sig_map  <<  9) & 0xffffffff;
		break;
	case 9: 
		sig_map2  = (sig_map2 << 10) & 0xfffffc00;
		sig_map2 |= (sig_map  >> 22) & 0x3ff;
		sig_map   = (sig_map  << 10) & 0xffffffff;
		break;
	case 10: 
		sig_map2  = (sig_map2 << 11) & 0xfffff800;
		sig_map2 |= (sig_map  >> 21) & 0x7ff;
		sig_map   = (sig_map  << 11) & 0xffffffff;
		break;
	case 11: 
		sig_map2  = (sig_map2 << 12) & 0xfffff000;
		sig_map2 |= (sig_map  >> 20) & 0xfff;
		sig_map   = (sig_map  << 12) & 0xffffffff;
		break;
	case 12: 
		sig_map2  = (sig_map2 << 13) & 0xffffe000;
		sig_map2 |= (sig_map  >> 19) & 0x1fff;
		sig_map   = (sig_map  << 13) & 0xffffffff;
		break;
	case 13: 
		sig_map2  = (sig_map2 << 14) & 0xffffc000;
		sig_map2 |= (sig_map  >> 18) & 0x3fff;
		sig_map   = (sig_map  << 14) & 0xffffffff;
		break;
	case 14: 
		sig_map2  = (sig_map2 << 15) & 0xffff8000;
		sig_map2 |= (sig_map  >> 17) & 0x7fff;
		sig_map   = (sig_map  << 15) & 0xffffffff;
		break;
	case 15: 
		sig_map2  = (sig_map2 << 16) & 0xffff0000;
		sig_map2 |= (sig_map  >> 16) & 0xffff;
		sig_map   = (sig_map  << 16) & 0xffffffff;
		break;
	case 16: 
		sig_map2  = (sig_map2 << 17) & 0xfffe0000;
		sig_map2 |= (sig_map  >> 15) & 0x1ffff;
		sig_map   = (sig_map  << 17) & 0xffffffff;
		break;
	case 17: 
		sig_map2  = (sig_map2 << 18) & 0xfffc0000;
		sig_map2 |= (sig_map  >> 14) & 0x3ffff;
		sig_map   = (sig_map  << 18) & 0xffffffff;
		break;
	case 18: 
		sig_map2  = (sig_map2 << 19) & 0xfff80000;
		sig_map2 |= (sig_map  >> 13) & 0x7ffff;
		sig_map   = (sig_map  << 19) & 0xffffffff;
		break;
	case 19: 
		sig_map2  = (sig_map2 << 20) & 0xfff00000;
		sig_map2 |= (sig_map  >> 12) & 0xfffff;
		sig_map   = (sig_map  << 20) & 0xffffffff;
		break;
	case 20: 
		sig_map2  = (sig_map2 << 21) & 0xffe00000;
		sig_map2 |= (sig_map  >> 11) & 0x1fffff;
		sig_map   = (sig_map  << 21) & 0xffffffff;
		break;
	case 21: 
		sig_map2  = (sig_map2 << 22) & 0xffc00000;
		sig_map2 |= (sig_map  >> 10) & 0x3fffff;
		sig_map   = (sig_map  << 22) & 0xffffffff;
		break;
	case 22: 
		sig_map2  = (sig_map2 << 23) & 0xff800000;
		sig_map2 |= (sig_map  >>  9) & 0x7fffff;
		sig_map   = (sig_map  << 23) & 0xffffffff;
		break;
	case 23: 
		sig_map2  = (sig_map2 << 24) & 0xff000000;
		sig_map2 |= (sig_map  >>  8) & 0xffffff;
		sig_map   = (sig_map  << 24) & 0xffffffff;
		break;
	case 24: 
		sig_map2  = (sig_map2 << 25) & 0xfe000000;
		sig_map2 |= (sig_map  >>  7) & 0x1ffffff;
		sig_map   = (sig_map  << 25) & 0xffffffff;
		break;
	case 25: 
		sig_map2  = (sig_map2 << 26) & 0xfc000000;
		sig_map2 |= (sig_map  >>  6) & 0x3ffffff;
		sig_map   = (sig_map  << 26) & 0xffffffff;
		break;
	case 26: 
		sig_map2  = (sig_map2 << 27) & 0xf8000000;
		sig_map2 |= (sig_map  >>  5) & 0x7ffffff;
		sig_map   = (sig_map  << 27) & 0xffffffff;
		break;
	case 27: 
		sig_map2  = (sig_map2 << 28) & 0xf0000000;
		sig_map2 |= (sig_map  >>  4) & 0xfffffff;
		sig_map   = (sig_map  << 28) & 0xffffffff;
		break;
	case 28: 
		sig_map2  = (sig_map2 << 29) & 0xe0000000;
		sig_map2 |= (sig_map  >>  3) & 0x1fffffff;
		sig_map   = (sig_map  << 29) & 0xffffffff;
		break;
	case 29: 
		sig_map2  = (sig_map2 << 30) & 0xc0000000;
		sig_map2 |= (sig_map  >>  2) & 0x3fffffff;
		sig_map   = (sig_map  << 30) & 0xffffffff;
		break;
	case 30: 
		sig_map2  = (sig_map2 << 31) & 0x80000000;
		sig_map2 |= (sig_map  >>  1) & 0x7fffffff;
		sig_map   = (sig_map  << 31) & 0xffffffff;
		break;
	case 31: sig_map2  = (sig_map <<  0) & 0xffffffff; sig_map = 0; break;
	case 32: sig_map2  = (sig_map <<  1) & 0xffffffff; sig_map = 0; break;
	case 33: sig_map2  = (sig_map <<  2) & 0xffffffff; sig_map = 0; break;
	case 34: sig_map2  = (sig_map <<  3) & 0xffffffff; sig_map = 0; break;
	case 35: sig_map2  = (sig_map <<  4) & 0xffffffff; sig_map = 0; break;
	case 36: sig_map2  = (sig_map <<  5) & 0xffffffff; sig_map = 0; break;
	case 37: sig_map2  = (sig_map <<  6) & 0xffffffff; sig_map = 0; break;
	case 38: sig_map2  = (sig_map <<  7) & 0xffffffff; sig_map = 0; break;
	case 39: sig_map2  = (sig_map <<  8) & 0xffffffff; sig_map = 0; break;
	case 40: sig_map2  = (sig_map <<  9) & 0xffffffff; sig_map = 0; break;
	case 41: sig_map2  = (sig_map <<  10) & 0xffffffff; sig_map = 0; break;
	case 42: sig_map2  = (sig_map <<  11) & 0xffffffff; sig_map = 0; break;
	case 43: sig_map2  = (sig_map <<  12) & 0xffffffff; sig_map = 0; break;
	case 44: sig_map2  = (sig_map <<  13) & 0xffffffff; sig_map = 0; break;
	case 45: sig_map2  = (sig_map <<  14) & 0xffffffff; sig_map = 0; break;
	case 46: sig_map2  = (sig_map <<  15) & 0xffffffff; sig_map = 0; break;
	case 47: sig_map2  = (sig_map <<  16) & 0xffffffff; sig_map = 0; break;
	case 48: sig_map2  = (sig_map <<  17) & 0xffffffff; sig_map = 0; break;
	case 49: sig_map2  = (sig_map <<  18) & 0xffffffff; sig_map = 0; break;
	case 50: sig_map2  = (sig_map <<  19) & 0xffffffff; sig_map = 0; break;
	case 51: sig_map2  = (sig_map <<  20) & 0xffffffff; sig_map = 0; break;
	case 52: sig_map2  = (sig_map <<  21) & 0xffffffff; sig_map = 0; break;
	case 53: sig_map2  = (sig_map <<  22) & 0xffffffff; sig_map = 0; break;
	case 54: sig_map2  = (sig_map <<  23) & 0xffffffff; sig_map = 0; break;
	case 55: sig_map2  = (sig_map <<  24) & 0xffffffff; sig_map = 0; break;
	case 56: sig_map2  = (sig_map <<  25) & 0xffffffff; sig_map = 0; break;
	case 57: sig_map2  = (sig_map <<  26) & 0xffffffff; sig_map = 0; break;
	case 58: sig_map2  = (sig_map <<  27) & 0xffffffff; sig_map = 0; break;
	case 59: sig_map2  = (sig_map <<  28) & 0xffffffff; sig_map = 0; break;
	case 60: sig_map2  = (sig_map <<  29) & 0xffffffff; sig_map = 0; break;
	case 61: sig_map2  = (sig_map <<  30) & 0xffffffff; sig_map = 0; break;
	case 62: sig_map2  = (sig_map <<  31) & 0xffffffff; sig_map = 0; break;
	case 63: sig_map2  = 0; sig_map = 0; break;
	default: sig_map2  = 0; sig_map = 0;
	}

	*sig_map_ptr = sig_map;
	*sig_map_ptr2 = sig_map2;
}

void ArithLevInforDecHigh (int blk_type, int blk_id)//这里主要有一些关于系数的东西是需要倒一下的，其他的应该没有什么变化
{
	int		coeff_pos;
	int		prefix0_num;
	int		coeff;
	int		num_t1;
	int		num_lgt1;
	int		level;
	int		blk_base;
	int		nzf_base;
	uint32  nz_flag;
	int16 * blk_ptr;
	int		offset_coeff;
	int		offset_nzf;
	int		blk_coeff_base_addr;
	int		blk_nzf_base_addr;

	int		start_pos;
	int		asic_order;
	int		hvld_dbuf_addr;
	int     hvld_dbuf_addr1,blk_nzf_base_addr1,position1,position2;

	/*coeff is decoded in reversed scan*/
	coeff_pos	= (blk_type == LUMA_AC_8x8) ? 64 : 16;
	num_t1		= 0;
	num_lgt1	= 0;

	nz_flag		= 0;

	start_pos = ((blk_type == LUMA_AC_I16) || (blk_type == CHROMA_AC)) ? 1 : 0;
	
	/*derive the block base address, and coefficient start position*///需要考虑8x8变换
	if (blk_type == LUMA_DC)//当采用的是luma16x16模式的时候，第一个word的前两个字节用来存储non_zero_flag
	{
		offset_coeff		= 0;
		offset_nzf			= 0;
		blk_coeff_base_addr = COEFF_LUMA_DC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_LUMA_DC_BASE_HIGH; //如果模式是luma_DC的话，那么luma_ac中的第一个系数，也就是第一个word的前16个比特肯定是用不到的。
	}
	else if ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16))
	{
// 		if (currMBBuf->transform_size_8x8_flag && blk_type == LUMA_AC)//CAVLC的时候8x8还是按照4个4x4块来做，所以还是先遍历16个块，但是当是CABAC的时候，这个需要重新改正了，因为8x8就是一个8x8的系数来做CABAC的。
// 		{
//			offset_coeff = (blk_id/4) * 8 * 4;			//为了将各个标准的统一起来，按行扫描来做（第0，1，2，3个4x4块从0开始，然后4，5，6，7从第二个8x8块起）
//			offset_nzf	 = (blk_id/4) * 2;				//start from 0 2 4 6
//		}
//		else
//		{
			offset_coeff = blk_id * 8;//g_inverse_scan_ord_tbl[blk_id] * 8;			//为了将各个标准的统一起来，按行扫描来做
			offset_nzf	 = blk_id / 2;//g_inverse_scan_ord_tbl[blk_id] / 2;//weihu
//		}

		blk_coeff_base_addr = COEFF_LUMA_AC_BASE_HIGH;
		blk_nzf_base_addr	= NZFLAG_LUMA_BASE_HIGH;
	}
	else if (blk_type == LUMA_AC_8x8)
	{
		offset_coeff = blk_id * 32;			//为了将各个标准的统一起来，按行扫描来做
		offset_nzf	 = blk_id * 2;

		blk_coeff_base_addr = COEFF_LUMA_AC_BASE_HIGH;
		blk_nzf_base_addr	= NZFLAG_LUMA_BASE_HIGH;
	}
	else if (blk_type == CHROMA_DC)
	{
		offset_coeff		= blk_id * 2;//四个系数，占两个word
		offset_nzf			= 0;
		blk_coeff_base_addr = COEFF_CHROMA_DC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_CHROMA_DC_BASE_HIGH;//weihu	//chroma AC 的第一个系数位置肯定用不到。
	}
	else
	{
		offset_coeff		= blk_id * 8;//4个块，所以不需要倒顺序
		offset_nzf			= blk_id / 2;
		blk_coeff_base_addr = COEFF_CHROMA_AC_BASE_HIGH;
		blk_nzf_base_addr   = NZFLAG_CHROMA_BASE_HIGH;
	}

	blk_coeff_base_addr += offset_coeff;
	blk_nzf_base_addr   += offset_nzf;

	blk_base = blk_coeff_base_addr;
	
	nzf_base = blk_nzf_base_addr;

	while (g_sig_map_reg != 0 || (/*blk_type != LUMA_AC_8x8 || */g_sig_map_reg2 != 0))
	{		
		/*get coefficient position*/
		if (blk_type == LUMA_AC_8x8)
			prefix0_num = GetPrefix0Num8x8 (g_sig_map_reg, g_sig_map_reg2);
		else
			prefix0_num = GetPrefix0Num (g_sig_map_reg);

		coeff_pos = coeff_pos - prefix0_num - 1;

		/*decode one level information*/
		coeff = ArithOneLevDecHigh (blk_type, num_t1, num_lgt1);

		level = ABS(coeff);

		if (level > 1)
		{
			num_lgt1++;
		}

		if (level == 1)
		{
			num_t1++;
		}

		/*update the sig_map_reg*/
		if (blk_type == LUMA_AC_8x8)
			UpdateSignMap8x8 (&g_sig_map_reg, &g_sig_map_reg2, prefix0_num);
		else
			UpdateSignMap (&g_sig_map_reg, prefix0_num);

		blk_ptr = (int16 *)(vsp_dct_io_0 + blk_base);

		if (blk_type == LUMA_AC_8x8)
		{
			asic_order = g_is_dctio_8x8_order_cabac[coeff_pos];
			position1 = g_is_dctio_8x8_order_ori[coeff_pos];//weihu
			position2= position1%8+((blk_id/4)&1)*8+(position1/8+(blk_id/8)*8)*16;
			hvld_dbuf_addr1 = COEFF_LUMA_AC_BASE_HIGH*2 + position2;
			blk_nzf_base_addr1=NZFLAG_LUMA_BASE_HIGH+(position2/32);
		}
		else
		{
			asic_order = (blk_type == CHROMA_DC) ? g_is_dctio_chroma_dc_order[coeff_pos] : g_is_dctio_4x4_order[coeff_pos];//因为chroma_DC只有四个系数，所以不需要倒顺序
			position1 = (blk_type == CHROMA_DC) ? coeff_pos : g_is_dctio_4x4_order_ori[coeff_pos];//weihu
			
			if (blk_type == CHROMA_DC) 
			{
				position2= position1 + blk_id * 4;
				hvld_dbuf_addr1 = COEFF_CHROMA_DC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_CHROMA_DC_BASE_HIGH+(position2/32);
			}
			else if (blk_type == LUMA_DC)
			{
				position2= position1;
				hvld_dbuf_addr1 = COEFF_LUMA_DC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_LUMA_DC_BASE_HIGH+(position2/32);
			}
			else if ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16))
			{
				position2= position1%4+(g_inverse_scan_ord_tbl[blk_id]%4)*4+(position1/4+(g_inverse_scan_ord_tbl[blk_id]/4)*4)*16;
				hvld_dbuf_addr1 = COEFF_LUMA_AC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_LUMA_BASE_HIGH+(position2/32);
			}
			else//chroma_ac
			{
                position2= position1%4+(blk_id%2)*4+(position1/4+(blk_id/2)*4)*8;
				hvld_dbuf_addr1 = COEFF_CHROMA_AC_BASE_HIGH*2 + position2;
				blk_nzf_base_addr1=NZFLAG_CHROMA_BASE_HIGH+(position2/32);
			}
			
		}	

		blk_ptr[asic_order] = coeff;
		((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] = (int16)level;//weihu

		/*nz_flag = nz_flag | (1 << asic_order);*/
		if (blk_type == LUMA_AC_8x8)
		{
			if (asic_order < 32)
			{
				hvld_dbuf_addr = nzf_base;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << asic_order);
				
			}
			else
			{
				hvld_dbuf_addr = nzf_base + 1;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << (asic_order - 32));
			
			}
			hvld_dbuf_addr1 = blk_nzf_base_addr1;//
			vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
		}
		else if (blk_type == LUMA_AC || blk_type == LUMA_AC_I16 || blk_type == CHROMA_AC)
		{
			if (blk_id & 1)
			{
				hvld_dbuf_addr = nzf_base;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << (asic_order + 16));
			}
			else
			{
				hvld_dbuf_addr = nzf_base;
				vsp_dct_io_0[hvld_dbuf_addr] |= (1 << asic_order);
			}
			hvld_dbuf_addr1 = blk_nzf_base_addr1;//
			vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
		}
		else if (blk_type == LUMA_DC)
		{
			hvld_dbuf_addr = nzf_base * 2;
			((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << asic_order);
			hvld_dbuf_addr1 = blk_nzf_base_addr1;//
			vsp_dct_io_1[hvld_dbuf_addr1] |= (1 << position2%32);//
		}
		else if (blk_type == CHROMA_DC)
		{
			if (blk_id & 1)
			{
				hvld_dbuf_addr = nzf_base * 2;
				((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << (asic_order + 4));
				hvld_dbuf_addr1 = blk_nzf_base_addr1 * 2+1;
				((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] |= (1 << (coeff_pos));//weihu
			}
			else
			{
				hvld_dbuf_addr = nzf_base * 2;
				((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] |= (1 << asic_order);
				hvld_dbuf_addr1 = blk_nzf_base_addr1 * 2;
				((int16 *)vsp_dct_io_1)[hvld_dbuf_addr1] |= (1 << coeff_pos );//weihu
			}	
		}
	}	

	//nzf_base[blk_id] = nz_flag;
/*	vsp_dct_io_0[nzf_base+blk_id] = nz_flag;*/
}

int32 readBlkCodedFlag_CABAC_High (int blk_cat, int blk_id)
{
	int		is_bp_mode;
	uint32	bsm_data;
	int		shift_bits;

	int		coded_flag_a;
	int		coded_flag_b;

	int		cbf;

	int		ctx_idx;
	int		ctx_idx_inc;
	int		ctx_idx_offset;
	uint8 * context_ptr;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	
	GetnCodedBlockFlag (
						/*input*/
						blk_cat, 
						blk_id, 
						
						/*output*/
						&coded_flag_a,
						&coded_flag_b
					);	

	/*compute ctx_idx_inc*/
	ctx_idx_inc		= coded_flag_a + 2*coded_flag_b;
	ctx_idx_offset	= blk_cat*4;
	ctx_idx			= ctx_idx_offset + ctx_idx_inc;

	context_ptr		= (uint8 *)(g_hvld_huff_tab+CODED_FLAG_CTX_BASE) + ctx_idx;

	/*arithmetic decoding*/
	is_bp_mode	= 0;
	bsm_data	= (uint32)BITSTREAMSHOWBITS (stream, 32);
	
	cbf = BiArithDec (
				is_bp_mode,	
				&bsm_data,	
				&g_range,	 
				&g_offset,	
				context_ptr, 
				&shift_bits
			);

	BITSTREAMFLUSHBITS (stream, shift_bits); 

	WriteBackTotalCoeffHigh (blk_cat, blk_id, cbf);

	return cbf;
}

void GetnCodedBlockFlag(int blk_type, int blk_id, int *coded_flag_a_ptr, int *coded_flag_b_ptr)//需要额外考虑8x8变换的问题
{
	int			na;				//left block's nnz
	int			nb;				//top block's nnz
	int			lblk_avail;		//left block available
	int			tblk_avail;		//top block available
	
	uint32		mb_top_nnz;
	uint32		mb_left_nnz;
	uint32		cur_nnz;

	int			coded_flag_a;
	int			coded_flag_b;

	int			is_intra;
	int			default_value;
	LINE_BUF_T *MBLineBuf;
	int			mb_x;
	int			mb_type;
	int			slice_type;

	slice_type	= currMBBuf->slice_type;
	mb_type		= currMBBuf->mb_type;
	mb_x		= currMBBuf->mb_x;
	MBLineBuf	= vldLineBuf + mb_x;

	is_intra = (IS_I_NxN || IS_I_16x16 || IS_I_PCM) ? 1 : 0;
	default_value = is_intra ? 1 : 0;

	if (blk_type == CHROMA_DC)
	{
		int shift_bits;
		
		shift_bits = (blk_id == 0) ? 1 : 2;

		if (currMBBuf->lmb_avail)
		{		
			coded_flag_a = (leftMBBuf->coded_dc_flag >> shift_bits) & 1;
		}
		else
		{
			coded_flag_a = default_value;
		}

		if (currMBBuf->tmb_avail)
		{
			coded_flag_b = (MBLineBuf->coded_dc_flag >> shift_bits) & 1;
		}
		else
		{
			coded_flag_b = default_value;
		}
	}
	else if (blk_type == CHROMA_AC)
	{
		mb_top_nnz	= (blk_id < 4) ? (MBLineBuf->nnz_c >> 16) : (MBLineBuf->nnz_c & 0x1fff);
		mb_left_nnz	= (blk_id < 4) ?  leftMBBuf->nnz_cb       :  leftMBBuf->nnz_cr;
		cur_nnz		= (blk_id < 4) ?  currMBBuf->nnz_cb       :  currMBBuf->nnz_cr;

		blk_id = blk_id & 3;
		
		if(blk_id == 0)
		{
			na = (mb_left_nnz >> 16) & 0x1f;
			nb = (mb_top_nnz  >> 8 ) & 0x1f;
			lblk_avail = currMBBuf->lmb_avail;
			tblk_avail = currMBBuf->tmb_avail;
		}
		else if (blk_id == 1)
		{
			na = (cur_nnz >> 24) & 0x1f;
			nb = (mb_top_nnz   ) & 0x1f;
			lblk_avail = 1;
			tblk_avail = currMBBuf->tmb_avail;
		}
		else if (blk_id == 2)
		{
			na = (mb_left_nnz >> 0 ) & 0x1f;
			nb = (cur_nnz     >> 24) & 0x1f;
			lblk_avail = currMBBuf->lmb_avail;
			tblk_avail = 1;
		}
		else
		{
			na = (cur_nnz >> 8 ) & 0x1f;
			nb = (cur_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = 1;
		}

		coded_flag_a = lblk_avail ? (na != 0) : default_value;
		coded_flag_b = tblk_avail ? (nb != 0) : default_value;
	}
	else
	{
		if (blk_type == LUMA_DC)
		{
			if (currMBBuf->lmb_avail)
			{
				coded_flag_a = (leftMBBuf->coded_dc_flag >> 0) & 1;
			}
			else
			{
				coded_flag_a = default_value;
			}
			
			if (currMBBuf->tmb_avail)
			{
				coded_flag_b = (MBLineBuf->coded_dc_flag >> 0) & 1;
			}
			else
			{
				coded_flag_b = default_value;
			}
		}
		else
		{
			tblk_avail = ((blk_id == 0) | (blk_id == 1) | (blk_id == 4) | (blk_id == 5)) ? currMBBuf->tmb_avail : 1;
			lblk_avail = ((blk_id == 0) | (blk_id == 2) | (blk_id == 8) | (blk_id == 10)) ? currMBBuf->lmb_avail : 1;

			mb_top_nnz	= MBLineBuf->nnz_y;
			mb_left_nnz	= (blk_id < 8) ? leftMBBuf->nnz_y1 : leftMBBuf->nnz_y3;

			if (blk_id < 8)
			{
				if (blk_id < 4)
				{
					if (blk_id == 0)
					{
						na = (mb_left_nnz >> 16) & 0x1f;
						nb = (mb_top_nnz  >> 24) & 0x1f;
					}
					else if (blk_id == 1)
					{
						na = (currMBBuf->nnz_y0 >> 24) & 0x1f;
						nb = (mb_top_nnz >> 16) & 0x1f;
					}
					else if (blk_id == 2)
					{
						na = (mb_left_nnz >> 0) & 0x1f;
						nb = (currMBBuf->nnz_y0 >> 24) & 0x1f;
					}
					else
					{
						na = (currMBBuf->nnz_y0 >> 8) & 0x1f;
						nb = (currMBBuf->nnz_y0 >> 16) & 0x1f;
					}
				}
				else
				{
					if (blk_id == 4)
					{
						na = (currMBBuf->nnz_y0 >> 16) & 0x1f;
						nb = (mb_top_nnz >> 8) & 0x1f;
					}
					else if (blk_id == 5)
					{
						na = (currMBBuf->nnz_y1 >> 24) & 0x1f;
						nb = (mb_top_nnz >> 0) & 0x1f;
					}
					else if (blk_id == 6)
					{
						na = (currMBBuf->nnz_y0 >> 0) & 0x1f;
						nb = (currMBBuf->nnz_y1 >> 24) & 0x1f;
					}
					else
					{
						na = (currMBBuf->nnz_y1 >> 8) & 0x1f;
						nb = (currMBBuf->nnz_y1 >> 16) & 0x1f;
					}				
				}
			}
			else
			{
				if (blk_id < 12)
				{
					if (blk_id == 8)
					{
						na = (mb_left_nnz >> 16) & 0x1f;
						nb = (currMBBuf->nnz_y0 >> 8) & 0x1f;
					}
					else if (blk_id == 9)
					{
						na = (currMBBuf->nnz_y2 >> 24) & 0x1f;
						nb = (currMBBuf->nnz_y0 >> 0) & 0x1f;
					}
					else if (blk_id == 10)
					{
						na = (mb_left_nnz >> 0) & 0x1f;
						nb = (currMBBuf->nnz_y2 >> 24) & 0x1f;
					}
					else
					{
						na = (currMBBuf->nnz_y2 >> 8) & 0x1f;
						nb = (currMBBuf->nnz_y2 >> 16) & 0x1f;
					}
				}
				else
				{
					if (blk_id == 12)
					{
						na = (currMBBuf->nnz_y2 >> 16) & 0x1f;
						nb = (currMBBuf->nnz_y1 >> 8) & 0x1f;
					}
					else if (blk_id == 13)
					{
						na = (currMBBuf->nnz_y3 >> 24) & 0x1f;
						nb = (currMBBuf->nnz_y1 >> 0) & 0x1f;
					}
					else if (blk_id == 14)
					{
						na = (currMBBuf->nnz_y2 >> 0) & 0x1f;
						nb = (currMBBuf->nnz_y3 >> 24) & 0x1f;
					}
					else
					{
						na = (currMBBuf->nnz_y3 >> 8) & 0x1f;
						nb = (currMBBuf->nnz_y3 >> 16) & 0x1f;
					}				
				}
			}

			coded_flag_a = lblk_avail ? (na != 0) : default_value;
			coded_flag_b = tblk_avail ? (nb != 0) : default_value;
		}
	}
	
	*coded_flag_a_ptr = coded_flag_a;
	*coded_flag_b_ptr = coded_flag_b;
}

/******************************************************************
			derive nc according to block type, block id
*******************************************************************/
void GetnC (
					 /*input*/
					 int		blk_type, 
					 int		blk_id, 
					 /*output*/
					 int	*	nc_ptr
					 )
{
	int			na;				//left block's nnz
	int			nb;				//top block's nnz
	int			nc;
	int			lblk_avail;		//left block available
	int			tblk_avail;		//top block available
	int			mb_x;
	
	uint32		mb_top_nnz;
	uint32		mb_left_nnz;
	uint32		cur_nnz;
	LINE_BUF_T *MBLineBuf;

	mb_x		= currMBBuf->mb_x;
	MBLineBuf	= vldLineBuf + mb_x;

	if (blk_type == CHROMA_DC)
	{
			nc = -1;
	}
	else if (blk_type == CHROMA_AC)
	{
		mb_top_nnz	= (blk_id < 4) ? (MBLineBuf->nnz_c >> 16) : (MBLineBuf->nnz_c & 0x1fff);
		mb_left_nnz	= (blk_id < 4) ?  leftMBBuf->nnz_cb       :  leftMBBuf->nnz_cr;
		cur_nnz		= (blk_id < 4) ?  currMBBuf->nnz_cb       :  currMBBuf->nnz_cr;

		blk_id = blk_id & 3;
		
		if(blk_id == 0)
		{
			na = (mb_left_nnz >> 16) & 0x1f;
			nb = (mb_top_nnz  >> 8 ) & 0x1f;
			lblk_avail = currMBBuf->lmb_avail;
			tblk_avail = currMBBuf->tmb_avail;
		}
		else if (blk_id == 1)
		{
			na = (cur_nnz >> 24) & 0x1f;
			nb = (mb_top_nnz   ) & 0x1f;
			lblk_avail = 1;
			tblk_avail = currMBBuf->tmb_avail;
		}
		else if (blk_id == 2)
		{
			na = (mb_left_nnz >> 0 ) & 0x1f;
			nb = (cur_nnz     >> 24) & 0x1f;
			lblk_avail = currMBBuf->lmb_avail;
			tblk_avail = 1;
		}
		else
		{
			na = (cur_nnz >> 8 ) & 0x1f;
			nb = (cur_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = 1;
		}
	}
	else
	{
		if (blk_type == LUMA_DC)
			blk_id = 0;

		tblk_avail = ((blk_id == 0) | (blk_id == 1) | (blk_id == 4) | (blk_id == 5)) ? currMBBuf->tmb_avail : 1;
		lblk_avail = ((blk_id == 0) | (blk_id == 2) | (blk_id == 8) | (blk_id == 10)) ? currMBBuf->lmb_avail : 1;

		mb_top_nnz	= MBLineBuf->nnz_y;
		mb_left_nnz	= (blk_id < 8) ? leftMBBuf->nnz_y1 : leftMBBuf->nnz_y3;

		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				if (blk_id == 0)
				{
					na = (mb_left_nnz >> 16) & 0x1f;
					nb = (mb_top_nnz  >> 24) & 0x1f;
				}
				else if (blk_id == 1)
				{
					na = (currMBBuf->nnz_y0 >> 24) & 0x1f;
					nb = (mb_top_nnz >> 16) & 0x1f;
				}
				else if (blk_id == 2)
				{
					na = (mb_left_nnz >> 0) & 0x1f;
					nb = (currMBBuf->nnz_y0 >> 24) & 0x1f;
				}
				else
				{
					na = (currMBBuf->nnz_y0 >> 8) & 0x1f;
					nb = (currMBBuf->nnz_y0 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 4)
				{
					na = (currMBBuf->nnz_y0 >> 16) & 0x1f;
					nb = (mb_top_nnz >> 8) & 0x1f;
				}
				else if (blk_id == 5)
				{
					na = (currMBBuf->nnz_y1 >> 24) & 0x1f;
					nb = (mb_top_nnz >> 0) & 0x1f;
				}
				else if (blk_id == 6)
				{
					na = (currMBBuf->nnz_y0 >> 0) & 0x1f;
					nb = (currMBBuf->nnz_y1 >> 24) & 0x1f;
				}
				else
				{
					na = (currMBBuf->nnz_y1 >> 8) & 0x1f;
					nb = (currMBBuf->nnz_y1 >> 16) & 0x1f;
				}				
			}
		}
		else
		{
			if (blk_id < 12)
			{
				if (blk_id == 8)
				{
					na = (mb_left_nnz >> 16) & 0x1f;
					nb = (currMBBuf->nnz_y0 >> 8) & 0x1f;
				}
				else if (blk_id == 9)
				{
					na = (currMBBuf->nnz_y2 >> 24) & 0x1f;
					nb = (currMBBuf->nnz_y0 >> 0) & 0x1f;
				}
				else if (blk_id == 10)
				{
					na = (mb_left_nnz >> 0) & 0x1f;
					nb = (currMBBuf->nnz_y2 >> 24) & 0x1f;
				}
				else
				{
					na = (currMBBuf->nnz_y2 >> 8) & 0x1f;
					nb = (currMBBuf->nnz_y2 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 12)
				{
					na = (currMBBuf->nnz_y2 >> 16) & 0x1f;
					nb = (currMBBuf->nnz_y1 >> 8) & 0x1f;
				}
				else if (blk_id == 13)
				{
					na = (currMBBuf->nnz_y3 >> 24) & 0x1f;
					nb = (currMBBuf->nnz_y1 >> 0) & 0x1f;
				}
				else if (blk_id == 14)
				{
					na = (currMBBuf->nnz_y2 >> 0) & 0x1f;
					nb = (currMBBuf->nnz_y3 >> 24) & 0x1f;
				}
				else
				{
					na = (currMBBuf->nnz_y3 >> 8) & 0x1f;
					nb = (currMBBuf->nnz_y3 >> 16) & 0x1f;
				}				
			}
		}
	}

	if (blk_type != CHROMA_DC)
	{
		if (tblk_avail & lblk_avail)
		{
			nc = (na + nb + 1) >> 1;
		}
		else if (lblk_avail)
		{
			nc = na;
		}
		else if (tblk_avail)
		{
			nc = nb;
		}
		else
		{
			nc = 0;
		}
	}
	else
	{
		nc = -1;
	}

	*nc_ptr = nc;
}

int CoeffTokenDecHigh (
					int			nc, 
					int		*	trailing_one_ptr, 
					int		*	total_coeff_ptr
					)
{
	uint32		bsm_token_data;
	uint32		token_tbuf_addr;
	int			code_len;
	int			token_error = 0;
	int			offset;
	int			trailing_one;
	int			total_coeff;
	uint32		tbuf_token_data;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	
	
	bsm_token_data = (uint32)(BITSTREAMSHOWBITS (stream, 32));
	
	if (nc == -1)
	{
		
		/*combinational logic to derive coeff_token*/
		if (bsm_token_data & 0xf0000000)			//prefix0 number < 4 
		{
			if (bsm_token_data & 0x80000000)
			{
				trailing_one = 1;
				total_coeff  = 1;
				code_len	 = 1;
			}
			else if (bsm_token_data & 0x40000000)
			{
				trailing_one = 0;
				total_coeff  = 0;
				code_len	 = 2;
			}
			else if (bsm_token_data & 0x20000000)
			{
				trailing_one = 2;
				total_coeff  = 2;
				code_len	 = 3;
			}
			else
			{
				int suffix_2bits;
				
				code_len	 = 6;

				suffix_2bits = (bsm_token_data >> 26) & 0x3;

				if (suffix_2bits == 3)
				{
					trailing_one = 0;
					total_coeff  = 1;
				}
				else if (suffix_2bits == 0)
				{
					trailing_one = 0;
					total_coeff  = 2;
				}
				else if (suffix_2bits == 2)
				{
					trailing_one = 1;
					total_coeff  = 2;
				}
				else
				{					
					trailing_one = 3;
					total_coeff	 = 3;
				}
			}
		}
		else
		{
			if (bsm_token_data & 0x08000000) 
			{
				code_len	 = 6;

				if ((bsm_token_data >> 26) & 1)
				{
					trailing_one = 0;
					total_coeff  = 3;
				}
				else
				{
					trailing_one = 0;
					total_coeff  = 4;
				}
			}
			else if (bsm_token_data & 0x04000000) 
			{
				code_len	 = 7;
					
				if ((bsm_token_data >> 25) & 1)
				{
					trailing_one = 1;
					total_coeff  = 3;
				}
				else
				{
					trailing_one = 2;
					total_coeff  = 3;
				}
			}
			else if (bsm_token_data & 0x02000000) 
			{
				code_len	 = 8;

				if ((bsm_token_data >> 24) & 1)
				{
					trailing_one = 1;
					total_coeff  = 4;
				}
				else
				{
					trailing_one = 2;
					total_coeff  = 4;
				}
			}
			else
			{
				trailing_one = 3;
				total_coeff  = 4;
				code_len	 = 7;
			}
		}		
	}
	else if (nc <4)
	{
		if (nc < 2)
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					token_tbuf_addr = 0;
					code_len		= 1;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					token_tbuf_addr = 1;
					code_len		= 2;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					token_tbuf_addr = 2;
					code_len		= 3;
				}
				else if (bsm_token_data & 0x10000000)  //3 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x3;	// =1 or =0 or =2
					token_tbuf_addr = 4 | offset;// = 4 or 5 or 6
					code_len		= (bsm_token_data & 0x08000000) ? 5 : 6;				
				}
				else if (bsm_token_data & 0x08000000)  //4 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x3;
					token_tbuf_addr = 8 | offset;
					code_len		= (bsm_token_data & 0x04000000) ? 6 : 7;
				}
				else if (bsm_token_data & 0x04000000)  //5 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x3;
					token_tbuf_addr = 12 | offset;
					code_len		= 8;				
				}
				else if (bsm_token_data & 0x02000000)  //6 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x3;
					token_tbuf_addr = 16 | offset;
					code_len		= 9;				
				}
				else
				{
					offset			= (bsm_token_data >> 22) & 0x3;
					token_tbuf_addr = 20 | offset;
					code_len		= 10;				
				}
			}
			else
			{
				if (bsm_token_data & 0x00800000)    //8 prefix_0
				{
					offset			= (bsm_token_data >> 21) & 0x3;
					token_tbuf_addr = 24 | offset;
					code_len		= 11;				
				}
				else if (bsm_token_data & 0x00400000)      //9 prefix_0 
				{
					offset			= (bsm_token_data >> 19) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 13;				
				}
				else if (bsm_token_data & 0x00200000)    //10 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 14;				
				}
				else if (bsm_token_data & 0x00100000)    //11 prefix_0
				{
					offset			= (bsm_token_data >> 17) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= 15;				
				}
				else if (bsm_token_data & 0x00080000)    //12 prefix_0
				{
					offset			= (bsm_token_data >> 16) & 0x7;
					token_tbuf_addr = 56 | offset;
					code_len		= 16;				
				}
				else if (bsm_token_data & 0x00040000)    //13 prefix_0
				{
					offset			= (bsm_token_data >> 16) & 0x3;
					token_tbuf_addr = 64 | offset;
					code_len		= 16;				
				}
				else if (bsm_token_data & 0x00020000)    //14 prefix_0
				{
					token_tbuf_addr = 68;
					code_len		= 15;				
				}
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}				
		}
		else
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					offset			= (bsm_token_data >> 30) & 0x1;
					token_tbuf_addr = 0 | offset;
					code_len		= 2;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					offset			= (bsm_token_data >> 28) & 0x3;
					token_tbuf_addr = 4 | offset;
					code_len		= (bsm_token_data & 0x20000000) ? 3 : 4;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x7;
					token_tbuf_addr = 8 | offset;
					code_len		= (bsm_token_data & 0x10000000) ? 5 : 6;
				}
				else if (bsm_token_data & 0x10000000)	 //3 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x3;
					token_tbuf_addr = 16 | offset;
					code_len		= 6;
				}
				else if (bsm_token_data & 0x08000000)	 //4 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x3;
					token_tbuf_addr = 20 | offset;
					code_len		= 7;
				}
				else if (bsm_token_data & 0x04000000)	 //5 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x3;
					token_tbuf_addr = 24 | offset;
					code_len		= 8;
				}
				else if (bsm_token_data & 0x02000000)	 //6 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x3;
					token_tbuf_addr = 28 | offset;
					code_len		= 9;
				}
				else if (bsm_token_data & 0x01000000)	 //7 prefix_0
				{
					offset			= (bsm_token_data >> 21) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 11;
				}
				
			}
			else
			{
				if (bsm_token_data & 0x00800000)	 //8 prefix_0
				{
					offset			= (bsm_token_data >> 20) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 12;
				}
				else if (bsm_token_data & 0x00400000)	 //9 prefix_0
				{
					offset			= (bsm_token_data >> 19) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= 13;
				}
				else if (bsm_token_data & 0x00200000)	 //10 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x7;
					token_tbuf_addr = 56 | offset;
					code_len		= (bsm_token_data & 0x00100000) ? 13 : 14;
				}
				else if (bsm_token_data & 0x00100000)	 //11 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x3;
					token_tbuf_addr = 64 | offset;
					code_len		= 14;
				}
				else if (bsm_token_data & 0x00080000)	 //12 prefix_0
				{
					token_tbuf_addr = 68;
					code_len		= 13;
				}
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}			
		}
	}
	else
	{
		if (nc < 8)
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					offset			= (bsm_token_data >> 28) & 0x7;
					token_tbuf_addr = 0 | offset;
					code_len		= 4;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					offset			= (bsm_token_data >> 27) & 0x7;
					token_tbuf_addr = 8 | offset;
					code_len		= 5;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x7;
					token_tbuf_addr = 16 | offset;
					code_len		= 6;
				}
				else if (bsm_token_data & 0x10000000)  //3 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x7;
					token_tbuf_addr = 24 | offset;
					code_len		= 7;
				}
				else if (bsm_token_data & 0x08000000)  //4 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 8;
				}
				else if (bsm_token_data & 0x04000000)  //5 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 9;
				}
				else if (bsm_token_data & 0x02000000)  //6 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= (((bsm_token_data >> 23) & 0x3) == 0x3) ? 9 : 10;
				}
				else  //7 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x3;
					token_tbuf_addr = 56 | offset;
					code_len		= 10;
				}
			}
			else
			{
				if (bsm_token_data & 0x00800000)  //8 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x1;
					token_tbuf_addr = 60 | offset;
					code_len		= 10;
				}
				else if (bsm_token_data & 0x00400000)  //7 prefix_0
				{
					token_tbuf_addr = 62;
					code_len		= 10;
				} 
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}
		}
		else
		{
			uint32		next_6bits;

			next_6bits = (bsm_token_data >> 26) & 0x3f;

			if ((next_6bits == 2) | (next_6bits == 7))
			{
				token_error		= 1;
				token_tbuf_addr = 0;
				code_len		= 0;
			}
			else
			{
				token_tbuf_addr = next_6bits;
				code_len		= 6;
			}
		}
	}	

	if (token_error)
		return 1;

	/*look up table*/
	if (nc != -1)
	{
		int token;

		tbuf_token_data = g_hvld_huff_tab [token_tbuf_addr];

		if (nc < 2)
		{
			token = (tbuf_token_data >> 24) & 0xff;
		}
		else if (nc < 4)
		{
			token = (tbuf_token_data >> 16) & 0xff;
		}
		else if (nc < 8)
		{
			token = (tbuf_token_data >> 8) & 0xff;
		}
		else
		{
			token = (tbuf_token_data >> 0) & 0xff;
		}

		trailing_one = (token >> 6) & 3;	// 1 1           1 1 1 1 1
		total_coeff  = token & 0x1f;
	}

	BITSTREAMFLUSHBITS (stream, code_len);

	*trailing_one_ptr	= trailing_one;
	*total_coeff_ptr	= total_coeff;

	return 0;
}

int H264VldLevDecHigh (
					int		trailing_one, 
					int		total_coeff
					)
{
	int		k;
	int		level;
	int		sign;
	uint32	bsm_lev_data;
	int		level_two_or_higher;
	int		suffix_len;
	int		lev_tbuf_addr;
	int		suffix_len_inc;
	int		lev_err;
	int		trailing_one_left;
	int		lev_num = 0;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	
	/*trailing one sign*/
	trailing_one_left = trailing_one;
	while (trailing_one_left != 0)
	{
		bsm_lev_data = (uint32)(BITSTREAMSHOWBITS (stream, 32));
		
		sign = (bsm_lev_data >> 31) & 1;
		
		level = 1 - sign*2;

		lev_tbuf_addr = total_coeff - 1 - (trailing_one - trailing_one_left) + LEV_TBUF_BASE_ADDR;//这里可能要改，注意!看了一下貌似不用改
		g_hvld_huff_tab[lev_tbuf_addr] = level;

		BITSTREAMFLUSHBITS (stream, 1);

		trailing_one_left--;
	}

	if (total_coeff == trailing_one)
		return 0;

	level_two_or_higher = (trailing_one == 3) ? 0 : 1;
	suffix_len			= ((total_coeff > 10) && (trailing_one < 3)) ? 1 : 0;
	

	/*the other coefficient */
	for (k = total_coeff - 1-trailing_one; k >= 0; k--)
	{
		ReadLevVlcHigh (suffix_len, &level, &sign, &lev_err);

		if (lev_err)
			return 1;

		if (level_two_or_higher)//when the index i is equal to TrailingOnes and TranilingOnes is less than 3, levelcode is incremented by 2.
		{
			level++;
			level_two_or_higher = 0;
		}

		/*suffix length update*/
		suffix_len_inc = SuffixLenIncHigh (suffix_len, level);

		suffix_len = suffix_len_inc ? suffix_len + 1 : suffix_len;	

		if ((k == total_coeff - 1-trailing_one) && (level > 3))
		{
			suffix_len = 2;			
		}

		level = sign ? -level : level;

		/*write level to huffman table, from 176 position of huffman table*/
		lev_tbuf_addr = LEV_TBUF_BASE_ADDR + k;
		g_hvld_huff_tab[lev_tbuf_addr] = level;

		lev_num++;
	}

	return 0;
}

void ReadLevVlcHigh (
				 int	suffix_len, 
				 int *	lev_ptr, 
				 int *	sign_ptr,
				 int *	lev_err_ptr
				 )
{
	int		level_prefix;
	int		bsm_lev_data;
	int		lev_prefix_error = 0;
	uint32	next_16bits;
	int		lev_suffix_size;
	int		level_code;
	int		sign;
	int		level;
	int		level_suffix;
	int		lev_bsm_len;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;

	bsm_lev_data = (uint32)(BITSTREAMSHOWBITS (stream, 32));

	/*prefix length*/
	if ((bsm_lev_data & 0xfffff000) == 0)
	{
		lev_prefix_error = 1;
		next_16bits		 = 0;
	}
	else if (bsm_lev_data & 0xff000000)//注意这里level_prefix最大可以达到19，那么level_suffix就可以解码16个bit，那么一个word的长度是不够的。
	{
		if (bsm_lev_data & 0xf0000000)
		{
			if (bsm_lev_data & 0x80000000)
			{
				level_prefix = 0;
				next_16bits  = (bsm_lev_data >> 15) & 0xffff;
			}
			else if (bsm_lev_data & 0x40000000)
			{
				level_prefix = 1;
				next_16bits  = (bsm_lev_data >> 14) & 0xffff;
			}
			else if (bsm_lev_data & 0x20000000)
			{
				level_prefix = 2;
				next_16bits  = (bsm_lev_data >> 13) & 0xffff;
			}
			else
			{
				level_prefix = 3;
				next_16bits  = (bsm_lev_data >> 12) & 0xffff;
			}
		}
		else
		{
			if (bsm_lev_data & 0x08000000)
			{
				level_prefix = 4;
				next_16bits  = (bsm_lev_data >> 11) & 0xffff;
			}
			else if (bsm_lev_data & 0x04000000)
			{
				level_prefix = 5;
				next_16bits  = (bsm_lev_data >> 10) & 0xffff;
			}
			else if (bsm_lev_data & 0x02000000)
			{
				level_prefix = 6;
				next_16bits  = (bsm_lev_data >> 9) & 0xffff;
			}
			else
			{
				level_prefix = 7;
				next_16bits  = (bsm_lev_data >> 8) & 0xffff;
			}
		}
	}
	else if (bsm_lev_data & 0x00ff0000)
	{
		if (bsm_lev_data & 0x00f00000)
		{
			if (bsm_lev_data & 0x00800000)
			{
				level_prefix = 8;
				next_16bits  = (bsm_lev_data >> 7) & 0xffff;
			}
			else if (bsm_lev_data & 0x00400000)
			{
				level_prefix = 9;
				next_16bits  = (bsm_lev_data >> 6) & 0xffff;
			}
			else if (bsm_lev_data & 0x00200000)
			{
				level_prefix = 10;
				next_16bits  = (bsm_lev_data >> 5) & 0xffff;
			}
			else
			{
				level_prefix = 11;
				next_16bits  = (bsm_lev_data >> 4) & 0xffff;
			}
		}
		else
		{
			if (bsm_lev_data & 0x00080000)
			{
				level_prefix = 12;
				next_16bits  = (bsm_lev_data >> 3) & 0xffff;
			}
			else if (bsm_lev_data & 0x00040000)
			{
				level_prefix = 13;
				next_16bits  = (bsm_lev_data >> 2) & 0xffff;
			}
			else if (bsm_lev_data & 0x00020000)
			{
				level_prefix = 14;
				next_16bits  = (bsm_lev_data >> 1) & 0xffff;//这里都需要考虑，因为可能bit数目不够了，我们应该支持的都是8bit的视频，那么level prefix可以达到19，那么就有二十个bit
			}
			else
			{
				level_prefix = 15;
				next_16bits  = (bsm_lev_data >> 0) & 0xffff;
			}
		}
	}
	else//for high profile
	{
		if (bsm_lev_data & 0x00008000)
		{
			level_prefix = 16;
			BITSTREAMFLUSHBITS (stream, 17);
		}
		else if (bsm_lev_data & 0x00004000)
		{
			level_prefix = 17;
			BITSTREAMFLUSHBITS (stream, 18);
		}
		else if (bsm_lev_data & 0x00002000)
		{
			level_prefix = 18;
			BITSTREAMFLUSHBITS (stream, 19);
		}
		else
		{
			level_prefix = 19;
			BITSTREAMFLUSHBITS (stream, 20);
		}

		next_16bits = (uint32)(BITSTREAMSHOWBITS (stream, 16));
	}

	if ((level_prefix == 14) && (suffix_len == 0))
	{
		lev_suffix_size = 4;
	}
	else if (level_prefix >= 15)//这里需要考虑level prefix大于15的情况，因为现在支持high profile了，以前只支持main profile
	{
		lev_suffix_size = level_prefix - 3;
	}
	else
	{
		lev_suffix_size = suffix_len;
	}

	level_suffix = next_16bits >> (16 - lev_suffix_size);

	level_code  = ((min(15,level_prefix)) << suffix_len) + level_suffix; 

	if ((level_prefix >= 15) && (suffix_len == 0))//需要考虑level prefix大于等于16的情况。
	{
		level_code += 15;
	}

	if ((level_prefix >= 16) )//&& (suffix_len == 0))//需要考虑level prefix大于等于16的情况。
	{
		level_code += (1<<(level_prefix-3))-4096;
	}

	sign  = level_code & 1;
	level = (level_code + 2) >> 1;

	if (level_prefix <= 15)
	{
		lev_bsm_len = lev_suffix_size + level_prefix + 1;
		BITSTREAMFLUSHBITS (stream, lev_bsm_len);
	}
	else
	{
		BITSTREAMFLUSHBITS (stream, lev_suffix_size);
	}

	*lev_ptr		= level;
	*sign_ptr		= sign;
	*lev_err_ptr	= lev_prefix_error;

}

int SuffixLenIncHigh (int suffix_len, int level)
{
	int		suffix_len_inc;

	if (suffix_len == 0)
	{
		suffix_len_inc = 1;
	}
	else if (suffix_len == 1)
	{
		suffix_len_inc = (level > 3) ? 1 : 0;
	}
	else if (suffix_len == 2)
	{
		suffix_len_inc = (level > 6) ? 1 : 0;
	}
	else if (suffix_len == 3)
	{
		suffix_len_inc = (level > 12) ? 1 : 0;
	}
	else if (suffix_len == 4)
	{
		suffix_len_inc = (level > 24) ? 1 : 0;
	}
	else if (suffix_len == 5)
	{
		suffix_len_inc = (level > 48) ? 1 : 0;
	}
	else
	{
		suffix_len_inc = 0;
	}

	return suffix_len_inc;	
}

int H264VldRunDecHigh (
				   int	total_coeff,
				   int	blk_type,
				   int	max_coeff_num
				   )
{
	int		total_zeros = 0;	
	int		run_befor;
	int		error = 0;
	int		run_num = 0;

	if (total_coeff < max_coeff_num)
	{
		error = GetTotalZerosHigh (total_coeff, blk_type, &total_zeros);
	}

	if (error)
		return 1;

	/*run before each coefficient are stored in register in hvld_blk module*/
	zeros_left_reg	= total_zeros;
	index_reg		= total_coeff - 1;
	
	while ((zeros_left_reg != 0) && (index_reg != 0))
	{
		error = ReadRunBeforeHigh (zeros_left_reg, &run_befor);

		run_num++;		

		if (error)
			return 1;
		
		zeros_left_reg = zeros_left_reg - run_befor;
		
		run_reg[index_reg] = run_befor;
		
		index_reg--;

	}
	
	if (zeros_left_reg != 0)
	{
		PrintfRunBefore (0, 0, zeros_left_reg, 0);

		run_reg[0] = zeros_left_reg;
	}

	return 0;
}

int GetTotalZerosHigh (
				   int		total_coeff, 
				   int		blk_type, 
				   int *	total_zeros_ptr
				   )
{	
	uint32	bsm_run_data;
	int		total_zeros;
	int		code_len;
	int		total_zeros_err = 0;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;

	bsm_run_data = (uint32)(BITSTREAMSHOWBITS (stream, 32));
	
	/*total zero*/
	if (blk_type == CHROMA_DC)
	{
		if (total_coeff == 1)
		{
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else if ((bsm_run_data >> 30) & 1)
			{
				total_zeros = 1;
				code_len = 2;
			}
			else if ((bsm_run_data >> 29) & 1)
			{
				total_zeros = 2;
				code_len = 3;
			}
			else
			{
				total_zeros = 3;
				code_len = 3;
			}
		}
		else if (total_coeff == 2)
		{			
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else if ((bsm_run_data >> 30) & 1)
			{
				total_zeros = 1;
				code_len = 2;
			}
			else
			{
				total_zeros = 2;
				code_len = 2;
			}
		}
		else
		{			
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else
			{
				total_zeros = 1;
				code_len = 1;
			}
		}		
	}
	else
	{
		/*for block not be chroma_dc*/
		 if (total_coeff < 8)
		 {
			 if (total_coeff < 4)
			 {
				 if (total_coeff < 2)    //total_coeff: 1
				 {
					 if (bsm_run_data & 0xf0000000)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 0;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 2;
							 code_len = 3;							 
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 3 : 4;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 5 : 6;
							 code_len = 5;
						 }
					 }
					 else
					 {
						 if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 7 : 8;
							 code_len = 6;							 
						 }
						 else if (bsm_run_data & 0x04000000)
						 {
							 total_zeros = (bsm_run_data & 0x02000000) ? 9 : 10;
							 code_len = 7;
						 }
						 else if (bsm_run_data & 0x02000000)
						 {
							 total_zeros = (bsm_run_data & 0x01000000) ? 11 : 12;
							 code_len = 8;
						 }
						 else if (bsm_run_data & 0x01000000)
						 {
							 total_zeros = (bsm_run_data & 0x00800000) ? 13 : 14;
							 code_len = 9;
						 }
						 else if (bsm_run_data & 0x00800000)
						 {
							 total_zeros = 15;
							 code_len = 9;
						 }
						 else
						 {
							 total_zeros = 0;
							 code_len = 0;
							 total_zeros_err = 1;
						 }						 
					 }
				 }
				 else
				 {
					 if (total_coeff == 2)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;

							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 0 : 1;							
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 4;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 5 : 6;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 7 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 10;	
							 code_len = 5;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 11 : 12;	
							 code_len = 6;
						 }
						 else
						 {							 
							 total_zeros = (bsm_run_data & 0x04000000) ? 13 : 14;	
							 code_len = 6;
						 }
					 }
					 else		//total_coeff: 3
					 {
						 if (bsm_run_data & 0x80000000)
						 {							 
							 code_len = 3;
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 2;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 7;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 0 : 4;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 5 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 10;	
							 code_len = 5;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 12;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 11 : 13;
							 code_len = 6;							 
						 }
					 }
				 }
			 }
			 else
			 {
				 if (total_coeff < 6)
				 {
					 if (total_coeff == 4)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;

							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 4;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 8;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 2 : 3;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 7 : 9;	
							 code_len = 4;							 
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 0 : 10;	
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 11 : 12;	
							 code_len = 5;
						 }						 
					 }
					 else    //total_coeff: 5
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 4;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 7;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 0 : 1;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 2 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 10;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 11;	
							 code_len = 5;							 
						 }
					 }
				 }
				 else
				 {
					 if (total_coeff == 6)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 4 : 5;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 6 : 7;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 9;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 8;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 1;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 10;	
							 code_len = 6;
						 }
					 }
					 else   //total_coeff: 7
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = 5;	
								 code_len = 2;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
								 code_len = 3;
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 4 : 6;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 8;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 7;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 1;	
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 9;	
							 code_len = 6;							 
						 }	
					 }
				 }
			 }
		 }
		 else
		 {
			 if (total_coeff < 12)
			 {
				 if (total_coeff < 10)
				 {
					 if (total_coeff == 8)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 4 : 5;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 6;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 7;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 1;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 2;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 8;	
							 code_len = 6;
						 }						 
					 }
					 else   //total coeff: 9
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 3 : 4;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 6;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 5;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 2;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 7;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 1;	
							 code_len = 6;
						 }
					 }
				 }
				 else
				 {
					 if (total_coeff == 10)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 3 : 4;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 5;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 2;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 6;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 0 : 1;	
							 code_len = 5;
						 }
					 }
					 else  //total_coeff: 11
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 4;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 3;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 2;
							 code_len = 3;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 1 : 0;
							 code_len = 4;
						 }						 
					 }
				 }
			 }
			 else
			 {
				 if (total_coeff < 14)
				 {
					 if (total_coeff == 12)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 3;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 2;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 4;
							 code_len = 3;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 1 : 0;
							 code_len = 4;
						 }
					 }
					 else   //total_coeff: 13
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 2;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 3;
							 code_len = 2;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 0;
							 code_len = 3;
						 }						 
					 }
				 }
				 else
				 {
					 if (total_coeff == 14)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 2;
							 code_len = 1;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 1 : 0;
							 code_len = 2;
						 }
					 }
					 else   //total_coeff: 15
					 {
						 total_zeros = (bsm_run_data & 0x80000000) ? 1 : 0;
						 code_len = 1;
					 }
				 }
			 }
		 }
	}

	BITSTREAMFLUSHBITS (stream, code_len);

	PrintfTotalZero (blk_type, bsm_run_data, total_coeff, total_zeros, code_len);

	*total_zeros_ptr = total_zeros;

	return total_zeros_err;
}

int ReadRunBeforeHigh (int zeros_left, int * run_before_ptr)
{
	int     code_len;
	int		run_before;
	int		error = 0;
	uint32  bsm_run_data;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;

	bsm_run_data = (uint32)(BITSTREAMSHOWBITS (stream, 32));

	if (zeros_left < 5)
	{
		if (zeros_left == 1)
		{
			code_len = 1;
			run_before = (bsm_run_data & 0x80000000) ? 0 : 1;
		}
		else if (zeros_left == 2)
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 1;
				run_before = 0;
			}
			else
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 1 : 2;
			}
		}
		else if (zeros_left == 3)
		{
			code_len = 2;

			if (bsm_run_data & 0x80000000)
			{
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else
			{
				run_before = (bsm_run_data & 0x40000000) ? 2 : 3;	
			}
		}
		else	//zero_lefe 4
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 2;
				run_before = 2;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 3 : 4;
			}			
		}
	}
	else
	{
		if (zeros_left == 5)
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 2 : 3;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 4 : 5;
			}
		}
		else if (zeros_left == 6)
		{
			if (bsm_run_data & 0x80000000)
			{
				if (bsm_run_data & 0x40000000)
				{
					code_len = 2;
					run_before = 0;
				}
				else
				{
					code_len = 3;
					run_before = (bsm_run_data & 0x20000000) ? 5 : 6;
				}
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 3 : 4;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 2 : 1;
			}
		}
		else
		{
			if (bsm_run_data & 0xf0000000)
			{
				if (bsm_run_data & 0x80000000)
				{
					if (bsm_run_data & 0x40000000)
					{
						code_len = 3;
						run_before = (bsm_run_data & 0x20000000) ? 0 : 1;
					}
					else
					{
						code_len = 3;
						run_before = (bsm_run_data & 0x20000000) ? 2 : 3;
					}
				}
				else if (bsm_run_data & 0x40000000)
				{
					code_len = 3;
					run_before = (bsm_run_data & 0x20000000) ? 4 : 5;
				}
				else if (bsm_run_data & 0x20000000)
				{
					code_len = 3;
					run_before = 6;
				}
				else
				{
					code_len = 4;
					run_before = 7;
				}
			}
			else
			{
				if (bsm_run_data & 0x08000000)
				{
					code_len = 5;
					run_before = 8;
				}
				else if (bsm_run_data & 0x04000000)
				{
					code_len = 6;
					run_before = 9;
				}
				else if (bsm_run_data & 0x02000000)
				{
					code_len = 7;
					run_before = 10;
				}
				else if (bsm_run_data & 0x01000000)
				{
					code_len = 8;
					run_before = 11;
				}
				else if (bsm_run_data & 0x00800000)
				{
					code_len = 9;
					run_before = 12;
				}
				else if (bsm_run_data & 0x00400000)
				{
					code_len = 10;
					run_before = 13;
				}
				else if (bsm_run_data & 0x00200000)
				{
					code_len = 11;
					run_before = 14;
				}
				else
				{
					code_len = 0;
					run_before = 0;
					error = 1;					
				}
			}
		}
	}

	BITSTREAMFLUSHBITS (stream, code_len);

	*run_before_ptr = run_before;

	PrintfRunBefore (bsm_run_data, zeros_left, run_before, code_len);

	return error;	
}

void TwoBinArithDecHigh (
					 int		syn_type,			//syntax type for first bin to be decoded
					 uint8	*	ctx_bin0_ptr,		//first bin context to be decoded
					 uint8	*	ctx_bin1_ptr,		//second bin context to be decoded
					 int	*	bin0_ptr,			//returned first binary value
					 int	*	bin1_ptr,			//returned second binary value
					 int		glm_sfx_num
					 )
{
	uint32		bsm_data;

	int			shift_bits_bin0 = 0;
	int			shift_bits_bin1 = 0;
	int			shift_bits;

	int			is_bp_mode;
	int			val_bin0 = 0;
	int			val_bin1 = 0;

	int			bin1_active;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	bsm_data = (uint32)BITSTREAMSHOWBITS (stream, 32); 


	is_bp_mode = (syn_type == CABAC_LEV_SIGN) || (syn_type == CABAC_GLM_PRX) || (syn_type == CABAC_GLM_SFX);

	/*instanciation two BiArithDec*/
	val_bin0 = BiArithDec (
							is_bp_mode,	
							&bsm_data,	
							&g_range,	 
							&g_offset,	
							ctx_bin0_ptr, 
							&shift_bits_bin0
							);

	/*********************************************************************************
	bsm_data, g_range, g_offset should use the forwarding value from instance0
	**********************************************************************************/
	//is_bp_mode = 1;//james
	if (syn_type == CABAC_CODED_FLAG)
	{
		bin1_active = 0;
	}
	else if (syn_type == CABAC_SIG_LAST)
	{
		bin1_active = val_bin0;
		is_bp_mode  = 0;
	}
	else if (syn_type == CABAC_LEV_BIN0)
	{
		//if val_bin0 is 0, decode sign, else decode other bin
		bin1_active = 1;
		is_bp_mode  = (val_bin0 == 0) ? 1 : 0;	
	}
	else if (syn_type == CABAC_LEV_BIN_OTH)
	{
		bin1_active = 1;
		*ctx_bin1_ptr = *ctx_bin0_ptr;			//forward status of first bin engine to second engine
		is_bp_mode  = (val_bin0 == 0) ? 1 : 0;
	}
	else if (syn_type == CABAC_GLM_PRX)
	{
		bin1_active = val_bin0;
		is_bp_mode  = 1;
	}
	else if (syn_type == CABAC_GLM_SFX)
	{
		bin1_active = (glm_sfx_num > 1) ? 1 : 0;
		is_bp_mode  = 1;
	}
	else if (syn_type == CABAC_LEV_SIGN)
	{
		bin1_active = 0;
		is_bp_mode  = 0;
	}
	
	if (bin1_active)
	{
		val_bin1 = BiArithDec (
								is_bp_mode,	
								&bsm_data,	
								&g_range,	 
								&g_offset,	
								ctx_bin1_ptr, 
								&shift_bits_bin1
							);	
	}

	shift_bits = shift_bits_bin0 + shift_bits_bin1;

	BITSTREAMFLUSHBITS (stream, shift_bits); 

	*bin0_ptr = val_bin0;
	*bin1_ptr = val_bin1;	
}

int ArithOneLevDecHigh (
					int blk_cat, 
					int num_t1, 
					int num_lgt1
					)//这个函数应该不需要改了，只需要将那个里头的ctx_bin方面的东西改了就好了。
{
	int		lev_done = 0;
	int		syn_type;
	int		val_bin0;
	int		val_bin1;

	int		glm_prx_num;
	int		glm_sfx_num;

	int		ctx_bin0_inc;
	int		ctx_binoth_inc;

	int		coeff;
	int		sign;
	int		level;
	int		bin_idx = 0;

	uint8	ctx_bin0;
	uint8	ctx_binoth;

	int		has_golomb = 0;

	
	int		symbol = 0;
	int		binary_symbol = 0;
				

	syn_type = CABAC_LEV_BIN0;
	
	/************************************************************************
	decode bin0-sign or bin0-bin1
	if level is 1, decode bin0 in engine0 and sign in engine1
	else, decode bin0 in engine0 and bin1 in engine1
	*************************************************************************/
	ctx_bin0_inc = (num_lgt1 > 0) ? 0 : mmin(4, 1+num_t1);
	ctx_binoth_inc = mmin(4, num_lgt1);

	ctx_bin0   = GetBin0Ctx (blk_cat, ctx_bin0_inc);
	ctx_binoth = GetBinothCtx (blk_cat, ctx_binoth_inc);

	TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

	
	if (val_bin0 == 0)
	{
		level = 1;
		sign  = val_bin1;
	}
	else
	{
		if (val_bin1 == 0)
		{
			syn_type = CABAC_LEV_SIGN;

			/*read sign, no ctx is updated*/
			TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

			level = 2;
			sign  = val_bin0;
		}
		else
		{
			level    = 2;
			bin_idx += 2;

			/*unary decodingg*/
			while (1)
			{
				/*decode bin1~bin13, */
				syn_type = CABAC_LEV_BIN_OTH;

				TwoBinArithDecHigh (syn_type, &ctx_binoth, &ctx_binoth, &val_bin0, &val_bin1, 0);

				if (val_bin0 == 0)
				{
					level += 1;
					sign = val_bin1;
					break;
				}
				else if (val_bin1 == 0)
				{
					/*read sign*/
					level += 2;
					syn_type = CABAC_LEV_SIGN;
					TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);					

					sign  = val_bin0;

					break;
				}
				
				bin_idx += 2;
				level += 2;

				if (bin_idx == 14)
				{
					has_golomb = 1;
					break;
				}
			}

			/*exp-golomb decoding*/
			if (has_golomb)
			{
				glm_prx_num = 0;

				/*golomb prefix decoding*/
				while (1)
				{
					syn_type = CABAC_GLM_PRX;
					TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);	

					if (val_bin0 == 0)
					{
						break;
					}
					else if (val_bin1 == 0)
					{
						symbol = symbol + (1 << glm_prx_num);

						glm_prx_num += 1;
						break;
					}
					else
					{
						symbol = symbol + (1 << glm_prx_num) + (1 << (glm_prx_num+1));
						glm_prx_num += 2;
					}
				}

				/*golomb surfix decoding*/
				glm_sfx_num = glm_prx_num;

				while (glm_sfx_num > 0)
				{					
					syn_type = CABAC_GLM_SFX;
					TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, glm_sfx_num);

					if (val_bin0 == 1)
					{
						binary_symbol = binary_symbol | (1 << (glm_sfx_num - 1));
					}

					if (glm_sfx_num > 1)
					{
						if (val_bin1 == 1)
						{
							binary_symbol = binary_symbol | (1 << (glm_sfx_num - 2));
						}
					}						

					/*level update*/

					if (glm_sfx_num < 2)
						break;

					glm_sfx_num -= 2;
				}
				
				level = level + symbol + binary_symbol + 1;

				syn_type = CABAC_LEV_SIGN;
				TwoBinArithDecHigh (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

				sign = val_bin0;
			}
		}
	}	

	coeff = (sign == 1) ? -level : level;
	
	UpdateBin0Ctx (blk_cat, ctx_bin0_inc, ctx_bin0);
	UpdateBinothCtx (blk_cat, ctx_binoth_inc, ctx_binoth);

	return coeff;		
}