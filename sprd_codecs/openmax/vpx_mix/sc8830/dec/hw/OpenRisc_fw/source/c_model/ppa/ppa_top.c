#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "ppa_global.h"
#include "common_global.h"
#include "buffer_global.h"
//#ifdef H264_DEC
#include "hvld_mode.h"
//#endif
//#define debugoutput
#ifdef debugoutput

#include <stdio.h>
FILE *fout1;
#endif

//#define CLIPZ(x,y,z)   (z<x)? x : ((z>y)? y : z )
//#define ISQT_FPRINT //fprintf
#define mmin(aa,bb)		(((aa) < (bb)) ? (aa) : (bb))


uint8 h264_QP_SCALER_CR_TBL[52]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
		12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
		28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
		37,38,38,38,39,39,39,39		
};

uint8 h264_qpPerRem_tbl [52][2] = {
	{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5},
	{1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5},
	{2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5},
	{3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5},
	{4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5},
	{5, 0}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5},
	{6, 0}, {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5},
	{7, 0}, {7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5},
	{8, 0}, {8, 1}, {8, 2}, {8, 3}
};

uint8 h264_blk_order_map[16+2 * 4] = 
{
    1 *6+1, 1 *6+2, 2 *6+1, 2 *6+2,  //first one block8x8
	1 *6+3, 1 *6+4, 2 *6+3, 2 *6+4, 
	3 *6+1, 3 *6+2, 4 *6+1, 4 *6+2, 
	3 *6+3, 3 *6+4, 4 *6+3, 4 *6+4, 
	
	6 *6+1, 6 *6+2, 7 *6+1, 7 *6+2,  //U's 4 block4x4
	6 *6+4, 6 *6+5, 7 *6+4, 7 *6+5, 
};

PPA_LINE_BUF ppa_line_buf[121];//ppa line buffer//can divide 3 as (mb_ram d*40 b*40 c*40 + a*1) =121 unit 
DIRECT_MV_REF_BUF col_mb_buf[17][120][68];//col_located MB para [addr_index][mb_x][mb_y]
LEFT_MB leftmb_info;
int qp_y_prev;//6b

