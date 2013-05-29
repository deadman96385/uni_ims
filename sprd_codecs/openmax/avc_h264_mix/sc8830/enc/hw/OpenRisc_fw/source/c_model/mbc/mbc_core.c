/******************************************************************************
 ** File Name:    mbc_core.c		                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
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


int32 tbl_offset[4] = {26*4, 28*4, 74*4, 76*4};

void mp4_add_ref_and_residual (void)
{
	int32 i;
	int32 iBlk;
	int32 cbp;
	int32 bIntra;
	int32 dstWidth;
	uint8 * pRef;
	uint8 * pMbc;
	uint32 mbc_cmd0 = g_mbc_reg_ptr->MBC_CMD0;
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	uint8 * pMcaBfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 * pMeaBfr = (uint8 *)vsp_mea_out_bfr;
	uint8 * pRefBfr;
	
	int32 bZeroBlk;	
	int mbc_offset;
	
	int32 bEncoder = (g_glb_reg_ptr->VSP_CFG0 >> 14 ) & 0x01;

	bIntra = !((mbc_cmd0 >> 30) & 1);
	cbp = mbc_cmd0 & 0x3f;
	
	if(bEncoder)
	{
		pRefBfr = pMeaBfr;
	}else
	{
		pRefBfr = pMcaBfr;
	}
		
	for (iBlk = 0; iBlk < 6; iBlk++)
	{
		if (iBlk < 4)
		{
			dstWidth = MB_SIZE;
			mbc_offset = MBC_Y_SIZE;
			pRef = pRefBfr + (iBlk>>1)*128 + (iBlk&1)*8;
			pMbc = pMbcBfr + tbl_offset[iBlk];
		}
		else if(iBlk == 4) //u
		{
			dstWidth = BLOCK_SIZE;
			mbc_offset = MBC_C_SIZE;
			pRef = pRefBfr + MCA_U_OFFSET;
			pMbc = pMbcBfr + MBC_U_OFFSET+18*4;
		}else //v
		{
			pRef = pRefBfr + MCA_V_OFFSET;
			pMbc = pMbcBfr + MBC_V_OFFSET+18*4;
		}
		
		if ( cbp & (1<<(5-iBlk)) )
		{
			bZeroBlk = FALSE;
		}else
		{
			bZeroBlk = TRUE;
		}
		
		for (i = 0; i < 8; i++)
		{
			pMbc[0] = IClip(0,255, pRef[0] + (bZeroBlk ? 0 : pBlkCff[i*8+0]));
			pMbc[1] = IClip(0,255, pRef[1] + (bZeroBlk ? 0 : pBlkCff[i*8+1]));
			pMbc[2] = IClip(0,255, pRef[2] + (bZeroBlk ? 0 : pBlkCff[i*8+2]));
			pMbc[3] = IClip(0,255, pRef[3] + (bZeroBlk ? 0 : pBlkCff[i*8+3]));
			pMbc[4] = IClip(0,255, pRef[4] + (bZeroBlk ? 0 : pBlkCff[i*8+4]));
			pMbc[5] = IClip(0,255, pRef[5] + (bZeroBlk ? 0 : pBlkCff[i*8+5]));
			pMbc[6] = IClip(0,255, pRef[6] + (bZeroBlk ? 0 : pBlkCff[i*8+6]));
			pMbc[7] = IClip(0,255, pRef[7] + (bZeroBlk ? 0 : pBlkCff[i*8+7]));
			
			pRef += dstWidth;
			pMbc += mbc_offset;
		}
		
		pBlkCff += 64;		
	}	
}

void h264_add_ref_and_residual (void)
{
	int32 i, j, k;
	int32 blkIdxInMB;
	int32 cbp24;
	int32 bIntra;
	uint8 * pRef;
	uint8 * pMbc;
	int16 *p4x4blk;
	uint32 mbc_cmd0 = g_mbc_reg_ptr->MBC_CMD0;
	int16 * pBlkCff = (int16 *)vsp_dct_io_0;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
#if defined(H264_DEC)
	uint8 * pRefBfr = (uint8 *)vsp_fw_mca_out_bfr;
#else
	uint8 * pRefBfr = (uint8 *)vsp_mea_out_bfr;
#endif
	int8 is_skip = g_mb_cache_ptr->is_skip;
	int32 bZeroBlk;	
	int32 uv;
	int32 s_blk_offset_in_ref_bfr[24] = 
	{
		//luma
		0,								4,                   8,                  12,
		4*MCA_Y_SIZE+0,		4*MCA_Y_SIZE+4,		4*MCA_Y_SIZE+8,		4*MCA_Y_SIZE+12,
		8*MCA_Y_SIZE+0,		8*MCA_Y_SIZE+4,		8*MCA_Y_SIZE+8,		8*MCA_Y_SIZE+12,
		12*MCA_Y_SIZE+0,	12*MCA_Y_SIZE+4,	12*MCA_Y_SIZE+8,	12*MCA_Y_SIZE+12,

		//chroma u
		MCA_U_OFFSET+0,					MCA_U_OFFSET+4,
		MCA_U_OFFSET+4*MCA_C_SIZE+0,	MCA_U_OFFSET+4*MCA_C_SIZE+4,

		//chroma v
		MCA_V_OFFSET+0,					MCA_V_OFFSET+4,
		MCA_V_OFFSET+4*MCA_C_SIZE+0,	MCA_V_OFFSET+4*MCA_C_SIZE+4,
	};

	bIntra = !((mbc_cmd0 >> 30) & 1);
	cbp24 = mbc_cmd0 & 0xffffff;

#if defined(H264_DEC)
	if (bIntra)
	{
		printf("error");
	}
#endif

	//luma
	//blkIdxInMB in follwing order
	//0,   1,   2,   3,
	//4,   5,   6,   7,
	//8,   9,  10,  11,
	//12, 13,  14,  15
	for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
	{
		i = blkIdxInMB>>2;
		j = blkIdxInMB&0x3;

		pMbc = pMbcBfr + 26*4 + i * 4 * MBC_Y_SIZE + j * 4;
		p4x4blk = pBlkCff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
		pRef = pRefBfr + s_blk_offset_in_ref_bfr[blkIdxInMB];

		bZeroBlk = !((g_mbc_reg_ptr->MBC_CMD0 >> (blkIdxInMB)) & 0x01);

		//add residual coef
		for (k = 0; k < 4; k++)
		{
			pMbc[0] = IClip(0,255, pRef[0] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+0]));
			pMbc[1] = IClip(0,255, pRef[1] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+1]));
			pMbc[2] = IClip(0,255, pRef[2] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+2]));
			pMbc[3] = IClip(0,255, pRef[3] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+3]));

			pMbc += MBC_Y_SIZE;
			pRef += MCA_Y_SIZE;
		}		
	}

	//chroma
	for (blkIdxInMB = 16; blkIdxInMB < 24; blkIdxInMB++)
	{
		if (blkIdxInMB < 20)
		{
			//16, 17
			//18, 19
			uv = 0;

			i = (blkIdxInMB - 16)/2;
			j = (blkIdxInMB - 16)%2;

			pMbc = pMbcBfr + MBC_U_OFFSET+18*4 + i*4*MBC_C_SIZE+j*4;
		}else
		{
			//20, 21,
			//22, 23
			
			uv = 1;

			i = (blkIdxInMB - 20)/2;
			j = (blkIdxInMB - 20)%2;

			pMbc = pMbcBfr + MBC_V_OFFSET+18*4 + i*4*MBC_C_SIZE+j*4;
		}

		p4x4blk = pBlkCff + blkIdxInMB * 16;
		pRef = pRefBfr + s_blk_offset_in_ref_bfr[blkIdxInMB];

		bZeroBlk = !((g_mbc_reg_ptr->MBC_CMD0 >> blkIdxInMB) & 0x01);

		//add residual coef
		for (k = 0; k < 4; k++)
		{
			pMbc[0] = IClip(0,255, pRef[0] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+0]));
			pMbc[1] = IClip(0,255, pRef[1] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+1]));
			pMbc[2] = IClip(0,255, pRef[2] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+2]));
			pMbc[3] = IClip(0,255, pRef[3] + ((bZeroBlk || is_skip) ? 0 : p4x4blk[k*4+3]));

			pMbc += MBC_C_SIZE;
			pRef += MCA_C_SIZE;
		}		
	}			

	return;
}

void DS_full_blk(uint8 *pSrc, uint8 *pDst, int32 width, int32 height)
{
	int32 i,j;
	uint8 *src = pSrc;
	uint8 *dst = pDst;
	int32 dst_width = width;
	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			dst[j] = src[j];
		}
		src += width;
		dst += dst_width;
	}

	return;
}

void DS_half_blk(uint8 *pSrc, uint8 *pDst, int32 width, int32 height)
{
	int32 i, j, k;
	uint8 *src = pSrc;
	int32 dst_width = width >> 1;
	uint8 *dst = (uint8*)malloc(dst_width * height);
	uint8 *p_dst = dst;
	for(i = 0; i < height; i++)
	{
		for(k = 0, j = 0; j < width; j+=2)
		{
			p_dst[k++] = (src[j] + src[j+1] + 1) >> 1;
		}		
		src += width;
		p_dst += dst_width;
	}
	
	src = dst;
	for(i = 0; i < height; i+=2)
	{
		for(j = 0; j < dst_width; j++)
		{
			pDst[j] = (src[(i*dst_width) + j] + src[((i+1)*dst_width) + j] ) >> 1;
		}
		pDst += dst_width;
	}

	free(dst); dst = PNULL;

	return;
}

void DS_quarter_blk(uint8 *pSrc, uint8 *pDst, int32 width, int32 height)
{
	int32 i, j, k;
	uint8 *src = pSrc;
	int32 dst_width = width >> 2;
	uint8 *dst = (uint8*)malloc(dst_width * height);
	uint8 *p_dst = dst;
	for(i = 0; i < height; i++)
	{
		for(k = 0, j = 0; j < width; j+=4)
		{
			p_dst[k++] = (src[j] + src[j+1] + src[j+2] + src[j+3] + 2) >> 2;
		}		
		src += width;
		p_dst += dst_width;
	}
	
	src = dst;
	for(i = 0; i < height; i+=4)
	{
		for(j = 0; j < dst_width; j++)
		{
			pDst[j] = (src[(i*dst_width) + j] + src[((i+1)*dst_width) + j] + src[((i+2)*dst_width) + j] + src[((i+3)*dst_width) + j]) >> 2;
		}	
		pDst += dst_width;
	}

	free(dst); dst = PNULL;
}

void Init_downsample_fun()
{
	g_downsample[0] = DS_full_blk;
	g_downsample[1] = DS_half_blk;
	g_downsample[2] = DS_quarter_blk;
}

//////////////////////////////////////////////////////////////////////////
/////						FORMAT CONVERTION						//////
//////////////////////////////////////////////////////////////////////////
void format_convertion_444to422()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i;
/*
	MBC BUFFER FORMAT
	Y: 8X8, OFFSET: 0
	U: 8X4, OFFSET: 64   
	V: 8X4, OFFSET: 64+32 = 96
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}
		
	//u, copy, clip and format convertion, 8x8->8x4
	for(i = 0; i < 32; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i*2]+128));
	}

	//v, same with u
	pBlkCff += 64;
	pMbcBfr += 32;
	for(i = 0; i < 32; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i*2]+128));
	}
}

void format_convertion_411to422()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
	U: 8X16, OFFSET: 64*4 = 256  
	V: 8X16, OFFSET: 64*4+128 = 384
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}

	//u, copy, clip and format convertion, 8x8->8x16
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[2*i]	= (uint8)(IClip(0, 255, pBlkCff[i]+128));
		pMbcBfr[2*i+1]	= pMbcBfr[2*i];
	}

	//v, same with u
	pBlkCff += 64;
	pMbcBfr += 128;
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[2*i]	= (uint8)(IClip(0, 255, pBlkCff[i]+128));
		pMbcBfr[2*i+1]	= pMbcBfr[2*i];
	}
}

void format_convertion_411Rto420()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i,j;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
	U: 16X4, OFFSET: 64*4 = 256  
	V: 16X4, OFFSET: 64*4+64 = 320
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}

	//u, copy, clip and format convertion, 8x8->16x4
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 4; j++)
		{
			pMbcBfr[2*i*4+j]	 = (uint8)(IClip(0, 255, pBlkCff[i*8+j*2]+128));
			pMbcBfr[(2*i+1)*4+j] = pMbcBfr[(2*i)*4+j];
		}		
	}

	//v, same with u
	pBlkCff += 64;
	pMbcBfr += 64;
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 4; j++)
		{
			pMbcBfr[2*i*4+j]	 = (uint8)(IClip(0, 255, pBlkCff[i*8+j*2]+128));
			pMbcBfr[(2*i+1)*4+j] = pMbcBfr[(2*i)*4+j];
		}		
	}
}

void format_convertion_422Rto420()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i,j;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
	U: 8X4, OFFSET: 64*2 = 128  
	V: 8X4, OFFSET: 64*2+32 = 160
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}

	//u, copy, clip and format convertion, 8x8->8x4
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 4; j++)
		{
			pMbcBfr[i*4+j] = (uint8)(IClip(0, 255, pBlkCff[i*8+j*2]+128));
		}		
	}

	//v, same with u
	pBlkCff += 64;
	pMbcBfr += 32;
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 4; j++)
		{
			pMbcBfr[i*4+j] = (uint8)(IClip(0, 255, pBlkCff[i*8+j*2]+128));
		}		
	}
}

//only add 128 and clip
void format_convertion_422to422()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
	U: 8X4, OFFSET: 64*2 = 128  
	V: 8X4, OFFSET: 64*2+64 = 192
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}

	//u, copy and clip. 
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
	}
	pBlkCff += 64;
	pMbcBfr += 64;
	
	//v, same with u
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));		
	}
}

//only add 128 and clip
void format_convertion_420to420()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
	U: 8X4, OFFSET: 64*4 = 256  
	V: 8X4, OFFSET: 64*4+64 = 320
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}

	//u, copy and clip. 
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
	}
	pBlkCff += 64;
	pMbcBfr += 64;
	
	//v, same with u
	for(i = 0; i < 64; i++)
	{
		pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));		
	}
}

//only add 128 and clip
void format_convertion_400to400()
{
	int16 * pBlkCff = (int16 *)vsp_dct_io_1;
	uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
	int32 y_blk_idx = 0;
	int32 i;
/*
	MBC BUFFER FORMAT
	Y: 8X8,  OFFSET: 0
*/

	//y, copy and clip data from dct_io_buffer to mbc_out_buffer
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		for(i = 0; i < 64; i++)
		{
			pMbcBfr[i] = (uint8)(IClip(0, 255, pBlkCff[i]+128));
		}

		pBlkCff += 64;
		pMbcBfr += 64;
	}
}
//////////////////////////////////////////////////////////////////////////
/////						DOWN SAMPLE								//////
//////////////////////////////////////////////////////////////////////////
void printf_4Pix(int32 dst_width, uint8 *dst_ptr)
{
	if (dst_width >= 4)
	{
		FomatPrint4Pix(dst_ptr, g_fp_mbc_tv);
		//FomatPrint4Pix(dst_ptr, g_fp_dbk_tv);//jzy
	}else //<4
	{
		uint8 tmp[4];
		tmp[0] = dst_ptr[0]; tmp[1] = dst_ptr[1]; tmp[2] = tmp[3] = 0;

		FomatPrint4Pix(tmp, g_fp_mbc_tv);
		//FomatPrint4Pix(tmp, g_fp_dbk_tv);//jzy
	}
}

