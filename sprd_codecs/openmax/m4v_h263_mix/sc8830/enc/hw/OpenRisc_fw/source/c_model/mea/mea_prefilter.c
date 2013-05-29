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
		
int8 wavelet_matrix1[4][4] = 
	{1,  1,  1,  1,
	1, -1,  1, -1,
	1,  1, -1, -1,
	1, -1, -1,  1};

#define CLIP(input,top, bottom) {if (input>top) input = top; if (input < bottom) input = bottom;}

void noise_reduction_fun2(uint8 pInput[4], uint8 *pOutput, uint8 Thresh) 										  
{
	int i,j;
	int w_coef[4];
	int tmp_val, shinkaged_coef;
	int Threshold = Thresh;
	
	for(i=0; i<4; i++)
	{
		tmp_val = 0;
		for(j=0; j<4; j++)
		{
			tmp_val+= wavelet_matrix1[i][j]*pInput[j];
		}
		shinkaged_coef = tmp_val;
		if(i!=0)
		{
			if(shinkaged_coef <= Threshold && shinkaged_coef >= -Threshold)
				shinkaged_coef  = 0;
			else if(shinkaged_coef>Threshold)
				shinkaged_coef = shinkaged_coef - Threshold;
			else
				shinkaged_coef = shinkaged_coef + Threshold;
		}
		w_coef[i] = shinkaged_coef;
	}
	
	for(i=0; i<4; i++)
	{
		tmp_val = 0;
		for(j=0; j<4; j++)
		{
			tmp_val+= wavelet_matrix1[i][j]*w_coef[j];
		}
		tmp_val  = (tmp_val+2)/4;
		CLIP(tmp_val, 255, 0);
		
		pOutput[i] = (uint8)(tmp_val);
	}
}

void pre_filter_mb(int32 mb_x, int32 mb_y, int32 pre_filter_thres)
{
#if defined(MPEG4_ENC)
	int32 i, j, k, l;
	uint8 ptmp[4];
	uint8 ptmpOutBuf[4];
	int32 frame_width = g_mea_frame_width << 2; //byte
	int32 frame_height = g_mea_frame_height;
	int32 frame_width_c = frame_width >> 1;
	int32 frame_height_c = frame_height >> 1;
	int32 start_pos_x = mb_x *MB_SIZE;
	int32 end_pos_x = (mb_x+1) *MB_SIZE;
	int32 start_pos_y = mb_y *MB_SIZE;
	int32 end_pos_y = (mb_y+1) *MB_SIZE;
	uint8 *mea_src_ptr = PNULL;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	
	if(end_pos_x == frame_width)
	{
		end_pos_x -= 1;
	}

	if(end_pos_y == frame_height)
	{
		end_pos_y -= 1;
	}

	//y
	mea_src_ptr = (uint8 *)g_mea_src_frame_y;
	for (i = start_pos_y; i < end_pos_y; i++)
	{
		for (j = start_pos_x; j < end_pos_x; j++ )
		{
			for(k = 0; k < 2; k++)
			{
				for(l = 0; l < 2; l++)
				{
					ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width + (j + l)] ;
				}
			}

			noise_reduction_fun2(ptmp,ptmpOutBuf,pre_filter_thres);
		
			mea_src_ptr[i * frame_width + j] = ptmpOutBuf[0] ;//y_out_ptr[i * iwidth + j];
			
		}
	}

	start_pos_x = mb_x *BLOCK_SIZE;
	end_pos_x = (mb_x+1) *BLOCK_SIZE;
	start_pos_y = mb_y *BLOCK_SIZE;
	end_pos_y = (mb_y+1) *BLOCK_SIZE;

	if(end_pos_x == frame_width_c)
	{
		end_pos_x -= 1;
	}

	if(end_pos_y == frame_height_c)
	{
		end_pos_y -= 1;
	}

	//u
	mea_src_ptr = (uint8 *)g_mea_src_frame_u;
	for (i = start_pos_y; i < end_pos_y; i++)
	{
		for (j = start_pos_x; j < end_pos_x; j++)
		{
			for(k = 0; k < 2; k++)
			{
				for(l = 0; l < 2; l++)
				{
					if (!uv_interleaved)
					{
						ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width_c + (j + l)] ;
					}else
					{
						ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width_c*2 + (j + l)*2] ;
					}
				}
			}
			
			noise_reduction_fun2(ptmp,ptmpOutBuf,pre_filter_thres);

			if (!uv_interleaved)
			{
				mea_src_ptr[i * frame_width_c + j] = ptmpOutBuf[0];
			}else
			{
				mea_src_ptr[i * frame_width_c*2 + j*2] = ptmpOutBuf[0];
			}
		}
	}

	//v
	if (!uv_interleaved) //three plane
	{
		mea_src_ptr = (uint8 *)g_mea_src_frame_v;
		for (i = start_pos_y; i < end_pos_y; i++)
		{
			for (j = start_pos_x; j < end_pos_x; j++)
			{
				for(k = 0; k < 2; k++)
				{
					for(l = 0; l < 2; l++)
					{
						ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width_c + (j + l)] ;
					}
				}
				
				noise_reduction_fun2(ptmp, ptmpOutBuf,pre_filter_thres);
				
				mea_src_ptr[i * frame_width_c + j] = ptmpOutBuf[0];			
			}
		}
	}else //two plane
	{
		mea_src_ptr = (uint8 *)g_mea_src_frame_u+1;
		for (i = start_pos_y; i < end_pos_y; i++)
		{
			for (j = start_pos_x; j < end_pos_x; j++)
			{
				for(k = 0; k < 2; k++)
				{
					for(l = 0; l < 2; l++)
					{
						if (!uv_interleaved) //three plane
						{
							ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width_c + (j + l)] ;
						}else
						{
							ptmp[k * 2 + l] = mea_src_ptr[(i + k) * frame_width_c*2 + (j + l)*2] ;
						}
					}
				}
				
				noise_reduction_fun2(ptmp, ptmpOutBuf,pre_filter_thres);
				
				if (!uv_interleaved) //three plane
				{
					mea_src_ptr[i * frame_width_c + j] = ptmpOutBuf[0];			
				}else //two plane
				{
					mea_src_ptr[i * frame_width_c*2 + j*2] = ptmpOutBuf[0];			
				}
			}
		}
	}
#endif //defined(MPEG4_ENC)
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
