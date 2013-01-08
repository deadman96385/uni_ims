/******************************************************************************
 ** File Name:    mp4enc_interface.c										  *
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
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

FILE* output_yuv = NULL;
void dump_input_yuv()
{
	
        int length = 0;
	int i = 0;
        int width = g_enc_vop_mode_ptr->FrameWidth;
	int height = g_enc_vop_mode_ptr->FrameHeight;
	unsigned char * src_y = g_enc_vop_mode_ptr->pYUVSrcFrame->imgY;
	unsigned char * src_u= src_y+width*height;//g_enc_vop_mode_ptr->pYUVSrcFrame->imgU;
	unsigned char * src_v = src_y+width*height*5/4;//g_enc_vop_mode_ptr->pYUVSrcFrame->imgV;
	
	if(g_nFrame_enc == 1)
	{
		output_yuv = fopen("/data/input.yuv","wb");  
	}

	SCI_TRACE_LOW("dump_input_yuv width %d,height %d,src_y  %x %x %x",width,height,src_y,src_u,src_v);
	if(output_yuv != NULL)
	{
        	fwrite(src_y,1,width*height,output_yuv);
		fwrite(src_u,1,width*height/4,output_yuv);
		fwrite(src_v,1,width*height/4,output_yuv);
	}
		
	if(g_nFrame_enc == 2)
	{
		fclose(output_yuv );
	}
}


void read_one_frame_yuv(uint8* ptr,void* file,uint32 size)
{	
	fread(ptr,1,size,(FILE *)file);
	if(g_nFrame_enc == 100)
	{
		fseek((FILE *)file,0,SEEK_SET);
	}

}
/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MMEncOut *pOutput)
{
	uint32 NumBits = 0;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);
	
	if(!pVop_mode->short_video_header)   //MPEG-4 case
	{	
		if (Mp4Enc_VSPInit(pVop_mode) != MMENC_OK)
		{
			return MMENC_ERROR;
		}
        Mp4Enc_InitBitStream(pVop_mode);
		NumBits = Mp4Enc_EncSequenceHeader(pVol_mode, pVop_mode);		
		NumBits += Mp4Enc_EncVOHeader(pVop_mode);	
		NumBits += Mp4Enc_EncVOLHeader(pVol_mode, pVop_mode);

	//	NumBits += Mp4Enc_OutputLeftBits();	
		
		//clear bsm-fifo, polling inactive status reg
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
		VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");

		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;	
        VSP_RELEASE_Dev();
	}else
	{
		pOutput->strmSize = 0;
	}

	pOutput->pOutBuf = pVop_mode->pOneFrameBitstream;
	//SCI_TRACE_LOW("MP4EncGenHeader strm_size %d ",pOutput->strmSize);
#if _DEBUG_
	if(0 != pOutput->strmSize)
	{
		fwrite(pOutput->pOutBuf ,1,pOutput->strmSize,(FILE *)pVop_mode->bits);
		SCI_TRACE_LOW("fwrite MP4EncGenHeader strm_size %d ",pOutput->strmSize);
	}
#endif
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncSetConf(MMEncConfig *pConf)
{
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);

	pVol_mode->short_video_header	= pConf->h263En;
	pVol_mode->ProfileAndLevel		= pConf->profileAndLevel;
	
	pVop_mode->FrameRate			= pConf->FrameRate;	
	pVop_mode->targetBitRate		= pConf->targetBitRate;
	pVop_mode->RateCtrlEnable		= pConf->RateCtrlEnable ;
	
	pVop_mode->StepI				= pConf->QP_IVOP;
	pVop_mode->StepP				= pConf->QP_PVOP;
	pVop_mode->vbv_buf_size			= pConf->vbv_buf_size;
SCI_TRACE_LOW("pVop_mode->short_video_header %d",pVop_mode->short_video_header);
SCI_TRACE_LOW("pVop_mode->targetBitRate %d",pVop_mode->targetBitRate);
SCI_TRACE_LOW("pVop_mode->RateCtrlEnable %d",pVop_mode->RateCtrlEnable);
SCI_TRACE_LOW("pVop_mode->StepI %d",pVop_mode->StepI);
SCI_TRACE_LOW("pVop_mode->StepP %d",pVop_mode->StepP);
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGetConf(MMEncConfig *pConf)
{
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);

	pConf->QP_IVOP = pVop_mode->StepI;
	pConf->QP_PVOP = pVop_mode->StepP;
	
	pConf->h263En = pVol_mode->short_video_header;
	pConf->profileAndLevel = pVol_mode->ProfileAndLevel;

	pConf->targetBitRate = pVop_mode->targetBitRate;
	pConf->FrameRate = pVop_mode->FrameRate;	
	pConf->RateCtrlEnable = pVop_mode->RateCtrlEnable; 
	
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncRelease(void)
{
// 	Mp4_CommonMemFree();
	Mp4Enc_MemFree();
	VSP_CLOSE_Dev(0);

// 	g_bVspInited = FALSE;
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	return MMENC_OK;
}

void MPEG4Enc_close(void)
{
	return;
}

/*****************************************************************************/
//  Description:   Init mpeg4 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr, MMEncVideoInfo *pVideoFormat)
{
	uint16 frame_width = pVideoFormat->frame_width;
	uint16 frame_height = pVideoFormat->frame_height;
	VOL_MODE_T  *vol_mode_ptr = NULL;
	ENC_VOP_MODE_T  *vop_mode_ptr = NULL;
	MMEncRet init_return = MMENC_OK;
	
	s_vsp_fd = -1;
	s_vsp_Vaddr_base = 0;


	if(pExtaMemBfr->size < 200 * 1024)
	{
		return 1;
	}
	
#if _CMODEL_
	VSP_Init_CModel();
#endif //_CMODEL_

	Mp4Enc_InitMem(pInterMemBfr, pExtaMemBfr);

	vol_mode_ptr = (VOL_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(VOL_MODE_T));	
	Mp4Enc_SetVolmode(vol_mode_ptr);

	vop_mode_ptr = (ENC_VOP_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_VOP_MODE_T)); 
