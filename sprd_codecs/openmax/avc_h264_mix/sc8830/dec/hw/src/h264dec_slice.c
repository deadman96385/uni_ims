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

int ref_list_buf[33];
int slice_info[50];

PUBLIC int32 H264Dec_Process_slice (DEC_NALU_T *nalu_ptr)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 curr_header;
	int32 new_picture;
	img_ptr->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
	img_ptr->nal_reference_idc = (nalu_ptr->nal_reference_idc);

	H264Dec_FirstPartOfSliceHeader (curr_slice_ptr, img_ptr);

	/*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
	position infomation(available, position) will change*/
	if (g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
	{
		g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

		//use PPS and SPS
		H264Dec_use_parameter_set (curr_slice_ptr->pic_parameter_set_id);
	}
	if(curr_slice_ptr->start_mb_nr>=img_ptr->frame_size_in_mbs)//for error
	{
		g_image_ptr->error_flag = TRUE;
		return -1;
	}

	H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);
#if _MVC_
	if(g_curr_slice_ptr->view_id >= 0)
	{
		g_curr_slice_ptr->p_Dpb = g_dpb_layer[g_curr_slice_ptr->view_id];
	}
#endif

        if (img_ptr->error_flag == TRUE)
	{
		return -1;
	}

	new_picture = H264Dec_is_new_picture (img_ptr);
    //new_picture |=  (g_dec_picture_ptr==NULL);  //for error stream//weihu

	if(g_dec_picture_ptr==NULL)//for error stream//weihu
        new_picture=TRUE;
	else if((img_ptr->curr_mb_nr <= (int)curr_slice_ptr->start_mb_nr)&&(curr_slice_ptr->start_mb_nr>0))//&&(img_ptr->curr_mb_nr>0)
		new_picture=FALSE;

	img_ptr->is_new_pic = new_picture;

	if (new_picture)
	{
		H264Dec_init_picture (img_ptr);
		curr_header = SOP;
	}else
	{
		curr_header = SOS;
	}

	H264Dec_init_list (img_ptr, img_ptr->type);
#if _MVC_
	if(g_curr_slice_ptr->svc_extension_flag == 0 || g_curr_slice_ptr->svc_extension_flag == 1)
		H264Dec_reorder_list_mvc ();
	else
#endif
 		H264Dec_reorder_list ();

	if (img_ptr->error_flag == TRUE)
            return -1;

	//configure ref_list_buf[24]
	{
		int i;

        //ref_list_buf[32]=0;
		memset(ref_list_buf, 0, 33*sizeof(int));

		for (i=0; i<img_ptr->num_ref_idx_l0_active; i++)//g_list_size[0]//weihu
		{
			ref_list_buf[32] |= ((g_list0[i]->is_long_term)<<i);
		}
		for (i=img_ptr->num_ref_idx_l0_active; i<MAX_REF_FRAME_NUMBER; i++)//g_list_size[0]
		{
			ref_list_buf[32] |= (1<<i);
		}
		for (i=0; i<g_list_size[1]; i++)
		{
			ref_list_buf[32] |= ((g_list1[i]->is_long_term)<<(i+16));
		}
		for (i=g_list_size[1]; i<MAX_REF_FRAME_NUMBER; i++)
		{
			ref_list_buf[32] |= (1<<(i+16));
		}

		for(i=0;i<img_ptr->num_ref_idx_l0_active;i++)//g_list_size[0]
		{
		   ref_list_buf[i] = g_list0[i]->poc;
		}
		for(i=0;i<g_list_size[1];i++)
		{
			ref_list_buf[i+16] = g_list1[i]->poc;
		}
	}

	img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;

	return curr_header;
}

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
	
#if _MVC_
	old_slice_ptr->view_id = g_curr_slice_ptr->view_id;
	old_slice_ptr->inter_view_flag = g_curr_slice_ptr->inter_view_flag;
	old_slice_ptr->anchor_pic_flag = g_curr_slice_ptr->anchor_pic_flag;
#endif
	
	return;
}

