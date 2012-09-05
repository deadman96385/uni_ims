/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
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
#ifdef WIN32
PUBLIC int32 get_unit (uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len, int32 *start_code_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
	static int32 declen = 0;
	static int32 s_bFisrtUnit = TRUE;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;
// 	uint8 *bfr = g_nalu_ptr->buf = pInStream;
	int32 *stream_ptr;// = (int32 *)bfr;
	int32 code;
	int32 byte_rest;

	ptr = pInStream;

//	SCI_TRACE_LOW("get_unit 0, frm_bs_len %d, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x\n",frm_bs_len,  
//		ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);

	//start code
	while ((data = *ptr++) == 0x00)
	{
		len++;
	}
	len++;
	*start_code_len = len;
	declen += len;
//	g_nalu_ptr->buf += (*start_code_len);

	byte_rest = (uint32)pInStream;
	byte_rest = ((byte_rest)>>2)<<2;	//word aligned
	
	//destuffing
	stream_ptr = /*(int32 *)bfr =*/ (int32 *)(byte_rest);
	g_nalu_ptr->buf = (uint8 *)(byte_rest);
//	len = 0;

	//read til next start code, and remove the third start code code emulation byte
	byte_rest = 4;
	declen = frm_bs_len - len;
	//while (declen < frm_bs_len)
	do
	{
		data = *ptr++;
//		len++;
//		declen++;
		
		if (zero_num < 2)
		{
			//*bfr++ = data; 
			zero_num++;
			byte_rest--;
			if (byte_rest >= 0)
			{
				code = (code <<8) | data;	//big endian
			}
			if (0 == byte_rest)
			{
				byte_rest = 4;
				*stream_ptr++ = code;
			}

			if(data != 0)
			{
				zero_num = 0;
			}
		}else
		{
			if ((zero_num == 2) && (data == 0x03))
			{
				zero_num = 0;
				stuffing_num++;
				goto next_data;
			}else
			{
				//*bfr++ = data; 
				byte_rest--;				
				if (byte_rest >= 0)
				{
					code = (code<<8) | data;	//big endian
				}
				if (0 == byte_rest)
				{
					byte_rest = 4;
					*stream_ptr++ = code;
				}

				if (data == 0x1)
				{
				//	if (zero_num >= 2)
					{
						startCode_len = zero_num + 1;
						break;
					}
				}else if (data == 0x00)
				{
					zero_num++;
				}else
				{
					zero_num = 0;
				}
			}
		}
next_data:
		declen--;
	}while(declen);

	if (((int32)stream_ptr) == (((((int32)ptr) - startCode_len/*len*/) >> 2) << 2))
	{
		g_need_back_last_word = 1;
		g_back_last_word = *stream_ptr;
	}else
	{
		g_need_back_last_word = 0;
	}
	
	*stream_ptr++ = code << (byte_rest*8);

	if (declen == 0)
	{
		declen = 1;
	}
	declen = frm_bs_len - declen;
	declen++;
	len = declen - len;
	
	*slice_unit_len = (declen - startCode_len /*+ stuffing_num*/);
	
 	g_nalu_ptr->len = len - startCode_len - stuffing_num;

	while (code && !(code&0xff))
	{
		g_nalu_ptr->len--;
		code >>= 8;

//		SCI_TRACE_LOW("code: %0x, nal->len: %d", code, g_nalu_ptr->len);
	}
	declen -= startCode_len;

	if (declen >= frm_bs_len)
	{
//		declen = 0;
		s_bFisrtUnit = TRUE;
		return 1;
	}

	return 0;
}
#endif

//#define DUMP_H264_ES

