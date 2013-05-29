/******************************************************************************
 ** File Name:    mp4enc_main.c												  *
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
#if defined(_FPGA_AUTO_VERIFICATION_)
#include "fpga_auto_vrf_def.h"
#endif //_FPGA_AUTO_VERIFICATION_

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC int g_samp_rate;

PUBLIC int g_total_search_point_cnt;

#ifdef _COMPUTE_PSNR_
MP4_LOCAL double g_SNR [3];
MP4_LOCAL double g_PSNRY;
MP4_LOCAL double g_PSNRU;
MP4_LOCAL double g_PSNRV;
#endif

//for rate control
MP4_LOCAL uint32 g_encoded_frame_num;
MP4_LOCAL uint32 g_enc_skip_count;
	
MP4_LOCAL double Mp4Enc_ComputeSNRY(uint8 *pSrc_frame,  uint8 *pRec_frame);
MP4_LOCAL double Mp4Enc_ComputeSNRUV(uint8 *pSrc_frame, uint8 *pRec_frame);

#ifdef _COMPUTE_PSNR_
MP4_LOCAL double Mp4Enc_ComputeSNRY(uint8 *pSrc_frame,  uint8 *pRec_frame)
{
	uint32 x, y;
	int32 diff;
	int32 sqr_diff = 0;
	double max_val;
	uint8 *pSrc;
	uint8 *pRec;
	double psnr;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();
	uint32 frame_width,frame_height;

	frame_width = pVop_mode->FrameWidth;
	frame_height = pVop_mode->FrameHeight;

	max_val = (double)((1<<pVol_mode->nBits)-1); 

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

MP4_LOCAL double Mp4Enc_ComputeSNRUV(uint8 *pSrc_frame, uint8 *pRec_frame)
{
	uint32 x, y;
	int32 diff;
	int32 sqr_diff = 0;
	double psnr;
	double max_val ;
	uint8 *pSrc;
	uint8 *pRec;
	VOL_MODE_T  *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T  *pVop_mode = Mp4Enc_GetVopmode();
	uint32 uv_frame_width,uv_frame_height;

	max_val = (double) ((1<<pVol_mode->nBits)-1); 

	uv_frame_width  = pVop_mode->OrgFrameWidth / 2;
	uv_frame_height = pVop_mode->OrgFrameHeight / 2; 

	pSrc = pSrc_frame;
	pRec = pRec_frame;

	for (y = 0; y < uv_frame_height; y++)
	{
		for (x = 0; x < uv_frame_width; x++)
		{
			diff     = (pSrc [x]) - (pRec [x]);
			sqr_diff += diff * diff;		
		}

		pSrc +=uv_frame_width;
		pRec += uv_frame_width;	
	}

	if (sqr_diff == 0) 
	{
		psnr = 1000000.0;
	}else
	{
		psnr = (log10 ((double) (max_val*max_val*uv_frame_width*uv_frame_height) / (double) sqr_diff) * 10.0);
	}
	return psnr;
}

void Mp4Enc_ComputeSNR(uint8 **pYuv_src_frame, uint8 **pYuv_rec_frame)
{
	/*computer Y snr*/
	g_SNR[0] = Mp4Enc_ComputeSNRY(pYuv_src_frame[0], pYuv_rec_frame[0]);
	g_SNR[1] = Mp4Enc_ComputeSNRUV(pYuv_src_frame[1], pYuv_rec_frame[1]);	
//	g_SNR[2] = Mp4Enc_ComputeSNRUV(pYuv_src_frame[2], pYuv_rec_frame[2]);

	g_PSNRY += g_SNR[0];
	g_PSNRU += g_SNR[1];
	g_PSNRV += g_SNR[2];
	
	#if defined(SIM_IN_WIN)
	PRINTF ("\t psnr_y: %f, psnr_u: %f, psnr_v: %f\n\n", 
		g_SNR[0], g_SNR[1], g_SNR[2]);

	FPRINTF (g_rgstat_fp, "\t psnr_y: %f, psnr_u: %f, psnr_v: %f\n", 
		g_SNR[0], g_SNR[1], g_SNR[2]);

	FPRINTF (g_psnr_pf, "%f\n", g_SNR[0]);
