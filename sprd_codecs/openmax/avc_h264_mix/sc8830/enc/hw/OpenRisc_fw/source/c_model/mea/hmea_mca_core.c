/******************************************************************************
 ** File Name:    hmea_mca_core.c											  *
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
#include "hmea_global.h"
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


void me_mc_luma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr)
{
#if defined(H264_ENC)
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
	uint8 *ref_frm_y_ptr = (uint8 *)g_ref_frame[0];

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;

	dx = start_pos_x & 0x3;
	dy = start_pos_y & 0x3;

	start_pos_x = (start_pos_x - dx)/4;
	start_pos_y = (start_pos_y - dy)/4;

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
				rec_blk_ptr[j*MB_SIZE+i] = ref_frm_y_ptr[y*width+x];
				x_cor++; //next pixel
			}
			y_cor++; //next line	
		}
	}else//other position
	{
		int32 p0, p1, p2, p3, p4, p5;
		int32 s_coeff_tbl[6] = {1, -5, 20, 20, -5, 1};
		int32 *coeff = s_coeff_tbl;
		int32 reslt;
		int16 tmp_reslt[MB_SIZE][MB_SIZE+5];

		if (dy == 0) //no vertical interpolation
		{
			//6-tap interpolation in horizontal

			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor);	
				
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}

			if ((dx&1) == 1) //1/4 precious
			{	
				y_cor = start_pos_y;		
				for (j = 0; j < blk_size; j++)
				{
					x_cor = start_pos_x;
					y = IClip(y_min, y_max, y_cor);	
					
					for (i = 0; i < blk_size; i++)
					{
						x = IClip(x_min, x_max, x_cor+dx/2);
						rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + ref_frm_y_ptr[y*width+x]+1)/2;
						x_cor++; //next pixel
					}
					y_cor++; //next line		
				}
			} //if ((dx&1) == 1)
		}//if (dy == 0)
		else if (dx == 0) ////no horizontal interpolation
		{
			//6-tap interpolation in vertical
			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min,y_max,y_cor);
						
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min, y_max, y_cor-2);	
					p0 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min, y_max, y_cor-1);	
					p1 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min, y_max, y_cor+0);	
					p2 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min, y_max, y_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min, y_max, y_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min, y_max, y_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}

			if ((dy&1) == 1) //1/4 precious
			{	
				y_cor = start_pos_y;
				for (j = 0; j < blk_size; j++)
				{
					x_cor = start_pos_x;
					y = IClip(y_min, y_max, y_cor+dy/2);
					
					for (i = 0; i < blk_size; i++)
					{
						x = IClip(x_min,x_max,x_cor);
											
						rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + ref_frm_y_ptr[y*width + x]+1)/2;
						x_cor++; //next pixel
					}
					y_cor++; //next line		
				}
			} //if ((dy&1) == 1)			
		} //if (dx == 0)
		else if (dx == 2) /* Vertical & horizontal interpolation */ //++ 参考像素横坐标在1/2像素点位置
		{
			//++ 水平方向6抽头滤波（1/2精度插值）
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);	
				
				for (i = 0; i < blk_size; i++)
				{				
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];

					tmp_reslt[i][j+2] = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					x_cor++; //next pixel
				}		
			}
			
			//++ 垂直方向6抽头滤波（1/2精度插值）
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

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+512)/1024);
				}	
			}

			if ((dy&1) == 1) //1/4 precious
			{	
				int32 tmp;

				for (j = 0; j < blk_size; j++)
				{				
					for (i = 0; i < blk_size; i++)
					{	
						tmp = IClip(0, 255, (tmp_reslt[i][j+2+dy/2]+16)/32);
						rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + tmp+1)/2;
					}
				}
			} //if ((dy&1) == 1)			
		} //if (dx == 2)
		else if (dy == 2)/* Horizontal & vertical interpolation */	//++ 参考像素纵坐标在1/2像素点位置
		{
			//++ 垂直方向6抽头滤波（1/2精度插值）
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
			
				for (i = -2; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					
					y = IClip(y_min,y_max,y_cor+j-2);
					p0 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+j-1);
					p1 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+j+0);
					p2 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+j+1);
					p3 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+j+2);
					p4 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+j+3);
					p5 = ref_frm_y_ptr[y*width + x];

					tmp_reslt[j][i+2] = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
				}		
			}

			//++ 水平方向6抽头滤波（1/2精度插值）
			for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)
				{
					p0 = tmp_reslt[j][i];
					p1 = tmp_reslt[j][i+1];
					p2 = tmp_reslt[j][i+2];
					p3 = tmp_reslt[j][i+3];
					p4 = tmp_reslt[j][i+4];
					p5 = tmp_reslt[j][i+5];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+512)/1024);
				}	
			}

			if ((dx&1) == 1) //1/4 precious
			{	
				int32 tmp;

				for (j = 0; j < blk_size; j++)
				{				
					for (i = 0; i < blk_size; i++)
					{	
						tmp = IClip(0, 255, (tmp_reslt[j][i+2+dx/2]+16)/32);
						rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + tmp+1)/2;
					}
				}
			} //if ((dy&1) == 1)					
		} //if (dy == 2)
		else
		{
			int32 tmp;
			
			/* Diagonal interpolation *///++ 参考像素横纵坐标都在1/4像素点位置
			for (j = 0; j < blk_size; j++)
			{
				y_cor = (dy == 1)? (start_pos_y+j) : (start_pos_y+j+1);
				y = IClip(y_min,y_max, y_cor);

				x_cor = start_pos_x;
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++; //next pixel
				}
			}

			for (j = 0; j < blk_size; j++)
			{	
				y_cor = start_pos_y+j;
								
				for (i = 0; i < blk_size; i++)
				{
					x_cor = (dx == 1)? (start_pos_x+i) : (start_pos_x+i+1);
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min,y_max,y_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];

					y = IClip(y_min,y_max,y_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					tmp = IClip(0, 255, (reslt+16)/32);
					rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + tmp + 1)/2;
				}
			}		
		} //else
	} //other position