PUBLIC int32 get_unit_avc1 (uint8 *pInStream, int32 slice_unit_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
//	static int32 declen = 0;
//	static int32 s_bFisrtUnit = TRUE;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;
 	uint8 *bfr = g_nalu_ptr->buf = pInStream;

	ptr = pInStream;

//	SCI_TRACE_LOW("get_unit_avc1 0, frm_bs_len %d, %0x, %0x, %0x, %0x\n",slice_unit_len,  bfr[0], bfr[1], bfr[2], bfr[3]);

	//read til next start code, and remove the third start code code emulation byte
	while (len < slice_unit_len)
	{
		data = *ptr++;len++;

		if (zero_num < 2)
		{
			*bfr++ = data; 
			zero_num++;
			if(data != 0)
			{
				zero_num = 0;
			}
		}else
		{
#ifndef DUMP_H264_ES
			if ((zero_num == 2) && (data == 0x03))
			{
				zero_num = 0;
				stuffing_num++;
				continue;
			}else
#endif
			{
				*bfr++ = data; 				

				if (data == 0x1)
				{
					if (zero_num >= 2)
					{
						startCode_len = zero_num + 1;
						break;
					}
				}else if (data == 0x00)
				{
					zero_num++;
				}else
				{
					zero_num = 0;
				}
			}
		}
	}

//	SCI_TRACE_LOW("get_unit_avc1 1, len %d, stuffing_num %d\n", len, stuffing_num);

 	g_nalu_ptr->len = len -  stuffing_num;

	return 0;
}

LOCAL void init_dequant8_coeff_table(DEC_IMAGE_PARAMS_T *img_ptr){
    int i,j,q,x;

    for(i=0; i<2; i++ ){
        img_ptr->dequant8_coeff[i] = img_ptr->dequant8_buffer[i];
        for(j=0; j<i; j++){
            if(!memcmp(g_active_pps_ptr->ScalingList8x8[j],g_active_pps_ptr->ScalingList8x8[i], 64*sizeof(uint8))){
     			memcpy(img_ptr->dequant8_coeff[i], img_ptr->dequant8_coeff[j],52*64*sizeof(uint32));
                break;
            }
        }
        if(j<i)
            continue;

        for(q=0; q<52; q++){
			int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift =qpPerRem>>8; //qp_per
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<64; x++)
                img_ptr->dequant8_coeff[i][q][x] =
                    ((int32)dequant8_coeff_init[idx][ dequant8_coeff_init_scan[((x>>1)&12) | (x&3)] ] *
                   g_active_pps_ptr->ScalingList8x8[i][x]) << shift;
        }
    }
}

LOCAL void init_dequant4_coeff_table(DEC_IMAGE_PARAMS_T *img_ptr){
    int i,j,q,x;
    for(i=0; i<6; i++ ){
       img_ptr->dequant4_coeff[i] = img_ptr->dequant4_buffer[i];
        for(j=0; j<i; j++){
            if(!memcmp(g_active_pps_ptr->ScalingList4x4[j], g_active_pps_ptr->ScalingList4x4[i], 16*sizeof(uint8))){
  			 memcpy(img_ptr->dequant4_coeff[i], img_ptr->dequant4_buffer[j], 52*16*sizeof(uint32));
                break;
            }
        }
        if(j<i)
            continue;

        for(q=0; q<52; q++){
			int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift = (qpPerRem>>8)+ 2; //qp_per + 2
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<16; x++)
               img_ptr->dequant4_coeff[i][q][x] =
                    ((int32)dequant4_coeff_init[idx][(x&1) + ((x>>2)&1)] *
                   g_active_pps_ptr->ScalingList4x4[i][x]) << shift;
        }
    }
}

LOCAL void init_dequant_tables(DEC_IMAGE_PARAMS_T *img_ptr){
    int i,x;

    if(!g_active_pps_ptr->pic_scaling_matrix_present_flag)
    {
	memcpy(g_active_pps_ptr->ScalingList4x4, g_active_sps_ptr->ScalingList4x4,6*16*sizeof(uint8));
	memcpy(g_active_pps_ptr->ScalingList8x8, g_active_sps_ptr->ScalingList8x8,2*64*sizeof(uint8));
    }
    init_dequant4_coeff_table(img_ptr);
    if(g_active_pps_ptr->transform_8x8_mode_flag)
        init_dequant8_coeff_table(img_ptr);
    if(g_active_sps_ptr->qpprime_y_zero_transform_bypass_flag){
        for(i=0; i<6; i++)
            for(x=0; x<16; x++)
                img_ptr->dequant4_coeff[i][0][x] = 1<<6;
     if(g_active_pps_ptr->transform_8x8_mode_flag)
            for(i=0; i<6; i++)
                for(x=0; x<64; x++)
                    img_ptr->dequant8_coeff[i][0][x] = 1<<6;
    }
}

