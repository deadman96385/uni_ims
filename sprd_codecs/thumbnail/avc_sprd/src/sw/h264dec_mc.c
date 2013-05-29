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
#include "sc8810_video_header.h"

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

#define CLIP_MV(img_ptr, mvx, mvy)\
{\
	if (mvx > img_ptr->mv_x_max)\
	{\
		mvx = img_ptr->mv_x_max;\
	}\
	else if (mvx < img_ptr->mv_x_min)\
	{\
		mvx = img_ptr->mv_x_min;\
	}\
	\
	if (mvy > img_ptr->mv_y_max)\
	{\
		mvy = img_ptr->mv_y_max;\
	}\
	else if (mvy < img_ptr->mv_y_min)\
	{\
		mvy= img_ptr->mv_y_min;\
	}\
}

void H264Dec_mc_16x16 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y)
{
	int32 dx, dy;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
		
	CLIP_MV (img_ptr, mv_x, mv_y);
		
	img_ptr->g_refPosx = img_ptr->xpos + mv_x;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y;

	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;
	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
//    LOGI("%s, %d", __FUNCTION__, __LINE__);

	g_MC16xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredMBY, 16);
//LOGI("%s, %d", __FUNCTION__, __LINE__);
	
	//four 4x4 chroma block interpolation
	pPredUV[0] = pPredMBU; pPredUV[1] = pPredMBV; 
//LOGI("%s, %d", __FUNCTION__, __LINE__);

	g_MC_chroma8xN (img_ptr, pRefFrame, pPredUV, 8);
//LOGI("%s, %d", __FUNCTION__, __LINE__);

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

void H264Dec_mc_16x8 (H264DecContext *img_ptr, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b16x8)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
		
	CLIP_MV (img_ptr, mv_x, mv_y);
			
	img_ptr->g_refPosx = img_ptr->xpos + mv_x;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y + b16x8 * 32/*8 * 4*/;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
	pPredY = pPredMBY + b16x8 * MB_SIZE_X8;
 //       LOGI("%s, %d, pPredUV: %0x, pPredUV [0]=%0x,  pPredUV [1]=%0x", __FUNCTION__, __LINE__, pPredUV,pPredMBU + b16x8*MB_CHROMA_SIZE_X4, pPredMBV + b16x8*MB_CHROMA_SIZE_X4);

 //  		LOGI("%s, %d, dx:%d, dy:%d", __FUNCTION__, __LINE__, dx, dy);
	g_MC16xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 8);
	
	pPredUV [0] = pPredMBU + b16x8*MB_CHROMA_SIZE_X4; pPredUV [1] = pPredMBV + b16x8*MB_CHROMA_SIZE_X4; 
//    		LOGI("%s, %d, pPredUV: %0x, pPredUV [0]=%0x,  pPredUV [1]=%0x", __FUNCTION__, __LINE__, pPredUV, pPredUV [0], pPredUV [1]);
	g_MC_chroma8xN (img_ptr, pRefFrame, pPredUV, 4);
//    		LOGI("%s, %d", __FUNCTION__, __LINE__);

}

void H264Dec_mc_8x16 (H264DecContext *img_ptr, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8x16)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;

	CLIP_MV (img_ptr, mv_x, mv_y);
								
	img_ptr->g_refPosx = img_ptr->xpos + mv_x + b8x16 * 32/*8 * 4*/;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
	pPredY = pPredMBY + (b8x16<<3)/**8*/;
//    		LOGI("%s, %d, dx=%d, dy=%d", __FUNCTION__, __LINE__, dx, dy);

	g_MC8xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 16);
//		LOGI("%s, %d", __FUNCTION__, __LINE__);

	/*two 4x4 chroma mc*/
	pPredUV [0] = pPredMBU + b8x16*4; pPredUV [1] = pPredMBV + b8x16*4; 
//		LOGI("%s, %d", __FUNCTION__, __LINE__);
	g_MC_chroma4xN (img_ptr, pRefFrame, pPredUV, 8);
//		LOGI("%s, %d", __FUNCTION__, __LINE__);
}

