#include "sc8810_video_header.h"
//#include "parser_global.h"

#define debugoutput
#ifdef debugoutput

#include <stdio.h>
FILE *fout1;

#endif//weihu

#ifdef PARSER_CMODEL
//void  ppa_module(void);//weihu
int32 MbPartPredMode(int8 mb_type, int8 mbPartIdx, uint8 slice_type);
int32 SubMbPartPredMode(int8 sub_mb_type, uint8 slice_type);
int32 last_is_skip;

extern int32 b4order[16];//weihu
extern void H264_IpcmDec ();

LOCAL void H264Dec_exit_slice (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;
	
	old_slice_ptr->frame_num = img_ptr->frame_num;
	old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
	old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
	old_slice_ptr->idr_flag = img_ptr->idr_flag;
	
	if (img_ptr->idr_flag)
	{
		old_slice_ptr->idr_pic_id = img_ptr->idr_pic_id;
	}
	
	if (g_active_sps_ptr->pic_order_cnt_type == 0)
	{
		old_slice_ptr->pic_order_cnt_lsb = img_ptr->pic_order_cnt_lsb;
		old_slice_ptr->delta_pic_order_cnt_bottom = img_ptr->delta_pic_order_cnt_bottom;
	}
	
	if (g_active_sps_ptr->pic_order_cnt_type == 1)
	{
		old_slice_ptr->delta_pic_order_cnt[0] = img_ptr->delta_pic_order_cnt[0];
		old_slice_ptr->delta_pic_order_cnt[1] = img_ptr->delta_pic_order_cnt[1];
	}
	
	return;
}