#endif
	return;
}

void me_mc_chroma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_u_ptr, uint8 *rec_blk_v_ptr)
{
#if defined(H264_ENC)
	int32 dx1, dy1;
	int32 dx2, dy2;
	int32 down_right, up_right, down_left, up_left;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
#if 1//uv interleaved
	int32 width = mb_num_x * MB_SIZE; 
#else
	int32 width = mb_num_x * MB_CHROMA_SIZE; 
#endif
	int32 height = mb_num_y * MB_CHROMA_SIZE;
	int32 i, j;
	uint8 *ref_frm_u_ptr = (uint8 *)g_ref_frame[1];
	uint8 *ref_frm_v_ptr = (uint8 *)g_ref_frame[1] + 1;
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

#if 1 //uv interleaved
	x_min = 0; x_max = width - 2;
#else
	x_min = 0; x_max = width - 1;
#endif
	y_min = 0; y_max = height - 1;

	//u
#if 1 //uv interleaved
	x_cor = 2*start_pos_x;
#else
	x_cor = start_pos_x;
#endif
	y_cor = start_pos_y;

	for (j = 0; j < blk_size; j++)
	{
#if 1
		x_cor = 2*start_pos_x;
#else
		x_cor = start_pos_x;
#endif
				
		for (i = 0; i < blk_size; i++)
		{
			y = IClip(y_min,y_max,y_cor);

			x = IClip(x_min,x_max,x_cor);
			p0 = ref_frm_u_ptr[y*width + x];
#if 1
			x = IClip(x_min,x_max,x_cor+2);
#else
			x = IClip(x_min,x_max,x_cor+1);
#endif
			p1 = ref_frm_u_ptr[y*width + x];

			y = IClip(y_min,y_max,y_cor+1);

			x = IClip(x_min,x_max,x_cor);
			p2 = ref_frm_u_ptr[y*width + x];

#if 1
			x = IClip(x_min,x_max,x_cor+2);
#else
			x = IClip(x_min,x_max,x_cor+1);
#endif
			p3 = ref_frm_u_ptr[y*width + x];

			rec_blk_u_ptr[j*MB_CHROMA_SIZE+i] = (up_left*p0 + up_right*p1 + down_left*p2 + down_right*p3 + 32)/64;
#if 1
			x_cor+=2; //next pixel
#else
			x_cor+=1; //next pixel
#endif
		}
		y_cor++; //next line
	}

	//v
#if 1//uv interleaved
	x_cor = 2*start_pos_x;
#else
	x_cor = start_pos_x;
#endif
	y_cor = start_pos_y;

	for (j = 0; j < blk_size; j++)
	{
#if 1   //interleave
		x_cor = 2*start_pos_x;
#else
		x_cor = 1*start_pos_x;
#endif
					
		for (i = 0; i < blk_size; i++)
		{
			y = IClip(y_min, y_max, y_cor);	

			x = IClip(x_min,x_max,x_cor);
			p0 = ref_frm_v_ptr[y*width + x];

#if 1   //interleave
			x = IClip(x_min,x_max,x_cor+2);
#else
			x = IClip(x_min,x_max,x_cor+1);
#endif
			p1 = ref_frm_v_ptr[y*width + x];

			y = IClip(y_min,y_max,y_cor+1);

			x = IClip(x_min,x_max,x_cor);
			p2 = ref_frm_v_ptr[y*width + x];

#if 1  //interleave
			x = IClip(x_min,x_max,x_cor+2);
#else
			x = IClip(x_min,x_max,x_cor+1);
#endif
			p3 = ref_frm_v_ptr[y*width + x];

			rec_blk_v_ptr[j*MB_CHROMA_SIZE+i] = (up_left*p0 + up_right*p1 + down_left*p2 + down_right*p3 + 32)/64;
#if 1    //interleave
			x_cor+=2; //next pixel
#else
			x_cor+=1; //next pixel
#endif
		}
		y_cor++; //next line
	}
#endif
	return;
}


