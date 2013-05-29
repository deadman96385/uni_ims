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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//BiContextType context[308];
volatile int *context;//weihu

int count;
const int INIT_MB_TYPE[3][11] =
{
	{1,1,1,0,1,1,1,1,1,0,0},
	{1,1,1,0,1,1,1,1,1,1,1},
	{1,1,1,0,1,1,1,1,1,1,0}
};
const int INIT_B8_TYPE[2][4] =
{
	{1,1,1,0},
	{1,1,1,1}
};
const int INIT_MV_RES[2][8] =
{
	{1,0,1,1,1,0,1,1},
	{1,1,1,1,1,1,1,1}
};
const int INIT_REF_NO[1][6] =
{
	{1,1,1,1,1,1}
};
const int INIT_DELTA_QP[4] ={1,1,1,1};
const int INIT_MB_AFF[3] ={1,1,1};
const int INIT_TRANSFORM_SIZE[3] ={1,1,1};
const int INIT_IPR[2] ={1,1};
const int INIT_CIPR[4] ={1,1,1,1};
const int INIT_CBP[3][4] =
{
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1},
};
const int INIT_BCBP[5][4] =
{
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1}
};
const int INIT_MAP[6][15] =
{
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};
const int INIT_LAST[6][15] =
{
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};
const int INIT_ONE[6][5] =
{
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1}
};
const int INIT_ABS[6][5] =
{
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,1},
	{1,1,1,1,0},
	{1,1,1,1,1}
};
#if SIM_IN_WIN
	#define BIARI_CTX_INIT3(ii,jj,ctx,tab,num) \
	{ \
	  for (i=0; i<ii; i++) \
	  for (j=0; j<jj; j++) \
	  { \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][tab ## _MAP[i]][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][tab ## _MAP[i]][j][0])); \
		if (tab[i][j])\
		{\
			OR1200_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[i][j],"cabac context");\
			context[map_context_idx(count)]=ctx[i][j];\
			count++;\
		}\
	} \
	}
	#define BIARI_CTX_INIT2(ii,jj,ctx,tab,num) \
	{ \
	  for (i=0; i<ii; i++) \
	  for (j=0; j<jj; j++) \
	{ \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][i][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][i][j][0])); \
		if (tab[i][j])\
		{\
		    OR1200_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[i][j],"cabac context");\
			context[map_context_idx(count)]=ctx[i][j];\
			count++;\
		}\
	} \
	}
	#define BIARI_CTX_INIT1(jj,ctx,tab,num) \
	{ \
	  for (j=0; j<jj; j++) \
	  { \
	  if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[j]), &(tab ## _I[num][0][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[j]), &(tab ## _P[num][0][j][0])); \
		if (tab[j])\
		{\
		    OR1200_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[j],"cabac context");\
			context[map_context_idx(count)]=ctx[j];\
			count++;\
		}\
	  } \
	}
#else
	#define BIARI_CTX_INIT3(ii,jj,ctx,tab,num) \
	{ \
		for (i=0; i<ii; i++) \
		for (j=0; j<jj; j++) \
	{ \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][tab ## _MAP[i]][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][tab ## _MAP[i]][j][0])); \
		if (tab[i][j])\
	{\
		context[map_context_idx(count)]=ctx[i][j];\
		count++;\
	}\
	} \
	}
	#define BIARI_CTX_INIT2(ii,jj,ctx,tab,num) \
	{ \
		for (i=0; i<ii; i++) \
		for (j=0; j<jj; j++) \
	{ \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][i][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][i][j][0])); \
		if (tab[i][j])\
	{\
		context[map_context_idx(count)]=ctx[i][j];\
		count++;\
	}\
	} \
	}
	#define BIARI_CTX_INIT1(jj,ctx,tab,num) \
	{ \
		for (j=0; j<jj; j++) \
	{ \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[j]), &(tab ## _I[num][0][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[j]), &(tab ## _P[num][0][j][0])); \
		if (tab[j])\
	{\
		context[map_context_idx(count)]=ctx[j];\
		count++;\
	}\
	} \
	}