#ifdef _DEBUG_
	vop_mode_ptr->bits = (FILE *)fopen("/data/outbits","wb");
	vop_mode_ptr->yuv_in = (FILE *)fopen("/data/test1.yuv","rb");
	SCI_TRACE_LOW("MP4EncInit open file to store the outputbits");
#endif
	Mp4Enc_SetVopmode(vop_mode_ptr);

	vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header  = pVideoFormat->is_h263;
	vop_mode_ptr->uv_interleaved = pVideoFormat->uv_interleaved;
	//vop_mode_ptr->uv_interleaved = 0;//debug
	vol_mode_ptr->VolWidth = frame_width;
	vol_mode_ptr->VolHeight = frame_height;

	g_enc_last_modula_time_base = 0;
	g_enc_tr = 0;
	g_HW_CMD_START=0;
	g_enc_is_prev_frame_encoded_success = FALSE;
	g_enc_p_frame_count = 0;	

	Mp4Enc_InitVolVopPara(vol_mode_ptr, vop_mode_ptr, pVideoFormat->time_scale);		

	Mp4Enc_InitSession(vol_mode_ptr, vop_mode_ptr); 
SCI_TRACE_LOW("b4 open VSP_OPEN_Dev() ");
	if (VSP_OPEN_Dev(1) < 0)
	{
		init_return = MMENC_ERROR;
		SCI_TRACE_LOW("VSP_OPEN_Dev() ");
		return init_return;
	}
