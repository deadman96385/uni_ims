/******************************************************************************
 ** File Name:    mea_trace.c                                                 *
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

void init_mea_trace()
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	g_pPureSadFile = fopen("..\\..\\trace\\PureSad.txt", "w");
	g_pFinalResultFile = fopen("..\\..\\trace\\FinalResult.txt", "w");
	g_p12x9UVBlockFile = fopen("..\\..\\trace\\trace_mea_UV12x9Block.txt", "w");
	g_pPredMVFile = fopen("..\\..\\trace\\PredMV.txt","r");
}
}

void print_src_mb(int32 mb_x, int32 mb_y)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int32 i, j;
	uint32 *pIntSrcY = g_mea_src_frame_y;
	uint8 *pSrcU = (uint8*)g_mea_src_frame_u;
	uint8 *pSrcV = (uint8*)g_mea_src_frame_v;
	int32 offset;
	uint32 *p_y;
	FILE *test_vector_fp = g_fp_mea_src_mb_tv;
	int32 frame_width = g_mea_frame_width << 2; //byte
	int32 frame_height = g_mea_frame_height;
	int32 frame_width_c = frame_width >> 1;
	int32 vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 data_format = (g_glb_reg_ptr->VSP_CFG1>>24)&0x7;
	int32 c_pix_pitch;
	int32 four_pix;
	int8 *p_u, *p_v;

	if (!uv_interleaved) //three plane
	{
		c_pix_pitch = 1;
	}else //two plane
	{
		c_pix_pitch = 2;
		pSrcV = pSrcU+1;
	}
	printf("mb_x %d mb_y %d\n", mb_x, mb_y);
	if(JPEG_FW_YUV420 == data_format) 
	{
		offset = (frame_width * (mb_y*16) + (mb_x * 16))>>2; //word
		p_y = pIntSrcY + offset;
		for(i = 0; i < 16; i++)
		{
			for(j = 0; j < 4; j++ )
			{
				fprintf_oneWord_hex(test_vector_fp, *p_y++);
			}
			p_y += (frame_width - 16)/4;
		}
	}else //422
	{
		offset = (frame_width * (mb_y*8) + (mb_x * 16))>>2; //word
		p_y = pIntSrcY + offset;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 4; j++ )
			{
				fprintf_oneWord_hex(test_vector_fp, *p_y++);
			}
			p_y += (frame_width - 16)/4;
		}
	}

	//u
	offset = (frame_width_c * (mb_y*8) + (mb_x * 8))*c_pix_pitch; //byte
		
	p_u = pSrcU+offset;
	for(i = 0; i < 8; i++)
	{
		four_pix = ((p_u[3*c_pix_pitch]&0xff)<<24)|((p_u[2*c_pix_pitch]&0xff)<<16)|
				   ((p_u[1*c_pix_pitch]&0xff)<<8)|((p_u[0]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);

		four_pix = ((p_u[7*c_pix_pitch]&0xff)<<24)|((p_u[6*c_pix_pitch]&0xff)<<16)|
			       ((p_u[5*c_pix_pitch]&0xff)<<8)|((p_u[4*c_pix_pitch]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);	
		p_u += frame_width_c*c_pix_pitch;
	}

	//v
	p_v = pSrcV+offset;
	for(i = 0; i < 8; i++)
	{
		four_pix = ((p_v[3*c_pix_pitch]&0xff)<<24)|((p_v[2*c_pix_pitch]&0xff)<<16)|
			       ((p_v[1*c_pix_pitch]&0xff)<<8)|((p_v[0]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);

		four_pix = ((p_v[7*c_pix_pitch]&0xff)<<24)|((p_v[6*c_pix_pitch]&0xff)<<16)|
			       ((p_v[5*c_pix_pitch]&0xff)<<8)|((p_v[4*c_pix_pitch]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);	
						
		p_v += frame_width_c*c_pix_pitch;
	}
}
}

//422		
void Dump_Src_frame_Data_422(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved)
{
	if(g_trace_enable_flag&TRACE_ENABLE_MEA)
	{
		int mb_num_x = (frame_width >> 4);
		int mb_num_y = (frame_height >> 3);
		uint32 *pIntSrcY = (uint32 *)input_yuv_bfr_ptr;
		uint32 *pIntSrcU = pIntSrcY + (frame_width*frame_height>>2);
		uint32 *pIntSrcV = pIntSrcU + (frame_width*frame_height>>3);
		int i,j;

		for(i = 0; i < frame_height; i++)
		{
			for(j = 0; j < (frame_width>>3); j++)
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);
				fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);

				fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
				if (uv_interleaved) //two plane
				{
					fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
				}else //three plane
				{
//					fprintf_oneWord_hex(g_fp_mea_src_frm_v_tv, *pIntSrcV++);
				}
			}
		}
	}
}

//420
void Dump_Src_frame_Data_420(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved)
{
	if(g_trace_enable_flag&TRACE_ENABLE_MEA)
	{
		int mb_num_x = (frame_width >> 4);
		int mb_num_y = (frame_height >> 4);
		uint32 *pIntSrcY = (uint32 *)input_yuv_bfr_ptr;
		uint32 *pIntSrcU = pIntSrcY + (frame_width*frame_height>>2);
		uint32 *pIntSrcV = pIntSrcU + (frame_width*frame_height>>4);
		int i,j;

		for(i = 0; i < frame_height; i++)
		{
			for(j = 0; j < (frame_width>>2); j++)
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);
			}
		}

		for(i = 0; i < frame_height>>1; i++)
		{
			for(j = 0; j < (frame_width>>3); j++)
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
				if (!uv_interleaved) //three plane
				{
//					fprintf_oneWord_hex(g_fp_mea_src_frm_v_tv, *pIntSrcV++);
				}else //two plane
				{
					fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
				}
			}
		}

	//	{
	//		FILE *fp = fopen("d:\\src.yuv", "wb");
	//		fwrite(input_yuv_bfr_ptr, 1, frame_width*frame_height*3/2, fp);
	//		fclose(fp);
	//	}
	}
}

void Dump_tv_frame_Data_420(uint32 *pIntSrcY, uint32 *pIntSrcU, int32 frame_width, int32 frame_height, int8 print_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		int i,j,k;
		FILE *output_y, *output_uv;
		FILE *src_y, *src_uv;
		char file_name[128];
		uint8 *pSrcY = (uint8 *)pIntSrcY;
		uint8 *pSrcU = (uint8 *)pIntSrcU;
		uint8 *pSrcV = (uint8 *)pIntSrcU + 1;
		//uint8 *pSrcV = pSrcU + (frame_height>>1)*(frame_width>>1);
if (g_nFrame_enc==79)
{
	g_nFrame_enc= g_nFrame_enc;
}
		if(print_type == 1)//source frame
		{
			output_y = g_fp_mea_src_frm_y_tv;
			output_uv = g_fp_mea_src_frm_u_tv;
			sprintf(file_name, "..\\..\\test_vectors\\src_yuv\\src_y_%d.txt", g_nFrame_enc);
			src_y = fopen(file_name, "w");
			sprintf(file_name, "..\\..\\test_vectors\\src_yuv\\src_uv_%d.txt", g_nFrame_enc);
			src_uv = fopen(file_name, "w");
		}
		else if(print_type == 0)//reference frame
		{
			output_y = g_fp_mea_ref_frm_y_tv;
			output_uv = g_fp_mea_ref_frm_u_tv;
		}
		else//print_type == 2//reconstruct frame(uv interleaved)
		{
			output_y = g_fp_rec_frm_tv;
			output_uv = g_fp_rec_frm_tv;
		}
		/*fprintf(output_y, "//frame_no %d\n", g_nFrame_enc);
		if(output_y != output_uv)
			fprintf(output_uv, "//frame_no %d\n", g_nFrame_enc);*/
		if (enable_anti_shake && print_type == 1)
		{
			//int offset;
			uint8 *tmp1 = malloc(input_width*frame_height*3/2);
			uint8 *tmp;

			tmp = tmp1;
			//offset = (input_width-frame_width)/2;
			memset(tmp, 0, input_width*frame_height*3/2*sizeof(uint8));
			for (i=0;i<frame_height;i++)
			{
				//memcpy(tmp+input_width*i+offset, pSrcY+frame_width*i, frame_width);
				memcpy(tmp+input_width*i+shift_x, pSrcY+frame_width*i, frame_width);
			}
			for(i=0;i<frame_height/2;i++)
			{
				//memcpy(tmp + input_width*frame_height + input_width*i + offset, pSrcU + frame_width*i, frame_width);
				memcpy(tmp + input_width*frame_height + input_width*i + shift_x, pSrcU + frame_width*i, frame_width);
			}
			for(i = 0; i < frame_height; i++)
			{
				for(j = 0; j < input_width>>3; j++)
				{
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					for(k=7;k>=0;k--)
					{
						fprintf(output_y, "%02x", tmp[k]);
						if(print_type == 1)
							fprintf(src_y, "%02x", tmp[k]);
					}
					tmp += 8;
					fprintf(output_y, "\n");
					if(print_type == 1)
						fprintf(src_y, "\n");
				}
			}
			tmp = tmp1 + frame_height*input_width;
			for(i = 0; i < frame_height>>1; i++)
			{
				for(j = 0; j < input_width>>3; j++)
				{
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					for(k=7;k>=0;k--)
					{
						fprintf(output_uv, "%02x", tmp[k]);
						if(print_type == 1)
							fprintf(src_uv, "%02x", tmp[k]);
					}
					tmp += 8;
					fprintf(output_uv, "\n");
					if(print_type == 1)
						fprintf(src_uv, "\n");
				}
			}

			free(tmp1);
		}
		else
		{
			//Y
			for(i = 0; i < frame_height; i++)
			{
				for(j = 0; j < (frame_width>>3); j++)
				{
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					//fprintf_oneWord_hex(output_y, *pIntSrcY++);
					//if (print_type == 2)
					{
						for(k=7;k>=0;k--)
						{
							fprintf(output_y, "%02x", pSrcY[k]);
							if(print_type == 1)
								fprintf(src_y, "%02x", pSrcY[k]);
						}
					}
					/*else
					{
						for(k=0;k<8;k++)
						{
							fprintf(output_y, "%02x", pSrcY[k]);
						}
					}*/
					pSrcY += 8;
					fprintf(output_y, "\n");
					if(print_type == 1)
						fprintf(src_y, "\n");
				}
			}
			//UV
			//if (print_type == 2)
			{
				for(i = 0; i < frame_height>>1; i++)
				{
					for(j = 0; j < (frame_width>>3); j++)
					{
						//fprintf_oneWord_hex(output_y, *pIntSrcY++);
						//fprintf_oneWord_hex(output_y, *pIntSrcY++);
						for(k=7;k>=0;k--)
						{
							fprintf(output_uv, "%02x", pSrcU[k]);
							if(print_type == 1)
								fprintf(src_uv, "%02x", pSrcU[k]);
						}
						pSrcU += 8;
						fprintf(output_uv, "\n");
						if(print_type == 1)
							fprintf(src_uv, "\n");
					}
				}
			}
		}
		/*else
		{
			for(i = 0; i < frame_height>>1; i++)
			{
				for(j = 0; j < (frame_width>>3); j++)
				{
					//fprintf_oneWord_hex(output_uv, *pIntSrcU++);
					//fprintf_oneWord_hex(output_uv, *pIntSrcU++);
					for(k=0;k<4;k++)
					{
						fprintf(output_uv, "%02x", pSrcU[k*2]);
						fprintf(output_uv, "%02x", pSrcV[k*2]);
					}
					pSrcU += 4*2;
					pSrcV += 4*2;
					fprintf(output_uv, "\n");
				}
			}
		}*/
		if(print_type == 1)
		{
			fclose(src_uv);
			fclose(src_y);
		}
	}

}