void H264Dec_parse_slice_data (void)
{
	uint8		slice_type;
	uint16		Slice_first_mb_x, Slice_first_mb_y;
	uint16		currMbx, currMbY;
	uint16		currMbNr;
	uint16		startMbNr;
	BOOLEAN		moreDataFlag = TRUE;
	uint32		mb_skip_run = 0;
	uint32		mb_skip_flag = 0;
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	uint32		i,j;
	int			parse_mb_layer = 0;
	LINE_BUF_T *MBLineBuf;
	char        error_flag;//weihu
	short         tmp1,blk;//weihu
	int          slice_end=0;


	//need to calculate the start mb_address of current slice and then derive the availability of left and top mb
	memset(inbuf, 0, 36*sizeof(int));
	Slice_first_mb_x = (g_par_reg_ptr->PARSER_CFG0 >> 0) & 0x7f;
	Slice_first_mb_y = (g_par_reg_ptr->PARSER_CFG0 >> 7) & 0x7f;
	slice_type = (g_par_reg_ptr->PARSER_CFG0 >> 15) & 0x03;
	currMBBuf->slice_type = slice_type;
	leftMBBuf->slice_type = slice_type;
	last_is_skip = 0;

	startMbNr = Slice_first_mb_y*(g_image_ptr->frame_width_in_mbs) + Slice_first_mb_x;
	currMbNr  = startMbNr;

	do 
	{

		parse_mb_layer = 0;
		currMbx = currMbNr%g_image_ptr->frame_width_in_mbs;
		currMbY = currMbNr/g_image_ptr->frame_width_in_mbs;
//#if _DEBUG_	
		if (currMbx == 0 && currMbY == 0 && g_nFrame_dec_h264 == 1)//weihu
		{
			currMbNr = currMbNr;
		}
//#endif

		inbuf[0] |= (currMbY << 3) | (currMbx << 10);

		currMBBuf->mb_x = currMbx;

		currMBBuf->lmb_avail = ((currMbx > 0) && ((currMbNr-1) >= startMbNr)); //left
		currMBBuf->tmb_avail = ((currMbY > 0) && ((currMbNr - g_image_ptr->frame_width_in_mbs) >= startMbNr)); //top

		if (slice_type != I_SLICE)
		{
			if (!g_active_pps_ptr->entropy_coding_mode_flag && (last_is_skip == 0))
			{
				if (mb_skip_run == 0) 
				{
					mb_skip_run = READ_UE_V(stream);
				}
				
//				for (i=0; i<mb_skip_run; i++)
//				{
				if (mb_skip_run > 0) 
				{
					g_ppa_buf_ptr->SYNTAX_BUF0 = (currMbx << 0) | (currMbY << 7);
					g_ppa_buf_ptr->SYNTAX_BUF1 = (1 << 16);
					
					MBLineBuf = vldLineBuf + currMbx;
		 		 	MBLineBuf->nnz_y = 0;
				 	MBLineBuf->nnz_c = 0;
					inbuf[0] &= 0xfffffff8;//set mb type
					inbuf[0] |= (1 << 17);// set is_skip
				}
//					g_ppa_buf_ptr->SYNTAX_BUF0 = (currMbx << 0) | (currMbY << 7);
//					g_ppa_buf_ptr->SYNTAX_BUF1 = (1 << 16);
//					
//					MBLineBuf = vldLineBuf + currMbx;
//		 		 	MBLineBuf->nnz_y = 0;
//				 	MBLineBuf->nnz_c = 0;
//					inbuf[0] &= 0xfffffff8;//set mb type
//					inbuf[0] |= (1 << 17);// set is_skip
// 					ppa_module();

//					currMbNr++;
//
//					currMbx = currMbNr%g_image_ptr->frame_width_in_mbs;
//					currMbY = currMbNr/g_image_ptr->frame_width_in_mbs;
// 				}

//				if (mb_skip_run > 0)
//				{
//					leftMBBuf->nnz_y0 = 0;
//					leftMBBuf->nnz_y1 = 0;
//					leftMBBuf->nnz_y2 = 0;
//					leftMBBuf->nnz_y3 = 0;
//					leftMBBuf->nnz_cb = 0;
//					leftMBBuf->nnz_cr = 0;
//				}		

//				currMBBuf->mb_x = currMbx;
//
//				currMBBuf->lmb_avail = ((currMbx > 0) && ((currMbNr-1) >= startMbNr)); //left
//				currMBBuf->tmb_avail = ((currMbY > 0) && ((currMbNr - g_image_ptr->frame_width_in_mbs) >= startMbNr)); //top
//
//				moreDataFlag = (!uvlc_startcode_follows(g_image_ptr));
			}
			else if (g_active_pps_ptr->entropy_coding_mode_flag)//CABAC mb_skip_flag
			{
				mb_skip_flag = readMB_skip_flagInfo_CABAC_High();	
				moreDataFlag = (!mb_skip_flag);
				currMBBuf->mb_skip_flag = mb_skip_flag;

				if (mb_skip_flag)
				{
					currMBBuf->cbp = 0;
					currMBBuf->delta_qp = 0;

					MBLineBuf = vldLineBuf + currMbx;

		 		 	MBLineBuf->nnz_y = 0;
				 	MBLineBuf->nnz_c = 0;
					MBLineBuf->mb_skip_flag = 1;
					MBLineBuf->cbp = 0;
					MBLineBuf->c_ipred_mode = 0;
					MBLineBuf->coded_dc_flag = 0;
					memset(MBLineBuf->mvd, 0, 8*sizeof(int32));
					MBLineBuf->ref[0] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
					MBLineBuf->ref[1] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
					
					leftMBBuf->nnz_y0 = 0;
					leftMBBuf->nnz_y1 = 0;
					leftMBBuf->nnz_y2 = 0;
					leftMBBuf->nnz_y3 = 0;
					leftMBBuf->nnz_cb = 0;
					leftMBBuf->nnz_cr = 0;
					leftMBBuf->mb_skip_flag = 1;
					leftMBBuf->delta_qp = 0;
					leftMBBuf->cbp = 0;
					leftMBBuf->c_ipred_mode = 0;
					leftMBBuf->coded_dc_flag = 0;
					memset(leftMBBuf->mvd, 0, 32*sizeof(int32));
					leftMBBuf->ref[0] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
					leftMBBuf->ref[1] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
				
					inbuf[0] &= 0xfffffff8;//set mb type
					inbuf[0] |= (1 << 17);// set is_skip
				}
			}
		}



		if (currMbx == 21 && currMbY == 9 && g_nFrame_dec_h264 == 7)
		{
			currMbx =currMbx;
		}//weihu
        g_glb_reg_ptr->VSP_CTRL0 = (((currMbY&0x7f)<<8) | (currMbx&0x7f));//weihu just for simulation

		if (moreDataFlag && (mb_skip_run == 0))
		{
			H264Dec_parse_MB_layer();
			last_is_skip = 0;
			parse_mb_layer = 1;
		}

		if (!g_active_pps_ptr->entropy_coding_mode_flag && ((mb_skip_run == 1 )|| (parse_mb_layer == 1)))
		{
			moreDataFlag = (!uvlc_startcode_follows(g_image_ptr));
		}
		else if(g_active_pps_ptr->entropy_coding_mode_flag)//CABAC related
		{
			moreDataFlag = (!biari_decode_final(g_image_ptr));
		}
		
		g_ppa_buf_ptr->SYNTAX_BUF0 = (currMbx << 0) | (currMbY << 7);
		g_ppa_buf_ptr->SYNTAX_BUF1 = (mb_skip_flag << 16);


	fout1=fopen("E:/project/HDVSP/h264_decoder/simulation/trace/dct_in1.txt","ab+");
	fprintf(fout1,"********************************************%d %d frame %d\n",currMbx,currMbY, g_nFrame_dec_h264);
/*	for(blk=0;blk<26;blk++)
	{
	   fprintf(fout1,"%d*********dct in*********\n ", blk);
	   for(i=0;i<4;i++)
	   {
		   if ((blk!=25)||(i<2))
		   {
		   tmp1= vsp_dct_io_0[blk*8+i*2]&0xffff;
		   fprintf(fout1,"%d ", tmp1);
		   tmp1= (vsp_dct_io_0[blk*8+i*2]>>16)&0xffff;
		   fprintf(fout1,"%d ", tmp1);
		   tmp1= vsp_dct_io_0[blk*8+i*2+1]&0xffff;
		   fprintf(fout1,"%d ", tmp1);
		   tmp1= (vsp_dct_io_0[blk*8+i*2+1]>>16)&0xffff;
		   fprintf(fout1,"%d ", tmp1);
		  
		   fprintf(fout1,"\n ");
		   }
    	}
	}*/	//weihu



		ppa_module(dct_para_buf, mbc_para_buf, dbk_para_buf, mca_para_buf, inbuf, ref_list_buf, STREAM_ID_H264,g_image_ptr->frame_width_in_mbs,g_image_ptr->frame_height_in_mbs, slice_info, error_flag);//weihu
		iqt_module_ppa (dct_out_buf, vsp_dct_io_0, STREAM_ID_H264, &slice_info[30], dct_para_buf);
		mca_module_ppa (mca_out_buf, slice_info, mca_para_buf, STREAM_ID_H264,g_image_ptr->frame_width_in_mbs,g_image_ptr->frame_height_in_mbs);
	    mbc_module_ppa (mbc_out_buf,mca_out_buf,dct_out_buf, slice_info, mbc_para_buf, STREAM_ID_H264,g_image_ptr->frame_width_in_mbs,g_image_ptr->frame_height_in_mbs);
	    dbk_module_ppa (dbk_out_buf, mbc_out_buf, slice_info, dbk_para_buf, STREAM_ID_H264,g_image_ptr->frame_width_in_mbs,g_image_ptr->frame_height_in_mbs);

		if (currMbx == 19 && currMbY == 10 && g_nFrame_dec_h264 == 4)
		{
			for(blk=0;blk<24;blk++)
			{
				fprintf(fout1,"%d*********dct output*********\n ", blk);
				for(i=0;i<4;i++)
				{
					tmp1= dct_out_buf[blk*16+i*4]&0xffff;
					fprintf(fout1,"%d ", tmp1);
					tmp1= dct_out_buf[blk*16+i*4+1]&0xffff;
					fprintf(fout1,"%d ", tmp1);
					tmp1= dct_out_buf[blk*16+i*4+2]&0xffff;
					fprintf(fout1,"%d ", tmp1);
					tmp1= dct_out_buf[blk*16+i*4+3]&0xffff;
					fprintf(fout1,"%d ", tmp1);
					
					fprintf(fout1,"\n ");
				}
			}	

			for(blk=0;blk<24;blk++)
			{
				fprintf(fout1,"%d*********mbc output*********\n ", blk);
				for(i=0;i<4;i++)
				{
					tmp1= mbc_out_buf[blk*16+i*4]&0xffff;
					fprintf(fout1,"%x ", tmp1);
					tmp1= mbc_out_buf[blk*16+i*4+1]&0xffff;
					fprintf(fout1,"%x ", tmp1);
					tmp1= mbc_out_buf[blk*16+i*4+2]&0xffff;
					fprintf(fout1,"%x ", tmp1);
					tmp1= mbc_out_buf[blk*16+i*4+3]&0xffff;
					fprintf(fout1,"%x ", tmp1);
					
					fprintf(fout1,"\n ");
				}
			}	
		}//weihu

	
	   //fprintf(fout1,"%d*********dbk output*********\n ", blk);
	   /*for(i=0;i<20;i++)//line
	   {
	      for(j=0;j<6;j++)
		  {
			   tmp1= dbk_out_buf[i*24+j*4]&0xffff;
			   fprintf(fout1,"%d ", tmp1);
			   tmp1= dbk_out_buf[i*24+j*4+1]&0xffff;
			   fprintf(fout1,"%d ", tmp1);
			   tmp1= dbk_out_buf[i*24+j*4+2]&0xffff;
			   fprintf(fout1,"%d ", tmp1);
			   tmp1= dbk_out_buf[i*24+j*4+3]&0xffff;
			   fprintf(fout1,"%d ", tmp1);
		   }
	    fprintf(fout1,"\n ");
		}*/
	
	fclose(fout1);//weihu
		
		if (!g_active_pps_ptr->entropy_coding_mode_flag && mb_skip_run != 0)
		{
			leftMBBuf->nnz_y0 = 0;
			leftMBBuf->nnz_y1 = 0;
			leftMBBuf->nnz_y2 = 0;
			leftMBBuf->nnz_y3 = 0;
			leftMBBuf->nnz_cb = 0;
			leftMBBuf->nnz_cr = 0;
			mb_skip_run--;
			if (mb_skip_run == 0)
			{
				last_is_skip = 1;
			}
		}
		memset(inbuf, 0, 36*sizeof(int));
		memset(g_ppa_buf_ptr, 0, 38*sizeof(uint32));
		memset(currMBBuf, 0, sizeof(MB_BUF_T));
		currMBBuf->slice_type = slice_type;
		currMBBuf->ref[0] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
		currMBBuf->ref[1] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 



		{
			//currMbNr++;//?
			g_image_ptr->num_dec_mb++;
			
			if (g_image_ptr->num_dec_mb == g_image_ptr->frame_size_in_mbs)
			{
				g_curr_slice_ptr->next_header = SOP;
				g_image_ptr->curr_mb_nr=0;
				g_image_ptr->num_dec_mb=0;//weihu
				g_glb_reg_ptr->VSP_CTRL0 = 0;//weihu just for simulation
				slice_end=1;
			}else
			{
				g_image_ptr->curr_mb_nr = H264Dec_Fmo_get_next_mb_num(g_image_ptr->curr_mb_nr, g_image_ptr);
				currMbNr =g_image_ptr->curr_mb_nr;

				if (g_image_ptr->curr_mb_nr == -1)
				{
					
					//nal_startcode_follows(g_image_ptr);
					slice_end=1;
				}
				
				if (!moreDataFlag&&(mb_skip_run==0))//(!nal_startcode_follows(g_image_ptr))
				{
					slice_end=1;
				}
				//else if (g_image_ptr->cod_counter <= 0)//P 帧 skip 结束 无数据
				//{
				//	slice_end=1;
				//}
				
				
			}
		}//weihu



	} while(!slice_end);//(moreDataFlag && (currMbNr!=g_image_ptr->frame_size_in_mbs));

    H264Dec_exit_slice(g_image_ptr);

}

