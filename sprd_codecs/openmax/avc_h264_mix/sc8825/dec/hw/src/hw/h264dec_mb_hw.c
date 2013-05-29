/******************************************************************************
 ** File Name:    h264dec_mb.c			                                      *
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
#include "sc8825_video_header.h"
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
LOCAL void H264Dec_read_and_derive_ipred_modes_hw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	//get 16 block4x4 intra prediction mode, luma
	if (mb_info_ptr->mb_type == I4MB_264)
	{
		int32 blk4x4StrIdx, blk4x4Idx;
		int32 pred_mode;
		int32 up_ipred_mode;
		int32 left_ipred_mode;
		int32 most_probable_ipred_mode;
		int8 *i4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;
		const uint8 *blk_order_map_tbl_ptr = g_blk_order_map_tbl;
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;
		int32 val;

		int32 i, j;
		uint32 mbc_id_map[8] = {3, 1, 2, 0, 7, 5, 6, 4};

		for (blk4x4Idx = 0, i = 0; i < 8; i++)
		{	
			uint32 maped_id = mbc_id_map[i];
			mb_cache_ptr->mbc_ipred_cmd[maped_id]  = 0;
			for (j = 1; j >= 0; j--)
			{
				if (!img_ptr->is_cabac)
				{
					val = BITSTREAMSHOWBITS(bitstrm, 4);
					if(val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!, noted by xwluo@20100618
					{
						pred_mode = -1;
						READ_BITS1(bitstrm);
					}else
					{
						pred_mode = val & 0x7; //remaining_mode_selector
						READ_FLC(bitstrm, 4);
					}
				}else
				{
					pred_mode = decode_cabac_mb_intra4x4_pred_mode (img_ptr);
				}
				
				blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
				left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
				up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-CTX_CACHE_WIDTH];
				most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

				if (most_probable_ipred_mode < 0)
				{
					most_probable_ipred_mode = DC_PRED;
				}

				pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));
					

			#if _H264_PROTECT_ & _LEVEL_LOW_
				//must! because BITSTRM may be error
				if ((pred_mode >9) || (pred_mode < 0))
				{
					img_ptr->error_flag |= ER_BSM_ID;
                    			img_ptr->return_pos |= (1<<24);
					return;
				}
			#endif	

				i4_pred_mode_ref_ptr[blk4x4StrIdx] = pred_mode;
				mb_cache_ptr->mbc_ipred_cmd[maped_id] |= (pred_mode << (4*j));
				blk4x4Idx++;
			}
		}
		
		mb_cache_ptr->mbc_mb_mode = MBC_I4x4;
	}else
	{
		int8 *i4x4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;

		((int32*)i4x4_pred_mode_ref_ptr)[4] = 
		((int32*)i4x4_pred_mode_ref_ptr)[7] = 
		((int32*)i4x4_pred_mode_ref_ptr)[10] = 
		((int32*)i4x4_pred_mode_ref_ptr)[13] = 0x02020202;	

		mb_cache_ptr->mbc_mb_mode = MBC_I16x16;

		((uint32*)(mb_cache_ptr->mbc_ipred_cmd))[0] = ((uint32*)(mb_cache_ptr->mbc_ipred_cmd))[1] = mb_cache_ptr->i16mode;
#if _H264_PROTECT_ & _LEVEL_LOW_
		if ((mb_cache_ptr->i16mode > 4) || (mb_cache_ptr->i16mode < 0))
		{
			img_ptr->error_flag |= ER_BSM_ID;
			img_ptr->return_pos |= (1<<25);
			return;
		}
#endif		
	}

	if (!img_ptr->is_cabac)
	{
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;

		//get chroma intra prediction mode
		mb_info_ptr->c_ipred_mode = READ_UE_V(bitstrm);
	}else
	{
		mb_info_ptr->c_ipred_mode = decode_cabac_mb_chroma_pre_mode(img_ptr, mb_info_ptr, mb_cache_ptr);
	}

	return;
}

/************************************************************************/
/* decode I_PCM data                                                    */
/************************************************************************/
LOCAL void H264Dec_decode_IPCM_MB_hw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, DEC_BS_T * bitstrm)
{
	int32 i,j;
	int32 bit_offset;
	int32 comp;
	
	bit_offset  = (bitstrm->bitsLeft & 0x7);
	if(bit_offset)
	{
		READ_FLC(bitstrm, bit_offset);
	}

	if (img_ptr->cabac.low & 0x1)
	{
		REVERSE_BITS(bitstrm, 8);
	}

	if (img_ptr->cabac.low & 0x1ff)
	{
		REVERSE_BITS(bitstrm, 8);
	}

	//Y, U, V
	for (comp = 0; comp < 3; comp++)
	{
		int32 blk_size = (comp == 0) ? MB_SIZE : MB_CHROMA_SIZE;
		
		for (j = 0; j < blk_size; j++)
		{
			for (i = 0; i < blk_size; i+=4)
			{
				READ_FLC(bitstrm, 32);
			}
		}
	}

	H264Dec_set_IPCM_nnz(mb_cache_ptr);

	mb_cache_ptr->cbp_iqt = 0;
	mb_cache_ptr->cbp_mbc = 0xffffff;

	//for cabac
	mb_info_ptr->c_ipred_mode = 0;
	mb_info_ptr->cbp = 0x3f;
	mb_info_ptr->skip_flag = 1;
	//For CABAC decoding of Dquant
	if (img_ptr->is_cabac)
	{
		uint32 nStuffedBits;

		last_dquant=0;
		mb_info_ptr->dc_coded_flag = 7;
		nStuffedBits = ff_init_cabac_decoder(img_ptr);

		if (nStuffedBits)
		{
			uint32 cmd = (nStuffedBits<<24) | 1;

			VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush N bits");	
#if _CMODEL_
			flush_nbits(nStuffedBits);
#endif
			VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);
		}
	}
}

