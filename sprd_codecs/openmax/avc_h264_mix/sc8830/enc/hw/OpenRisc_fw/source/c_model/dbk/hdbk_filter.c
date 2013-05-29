/*hvld_filter.c*/
#include "video_common.h"
#include "hdbk_mode.h"
#include "hdbk_global.h"

void DbkFilter (
				int		line_id,
				uint8	pix_p[4], 
				uint8	pix_q[4],
				int		is_luma,
				int		Bs, 
				int		Qp, 
				int		alpha, 
				int		beta, 
				int		clip_par
				)
{
	int			is_strong_flt;
	int			filt_ena;

	int			abs_p0_q0;
	int			abs_p1_p0;
	int			abs_q1_q0;

	int			sum_p0_q0;
	int			sum_p0_q0_p1;
	int			sum_p0_q0_q1;
	int			sum_p1_q1;
	int			sum_p1_p0;
	int			sum_q1_q0;

	int			ap;
	int			aq;
	int			alph_thr;
	int			is_ap_lt_b;
	int			is_aq_lt_b;
	int			small_gap;

	int		p0_sf0;
	int		p1_sf;
	int		p2_sf;
	int		p0_sf;
	int		p0_sf1;
	int		q0_sf0;

	int		q1_sf;
	int		q2_sf;
	int		q0_sf1;
	int		q0_sf;

	
	int			p0_upt_s;
	int			q0_upt_s;
	int			p1_upt_s;
	int			q1_upt_s;
	int			p2_upt_s;
	int			q2_upt_s;


	int			clip_c0;
	
	int			sub_q0_p0;
	int			sub_p1_q1;
	int			delta_0_tmp;
	int			delta_p1_tmp;
	int			delta_q1_tmp;
	int			delta_0;
	int			delta_p1;
	int			delta_q1;
	
	uint8		p0_wf;
	uint8		q0_wf;
	uint8		p1_wf;
	uint8		q1_wf;

	int			p0_upt_w;
	int			q0_upt_w;
	int			p1_upt_w;
	int			q1_upt_w;

	int			p0_upt;
	int			p1_upt;
	int			p2_upt;
	int			q0_upt;
	int			q1_upt;
	int			q2_upt;

	uint8		p0_f;
	uint8		q0_f;
	uint8		p1_f;
	uint8		q1_f;
	uint8		p2_f;
	uint8		q2_f;

	uint8		p3 = pix_p[3];
	uint8		p2 = pix_p[2];
	uint8		p1 = pix_p[1];
	uint8		p0 = pix_p[0];
	uint8		q0 = pix_q[0];
	uint8		q1 = pix_q[1];
	uint8		q2 = pix_q[2];
	uint8		q3 = pix_q[3];


	is_strong_flt = (Bs == 4) ? 1 : 0;

	/*compute filter condition*/
	abs_p0_q0		= abs(p0 - q0);
	abs_p1_p0		= abs(p1 - p0);
	abs_q1_q0		= abs(q1 - q0);

	filt_ena		= (Bs != 0) && (abs_p0_q0 < alpha) && (abs_p1_p0 < beta) && (abs_q1_q0 < beta);

	ap				= abs(p2 - p0);
	aq				= abs(q2 - q0);
	
	alph_thr		= (alpha >> 2) + 2;

	is_ap_lt_b		= (ap < beta) ? 1 : 0;
	is_aq_lt_b		= (aq < beta) ? 1 : 0;
	small_gap		= (abs_p0_q0 < alph_thr) ? 1 : 0;

	/*share resource*/
	sum_p0_q0		= p0 + q0;
	sum_p0_q0_p1	= sum_p0_q0 + p1;	
	sum_p0_q0_q1	= sum_p0_q0 + q1;
	
	sum_p1_q1		= p1 + q1;
	sum_p1_p0		= p1 + p0;
	sum_q1_q0		= q1 + q0;

	/****************************************************************
				strong filter of h.264 
	****************************************************************/
	p0_sf0			= (sum_p0_q0_p1*2 + p2 + q1 + 4) >> 3;
	p1_sf			= (sum_p0_q0_p1 + p2 + 2) >> 2;
	p2_sf			= (sum_p0_q0_p1 + (p3+p2)*2 + p2 + 4) >> 3;
	
	
	p0_sf1			= (sum_p1_q1 + sum_p1_p0 + 2) >> 2;
	
	p0_sf			= (is_ap_lt_b && small_gap && is_luma) ? p0_sf0 : p0_sf1;
	
	q0_sf0			= (sum_p0_q0_q1 * 2 + p1 + q2 + 4) >> 3;
	q1_sf			= (sum_p0_q0_q1 + q2 + 2) >> 2;
	q2_sf			= (sum_p0_q0_q1 + (q3+q2)* 2 + q2 + 4) >> 3;
	
	q0_sf1			= (sum_p1_q1 + sum_q1_q0 + 2) >> 2;
	
	q0_sf			= (is_aq_lt_b && small_gap && is_luma) ? q0_sf0 : q0_sf1;


	p0_upt_s		= filt_ena ? 1 : 0;
	q0_upt_s		= filt_ena ? 1 : 0;

	p1_upt_s		= (filt_ena & (is_ap_lt_b & small_gap) & is_luma) ? 1 : 0;
	q1_upt_s		= (filt_ena & (is_aq_lt_b & small_gap) & is_luma) ? 1 : 0;

	p2_upt_s		= p1_upt_s;

	q2_upt_s		= q1_upt_s;


	/****************************************************************
				weak filter of h.264 
	****************************************************************/
	clip_c0			= is_luma ? (clip_par + is_ap_lt_b + is_aq_lt_b) : (clip_par + 1);


	sub_q0_p0		= q0 - p0;
	sub_p1_q1		= p1 - q1;
	
	delta_0_tmp		= (sub_q0_p0 * 4 + sub_p1_q1 + 4) >> 3;
	delta_p1_tmp	= (p2 + (sum_p0_q0 + 1)/2 - p1*2) >> 1;
	delta_q1_tmp	= (q2 + (sum_p0_q0 + 1)/2 - q1*2) >> 1;
	

	delta_0			= IClip (-clip_c0, clip_c0, delta_0_tmp);
	delta_p1		= IClip (-clip_par, clip_par, delta_p1_tmp);
	delta_q1		= IClip (-clip_par, clip_par, delta_q1_tmp);


	p0_wf			= IClip(0, 255, p0+delta_0);
	q0_wf			= IClip(0, 255, q0-delta_0);
	p1_wf			= p1+delta_p1;
	q1_wf			= q1+delta_q1;

	
	/*derive the pixel updated signal for weak filter*/
	p0_upt_w		= filt_ena ? 1 : 0;
	q0_upt_w		= filt_ena ? 1 : 0;
	p1_upt_w		= (!filt_ena | !is_luma) ? 0 : is_ap_lt_b; 
	q1_upt_w		= (!filt_ena | !is_luma) ? 0 : is_aq_lt_b; 
	
	
	/*mux filtered pixels and updated signal of strong and weak type*/
	p0_upt			= is_strong_flt ? p0_upt_s : p0_upt_w;
	q0_upt			= is_strong_flt ? q0_upt_s : q0_upt_w;
	p1_upt			= is_strong_flt ? p1_upt_s : p1_upt_w;
	q1_upt			= is_strong_flt ? q1_upt_s : q1_upt_w;
	p2_upt			= is_strong_flt ? p2_upt_s : 0;
	q2_upt			= is_strong_flt ? q2_upt_s : 0;
	
	
	p0_f			= is_strong_flt ? p0_sf : p0_wf;
	q0_f			= is_strong_flt ? q0_sf : q0_wf;
	p1_f			= is_strong_flt ? p1_sf : p1_wf;
	q1_f			= is_strong_flt ? q1_sf : q1_wf;
	p2_f			= is_strong_flt ? p2_sf : p2;
	q2_f			= is_strong_flt ? q2_sf : q2;

	PixBlkUptFilter (
						line_id,
						p0_f,
						p1_f,
						p2_f,
						p0_upt,
						p1_upt,
						p2_upt,

						q0_f,
						q1_f,
						q2_f,
						q0_upt,
						q1_upt,
						q2_upt
				);

#if 0
	{
		/*printf upt information for verification*/
		uint32	upt_flag;
		uint32	p_f;
		uint32	q_f;
		
		upt_flag = (q2_upt << 5) | (q1_upt << 4) | (q0_upt << 3) | (p2_upt << 2) | (p1_upt << 1)  | (p0_upt << 0);
		if (upt_flag != 0)
		{
			p_f = (p2_f << 16) | (p1_f << 8) | (p0_f << 0);
			q_f = (q2_f << 16) | (q1_f << 8) | (q0_f << 0);
			
			PrintfFilterUpt (
								g_mb_cnt,
								g_blk_id,
								line_id,
								upt_flag,
								p_f,
								q_f			
								);
		}
	}
#endif
	
}