void ppa_module (
				 int dct_buf[10], //for dct para
				 int mbc_buf[4], //for mbc para
				 int dbk_buf[8], //for dct para
				 int mca_buf[38], //for dct para
				 int inbuf[36], //parser output. B_MB: 36x32b P_MB:20x32b I_MB: 4*32b
                 //int *col_mb_buf,//col_located MB para
                 int ref_list_buf[25],//refpic list buf
				 char decoder_format,//3b
				 char picwidthinMB,//7b
				 char picheightinMB,//7b
				 int slice_info[40],
				 int error_flag
				 //char MB_info
				 )
{

	int i,j,blk4x4Idx,blk4x4StrIdx,blk8x8Idx;
	
	
	int cur_mb_num;

	//pic slice para
	int slice_type;//2b 
	unsigned int slice_num;//9b
	int constrained_intra_pred_flag;//1b
	int direct_spatial_mv_pred_flag;//1b
	int direct_8x8_inference_flag;//1b
	int slice_qp_y;//6b
	int cb_qp_offset;//5b
	int cr_qp_offset;//5b
	int disable_deblocking_filter_idc;//2b
	int weighted_bipred_idc;//2b
	int curr_POC;//32b
	int addr_index;//5b
	
	int num_ref_idx_l0_active;//5b
	int	list_size[2];//5b//? need 2
	int ref_max;
	int maplist1_to_list0[16];
	int Slice_first_mb_x;//7b
	int Slice_first_mb_y;//7b

	// MB para 
	int mb_type;//3b 
	int cur_mb_x;//7b 
	int cur_mb_y;//7b;	
	int is_skip;//1b; 
	int mb_qp_delta;//6b 
	int transform_size_8x8_flag;//1b 	

	int cbp;//24b; 

	int sub_mb_type[4];//3b
	int intra16x16_luma_pred_mode;//2b
	int intra_chroma_pred_mode;//2b 
	int prev_intra4x4_pred_mode_flag;//16b; 
	int prev_intra8x8_pred_mode_flag;//4b; 
	int rem_intra4x4_pred_mode[16];//3b
	int rem_intra8x8_pred_mode[4];//3b
	int ref_idxl0[4];//5b
	int ref_idxl1[4];//5b
	int mvd_x[2][16];//15b
	int mvd_y[2][16];//13b
	//int mvd_l0_x[16];//15b
	//int mvd_l0_y[16];//13b
	//int mvd_l1_x[16];//15b
	//int mvd_l1_y[16];//13b
	
	// refpic list
	int list0[16];
	int list0_POC[16];
	int list0_longterm;//16b
	int list1[16];
	int list1_POC[16];
	int list1_longterm;//16b

	// col_buf
	char col_ref_idx[4];
	short col_mv[16][2];
	int  col_ref_poc[4];

	//dct
	
    int mb_qp_y;
	int mb_qp_cb;
	int mb_qp_cr;
	int qp_per_y;
	int qp_rem_y;
	int qp_per_cb;
	int qp_rem_cb;
	int qp_per_cr;
	int qp_rem_cr;
	int cbp26;
	char need_y_hadama;
	char is_intra;
	char skip_idct;
	char dct_size;

	//mca
	char predflag[4][2];//[blk][reflist]//1b
	int w[4][2];//weight[blk][reflist]//9b
    char mb_type_mca;//2b
	char sub_mb_type_mca[4];//2b
	char mb_avail_a;//1b
	char mb_avail_b;//1b
	char mb_avail_c;//1b
	char mb_avail_d;//1b
	short mv[2][30][2];//14b/12b
	char ref_idx[2][30];//5b  //2*16
	char direct_blk[4];//1b  //4
	int listx,list_num;
	int width_inb8,height_inb8,b8inblk,b8_x,b8_y;
	int width_inb4,height_inb4,b4_x,b4_y;
	int mv_a_x, mv_b_x, mv_c_x, pred_mv_x[2];
	int mv_a_y, mv_b_y, mv_c_y, pred_mv_y[2];
	int ref_idx_a, ref_idx_b, ref_idx_c;
	int match_cnt;
	int direct_blk_idx;
	int use_mv_pred;
	int map_idx;
	char out_col_ref_idx[4];
	short out_col_mv[16][2];
	
	
    
	//mbc
	char Ipred_avail_a;//1b
	char Ipred_avail_b;//1b
	char Ipred_avail_c;//1b
	char Ipred_avail_d;//1b
	char is_IPCM;//1b
	char Ipred_size;//2b
	//char intra16x16_pred_mode;//2b
    //char intra8x8_pred_mode[4];//4b
	//char intra4x4_pred_mode[16];//4b
    char i4_pred_mode_ref[30];//5*6 grid for save para
	char left_ipred_mode;
	char up_ipred_mode;
	char most_probable_ipred_mode;
	char prev_ipred_flag;
	char pred_mode;
	int b4inblk;
	//char Itop_avail,Ileft_avail;


	    
	

	//dbk
	char skip_deblock;
	char cbp_blk[30];//1b
	char addr_idx[2][30];//5b  //2*16
	char qp_left,qp_top;//6b
	char bs_h[16];//3b
	char bs_v[16];//3b
	int  filter_left_edge;
	int  filter_top_edge;
	//int  filter_internal_edge;
	int  left_mb_intra;
	int  top_mb_intra;
	int  ref0_p,ref1_p,ref0_q,ref1_q;
	int  mv0_p[2],mv1_p[2],mv0_q[2],mv1_q[2];
	int  condition0, condition1, condition2, condition3;



	/*******************************/
	error_flag = FALSE;
	
	
	/**********slice para in*********/
      slice_type= slice_info[0];//2b 
      slice_num= slice_info[1];//9b
      constrained_intra_pred_flag=slice_info[2];//1b
      direct_spatial_mv_pred_flag=slice_info[3];//1b
	  direct_8x8_inference_flag=slice_info[4];//1b
	  slice_qp_y=slice_info[5];//6b
	  cb_qp_offset=slice_info[6];//5b
	  cr_qp_offset=slice_info[7];//5b
	  disable_deblocking_filter_idc=slice_info[8];//2b
	  weighted_bipred_idc=slice_info[9];//2b
	  curr_POC=slice_info[10];//32b
      num_ref_idx_l0_active=slice_info[11];//5b
	  list_size[0]=slice_info[12];//5b//? need 2
      list_size[1]=slice_info[13];//5b//? need 2
	  for(i=0;i<16;i++)
		  maplist1_to_list0[i]=slice_info[14+i];
	  addr_index=slice_info[35];
	  Slice_first_mb_x=slice_info[38];
	  Slice_first_mb_y=slice_info[39];
	  
	  ref_max = mmin(num_ref_idx_l0_active, list_size[0]);

  // MB para in
	  if(decoder_format==STREAM_ID_H264)
	  {
	  
		  mb_type=inbuf[0]&0x7;//3b [2:0]
		  cur_mb_y=(inbuf[0]>>3)&0x3f;//7b [9:3]
		  cur_mb_x=(inbuf[0]>>10)&0x3f;//7b; [16:10]
  		  is_skip=(inbuf[0]>>17)&0x1;//1b; [17]
		  mb_qp_delta=is_skip ? 0 : ((inbuf[0]>>18)&0x3f)|((inbuf[0]>>18)&0x20 ? 0xffffffc0: 0);//6b [23:18]
		  transform_size_8x8_flag=(inbuf[0]>>24)&0x1;//1b [24]
		  
		  cbp=(inbuf[1])&0xffffff;//24b; [23:0]
		  
		  intra_chroma_pred_mode=(inbuf[2]>>16)&0x3;//2b [17:16]
		  prev_intra8x8_pred_mode_flag=(inbuf[2])&0xf;//4b; [3:0]
		  if(transform_size_8x8_flag)
          prev_intra4x4_pred_mode_flag=(prev_intra8x8_pred_mode_flag&0x1)|((prev_intra8x8_pred_mode_flag&0x2)<<3)|((prev_intra8x8_pred_mode_flag&0x4)<<6)|((prev_intra8x8_pred_mode_flag&0x8)<<9);
		  else
		  prev_intra4x4_pred_mode_flag=(inbuf[2])&0xffff;//16b; [15:0]
		  intra16x16_luma_pred_mode=(inbuf[2])&0x3;//2b; [1:0]
		  sub_mb_type[0]  =mb_type ? (inbuf[2])&0x7 : 0x4;//3b [2:0]
		  sub_mb_type[1]  =mb_type ? (inbuf[2]>>3)&0x7 : 0x4;//3b [5:3]
		  sub_mb_type[2]  =mb_type ? (inbuf[2]>>6)&0x7 : 0x4;//3b [8:6]
		  sub_mb_type[3]  =mb_type ? (inbuf[2]>>9)&0x7 : 0x4;//3b [11:9]
		  for(i=0;i<8;i++)
		  {
			  
			  if(i<4)
			  {
				  rem_intra8x8_pred_mode[i]=(inbuf[3]>>(3*i))&0x7;//[11:0]
				  ref_idxl0[i]=(inbuf[2]>>(5*i+12))&0x10 ? -1 : (inbuf[2]>>(5*i+12))&0x1f;//[31:12]
				  ref_idxl1[i]=(inbuf[3]>>(5*i))&0x10 ? -1 : (inbuf[3]>>(5*i))&0x1f;//[19:0]
			  }
              if(transform_size_8x8_flag)
			  {
				  if(i<4)
				  rem_intra4x4_pred_mode[i*4]=rem_intra8x8_pred_mode[i];
              }
			  else
			  {
			  	  rem_intra4x4_pred_mode[i]=(inbuf[3]>>(3*i))&0x7;//[23:0]
			      rem_intra4x4_pred_mode[i+8]=(inbuf[4]>>(3*i))&0x7;//[23:0]
			  }
			  
		  }
  
		  for(i=0;i<16;i++)
		  {
			  mvd_x[0][i]=((inbuf[4+i]>>13)&0x7fff)|((inbuf[4+i]>>27)?0xffff8000 : 0); 
			  mvd_y[0][i]=((inbuf[4+i])&0x1fff)|((inbuf[4+i]>>12)&0x1 ?0xffffe000 : 0);//x:15b y:13b
			  mvd_x[1][i]=((inbuf[20+i]>>13)&0x7fff)|((inbuf[20+i]>>27)?0xffff8000 : 0); 
			  mvd_y[1][i]=((inbuf[20+i])&0x1fff)|((inbuf[20+i]>>12)&0x1 ?0xffffe000 : 0);//inbuf[20]->[35]
		  }

          

		  cur_mb_num=picwidthinMB*cur_mb_y+cur_mb_x;
	  }//if(decoder_format==STREAM_ID_H264)

     if ((cur_mb_x==0)&&(cur_mb_y==0))
	 {
         i=0;
     }




   // refpic list in
	  if(decoder_format==STREAM_ID_H264)
	  {
		  
		  list0_longterm=(ref_list_buf[24])&0xffff;//16b
		  list1_longterm=(ref_list_buf[24]>>16)&0xffff;//16b

		  for(i=0;i<16;i++)
		  {
			  if(i%1)
				 list0[i]=ref_list_buf[i/2]&0xffff;
			  else
				 list0[i]=(ref_list_buf[i/2]>>16)&0xffff;
			  list0_POC[i]=ref_list_buf[i+8];
		  }

		  for(i=0;i<16;i++)
		  {
			  list1[i]=list0[maplist1_to_list0[i]];
			  list1_POC[i]=list0_POC[maplist1_to_list0[i]];
		  }
	  }//if(decoder_format==STREAM_ID_H264)


	    

	  /**********dct para buf generate***********/
	  if(decoder_format==STREAM_ID_H264)
	  {
		  is_IPCM = (mb_type == IPCM_h264);
		  if((cur_mb_x==Slice_first_mb_x)&&(cur_mb_y==Slice_first_mb_y))
		      qp_y_prev =  slice_qp_y ;
		  if(is_IPCM)
			mb_qp_y=0;
		  else
		    mb_qp_y=(qp_y_prev+mb_qp_delta+52)%52;
          
		  if(!is_IPCM&&!is_skip)
             qp_y_prev = mb_qp_y;

	      mb_qp_cb=h264_QP_SCALER_CR_TBL[IClip(0, 51,mb_qp_y+cb_qp_offset)];
	      mb_qp_cr=h264_QP_SCALER_CR_TBL[IClip(0, 51,mb_qp_y+cr_qp_offset)];
		  qp_per_y=h264_qpPerRem_tbl[mb_qp_y][0];
		  qp_rem_y=h264_qpPerRem_tbl[mb_qp_y][1];
		  qp_per_cb=h264_qpPerRem_tbl[mb_qp_cb][0];
		  qp_rem_cb=h264_qpPerRem_tbl[mb_qp_cb][1];
		  qp_per_cr=h264_qpPerRem_tbl[mb_qp_cr][0];
		  qp_rem_cr=h264_qpPerRem_tbl[mb_qp_cr][1];

          is_intra=(mb_type>4);
		  need_y_hadama=(mb_type==I16_h264);

		  dct_size=transform_size_8x8_flag;//0:4x4 1:8x8
		  
		  
		  if (is_IPCM || is_skip)
			  cbp26=0;
		  else if(cbp>>16)
			  cbp26=(1<<24)|cbp;
		  else
			  cbp26=cbp;

		  if((cbp26==0)&&(!is_intra)||is_IPCM)
			  skip_idct=1;
		  else
              skip_idct=0;
		
		  //output buf
		  dct_buf[0]=(is_IPCM<<18)|(skip_idct<<17)|(is_intra<<16)|(dct_size<<15)|(need_y_hadama<<14)|(cur_mb_x<<7)|cur_mb_y;//18b
		  dct_buf[1]=(qp_per_cr<<17)|(qp_rem_cr<<14)|(qp_per_cb<<10)|(qp_rem_cb<<7)|(qp_per_y<<3)|qp_rem_y;//21b
		  dct_buf[2]=cbp26;//26b	  
	  }

      /**********mca para buf generate***********/

	  
	  

	  // col buff in
	  if(decoder_format==STREAM_ID_H264)
	  {
		  //if(slice_type==B_slice)
		  if((slice_type==B_slice)&&((mb_type==skip_direct_h264)||(sub_mb_type[0]==direct_8X8)||(sub_mb_type[1]==direct_8X8)||(sub_mb_type[2]==direct_8X8)||(sub_mb_type[3]==direct_8X8)))
		  {
			  int address_idx;
              address_idx=g_list0_map_addr[maplist1_to_list0[0]];
			  if(!direct_spatial_mv_pred_flag)
			  {
			  	  col_ref_poc[0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg0.h264.blk0_ref_poc;
			      col_ref_poc[1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg1.h264.blk1_ref_poc;
				  col_ref_poc[2]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg2.h264.blk2_ref_poc;
				  col_ref_poc[3]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg3.h264.blk3_ref_poc;
			  }
			  col_ref_idx[0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg4.h264.blk0_ref_idx;
			  col_ref_idx[1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg5.h264.blk1_ref_idx;
			  col_ref_idx[2]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg6.h264.blk2_ref_idx;
			  col_ref_idx[3]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg7.h264.blk3_ref_idx;
			  
			  if(!direct_8x8_inference_flag)
			  {
				  col_mv[0][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg4.h264.mv_0_x;
				  col_mv[0][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg4.h264.mv_0_y;
				  col_mv[5][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg5.h264.mv_5_x;
				  col_mv[5][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg5.h264.mv_5_y;
				  col_mv[10][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg6.h264.mv_10_x;
				  col_mv[10][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg6.h264.mv_10_y;
				  col_mv[15][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg7.h264.mv_15_x;
				  col_mv[15][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg7.h264.mv_15_y;
				  col_mv[4][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg8.h264.mv_4_x;
				  col_mv[4][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg8.h264.mv_4_y;
				  col_mv[1][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg9.h264.mv_1_x;
				  col_mv[1][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg9.h264.mv_1_y;
				  col_mv[6][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg10.h264.mv_6_x;
				  col_mv[6][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg10.h264.mv_6_y;
				  col_mv[7][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg11.h264.mv_7_x;
				  col_mv[7][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg11.h264.mv_7_y;
				  col_mv[8][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg12.h264.mv_8_x;
				  col_mv[8][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg12.h264.mv_8_y;
				  col_mv[9][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg13.h264.mv_9_x;
				  col_mv[9][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg13.h264.mv_9_y;
				  col_mv[2][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg14.h264.mv_2_x;
				  col_mv[2][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg14.h264.mv_2_y;
				  col_mv[11][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg15.h264.mv_11_x;
				  col_mv[11][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg15.h264.mv_11_y;
				  col_mv[12][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg16.h264.mv_12_x;
				  col_mv[12][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg16.h264.mv_12_y;
				  col_mv[13][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg17.h264.mv_13_x;
				  col_mv[13][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg17.h264.mv_13_y;
				  col_mv[14][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg18.h264.mv_14_x;
				  col_mv[14][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg18.h264.mv_14_y;
				  col_mv[3][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg19.h264.mv_3_x;
				  col_mv[3][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg19.h264.mv_3_y;
			  }
			  else
			  {
                  col_mv[0][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg4.h264.mv_0_x;
				  col_mv[0][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg4.h264.mv_0_y;
				  col_mv[5][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg5.h264.mv_5_x;
				  col_mv[5][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg5.h264.mv_5_y;
				  col_mv[10][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg6.h264.mv_10_x;
				  col_mv[10][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg6.h264.mv_10_y;
				  col_mv[15][0]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg7.h264.mv_15_x;
				  col_mv[15][1]=col_mb_buf[address_idx][cur_mb_x][cur_mb_y].reg7.h264.mv_15_y;
				  
			  }
			  
		  }
		  
	  }
	  

	  if(decoder_format==STREAM_ID_H264)
	  {

		  if(slice_type==B_slice)
		  {
			  direct_blk[0]=(mb_type==skip_direct_h264)||(sub_mb_type[0]==direct_8X8)&&(!is_intra);
			  direct_blk[1]=(mb_type==skip_direct_h264)||(sub_mb_type[1]==direct_8X8)&&(!is_intra);
			  direct_blk[2]=(mb_type==skip_direct_h264)||(sub_mb_type[2]==direct_8X8)&&(!is_intra);
			  direct_blk[3]=(mb_type==skip_direct_h264)||(sub_mb_type[3]==direct_8X8)&&(!is_intra);
		  }
		  else if((slice_type==P_slice)&&is_skip)
		  {
			  direct_blk[0]=direct_blk[1]=direct_blk[2]=direct_blk[3]=1;
		  }
		  else
		  {
              direct_blk[0]=direct_blk[1]=direct_blk[2]=direct_blk[3]=0;
		  }

		  mb_avail_a = (cur_mb_x > 0) && (slice_num == leftmb_info.reg0.h264.slice_nr); //left
		  mb_avail_b = (cur_mb_y > 0) && (slice_num == ppa_line_buf[cur_mb_x].reg0.h264.slice_nr);// (mb_info_ptr - frame_width_in_mbs)->slice_nr //top
		  mb_avail_c = (cur_mb_y > 0) && (cur_mb_x < (picwidthinMB -1)) && (slice_num == ppa_line_buf[cur_mb_x+1].reg0.h264.slice_nr);//(mb_info_ptr - frame_width_in_mbs+1)->slice_nr //top right
		  mb_avail_d = (cur_mb_x > 0) && (cur_mb_y > 0) && (slice_num  == ppa_line_buf[cur_mb_x-1].reg0.h264.slice_nr);//(mb_info_ptr - frame_width_in_mbs-1)->slice_nr); //top left
		  
		

		  if(!mb_avail_b||(cur_mb_y > 0) && ppa_line_buf[cur_mb_x].reg0.h264.is_intra)
		  {
			  mv[0][1][0]=mv[0][2][0]=mv[0][3][0]=mv[0][4][0]=0;
			  mv[0][1][1]=mv[0][2][1]=mv[0][3][1]=mv[0][4][1]=0;
			  mv[1][1][0]=mv[1][2][0]=mv[1][3][0]=mv[1][4][0]=0;
			  mv[1][1][1]=mv[1][2][1]=mv[1][3][1]=mv[1][4][1]=0;
			  if (!mb_avail_b) 
			  {
				  ref_idx[0][1]=ref_idx[0][2]=ref_idx[0][3]=ref_idx[0][4]=-2;
				  ref_idx[1][1]=ref_idx[1][2]=ref_idx[1][3]=ref_idx[1][4]=-2;
			  }
			  else
			  {
			  	  ref_idx[0][1]=ref_idx[0][2]=ref_idx[0][3]=ref_idx[0][4]=-1;
			  	  ref_idx[1][1]=ref_idx[1][2]=ref_idx[1][3]=ref_idx[1][4]=-1;
			  }
		  }
		  else 
		  {
			  mv[0][1][0]= ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_x;
			  mv[0][1][1]= ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_y;
			  mv[0][2][0]= ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_x;
			  mv[0][2][1]= ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_y;
			  mv[0][3][0]= ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_x;
			  mv[0][3][1]= ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_y;
			  mv[0][4][0]= ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_x;
			  mv[0][4][1]= ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_y;
			  mv[1][1][0]= ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_x;
			  mv[1][1][1]= ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_y;
			  mv[1][2][0]= ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_x;
			  mv[1][2][1]= ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_y;
			  mv[1][3][0]= ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_x;
			  mv[1][3][1]= ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_y;
			  mv[1][4][0]= ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_x;
			  mv[1][4][1]= ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_y;
			  

			  ref_idx[0][1]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l0;
			  ref_idx[0][2]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l0;
			  ref_idx[1][1]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l1;
			  ref_idx[1][2]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l1;
			  ref_idx[0][3]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l0;
			  ref_idx[0][4]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l0;
			  ref_idx[1][3]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l1;
			  ref_idx[1][4]= ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l1;
		  }

		  if(!mb_avail_a||(cur_mb_x > 0) && leftmb_info.reg0.h264.is_intra)
		  {
			  mv[0][6][0]=mv[0][12][0]=mv[0][18][0]=mv[0][24][0]=0;
			  mv[0][6][1]=mv[0][12][1]=mv[0][18][1]=mv[0][24][1]=0;
			  mv[1][6][0]=mv[1][12][0]=mv[1][18][0]=mv[1][24][0]=0;
			  mv[1][6][1]=mv[1][12][1]=mv[1][18][1]=mv[1][24][1]=0;
			  if (!mb_avail_a) 
			  {
				  ref_idx[0][6]=ref_idx[0][12]=ref_idx[0][18]=ref_idx[0][24]=-2;
				  ref_idx[1][6]=ref_idx[1][12]=ref_idx[1][18]=ref_idx[1][24]=-2;
			  }
			  else
			  {
			  	  ref_idx[0][6]=ref_idx[0][12]=ref_idx[0][18]=ref_idx[0][24]=-1;
			  	  ref_idx[1][6]=ref_idx[1][12]=ref_idx[1][18]=ref_idx[1][24]=-1;
			  }
		  }
		  else 
		  {
			  mv[0][6][0]= leftmb_info.reg2.h264.mv_l0_5_x;
			  mv[0][6][1]= leftmb_info.reg2.h264.mv_l0_5_y;
			  mv[0][12][0]= leftmb_info.reg3.h264.mv_l0_7_x;
			  mv[0][12][1]= leftmb_info.reg3.h264.mv_l0_7_y;
			  mv[0][18][0]= leftmb_info.reg4.h264.mv_l0_13_x;
			  mv[0][18][1]= leftmb_info.reg4.h264.mv_l0_13_y;
			  mv[0][24][0]= leftmb_info.reg5.h264.mv_l0_15_x;
			  mv[0][24][1]= leftmb_info.reg5.h264.mv_l0_15_y;
			  mv[1][6][0]= leftmb_info.reg6.h264.mv_l1_5_x;
			  mv[1][6][1]= leftmb_info.reg6.h264.mv_l1_5_y;
			  mv[1][12][0]= leftmb_info.reg7.h264.mv_l1_7_x;
			  mv[1][12][1]= leftmb_info.reg7.h264.mv_l1_7_y;
			  mv[1][18][0]= leftmb_info.reg8.h264.mv_l1_13_x;
			  mv[1][18][1]= leftmb_info.reg8.h264.mv_l1_13_y;
			  mv[1][24][0]= leftmb_info.reg9.h264.mv_l1_15_x;
			  mv[1][24][1]= leftmb_info.reg9.h264.mv_l1_15_y;
			  
			  ref_idx[0][6]= leftmb_info.reg1.h264_inter.ref_idx_1_l0;
			  ref_idx[0][12]= leftmb_info.reg1.h264_inter.ref_idx_1_l0;
			  ref_idx[1][6]= leftmb_info.reg1.h264_inter.ref_idx_1_l1;
			  ref_idx[1][12]= leftmb_info.reg1.h264_inter.ref_idx_1_l1;
			  ref_idx[0][18]= leftmb_info.reg1.h264_inter.ref_idx_3_l0;
			  ref_idx[0][24]= leftmb_info.reg1.h264_inter.ref_idx_3_l0;
			  ref_idx[1][18]= leftmb_info.reg1.h264_inter.ref_idx_3_l1;
			  ref_idx[1][24]= leftmb_info.reg1.h264_inter.ref_idx_3_l1;
		  }
		 
		  if(!mb_avail_c||(cur_mb_y > 0) && (cur_mb_x < (picwidthinMB -1)) && ppa_line_buf[cur_mb_x+1].reg0.h264.is_intra)
		  {
			  mv[0][5][0]=mv[0][5][1]=0;
			  mv[1][5][0]=mv[1][5][1]=0;
			  if(!mb_avail_c)
			  {
				  ref_idx[0][5]=-2;
			      ref_idx[1][5]=-2;
			  }
			  else
			  {
				  ref_idx[0][5]=-1;
				  ref_idx[1][5]=-1;
			  }
		  }
		  else 
		  {
			  mv[0][5][0]= ppa_line_buf[cur_mb_x+1].reg2.h264.mv_l0_10_x;
			  mv[0][5][1]= ppa_line_buf[cur_mb_x+1].reg2.h264.mv_l0_10_y;
			  mv[1][5][0]= ppa_line_buf[cur_mb_x+1].reg6.h264.mv_l1_10_x;
			  mv[1][5][1]= ppa_line_buf[cur_mb_x+1].reg6.h264.mv_l1_10_y;
			  ref_idx[0][5]= ppa_line_buf[cur_mb_x+1].reg1.h264_inter.ref_idx_2_l0;
			  ref_idx[1][5]= ppa_line_buf[cur_mb_x+1].reg1.h264_inter.ref_idx_2_l1;

		  }

		  if(!mb_avail_d||(cur_mb_x > 0) && (cur_mb_y > 0) && ppa_line_buf[cur_mb_x-1].reg0.h264.is_intra)
		  {
			  mv[0][0][0]=mv[0][0][1]=0;
			  mv[1][0][0]=mv[1][0][1]=0;
			  ref_idx[0][0]=-2;
			  ref_idx[1][0]=-2;//-1
		  }
		  else 
		  {
			  mv[0][0][0]= ppa_line_buf[cur_mb_x-1].reg5.h264.mv_l0_15_x;
			  mv[0][0][1]= ppa_line_buf[cur_mb_x-1].reg5.h264.mv_l0_15_y;
			  mv[1][0][0]= ppa_line_buf[cur_mb_x-1].reg9.h264.mv_l1_15_x;
			  mv[1][0][1]= ppa_line_buf[cur_mb_x-1].reg9.h264.mv_l1_15_y;
			  ref_idx[0][0]= ppa_line_buf[cur_mb_x-1].reg1.h264_inter.ref_idx_3_l0;
			  ref_idx[1][0]= ppa_line_buf[cur_mb_x-1].reg1.h264_inter.ref_idx_3_l1;
			  
		  }

		  for(i=0;i<16;i++)
		  {
               ref_idx[0][h264_blk_order_map[i]]=-2;//ref_idxl0[i/4];
               ref_idx[1][h264_blk_order_map[i]]=-2;//ref_idxl1[i/4];
		  }
		  ref_idx[0][11]=ref_idx[0][17]=ref_idx[0][23]=ref_idx[0][29]=-2;
		  ref_idx[1][11]=ref_idx[1][17]=ref_idx[1][23]=ref_idx[1][29]=-2;
		  mv[0][11][0]=mv[0][17][0]=mv[0][23][0]=mv[0][29][0]=0;
		  mv[0][11][1]=mv[0][17][1]=mv[0][23][1]=mv[0][29][1]=0;
		  mv[1][11][0]=mv[1][17][0]=mv[1][23][0]=mv[1][29][0]=0;
		  mv[1][11][1]=mv[1][17][1]=mv[1][23][1]=mv[1][29][1]=0;
          
		  if(!is_intra)
		  {
			  if(mb_type==PB16x16_h264)
			  {
				  b8inblk=4;     width_inb8=2;   height_inb8=2;       			  
			  }
			  else if(mb_type==PB16x8_h264)
			  {
                  b8inblk=2;     width_inb8=2;   height_inb8=1;
			  }
			  else if(mb_type==PB8x16_h264)
			  {
                  b8inblk=1;     width_inb8=1;   height_inb8=2; //b8inblk=2;
			  }
			  else 
			  {
                  b8inblk=1;     width_inb8=1;   height_inb8=1;
			  }
		  
			  if(slice_type==B_slice)
				  list_num=2;
			  else
				  list_num=1;

			  
			  
			  blk4x4Idx=0;
			  blk8x8Idx=0;
			  //for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx+=b8inblk)
			  for (b8_y = 0; b8_y < 2; b8_y+=1)
			  {
				  for (b8_x = 0; b8_x < 2; b8_x+=1)
				  {
					  
					  for (listx = 0; listx < list_num; listx++)
					  {
                          blk4x4Idx=blk8x8Idx*4;
						  if((b8_y%height_inb8==0)&&(b8_x%width_inb8==0))
						  {
							  
							  if((mb_type==skip_direct_h264)||(sub_mb_type[blk8x8Idx]==direct_8X8))
							  {
								  
								  if(mb_type==skip_direct_h264) //P_SLICE skip
								  {
									width_inb4=4;  height_inb4=4;  b4inblk=16;
								  }
								  else
								  {
									  width_inb4=4; //direct_spatial_mv_pred_flag
									  //width_inb4=2; 
									  height_inb4=2;  b4inblk=4;
								  }
							  }
							  else if(mb_type!=PB8x8_h264)
							  {
								 height_inb4=2*height_inb8; width_inb4=2*width_inb8; b4inblk=4*b8inblk;
							  }
							  else
							  {
								  if((sub_mb_type[blk8x8Idx]==SUB_8X8))
								  {
									  height_inb4=2; width_inb4=2; b4inblk=4;
								  }
								  else if(sub_mb_type[blk8x8Idx]==SUB_8X4)
								  {
									  height_inb4=1; width_inb4=2; b4inblk=2;
								  }
								  else if(sub_mb_type[blk8x8Idx]==SUB_4X8)
								  {
									  height_inb4=2; width_inb4=1; b4inblk=1;//2;
								  }
								  else //if(sub_mb_type[blk4x4Idx/4]==SUB_4X4)
								  {
									  height_inb4=1; width_inb4=1; b4inblk=1;
								  }

							  }
						  }

						  for (b4_y = 0; b4_y < 2; b4_y+=1)
						  {
							  for (b4_x = 0; b4_x < 2; b4_x+=1)
							  {
								  int b4StrIdx;

                                  use_mv_pred=1;
								  blk4x4StrIdx = h264_blk_order_map[blk4x4Idx];
								  if(((b8_y*2+b4_y)%height_inb4)&&(height_inb4>width_inb4))//for 8x16 4*8
									  b4StrIdx=blk4x4StrIdx-6*((b8_y*2+b4_y)%height_inb4);
								  else
								      b4StrIdx=blk4x4StrIdx;

								  if(direct_blk[blk8x8Idx])//(sub_mb_type[blk8x8Idx]==direct_8X8)||(mb_type==skip_direct_h264))
								  {
									  if(slice_type==B_slice)
									  {
									  
										  if(direct_8x8_inference_flag)
										  {
											  direct_blk_idx=blk8x8Idx*5;
										  }
										  else
										  {
											  direct_blk_idx=blk4x4Idx;
										  }
										  
										  if(direct_spatial_mv_pred_flag)
										  {
											  int tmp,colzeroflag;

											  b4StrIdx=7;
											  if (mb_avail_c)
											  {
												  ref_idx_c = ref_idx[listx][5];
											  }else
											  {
												  ref_idx_c = ref_idx[listx][0];
											  }
									
											  ref_idx_a = ref_idx[listx][6];
											  ref_idx_b = ref_idx[listx][1];
											  tmp = (ref_idx_a >= 0 && ref_idx_b >= 0) ? mmin(ref_idx_a,ref_idx_b): mmax(ref_idx_a,ref_idx_b);
											  ref_idx[listx][blk4x4StrIdx] = (tmp >= 0 && ref_idx_c >= 0) ? mmin(tmp,ref_idx_c): mmax(tmp,ref_idx_c);
											  colzeroflag=((abs(col_mv[direct_blk_idx][0])>>1)==0)&&((abs(col_mv[direct_blk_idx][1])>>1)==0)&&(col_ref_idx[blk8x8Idx]==0)&&!((list0_longterm>>maplist1_to_list0[0])&0x1);
											  if((ref_idx[listx][blk4x4StrIdx]<0)||(ref_idx[listx][blk4x4StrIdx]==0)&&(colzeroflag))
											  {
												  mv[listx][blk4x4StrIdx][0]=mv[listx][blk4x4StrIdx][1]=0;
												  use_mv_pred=0;												  
											  }
											  
											  if((listx==1)&&(ref_idx[0][blk4x4StrIdx]<0)&&(ref_idx[1][blk4x4StrIdx]<0))
											  {           
											         ref_idx[0][blk4x4StrIdx]=ref_idx[1][blk4x4StrIdx]=0;
											  }
										  }
										  else
										  {   
											  int32 prescale, iTRb, iTRp,mvscale,tmp1,tmp2;
											  
											  use_mv_pred=0;
											  if (listx==0) 
											  {
											  
												  ref_idx[1][blk4x4StrIdx]=0;
												  if(col_ref_idx[blk8x8Idx]<0)
												  {
													  ref_idx[0][blk4x4StrIdx]=0;
												  }
												  else
												  {
													  map_idx=0;
													  while ((col_ref_poc[blk8x8Idx]!=list0_POC[map_idx])&&(map_idx<ref_max))
													  {
														  map_idx+=1;
													  }
													  if (map_idx==ref_max) 
													  {
														  error_flag = TRUE;
														  //map_idx=0;//do not process error
														  //return;
													  }
													  ref_idx[0][blk4x4StrIdx]=map_idx;
												  }

                                                  iTRp = Clip3( -128, 127,  (list0_POC[maplist1_to_list0[0]] - col_ref_poc[blk8x8Idx]));

												  if ((iTRp == 0) || ((list0_longterm>>map_idx)&0x1))
												  {
													  mv[0][blk4x4StrIdx][0]= col_mv[direct_blk_idx][0];
													  mv[0][blk4x4StrIdx][1]= col_mv[direct_blk_idx][1];
													  mv[1][blk4x4StrIdx][0]= mv[1][blk4x4StrIdx][1]=0;
												  }
												  else
												  {
													  
													  
													  iTRb = Clip3( -128, 127, (curr_POC - col_ref_poc[blk8x8Idx] ));
													  
													  
													  prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;//prescale[iTRp]

													  mvscale = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;

													  tmp1 = (mvscale * col_mv[direct_blk_idx][0] + 128) >> 8;
													  tmp2 = (mvscale * col_mv[direct_blk_idx][1] + 128) >> 8;						
													  mv[0][blk4x4StrIdx][0]=tmp1;
													  mv[0][blk4x4StrIdx][1]=tmp2;
													  mv[1][blk4x4StrIdx][0] = tmp1 - col_mv[direct_blk_idx][0];
													  mv[1][blk4x4StrIdx][1] = tmp2 - col_mv[direct_blk_idx][1];													 

												  }
											  }//if (listx==1)
										  }
									  }
									  else//if(slice_type==P_slice)//listx==0
									  {
										  ref_idx[0][blk4x4StrIdx]=0;
										  ref_idx[1][blk4x4StrIdx]=-1;
										  if (!mb_avail_a||!mb_avail_b||(ref_idx[listx][6]==0)&&(mv[listx][6][0]==0)&&(mv[listx][6][1]==0)||(ref_idx[listx][1]==0)&&(mv[listx][1][0]==0)&&(mv[listx][1][1]==0))
										  {   
											  mv[listx][blk4x4StrIdx][0]=mv[listx][blk4x4StrIdx][1]=0;
											  use_mv_pred=0;
										  }
										 
									  }
								  }
								  else
								  {
									   ref_idx[0][blk4x4StrIdx]=ref_idxl0[blk8x8Idx];
									   ref_idx[1][blk4x4StrIdx]=ref_idxl1[blk8x8Idx];
								  }

								  
                                       
								  if((blk4x4Idx%b4inblk==0)&&(direct_spatial_mv_pred_flag||(slice_type==P_slice)||!direct_blk[blk8x8Idx]))//((b4_y%height_inb4==0)&&(b4_x%width_inb4==0)||direct_blk[blk8x8Idx]&&(slice_type==B_slice))&&use_mv_pred)
								  {
									  //if(!direct_spatial_mv_pred_flag)
									  {
										  //ref_idx_c = ref_idx[blk4x4StrIdx-6+width_inb4];
										  if (ref_idx[listx][b4StrIdx-6+width_inb4] == -2)
										  {
											  ref_idx_c = ref_idx[listx][b4StrIdx-7];
											  mv_c_x = mv[listx][b4StrIdx-7][0];
											  mv_c_y = mv[listx][b4StrIdx-7][1];
										  }else 
										  {
											  ref_idx_c =ref_idx[listx][b4StrIdx-6+width_inb4];
											  mv_c_x = mv[listx][b4StrIdx-6+width_inb4][0];
											  mv_c_y = mv[listx][b4StrIdx-6+width_inb4][1];
										  }
										  
										  //left block ref and mv
										  ref_idx_a = ref_idx[listx][b4StrIdx-1];
										  mv_a_x = mv[listx][b4StrIdx-1][0];
										  mv_a_y = mv[listx][b4StrIdx-1][1];
										  //top block ref and mv
										  ref_idx_b = ref_idx[listx][b4StrIdx-6];
										  mv_b_x = mv[listx][b4StrIdx-6][0];
										  mv_b_y = mv[listx][b4StrIdx-6][1];
									  }
								  
									  if ((mb_type==PB16x8_h264)&&(b8_y==0) &&(ref_idx_b==ref_idx[listx][blk4x4StrIdx]))
									  {
										  pred_mv_x[listx]=mv_b_x;//mv[listx][b4StrIdx-6][0];
										  pred_mv_y[listx]=mv_b_y;//mv[listx][b4StrIdx-6][1];
									  }
									  else if ((mb_type==PB16x8_h264)&&(b8_y!=0) &&(ref_idx_a==ref_idx[listx][blk4x4StrIdx]))
									  {
										  pred_mv_x[listx]=mv_a_x;//mv[listx][b4StrIdx-1][0];
										  pred_mv_y[listx]=mv_a_y;//mv[listx][b4StrIdx-1][1];
									  }
									  else if((mb_type==PB8x16_h264)&&(b8_x==0) &&(ref_idx_a==ref_idx[listx][blk4x4StrIdx]))
									  {
										  pred_mv_x[listx]=mv_a_x;//mv[listx][b4StrIdx-1][0];
										  pred_mv_y[listx]=mv_a_y;//mv[listx][b4StrIdx-1][1];
									  }
									  else if ((mb_type==PB8x16_h264)&&(b8_x!=0) &&(ref_idx_c==ref_idx[listx][blk4x4StrIdx]))
									  {
										  pred_mv_x[listx]=mv_c_x;//mv[listx][b4StrIdx-6+2][0];
										  pred_mv_y[listx]=mv_c_y;//mv[listx][b4StrIdx-6+2][1];
									  }
									  else
									  {
										  match_cnt = ((ref_idx[listx][blk4x4StrIdx] == ref_idx_a) + (ref_idx[listx][blk4x4StrIdx]  == ref_idx_b) + (ref_idx[listx][blk4x4StrIdx]  == ref_idx_c));
										  
										  if((match_cnt == 0)&&(ref_idx_b == -2) && (ref_idx_c == -2) && (ref_idx_a != -2))
										  {
												  pred_mv_x[listx] = mv_a_x;
												  pred_mv_y[listx] = mv_a_y;
										  }
										  else if(match_cnt == 1)
										  {
											  if (ref_idx[listx][blk4x4StrIdx] == ref_idx_a)
											  {
												  pred_mv_x[listx] = mv_a_x;
												  pred_mv_y[listx] = mv_a_y;
											  }else if (ref_idx[listx][blk4x4StrIdx] == ref_idx_b)
											  {
												  pred_mv_x[listx] = mv_b_x;
												  pred_mv_y[listx] = mv_b_y;
											  }else // (ref_idx[listx][blk4x4StrIdx] == ref_idx_c)
											  {
												  pred_mv_x[listx] = mv_c_x;
												  pred_mv_y[listx] = mv_c_y;
											  }
										  }
										  else //(match_cnt > 1)
										  {
											  pred_mv_x[listx] = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
											  pred_mv_y[listx] = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
										  }
										  
										 
									  }
								  }//if((b4_y%height_inb4==0)&&(b4_x%width_inb4==0))
								
								  if(use_mv_pred)
								  {
									  if(ref_idx[listx][blk4x4StrIdx]>=0)
									  {									  
										  mv[listx][blk4x4StrIdx][0]=direct_blk[blk8x8Idx] ? pred_mv_x[listx] : pred_mv_x[listx]+mvd_x[listx][blk4x4Idx];
										  mv[listx][blk4x4StrIdx][1]=direct_blk[blk8x8Idx] ? pred_mv_y[listx] : pred_mv_y[listx]+mvd_y[listx][blk4x4Idx];
									  }
									  else
									  {
                                          mv[listx][blk4x4StrIdx][0]=mv[listx][blk4x4StrIdx][1]=0;
									  }
								  }

								  blk4x4Idx+=1;//b4inblk;
							  }//for (b4_x = 0; b4_x < 2; b4_x+=width_inb4)
						  }//for (b4_y = 0; b4_y < 2; b4_y+=height_inb4)
						  
					  }//for (listx = 0; listx < 2; listx++)

					  blk4x4StrIdx = h264_blk_order_map[blk8x8Idx*4];
					  if (ref_idx[0][blk4x4StrIdx]>=0)
					  {
					  	  predflag[blk8x8Idx][0]=1;
						  addr_idx[0][blk4x4StrIdx]=g_list0_map_addr[ref_idx[0][blk4x4StrIdx]];
					  }
					  else
					  {
						  predflag[blk8x8Idx][0]=0;
						  addr_idx[0][blk4x4StrIdx]=0x1f;
					  }
					 
					  if((slice_type==B_slice)&&(ref_idx[1][blk4x4StrIdx]>=0))
					  {
					       predflag[blk8x8Idx][1]=1;
						   addr_idx[1][blk4x4StrIdx]=g_list0_map_addr[maplist1_to_list0[ref_idx[1][blk4x4StrIdx]]];
					  }
					  else
					  {
						  predflag[blk8x8Idx][1]=0;
						  addr_idx[1][blk4x4StrIdx]=0x1f;
					  }

					  if ((slice_type==B_slice)&&(weighted_bipred_idc==0x2))
					  {   
						  if(predflag[blk8x8Idx][0]&&predflag[blk8x8Idx][1]) 
						  {
							  int32 prescale, iTRb, iTRp,mvscale,mvscale1,tmp1,tmp2;
							  tmp1=	ref_idx[0][blk4x4StrIdx];
							  tmp2= maplist1_to_list0[ref_idx[1][blk4x4StrIdx]];
							  iTRp = Clip3( -128, 127,  (list0_POC[tmp2] - list0_POC[tmp1]));
							  
							  if((iTRp==0)||((list0_longterm>>tmp1)&0x1)||((list0_longterm>>tmp2)&0x1))
							  {
								  w[blk8x8Idx][0]=w[blk8x8Idx][1]=32;
							  }
							  else
							  {
								  iTRb = Clip3( -128, 127, (curr_POC - list0_POC[tmp1] ));
								  
						  		  prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;//prescale[iTRp]//16b*256
						  		  mvscale = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 );//11b//1b sign + 15b*8b unsigned mul
								  mvscale1 = mvscale>>2 ;
								  if ((mvscale1<-64)||(mvscale1>128)) 
								  {
									  w[blk8x8Idx][0]=w[blk8x8Idx][1]=32;
								  }
								  else
								  {
									  w[blk8x8Idx][0]=64-mvscale1;
									  w[blk8x8Idx][1]=mvscale1;
								  }
							  }
						
						  }
						  else
						  {
							  w[blk8x8Idx][0]= predflag[blk8x8Idx][0] ? 32 : 0;
							  w[blk8x8Idx][1]= predflag[blk8x8Idx][1] ? 32 : 0;
						  }
					  }
					  else
					  {
						  w[blk8x8Idx][0]=w[blk8x8Idx][1]=0;
					  }
					  
					  
					  blk8x8Idx+=1;//b8inblk;

				  }// for (b8_x = 0; b8_x < 2; b8_x+=width_inb8)
			  }//for (b8_y = 0; b8_y < 2; b8_y+=1)
			  
		  }//if(!is_intra)
		  

          if((cur_mb_x != 0)&& !leftmb_info.reg0.h264.is_intra)
		  {
			  
			  //mv[0][29][0]= leftmb_info.reg5.h264.mv_l0_15_x;
			  //mv[0][29][1]= leftmb_info.reg5.h264.mv_l0_15_y;
			  //mv[1][29][0]= leftmb_info.reg9.h264.mv_l1_15_x;
			  //mv[1][29][1]= leftmb_info.reg9.h264.mv_l1_15_y;
			  //ref_idx[0][29]= leftmb_info.reg1.h264_inter.ref_idx_3_l0;
			  //ref_idx[1][29]= leftmb_info.reg1.h264_inter.ref_idx_3_l1;
			  
			  ppa_line_buf[cur_mb_x-1].reg1.h264_inter.ref_idx_3_l0 = leftmb_info.reg1.h264_inter.ref_idx_3_l0;
			  ppa_line_buf[cur_mb_x-1].reg1.h264_inter.ref_idx_3_l1 = leftmb_info.reg1.h264_inter.ref_idx_3_l1;
			  ppa_line_buf[cur_mb_x-1].reg5.h264.mv_l0_15_x =leftmb_info.reg5.h264.mv_l0_15_x;
			  ppa_line_buf[cur_mb_x-1].reg5.h264.mv_l0_15_y =leftmb_info.reg5.h264.mv_l0_15_y;
			  ppa_line_buf[cur_mb_x-1].reg9.h264.mv_l1_15_x =leftmb_info.reg9.h264.mv_l1_15_x;
			  ppa_line_buf[cur_mb_x-1].reg9.h264.mv_l1_15_y =leftmb_info.reg9.h264.mv_l1_15_y;

			 
		  }
		  if((cur_mb_x != 0)&&leftmb_info.reg0.h264.is_intra)
		  {
			  //i4_pred_mode_ref[29]= leftmb_info.reg1.h264_intra.intra4x4_15_mode;
			  //ppa_line_buf[cur_mb_x-1].reg1.h264_intra.intra4x4_15_mode=i4_pred_mode_ref[29];
			  ppa_line_buf[cur_mb_x-1].reg1.h264_intra.intra4x4_14_mode=leftmb_info.reg1.h264_intra.intra4x4_14_mode;
			  ppa_line_buf[cur_mb_x-1].reg1.h264_intra.intra4x4_15_mode=leftmb_info.reg1.h264_intra.intra4x4_15_mode;
		  }

         
		  

		  if(!is_intra)
		  {
			  leftmb_info.reg1.h264_inter.ref_idx_1_l0= ref_idx[0][10];
			  leftmb_info.reg1.h264_inter.ref_idx_3_l0= ref_idx[0][22];			  
			  leftmb_info.reg2.h264.mv_l0_5_x =mv[0][10][0];
			  leftmb_info.reg2.h264.mv_l0_5_y =mv[0][10][1];
			  leftmb_info.reg3.h264.mv_l0_7_x =mv[0][16][0];
			  leftmb_info.reg3.h264.mv_l0_7_y =mv[0][16][1];
			  leftmb_info.reg4.h264.mv_l0_13_x =mv[0][22][0];
			  leftmb_info.reg4.h264.mv_l0_13_y =mv[0][22][1];
			  leftmb_info.reg5.h264.mv_l0_15_x =mv[0][28][0];
			  leftmb_info.reg5.h264.mv_l0_15_y =mv[0][28][1];
			  
			  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l0 = ref_idx[0][25];
			  ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_x =mv[0][25][0];
			  ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_y =mv[0][25][1];
			  ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_x =mv[0][26][0];
			  ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_y =mv[0][26][1];
			  ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_x =mv[0][27][0];
			  ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_y =mv[0][27][1];
			  if(cur_mb_x == picwidthinMB-1)
			  {
				  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l0 = ref_idx[0][28];
				  ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_x =mv[0][28][0];
				  ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_y =mv[0][28][1];
			  }
			  
			  if (slice_type==B_slice)
			  {
				  leftmb_info.reg1.h264_inter.ref_idx_1_l1= ref_idx[1][10];
				  leftmb_info.reg1.h264_inter.ref_idx_3_l1= ref_idx[1][22];	
				  leftmb_info.reg6.h264.mv_l1_5_x =mv[1][10][0];
				  leftmb_info.reg6.h264.mv_l1_5_y =mv[1][10][1];
				  leftmb_info.reg7.h264.mv_l1_7_x =mv[1][16][0];
				  leftmb_info.reg7.h264.mv_l1_7_y =mv[1][16][1];
				  leftmb_info.reg8.h264.mv_l1_13_x =mv[1][22][0];
				  leftmb_info.reg8.h264.mv_l1_13_y =mv[1][22][1];
				  leftmb_info.reg9.h264.mv_l1_15_x =mv[1][28][0];
				  leftmb_info.reg9.h264.mv_l1_15_y =mv[1][28][1];
				  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l1 = ref_idx[1][25];
				  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_x = mv[1][25][0];
				  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_y = mv[1][25][1];
				  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_x = mv[1][26][0];
				  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_y = mv[1][26][1];
				  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_x = mv[1][27][0];
				  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_y = mv[1][27][1];
			      if(cur_mb_x == picwidthinMB-1)
				  {
                     ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l1 = ref_idx[1][28];
					 ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_x = mv[1][28][0];
					 ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_y = mv[1][28][1];
				  }
			  }
			  else
			  {
				  leftmb_info.reg1.h264_inter.ref_idx_1_l1= -1;
				  leftmb_info.reg1.h264_inter.ref_idx_3_l1= -1;	
				  leftmb_info.reg6.h264.mv_l1_5_x =0;
				  leftmb_info.reg6.h264.mv_l1_5_y =0;
				  leftmb_info.reg7.h264.mv_l1_7_x =0;
				  leftmb_info.reg7.h264.mv_l1_7_y =0;
				  leftmb_info.reg8.h264.mv_l1_13_x =0;
				  leftmb_info.reg8.h264.mv_l1_13_y =0;
				  leftmb_info.reg9.h264.mv_l1_15_x =0;
				  leftmb_info.reg9.h264.mv_l1_15_y =0;
				  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l1 = -1;
				  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_x = 0;
				  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_y = 0;
				  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_x = 0;
				  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_y = 0;
				  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_x = 0;
				  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_y = 0;
				  if(cur_mb_x == picwidthinMB-1)
				  {
					  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l1 = -1;
					  ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_x = 0;
					  ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_y = 0;
				  }

			  }
		  }
		  /*else
		  {
			  leftmb_info.reg1.h264_inter.ref_idx_1_l0= -1;
			  leftmb_info.reg1.h264_inter.ref_idx_3_l0= -1;			  
			  leftmb_info.reg2.h264.mv_l0_5_x =0;
			  leftmb_info.reg2.h264.mv_l0_5_y =0;
			  leftmb_info.reg3.h264.mv_l0_7_x =0;
			  leftmb_info.reg3.h264.mv_l0_7_y =0;
			  leftmb_info.reg4.h264.mv_l0_13_x =0;
			  leftmb_info.reg4.h264.mv_l0_13_y =0;
			  leftmb_info.reg5.h264.mv_l0_15_x =0;
			  leftmb_info.reg5.h264.mv_l0_15_y =0;
			  leftmb_info.reg1.h264_inter.ref_idx_1_l1= -1;
			  leftmb_info.reg1.h264_inter.ref_idx_3_l1= -1;	
			  leftmb_info.reg6.h264.mv_l1_5_x =0;
			  leftmb_info.reg6.h264.mv_l1_5_y =0;
			  leftmb_info.reg7.h264.mv_l1_7_x =0;
			  leftmb_info.reg7.h264.mv_l1_7_y =0;
			  leftmb_info.reg8.h264.mv_l1_13_x =0;
			  leftmb_info.reg8.h264.mv_l1_13_y =0;
			  leftmb_info.reg9.h264.mv_l1_15_x =0;
			  leftmb_info.reg9.h264.mv_l1_15_y =0;
			  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l0 = -1;
              ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_2_l1 = -1;
			  ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_x =0;
			  ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_y =0;
			  ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_x =0;
			  ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_y =0;
			  ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_x =0;
			  ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_y =0;
			  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_x =0;
			  ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_y =0;
			  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_x =0;
			  ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_y =0;
			  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_x =0;
			  ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_y =0;
			  
			  if(cur_mb_x == picwidthinMB-1)
			  {
				  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l0 = -1;
				  ppa_line_buf[cur_mb_x].reg1.h264_inter.ref_idx_3_l1 = -1;
				  ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_x =0;
				  ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_y =0;
				  ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_x =0;
				  ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_y =0;
				  
			  } 
		  }*/

		  //mca out
		  if (mb_type==0)
		  	  mb_type_mca=(slice_type==P_slice) ? 0 : 3;//(direct_8x8_inference_flag ? 3 :)
		  else
              mb_type_mca=mb_type-1;//0:16x16 1:16x8 2:8x16 3:8x8
		  if (direct_8x8_inference_flag)//||direct_spatial_mv_pred_flag
		  {
			  sub_mb_type_mca[0]=(sub_mb_type[0]&0x4) ? 0 : sub_mb_type[0];//0:8x8 1:8x4 2:4x8 3:4x4
			  sub_mb_type_mca[1]=(sub_mb_type[1]&0x4) ? 0 : sub_mb_type[1];
			  sub_mb_type_mca[2]=(sub_mb_type[2]&0x4) ? 0 : sub_mb_type[2];
			  sub_mb_type_mca[3]=(sub_mb_type[3]&0x4) ? 0 : sub_mb_type[3];
		  }
		  else
		  {
			  sub_mb_type_mca[0]=(sub_mb_type[0]&0x4) ? 3 : sub_mb_type[0];
			  sub_mb_type_mca[1]=(sub_mb_type[1]&0x4) ? 3 : sub_mb_type[1];
			  sub_mb_type_mca[2]=(sub_mb_type[2]&0x4) ? 3 : sub_mb_type[2];
			  sub_mb_type_mca[3]=(sub_mb_type[3]&0x4) ? 3 : sub_mb_type[3];
		  }
		  
		  mca_buf[0]=(mb_type_mca<<15)|(is_intra<<14)|(cur_mb_x<<7)|cur_mb_y;//17b
		  mca_buf[1]=(predflag[3][1]<<15)|(predflag[2][1]<<14)|(predflag[1][1]<<13)|(predflag[0][1]<<12)|(predflag[3][0]<<11)|(predflag[2][0]<<10)|(predflag[1][0]<<9)|(predflag[0][0]<<8)|(sub_mb_type_mca[3]<<6)|(sub_mb_type_mca[2]<<4)|(sub_mb_type_mca[1]<<2)|sub_mb_type_mca[0];//16b
		  mca_buf[2]=(w[1][0]<<17)|(w[0][0]<<8)|((ref_idx[0][9]&0xf)<<4)|(ref_idx[0][7]&0xf);//26b
		  mca_buf[3]=(w[3][0]<<17)|(w[2][0]<<8)|((ref_idx[0][21]&0xf)<<4)|(ref_idx[0][19]&0xf);//26b
		  mca_buf[20]=(w[1][1]<<17)|(w[0][1]<<8)|((ref_idx[1][9]&0xf)<<4)|(ref_idx[1][7]&0xf);//26b
		  mca_buf[21]=(w[3][1]<<17)|(w[2][1]<<8)|((ref_idx[1][21]&0xf)<<4)|(ref_idx[1][19]&0xf);//26b
		  for(blk4x4Idx=0;blk4x4Idx<16;blk4x4Idx++)
		  {
			  blk4x4StrIdx = h264_blk_order_map[blk4x4Idx];
		      mca_buf[4+blk4x4Idx]=(mv[0][blk4x4StrIdx][0]<<12)|(mv[0][blk4x4StrIdx][1]&0xfff);//26b
              mca_buf[22+blk4x4Idx]=(mv[1][blk4x4StrIdx][0]<<12)|(mv[1][blk4x4StrIdx][1]&0xfff);//26b
		  }

	  }

	  // col buff out
	  if(decoder_format==STREAM_ID_H264)
	  {
		  int list_sel;
          for(blk4x4Idx=0;blk4x4Idx<16;blk4x4Idx++)
		  {
			  
			  blk4x4StrIdx = h264_blk_order_map[blk4x4Idx];
			  blk8x8Idx=blk4x4Idx/4;
			  if(blk4x4Idx%4==0)
			  {
				  list_sel=0;
				  if (is_intra) 
				  {
					  out_col_ref_idx[blk8x8Idx]=-1;
				  }
				  else
				  {
					  if ((slice_type==B_slice)&&(ref_idx[0][blk4x4StrIdx]<0)&&(ref_idx[1][blk4x4StrIdx]>=0)) 
					  {
						  out_col_ref_idx[blk8x8Idx]=ref_idx[1][blk4x4StrIdx];
						  list_sel=1;
						  col_ref_poc[blk8x8Idx]=list0_POC[maplist1_to_list0[ref_idx[1][blk4x4StrIdx]]];
					  }
					  else
					  {
						  out_col_ref_idx[blk8x8Idx]=ref_idx[0][blk4x4StrIdx];
						  col_ref_poc[blk8x8Idx]=list0_POC[ref_idx[0][blk4x4StrIdx]];
					  }
				  }
				  
			  }
			  out_col_mv[blk4x4Idx][0]=is_intra ? 0 : mv[list_sel][blk4x4StrIdx][0];
			  out_col_mv[blk4x4Idx][1]=is_intra ? 0 : mv[list_sel][blk4x4StrIdx][1];
			  
		  }
		  


		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg0.h264.blk0_ref_poc=col_ref_poc[0];//weihu
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg1.h264.blk1_ref_poc=col_ref_poc[1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg2.h264.blk2_ref_poc=col_ref_poc[2];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg3.h264.blk3_ref_poc=col_ref_poc[3];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg4.h264.blk0_ref_idx=out_col_ref_idx[0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg5.h264.blk1_ref_idx=out_col_ref_idx[1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg6.h264.blk2_ref_idx=out_col_ref_idx[2];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg7.h264.blk3_ref_idx=out_col_ref_idx[3];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg4.h264.mv_0_x=out_col_mv[0][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg4.h264.mv_0_y=out_col_mv[0][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg5.h264.mv_5_x=out_col_mv[5][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg5.h264.mv_5_y=out_col_mv[5][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg6.h264.mv_10_x=out_col_mv[10][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg6.h264.mv_10_y=out_col_mv[10][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg7.h264.mv_15_x=out_col_mv[15][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg7.h264.mv_15_y=out_col_mv[15][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg8.h264.mv_4_x=out_col_mv[4][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg8.h264.mv_4_y=out_col_mv[4][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg9.h264.mv_1_x=out_col_mv[1][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg9.h264.mv_1_y=out_col_mv[1][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg10.h264.mv_6_x=out_col_mv[6][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg10.h264.mv_6_y=out_col_mv[6][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg11.h264.mv_7_x=out_col_mv[7][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg11.h264.mv_7_y=out_col_mv[7][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg12.h264.mv_8_x=out_col_mv[8][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg12.h264.mv_8_y=out_col_mv[8][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg13.h264.mv_9_x=out_col_mv[9][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg13.h264.mv_9_y=out_col_mv[9][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg14.h264.mv_2_x=out_col_mv[2][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg14.h264.mv_2_y=out_col_mv[2][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg15.h264.mv_11_x=out_col_mv[11][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg15.h264.mv_11_y=out_col_mv[11][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg16.h264.mv_12_x=out_col_mv[12][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg16.h264.mv_12_y=out_col_mv[12][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg17.h264.mv_13_x=out_col_mv[13][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg17.h264.mv_13_y=out_col_mv[13][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg18.h264.mv_14_x=out_col_mv[14][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg18.h264.mv_14_y=out_col_mv[14][1];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg19.h264.mv_3_x=out_col_mv[3][0];
		  col_mb_buf[addr_index][cur_mb_x][cur_mb_y].reg19.h264.mv_3_y=out_col_mv[3][1];
	  }

	  /**********mbc para buf generate***********/
	  
	  if(decoder_format==STREAM_ID_H264)
	  {
		  Ipred_avail_a = mb_avail_a && (constrained_intra_pred_flag ? leftmb_info.reg0.h264.is_intra : 1);
		  Ipred_avail_b = mb_avail_b && (constrained_intra_pred_flag ? ppa_line_buf[cur_mb_x].reg0.h264.is_intra : 1);
		  Ipred_avail_c = mb_avail_c && (constrained_intra_pred_flag ? ppa_line_buf[cur_mb_x+1].reg0.h264.is_intra : 1);
		  Ipred_avail_d = mb_avail_d && (constrained_intra_pred_flag ? ppa_line_buf[cur_mb_x-1].reg0.h264.is_intra : 1);
		  
          if(!Ipred_avail_b)//(!(Ipred_avail_a&&Ipred_avail_b))
		  {
             i4_pred_mode_ref[1]=i4_pred_mode_ref[2]=i4_pred_mode_ref[3]=i4_pred_mode_ref[4]=-2;
		  }
		  else if(!(ppa_line_buf[cur_mb_x].reg0.h264.mb_mode==INxN_h264))//(!(mb_type==INxN_h264))
		  {
             i4_pred_mode_ref[1]=i4_pred_mode_ref[2]=i4_pred_mode_ref[3]=i4_pred_mode_ref[4]=2;
		  }
		  else
		  {
			  i4_pred_mode_ref[1]= ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_10_mode;
			  i4_pred_mode_ref[2]= ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_11_mode;
			  i4_pred_mode_ref[3]= ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_14_mode;
			  i4_pred_mode_ref[4]= ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_15_mode;
		  }

		  if(!Ipred_avail_a)//(!(Ipred_avail_a&&Ipred_avail_b))
		  {
			  i4_pred_mode_ref[6]=i4_pred_mode_ref[12]=i4_pred_mode_ref[18]=i4_pred_mode_ref[24]=-2;
		  }
		  else if(!(leftmb_info.reg0.h264.mb_mode ==INxN_h264))//(!(mb_type==INxN_h264))
		  {
			  i4_pred_mode_ref[6]=i4_pred_mode_ref[12]=i4_pred_mode_ref[18]=i4_pred_mode_ref[24]=2;
		  }
		  else
		  {
			  i4_pred_mode_ref[6]= leftmb_info.reg1.h264_intra.intra4x4_5_mode;
			  i4_pred_mode_ref[12]= leftmb_info.reg1.h264_intra.intra4x4_7_mode;
			  i4_pred_mode_ref[18]= leftmb_info.reg1.h264_intra.intra4x4_13_mode;
			  i4_pred_mode_ref[24]= leftmb_info.reg1.h264_intra.intra4x4_15_mode;
		  }
          

          
	      if(is_intra&&(!is_IPCM))
		  {
			  if(mb_type==I16_h264)
			  {
				  Ipred_size=I_16X16;                  			  
			  }
			  else//intra4x4 intra8x8
			  {
                  if(transform_size_8x8_flag)
				  {
					  Ipred_size=I_8X8;
					  b4inblk=4;
				  }
				  else
				  {
					  Ipred_size=I_4X4;
					  b4inblk=1;
				  }

				  for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx+=b4inblk)
				  {
					  
                       blk4x4StrIdx = h264_blk_order_map[blk4x4Idx];
					   left_ipred_mode = i4_pred_mode_ref[blk4x4StrIdx-1];
					   up_ipred_mode = i4_pred_mode_ref[blk4x4StrIdx-CONTEXT_CACHE_WIDTH];
					   most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);
					   if (most_probable_ipred_mode < 0)
					   {
						   most_probable_ipred_mode = 2;//dc
					   }
					   
					   pred_mode = rem_intra4x4_pred_mode[blk4x4Idx];
					   prev_ipred_flag=((prev_intra4x4_pred_mode_flag>>blk4x4Idx)&0x1);
					   
					   pred_mode = ( prev_ipred_flag ? most_probable_ipred_mode : ( (pred_mode >= most_probable_ipred_mode) ? pred_mode +1 : pred_mode ));
					   
					   //must! because BITSTRM may be error
					   if ((pred_mode >9) || (pred_mode < 0))
					   {
						   error_flag = TRUE;
						   return;
					   }
					   
					   i4_pred_mode_ref[blk4x4StrIdx] = pred_mode;
					   if(Ipred_size==I_8X8)
					   {
						   i4_pred_mode_ref[blk4x4StrIdx+1] = i4_pred_mode_ref[blk4x4StrIdx+6]=i4_pred_mode_ref[blk4x4StrIdx+7]=pred_mode;
					   }
				  }

				  leftmb_info.reg1.h264_intra.intra4x4_5_mode= i4_pred_mode_ref[10];
				  leftmb_info.reg1.h264_intra.intra4x4_7_mode= i4_pred_mode_ref[16];
				  leftmb_info.reg1.h264_intra.intra4x4_13_mode= i4_pred_mode_ref[22];
				  leftmb_info.reg1.h264_intra.intra4x4_14_mode= i4_pred_mode_ref[27];
				  leftmb_info.reg1.h264_intra.intra4x4_15_mode= i4_pred_mode_ref[28];
				  ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_10_mode=i4_pred_mode_ref[25];
				  ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_11_mode=i4_pred_mode_ref[26];
				  //ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_14_mode=i4_pred_mode_ref[27];
				  
				  if(cur_mb_x == picwidthinMB-1)
				  {
					  ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_14_mode=i4_pred_mode_ref[27];
					  ppa_line_buf[cur_mb_x].reg1.h264_intra.intra4x4_15_mode=i4_pred_mode_ref[28];
				  }

			  }
		  }
		  
          
		  //mbc out
		  mbc_buf[0]=(intra_chroma_pred_mode<<23)|(Ipred_size<<21)|(Ipred_avail_d<<20)|(Ipred_avail_c<<19)|(Ipred_avail_b<<18)|(Ipred_avail_a<<17)|(skip_idct<<16)|(is_IPCM<<15)|(is_intra<<14)|(cur_mb_x<<7)|cur_mb_y;//25b
          mbc_buf[1]=is_intra&&(is_IPCM||(Ipred_size==I_16X16))? 0xffffff: cbp26&0xffffff;//24b
		  if(Ipred_size==I_16X16)
		  {
		  	  mbc_buf[2]=intra16x16_luma_pred_mode;//2b
			  mbc_buf[3]=0;//2b
		  }
		  else
		  {
			  mbc_buf[2]=(i4_pred_mode_ref[16]<<28)|(i4_pred_mode_ref[15]<<24)|(i4_pred_mode_ref[10]<<20)|(i4_pred_mode_ref[9]<<16)|(i4_pred_mode_ref[14]<<12)|(i4_pred_mode_ref[13]<<8)|(i4_pred_mode_ref[8]<<4)|i4_pred_mode_ref[7];//32b
			  mbc_buf[3]=(i4_pred_mode_ref[28]<<28)|(i4_pred_mode_ref[27]<<24)|(i4_pred_mode_ref[22]<<20)|(i4_pred_mode_ref[21]<<16)|(i4_pred_mode_ref[26]<<12)|(i4_pred_mode_ref[25]<<8)|(i4_pred_mode_ref[20]<<4)|i4_pred_mode_ref[19];//32b
		  }

	  }

	  /**********dbk para buf generate***********/
	  
	  if(decoder_format==STREAM_ID_H264)
	  {
          
          left_mb_intra=leftmb_info.reg0.h264.is_intra;
		  top_mb_intra=ppa_line_buf[cur_mb_x].reg0.h264.is_intra;
          //after mca finish
          if( !mb_avail_b && !top_mb_intra &&(cur_mb_y > 0))
		  {
			  mv[0][1][0]= ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_x;
			  mv[0][1][1]= ppa_line_buf[cur_mb_x].reg2.h264.mv_l0_10_y;
			  mv[0][2][0]= ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_x;
			  mv[0][2][1]= ppa_line_buf[cur_mb_x].reg3.h264.mv_l0_11_y;
			  mv[0][3][0]= ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_x;
			  mv[0][3][1]= ppa_line_buf[cur_mb_x].reg4.h264.mv_l0_14_y;
			  mv[0][4][0]= ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_x;
			  mv[0][4][1]= ppa_line_buf[cur_mb_x].reg5.h264.mv_l0_15_y;
			  mv[1][1][0]= ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_x;
			  mv[1][1][1]= ppa_line_buf[cur_mb_x].reg6.h264.mv_l1_10_y;
			  mv[1][2][0]= ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_x;
			  mv[1][2][1]= ppa_line_buf[cur_mb_x].reg7.h264.mv_l1_11_y;
			  mv[1][3][0]= ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_x;
			  mv[1][3][1]= ppa_line_buf[cur_mb_x].reg8.h264.mv_l1_14_y;
			  mv[1][4][0]= ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_x;
			  mv[1][4][1]= ppa_line_buf[cur_mb_x].reg9.h264.mv_l1_15_y;
		  }	  
		  if( !top_mb_intra &&(cur_mb_y > 0))
		  {
			  addr_idx[0][1]= ppa_line_buf[cur_mb_x].reg2.h264.addr_idx_2_l0;
			  addr_idx[0][2]= ppa_line_buf[cur_mb_x].reg2.h264.addr_idx_2_l0;
			  addr_idx[1][1]= ppa_line_buf[cur_mb_x].reg6.h264.addr_idx_2_l1;
			  addr_idx[1][2]= ppa_line_buf[cur_mb_x].reg6.h264.addr_idx_2_l1;
			  addr_idx[0][3]= ppa_line_buf[cur_mb_x].reg4.h264.addr_idx_3_l0;
			  addr_idx[0][4]= ppa_line_buf[cur_mb_x].reg4.h264.addr_idx_3_l0;
			  addr_idx[1][3]= ppa_line_buf[cur_mb_x].reg8.h264.addr_idx_3_l1;
			  addr_idx[1][4]= ppa_line_buf[cur_mb_x].reg8.h264.addr_idx_3_l1;
		  }
		  
		  if(!mb_avail_a && !left_mb_intra &&(cur_mb_x > 0))
		  {
			  mv[0][6][0]= leftmb_info.reg2.h264.mv_l0_5_x;
			  mv[0][6][1]= leftmb_info.reg2.h264.mv_l0_5_y;
			  mv[0][12][0]= leftmb_info.reg3.h264.mv_l0_7_x;
			  mv[0][12][1]= leftmb_info.reg3.h264.mv_l0_7_y;
			  mv[0][18][0]= leftmb_info.reg4.h264.mv_l0_13_x;
			  mv[0][18][1]= leftmb_info.reg4.h264.mv_l0_13_y;
			  mv[0][24][0]= leftmb_info.reg5.h264.mv_l0_15_x;
			  mv[0][24][1]= leftmb_info.reg5.h264.mv_l0_15_y;
			  mv[1][6][0]= leftmb_info.reg6.h264.mv_l1_5_x;
			  mv[1][6][1]= leftmb_info.reg6.h264.mv_l1_5_y;
			  mv[1][12][0]= leftmb_info.reg7.h264.mv_l1_7_x;
			  mv[1][12][1]= leftmb_info.reg7.h264.mv_l1_7_y;
			  mv[1][18][0]= leftmb_info.reg8.h264.mv_l1_13_x;
			  mv[1][18][1]= leftmb_info.reg8.h264.mv_l1_13_y;
			  mv[1][24][0]= leftmb_info.reg9.h264.mv_l1_15_x;
			  mv[1][24][1]= leftmb_info.reg9.h264.mv_l1_15_y;
		  }
		  if(!left_mb_intra &&(cur_mb_x > 0))
		  {
			  addr_idx[0][6]= leftmb_info.reg2.h264.addr_idx_1_l0;
			  addr_idx[0][12]= leftmb_info.reg2.h264.addr_idx_1_l0;
			  addr_idx[1][6]= leftmb_info.reg6.h264.addr_idx_1_l1;
			  addr_idx[1][12]= leftmb_info.reg6.h264.addr_idx_1_l1;
			  addr_idx[0][18]= leftmb_info.reg4.h264.addr_idx_3_l0;
			  addr_idx[0][24]= leftmb_info.reg4.h264.addr_idx_3_l0;
			  addr_idx[1][18]= leftmb_info.reg8.h264.addr_idx_3_l1;
			  addr_idx[1][24]= leftmb_info.reg8.h264.addr_idx_3_l1;
		  }

		  if (cur_mb_x > 0)
		  {
              cbp_blk[6]= leftmb_info.reg0.h264.cbp_blk&0x1;
			  cbp_blk[12]= (leftmb_info.reg0.h264.cbp_blk>>1)&0x1;
			  cbp_blk[18]= (leftmb_info.reg0.h264.cbp_blk>>2)&0x1;
			  cbp_blk[24]= (leftmb_info.reg0.h264.cbp_blk>>3)&0x1;	
		  }
		  if (cur_mb_y > 0)
		  {	  
			  cbp_blk[1]= ppa_line_buf[cur_mb_x].reg0.h264.cbp_blk&0x1;
			  cbp_blk[2]= (ppa_line_buf[cur_mb_x].reg0.h264.cbp_blk>>1)&0x1;
			  cbp_blk[3]= (ppa_line_buf[cur_mb_x].reg0.h264.cbp_blk>>2)&0x1;
			  cbp_blk[4]= (ppa_line_buf[cur_mb_x].reg0.h264.cbp_blk>>3)&0x1;		
		  }
		  
		  for(i=0;i<16;i++)
			  cbp_blk[h264_blk_order_map[i]]=(cbp>>i)&0x1;

		  if (cur_mb_y) 
		    qp_top=ppa_line_buf[cur_mb_x].reg0.h264.qp;
		  else
			qp_top=0;
		  if (cur_mb_x) 
             qp_left=leftmb_info.reg0.h264.qp;
		  else
			  qp_left=0;
		  skip_deblock=(disable_deblocking_filter_idc==1);
		  
	
		  filter_left_edge=(cur_mb_x > 0)&&(mb_avail_a||(disable_deblocking_filter_idc!=2))&&!skip_deblock;
		  filter_top_edge=(cur_mb_y > 0)&&(mb_avail_b||(disable_deblocking_filter_idc!=2))&&!skip_deblock;
		  //filter_internal_edge=!skip_deblock;
		  for(i=0;i<4;i++)//horizontal line
		  {
			  for(j=0;j<4;j++)//part
			  {
				  blk4x4StrIdx = j*6+7+i;
				  ref0_p=addr_idx[0][blk4x4StrIdx-1];
				  ref1_p=addr_idx[1][blk4x4StrIdx-1];
				  ref0_q=addr_idx[0][blk4x4StrIdx];
				  ref1_q=addr_idx[1][blk4x4StrIdx];
				  mv0_p[0]=mv[0][blk4x4StrIdx-1][0];
				  mv0_p[1]=mv[0][blk4x4StrIdx-1][1];
				  mv1_p[0]=mv[1][blk4x4StrIdx-1][0];
				  mv1_p[1]=mv[1][blk4x4StrIdx-1][1];
				  mv0_q[0]=mv[0][blk4x4StrIdx][0];
				  mv0_q[1]=mv[0][blk4x4StrIdx][1];
				  mv1_q[0]=mv[1][blk4x4StrIdx][0];
                  mv1_q[1]=mv[1][blk4x4StrIdx][1];				  
				  condition0=(ref0_p==ref0_q)&&((ref0_p==0x1f)||(abs(mv0_p[0]-mv0_q[0])<4)&&(abs(mv0_p[1]-mv0_q[1])<4));
				  condition1=(ref0_p==ref1_q)&&((ref0_p==0x1f)||(abs(mv0_p[0]-mv1_q[0])<4)&&(abs(mv0_p[1]-mv1_q[1])<4));
                  condition2=(ref1_p==ref0_q)&&((ref1_p==0x1f)||(abs(mv1_p[0]-mv0_q[0])<4)&&(abs(mv1_p[1]-mv0_q[1])<4));
				  condition3=(ref1_p==ref1_q)&&((ref1_p==0x1f)||(abs(mv1_p[0]-mv1_q[0])<4)&&(abs(mv1_p[1]-mv1_q[1])<4));

				  if ((i==0) &&(!filter_left_edge)||transform_size_8x8_flag&&((i==1)||(i==3))||skip_deblock)
				  {
                       bs_h[4*i+j]=0;
				  }
				  else if ((i==0) &&(is_intra||left_mb_intra)) 
				  {
                       bs_h[4*i+j]=4;
				  }
				  else if ((i!=0) &&(is_intra))
				  {
					  bs_h[4*i+j]=3;
				  }
				  else if (cbp_blk[blk4x4StrIdx]||cbp_blk[blk4x4StrIdx-1])
				  {
					  bs_h[4*i+j]=2;
				  }
				  else 
				  {
                       if((ref0_p==ref1_p)||(ref0_q==ref1_q))
						   bs_h[4*i+j]=condition0&&condition1&&condition2&&condition3 ? 0 : 1;
					   else
						   bs_h[4*i+j]=condition0&&condition3&&!condition1&&!condition2||!condition0&&!condition3&&condition1&&condition2 ? 0 : 1;
				  }


			  }//for(j=0;j<4;j++)//part
		  }

		  for(i=0;i<4;i++)//vertical line
		  {
			  for(j=0;j<4;j++)//part
			  {
				  blk4x4StrIdx = i*6+7+j;
				  ref0_p=addr_idx[0][blk4x4StrIdx-6];
				  ref1_p=addr_idx[1][blk4x4StrIdx-6];
				  ref0_q=addr_idx[0][blk4x4StrIdx];
				  ref1_q=addr_idx[1][blk4x4StrIdx];
				  mv0_p[0]=mv[0][blk4x4StrIdx-6][0];
				  mv0_p[1]=mv[0][blk4x4StrIdx-6][1];
				  mv1_p[0]=mv[1][blk4x4StrIdx-6][0];
				  mv1_p[1]=mv[1][blk4x4StrIdx-6][1];
				  mv0_q[0]=mv[0][blk4x4StrIdx][0];
				  mv0_q[1]=mv[0][blk4x4StrIdx][1];
				  mv1_q[0]=mv[1][blk4x4StrIdx][0];
				  mv1_q[1]=mv[1][blk4x4StrIdx][1];				  
				  condition0=(ref0_p==ref0_q)&&((ref0_p==0x1f)||(abs(mv0_p[0]-mv0_q[0])<4)&&(abs(mv0_p[1]-mv0_q[1])<4));
				  condition1=(ref0_p==ref1_q)&&((ref0_p==0x1f)||(abs(mv0_p[0]-mv1_q[0])<4)&&(abs(mv0_p[1]-mv1_q[1])<4));
				  condition2=(ref1_p==ref0_q)&&((ref1_p==0x1f)||(abs(mv1_p[0]-mv0_q[0])<4)&&(abs(mv1_p[1]-mv0_q[1])<4));
				  condition3=(ref1_p==ref1_q)&&((ref1_p==0x1f)||(abs(mv1_p[0]-mv1_q[0])<4)&&(abs(mv1_p[1]-mv1_q[1])<4));
					  
				  if ((i==0) &&(!filter_top_edge)||transform_size_8x8_flag&&((i==1)||(i==3))||skip_deblock)
				  {
					  bs_v[4*i+j]=0;
				  }
				  else if ((i==0) &&(is_intra||top_mb_intra)) 
				  {
					  bs_v[4*i+j]=4;
				  }
				  else if ((i!=0) &&(is_intra))
				  {
					  bs_v[4*i+j]=3;
				  }
				  else if (cbp_blk[blk4x4StrIdx]||cbp_blk[blk4x4StrIdx-6])
				  {
					  bs_v[4*i+j]=2;
				  }
				  else 
				  {
					  if((ref0_p==ref1_p)||(ref0_q==ref1_q))
						  bs_v[4*i+j]=condition0&&condition1&&condition2&&condition3 ? 0 : 1;
					  else
						  bs_v[4*i+j]=condition0&&condition3&&!condition1&&!condition2||!condition0&&!condition3&&condition1&&condition2 ? 0 : 1;
				  }
					  
					  
			  }//for(j=0;j<4;j++)//part
		  }



		  ppa_line_buf[cur_mb_x].reg2.h264.addr_idx_2_l0=addr_idx[0][25];
		  ppa_line_buf[cur_mb_x].reg6.h264.addr_idx_2_l1 =addr_idx[1][25];
		  ppa_line_buf[cur_mb_x].reg4.h264.addr_idx_3_l0=addr_idx[0][28];
		  ppa_line_buf[cur_mb_x].reg8.h264.addr_idx_3_l1=addr_idx[1][28];
		  leftmb_info.reg2.h264.addr_idx_1_l0=addr_idx[0][10];		 
		  leftmb_info.reg6.h264.addr_idx_1_l1=addr_idx[1][10];		  
		  leftmb_info.reg4.h264.addr_idx_3_l0=addr_idx[0][28];		  
		  leftmb_info.reg8.h264.addr_idx_3_l1=addr_idx[1][28];
		  
		  
		  leftmb_info.reg0.h264.cbp_blk=(cbp_blk[28]<<3)|(cbp_blk[22]<<2)|(cbp_blk[16]<<1)|cbp_blk[10];		   
		  ppa_line_buf[cur_mb_x].reg0.h264.cbp_blk=(cbp_blk[28]<<3)|(cbp_blk[27]<<2)|(cbp_blk[26]<<1)|cbp_blk[25];	
		  
		  //dbk out
		  dbk_buf[0]=(qp_top<<26)|(qp_left<<20)|(mb_qp_y<<14)|(cur_mb_x<<7)|cur_mb_y;//32b
		  dbk_buf[1]=(bs_h[7]<<28)|(bs_h[6]<<24)|(bs_h[5]<<20)|(bs_h[4]<<16)|(bs_h[3]<<12)|(bs_h[2]<<8)|(bs_h[1]<<4)|bs_h[0];//32b
          dbk_buf[2]=(bs_h[15]<<28)|(bs_h[14]<<24)|(bs_h[13]<<20)|(bs_h[12]<<16)|(bs_h[11]<<12)|(bs_h[10]<<8)|(bs_h[9]<<4)|bs_h[8];//32b
		  dbk_buf[3]=(bs_v[7]<<28)|(bs_v[6]<<24)|(bs_v[5]<<20)|(bs_v[4]<<16)|(bs_v[3]<<12)|(bs_v[2]<<8)|(bs_v[1]<<4)|bs_v[0];//32b
          dbk_buf[4]=(bs_v[15]<<28)|(bs_v[14]<<24)|(bs_v[13]<<20)|(bs_v[12]<<16)|(bs_v[11]<<12)|(bs_v[10]<<8)|(bs_v[9]<<4)|bs_v[8];//32b
          //dbk_buf[1]=(bs_h[7]<<21)|(bs_h[6]<<18)|(bs_h[5]<<15)|(bs_h[4]<<12)|(bs_h[3]<<9)|(bs_h[2]<<6)|(bs_h[1]<<3)|bs_h[0];//24b
          //dbk_buf[2]=(bs_h[15]<<21)|(bs_h[14]<<18)|(bs_h[13]<<15)|(bs_h[12]<<12)|(bs_h[11]<<9)|(bs_h[10]<<6)|(bs_h[9]<<3)|bs_h[8];//24b
		  //dbk_buf[3]=(bs_v[7]<<21)|(bs_v[6]<<18)|(bs_v[5]<<15)|(bs_v[4]<<12)|(bs_v[3]<<9)|(bs_v[2]<<6)|(bs_v[1]<<3)|bs_v[0];//24b
          //dbk_buf[4]=(bs_v[15]<<21)|(bs_v[14]<<18)|(bs_v[13]<<15)|(bs_v[12]<<12)|(bs_v[11]<<9)|(bs_v[10]<<6)|(bs_v[9]<<3)|bs_v[8];//24b
	  }
	  
	  /*******************/
	  if(cur_mb_x>0)
	  {	  
		  ppa_line_buf[cur_mb_x-1].reg0.h264.slice_nr=leftmb_info.reg0.h264.slice_nr;// not fmo //weihu	  
		  ppa_line_buf[cur_mb_x-1].reg0.h264.mb_mode=leftmb_info.reg0.h264.mb_mode;
		  ppa_line_buf[cur_mb_x-1].reg0.h264.is_intra=leftmb_info.reg0.h264.is_intra;
		  ppa_line_buf[cur_mb_x-1].reg0.h264.is_skip=leftmb_info.reg0.h264.is_skip;
		  ppa_line_buf[cur_mb_x-1].reg0.h264.qp=leftmb_info.reg0.h264.qp;
	  }
	  
	  if(cur_mb_x == picwidthinMB-1)
	  {	  
		  ppa_line_buf[cur_mb_x].reg0.h264.slice_nr=slice_num;// not fmo //weihu	  
		  ppa_line_buf[cur_mb_x].reg0.h264.mb_mode=mb_type;
		  ppa_line_buf[cur_mb_x].reg0.h264.is_intra=is_intra;
		  ppa_line_buf[cur_mb_x].reg0.h264.is_skip=is_skip;
		  ppa_line_buf[cur_mb_x].reg0.h264.qp=mb_qp_y;
	  }
      

	  leftmb_info.reg0.h264.slice_nr=slice_num;// not fmo //weihu
	  leftmb_info.reg0.h264.mb_mode=mb_type;
      leftmb_info.reg0.h264.is_intra=is_intra;
	  leftmb_info.reg0.h264.is_skip=is_skip;
	  leftmb_info.reg0.h264.qp=mb_qp_y;
	  
	  

}





















































































