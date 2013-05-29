#include "sci_types.h"
#include "video_common.h"
#include "sc6800x_video_header.h"

#ifdef SIM_IN_WIN
#include "tv_mea.h"
#include <time.h>
#include <sys/timeb.h>

FILE * stat_out;

//void set_default_param (x264_PARAMS_T *param)
//{
//	memset (param, 0, sizeof(x264_PARAMS_T));
//
//	//video properties
//	param->i_width = 0;
//	param->i_height = 0;
//	param->vui.i_sar_width	= 0;
//	param->vui.i_sar_height = 0;
//	param->vui.i_overscan	= 0; //undef
//	param->vui.i_vidformat	= 0;	//undef
//	param->vui.i_transfer	= 2;	//undef
//	param->vui.i_colmatrix	= 2;	//undef
//	param->vui.i_chroma_loc	= 0;	//left center
//	param->i_fps_num	= 25;
//	param->i_fps_den	= 1;
//	param->i_level_idc	= 51;	//as close to "unresticted as we can get
//
//	//encoder parameters
//	param->i_frame_reference_num	= 1;
//	param->i_keyint_max	= 250;
//	param->i_keyint_min	= 25;
//	param->i_scenecut_threshold	= 40;
//	
//	param->b_deblocking_filter	= 1;
//	param->i_deblocking_filter_alphac0	= 0;
//	param->i_deblocking_filter_beta		= 0;
//
//	param->analyse.intra = X264_ANALYSE_I4x4;
//	param->analyse.inter	= X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
//	param->analyse.i_me_method	= X264_ME_DIA;
//	param->analyse.i_me_range	= 16;
//	param->analyse.i_subpel_refine	= 5;
//	param->analyse.i_mv_range	= -1;	//set from level_idc
//	param->analyse.i_chroma_qp_offset	= 0;
//	param->analyse.b_fast_pskip	= 1;
//	param->analyse.b_dct_decimate = 1;
//	param->i_cqm_preset	= X264_CQM_FLAT;	
//}

void H264Enc_PrintStat (int enc_frame_num)
{
	double avg_psnr[3];

	avg_psnr[0] = g_PSNR[0] / enc_frame_num;
	avg_psnr[1] = g_PSNR[1] / enc_frame_num;
	avg_psnr[2] = g_PSNR[2] / enc_frame_num;

	printf ("psnr_y: %f, psnr_u: %f, pnsr_v: %f\n", avg_psnr[0], avg_psnr[1], avg_psnr[2]);
	fprintf(stat_out, "psnr_y: %f, psnr_u: %f, pnsr_v: %f\n", avg_psnr[0], avg_psnr[1], avg_psnr[2]);
}

double H264Enc_ComputeSNRY(uint8 *pSrc_frame,  uint8 *pRec_frame, int frame_width, int frame_height)
{
	int x, y;
	int32 diff;
	int32 sqr_diff = 0;
	double max_val;
	uint8 *pSrc;
	uint8 *pRec;
	double psnr;
	
	max_val = 255; 
	
	pSrc = pSrc_frame;
	pRec = pRec_frame;
	
	for (y = 0; y < frame_height; y++)
	{
		for (x = 0; x < frame_width; x++)
		{
			diff = (pSrc [x]) - (pRec [x]);
			sqr_diff += diff * diff;			
		}
		
		pSrc += frame_width;
		pRec += frame_width;
	}
	
	if(sqr_diff == 0)
	{
		psnr = 10000.0;
	}else
	{
		psnr = (log10 ((max_val*max_val*frame_width*frame_height) /(double) sqr_diff) * 10.0);
	}
	
	return psnr;
}

