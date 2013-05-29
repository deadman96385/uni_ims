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


#ifdef   MCcache
#define cacheLEN 64
#define cacheLINE 16
static long  cache_buffer[cacheLEN];
static long update_ptr=0;
static long valid_ptr=0;
static long cache_len=0;
long hit_cnt=0;
long check_cnt=0;



static long  cache_blk_buf[512];
static long  blk_buf_ptr=0;

void cache_hit_check (uint8 *ref_pixel, int block_start)
{
	int i,j,m;
    int hit=0;
	int in_blcok=0;
	int addr= ((int)(&ref_pixel[0]))/cacheLINE;

//check in block
	if(block_start)
	{
		for(i=0;i<blk_buf_ptr;i++)
            cache_blk_buf[i]=0;
		blk_buf_ptr=0;
	}


	for(m=blk_buf_ptr-1;m>=0;m--)
	{
		if(addr==cache_blk_buf[m])
		{
						in_blcok=1;
						break;
		}
	}
     
//check in cache
    if(!in_blcok)
	{
		//add to block
		cache_blk_buf[blk_buf_ptr]=addr;
		blk_buf_ptr+=1;
        //cache check
	   j=valid_ptr;
       for(i=cache_len;i>0;i--)
	   {
		    if(addr==cache_buffer[j])
			{
			    
				hit=1;
              	hit_cnt+=1;
               
				break;
			}
		j--;
		if(j<0)
		j+=cacheLEN;
	   }
	

        //update
	    if(!hit)
		{
            if(cache_len<cacheLEN)
                cache_len+=1;
            cache_buffer[update_ptr]=addr;
		    valid_ptr=update_ptr;
            update_ptr+=1; 
		    if(update_ptr==cacheLEN)
			    update_ptr=0;
		}

	
	      check_cnt+=1;

	}//if(!in_blcok)
    
	
}
#endif

