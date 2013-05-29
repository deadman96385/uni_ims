
#include "sci_types.h"
#include "vp8_treecoder.h"
#include "vp8_basic.h"
#include "vp8_mode.h"

//static vp8_tree_index cat1[2], cat2[4], cat3[6], cat4[8], cat5[10], cat6[22];
//int16 vp8_default_zig_zag_mask[16];

const int vp8_default_zig_zag1d[16] =
{
    0,  1,  4,  8,
    5,  2,  3,  6,
    9, 12, 13, 10,
    7, 11, 14, 15,
};

const int vp8_mb_feature_data_bits[MB_LVL_MAX] = {7, 6};

/* Array indices are identical to previously-existing CONTEXT_NODE indices */

const vp8_tree_index vp8_coef_tree[ 22] =     /* corresponding _CONTEXT_NODEs */
{
    -DCT_EOB_TOKEN, 2,                             /* 0 = EOB */
    -ZERO_TOKEN, 4,                               /* 1 = ZERO */
    -ONE_TOKEN, 6,                               /* 2 = ONE */
    8, 12,                                      /* 3 = LOW_VAL */
    -TWO_TOKEN, 10,                            /* 4 = TWO */
    -THREE_TOKEN, -FOUR_TOKEN,                /* 5 = THREE */
    14, 16,                                    /* 6 = HIGH_LOW */
    -DCT_VAL_CATEGORY1, -DCT_VAL_CATEGORY2,   /* 7 = CAT_ONE */
    18, 20,                                   /* 8 = CAT_THREEFOUR */
    -DCT_VAL_CATEGORY3, -DCT_VAL_CATEGORY4,  /* 9 = CAT_THREE */
    -DCT_VAL_CATEGORY5, -DCT_VAL_CATEGORY6   /* 10 = CAT_FIVE */
};

/* vp8_coef_encodings generated with:
    vp8_tokens_from_tree(vp8_coef_encodings, vp8_coef_tree);
*/
const struct vp8_token_struct vp8_coef_encodings[MAX_ENTROPY_TOKENS] =
{
    {2, 2},
    {6, 3},
    {28, 5},
    {58, 6},
    {59, 6},
    {60, 6},
    {61, 6},
    {124, 7},
    {125, 7},
    {126, 7},
    {127, 7},
    {0, 1}
};

#include "vp8_default_coef_probs.h"

void vp8_default_coef_probs(VP8_COMMON *pc)
{
	vpx_memcpy(pc->fc.coef_probs, default_coef_probs, sizeof(default_coef_probs));
}

