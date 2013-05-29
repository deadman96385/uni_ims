/******************************************************************************
 ** File Name:    mca_core_h264.c											  *
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
#include "vp8_blockd.h"
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
typedef const int array2d[6];

const int32 BilinearFilters[8][6] =
{
	{ 0, 0, 128, 0, 0, 0 },
	{ 0, 0, 112, 16, 0, 0 },
	{ 0, 0, 96, 32, 0, 0 },
	{ 0, 0, 80, 48, 0, 0 },
	{ 0, 0, 64, 64, 0, 0 },
	{ 0, 0, 48, 80, 0, 0 },
	{ 0, 0, 32, 96, 0, 0 },
	{ 0, 0, 16, 112, 0, 0 }
};
const int32 filters [8] [6] = { /* indexed by displacement */
	{ 0, 0, 128, 0, 0, 0 }, /* degenerate whole-pixel */
	{ 0, -6, 123, 12, -1, 0 }, /* 1/8 */
	{ 2, -11, 108, 36, -8, 1 }, /* 1/4 */
	{ 0, -9, 93, 50, -6, 0 }, /* 3/8 */
	{ 3, -16, 77, 77, -16, 3 }, /* 1/2 is symmetric */
	{ 0, -6, 50, 93, -9, 0 }, /* 5/8 = reverse of 3/8 */
	{ 1, -8, 36, 108, -11, 2 }, /* 3/4 = reverse of 1/4 */
	{ 0, -1, 12, 123, -6, 0 } /* 7/8 = reverse of 1/8 */
};

// use this define on systems where unaligned int reads and writes are
// not allowed, i.e. ARM architectures
//#define MUST_BE_ALIGNED

static const int bbb[4] = {0, 2, 8, 10};


void vp8_mc (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd)
{
#if defined(VP8_DEC)
	int32 dx, dy;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 width = mb_num_x * MB_SIZE;
	int32 height = mb_num_y * MB_SIZE;
	int32 i, j;
// 	uint8 *fw_mca_bfr_ptr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *ref_frm_y_ptr = y_buffer;//g_list0[ref_fw_frame_id]->imgY;
	uint32 pre_stride = width + 2*32;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;

	dx = start_pos_x & 0x7;
	dy = start_pos_y & 0x7;

	start_pos_x = (start_pos_x - dx)>>3;
	start_pos_y = (start_pos_y - dy)>>3;

	x_cor = start_pos_x;
	y_cor = start_pos_y;

	if ((dx == 0) && (dy == 0)) //full pixel position
	{
		for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor);	
			
			for (i = 0; i < blk_size; i++)
			{
				x = IClip(x_min, x_max, x_cor);
				rec_blk_ptr[j*MB_SIZE+i] = ref_frm_y_ptr[y*pre_stride+x];
				x_cor++; //next pixel
			}
			y_cor++; //next line	
		}
	}else//other position
	{
		int32 p0, p1, p2, p3, p4, p5;
		int32 pos,pos_x,pos_y;
		array2d * coeff;
		int32 reslt;
		int16 tmp_reslt[MB_SIZE][MB_SIZE+5];
		if(!int_type)
		{
		coeff = filters;
		}
		else
		{
		coeff = BilinearFilters;
		}
		

		if (dy == 0) //no vertical interpolation
		{
			//6-tap interpolation in horizontal
			pos  = dx;

			///1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor);	
				
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}
		}//if (dy == 0)
		else if (dx == 0) ////no horizontal interpolation
		{
			//6-tap interpolation in vertical
			pos  = dy;
			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min,y_max,y_cor);
						
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min, y_max, y_cor-2);	
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor-1);	
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+0);	
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}	
		} //if (dx == 0)
		
		else		//the other 9 points (1 1/2point and 8 1/4 point) Jin
		{
			//int32 tmp;
			pos_x = dx;
			pos_y = dy;
			
			/* Diagonal interpolation *///++ 参考像素横纵坐标都在1/4像素点位置
			for (j = -2; j < blk_size+3; j++)
			{
				//y_cor = (dy == 1)? (start_pos_y+j) : (start_pos_y+j+1);
				//y = IClip(y_min,y_max, y_cor);
				y = IClip(y_min, y_max, y_cor+j);	
				x_cor = start_pos_x;
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos_x][0]+p1*coeff[pos_x][1]+p2*coeff[pos_x][2]+p3*coeff[pos_x][3]+p4*coeff[pos_x][4]+p5*coeff[pos_x][5];
					tmp_reslt[i][j+2] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
			}
			for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)
				{
					p0 = tmp_reslt[i][j];
					p1 = tmp_reslt[i][j+1];
					p2 = tmp_reslt[i][j+2];
					p3 = tmp_reslt[i][j+3];
					p4 = tmp_reslt[i][j+4];
					p5 = tmp_reslt[i][j+5];

					reslt = p0*coeff[pos_y][0]+p1*coeff[pos_y][1]+p2*coeff[pos_y][2]+p3*coeff[pos_y][3]+p4*coeff[pos_y][4]+p5*coeff[pos_y][5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+64)/128);
				}	
			}
		} //else
	} //other position
