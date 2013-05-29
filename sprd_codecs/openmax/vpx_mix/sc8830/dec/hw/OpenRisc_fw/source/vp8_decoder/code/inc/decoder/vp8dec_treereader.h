
#ifndef VP8DEC_TREEREAD_H
#define VP8DEC_TREEREAD_H

#include "sci_types.h"
#include "vp8_treecoder.h"
#include "vp8dec_dboolhuff.h"

typedef BOOL_DECODER vp8_reader;

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( R, vp8_prob_half)

int vp8_treed_read(
    vp8_reader *const r,        /* !!! must return a 0 or 1 !!! */
    vp8_tree t,
    const vp8_prob *const p
);

#endif //VP8DEC_TREEREAD_H