PUBLIC int32 H264Dec_process_slice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_NALU_T *nalu_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 curr_header;
	int32 new_picture;

	img_ptr->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
	img_ptr->nal_reference_idc = (nalu_ptr->nal_reference_idc);

	H264Dec_FirstPartOfSliceHeader (curr_slice_ptr, img_ptr);

	/*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
	position infomation(available, position) will change*/
#if 0
	if (g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
#endif		
	{
		g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

		//use PPS and SPS
		H264Dec_use_parameter_set (img_ptr, curr_slice_ptr->pic_parameter_set_id);
#if _H264_PROTECT_ & _LEVEL_LOW_
		if(img_ptr->error_flag)
		{
			img_ptr->return_pos1 |= (1<<25);
			return -1;
		}
#endif
		init_dequant_tables(img_ptr);
	}

	H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);

	if (H264Dec_FMO_init(img_ptr) == FALSE || img_ptr->error_flag)
	{
#if _H264_PROTECT_ & _LEVEL_LOW_	
		img_ptr->error_flag |= ER_BSM_ID;
        	img_ptr->return_pos1 |= (1<<26);
#endif
		return -1;
	}

	new_picture = H264Dec_is_new_picture (img_ptr);
	img_ptr->is_new_pic = new_picture;

	if (new_picture)
	{
		H264Dec_init_picture (img_ptr);
	#if _H264_PROTECT_ & _LEVEL_LOW_
		if(img_ptr->error_flag )
		{
			img_ptr->return_pos1 |= (1<<27);
			return -1;		
		}
	#endif	
		curr_header = SOP;
	}else
	{
		curr_header = SOS;
	}

	H264Dec_init_list (img_ptr, img_ptr->type);
	H264Dec_reorder_list (img_ptr, curr_slice_ptr);

	if (img_ptr->is_cabac)
	{
		ff_init_cabac_decoder(img_ptr);	//arideco_start_decoding (img_ptr);
	}

//	if (new_picture)
//	if (img_ptr->VSP_used)
//	{
//		h264Dec_PicLevelSendRefAddressCommmand (img_ptr);
//	}
	img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;

	return curr_header;
}

PUBLIC void H264Dec_exit_slice (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;

	old_slice_ptr->frame_num = img_ptr->frame_num;
	old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
	old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
	old_slice_ptr->idr_flag = img_ptr->idr_flag;

	if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
	{
		old_slice_ptr->frame_num = -1;
	}

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

	if (img_ptr->VSP_used)
	{
		VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 17, 1, 1, "DBK_CTR1: polling dbk slice idle");
		VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

		VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "VSP_DBG: polling AHB idle");
		VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|VSP_DBG_WOFF));
	}
	
	return;
}

#if _DEBUG_
void foo2(void)
{
}
#endif

PUBLIC void set_ref_pic_num(DEC_IMAGE_PARAMS_T *img_ptr)
{
	int list ,i/*,j*/;
	int slice_id=img_ptr->slice_nr;

  	for (list = 0; list < img_ptr->list_count; list++)
  	{
		for (i=0;i<g_list_size[list];i++)
	  	{
			g_dec_picture_ptr->pic_num_ptr		  [slice_id][list][i] = g_list[list][i]->poc * 2 ;
	  	}
  	}
}