SCI_TRACE_LOW(" open VSP_OPEN_Dev() successful");	
#if 0	
	if(!pVideoFormat->is_h263)
	{
		uint32 cmd;
		
	//	VSP_Reset ();

		/*clear time_out int_raw flag, if timeout occurs*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");

		/*init dcam command*/  //Control bit of access to share registers and VLD BUFFER 0:hardware 1:AHB
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) , "DCAM_CFG: configure DCAM register");
		
		/*init vsp command*/	
		cmd = (1<<17) |(0<<15) |(1<<14)|(1<<13)|(1<<12) | ((!vop_mode_ptr->short_video_header)<<8) | (1<<7) | (0<<6) | 
			(1<<5) | (1<<4) |(0<<3) | (1<<2) | (1<<1) | (1<<0);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: init the global register, little endian");

		//cmd = (0 << 16) |((uint32)0xffff);
		cmd = (1<< 31)|(1 << 30)|(TIME_OUT_CLK);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

		cmd =vop_mode_ptr->uv_interleaved ?0:1;//00: uv_interleaved, two plane, 1: three plane
             	cmd = (cmd<<27);
		cmd |= 0x1ff;
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
		//if(2 ==vop_mode_ptr->uv_interleaved )//vu_interleaved
		{
			cmd |= ((1<<21)|(1<<18));
			VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
		}
		//now, for uv_interleaved
	#if _CMODEL_ //for RTL simulation
		cmd = (720*576)>>2; //word unit, updated by xwluo@20111205 for 720x576 supported, from 352*288
	#else
		cmd = (vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight)>>2; //word unit
	#endif
		cmd = (vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight)>>2; //word unit //?????leon
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_UV_OFFSET_OFF, cmd, "configure uv offset");


		

		Mp4Enc_InitBitStream(vop_mode_ptr);
	}
#endif
	init_return = MMENC_OK;
	
	return init_return;
}

/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncStrmEncode(MMEncIn *pInput, MMEncOut *pOutput)
{
	uint32 cmd;
	int32 ret;
	VOP_PRED_TYPE_E frame_type;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();
	BOOLEAN *pIs_prev_frame_success = &g_enc_is_prev_frame_encoded_success;
	uint32 frame_width = pVop_mode->FrameWidth;
	uint32 frame_height = pVop_mode->FrameHeight;
	uint32 frame_size = frame_width * frame_height;
	BOOLEAN frame_skip = FALSE;

//SCI_TRACE_LOW(" input y_addr %x,u %x ,v %x",pInput->p_src_y,pInput->p_src_u,pInput->p_src_v);
#if 0 //only for verify by read yuv from file instead of from camera
	read_one_frame_yuv(pInput->p_src_y,pVop_mode->yuv_in,frame_size*3/2);
#endif

#if 0
//	VSP_Reset();
	int vsp_stat = VSP_ACQUIRE_Dev();
	if(vsp_stat)
	{
		VSP_RELEASE_Dev();
		SCI_TRACE_LOW("MP4EncStrmEncode VSP_ACQUIRE_Dev ERR");
		return MMENC_ERROR;
	}
	
	/*clear time_out int_raw flag, if timeout occurs*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
	
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<16), "DCAM_INT_MASK: enable MP4 ENC DONE INT bit");

	/*init vsp command*/
	cmd = (1<<17) |(0<<15) |(1<<14)|(1<<13)|(1<<12) | ((!pVop_mode->short_video_header)<<8) | (1<<7) | (0<<6) | 
			(1<<5) | (1<<4) |(0<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: little endian");

	//cmd = (0 << 16) |((uint32)0xffff);
	cmd = (1<< 31)|(1 << 30)|(TIME_OUT_CLK);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");
    
    cmd =pVop_mode->uv_interleaved ?0:1;//00: uv_interleaved, two plane, 1: three plane
    cmd = (cmd<<27);
	cmd |= 0x1ff;
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
		//if(2 ==vop_mode_ptr->uv_interleaved )//vu_interleaved
	{
		cmd |= ((1<<21)|(1<<18));
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
	}
		
	//now, for uv_interleaved
#if _CMODEL_ //for RTL simulation
	cmd = (720*576)>>2; //word unit, updated by xwluo@20111205 for 720x576 supported, from 352*288
#else
	cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>2; //word unit
#endif
	cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>2; //word unit //?????leon
	//cmd = (pVop_mode->pYUVSrcFrame->imgUAddr-pVop_mode->pYUVSrcFrame->imgYAddr)>>2;
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_UV_OFFSET_OFF, cmd, "configure uv offset");
	cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>4;
	//cmd = (pVop_mode->pYUVSrcFrame->imgVAddr-pVop_mode->pYUVSrcFrame->imgUAddr)>>2;
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AHBM_V_ADDR_OFFSET,cmd, "configure uv offset");
#endif
//	pVop_mode->mbline_num_slice	= 1;
	pVop_mode->intra_mb_dis		= 30;	
       pVop_mode->sliceNumber = 0;
	
	if(!pVop_mode->bInitRCSuceess)
	{
		Mp4Enc_InitRateCtrl(&g_rc_par, &g_stat_rc);
		pVop_mode->bInitRCSuceess = TRUE;
		g_nFrame_enc = 0;	
		g_enc_last_modula_time_base = pInput->time_stamp / (int32)pVol_mode->ClockRate; //the first frame
	}
	g_nFrame_enc++;
	if(g_nFrame_enc == 11)
	{
		g_nFrame_enc = g_nFrame_enc;
	}

	if(pInput->vopType == IVOP)
	{
		g_rc_par.p_count = 0;
	}

	g_stat_rc.B				= pInput->bs_remain_len;
	frame_type 				= Mp4Enc_JudgeFrameType (&g_rc_par, &g_stat_rc);
	pVop_mode->VopPredType	= (pInput->vopType == IVOP)?IVOP:frame_type;
	pInput->vopType = pVop_mode->VopPredType;
	//frame_skip				= (pVop_mode->VopPredType == NVOP) ? 1 : 0;
	
	SCI_TRACE_LOW("g_nFrame_enc %d frame_type %d ", g_nFrame_enc,pVop_mode->VopPredType);
	if(!frame_skip)
	{	
		if (Mp4Enc_VSPInit(pVop_mode) != MMENC_OK)
		{
			return MMENC_ERROR;
		}
		
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<16), "DCAM_INT_MASK: enable MP4 ENC DONE INT bit");
		//now, for uv_interleaved
