#ifndef VP8DEC_MODE_H
#define VP8DEC_MODE_H

#include "vp8dec_basic.h"
#include "vp8dec.h"

typedef int ENTROPY_CONTEXT;

/* For keyframes, intra block modes are predicted by the (already decoded)
   modes for the Y blocks to the left and above us; for interframes, there
   is a single probability table. */

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

    FRAME_TYPE_E frame_type;

    int32 up_available;
    int32 left_available;


    // 0 indicates segmentation at MB level is not enabled. Otherwise the individual bits indicate which features are active.
    uint8 segmentation_enabled;

    // 0 (do not update) 1 (update) the macroblock segmentation map.
    uint8 update_mb_segmentation_map;

    // 0 (do not update) 1 (update) the macroblock segmentation feature data.
    uint8 update_mb_segmentation_data;

    // 0 (do not update) 1 (update) the macroblock segmentation feature data.
    uint8 mb_segement_abs_delta;

    // Per frame flags that define which MB level features (such as quantizer or loop filter level)
    // are enabled and when enabled the proabilities used to decode the per MB flags in MB_MODE_INFO
    vp8_prob mb_segment_tree_probs[MB_FEATURE_TREE_PROBS];         // Probability Tree used to code Segment number
    uint8 dumb_byte;  //for 32-bit alignement

    int8 segment_feature_data[MB_LVL_MAX][MAX_MB_SEGMENTS];            // Segment parameters

    // mode_based Loop filter adjustment
    uint8 mode_ref_lf_delta_enabled;
    uint8 mode_ref_lf_delta_update;
    uint8 dumb_byte2[2];  //for 32-bit alignement

    // Delta values have the range +/- MAX_LOOP_FILTER
    //char ref_lf_deltas[MAX_REF_LF_DELTAS];                      // 0 = Intra, Last, GF, ARF
    //char mode_lf_deltas[MAX_MODE_LF_DELTAS];                            // 0 = BPRED, ZERO_MV, MV, SPLIT
    int8 ref_lf_deltas[MAX_REF_LF_DELTAS];                     // 0 = Intra, Last, GF, ARF
    int8 mode_lf_deltas[MAX_MODE_LF_DELTAS];                           // 0 = BPRED, ZERO_MV, MV, SPLIT

    void *current_bc;
} MACROBLOCKD;

typedef enum
{
    SUBMVREF_NORMAL,
    SUBMVREF_LEFT_ZED,
    SUBMVREF_ABOVE_ZED,
    SUBMVREF_LEFT_ABOVE_SAME,
    SUBMVREF_LEFT_ABOVE_ZED
} sumvfref_t;

typedef const int vp8_mbsplit[16];
#define VP8_NUMMBSPLITS 4
#define SUBMVREF_COUNT 5

// FRK
#define MAX_LOOP_FILTER 63

typedef enum
{
    NORMAL_LOOPFILTER = 0,
    SIMPLE_LOOPFILTER = 1
} LOOPFILTERTYPE;

#define VP8BORDERINPIXELS       32

/*************************************
 For INT_YUV:

 Y = (R+G*2+B)/4;
 U = (R-B)/2;
 V =  (G*2 - R - B)/4;
And
 R = Y+U-V;
 G = Y+V;
 B = Y-U-V;
************************************/
typedef enum
{
    REG_YUV = 0,    // Regular yuv
    INT_YUV = 1     // The type of yuv that can be tranfer to and from RGB through integer transform
} YUV_TYPE_E;

typedef struct
{
    int32   y_width;
    int32   y_height;
    int32   y_stride;

    int32   uv_width;
    int32   uv_height;
    int32   uv_stride;

    uint8 *y_buffer;
    uint8 *u_buffer;
    uint8 *v_buffer;

    uint8 *buffer_alloc;
    int32 addr_idx;
    int32 border;
    int32 frame_size;

    uint_32or64 y_buffer_virtual;
    uint_32or64 u_buffer_virtual;

    void *pBufferHeader;
} YV12_BUFFER_CONFIG;