LOCAL void H264Dec_read_intraMB_context_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);
	
	if (mb_info_ptr->mb_type != IPCM)
	{
		H264Dec_read_and_derive_ipred_modes_hw (img_ptr, mb_info_ptr, mb_cache_ptr);
		H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		int32 *i4x4pred_mode_cache = (int32 *)(mb_cache_ptr->i4x4pred_mode_cache);
		
		i4x4pred_mode_cache[4] = i4x4pred_mode_cache[7] =
		i4x4pred_mode_cache[10] = i4x4pred_mode_cache[13] = 0x02020202;	

		mb_cache_ptr->mbc_mb_mode = MBC_IPCM;
		mb_info_ptr->qp = 0;
	}
}

PUBLIC void H264Dec_read_one_macroblock_ISlice_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->is_intra = TRUE;

	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_I (mb_info_ptr, mb_cache_ptr);	
	H264Dec_read_intraMB_context_hw(img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

PUBLIC void H264Dec_read_one_macroblock_PSlice_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_P(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context_hw(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
#if _H264_PROTECT_ & _LEVEL_LOW_
		if (g_list[0][0] == NULL)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
            		img_ptr->return_pos |= (1<<26);
			return;
		}
#endif		

		mb_cache_ptr->read_ref_id_flag[0] =( (img_ptr->ref_count[0] > 1) && (!mb_cache_ptr->all_zero_ref));
		mb_cache_ptr->read_ref_id_flag[1] = ((img_ptr->ref_count[1] > 1) && (!mb_cache_ptr->all_zero_ref));

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			mb_cache_ptr->cbp_iqt = 0;
			mb_cache_ptr->cbp_mbc = 0;
		}
	}
	
	return;
}

PUBLIC void H264Dec_read_one_macroblock_BSlice_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_B(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context_hw(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
#if _H264_PROTECT_ & _LEVEL_LOW_
		if (g_list[0][0] == NULL)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos1 |= (1<<1);
			return;
		}
#endif		

		mb_cache_ptr->read_ref_id_flag[0] = (img_ptr->ref_count[0] > 1);
		mb_cache_ptr->read_ref_id_flag[1] = (img_ptr->ref_count[1] > 1);

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);
		
		if (!mb_cache_ptr->is_skipped)
		{
#if 0
			if (IS_INTERMV(mb_info_ptr))
#else
			if (mb_info_ptr->mb_type != 0)
#endif
			{
				H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			}
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			mb_cache_ptr->cbp_iqt = 0;
			mb_cache_ptr->cbp_mbc = 0;
		}

		if ( mb_cache_ptr->is_direct)
		{
			direct_mv (img_ptr, mb_info_ptr, mb_cache_ptr);
		}
	}
	
	return;
}

uint32 vsp_mbtype_map[15] = {	H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, 
								H264_INTERMB, H264_INTERMB, H264_IMB4X4, H264_IMB16X16, H264_INTERMB, H264_INTERMB, H264_INTERMB, 
								H264_IPCM
							};
							
