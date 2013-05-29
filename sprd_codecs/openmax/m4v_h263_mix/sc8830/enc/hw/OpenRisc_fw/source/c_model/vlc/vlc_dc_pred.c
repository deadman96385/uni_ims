/******************************************************************************
 ** File Name:    vlc_dc_pred.c                                               *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


#define DEFAULT_VALUE	1024

#define VLC_HORIZONTAL		1
#define VLC_VERTICAL		2

/*****************************************************************************************************************
tl_avail: top left mb is available (is exist and is intra)
left_avail: left mb is available;
top_avail: top mb is available;
left coeff: if left pred is available, it is stored in left_dc_y1, left_dc_y3, left_dc_u, left_dc_v;
top left coeff: if available, mb's top left coeff is stored in tl_dc_y or tl_dc_u or tl_dc_v. 

dc pred line buffer used to store one block's dc coeff, and it will be updated after one mb decoded, and 
before it be covered, dc of blk_3, blk_4, blk_5 will be back up in tl_dc register in intra mb decoding, so if 
left mb is inter, then the top left dc is still in dc_line_buffer other than in tl_dc register. so before a intra
MB decoding, tl_dc register will be updated.

dc pediction of one block line is stored in huffman table buffer, the start position is 128;
*****************************************************************************************************************/
int intra_blk_cnt = 0;

