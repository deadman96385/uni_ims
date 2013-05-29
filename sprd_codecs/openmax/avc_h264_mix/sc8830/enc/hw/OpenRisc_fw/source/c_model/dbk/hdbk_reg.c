/*dbk_reg.c*/
#include "video_common.h"
//#include "h264dec_global.h"
#include "hdbk_mode.h"
#include "hdbk_global.h"

const uint8 QP_SCALE_CR[52]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
		12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
		28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
		37,38,38,38,39,39,39,39		
};

const uint8 ALPHA_TABLE[52]  = {
	0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6, 
	7,8,9,10,12,13,15,17, 20,22,25,28,32,36,40,45, 
	50,56,63,71,80,90,101,113,127,144,162,182,203,226,255,255
} ;

const uint8  BETA_TABLE[52]  = {
	0,0,0,0,0,0,0,0,0,0,0,0, 
	0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,  
	7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14, 
	15, 15, 16, 16, 17, 17, 18, 18
} ;

 const uint8 CLIP_TAB[52][5] = 
    {
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 0, 0
        }
        , 
        {
            0, 0, 0, 1, 1
        }
        , 
        {
            0, 0, 0, 1, 1
        }
        , 
        {
            0, 0, 0, 1, 1
        }
        , 
        {
            0, 0, 0, 1, 1
        }
        , 
        {
            0, 0, 1, 1, 1
        }
        , 
        {
            0, 0, 1, 1, 1
        }
        , 
        {
            0, 1, 1, 1, 1
        }
        , 
        {
            0, 1, 1, 1, 1
        }
        , 
        {
            0, 1, 1, 1, 1
        }
        , 
        {
            0, 1, 1, 1, 1
        }
        , 
        {
            0, 1, 1, 2, 2
        }
        , 
        {
            0, 1, 1, 2, 2
        }
        , 
        {
            0, 1, 1, 2, 2
        }
        , 
        {
            0, 1, 1, 2, 2
        }
        , 
        {
            0, 1, 2, 3, 3
        }
        , 
        {
            0, 1, 2, 3, 3
        }
        , 
        {
            0, 2, 2, 3, 3
        }
        , 
        {
            0, 2, 2, 4, 4
        }
        , 
        {
            0, 2, 3, 4, 4
        }
        , 
        {
            0, 2, 3, 4, 4
        }
        , 
        {
            0, 3, 3, 5, 5
        }
        , 
        {
            0, 3, 4, 6, 6
        }
        , 
        {
            0, 3, 4, 6, 6
        }
        , 
        {
            0, 4, 5, 7, 7
        }
        , 
        {
            0, 4, 5, 8, 8
        }
        , 
        {
            0, 4, 6, 9, 9
        }
        , 
        {
            0, 5, 7, 10, 10
        }
        , 
        {
            0, 6, 8, 11, 11
        }
        , 
        {
            0, 6, 8, 13, 13
        }
        , 
        {
            0, 7, 10, 14, 14
        }
        , 
        {
            0, 8, 11, 16, 16
        }
        , 
        {
            0, 9, 12, 18, 18
        }
        , 
        {
            0, 10, 13, 20, 20
        }
        , 
        {
            0, 11, 15, 23, 23
        }
        , 
        {
            0, 13, 17, 25, 25
        }
    };

