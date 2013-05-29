/*hvld_reg.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "common_global.h"

void NnzRegInit ()
{
	g_hvld_reg_ptr->nnz_blk_0 = 0;
	g_hvld_reg_ptr->nnz_blk_1 = 0;
	g_hvld_reg_ptr->nnz_blk_2 = 0;
	g_hvld_reg_ptr->nnz_blk_3 = 0;
	g_hvld_reg_ptr->nnz_blk_4 = 0;
	g_hvld_reg_ptr->nnz_blk_5 = 0;

	g_hvld_reg_ptr->cbp_uv	  = 0;
}

/******************************************************************
			derive nc according to block type, block id
*******************************************************************/
void GetNeighborNnz (
					 /*input*/
					 int		blk_type, 
					 int		blk_id, 
					 int		lmb_avail, 
					 int		tmb_avail, 

					 /*output*/
					 int	*	nc_ptr
					 )
{
	int		na;				//left block's nnz
	int		nb;				//top block's nnz
	int		nc;
	int		lblk_avail;		//left block available
	int		tblk_avail;		//top block available


	uint32	neighbor_nnz;
	uint32	cur_nnz;

	if (blk_type == CHROMA_DC)
	{
		nc = -1;
	}
	else if (blk_type == CHROMA_AC)
	{
		neighbor_nnz = (blk_id < 4) ? g_hvld_reg_ptr->tl_nnz_cb : g_hvld_reg_ptr->tl_nnz_cr;
		cur_nnz		 = (blk_id < 4) ? g_hvld_reg_ptr->nnz_blk_4 : g_hvld_reg_ptr->nnz_blk_5;

		blk_id = blk_id & 3;
		
		if(blk_id == 0)
		{
			na = (neighbor_nnz >> 8) & 0x1f;
			nb = (neighbor_nnz >> 24) & 0x1f;
			lblk_avail = lmb_avail;
			tblk_avail = tmb_avail;
		}
		else if (blk_id == 1)
		{
			na = (cur_nnz >> 24) & 0x1f;
			nb = (neighbor_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = tmb_avail;
		}
		else if (blk_id == 2)
		{
			na = (neighbor_nnz >> 0) & 0x1f;
			nb = (cur_nnz >> 24) & 0x1f;
			lblk_avail = lmb_avail;
			tblk_avail = 1;
		}
		else
		{
			na = (cur_nnz >> 8) & 0x1f;
			nb = (cur_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = 1;
		}
	}
	else
	{
		if (blk_type == LUMA_DC)
			blk_id = 0;

		tblk_avail = ((blk_id == 0) | (blk_id == 1) | (blk_id == 4) | (blk_id == 5)) ? tmb_avail : 1;
		lblk_avail = ((blk_id == 0) | (blk_id == 2) | (blk_id == 8) | (blk_id == 10)) ? lmb_avail : 1;
		
		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				if (blk_id == 0)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 24) & 0x1f;
				}
				else if (blk_id == 1)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 16) & 0x1f;
				}
				else if (blk_id == 2)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 4)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 8) & 0x1f;
				}
				else if (blk_id == 5)
				{
					na = (g_hvld_reg_ptr->nnz_blk_1 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 0) & 0x1f;
				}
				else if (blk_id == 6)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_1 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 16) & 0x1f;
				}				
			}
		}
		else
		{
			if (blk_id < 12)
			{
				if (blk_id == 8)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 8) & 0x1f;
				}
				else if (blk_id == 9)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 0) & 0x1f;
				}
				else if (blk_id == 10)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_2 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_2 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 12)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 8) & 0x1f;
				}
				else if (blk_id == 13)
				{
					na = (g_hvld_reg_ptr->nnz_blk_3 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 0) & 0x1f;
				}
				else if (blk_id == 14)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_3 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_3 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_3 >> 16) & 0x1f;
				}				
			}
		}		
	}

	if (blk_type != CHROMA_DC)
	{
		if (tblk_avail & lblk_avail)
		{
			nc = (na + nb + 1) >> 1;
		}
		else if (lblk_avail)
		{
			nc = na;
		}
		else if (tblk_avail)
		{
			nc = nb;
		}
		else
		{
			nc = 0;
		}
	}
	else
	{
		nc = -1;
	}

	*nc_ptr = nc;
}

void WriteBackTotalCoeff (
						  int	blk_type, 
						  int	blk_id, 
						  int	total_coeff
						  )
{
	int nnz_4blk;

	if (blk_type == LUMA_AC)
	{
		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_0;

				if (blk_id == 0)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 1)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 2)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}

				g_hvld_reg_ptr->nnz_blk_0 = nnz_4blk;
			}
			else
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_1;

				if (blk_id == 4)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 5)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 6)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_1 = nnz_4blk;
			}
		}
		else
		{
			if (blk_id < 12)
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_2;
				
				if (blk_id == 8)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 9)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 10)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_2 = nnz_4blk;
			}
			else
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_3;
				
				if (blk_id == 12)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 13)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 14)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_3 = nnz_4blk;
			}
		}
	}
	else
	{
		if (blk_id < 4)
		{
			nnz_4blk = g_hvld_reg_ptr->nnz_blk_4;
			
			if (blk_id == 0)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 1)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 2)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
			}
			
			g_hvld_reg_ptr->nnz_blk_4 = nnz_4blk;
		}
		else
		{
			
			nnz_4blk = g_hvld_reg_ptr->nnz_blk_5;
			
			if (blk_id == 4)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 5)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 6)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
			}
			
			g_hvld_reg_ptr->nnz_blk_5 = nnz_4blk;
		}
	}
}