void H264Dec_parse_MB_layer (void) //8810很大一部分是软解码的，就调用软解码的函数就可以了。先把流程理清楚。
{
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	int8		mb_type, mb_mode;
	int8		sub_mb_type[4];
	BOOLEAN		transform_8x8_mode_flag;
	BOOLEAN		noSubMbPartSizeLess8x8;
	//mode syntax element
	BOOLEAN		transform_size_8x8_flag = 0;

	//residue related syntax
	uint8		coded_block_pattern;
	uint8		cbpLuma;
	uint8		cbpChroma;
	int32		mb_qp_delta;
	uint8		slice_type;
	uint8		mbPartIdx;
	LINE_BUF_T *MBLineBuf;

	MBLineBuf = vldLineBuf + currMBBuf->mb_x;


	
	transform_8x8_mode_flag = (g_par_reg_ptr->PARSER_CFG0 >> 14) & 0x01;
	slice_type = (g_par_reg_ptr->PARSER_CFG0 >> 15) & 0x03;

	if (g_active_pps_ptr->entropy_coding_mode_flag)
	{
		mb_type = readMB_typeInfo_CABAC_High();
	}
	else //cavlc
	{
		mb_type = READ_UE_V(stream);
		currMBBuf->mb_type = mb_type;
	}
	
	if (slice_type == I_SLICE)
	{
		Interpret_mb_mode_I(mb_type);
	}
	else if (slice_type == P_SLICE)
	{
		Interpret_mb_mode_P(mb_type);
	}
	else
	{
		Interpret_mb_mode_B(mb_type);
	}

	mb_mode = currMBBuf->mb_mode;

	g_ppa_buf_ptr->SYNTAX_BUF0 |= (mb_type << 13);
	inbuf[0] |= mb_mode;
	inbuf[0] |= (0 << 17); //here can not be skip mode, otherwise will go through other branch

	if (mb_mode == I16_h264)
	{
		int i16_mode;

		if (slice_type == I_SLICE)
		{
			i16_mode = (mb_type-1)%4;
		}
		else if (slice_type == P_SLICE)
		{
			i16_mode = (mb_type-6)%4;
		}
		else
		{
			i16_mode = (mb_type-24)%4;
		}
		inbuf[2] |= i16_mode;
	}
	
	if (mb_mode != IPCM_h264)//read mode
	{
		noSubMbPartSizeLess8x8 = TRUE;

		if (/*mb_mode != INxN_h264 && mb_mode != I16_h264 &&*/ mb_mode == PB8X8_h264) // sub mb type related
		{
			H264Dec_parse_subMB_pred(mb_type, sub_mb_type);

			for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
			{
				if ((slice_type!=B_SLICE) || (sub_mb_type[mbPartIdx]!=0))
				{
					if (((slice_type==P_SLICE)&&(sub_mb_type[mbPartIdx]>0)) || 
					    ((slice_type==B_SLICE)&&(sub_mb_type[mbPartIdx]>3)))
					{
						noSubMbPartSizeLess8x8 = FALSE;
					}
				}
				else if(!g_active_sps_ptr->direct_8x8_reference_flag)
				{
					noSubMbPartSizeLess8x8 = FALSE;
				}
			}
		}
		//first decide whether need to read 8x8 flag 
		else 
		{
			if (transform_8x8_mode_flag && mb_mode == INxN_h264)
			{
				if (g_active_pps_ptr->entropy_coding_mode_flag)
				{
					transform_size_8x8_flag = readMB_tran8x8_flagInfo_CABAC_High();
				}
				else //cavlc
				{
					transform_size_8x8_flag = READ_FLC(stream, 1);	
				}
				
				g_ppa_buf_ptr->SYNTAX_BUF2 |= transform_size_8x8_flag;
				inbuf[0] |= (transform_size_8x8_flag << 24); 
			}
			
			H264Dec_parse_MB_pred(mb_type, transform_size_8x8_flag);
		}
		
		memcpy(MBLineBuf->mvd, currMBBuf->mvd[12], 8 * sizeof(int32));
		MBLineBuf->ref[0] = (currMBBuf->ref[0] >> 16);
		MBLineBuf->ref[1] = (currMBBuf->ref[1] >> 16);

		if (mb_mode != I16_h264)
		{
			if (!g_active_pps_ptr->entropy_coding_mode_flag)
			{
				int32 val = READ_UE_V(stream);

				if(mb_mode == INxN_h264)
				{
					coded_block_pattern = g_cbp_intra_tbl[val];
				}else
				{
					coded_block_pattern = g_cbp_inter_tbl[val];
				}
			}
			else //CABAC
			{
				coded_block_pattern = readCBP_CABAC_High();
			}
			
			g_ppa_buf_ptr->SYNTAX_BUF2 |= (coded_block_pattern << 1);

			cbpLuma	  = coded_block_pattern%16;
			cbpChroma = coded_block_pattern/16;

			currMBBuf->cbp = coded_block_pattern;

			if ((cbpLuma>0) && (transform_8x8_mode_flag) && (mb_mode != INxN_h264) && noSubMbPartSizeLess8x8 && 
			   (((slice_type!=B_SLICE)||(mb_type!=0))||g_active_sps_ptr->direct_8x8_reference_flag))
				     /*mb_type != B_Direct_16x16*/
			{
				if (g_active_pps_ptr->entropy_coding_mode_flag)
				{
					transform_size_8x8_flag = readMB_tran8x8_flagInfo_CABAC_High();
				}
				else //cavlc
				{
					transform_size_8x8_flag = READ_FLC(stream, 1);	
				}
				
				g_ppa_buf_ptr->SYNTAX_BUF2 |= transform_size_8x8_flag;
				inbuf[0] |= (transform_size_8x8_flag << 24); 
			}
		}
		else
		{
			if (slice_type==B_SLICE)
			{
				coded_block_pattern = g_ICBP_TBL[(mb_type-24)>>2];
			}
			else if (slice_type==P_SLICE)
			{
				coded_block_pattern = g_ICBP_TBL[(mb_type-6)>>2];
			}
			else
			{
				coded_block_pattern = g_ICBP_TBL[(mb_type-1)>>2];
			}
			cbpLuma	  = coded_block_pattern%16;
			cbpChroma = coded_block_pattern/16;
			currMBBuf->cbp = coded_block_pattern;
		}

		MBLineBuf->cbp = currMBBuf->cbp;

		currMBBuf->transform_size_8x8_flag = transform_size_8x8_flag;
		MBLineBuf->transform_size_8x8_flag = transform_size_8x8_flag;
		MBLineBuf->mb_type = mb_type;
		MBLineBuf->c_ipred_mode = currMBBuf->c_ipred_mode;
		MBLineBuf->mb_skip_flag = currMBBuf->mb_skip_flag;

		if (cbpLuma>0 || cbpChroma>0 || mb_mode == I16_h264)
		{
			if (!g_active_pps_ptr->entropy_coding_mode_flag)
			{
				mb_qp_delta = READ_SE_V(stream);
			}else
			{
				mb_qp_delta = readDquant_CABAC_High();
			}

			currMBBuf->delta_qp = mb_qp_delta;

			g_ppa_buf_ptr->SYNTAX_BUF1 |= (mb_qp_delta << 17);
			inbuf[0] |= ((mb_qp_delta&0x3f) << 18);

 			coeff_vld_module();
		}
		else
		{
			MBLineBuf = vldLineBuf + currMBBuf->mb_x;

		 	MBLineBuf->nnz_y = 0;
			MBLineBuf->nnz_c = 0;
			MBLineBuf->coded_dc_flag = 0;

			{
				MB_BUF_T *tempMBBuf;	
				tempMBBuf = leftMBBuf;
				leftMBBuf = currMBBuf;
				currMBBuf = tempMBBuf;	
			}
			leftMBBuf->nnz_y0 = 0;
			leftMBBuf->nnz_y1 = 0;
			leftMBBuf->nnz_y2 = 0;
			leftMBBuf->nnz_y3 = 0;
			leftMBBuf->nnz_cb = 0;
			leftMBBuf->nnz_cr = 0;
			leftMBBuf->coded_dc_flag = 0;	
			
			//clear dct/io buffer only for verification
			memset (vsp_dct_io_0, 0, 256*sizeof(uint32));//weihu
		}
	}
	else //I PCM related syntax
	{
		int32 i,j;
		int32 bit_offset;

		

		bit_offset  = (stream->bitsLeft & 0x7);
		
		if(bit_offset)
		{
			READ_FLC(stream, bit_offset);
		}

		H264_Ipcm_parser ();
/*
		//Y
		for(j = 0; j < 16; j++)
		{
			for(i = 0; i < 16; i += 4)
			{
				READ_FLC(stream, 32);
			}
		}

		//U
		for(j = 0; j < 8; j++)
		{
			for(i = 0; i < 8; i += 4)
			{
				READ_FLC(stream, 32);
			}
		}	
		//V
		for(j = 0; j < 8; j++)
		{
			for(i = 0; i < 8; i += 4)
			{
				READ_FLC(stream, 32);
				
			}
		}*///weihu

		//need to store//weihu
		if (g_active_pps_ptr->entropy_coding_mode_flag)
		{
			arideco_start_decoding(g_image_ptr);
		}
		leftMBBuf->mb_type= currMBBuf->mb_type;
		leftMBBuf->nnz_y0 = 0x10101010;
		leftMBBuf->nnz_y1 = 0x10101010;
		leftMBBuf->nnz_y2 = 0x10101010;
		leftMBBuf->nnz_y3 = 0x10101010;
		leftMBBuf->nnz_cb = 0x10101010;
		leftMBBuf->nnz_cr = 0x10101010;
		leftMBBuf->delta_qp = 0;
		leftMBBuf->coded_dc_flag = 7;
		leftMBBuf->ref[0] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
		leftMBBuf->ref[1] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
		memset(leftMBBuf->mvd, 0, 32 * sizeof(int32));

		MBLineBuf = vldLineBuf + currMBBuf->mb_x;
		MBLineBuf->mb_type= currMBBuf->mb_type;
		MBLineBuf->nnz_y = 0x10101010;
		MBLineBuf->nnz_c = 0x10101010;
		MBLineBuf->coded_dc_flag = 7;
		memset(MBLineBuf->mvd, 0, 8 * sizeof(int32));
		MBLineBuf->ref[0] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
		MBLineBuf->ref[1] = (-1 << 24) | (-1 << 16) | (-1 << 8) | (-1); 
	}

	return;
}