void H264Dec_Cfg_ScalingMatix (DEC_IMAGE_PARAMS_T *img_ptr)
{
    int i, j;
    	unsigned int tmp;


    if(g_active_pps_ptr->pic_scaling_matrix_present_flag || g_active_sps_ptr->seq_scaling_matrix_present_flag)
    {
        if(g_active_sps_ptr->seq_scaling_matrix_present_flag) // check sps first
        {
            for(i=0; i<8; i++)
            {
                if(i<6)
                {
                    if(!g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
                    {
			if(i==0)
                            SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                        else if(i==3)
                            SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                        else
                            SCI_MEMCPY(weightscale4x4[i], weightscale4x4[i-1], 16*sizeof(char));
                    }else
                    {
			if(g_active_sps_ptr->UseDefaultScalingMatrix4x4Flag[i])
			{
			    if (i<3)
			    {
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
			    }else
			    {
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
			    }
			}else
                            SCI_MEMCPY(weightscale4x4[i], g_active_sps_ptr->ScalingList4x4[i], 16*sizeof(char));
                    }
                }	else
		{
                    if(!g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
                    {
			if(i==6)
                            SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                        else
                            SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                    }else
                    {
			if(g_active_sps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
			{
			    if (i==6)
                            {
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
			    }else
			    {
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
			    }
			}else
                            SCI_MEMCPY(weightscale8x8[i-6], g_active_sps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
                    }
                }
            }
        }

        if(g_active_pps_ptr->pic_scaling_matrix_present_flag) // then check pps
        {
            for(i=0; i<8; i++)
            {
                if(i<6)
		{
                    if(!g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
                    {
                        if (i==0)
			{
                            if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                        }else if (i==3)
			{
                            if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
			}else
                            SCI_MEMCPY(weightscale4x4[i], weightscale4x4[i-1], 16*sizeof(char));
                    }else
                    {
			if(g_active_pps_ptr->UseDefaultScalingMatrix4x4Flag[i])
			{
			    if (i<3)
                            {
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                            }else
                            {
				SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                            }
                        }else
                            SCI_MEMCPY(weightscale4x4[i], g_active_pps_ptr->ScalingList4x4[i], 16*sizeof(char));
                    }
                }	else
                {
                    if(!g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
                    {
			if (i==6)
			{
                            if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                        }else
			{
			    if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                        }
                    }else
                    {
			if(g_active_pps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
			{
			    if (i==6)
                            {
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                            }else
                            {
				SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                            }
                        }else
                            SCI_MEMCPY(weightscale8x8[i-6], g_active_pps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
                    }
                }
            }
        }
     }

    		if (img_ptr->apply_weights)
		{		
			for (i=0; i<2; i++)
			{
				for (j=0; j<16; j++)
				{      	
					tmp=(g_wp_weight[i][j][2]<<18)|((g_wp_weight[i][j][1]&0x1ff)<<9)|(g_wp_weight[i][j][0]&0x1ff);
					OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+136+(2*j+32*i)*4,tmp,"g_wp_weight");				
					tmp=(g_wp_offset[i][j][2]<<16)|((g_wp_offset[i][j][1]&0xff)<<8)|(g_wp_offset[i][j][0]&0xff);
					OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+140+(2*j+32*i)*4,tmp,"g_wp_offset");
					
				}
			}
		}//weihu
	
		if(g_active_pps_ptr->pic_scaling_matrix_present_flag || g_active_sps_ptr->seq_scaling_matrix_present_flag)
		{
			for(i=3;i<6;i++)//inter
			{		
				for(j=0;j<4;j++)
				{
					tmp=((weightscale4x4[i][j][3]&0xff)<<24)|((weightscale4x4[i][j][2]&0xff)<<16)|((weightscale4x4[i][j][1]&0xff)<<8)|((weightscale4x4[i][j][0]&0xff)<<0);
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER4x4Y_OFF+(i-3)*16+j*4,tmp,"weightscale inter4x4");
				}
			}
			
			for(i=0;i<3;i++)//intra
			{		
				for(j=0;j<4;j++)
				{
					tmp=((weightscale4x4[i][j][3]&0xff)<<24)|((weightscale4x4[i][j][2]&0xff)<<16)|((weightscale4x4[i][j][1]&0xff)<<8)|((weightscale4x4[i][j][0]&0xff)<<0);
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA4x4Y_OFF+i*16+j*4,tmp,"weightscale intra4x4");
				}
			}
					
			for(j=0;j<8;j++)
			{
				tmp=((weightscale8x8[1][j][3]&0xff)<<24)|((weightscale8x8[1][j][2]&0xff)<<16)|((weightscale8x8[1][j][1]&0xff)<<8)|((weightscale8x8[1][j][0]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+2*j*4,tmp,"weightscale inter8x8");
				tmp=((weightscale8x8[1][j][7]&0xff)<<24)|((weightscale8x8[1][j][6]&0xff)<<16)|((weightscale8x8[1][j][5]&0xff)<<8)|((weightscale8x8[1][j][4]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+(2*j+1)*4,tmp,"weightscale inter8x8");
			}
			for(j=0;j<8;j++)
			{
				tmp=((weightscale8x8[0][j][3]&0xff)<<24)|((weightscale8x8[0][j][2]&0xff)<<16)|((weightscale8x8[0][j][1]&0xff)<<8)|((weightscale8x8[0][j][0]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+2*j*4,tmp,"weightscale intra8x8");
				tmp=((weightscale8x8[0][j][7]&0xff)<<24)|((weightscale8x8[0][j][6]&0xff)<<16)|((weightscale8x8[0][j][5]&0xff)<<8)|((weightscale8x8[0][j][4]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+(2*j+1)*4,tmp,"weightscale intra8x8");
			}
	
		}

}

//extern BiContextType context[308];
extern int *context;
int tmp1;
PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = g_curr_slice_ptr;
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;

	MMDecRet ret = MMDEC_ERROR;
	unsigned int tmp;

	uint16 Slice_first_mb_x, Slice_first_mb_y;
	uint8  slice_type = curr_slice_ptr->picture_type;
	uint8  weighted_en=(g_active_pps_ptr->weighted_bipred_idc>0)&&(slice_type==B_slice)||g_active_pps_ptr->weighted_pred_flag&&(slice_type==P_slice);
	int	   i,j;
	
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	
	if (active_pps_ptr->entropy_coding_mode_flag)
	{
		init_contexts (img_ptr);		
	}

	Slice_first_mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
	Slice_first_mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
	
        if ((img_ptr->type == B_SLICE) && g_active_pps_ptr->weighted_bipred_idc == 2)
        {
		img_ptr->luma_log2_weight_denom = 5;
		img_ptr->chroma_log2_weight_denom = 5;
		img_ptr->wp_round_luma = 16;
		img_ptr->wp_round_chroma = 16;
	}//weihu

	//configure the slice info buf
	slice_info[0] = slice_type;
	slice_info[1] = img_ptr->slice_nr;
	slice_info[2] = g_active_pps_ptr->constrained_intra_pred_flag;
	slice_info[3] = img_ptr->direct_type;
	slice_info[4] = g_active_sps_ptr->direct_8x8_reference_flag;
	slice_info[5] = curr_slice_ptr->qp;
	slice_info[6] = g_active_pps_ptr->chroma_qp_index_offset;
	slice_info[7] = g_active_pps_ptr->second_chroma_qp_index_offset;
	slice_info[8] = curr_slice_ptr->LFDisableIdc;
	slice_info[9] = g_active_pps_ptr->weighted_bipred_idc;
	slice_info[10] = img_ptr->framepoc;
	slice_info[11] = img_ptr->num_ref_idx_l0_active;
	slice_info[12] = mmin(slice_info[11], g_list_size[0]);
	slice_info[13] = mmin(img_ptr->num_ref_idx_l1_active, g_list_size[1]);

	//for(i=0;i<16;i++)
	//  slice_info[14+i]=g_list1_map_list0[i];//weihu
		
	slice_info[30] = g_active_sps_ptr->seq_scaling_matrix_present_flag;
	slice_info[31] = g_active_pps_ptr->pic_scaling_matrix_present_flag;
	slice_info[32] = g_active_pps_ptr->weighted_pred_flag;//weihu
	slice_info[33] = g_image_ptr->luma_log2_weight_denom;
	slice_info[34] = g_image_ptr->chroma_log2_weight_denom;
	slice_info[35] = img_ptr->DPB_addr_index;
        slice_info[36] = curr_slice_ptr->LFAlphaC0Offset;
        slice_info[37] = curr_slice_ptr->LFBetaOffset;
	slice_info[38] = Slice_first_mb_x;
        slice_info[39] = Slice_first_mb_y;
	slice_info[40] = g_active_pps_ptr->transform_8x8_mode_flag;
	slice_info[41] = g_active_pps_ptr->entropy_coding_mode_flag;
	slice_info[42] = img_ptr->num_ref_idx_l1_active;
	slice_info[43] = (g_active_sps_ptr->profile_idc!=0x42);
	slice_info[44] = weighted_en;
	//chroma_log2_weight_denom, 
	//Luma_log2_weight_denom
	// OR1200_WRITE_REG(0x180020, 0x12000000,"for FPGA debug 8");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+IMG_SIZE_OFF, ((img_ptr->frame_height_in_mbs&0xff)<<8)|img_ptr->frame_width_in_mbs&0xff,"IMG_SIZE");
    
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, ((0x1)<<31)|((curr_slice_ptr->qp&0x3f)<<25)|((img_ptr->slice_nr&0x1ff)<<16)|((img_ptr->frame_size_in_mbs&0x1fff)<<3)|slice_type&0x7,"VSP_CFG0");
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, g_nalu_ptr->len&0xfffff,"VSP_CFG1");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF,((g_active_sps_ptr->profile_idc!=0x42)<<31)|(((slice_info[30]||slice_info[31])&0x1)<<29)|((img_ptr->curr_mb_nr&0x1fff)<<16) |((Slice_first_mb_y&0x7f)<<8)|Slice_first_mb_x&0x7f,"VSP_CFG2");
		
        tmp=((slice_info[8]&0x3)<<19)|((slice_info[9]&0x3)<<17)|((slice_info[32]&0x1)<<16)|((weighted_en&0x1)<<15)|((slice_info[41]&0x1)<<14)|((slice_info[40]&0x1)<<13)|((slice_info[4]&0x1)<<12)|((slice_info[3]&0x1)<<11)|((slice_info[2]&0x1)<<10) |((slice_info[37]&0x1f)<<5)|(slice_info[36]&0x1f);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,tmp,"VSP_CFG3");
	tmp=((slice_info[13]&0x1f)<<25)|((slice_info[12]&0x1f)<<20)|(((slice_info[42]-1)&0x1f)<<15)|(((slice_info[11]-1)&0x1f)<<10)|((slice_info[7]&0x1f)<<5)|((slice_info[6]&0x1f)<<0);		
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF,tmp,"VSP_CFG4");
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,img_ptr->framepoc,"VSP_CFG5 cur_POC");
	
	tmp=((g_list0_map_addr[15]&0x3)<<30)|((g_list0_map_addr[4]&0x3f)<<24)|((g_list0_map_addr[3]&0x3f)<<18)|((g_list0_map_addr[2]&0x3f)<<12)|((g_list0_map_addr[1]&0x3f)<<6)|((g_list0_map_addr[0]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX0_OFF,tmp,"g_list0_map_addr 0");//next 376 
	tmp=(((g_list0_map_addr[15]>>2)&0x3)<<30)|((g_list0_map_addr[9]&0x3f)<<24)|((g_list0_map_addr[8]&0x3f)<<18)|((g_list0_map_addr[7]&0x3f)<<12)|((g_list0_map_addr[6]&0x3f)<<6)|((g_list0_map_addr[5]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX1_OFF,tmp,"g_list0_map_addr 1");//next 376 
	tmp=(((g_list0_map_addr[15]>>4)&0x3)<<30)|((g_list0_map_addr[14]&0x3f)<<24)|((g_list0_map_addr[13]&0x3f)<<18)|((g_list0_map_addr[12]&0x3f)<<12)|((g_list0_map_addr[11]&0x3f)<<6)|((g_list0_map_addr[10]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX2_OFF,tmp,"g_list0_map_addr 2");//next 376 
	tmp=((g_list1_map_addr[15]&0x3)<<30)|((g_list1_map_addr[4]&0x3f)<<24)|((g_list1_map_addr[3]&0x3f)<<18)|((g_list1_map_addr[2]&0x3f)<<12)|((g_list1_map_addr[1]&0x3f)<<6)|((g_list1_map_addr[0]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX3_OFF,tmp,"g_list1_map_addr 0");//next 376 
	tmp=(((g_list1_map_addr[15]>>2)&0x3)<<30)|((g_list1_map_addr[9]&0x3f)<<24)|((g_list1_map_addr[8]&0x3f)<<18)|((g_list1_map_addr[7]&0x3f)<<12)|((g_list1_map_addr[6]&0x3f)<<6)|((g_list1_map_addr[5]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX4_OFF,tmp,"g_list1_map_addr 1");//next 376 
		
	tmp=(((g_list1_map_addr[15]>>4)&0x3)<<30)|((g_list1_map_addr[14]&0x3f)<<24)|((g_list1_map_addr[13]&0x3f)<<18)|((g_list1_map_addr[12]&0x3f)<<12)|((g_list1_map_addr[11]&0x3f)<<6)|((g_list1_map_addr[10]&0x3f)<<0);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX5_OFF,tmp,"g_list1_map_addr 2");//next 376
	
	//configure scaling matrix
	H264Dec_Cfg_ScalingMatix(img_ptr);
    
	for(i=0;i<33;i++)
	{
		OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+i*4,ref_list_buf[i],"ref_list_buf");
	}
		
	tmp=(g_image_ptr->chroma_log2_weight_denom<<8)|(g_image_ptr->luma_log2_weight_denom&0xff);
	OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+132,tmp,"chroma_log2_weight_denom,luma_log2_weight_denom");		

        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0xfFF,"clear BSM_frame done int");  

#define USE_INTERRUPT
       
	tmp1=0;
#ifdef USE_INTERRUPT	
	OR1200_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
#endif
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_2 | V_BIT_4 | V_BIT_5,"VSP_INT_MASK, enable mbw_slice_done, vld_err, time_out");//enable int //frame done/error/timeout
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF,0xa|img_ptr->is_need_init_vsp_hufftab,"VSP_START");//start vsp   vld/vld_table//load_vld_table_en
		    
#ifdef USE_INTERRUPT
        tmp1 = VSP_POLL_COMPLETE();
        SCI_TRACE_LOW("%s, %d, tmp1: %0x", __FUNCTION__, __LINE__, tmp1);
#else
	tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
	while ((tmp1&0x34)==0) //weihu tmp
	{
		tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
	}
        //OR1200_WRITE_REG(VSP_REG_BASE_ADDR+0x18,0x4,"VSP_INT_CLR");//enable int //frame done/error/timeout
#endif

	if(tmp1&0x30)
	{
                img_ptr->error_flag=1;
        }else if((tmp1&0x00000004)==0x00000004)
	{
		img_ptr->error_flag=0;
	}
//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame error int");//weihu// 0x34

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
	img_ptr->is_need_init_vsp_hufftab = FALSE;
	H264Dec_exit_slice(img_ptr);//g_image_ptr);

        img_ptr->slice_nr++;

	if (img_ptr->error_flag)
 	{
		return MMDEC_ERROR;
	}
	
	//if(end of picture)
	tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_DBG_STS0_OFF, "check mb_x mb_y number");//weihu tmp
	if((((tmp1>>8)&0xff)==(uint)(img_ptr->frame_width_in_mbs-1))&&((tmp1&0xff)==(uint)(img_ptr->frame_height_in_mbs-1)))//weihu tmp
	{
		ret =MMDEC_OK;

		H264Dec_exit_picture (img_ptr);

//		H264Dec_display_control ();
    frame_dec_finish=TRUE;

		if(display_array_len>0)
		{
			g_dispFrmNum++;
			dec_output_ptr->frameEffective = TRUE;
			dec_output_ptr->pOutFrameY = display_array_Y[0];
			dec_output_ptr->pOutFrameU = display_array_UV[0];
			dec_output_ptr->pBufferHeader = display_array_BH[0];
			dec_output_ptr->mPicId = display_array_mPicId[0];
			//dec_output_ptr->pOutFrameV = display_array_UV[0];//g_dec_picture_ptr->imgV;
			OR_VSP_UNBIND(display_array_BH[0]);
			display_array_len--;
			for(i =0;i<display_array_len; i++)
			{
				display_array_BH[i]=display_array_BH[i+1];
				display_array_Y[i] = display_array_Y[i+1];
				display_array_UV[i] = display_array_UV[i+1];
				display_array_mPicId[i] = display_array_mPicId[i+1];
			}
		}

		
		dec_output_ptr->frame_width = img_ptr->width;
		dec_output_ptr->frame_height = img_ptr->height;
		

		
			
		g_dec_picture_ptr = NULL;//weihu for output
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




















































































