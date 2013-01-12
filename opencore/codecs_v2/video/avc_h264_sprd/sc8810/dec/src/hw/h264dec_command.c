/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
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

LOCAL MMDecRet H264Dec_VSPInit (void)
{
	int cmd;

	int vsp_stat = VSP_ACQUIRE_Dev();
	if(vsp_stat)
	{
		VSP_RELEASE_Dev();
		return MMENC_ERROR;
	}

	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");	

   	 /*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");
        	
    	/*init dcam command*/
   	 VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

   	 cmd = (1 << 16) |((uint32)0xffff);
   	 VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: enable hardware timer out");
	
	return MMDEC_OK;
}

PUBLIC MMDecRet H264Dec_InitBitstream(/*void *pOneFrameBitstream, */int32 length)
{
	uint32 flushBytes;
//	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;

	VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 28, 7, 0, "BSM_DEBUG: polling bsm is in idle status");
	VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | ((1<<7) | BSM_DEBUG_WOFF));

	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+ VSP_BSM_RST_OFF, 1, "reset bsm module");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (1<<24) | VSP_BSM_RST_WOFF);

	if(!g_firstBsm_init_h264)
	{
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (1<<2) | (1<<1), "BSM_CFG2: clear bsm and clear counter");
		VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);
	}
	g_firstBsm_init_h264 = FALSE;

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (0<<31)|(g_stream_offset>>2), "BSM_CFG1: configure the bitstream address, disable DE-STUFFING");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (0<<31)|(((uint32)g_stream_offset>>2) & 0x3f), "BSM_CFG1: configure the bitstream address, disable DE-STUFFING");
	flushBytes = (uint32)g_stream_offset & 0x3;
	#if 0 //xweiluo@20110520, removed it due to sw vld.
	g_nalu_ptr->len += flushBytes; //MUST!! for more_rbsp_data()
	#endif
#endif

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | 0x3ffff, "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#else
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | (((length+flushBytes+800)>>2)&0x3ffff), "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#endif
	
	/*polling bsm status*/
//	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");
	VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 31, 1, 1, "BSM_DEBUG: polling bsm status");
	VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (3<<24) | (((1<<7)|BSM_DEBUG_WOFF)<<16) | (BSM_CFG0_WOFF<<8) | BSM_CFG1_WOFF);

#if _CMODEL_
	g_bs_pingpang_bfr0 = (uint8 *)pOneFrameBitstream ;
	g_bs_pingpang_bfr_len = length+1;

	init_bsm();
#endif //_CMODEL_

	flush_unalign_bytes(flushBytes);

	return MMDEC_OK;
}