#if _CMODEL_ //for RTL simulation
		cmd = (720*576)>>2; //word unit, updated by xwluo@20111205 for 720x576 supported, from 352*288
#else
		cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>2; //word unit
#endif
		cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>2; //word unit //?????leon
		//cmd = (pVop_mode->pYUVSrcFrame->imgUAddr-pVop_mode->pYUVSrcFrame->imgYAddr)>>2;
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_UV_OFFSET_OFF, cmd, "configure uv offset");
		cmd = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight)>>4;
		//cmd = (pVop_mode->pYUVSrcFrame->imgVAddr-pVop_mode->pYUVSrcFrame->imgUAddr)>>2;
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AHBM_V_ADDR_OFFSET,cmd, "configure uv offset");
	
		g_stat_rc.be_re_enc		= FALSE;
		g_stat_rc.be_skip_frame = FALSE;

#ifdef RE_ENC_SHEME
FRAME_ENC:
#endif
		Mp4Enc_InitRCFrame (&g_rc_par);
		
		Mp4Enc_InitBitStream(pVop_mode);

#if defined(_SIMULATION_) 
		memcpy(g_enc_vop_mode_ptr->pYUVSrcFrame->imgY, pInput->pInBuf, frame_size);
		if (!pVop_mode->uv_interleaved)
		{
			memcpy(g_enc_vop_mode_ptr->pYUVSrcFrame->imgU, pInput->pInBuf+frame_size, frame_size/4);
			memcpy(g_enc_vop_mode_ptr->pYUVSrcFrame->imgV, pInput->pInBuf+frame_size*5/4, frame_size/4);
		}else
		{
			memcpy(g_enc_vop_mode_ptr->pYUVSrcFrame->imgU, pInput->pInBuf+frame_size, frame_size/2);
		}