#endif
unsigned short list_18_29[12]={24,25,26,27,28,29,21,22,23,18,19,20};
unsigned short list_37_43[7]={41,42,43,37,38,39,40};
unsigned short list_58_69[12]={64,65,66,271,272,273,62,63,58,59,60,61};
unsigned short list_253_307[55]={231,232,233,234,235,298,299,300,301,302,241,242,243,244,245,251,252,253,
254,255,260,261,262,263,264,226,227,228,229,230,236,237,238,239,240,303,
304,305,306,307,246,247,248,249,250,256,257,258,259,265,266,267,268,269,
270};
int map_context_idx(int idx)//weihu
{
	int tmp;
	
	if(idx<70)
	{
		if(idx>=58)
            tmp=list_58_69[idx-58];
		else if((idx>=18)&&(idx<=29))
			tmp=list_18_29[idx-18];
		else if((idx>=37)&&(idx<=43))
			tmp=list_37_43[idx-37];
		else
			tmp=idx;

	}
	else if(idx>=253)
		 tmp=list_253_307[idx-253];
	else if(idx<=130)
		 tmp=idx-3;
	else if(idx>=216)
		 tmp=idx-27;
	else if((idx>=146)&&(idx<=206))
		 tmp=idx-18;
	else if(idx<146)
		 tmp=idx+143;
	else
         tmp=idx+82;
	return tmp;
}
#if SIM_IN_WIN
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
	uint32 *huff_tab = g_hvld_huff_tab+69;//for funny//weihu
#endif

	//cfg coded_block_flag context
	{
		uint32 i, tbl_idx;
		//uint8 map_tbl[5] = {0, 1, 4, 5, 6};//jzy

		bfr_ptr = (uint32 )cabac_bfr_ptr;

		ctx = &(tc->bcbp_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = i;//map_tbl[i];//jzy
			cmd = ( ( ((ctx[tbl_idx*4+3]&0x1)<<6) | (ctx[tbl_idx*4+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*4+2]&0x1)<<6) | (ctx[tbl_idx*4+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*4+1]&0x1)<<6) | (ctx[tbl_idx*4+1]>>1) ) << 8 ) |
				  ( ((ctx[tbl_idx*4+0]&0x1)<<6)   | (ctx[tbl_idx*4+0]>>1) )	;	
			WRITE_VLD_CABAC_BFR(bfr_ptr+i*4, cmd, "HUFFMAN_TBL_ADDR: configure vlc table");
			*huff_tab++ = cmd;
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
#ifdef LUMA_8x8_CABAC
		uint32 LAST_SIG_CAT5_ADDR	= (LAST_SIG_CAT4_ADDR+8*4);
#endif

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
			*huff_tab++ = cmd;
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
			*huff_tab++ = cmd;
		}
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT1_ADDR+(15>>1)*4, cmd, "LAST_SIG_CAT1_ADDR: RESV");
		*huff_tab++ = cmd;

		//block ctx 2 (luma-4x4)
		lst_ctx = &(tc->last_contexts[3][0]); //5
		sig_ctx = &(tc->map_contexts[3][0]); //5
		for (i = 0; i < 15; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT2_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT2_ADDR: luma-4x4");
			*huff_tab++ = cmd;
		}

		//block ctx 3 (chroma-DC)
		lst_ctx = &(tc->last_contexts[4][0]); //6
		sig_ctx = &(tc->map_contexts[4][0]); //6
		for (i = 0; i < 3; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT3_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT3_ADDR: chroma-DC");
			*huff_tab++ = cmd;
		}

		//block ctx 4 (chroma-AC)
		lst_ctx = &(tc->last_contexts[5][0]); //7
		sig_ctx = &(tc->map_contexts[5][0]); //7
		for (i = 1; i < 15; i+=2)
		{
			cmd = ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(i>>1)*4, cmd, "LAST_SIG_CAT4_ADDR: chroma-AC");
			*huff_tab++ = cmd;
		}
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(15>>1)*4, cmd, "LAST_SIG_CAT4_ADDR: RESV");
		WRITE_VLD_CABAC_BFR(LAST_SIG_CAT4_ADDR+(16>>1)*4, cmd, "RESV0");
	