void PrintfSrcMB(ENC_VOP_MODE_T *p_mode)
{	
	if(g_vector_enable_flag&VECTOR_ENABLE_IEA)
	{
		uint8 *imgY, *imgU, *imgV, *pCurMB_Y, *pCurMB_U, *pCurMB_V;
		int i,j,k;
		imgY = p_mode->pYUVSrcFrame->imgY;
		imgU = p_mode->pYUVSrcFrame->imgU;
		imgV = p_mode->pYUVSrcFrame->imgU + 1;//(p_mode->FrameHeight>>1)*(p_mode->FrameWidth>>1);
		pCurMB_Y = imgY + (p_mode->mb_y*p_mode->FrameWidth + p_mode->mb_x)*MB_SIZE;
		pCurMB_U = imgU + (p_mode->mb_y*p_mode->MBNumX*(MB_SIZE>>1) + p_mode->mb_x)*(MB_SIZE>>1);
		pCurMB_V = imgV + (p_mode->mb_y*p_mode->MBNumX*(MB_SIZE>>1) + p_mode->mb_x)*(MB_SIZE>>1);
		fprintf(g_fp_iea_src_mb_tv, "frame_cnt=%d,mb_x=%d,mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		for(i=0;i<16;i++)
		{
			for(j=0;j<2;j++)
			{
				for(k=7;k>=0;k--)
				{
					//fprintf(g_fp_iea_src_mb_tv, "%03d ", pCurMB_Y[j*8+k]);
					fprintf(g_fp_iea_src_mb_tv, "%02x", pCurMB_Y[j*8+k]);
				}
				fprintf(g_fp_iea_src_mb_tv, "\n");
			}
			pCurMB_Y += p_mode->FrameWidth;
		}
		for(i=0;i<8;i++)
		{
			for(j=7;j>=0;j--)
			{
				//fprintf(g_fp_iea_src_mb_tv, "%03d ", pCurMB_U[j]);
				fprintf(g_fp_iea_src_mb_tv, "%02x", pCurMB_U[j*2]);
			}
			pCurMB_U += p_mode->FrameWidth/2*2;
			fprintf(g_fp_iea_src_mb_tv, "\n");
		}
		for(i=0;i<8;i++)
		{
			for(j=7;j>=0;j--)
			{
				//fprintf(g_fp_iea_src_mb_tv, "%03d ", pCurMB_V[j]);
				fprintf(g_fp_iea_src_mb_tv, "%02x", pCurMB_U[j*2]);
			}
			pCurMB_V += p_mode->FrameWidth/2*2;
			fprintf(g_fp_iea_src_mb_tv, "\n");
		}
	}
}
extern void LookupUVMV(MB_MODE_E MB_Mode, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pMv_uv);
void PrintfMEAbufParam(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		int32 temp;
		int8 i;
		MOTION_VECTOR_T mv_uv, mv[5];

		memset(mv, 0, sizeof(mv));
		if(VOP_type != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)
			{
				mv[2].y = (int8)((g_mea_reg_ptr->mea_result_mv_8x8[1]>>8)&0xff);//mv_y
				mv[2].x = (int8)(g_mea_reg_ptr->mea_result_mv_8x8[1]&0xff);//mv_x
				mv[1].y = (int8)((g_mea_reg_ptr->mea_result_mv_8x8[0]>>8)&0xff);//mv_y
				mv[1].x = (int8)(g_mea_reg_ptr->mea_result_mv_8x8[0]&0xff);//mv_x
				mv[4].y = (int8)((g_mea_reg_ptr->mea_result_mv_8x8[3]>>8)&0xff);//mv_y
				mv[4].x = (int8)(g_mea_reg_ptr->mea_result_mv_8x8[3]&0xff);//mv_x
				mv[3].y = (int8)((g_mea_reg_ptr->mea_result_mv_8x8[2]>>8)&0xff);//mv_y
				mv[3].x = (int8)(g_mea_reg_ptr->mea_result_mv_8x8[2]&0xff);//mv_x
			}
			else if(pMb_mode->dctMd==INTER)
			{
				mv[0].y = (int8)((g_mea_reg_ptr->mea_result_mv_16x16>>8)&0xff);//mv_y
				mv[0].x = (int8)(g_mea_reg_ptr->mea_result_mv_16x16&0xff);//mv_x
			}
			else
			{
				mv[0].y = 0;
				mv[0].x = 0;
			}
			LookupUVMV(pMb_mode->dctMd, mv, &mv_uv);
		}
		else
		{
			mv_uv.x = 0;
			mv_uv.y = 0;
		}
		

		fprintf(g_fp_mea_ppa_buf_tv, "//frame_cnt %d, mb_x %d, mb_y %d c_mv_x %d c_mv_y %d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y, mv_uv.x, mv_uv.y);
		temp = 0;
		if(VOP_type != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]>>16)+
						 (g_mea_reg_ptr->mea_result_mv_8x8[1]>>16)+
						 (g_mea_reg_ptr->mea_result_mv_8x8[2]>>16)+
						 (g_mea_reg_ptr->mea_result_mv_8x8[3]>>16));//sad[15:0]
			}
			else if(pMb_mode->dctMd==INTER)
				temp |= (g_mea_reg_ptr->mea_result_mv_16x16>>16);//sad[15:0]
			else//intra
				temp = 0;
		}
		fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);
		temp = 0;
		temp |= pMb_mode->bIntra;//is_intra[0]
		temp |= (pMb_mode->dctMd==INTER4V?1:0)<<1;//mb_partition 0:16x16 1:8x8
		temp |= p_mode->mb_y<<3;//curr_mb_y[9:3]
		temp |= p_mode->mb_x<<10;//curr_mb_x[16:10]
		//temp |= pMb_mode->bSkip<<17;//is_skip[17]
		if(VOP_type == IVOP)
			temp |= p_mode->StepI<<18;//QP[22:18]
		else
			temp |= p_mode->StepP<<18;//QP[22:18]
		temp |= (((p_mode->mb_y+1)%p_mode->mbline_num_slice==0 ||(p_mode->mb_y+1)== p_mode->MBNumY) && (p_mode->mb_x+1)==p_mode->MBNumX)<<31;//Is_Last_mb[31] last mb of slice
		fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);
		temp = 0;
		if(VOP_type != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[1]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[1]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[1]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_1
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[0]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_0
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[3]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[3]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[3]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_3
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[2]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[2]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[2]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_2
			}
			else if(pMb_mode->dctMd==INTER)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_16x16>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_16x16&0xff)|((g_mea_reg_ptr->mea_result_mv_16x16&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_1
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_0
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_3
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_2
			}
			else//intra
			{
				temp = 0;
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_1
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_0
				fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_3
				fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_2
			}
		}
		else
		{
			temp = 0;
			fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_1
			fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_0
			fprintf(g_fp_mea_ppa_buf_tv, "%08x", temp);//mv_l0_3
			fprintf(g_fp_mea_ppa_buf_tv, "%08x\n", temp);//mv_l0_2
		}
		for(i=0;i<15;i++)
		{
			fprintf(g_fp_mea_ppa_buf_tv, "%016x\n", 0);
		}
	}
}
void PrintfMBCPARA(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
	{
		int32 temp = 0;
		ENC_MB_MODE_T *pTopRIGHTMB;

		fprintf(g_fp_mbc_para_tv, "//frame_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);

		pTopRIGHTMB = p_mode->pMbModeAbv + p_mode->mb_x + 1;
#if 0
		temp |= pMb_mode->bIntra;//MB_TYPE[0] intra:1 inter:0
		temp |= 0;//IPCM[1]
		if(p_mode->pMBCache->bTopMBAvail && !pTopRIGHTMB->bIntra)
			temp |= 1<<2;//bit2: top_right valid
		temp |= p_mode->pMBCache->bTopMBAvail<<3;//bit3: top valid
		temp |= p_mode->pMBCache->bLeftMBAvail<<4;//bit4: left valid
		temp |= p_mode->pMBCache->bLeftTopAvail<<5;//bit5: top_left valid
		temp |= 1<<6;//IPRED_TYPE[7:6]
		temp |= 0<<8;//CHROMA_MODE[9:8]
#else //set [9:1] as 0, because of no Intra Pred mode in mpeg4
		temp |= pMb_mode->bIntra;//MB_TYPE[0] intra:1 inter:0
				  //IPCM[1]
				  //bit2: top_right valid
				  //bit3: top valid
				  //bit4: left valid
				  //bit5: top_left valid
				  //IPRED_TYPE[7:6]
				  //CHROMA_MODE[9:8]
#endif
		temp |= p_mode->mb_y<<10;//MB_Y_ID[16:10]
		temp |= (((p_mode->mb_y+1)%p_mode->mbline_num_slice==0 || (p_mode->mb_y+1)== p_mode->MBNumY) && (p_mode->mb_x+1)==p_mode->MBNumX)<<31;//Is_Last_mb[31] last mb of slice
		fprintf(g_fp_mbc_para_tv, "%08x\n", temp);

		temp = 0;
		fprintf(g_fp_mbc_para_tv, "%08x\n", temp);//LUMA_MODE0

		fprintf(g_fp_mbc_para_tv, "%08x\n", temp);//LUMA_MODE1

		temp = 0;//MB_CBP[23:0] //no cpb value before MBC
		temp |= p_mode->mb_x<<24;//MB_X_ID[30:24]
		fprintf(g_fp_mbc_para_tv, "%08x\n", temp);
	}
}
void PrintfPPAIN(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
	{
		int32 temp = 0;
		int8 i = 0;
/*if (g_nFrame_enc==1)
{
	temp = temp;
}*/
		fprintf(g_fp_ppa_in_tv, "//frame_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		if(p_mode->VopPredType != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]>>16)+
					(g_mea_reg_ptr->mea_result_mv_8x8[1]>>16)+
					(g_mea_reg_ptr->mea_result_mv_8x8[2]>>16)+
					(g_mea_reg_ptr->mea_result_mv_8x8[3]>>16));//sad[15:0]
			}
			else if(pMb_mode->dctMd==INTER)
				temp |= (g_mea_reg_ptr->mea_result_mv_16x16>>16);//sad[15:0]
			else//intra
				temp = 0;
		}
		fprintf(g_fp_ppa_in_tv, "%08x", temp);

		temp = 0;
		temp |= pMb_mode->bIntra;//is_intra[0]
		temp |= (pMb_mode->dctMd==INTER4V?1:0)<<1;//mb_partition[1] 0:16x16 1:8x8
		temp |= p_mode->mb_y<<3;//cur_mb_y[6:0]; //bit[9:3]
		temp |= p_mode->mb_x<<10;//cur_mb_y[6:0]; //bit[16:10]
		temp |= 0<<17;//pMb_mode->bSkip<<17;//is_skip[17]
		temp |= (p_mode->VopPredType==IVOP?p_mode->StepI:p_mode->StepP)<<18;//QP[22:18]
		temp |= (((p_mode->mb_y+1)%p_mode->mbline_num_slice==0 || (p_mode->mb_y+1)== p_mode->MBNumY) && (p_mode->mb_x+1)==p_mode->MBNumX)<<31;//Is_Last_mb[31] last mb of slice
		fprintf(g_fp_ppa_in_tv, "%08x\n", temp);

		temp = 0;
		if(p_mode->VopPredType != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[1]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[1]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[1]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_1
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[0]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[0]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_0
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[3]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[3]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[3]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_3
				temp = 0;
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[2]>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_8x8[2]&0xff)|((g_mea_reg_ptr->mea_result_mv_8x8[2]&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_2
			}
			else if(pMb_mode->dctMd==INTER)
			{
				temp |= ((g_mea_reg_ptr->mea_result_mv_16x16>>8)&0xff);//mv_y
				temp |= ((g_mea_reg_ptr->mea_result_mv_16x16&0xff)|((g_mea_reg_ptr->mea_result_mv_16x16&0x80)<<1))<<16;//mv_x
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_1
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_0
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_3
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_2
			}
			else//intra
			{
				temp = 0;
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_1
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_0
				fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_3
				fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_2
			}
		}
		else
		{
			temp = 0;
			fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_1
			fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_0
			fprintf(g_fp_ppa_in_tv, "%08x", temp);//mv_l0_3
			fprintf(g_fp_ppa_in_tv, "%08x\n", temp);//mv_l0_2
		}
		for (i=0;i<15;i++)
		{
			fprintf(g_fp_ppa_in_tv, "%016x\n", 0);
		}
	}
}
#if !defined(_LIB)
void PrintfPPACFG(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
	{
		uint16 slice_first_mb_x, slice_first_mb_y,  slice_last_mb_y;

		slice_first_mb_x = 0;
		slice_first_mb_y = p_mode->mb_y - (p_mode->mb_y%p_mode->mbline_num_slice);

		if(slice_first_mb_x != p_mode->mb_x || slice_first_mb_y != p_mode->mb_y)
			return;

		slice_last_mb_y = slice_first_mb_y + p_mode->mbline_num_slice - 1;
		slice_last_mb_y = (slice_last_mb_y>(p_mode->MBNumY-1))?(p_mode->MBNumY-1):slice_last_mb_y;
		
		fprintf(g_fp_ppa_cfg_tv, "work_mode= %x\n", 1);
		fprintf(g_fp_ppa_cfg_tv, "standard= %x\n", g_input->is_short_header?0:1);
		fprintf(g_fp_ppa_cfg_tv, "slice_first_mb_x= %x\n", slice_first_mb_x);
		fprintf(g_fp_ppa_cfg_tv, "slice_first_mb_y= %x\n", slice_first_mb_y);
		fprintf(g_fp_ppa_cfg_tv, "slice_last_mb_y= %x\n", slice_last_mb_y);
		fprintf(g_fp_ppa_cfg_tv, "picwidth_in_mb= %x\n", p_mode->MBNumX);
		fprintf(g_fp_ppa_cfg_tv, "picheight_in_mb= %x\n", p_mode->MBNumY);
		fprintf(g_fp_ppa_cfg_tv, "slice_type= %x\n", p_mode->VopPredType);
		fprintf(g_fp_ppa_cfg_tv, "slice_num= %x\n", p_mode->mb_y/p_mode->mbline_num_slice);
		fprintf(g_fp_ppa_cfg_tv, "ppa_info_vdb_en= %x\n", 0);
		fprintf(g_fp_ppa_cfg_tv, "vop_fcode_forward= %x\n", p_mode->mvInfoForward.FCode);
		fprintf(g_fp_ppa_cfg_tv, "vop_fcode_backward= %x\n", 0);//no backward mv
		fprintf(g_fp_ppa_cfg_tv, "trd= %x\n", 0);
		fprintf(g_fp_ppa_cfg_tv, "trb= %x\n", 0);
		fprintf(g_fp_ppa_cfg_tv, "1/trd= %x\n", 0);
		fprintf(g_fp_ppa_cfg_tv, "quant_type= %x\n", p_mode->QuantizerType);
		fprintf(g_fp_ppa_cfg_tv, "skip_threshold = %x\n", 0);
		fprintf(g_fp_ppa_cfg_tv, "slice_qp = %x\n", p_mode->VopPredType==IVOP?p_mode->StepI:p_mode->StepP);
	}
}
#endif
void PrintfVLCParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
	{
		uint8 s_MbType[] = {3, 4, 0, 1, 2};
		int32 temp = 0, temp1 = 0;
		int8 i;

		fprintf(g_fp_vlc_para_tv, "//frame_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);

		//temp |= (pMb_mode->CBP & 3);//cbpc[1:0]
		//temp |= ((pMb_mode->CBP>>2)&0xf)<<2;//cbpy[5:2]
		temp |= pMb_mode->CBP;//cbpc[1:0] //cbpy[5:2]
		temp |= (((p_mode->mb_y+1)%p_mode->mbline_num_slice==0 || (p_mode->mb_y+1)== p_mode->MBNumY) && (p_mode->mb_x+1)==p_mode->MBNumX)<<8;//Is_Last_mb[8] last mb of slice
		temp |= (s_MbType[pMb_mode->dctMd]&0x07)<<10;//mb_type[12:10]
		temp |= p_mode->pMBCache->bTopMBAvail<<14;//top_avail[14]
		temp |= p_mode->pMBCache->bLeftMBAvail<<16;//left_avail[16]
		temp |= p_mode->pMBCache->bLeftTopAvail<<18;//TL_avail[18]
		temp |= (VOP_type==IVOP?p_mode->StepI:p_mode->StepP)<<20;//[24:20]qp
		fprintf(g_fp_vlc_para_tv, "%07x", temp);

		temp = 0;
		temp |= p_mode->mb_y<<4;//curr_mb_y[10:4]
		temp |= p_mode->mb_x<<12;//curr_mb_x[18:12]
		temp |= pMb_mode->bSkip<<20;//is_skip[20]
		temp |= (pMb_mode->intStepDelta&0x07)<<22;//dquant[24:22]
		temp |= pMb_mode->bACPrediction<<26;//Ac_pred_flag[26]
		fprintf(g_fp_vlc_para_tv, "%07x\n", temp);

		temp = 0;
		temp |= VOP_type<<2;//[3:2]slice type
		temp |= p_mode->mvInfoForward.FCode<<4;//[6:4]Fcode_fwd
		temp |= p_mode->short_video_header<<8;//[8] is_short_header
		fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, 0);

		temp = 0;
		if(VOP_type != IVOP)
		{
			if(pMb_mode->dctMd==INTER4V)//8x8
			{
				
				temp |= (tv_Diff_MV[0].y&0x1fff)<<14;//MVd0_y[26:14]
				temp |= (tv_Diff_MV[0].x&0x3fff);//MVd0_x[13:0]
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, 0);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				temp1 = temp;

				temp = 0;
				temp |= (tv_Diff_MV[1].y&0x1fff)<<14;//MVd1_y[26:14]
				temp |= (tv_Diff_MV[1].x&0x3fff);//MVd1_x[13:0]
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp1);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				temp1 = temp;

				temp = 0;
				temp |= (tv_Diff_MV[2].y&0x1fff)<<14;//MVd2_y[26:14]
				temp |= (tv_Diff_MV[2].x&0x3fff);//MVd2_x[13:0]
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp1);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				temp1 = temp;

				temp = 0;
				temp |= (tv_Diff_MV[3].y&0x1fff)<<14;//MVd3_y[26:14]
				temp |= (tv_Diff_MV[3].x&0x3fff);//MVd3_x[13:0]
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp1);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				temp1 = temp;
			}
			else if(pMb_mode->dctMd==INTER && !pMb_mode->bSkip)//16x16
			{
				temp |= (tv_Diff_MV[0].y&0x1fff)<<14;//MVd0_y[26:14]
				temp |= (tv_Diff_MV[0].x&0x3fff);//MVd0_x[13:0]
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, 0);//MVd0
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);//MVd1
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);//MVd2
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);//MVd3
				fprintf(g_fp_vlc_para_tv, "%07x%07x\n", temp, temp);
				temp1 = temp;
			}
			else//if skip or intra print mv=0
			{
				temp = 0;
				fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd0
				fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
				fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd1
				fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
				fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd2
				fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
				fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd3
				fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
				temp1 = temp;
			}
		}
		else
		{
			temp = 0;
			fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd0
			fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
			fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd1
			fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
			fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd2
			fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
			fprintf(g_fp_vlc_para_tv, "%014x\n", temp);//MVd3
			fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
			temp1 = temp;
		}
		fprintf(g_fp_vlc_para_tv, "%07x%07x\n", 0, temp1);//MVd3
		for (i=0;i<14;i++)
		{
			fprintf(g_fp_vlc_para_tv, "%014x\n", 0);
		}
	}
}
void PrintfDCTParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
	{
		uint8 g_dc_scaler_table_y[32] = 
		{
			0,   8,    8,  8,  8, 10, 12,  14,	16, 17, 18, 19, 20, 21, 22, 23,
			24, 25, 26, 27, 28, 29, 30, 31,	32, 34, 36, 38, 40, 42, 44, 46
		};

		uint8 g_dc_scaler_table_c[32] =
		{
			0,    8,  8,   8,  8,   9,  9, 10,	10, 11, 11, 12, 12, 13, 13, 14,
			14, 15, 15, 16, 16, 17, 17, 18,	18, 19, 20, 21, 22, 23, 24, 25
		};
		uint32 temp = 0;
		int8 iDcScalerY, iDcScalerC, i;

		fprintf(g_fp_dct_para_tv, "//frame_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		temp |= p_mode->mb_y;//curr_mb_y[6:0]
		temp |= p_mode->mb_x<<7;//curr_mb_x[13:7]
		temp |= pMb_mode->bIntra<<16;//bIntra[16]
		temp |= 0<<17;//skip_idct[17]
		temp |= p_mode->short_video_header<<18;//is_short_header[18]
		temp |= p_mode->QuantizerType<<19;//Quant_type[19]
		fprintf(g_fp_dct_para_tv, "%08x\n", temp);

		temp = 0;
		if(VOP_type == IVOP)
		{
			temp |= p_mode->StepI;//QP[4:0]
			/*iDcScalerY = g_dc_scaler_table_y[p_mode->StepI];
			iDcScalerC = g_dc_scaler_table_c[p_mode->StepI];
			temp |= iDcScalerY<<5;//iDcScalerY;[9:5] 
			temp |= iDcScalerC<<10;//iDcScalerC;[14:10] */
		}
		else
		{
			temp |= p_mode->StepP;//QP[4:0]
			/*iDcScalerY = g_dc_scaler_table_y[p_mode->StepP];
			iDcScalerC = g_dc_scaler_table_c[p_mode->StepP];
			temp |= iDcScalerY<<5;//iDcScalerY;[9:5] 
			temp |= iDcScalerC<<10;//iDcScalerC;[14:10] */
		}
		fprintf(g_fp_dct_para_tv, "%08x\n", temp);

		temp = 0;
		//temp |= pMb_mode->CBP;//cbp[5:0]
		fprintf(g_fp_dct_para_tv, "%08x\n", temp);

		for(i=0;i<7;i++)
			fprintf(g_fp_dct_para_tv, "%08x\n", 0);
	}
}
void PrintfMCAParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MCA)
	{
		uint32 temp = 0;
		int8 i;
		fprintf(g_fp_mcapara_tv, "//frame_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);

		temp |= (p_mode->mb_y&0x7f);//cur_mb_x[6:0]:7b
		temp |= (p_mode->mb_x&0x7f)<<7;//cur_mb_x[13:7]:7b
		temp |= (pMb_mode->bIntra&0x1)<<14;//Is_intra[14]
		temp |= (pMb_mode->dctMd&0x3)<<15;//mb_mode[16:15]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);
		
		temp = 0;
		temp |= ((pMb_mode->mv[0].x|pMb_mode->mv[0].y)?1:0)<<16;//predflag_0_L0[16]
		temp |= 0<<17;//predflag_0_L1[17]
		temp |= ((pMb_mode->mv[1].x|pMb_mode->mv[1].y)?1:0);//predflag_1_L0,[18]
		temp |= 0<<19;//predflag_1_L1,[19]
		temp |= ((pMb_mode->mv[2].x|pMb_mode->mv[2].y)?1:0)<<20;//predflag_2_L0,[20]
		temp |= 0<<21;//predflag_2_L1, [21]
		temp |= ((pMb_mode->mv[3].x|pMb_mode->mv[3].y)?1:0)<<22;//predflag_3_L0,[22]
		temp |= 0<<23;//predflag_3_L1, [23]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);

		for (i=0;i<8;i++)
		{
			fprintf(g_fp_mcapara_tv, "%07x\n", 0);
		}

		temp = 0;
		temp |= (pMb_mode->mv[0].y&0x1fff);//Mv0_y[12:0]
		temp |= (pMb_mode->mv[0].x&0x1fff)<<13;//Mv0_x[25:13]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);

		for (i=0;i<3;i++)
		{
			fprintf(g_fp_mcapara_tv, "%07x\n", 0);
		}

		temp = 0;
		temp |= (pMb_mode->mv[1].y&0x1fff);//Mv1_y[12:0]
		temp |= (pMb_mode->mv[1].x&0x1fff)<<13;//Mv1_x[25:13]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);

		for (i=0;i<3;i++)
		{
			fprintf(g_fp_mcapara_tv, "%07x\n", 0);
		}

		temp = 0;
		temp |= (pMb_mode->mv[2].y&0x1fff);//Mv2_y[12:0]
		temp |= (pMb_mode->mv[2].x&0x1fff)<<13;//Mv2_x[25:13]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);
		
		for (i=0;i<3;i++)
		{
			fprintf(g_fp_mcapara_tv, "%07x\n", 0);
		}

		temp = 0;
		temp |= (pMb_mode->mv[3].y&0x1fff);//Mv3_y[12:0]
		temp |= (pMb_mode->mv[3].x&0x1fff)<<13;//Mv3_x[25:13]
		fprintf(g_fp_mcapara_tv, "%07x\n", temp);
		
		for (i=0;i<27;i++)
		{
			fprintf(g_fp_mcapara_tv, "%07x\n", 0);
		}
	}
}
void PrintMV_sad_cost(uint32 sad_cost, uint32 mvd_cost, uint8* comment, FILE* fp)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		fprintf(fp, "%08x", sad_cost);
		fprintf(fp, "\t\t// %s :SAD\n", comment);
		fprintf(fp, "%08x", mvd_cost);
		fprintf(fp, "\t\t// MVD Cost\n");
		fprintf(fp, "%08x", sad_cost+mvd_cost);
		fprintf(fp, "\t\t// SAD+MVD Cost\n");
	}
}
void PrintMV_best_sad_cost(uint32 sad_cost, uint8* comment, FILE* fp)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		fprintf(fp, "%08x", sad_cost);
		fprintf(fp, "\t\t// %s \n", comment);
	}
}
void Print_IMB_fbuf(ENC_VOP_MODE_T *p_mode, MB_MODE_E MB_Mode, MOTION_VECTOR_T *mv, MEA_INFO mea_info)
{
	int32 x_min, x_max;
	int32 y_min, y_max;
	int32 i, j, k, x, y;
	int32 start_x = p_mode->mb_x * 16;
	int32 start_y = p_mode->mb_y * 16;
	int32 x_cor, y_cor;
	uint8 *ref_frame_y_ptr = p_mode->pYUVRefFrame->imgY;
	int8 mvx_valid, mvy_valid;
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		x_min = 0; x_max = p_mode->FrameWidth - 1;
		y_min = 0; y_max = p_mode->FrameHeight - 1;

		fprintf(g_fp_mea_mb_ime_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);

		if(MB_Mode==-1)
		{
			for (i=0;i<56;i++)
			{
				fprintf(g_fp_mea_mb_ime_tv, "xxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
			}
			return;
		}

		for(i=0;i<4;i++)
		{
			mvx_valid = (mea_info.mv_valid>>(7-i*2))&0x1;
			mvy_valid = (mea_info.mv_valid>>(6-i*2))&0x1;
			if(MB_Mode != INTER4V)
			{
				x_cor = (start_x + (i%2)*BLOCK_SIZE + mv[0].x) - 1;
				y_cor = (start_y + (i/2)*BLOCK_SIZE + mv[0].y) - 1;
			}
			else
			{
				x_cor = (start_x + (i%2)*BLOCK_SIZE + mv[i+1].x) - 1;
				y_cor = (start_y + (i/2)*BLOCK_SIZE + mv[i+1].y) - 1;
			}

			for(j=0;j<14;j++)
			{
				y = y_cor + j;
				if(j<10)
				{
					fprintf(g_fp_mea_mb_ime_tv, "%8s", "xxxxxxxx");	
					for(k=9;k>=0;k--)
					{
						x = x_cor + k;
						if((x<x_min)||(x>x_max)||(y<y_min)||(y>y_max)||(!mvy_valid && ((j==0)||(j==9)))||(!mvx_valid && ((k==0)||(k==9))))
							fprintf(g_fp_mea_mb_ime_tv, "%2s", "xx");
						else
							fprintf(g_fp_mea_mb_ime_tv, "%02x", ref_frame_y_ptr[x+y*p_mode->FrameWidth]);
					}
				}
				else
				{
					fprintf(g_fp_mea_mb_ime_tv, "%28s", "xxxxxxxxxxxxxxxxxxxxxxxxxxxx");
				}
				fprintf(g_fp_mea_mb_ime_tv, "\n");
			}
		}
	}
}
void PrintMV_ime_src_mb(ENC_VOP_MODE_T *p_mode, MEA_INFO mea_info)
{
	uint8 *imgY, *imgUV, *pCurMB_Y, *pCurMB_UV, *ptmp_UV;
	int8 i,j;
	unsigned __int64 temp = 0;
	
	if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
	{
		imgY = p_mode->pYUVSrcFrame->imgY;
		imgUV = p_mode->pYUVSrcFrame->imgU;
		//imgV = p_mode->pYUVSrcFrame->imgU + (p_mode->FrameHeight>>1)*(p_mode->FrameWidth>>1);
		pCurMB_Y = imgY + (p_mode->mb_y*p_mode->FrameWidth + p_mode->mb_x)*MB_SIZE;
		pCurMB_UV = imgUV + (p_mode->mb_y*p_mode->MBNumX*(MB_SIZE>>1) + p_mode->mb_x)*(MB_SIZE);
		//pCurMB_V = imgV + (p_mode->mb_y*p_mode->MBNumX*(MB_SIZE>>1) + p_mode->mb_x)*(MB_SIZE>>1);

		fprintf(g_fp_mea_src_mb_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		fprintf(g_fp_mea_mb_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		//Y
		for(i=0;i<16;i++)
		{
			for(j=7;j>=0;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_Y[j]);
				fprintf(g_fp_mea_mb_tv, "%02x", pCurMB_Y[j]);
			}
			fprintf(g_fp_mea_src_mb_tv, "\n");
			fprintf(g_fp_mea_mb_tv, "\n");
			for(j=15;j>=8;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_Y[j]);
				fprintf(g_fp_mea_mb_tv, "%02x", pCurMB_Y[j]);
			}
			pCurMB_Y += p_mode->FrameWidth;
			fprintf(g_fp_mea_src_mb_tv, "\n");
			fprintf(g_fp_mea_mb_tv, "\n");
		}
		//UV
		ptmp_UV = pCurMB_UV;
		//ptmp_V = pCurMB_V;
		for(i=0;i<8;i++)
		{
			/*for(j=3;j>=0;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%03d ", pCurMB_U[j]);
			}
			pCurMB_U += p_mode->FrameWidth/2;
			fprintf(g_fp_mea_src_mb_tv, "\n");
			for(j=3;j>=0;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%03d ", pCurMB_V[j]);
			}
			pCurMB_V += p_mode->FrameWidth/2;
			fprintf(g_fp_mea_src_mb_tv, "\n");*/
			/*for(j=3;j>=0;j--)//not interleaved
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_V[j]);
			}
			for(j=3;j>=0;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_U[j]);
			}
			fprintf(g_fp_mea_src_mb_tv, "\n");
			for(j=7;j>=4;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_V[j]);
			}
			for(j=7;j>=4;j--)
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", pCurMB_U[j]);
			}*/
			for (j=3;j>=0;j--)//V
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", ptmp_UV[j*2+1]);
			}
			for (j=3;j>=0;j--)//U
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", ptmp_UV[j*2]);
			}
			fprintf(g_fp_mea_src_mb_tv, "\n");
			for (j=7;j>=4;j--)//V
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", ptmp_UV[j*2+1]);
			}
			for (j=7;j>=4;j--)//U
			{
				fprintf(g_fp_mea_src_mb_tv, "%02x", ptmp_UV[j*2]);
			}
			ptmp_UV += p_mode->FrameWidth;
			//pCurMB_V += p_mode->FrameWidth/2;
			fprintf(g_fp_mea_src_mb_tv, "\n");
		}
		ptmp_UV = pCurMB_UV;
		for(i=0;i<8;i++)//U
		{
			for(j=7;j>=0;j--)
			{
				fprintf(g_fp_mea_mb_tv, "%02x", ptmp_UV[j*2]);
			}
			ptmp_UV += p_mode->FrameWidth;
			fprintf(g_fp_mea_mb_tv, "\n");
		}
		ptmp_UV = pCurMB_UV;
		for(i=0;i<8;i++)//V
		{
			for(j=7;j>=0;j--)
			{
				fprintf(g_fp_mea_mb_tv, "%02x", ptmp_UV[j*2+1]);
			}
			ptmp_UV += p_mode->FrameWidth;
			fprintf(g_fp_mea_mb_tv, "\n");
		}
	if (p_mode->mb_x == 11 && p_mode->mb_y==10 && g_nFrame_enc==4)
	{
		p_mode = p_mode;
	}
		if(p_mode->VopPredType != IVOP)
		{
			temp |= ((__int64)mea_info.partition_mode<<1);//partition_mode
			temp |= ((__int64)(mea_info.mb_y&0x7f)<<2);//curr_mb_y
			temp |= ((__int64)(mea_info.mb_x&0x7f)<<9);//curr_mb_x
			temp |= ((__int64)(mea_info.mv_valid&0xff)<<16);//mv_vaild
			temp |= ((__int64)(mea_info.QP&0x3f)<<40);//qp[5:0]
			temp |= ((__int64)(mea_info.total_cost_min&0xfff)<<48);// total_cost_min[11:0]
			fprintf(g_fp_mea_src_mb_tv, "%016I64x\n", (__int64)temp);

			temp = 0;
			temp |= ((__int64)(mea_info.imv_x_blk3&0x7f)<<0);//imv_x_blk3[6:0]
			temp |= ((__int64)(mea_info.imv_y_blk3&0x3f)<<7);//imv_y_blk3[5:0]
			temp |= ((__int64)(mea_info.imv_x_blk2&0x7f)<<16);//imv_x_blk2[6:0]
			temp |= ((__int64)(mea_info.imv_y_blk2&0x3f)<<23);//imv_y_blk2[5:0]
			temp |= ((__int64)(mea_info.imv_x_blk1&0x7f)<<32);//imv_x_blk1[6:0]
			temp |= ((__int64)(mea_info.imv_y_blk1&0x3f)<<39);//imv_y_blk1[5:0]
			temp |= ((__int64)(mea_info.imv_x_blk0&0x7f)<<48);//imv_x_blk0[6:0]
			temp |= ((__int64)(mea_info.imv_y_blk0&0x3f)<<55);//imv_y_blk0[5:0]
			fprintf(g_fp_mea_src_mb_tv, "%016I64x\n", (__int64)temp);
		}
		else
		{
			temp |= ((__int64)0<<1);//partition_mode
			temp |= ((__int64)(p_mode->mb_y&0x7f)<<2);//curr_mb_y
			temp |= ((__int64)(p_mode->mb_x&0x7f)<<9);//curr_mb_x
			temp |= ((__int64)(0&0xff)<<16);//mv_vaild
			temp |= ((__int64)(p_mode->StepI&0x3f)<<40);//qp[5:0]
			temp |= ((__int64)(0&0xfff)<<48);// total_cost_min[11:0]
			fprintf(g_fp_mea_src_mb_tv, "%016I64x\n", (__int64)temp);
			
			fprintf(g_fp_mea_src_mb_tv, "%016I64x\n", (__int64)0);//mv = 0
		}
	}//if(g_vector_enable_flag&VECTOR_ENABLE_FW)
}
void PrintDCT_TV()
{
	ENC_VOP_MODE_T  *pVop_mode = Mp4Enc_GetVopmode();
	int16 *temp = (int16 *)vsp_dct_io_1;
	int16 *temp2;
	int16 i, j, blk_line_num;
	int16 reorder[384], dctAsic[64];
	if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
	{
		memcpy(reorder, vsp_dct_io_1, sizeof(int16)*384);
		AsicOrderBlock(reorder+0, dctAsic);
		memcpy(reorder, dctAsic, sizeof(dctAsic));
		AsicOrderBlock(reorder+64, dctAsic);
		memcpy(reorder+64, dctAsic, sizeof(dctAsic));
		AsicOrderBlock(reorder+128, dctAsic);
		memcpy(reorder+128, dctAsic, sizeof(dctAsic));
		AsicOrderBlock(reorder+192, dctAsic);
		memcpy(reorder+192, dctAsic, sizeof(dctAsic));
		AsicOrderBlock(reorder+256, dctAsic);
		memcpy(reorder+256, dctAsic, sizeof(dctAsic));
		AsicOrderBlock(reorder+320, dctAsic);
		memcpy(reorder+320, dctAsic, sizeof(dctAsic));
		fprintf(g_fp_dct_out_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, pVop_mode->mb_x, pVop_mode->mb_y);
		//Y
		temp = reorder;
		for(j=0;j<8*2;j++)
		{
			blk_line_num = (int16)(j/8);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 3 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 2 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 1 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + (j%8)*8 + 0 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 7 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 6 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 5 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + (j%8)*8 + 4 + blk_line_num*128)&0xffff);//block0
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 3 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 2 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 1 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + (j%8)*8 + 8*8 + 0 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 7 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 6 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + (j%8)*8 + 8*8 + 5 + blk_line_num*128)&0xffff);//block1
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + (j%8)*8 + 8*8 + 4 + blk_line_num*128)&0xffff);//block1
		}
		//U
		temp = (int16 *)(reorder)+MB_SIZE*MB_SIZE;
		for (j=0;j<8;j++)
		{
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 3)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 2)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 1)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 0)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 7)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 6)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x" ,  *(temp + 5)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 4)&0xffff);
			temp += 8;
		}
		//V
		temp = (int16 *)(reorder)+MB_SIZE*MB_SIZE+BLOCK_SIZE*BLOCK_SIZE;
		for (j=0;j<8;j++)
		{
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 3)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 2)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 1)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 0)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 7)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 6)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x"  , *(temp + 5)&0xffff);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 4)&0xffff);
			temp += 8;
		}
		//Y DC
		temp = (int16 *)reorder;
		{
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp +   0)&0xffff);//Y DC0
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp +  64)&0xffff);//Y DC1
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 128)&0xffff);//Y DC2
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n", *(temp + 192)&0xffff);//Y DC3
		}
		//U DC
		temp = (int16 *)(reorder) + MB_SIZE*MB_SIZE;
		{
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n"   , (*(temp)&0xffff));//U DC
		}
		//V DC
		temp = (int16 *)(reorder) + MB_SIZE*MB_SIZE + BLOCK_SIZE*BLOCK_SIZE;
		{
			fprintf(g_fp_dct_out_tv, "%0.12x", 0);
			fprintf(g_fp_dct_out_tv, "%0.4x\n"   , (*(temp)&0xffff));//V DC
		}
		//Y nzf counter & Coeff nz-flag
		{
			int16 nzc;
			__int64 nzflag[6];
			fprintf(g_fp_dct_out_tv, "%0.8x", 0);
			for (i=3;i>=0;i--)
			{
				nzc = 0;
				nzflag[i] = 0;
				temp = (int16 *)reorder;
				//temp += 64;
				for (j=64;j>=0;j--)
				{
					//temp += j;
					if(*(temp+j+i*64))
					{
						nzc++;
						nzflag[i] |= 1;
					}
					if(j>0)
						nzflag[i] = nzflag[i] << 1;
				}
				fprintf(g_fp_dct_out_tv, "%0.2x", nzc);
					
			}
			fprintf(g_fp_dct_out_tv, "\n");
			fprintf(g_fp_dct_out_tv, "%0.16x\n", 0);
			//UV nzf counter & Coeff nz-flag
			for (i=5;i>=4;i--)
			{
				fprintf(g_fp_dct_out_tv, "%0.6x", 0);
				nzc = 0;
				nzflag[i] = 0;
				//temp = (int16 *)(vsp_dct_io_1) + i*64;
				temp = (int16 *)reorder;
				for (j=63;j>=0;j--)
				{
					//temp += j;
					if(*(temp+j+i*64))
					{
						nzc++;
						nzflag[i] |= 1;
					}
					if(j>0)
						nzflag[i] = nzflag[i] << 1;
					//temp++;
				}
				fprintf(g_fp_dct_out_tv, "%0.2x", nzc);
			}
			fprintf(g_fp_dct_out_tv, "\n");

			{
				int16 YDC, UDC, VDC;
				temp = (int16 *)reorder;
				YDC = UDC = VDC = 0;
				YDC = ((*temp)!=0) + (*(temp+64)!=0) + (*(temp+128)!=0) + (*(temp+192)!=0);
				UDC = (*(temp+256)!=0);
				VDC = (*(temp+320)!=0);
				fprintf(g_fp_dct_out_tv, "%0.10x%0.2x%0.2x%0.2x\n", 0, VDC, UDC, YDC);
			}
			/*for (i=0;i<6;i++)
			{
				int32 tmp0 = nzflag[i]&0xff;
				int32 tmp1 = (nzflag[i]>>8)&0xff;
				int32 tmp2 = (nzflag[i]>>16)&0xff;
				int32 tmp3 = (nzflag[i]>>24)&0xff;
				int32 tmp4 = (nzflag[i]>>32)&0xff;
				int32 tmp5 = (nzflag[i]>>40)&0xff;
				int32 tmp6 = (nzflag[i]>>48)&0xff;
				int32 tmp7 = (nzflag[i]>>56)&0xff;
				fprintf(g_fp_dct_out_tv, "%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n", tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
			}*/
			{
				int32 tmp[6][8];
				for (i=0;i<6;i++)
				{
					for (j=0;j<8;j++)
					{
						tmp[i][7-j] = (nzflag[i]>>(j*8))&0xff;
					}
				}
				//y nzflag
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[1][4], tmp[0][4], tmp[1][5], tmp[0][5], tmp[1][6], tmp[0][6], tmp[1][7], tmp[0][7]);
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[1][0], tmp[0][0], tmp[1][1], tmp[0][1], tmp[1][2], tmp[0][2], tmp[1][3], tmp[0][3]);
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[3][4], tmp[2][4], tmp[3][5], tmp[2][5], tmp[3][6], tmp[2][6], tmp[3][7], tmp[2][7]);
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[3][0], tmp[2][0], tmp[3][1], tmp[2][1], tmp[3][2], tmp[2][2], tmp[3][3], tmp[2][3]);
				//uv nzflag
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[4][0], tmp[4][1], tmp[4][2], tmp[4][3], tmp[4][4], tmp[4][5], tmp[4][6], tmp[4][7]);
				fprintf(g_fp_dct_out_tv,"%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x\n"
									   , tmp[5][0], tmp[5][1], tmp[5][2], tmp[5][3], tmp[5][4], tmp[5][5], tmp[5][6], tmp[5][7]);
			}
		}
	}
}
#ifdef VLC_TV_SPLIT
void VLCSplitInit(ENC_VOP_MODE_T *p_mode)
{
	if( ((g_nFrame_enc%FRAME_X) == 0) )
	{
		FILE *dir_config;
		unsigned char s[100];
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_para.txt", g_nFrame_enc);
        assert(NULL != (g_fp_vlc_para_tv = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/dct_para.txt", g_nFrame_enc);
		assert(NULL != (g_fp_dct_para_tv = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/dct_out.txt", g_nFrame_enc);
		assert(NULL != (g_fp_dct_out_tv = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_out.txt", g_nFrame_enc); // vlc_offset.txt
		assert(NULL != (g_fp_vlc_tv = fopen(s, "w")));

		fprintf(g_fp_dct_para_tv, "//dct_mode=1 vsp_standard=1 scale_enable=0\n");
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_golomb.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_golomb_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_level.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_trace_level_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_run_before.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_trace_run_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_coeff.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_trace_token_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_coeff_token.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_table_coeff_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_total_zero.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_table_zero_fp = fopen(s, "w")));
		//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_run_before.txt", g_nFrame_enc);
		//		assert(NULL != (g_vlc_table_run_fp = fopen(s, "w")));
		
		
		sprintf(s, "../../test_vectors/vlc/frame_%03d/config.txt", g_nFrame_enc);
		assert(NULL != (dir_config = fopen(s, "w")));
		fprintf (dir_config, "//vsp_standard\n");
		fprintf (dir_config, "// h263 =0 , mp4=1, vp8=2,flv=3,h264=4,real8=5,real9=6\n");
		fprintf (dir_config, "1\n");
		fprintf (dir_config, "//block number\n");
		fprintf (dir_config, "%x\n", p_mode->MBNumX*p_mode->MBNumY);//img_ptr->frame_size_in_mbs
		fprintf (dir_config, "//pic x size\n");
		fprintf (dir_config, "%x\n", p_mode->MBNumX*MB_SIZE);//(img_ptr->width+15)&0xfffffff0);
		fprintf (dir_config, "//pic y size\n");
		fprintf (dir_config, "%x\n", p_mode->MBNumY*MB_SIZE);//(img_ptr->height+15)&0xfffffff0);
		fprintf (dir_config, "//weight enable\n");
		fprintf (dir_config, "0\n");
		fclose(dir_config);
	}
}

void VLCSplitDeinit(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode)
{
	if( (((g_nFrame_enc%FRAME_X) == (FRAME_X-1)) || ((g_nFrame_enc+1) == g_input->frame_num_enc))
		/*&& (img_ptr->sh.i_first_mb == 0)*/ )
	{
		//		PrintfGolombSyntax();
		//		PrintfVLCTable();
		fclose(g_fp_vlc_para_tv);
		fclose(g_fp_dct_para_tv);
		fclose(g_fp_dct_out_tv);
		fclose(g_fp_vlc_tv);
		//		fclose(g_vlc_golomb_fp);
		//		fclose(g_vlc_trace_level_fp);
		//		fclose(g_vlc_trace_run_fp);
		//		fclose(g_vlc_trace_token_fp);
		//		fclose(g_vlc_table_coeff_fp);
		//		fclose(g_vlc_table_zero_fp);
		//		fclose(g_vlc_table_run_fp);
	}
}
#endif
#if !defined(_LIB)
void PrintfVLCCFG(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
	{
		fprintf(g_fp_mvlc_cfg, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		fprintf(g_fp_mvlc_cfg, "standard=%d\n", g_input->is_short_header?0:1);
		fprintf(g_fp_mvlc_cfg, "mb_type=%d\n", pMb_mode->dctMd==INTRA?0:1);
		fprintf(g_fp_mvlc_cfg, "cur_mb_x=%x\n", p_mode->mb_x);
		fprintf(g_fp_mvlc_cfg, "cur_mb_y=%x\n", p_mode->mb_y);
		fprintf(g_fp_mvlc_cfg, "top_avail=%d\n", p_mode->pMBCache->bTopMBAvail);
		fprintf(g_fp_mvlc_cfg, "left_avail=%d\n", p_mode->pMBCache->bLeftMBAvail);
		fprintf(g_fp_mvlc_cfg, "tl_avail=%d\n", p_mode->pMBCache->bLeftTopAvail);
		fprintf(g_fp_mvlc_cfg, "cbp=%x\n", pMb_mode->CBP);
		if (p_mode->VopPredType==IVOP)
			fprintf(g_fp_mvlc_cfg, "cur_qp=%d\n", p_mode->StepI);
		else
			fprintf(g_fp_mvlc_cfg, "cur_qp=%d\n", p_mode->StepP);
	}
}
void PrintfFrameCfg(ENC_VOP_MODE_T *p_mode, int8 frame_type)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
	{
		fprintf(g_fp_frame_cfg, "work_mode=%d\n", 1);
		fprintf(g_fp_frame_cfg, "standard=%d\n", g_input->is_short_header?0:1);
		fprintf(g_fp_frame_cfg, "mb_x_max=%x\n", p_mode->MBNumX);
		fprintf(g_fp_frame_cfg, "mb_y_max=%x\n", p_mode->MBNumY);
		fprintf(g_fp_frame_cfg, "frm_type=%d\n", frame_type);
		fprintf(g_fp_frame_cfg, "Slice_Qp=%x\n", frame_type==IVOP?p_mode->StepI:p_mode->StepP);
		fprintf(g_fp_frame_cfg, "MB_Qp=%x\n", frame_type==IVOP?p_mode->StepI:p_mode->StepP);
		fprintf(g_fp_frame_cfg, "srch_16_max=%x\n", MAX_SEARCH_CYCLE);
		fprintf(g_fp_frame_cfg, "srch_8_max=%x\n", 1);
		fprintf(g_fp_frame_cfg, "deblocking_eb=%x\n", 0);
		fprintf(g_fp_frame_cfg, "mca_rounding=%x\n", 1);
		fprintf(g_fp_frame_cfg, "slice_ver_num=%x\n", p_mode->mbline_num_slice);
	}
}
void PrintfTV(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 frame_type)
{
	PrintfSrcMB(p_mode);
	PrintfMEAbufParam(p_mode, pMb_mode, frame_type);
	/*if(frame_type != IVOP)
	{
		//PrintfSrcMB(p_mode);
		PrintfMEAbufParam(p_mode, pMb_mode, frame_type);
	}*/
	if (g_vector_enable_flag&VECTOR_ENABLE_DBK)
	{
		uint32 temp = 0;
		int i;
		fprintf(g_fp_dbk_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		temp |= (((p_mode->mb_y+1)%p_mode->mbline_num_slice==0 ||(p_mode->mb_y+1)== p_mode->MBNumY) && (p_mode->mb_x+1)==p_mode->MBNumX)<<31;//Is_Last_mb[31] last mb of slice
		temp |= (p_mode->mb_x<<8);//cur_mb_x[15:8]
		temp |= (p_mode->mb_y);//cur_mb_x[7:0]
		fprintf(g_fp_dbk_tv, "%08x\n", temp);
		for (i=0;i<7;i++)
		{
			fprintf(g_fp_dbk_tv, "%08x\n", 0);
		}
	}
	if (g_vector_enable_flag&VECTOR_ENABLE_MBC)
	{
		int i;
		fprintf(g_fp_mbc_ipred, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, p_mode->mb_x, p_mode->mb_y);
		for (i=0;i<48;i++)
		{
			fprintf(g_fp_mbc_ipred, "%016x\n", 0);
		}
	}
	PrintfMBCPARA(p_mode, pMb_mode, frame_type);
	PrintfVLCParabuf(p_mode, pMb_mode, frame_type);
	PrintfDCTParabuf(p_mode, pMb_mode, frame_type);
//	PrintfMCAParabuf(p_mode, pMb_mode, frame_type);
	PrintfPPACFG(p_mode, pMb_mode, frame_type);
	PrintfPPAIN(p_mode, pMb_mode, frame_type);
//	Mp4Enc_Trace_MBC_result_one_macroblock(p_mode, pMb_mode);
}
#endif
#if defined(MPEG4_ENC)
/*printf fetched reference Y data for verification*/
void trace_mea_fetch_one_macroblock(MEA_FETCH_REF *pMea_fetch)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int32 frame_width = g_mea_frame_width<<2; //byte
	int32 frame_height = g_mea_frame_height;
	uint8 * pRefChar;
	uint32 * pRefInt;
	int i, j;
	uint32 fourPix;
	int start_Y = pMea_fetch->start_Y;
	int start_X = pMea_fetch->start_X;
	int fetch_XWidth = pMea_fetch->fetch_XWidth; 
	int fetch_YHeigth = pMea_fetch->fetch_YHeigth;

	pRefChar = g_enc_vop_mode_ptr->pYUVRefFrame->imgY + start_Y*frame_width + start_X;

	for (j = 0; j < fetch_YHeigth; j++)
	{
		pRefInt = (uint32 *)pRefChar;
		for (i = 0; i < fetch_XWidth/4; i++)
		{
			fourPix = pRefInt [i];
			fprintf (g_fp_mea_tv, "%08x\n", fourPix);
		}

		pRefChar += frame_width;
	}
}
}
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
