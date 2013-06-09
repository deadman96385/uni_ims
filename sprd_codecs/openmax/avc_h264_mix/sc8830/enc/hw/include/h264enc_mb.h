#ifndef _H264ENC_MACROBLOCK_H_
#define _H264ENC_MACROBLOCK_H_

#include "h264enc_mode.h"

void H264Enc_start_macroblock (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info, ENC_MB_CACHE_T *mb_cache);
void ConfigureMeaFetch (int mb_y, int mb_x, int is_i_frame);
void H264Enc_AnalyseStart( ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void h264enc_macroblock_encode (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void h264enc_block_encode (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr, uint8 blkIdx);
void h264enc_macroblock_write_cavlc (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void H264Enc_start_iqt_macroblock (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void H264Enc_CheckMBCStatus (ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void h264enc_macroblock_cache_save(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);

#endif //_H264ENC_MACROBLOCK_H_