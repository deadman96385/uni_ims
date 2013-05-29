/******************************************************************************
 ** File Name:    mea_fetch_data.c                                            *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//for video
void fetch_src_mb_to_dctiobfr(int32 mb_x, int32 mb_y)
{
	uint8 * src_mb_y;
	uint8 * src_mb_u;
	uint8 * src_mb_v;

	int16 * dct_io_y;
	int16 * dct_io_u;
	int16 * dct_io_v;

	int32 frm_width_y;
	int32 frm_width_c;

	int32 i, j;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);

	frm_width_y = g_mea_frame_width << 2; //byte
	frm_width_c = frm_width_y >> 1;

	src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*MB_SIZE*frm_width_y+mb_x*MB_SIZE;
	if (!uv_interleaved) //three plane
	{
		src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
		src_mb_v = (uint8*)g_mea_src_frame_v + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
	}else //two plane
	{
		frm_width_c = frm_width_y;
		src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE*2;
		src_mb_v = src_mb_u+1;
	}

	dct_io_y = (int16*)vsp_dct_io_0;
	dct_io_u = (int16*)(dct_io_y + MB_SIZE*MB_SIZE);
	dct_io_v = (int16*)(dct_io_u + BLOCK_SIZE*BLOCK_SIZE);

	//y0
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			dct_io_y[j] = (int16)(src_mb_y[j]);
		}
			
		dct_io_y += 8;
		src_mb_y += frm_width_y;
	}

	//y1
	src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*MB_SIZE*frm_width_y+mb_x*MB_SIZE + 8;
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			dct_io_y[j] = (int16)(src_mb_y[j]);
		}
			
		dct_io_y += 8;
		src_mb_y += frm_width_y;
	}

	//y2
	src_mb_y = (uint8*)g_mea_src_frame_y + (mb_y*MB_SIZE+8)*frm_width_y+mb_x*MB_SIZE;
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			dct_io_y[j] = (int16)(src_mb_y[j]);
		}
		
		dct_io_y += 8;
		src_mb_y += frm_width_y;
	}

	//y3
	src_mb_y = (uint8*)g_mea_src_frame_y + (mb_y*MB_SIZE+8)*frm_width_y+mb_x*MB_SIZE+8;
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			dct_io_y[j] = (int16)(src_mb_y[j]);
		}
			
		dct_io_y += 8;
		src_mb_y += frm_width_y;
	}

	//u
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dct_io_u[j] = (int16)(src_mb_u[j]);
			}else //two plane
			{
				dct_io_u[j] = (int16)(src_mb_u[2*j]);
			}
		}

		dct_io_u += 8;
		src_mb_u += frm_width_c;
	}

	//v
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dct_io_v[j] = (int16)(src_mb_v[j]);
			}else //two plane
			{
				dct_io_v[j] = (int16)(src_mb_v[2*j]);
			}
		}
			
		dct_io_v += 8;
		src_mb_v += frm_width_c;
	}		
}

void fetch_src_mb_to_dctiobfr_jpeg(int32 mb_x, int32 mb_y)
{
	uint8 * src_mb_y;
	uint8 * src_mb_u;
	uint8 * src_mb_v;

	int8 * dct_io_y;
	int8 * dct_io_u;
	int8 * dct_io_v;

	int32 frm_width_y;
	int32 frm_width_c;

	int32 i, j;
	uint8 clip = 0x80;

	int32 standard;
	int32 data_format;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);

	standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	data_format = (g_glb_reg_ptr->VSP_CFG1>>24)&0x7;

	frm_width_y = g_mea_frame_width << 2; //byte
	frm_width_c = frm_width_y >> 1;

	if(VSP_JPEG == standard)
	{
		clip = 0x80;
	}else
	{
		clip = 0;
	}
	
	if(JPEG_FW_YUV420 == data_format) 
	{
		src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*MB_SIZE*frm_width_y+mb_x*MB_SIZE;
		if (!uv_interleaved) //three plane
		{
			src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
			src_mb_v = (uint8*)g_mea_src_frame_v + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
		}else //two plane
		{
			frm_width_c = frm_width_y;
			src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE*2;
			src_mb_v = src_mb_u+1;
		}
		
		dct_io_y = (int8*)vsp_dct_io_0;
		dct_io_u = (int8*)(dct_io_y + MB_SIZE*MB_SIZE);
		dct_io_v = (int8*)(dct_io_u + BLOCK_SIZE*BLOCK_SIZE);

		//y0
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//y1
		src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*MB_SIZE*frm_width_y+mb_x*MB_SIZE + 8;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//y2
		src_mb_y = (uint8*)g_mea_src_frame_y + (mb_y*MB_SIZE+8)*frm_width_y+mb_x*MB_SIZE;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//y3
		src_mb_y = (uint8*)g_mea_src_frame_y + (mb_y*MB_SIZE+8)*frm_width_y+mb_x*MB_SIZE+8;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//u
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				if (!uv_interleaved) //three plane
				{
					dct_io_u[j] = (int8)(src_mb_u[j] - clip);
				}else //two plane
				{
					dct_io_u[j] = (int8)(src_mb_u[2*j] - clip);
				}
			}

			dct_io_u += 8;
			src_mb_u += frm_width_c;
		}

		//v
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				if (!uv_interleaved) //three plane
				{
					dct_io_v[j] = (int8)(src_mb_v[j] - clip);
				}else //two plane
				{
					dct_io_v[j] = (int8)(src_mb_v[2*j] - clip);
				}
			}
			
			dct_io_v += 8;
			src_mb_v += frm_width_c;
		}
	}else //422
	{
		src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*BLOCK_SIZE*frm_width_y+mb_x*MB_SIZE;
		if(!uv_interleaved) //three plane
		{
			src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
			src_mb_v = (uint8*)g_mea_src_frame_v + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE;
		}else
		{
			frm_width_c = frm_width_y;
			src_mb_u = (uint8*)g_mea_src_frame_u + mb_y*BLOCK_SIZE*frm_width_c+mb_x*BLOCK_SIZE*2;
			src_mb_v = src_mb_u+1;
		}
		
		dct_io_y = (int8*)vsp_dct_io_0;
		dct_io_u = (int8*)(dct_io_y + MB_SIZE*BLOCK_SIZE);
		dct_io_v = (int8*)(dct_io_u + BLOCK_SIZE*BLOCK_SIZE);

		//y0
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//y1
		src_mb_y = (uint8*)g_mea_src_frame_y + mb_y*BLOCK_SIZE*frm_width_y+mb_x*MB_SIZE + 8;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				dct_io_y[j] = (int8)(src_mb_y[j] - clip);
			}
			
			dct_io_y += 8;
			src_mb_y += frm_width_y;
		}

		//u
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				if (!uv_interleaved) //three plane
				{
					dct_io_u[j] = (int8)(src_mb_u[j] - clip);
				}else //two plane
				{
					dct_io_u[j] = (int8)(src_mb_u[j*2] - clip);
				}
			}

			dct_io_u += 8;
			src_mb_u += frm_width_c;
		}

		//v
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 8; j++)
			{
				if (!uv_interleaved) //three plane
				{
					dct_io_v[j] = (int8)(src_mb_v[j] - clip);
				}else //two plane
				{
					dct_io_v[j] = (int8)(src_mb_v[j*2] - clip);
				}
			}
			
			dct_io_v += 8;
			src_mb_v += frm_width_c;
		}	
	}		
}

void fetch_err_mb_to_dctiobfr(void)
{
	uint32 i;
	uint32 blk_num;

	uint8 * pSrc;
	uint8 * pRef;
	int16 * pDst;

	int16 * dct_io_bfr = (int16*)vsp_dct_io_0;

	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
		pSrc = g_SrcMB[blk_num];
		pRef = g_FinalRefMB[blk_num];
		pDst = dct_io_bfr + 64 * blk_num;

		for(i = 0; i < 64; i++)
		{
			pDst[i] = (int16)pSrc[i] - (int16)pRef[i]; 	
		}
	}
}

//to mea_out_buffer
void output_ref_mb(void)
{
	int32 iBlk;
	int32 dstWidth;
	int32 i, j;
	uint32 *mea_out_bfr = vsp_mea_out_bfr;
	uint32 *pMea;
	uint32 *pRef;

	for (iBlk = 0; iBlk < 6; iBlk++)
	{
		if (iBlk < 4)
		{
			dstWidth = MB_SIZE>>2;
			pMea = mea_out_bfr + ((iBlk>>1)*128 + (iBlk&1)*8)/4;	
		}
		else if(iBlk == 4) //u
		{
			dstWidth = BLOCK_SIZE>>2;
			pMea = mea_out_bfr + (MB_SIZE*MB_SIZE>>2);
		}else //v
		{
			pMea = mea_out_bfr + (MB_SIZE*MB_SIZE+BLOCK_SIZE*BLOCK_SIZE)/4;
		}
		
		pRef = (uint32 *)(g_FinalRefMB[iBlk]);

		for (i = 0; i < 8; i++)
		{
			pMea[0] = pRef[0];
			pMea[1] = pRef[1];
			
			pMea += dstWidth;
			pRef += 2;
		}	
	}
	
	//y
	pMea = mea_out_bfr;
	for(i = 0; i < 16; i++)
	{
		for(j = 0; j < 4; j++ )
		{
			fprintf_oneWord_hex(g_fp_mea_ref_mb_tv, pMea[j]);
		}
		pMea += 4;
	}
			
	//u
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 2; j++ )
		{
			fprintf_oneWord_hex(g_fp_mea_ref_mb_tv, pMea[j]);
		}
				
		pMea += 2;
	}
			
	//v
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 2; j++ )
		{
			fprintf_oneWord_hex(g_fp_mea_ref_mb_tv, pMea[j]);
		}
				
		pMea += 2;
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
