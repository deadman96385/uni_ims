/*dbk_reg.c*/
#ifdef VP8_DEC
#include "vp8dbk_global.h"
#else
#include "sc8810_video_header.h"
#endif

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
	int		u_qp_offset,chroma_qp_offset;//weihu
	int     v_qp_offset;


	
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
		u_qp_offset = (g_dbk_reg_ptr->HDBK_PARS >> 16) & 0x1f;
		v_qp_offset = (g_dbk_reg_ptr->HDBK_PARS >> 21) & 0x1f;
		if (filt_blk_comp == BLK_COMPONET_U)
              chroma_qp_offset = u_qp_offset;
		else
              chroma_qp_offset = v_qp_offset;
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


int GetEdgeLimit(
					int	is_hor_filter, 
					int	filt_blk_comp, 
					int	filt_blk_y, 
					int	filt_blk_x,
					
					int mbflim,
					int interior_diff_limit,
					int filter_level,
					int b_1st_row,
					int b_1st_col,
					int dc_diff,
					int filter_type)
{

	int	 edge_filter_level;
	int	 edge_limit;

	if (filter_level == 0)
	{
		//disable filter if loop_filter_level equals 0.
		edge_limit = 0;
	} 
	else if ((filter_type == 1) && (filt_blk_comp != BLK_COMPONET_Y))
	{
		//disable filter for chroma edges in simple loop filter.
		edge_limit = 0;
	}
	else
	{
		if (is_hor_filter)
		{
			if (filt_blk_x == 0)
			{
				edge_limit = 0;
			}
			else if (filt_blk_x == 1)
			{
				if (b_1st_col)
				{
					edge_limit = 0;
				} 
				else
				{
					edge_filter_level = mbflim;
					edge_limit		  = (edge_filter_level << 1) + interior_diff_limit;
				}
			}
			else
			{
				if (filt_blk_comp == BLK_COMPONET_V && filt_blk_x > 2)
				{
					edge_limit = 0;
				}
				else if (dc_diff)
				{
					edge_filter_level = filter_level;
					edge_limit		  = (edge_filter_level << 1) + interior_diff_limit;
				} 
				else
				{
					edge_limit = 0;
				}
			}

		}
		else
		{
			if (filt_blk_y == 0)
			{
				if (b_1st_row)
				{
					edge_limit = 0;
				} 
				else
				{
					if (filt_blk_x == 0)
					{
						edge_limit = 0;
					} 
					else
					{
						edge_filter_level = mbflim;
						edge_limit		  = (edge_filter_level << 1) + interior_diff_limit;
					}
				}
			}
			else
			{
				if (filt_blk_x == 0)
				{
					edge_limit = 0;
				} 
				else
				{
					if (filt_blk_comp == BLK_COMPONET_V && filt_blk_x > 3)
					{
						edge_limit = 0;
					}
					else if (dc_diff)
					{
						edge_filter_level = filter_level;
						edge_limit		  = (edge_filter_level << 1) + interior_diff_limit;
					} 
					else
					{
						edge_limit = 0;
					}
				}
			}
		}
	}
	return edge_limit;	
}

int GetEdgeFilterType(
						int		 is_hor_filter,
						int		 filt_blk_comp,
						int		 filt_blk_y,
						int		 filt_blk_x,
								 
						int		 filter_type)
{
	int edge_filter_type;
	
	if (filter_type == 1)
	{
		edge_filter_type = 0;
	} 
	else
	{
		if (is_hor_filter)
		{
			edge_filter_type = (filt_blk_x == 1) ? 2 : 1;
		} 
		else
		{
			edge_filter_type = (filt_blk_y == 0) ? 2 : 1;
		}
	}

	return edge_filter_type;
}

/*get filter parameter, including filter type, edge limit, interior diff limit, HEVThr*/
#ifdef VP8_DEC
void GetFilterPara_VP8 (
					/*input*/
					int	is_hor_filter, 
					int	filt_blk_comp, 
					int	filt_blk_y, 
					int	filt_blk_x,
	
					/*output*/
					int * edge_limit_ptr,
					int * interior_diff_limit_ptr,
					int * hevthr_ptr,
					int * filter_type_ptr
					)
{	
	int  mbflim;
	int	 filter_level;
	int  edge_limit;
	int  interior_diff_limit;
	int  hevthr;
	int  filter_type; //filter type. NORMAL_LOOPFILTER = 0,SIMPLE_LOOPFILTER = 1.
	int	 b_1st_row;
	int  b_1st_col;
	int  dc_diff;
	int	 edge_filter_type; // 2'b00: Simple filter. 2'b01: subblock filter. 2'b10: MB filter
											//[31:24]	mbflim
											//[21:16]	block_inside_limint. lim
											//[13:8]	Sub block edge limit. loop filter level. flim
											//[5:4]		HEVThreshold. mbthr
											//[3]		b_1st_row. 
											//[2]		b_1st_col
											//[1]		Dc diff. 1, nonzero. 0, zero
											//[0]		filter type. NORMAL_LOOPFILTER = 0,SIMPLE_LOOPFILTER = 1.
	mbflim				= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 24)	& 0x7f;

	interior_diff_limit = (g_dbk_reg_ptr->VP8DBK_CFG0 >> 16)	& 0x3f;

	filter_level		= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 8)	& 0x3f;

	hevthr				= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 4)	& 0x3;

	b_1st_row			= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 3)	& 0x1;

	b_1st_col			= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 2)	& 0x1;

	dc_diff				= (g_dbk_reg_ptr->VP8DBK_CFG0 >> 1)	& 0x1;

	filter_type			= (g_dbk_reg_ptr->VP8DBK_CFG0 	 )	& 0x1;


	edge_limit			= GetEdgeLimit(
								 is_hor_filter, 
								 filt_blk_comp, 
								 filt_blk_y, 
								 filt_blk_x,
								 
								 mbflim,
								 interior_diff_limit,
								 filter_level,
								 b_1st_row,
								 b_1st_col,
								 dc_diff,
								 filter_type);

	edge_filter_type	= GetEdgeFilterType(
								 is_hor_filter,
								 filt_blk_comp,
								 filt_blk_y,
								 filt_blk_x,
								 
								 filter_type);


	* edge_limit_ptr			= edge_limit;
	* interior_diff_limit_ptr	= interior_diff_limit;
	* hevthr_ptr				= hevthr;
	* filter_type_ptr			= edge_filter_type;	
}
#endif