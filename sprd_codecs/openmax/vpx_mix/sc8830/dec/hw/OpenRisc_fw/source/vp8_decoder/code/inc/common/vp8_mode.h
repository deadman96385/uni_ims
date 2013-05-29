
#ifndef VP8_MODE_H
#define VP8_MODE_H

#include "vp8_basic.h"
// #include "vp8_loopfilter.h"
#include "vp8_yv12config.h"
#include "vp8_postproc.h"
#include "vp8_blockd.h"

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


typedef void (*loopfilter_block)(unsigned char *y, unsigned char *u, unsigned char *v,
             int ystride, int uv_stride, loop_filter_info *lfi, int simpler);

typedef struct VP8Common
{
    short Y1dequant[QINDEX_RANGE][4][4];
    short Y2dequant[QINDEX_RANGE][4][4];
    short UVdequant[QINDEX_RANGE][4][4];

	uint8	ycode[16];
	uint8	uvcode[16];

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
#ifdef SIM_IN_WIN
	uint8 *y_rec;
	uint8 *uv_rec;
    MODE_INFO *mip; /* Base of allocated array */
    MODE_INFO *mi;  /* Corresponds to upper left visible macroblock */
#endif
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

#ifdef SIM_IN_WIN
    // Y,U,V,Y2
    ENTROPY_CONTEXT *above_context[4];   // row of context for each plane
    ENTROPY_CONTEXT left_context[4][4];  // (up to) 4 contexts ""
#endif
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

#endif //VP8_MODE_H