#endif
}

void Mp4Enc_PrintStatistics(void)
{
	double bitrate;
	double last_t;
	double average_psnrY;
	double average_psnrU;
	double average_psnrV;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();

	last_t  = 1.0 * g_nFrame_enc / (g_rc_par.frame_rate);
	bitrate = (double)g_rc_par.total_bits / last_t;
	
	average_psnrY = g_PSNRY / g_encoded_frame_num;
	average_psnrU = g_PSNRU / g_encoded_frame_num;
	average_psnrV = g_PSNRV / g_encoded_frame_num;

#if defined(SIM_IN_WIN)	
	PRINTF("skipped frame number: %d\n", g_enc_skip_count);
	PRINTF("encoded frame number: %d\n", g_encoded_frame_num);
	PRINTF("re-encoded frame number: %d\n", g_re_enc_frame_number);
	PRINTF("average psnrY: %f\n", average_psnrY);
	PRINTF("average psnrU: %f\n", average_psnrU);
	PRINTF("average psnrV: %f\n", average_psnrV);
	PRINTF("bitrate: %fk bps\n", bitrate/1000);
	PRINTF("total me search point count:%d\n", g_total_search_point_cnt);

// 	printf ("packet number: %d, error packet number: %d\n", g_pck_num, g_err_pck_num);

	fprintf(g_rgstat_fp, "skipped frame number: %d\n", g_enc_skip_count);
	fprintf(g_rgstat_fp, "encoded frame number: %d\n", g_encoded_frame_num);
	fprintf(g_rgstat_fp, "re-encoded frame number: %d\n", g_re_enc_frame_number);
	fprintf(g_rgstat_fp, "average psnrY: %f\n", average_psnrY);
	fprintf(g_rgstat_fp, "average psnrU: %f\n", average_psnrU);
	fprintf(g_rgstat_fp, "average psnrV: %f\n", average_psnrV);
	fprintf(g_rgstat_fp, "bitrate: %fk bps\n", bitrate/1000);
#endif
}
#endif

