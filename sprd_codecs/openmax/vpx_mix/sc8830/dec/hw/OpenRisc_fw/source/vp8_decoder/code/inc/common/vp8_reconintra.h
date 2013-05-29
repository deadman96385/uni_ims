#ifndef VP8_RECONINTRA_H
#define VP8_RECONINTRA_H

typedef void (*vp8_build_intra_predictors_mby_ptr)(MACROBLOCKD *x);
typedef void (*vp8_build_intra_predictors_mby_s_ptr)(MACROBLOCKD *x);

void vp8_recon_intra_mbuv(MACROBLOCKD *x);
void vp8_build_intra_predictors_mby_s(MACROBLOCKD *x);
void vp8_build_intra_predictors_mby(MACROBLOCKD *x);
void vp8_build_intra_predictors_mbuv(MACROBLOCKD *x);
void vp8_build_intra_predictors_mbuv_s(MACROBLOCKD *x);//weihu

#endif //VP8_RECONINTRA_H