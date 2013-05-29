
#ifndef VP8DEC_DEQUANT_H
#define VP8DEC_DEQUANT_H

#include "vp8_blockd.h"
#include "vp8dec_mode.h"

void vp8cx_init_de_quantizer(VP8D_COMP *pbi);
void de_quantand_idct(VP8D_COMP *pbi, MACROBLOCKD *xd);
void mb_init_dequantizer(VP8D_COMP *pbi, MACROBLOCKD *xd);

#endif //VP8DEC_DEQUANT_H