int32 s_coeff_tbl[6] = {1, -5, 20, 20, -5, 1};
extern char mbc_in_buf_luma[36*36];//jzy
extern char mbc_in_buf_chroma[18*18*2];
extern int32 b4order[16];
void h264_mc_luma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, int32 list)
{
#if defined(H264_DEC)
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
	uint8 *ref_frm_y_ptr = g_list0[ref_fw_frame_id]->imgY;
	

	int size,blkx,blky;//jzy

	if (list == 1)
	{
		ref_frm_y_ptr = g_list1[ref_bw_frame_id]->imgY;//weihu0730//g_list0
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;

	dx = start_pos_x & 0x3;
	dy = start_pos_y & 0x3;

	start_pos_x = (start_pos_x - dx)/4;
	start_pos_y = (start_pos_y - dy)/4;

	x_cor = start_pos_x;
	y_cor = start_pos_y;

	//jzy

	if(blk_size == 16)
	{
		size=36;blkx=0;blky=0;
	}
	else if (blk_size == 8)
	{
		size=18;
		blkx=ref_blk_id%2;
		blky=ref_blk_id/2;
	}
	else
	{
		size=9;
        blkx=b4order[ref_blk_id]%4;
		blky=b4order[ref_blk_id]/4;
	}


	if ((dx == 0) && (dy == 0)) //full pixel position
	{
		for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor+j);
			for (i =0; i < blk_size; i++)
			{
				x = IClip(x_min,x_max,x_cor+i);
				//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
				mbc_in_buf_luma[blky*size*36+blkx*size+(i)+(j)*36] = ref_frm_y_ptr[y*width+x];
			}				
		}
		MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
		
		/*for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor+j);
			for (i = ; i < blk_size; i++)
			{
				x = IClip(x_min,x_max,x_cor+i);					
				MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",ref_frm_y_ptr[y*width + x] );
			}
			MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
		}*/
		
		//MCA_FPRINTF (g_fp_trace_mcaflt, "//horizontal flt output**************************\n");
		//MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");
		//MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");
		
		for (j = 0; j < blk_size; j++)
		{
			x_cor = start_pos_x;
			y = IClip(y_min, y_max, y_cor);	
			
			for (i = 0; i < blk_size; i++)
			{
				x = IClip(x_min, x_max, x_cor);
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x], (i==0)&&(j==0));
#endif
				rec_blk_ptr[j*MB_SIZE+i] = ref_frm_y_ptr[y*width+x];
				x_cor++; //next pixel
			}
			y_cor++; //next line	
		}

		/*for (j = 0; j < blk_size; j++)
		{
			for (i = 0; i < blk_size; i++)
			{
				MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
			}
			MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			
		}*/
		
	}else//other position
	{
		int32 p0, p1, p2, p3, p4, p5;
		int32 *coeff = s_coeff_tbl;
		int32 reslt;
		int16 tmp_reslt[MB_SIZE][MB_SIZE+5];

		if (dy == 0) //no vertical interpolation
		{

			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i =-2 ; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
					mbc_in_buf_luma[blky*size*36+blkx*size+(i+2)+(j)*36] = ref_frm_y_ptr[y*width+x];
				}				
			}
			//6-tap interpolation in horizontal


			MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
			
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = -2; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

			//1/2 precious
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor);	
				
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],(i==0)&&(j==0));
#endif
					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}

            MCA_FPRINTF (g_fp_trace_mcaflt, "//horizontal flt output**************************\n");
            for (j = 0; j < blk_size; j++)
			{
				for (i = 0; i < blk_size; i++)
				{
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
				
			}
			
            MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");
            MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");


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

            for (j = 0; j < blk_size; j++)
			{
				for (i = 0; i < blk_size; i++)
				{
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
				
			}

		}//if (dy == 0)
		else if (dx == 0) ////no horizontal interpolation
		{
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i =0 ; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
					mbc_in_buf_luma[blky*size*36+blkx*size+(i)+(j+2)*36] = ref_frm_y_ptr[y*width+x];
				}				
			}
			//6-tap interpolation in vertical

           	MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
			
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

			MCA_FPRINTF (g_fp_trace_mcaflt, "//horizontal flt output**************************\n");
			MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");
            
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
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],(i==0)&&(j==0));
#endif
					y = IClip(y_min, y_max, y_cor-1);	
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif
					y = IClip(y_min, y_max, y_cor+0);	
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min, y_max, y_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min, y_max, y_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min, y_max, y_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++; //next pixel
				}
				y_cor++; //next line		
			}

			for (j = 0; j < blk_size; j++)
			{
				for (i = 0; i < blk_size; i++)
				{
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
				
			}

			MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");
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
			
            for (j = 0; j < blk_size; j++)
			{
				for (i = 0; i < blk_size; i++)
				{
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
				
			}

		} //if (dx == 0)
		else if (dx == 2) /* Vertical & horizontal interpolation */ //++ 参考像素横坐标在1/2像素点位置
		{

			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i =-2 ; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
					mbc_in_buf_luma[blky*size*36+blkx*size+(i+2)+(j+2)*36] = ref_frm_y_ptr[y*width+x];
				}				
			}

			if((blk_size==8)&&(start_pos_x==23)&&(start_pos_y==87)&&(dx==2)&&(dy==3))
                     blk_size=blk_size;
			MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
			
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = -2; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

			MCA_FPRINTF (g_fp_trace_mcaflt, "//horizotal flt output**************************\n");

			//++ 水平方向6抽头滤波（1/2精度插值）
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);	
				
				for (i = 0; i < blk_size; i++)
				{				
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],(i==0)&&(j==-2));
#endif

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					tmp_reslt[i][j+2] = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					x_cor++; //next pixel

					//MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}		
			}
			

			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = 0; i < blk_size; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					//MCA_FPRINTF (g_fp_trace_mcaflt, "%02x %02x ",p0,tmp_reslt[i][j+2] );
					MCA_FPRINTF (g_fp_trace_mcaflt, "%04x",tmp_reslt[i][j+2] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

            MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");

			//++ 垂直方向6抽头滤波（1/2精度插值）
			for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{
					p0 = tmp_reslt[i][j];
					p1 = tmp_reslt[i][j+1];
					p2 = tmp_reslt[i][j+2];
					p3 = tmp_reslt[i][j+3];
					p4 = tmp_reslt[i][j+4];
					p5 = tmp_reslt[i][j+5];

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+512)/1024);
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );//weihu
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
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
			MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");

            for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{	
                  MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );//weihu
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

		} //if (dx == 2)
		else if (dy == 2)/* Horizontal & vertical interpolation */	//++ 参考像素纵坐标在1/2像素点位置
		{
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i =-2 ; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
					mbc_in_buf_luma[blky*size*36+blkx*size+(i+2)+(j+2)*36] = ref_frm_y_ptr[y*width+x];
				}				
			}
			
			MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
			
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = -2; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}
			
			MCA_FPRINTF (g_fp_trace_mcaflt, "//horizotal flt output**************************\n");
			MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");
			
			
			//++ 垂直方向6抽头滤波（1/2精度插值）
			for (j = 0; j < blk_size; j++)
			{
				x_cor = start_pos_x;
			
				for (i = -2; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					
					y = IClip(y_min,y_max,y_cor+j-2);
					p0 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],(i==-2)&&(j==0));