#if defined(SIM_IN_ADS)
/*****************************************************************************
 **	Name : 			Mp4_WriteRecFrame
 ** Description:	Write one reconstruction frame to reconstruction buffer or file. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4_WriteRecFrame(uint8 **pSrc_buffer, uint8 *pDst_buffer, uint32 frame_width, uint32 frame_height)
{
	uint32 size = frame_width * frame_height;
	uint8 *pDst_y = pDst_buffer;
	uint8 *pDst_u = pDst_y + size;
	uint8 *pDst_v = pDst_u + (size>>2);
	
	memcpy(pDst_y, pSrc_buffer[0], size);
	size = size>>2;
	memcpy(pDst_u, pSrc_buffer[1], size);
	memcpy(pDst_v, pSrc_buffer[2], size);
}
#elif defined(SIM_IN_WIN)
void Mp4_WriteRecFrame(uint8 *pRec_Y, uint8 *pRec_U, uint8 *pRec_V, FILE *pf_recon_file, uint32 frame_width, uint32 frame_height)
{
	uint32 size = frame_width * frame_height;

 	fwrite(pRec_Y,1,size, pf_recon_file);

	if (!g_uv_interleaved)
	{
		size = size>>2;
		fwrite(pRec_U,1,size, pf_recon_file);
		fwrite(pRec_V,1,size, pf_recon_file);
	}else
	{
		size = size>>1;
		fwrite(pRec_U,1,size, pf_recon_file);
	}
	
	return;
}
#endif

Bool Mp4Enc_ReadoutOneFrame(uint32 frame_num, uint32 frame_size, uint8 *pYuv_frame, int32 samp_rate)
{
#ifdef _ARM_
	uint32 offset;
	const uint8 *pInput_yuv = g_pEnc_input_yuv_buffer;
	const uint8 *pIn_yuv;	
	
	offset = frame_num * samp_rate * frame_size * 3/2;
	
	pIn_yuv = &(pInput_yuv[offset]);
	
	memcpy(pYuv_frame, pIn_yuv, frame_size *3/2);
	
	return FALSE;

#else
#ifdef SIM_IN_WIN
	uint32 offset;
	uint32 length;

	FILE *pf_Yuv = s_pEnc_input_src_file;
	
	if (enable_anti_shake)
	{
		frame_size = input_width*input_height;
	}

	offset = frame_num * frame_size*3/2;
	
	fseek(pf_Yuv, offset, SEEK_SET);

	length = fread(pYuv_frame, 1, frame_size*3/2, pf_Yuv);
	if(length < frame_size)
	{
		return TRUE;
	}
#endif
	return FALSE;
#endif
}

void Mp4Enc_main(void)
{
	uint8 *enc_yuv_one_frame_buffer;
	Bool is_stop_encode = FALSE;
	uint32 frame_rate = 15;
	uint32 enc_frame_size;
	MMEncVideoInfo video_info;
	MMEncIn *pEncInput = NULL;
	MMEncOut *pEncOutput = NULL;
	MMEncOut *pEncHeaderOutput = NULL;
	MMEncConfig *pEncConf = NULL;
	MMCodecBuffer *pEnc_Inter_malloc_Bfr = NULL;
	MMCodecBuffer *pEnc_Extra_malloc_Bfr = NULL;

	uint32 uEncBsTotalBits = 0;
	int32  channel_quality = 0; //default is (0).
//#ifdef SIM_IN_WIN
	pEncInput = (MMEncIn *)MallocInterMem(sizeof(MMEncIn));// MallocExtraMem
	pEncOutput = (MMEncOut *)MallocInterMem(sizeof(MMEncOut));// MallocExtraMem
	pEncHeaderOutput = (MMEncOut *)MallocInterMem(sizeof(MMEncOut));// MallocExtraMem
	pEncConf = (MMEncConfig *)MallocInterMem(sizeof(MMEncConfig));// MallocExtraMem
//#endif
	pEnc_Inter_malloc_Bfr =  (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer));// MallocExtraMem
	pEnc_Extra_malloc_Bfr =  (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer));// MallocExtraMem

	g_total_search_point_cnt = 0;
	g_encoded_frame_num = 0;

	/*memset(&g_stat_rc, 0, sizeof(RCMode));
	g_stat = 0;
	memset(&g_rc_par, 0, sizeof(RateCtrlPara));*/
	{
		int i;
		g_stat_rc.Ec_Q8 = 0;
		g_stat_rc.Qc = 0;
		g_stat_rc.Rc = 0;
		for (i=0;i<RC_MAX_SLIDING_WINDOW;i++) 
		{
			g_stat_rc.rgQp[i] = 0;
			g_stat_rc.rgRp[i] = 0;
			g_stat_rc.EcP_Q8[i] = 0;
		}
	}
#ifdef SIM_IN_WIN
	video_info.frame_height = g_input->pic_height;
	video_info.frame_width  = g_input->pic_width;
	video_info.is_h263 = g_input->is_short_header;//0;
	video_info.time_scale = 15 * 1000;
	video_info.uv_interleaved = g_uv_interleaved;
	or1200_print = 1;//for or1200 cmd
#else
	{
		int tmp;
		tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x04, "read img_size");
		video_info.frame_width = (tmp&0xfff);
		video_info.frame_height  = ((tmp>>12)&0xfff);
		tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x50, "ORSC_SHARE:actual buffer height & width");
		input_width = tmp & 0xfff;
		input_height = (tmp >> 12) & 0xfff;
		enable_anti_shake = (video_info.frame_height != input_height || video_info.frame_width != input_width);
		video_info.is_h263 = 0;//g_input->is_short_header;//0;
		video_info.time_scale = 15 * 1000;
		video_info.uv_interleaved = g_uv_interleaved;
		or1200_print = 1;//for or1200 cmd
	}
#endif
#ifdef SIM_IN_WIN
	g_pEnc_output_bs_buffer = (uint8 *)MallocExtraMem(1000 * 1024);//allocate 1000kbyte for encoded bitstream.
#else
	g_pEnc_output_bs_buffer = (uint8 *)bs_start_addr;
