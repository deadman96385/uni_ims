#ifndef VP8_BLCOKD_H
#define VP8_BLCOKD_H

#include "sci_types.h"
#include "video_common.h"
#include "vp8_basic.h"
#include "vp8_mv.h"
#include "vp8_subpix.h"
#include "vp8_yv12config.h"

typedef int ENTROPY_CONTEXT;

/* For keyframes, intra block modes are predicted by the (already decoded)
   modes for the Y blocks to the left and above us; for interframes, there
   is a single probability table. */

typedef struct
{
    B_PREDICTION_MODE mode;
    union
    {
        int as_int;
        MV  as_mv;
    } mv;
//	MV mvd;
} B_MODE_INFO;

typedef enum
{
    INTRA_FRAME = 0,
    LAST_FRAME = 1,
    GOLDEN_FRAME = 2,
    ALTREF_FRAME = 3,
    MAX_REF_FRAMES = 4
} MV_REFERENCE_FRAME;

typedef struct
{
    MB_PREDICTION_MODE mode, uv_mode;
    MV_REFERENCE_FRAME ref_frame;
    union
    {
        int as_int;
        MV  as_mv;
    } mv;
//	MV mvd;
    int partitioning;
    int partition_count;
    int mb_skip_coeff;			//does this mb has coefficients at all, 1=no coefficients, 0=need decode tokens
    int dc_diff;
    unsigned int segment_id;	// Which set of segmentation parameters should be used for this MB
//    int force_no_skip;

    B_MODE_INFO partition_bmi[16];

} MB_MODE_INFO;

typedef struct
{
    MB_MODE_INFO mbmi;
    B_MODE_INFO bmi[16];
} MODE_INFO;

typedef struct
{
    int16 *qcoeff;
    int16 *dqcoeff;
    uint8  *predictor;
    int16 *diff;
    int16 *reference;

    int16 (*dequant)[4];

    // 16 Y blocks, 4 U blocks, 4 V blocks each with 16 entries
    uint8 **base_pre;
    int pre;
    int pre_stride;

    uint8 **base_dst;
    int dst;
    int dst_stride;

    int eob;

    B_MODE_INFO bmi;

} BLOCKD;

typedef struct
{
#ifdef SIM_IN_WIN
    int16 diff[400];      // from idct diff
    uint8 predictor[384];
//    int16 reference[384];
    int16 qcoeff[400];
    int16 dqcoeff[400];

    // 16 Y blocks, 4 U, 4 V, 1 DC 2nd order block, each with 16 entries.
    BLOCKD block[25];

    YV12_BUFFER_CONFIG pre; // Filtered copy of previous frame reconstruction
    YV12_BUFFER_CONFIG dst;

    MODE_INFO *mode_info_context;
    MODE_INFO *mode_info;

    int mode_info_stride;

	MB_MODE_INFO mbmi;
#endif

    FRAME_TYPE frame_type;

    int up_available;
    int left_available;

#ifdef SIM_IN_WIN
    // Y,U,V,Y2
    ENTROPY_CONTEXT *above_context[4];   // row of context for each plane
    ENTROPY_CONTEXT(*left_context)[4];   // (up to) 4 contexts ""
#endif

    // 0 indicates segmentation at MB level is not enabled. Otherwise the individual bits indicate which features are active.
    uint8 segmentation_enabled;

    // 0 (do not update) 1 (update) the macroblock segmentation map.
    unsigned char update_mb_segmentation_map;

    // 0 (do not update) 1 (update) the macroblock segmentation feature data.
    unsigned char update_mb_segmentation_data;

    // 0 (do not update) 1 (update) the macroblock segmentation feature data.
    unsigned char mb_segement_abs_delta;

    // Per frame flags that define which MB level features (such as quantizer or loop filter level)
    // are enabled and when enabled the proabilities used to decode the per MB flags in MB_MODE_INFO
    vp8_prob mb_segment_tree_probs[MB_FEATURE_TREE_PROBS];         // Probability Tree used to code Segment number
	unsigned char dumb_byte;  //for 32-bit alignement

    signed char segment_feature_data[MB_LVL_MAX][MAX_MB_SEGMENTS];            // Segment parameters

    // mode_based Loop filter adjustment
    unsigned char mode_ref_lf_delta_enabled;
    unsigned char mode_ref_lf_delta_update;
	unsigned char dumb_byte2[2];  //for 32-bit alignement

    // Delta values have the range +/- MAX_LOOP_FILTER
    //char ref_lf_deltas[MAX_REF_LF_DELTAS];                      // 0 = Intra, Last, GF, ARF
    //char mode_lf_deltas[MAX_MODE_LF_DELTAS];                            // 0 = BPRED, ZERO_MV, MV, SPLIT
    signed char ref_lf_deltas[MAX_REF_LF_DELTAS];                     // 0 = Intra, Last, GF, ARF
    signed char mode_lf_deltas[MAX_MODE_LF_DELTAS];                           // 0 = BPRED, ZERO_MV, MV, SPLIT

#ifdef SIM_IN_WIN
    // Distance of MB away from frame edges
    short mb_to_left_edge;
    short mb_to_right_edge;
    short mb_to_top_edge;
    short mb_to_bottom_edge;

    //char * gf_active_ptr;
    //signed char *gf_active_ptr;

    vp8_subpix_fn_t  subpixel_predict;
    vp8_subpix_fn_t  subpixel_predict8x4;
    vp8_subpix_fn_t  subpixel_predict8x8;
    vp8_subpix_fn_t  subpixel_predict16x16;
#endif

    void *current_bc;

#if CONFIG_RUNTIME_CPU_DETECT
    struct VP8_COMMON_RTCD  *rtcd;
#endif
} MACROBLOCKD;

extern const int vp8_block2left[25];
extern const int vp8_block2above[25];
extern const int vp8_block2type[25];
extern const int vp8_block2context[25];


extern void vp8_build_block_doffsets(MACROBLOCKD *x);
extern void vp8_setup_block_dptrs(MACROBLOCKD *x);//weihu

#endif //VP8_BLCOKD_H