#ifdef LUMA_8x8_CABAC//这里还是将两个ctx分开放算了
		//block ctx 5 (luma-8x8)
		lst_ctx = &(tc->last_contexts[2][0]); 
		sig_ctx = &(tc->map_contexts[2][0]); 
		for (i = 0; i < 15; i+=4)
		{
			cmd = ( ( ((sig_ctx[i+3]&0x1)<<6) | (sig_ctx[i+3]>>1) ) << 24) |
				  ( ( ((sig_ctx[i+2]&0x1)<<6) | (sig_ctx[i+2]>>1) ) << 16) |
				  ( ( ((sig_ctx[i+1]&0x1)<<6) | (sig_ctx[i+1]>>1) ) <<  8) |
				  ( ( ((sig_ctx[i+0]&0x1)<<6) | (sig_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT5_ADDR+(i>>2)*4, cmd, "LAST_SIG_CAT5_ADDR: luma-8x8");
			*huff_tab++ = cmd;
		}

		for (i = 0; i < 9; i+=4)
		{
			cmd = ( ( ((lst_ctx[i+3]&0x1)<<6) | (lst_ctx[i+3]>>1) ) << 24) |
				  ( ( ((lst_ctx[i+2]&0x1)<<6) | (lst_ctx[i+2]>>1) ) << 16) |
				  ( ( ((lst_ctx[i+1]&0x1)<<6) | (lst_ctx[i+1]>>1) ) <<  8) |
				  ( ( ((lst_ctx[i+0]&0x1)<<6) | (lst_ctx[i+0]>>1) ) <<  0);
			WRITE_VLD_CABAC_BFR(LAST_SIG_CAT5_ADDR+16+(i>>2)*4, cmd, "LAST_SIG_CAT5_ADDR: luma-8x8");
			*huff_tab++ = cmd;
		}
#endif	
	}
}
#endif

uint32 g_vld_cabac_offset = 0;
static int INIT_ONE_MAP[]={0,1,2,4,5,6};
static int INIT_ABS_MAP[]={0,1,2,4,5,6};
static int INIT_LAST_MAP[]={0,1,2,5,6,7};
static int INIT_MAP_MAP[]={0,1,2,5,6,7};
static int INIT_BCBP_MAP[]={0,1,4,5,6};

void init_contexts (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

	MotionInfoContexts*  mc = curr_slice_ptr->mot_ctx;
	TextureInfoContexts* tc = curr_slice_ptr->tex_ctx;
	int i, j;
	uint32 *cabac_bfr_ptr;

//#if !_CMODEL_
//	g_vld_cabac_offset = (img_ptr->slice_nr * 40);//word
//#endif
	cabac_bfr_ptr = img_ptr->vld_cabac_table_ptr + g_vld_cabac_offset;
	//PRINTF("%d -", img->model_number);

	//--- motion coding contexts ---
	count=0;
	BIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mb_type_contexts,     INIT_MB_TYPE,    img_ptr->model_number);
	BIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    img_ptr->model_number);
	BIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mv_res_contexts,      INIT_MV_RES,     img_ptr->model_number);
	//BIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     img_ptr->model_number);
	BIARI_CTX_INIT2 (1, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     img_ptr->model_number);
	BIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   img_ptr->model_number);
	BIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     img_ptr->model_number);
#ifdef LUMA_8x8_CABAC
    BIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX,  mc->transform_size_contexts,    INIT_TRANSFORM_SIZE,   img_ptr->model_number);
#endif
	
	//--- texture coding contexts ---
	BIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       img_ptr->model_number);
	BIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      img_ptr->model_number);
	BIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       img_ptr->model_number);
	//BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      img_ptr->model_number);
	//BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts,     INIT_MAP,       img_ptr->model_number);
	//BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts,    INIT_LAST,      img_ptr->model_number);
//	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       img_ptr->model_number);
	//BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       img_ptr->model_number);
	//	BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->fld_map_contexts, INIT_FLD_MAP,   img_ptr->model_number);
	//BIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->fld_last_contexts,INIT_FLD_LAST,  img_ptr->model_number);
	BIARI_CTX_INIT3 (5, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      img_ptr->model_number);
	BIARI_CTX_INIT3 (6, NUM_MAP_CTX,  tc->map_contexts,     INIT_MAP,       img_ptr->model_number);
	BIARI_CTX_INIT3 (6, NUM_LAST_CTX, tc->last_contexts,    INIT_LAST,      img_ptr->model_number);
	BIARI_CTX_INIT3 (6, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       img_ptr->model_number);
    BIARI_CTX_INIT3 (6, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       img_ptr->model_number);
    context[270]=0;	
