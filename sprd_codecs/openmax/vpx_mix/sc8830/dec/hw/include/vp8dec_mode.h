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

    FRAME_TYPE frame_type;

    int up_available;
    int left_available;


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
// Need to align this structure so when it is declared and
// passed it can be loaded into vector registers.
// FRK
typedef struct
{
    signed char lim[16];
    signed char flim[16];
    signed char thr[16];
    signed char mbflim[16];
    signed char mbthr[16];
    signed char uvlim[16];
    signed char uvflim[16];
    signed char uvthr[16];
    signed char uvmbflim[16];
    signed char uvmbthr[16];
} loop_filter_info;
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
} YUV_TYPE;

typedef struct
{
    int   y_width;
    int   y_height;
    int   y_stride;
//    int   yinternal_width;

    int   uv_width;
    int   uv_height;
    int   uv_stride;
//    int   uvinternal_width;

    unsigned char *y_buffer;
    unsigned char *u_buffer;
    unsigned char *v_buffer;

    unsigned char *buffer_alloc;
    int addr_idx;
    int border;
    int frame_size;


    unsigned int y_buffer_virtual;
    unsigned int u_buffer_virtual;
//        YUV_TYPE clrtype;

    void *pBufferHeader;
} YV12_BUFFER_CONFIG;

typedef struct VP8Common
{
    short Y1dequant[QINDEX_RANGE][4][4];
    short Y2dequant[QINDEX_RANGE][4][4];
    short UVdequant[QINDEX_RANGE][4][4];

    int Width;
    int Height;
    int horiz_scale;
    int vert_scale;

    YUV_TYPE clr_type;
    CLAMP_TYPE  clamp_type;

    unsigned char *FRAME_ADDR_4;
    YV12_BUFFER_CONFIG last_frame;
    YV12_BUFFER_CONFIG golden_frame;
    YV12_BUFFER_CONFIG alt_ref_frame;
    YV12_BUFFER_CONFIG new_frame;
    YV12_BUFFER_CONFIG *frame_to_show;
    //YV12_BUFFER_CONFIG buffer_pool[4];	// For buffer management, 2012-11-06 Derek
    int buffer_count;	// For buffer management, 2012-11-06 Derek
    int ref_count[4];	// For buffer management, 2012-11-06 Derek
    void * buffer_pool[4];
//    YV12_BUFFER_CONFIG post_proc_buffer;
//    YV12_BUFFER_CONFIG temp_scale_frame;

    FRAME_TYPE frame_type;

    int show_frame;

    int frame_flags;
    int MBs;
    int mb_rows;
    int mb_cols;
    int mode_info_stride;

    // prfile settings
    int mb_no_coeff_skip;
    int no_lpf;
    int simpler_lpf;
    int use_bilinear_mc_filter;
    int full_pixel;

    int base_qindex;
//    int last_kf_gf_q;  // Q used on the last GF or KF

    int y1dc_delta_q;
    int y2dc_delta_q;
    int y2ac_delta_q;
    int uvdc_delta_q;
    int uvac_delta_q;

    //unsigned char *gf_active_flags;   // Record of which MBs still refer to last golden frame either directly or through 0,0
    //int gf_active_count;

    /* We allocate a MODE_INFO struct for each macroblock, together with
       an extra row on top and column on the left to simplify prediction. */
    LOOPFILTERTYPE filter_type;
    uint32 filter_level;
    uint32 sharpness_level;

    int refresh_last_frame;       // Two state 0 = NO, 1 = YES
    int refresh_golden_frame;     // Two state 0 = NO, 1 = YES
    int refresh_alt_ref_frame;     // Two state 0 = NO, 1 = YES

    int copy_buffer_to_gf;         // 0 none, 1 Last to GF, 2 ARF to GF
    int copy_buffer_to_arf;        // 0 none, 1 Last to ARF, 2 GF to ARF

    int refresh_entropy_probs;    // Two state 0 = NO, 1 = YES

    int ref_frame_sign_bias[MAX_REF_FRAMES];    // Two state 0, 1

    // keyframe block modes are predicted by their above, left neighbors

    vp8_prob kf_bmode_prob [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1];
    vp8_prob kf_ymode_prob [VP8_YMODES-1];  /* keyframe "" */
    vp8_prob kf_uv_mode_prob [VP8_UV_MODES-1];
    unsigned char dumb_byte;  //for 32-bit alignement

    FRAME_CONTEXT lfc; // last frame entropy
    FRAME_CONTEXT fc;  // this frame entropy

//    unsigned int current_video_frame;

//    int near_boffset[3];
    int version;
    int error_flag;

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
#define VP8_BOOL_DECODER_PTR_MASK (~(unsigned int)(VP8_BOOL_DECODER_SZ))

#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))
typedef struct
{
//    unsigned int         lowvalue;
    unsigned int         range;
    unsigned int         value;
    int                  count;
    const unsigned char *user_buffer;
    unsigned int         user_buffer_sz;
    unsigned char       *decode_buffer;
    const unsigned char *read_ptr;
    unsigned char       *write_ptr;
} BOOL_DECODER;

typedef void   *VP8D_PTR;

typedef BOOL_DECODER vp8_reader;

#define vp8_prob_half ( (vp8_prob) 128)

typedef const vp8_tree_index vp8_tree[], *vp8_tree_p;

typedef const struct vp8_token_struct
{
    int value;
    int Len;
} vp8_token;

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( vo, R, vp8_prob_half)

typedef struct
{
    int     Width;
    int     Height;
    int     Version;
    int     postprocess;
    int     max_threads;
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
    int ithread;
    void *ptr1;
    void *ptr2;
} DECODETHREAD_DATA;

typedef struct
{
    MACROBLOCKD  mbd;
    int mb_row;
    int current_mb_col;
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
    int *scan;
    uint8 *ptr_onyxblock2context_leftabove;
    vp8_tree_index *vp8_coef_tree_ptr;  //onyx_coef_tree_ptr; ???
    TOKENEXTRABITS *teb_base_ptr;
    unsigned char *norm_ptr;
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
    uint8* v_base;  //virtual address
    uint32 p_base;  //physical address
} CODEC_BUF_T;

typedef struct tagVPXDecObject
{
    uint32 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 ddr_bandwidth_req_cnt;
    uint32 vsp_freq_div;

    VPXHandle  *vpxHandle;

    MACROBLOCKD mb;

    VP8_COMMON common;

    vp8_reader bc, bc2;

    const unsigned char *Source;
    unsigned int   source_sz;

    int last_mb_row_decoded;
    int current_mb_col_main;

    vp8_reader *mbc;

    CODEC_BUF_T mem[MAX_MEM_TYPE];

    VSP_FH_REG_T   * g_fh_reg_ptr;
} VPXDecObject;

#endif //VP8DEC_MODE_H