int GetBoundaryStrength (
						  int	is_hor_filter, 
						  int	filt_blk_comp, 
						  int	filt_blk_y, 
						  int	filt_blk_x,
						  int	line_id
						  )
{
	int		Bs;

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
				Bs = (filt_blk_y == 0) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 0) & 0xf) :
					 (filt_blk_y == 1) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 4) & 0xf) :
					 (filt_blk_y == 2) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 8) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_H0 >> 12) & 0xf);
			}
			else if (filt_blk_x == 2)
			{
				Bs = (filt_blk_y == 0) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 16) & 0xf) :
					 (filt_blk_y == 1) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 20) & 0xf) :
					 (filt_blk_y == 2) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 24) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_H0 >> 28) & 0xf);					 
			}
			else if (filt_blk_x == 3)
			{				
				Bs = (filt_blk_y == 0) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 0) & 0xf) :
					 (filt_blk_y == 1) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 4) & 0xf) :
					 (filt_blk_y == 2) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 8) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_H1 >> 12) & 0xf);
					
			}
			else
			{
				Bs = (filt_blk_y == 0) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 16) & 0xf) :
					 (filt_blk_y == 1) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 20) & 0xf) :
					 (filt_blk_y == 2) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 24) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_H1 >> 28) & 0xf);
			}

		}
		else
		{
			if (filt_blk_y == 0)
			{
				Bs = (filt_blk_x <  0) ? 0 :
					 (filt_blk_x == 0) ? 0 :
					 (filt_blk_x == 1) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 0) & 0xf) :
					 (filt_blk_x == 2) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 4) & 0xf) :
					 (filt_blk_x == 3) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 8) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_V0 >> 12) & 0xf);
			}
			else if (filt_blk_y == 1)
			{
				Bs = (filt_blk_x == 0) ? 0 :
					 (filt_blk_x == 1) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 16) & 0xf) :
					 (filt_blk_x == 2) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 20) & 0xf) :
					 (filt_blk_x == 3) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 24) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_V0 >> 28) & 0xf);
			}
			else if (filt_blk_y == 2)
			{
				Bs = (filt_blk_x == 0) ? 0 :
					 (filt_blk_x == 1) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 0) & 0xf) :
					 (filt_blk_x == 2) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 4) & 0xf) :
					 (filt_blk_x == 3) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 8) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_V1 >> 12) & 0xf);
			}
			else
			{
				Bs = (filt_blk_x == 0) ? 0 :
					 (filt_blk_x == 1) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 16) & 0xf) :
					 (filt_blk_x == 2) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 20) & 0xf) :
					 (filt_blk_x == 3) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 24) & 0xf) :
					 ((g_dbk_reg_ptr->HDBK_BS_V1 >> 28) & 0xf);
			}
		}
	}
	else
	{
		if (is_hor_filter)
		{
			if ((filt_blk_x == 0) || (filt_blk_x > 2))
			{
				Bs = 0;
			}
			else if (filt_blk_x == 1)
			{
				if (filt_blk_y == 0)
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 4) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_H0 >> 0) & 0xf);
				}
				else
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_H0 >> 12) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_H0 >> 8) & 0xf);
				}
			}
			else
			{
				if (filt_blk_y == 0)
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 4) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_H1 >> 0) & 0xf);
				}
				else
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_H1 >> 12) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_H1 >> 8) & 0xf);
				}
			}
		}
		else
		{
			if (filt_blk_y == 0)
			{
				if ((filt_blk_x == 0)  || (filt_blk_x > 2))
				{
					Bs = 0;
				}
				else if (filt_blk_x == 1)
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 4) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_V0 >> 0) & 0xf);
				}
				else
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_V0 >> 12) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_V0 >> 8) & 0xf);
				}
			}
			else
			{
				if ((filt_blk_x == 0) || (filt_blk_x > 2))
				{
					Bs = 0;
				}
				else if (filt_blk_x == 1)
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 4) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_V1 >> 0) & 0xf);
				}
				else
				{
					Bs = ((line_id >> 1) & 1) ? ((g_dbk_reg_ptr->HDBK_BS_V1 >> 12) & 0xf):
						 ((g_dbk_reg_ptr->HDBK_BS_V1 >> 8) & 0xf);
				}
			}
		}
	}

	return Bs;
}

