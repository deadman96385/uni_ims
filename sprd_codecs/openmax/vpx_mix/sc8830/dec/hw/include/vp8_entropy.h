
#ifndef _VP8_ENTROPY_H_
#define _VP8_ENTROPY_H_

#include "sci_types.h"
#include "vp8_basic.h"
#include "vp8_mode.h"

extern const int vp8_mb_feature_data_bits[MB_LVL_MAX];
extern const vp8_tree_index vp8_coef_tree[];
//extern const struct vp8_token_struct vp8_coef_encodings[vp8_coef_tokens];
extern const struct vp8_token_struct *vp8_coef_encodings;
extern const unsigned char vp8_coef_bands[16];
extern const unsigned char vp8_prev_token_class[vp8_coef_tokens];
extern const vp8_prob vp8_coef_update_probs [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [vp8_coef_tokens-1];
extern const int vp8_default_zig_zag1d[16];

void vp8_default_coef_probs(VP8_COMMON *pc);

#endif //_VP8_ENTROPY_H_
