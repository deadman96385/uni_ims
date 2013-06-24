
#include "sci_types.h"
#include "vp8dec_treereader.h"

/* Intent of tree data structure is to make decoding trivial. */

int vp8_treed_read(
    vp8_reader *const r,        /* !!! must return a 0 or 1 !!! */
    vp8_tree t,
    const vp8_prob *const p
)
{
    /*register*/ vp8_tree_index i = 0;

    while ((i = t[ i + vp8_read(r, p[i>>1])]) > 0) ;

    return -i;
}