int GetBlockQp (
				 int	is_hor_filter, 
				 int	filt_blk_comp, 
				 int	filt_blk_y, 
				 int	filt_blk_x
				)
{
	int		Qp_y_p;
	int		Qp_y_q;
	int		Qp_c_p;
	int		Qp_c_q;
	int		Qp_y;
	int		Qp_c;
	int		Qp;
//	int		u_qp_offset, v_qp_offset;//weihu
	int     chroma_qp_offset;


	
	Qp_y_q = (g_dbk_reg_ptr->HDBK_MB_INFO >> 0) & 0x3f;

	if (is_hor_filter)
	{
			Qp_y_p = (filt_blk_x == 0) ? 0:
					 (filt_blk_x == 1) ? ((g_dbk_reg_ptr->HDBK_MB_INFO >> 8) & 0x3f) :
					 ((g_dbk_reg_ptr->HDBK_MB_INFO >> 0) & 0x3f);
	}
	else
	{
			Qp_y_p = (filt_blk_y == 0) ? ((g_dbk_reg_ptr->HDBK_MB_INFO >> 16) & 0x3f) :
					 ((g_dbk_reg_ptr->HDBK_MB_INFO >> 0) & 0x3f);
	}

	Qp_y = (Qp_y_p + Qp_y_q + 1) / 2;

	if (filt_blk_comp == BLK_COMPONET_Y)
	{
        Qp	= Qp_y;
	}
	else
	{
#ifdef H264_DEC	
		u_qp_offset = (g_dbk_reg_ptr->HDBK_PARS >> 16) & 0x1f;
		v_qp_offset = (g_dbk_reg_ptr->HDBK_PARS >> 21) & 0x1f;
		if (filt_blk_comp == BLK_COMPONET_U)
              chroma_qp_offset = u_qp_offset;
		else
              chroma_qp_offset = v_qp_offset;
#else
        chroma_qp_offset = (g_dbk_reg_ptr->HDBK_PARS >> 16) & 0x1f;
#endif
		if ((chroma_qp_offset >> 4) & 1)    //negative
		{
			chroma_qp_offset = (0xfffffff << 5) | chroma_qp_offset;
		}	

		Qp_c_p = QP_SCALE_CR[IClip(0, 51, Qp_y_p + chroma_qp_offset)];
		Qp_c_q = QP_SCALE_CR[IClip(0, 51, Qp_y_q + chroma_qp_offset)];

		Qp_c   = (Qp_c_p + Qp_c_q + 1) / 2;
		
	    Qp   = Qp_c;
	}

	return Qp;
}

/*get filter parameter, including BS, QP and alpha, beta*/
void GetFilterPara (
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
{
	int		Bs;
	int		Qp;
	int		alpha;
	int		beta;
	int		alpha_offset;
	int		beta_offset;
	int		clip_par;
	
	
	Bs = GetBoundaryStrength (
						  is_hor_filter, 
						  filt_blk_comp, 
						  filt_blk_y, 
						  filt_blk_x,
						  line_id							
						 );
	
	Qp = GetBlockQp (
					is_hor_filter, 
					filt_blk_comp, 
					filt_blk_y, 
					filt_blk_x
					);

	alpha_offset = (g_dbk_reg_ptr->HDBK_PARS >> 8) & 0x1f;
	beta_offset  = (g_dbk_reg_ptr->HDBK_PARS >> 0) & 0x1f;

	if ((alpha_offset >> 4) & 1)    //negative
	{
		alpha_offset = (0xfffffff << 5) | alpha_offset;
	}

	if ((beta_offset >> 4) & 1)    //negative
	{
		beta_offset = (0xfffffff << 5) | beta_offset;
	}

	alpha = ALPHA_TABLE[IClip(0,51,Qp + alpha_offset)];

	beta  = BETA_TABLE[IClip(0,51,Qp + beta_offset)];

	clip_par = CLIP_TAB[IClip(0,51,Qp + alpha_offset)][Bs];

	*Bs_ptr			= Bs;
	*Qp_ptr			= Qp;
	*alpha_ptr		= alpha;
	*beta_ptr		= beta;
	*clip_par_ptr	= clip_par;
}

