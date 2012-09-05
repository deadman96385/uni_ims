/******************************************************************************
 ** File Name:      h264Dec_vld.h                                              *
 ** Author:         jiang.lin                                                *
 ** DATE:           06/01/2006                                                *
 ** Copyright:      2006 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                      *
 ******************************************************************************/

#ifndef _H264DEC_VLD_H_
#define _H264DEC_VLD_H_

#include "h264dec_basic.h"
#include "h264dec_mode.h"

#define LUMA_DC_BLOCK_INDEX	25
#define CHROMA_DC_BLOCK_INDEX		26

#define CHROMA_DC_COEFF_TOKEN_VLC_BITS	8
#define COEFF_TOKEN_VLC_BITS				8
#define TOTAL_ZEROS_VLC_BITS				9
#define CHROMA_DC_TOTAL_ZEROS_VLC_BITS	3
#define RUN_VLC_BITS						3
#define RUN7_VLC_BITS						6

extern SHORT_VLC_SYMBOL_T coeff_token_vlc[4];
extern BYTE_VLC_SYMBOL_T chroma_dc_coeff_token_vlc;

extern BYTE_VLC_SYMBOL_T total_zeros_vlc[15];
extern BYTE_VLC_SYMBOL_T chroma_dc_total_zeros_vlc[3];

extern BYTE_VLC_SYMBOL_T run_vlc[6];
extern BYTE_VLC_SYMBOL_T run7_vlc;

void H264Dec_init_vld_table (void);
int get_level_prefix (DEC_BS_T *stream);
int pred_non_zero_count (DEC_MB_CACHE_T *mb_cache_ptr, int blkIndex);

int32 get_cabac_cbf_ctx(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T * mb_cache_ptr, int cat,  int blk_id );

void decode_mb_cavlc_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

void decode_mb_cabac_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void decode_mb_cavlc_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

#endif