void H264Enc_ComputePSNR (ENC_IMAGE_PARAMS_T * img_ptr)
{
	double psnr[3];
	int frame_width  = img_ptr->frame_width_in_mbs * 16;
	int frame_height = img_ptr->frame_height_in_mbs * 16;
	
	psnr[0] = H264Enc_ComputeSNRY (img_ptr->pYUVSrcFrame->imgY, img_ptr->pYUVRecFrame->imgY, frame_width, frame_height);
	psnr[1] = H264Enc_ComputeSNRY (img_ptr->pYUVSrcFrame->imgUV, img_ptr->pYUVRecFrame->imgUV, frame_width, frame_height/2);
//	psnr[2] = H264Enc_ComputeSNRY (img_ptr->pYUVSrcFrame->imgUV, img_ptr->pYUVRecFrame->imgUV, frame_width, frame_height/2);
	psnr[2] = 0;
	
	g_PSNR[0] += psnr[0];
	g_PSNR[1] += psnr[1];
	g_PSNR[2] += psnr[2];
	
	PRINTF ("\t psnr_y: %f, psnr_u: %f, psnr_v: %f\n", psnr[0], psnr[1], psnr[2]);	
	fprintf(stat_out, "\t psnr_y: %f, psnr_u: %f, psnr_v: %f\n", psnr[0], psnr[1], psnr[2]);
}

BOOLEAN H264Enc_ReadoutOneFrame(uint32 frame_num, uint32 frame_size, uint8 *pYuv_frame)
{
#ifdef _ARM_
	uint32 offset;
	const uint8 *pInput_yuv = g_pEnc_input_yuv_buffer;
	const uint8 *pIn_yuv;	
	
	offset = frame_num * frame_size * 3/2;
	
	pIn_yuv = &(pInput_yuv[offset]);
	
	memcpy(pYuv_frame, pIn_yuv, frame_size *3/2);
	
	return FALSE;

#else
	uint32 offset;
	uint32 length;
	FILE *pf_Yuv = s_pEnc_input_src_file;
		
	offset = frame_num * frame_size*3/2;
	
	fseek(pf_Yuv, offset, SEEK_SET);

	length = fread(pYuv_frame, 1, frame_size*3/2, pf_Yuv);
	if(length < frame_size)
	{
		return TRUE;
	}

	return FALSE;
#endif
}
#endif // SIM_IN_WIN

void Write_ARM_GlbReg();
void H264Enc_main()
{
#ifdef SIM_IN_WIN
	uint8 *pEncBs = NULL;
	uint8 *enc_yuv_one_frame_buffer;
	uint32 enc_frame_size;
	uint32 uEncBsTotalBits = 0;
	BOOLEAN is_stop_encode = FALSE;
	uint32 frame_rate = 30;
#endif
	uint32	enc_frame_num = 0;
	//uint32	enc_last_frame = 20;	
	//uint32 frame_num = 0;
	MMEncIn *enc_input_ptr = NULL;
	MMEncOut *enc_output_ptr = NULL;
	//MMEncVideoInfo video_info;
	//MMEncOut *enc_header_output_ptr = NULL;
	//MMEncConfig *enc_config_ptr = NULL;
	//MMCodecBuffer *enc_common_bfr_ptr = NULL;
	MMCodecBuffer *enc_inter_malloc_bfr_ptr = NULL;
	MMCodecBuffer *enc_extra_malloc_bfr_ptr = NULL;

#ifdef SIM_IN_WIN
	struct timeb tstruct_start;
	struct timeb tstruct_end;
	time_t	total_time = 0;
	time_t ltime_start;               // for time measurement
	time_t ltime_end;                 // for time measurement

	g_steps_total16 = g_steps_total8 = g_step_count = 0;
#endif

#ifdef SIM_IN_WIN
	SCI_ASSERT(NULL != (enc_input_ptr = (MMEncIn *)MallocInterMem(sizeof(MMEncIn))));
	SCI_ASSERT(NULL != (enc_output_ptr = (MMEncOut *)MallocInterMem(sizeof(MMEncOut))));
#endif
	//SCI_ASSERT(NULL != (enc_header_output_ptr = (MMEncOut *)MallocExtraMem(sizeof(MMEncOut))));
	//SCI_ASSERT(NULL != (enc_config_ptr = (MMEncConfig *)MallocExtraMem(sizeof(MMEncConfig))));
	SCI_ASSERT(NULL != (enc_inter_malloc_bfr_ptr = (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer))));
	SCI_ASSERT(NULL != (enc_extra_malloc_bfr_ptr = (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer))));

	enc_inter_malloc_bfr_ptr->size = total_inter_malloc_size - g_inter_malloced_size;
	SCI_ASSERT(NULL != (enc_inter_malloc_bfr_ptr->common_buffer_ptr = (uint8 *)MallocInterMem(enc_inter_malloc_bfr_ptr->size)));