void H264Dec_parse_MB_pred (int8 mb_type, BOOLEAN transform_size_8x8_flag)
{
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	BOOLEAN		prev_intra4x4_pred_mode_flag[16];
	BOOLEAN		prev_intra8x8_pred_mode_flag[4];
	int8		rem_intra4x4_pred_mode[16];
	int8		rem_intra8x8_pred_mode[4];
	int8		intra_chroma_pred_mode;
	uint8		slice_type;
	uint8		luma4x4BlkIdx, luma8x8BlkIdx, mbPartIdx;
	uint8		i, j, k, l;
	uint8		num_ref_idx_l0_active_minus1, num_ref_idx_l1_active_minus1;
	int32		ref_idx_l0[4], ref_idx_l1[4];
	int32		mvd_l0[16][2], mvd_l1[16][2];
	int32		ref_idx, mvd[2];
	int32		compIdx;
	uint32		*syntaxBufPtr;

	slice_type = (g_par_reg_ptr->PARSER_CFG0 >> 15) & 0x03;

	if (IS_I_NxN || IS_I_16x16)
	{
		if (!transform_size_8x8_flag && IS_I_NxN) //4x4
		{
			for(luma4x4BlkIdx=0; luma4x4BlkIdx<16; luma4x4BlkIdx++)
			{
				if (!g_active_pps_ptr->entropy_coding_mode_flag)
				{
					prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = READ_FLC(stream, 1);
				}
				else
				{
					prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = readPrev_intra_pred_mode_flag_CABAC_High();
				}
				
				g_ppa_buf_ptr->SYNTAX_BUF3 |= (prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]<<(2+luma4x4BlkIdx));
				inbuf[2] |= (prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]<<luma4x4BlkIdx);

				if (!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx])
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						rem_intra4x4_pred_mode[luma4x4BlkIdx]	= READ_FLC(stream, 3);
					}
					else
					{
						rem_intra4x4_pred_mode[luma4x4BlkIdx]	= readRem_intra_pred_mode_CABAC_High();
					}
					if (luma4x4BlkIdx < 8)
					{
						g_ppa_buf_ptr->SYNTAX_BUF4 |= (rem_intra4x4_pred_mode[luma4x4BlkIdx]<<(3*luma4x4BlkIdx));
						inbuf[3] |= (rem_intra4x4_pred_mode[luma4x4BlkIdx]<<(3*luma4x4BlkIdx));
					}
					else
					{
						g_ppa_buf_ptr->SYNTAX_BUF5 |= (rem_intra4x4_pred_mode[luma4x4BlkIdx]<<(3*(luma4x4BlkIdx-8)));
						inbuf[4] |= (rem_intra4x4_pred_mode[luma4x4BlkIdx]<<(3*(luma4x4BlkIdx-8)));//weihu
					}
					
				}
			}
		}
		else if (transform_size_8x8_flag && IS_I_NxN)//8x8
		{
			for(luma8x8BlkIdx=0; luma8x8BlkIdx<4; luma8x8BlkIdx++)
			{
				if (!g_active_pps_ptr->entropy_coding_mode_flag)
				{
					prev_intra8x8_pred_mode_flag[luma8x8BlkIdx] = READ_FLC(stream, 1);
				}
				else
				{
					prev_intra8x8_pred_mode_flag[luma8x8BlkIdx] = readPrev_intra_pred_mode_flag_CABAC_High();
				}
				g_ppa_buf_ptr->SYNTAX_BUF3 |= (prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]<<(18+luma8x8BlkIdx));
				inbuf[2] |= (prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]<<luma8x8BlkIdx);

				if (!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx])
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						rem_intra8x8_pred_mode[luma8x8BlkIdx]	= READ_FLC(stream, 3);
					}
					else
					{
						rem_intra8x8_pred_mode[luma8x8BlkIdx]	= readRem_intra_pred_mode_CABAC_High();
					}
					g_ppa_buf_ptr->SYNTAX_BUF4 |= (rem_intra8x8_pred_mode[luma8x8BlkIdx]<<(3*luma8x8BlkIdx));
					inbuf[3] |= (rem_intra8x8_pred_mode[luma8x8BlkIdx]<<(3*luma8x8BlkIdx));
				}
			}
		}

		if (!g_active_pps_ptr->entropy_coding_mode_flag)
		{
			intra_chroma_pred_mode = READ_UE_V(stream);
		}
		else
		{
			intra_chroma_pred_mode = readCIPredMode_CABAC_High();
		}
		currMBBuf->c_ipred_mode = intra_chroma_pred_mode;
		g_ppa_buf_ptr->SYNTAX_BUF3 |= intra_chroma_pred_mode;
		inbuf[2] |= (intra_chroma_pred_mode << 16);
	}
	else if ((slice_type!=B_SLICE)||(mb_type!=0))
	{
		int32 numPart;
		int32 widthPart;
		int32 heightPart;
		int32 numPartTbl[3] = {1, 2 ,2};
		int32 widthPartTbl[3] = {16, 16 ,8};
		int32 heightPartTbl[3] = {16, 8, 16};
		int32 mb_mode = currMBBuf->mb_mode;
		
		num_ref_idx_l0_active_minus1 = ((g_par_reg_ptr->PARSER_CFG0 >> 17) & 0x3f);
		num_ref_idx_l1_active_minus1 = ((g_par_reg_ptr->PARSER_CFG0 >> 23) & 0x3f);

		numPart = numPartTbl[mb_mode-1];
		widthPart = widthPartTbl[mb_mode-1];
		heightPart = heightPartTbl[mb_mode-1];

		//////////////////ref_idx decoding////////////////////

		for (mbPartIdx=0; mbPartIdx<numPart; mbPartIdx++)// list 0 decoding
		{
			int startPosj = (mbPartIdx != 0 && mb_mode == 2) ? 1 : 0; 
			int startPosi = (mbPartIdx != 0 && mb_mode == 3) ? 1 : 0; 

			if (MbPartPredMode(mb_type, mbPartIdx, slice_type)!=Pred_L1)
			{
				if ((num_ref_idx_l0_active_minus1>0))
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						if (num_ref_idx_l0_active_minus1 == 1)
						{
							ref_idx = READ_FLC(stream, 1);
							ref_idx = 1 - ref_idx;
						}
						else
						{
							ref_idx = READ_UE_V(stream);
						}
					}
					else
					{
						int block4x4Idx = 0;

						if (mb_mode == PB16x8_h264)
						{
							block4x4Idx = (mbPartIdx << 3);  // 0 or 8
						}
						else if (mb_mode == PB8x16_h264)
						{
							block4x4Idx = (mbPartIdx << 1);  // 0 or 2
						}

						ref_idx = readRefFrame_CABAC_High(block4x4Idx,0);
					}	
				}
				else
				{
					ref_idx = 0;
				}	
			}
			else
			{
				ref_idx = -1;
			}
			
			for (j=startPosj,k=0; k<(heightPart>>3); j++,k++)
			{
				for (i=startPosi,l=0; l<(widthPart>>3); i++,l++)
				{
					ref_idx_l0[j*2+i] = ref_idx;
				}
			}
			
			currMBBuf->ref[0] = ((ref_idx_l0[3] << 24) & 0xff000000) | 
								((ref_idx_l0[2] << 16) &0xff0000) | 
								((ref_idx_l0[1] << 8) & 0xff00) | ((ref_idx_l0[0] << 0) &0xff);
		}
		
		for (luma8x8BlkIdx=0; luma8x8BlkIdx<4; luma8x8BlkIdx++)
		{
			g_ppa_buf_ptr->SYNTAX_BUF4 |= (ref_idx_l0[luma8x8BlkIdx]<<(5*luma8x8BlkIdx));
			inbuf[2] |= ((ref_idx_l0[luma8x8BlkIdx]&0x1f)<<(5*luma8x8BlkIdx+12));
		}

		for (mbPartIdx=0; mbPartIdx<numPart; mbPartIdx++)// list 1 decoding, code almost same with list 0
		{
			int startPosj = (mbPartIdx != 0 && mb_mode == 2) ? 1 : 0; 
			int startPosi = (mbPartIdx != 0 && mb_mode == 3) ? 1 : 0; 
			
			if (MbPartPredMode(mb_type, mbPartIdx, slice_type)!=Pred_L0)
			{
				if (num_ref_idx_l1_active_minus1>0)
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						if (num_ref_idx_l1_active_minus1 == 1)
						{
							ref_idx = READ_FLC(stream, 1);
							ref_idx = 1 - ref_idx;
						}
						else
						{
							ref_idx = READ_UE_V(stream);
						}
					}
					else
					{
						int block4x4Idx = 0;

						if (mb_mode == PB16x8_h264)
						{
							block4x4Idx = (mbPartIdx << 3);  // 0 or 8
						}
						else if (mb_mode == PB8x16_h264)
						{
							block4x4Idx = (mbPartIdx << 1);  // 0 or 2
						}
						
						ref_idx = readRefFrame_CABAC_High(block4x4Idx,1);
					}
				}
				else
				{
					ref_idx = 0;
				}
			}
			else
			{
				ref_idx = -1;
			}

			for (j=startPosj,k=0; k<(heightPart>>3); j++,k++)
			{
				for (i=startPosi,l=0; l<(widthPart>>3); i++,l++)
				{
					ref_idx_l1[j*2+i] = ref_idx;
				}
			}

			currMBBuf->ref[1] = ((ref_idx_l1[3] << 24) & 0xff000000) | 
								((ref_idx_l1[2] << 16) &0xff0000) | 
								((ref_idx_l1[1] << 8) & 0xff00) | ((ref_idx_l1[0] << 0) &0xff);
		}
		
		for (luma8x8BlkIdx=0; luma8x8BlkIdx<4; luma8x8BlkIdx++)
		{
			g_ppa_buf_ptr->SYNTAX_BUF5 |= (ref_idx_l1[luma8x8BlkIdx]<<(5*luma8x8BlkIdx));
			inbuf[3] |= ((ref_idx_l1[luma8x8BlkIdx]&0x1f)<<(5*luma8x8BlkIdx));
		}

		//////////////////mv decoding////////////////////

		for (mbPartIdx=0; mbPartIdx<numPart; mbPartIdx++)// list 0 mv decoding
		{
			int startPosj = (mbPartIdx != 0 && mb_mode == 2) ? 2 : 0; 
			int startPosi = (mbPartIdx != 0 && mb_mode == 3) ? 2 : 0; 

			if (MbPartPredMode(mb_type, mbPartIdx, slice_type)!=Pred_L1)
			{
				for (compIdx=0; compIdx<2; compIdx++)
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						mvd[compIdx] = READ_SE_V(stream);
					}
					else
					{
						int block4x4Idx = 0;

						if (mb_mode == PB16x8_h264)
						{
							block4x4Idx = (mbPartIdx << 3);  // 0 or 8
						}
						else if (mb_mode == PB8x16_h264)
						{
							block4x4Idx = (mbPartIdx << 1);  // 0 or 2
						}

						mvd[compIdx] = readMVD_CABAC_High(block4x4Idx, 0, compIdx);	
					}
				}		
			}
			else
			{
				mvd[0] = mvd[1] = 0;
			}

			for (j=startPosj,k=0; k<(heightPart>>2); j++,k++)
			{
				for (i=startPosi,l=0; l<(widthPart>>2); i++,l++)
				{
					for (compIdx=0; compIdx<2; compIdx++)
					{
						mvd_l0[j*4+i][compIdx] = mvd[compIdx];
					}
					currMBBuf->mvd[j*4+i][0] = (mvd[1] << 16) | (mvd[0] & 0xffff);
				}
			}
		}
		
		syntaxBufPtr = &(g_ppa_buf_ptr->SYNTAX_BUF6);
		for (luma4x4BlkIdx=0; luma4x4BlkIdx<16; luma4x4BlkIdx++)
		{
			inbuf[4+luma4x4BlkIdx] = ((mvd_l0[b4order[luma4x4BlkIdx]][0]<<13) | ((mvd_l0[b4order[luma4x4BlkIdx]][1]&0x1fff)<<0));
			(*syntaxBufPtr) |= ((mvd_l0[luma4x4BlkIdx][0]<<0) | (mvd_l0[luma4x4BlkIdx][1]<<13));
			syntaxBufPtr++;
		}

		for (mbPartIdx=0; mbPartIdx<numPart; mbPartIdx++)// list 1 mv decoding
		{
			int startPosj = (mbPartIdx != 0 && mb_mode == 2) ? 2 : 0; 
			int startPosi = (mbPartIdx != 0 && mb_mode == 3) ? 2 : 0; 

			if (MbPartPredMode(mb_type, mbPartIdx, slice_type)!=Pred_L0)
			{
				for (compIdx=0; compIdx<2; compIdx++)
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						mvd[compIdx] = READ_SE_V(stream);
					}
					else
					{
						int block4x4Idx = 0;

						if (mb_mode == PB16x8_h264)
						{
							block4x4Idx = (mbPartIdx << 3);  // 0 or 8
						}
						else if (mb_mode == PB8x16_h264)
						{
							block4x4Idx = (mbPartIdx << 1);  // 0 or 2
						}

						mvd[compIdx] = readMVD_CABAC_High(block4x4Idx, 1, compIdx);	
					}
				}
			}
			else
			{
				mvd[0] = mvd[1] = 0;
			}

			for (j=startPosj,k=0; k<(heightPart>>2); j++,k++)
			{
				for (i=startPosi,l=0; l<(widthPart>>2); i++,l++)
				{
					for (compIdx=0; compIdx<2; compIdx++)
					{
						mvd_l1[j*4+i][compIdx] = mvd[compIdx];
					}
					currMBBuf->mvd[j*4+i][1] = (mvd[1] << 16) | (mvd[0] & 0xffff);
				}
			}		
		}

		syntaxBufPtr = &(g_ppa_buf_ptr->SYNTAX_BUF22);
		for (luma4x4BlkIdx=0; luma4x4BlkIdx<16; luma4x4BlkIdx++)
		{
			inbuf[20+luma4x4BlkIdx] = ((mvd_l1[b4order[luma4x4BlkIdx]][0]<<13) | ((mvd_l1[b4order[luma4x4BlkIdx]][1]&0x1fff)<<0));
			(*syntaxBufPtr) |= ((mvd_l1[luma4x4BlkIdx][0]<<0) | (mvd_l1[luma4x4BlkIdx][1]<<13));
			syntaxBufPtr++;
		}
	}	
}

