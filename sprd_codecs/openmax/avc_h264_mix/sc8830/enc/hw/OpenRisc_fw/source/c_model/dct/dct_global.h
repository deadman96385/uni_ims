/******************************************************************************
 ** File Name:      dct_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _DCT_GLOBAL_H_
#define _DCT_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
//#include "vsp_global_defines.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern const uint8 g_ASIC_DCT_Matrix[64];

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void get_normal_order_coeffBlk (int32 * pDCTIOBuf, int iblk, int16 *pBlk_dst);
PUBLIC void blk_asic_normal_change (int16 * pBlk_src, int16 *pBlk_dst);
void Mp4Enc_Asic_Dct_Intra(int8 *pSrc, int16 *pDst,int32 DCScaler);
void JpegEnc_Asic_Dct_Intra(int8 *pSrc, int16 *pDst,int32 DCScaler);
void Mp4Enc_Asic_Dct_Inter(int16 *pSrc, int16 *pDst);
void quant(int16 *pDct, int32 Inter_mode_en, int32 standard, int32 blk_idx);
void iquant(int16 *pDct, int32 Inter_mode_en, int32 blk_idx);
void Mp4_Asic_idct1(int16 *pBlk);

void dct_module();

void printf_DCTCoeff_block (FILE *pf, int16 * pBlock);
void printf_mb_idct(FILE *pfIDCT, int32 *pDctIOBuf, int cbp);
void printf_nonezeroFlag_enc (int32 blk_num);
void printf_mb_coeff_flag (int32 * pDCTIOBuf);

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/**---------------------------------------------------------------------------*/
// End 
#endif  //_DCT_GLOBAL_H_