void dc_prediction (
					  /*input*/
						int		mb_x,
						int		blk_id,
						int		left_avail,
						int		top_avail,
						int		tl_avail,
						int16	curr_dc,

						/*output*/
						int *	dir_pred_ptr,
						int16 *	dc_pred_ptr
					  )
{

	/*wire and combinational reg definition*/
	int16	tl_dc_w;
	int16	left_dc_w;
	int16	top_dc_w;

	int		grad_ver;
	int		grad_hor;

	int		left_blk_avail;
	int		top_blk_avail;
	int		tl_blk_avail;

	int		vlc_tbuf_addr;
	int		vlc_tbuf_rdata;
	int		vlc_tbuf_wdata;

	/*register definition*/
	static int16	tl_dc_y = 0;
	static int16	tl_dc_u = 0;
	static int16	tl_dc_v = 0;
	static int16	left_dc_y1 = 0;
	static int16	left_dc_y3 = 0;
	static int16	left_dc_u = 0;
	static int16	left_dc_v = 0;
	static int16	dc_y_tmp;
	static int16	tl_dc_y_tmp;

#if defined(MPEG4_ENC)
	if ((g_enc_vop_mode_ptr->mb_x == 0) && (g_enc_vop_mode_ptr->mb_y == 1) && (blk_id == 1))
		printf ("");
#endif

	if ((blk_id == 0) || (blk_id == 4) || (blk_id == 5))
	{
		left_blk_avail = left_avail;
		top_blk_avail  = top_avail;
		tl_blk_avail   = tl_avail;
	}
	else if (blk_id == 1)
	{
		left_blk_avail = 1;
		top_blk_avail  = top_avail;
		tl_blk_avail   = top_avail;
	}
	else if (blk_id == 2)
	{
		left_blk_avail = left_avail;
		top_blk_avail  = 1;
		tl_blk_avail   = left_avail;
	}
	else
	{
		left_blk_avail = 1;
		top_blk_avail  = 1;
		tl_blk_avail   = 1;
	}

	if (blk_id == 0)
	{
		if (!left_blk_avail && tl_blk_avail)
		{
			vlc_tbuf_addr = 128 + (mb_x-1)*2;					//tbuff access 0
			vlc_tbuf_rdata = vsp_huff_dcac_tab[vlc_tbuf_addr];	
			tl_dc_y = (int16)(vlc_tbuf_rdata >> 16);			//tl_dc_y register update		
		}
	}

	if (blk_id == 4)
	{
		if (!left_blk_avail && tl_blk_avail)
		{
			vlc_tbuf_addr = 128 + (mb_x-1)*2 + 1;				//tbuf access 1
			vlc_tbuf_rdata = vsp_huff_dcac_tab[vlc_tbuf_addr];
			tl_dc_u = (int16)((vlc_tbuf_rdata << 16) >> 16);	//tl_dc_u and v register update
			tl_dc_v = (int16)(vlc_tbuf_rdata >> 16);			
		}
	}

	/*get top_dc and tl_dc(for blk3)*/
	vlc_tbuf_addr = (128 + mb_x * 2) + ((blk_id < 4) ? 0 : 1);		//tbuf access 2
	vlc_tbuf_rdata = vsp_huff_dcac_tab[vlc_tbuf_addr];


	if (blk_id == 0)
	{
		tl_dc_w = tl_blk_avail ? tl_dc_y : DEFAULT_VALUE;
		left_dc_w = left_blk_avail ? left_dc_y1 : DEFAULT_VALUE;
		top_dc_w = top_blk_avail ? (int16)((vlc_tbuf_rdata << 16) >> 16) : DEFAULT_VALUE;
	}
	else if (blk_id == 1)
	{
		tl_dc_w = top_blk_avail ?  (int16)((vlc_tbuf_rdata << 16) >> 16) : DEFAULT_VALUE;
		left_dc_w = left_dc_y1;
		top_dc_w = top_blk_avail ? (int16)(vlc_tbuf_rdata>> 16) : DEFAULT_VALUE;
	}
	else if (blk_id == 2)
	{
		tl_dc_w = left_blk_avail ? dc_y_tmp : DEFAULT_VALUE;
		left_dc_w = left_blk_avail ? left_dc_y3 : DEFAULT_VALUE;
		top_dc_w = (int16)((vlc_tbuf_rdata << 16) >> 16);
	}
	else if (blk_id == 3)
	{
		tl_dc_w = (int16)((vlc_tbuf_rdata << 16) >> 16);
		left_dc_w = left_dc_y3;
		top_dc_w = (int16)(vlc_tbuf_rdata>> 16);
	}
	else if (blk_id == 4)
	{
		tl_dc_w = tl_blk_avail ? tl_dc_u : DEFAULT_VALUE;
		left_dc_w = left_blk_avail ? left_dc_u : DEFAULT_VALUE;
		top_dc_w = top_blk_avail ? (int16)((vlc_tbuf_rdata << 16) >> 16) : DEFAULT_VALUE;
	}
	else
	{
		tl_dc_w = tl_blk_avail ? tl_dc_v : DEFAULT_VALUE;
		left_dc_w = left_blk_avail ? left_dc_v : DEFAULT_VALUE;
		top_dc_w = top_blk_avail ? (int16)(vlc_tbuf_rdata>> 16) : DEFAULT_VALUE;
	}


	grad_ver = (tl_dc_w >= left_dc_w) ? (tl_dc_w - left_dc_w) : (left_dc_w - tl_dc_w);
	grad_hor = (tl_dc_w >= top_dc_w)  ? (tl_dc_w - top_dc_w)  : (top_dc_w - tl_dc_w);

	*dir_pred_ptr = (grad_ver < grad_hor) ? VLC_VERTICAL : VLC_HORIZONTAL;
	*dc_pred_ptr  = (*dir_pred_ptr == VLC_VERTICAL) ? top_dc_w : left_dc_w;


	/*store current dc into tbuf for later prediction*/	
	if (blk_id == 1)
	{
		vlc_tbuf_addr  = (128 + mb_x * 2);
		vlc_tbuf_wdata =  (curr_dc << 16) | (left_dc_y1 & 0xffff);		
	}
	else if (blk_id == 3)
	{
		vlc_tbuf_addr  = (128 + mb_x * 2);
		vlc_tbuf_wdata =  (curr_dc << 16) | (left_dc_y3 & 0xffff);	
	}
	else if (blk_id == 5)
	{		
		vlc_tbuf_addr  = (128 + mb_x * 2) + 1;
		vlc_tbuf_wdata =  (curr_dc << 16) | (left_dc_u & 0xffff);	
	}

	if (blk_id & 1)
		vsp_huff_dcac_tab[vlc_tbuf_addr] = vlc_tbuf_wdata;
	
	/*register update*/

	/*left dc coeff update*/
	if ((blk_id == 0) || (blk_id == 1))
		left_dc_y1 = curr_dc;
	else if ((blk_id == 2) || (blk_id == 3))
		left_dc_y3 = curr_dc;
	else if (blk_id == 4)
		left_dc_u = curr_dc;
	else if (blk_id == 5)
		left_dc_v = curr_dc;
	

	/*top left coeff update*/ 
	if (blk_id == 1)
		tl_dc_y_tmp = top_dc_w;

	if (blk_id == 3)
		tl_dc_y = tl_dc_y_tmp;
	
	if (blk_id == 4)
		tl_dc_u = top_dc_w;

	if (blk_id == 5)
		tl_dc_v = top_dc_w;	

	/*temp register update*/
	if (blk_id == 0)
		dc_y_tmp = left_dc_w;

	intra_blk_cnt++;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 