PUBLIC void H264Dec_start_vld_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 up_nnz_y, left_nnz_y;
	uint32 nnz_cb, nnz_cr;
	uint32 vsp_mb_type, vsp_lmb_type, vsp_tmb_type;
	int32 left_mb_avail, top_mb_avail;
	int8 *nnz_ref_ptr;
	uint32 cmd;
	uint32 flush_bits = img_ptr->bitstrm_ptr->bitcnt - img_ptr->bitstrm_ptr->bitcnt_before_vld;

	{
		uint32 nWords = flush_bits/32;
		uint32 i = 0;

		flush_bits -= nWords*32;
		cmd = (flush_bits<<24) | 1;
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush N bits");	
	#if _CMODEL_
		flush_nbits(flush_bits);
	#endif
		VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);


		if (nWords)
		{
			cmd = (32<<24) | 1;

			for (i = 0; i < nWords; i++)
			{
				VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");
				VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one word");	
		#if _CMODEL_
				flush_nbits(32);
		#endif
				
				VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7)|BSM_DEBUG_WOFF));
			}
		}
	}

	if (img_ptr->is_cabac)
	{
	//	DecodingEnvironment* dep = &img_ptr->de_cabac;
		CABACContext *c = &(img_ptr->cabac);
		cmd = ((c->range << 16) | (c->low & 0x1ff)); 

		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_ARTHI_BS_STATE_OFFSET, cmd, "configure bistream status");

		cmd = ( (mb_info_ptr - img_ptr->frame_width_in_mbs)->dc_coded_flag <<4) |((mb_info_ptr - 1)->dc_coded_flag);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CODED_DC_FLAG_OFFSET, cmd, "configure decode_dc_flag");
		mb_cache_ptr->vld_dc_coded_flag = cmd;

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (2<<24) |(HVLD_CODED_DC_FLAG_WOFF<<8) |HVLD_ARTHI_BS_STATE_WOFF);
	}
	
	vsp_mb_type	=	vsp_mbtype_map[mb_info_ptr->mb_type];
	vsp_lmb_type	=	vsp_mbtype_map[(mb_info_ptr-1)->mb_type];
	vsp_tmb_type	=	vsp_mbtype_map[img_ptr->abv_mb_info->mb_type];

	//configure mb information
	left_mb_avail = mb_cache_ptr->mb_avail_a;
	top_mb_avail = mb_cache_ptr->mb_avail_b;

	//require to configure entropy decoding type, and left/top MB type
	cmd = (img_ptr->is_cabac << 31) | (vsp_lmb_type << 20) | (vsp_tmb_type << 22) |
		  ((top_mb_avail<<17) | (left_mb_avail << 16) | (mb_info_ptr->cbp << 8) | (vsp_mb_type << 0));
	
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_MB_INFO_OFFSET, cmd, "configure mb information");

	//configure nnz	
	nnz_ref_ptr = mb_cache_ptr->nnz_cache;
	up_nnz_y	= (nnz_ref_ptr[4]<<24) | (nnz_ref_ptr[5]<<16) | (nnz_ref_ptr[6]<<8) | (nnz_ref_ptr[7]);
	left_nnz_y	= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X1 + 3] << 24)  | 
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X2 + 3] << 16)  |
			     		(nnz_ref_ptr[CTX_CACHE_WIDTH_X3 + 3] << 8  )  |
			     		(nnz_ref_ptr[CTX_CACHE_WIDTH_X4 + 3]);
	nnz_cb		= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 4] << 24) |
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 5] << 16) |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X6 + 3] << 8)   |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X7 + 3]);
	nnz_cr		= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 8] << 24) |
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 9] << 16) |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X6 + 7] << 8)   |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X7 + 7]);
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TOP_NNZ_Y_OFFSET,	up_nnz_y, "configure up_nnz_y");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_LEFT_NNZ_Y_OFFSET,	left_nnz_y, "configure left_nnz_y");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CB_OFFSET,	nnz_cb, "configure nnz_cb");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CR_OFFSET,	nnz_cr, "configure nnz_cr");
	
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, 1, "start vld one mb");

#if _CMODEL_
 	H264VldMBCtr ();
#endif

	VSP_READ_REG_POLL_CQM(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, 1, 1, 1,"VLD: polling VLD one MB status");
	
	VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (7<<24) | (HVLD_LEFT_NNZ_Y_WOFF<<16)|(HVLD_TOP_NNZ_Y_WOFF<<8) |HVLD_MB_INFO_WOFF);
	VSP_WRITE_CMD_INFO((((1<<7)|HVLD_CTRL_WOFF)<<24) | (HVLD_CTRL_WOFF<<16) | (HVLD_TL_NNZ_CR_WOFF<<8) |HVLD_TL_NNZ_CB_WOFF);

//SW VLD
	if (mb_info_ptr->mb_type != IPCM)
	{
		mb_cache_ptr->cbp_luma_iqt = 0;

		sw_vld_mb (mb_info_ptr, mb_cache_ptr);
		H264Dec_ComputeCBPIqtMbc (mb_info_ptr, mb_cache_ptr);
	}else
	{
		H264Dec_decode_IPCM_MB_hw(img_ptr, mb_info_ptr, mb_cache_ptr, img_ptr->bitstrm_ptr);
	}

	img_ptr->bitstrm_ptr->bitcnt_before_vld = img_ptr->bitstrm_ptr->bitcnt;

	return;
}