#endif

	pEncHeaderOutput->strmSize = ONEFRAME_BITSTREAM_BFR_SIZE;
	pEncHeaderOutput->pOutBuf = (uint8 *)ExtraMemAlloc_64WordAlign(pEncHeaderOutput->strmSize);

	pEncOutput->strmSize = ONEFRAME_BITSTREAM_BFR_SIZE;
	pEncOutput->pOutBuf = (uint8 *)ExtraMemAlloc_64WordAlign(pEncOutput->strmSize);

	if (enable_anti_shake)
		enc_frame_size = input_width * input_height;
	else
		enc_frame_size = g_input->pic_height * g_input->pic_width;
	enc_yuv_one_frame_buffer = (uint8 *)MallocInterMem(enc_frame_size*3/2);//MallocExtraMem
	
	pEnc_Inter_malloc_Bfr->size = total_inter_malloc_size - g_inter_malloced_size;	
	pEnc_Inter_malloc_Bfr->common_buffer_ptr = (uint8 *)MallocInterMem(pEnc_Inter_malloc_Bfr->size);
#ifdef SIM_IN_WIN
	memset(pEnc_Inter_malloc_Bfr->common_buffer_ptr, 0, pEnc_Inter_malloc_Bfr->size);
#endif
	pEnc_Extra_malloc_Bfr->size = total_extra_malloc_size - g_extra_malloced_size;	
	pEnc_Extra_malloc_Bfr->common_buffer_ptr = (uint8 *)MallocExtraMem(pEnc_Extra_malloc_Bfr->size);	
#ifdef SIM_IN_WIN
	memset(pEnc_Extra_malloc_Bfr->common_buffer_ptr, 0, pEnc_Extra_malloc_Bfr->size);
#endif

	MP4EncInit(pEnc_Inter_malloc_Bfr, pEnc_Extra_malloc_Bfr, &video_info);
	{
		uint32 cmd;
		cmd = 0;
		cmd |= (g_enc_vol_mode_ptr->short_video_header)?STREAM_ID_H263:STREAM_ID_MPEG4 << 0;	// VSP_standard[3:0], 0x1:STREAM_ID_MPEG4
		cmd |= 1 << 4;		// Work_mode[4], 1:encode, 0:decode
		cmd |= 0 << 5;		// Manual_mode[5], 1:enable manual mode
		//OR1200_READ_REG(ORSC_SHARERAM_OFF + 0, "ORSC: VSP_MODE: Read from share ram");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, cmd, "ORSC: VSP_MODE: Set standard, work mode and manual mode");
		cmd = 0;
		cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
#ifndef SIM_IN_WIN
		//cmd = OR1200_READ_REG(GLB_REG_BASE_ADDR+0x10, "ORSC: AXIM_ENDIAN_SET");
		//cmd &= 0xffffffc7;
		//cmd |= 0x00000038;
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x10, cmd, "ORSC: AXIM_ENDIAN_SET: Bs_endian=101");
		OR1200_WRITE_REG(0x180000, 123456, "debug");
#endif
		BSM_Init();
	}
//	pEncHeaderOutput->pOutBuf = pEncBsBfr = g_pEnc_output_bs_buffer;
	MP4EncGenHeader(pEncHeaderOutput);
	pEncOutput->strmSize = pEncHeaderOutput->strmSize;//count header bits with first frame
//	pEncBsBfr +=  pEncHeaderOutput->strmSize;
	uEncBsTotalBits += pEncHeaderOutput->strmSize;
#ifdef SIM_IN_WIN
	fwrite (pEncHeaderOutput->pOutBuf, 1, pEncHeaderOutput->strmSize, s_pEnc_output_bs_file);
#else
	/*g_stream_offset += pEncHeaderOutput->strmSize;
	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, g_stream_offset,"shareRAM 0x20 stream_len");
	memcpy (g_pEnc_output_bs_buffer, pEncHeaderOutput->pOutBuf, pEncHeaderOutput->strmSize);*/
	//save g_stream_offset to shareram//for arm to decide if need update bsm buffer or not
#endif
#ifdef SIM_IN_WIN
	pEncConf->h263En			= g_input->is_short_header;
	pEncConf->profileAndLevel	= 0;
	pEncConf->RateCtrlEnable	= g_input->rc_ena;
	pEncConf->targetBitRate		= g_input->bit_rate * 1024;
	pEncConf->FrameRate			= (g_input->src_frm_rate/g_input->samp_rate);
	pEncConf->QP_IVOP			= g_input->step_I;
	pEncConf->QP_PVOP			= g_input->step_P;
