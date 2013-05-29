

#ifndef VP8DEC_DETOKENIZE_H
#define VP8DEC_DETOKENIZE_H

#include "vp8dec_basic.h"
#include "vp8dec_mode.h"

//extern const TOKENEXTRABITS vp8d_token_extra_bits2[MAX_ENTROPY_TOKENS];

void vp8_reset_mb_tokens_context(MACROBLOCKD *x);
int vp8_decode_mb_tokens(VP8D_COMP *dx, MACROBLOCKD *x);

#endif // VP8DEC_DETOKENIZE_H