#endif

					y = IClip(y_min,y_max,y_cor+j-1);
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+j+0);
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+j+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+j+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+j+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

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

						if(dx==1)
							MCA_FPRINTF (g_fp_trace_mcaflt, "%02x%02x",tmp,rec_blk_ptr[j*MB_SIZE+i]);
						else
							MCA_FPRINTF (g_fp_trace_mcaflt, "%02x%02x",rec_blk_ptr[j*MB_SIZE+i],tmp);
						rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + tmp+1)/2;
						
					}
					MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
				}
			} //if ((dy&1) == 1)
			
			MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");
			
            for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{	
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );//weihu
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

			
		} //if (dy == 2)
		else
		{
			int32 tmp;
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i =-2 ; i < blk_size+3; i++)
				{
					x = IClip(x_min,x_max,x_cor+i);
					//mbc_in_buf_luma[ref_blk_id*size*size+(j+2)*size+(i+2)] = ref_frm_y_ptr[y*width+x];
					mbc_in_buf_luma[blky*size*36+blkx*size+(i+2)+(j+2)*36] = ref_frm_y_ptr[y*width+x];
				}				
			}


			MCA_FPRINTF (g_fp_trace_mcaflt, "//luma input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx,dy );
			
			for (j = -2; j < blk_size+3; j++)
			{
				x_cor = start_pos_x;
				y = IClip(y_min, y_max, y_cor+j);
				for (i = -2; i < blk_size+3; i++)//for (i =blk_size+2; i >= -2; i--)
				{
					x = IClip(x_min,x_max,x_cor+i);
					p0 = ref_frm_y_ptr[y*width + x];
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}
			
			MCA_FPRINTF (g_fp_trace_mcaflt, "//horizotal flt output**************************\n");
			
			/* Diagonal interpolation *///++ 参考像素横纵坐标都在1/4像素点位置
			for (j = 0; j < blk_size; j++)
			{
				y_cor = (dy == 1)? (start_pos_y+j) : (start_pos_y+j+1);
				y = IClip(y_min,y_max, y_cor);

				x_cor = start_pos_x;//start_pos_x+blk_size-1;//
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{
					x = IClip(x_min,x_max,x_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],(i==0)&&(j==0));
#endif

					x = IClip (x_min, x_max, x_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip(x_min,x_max,x_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip (x_min, x_max, x_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip(x_min,x_max,x_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					x = IClip (x_min, x_max, x_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif


					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					rec_blk_ptr[j*MB_SIZE+i] = IClip(0, 255, (reslt+16)/32);
					x_cor++;//x_cor--;// //next pixel
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}

			MCA_FPRINTF (g_fp_trace_mcaflt, "//vertical flt output**************************\n");
			for (j = 0; j < blk_size; j++)
			{	
				y_cor = start_pos_y+j;
								
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{
					x_cor = (dx == 1)? (start_pos_x+i) : (start_pos_x+i+1);
					x = IClip(x_min,x_max,x_cor);

					y = IClip(y_min,y_max,y_cor-2);
					p0 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor-1);
					p1 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+0);
					p2 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+1);
					p3 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+2);
					p4 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					y = IClip(y_min,y_max,y_cor+3);
					p5 = ref_frm_y_ptr[y*width + x];
#ifdef MCcache
				cache_hit_check(&ref_frm_y_ptr[y*width+x],0);
#endif

					reslt = p0*coeff[0]+p1*coeff[1]+p2*coeff[2]+p3*coeff[3]+p4*coeff[4]+p5*coeff[5];
					tmp = IClip(0, 255, (reslt+16)/32);

					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",tmp );//weihu
					rec_blk_ptr[j*MB_SIZE+i] = (rec_blk_ptr[j*MB_SIZE+i] + tmp + 1)/2;
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}
			

			MCA_FPRINTF (g_fp_trace_mcaflt, "//final flt output**************************\n");
			
            for (j = 0; j < blk_size; j++)
			{				
				for (i = 0; i < blk_size; i++)//for (i = blk_size-1; i >=0 ; i--)//
				{	
					MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_ptr[j*MB_SIZE+i] );//weihu
				}
				MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
			}
			
		} //else
	} //other position
#endif
	return;
}