PUBLIC MMDecRet H264Dec_init_slice_VSP(DEC_IMAGE_PARAMS_T *img_ptr, DEC_SLICE_T *curr_slice_ptr)
{
#if 0
	if ((img_ptr->curr_mb_nr == 0) || (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs))
#else
	if (img_ptr->is_new_pic)
#endif
	{
		uint32 cmd;
				
		g_stream_offset = 0;
		 g_cmd_data_ptr = g_cmd_data_base = img_ptr->cmd_data_buf[img_ptr->cmd_buf_idx];
		g_cmd_info_ptr = g_cmd_info_base =img_ptr->cmd_info_buf[img_ptr->cmd_buf_idx];

		img_ptr->frame_bistrm_ptr = img_ptr->frame_bistrm_buf[img_ptr->cmd_buf_idx];
		img_ptr->cmd_buf_idx = 1 - img_ptr->cmd_buf_idx;

		/*init vsp command*/
		cmd = (1<<19)|((img_ptr->is_cabac)<<18) |(1<<17)|(0<<16) |(1<<15) | (0<<14) |(1<<12) | (VSP_H264<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");
		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TIME_OUT_VAL, TIME_OUT_CLK, "GLB_TIMEOUT_VALUE: timeout value");
				
		/*frame size, YUV format, max_X, max_Y*/
		cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");			
		VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (3<<24) | (GLB_CFG1_WOFF<<16) | (VSP_TIME_OUT_VAL_WOFF<<8) | (GLB_CFG0_WOFF));
				
		//config dbk fmo mode or not				
		VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CFG_OFF, ((img_ptr->fmo_used)<<3)|6, "configure dbk free run mode and enable filter, fmo mode or not");
		VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, 1, "configure dbk ping-pang buffer0 enable");
		VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1, "configure dbk control flag");
		VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT)| (3<<24) |(DBK_CTR1_WOFF<<16) | (DBK_VDB_BUF_ST_WOFF<<8) | (DBK_CFG_WOFF));
	}	
			
	if ((curr_slice_ptr->picture_type == P_SLICE) || (curr_slice_ptr->picture_type == B_SLICE))
	{	
		int32 i;	
		uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;
		for (i = 0; i < g_list_size[0]/*16*/; i++)
		{
			if (!g_list[0][i] || !(g_list[0][i]->imgYAddr))
			{
				img_ptr->error_flag |= ER_BSM_ID;
				return MMDEC_ERROR;
			}
			VSP_WRITE_REG_CQM(pTableAddr+ (4+i)*4, g_list[0][i]->imgYAddr, "configure reference picture");			
			VSP_WRITE_CMD_INFO((VSP_RAM10 << CQM_SHIFT_BIT) | (1<<24) | (4+i));
		}
	}
		
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#else //actual envioronment
	VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 8, ((uint32)(H264Dec_ExtraMem_V2P((img_ptr->frame_bistrm_ptr+g_stream_offset), HW_CACHABLE))>>8), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#endif
	VSP_WRITE_CMD_INFO((VSP_RAM10 << CQM_SHIFT_BIT) | (1<<24) | 2);

	return MMDEC_OK;
}

PUBLIC void H264Dec_ComputeCBPIqtMbc (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 	cbp		= mb_info_ptr->cbp;
	uint32	cbp_uv = mb_cache_ptr->cbp_uv;
	uint32	cbp_iqt , cbp_mbc;
	uint32	cbp_luma_iqt, cbp_luma_mbc;

	//calculate cbp26
	cbp_luma_iqt = mb_cache_ptr->cbp_luma_iqt;
	cbp_luma_mbc =  (cbp_luma_iqt & 0xc3c3) | 
					(((cbp_luma_iqt >>  2) & 0x303) <<  4) | 
					(((cbp_luma_iqt >>  4) & 0x303) <<  2);

	cbp_mbc = (cbp_uv << 16) | cbp_luma_mbc;
	cbp_iqt    = (cbp_uv << 16) | cbp_luma_iqt;

	if (cbp > 15)	cbp_iqt |= (1<<24);	
	if (cbp > 31)	cbp_iqt |= (1<<25);
		
	if (mb_info_ptr->mb_type == I16MB)
	{
		cbp_mbc |= 0xffff;
	}

	mb_cache_ptr->cbp_iqt = cbp_iqt;
	mb_cache_ptr->cbp_mbc = cbp_mbc;
}

LOCAL void H264Dec_get_HW_status(DEC_IMAGE_PARAMS_T *img_ptr)
{
	img_ptr->ahb_state = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, "AHB: AHB status");
   	img_ptr->vsp_state = VSP_READ_REG(VSP_GLB_REG_BASE+VSP_DBG_OFF, "VSP GLOABL: VSP GLOBAL status");
    img_ptr->dbk_state = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, "DBK: DBK status");
  	img_ptr->mbc_state = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC: mbc status");
    img_ptr->vld_state = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, "VLD: vld status");
}

PUBLIC void H264Dec_SeqLevelConfig (DEC_IMAGE_PARAMS_T *img_ptr)
{
	return;
}