#endif
	return;
}

void vp8_mc_u (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd)
{
#if defined(VP8_DEC)
	int32 dx, dy;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 width = mb_num_x * MB_SIZE;
	int32 height = mb_num_y * MB_SIZE;
	int32 i, j;
// 	uint8 *fw_mca_bfr_ptr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *ref_frm_y_ptr = u_buffer;//g_list0[ref_fw_frame_id]->imgY;
	uint32 pre_stride = (width + 2*32)>>1;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;

	dx = start_pos_x & 0x7;
	dy = start_pos_y & 0x7;

	start_pos_x = (start_pos_x - dx)>>3;
	start_pos_y = (start_pos_y - dy)>>3;

	x_cor = start_pos_x;
	y_cor = start_pos_y;

	if ((dx == 0) && (dy == 0)) //full pixel position
	{
		for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor);	
			
			for (i = 0; i < blk_size; i++)
			{
				x = IClip(x_min, x_max, x_cor);
				rec_blk_ptr[j*BLOCK_SIZE+i] = ref_frm_y_ptr[y*pre_stride+x];
				x_cor++; //next pixel
			}
			y_cor++; //next line	
		}
	}else//other position
	{
		int32 p0, p1, p2, p3, p4, p5;
		int32 pos,pos_x,pos_y;
		int32 reslt;
		int16 tmp_reslt[MB_SIZE][MB_SIZE+5];
		array2d * coeff;
		if(!int_type)
		{
		coeff = filters;
		}
		else
		{
		coeff = BilinearFilters;
		}

		if (dy == 0) //no vertical interpolation
		{
			//6-tap interpolation in horizontal
			pos  = dx;

			///1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor);	
				
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}
		}//if (dy == 0)
		else if (dx == 0) ////no horizontal interpolation
		{
			//6-tap interpolation in vertical
			pos  = dy;
			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min,y_max,y_cor);
						
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min, y_max, y_cor-2);	
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor-1);	
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+0);	
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}	
		} //if (dx == 0)
		

		else		//the other 9 points (1 1/2point and 8 1/4 point) Jin
		{
			//int32 tmp;
			pos_x = dx;
			pos_y = dy;
			
			/* Diagonal interpolation *///++ 参考像素横纵坐标都在1/4像素点位置
			for (j = -2; j < blk_size+3; j++)
			{
				//y_cor = (dy == 1)? (start_pos_y+j) : (start_pos_y+j+1);
				//y = IClip(y_min,y_max, y_cor);
				y = IClip(y_min, y_max, y_cor+j);	
				x_cor = start_pos_x;
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos_x][0]+p1*coeff[pos_x][1]+p2*coeff[pos_x][2]+p3*coeff[pos_x][3]+p4*coeff[pos_x][4]+p5*coeff[pos_x][5];
					tmp_reslt[i][j+2] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
			}
			for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)
				{
					p0 = tmp_reslt[i][j];
					p1 = tmp_reslt[i][j+1];
					p2 = tmp_reslt[i][j+2];
					p3 = tmp_reslt[i][j+3];
					p4 = tmp_reslt[i][j+4];
					p5 = tmp_reslt[i][j+5];

					reslt = p0*coeff[pos_y][0]+p1*coeff[pos_y][1]+p2*coeff[pos_y][2]+p3*coeff[pos_y][3]+p4*coeff[pos_y][4]+p5*coeff[pos_y][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
				}	
			}
		} //else
	} //other position
