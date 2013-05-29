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
#include "sc6800x_video_header.h"
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
		
extern int32 MCA_CMD_Buf[33];
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

PUBLIC void Mp4Dec_Trace_MCA_result_one_macroblock(uint8 *pMcaBfr);
PUBLIC void Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug(FILE *pfIntp, uint8 *pMcaBfr);

PUBLIC void mca_module(void);
void Mp4Dec_MC_GetAverage();
PUBLIC void init_mca(void);

void h264_mc_luma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr);
void h264_mc_chroma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_u_ptr, uint8 *rec_blk_v_ptr);


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MCA_GLOBAL_H_
