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
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

//BiContextType context[308];

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
const int INIT_DELTA_QP[4] = {1,1,1,1};
const int INIT_MB_AFF[3] = {1,1,1};
const int INIT_TRANSFORM_SIZE[3] = {1,1,1};
const int INIT_IPR[2] = {1,1};
const int INIT_CIPR[4] = {1,1,1,1};
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
#define BIARI_CTX_INIT3(ii,jj,ctx,tab,num) \
	{ \
		for (i=0; i<ii; i++) \
		for (j=0; j<jj; j++) \
	{ \
		if      (img_ptr->type==I_SLICE)  biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _I[num][tab ## _MAP[i]][j][0])); \
		else                            biari_init_context (img_ptr, &(ctx[i][j]), &(tab ## _P[num][tab ## _MAP[i]][j][0])); \
		if (tab[i][j])\
	{\
		VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[i][j],"cabac context");\
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
		VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[i][j],"cabac context");\
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
		VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+map_context_idx(count)*4, ctx[j],"cabac context");\
		count++;\
	}\
	} \
	}
unsigned short list_18_29[12]= {24,25,26,27,28,29,21,22,23,18,19,20};
unsigned short list_37_43[7]= {41,42,43,37,38,39,40};
unsigned short list_58_69[12]= {64,65,66,271,272,273,62,63,58,59,60,61};
unsigned short list_253_307[55]= {231,232,233,234,235,298,299,300,301,302,241,242,243,244,245,251,252,253,
                                  254,255,260,261,262,263,264,226,227,228,229,230,236,237,238,239,240,303,
                                  304,305,306,307,246,247,248,249,250,256,257,258,259,265,266,267,268,269,
                                  270
                                 };
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

static int INIT_ONE_MAP[]= {0,1,2,4,5,6};
static int INIT_ABS_MAP[]= {0,1,2,4,5,6};
static int INIT_LAST_MAP[]= {0,1,2,5,6,7};
static int INIT_MAP_MAP[]= {0,1,2,5,6,7};
static int INIT_BCBP_MAP[]= {0,1,4,5,6};

void init_contexts (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

    MotionInfoContexts*  mc = curr_slice_ptr->mot_ctx;
    TextureInfoContexts* tc = curr_slice_ptr->tex_ctx;
    int i, j;

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
    //context[270]=0;
    VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+270*4, 0,"cabac context 276");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264|0x40,"VSP_MODE");
    vo->is_need_init_vsp_hufftab = FALSE;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
