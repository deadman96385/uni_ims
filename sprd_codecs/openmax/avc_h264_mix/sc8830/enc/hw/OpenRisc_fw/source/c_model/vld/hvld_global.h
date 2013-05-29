/*hvld_global.h*/
#ifndef _HVLD_GLOBAL_H_
#define _HVLD_GLOBAL_H_
// #include "h264dec_bitstream.h"
#include "hvld_mode.h"
// #include "h264dec_global.h"
#include "hvld_test_vector.h"

// extern HVLD_REG_T	 *	g_hvld_reg_ptr;

// extern int32			g_dct_io_buf[256];
extern uint32			g_hvld_huff_tab[196];

extern char				g_inverse_scan[16];
extern char				g_is_dctorder [16];

extern uint32			g_huff_tab_token [69];


extern FILE		*		g_hvld_trace_fp;
extern int				g_hvld_dec_status;


void NnzRegInit ();

void H264_IpcmDec ();

void GetNeighborNnz (
					 /*input*/
					 int		blk_type, 
					 int		blk_id, 
					 int		lmb_avail, 
					 int		tmb_avail, 
					 
					 /*output*/
					 int	*	nc_ptr
					 );

int H264VldLevDec (
				   int		trailing_one, 
				   int		total_coeff
				   );

int H264VldRunDec (
				   int	total_coeff,
				   int	blk_type,
				   int	max_coeff_num
				   );



void H264VldBlk (
				 /*input*/
				 int	blk_type,
				 int	blk_id,
				 int	start_pos,
				 int	max_coeff_num,
				 int	lmb_avail,
				 int	tmb_avail,
				 
				 /*output*/
				 int *	err_ptr
);

void WriteBackTotalCoeff (
						  int	blk_type, 
						  int	blk_id, 
						  int	total_coeff
						  );

void H264VldMBCtr (
				   /*input*/
				   /*output*/
				   );

void InitVldTrace ();

void init_hvld ();


#endif