int32 MbPartPredMode(int8 mb_type, int8 mbPartIdx, uint8 slice_type)
{
	int32 mbTypeIdxGroup;
	int32 mbTypeIdxOff;

	if (slice_type == P_SLICE)
	{
		return Pred_L0;
	}
	else
	{
		mbTypeIdxGroup = (mb_type >> 2);
		mbTypeIdxOff   = (mb_type % 4);
		
		if (mbPartIdx == 0)
		{
			if (mbTypeIdxGroup > 3)
			{
				return BiPred;
			}
			else if (mbTypeIdxGroup > 0)
			{
				return (mbTypeIdxOff<=1) ? Pred_L0 : Pred_L1;
			}
			else
			{
				return mbTypeIdxOff-1;
			}
		}
		else
		{
			if (mbTypeIdxGroup == 3 || mbTypeIdxGroup > 4)
			{
				return BiPred;
			}
			else if (mbTypeIdxGroup == 1 || mbTypeIdxGroup == 4)
			{
				return (mbTypeIdxOff<=1) ? Pred_L0 : Pred_L1;
			}
			else
			{
				return (mbTypeIdxOff>1) ? Pred_L0 : Pred_L1;
			}
		}
	}
}

void H264Dec_parse_subMB_pred (int8 mb_type, int8 *sub_mb_type)
{
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	uint8		mbPartIdx, subMbPartIdx;
	uint8		num_ref_idx_l0_active_minus1, num_ref_idx_l1_active_minus1;
	int32		ref_idx_l0[4], ref_idx_l1[4];
	int32		mvd_l0[16][2], mvd_l1[16][2];
	int32		mvd[2];
	int32		compIdx;
	uint8		slice_type;
	uint8		i, j, blkIdx, k, l;
	uint32		*syntaxBufPtr;
	int			b8modeTbl [13] = {4, 0, 0, 0, 1, 2, 1, 2, 1, 2, 3, 3, 3};
	int			numbSubPartTbl[5] = {1, 2, 2, 4, 4};
	int32		widthSubPartTbl[5] = {8, 8 ,4, 4, 4};
	int32		heightSubPartTbl[5] = {8, 4, 8, 4, 4};
	int			numbSubPart;
	int			wSubPart, hSubPart;

	slice_type = (g_par_reg_ptr->PARSER_CFG0 >> 15) & 0x03;
	num_ref_idx_l0_active_minus1 = ((g_par_reg_ptr->PARSER_CFG0 >> 17) & 0x3f);
	num_ref_idx_l1_active_minus1 = ((g_par_reg_ptr->PARSER_CFG0 >> 23) & 0x3f);

	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		if (!g_active_pps_ptr->entropy_coding_mode_flag)
		{
			sub_mb_type[mbPartIdx] = READ_UE_V(stream);
		}
		else
		{
			sub_mb_type[mbPartIdx] = readB8_typeInfo_CABAC_High();
		}
		if (slice_type == P_SLICE)
		{
			currMBBuf->sub_mb_mode[mbPartIdx] = sub_mb_type[mbPartIdx];
		}
		else //B_SLICE
		{
			currMBBuf->sub_mb_mode[mbPartIdx] = b8modeTbl[sub_mb_type[mbPartIdx]];
		}
		inbuf[2] |= (currMBBuf->sub_mb_mode[mbPartIdx] << (3*mbPartIdx));
	}

	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		if (((slice_type!=P_SLICE)||(mb_type!=4)) //mb_type!=P_8x8ref0
			&&((slice_type!=B_SLICE)||(sub_mb_type[mbPartIdx]!=0))
			&&SubMbPartPredMode(sub_mb_type[mbPartIdx], slice_type)!=Pred_L1)
		{
			if (num_ref_idx_l0_active_minus1 > 0)
			{
				if (!g_active_pps_ptr->entropy_coding_mode_flag)
				{
					if (num_ref_idx_l0_active_minus1 == 1)
					{
						ref_idx_l0[mbPartIdx] = READ_FLC(stream, 1);
						ref_idx_l0[mbPartIdx] = 1 - ref_idx_l0[mbPartIdx];
					}
					else
					{
						ref_idx_l0[mbPartIdx] = READ_UE_V(stream);
					}
				}
				else
				{
					int block4x4Idx = (mbPartIdx/2)*8 + (mbPartIdx&1)*2;
								  
					ref_idx_l0[mbPartIdx] = readRefFrame_CABAC_High(block4x4Idx,0);
				}
			}
			else
			{
				ref_idx_l0[mbPartIdx] = 0;
			}		
		}
		else
		{
			if((slice_type==P_SLICE)&&(mb_type==4))//mb_type!=P_8x8ref0
                ref_idx_l0[mbPartIdx] = 0;
			else
			    ref_idx_l0[mbPartIdx] = -1;
		}
		currMBBuf->ref[0] = ((ref_idx_l0[3] << 24) & 0xff000000) | 
							((ref_idx_l0[2] << 16) &0xff0000) | 
							((ref_idx_l0[1] << 8) & 0xff00) | ((ref_idx_l0[0] << 0) &0xff);
	}

	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		g_ppa_buf_ptr->SYNTAX_BUF4 |= (ref_idx_l0[mbPartIdx]<<(5*mbPartIdx));
		inbuf[2] |= ((ref_idx_l0[mbPartIdx]&0x1f)<<(5*mbPartIdx+12));
	}

	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		if (((slice_type!=B_SLICE)||(sub_mb_type[mbPartIdx]!=0))
			&&SubMbPartPredMode(sub_mb_type[mbPartIdx], slice_type)!=Pred_L0)
		{
			if (num_ref_idx_l1_active_minus1>0)
			{
				if (!g_active_pps_ptr->entropy_coding_mode_flag)
				{
					if (num_ref_idx_l1_active_minus1 == 1)
					{
						ref_idx_l1[mbPartIdx] = READ_FLC(stream, 1);
						ref_idx_l1[mbPartIdx] = 1 - ref_idx_l1[mbPartIdx];//weihu
					}
					else
					{
						ref_idx_l1[mbPartIdx] = READ_UE_V(stream);
					}
				}
				else
				{
					int block4x4Idx = (mbPartIdx/2)*8 + (mbPartIdx&1)*2;
								  
					ref_idx_l1[mbPartIdx] = readRefFrame_CABAC_High(block4x4Idx,1);
				}
			}
			else
			{
				ref_idx_l1[mbPartIdx] = 0;
			}			
		}
		else
		{
			ref_idx_l1[mbPartIdx] = -1;
		}
		currMBBuf->ref[1] = ((ref_idx_l1[3] << 24) & 0xff000000) | 
							((ref_idx_l1[2] << 16) &0xff0000) | 
							((ref_idx_l1[1] << 8) & 0xff00) | ((ref_idx_l1[0] << 0) &0xff);
	}

	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		g_ppa_buf_ptr->SYNTAX_BUF5 |= (ref_idx_l1[mbPartIdx]<<(5*mbPartIdx));
		inbuf[3] |= ((ref_idx_l1[mbPartIdx]&0x1f)<<(5*mbPartIdx));
	}
	////list 0 mv decoding
	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		int sub_mb_mode = currMBBuf->sub_mb_mode[mbPartIdx];
		int b8_y = (mbPartIdx >> 1);
		int b8_x = (mbPartIdx % 2);
		int b8_offset = (b8_y << 3) + (b8_x << 1);
		int startPosj, startPosi; 
		numbSubPart = numbSubPartTbl[sub_mb_mode];
		wSubPart	= widthSubPartTbl[sub_mb_mode];
		hSubPart	= heightSubPartTbl[sub_mb_mode];

		if (((slice_type!=B_SLICE)||(sub_mb_type[mbPartIdx]!=0))
			&&SubMbPartPredMode(sub_mb_type[mbPartIdx], slice_type)!=Pred_L1)
		{
			for (subMbPartIdx=0; subMbPartIdx<numbSubPart; subMbPartIdx++)
			{
				for (compIdx=0; compIdx<2; compIdx++)
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						mvd[compIdx] = READ_SE_V(stream);
					}
					else
					{
						int block4x4Idx;

						if (sub_mb_mode == SUB_8X8)
						{
							block4x4Idx = b8_offset;
						}
						else if (sub_mb_mode == SUB_8X4)
						{
							block4x4Idx = (subMbPartIdx << 2) + b8_offset;
						}
						else if (sub_mb_mode == SUB_4X8)
						{
							block4x4Idx = subMbPartIdx + b8_offset;
						}
						else
						{
							int b4_offset = ((subMbPartIdx >> 1) << 2) + (subMbPartIdx % 2);

							block4x4Idx = b4_offset + b8_offset;
						}

						mvd[compIdx] = readMVD_CABAC_High(block4x4Idx, 0, compIdx);	
					}
				}

				if (subMbPartIdx == 0)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_4X8)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = 1+(mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_8X4)
				{
					startPosj = 1+(mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else//4x4
				{
					startPosj = (subMbPartIdx/2)+(mbPartIdx/2)*2; 
					startPosi = (subMbPartIdx&1)+(mbPartIdx&1)*2;
				}
				for (j=startPosj,k=0; k<(hSubPart>>2); j++,k++)
				{
					for (i=startPosi,l=0; l<(wSubPart>>2); i++,l++)
					{
						for (compIdx=0; compIdx<2; compIdx++)
						{
							mvd_l0[j*4+i][compIdx] = mvd[compIdx];
						}
						currMBBuf->mvd[j*4+i][0] = (mvd[1] << 16) | (mvd[0] & 0xffff);
					}
				}
			}	
		}
		else
		{
			for (subMbPartIdx=0; subMbPartIdx<numbSubPart; subMbPartIdx++)
			{
				if (subMbPartIdx == 0)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_4X8)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = 1+(mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_8X4)
				{
					startPosj = 1+(mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else//4x4
				{
					startPosj = (subMbPartIdx/2)+(mbPartIdx/2)*2; 
					startPosi = (subMbPartIdx&1)+(mbPartIdx&1)*2;
				}
				for (j=startPosj,k=0; k<(hSubPart>>2); j++,k++)
				{
					for (i=startPosi,l=0; l<(wSubPart>>2); i++,l++)
					{
						for (compIdx=0; compIdx<2; compIdx++)
						{
							mvd_l0[j*4+i][compIdx] = 0;
						}
						currMBBuf->mvd[j*4+i][0] = 0;
					}
				}
			}
		}	
	}	

	syntaxBufPtr = &(g_ppa_buf_ptr->SYNTAX_BUF6);
	for (blkIdx=0; blkIdx<16; blkIdx++)
	{
		inbuf[ 4+blkIdx] = ((mvd_l0[b4order[blkIdx]][0]<<13) | ((mvd_l0[b4order[blkIdx]][1]&0x1fff)<<0));
		(*syntaxBufPtr) |= ((mvd_l0[blkIdx][0]<<0) | (mvd_l0[blkIdx][1]<<13));
		syntaxBufPtr++;
	}
	////list 1 mv decoding
	for (mbPartIdx=0; mbPartIdx<4; mbPartIdx++)
	{
		int sub_mb_mode = currMBBuf->sub_mb_mode[mbPartIdx];
		int b8_y = (mbPartIdx >> 1);
		int b8_x = (mbPartIdx % 2);
		int b8_offset = (b8_y << 3) + (b8_x << 1);
		int startPosj, startPosi; 
		numbSubPart = numbSubPartTbl[sub_mb_mode];
		wSubPart	= widthSubPartTbl[sub_mb_mode];
		hSubPart	= heightSubPartTbl[sub_mb_mode];

		if (((slice_type!=B_SLICE)||(sub_mb_type[mbPartIdx]!=0))
			&&SubMbPartPredMode(sub_mb_type[mbPartIdx], slice_type)!=Pred_L0)
		{
			for (subMbPartIdx=0; subMbPartIdx<numbSubPart; subMbPartIdx++)
			{
				for (compIdx=0; compIdx<2; compIdx++)
				{
					if (!g_active_pps_ptr->entropy_coding_mode_flag)
					{
						mvd[compIdx] = READ_SE_V(stream);
					}
					else
					{
						int block4x4Idx;

						if (sub_mb_mode == SUB_8X8)
						{
							block4x4Idx = b8_offset;
						}
						else if (sub_mb_mode == SUB_8X4)
						{
							block4x4Idx = (subMbPartIdx << 2) + b8_offset;
						}
						else if (sub_mb_mode == SUB_4X8)
						{
							block4x4Idx = subMbPartIdx + b8_offset;
						}
						else
						{
							int b4_offset = ((subMbPartIdx >> 1) << 2) + (subMbPartIdx % 2);

							block4x4Idx = b4_offset + b8_offset;
						}

						mvd[compIdx] = readMVD_CABAC_High(block4x4Idx, 1, compIdx);	
					}
				}

				if (subMbPartIdx == 0)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_4X8)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = 1+(mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_8X4)
				{
					startPosj = 1+(mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else//4x4
				{
					startPosj = (subMbPartIdx/2)+(mbPartIdx/2)*2; 
					startPosi = (subMbPartIdx&1)+(mbPartIdx&1)*2;
				}

				for (j=startPosj,k=0; k<(hSubPart>>2); j++,k++)
				{
					for (i=startPosi,l=0; l<(wSubPart>>2); i++,l++)
					{
						for (compIdx=0; compIdx<2; compIdx++)
						{
							mvd_l1[j*4+i][compIdx] = mvd[compIdx];
						}
						currMBBuf->mvd[j*4+i][1] = (mvd[1] << 16) | (mvd[0] & 0xffff);
					}
				}
			}
			
		}
		else
		{
			for (subMbPartIdx=0; subMbPartIdx<numbSubPart; subMbPartIdx++)
			{
				if (subMbPartIdx == 0)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_4X8)
				{
					startPosj = (mbPartIdx/2)*2; 
					startPosi = 1+(mbPartIdx&1)*2;
				}
				else if (sub_mb_mode == SUB_8X4)
				{
					startPosj = 1+(mbPartIdx/2)*2; 
					startPosi = (mbPartIdx&1)*2;
				}
				else//4x4
				{
					startPosj = (subMbPartIdx/2)+(mbPartIdx/2)*2; 
					startPosi = (subMbPartIdx&1)+(mbPartIdx&1)*2;
				}

				for (j=startPosj,k=0; k<(hSubPart>>2); j++,k++)
				{
					for (i=startPosi,l=0; l<(wSubPart>>2); i++,l++)
					{
						for (compIdx=0; compIdx<2; compIdx++)
						{
							mvd_l1[j*4+i][compIdx] = 0;
						}
						currMBBuf->mvd[j*4+i][1] = 0;
					}
				}
			}	
		}
	}

	syntaxBufPtr = &(g_ppa_buf_ptr->SYNTAX_BUF22);
	for (blkIdx=0; blkIdx<16; blkIdx++)
	{
		inbuf[20+blkIdx] = ((mvd_l1[b4order[blkIdx]][0]<<13) | ((mvd_l1[b4order[blkIdx]][1]&0x1fff)<<0));
		(*syntaxBufPtr) |= ((mvd_l1[blkIdx][0]<<0) | (mvd_l1[blkIdx][1]<<13));
		syntaxBufPtr++;
	}
}