#else
	{
		int tmp;
		pEncConf->h263En			= 0;//g_input->is_short_header;
		pEncConf->profileAndLevel	= 0;
#if 1
		tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x38, "read rate control setting");
		pEncConf->RateCtrlEnable	= (tmp>>31)&0x1;
		pEncConf->targetBitRate		= (tmp&0x7fffffff) * 1024;
#else
		pEncConf->RateCtrlEnable	= 0;
		pEncConf->targetBitRate		= 0;
#endif
		pEncConf->FrameRate			= 15;//(g_input->src_frm_rate/g_input->samp_rate);//???
		pEncConf->QP_IVOP			= 4;//g_input->step_I;//???
		pEncConf->QP_PVOP			= 6;//g_input->step_P;//???
	}
#endif

	g_nFrame_enc = 0;

	//config the init parameter.1
	MP4EncSetConf(pEncConf);

	while(!is_stop_encode)
	{
#ifdef SIM_IN_WIN
		is_stop_encode = Mp4Enc_ReadoutOneFrame(g_nFrame_enc, enc_frame_size, enc_yuv_one_frame_buffer, g_samp_rate);
		//Dump_Src_frame_Data_420(enc_yuv_one_frame_buffer, g_input->pic_width, g_input->pic_height, video_info.uv_interleaved);

		if(is_stop_encode)
		{
			break;
		}
		FPRINTF(g_fp_trace_fw,"enc_frame_num: %d\n", g_nFrame_enc);
		if(g_nFrame_enc == 20)
		{
			g_nFrame_enc = 20;
		}
#else
		//第一次进入或重新进入或继续循环
		OR1200_READ_REG_POLL(SHARE_RAM_BASE_ADDR+0x0, 0x80000000,0x80000000, "ORSC_SHARE: shareRAM 0x0 MODE_CFG run"); //polling until can run	
		//如果reset后重新进入，需恢复现场寄存器        
		if(g_not_first_reset)	// var need update?
		{
			//恢复现场寄存器
		}
		//first_mb
		{
			int tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x0, "ORSC_SHARE: MODE_CFG");
			input_buffer_update = (tmp>>8)&1; //1
			if(input_buffer_update)
			{
				/*bs_start_addr = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"ORSC_SHARE: STREAM_BUF_ADDR");
				bs_buffer_length = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"ORSC_SHARE: STREAM_BUF_SIZE");	//byte
				g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "ORSC_SHARE: stream_len");*/
				if (g_nFrame_enc != 0)//bitstream header send with first frame
				{
					g_enc_vop_mode_ptr->stm_offset = 0;
					g_enc_vop_mode_ptr->pOneFrameBitstream = (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"ORSC_SHARE: STREAM_BUF_ADDR");
				}
				g_enc_vop_mode_ptr->pYUVSrcFrame->imgY = (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x2c,"ORSC_SHARE: Frame_Y_ADDR");
				g_enc_vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)g_enc_vop_mode_ptr->pYUVSrcFrame->imgY / 8;
				g_enc_vop_mode_ptr->pYUVSrcFrame->imgU = (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x30,"ORSC_SHARE: Frame_UV_ADDR");
				g_enc_vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)g_enc_vop_mode_ptr->pYUVSrcFrame->imgU / 8;
				frame_buf_size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x34, "ORSC_SHARE: UV_FRAME_BUF_SIZE");	// Y_frame_buffer_size = 2*UV_frame_buffer_size, R for encoder mode
				// input_buffer_update = 0;
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x0, tmp&0xfffffeff,"ORSC_SHARE: MODE_CFG clear input_buffer_update");
			}
		}
#endif
		pEncInput->pInBuf			= enc_yuv_one_frame_buffer;
		pEncInput->inBufSize		= enc_frame_size* 3/2;
		pEncInput->time_stamp		= 15 * 1000 / pEncConf->FrameRate * g_nFrame_enc;
		pEncInput->channel_quality	= channel_quality; 
		pEncInput->vopType			= (channel_quality == 2)?IVOP :PVOP;

		or1200_print = 1;
		
		MP4EncStrmEncode(pEncInput, pEncOutput);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
		//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + VSP_INT_SYS_OFF, V_BIT_0, 0x00000001, "ORSC: Polling VSP_INT_STS: BSM_FRM_DONE"); // check HW INT
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_CLR_OFF, V_BIT_0, "ORSC: VSP_INT_CLR: BSM_FRM_DONE"); // clear HW INT

