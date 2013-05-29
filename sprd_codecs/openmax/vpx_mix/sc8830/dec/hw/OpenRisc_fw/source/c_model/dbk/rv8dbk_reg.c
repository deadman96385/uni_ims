/*dbk_reg.c*/

#include "rdbk_mode.h"
#include "rdbk_global.h"
#include "common_global.h"


int GetBoundaryStrength_rv8 (
						  int	is_hor_filter, 
						  int	filt_blk_comp, 
						  int	filt_blk_y, 
						  int	filt_blk_x,
						  int	line_id
						  )
{
	int		Bs;
	int32	CBP_topleft;
	int32	CBP_top;
	int32	CBP_left;
	int32	CBP_current;
	int32	Y_expand;
	int32	U_expand;
	int32	V_expand;

	int		mb_num_x;
	int		mb_x;
	int		mb_y;

	int		bOnleftedge;
	int		bOnrightedge;
	int		bOntopedge;

	int		bIntraPic;

	CBP_topleft	= g_dbk_reg_ptr->HDBK_BS_V1 & 0xffffff;
	CBP_top		= g_dbk_reg_ptr->HDBK_BS_V0	& 0xffffff;
	CBP_left	= g_dbk_reg_ptr->HDBK_BS_H1	& 0xffffff;
	CBP_current	= g_dbk_reg_ptr->HDBK_BS_H0	& 0xffffff;

	mb_num_x	= ((g_glb_reg_ptr->VSP_CFG1) & 0xff);//?
	mb_x		= (g_glb_reg_ptr->VSP_CTRL0) & 0x7f;//weihu for 1080p//1f
	mb_y		= (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;//weihu for 1080p//1f

	bIntraPic	= (g_dbk_reg_ptr->DBK_CFG >>  4) & 0x1 ;
	bOnleftedge	= (mb_x == 0) ;
	bOnrightedge= (mb_x == mb_num_x -1);
	bOntopedge	= (mb_y == 0) ;
	

	
	Y_expand	= (((CBP_current >> 12) & 0xf) << 21) | (((CBP_left >> 15) & 0x1) << 20)
			    | (((CBP_current >> 8 ) & 0xf) << 16) | (((CBP_left >> 11) & 0x1) << 15)
				| (((CBP_current >> 4 ) & 0xf) << 11) | (((CBP_left >> 7 ) & 0x1) << 10)
				| (((CBP_current >> 0 ) & 0xf) << 6 ) | (((CBP_left >> 3 ) & 0x1) << 5)
				| (((CBP_top	 >> 12) & 0xf) << 1 ) | (((CBP_topleft >> 15 ) & 0x1) << 0);

	U_expand	= (((CBP_current >> 18) & 0x3) << 7) | (((CBP_left >> 19) & 0x1) << 6)
				| (((CBP_current >> 16) & 0x3) << 4) | (((CBP_left >> 17) & 0x1) << 3)
				| (((CBP_top	 >> 18) & 0x3) << 1) | (((CBP_topleft >> 19 ) & 0x1) << 0);
		
	V_expand	= (((CBP_current >> 22) & 0x3) << 7) | (((CBP_left >> 23) & 0x1) << 6)
				| (((CBP_current >> 20) & 0x3) << 4) | (((CBP_left >> 21) & 0x1) << 3)
				| (((CBP_top	 >> 22) & 0x3) << 1) | (((CBP_topleft >> 23 ) & 0x1) << 0);


	if (filt_blk_comp == BLK_COMPONET_Y)
	{
		if (is_hor_filter)
		{
			if (filt_blk_x == 0)
			{
				Bs = 0;
			}
			else if (filt_blk_x == 1)
			{
				Bs = bOnleftedge ? 0 :
					 (filt_blk_y == 0) ? (((Y_expand >>  5) & 0x1) | ((Y_expand >>  6) & 0x1))://h0
					 (filt_blk_y == 1) ? (((Y_expand >> 10) & 0x1) | ((Y_expand >> 11) & 0x1))://h1
					 (filt_blk_y == 2) ? (((Y_expand >> 15) & 0x1) | ((Y_expand >> 16) & 0x1))://h2
					 (((Y_expand >> 20) & 0x1) | ((Y_expand >> 21) & 0x1));					   //h3
			}
			else if (filt_blk_x == 2)
			{
				Bs = (filt_blk_y == 0) ? (((Y_expand >>  6) & 0x1) | ((Y_expand >>  7) & 0x1))://h4
					 (filt_blk_y == 1) ? (((Y_expand >> 11) & 0x1) | ((Y_expand >> 12) & 0x1))://h5
					 (filt_blk_y == 2) ? (((Y_expand >> 16) & 0x1) | ((Y_expand >> 17) & 0x1))://h6
					 (((Y_expand >> 21) & 0x1) | ((Y_expand >> 22) & 0x1));					   //h7
			}
			else if (filt_blk_x == 3)
			{				
				Bs = (filt_blk_y == 0) ? (((Y_expand >>  7) & 0x1) | ((Y_expand >>  8) & 0x1))://h8
					 (filt_blk_y == 1) ? (((Y_expand >> 12) & 0x1) | ((Y_expand >> 13) & 0x1))://h9
					 (filt_blk_y == 2) ? (((Y_expand >> 17) & 0x1) | ((Y_expand >> 18) & 0x1))://h10
					 (((Y_expand >> 22) & 0x1) | ((Y_expand >> 23) & 0x1));					   //h11
					
			}
			else
			{
				Bs = (filt_blk_y == 0) ? (((Y_expand >>  8) & 0x1) | ((Y_expand >>  9) & 0x1))://h12
					 (filt_blk_y == 1) ? (((Y_expand >> 13) & 0x1) | ((Y_expand >> 14) & 0x1))://h13
					 (filt_blk_y == 2) ? (((Y_expand >> 18) & 0x1) | ((Y_expand >> 19) & 0x1))://h14
					 (((Y_expand >> 23) & 0x1) | ((Y_expand >> 24) & 0x1));					   //h15
			}

		}
		else
		{
			if (filt_blk_y == 0)
			{
				Bs = bOntopedge ? 0 :
					 (filt_blk_x == 0) ? (((Y_expand >>  0) & 0x1) | ((Y_expand >>  5) & 0x1))://v0
					 (filt_blk_x == 1) ? (((Y_expand >>  1) & 0x1) | ((Y_expand >>  6) & 0x1))://v1
					 (filt_blk_x == 2) ? (((Y_expand >>  2) & 0x1) | ((Y_expand >>  7) & 0x1))://v2
					 (filt_blk_x == 3) ? (((Y_expand >>  3) & 0x1) | ((Y_expand >>  8) & 0x1))://v3
					 bOnrightedge ? (((Y_expand >>  4) & 0x1) | ((Y_expand >>  9) & 0x1)) : 0 ;//L0
			}
			else if (filt_blk_y == 1)
			{
				Bs = (filt_blk_x == 0) ? (((Y_expand >>  5) & 0x1) | ((Y_expand >> 10) & 0x1))://v4
					 (filt_blk_x == 1) ? (((Y_expand >>  6) & 0x1) | ((Y_expand >> 11) & 0x1))://v5
					 (filt_blk_x == 2) ? (((Y_expand >>  7) & 0x1) | ((Y_expand >> 12) & 0x1))://v6
					 (filt_blk_x == 3) ? (((Y_expand >>  8) & 0x1) | ((Y_expand >> 13) & 0x1))://v7
					 bOnrightedge ? (((Y_expand >>  9) & 0x1) | ((Y_expand >> 14) & 0x1)) : 0 ;//L1
			}
			else if (filt_blk_y == 2)
			{
				Bs = (filt_blk_x == 0) ? (((Y_expand >> 10) & 0x1) | ((Y_expand >> 15) & 0x1))://v8
					 (filt_blk_x == 1) ? (((Y_expand >> 11) & 0x1) | ((Y_expand >> 16) & 0x1))://v9
					 (filt_blk_x == 2) ? (((Y_expand >> 12) & 0x1) | ((Y_expand >> 17) & 0x1))://v10
					 (filt_blk_x == 3) ? (((Y_expand >> 13) & 0x1) | ((Y_expand >> 18) & 0x1))://v11
					 bOnrightedge ? (((Y_expand >> 14) & 0x1) | ((Y_expand >> 19) & 0x1)) : 0 ;//L2
			}
			else
			{
				Bs = (filt_blk_x == 0) ? (((Y_expand >> 15) & 0x1) | ((Y_expand >> 20) & 0x1))://v12
					 (filt_blk_x == 1) ? (((Y_expand >> 16) & 0x1) | ((Y_expand >> 21) & 0x1))://v13
					 (filt_blk_x == 2) ? (((Y_expand >> 17) & 0x1) | ((Y_expand >> 22) & 0x1))://v14
					 (filt_blk_x == 3) ? (((Y_expand >> 18) & 0x1) | ((Y_expand >> 23) & 0x1))://v15
					 bOnrightedge ? (((Y_expand >> 19) & 0x1) | ((Y_expand >> 24) & 0x1)) : 0 ;//L3
			}
		}
	}
	else if (filt_blk_comp == BLK_COMPONET_U)
	{
		if (bIntraPic)
		{

			if (is_hor_filter)
			{
				if (filt_blk_x == 0)
				{
					Bs = 0;
				}
				else if (filt_blk_x == 1)
				{
					Bs = bOnleftedge ? 0 :
						 (filt_blk_y == 0) ? (((U_expand >>  3) & 0x1) | ((U_expand >>  4) & 0x1)): //h0
						 (((U_expand >>  6) & 0x1) | ((U_expand >>  7) & 0x1));						//h1
				}
				else
				{
					Bs = (filt_blk_y == 0) ? (((U_expand >>  4) & 0x1) | ((U_expand >>  5) & 0x1)): //h2
						 (((U_expand >>  7) & 0x1) | ((U_expand >>  8) & 0x1));						//h3
				}
			}
			else
			{
				if (filt_blk_y == 0)
				{
					Bs = bOntopedge ? 0 : 
						 (filt_blk_x == 0) ? (((U_expand >>  0) & 0x1) | ((U_expand >>  3) & 0x1)): //v0
						 (filt_blk_x == 1) ? (((U_expand >>  1) & 0x1) | ((U_expand >>  4) & 0x1)): //v1
						 bOnrightedge ? (((U_expand >>  2) & 0x1) | ((U_expand >>  5) & 0x1)) : 0 ; //L0
				}
				else
				{
					Bs = (filt_blk_x == 0) ? (((U_expand >>  3) & 0x1) | ((U_expand >>  6) & 0x1)): //v2
						 (filt_blk_x == 1) ? (((U_expand >>  4) & 0x1) | ((U_expand >>  7) & 0x1)): //v3
						 bOnrightedge ? (((U_expand >>  5) & 0x1) | ((U_expand >>  8) & 0x1)) : 0 ; //L1
				}
			}

		} 
		else
		{
			Bs = 0 ;
		}

	}
	else // BLK_COMPONET_V
	{
		if (bIntraPic)
		{

			if (is_hor_filter)
			{
				if (filt_blk_x == 0 || filt_blk_x >2)
				{
					Bs = 0;
				}
				else if (filt_blk_x == 1)
				{
					Bs = bOnleftedge ? 0 :
						 (filt_blk_y == 0) ? (((V_expand >>  3) & 0x1) | ((V_expand >>  4) & 0x1)): //h0
						 (((V_expand >>  6) & 0x1) | ((V_expand >>  7) & 0x1));						//h1
				}
				else
				{
					Bs = (filt_blk_y == 0) ? (((V_expand >>  4) & 0x1) | ((V_expand >>  5) & 0x1)): //h2
						 (((V_expand >>  7) & 0x1) | ((V_expand >>  8) & 0x1));						//h3
				}
			}
			else
			{
				if (filt_blk_x >2)
				{
					Bs = 0;
				}
				else if (filt_blk_y == 0)
				{
					Bs = bOntopedge ? 0 : 
						 (filt_blk_x == 0) ? (((V_expand >>  0) & 0x1) | ((V_expand >>  3) & 0x1)): //v0
						 (filt_blk_x == 1) ? (((V_expand >>  1) & 0x1) | ((V_expand >>  4) & 0x1)): //v1
						 bOnrightedge ? (((V_expand >>  2) & 0x1) | ((V_expand >>  5) & 0x1)) : 0 ; //L0
				}
				else
				{
					Bs = (filt_blk_x == 0) ? (((V_expand >>  3) & 0x1) | ((V_expand >>  6) & 0x1)): //v2
						 (filt_blk_x == 1) ? (((V_expand >>  4) & 0x1) | ((V_expand >>  7) & 0x1)): //v3
						 bOnrightedge ? (((V_expand >>  5) & 0x1) | ((V_expand >>  8) & 0x1)) : 0 ; //L1
				}
			}

		} 
		else
		{
			Bs = 0;
		}

	}


	return Bs;
}


/*get filter parameter, including BS, QP and alpha, beta*/
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
					)
					// useful for real 8:
					// Bs: whether to filter. 1 bit : 0 no 1 yes
					// Qp: actually the strength 3 bit which has been already look up from table through QP 
{
	int		Bs;
	int		Qp;
	int		alpha;
	int		beta;
	int		alpha_offset;
	int		beta_offset;
	int		clip_par;
	
	
	Bs = GetBoundaryStrength_rv8 (
						  is_hor_filter, 
						  filt_blk_comp, 
						  filt_blk_y, 
						  filt_blk_x,
						  line_id							
						 );

	Qp	   = (g_dbk_reg_ptr->HDBK_MB_INFO) & 0x7;

	alpha = beta = clip_par = 0;

	*Bs_ptr			= Bs;
	*Qp_ptr			= Qp;
	*alpha_ptr		= alpha;
	*beta_ptr		= beta;
	*clip_par_ptr	= clip_par;
}