uint32 s_iqt_mb_type[] = {	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER, 	IQT_OTHER,	IQT_OTHER,	IQT_OTHER, 
							IQT_OTHER,	IQT_OTHER,	IQT_I16,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_PCM,	IQT_OTHER 
					};

PUBLIC void H264Dec_start_iqt_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 cbp26, mb_info_cmd, qp_info_cmd;
	int32 mb_type	= mb_info_ptr->mb_type;
	int32 cbp		= mb_info_ptr->cbp;
	int32 cbp_uv	= mb_cache_ptr->cbp_uv;
	
	if ((mb_type == IPCM) || (mb_cache_ptr->is_skipped))//IPCM or skip_mb
	{
		cbp26 = 0;
		mb_info_cmd = 0;
	}else
	{
		int32 qp_c;
		
		cbp26 = mb_cache_ptr->cbp_iqt;
		mb_info_cmd = cbp26;
		qp_c = g_QP_SCALER_CR_TBL[IClip(0, 51, mb_info_ptr->qp+img_ptr->chroma_qp_offset)];
		qp_info_cmd = (g_qpPerRem_tbl[mb_info_ptr->qp]<<4) | g_qpPerRem_tbl[qp_c];
		VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CFG1_OFF, qp_info_cmd, "configure qp information"); 
		VSP_WRITE_CMD_INFO((VSP_DCT << CQM_SHIFT_BIT) | (1<<24) |IICT_CFG1_WOFF);
	}
	
	//intra and inter(with non-zero coeff) should start idct, otherwise MUST NOT
	if (!mb_info_ptr->is_intra && (cbp26 == 0))
	{
		//MUST!! don't start IICT
		return;
	}	

	mb_info_cmd |= s_iqt_mb_type[mb_type];
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CFG0_OFF, mb_info_cmd, "configure mb_type and cbp");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CTRL_OFF, 1, "start iqt");
		
#if _CMODEL_
	iqt_module();
#endif
	
	VSP_READ_REG_POLL_CQM(VSP_DCT_REG_BASE+IICT_CTRL_OFF, 1, 1, 1,"iict: polling iict one MB status");	
	VSP_WRITE_CMD_INFO((VSP_DCT << CQM_SHIFT_BIT) | (3<<24) |(((1<<7)|IICT_CTRL_WOFF)<<16)|(IICT_CTRL_WOFF<<8)|IICT_CFG0_WOFF);
	
	return;
}

PUBLIC void H264Dec_start_mbc_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 cmd;

	if (mb_info_ptr->is_intra)
	{
		int32 mb_avail_a = mb_cache_ptr->mb_avail_a;
		int32 mb_avail_b = mb_cache_ptr->mb_avail_b;
		int32 mb_avail_c = mb_cache_ptr->mb_avail_c;
		int32 mb_avail_d = mb_cache_ptr->mb_avail_d;
		uint32 ipred_cmd0, ipred_cmd1;

		ipred_cmd0 = ((uint32 *)(mb_cache_ptr->mbc_ipred_cmd))[0];
		ipred_cmd1 = ((uint32 *)(mb_cache_ptr->mbc_ipred_cmd))[1];
		
		if (img_ptr->constrained_intra_pred_flag)
		{
			if (mb_avail_a)
			{
				mb_avail_a = (mb_info_ptr-1)->is_intra;
			}

			if (mb_avail_b)
			{
				mb_avail_b = img_ptr->abv_mb_info->is_intra;
			}

			if (mb_avail_c)
			{
				mb_avail_c = (img_ptr->abv_mb_info+1)->is_intra;
			}

			if (mb_avail_d)
			{
				mb_avail_d = (img_ptr->abv_mb_info-1)->is_intra;
			}
		}

		cmd = (MBC_INTRA_MB<<30) |	((mb_cache_ptr->mbc_mb_mode & 0x3) << 28) | (mb_avail_d << 27) | (mb_avail_a << 26) | 
			(mb_avail_b << 25) | (mb_avail_c << 24) | (mb_cache_ptr->cbp_mbc & 0xffffff);
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");

		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD1_OFF, ipred_cmd0, "configure mb intra prediction mode, luma sub-block 0-7");
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD2_OFF, ipred_cmd1, "configure mb intra prediction mode, luma sub-block 8-15");

		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD3_OFF, (mb_info_ptr->c_ipred_mode & 0x3), "configure mb intra prediction mode, block 0-7");

		VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (4<<24) |(MBC_CMD2_WOFF<<16)|(MBC_CMD1_WOFF<<8)|MBC_CMD0_WOFF);
		VSP_WRITE_CMD_INFO(MBC_CMD3_WOFF);
	}else //inter mb
	{
		cmd = (MBC_INTER_MB<<30) |	(mb_cache_ptr->cbp_mbc & 0xffffff);
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");
		VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (1<<24) |MBC_CMD0_WOFF);
	}
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