// 		g_total_search_point_cnt += g_search_point_cnt;
		g_nFrame_enc++;
		if( pEncOutput->strmSize)
		{
			g_encoded_frame_num++;
		}else
		{
			g_enc_skip_count++;
		}

/*#ifdef _ARM_
		if(pEncOutput->strmSize)
		{
		//	Mp4_WriteRecFrame(g_enc_vop_mode_ptr->pYUVRecFrame->imgY,g_pEnc_recon_yuv_buffer,video_info.frame_width,video_info.frame_height);
			memcpy (g_pEnc_output_bs_buffer, pEncOutput->pOutBuf, pEncOutput->strmSize);
			//save g_stream_offset to shareram//for arm to decide if need update bsm buffer or not
			g_stream_offset += pEncOutput->strmSize;
			OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, g_stream_offset,"shareRAM 0x20 stream_len");
		}
#else*/
		if(pEncOutput->strmSize)
		{
//			Mp4_WriteRecFrame(g_enc_vop_mode_ptr->pYUVRecFrame->imgY, g_enc_vop_mode_ptr->pYUVRecFrame->imgU,
//				g_enc_vop_mode_ptr->pYUVRecFrame->imgV, s_pEnc_output_recon_file,video_info.frame_width,video_info.frame_height);
#ifdef SIM_IN_WIN
			fwrite (pEncOutput->pOutBuf, 1, pEncOutput->strmSize, s_pEnc_output_bs_file);

			PRINTF("\t%d\n", pEncOutput->strmSize/*+12*/);
			
#else
			/*memcpy (g_pEnc_output_bs_buffer, pEncOutput->pOutBuf, pEncOutput->strmSize);
			//save g_stream_offset to shareram//for arm to decide if need update bsm buffer or not
			g_stream_offset += pEncOutput->strmSize;
			OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, g_stream_offset,"shareRAM 0x20 stream_len");*/
		
#endif
		}
	//	pEncBsBfr += pEncOutput->strmSize;
		uEncBsTotalBits += pEncOutput->strmSize;
		pEncOutput->strmSize = 0;
//#endif	
#ifdef SIM_IN_WIN
		if(g_nFrame_enc >= g_input->frame_num_enc)
		{
			is_stop_encode = TRUE;
		}
#else
		//if(g_image_ptr->error_flag||dec_output_ptr->frameEffective||file_end/*(g_stream_offset>=bs_buffer_length)*/||(video_size_get&&!video_buffer_malloced))//bsm buffer数据耗尽)||(第一次获得图像尺寸 video_size_get)
		{ 
			int tmp;
			//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, g_stream_offset,"shareRAM 0x20 stream_len");//g_stream_offset保存到shareram，//供arm判断是否更新bsm buffer
			
			cpu_will_be_reset=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>30)&1;
			if(cpu_will_be_reset)//如果重新进入会reset，则需要保存现场寄存器
			{
				//保存现场寄存器
			}
			tmp=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
			OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, tmp&0x7fffffff,"shareRAM 0x0 stop");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
			//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));
			//返回arm，等待bsm buffer跟新或申请空间
			
		}
		
		//返回arm或继续
#endif

	}
#ifdef SIM_IN_WIN
#ifdef OUTPUT_TEST_VECTOR
	fprintf(g_fp_global_tv, "f_01234567_89abcdef\n");
	{
		FILE *bsm_length = fopen("..\\..\\test_vectors\\bsm_enc_length.txt", "w");
		fprintf(bsm_length, "%x", ((g_enc_vop_mode_ptr->stm_offset + 7) >> 3));
		fclose(bsm_length);
	}
#endif
 	Mp4Enc_PrintStatistics();
#endif
#ifdef _ARM_
	Mp4Enc_MemFree();
#endif
//	system("PAUSE");
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
