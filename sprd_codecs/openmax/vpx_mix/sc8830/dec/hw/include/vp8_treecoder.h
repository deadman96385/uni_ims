
#ifndef _VP8_TREECODER_H_
#define _VP8_TREECODER_H_

#include "sci_types.h"

#define vp8_prob_half ( (vp8_prob) 128)

typedef const vp8_tree_index vp8_tree[], *vp8_tree_p;

typedef const struct vp8_token_struct
{
    int value;
    int Len;
} vp8_token;

void vp8_tokens_from_tree(struct vp8_token_struct *p, vp8_tree t);

void vp8_tree_probs_from_distribution(
    int n,                      /* n = size of alphabet */
    vp8_token tok               [ /* n */ ],
    vp8_tree tree,
    vp8_prob probs          [ /* n-1 */ ],
    unsigned int branch_ct       [ /* n-1 */ ] [2],
    const unsigned int num_events[ /* n */ ],
    unsigned int Pfac,
    int rd
);

#endif //_VP8_TREECODER_H_