int32 SubMbPartPredMode(int8 sub_mb_type, uint8 slice_type)
{

	if (slice_type == P_SLICE)
	{
		return Pred_L0;
	}
	else
	{	
		if (sub_mb_type>=1&&sub_mb_type<=3)
		{
			return sub_mb_type-1;
		}
		else if (sub_mb_type>=10&&sub_mb_type<=12)
		{
			return sub_mb_type-10;
		}
		else
		{
			return ((sub_mb_type-4)>>1);
		}
	}
}

void Interpret_mb_mode_I (int8 mb_type)
{
	if (mb_type == 0)
	{
		currMBBuf->mb_mode = INxN_h264;
	}
	else if (mb_type == 25)
	{
		currMBBuf->mb_mode = IPCM_h264;
	}
	else
	{
		currMBBuf->mb_mode = I16_h264;
		g_ppa_buf_ptr->SYNTAX_BUF4 = ((mb_type-1)&0x3);
	}

	return;
}

void Interpret_mb_mode_P (int8 mb_type)
{
	if (mb_type < 3)
	{
		currMBBuf->mb_mode = mb_type+1;
	}
	else if ((mb_type == 3) || (mb_type == 4))
	{
		currMBBuf->mb_mode = PB8X8_h264;
		if (mb_type == 4)
		{
			g_ppa_buf_ptr->SYNTAX_BUF4 = 0;
		}
	}
	else if (mb_type == 5)
	{
		currMBBuf->mb_mode = INxN_h264;
	}
	else if (mb_type == 30)
	{
		currMBBuf->mb_mode = IPCM_h264;
	}
	else
	{
		mb_type -= 6;
		currMBBuf->mb_mode = I16_h264;
		g_ppa_buf_ptr->SYNTAX_BUF4 = (mb_type&0x3);
	}

	return;
}