typedef struct VP8Common
{
    int16 Y1dequant[QINDEX_RANGE][4][4];
    int16 Y2dequant[QINDEX_RANGE][4][4];
    int16 UVdequant[QINDEX_RANGE][4][4];

    int32 Width;
    int32 Height;
    int32 horiz_scale;
    int32 vert_scale;

    YUV_TYPE_E clr_type;
    CLAMP_TYPE_E  clamp_type;

    uint8 *FRAME_ADDR_4;
    YV12_BUFFER_CONFIG last_frame;
    YV12_BUFFER_CONFIG golden_frame;
    YV12_BUFFER_CONFIG alt_ref_frame;
    YV12_BUFFER_CONFIG new_frame;
    YV12_BUFFER_CONFIG *frame_to_show;
    //YV12_BUFFER_CONFIG buffer_pool[4];	// For buffer management, 2012-11-06 Derek
    int32 buffer_count;	// For buffer management, 2012-11-06 Derek
    int32 ref_count[YUV_BUFFER_NUM];	// For buffer management, 2012-11-06 Derek
    void * buffer_pool[YUV_BUFFER_NUM];
//    YV12_BUFFER_CONFIG post_proc_buffer;
//    YV12_BUFFER_CONFIG temp_scale_frame;

    FRAME_TYPE_E frame_type;

    int32 show_frame;

    int32 frame_flags;
    int32 MBs;
    int32 mb_rows;
    int32 mb_cols;
    int32 mode_info_stride;

    // prfile settings
    int32 mb_no_coeff_skip;
    int32 no_lpf;
    int32 simpler_lpf;
    int32 use_bilinear_mc_filter;
    int32 full_pixel;

    int32 base_qindex;
//    int last_kf_gf_q;  // Q used on the last GF or KF

    int32 y1dc_delta_q;
    int32 y2dc_delta_q;
    int32 y2ac_delta_q;
    int32 uvdc_delta_q;
    int32 uvac_delta_q;

    //unsigned char *gf_active_flags;   // Record of which MBs still refer to last golden frame either directly or through 0,0
    //int gf_active_count;

    /* We allocate a MODE_INFO struct for each macroblock, together with
       an extra row on top and column on the left to simplify prediction. */
    LOOPFILTERTYPE filter_type;
    uint32 filter_level;
    uint32 sharpness_level;

    int32 refresh_last_frame;       // Two state 0 = NO, 1 = YES
    int32 refresh_golden_frame;     // Two state 0 = NO, 1 = YES
    int32 refresh_alt_ref_frame;     // Two state 0 = NO, 1 = YES

    int32 copy_buffer_to_gf;         // 0 none, 1 Last to GF, 2 ARF to GF
    int32 copy_buffer_to_arf;        // 0 none, 1 Last to ARF, 2 GF to ARF

    int32 refresh_entropy_probs;    // Two state 0 = NO, 1 = YES

    int32 ref_frame_sign_bias[MAX_REF_FRAMES];    // Two state 0, 1

    // keyframe block modes are predicted by their above, left neighbors

    vp8_prob kf_bmode_prob [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1];
    vp8_prob kf_ymode_prob [VP8_YMODES-1];  /* keyframe "" */
    vp8_prob kf_uv_mode_prob [VP8_UV_MODES-1];
    uint8 dumb_byte;  //for 32-bit alignement

    FRAME_CONTEXT lfc; // last frame entropy
    FRAME_CONTEXT fc;  // this frame entropy

//    unsigned int current_video_frame;

//    int near_boffset[3];
    int32 version;

    TOKEN_PARTITION multi_token_partition;

#ifdef PACKET_TESTING
    VP8_HEADER oh;
#endif
//    double bitrate;
//    double framerate;

#if CONFIG_RUNTIME_CPU_DETECT
    VP8_COMMON_RTCD rtcd;
#endif
//    struct postproc_state  postproc_state;

    int32 bInitSuceess;
} VP8_COMMON;

/* Size of the bool decoder backing storage
 *
 * This size was chosen to be greater than the worst case encoding of a
 * single macroblock. This was calcluated as follows (python):
 *
 *     def max_cost(prob):
 *         return max(prob_costs[prob], prob_costs[255-prob]) / 256;
 *
 *     tree_nodes_cost = 7 * max_cost(255)
 *     extra_bits_cost = sum([max_cost(bit) for bit in extra_bits])
 *     sign_bit_cost = max_cost(128)
 *     total_cost = tree_nodes_cost + extra_bits_cost + sign_bit_cost
 *
 * where the prob_costs table was taken from the C vp8_prob_cost table in
 * boolhuff.c and the extra_bits table was taken from the 11 extrabits for
 * a category 6 token as defined in vp8d_token_extra_bits2/detokenize.c
 *
 * This equation produced a maximum of 79 bits per coefficient. Scaling up
 * to the macroblock level:
 *
 *     79 bits/coeff * 16 coeff/block * 25 blocks/macroblock = 31600 b/mb
 *
 *     4096 bytes = 32768 bits > 31600
 */
#define VP8_BOOL_DECODER_SZ       (1024*800)//4096
#define VP8_BOOL_DECODER_MASK     (VP8_BOOL_DECODER_SZ-1)
#define VP8_BOOL_DECODER_PTR_MASK (~(uint32)(VP8_BOOL_DECODER_SZ))

