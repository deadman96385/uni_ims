#include "rdbk_global.h"



void RDBKFilter(uint8 pix_p[4], uint8 pix_q[4], // pixels on left and right side of edge
				int Cl, int Cr,					// clip value left and right
				int alpha, int beta, 
				int Al, int Ar,					// para
				int filt_type,					// Filter type decision logic : 3:strong, 2:normal, 1:weak or 0:none
				int bLuma,						// whether edge is luma edge
				int edge_id, int line_id, int second_phase)		// For upt pix arr
{
	uint8	L4 = pix_p[3];
	uint8	L3 = pix_p[2];
	uint8	L2 = pix_p[1];
	uint8	L1 = pix_p[0];
	uint8	R1 = pix_q[0];
	uint8	R2 = pix_q[1];
	uint8	R3 = pix_q[2];
	uint8	R4 = pix_q[3];

	uint8	clip_value1; // (Cl+Cr+Al+Ar)/2
	uint8	clip_value2; // (Cl+Cr+Al+Ar)/4
	uint8	half_Cl = Cl>>1;
	uint8	half_Cr = Cr>>1;

	int		delta_R1_L1;
	int		abs_delta_R1_L1;
	int		abs_delta_L2_L3;
	int		abs_delta_R2_R3;
	int		N;
	uint8	clip_value;

	int		tmp_L1_sf;
	int		tmp_R1_sf;
	uint8	tmp_L2_sf;
	uint8	tmp_R2_sf;
	uint8	L1_sf;
	uint8	R1_sf;
	uint8	L2_sf;
	uint8	R2_sf;
	uint8	L3_sf;
	uint8	R3_sf;

	int		tmp_delta_nf;
	int		tmp_deltaL_nf;
	int		tmp_deltaR_nf;
	int		delta_nf;
	int		deltaL_nf;
	int		deltaR_nf;
	uint8	L1_nf;
	uint8	R1_nf;
	uint8	L2_nf;
	uint8	R2_nf;

	int		tmp_delta_wf;
	int		tmp_deltaL_wf;
	int		tmp_deltaR_wf;
	int		delta_wf;
	int		deltaL_wf;
	int		deltaR_wf;
	uint8	L1_wf;
	uint8	R1_wf;
	uint8	L2_wf;
	uint8	R2_wf;

	int		L1_upt, L1_upt_s, L1_upt_n, L1_upt_w;
	int		L2_upt, L2_upt_s, L2_upt_n, L2_upt_w;
	int		L3_upt, L3_upt_s, L3_upt_n, L3_upt_w;
	
	int		R1_upt, R1_upt_s, R1_upt_n, R1_upt_w;
	int		R2_upt, R2_upt_s, R2_upt_n, R2_upt_w;
	int		R3_upt, R3_upt_s, R3_upt_n, R3_upt_w;	

	uint8	L1_f, L2_f, L3_f;
	uint8	R1_f, R2_f, R3_f;
	
	clip_value1		= (Cr + Cl + Al + Ar) >>1;
	clip_value2		= clip_value1 >> 1;	
	
	delta_R1_L1		= R1 - L1;
	abs_delta_R1_L1 = abs(delta_R1_L1);
	abs_delta_L2_L3 = abs(L2-L3);
	abs_delta_R2_R3 = abs(R2-R3);
	N				= (abs_delta_R1_L1 * alpha) >> 7;

	

	if (filt_type == 0)
	{
		clip_value	= 0;
	}
	else if (filt_type == 1)
	{
		clip_value = (delta_R1_L1 ==0 || N>=4) ? 0 : clip_value2;
	}
	else if (filt_type == 2)
	{
		clip_value	= (delta_R1_L1 == 0 || N>=3) ? 0 : clip_value1;
	}
	else if (filt_type == 3)
	{
		clip_value = (delta_R1_L1 == 0 || N >1) ? 0: (N == 0)? 255 : clip_value1;
	}
	/************************************************************************/
	/*  Strong Filter                                                       */
	/************************************************************************/
	
	tmp_L1_sf	= ((25*L3 + 26*L2) + 26*(L1+R1) +(25*R2 + 64)) >>7;
	tmp_R1_sf	= ((25*L2 + 26*L1) + 26*(R1+R2) +(25*R3 + 64)) >>7;

	L1_sf		= Rv_Clip((L1-clip_value),(L1+clip_value),tmp_L1_sf);		
	R1_sf		= Rv_Clip((R1-clip_value),(R1+clip_value),tmp_R1_sf);

	tmp_L2_sf	= ((25*L4 + 26*L3) + (26*L2 +25*R1) +(26*L1_sf + 64)) >>7;
	tmp_R2_sf	= ((25*L1 + 26*R2) + (26*R3 +25*R4) +(26*R1_sf + 64)) >>7;

	L2_sf		= Rv_Clip((L2-clip_value),(L2+clip_value),tmp_L2_sf);
	R2_sf		= Rv_Clip((R2-clip_value),(R2+clip_value),tmp_R2_sf);

	L3_sf		= ((26*L4 + 51*L3) + 26*L2_sf + (25*L1_sf + 64)) >>7;
	R3_sf		= ((25*R1_sf + 64) + 26*R2_sf + (51*R3 + 26*R4)) >>7;

	L1_upt_s	= (clip_value != 0) ? 1: 0;
	R1_upt_s	= L1_upt_s;
	L2_upt_s	= L1_upt_s;
	R2_upt_s	= L1_upt_s;
	L3_upt_s	= L1_upt_s &&bLuma;
	R3_upt_s	= L1_upt_s &&bLuma;


	/************************************************************************/
	/*  normal filter                                                       */
	/************************************************************************/

	tmp_delta_nf	= ((delta_R1_L1<<2) + (L2-R2) +4) >>3;	
	delta_nf		= Rv_Clip(-clip_value,clip_value,tmp_delta_nf);

	L1_nf			= Rv_Clip(0,255,(L1 + delta_nf));
	R1_nf			= Rv_Clip(0,255,(R1 - delta_nf));

	tmp_deltaL_nf	= ((L2-L3) + (L2-L1) - delta_nf) >>1;
	deltaL_nf		= Rv_Clip(-Cl,Cl,tmp_deltaL_nf);

	tmp_deltaR_nf	= ((R2-R3) + (R2-R1) + delta_nf) >>1;
	deltaR_nf		= Rv_Clip(-Cr,Cr,tmp_deltaR_nf);

	L2_nf			= Rv_Clip(0,255,(L2-deltaL_nf));
	R2_nf			= Rv_Clip(0,255,(R2-deltaR_nf));

	L1_upt_n	= 1;
	R1_upt_n	= 1;
	L2_upt_n	= (clip_value !=0 &&abs_delta_L2_L3 <=beta)? 1 : 0;
	R2_upt_n	= (clip_value !=0 &&abs_delta_R2_R3 <=beta)? 1 : 0;
	L3_upt_n	= 0;
	R3_upt_n	= 0;


	/************************************************************************/
	/* weak filter                                                          */
	/************************************************************************/
	
	tmp_delta_wf	= (delta_R1_L1 +1) >>1;
	delta_wf		= Rv_Clip(-clip_value,clip_value,tmp_delta_wf);

	L1_wf			= Rv_Clip(0,255,(L1 + delta_wf));
	R1_wf			= Rv_Clip(0,255,(R1 - delta_wf));

	tmp_deltaL_wf	= ((L2-L3) + (L2-L1) - delta_wf) >>1;
	deltaL_wf		= Rv_Clip(-half_Cl,half_Cl,tmp_deltaL_wf);

	tmp_deltaR_wf	= ((R2-R3) + (R2-R1) + delta_wf) >>1;
	deltaR_wf		= Rv_Clip(-half_Cr,half_Cr,tmp_deltaR_wf);
	
	L2_wf			= Rv_Clip(0,255,(L2-deltaL_wf));
	R2_wf			= Rv_Clip(0,255,(R2-deltaR_wf));	
					
	L1_upt_w	= 1;
	R1_upt_w	= 1;
	L2_upt_w	= (clip_value !=0 &&abs_delta_L2_L3 <=beta && Al>1)? 1 : 0;
	R2_upt_w	= (clip_value !=0 &&abs_delta_R2_R3 <=beta && Ar>1)? 1 : 0;
	L3_upt_w	= 0;
	R3_upt_w	= 0;

	/************************************************************************/
	/* output and update pix array                                          */
	/************************************************************************/
	L1_f		= (filt_type == 3)? L1_sf : (filt_type == 2) ? L1_nf : L1_wf;
	L2_f		= (filt_type == 3)? L2_sf : (filt_type == 2) ? L2_nf : L2_wf;
	L3_f		= L3_sf ;
	R1_f		= (filt_type == 3)? R1_sf : (filt_type == 2) ? R1_nf : R1_wf;
	R2_f		= (filt_type == 3)? R2_sf : (filt_type == 2) ? R2_nf : R2_wf;
	R3_f		= R3_sf ;

	L1_upt		= (filt_type == 3)? L1_upt_s : (filt_type == 2)? L1_upt_n : (filt_type == 1) ? L1_upt_w : 0;
	L2_upt		= (filt_type == 3)? L2_upt_s : (filt_type == 2)? L2_upt_n : (filt_type == 1) ? L2_upt_w : 0;
	L3_upt		= (filt_type == 3)? L3_upt_s : (filt_type == 2)? L3_upt_n : (filt_type == 1) ? L3_upt_w : 0;
	R1_upt		= (filt_type == 3)? R1_upt_s : (filt_type == 2)? R1_upt_n : (filt_type == 1) ? R1_upt_w : 0;
	R2_upt		= (filt_type == 3)? R2_upt_s : (filt_type == 2)? R2_upt_n : (filt_type == 1) ? R2_upt_w : 0;
	R3_upt		= (filt_type == 3)? R3_upt_s : (filt_type == 2)? R3_upt_n : (filt_type == 1) ? R3_upt_w : 0;

	PixArrUptFilt_rv9(	L1_f, L2_f, L3_f,
						R1_f, R2_f, R3_f,
						L1_upt, L2_upt, L3_upt,
						R1_upt, R2_upt, R3_upt,
						edge_id, line_id, second_phase);

#if RDBK_TRACE_ON
	// Print filter_data.txt
	RDBK_TRACE(fp_rdbk_filter_data_trace,"line_id = %d\n",line_id);
	RDBK_TRACE(fp_rdbk_filter_data_trace,"%2x %2x %2x %2x %2x %2x %2x %2x\n",L4,L3,L2,L1,R1,R2,R3,R4);
	RDBK_TRACE(fp_rdbk_filter_data_trace,"%2x %2x %2x %2x %2x %2x\n",L3_f,L2_f,L1_f,R1_f,R2_f,R3_f);	
	RDBK_TRACE(fp_rdbk_filter_data_trace,"%d %d %d %d %d %d\n",L3_upt,L2_upt,L1_upt,R1_upt,R2_upt,R3_upt);
#endif
}