/******************************************************************************
 ** File Name:    hdbk_core_ctrl_jpeg.c										  *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void out_org444(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width  = (8 >> scale_down_factor);
	int32 output_mcu_height = (8 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}

	//update
	output_mcu_width = output_mcu_width>>1;
	output_blk_width = output_blk_width>>1;
	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 64;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
			
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 96;
	if (!uv_interleaved) //three plane
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}else //two plane
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;

		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];			
			}
		}
	}

	return;
}

void out_org411(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width  = (32 >> scale_down_factor);
	int32 output_mcu_height = (8 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
		dst_ptr += output_blk_width;
	}

	//update
	output_mcu_width = (16 >> scale_down_factor);
	output_blk_width = (16 >> scale_down_factor);
	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 256;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else //two plane
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
			
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 384;

	if (!uv_interleaved) //three plane
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}else //two plane
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
		}
	}
	
	return;
}

void out_org411R(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width  = (8 >> scale_down_factor);
	int32 output_mcu_height = (32 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
		dst_ptr += (output_blk_height*out_width_y);
	}

	//update
	output_mcu_width	= (4 >> scale_down_factor);
	output_mcu_height	= (16 >> scale_down_factor);
	output_blk_width	= (4 >> scale_down_factor);
	output_blk_height	= (16 >> scale_down_factor);
	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 256;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 320;
	if (!uv_interleaved) //three plane
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}else
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j*2] = src_ptr[i*output_blk_width+j];
			}
		}
	}
	
}

void out_org422R(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width  = (8 >> scale_down_factor);
	int32 output_mcu_height = (16 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
		dst_ptr += (output_blk_height*out_width_y);
	}

	//update
	output_mcu_width = (4 >> scale_down_factor);
	output_mcu_height = (8 >> scale_down_factor);
	output_blk_width = (4 >> scale_down_factor);
	output_blk_height = (8 >> scale_down_factor);
	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 128;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else //two plane
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}		
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 160;
	if (!uv_interleaved)
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j*2] = src_ptr[i*output_blk_width+j];
			}
		}
	}else
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j*2] = src_ptr[i*output_blk_width+j];
			}
		}
	}

	return;
}

void out_org422(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width  = (16 >> scale_down_factor);
	int32 output_mcu_height = (8 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y>>1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 2; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
		dst_ptr += output_blk_width;
	}

	//update
	output_mcu_width = (8 >> scale_down_factor);
	output_mcu_height = (8 >> scale_down_factor);
	output_blk_width = (8 >> scale_down_factor);
	output_blk_height = (8 >> scale_down_factor);
	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 128;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else //two plane
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 192;
	if (!uv_interleaved) //three plane
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}else //two plane
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j*2] = src_ptr[i*output_blk_width+j];
			}
		}
	}
	
	return;
}

void out_org420(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 output_mcu_width	= (16 >> scale_down_factor);
	int32 output_mcu_height = (16 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;

	//y
	for(y_blk_idx = 0; y_blk_idx < 4; y_blk_idx++)
	{
		x_offset = mcu_x * output_mcu_width + (y_blk_idx % 2) * output_blk_width;
		y_offset = mcu_y * output_mcu_height + (y_blk_idx >> 1) * output_blk_height;

		dst_ptr = frm_y + out_width_y * y_offset + x_offset;

		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
	}

	//update
	output_mcu_width = (8 >> scale_down_factor);
	output_mcu_height = (8 >> scale_down_factor);
	output_blk_width = (8 >> scale_down_factor);
	output_blk_height = (8 >> scale_down_factor);
	x_offset = mcu_x * output_blk_width;
	y_offset = mcu_y * output_blk_height;

	if (uv_interleaved)
	{
		out_width_c <<= 1;
		x_offset <<= 1;
	}

	//u
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 256;
	dst_ptr = frm_u + out_width_c * y_offset + x_offset;
	for(i = 0; i < output_blk_height; i++)
	{
		for(j = 0; j < output_blk_width; j++)
		{
			if (!uv_interleaved) //three plane
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}else
			{
				dst_ptr[i*out_width_c+2*j] = src_ptr[i*output_blk_width+j];
			}
		}
	}

	//v
	src_ptr = (uint8 *)vsp_dbk_out_bfr + 320;
	if (!uv_interleaved)
	{
		dst_ptr = frm_v + out_width_c * y_offset + x_offset;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j] = src_ptr[i*output_blk_width+j];
			}
		}
	}else
	{
		dst_ptr = frm_u + out_width_c * y_offset + x_offset+1;
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_c+j*2] = src_ptr[i*output_blk_width+j];
			}
		}
	}
	return;
}

void out_org400(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor)
{
	int32 output_mcu_width = (8 >> scale_down_factor);
	int32 output_mcu_height = (8 >> scale_down_factor);
	int32 output_blk_width  = (8 >> scale_down_factor);
	int32 output_blk_height = (8 >> scale_down_factor);
	int32 out_width_c = out_width_y >> 1;
	int32 y_blk_idx = 0;
	int32 x_offset, y_offset;
	uint8 *src_ptr = NULL;
	uint8 *dst_ptr = NULL;
	int32 i, j;

	x_offset = mcu_x * output_mcu_width;
	y_offset = mcu_y * output_mcu_height;

	src_ptr = (uint8 *)vsp_dbk_out_bfr;
	dst_ptr = frm_y + out_width_y * y_offset + x_offset;

	//y
	for(y_blk_idx = 0; y_blk_idx < 1; y_blk_idx++)
	{
		for(i = 0; i < output_blk_height; i++)
		{
			for(j = 0; j < output_blk_width; j++)
			{
				dst_ptr[i*out_width_y+j] = src_ptr[i*output_blk_width+j];
			}
		}
		src_ptr += 64;
		dst_ptr += output_blk_width;
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