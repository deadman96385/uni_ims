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

#endif //VP8DEC_MODE_H