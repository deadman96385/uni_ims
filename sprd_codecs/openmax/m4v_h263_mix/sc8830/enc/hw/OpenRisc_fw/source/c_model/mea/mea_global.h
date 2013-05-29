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
	int32 mvd_cost;
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

MOTION_VECTOR_T tv_Diff_MV[4];
MOTION_VECTOR_T mv_diff_test;
typedef struct{
	uint8 mv_valid;//mv_valid[7:0]
	uint8 partition_mode;//partition_mode
	int8 imv_y_blk0;//imv_y_blk0[5:0]
	int8 imv_x_blk0;//imv_x_blk0[6:0]
	int8 imv_y_blk1;//imv_y_blk1[5:0]
	int8 imv_x_blk1;//imv_x_blk1[6:0]
	int8 imv_y_blk2;//imv_y_blk2[5:0]
	int8 imv_x_blk2;//imv_x_blk2[6:0]
	int8 imv_y_blk3;//imv_y_blk3[5:0]
	int8 imv_x_blk3;//imv_x_blk3[6:0]
	uint32 QP;//Qp[5:0]
	int32 mb_x;//curr_mb_x[6:0]
	int32 mb_y;//curr_mb_y[6:0]
	int32 total_cost_min;//total_cost_min[11:0]
}MEA_INFO;
void Dump_tv_frame_Data_420(uint32 *pIntSrcY, uint32 *pIntSrcU, int32 frame_width, int32 frame_height, int8 print_type);
void PrintfTV(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 frame_type);
void PrintfSrcMB(ENC_VOP_MODE_T *p_mode);
void PrintfMEAbufParam(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfPPAIN(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfMBCPARA(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfVLCParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfDCTParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfMCAParabuf(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintMV_sad_cost(uint32 sad_cost, uint32 mvd_cost, uint8* comment, FILE* fp);
void PrintMV_best_sad_cost(uint32 sad_cost, uint8* comment, FILE* fp);
void PrintMV_ime_src_mb(ENC_VOP_MODE_T *p_mode, MEA_INFO mea_info);
void Print_IMB_fbuf(ENC_VOP_MODE_T *p_mode, MB_MODE_E MB_Mode, MOTION_VECTOR_T *mv, MEA_INFO mea_info);
void PrintDCT_TV();
void VLCSplitInit(ENC_VOP_MODE_T *p_mode);
void VLCSplitDeinit(ENC_VOP_MODE_T *p_mode);
#if !defined(_LIB)
void PrintfPPACFG(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode, int8 VOP_type);
void PrintfFrameCfg(ENC_VOP_MODE_T *p_mode, int8 frame_type);
void PrintfVLCCFG(ENC_VOP_MODE_T *p_mode, ENC_MB_MODE_T *pMb_mode);
#endif
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
