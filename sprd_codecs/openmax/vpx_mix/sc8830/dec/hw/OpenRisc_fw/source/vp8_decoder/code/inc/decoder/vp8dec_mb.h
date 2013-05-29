
#ifndef VP8DEC_MB_H
#define VP8DEC_MB_H

#include "vp8dec_mode.h"
#include "vp8_blockd.h"

void skip_recon_mb(VP8D_COMP *pbi, MACROBLOCKD *xd);
void reconstruct_mb(VP8D_COMP *pbi, MACROBLOCKD *xd);

#endif //VP8DEC_MB_H