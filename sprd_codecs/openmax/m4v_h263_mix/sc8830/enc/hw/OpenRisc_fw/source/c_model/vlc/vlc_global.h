/******************************************************************************
 ** File Name:      vlc_global.h	                                          *
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
#ifndef _VLC_GLOBAL_H_
#define _VLC_GLOBAL_H_

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

/*mpeg4 intra vlc table offset for each catagory*/
#define VLC_INTRA_OFFSET_L0R0		0
#define VLC_INTRA_OFFSET_L1R0		28
#define VLC_INTRA_OFFSET_L0L1		37
#define VLC_INTRA_OFFSET_L1L1		52
#define VLC_INTRA_OFFSET_L0L2		73
#define VLC_INTRA_OFFSET_L1L2		83
#define VLC_INTRA_OFFSET_L0L3		90
#define VLC_INTRA_OFFSET_L0R1		98
#define VLC_INTRA_OFFSET_L0R2		109
#define VLC_INTRA_OFFSET_L0R3		115
#define VLC_INTRA_OFFSET_L1L3		120

/*mpeg4 inter vlc table offset for each catagory*/
#define VLC_INTER_OFFSET_L0R0		0
#define VLC_INTER_OFFSET_L0L1		13
#define VLC_INTER_OFFSET_L1L1		40
#define VLC_INTER_OFFSET_L0R1		81
#define VLC_INTER_OFFSET_L0L2		88
#define VLC_INTER_OFFSET_L0L3		99
#define VLC_INTER_OFFSET_L0R2		106
#define VLC_INTER_OFFSET_L1R0		111
#define VLC_INTER_OFFSET_L1R1		115

void Mp4Enc_TestVecInit ();
void GenMPEG4VLCTable ();
void mpeg4_vlc (
				int		is_short_header, 
				int		is_intra, 
				int		run, 
				int16	level, 
				int		last
				);
void PrintRlcInf (int run, int level, int last);
int GetDCScaler (int qp_pred, int blk_cnt);
void dc_prediction (
					  /*input*/
						int		mb_x,
						int		blk_id,
						int		left_avail,
						int		top_avail,
						int		tl_avail,
						int16	curr_dc,

						/*output*/
						int *	dir_pred_ptr,
						int16 *	dc_pred_ptr
					  );
int vlc_GetDivInverse (int dc_scaler);
void PrintDCPred (int dir_pred, uint16 dc_pred);
void mpeg4_dc_enc (
				   int16	dc_diff_q, 
				   int		blk_cnt, 
				   int *    vlc_bsm_wdata_ptr, 
				   int *	vlc_bsm_length_ptr
				   );

void rlc_blk (
			  /**/
			  int		blk_id,
			  int		standard,
			  int		start_position,

			  /**/
			  int		is_intra,
			  int		is_short_header
			  );
void PrintVlcEvent (int escape_mode, int lmax, int rmax, int vlc_tbuf_addr);
void PrintDCEnc (int dc_size_huf_code, int dc_size_huf_len);
void PrintfBsmOut (uint32 val, int nbits);
void PrintfVLCOut(uint32 val, int nbits, uint8* comment);

void mvlc_ctr ();
void vlc_module();

void jpeg_vlc (
			   int			blk_id, 
			   int			is_dc, 
			   int			run, 
			   int16		level
			   );
void clear_vlc();
void init_vlc_module();


extern int intra_blk_cnt;

extern int g_vlc_status;
extern uint16 g_enc_dc_pred_y;
extern uint16 g_enc_dc_pred_u; 
extern uint16 g_enc_dc_pred_v;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_COMMON_GLOBAL_H_