LOCAL void H264Dec_fill_wp_params (DEC_IMAGE_PARAMS_T *img_ptr, DEC_PPS_T *active_pps_ptr)
{
	int32 i, j/*, k*/;
  	int32 comp;
 	int32 log_weight_denom;
	int32 tb, td;
//  	int32 bframe = (img_ptr->type==B_SLICE);
  	int32 max_bwd_ref, max_fwd_ref;
  	int32 tx,DistScaleFactor;
  	
	if (active_pps_ptr->weighted_bipred_idc == 2)
	{
	    	img_ptr->luma_log2_weight_denom = 5;
	    	img_ptr->chroma_log2_weight_denom = 5;
	    	img_ptr->wp_round_luma = 16;
	    	img_ptr->wp_round_chroma = 16;

	    	for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
	    	{
	      		for (comp = 0; comp < 3; comp++)
	      		{
	        		log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
	        		g_wp_weight[0][i][comp] = 1<<log_weight_denom;
	        		g_wp_weight[1][i][comp] = 1<<log_weight_denom;
	        		g_wp_offset[0][i][comp] = 0;
	        		g_wp_offset[1][i][comp] = 0;
	      		}
	    	}
	 }
		
	max_fwd_ref = img_ptr->ref_count[0];
 	max_bwd_ref = img_ptr->ref_count[1];

    	for (i=0; i<max_fwd_ref; i++)
    	{
      		for (j=0; j<max_bwd_ref; j++)
      		{
        		for (comp = 0; comp<3; comp++)
        		{
        			log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
          			if (active_pps_ptr->weighted_bipred_idc == 1)
          			{
            				g_wbp_weight[0][i][j][comp] =  g_wp_weight[0][i][comp];
            				g_wbp_weight[1][i][j][comp] =  g_wp_weight[1][j][comp];
          			}else if (active_pps_ptr->weighted_bipred_idc == 2)
          			{
	            			td = Clip3(-128,127,g_list[1][j]->poc - g_list[0][i]->poc);
	            			if (td == 0 || g_list[1][j]->is_long_term || g_list[0][i]->is_long_term)
	            			{
	              				g_wbp_weight[0][i][j][comp] =   32;
	              				g_wbp_weight[1][i][j][comp] =   32;
	            			}else
	            			{
	              				tb = Clip3(-128,127,img_ptr->ThisPOC - g_list[0][i]->poc);
	              				tx = (16384 + ABS(td/2))/td;
	              				DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);

	              				g_wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
	              				g_wbp_weight[0][i][j][comp] = 64 - g_wbp_weight[1][i][j][comp];
	              				if (g_wbp_weight[1][i][j][comp] < -64 || g_wbp_weight[1][i][j][comp] > 128)
	              				{
	                				g_wbp_weight[0][i][j][comp] = 32;
	                				g_wbp_weight[1][i][j][comp] = 32;
	                				g_wp_offset[0][i][comp] = 0;
	                				g_wp_offset[1][j][comp] = 0;
	              				}
	            			}
          			}
        		}
      		}
   	}
}