void H264Dec_mc_8x8 (H264DecContext *img_ptr, uint8 * pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	int32 offset = s_chroma_blk2x2_addr[4*b8];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
				
	CLIP_MV (img_ptr, mv_x, mv_y);
								
	img_ptr->g_refPosx = img_ptr->xpos + mv_x + (b8 & 0x1) * 32;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y + (b8 >> 1) * 32/*8 * 4*/;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);			
	pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8];	//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8;
	g_MC8xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 8);

	pPredUV [0] = pPredMBU + offset; pPredUV [1] = pPredMBV + offset; 
	g_MC_chroma4xN (img_ptr, pRefFrame, pPredUV, 4);
}

void H264Dec_mc_8x4 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b8x4)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	int32 offset = s_chroma_blk2x2_addr[4*b8+2*b8x4];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + b8x4 * MB_CHROMA_SIZE_X2;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
				
	CLIP_MV (img_ptr, mv_x, mv_y);
								
	img_ptr->g_refPosx = img_ptr->xpos + mv_x + (b8 & 0x1) * 32;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y + (b8 >> 1) * 32 + b8x4 * 16;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
	pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+2*b8x4];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + b8x4 * MB_SIZE_X4;
	g_MC8xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 4);

	pPredUV [0] = pPredMBU + offset; 
	pPredUV [1] = pPredMBV + offset; 
	g_MC_chroma4xN (img_ptr, pRefFrame, pPredUV, 2);
}

void H264Dec_mc_4x8 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4x8)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	int32 offset = s_chroma_blk2x2_addr[4*b8+b4x8];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + b4x8 * 2;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
			
	CLIP_MV (img_ptr, mv_x, mv_y);
								
	img_ptr->g_refPosx = img_ptr->xpos + mv_x + (b8 & 0x1) * 32 + b4x8 * 16;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y + (b8 >> 1) * 32 ;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
	pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+b4x8];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + b4x8 * 4;
	g_MC4xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 8);

	pPredUV [0] = pPredMBU + offset; 	pPredUV [1] = pPredMBV + offset; 
	g_MC_chroma2xN (img_ptr, pRefFrame, pPredUV, 4);
}

void H264Dec_mc_4x4 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4)
{
	int32 dx, dy;
	uint8 * pPredY;
	uint8 * pPredUV[2];
	uint8 * pPredMBU = pPredMBY + 256;
	uint8 * pPredMBV = pPredMBU + 64;
	int32 offset = s_chroma_blk2x2_addr[4*b8+b4];//(b8 >> 1) * MB_CHROMA_SIZE_X4 + (b8 & 0x1) * 4 + (b4>>1) * MB_CHROMA_SIZE_X2 + (b4 & 0x1) * 2;
	uint8 * pRefBlk;
	int32 ext_width = img_ptr->ext_width;
				
	CLIP_MV (img_ptr, mv_x, mv_y);
				
	img_ptr->g_refPosx = img_ptr->xpos + mv_x + (b8 & 0x1) * 32 + (b4 & 0x1) * 16;
	img_ptr->g_refPosy = img_ptr->ypos + mv_y + (b8 >> 1) * 32  + (b4 >> 1) * 16;			
	dx = img_ptr->g_refPosx & 0x03;
	dy = img_ptr->g_refPosy & 0x03;

	pRefBlk = pRefFrame[0] + (img_ptr->g_refPosy >> 2) * ext_width + (img_ptr->g_refPosx >> 2);
	pPredY = pPredMBY + s_luma_blk4x4_addr[4*b8+b4];//(b8 >> 1) * MB_SIZE_X8 + (b8 & 0x1) * 8 + (b4 >> 1) * MB_SIZE_X4 + (b4 & 0x1) * 4;
	g_MC4xN_luma[(dx<<2) | dy] (img_ptr, pRefBlk, pPredY, 4);

	pPredUV [0] = pPredMBU + offset; 	pPredUV [1] = pPredMBV + offset; 
	g_MC_chroma2xN (img_ptr, pRefFrame, pPredUV, 2);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 