//	memset(enc_inter_malloc_bfr_ptr->common_buffer_ptr, 0, enc_inter_malloc_bfr_ptr->size);
 
	enc_extra_malloc_bfr_ptr->size = total_extra_malloc_size - g_extra_malloced_size;
	SCI_ASSERT(NULL != (enc_extra_malloc_bfr_ptr->common_buffer_ptr = (uint8 *)MallocExtraMem(enc_extra_malloc_bfr_ptr->size)));
//	memset(enc_extra_malloc_bfr_ptr->common_buffer_ptr, 0, enc_extra_malloc_bfr_ptr->size);
	
#ifdef SIM_IN_WIN
	stat_out = fopen("stats_out.log", "w");//jin
	fprintf(stat_out,"Sequence:\t %s,\tQP: %d\n", g_input->infile,g_input->qp_PSLICE);
#endif

	g_nFrame_enc = 0;
	H264EncInit(enc_inter_malloc_bfr_ptr, enc_extra_malloc_bfr_ptr/*, param*/);

#ifdef SIM_IN_WIN
	enc_frame_size = g_input->ori_height * g_input->ori_width;
	SCI_ASSERT(NULL != (enc_yuv_one_frame_buffer = (uint8 *)h264enc_extra_mem_alloc(enc_frame_size*3/2)));

	//g_bit_stat_fp = fopen ("..\\..\\trace\\bit_trace.txt", "w");	
	TVMeaInit ();
	ftime (&tstruct_start);
	time  ( &(ltime_start));

	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
#endif

	while(1)
	{
#ifdef SIM_IN_WIN
		if(g_enc_image_ptr->sh.i_first_mb == 0)
			is_stop_encode = H264Enc_ReadoutOneFrame (enc_frame_num, enc_frame_size, enc_yuv_one_frame_buffer);
		
		if(is_stop_encode)
			break;

		FPRINTF(g_fp_trace_fw, "enc frame num: %d\n", enc_frame_num);
		fprintf(stat_out, "enc frame num: %d\n", enc_frame_num);
		
		enc_input_ptr->p_src_y	  = enc_yuv_one_frame_buffer;
		enc_input_ptr->p_src_u	  = enc_input_ptr->p_src_y+enc_frame_size;
		
		enc_output_ptr->pOutBuf = pEncBs;
		or1200_print = 1;
#else
		//第一次进入或重新进入或继续循环
		OR1200_READ_REG_POLL(ORSC_SHARERAM_OFF+0, 0x80000000,0x80000000, "ORSC_SHARE: shareRAM 0x0 MODE_CFG run"); //polling until can run	
		//如果reset后重新进入，需恢复现场寄存器        
		if(g_not_first_reset)	// var need update?
		{
			//恢复现场寄存器
		}
		if(g_enc_image_ptr->sh.i_first_mb == 0)
		{
			int tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
			input_buffer_update = (tmp>>8)&1; //1
			if(input_buffer_update)
			{
				g_enc_image_ptr->stm_offset = 0;
				g_enc_image_ptr->pOneFrameBitstream = (uint8*)OR1200_READ_REG(ORSC_SHARERAM_OFF+0x18,"ORSC_SHARE: STREAM_BUF_ADDR");
				g_enc_image_ptr->pYUVSrcFrame->imgY = (uint8*)OR1200_READ_REG(ORSC_SHARERAM_OFF+0x2c,"ORSC_SHARE: Frame_Y_ADDR");
				g_enc_image_ptr->pYUVSrcFrame->imgUV = (uint8*)OR1200_READ_REG(ORSC_SHARERAM_OFF+0x30,"ORSC_SHARE: Frame_UV_ADDR");
				frame_buf_size = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x34, "ORSC_SHARE: UV_FRAME_BUF_SIZE");	// Y_frame_buffer_size = 2*UV_frame_buffer_size, R for encoder mode
				// input_buffer_update = 0;
				OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x0, tmp&0xfffffeff,"ORSC_SHARE: MODE_CFG clear input_buffer_update");

				// Frame Crop
				if( (g_input->ori_width != g_input->pic_width) || (g_input->ori_height != g_input->pic_height) )
				{
					//g_enc_image_ptr->crop_x = Clip3(0, (g_input->ori_width-g_input->pic_width), g_enc_image_ptr->crop_x+2*1);
					//g_enc_image_ptr->crop_y = Clip3(0, (g_input->ori_height-g_input->pic_height), g_enc_image_ptr->crop_y+2*1);
					tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x44, "ORSC_SHARE: Size_crop");
					g_enc_image_ptr->crop_x = ((tmp>>8)&0xff);// Left
					g_enc_image_ptr->crop_y = (tmp & 0xff);	// Top
					g_enc_image_ptr->crop_x = Clip3(0, (g_input->ori_width-g_input->pic_width), g_enc_image_ptr->crop_x);
					g_enc_image_ptr->crop_y = Clip3(0, (g_input->ori_height-g_input->pic_height), g_enc_image_ptr->crop_y);
					
					g_enc_image_ptr->pYUVSrcFrame->imgYAddr = (uint32)(g_enc_image_ptr->pYUVSrcFrame->imgY + g_input->ori_width * g_enc_image_ptr->crop_y) >> 3;
					g_enc_image_ptr->pYUVSrcFrame->imgUVAddr = (uint32)(g_enc_image_ptr->pYUVSrcFrame->imgUV + g_input->ori_width * (g_enc_image_ptr->crop_y>>1)) >> 3;
				}
				else
				{
					g_enc_image_ptr->pYUVSrcFrame->imgYAddr = (uint32)g_enc_image_ptr->pYUVSrcFrame->imgY >> 3;
					g_enc_image_ptr->pYUVSrcFrame->imgUVAddr = (uint32)g_enc_image_ptr->pYUVSrcFrame->imgUV >> 3;
				}
			}
		}
