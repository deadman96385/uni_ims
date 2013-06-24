#ifndef VP8DEC_MODE_H
#define VP8DEC_MODE_H

#include "vp8_yv12config.h"
#include "vp8_blockd.h"
#include "vp8_basic.h"
#include "vp8_mode.h"
#include "vp8dec_treereader.h"
#include "vp8dec_basic.h"

typedef struct VP8Decompressor
{
    MACROBLOCKD mb;

    VP8_COMMON common;

    vp8_reader bc, bc2;

    const unsigned char *Source;
    unsigned int   source_sz;

    int last_mb_row_decoded;
    int current_mb_col_main;

#if CONFIG_MULTITHREAD
    pthread_t           h_thread_lpf;         // thread for postprocessing
    sem_t               h_event_lpf;          // Event for post_proc completed
    sem_t               h_event_start_lpf;
    pthread_t           *h_decoding_thread;
    sem_t               *h_event_mbrdecoding;
    sem_t               h_event_main;
    // end of threading data
#endif
    vp8_reader *mbc;

#if CONFIG_RUNTIME_CPU_DETECT
    vp8_dequant_rtcd_vtable_t        dequant;
    struct vp8_dboolhuff_rtcd_vtable dboolhuff;
#endif
} VP8D_COMP;


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
#ifdef SIM_IN_WIN
	volatile uint32 FH_CFG1;					// [13:0]:	width
												// [27:14]:	height
												// [29:28]:	horiz_scale
												// [31:30]:	vert_scale
	volatile uint32 FH_CFG2;					// [0]:		segmentation_enabled
												// [1]:		update_mb_segmentation_map
												// [8:2]:	quantizer update value[segment_id=0]
												// [15:9]:	quantizer update value[segment_id=1]
												// [22:16]:	quantizer update value[segment_id=2]
												// [29:23]:	quantizer update value[segment_id=3]
	volatile uint32 FH_CFG3;					// [5:0]:	loop filter update value[segment_id=0]
												// [11:6]:	loop filter update value[segment_id=1]
												// [17:12]:	loop filter update value[segment_id=2]
												// [23:18]:	loop filter update value[segment_id=3]
	volatile uint32 FH_CFG4;					// [7:0]:	segment_prob_update[0], used to decode segment_id, default = 255
												// [15:8]:	segment_prob_update[1], used to decode segment_id, default = 255
												// [23:16]:	segment_prob_update[2], used to decode segment_id, default = 255
	volatile uint32 FH_CFG5;					// [5:0]:	ref_frame_delta_update[0, ref_frame=intra], used to update filter_level
												// [11:6]:	ref_frame_delta_update[1, ref_frame=last], used to update filter_level
												// [17:12]:	ref_frame_delta_update[2, ref_frame=GF], used to update filter_level
												// [23:18]:	ref_frame_delta_update[3, ref_frame=ARF], used to update filter_level
	volatile uint32 FH_CFG6;					// [5:0]:	mb_mode_delta_update[0, mb_mode=intra BPRED], used to update filter_level
												// [11:6]:	mb_mode_delta_update[1, mb_mode=ZERO_MV], used to update filter_level
												// [17:12]:	mb_mode_delta_update[2, mb_mode=Nearest, Near, New], used to update filter_level
												// [23:18]:	mb_mode_delta_update[3, mb_mode=SPLIT_MV], used to update filter_level
	volatile uint32 FH_CFG7;					// [6:0]:	y_ac_qi, base DQ table index
												// [10:7]	y_dc_delta, default = 0
												// [14:11]	y2_dc_delta, default = 0
												// [18:15]	y2_ac_delta, default = 0
												// [22:19]	uv_dc_delta, default = 0
												// [26:23]	uv_ac_delta, default = 0
#endif
	volatile uint32 FH_CFG8;					// [7:0]:	prob_skip_false
												// [15:8]	prob_intra
												// [23:16]	prob_last
												// [31:24]	prob_gf
#ifdef SIM_IN_WIN
	volatile uint32 FH_CFG9;					// [7:0]	intra_16x16_prob[0]
												// [15:8]	intra_16x16_prob[1]
												// [23:16]	intra_16x16_prob[2]
												// [31:24]	intra_16x16_prob[3]
	volatile uint32 FH_CFG10;					// [7:0]	intra_chroma_prob[0]
												// [15:8]	intra_chroma_prob[1]
												// [23:16]	intra_chroma_prob[2]
#endif
	volatile uint32 FH_CFG11;					// [31:0]	dct_part_offset of the first dct partition
#ifdef SIM_IN_WIN
	volatile uint32 FH_CFG12;					// [7:0]	range of bool decoder
												// [23:8]	value of bool decoder
												// [31:24]	count of bool decoder
	volatile uint8 FH_MV_PROBS[2][MVPcount];	// [0=row|1=cols][19]
	volatile uint8 temp[2];						// For 32-bit alignment
	volatile uint8 FH_COEFF_PROBS[BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [vp8_coef_tokens-1];
#endif
}VSP_FH_REG_T;

#endif //VP8DEC_MODE_H