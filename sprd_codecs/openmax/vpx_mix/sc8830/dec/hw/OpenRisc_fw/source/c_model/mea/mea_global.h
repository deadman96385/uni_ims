/******************************************************************************
 ** File Name:      nea_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MEA_GLOBAL_H_
#define _MEA_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

typedef struct sad_search_tag
{
	int16 x_offset;
	int16 y_offset;
	int32 sad;
}SAD_SEARCH_T;

#define MAX_SAD					0x7FFFFFFF

extern uint32 * g_mea_src_frame_y;
extern uint32 * g_mea_src_frame_u;
extern uint32 * g_mea_src_frame_v;

extern uint32 * g_mea_ref_frame_y;
extern uint32 * g_mea_ref_frame_u;
extern uint32 * g_mea_ref_frame_v;

extern uint32 g_mea_frame_width;	//word
extern uint32 g_mea_frame_height; 

extern uint8 g_SrcMB[6][64];
extern uint8 g_RefMB[6][64];
extern int16 g_ErrMB[6][64];
extern uint8 g_FinalRefMB[6][64];

extern FILE *g_pPureSadFile;
extern FILE *g_pFinalResultFile;
extern FILE *g_p12x9UVBlockFile;
extern FILE *g_pPredMVFile;

extern int32 g_search_point_cnt;

void pre_filter_mb(int32 mb_x, int32 mb_y, int32 pre_filter_thres);
void fetch_src_mb_to_dctiobfr_jpeg(int32 mb_x, int32 mb_y);
void fetch_src_mb_to_dctiobfr(int32 mb_x, int32 mb_y);
void fetch_err_mb_to_dctiobfr(void);
void output_ref_mb(void);
void init_mea_trace();
void filter_ctr (
				 int	mb_x,
				 int	mb_y,
				 int	mb_width,
				 int	mb_height,
				 int	qp
				 );

void print_src_mb(int32 mb_x, int32 mb_y);

PUBLIC void init_mea(void);

void mea_module(void);

void SoftMotionEstimation(int32 mb_x, int32 mb_y);
void Dump_Src_frame_Data_420(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved);
void Dump_Src_frame_Data_422(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved);

#if defined(MPEG4_ENC)
void trace_mea_fetch_one_macroblock(MEA_FETCH_REF *pMea_fetch);
#endif //

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MEA_GLOBAL_H_