#endif
	return;
}

void vp8_mc_v (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd)
{
#if defined(VP8_DEC)
	int32 dx, dy;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 width = mb_num_x * MB_SIZE;
	int32 height = mb_num_y * MB_SIZE;
	int32 i, j;
// 	uint8 *fw_mca_bfr_ptr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *ref_frm_y_ptr = v_buffer;//g_list0[ref_fw_frame_id]->imgY;
	uint32 pre_stride = (width + 2*32)>>1;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;

	dx = start_pos_x & 0x7;
	dy = start_pos_y & 0x7;

	start_pos_x = (start_pos_x - dx)>>3;
	start_pos_y = (start_pos_y - dy)>>3;

	x_cor = start_pos_x;
	y_cor = start_pos_y;

	if ((dx == 0) && (dy == 0)) //full pixel position
	{
		for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor);	
			
			for (i = 0; i < blk_size; i++)
			{
				x = IClip(x_min, x_max, x_cor);
				rec_blk_ptr[j*BLOCK_SIZE+i] = ref_frm_y_ptr[y*pre_stride+x];
				x_cor++; //next pixel
			}
			y_cor++; //next line	
		}
	}else//other position
	{
		int32 p0, p1, p2, p3, p4, p5;
		int32 pos,pos_x,pos_y;
		array2d * coeff;
		int32 reslt;
		int16 tmp_reslt[MB_SIZE][MB_SIZE+5];
		if(!int_type)
		{
		coeff = filters;
		}
		else
		{
		coeff = BilinearFilters;
		}
		

		if (dy == 0) //no vertical interpolation
		{
			//6-tap interpolation in horizontal
			pos  = dx;

			///1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor);	
				
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}
		}//if (dy == 0)
		else if (dx == 0) ////no horizontal interpolation
		{
			//6-tap interpolation in vertical
			pos  = dy;
			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min,y_max,y_cor);
						
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min, y_max, y_cor-2);	
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor-1);	
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+0);	
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					y = IClip(y_min, y_max, y_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos][0]+p1*coeff[pos][1]+p2*coeff[pos][2]+p3*coeff[pos][3]+p4*coeff[pos][4]+p5*coeff[pos][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}	
		} //if (dx == 0)
		

		else		//the other 9 points (1 1/2point and 8 1/4 point) Jin
		{
			//int32 tmp;
			pos_x = dx;
			pos_y = dy;
			
			/* Diagonal interpolation *///++ 参考像素横纵坐标都在1/4像素点位置
			for (j = -2; j < blk_size+3; j++)
			{
				//y_cor = (dy == 1)? (start_pos_y+j) : (start_pos_y+j+1);
				//y = IClip(y_min,y_max, y_cor);
				y = IClip(y_min, y_max, y_cor+j);	
				x_cor = start_pos_x;
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*pre_stride + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*pre_stride + x];

					reslt = p0*coeff[pos_x][0]+p1*coeff[pos_x][1]+p2*coeff[pos_x][2]+p3*coeff[pos_x][3]+p4*coeff[pos_x][4]+p5*coeff[pos_x][5];
					tmp_reslt[i][j+2] = IClip(0, 255, (reslt+64)/128);
					x_cor++; //next pixel
				}
			}
			for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)
				{
					p0 = tmp_reslt[i][j];
					p1 = tmp_reslt[i][j+1];
					p2 = tmp_reslt[i][j+2];
					p3 = tmp_reslt[i][j+3];
					p4 = tmp_reslt[i][j+4];
					p5 = tmp_reslt[i][j+5];

					reslt = p0*coeff[pos_y][0]+p1*coeff[pos_y][1]+p2*coeff[pos_y][2]+p3*coeff[pos_y][3]+p4*coeff[pos_y][4]+p5*coeff[pos_y][5];
					rec_blk_ptr[j*BLOCK_SIZE+i] = IClip(0, 255, (reslt+64)/128);
				}	
			}
		} //else
	} //other position
