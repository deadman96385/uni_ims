 
#ifndef VP8DEC_BASIC_H
#define VP8DEC_BASIC_H

#include "vp8_blockd.h"
#include "vp8dec_dboolhuff.h"

typedef void   *VP8D_PTR;


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
    int64 time_stamp;
    int size;
} DATARATE;

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
//  UINT16 *ptr_onyx_coef_bands_x;
    uint8 *ptr_onyx_coef_bands_x;

    ENTROPY_CONTEXT   **A;
    ENTROPY_CONTEXT(*L)[4];

    int16 *qcoeff_start_ptr;
    BOOL_DECODER *current_bc;

    uint8 *coef_probs[4];

    uint8 eob[25];

} DETOK;

#endif //VP8DEC_BASIC_H