#endif
		H264EncStrmEncode(enc_input_ptr, enc_output_ptr);
		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
#ifdef SIM_IN_WIN
		PRINTF("\t%d\n", enc_output_ptr->strmSize*8);
		fprintf(stat_out, "\t%d\n", enc_output_ptr->strmSize);
		if(g_enc_image_ptr->sh.i_first_mb == 0)
			H264Enc_ComputePSNR (g_enc_image_ptr);
		
		pEncBs += enc_output_ptr->strmSize;
		uEncBsTotalBits += enc_output_ptr->strmSize;
#endif
		if(g_enc_image_ptr->sh.i_first_mb == 0)
		{
#ifdef SIM_IN_WIN
#else
			/*uint32 offset = ((uint32)g_enc_image_ptr->pOneFrameBitstream);
			int i;
			offset += g_enc_image_ptr->stm_offset;

			for(i=0; i<g_enc_image_ptr->slice_nr; i++)
			{
				if( *((uint32 volatile *)offset) == 0xffffffff )
					*((uint32 volatile *)offset) = 0x00000001;
				offset += (g_enc_image_ptr->slice_sz[i] >> 3);
			}*/
#ifdef FPGA_AUTO_VERIFICATION
			uint32 tmp;
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x20, g_enc_image_ptr->stm_offset, "ORSC_SHARE: stream_len used");
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x00, tmp&0x7fffffff, "ORSC_SHARE: MODE_CFG Stop");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
#endif
#endif
			enc_frame_num++;
		}
	
		if (enc_frame_num >= g_input->frame_num_dec)
			break;
	}