#else
		        //--y,u,v address should be 256 bytes aligned. YUV420.????
        pVop_mode->pYUVSrcFrame->imgY = pInput->p_src_y;
        pVop_mode->pYUVSrcFrame->imgU = pInput->p_src_u;
        pVop_mode->pYUVSrcFrame->imgV = pInput->p_src_v;

#ifdef _VSP_LINUX_
        pVop_mode->pYUVSrcFrame->imgYAddr = (uint32)pInput->p_src_y_phy;//>> 8;
        pVop_mode->pYUVSrcFrame->imgUAddr = (uint32)pInput->p_src_u_phy;//>> 8;
        pVop_mode->pYUVSrcFrame->imgVAddr = (uint32)pInput->p_src_v_phy;//>> 8;
  //      SCI_TRACE_LOW(" input y_addr %x,u %x ,v %x",pInput->p_src_y_phy,pInput->p_src_u_phy,pInput->p_src_v_phy);
#else
        pVop_mode->pYUVSrcFrame->imgYAddr = (uint32)pVop_mode->pYUVSrcFrame->imgY ;//>> 8;
        pVop_mode->pYUVSrcFrame->imgUAddr = (uint32)pVop_mode->pYUVSrcFrame->imgU;// >> 8;
        pVop_mode->pYUVSrcFrame->imgVAddr = (uint32)pVop_mode->pYUVSrcFrame->imgV ;//>> 8;
#endif
#endif

#if _DEBUG_
      		dump_input_yuv();
#endif

#if 0
	 //optimize for android 4.0 8810 encoder can not meet the requirement of D1 30Hz 
	 if(pVop_mode->big_size_flag){
	 	if((IVOP != pVop_mode->VopPredType)&&(0==g_nFrame_enc%3)){
			pVop_mode->VopPredType = NVOP;
	 	}
	 }
	 //optimize for android 4.0 8810 encoder end
#endif
		if(IVOP == pVop_mode->VopPredType)
		{
		#if defined(SIM_IN_WIN)
			FPRINTF(g_rgstat_fp, "\nNo.%d Frame:\t I VOP\n", g_nFrame_enc);
		//	FPRINTF(g_fp_trace_fw,"\t I VOP\n");
		#endif
			PRINTF ("No.%d Frame:\t IVOP\n", g_nFrame_enc);
			Mp4Enc_InitRCGOP (&g_rc_par);
			ret = Mp4Enc_EncIVOP(pVop_mode, pInput->time_stamp);				
		    g_enc_p_frame_count = pVol_mode->PbetweenI;
		}
		else if (PVOP == pVop_mode->VopPredType)
		{
		#if defined(SIM_IN_WIN)
			FPRINTF(g_rgstat_fp, "\nNo.%d Frame:\t P VOP\n", g_nFrame_enc);
//			FPRINTF(g_fp_trace_fw,"\t P VOP\n");
		#endif
			PRINTF ("No.%d Frame:\t PVOP\n", g_nFrame_enc);

			g_enc_p_frame_count--;

			ret = Mp4Enc_EncPVOP(pVop_mode, pInput->time_stamp);
		}
		else
		{
			g_enc_p_frame_count--;

			ret = Mp4Enc_EncNVOP(pVop_mode, pInput->time_stamp);
		}

		(*pIs_prev_frame_success) = ret;
	
		//clear bsm-fifo, polling inactive status reg
// 		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
// 		VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");

		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->pOutBuf = pVop_mode->pOneFrameBitstream;
		
#ifdef _DEBUG_
	if(pVop_mode->bits != NULL)
	{
		SCI_TRACE_LOW("strmSize %d pOutBuf %x g_nFrame_enc %d",pOutput->strmSize,pOutput->pOutBuf,g_nFrame_enc);
		fwrite(pOutput->pOutBuf,1,pOutput->strmSize,(FILE *)(pVop_mode->bits));
		if(g_nFrame_enc == 300)
		{
			fclose((FILE *)(pVop_mode->bits));
			pVop_mode->bits = NULL;
		}
	}
#endif
		g_rc_par.nbits_total = pOutput->strmSize<<3;