PUBLIC MMDecRet h264Dec_PicLevelSendRefAddressCommmand (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
	{
		img_ptr->error_flag |= ER_PICTURE_NULL_ID;
       		img_ptr->return_pos |= (1<<8);
		return MMDEC_ERROR;
	}
#endif

	if(VSP_READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
	#if _H264_PROTECT_ & _LEVEL_LOW_
		img_ptr->error_flag |= ER_AHB_ID;
        	img_ptr->return_pos |= (1<<9);
		H264Dec_get_HW_status(img_ptr);
	#endif	
		return MMDEC_HW_ERROR;
	}

	open_vsp_iram();

	VSP_WRITE_REG(pTableAddr + 0, g_dec_picture_ptr->imgYAddr, "configure reconstruct frame Y");

#if _CMODEL_ //for RTL simulation
	VSP_WRITE_REG(pTableAddr + 4, ((uint32)INTRA_PREDICTION_ADDR)>>8, "Top MB bottom data block first line pixel data for IPRED prediction");
	VSP_WRITE_REG(pTableAddr + 4*3, ((uint32)VLD_CABAC_TBL_ADDR)>>8, "vld cabac buffer address");
#else
	VSP_WRITE_REG(pTableAddr + 4, ((uint32)H264Dec_ExtraMem_V2P(img_ptr->ipred_top_line_buffer, HW_NO_CACHABLE))>>8, "Top MB bottom data block first line pixel data for IPRED prediction");
	VSP_WRITE_REG(pTableAddr + 4*3, ((uint32)H264Dec_ExtraMem_V2P(img_ptr->vld_cabac_table_ptr, HW_NO_CACHABLE))>>8, "vld cabac buffer address");
#endif
	
	close_vsp_iram();
	
	return MMDEC_OK;
}

PUBLIC void H264Dec_mb_level_sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
#if _CMODEL_
	mbc_module();
#endif

	VSP_READ_REG_POLL_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, 5, 1, 1, "MBC: polling mbc done");	
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, (0<<30), "hold next mb's mbc by setting mb type to be intra");

	VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (3<<24) |(MBC_CMD0_WOFF<<16) |(MBC_ST0_WOFF<<8)|((1<<7)|MBC_ST0_WOFF));

#if _CMODEL_
	dbk_module();
#endif	

	return;
}

PUBLIC MMDecRet H264Dec_Picture_Level_Sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 cmd/*, cmd1*/;
#ifdef _DEBUG_TIME_
	long long  cur_time;
#endif

	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
	VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec_h264, "VSP_TST: configure frame_cnt to debug register for end of picture");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (2<<24) | (VSP_TST_WOFF<<8)|((1<<7)|VSP_DBG_WOFF));
		
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
	VSP_WRITE_CMD_INFO(0x12345678);

#ifdef _DEBUG_TIME_
	gettimeofday(&tpend1,NULL);

	cur_time = tpend1.tv_usec-tpstart.tv_usec;
	if(cur_time < 0)	cur_time += 1000000;
	SCI_TRACE_LOW("SW  time frameNO %d %lld",g_nFrame_dec_h264,cur_time);
#endif

	if(!img_ptr->is_previous_cmd_done)
	{ 
            SCI_TRACE_LOW("%s, %d, polling PREVIOUS cqm", __FUNCTION__, __LINE__);
            VSP_START_CQM();
            img_ptr->is_previous_cmd_done = TRUE;
	}

	if (H264Dec_VSPInit () != MMDEC_OK)
	{
		return MMDEC_ERROR;
	}
	
	h264Dec_PicLevelSendRefAddressCommmand (img_ptr);

	if (img_ptr->is_need_init_vsp_hufftab /*&& img_ptr->is_new_pic*/)
	{
		configure_huff_tab ((uint32*)g_huff_tab_token, 69);
		//img_ptr->is_need_init_vsp_hufftab = FALSE;
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<15)|(1<<10), "DCAM_INT_RAW: enable CMD DONE INT bit");

	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		
	cmd |= ((img_ptr->width*img_ptr->height/4) << 4); //word unit
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, ((img_ptr->height << 16) | img_ptr->width), "configure frame width");
			
	//configure command infor and data address into register, and enable cmd-exe