void dump_yuv( uint8* pBuffer,int32 aInBufSize)
{
	FILE *fp = fopen("/data/video_frm132.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

void dump_yuv1( uint8* pBuffer,int32 aInBufSize)
{
	FILE *fp = fopen("/data/video_frm133.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

void dump_yuv2( uint8* pBuffer,int32 aInBufSize)
{
	FILE *fp = fopen("/data/video_frm134.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

void dump_yuv_all( uint8* pBuffer,int32 aInBufSize)
{
	FILE *fp = fopen("/data/video_dec.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

void dump_yuv_all1( uint8* pBuffer,int32 aInBufSize)
{
	FILE *fp = fopen("/data/video_dec_out.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

LOCAL void H264Dec_output_one_frame_BP (DEC_IMAGE_PARAMS_T *img_ptr, MMDecOutput * dec_out)
{
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr= g_dpb_ptr;
	DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic[0];
	DEC_STORABLE_PICTURE_T *cur = g_dec_picture_ptr;
	DEC_STORABLE_PICTURE_T *out ;
	int32 i;
#if 0
 	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		if (cur == dpb_ptr->fs[i]->frame)
		{ 
			if(dpb_ptr->fs[i]->is_reference == 0)
			{
				dpb_ptr->fs[i]->is_reference = DELAYED_PIC_REF;

#ifdef _VSP_LINUX_
				if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
				{
//					SCI_TRACE_LOW("bind in output_frame_func\t");
					(*VSP_bindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
				}
#endif
			}
		}
	}
#endif

		
	
#ifdef _VSP_LINUX_	
	dec_out->reqNewBuf = 1;				
#endif	
	dec_out->frame_width = img_ptr->width;
	dec_out->frame_height = img_ptr->height;
	
	if(!img_ptr->is_first_frame)
	{
		out = prev;
		dec_out->frameEffective =  1;
		dec_out->pOutFrameY = out->imgY;
		dec_out->pOutFrameU = out->imgU;
		dec_out->pOutFrameV = out->imgV;
#ifdef _VSP_LINUX_	
		dec_out->pBufferHeader = out->pBufferHeader;
#endif		
	}else
	{
		dec_out->frameEffective =  0;
		dec_out->pOutFrameY = PNULL;
		dec_out->pOutFrameU = PNULL;
		dec_out->pOutFrameV = PNULL;
#ifdef _VSP_LINUX_	
		dec_out->pBufferHeader = NULL;
/*
	 	for (i = 0; i < dpb_ptr->used_size; i++)
		{
			if (cur == dpb_ptr->fs[i]->frame)
			{ 
				//SCI_TRACE_LOW("bind in output_frame_func\t");
				(*VSP_bindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
			}
		}		
*/
#endif				
	}
//	SCI_TRACE_LOW("H264Dec_output_one_frame_BP:at frame num %d,  out put valid : %d, frame:%x ,header:%x\n",g_nFrame_dec_h264,dec_out->frameEffective,out,out?out->pBufferHeader:0 );
//	SCI_TRACE_LOW("H264Dec_output_one_frame_BP:decode picture : %x,header: %x\n",g_dec_picture_ptr,g_dec_picture_ptr->pBufferHeader );

	dpb_ptr->delayed_pic[0] = cur;
	dpb_ptr->delayed_pic_num = 1;

		for (i = 0; i < /*dpb_ptr->used_size*/MAX_REF_FRAME_NUMBER; i++)
		{
			if (out == dpb_ptr->fs[i]->frame)
			{
				if(dpb_ptr->fs[i]->is_reference == DELAYED_PIC_REF)
				{
					dpb_ptr->fs[i]->is_reference = 0;

#ifdef _VSP_LINUX_
					if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
					{
//						SCI_TRACE_LOW("unbind in output_frame_func\t");
						(*VSP_unbindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
						dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
					}
#endif
				}
			}
		}

	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		if (cur == dpb_ptr->fs[i]->frame)
		{ 
			//if(dpb_ptr->fs[i]->is_reference == 0)
			{
				//dpb_ptr->fs[i]->is_reference = DELAYED_PIC_REF;

#ifdef _VSP_LINUX_
				if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
				{
//					SCI_TRACE_LOW("bind in output_frame_func\t");
					(*VSP_bindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
				}
#endif
			}
		}
	}

	img_ptr->is_first_frame = FALSE;
}

LOCAL void H264Dec_output_one_frame (DEC_IMAGE_PARAMS_T *img_ptr, MMDecOutput * dec_out)
{
	DEC_VUI_T *vui_seq_parameters_ptr = g_sps_ptr->vui_seq_parameters;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr= g_dpb_ptr;
	DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic_ptr;
	DEC_STORABLE_PICTURE_T *cur = g_dec_picture_ptr;
	DEC_STORABLE_PICTURE_T *out = cur;
	int i, pics, cross_idr, out_of_order, out_idx;

	if(vui_seq_parameters_ptr->bitstream_restriction_flag && (img_ptr->has_b_frames < vui_seq_parameters_ptr->num_reorder_frames))
	{
		img_ptr->has_b_frames = vui_seq_parameters_ptr->num_reorder_frames;
//		s->low_delay = 0;
    	}

	//SCI_TRACE_LOW("dec poc: %d\t", cur->poc);

	dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = cur;
	pics = dpb_ptr->delayed_pic_num;
	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		if (cur == dpb_ptr->fs[i]->frame)
		{
			if(dpb_ptr->fs[i]->is_reference == 0)
			{
				dpb_ptr->fs[i]->is_reference = DELAYED_PIC_REF;

#ifdef _VSP_LINUX_
				if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
				{
//					SCI_TRACE_LOW("bind in output_frame_func\t");
					(*VSP_bindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
				}
#endif
			}
		}
	}

	cross_idr = 0;
	for(i=0; i < dpb_ptr->delayed_pic_num; i++)
	{
		if(dpb_ptr->delayed_pic[i]->idr_flag || (dpb_ptr->delayed_pic[i]->poc == 0))
		{
	    		cross_idr = 1;
		}
	}

	//find the smallest POC frame in dpb buffer
	out = dpb_ptr->delayed_pic[0];
	out_idx = 0;
	for(i=1; dpb_ptr->delayed_pic[i] && !dpb_ptr->delayed_pic[i]->idr_flag; i++)
	{
		if(dpb_ptr->delayed_pic[i]->poc < out->poc)
		{
	   		out = dpb_ptr->delayed_pic[i];
	   		out_idx = i;
		}
	}
	
	out_of_order = !cross_idr && prev && (out->poc < prev->poc);
	if(vui_seq_parameters_ptr->bitstream_restriction_flag && img_ptr->has_b_frames >= vui_seq_parameters_ptr->num_reorder_frames)
	{
	}else if(prev && pics <= img_ptr->has_b_frames)
	{
		out = prev;
	}else if((out_of_order && (pics-1) == img_ptr->has_b_frames && pics < 15/*why 15?, xwluo@20120316 */)  ||
		((g_sps_ptr->profile_idc != 0x42/*!bp*/)&&(img_ptr->low_delay) && ((!cross_idr && prev && out->poc > (prev->poc + 2)) || cur->slice_type == B_SLICE)))
    	{
		img_ptr->low_delay = 0;
        	img_ptr->has_b_frames++;
        	out = prev;
	} else if(out_of_order)
    	{
    		out = prev;
    	}
	dpb_ptr->delayed_pic_ptr = out;

	//flush one frame from dpb and re-organize the delayed_pic buffer
	if(/*out_of_order ||*/ pics > img_ptr->has_b_frames)
	{
		for(i=out_idx; dpb_ptr->delayed_pic[i]; i++)
		{
	    		dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
		}
		dpb_ptr->delayed_pic_num--;
	}

	dec_out->frameEffective = (prev == out) ? 0 : 1;
#ifdef _VSP_LINUX_	
	dec_out->reqNewBuf = 1;				
#endif
	if (dec_out->frameEffective)
	{
	    if (img_ptr->VSP_used)
        {
        	dec_out->frame_width = img_ptr->width;
			dec_out->frame_height = img_ptr->height;
        }else
		{
		#ifndef YUV_THREE_PLANE
			dec_out->frame_width = img_ptr->width;
			dec_out->frame_height = img_ptr->height;
		#else
			dec_out->frame_width = img_ptr->ext_width;
			dec_out->frame_height = img_ptr->ext_height;
		#endif
		}	
		dec_out->pOutFrameY = out->imgY;
		dec_out->pOutFrameU = out->imgU;
		dec_out->pOutFrameV = out->imgV;
#ifdef _VSP_LINUX_	
		dec_out->pBufferHeader = out->pBufferHeader;
#endif

		for (i = 0; i < /*dpb_ptr->used_size*/MAX_REF_FRAME_NUMBER; i++)
		{
			if (out == dpb_ptr->fs[i]->frame)
			{
				if(dpb_ptr->fs[i]->is_reference == DELAYED_PIC_REF)
				{
					dpb_ptr->fs[i]->is_reference = 0;

#ifdef _VSP_LINUX_
					if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
					{
//						SCI_TRACE_LOW("unbind in output_frame_func\t");
						(*VSP_unbindCb)(g_user_data,dpb_ptr->fs[i]->frame->pBufferHeader);
						dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
					}
#endif
				}
			}
		}
//		SCI_TRACE_LOW("out poc: %d\t", out->poc);
	}else
	{	
//		SCI_TRACE_LOW("out poc: %d\n", out->poc);
	}

//	for (i = 0; i < 16; i++)
//	{
//		SCI_TRACE_LOW("dpb_ptr->fs[%d]->is_reference =  %d", i, dpb_ptr->fs[i]->is_reference);
//	}

	return;
}

PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MMDecRet ret = MMDEC_ERROR;
	
	if (img_ptr->is_cabac)
	{
		init_contexts (img_ptr);
		cabac_new_slice();
	}

//	if ((active_pps_ptr->weighted_bipred_idc > 0 && (img_ptr->type == B_SLICE)) || (active_pps_ptr->weighted_pred_flag && img_ptr->type != I_SLICE))
	if ((img_ptr->type == B_SLICE) && ((active_pps_ptr->weighted_bipred_idc > 0) || (active_pps_ptr->weighted_pred_flag)))
	{
		H264Dec_fill_wp_params (img_ptr, active_pps_ptr);
	}	
	
	img_ptr->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
          || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

	if (img_ptr->VSP_used && img_ptr->apply_weights)
	{
		img_ptr->mbc_cfg_cmd = (img_ptr->luma_log2_weight_denom<<4) | (img_ptr->chroma_log2_weight_denom<<1) | 1;
	}else
	{
		img_ptr->mbc_cfg_cmd = 0;
	}
		
	if (curr_slice_ptr->picture_type == I_SLICE)
	{
		img_ptr->list_count = 0;
		if (img_ptr->VSP_used)
		{
			H264Dec_decode_one_slice_I_hw (img_ptr);
		}else
		{
			H264Dec_decode_one_slice_I_sw (img_ptr);
		}
	}else if (curr_slice_ptr->picture_type == P_SLICE)
	{
		img_ptr->list_count = 1;
		if (img_ptr->VSP_used)
		{
			H264Dec_decode_one_slice_P_hw (img_ptr);
		}else
		{
			H264Dec_decode_one_slice_P_sw (img_ptr);
		}
	}else if (curr_slice_ptr->picture_type == B_SLICE)
	{
	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (g_nFrame_dec_h264 < 2 || g_searching_IDR_pic)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos1 = (1<<30);
			return MMDEC_ERROR;
		}
	#endif	
		img_ptr->list_count = 2;

		if (!img_ptr->direct_spatial_mv_pred_flag)
		{
			int32 iref_max = mmin(img_ptr->ref_count[0], g_list_size[0]);
			const int32 poc = g_dec_picture_ptr->poc;
			const int32 poc1 = g_list[1][0]->poc;
			int32 i;
			
			for (i = 0; i < iref_max; i++)
			{
				int32 prescale, iTRb, iTRp;
				int32 poc0 = g_list[0][i]->poc;

				iTRp = Clip3( -128, 127,  (poc1 - poc0));

				if (iTRp!=0)
				{
					iTRb = Clip3( -128, 127, (poc - poc0 ));
					prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;
					img_ptr->dist_scale_factor[i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
				}else
				{
				  	img_ptr->dist_scale_factor[i] = 9999;
				}
			}
		}
	
		if (img_ptr->VSP_used)
		{
			H264Dec_decode_one_slice_B_hw (img_ptr);
		}else
		{
			H264Dec_decode_one_slice_B_sw (img_ptr);
		}
	}else
	{
	#if _H264_PROTECT_ & _LEVEL_LOW_
		img_ptr->error_flag |= ER_BSM_ID;
		img_ptr->return_pos2 = (1<<7);
	#endif	

		SCI_TRACE_LOW ("the other picture type is not supported!\n");
	}

#if _H264_PROTECT_ & _LEVEL_HIGH_
	if (img_ptr->error_flag)
	{
		SCI_TRACE_LOW("H264Dec_decode_one_slice_data: mb_x: %d, mb_y: %d, bit_cnt: %d, pos: %x, pos1: %0x, pos2: %0x, err_flag: %x\n",
			img_ptr->mb_x, img_ptr->mb_y, img_ptr->bitstrm_ptr->bitcnt, img_ptr->return_pos, img_ptr->return_pos1, img_ptr->return_pos2, img_ptr->error_flag);
		img_ptr->return_pos2 = (1<<8);
		g_searching_IDR_pic = 1;
		return MMDEC_ERROR;
	}
#endif	

	img_ptr->slice_nr++;

	if (SOP == curr_slice_ptr->next_header) //end of picture
	{
		if (img_ptr->VSP_used)
		{
			ret = H264Dec_Picture_Level_Sync (img_ptr);
			H264Dec_exit_picture (img_ptr);
			H264Dec_output_one_frame_BP(img_ptr,dec_output_ptr);			
		
			if (img_ptr->fmo_used)
			{
  		  	 	H264Dec_deblock_one_frame (img_ptr);
			}				
		}else
		{	
			H264Dec_exit_picture (img_ptr);	

			H264Dec_output_one_frame(img_ptr,dec_output_ptr);
		}		

		g_dec_picture_ptr = NULL;
		g_nFrame_dec_h264++;
	}

	return ret;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
