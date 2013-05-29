/******************************************************************************
 ** File Name:      mca_global.h	                                          *
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
#ifndef _MCA_GLOBAL_H_
#define _MCA_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
#ifdef VP8_DEC
#include "vp8_blockd.h"
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define MCA_Y_SIZE	16
#define MCA_C_SIZE	8

//byte
#define MCA_Y_OFFSET		0
#define MCA_U_OFFSET		(16*16)
#define MCA_V_OFFSET		(16*16+8*8)


/*ref block size*/
#define MCA_BLOCK2X2		0
#define	MCA_BLOCK4X4		1
#define MCA_BLOCK8X8		2
#define MCA_BLOCK16X16		3

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
typedef void (*MC_16x16)(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y);

typedef void (*MC_8x8)(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);

typedef void (*MC_8x4)(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);

void Mp4_mc_xyfull_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y);
void Mp4_mc_xhalfyfull_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y);
void Mp4_mc_xfullyhalf_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y);
void Mp4_mc_xyhalf_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y);

void Mp4_mc_xyfull_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xfullyhalf_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xhalfyfull_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xyhalf_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);

void Mp4_mc_xyfull_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xfullyhalf_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xhalfyfull_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);
void Mp4_mc_xyhalf_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y);

extern MC_16x16 g_dec_mc_16x16[4];
extern MC_8x8   g_dec_mc_8x8[4];
extern MC_8x8   g_dec_mc_8x8_me[4];
#ifdef VP8_DEC
extern int32 MCA_CMD_Buf[16200];//[16200];
#else
extern int32 MCA_CMD_Buf[49];//[16200];
#endif		
//extern int32 MCA_CMD_Buf[300];//[49];//[16200];
extern int32 MCA_CMD_Num;

extern int ref_blk_id;
extern int ref_blk_size;
extern int ref_blk_end;
extern int ref_bir_blk;
extern int ref_cmd_type;
extern int ref_bw_frame_id;
extern int ref_fw_frame_id;
extern int16 mv_x;
extern int16 mv_y;
extern int cmd_idx;
extern int skip_flag;

PUBLIC void Mp4Dec_Trace_MCA_result_one_macroblock(uint8 *pMcaBfr);
PUBLIC void Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug(FILE *pfIntp, uint8 *pMcaBfr);

//PUBLIC void mca_module(MACROBLOCKD* xd);
#ifdef VP8_DEC //weihu
PUBLIC void mca_module(MACROBLOCKD *xd);
#else
PUBLIC void mca_module();
#endif

void Mp4Dec_MC_GetAverage();
void h264_MC_GetAverage(int32 b8);
PUBLIC void init_mca(void);

void h264_mc_luma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, int32 list);
void h264_mc_chroma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_u_ptr, uint8 *rec_blk_v_ptr, int32 list);

typedef void(*RvDec_MC)(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);

void RvDec_MC_Luma_H00V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H00V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H00V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);

void RvDec_MC_Chroma_H00V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);

void RvDec_MC_Luma_H00V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H03V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H00V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H03V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H00V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H03V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H00V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H01V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H02V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Luma_H03V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H03V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H00V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H03V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H03V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H01V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H02V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);
void RvDec_MC_Chroma_H03V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, int32 start_x, int32 start_y, uint32 uBlkWidth);

#ifdef VP8_DEC //weihu
void vp8_mc_luma(int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 * rec_blk_ptr, uint8 int_type,MACROBLOCKD *xd);
void vp8_mc_chroma(int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 * rec_blk_u_ptr, uint8 * rec_blk_v_ptr, uint8 int_type,MACROBLOCKD *xd);
void vp8_mc (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd);
void vp8_mc_u (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd);
void vp8_mc_v (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr, uint8 int_type, MACROBLOCKD *xd);

void vp8_motion_compensation(int32 mb_x, int32 mb_y,MACROBLOCKD *xd);
#endif
#ifdef H264_DEC
void mca_module_ppa (char  mca_out_buf[384],
					 int slice_info[50],
					 int mca_para_buf[50], //38*26b
					 char decoder_format,//3b
					 char picwidthinMB,//7b
					 char picheightinMB//7b					 
					 );//weihu
#endif
PUBLIC void init_mca_cmd();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MCA_GLOBAL_H_