#if _CMODEL_
 	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_INFO_BUF_ADDR_OFFSET, ((uint32)CMD_CONTROL_INFO_ADDR)>>8, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_DATA_BUF_ADDR_OFFSET, ((uint32)CMD_CONTROL_DATA_ADDR)>>8, "config CMD control data buffer address");
#else
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_INFO_BUF_ADDR_OFFSET, ((uint32)H264Dec_ExtraMem_V2P(g_cmd_info_base, HW_CACHABLE))>>8, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_DATA_BUF_ADDR_OFFSET, ((uint32)H264Dec_ExtraMem_V2P(g_cmd_data_base, HW_CACHABLE))>>8, "config CMD control data buffer address");
#endif

//flush cache
	if(VSP_fluchCacheCb)
	{
    	    MMCodecBuffer IonBuffer;
	    H264Dec_ExtraMem_GetInfo(&IonBuffer, HW_CACHABLE);
            
    	    int ret = (*VSP_fluchCacheCb)(g_user_data,(int *)(g_cmd_data_base),(int *)(H264Dec_ExtraMem_V2P(g_cmd_data_base, HW_CACHABLE)),IonBuffer.size/2);          
	}
	//now, can enable cmd-exe
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd |= (1<<8) | (1<<9);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");

	img_ptr->is_previous_cmd_done = FALSE;

	return MMDEC_OK;
}

