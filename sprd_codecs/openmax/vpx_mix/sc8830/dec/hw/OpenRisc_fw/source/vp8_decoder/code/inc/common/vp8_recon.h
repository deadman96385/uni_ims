
#ifndef VP8_RECON_H
#define VP8_RECON_H

void vp8_recon_b_c
(
    unsigned char *pred_ptr,
    short *diff_ptr,
    unsigned char *dst_ptr,
    int stride
);

void vp8_recon2b_c
(
    unsigned char *pred_ptr,
    short *diff_ptr,
    unsigned char *dst_ptr,
    int stride
);

void vp8_recon16x16mb(MACROBLOCKD *x);

#endif //VP8_RECON_H