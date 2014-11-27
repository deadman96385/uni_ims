/******************************************************************************
** File Name:      vp8dec_basic.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           07/04/2013                                                *
** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VP8_BASIC_H_
#define _VP8_BASIC_H_

#include "sci_types.h"

#define MINQ 0
#define MAXQ 127
#define QINDEX_RANGE (MAXQ + 1)

typedef enum
{
    RECON_CLAMP_REQUIRED        = 0,
    RECON_CLAMP_NOTREQUIRED     = 1
} CLAMP_TYPE_E;

typedef enum
{
    SIXTAP   = 0,
    BILINEAR = 1
} INTERPOLATIONFILTERTYPE;

/* Coefficient token alphabet */

#define ZERO_TOKEN              0       //0         Extra Bits 0+0
#define ONE_TOKEN               1       //1         Extra Bits 0+1
#define TWO_TOKEN               2       //2         Extra Bits 0+1
#define THREE_TOKEN             3       //3         Extra Bits 0+1
#define FOUR_TOKEN              4       //4         Extra Bits 0+1
#define DCT_VAL_CATEGORY1       5       //5-6       Extra Bits 1+1
#define DCT_VAL_CATEGORY2       6       //7-10      Extra Bits 2+1
#define DCT_VAL_CATEGORY3       7       //11-26     Extra Bits 4+1
#define DCT_VAL_CATEGORY4       8       //11-26     Extra Bits 5+1
#define DCT_VAL_CATEGORY5       9       //27-58     Extra Bits 5+1
#define DCT_VAL_CATEGORY6       10      //59+       Extra Bits 11+1
#define DCT_EOB_TOKEN           11      //EOB       Extra Bits 0+0

#define vp8_coef_tokens 12
#define MAX_ENTROPY_TOKENS vp8_coef_tokens
#define ENTROPY_NODES 11

/* Coefficients are predicted via a 3-dimensional probability table. */

/* Outside dimension.  0 = Y no DC, 1 = Y2, 2 = UV, 3 = Y with DC */

#define BLOCK_TYPES 4

/* Middle dimension is a coarsening of the coefficient's
   position within the 4x4 DCT. */

#define COEF_BANDS 8

/* Inside dimension is 3-valued measure of nearby complexity, that is,
   the extent to which nearby coefficients are nonzero.  For the first
   coefficient (DC, unless block type is 0), we look at the (already encoded)
   blocks above and to the left of the current block.  The context index is
   then the number (0,1,or 2) of these blocks having nonzero coefficients.
   After decoding a coefficient, the measure is roughly the size of the
   most recently decoded coefficient (0 for 0, 1 for 1, 2 for >1).
   Note that the intuitive meaning of this measure changes as coefficients
   are decoded, e.g., prior to the first token, a zero means that my neighbors
   are empty while, after the first token, because of the use of end-of-block,
   a zero means we just decoded a zero and hence guarantees that a non-zero
   coefficient will appear later in this block.  However, this shift
   in meaning is perfectly OK because our context depends also on the
   coefficient band (and since zigzag positions 0, 1, and 2 are in
   distinct bands). */

/*# define DC_TOKEN_CONTEXTS        3 // 00, 0!0, !0!0 */
#define PREV_COEF_CONTEXTS       3

typedef enum
{
    B_DC_PRED,          // average of above and left pixels
    B_TM_PRED,

    B_VE_PRED,           // vertical prediction
    B_HE_PRED,           // horizontal prediction

    B_LD_PRED,
    B_RD_PRED,

    B_VR_PRED,
    B_VL_PRED,
    B_HD_PRED,
    B_HU_PRED,

    LEFT4X4,
    ABOVE4X4,
    ZERO4X4,
    NEW4X4,

    B_MODE_COUNT
} B_PREDICTION_MODE;

#define VP8_BINTRAMODES (B_HU_PRED + 1)  /* 10 */
#define VP8_SUBMVREFS (1 + NEW4X4 - LEFT4X4)

// Segment Feature Masks
#define SEGMENT_ALTQ    0x01
#define SEGMENT_ALT_LF  0x02

typedef enum
{
    DC_PRED,            // average of above and left pixels
    V_PRED,             // vertical prediction
    H_PRED,             // horizontal prediction
    TM_PRED,            // Truemotion prediction
    B_PRED,             // block based prediction, each block has its own prediction mode

    NEARESTMV,
    NEARMV,
    ZEROMV,
    NEWMV,
    SPLITMV,

    MB_MODE_COUNT
} MB_PREDICTION_MODE;

// Macroblock level features
typedef enum
{
    MB_LVL_ALT_Q = 0,               // Use alternate Quantizer ....
    MB_LVL_ALT_LF = 1,              // Use alternate loop filter value...
    MB_LVL_MAX = 2,                 // Number of MB level features supported

} MB_LVL_FEATURES;

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

#define VP8_YMODES  (B_PRED + 1) /*4+1*/
#define VP8_UV_MODES (TM_PRED + 1) /*3+1*/

#define VP8_MVREFS (1 + SPLITMV - NEARESTMV)	// 1+9-5

typedef struct frame_contexts
{
    vp8_prob bmode_prob [VP8_BINTRAMODES-1];
    vp8_prob ymode_prob [VP8_YMODES-1];   /* interframe intra mode probs */
    vp8_prob uv_mode_prob [VP8_UV_MODES-1];
//    vp8_prob sub_mv_ref_prob [VP8_SUBMVREFS-1];
    vp8_prob coef_probs [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [ENTROPY_NODES];
    MV_CONTEXT mvc[2];
//    MV_CONTEXT pre_mvc[2];  //not to caculate the mvcost for the frame if mvc doesn't change.
//	unsigned char dumb_byte[2];  //for 32-bit alignement
} FRAME_CONTEXT;

typedef enum
{
    ONE_PARTITION  = 0,
    TWO_PARTITION  = 1,
    FOUR_PARTITION = 2,
    EIGHT_PARTITION = 3
} TOKEN_PARTITION;


#define MB_FEATURE_TREE_PROBS   3
#define MAX_MB_SEGMENTS         4

#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      4

// Segment Feature Masks
#define SEGMENT_DELTADATA   0
#define SEGMENT_ABSDATA     1

#define YUV_BUFFER_NUM	4

typedef enum
{
    KEY_FRAME = 0,
    INTER_FRAME = 1
} FRAME_TYPE_E;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_VP8DEC_BASIC_H_