#if SIM_IN_WIN
	OR1200_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+270*4, 0,"cabac context 276");
#endif
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264|0x40,"VSP_MODE");
	g_image_ptr->is_need_init_vsp_hufftab = FALSE;

#if SIM_IN_WIN	
{
	uint32 cmd = 0;
	BiContextType *ctx;

//#if _CMODEL_
//	memset (g_hvld_huff_tab, 0, 69*sizeof(int));//for funny //weihu
//#endif

	write_vld_cabac(img_ptr, cabac_bfr_ptr);

	cmd = (1<<31) | (40<<20)|g_vld_cabac_offset;
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_VDB_CFG_OFFSET, cmd, "HVLD_VDB_CFG:");
	VSP_READ_REG_POLL(VSP_VLD_REG_BASE+HVLD_VDB_CFG_OFFSET, 31, 1, 0, "polling vld fetch done");
	VSP_WRITE_CMD_INFO((VSP_VLD << MID_SHIFT_BIT) | (2<<24) | (((1<<7)|HVLD_VDB_CFG_WOFF)<<8)|HVLD_VDB_CFG_WOFF);
//#if _CMODEL_
//	g_vld_cabac_offset += (40); //word //weihu
//#endif
	
	//cfg bin0
	{
		uint32 i, tbl_idx;
		uint8 map_tbl[5] = {0, 1, 3, 4, 5};//jzy

		ctx = &(tc->one_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*5+3]&0x1)<<6) | (ctx[tbl_idx*5+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*5+2]&0x1)<<6) | (ctx[tbl_idx*5+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*5+1]&0x1)<<6) | (ctx[tbl_idx*5+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*5+0]&0x1)<<6) | (ctx[tbl_idx*5+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BIN0_CAT0: bin0");
		}

		//inc4
		cmd = ( ( ((ctx[0*5+4]&0x1)<<6) | (ctx[0*5+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*5+4]&0x1)<<6) | (ctx[1*5+4]>>1) ) <<  8) |
			  ( ( ((ctx[3*5+4]&0x1)<<6) | (ctx[3*5+4]>>1) ) << 16) |//4
			  ( ( ((ctx[4*5+4]&0x1)<<6) | (ctx[4*5+4]>>1) ) << 24)	;//5	
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT0_3_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT0_3");

		//ctx_bin0_inc4_cat4
		cmd = ( ( ((ctx[5*5+4]&0x1)<<6) | (ctx[5*5+4]>>1) ) << 0 )	;//6	
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT4_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT4");

