
#ifndef VP8_FIND_NEARMV_H
#define VP8_FIND_NEARMV_H

void vp8_find_near_mvs
(
    MACROBLOCKD *xd,
    const MODE_INFO *here,
    MV *nearest, MV *nearby, MV *best,
    int near_mv_ref_cts[4],
    int refframe,
    int *ref_frame_sign_bias
);

vp8_prob *vp8_mv_ref_probs(
    vp8_prob p[VP8_MVREFS-1], const int near_mv_ref_ct[4]
);

void vp8_clamp_mv(MV *mv, const MACROBLOCKD *xd);
const B_MODE_INFO *vp8_left_bmi(const MODE_INFO *cur_mb, int b);

const B_MODE_INFO *vp8_above_bmi(const MODE_INFO *cur_mb, int b, int mi_stride);

#define LEFT_TOP_MARGIN (short)(16 << 3)
#define RIGHT_BOTTOM_MARGIN (short)(16 << 3)

#endif //VP8_FIND_NEARMV_H
