/*hvld_filter.c*/
#include "rdbk_mode.h"
#include "rdbk_global.h"

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

	int			delta_p1_q1;
	int			delta_p0_q0;
	int			d1;
	int			clip_strength;
	int			abs_delta;

	uint8		p0_rv8;
	uint8		q0_rv8;



	/************************************************************************/
	/*  Real 8 filter                                                       */
	/************************************************************************/
	delta_p1_q1		= p1 - q1;
	delta_p0_q0		= p0 - q0;
	clip_strength	= Bs ? Qp : 0;

	d1				= (delta_p1_q1 - (delta_p0_q0<<2)) >>3 ;
	abs_delta		= IClip(-clip_strength,clip_strength,d1);
	p0_rv8			= IClip(0,255,(p0 + abs_delta));
	q0_rv8			= IClip(0,255,(q0 - abs_delta));
	
	p0_upt			= 1;
	q0_upt			= 1;
	p1_upt			= 0;
	q1_upt			= 0;
	p2_upt			= 0;
	q2_upt			= 0;
	
	
	p0_f			= p0_rv8;
	q0_f			= q0_rv8;
	p1_f			= p1;
	q1_f			= q1;
	p2_f			= p2;
	q2_f			= q2;


	PixBlkUptFilter_rv8 (
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

	
}