#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))
typedef struct
{
//    unsigned int         lowvalue;
    uint32         range;
    uint32         value;
    int32                  count;
    const uint8 *user_buffer;
    uint32         user_buffer_sz;
    uint8       *decode_buffer;
    const uint8 *read_ptr;
    uint8       *write_ptr;
} BOOL_DECODER;

typedef void   *VP8D_PTR;

typedef BOOL_DECODER vp8_reader;

#define vp8_prob_half ( (vp8_prob) 128)

typedef const vp8_tree_index vp8_tree[], *vp8_tree_p;

typedef const struct vp8_token_struct
{
    int32 value;
    int32 Len;
} vp8_token;

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( vo, R, vp8_prob_half)

typedef struct
{
    int32     Width;
    int32     Height;
    int32     Version;
    int32     postprocess;
    int32     max_threads;
} VP8D_CONFIG;

typedef enum
{
    VP8_LAST_FLAG = 1,
    VP8_GOLD_FLAG = 2,
    VP8_ALT_FLAG = 4
} VP8_REFFRAME;

typedef enum
{
    VP8D_OK = 0
} VP8D_SETTING;

typedef struct
{
    int32 ithread;
    void *ptr1;
    void *ptr2;
} DECODETHREAD_DATA;

typedef struct
{
    MACROBLOCKD  mbd;
    int32 mb_row;
    int32 current_mb_col;
    short *coef_ptr;
} MB_ROW_DEC;

typedef struct
{
    int16         min_val;
    int16         Length;
    uint8 Probs[12];
} TOKENEXTRABITS;

typedef struct
{
    int32 *scan;
    uint8 *ptr_onyxblock2context_leftabove;
    vp8_tree_index *vp8_coef_tree_ptr;  //onyx_coef_tree_ptr; ???
    TOKENEXTRABITS *teb_base_ptr;
    uint8 *norm_ptr;
    uint8 *ptr_onyx_coef_bands_x;

    ENTROPY_CONTEXT   **A;
    ENTROPY_CONTEXT(*L)[4];

    int16 *qcoeff_start_ptr;
    BOOL_DECODER *current_bc;

    uint8 *coef_probs[4];

    uint8 eob[25];

} DETOK;

typedef struct
{
    volatile uint32 FH_CFG0;					// [0]:		frame_type,	0 - Key Frame, 1 - Inter Frame
    // [3:1]:	version
    // [4]:		show_frame
    // [5]:		color_space, no use so far
    // [6]:		clamping_type
    // [7]:		segment_feature_mode, 0 - delta, 1 - abs
    // [8]:		filter_type
    // [14:9]:	loop_filter_level
    // [17:15]:	sharpness_level
    // [18]:	loop_filter_adj_enable
    // [20:19]:	log2_nbr_of_dct_partitions
    // [21]:	refresh_entropy_probs
    // [22]:	refresh_golden_frame
    // [23]:	refresh_alternate_frame
    // [25:24]:	copy_buffer_to_golden
    // [27:26]:	copy_buffer_to_alternate
    // [28]:	sign_bias_golden
    // [29]:	sign_bias_alternate
    // [30]:	refresh_last
    // [31]:	mb_no_skip_coeff
    volatile uint32 FH_CFG8;					// [7:0]:	prob_skip_false
    // [15:8]	prob_intra
    // [23:16]	prob_last
    // [31:24]	prob_gf
    volatile uint32 FH_CFG11;					// [31:0]	dct_part_offset of the first dct partition
} VSP_FH_REG_T;

typedef enum
{
    INTER_MEM = 0, /*physical continuous and no-cachable, constant length */
    EXTRA_MEM,   /*physical continuous and no-cachable, variable length, need allocated according to image resolution */
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint_32or64 v_base;  //virtual address
    uint_32or64 p_base;  //physical address
} CODEC_BUF_T;

typedef struct tagVPXDecObject
{
    uint_32or64 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 vsp_freq_div;
    int32	error_flag;
    int32   vsp_version;
    int32 trace_enabled;

    VPXHandle  *vpxHandle;

    MACROBLOCKD mb;

    VP8_COMMON common;

    vp8_reader bc, bc2;

    const uint8 *Source;
    uint32   source_sz;

    int last_mb_row_decoded;
    int current_mb_col_main;

    vp8_reader *mbc;

    CODEC_BUF_T mem[MAX_MEM_TYPE];

    VSP_FH_REG_T   * g_fh_reg_ptr;
    int32 yuv_format;
} VPXDecObject;

#endif //VP8DEC_MODE_H