void MC_get_ref_mb(MODE_DECISION_T * mode_dcs_ptr)
{
	int32 start_pos_x, start_pos_y;
	int32 blk_size;
	uint8 *rec_blk_y_ptr, *rec_blk_u_ptr, *rec_blk_v_ptr;
	int32 mv_x, mv_y;
	int32 mb_x = g_mode_dcs_ptr->mb_x;
	int32 mb_y = g_mode_dcs_ptr->mb_y;

	if( (mode_dcs_ptr->mb_type == INTER_MB_16X16) || (mode_dcs_ptr->mb_type == INTER_MB_SKIP) )
	{
		mv_x = mode_dcs_ptr->blk_mv[0].x;
		mv_y = mode_dcs_ptr->blk_mv[0].y;

		start_pos_x = mb_x*MB_SIZE*4+mv_x;
		start_pos_y = mb_y*MB_SIZE*4+mv_y;
		
		blk_size = MB_SIZE;
		rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr;

		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

		blk_size = MB_CHROMA_SIZE;
		rec_blk_u_ptr = (uint8 *)vsp_mea_out_bfr+MCA_U_OFFSET;
		rec_blk_v_ptr = (uint8 *)vsp_mea_out_bfr+MCA_V_OFFSET;
		me_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);
	}
	else if ((mode_dcs_ptr->mb_type == INTER_MB_16X8) || (mode_dcs_ptr->mb_type == INTER_MB_8X16))
	{
		int32 i, j;
		int32 ref_blk_id;
		int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
		int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

		for (ref_blk_id = 0; ref_blk_id < 4; ref_blk_id++)
		{
			i = ref_blk_id%2;
			j = ref_blk_id/2;

			mv_x = mode_dcs_ptr->blk_mv[ref_blk_id].x;
			mv_y = mode_dcs_ptr->blk_mv[ref_blk_id].y;

			start_pos_x = mb_x*MB_SIZE*4+mv_x;
			start_pos_y = mb_y*MB_SIZE*4+mv_y;
			start_pos_x += (i*8*4);
			start_pos_y += (j*8*4);

			blk_size = MB_SIZE/2;
			rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr + b8_luma_offset[ref_blk_id];
			me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

			blk_size = MB_CHROMA_SIZE/2;
			rec_blk_u_ptr = (uint8 *)vsp_mea_out_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id];
			rec_blk_v_ptr = (uint8 *)vsp_mea_out_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id];
			me_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);
		}	
	}
	else
	{
		int32 i_blk;
		int32 sub_mb_type;

		for (i_blk = 0; i_blk < 4; i_blk++)
		{
		//	sub_mb_type = mode_dcs_ptr->sub_mb_type[i_blk];
			sub_mb_type = BLK8X8;

			if (sub_mb_type == BLK8X8)
			{
				int32 i, j;
				int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
				int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};
				
				i = i_blk%2;
				j = i_blk/2;

				mv_x = mode_dcs_ptr->blk_mv[i_blk].x;
				mv_y = mode_dcs_ptr->blk_mv[i_blk].y;

				start_pos_x = mb_x*MB_SIZE*4+mv_x;
				start_pos_y = mb_y*MB_SIZE*4+mv_y;
				start_pos_x += (i*8*4);
				start_pos_y += (j*8*4);

				blk_size = MB_SIZE/2;
				rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr + b8_luma_offset[i_blk];
				me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

				blk_size = MB_CHROMA_SIZE/2;
				rec_blk_u_ptr = (uint8 *)vsp_mea_out_bfr+MCA_U_OFFSET+b8_chroma_offset[i_blk];
				rec_blk_v_ptr = (uint8 *)vsp_mea_out_bfr+MCA_V_OFFSET+b8_chroma_offset[i_blk];
				me_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);
			}
			else// if (sub_mb_type == BLK8X4)||(sub_mb_type == BLK4X8)||4x4
			{	
				int i_blk4;
								
				for (i_blk4 = 0; i_blk4 < 4; i_blk4++)
				{
					int32 i, j;
					//0, 1,    4,  5
					//2, 3,    6,  7
					//8, 9,   12, 13
					//10, 11, 14, 15
					int32 sub4x4_luma_offset[16] = 
						{
								0,        4,     4*16,    4*16+4,
								8,       12,   4*16+8,   4*16+12,
							 8*16,   8*16+4,    12*16,   12*16+4,
							8*16+8, 8*16+12,  12*16+8,  12*16+12
						};
					int32 sub4x4_chroma_offset[16] = 
						{
							0,		2,		2*8,	2*8+2,
							4,		6,		2*8+4,	2*8+6,
							4*8,	4*8+2,	6*8,	6*8+2,
							4*8+4,	4*8+6,	6*8+4,	6*8+6

						};
					int32 i_idx[16] = {0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
					int32 j_idx[16] = {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};
					
					ref_blk_id = i_blk *4 + i_blk4;
					
					i = i_idx[ref_blk_id];
					j = j_idx[ref_blk_id];

					mv_x = mode_dcs_ptr->blk_mv[j*4+i].x;
					mv_y = mode_dcs_ptr->blk_mv[j*4+i].y;

					start_pos_x = mb_x*MB_SIZE*4+mv_x;
					start_pos_y = mb_y*MB_SIZE*4+mv_y;
					start_pos_x += (i*4*4);
					start_pos_y += (j*4*4);

					blk_size = MB_SIZE/4;
					rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr + sub4x4_luma_offset[ref_blk_id];
					me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

					blk_size = MB_CHROMA_SIZE/4;
					rec_blk_u_ptr = (uint8 *)vsp_mea_out_bfr+MCA_U_OFFSET+sub4x4_chroma_offset[ref_blk_id];
					rec_blk_v_ptr = (uint8 *)vsp_mea_out_bfr+MCA_V_OFFSET+sub4x4_chroma_offset[ref_blk_id];
					me_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);		
				}
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