#ifndef _CMODEL_
	VSP_WRITE_REG (VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, ((1<<26) |(1<<25)), "'STOP AXIM");
	VSP_READ_REG_POLL(VSP_AXIM_REG_BASE+AXIM_STS_OFF, V_BIT_0, 0, TIME_OUT_CLK, "AXIM_STS: polling AXI idle status");	
	
	VSP_READ_REG(VSP_RESET_ADDR,"read VSP_RESET_ADDR");
   	 VSP_WRITE_REG(VSP_RESET_ADDR,V_BIT_15,"reset VSP_RESET_ADDR");
	VSP_WRITE_REG(VSP_RESET_ADDR,0,"reset VSP_RESET_ADDR");
#endif
		if (pVop_mode->RateCtrlEnable)
		{
			int32 Ec_Q8 = (g_rc_par.sad/pVop_mode->MBNum);

			if (Ec_Q8 < 256)  //to avoid devided by zero
				Ec_Q8 = 256;
#ifdef RE_ENC_SHEME
			Mp4Enc_AnalyzeEncResult(&g_stat_rc, g_rc_par.nbits_total, pVop_mode->VopPredType, Ec_Q8);

			if (g_stat_rc.be_re_enc)
			{
				if (g_stat_rc.be_scene_cut)
				{
					pVop_mode->VopPredType = IVOP;
					g_rc_par.p_count = g_rc_par.p_between_i;
				}

				PRINTF("\t bisTotalCur: %d, frame re-encoded\n", g_rc_par.nbits_total);

				g_re_enc_frame_number++;

				goto FRAME_ENC;
			}

			if (g_stat_rc.be_skip_frame)
			{
				pOutput->strmSize = 0;
				PRINTF("\t bitsTotalCur: %d, frame skipped\n", g_rc_par.nbits_total);
					VSP_RELEASE_Dev();
				return MMENC_OK;
			}
#endif

			if (IVOP == pVop_mode->VopPredType)
			{
				Mp4Enc_ResetRCModel (pVop_mode, &g_stat_rc, &g_rc_par);
			}
			else
			{						
				Mp4Enc_UpdateRCModel (pVop_mode, &g_stat_rc, &g_rc_par, Ec_Q8);
			}
		}

#if 0 //_DEBUG_
#if defined(SIM_IN_WIN) && defined(MPEG4_ENC)
		Mp4_WriteRecFrame(
						pVop_mode->pYUVRecFrame->imgY, 
						pVop_mode->pYUVRecFrame->imgU,
						pVop_mode->pYUVRecFrame->imgV, 
						s_pEnc_output_recon_file,
					0pVop_mode->FrameWidth,
						pVop_mode->FrameHeight
						);

		//write source frame out for computing psnr in decoder
//		Mp4_WriteRecFrame (
//						pVop_mode->pYUVSrcFrame->imgY, 
//						pVop_mode->pYUVSrcFrame->imgU,
//						pVop_mode->pYUVSrcFrame->imgV, 
//						s_fp_src_enc,
//						pVop_mode->FrameWidth,
//						pVop_mode->FrameHeight
//						);

// 		Mp4Enc_ComputeSNR(pVop_mode->pYUVSrcFrame, pVop_mode->pYUVRecFrame);
#endif
#endif
		
		//update ref frame
		Mp4Enc_UpdateRefFrame(pVop_mode);
	}else
	{
		pOutput->strmSize = 0;
		PRINTF ("No. %d Frame:\t skipped\n\n", g_nFrame_enc);

	#if defined(SIM_IN_WIN)
		FPRINTF (g_rgstat_fp, "No. %d Frame:\t skipped\n\n", g_nFrame_enc);
	#endif
	}

	VSP_RELEASE_Dev();		
	
	return MMENC_OK;
}

/*****************************************************************************/
//  Description: check whether VSP can used for video encoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN MPEG4ENC_VSP_Available(void)
{
	int dcam_cfg;

	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0)
		return TRUE;
	else
		return FALSE;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