#ifdef LUMA_8x8_CABAC
		//ctx_bin0_cat5
		cmd = ( ( ((ctx[2*5+3]&0x1)<<6) | (ctx[2*5+3]>>1) ) << 24) |
			  ( ( ((ctx[2*5+2]&0x1)<<6) | (ctx[2*5+2]>>1) ) << 16) |
			  ( ( ((ctx[2*5+1]&0x1)<<6) | (ctx[2*5+1]>>1) ) << 8 ) |
			  ( ( ((ctx[2*5+0]&0x1)<<6) | (ctx[2*5+0]>>1) ) << 0 )	;	
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_CAT5_OFFSET, cmd, "HVLD_CTX_BIN0_CAT5");
		//ctx_bin0_cat5_inc4
		//cmd = ( ( ((ctx[2*5+4]&0x1)<<6) | (ctx[2*5+4]>>1) ) << 24) ;	
		cmd = ( ( ((ctx[2*5+4]&0x1)<<6) | (ctx[2*5+4]>>1) )) ;	//james
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BIN0_INC4_CAT5_OFFSET, cmd, "HVLD_CTX_BIN0_INC4_CAT5");
#endif

		VSP_WRITE_CMD_INFO((VSP_VLD << MID_SHIFT_BIT) | (7<<24) | (HVLD_CTX_BIN0_CAT2_WOFF<<16)|(HVLD_CTX_BIN0_CAT1_WOFF<<8)|HVLD_CTX_BIN0_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BIN0_INC4_CAT4_WOFF<<24)|(HVLD_CTX_BIN0_INC4_CAT0_3_WOFF<<16)|(HVLD_CTX_BIN0_CAT4_WOFF<<8)|HVLD_CTX_BIN0_CAT3_WOFF);
	}

	//cfg binother
	{
		uint32 i, tbl_idx;
		uint8 map_tbl[5] = {0, 1, 3, 4, 5};//jzy

		ctx = &(tc->abs_contexts[0][0]);
		for (i = 0; i < 5; i++) //5 is block category num
		{
			tbl_idx = map_tbl[i];
			cmd = ( ( ((ctx[tbl_idx*5+3]&0x1)<<6) | (ctx[tbl_idx*5+3]>>1) ) << 24) |
				  ( ( ((ctx[tbl_idx*5+2]&0x1)<<6) | (ctx[tbl_idx*5+2]>>1) ) << 16) |
				  ( ( ((ctx[tbl_idx*5+1]&0x1)<<6) | (ctx[tbl_idx*5+1]>>1) ) << 8 ) |
				  ( ( ((ctx[tbl_idx*5+0]&0x1)<<6) | (ctx[tbl_idx*5+0]>>1) ) << 0 )	;	
			VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_CAT0_OFFSET+i*4, cmd, "HVLD_CTX_BINOTH_CAT0");
		}

		//inc4
		cmd = ( ( ((ctx[0*5+4]&0x1)<<6) | (ctx[0*5+4]>>1) ) <<  0) |
			  ( ( ((ctx[1*5+4]&0x1)<<6) | (ctx[1*5+4]>>1) ) <<  8) |
			  ( ( ((ctx[3*5+4]&0x1)<<6) | (ctx[3*5+4]>>1) ) << 16) |//4
			  ( ( ((ctx[5*5+4]&0x1)<<6) | (ctx[5*5+4]>>1) ) << 24)	;//6	
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_INC4_OFFSET, cmd, "HVLD_CTX_BINOTH_INC4");

#ifdef LUMA_8x8_CABAC
		//ctx_binoth_cat5
		cmd = ( ( ((ctx[2*5+3]&0x1)<<6) | (ctx[2*5+3]>>1) ) << 24) |
			  ( ( ((ctx[2*5+2]&0x1)<<6) | (ctx[2*5+2]>>1) ) << 16) |
			  ( ( ((ctx[2*5+1]&0x1)<<6) | (ctx[2*5+1]>>1) ) << 8 ) |
			  ( ( ((ctx[2*5+0]&0x1)<<6) | (ctx[2*5+0]>>1) ) << 0 )	;	
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_CAT5_OFFSET, cmd, "HVLD_CTX_BINOTH_CAT5");
		//ctx_binoth_cat5_inc4
		//cmd = ( ( ((ctx[2*5+4]&0x1)<<6) | (ctx[2*5+4]>>1) ) << 24) ;	
		cmd = ( ( ((ctx[2*5+4]&0x1)<<6) | (ctx[2*5+4]>>1) )) ;	//james
		VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTX_BINOTH_INC4_CAT5_OFFSET, cmd, "HVLD_CTX_BINOTH_INC4_CAT5");
#endif
		
		VSP_WRITE_CMD_INFO((VSP_VLD << MID_SHIFT_BIT) | (6<<24) | (HVLD_CTX_BINOTH_CAT2_WOFF<<16)|(HVLD_CTX_BINOTH_CAT1_WOFF<<8)|HVLD_CTX_BINOTH_CAT0_WOFF);
		VSP_WRITE_CMD_INFO((HVLD_CTX_BINOTH_INC4_WOFF<<16)|(HVLD_CTX_BINOTH_CAT4_WOFF<<8)|HVLD_CTX_BINOTH_CAT3_WOFF);
	}
	
		
		for (i = 0; i < 308; i++)
		{		
			CONTEXT_FPRINTF(g_fp_context_tv,"%02x\n",context[i]&0x7f);							
		}
		//CONTEXT_FPRINTF(g_fp_context_tv,"%02x\n",0);
		

}
#endif//SIM_IN_WIN

}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