void write_vld_cabac(DEC_IMAGE_PARAMS_T *img_ptr, uint32 *cabac_bfr_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

	int i;
	uint32 cmd = 0;
	uint8 *ctx;
	uint32 bfr_ptr;
#if _CMODEL_
	uint32 *huff_tab = g_hvld_huff_tab;
#endif

	//cfg coded_block_flag context
	{
		uint32 i, tbl_idx;
//		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		bfr_ptr = (uint32 )cabac_bfr_ptr;

		ctx = &(img_ptr->cabac_state[85]); //&(tc->bcbp_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = i; //map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*4+3]&0x1)<<6) | (ctx[tbl_idx*4+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*4+2]&0x1)<<6) | (ctx[tbl_idx*4+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*4+1]&0x1)<<6) | (ctx[tbl_idx*4+1]>>1) ) << 8 ) |
				  ( ((ctx[tbl_idx*4+0]&0x1)<<6)   | (ctx[tbl_idx*4+0]>>1) )	;	
			WRITE_VLD_CABAC_BFR(bfr_ptr+i*4, cmd, "HUFFMAN_TBL_ADDR: configure vlc table");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}
	}
	
	//cfg last, siginicence
	{	
		uint8 *lst_ctx, *sig_ctx;
		
		uint32 LAST_SIG_CAT0_ADDR	= (bfr_ptr+5*4);
		uint32 LAST_SIG_CAT1_ADDR	= (LAST_SIG_CAT0_ADDR+8*4);
		uint32 LAST_SIG_CAT2_ADDR	= (LAST_SIG_CAT1_ADDR+8*4);
		uint32 LAST_SIG_CAT3_ADDR	= (LAST_SIG_CAT2_ADDR+8*4);
		uint32 LAST_SIG_CAT4_ADDR	= (LAST_SIG_CAT3_ADDR+2*4);

		//block ctx 0 (luma-intra16-DC)
		lst_ctx = &(img_ptr->cabac_state[166]);	//&(tc->last_contexts[0][0]); 
		sig_ctx = &(img_ptr->cabac_state[105]);	//&(tc->map_contexts[0][0]); 
		for (i = 0; i < 15; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT0_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT0_ADDR: luma-intra16-DC");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}

		//block ctx 1 (luma-intra16-AC)
		lst_ctx = &(img_ptr->cabac_state[181]);	//&(tc->last_contexts[1][0]); 
		sig_ctx = &(img_ptr->cabac_state[120]);	//&(tc->map_contexts[1][0]); 
		for (i = 0; i < 14; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT1_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT1_ADDR: luma-intra16-AC");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT1_ADDR+(15>>1)*4, cmd, "LAST_SIG_CAT1_ADDR: RESV");
	#if _CMODEL_	
		*huff_tab++ = cmd;
	#endif

		//block ctx 2 (luma-4x4)
		lst_ctx = &(img_ptr->cabac_state[195]);	//&(tc->last_contexts[5][0]); 
		sig_ctx = &(img_ptr->cabac_state[134]);	//&(tc->map_contexts[5][0]); 
		for (i = 0; i < 15; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT2_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT2_ADDR: luma-4x4");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}

		//block ctx 3 (chroma-DC)
		lst_ctx = &(img_ptr->cabac_state[210]);	//&(tc->last_contexts[6][0]); 
		sig_ctx = &(img_ptr->cabac_state[149]);	//&(tc->map_contexts[6][0]); 
		for (i = 0; i < 3; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT3_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT3_ADDR: chroma-DC");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}

		//block ctx 4 (chroma-AC)
		lst_ctx = &(img_ptr->cabac_state[213]);	//&(tc->last_contexts[7][0]); 
		sig_ctx = &(img_ptr->cabac_state[152]);	//&(tc->map_contexts[7][0]); 
		for (i = 0; i < 14; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT4_ADDR: chroma-AC");
		#if _CMODEL_	
			*huff_tab++ = cmd;
		#endif
		}
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(15>>1)*4, cmd, "LAST_SIG_CAT4_ADDR: RESV");
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(16>>1)*4, cmd, "RESV0");
	}
}

uint32 g_vld_cabac_offset = 0;

PUBLIC MMDecRet H264Dec_init_CABAC_VSP(DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 cmd = 0;
	uint8 *ctx;
	uint32 *cabac_bfr_ptr;

#if !_CMODEL_
	g_vld_cabac_offset = (img_ptr->slice_nr * 40);//word
#endif
	cabac_bfr_ptr = img_ptr->vld_cabac_table_ptr + g_vld_cabac_offset;

#if _CMODEL_
	memset (g_hvld_huff_tab, 0, 69*sizeof(int));
#endif

	write_vld_cabac(img_ptr, cabac_bfr_ptr);

	cmd = (1<<31) | (40<<20)|g_vld_cabac_offset;
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_VDB_CFG_OFFSET, cmd, "HVLD_VDB_CFG:");
	VSP_READ_REG_POLL_CQM(VSP_VLD_REG_BASE+HVLD_VDB_CFG_OFFSET, 31, 1, 0, "polling vld fetch done");
	VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (2<<24) | (((1<<7)|HVLD_VDB_CFG_WOFF)<<8)|HVLD_VDB_CFG_WOFF);
#if _CMODEL_
	g_vld_cabac_offset += (40); //word
