#include "h264enc_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

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

#ifdef SIM_IN_WIN
#define BIARI_CTX_INIT3(ii,jj,tab,num) \
    { \
        for (i=0; i<ii; i++) \
        for (j=0; j<jj; j++) \
        { \
            if (tab[i][j])\
            {\
                /*OR1200_WRITE_REG(ORSC_CABAC_CTX_OFF+map_context_idx(count)*4, ctx[i][j],"cabac context");*/\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context(img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][tab ## _MAP[i]][j][0]));\
                else biari_init_context(img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][tab ## _MAP[i]][j][0]));\
                count++;\
            }\
        } \
    }
#define BIARI_CTX_INIT2(ii,jj,tab,num) \
    { \
        for (i=0; i<ii; i++) \
        for (j=0; j<jj; j++) \
        { \
            if (tab[i][j])\
            {\
                /*OR1200_WRITE_REG(ORSC_CABAC_CTX_OFF+map_context_idx(count)*4, ctx[i][j],"cabac context");*/\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context (img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][i][j][0]));\
                else biari_init_context (img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][i][j][0]));\
                count++;\
            }\
        } \
    }
#define BIARI_CTX_INIT1(jj,tab,num) \
    { \
        for (j=0; j<jj; j++) \
        { \
            if (tab[j])\
            {\
                /*OR1200_WRITE_REG(ORSC_CABAC_CTX_OFF+map_context_idx(count)*4, ctx[j],"cabac context");*/\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context (img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][0][j][0]));\
                else biari_init_context (img_ptr, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][0][j][0]));\
                count++;\
            }\
        } \
    }
#else
#define BIARI_CTX_INIT3(ii,jj,tab,num) \
    { \
        for (i=0; i<ii; i++) \
        for (j=0; j<jj; j++) \
        { \
            if (tab[i][j])\
            {\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context(vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][tab ## _MAP[i]][j][0]));\
                else biari_init_context(vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][tab ## _MAP[i]][j][0]));\
                count++;\
            }\
        } \
    }
#define BIARI_CTX_INIT2(ii,jj,tab,num) \
    { \
        for (i=0; i<ii; i++) \
        for (j=0; j<jj; j++) \
        { \
            if (tab[i][j])\
            {\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context (vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][i][j][0]));\
                else biari_init_context (vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][i][j][0]));\
                count++;\
            }\
        } \
    }
#define BIARI_CTX_INIT1(jj,tab,num) \
    { \
        for (j=0; j<jj; j++) \
        { \
            if (tab[j])\
            {\
                if (img_ptr->sh.i_type==SLICE_TYPE_I) biari_init_context (vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _I[num][0][j][0]));\
                else biari_init_context (vo, &(img_ptr->context[map_context_idx(count)]), &(tab ## _P[num][0][j][0]));\
                count++;\
            }\
        } \
    }
#endif

const int INIT_ONE_MAP[]= {0,1,2,4,5,6};
const int INIT_ABS_MAP[]= {0,1,2,4,5,6};
const int INIT_LAST_MAP[]= {0,1,2,5,6,7};
const int INIT_MAP_MAP[]= {0,1,2,5,6,7};
const int INIT_BCBP_MAP[]= {0,1,4,5,6};

/*!
 ************************************************************************
 * \brief
 *    Initializes a given context with some pre-defined probability state
 ************************************************************************
 */

#ifdef SIM_IN_WIN
void biari_init_context(ENC_IMAGE_PARAMS_T *img_ptr, BiContextType *ctx, const int* ini)
#else
void biari_init_context(H264EncObject *vo, int32 *ctx, const int32* ini)
#endif // SIM_IN_WIN
{
    int pstate;
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;

    SPRD_CODEC_LOGD ("%s, %d, img_ptr=%x, ini=%x, ctx=%x \n", __FUNCTION__, __LINE__, img_ptr, ini, ctx);

    pstate = ((ini[0]*img_ptr->qp)>>4) + ini[1];
    pstate = mmin (mmax ( 1, pstate), 126);

    SPRD_CODEC_LOGD ("%s, %d, pstate=%d.\n", __FUNCTION__, __LINE__, pstate);

    if( pstate <= 63 )
    {
        //*ctx = 2 * ( 63 - pstate ) + 0;
        VSP_WRITE_REG(ctx, 2 * ( 63 - pstate ) + 0,"axim endian set, vu format"); //VSP and OR endian.
    }
    else
    {
        //*ctx = 2 * ( pstate - 64 ) + 1;
        VSP_WRITE_REG(ctx, 2 * ( pstate - 64 ) + 1,"axim endian set, vu format"); //VSP and OR endian.
    }
    SPRD_CODEC_LOGD ("%s, %d, pstate=%d.\n", __FUNCTION__, __LINE__, pstate);
}

void init_contexts (H264EncObject *vo)
{
    int i=0, j=0;
    int count=0;
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;

    SPRD_CODEC_LOGD ("%s, %d.\n", __FUNCTION__, __LINE__);

    BIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,     INIT_MB_TYPE,    img_ptr->model_number);

    SPRD_CODEC_LOGD ("%s, %d.\n", __FUNCTION__, __LINE__);
    BIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,     INIT_B8_TYPE,    img_ptr->model_number);
    BIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,      INIT_MV_RES,     img_ptr->model_number);
    BIARI_CTX_INIT2 (1, NUM_REF_NO_CTX,      INIT_REF_NO,     img_ptr->model_number);
    BIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,    INIT_DELTA_QP,   img_ptr->model_number);
    BIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,      INIT_MB_AFF,     img_ptr->model_number);

    SPRD_CODEC_LOGD ("%s, %d.\n", __FUNCTION__, __LINE__);
#ifdef LUMA_8x8_CABAC
    BIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX,    INIT_TRANSFORM_SIZE,   img_ptr->model_number);
#endif

    SPRD_CODEC_LOGD ("%s, %d.\n", __FUNCTION__, __LINE__);

    //--- texture coding contexts ---
    BIARI_CTX_INIT1 (                 NUM_IPR_CTX,     INIT_IPR,       img_ptr->model_number);
    BIARI_CTX_INIT1 (                 NUM_CIPR_CTX,    INIT_CIPR,      img_ptr->model_number);
    BIARI_CTX_INIT2 (3,               NUM_CBP_CTX,     INIT_CBP,       img_ptr->model_number);
    BIARI_CTX_INIT3 (5, NUM_BCBP_CTX,    INIT_BCBP,      img_ptr->model_number);
    BIARI_CTX_INIT3 (6, NUM_MAP_CTX,     INIT_MAP,       img_ptr->model_number);
    BIARI_CTX_INIT3 (6, NUM_LAST_CTX,    INIT_LAST,      img_ptr->model_number);
    BIARI_CTX_INIT3 (6, NUM_ONE_CTX,     INIT_ONE,       img_ptr->model_number);
    BIARI_CTX_INIT3 (6, NUM_ABS_CTX,     INIT_ABS,       img_ptr->model_number);

    i = 270;
    VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+ (i << 2),0, "ORSC: CABAC Context");
    /*
        img_ptr->context[270]=0;
        for (i = 0; i < 308; i++)
        {

            //    OR1200_WRITE_REG(ORSC_CABAC_CTX_OFF + (i << 2), 0x7f&img_ptr->context[i], "ORSC: CABAC Context");
            VSP_WRITE_REG(CABAC_CONTEXT_BASE_ADDR+ (i << 2), 0x7f&img_ptr->context[i], "ORSC: CABAC Context");
        }

        j = 4;
    */
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
