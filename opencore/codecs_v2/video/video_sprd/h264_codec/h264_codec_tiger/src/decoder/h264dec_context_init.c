/******************************************************************************
 ** File Name:      h264dec_biaridecod.c                                      *
 ** Author:         Xiaowei.Luo                                               *
 ** DATE:           03/29/2010                                                *
 ** Copyright:      2010 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                      *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 03/29/2010     Xiaowei.Luo      Create.                                   *
 ******************************************************************************/
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define BIARI_CTX_INIT2(ii,jj,ctx,tab,num) \
{ \
  for (i=0; i<ii; i++) \
  for (j=0; j<jj; j++) \
  { \
    if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][i][j][0])); \
    else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][i][j][0])); \
  } \
}
#define BIARI_CTX_INIT1(jj,ctx,tab,num) \
{ \
  for (j=0; j<jj; j++) \
  { \
  if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[j]), &(tab ## _I[num][0][j][0])); \
    else                            biari_init_context (img_ptr, &(ctx[j]), &(tab ## _P[num][0][j][0])); \
  } \
}

void write_vld_cabac(DEC_IMAGE_PARAMS_T *img_ptr, uint32 *cabac_bfr_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

	MotionInfoContexts*  mc = curr_slice_ptr->mot_ctx;
	TextureInfoContexts* tc = curr_slice_ptr->tex_ctx;
	int i;
	uint32 cmd = 0;
	BiContextType *ctx;
	uint32 bfr_ptr;
#if _CMODEL_
	uint32 *huff_tab = g_hvld_huff_tab;
#endif

	//cfg coded_block_flag context
	{
		uint32 i, tbl_idx;
		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		bfr_ptr = (uint32 )cabac_bfr_ptr;

		ctx = &(tc->bcbp_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = map_tbl[i];
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
		BiContextType *lst_ctx, *sig_ctx;
		
		uint32 LAST_SIG_CAT0_ADDR	= (bfr_ptr+5*4);
		uint32 LAST_SIG_CAT1_ADDR	= (LAST_SIG_CAT0_ADDR+8*4);
		uint32 LAST_SIG_CAT2_ADDR	= (LAST_SIG_CAT1_ADDR+8*4);
		uint32 LAST_SIG_CAT3_ADDR	= (LAST_SIG_CAT2_ADDR+8*4);
		uint32 LAST_SIG_CAT4_ADDR	= (LAST_SIG_CAT3_ADDR+2*4);

		//block ctx 0 (luma-intra16-DC)
		lst_ctx = &(tc->last_contexts[0][0]); 
		sig_ctx = &(tc->map_contexts[0][0]); 
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
		lst_ctx = &(tc->last_contexts[1][0]); 
		sig_ctx = &(tc->map_contexts[1][0]); 
		for (i = 1; i < 15; i+=2)
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
		lst_ctx = &(tc->last_contexts[5][0]); 
		sig_ctx = &(tc->map_contexts[5][0]); 
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
		lst_ctx = &(tc->last_contexts[6][0]); 
		sig_ctx = &(tc->map_contexts[6][0]); 
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
		lst_ctx = &(tc->last_contexts[7][0]); 
		sig_ctx = &(tc->map_contexts[7][0]); 
		for (i = 1; i < 15; i+=2)
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

void init_contexts (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

	MotionInfoContexts*  mc = curr_slice_ptr->mot_ctx;
	TextureInfoContexts* tc = curr_slice_ptr->tex_ctx;
	int i, j;
	uint32 *cabac_bfr_ptr;

#if !_CMODEL_
	g_vld_cabac_offset = (img_ptr->slice_nr * 40);//word
#endif
	cabac_bfr_ptr = img_ptr->vld_cabac_table_ptr + g_vld_cabac_offset;
	//printf("%d -", img->model_number);

	//--- motion coding contexts ---
	BIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mb_type_contexts,     INIT_MB_TYPE,    img_ptr->model_number);
	BIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    img_ptr->model_number);
	BIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mv_res_contexts,      INIT_MV_RES,     img_ptr->model_number);
	BIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     img_ptr->model_number);
	BIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   img_ptr->model_number);
	BIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     img_ptr->model_number);

	//--- texture coding contexts ---
	BIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       img_ptr->model_number);
	BIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      img_ptr->model_number);
	BIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts,     INIT_MAP,       img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts,    INIT_LAST,      img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->fld_map_contexts, INIT_FLD_MAP,   img_ptr->model_number);
	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->fld_last_contexts,INIT_FLD_LAST,  img_ptr->model_number);

{
	uint32 cmd = 0;
	BiContextType *ctx;

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
		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		ctx = &(tc->one_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*5+3]&0x1)<<6) | (ctx[tbl_idx*5+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*5+2]&0x1)<<6) | (ctx[tbl_idx*5+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*5+1]&0x1)<<6) | (ctx[tbl_idx*5+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*5+0]&0x1)<<6) | (ctx[tbl_idx*5+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BIN0_CAT0: bin0");
		}

		//inc4
		cmd = ( ( ((ctx[0*5+4]&0x1)<<6) | (ctx[0*5+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*5+4]&0x1)<<6) | (ctx[1*5+4]>>1) ) <<  8) |
			  ( ( ((ctx[4*5+4]&0x1)<<6) | (ctx[4*5+4]>>1) ) << 16) |
			  ( ( ((ctx[5*5+4]&0x1)<<6) | (ctx[5*5+4]>>1) ) << 24)	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT0_3_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT0_3");

		//ctx_bin0_inc4_cat4
		cmd = ( ( ((ctx[6*5+4]&0x1)<<6) | (ctx[6*5+4]>>1) ) << 0 )	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT4_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT4");

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (7<<24) | (HVLD_CTX_BIN0_CAT2_WOFF<<16)|(HVLD_CTX_BIN0_CAT1_WOFF<<8)|HVLD_CTX_BIN0_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BIN0_INC4_CAT4_WOFF<<24)|(HVLD_CTX_BIN0_INC4_CAT0_3_WOFF<<16)|(HVLD_CTX_BIN0_CAT4_WOFF<<8)|HVLD_CTX_BIN0_CAT3_WOFF);
	}

	//cfg binother
	{
		uint32 i, tbl_idx;
		uint8 map_tbl[5] = {0, 1, 4, 5, 6};

		ctx = &(tc->abs_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*5+3]&0x1)<<6) | (ctx[tbl_idx*5+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*5+2]&0x1)<<6) | (ctx[tbl_idx*5+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*5+1]&0x1)<<6) | (ctx[tbl_idx*5+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*5+0]&0x1)<<6) | (ctx[tbl_idx*5+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BINOTH_CAT0");
		}

		//inc4
		cmd = ( ( ((ctx[0*5+4]&0x1)<<6) | (ctx[0*5+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*5+4]&0x1)<<6) | (ctx[1*5+4]>>1) ) <<  8) |
			  ( ( ((ctx[4*5+4]&0x1)<<6) | (ctx[4*5+4]>>1) ) << 16) |
			  ( ( ((ctx[6*5+4]&0x1)<<6) | (ctx[6*5+4]>>1) ) << 24)	;	
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_INC4_OFFSET, cmd, "HVLD_CTX_BINOTH_INC4");

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (6<<24) | (HVLD_CTX_BINOTH_CAT2_WOFF<<16)|(HVLD_CTX_BINOTH_CAT1_WOFF<<8)|HVLD_CTX_BINOTH_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BINOTH_INC4_WOFF<<16)|(HVLD_CTX_BINOTH_CAT4_WOFF<<8)|HVLD_CTX_BINOTH_CAT3_WOFF);
	}	
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
