
#ifndef VP8_ENTROPY_MODE_H
#define VP8_ENTROPY_MODE_H

#include "sci_types.h"
#include "vp8_blockd.h"
#include "vp8_basic.h"
#include "vp8_mode.h"

extern vp8_mbsplit vp8_mbsplits [VP8_NUMMBSPLITS];

extern const int vp8_mbsplit_count [VP8_NUMMBSPLITS];    /* # of subsets */

extern const vp8_prob vp8_mbsplit_probs [VP8_NUMMBSPLITS-1];
extern int vp8_mv_cont(const MV *l, const MV *a);

extern const vp8_prob vp8_sub_mv_ref_prob2 [SUBMVREF_COUNT][VP8_SUBMVREFS-1];

extern const unsigned int vp8_kf_default_bmode_counts [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES];

extern const vp8_tree_index vp8_bmode_tree[];

extern const vp8_tree_index  vp8_ymode_tree[];
extern const vp8_tree_index  vp8_kf_ymode_tree[];
extern const vp8_tree_index  vp8_uv_mode_tree[];

extern const vp8_tree_index  vp8_mbsplit_tree[];
extern const vp8_tree_index  vp8_mv_ref_tree[];
extern const vp8_tree_index  vp8_sub_mv_ref_tree[];

/*extern const struct vp8_token_struct vp8_bmode_encodings   [VP8_BINTRAMODES];
extern const struct vp8_token_struct vp8_ymode_encodings   [VP8_YMODES];
extern const struct vp8_token_struct vp8_kf_ymode_encodings [VP8_YMODES];
extern const struct vp8_token_struct vp8_uv_mode_encodings  [VP8_UV_MODES];
extern const struct vp8_token_struct vp8_mbsplit_encodings  [VP8_NUMMBSPLITS];*/
extern const struct vp8_token_struct *vp8_bmode_encodings;
extern const struct vp8_token_struct *vp8_ymode_encodings;
extern const struct vp8_token_struct *vp8_kf_ymode_encodings;
extern const struct vp8_token_struct *vp8_uv_mode_encodings;
extern const struct vp8_token_struct *vp8_mbsplit_encodings;

/* Inter mode values do not start at zero */

//extern const struct vp8_token_struct vp8_mv_ref_encoding_array    [VP8_MVREFS];
//extern const struct vp8_token_struct vp8_sub_mv_ref_encoding_array [VP8_SUBMVREFS];
extern const struct vp8_token_struct *vp8_mv_ref_encoding_array;
extern const struct vp8_token_struct *vp8_sub_mv_ref_encoding_array;

extern const vp8_tree_index vp8_small_mvtree[];

//extern const struct vp8_token_struct vp8_small_mvencodings [8];
extern const struct vp8_token_struct *vp8_small_mvencodings;

void vp8_init_mbmode_probs(VP8_COMMON *x);
void vp8_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES-1]);
void vp8_kf_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1]);//weihu


#endif //VP8_ENTROPY_MODE_H