/******************************************************************************
 ** File Name:      vld_global.h                                              *
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
#ifndef _VLD_GLOBAL_H_
#define _VLD_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern FILE * g_mpeg4dec_vld_trace_fp;  //asic order, intra is after inverse ac_dc prediction
extern FILE * g_mpegdec_vld_no_acdc_fp; //normal order, intra is before ac_dc prediction
extern FILE * g_pfRunLevel_mpeg4dec;

extern int16 g_pre_dc_value[3];				/*store pre block value for DPCM coding*/

void Mp4Dec_VldTraceInit ();
void FprintfOneBlock (int16 * dct_coef_blk_ptr, FILE * file_ptr);
void printf_codeWordInfo (int codeWord, int codeLen, FILE * pfHuffVal);
int mpeg4dec_vld (
				   int		standard,				// mpeg4 or h.263
				   int		is_rvlc,				//is rvlc or not
				   int		mb_type,				//intra or inter
				   int		cbp,					//only for inter mb
				   int		start_position,			//0 or 1
				   int		zigzag_mode,			//standard, horizontal, vertical
				   int		blk_id,					//for storing address in dct/io buffer, only for intra mb
				   int		rotation_ena
				   );
void mpeg4dec_acdc_pred (
						 int standard,
						 int is_rvlc,
						 int mb_type,
						 int cbp,
						 int rotation_ena
						 );
int RvlcOneBlock (
				  int		is_intra,
				  int		zigzag_mode,
				  int		start_pos,
				  int		blk_id
				  );

#if defined(JPEG_DEC)
void jpeg_vld_Block(
					int16 * pBlk_asic,
					HUFF_TBL_T 	*dc_tbl,
					HUFF_TBL_T 	*ac_tbl,
					const uint8 *quant,
					int blkId,
					int16 DCPred,
					int8 bChroma
					);
#endif

int32 InPutRstMarker(void);
void blk_to_asicOrder(int16 * pblock, int16 * pblk_asic, uint32 * pNZFlag);
void configure_noneZeroFlag_blk (uint32 * pNZFlag, uint32 * pNoneZeroFlag);

PUBLIC void vld_module();
void init_vld();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VLD_GLOBAL_H_