/*hvld_global.h*/

#ifndef _HDBK_GLOBAL_H_
#define _HDBK_GLOBAL_H_

#include "hdbk_mode.h"
//#include "h264dbk_trace.h"
//#include "hdbk_test_vector.h"


// extern VSP_DBK_REG_T	*	g_dbk_reg_ptr;	


//extern uint32	*	g_vsp_mbc_out_bfr;
//extern uint32	*	g_vsp_dbk_out_bfr;
extern uint32		g_dbk_line_buf[4096];

extern uint32	*	g_frame_y_ptr;
extern uint32	*	g_frame_c_ptr;

extern uint32	*	g_frame_y_dsp_ptr;
extern uint32	*	g_frame_c_dsp_ptr;

extern int			g_blk_id;
extern int			g_mb_cnt;

void GetFourPixWrite	(int line_id, uint8 pix_write[4]);

void Get8PixFiltering	(int line_id, uint8 pix_p[4], uint8 pix_q[4]);

void PixBlkExchange		();

void PixBlkUptFilter	(
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

void PixBlkUptWrite		(
						int	line_id, 
						uint8	pix0,
						uint8	pix1,
						uint8	pix2,
						uint8	pix3
						);

void DbkFilter			(
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

void GetFilterPara		(
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

void hdbk_core_ctr (int		mb_x);

#ifdef VP8_DEC
void dbk_module(unsigned char *y, unsigned char *u, unsigned char *v,int ystride, int uv_stride);
#else
				void dbk_module();
#endif

void dbk_module_ppa (
					 char  dbk_out_buf[864],//216*4
					 char  mbc_out_buf[384],
					 int slice_info[40],
					 int dbk_para_buf[8], //8*32b
					 char decoder_format,//3b
					 char picwidthinMB,//7b
					 char picheightinMB//7b					 
					 );//weihu

void out_org444(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org411(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org411R(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org422R(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org422(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org420(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);
void out_org400(uint8 *frm_y, uint8 *frm_u, uint8 *frm_v, int32 out_width_y, int32 mcu_x, int32 mcu_y, int32 scale_down_factor);

#endif