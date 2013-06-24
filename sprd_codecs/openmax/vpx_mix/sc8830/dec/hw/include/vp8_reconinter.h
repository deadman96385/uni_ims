

#ifndef VP8_RECONINTER_H
#define VP8_RECONINTER_H

void vp8_build_inter_predictors_mb(MACROBLOCKD *x);
void vp8_build_uvmvs(MACROBLOCKD *x, int fullpixel);
void vp8_build_inter_predictors_mb_s(MACROBLOCKD *x);

#endif // VP8_RECONINTER_H