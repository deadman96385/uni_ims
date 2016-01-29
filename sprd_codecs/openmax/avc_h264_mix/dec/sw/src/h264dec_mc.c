/******************************************************************************
 ** File Name:    h264dec_mc8x8.c		                                      *
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
#include "h264dec_video_header.h"

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

#define CLIP_MV(vo, mvx, mvy)\
{\
	if (mvx > vo->mv_x_max)\
	{\
		mvx = vo->mv_x_max;\
	}\
	else if (mvx < vo->mv_x_min)\
	{\
		mvx = vo->mv_x_min;\
	}\
	\
	if (mvy > vo->mv_y_max)\
	{\
		mvy = vo->mv_y_max;\
	}\
	else if (mvy < vo->mv_y_min)\
	{\
		mvy= vo->mv_y_min;\
	}\
}

void H264Dec_mc_16x16 (H264DecContext *vo, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y)
{
    int32 dx, dy;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x;
    vo->g_refPosy = vo->ypos + mv_y;

    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;
    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    g_MC16xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredMBY, 16);

    //four 4x4 chroma block interpolation
    pPredUV[0] = pPredMBU;
    pPredUV[1] = pPredMBV;
    g_MC_chroma8xN (vo, pRefFrame, pPredUV, 8);
}

int32 s_luma_blk4x4_addr[16] =
{
    0, 		4, 			4*16,		4*16+4,
    8,		12,			4*16+8,		4*16+12,
    8*16,	8*16+4,		12*16,		12*16+4,
    8*16+8,	8*16+12,	12*16+8,	12*16+12,
};

int32 s_chroma_blk2x2_addr[16] =
{
    0,		2,			2*8,			2*8+2,
    4,		6,			2*8+4,		2*8+6,
    4*8+0,	4*8+2,		6*8+0,		6*8+2,
    4*8+4,	4*8+6,		6*8+4,		6*8+6,
};

void H264Dec_mc_16x8 (H264DecContext *vo, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b16x8)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x;
    vo->g_refPosy = vo->ypos + mv_y + b16x8 * 32/*8 * 4*/;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + b16x8 * MB_SIZE_X8;
    g_MC16xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 8);

    pPredUV [0] = pPredMBU + b16x8*MB_CHROMA_SIZE_X4;
    pPredUV [1] = pPredMBV + b16x8*MB_CHROMA_SIZE_X4;
    g_MC_chroma8xN (vo, pRefFrame, pPredUV, 4);
}

void H264Dec_mc_8x16 (H264DecContext *vo, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8x16)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x + b8x16 * 32/*8 * 4*/;
    vo->g_refPosy = vo->ypos + mv_y;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + (b8x16<<3)/**8*/;
    g_MC8xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 16);

    /*two 4x4 chroma mc*/
    pPredUV [0] = pPredMBU + b8x16*4;
    pPredUV [1] = pPredMBV + b8x16*4;
    g_MC_chroma4xN (vo, pRefFrame, pPredUV, 8);
}

void H264Dec_mc_8x8 (H264DecContext *vo, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    int32 offset = s_chroma_blk2x2_addr[4*b8];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x + (b8 & 0x1) * 32;
    vo->g_refPosy = vo->ypos + mv_y + (b8 >> 1) * 32/*8 * 4*/;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8];	//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8;
    g_MC8xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 8);

    pPredUV [0] = pPredMBU + offset;
    pPredUV [1] = pPredMBV + offset;
    g_MC_chroma4xN (vo, pRefFrame, pPredUV, 4);
}

void H264Dec_mc_8x4 (H264DecContext *vo, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b8x4)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    int32 offset = s_chroma_blk2x2_addr[4*b8+2*b8x4];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + b8x4 * MB_CHROMA_SIZE_X2;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x + (b8 & 0x1) * 32;
    vo->g_refPosy = vo->ypos + mv_y + (b8 >> 1) * 32 + b8x4 * 16;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+2*b8x4];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + b8x4 * MB_SIZE_X4;
    g_MC8xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 4);

    pPredUV [0] = pPredMBU + offset;
    pPredUV [1] = pPredMBV + offset;
    g_MC_chroma4xN (vo, pRefFrame, pPredUV, 2);
}

void H264Dec_mc_4x8 (H264DecContext *vo, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4x8)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    int32 offset = s_chroma_blk2x2_addr[4*b8+b4x8];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + b4x8 * 2;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x + (b8 & 0x1) * 32 + b4x8 * 16;
    vo->g_refPosy = vo->ypos + mv_y + (b8 >> 1) * 32 ;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+b4x8];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + b4x8 * 4;
    g_MC4xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 8);

    pPredUV [0] = pPredMBU + offset;
    pPredUV [1] = pPredMBV + offset;
    g_MC_chroma2xN (vo, pRefFrame, pPredUV, 4);
}

void H264Dec_mc_4x4 (H264DecContext *vo, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4)
{
    int32 dx, dy;
    uint8 * pPredY;
    uint8 * pPredUV[2];
    uint8 * pPredMBU = pPredMBY + 256;
    uint8 * pPredMBV = pPredMBU + 64;
    int32 offset = s_chroma_blk2x2_addr[4*b8+b4];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + (b4>>1) * MB_CHROMA_SIZE_X2 + (b4 & 0x1) * 2;
    uint8 * pRefBlk;
    int32 ext_width = vo->ext_width;

    CLIP_MV (vo, mv_x, mv_y);

    vo->g_refPosx = vo->xpos + mv_x + (b8 & 0x1) * 32 + (b4 & 0x1) * 16;
    vo->g_refPosy = vo->ypos + mv_y + (b8 >> 1) * 32  + (b4 >> 1) * 16;
    dx = vo->g_refPosx & 0x03;
    dy = vo->g_refPosy & 0x03;

    pRefBlk = pRefFrame[0] + (vo->g_refPosy >> 2) * ext_width + (vo->g_refPosx >> 2);
    pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+b4];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + (b4 >> 1) * MB_SIZE_X4 + (b4 & 0x1) * 4;
    g_MC4xN_luma[(dx<<2) | dy] (vo, pRefBlk, pPredY, 4);

    pPredUV [0] = pPredMBU + offset;
    pPredUV [1] = pPredMBV + offset;
    g_MC_chroma2xN (vo, pRefFrame, pPredUV, 2);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End