#endif
	
	//cfg bin0
	{
		uint32 i, tbl_idx;
//		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		ctx = &(img_ptr->cabac_state[227]);	//&(tc->one_contexts[0][0]);
		for (i = 0; i < 4; i++) //4+1 is block category num
		{
			tbl_idx = i;//map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*10+3]&0x1)<<6) | (ctx[tbl_idx*10+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*10+2]&0x1)<<6) | (ctx[tbl_idx*10+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*10+1]&0x1)<<6) | (ctx[tbl_idx*10+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*10+0]&0x1)<<6) | (ctx[tbl_idx*10+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BIN0_CAT0: bin0");
		}
		cmd = ( ( ((ctx[4*10+3-1]&0x1)<<6) | (ctx[4*10+3-1]>>1) ) << 24) |
		  ( ( ((ctx[4*10+2-1]&0x1)<<6) | (ctx[4*10+2-1]>>1) ) << 16) |
		  ( ( ((ctx[4*10+1-1]&0x1)<<6) | (ctx[4*10+1-1]>>1) ) << 8 ) |
		  ( ( ((ctx[4*10+0-1]&0x1)<<6) | (ctx[4*10+0-1]>>1) ) << 0 )	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_CAT0_OFFSET+4*4, cmd, "HVLD_CTX_BIN0_CAT0: bin0");

		//inc4
		cmd = ( ( ((ctx[0*10+4]&0x1)<<6) | (ctx[0*10+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*10+4]&0x1)<<6) | (ctx[1*10+4]>>1) ) <<  8) |
			  ( ( ((ctx[2*10+4]&0x1)<<6) | (ctx[2*10+4]>>1) ) << 16) |
			  ( ( ((ctx[3*10+4]&0x1)<<6) | (ctx[3*10+4]>>1) ) << 24)	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT0_3_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT0_3");

		//ctx_bin0_inc4_cat4
		cmd = ( ( ((ctx[270-227]&0x1)<<6) | (ctx[270-227]>>1) ) << 0 )	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT4_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT4");

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (7<<24) | (HVLD_CTX_BIN0_CAT2_WOFF<<16)|(HVLD_CTX_BIN0_CAT1_WOFF<<8)|HVLD_CTX_BIN0_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BIN0_INC4_CAT4_WOFF<<24)|(HVLD_CTX_BIN0_INC4_CAT0_3_WOFF<<16)|(HVLD_CTX_BIN0_CAT4_WOFF<<8)|HVLD_CTX_BIN0_CAT3_WOFF);
	}

	//cfg binother
	{
		uint32 i, tbl_idx;
//		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		ctx = &(img_ptr->cabac_state[232]);	//&(tc->abs_contexts[0][0]);
		for (i = 0; i < 4; i++) //4+1 is block category num
		{
			tbl_idx = i;//map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*10+3]&0x1)<<6) | (ctx[tbl_idx*10+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*10+2]&0x1)<<6) | (ctx[tbl_idx*10+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*10+1]&0x1)<<6) | (ctx[tbl_idx*10+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*10+0]&0x1)<<6) | (ctx[tbl_idx*10+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BINOTH_CAT0");
		}
		cmd = ( ( ((ctx[4*10+3-1]&0x1)<<6) | (ctx[4*10+3-1]>>1) ) << 24) |
		  ( ( ((ctx[4*10+2-1]&0x1)<<6) | (ctx[4*10+2-1]>>1) ) << 16) |
		  ( ( ((ctx[4*10+1-1]&0x1)<<6) | (ctx[4*10+1-1]>>1) ) << 8 ) |
		  ( ( ((ctx[4*10+0-1]&0x1)<<6) | (ctx[4*10+0-1]>>1) ) << 0 )	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_CAT0_OFFSET+4*4, cmd, "HVLD_CTX_BINOTH_CAT0");

		//inc4
		cmd = ( ( ((ctx[0*10+4]&0x1)<<6) | (ctx[0*10+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*10+4]&0x1)<<6) | (ctx[1*10+4]>>1) ) <<  8) |
			  ( ( ((ctx[2*10+4]&0x1)<<6) | (ctx[2*10+4]>>1) ) << 16) |
			  ( ( ((ctx[4*10+4-1]&0x1)<<6) | (ctx[4*10+4-1]>>1) ) << 24)	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_INC4_OFFSET, cmd, "HVLD_CTX_BINOTH_INC4");

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (6<<24) | (HVLD_CTX_BINOTH_CAT2_WOFF<<16)|(HVLD_CTX_BINOTH_CAT1_WOFF<<8)|HVLD_CTX_BINOTH_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BINOTH_INC4_WOFF<<16)|(HVLD_CTX_BINOTH_CAT4_WOFF<<8)|HVLD_CTX_BINOTH_CAT3_WOFF);
	}

	return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
