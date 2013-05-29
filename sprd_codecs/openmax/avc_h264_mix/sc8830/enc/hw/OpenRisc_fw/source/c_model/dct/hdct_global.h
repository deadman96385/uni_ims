#ifndef _HDCT_GLOBAL_H_
#define _HDCT_GLOBAL_H_

#include "sci_types.h"

void trans4x4(int16 *src_ptr, int32 src_width, int16 *dst_ptr, int32 dst_width);
void quant4x4 (int16 *dct4x4, int32 qp_per, int32 qp_rem, int32 b_intra);
void zigzag4x4(int16 *src4x4, int16 *dst4x4);
void dequant4x4(int16 dct[16], int32 qp_per, int32 qp_rem);
void dct4x4dc(int16 dc[16]);
void quant4x4dc(int16 dc[16], int32 qp_per, int32 qp_rem);
void zigzag4x4_full(int16 *src4x4, int16 *dst4x4);
void idct4x4dc(int16 dc[16]);
void dequant4x4dc(int16 dc[16], int32 qp_per, int32 qp_rem);
void idct4x4(int16 *src_ptr, int32 src_width, int16 *dst_ptr, int32 dst_width);
uint8 get_nnz4x4(int16 *vlc_coef_ptr, int32 start_pos);
void dct2x2dc(int16 dc[4]);
void quant2x2dc(int16 dc[4], int32 qp_per, int32 qp_rem, int32 b_intra);
void zigzag2x2_dc(int16 *src2x2, int16 *dst2x2);
void dequant2x2dc(int16 dc[4], int32 qp_per, int32 qp_rem);

void hdct_module();

#endif //_HDCT_GLOBAL_H_