static const int32 offset2pdir16x16_tbl[12]   = {0, 0, 1, 2, 0,0,0,0,0,0,0,0};
static const int32 offset2pdir16x8_tbl[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
                                         {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}};
static const int32 offset2pdir8x16_tbl[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
                                         {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}};

void Interpret_mb_mode_B (int8 mb_type)
{
	int32 mbmode;

	//set mbtype, b8type, and b8pdir
	if (mb_type== 0) //B_Skip,B_Direct_16*16
	{
		mbmode = 0;
	}
	else if (mb_type == 23) //intra4x4
	{
		mbmode = INxN_h264;
	}
	else if ((mb_type > 23) && (mb_type < 48)) //intra16x16
	{
		mbmode = I16_h264; 
		g_ppa_buf_ptr->SYNTAX_BUF4 = (mb_type-24)&0x3;
	}
	else if (mb_type == 22) //8x8(+split)
	{
		mbmode = PB8X8_h264;
	}
	else if (mb_type < 4) //16x16
	{
		mbmode = PB16x16_h264;
	}
	else if (mb_type == 48)
	{
		mbmode = IPCM_h264;
	}
	else if (mb_type%2==0) //16x8
	{
		mbmode = PB16x8_h264;
	}
	else
	{
		mbmode = PB8x16_h264;
	}

	currMBBuf->mb_mode = mbmode;

	return;
}