#ifdef SIM_IN_WIN
#ifdef TV_OUT
#ifndef VLC_TV_SPLIT
//	PrintfGolombSyntax();
//	PrintfVLCTable();
	{
		FILE *dir_config;
		assert(NULL != (dir_config = fopen("../../test_vectors/config.txt", "w")));
		fprintf (dir_config, "//vsp_standard\n");
		fprintf (dir_config, "// h263 =0 , mp4=1, vp8=2,flv=3,h264=4,real8=5,real9=6\n");
		fprintf (dir_config, "4\n");
		fprintf (dir_config, "//block number\n");
		fprintf (dir_config, "%x\n", (g_enc_image_ptr->frame_size_in_mbs * enc_frame_num));
		fprintf (dir_config, "//pic x size\n");
		fprintf (dir_config, "%x\n", (g_enc_image_ptr->width+15)&0xfffffff0);
		fprintf (dir_config, "//pic y size\n");
		fprintf (dir_config, "%x\n", (g_enc_image_ptr->height+15)&0xfffffff0);
		fprintf (dir_config, "//weight enable\n");
		fprintf (dir_config, "0\n");
		fclose(dir_config);
	}
#endif
	Write_ARM_GlbReg();
	PrintBSMLineNum();
#endif

	FPRINTF_ORSC(g_vsp_glb_reg_fp, "f_01234567_89abcdef\n");

	ftime (&tstruct_end);
	time  ( &(ltime_end)); 

	total_time += (ltime_end * 1000 + tstruct_end.millitm) - (ltime_start * 1000 + tstruct_start.millitm);

	PRINTF("bit-rate:%.2fkb/s\tdecoding time: %.3f sec\t", ((float)uEncBsTotalBits)/g_input->frame_num_dec*frame_rate/125, total_time*0.001);
	fprintf(stat_out, "bit-rate:%.2fkb/s\t", ((float)uEncBsTotalBits)/g_input->frame_num_dec*frame_rate/125);
	fprintf(stat_out, "decoding time: %.3f sec\t", total_time*0.001);
//	fprintf(stat_out, "\naverage_all %d,    average_16x16_step = %d, 8x8 step step = %d\n", g_step_count/((g_input->frame_num_dec-1)*(g_input->pic_width/16)*(g_input->pic_height/16)), g_steps_total16/((g_input->frame_num_dec-1)*(1280/16)*(720/16)), g_steps_total8/((g_input->frame_num_dec-1)*(g_input->pic_width/16)*(g_input->pic_height/16)));

	H264Enc_PrintStat (enc_frame_num);
	fclose(stat_out);
#endif

#ifdef ORSC_FW
	{
		uint32 tmp;
		tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
		OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x00, tmp&0x7fffffff, "ORSC_SHARE: MODE_CFG Stop");

		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");
		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
	}
#endif
}


