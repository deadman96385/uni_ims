/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void H264Dec_decode_one_slice_I_hw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_I: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);
	while (!end_of_slice)
	{
	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (img_ptr->curr_mb_nr >= img_ptr->frame_size_in_mbs)
		{
			img_ptr->return_pos1 |= (1<<28);
			return;
		}
	#endif	
		
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_ISlice_hw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	

	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (img_ptr->error_flag)
		{
			img_ptr->return_pos1 |= (1<<29);
			return;
		}
	#endif
	
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_mb_level_sync (img_ptr);
	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (img_ptr->error_flag)
		{
			img_ptr->return_pos2 |= (1<<0);
			return;
		}
	#endif	
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

PUBLIC void H264Dec_decode_one_slice_P_hw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_P: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);

	while (!end_of_slice)
	{		
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_PSlice_hw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
        	if (img_ptr->error_flag)
        	{
        		img_ptr->return_pos2 |= (1<<1);
         		return;
        	}
#endif
		//mvp, stand here for let MCA launch ahead.
		if (!curr_mb_info_ptr->is_intra)
		{
		#if _H264_PROTECT_ & _LEVEL_HIGH_
			if (g_list_size[0])
			{
				uint32 i;

				for (i = 0; i < g_list_size[0]; i++)
				{
					if (g_list[0][i] == PNULL || g_list[0][i]->imgY == PNULL)
					{
						img_ptr->error_flag |= ER_BSM_ID;
						img_ptr->return_pos2 |= (1<<2);
						return;
					}
				}
			}else
			{
				img_ptr->error_flag |= ER_BSM_ID;
				img_ptr->return_pos2 |= (1<<3);
				return;
			}
		#endif	
			
			H264Dec_mv_prediction_hw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}

		//vld
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}

	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (img_ptr->error_flag)
		{
			img_ptr->return_pos2 |= (1<<4);
			return;
		}
	#endif	
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
  		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);		
		H264Dec_mb_level_sync (img_ptr);	
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

PUBLIC void H264Dec_decode_one_slice_B_hw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_B: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);

	while (!end_of_slice)
	{
	#if _DEBUG_
		if ((img_ptr->mb_x == 0) && (img_ptr->mb_y == 2) && (g_nFrame_dec_h264 == 21))
		{
			foo2();
		}
	#endif

		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

		H264Dec_read_one_macroblock_BSlice_hw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

	#if _H264_PROTECT_ & _LEVEL_HIGH_
	        if (img_ptr->error_flag)
        	{
         		img_ptr->return_pos2 |= (1<<5);
          		return;
        	}
	#endif		

		//mvp, stand here for let MCA launch ahead.
		if (!curr_mb_info_ptr->is_intra)
		{
  	  		H264Dec_mv_prediction_hw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}

		//vld
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}

	#if _H264_PROTECT_ & _LEVEL_HIGH_
		if (img_ptr->error_flag)
		{
			img_ptr->return_pos2 |= (1<<6);
			return;
		}
	#endif	
		
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);		
		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		
		curr_mb_info_ptr++; 
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