void h264_mc_chroma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_u_ptr, uint8 *rec_blk_v_ptr, int32 list)
{
#if defined(H264_DEC)
	int32 dx1, dy1;
	int32 dx2, dy2;
	int32 down_right, up_right, down_left, up_left;
	int32 x,y;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 width = mb_num_x * MB_SIZE; //uv interleaved
	int32 height = mb_num_y * MB_CHROMA_SIZE;
	int32 i, j;
	uint8 *ref_frm_u_ptr = g_list0[ref_fw_frame_id]->imgU;
	uint8 *ref_frm_v_ptr = g_list0[ref_fw_frame_id]->imgU+1;
	uint8 p0, p1, p2, p3;
	int size,blkx,blky,a,b;//jzy

	if (list == 1)
	{
		ref_frm_u_ptr = g_list1[ref_bw_frame_id]->imgU;//weihu0730//g_list0
		ref_frm_v_ptr = g_list1[ref_bw_frame_id]->imgU+1;//weihu0730//g_list0
	}


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

		//jzy

	if(blk_size == 8)
	{
		size=18;blkx=0;blky=0;
		a=b=0;
	}
	else if (blk_size == 4)
	{
		size=9;
		blkx=ref_blk_id%2;
		blky=ref_blk_id/2;
		a=b=0;
	}
	else
	{		
        blkx=b4order[ref_blk_id]%4;
		blky=b4order[ref_blk_id]/4;
		size=4;
		a=blkx/2;
		b=blky/2;
	}

	for (j = 0; j < blk_size+!(dy1==0); j++)
	{
		x_cor = 2*start_pos_x;
		y = IClip(y_min, y_max, y_cor+j);		
		for (i = 0; i < blk_size+!(dx1==0); i++)
		{
			x = IClip(x_min, x_max, x_cor);						
			mbc_in_buf_chroma[(blky*size+b)*18+(blkx*size+a)+i+j*18] = ref_frm_u_ptr[y*width+x];
			mbc_in_buf_chroma[18*18+(blky*size+b)*18+blkx*size+a+i+j*18] = ref_frm_v_ptr[y*width+x];
			
			x_cor+=2; //next pixel
		}
	     //next line
	}		
	if(dx1|dy1)	
	MCA_FPRINTF (g_fp_trace_mcaflt, "//chroma u input*****size=%d x=%3d  y=%3d  dx=%1d dy=%1d  \n",blk_size,start_pos_x,start_pos_y,dx1,dy1 );
	
	for (j = 0; j < blk_size+1; j++)
	{
		x_cor = start_pos_x*2;
		y = IClip(y_min, y_max, y_cor+j);
		for (i = 0; i < blk_size+1; i++)
		{
			x = IClip(x_min,x_max,x_cor+i*2);
			p0 = ref_frm_u_ptr[y*width + x];
			if(dx1|dy1)
			MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
		}
		if(dx1|dy1)
		MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
	}

	if(dx1|dy1)
	MCA_FPRINTF (g_fp_trace_mcaflt, "//u final flt output**************************\n");
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
			if(dx1|dy1)
			MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_u_ptr[j*MB_CHROMA_SIZE+i] );
		}
		y_cor++; //next line
		if(dx1|dy1)
		MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
	}

	
	//v
	x_cor = 2*start_pos_x;
	y_cor = start_pos_y;

	if(dx1|dy1)
	MCA_FPRINTF (g_fp_trace_mcaflt, "//chroma v input******************  \n" );
	for (j = 0; j < blk_size+1; j++)
	{
		x_cor = start_pos_x*2;
		y = IClip(y_min, y_max, y_cor+j);
		for (i = 0; i < blk_size+1; i++)
		{
			x = IClip(x_min,x_max,x_cor+i*2);
			p0 = ref_frm_v_ptr[y*width + x];
			if(dx1|dy1)
			MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",p0 );
		}
		if(dx1|dy1)
		MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
	}

	if(dx1|dy1)
	MCA_FPRINTF (g_fp_trace_mcaflt, "//v final flt output******\n");
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
			if(dx1|dy1)
			MCA_FPRINTF (g_fp_trace_mcaflt, "%02x",rec_blk_v_ptr[j*MB_CHROMA_SIZE+i] );
		}
		y_cor++; //next line
		if(dx1|dy1)
		MCA_FPRINTF (g_fp_trace_mcaflt, "\n");
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




















// End 




















// End 