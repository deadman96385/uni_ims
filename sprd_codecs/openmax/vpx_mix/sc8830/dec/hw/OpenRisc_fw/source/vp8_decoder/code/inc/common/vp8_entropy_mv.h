
#ifndef VP8_ENTROPY_MV_H
#define VP8_ENTROPY_MV_H

#include "sci_types.h"

enum
{
    mv_max  = 1023,              /* max absolute value of a MV component */
    MVvals = (2 * mv_max) + 1,   /* # possible values "" */

    mvlong_width = 10,       /* Large MVs have 9 bit magnitudes */
    mvnum_short = 8,         /* magnitudes 0 through 7 */

    /* probability offsets for coding each MV component */

    mvpis_short = 0,         /* short (<= 7) vs long (>= 8) */
    MVPsign,                /* sign for non-zero */
    MVPshort,               /* 8 short values = 7-position tree */

    MVPbits = MVPshort + mvnum_short - 1, /* mvlong_width long value bits */
    MVPcount = MVPbits + mvlong_width    /* (with independent probabilities) */
};

typedef struct mv_context
{
    vp8_prob prob[MVPcount];  /* 19 */ /* often come in row, col pairs */
} MV_CONTEXT;

extern const MV_CONTEXT vp8_mv_update_probs[2], vp8_default_mv_context[2];

#endif // VP8_ENTROPY_MV_H