//HxW

//y: 8x8, u/v: 8x4
void downsample_org444(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	if (scale_down_factor > 1)
	{
		printf("scaler factor should not larger than 1 for 411R format.\n");
		exit(-1);
	}

	//y
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 4, 8);
	src_data += 32;
	dst_data += 32;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 4, 8);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//u
	dst_data += 64;
	dst_width = (4>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += 32;
	dst_width = (4>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

//y: 8x32, u/v: 8x16
void downsample_org411(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 16, 8);
	src_data += 128;
	dst_data += 128;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 16, 8);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		if (scale_down_factor < 2)
		{
			dst_ptr = dst_data+i*dst_width;//blk0
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64+i*dst_width;//blk1
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64*2+i*dst_width;//blk2
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64*3+i*dst_width;//blk3
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
		}else //scale_down_factor = 2
		{
			uint8 tmp[4];
			
			dst_ptr = dst_data+i*dst_width;//blk0
			tmp[0] = dst_ptr[0];	tmp[1] = dst_ptr[1];
			dst_ptr = dst_data+64+i*dst_width;//blk1
			tmp[2] = dst_ptr[0];	tmp[3] = dst_ptr[1];
			printf_4Pix(4, tmp);

			dst_ptr = dst_data+64*2+i*dst_width;//blk2
			tmp[0] = dst_ptr[0];	tmp[1] = dst_ptr[1];
			dst_ptr = dst_data+64*3+i*dst_width;//blk3
			tmp[2] = dst_ptr[0];	tmp[3] = dst_ptr[1];
			printf_4Pix(4, tmp);
		}		
	}
	//u
	dst_data += 64*4;
	dst_width = (16>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += (8*16);
	dst_width = (16>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

//y: 8x8, u/v: 16x4
void downsample_org411R(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	if (scale_down_factor > 1)
	{
		printf("scaler factor should not larger than 1 for 411R format.\n");
		exit(-1);
	}

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 4, 16);
	src_data += 64;
	dst_data += 64;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 4, 16);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;//blk0
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+64+i*dst_width;//blk1
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+64*2+i*dst_width;//blk2
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+64*3+i*dst_width;//blk3
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//u
	dst_data += 64*4;
	dst_width = (4>>scale_down_factor);
	dst_height = (16>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += (4*16);
	dst_width = (4>>scale_down_factor);
	dst_height = (16>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

//y: 16x8, u/v: 8x4
void downsample_org422R(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	if (scale_down_factor > 1)
	{
		printf("scaler factor should not larger than 1 for 422R format.\n");
		exit(-1);
	}

	//y
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 4, 8);
	src_data += 32;
	dst_data += 32;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 4, 8);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;//blk0
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+64+i*dst_width;//blk1
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//u
	dst_data += 64*2;
	dst_width = (4>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += (4*8);
	dst_width = (4>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

//y: 8x16, u/v: 8x8
void downsample_org422(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	//y
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
	src_data += 64;
	dst_data += 64;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 8, 8);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		if (scale_down_factor < 2)
		{
			dst_ptr = dst_data+i*dst_width;//blk0
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64+i*dst_width;//blk1
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
		}else //scale_down_factor=2
		{
			uint8 tmp[4];
			dst_ptr = dst_data+i*dst_width;//blk0
			tmp[0] = dst_ptr[0];
			tmp[1] = dst_ptr[1];
			dst_ptr = dst_data+64+i*dst_width;//blk1
			tmp[2] = dst_ptr[0];
			tmp[3] = dst_ptr[1];
			printf_4Pix(4, tmp);
		}		
	}
	//u
	dst_data += 64*2;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += (8*8);
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

//y: 16x16, u/v: 8x8
void downsample_org420(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}
	
	//u
	g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
	src_data += 64;
	dst_data += 64;
	
	//v
	g_downsample[scale_down_factor](src_data, dst_data, 8, 8);

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		if (scale_down_factor < 2)
		{
			dst_ptr = dst_data+i*dst_width;//blk0
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64+i*dst_width;//blk1
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
		}else //scale_down_factor=2
		{
			uint8 tmp[4];
			dst_ptr = dst_data+i*dst_width;//blk0
			tmp[0] = dst_ptr[0];
			tmp[1] = dst_ptr[1];
			dst_ptr = dst_data+64+i*dst_width;//blk1
			tmp[2] = dst_ptr[0];
			tmp[3] = dst_ptr[1];
			printf_4Pix(4, tmp);
		}		
	}
	for (i = 0; i < dst_height; i++)
	{
		if (scale_down_factor < 2)
		{
			dst_ptr = dst_data+64*2+i*dst_width;//blk2
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
			dst_ptr = dst_data+64*3+i*dst_width;//blk3
			for (j = 0; j < dst_width; j+=4) 
			{
				printf_4Pix(dst_width, dst_ptr+j);
			}
		}else //scale_down_factor=2
		{
			uint8 tmp[4];
			dst_ptr = dst_data+64*2+i*dst_width;//blk2
			tmp[0] = dst_ptr[0];
			tmp[1] = dst_ptr[1];
			dst_ptr = dst_data+64*3+i*dst_width;//blk3
			tmp[2] = dst_ptr[0];
			tmp[3] = dst_ptr[1];
			printf_4Pix(4, tmp);
		}		
	}

	//u
	dst_data += 64*4;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
	//v
	dst_data += (8*8);
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;
		for (j = 0; j < dst_width; j+=4)
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

void downsample_org400(int32 scale_down_factor)
{
	uint8 *src_data = (uint8 *)vsp_mbc_out_bfr;
	uint8 *dst_data = (uint8 *)vsp_dbk_out_bfr;
	int32 y_blk_idx = 0;

	if (scale_down_factor > 1)
	{
		printf("scaler factor should not larger than 1 for 400 format.\n");
		exit(-1);
	}

	//y
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		g_downsample[scale_down_factor](src_data, dst_data, 8, 8);
		src_data += 64;
		dst_data += 64;
	}

	//printf data
{
	uint8 *dst_ptr;
	uint8 dst_width, dst_height;
	int32 i, j;

	//y	
	dst_data = (uint8 *)vsp_dbk_out_bfr;
	dst_width = (8>>scale_down_factor);
	dst_height = (8>>scale_down_factor);
	for (i = 0; i < dst_height; i++)
	{
		dst_ptr = dst_data+i*dst_width;//blk0
		for (j = 0; j < dst_width; j+=4) 
		{
			printf_4Pix(dst_width, dst_ptr+j);
		}
	}
}
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