#endif
	return;
}
void vp8_mc_chroma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_u_ptr, uint8 *rec_blk_v_ptr)
{
#if defined(VP8_DEC_disabled)
	int32 dx1, dy1;
	int32 dx2, dy2;
	int32 down_right, up_right, down_left, up_left;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 pre_stride = mb_num_x * MB_SIZE; //uv interleaved
	int32 height = mb_num_y * MB_CHROMA_SIZE;
	int32 i, j;
	uint8 *ref_frm_u_ptr = g_list0[ref_fw_frame_id]->imgU;
	uint8 *ref_frm_v_ptr = g_list0[ref_fw_frame_id]->imgU+1;
	uint8 p0, p1, p2, p3;

	dx1 = start_pos_x & 0x7;
	dy1 = start_pos_y & 0x7;
	dx2 = 8 - dx1;
	dy2 = 8 - dy1;

	down_right = dx1 * dy1;
	up_right = dx1 * dy2;
	down_left = dx2 * dy1;
	up_left = dx2 * dy2;

	start_pos_x >>= 3;
	start_pos_y >>= 3;

	x_min = 0; x_max = width - 2;
	y_min = 0; y_max = height - 1;

	//u
	x_cor = 2*start_pos_x;
	y_cor = start_pos_y;

	for (j = 0; j < blk_size; j++)
	{
		x_cor = 2*start_pos_x;
				
		for (i = 0; i < blk_size; i++)
		{
			y = IClip(y_min,y_max,y_cor);

			x = IClip(x_min,x_max,x_cor);
			p0 = ref_frm_u_ptr[y*width + x];

			x = IClip(x_min,x_max,x_cor+2);
			p1 = ref_frm_u_ptr[y*width + x];

			y = IClip(y_min,y_max,y_cor+1);

			x = IClip(x_min,x_max,x_cor);
			p2 = ref_frm_u_ptr[y*width + x];

			x = IClip(x_min,x_max,x_cor+2);
			p3 = ref_frm_u_ptr[y*width + x];

			rec_blk_u_ptr[j*MB_CHROMA_SIZE+i] = (up_left*p0 + up_right*p1 + down_left*p2 + down_right*p3 + 32)/64;
			x_cor+=2; //next pixel
		}
		y_cor++; //next line
	}

	//v
	x_cor = 2*start_pos_x;
	y_cor = start_pos_y;

	for (j = 0; j < blk_size; j++)
	{
		x_cor = 2*start_pos_x;
					
		for (i = 0; i < blk_size; i++)
		{
			y = IClip(y_min, y_max, y_cor);	

			x = IClip(x_min,x_max,x_cor);
			p0 = ref_frm_v_ptr[y*width + x];

			x = IClip(x_min,x_max,x_cor+2);
			p1 = ref_frm_v_ptr[y*width + x];

			y = IClip(y_min,y_max,y_cor+1);

			x = IClip(x_min,x_max,x_cor);
			p2 = ref_frm_v_ptr[y*width + x];

			x = IClip(x_min,x_max,x_cor+2);
			p3 = ref_frm_v_ptr[y*width + x];

			rec_blk_v_ptr[j*MB_CHROMA_SIZE+i] = (up_left*p0 + up_right*p1 + down_left*p2 + down_right*p3 + 32)/64;
			x_cor+=2; //next pixel
		}
		y_cor++; //next line
	}
#endif
	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
