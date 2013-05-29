#ifndef _RDBK_GLOBAL_H_
#define _RDBK_GLOBAL_H_

#include "rdbk_mode.h"
//#include "rvdec_basic.h"
//#include "rvdec_mode.h"
#include "video_common.h"
#include "vsp_dbk.h"
#include <stdlib.h>

#define RDBK_IN_PIPELINE 0 


#define RDBK_OBUF_SIZE (30*4 + 2*12*4)
#define RDBK_LBUF_SIZE (4*2*Y_LINE_BUF_WIDTH)//(4*256) // 4 line * (Y + U + V) * MB * 4
#define RDBK_TBUF_SIZE (4*(6+4+4))

#define RV8_PARA_FW 0 // whether to calcluate para in firmware for real 8 dbk
#define	RDBK_OWN_REG 1 // whether to use g_rdbk_reg_ptr or g_dbk_reg_ptr. 1 : g_rdbk_reg_ptr; 0 : g_dbk_reg_ptr

#define MB_SIZE	16
#define UV_MB_SIZE (MB_SIZE/2)

#define Rv_Clip(Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

//extern const uint8 g_strength [32];

extern  uint8 ** pMBYUV;
extern 	uint32	*vsp_rdbk_lbuf; //Line buffer
extern	uint32	*vsp_rdbk_obuf; //Out buffer
extern	uint32	*vsp_rdbk_tbuf; //Trans buffer
// extern	VSP_RDBK_REG_T * g_rdbk_reg_ptr;//RDBK register
// extern	VSP_DBK_REG_T  * g_dbk_reg_ptr;// Vsp DBK register

extern uint32	*vsp_ripred_lbuf; // Line buffer for Ipred to store undeblocked pixels of last line of above MB
extern uint8	vsp_ripred_left[35]; // register to store undeblocked pixels of right most colum of left MB and three topleft pixels
extern uint8	vsp_ripred_topleft_pix[3];

void DBK_malloc_buf();


/************************************************************************/
/* Real 9 function			                                            */
/************************************************************************/
//void Rv9_DBK_FW_Command(RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd,uint32 *pmvd);

void rdbk_module_rv9();

void rdbk_core_ctr_rv9();


void RDBKFilter(uint8 pix_p[4], uint8 pix_q[4], // pixels on left and right side of edge
				int Cl, int Cr,					// clip value left and right
				int alpha, int beta, 
				int Al, int Ar,					// para
				int filt_type,					// Filter type decision logic : 3:strong, 2:normal, 1:weak or 0:none
				int bLuma,						// whether edge is luma edge
				int edge_id, int line_id, int second_phase);		// For upt pix arr

void UpdatePixArrPipe_rv9();

void Get8PixFiltering_rv9(int second_phase, int edge_id, int line_id,
					  uint8 pix_p[4],uint8 pix_q[4]);

void GetEightPixWrite_rv9(int line_id, uint8 pixel_write0[4], uint8 pixel_write1[4]);

void GetLeftRightArr_rv9(int edge_id, int second_phase,
					 uint8 left_arr[12],uint8 right_arr[12]);

void PixArrUptFilt_rv9(	uint8	L1_f, uint8 L2_f, uint8 L3_f,
					uint8	R1_f, uint8 R2_f, uint8 R3_f,
					int	L1_upt, int L2_upt, int L3_upt,
					int	R1_upt, int R2_upt, int R3_upt,
					int edge_id, int line_id, int second_phase);

void ReadPixUptArr_rv9 ( int	line_id, uint8 pixel_read0[4], uint8 pixel_read1[4]);


void CalBlkPara_RDBK_rv9(int blk_x, int blk_y, int blk_comp, int second_phase,//input blk position 
					int proc_edge[2], int edge_clip_left[2]/*or above*/, int edge_clip_right[2]/*or below*/,
				int *alpha, int *beta, int *beta2);

void CalBlkPara_RDBK_SW_rv9(int blk_x, int blk_y, int blk_comp, int second_phase,//input blk position 
					int proc_edge[2], int edge_clip_left[2]/*or above*/, int edge_clip_right[2]/*or below*/,
				int *alpha, int *beta, int *beta2);

void CalFilterType_rv9(uint8 left_arr[12],uint8 right_arr[12],int bStrong,
				   int beta, int beta2,
				   int *Al,int *Ar,int *filter_type);

// For real 9 dbk. 
uint8 getMBStrength(uint32 mbType) ;
int deblockRefDiff (uint32 mbType, uint32 adjMbType) ;

/************************************************************************/
/* Real 8 function                                                      */
/************************************************************************/

//void Rv8_DBK_FW_Command(RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd);
void dbk_module_rv8 (/*RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd, uint8 ** pMBYUV, uint8 **pFrm*/);

void rv8dbk_core_ctr (int		mb_x,int mb_num_x);

void DbkFilter_rv8 (
				int		line_id,
				uint8	pix_p[4], 
				uint8	pix_q[4],
				int		is_luma,
				int		Bs, 
				int		Qp, 
				int		alpha, 
				int		beta, 
				int		clip_par
				);

void Get8PixFiltering_rv8 (int line_id, uint8 pix_p[4], uint8 pix_q[4]);

void GetFourPixWrite_rv8 (int line_id, uint8 pix_write[4]);

void PixBlkExchange_rv8 ();

void PixBlkUptFilter_rv8 (
					  int		line_id,
					  uint8		p0,
					  uint8		p1,
					  uint8		p2,
					  int		p0_upt,
					  int		p1_upt,
					  int		p2_upt,
					  uint8		q0,
					  uint8		q1,
					  uint8		q2,
					  int		q0_upt,
					  int		q1_upt,
					  int		q2_upt
					  );

void PixBlkUptWrite_rv8 (
					 int	line_id, 
					 uint8	pix0,
					 uint8	pix1,
					 uint8	pix2,
					 uint8	pix3
					 );

void GetFilterPara_rv8 (
					/*input*/
					int	is_hor_filter, 
					int	filt_blk_comp, 
					int	filt_blk_y, 
					int	filt_blk_x,
					int	line_id,

					/*output*/
					int	*	Bs_ptr,
					int *	Qp_ptr,
					int *	alpha_ptr,
					int *	beta_ptr,
					int *	clip_par_ptr
					);





#endif