void H264_Ipcm_parser ()
{
	int		i;
	int		bit_cnt;
	int		flush_bits;
	int		left_bits_byte;
	int		bsm_ipcm_data;
	uint8	pix0;
	uint8	pix1;
	uint32	ipcm_dbuf_wdata;
	uint8	ipcm_dbuf_addr;
	int		blk_type;
	int		x_cor;
	int		y_cor;
	int		offset;
	int		blk4x4_x_id;
	int		blk4x4_y_id;
	int		blk8x8_x_id;
	int		blk8x8_y_id;
	int		blk8x8_id;
	int		blk4x4_id;
	int		x_cor_blk;		//x offset from block 4x4 start point
	int		y_cor_blk;		//y offset from block 4x4 start point
	DEC_BS_T	*stream = g_image_ptr->bitstrm_ptr;
	
	//bsm_ipcm_data = show_nbits (32);
	
	
	//byte align	
//	bit_cnt			= g_bsm_reg_ptr->TOTAL_BITS; // g_bitstream->bitcnt;
//	left_bits_byte	= 8 - (bit_cnt & 7);
//	flush_bits		= (left_bits_byte == 8) ? 0 : left_bits_byte;
	
//	flush_nbits (flush_bits);
	
	
	for (i = 0; i < 192; i++)
	{
		//read 16 bits from bitstream
		bsm_ipcm_data = READ_FLC(stream, 16);//show_nbits (32);
		pix0		  = (bsm_ipcm_data >> 8) & 0xff;
		pix1		  = (bsm_ipcm_data >> 0) & 0xff;
		
		//flush_nbits (16);
		
		//write to dct/io buffer
		ipcm_dbuf_wdata = (pix1 << 16) | (pix0 << 0);
		
		if ((i & 0x80) == 0)
		{
			blk_type = BLK_LUMA;
			offset	= i;
		}
		else if((i & 0x20) == 0)
		{
			blk_type = BLK_CB;
			offset = i & 0x1f;
		}
		else
		{
			blk_type = BLK_CR;
			offset = i & 0x1f;
		}
		
		if (blk_type == BLK_LUMA)
		{
			x_cor = offset & 0x7;
			y_cor = offset >> 3;
		}
		else
		{
			x_cor = offset & 0x3;
			y_cor = offset >> 2;
		}
		
		blk4x4_x_id = x_cor >> 1;
		blk4x4_y_id = y_cor >> 2;
		
		blk8x8_x_id = blk4x4_x_id >> 1;
		blk8x8_y_id = blk4x4_y_id >> 1;
		
		blk8x8_id = (blk_type == BLK_LUMA) ? (blk8x8_y_id * 2 | blk8x8_x_id) :
		(blk_type == BLK_CB) ? 4 : 5;
		
		blk4x4_id = (blk8x8_id << 2) | ((blk4x4_y_id & 1) << 1) | (blk4x4_x_id & 1);
		
		x_cor_blk = x_cor & 0x1;
		y_cor_blk = y_cor & 0x3;
		
		ipcm_dbuf_addr = (blk4x4_id << 3) |  (y_cor_blk << 1) | x_cor_blk;
		
		vsp_dct_io_0[ipcm_dbuf_addr] = ipcm_dbuf_wdata;
	}
}
/*
void ppa_module(void)
{
}*/
#endif


















































































