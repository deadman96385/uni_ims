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

#define CODED_FLAG_CTX_BASE		0
#define SIG_MAP_CTX_BASE		5

extern char				g_inverse_scan[16];
extern char				g_is_dctorder [16];

extern uint32			g_huff_tab_token [69];


extern FILE		*		g_hvld_trace_fp;
extern int				g_hvld_dec_status;


void NnzRegInit ();

void H264_IpcmDec ();

void GetNeighborCodedInfo (
						   /*input*/
						   int		mb_type,
						   int		blk_type, 
						   int		blk_id, 
						   int		lmb_avail, 
						   int		tmb_avail, 
						   
						   /*output*/
						   int	*	coded_flag_a_ptr,
						   int	*	coded_flag_b_ptr,
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

/************************************************************************
							for cabac decoding
*************************************************************************/
extern int		g_range;
extern int		g_offset;

extern int		g_sig_map_reg;

void CabacBlk (int mb_type, int blk_type, int blk_id);

int BiArithDec (
				int			is_bp_mode,				//is by-pass mode
				uint32	*	bsm_data_ptr,			//next 32 bits in bitstream
				int		*	range_ptr,				//code range of arithmetic engine 
				int		*	offset_ptr,				//offset of arithmetic engine
				uint8	*	context_ptr,			//point to context model status to be decoded
				int		*	shift_bits_ptr			//consumed bits for the bin
				);

void TwoBinArithDec (
					 int		syn_type,			//syntax type: coded_blk_flag
					 uint8	*	ctx_bin0_ptr,		//first bin context to be decoded
					 uint8	*	ctx_bin1_ptr,		//second bin context to be decoded
					 int	*	bin0_ptr,			//returned first binary value
					 int	*	bin1_ptr,			//returned second binary value
					 int		glm_sfx_num
					 );

void ArithSigMapDecoder (int blk_cat, int blk_id);
int ArithOneLevDec (
					int blk_cat, 
					int num_t1, 
					int num_lgt1
					);
void ArithLevInforDec (int blk_type, int blk_id);
int GetPrefix0Num (int sig_map_reg);
void UpdateSignMap ( int * sig_map_ptr, int prefix0_num);
int GetBin0Ctx (int blk_cat, int ctx_bin0_inc);
int GetBinothCtx (int blk_cat, int ctx_binoth_inc);
void UpdateBin0Ctx (int blk_cat, int ctx_bin0_inc, int ctx_bin0_upt);
void UpdateBinothCtx (int blk_cat, int ctx_binoth_inc, int ctx_binoth_upt);

#endif