#ifdef SIM_IN_WIN
#define ARM_WRITE_REG(reg_addr, value, pstring) fprintf(arm_fp,"1_%08x_%08x   //%s\n", reg_addr, value, pstring)
void Write_ARM_GlbReg()
{
	FILE *arm_fp = fopen ("..\\..\\test_vectors\\arm_glb_reg.txt", "w");
	uint32 cmd;

	fprintf(arm_fp,
		"1_20900400_18000000   //boot code\n"
		"1_20900404_a8204005   //boot code\n"
		"1_20900408_c0000811   //boot code\n"
		"1_2090040c_c1400000   //boot code\n"
		"1_20900410_18200007   //boot code\n"
		"1_20900414_0400000f   //boot code\n"
		"1_20900418_15000000   //boot code\n"
		"1_2090041c_18a08000   //boot code\n"
		"1_20900420_9ca502fc   //boot code\n"
		"1_20900424_18c01234   //boot code\n"
		"1_20900428_9cc65678   //boot code\n"
		"1_2090042c_84e50000   //boot code\n"
		"1_20900430_e4273000   //boot code\n"
		"1_20900434_13ffffff   //boot code\n"
		"1_20900438_84e50000   //boot code\n"
		"1_2090043c_18a00000   //boot code\n"
		"1_20900440_48002800   //boot code\n"
		"1_20900444_15000000   //boot code\n"
		"1_20900448_00000000   //boot code\n"
		"1_2090044c_15000000   //boot code\n"
		"1_20900450_b4c00011   //boot code\n"
		"1_20900454_9ca0ffff   //boot code\n"
		"1_20900458_aca50010   //boot code\n"
		"1_2090045c_e0a62803   //boot code\n"
		"1_20900460_c0002811   //boot code\n"
		"1_20900464_b4600006   //boot code\n"
		"1_20900468_a4830080   //boot code\n"
		"1_2090046c_b8a40047   //boot code\n"
		"1_20900470_a8c00010   //boot code\n"
		"1_20900474_e1c62808   //boot code\n"
		"1_20900478_a4830078   //boot code\n"
		"1_2090047c_b8a40043   //boot code\n"
		"1_20900480_a8c00001   //boot code\n"
		"1_20900484_e0e62808   //boot code\n"
		"1_20900488_9cc00000   //boot code\n"
		"1_2090048c_e0ae2808   //boot code\n"
		"1_20900490_c0803002   //boot code\n"
		"1_20900494_e4262800   //boot code\n"
		"1_20900498_13fffffe   //boot code\n"
		"1_2090049c_e0c67000   //boot code\n"
		"1_209004a0_b4c00011   //boot code\n"
		"1_209004a4_a8c60010   //boot code\n"
		"1_209004a8_c0003011   //boot code\n"
		"1_209004ac_15000000   //boot code\n"
		"1_209004b0_15000000   //boot code\n"
		"1_209004b4_15000000   //boot code\n"
		"1_209004b8_15000000   //boot code\n"
		"1_209004bc_15000000   //boot code\n"
		"1_209004c0_15000000   //boot code\n"
		"1_209004c4_15000000   //boot code\n"
		"1_209004c8_15000000   //boot code\n"
		"1_209004cc_b4c00011   //boot code\n"
		"1_209004d0_9ca0ffff   //boot code\n"
		"1_209004d4_aca50008   //boot code\n"
		"1_209004d8_e0a62803   //boot code\n"
		"1_209004dc_c0002811   //boot code\n"
		"1_209004e0_b4600005   //boot code\n"
		"1_209004e4_a4830080   //boot code\n"
		"1_209004e8_b8a40047   //boot code\n"
		"1_209004ec_a8c00010   //boot code\n"
		"1_209004f0_e1c62808   //boot code\n"
		"1_209004f4_a4830078   //boot code\n"
		"1_209004f8_b8a40043   //boot code\n"
		"1_209004fc_a8c00001   //boot code\n"
		"1_20900500_e0e62808   //boot code\n"
		"1_20900504_9cc00000   //boot code\n"
		"1_20900508_e0ae2808   //boot code\n"
		"1_2090050c_c0603003   //boot code\n"
		"1_20900510_e4262800   //boot code\n"
		"1_20900514_13fffffe   //boot code\n"
		"1_20900518_e0c67000   //boot code\n"
		"1_2090051c_b4c00011   //boot code\n"
		"1_20900520_a8c60008   //boot code\n"
		"1_20900524_c0003011   //boot code\n"
		"1_20900528_15000000   //boot code\n"
		"1_2090052c_15000000   //boot code\n"
		"1_20900530_15000000   //boot code\n"
		"1_20900534_15000000   //boot code\n"
		"1_20900538_15000000   //boot code\n"
		"1_2090053c_15000000   //boot code\n"
		"1_20900540_44004800   //boot code\n"
		"1_20900544_15000000   //boot code\n"
		"1_20900548_00000000   //boot code\n"
		"1_2090054c_00000000   //boot code\n"
		"1_20900550_00000000   //boot code\n"
		"1_20900554_00000000   //boot code\n"
		"1_20900558_00000000   //boot code\n"
		"1_2090055c_00000000   //boot code\n"
		"1_20900560_00000000   //boot code\n"
		"1_20900564_00000000   //boot code\n"
		"1_20900568_00000000   //boot code\n"
		"1_2090056c_00000000   //boot code\n"
		"1_20900570_00000000   //boot code\n"
		"1_20900574_00000000   //boot code\n"
		"1_20900578_00000000   //boot code\n"
		"1_2090057c_00000000   //boot code\n"
		"1_20900580_00000000   //boot code\n"
		"1_20900584_00000000   //boot code\n"
		"1_20900588_00000000   //boot code\n"
		"1_2090058c_00000000   //boot code\n"
		"1_20900590_00000000   //boot code\n"
		"1_20900594_00000000   //boot code\n"
		"1_20900598_00000000   //boot code\n"
		"1_2090059c_00000000   //boot code\n"
		"1_209005a0_00000000   //boot code\n"
		"1_209005a4_00000000   //boot code\n"
		"1_209005a8_00000000   //boot code\n"
		"1_209005ac_00000000   //boot code\n"
		"1_209005b0_00000000   //boot code\n"
		"1_209005b4_00000000   //boot code\n"
		"1_209005b8_00000000   //boot code\n"
		"1_209005bc_00000000   //boot code\n"
		"1_209005c0_00000000   //boot code\n"
		"1_209005c4_00000000   //boot code\n"
		"1_209005c8_00000000   //boot code\n"
		"1_209005cc_00000000   //boot code\n"
		"1_209005d0_00000000   //boot code\n"
		"1_209005d4_00000000   //boot code\n"
		"1_209005d8_00000000   //boot code\n"
		"1_209005dc_00000000   //boot code\n"
		"1_209005e0_00000000   //boot code\n"
		"1_209005e4_00000000   //boot code\n"
		"1_209005e8_00000000   //boot code\n"
		"1_209005ec_00000000   //boot code\n"
		"1_209005f0_00000000   //boot code\n"
		"1_209005f4_00000000   //boot code\n"
		"1_209005f8_00000000   //boot code\n"
		"1_209005fc_00000000   //boot code\n"
		"1_20900600_c1200002   //boot code\n"
		"1_20900604_24000000   //boot code\n"
		"1_20900608_15000000   //boot code\n"
		"1_20900020_00000000   //Iwb_addr_base\n"
		"1_20900024_00000000   //Dwb_addr_base\n"
		"1_20900210_02600000   //shareRAM 0x10 VSP_MEM1_ST_ADDR\n"
		"1_20900214_00E00000   //shareRAM 0x14 VSP_MEM1_SIZE\n"
		"1_20900234_000ff000   //shareRAM 0x34 UVframe_size\n");

	cmd = 0;
	cmd |= (g_input->pic_width & 0xfff);
	cmd |= (g_input->pic_height & 0xfff) << 12;
	cmd |= 1 << 31;
	ARM_WRITE_REG(0x20900204, cmd, "shareRAM 0x4 pic_height&pic_width");

	cmd = 0;
	cmd |= (g_input->ori_width & 0xfff);
	cmd |= (g_input->ori_height & 0xfff) << 12;
	ARM_WRITE_REG(0x20900250, cmd, "shareRAM 0x50 frame buf ori_width&ori_height");

	cmd = 0;
	cmd |= (target_rate & 0x7fffffff);
	cmd |= rate_control_en << 31;
	ARM_WRITE_REG(0x20900238, cmd, "shareRAM 0x38 RATE_CONTROL");

	fprintf(arm_fp,
		"1_20900014_00000000   //arm int mask set\n"
		"1_20900000_00000000   //RAM_ACC_by arm\n"
		"1_20900208_00080000   //shareRAM 8 VSP_MEM0_ST_ADDR\n"
		"1_2090020c_00100000   //shareRAM c CODE_RUN_SIZE\n"
		"1_209002fc_12345678   //boot code jump to main enable\n"
		"1_20900000_00000000   //RAM_ACC_by arm\n"
		"1_2090022c_00200000   //shareRAM 0x2c frame_Y_addr\n"
		"1_20900230_02400000   //shareRAM 0x30 frame_UV_addr\n"
		"1_20900218_04600000   //shareRAM 0x18 STREAM_BUF_ADDR\n"
		"1_2090021c_003fff70   //shareRAM 0x1c STREAM_BUF_SIZE\n"
		"1_20900220_00000000   //shareRAM 0x20 stream_len\n"
		"1_20900200_80010114   //shareRAM 0\n"
		"1_20900000_00000007   //RAM_ACC_by arm\n"
		"1_20900008_00000001   //VSP clk gate en\n"
		"2_20900010_00000001_00000001   //ARM INT MCU_done\n"
		"1_20900018_00000007   //clr arm int\n"
		"1_20900008_00000000   //VSP clk gate\n");

	fclose(arm_fp);
}
#endif