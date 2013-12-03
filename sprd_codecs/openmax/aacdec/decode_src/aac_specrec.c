/*************************************************************************
** File Name:      specrec.c                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is to do
					Spectral reconstruction:
					- grouping/sectioning
					- inverse quantization
					- applying scalefactors

**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_specrec.h"
#include "aac_filtbank.h"
#include "aac_iq_table.h"
#include "aac_ms.h"
#include "aac_is.h"
#include "aac_tns.h"
#include "aac_pns.h"
#include "aac_ltp.h"



ALIGN static const uint8 num_swb_1024_window[] =
{
    41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};

ALIGN static const uint8 num_swb_128_window[] =
{
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

ALIGN static const uint16 swb_offset_1024_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};

ALIGN static const uint16 swb_offset_128_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};/**/

ALIGN static const uint16 swb_offset_1024_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984, 1024
};

ALIGN static const uint16 swb_offset_128_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};/**/

ALIGN static const uint16 swb_offset_1024_48[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 1024
};

ALIGN static const uint16 swb_offset_128_48[] =
{
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
};

ALIGN static const uint16 swb_offset_1024_32[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992, 1024
};

ALIGN static const uint16 swb_offset_1024_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960, 1024
};

ALIGN static const uint16 swb_offset_128_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};

ALIGN static const uint16 swb_offset_1024_16[] =
{
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};

ALIGN static const uint16 swb_offset_128_16[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};

ALIGN static const uint16 swb_offset_1024_8[] =
{
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

ALIGN static const uint16 swb_offset_128_8[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
};

ALIGN static const uint16 *swb_offset_1024_window[] =
{
    swb_offset_1024_96,      /* 96000 */
    swb_offset_1024_96,      /* 88200 */
    swb_offset_1024_64,      /* 64000 */
    swb_offset_1024_48,      /* 48000 */
    swb_offset_1024_48,      /* 44100 */
    swb_offset_1024_32,      /* 32000 */
    swb_offset_1024_24,      /* 24000 */
    swb_offset_1024_24,      /* 22050 */
    swb_offset_1024_16,      /* 16000 */
    swb_offset_1024_16,      /* 12000 */
    swb_offset_1024_16,      /* 11025 */
    swb_offset_1024_8        /* 8000  */
};





ALIGN static const  uint16 *swb_offset_128_window[] =
{
    swb_offset_128_96,       /* 96000 */
    swb_offset_128_96,       /* 88200 */
    swb_offset_128_64,       /* 64000 */
    swb_offset_128_48,       /* 48000 */
    swb_offset_128_48,       /* 44100 */
    swb_offset_128_48,       /* 32000 */
    swb_offset_128_24,       /* 24000 */
    swb_offset_128_24,       /* 22050 */
    swb_offset_128_16,       /* 16000 */
    swb_offset_128_16,       /* 12000 */
    swb_offset_128_16,       /* 11025 */
    swb_offset_128_8         /* 8000  */
};




#define bit_set(A, B) ((A) & (1<<(B)))	 
						 

uint8 AAC_WindowGroupingInfoPairParsing(NeAACDecHandle hDecoder_ptr, AAC_ICS_STREAM_T *ics1, AAC_ICS_STREAM_T *ics2)
{
    uint8 i, g, tt;	
    uint8 sf_index = hDecoder_ptr->sf_index;	
    int32 frm_len;
    if (sf_index > 11)  //  sample-rate idx error!!
    {
        return 0x25;
    }
    switch (ics2->window_sequence) 
    {
    case AAC_ONLY_LONG_SEQUENCE:
    case AAC_LONG_START_SEQUENCE:
    case AAC_LONG_STOP_SEQUENCE:		
        ics1->num_windows = 1;
        ics2->num_windows = 1;
        ics1->num_window_groups = 1;
        ics2->num_window_groups = 1;
        tt = (uint8) (ics1->num_window_groups-1);
        ics1->window_group_length[tt] = 1;
        ics2->window_group_length[tt] = 1;
        tt = num_swb_1024_window[sf_index];
        ics1->num_swb = tt;
        ics2->num_swb = tt;
        ics1->sect_sfb_offset = (u16_complex120 *) swb_offset_1024_window[sf_index];
        ics2->sect_sfb_offset = (u16_complex120 *) swb_offset_1024_window[sf_index];
        //AAC_DEC_MEMCPY(ics1->sect_sfb_offset[0], swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
        //AAC_DEC_MEMCPY(ics2->sect_sfb_offset[0], swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
        AAC_DEC_MEMCPY(ics1->swb_offset, swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
        AAC_DEC_MEMCPY(ics2->swb_offset, swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
         return 0;
    case AAC_EIGHT_SHORT_SEQUENCE:
         ics1->num_windows = 8;
         ics1->num_window_groups = 1;
         ics1->window_group_length[ics1->num_window_groups-1] = 1;
         ics1->num_swb = num_swb_128_window[sf_index];
         ics2->num_windows = 8;
         ics2->num_window_groups = 1;
         ics2->window_group_length[ics2->num_window_groups-1] = 1;
         ics2->num_swb = num_swb_128_window[sf_index];
         for (i = 0; i < ics1->num_swb; i++)
        {
             uint16 tmp;
             tmp = swb_offset_128_window[sf_index][i];
             ics1->swb_offset[i] = tmp;
             ics2->swb_offset[i] = tmp;
        }
        ics1->swb_offset[ics1->num_swb] = 128;
        ics2->swb_offset[ics2->num_swb] = 128;
        for (i = 0; i < ics1->num_windows-1; i++) 
        {
            if (bit_set(ics1->scale_factor_grouping, 6-i) == 0)
            {
                uint8 t;				
                ics1->num_window_groups += 1;
                t = (uint8) (ics1->num_window_groups-1);
                ics1->window_group_length[t] = 1;
                ics2->num_window_groups += 1;
                ics2->window_group_length[t] = 1;
            } else 
           {
               uint8 t;
               t = (uint8) ( ics1->num_window_groups-1);
               ics1->window_group_length[t] += 1;
               ics2->window_group_length[t] += 1;
          }
    }		
        if (ics1->num_window_groups>8)
        {
            return 0x02;
        }
     /* preparation of sect_sfb_offset for short blocks */
        frm_len = 0;
    for (g = 0; g < ics1->num_window_groups; g++)
    {
        uint16 width;
        uint8 sect_sfb = 0;
        uint16 offset = 0;			
        for (i = 0; i < ics1->num_swb; i++)
       {
             if (i+1 == ics1->num_swb)
             {
                    width = (uint16) ((hDecoder_ptr->frameLength/8) - swb_offset_128_window[sf_index][i]);
              } else 
             {
                    width = (uint16) (swb_offset_128_window[sf_index][i+1] - swb_offset_128_window[sf_index][i]);
             }
             width = (uint16) (width *ics1->window_group_length[g]);
             ics1->sect_sfb_offset[g][sect_sfb] = offset;
             ics2->sect_sfb_offset[g][sect_sfb++] = offset;
             offset = (uint16) (offset +width);
         }
            ics1->sect_sfb_offset[g][sect_sfb] = offset;
            ics2->sect_sfb_offset[g][sect_sfb] = offset;
            frm_len += offset;
        }
        if (frm_len > 1024)
        {
            return 0x3;
        }
            return 0;
        default:
            return 1;
    }
}
uint8 AAC_WindowGroupingInfoSingleParsing(NeAACDecHandle hDecoder_ptr, AAC_ICS_STREAM_T *ics1)
{
    uint8 i, g, tt;	
    uint8 sf_index = hDecoder_ptr->sf_index;	
    int32 frm_len;
    if (sf_index > 11)  //  sample-rate idx error!!
    {
        return 0x25;
    }
    switch (ics1->window_sequence) 
    {
    case AAC_ONLY_LONG_SEQUENCE:
    case AAC_LONG_START_SEQUENCE:
    case AAC_LONG_STOP_SEQUENCE:		
        ics1->num_windows = 1;
        ics1->num_window_groups = 1;
        tt = (uint8) (ics1->num_window_groups-1);
        ics1->window_group_length[tt] = 1;
        tt = num_swb_1024_window[sf_index];
        ics1->num_swb = tt;
        ics1->sect_sfb_offset = (u16_complex120 *) swb_offset_1024_window[sf_index];
        //AAC_DEC_MEMCPY(ics1->sect_sfb_offset[0], swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
        //AAC_DEC_MEMCPY(ics2->sect_sfb_offset[0], swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
        AAC_DEC_MEMCPY(ics1->swb_offset, swb_offset_1024_window[sf_index], (ics1->num_swb+1) * sizeof(int16));
         return 0;
    case AAC_EIGHT_SHORT_SEQUENCE:
         ics1->num_windows = 8;
         ics1->num_window_groups = 1;
         ics1->window_group_length[ics1->num_window_groups-1] = 1;
         ics1->num_swb = num_swb_128_window[sf_index];
         for (i = 0; i < ics1->num_swb; i++)
        {
             uint16 tmp;
             tmp = swb_offset_128_window[sf_index][i];
             ics1->swb_offset[i] = tmp;
        }
        ics1->swb_offset[ics1->num_swb] = 128;
        for (i = 0; i < ics1->num_windows-1; i++) 
        {
            if (bit_set(ics1->scale_factor_grouping, 6-i) == 0)
            {
                uint8 t;				
                ics1->num_window_groups += 1;
                t = (uint8) (ics1->num_window_groups-1);
                ics1->window_group_length[t] = 1;
            } else 
           {
               uint8 t;
               t = (uint8) ( ics1->num_window_groups-1);
               ics1->window_group_length[t] += 1;
          }
    }		
        if (ics1->num_window_groups>8)
        {
            return 0x02;
        }
     /* preparation of sect_sfb_offset for short blocks */
        frm_len = 0;
    for (g = 0; g < ics1->num_window_groups; g++)
    {
        uint16 width;
        uint8 sect_sfb = 0;
        uint16 offset = 0;			
        for (i = 0; i < ics1->num_swb; i++)
       {
             if (i+1 == ics1->num_swb)
             {
                    width = (uint16) ((hDecoder_ptr->frameLength/8) - swb_offset_128_window[sf_index][i]);
              } else 
             {
                    width = (uint16) (swb_offset_128_window[sf_index][i+1] - swb_offset_128_window[sf_index][i]);
             }
             width = (uint16) (width *ics1->window_group_length[g]);
             ics1->sect_sfb_offset[g][sect_sfb++] = offset;
             offset = (uint16) (offset +width);
         }
            ics1->sect_sfb_offset[g][sect_sfb] = offset;
            frm_len += offset;
        }
        if (frm_len > 1024)
        {
            return 0x03;
        }
            return 0;
        default:
            return 1;
    }
}

uint8 AAC_ReconstructSingleChannel(NeAACDecHandle hDecoder_ptr,
								   AAC_ICS_STREAM_T *ics,
                                                                          AAC_ELEMENT_T *sce, 
								   int32 *tmp_spec_coef1_ptr,
								   void          *aac_dec_mem_ptr)
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    uint8  retval;
    uint8  ele = (uint8) (hDecoder_ptr->fr_ch_ele);
    int16  aac_format_sign = AAC_LC;   // 2: AAC-LC, 5: AAC+, 4: AAC_LTP
    int32   *tmp_aac_overlap;	
    if (((hDecoder_ptr->sbr_present_flag == 1) || (hDecoder_ptr->forceUpSampling == 1))  && (1 == hDecoder_ptr->aacplus_decode_flag))
    {
        aac_format_sign = AAC_SBR; // sbr 
    }
    if (hDecoder_ptr->element_alloced[hDecoder_ptr->fr_ch_ele] == 0)
    { 
        hDecoder_ptr->element_alloced[hDecoder_ptr->fr_ch_ele] = 1;
    }
    /* PNS processing */
    AAC_PnsDecode(ics, NULL, tmp_spec_coef1_ptr, NULL, hDecoder_ptr->frameLength, 0, hDecoder_ptr->object_type);    
    /* LTP processing */
    if (AAC_LTP == hDecoder_ptr->object_type)
    {
        int32 *pred_sample = aac_dec_struc_ptr->g_shared_buffer;
        aac_format_sign = AAC_LTP;
        AAC_DEC_LtpModel(tmp_spec_coef1_ptr,
                         ics,
                         hDecoder_ptr->window_shape_prev[0],
                         pred_sample,
                         pred_sample+2048,
                         
                         (int16 *)aac_dec_struc_ptr->pcm_out_l_ptr,
                         aac_dec_struc_ptr->g_aac_overlap_l);
                         
        AAC_DEC_MEMCPY(ics->ltp_data_s.last_pcm_data, aac_dec_struc_ptr->pcm_out_l_ptr, 1024 * sizeof(int16));                 
    }
#ifdef AAC_TEST_DATA
    //if (g_frm_counter > TEST_FRAME)
    {        
        FILE *fp = fopen("test_bf_tns.txt", "wb");        
        int32 i;
        for (i = 0; i < 1024; i+=1)
        {            			
               fprintf(fp, "frm: %4d, idx:%4d, left: %10d\n", g_frm_counter, i, tmp_spec_coef1_ptr[i]);
        }		
        fclose(fp);
    }
#endif
    /* tns decoding */
    AAC_TnsDecodeFrame(ics, 
                                           &(ics->tns),
                                           hDecoder_ptr->sf_index, 
                                           hDecoder_ptr->object_type,
                                           tmp_spec_coef1_ptr);    
        
    /* filter bank */
    tmp_aac_overlap = aac_dec_struc_ptr->g_aac_overlap_l;
    AAC_IfilterBank( 
                                 ics->window_sequence, 
                                  ics->window_shape,
                                  hDecoder_ptr->window_shape_prev[sce->channel], 
                                  tmp_spec_coef1_ptr,					
                                  tmp_aac_overlap,
                                  (int16 *)aac_dec_struc_ptr->pcm_out_l_ptr, 
                                  aac_format_sign,
                                  aac_dec_mem_ptr,
                                  ics->ltp_data_s.ltp_pred_sample
                                  );
#ifdef AAC_SBR_DEC
    if (AAC_SBR == aac_format_sign)
    {        
        /* after the analysis filter the data precision is S27.5 */
        if (hDecoder_ptr->ps_used[ele] == 0)
        {
            retval = sbrDecodeSingleFrame(hDecoder_ptr->sbr, 
                                                                    tmp_spec_coef1_ptr,
                                                                    hDecoder_ptr->postSeekResetFlag,
                                                                    aac_dec_mem_ptr);/**/			
           /* copy the left channel pcm data to right channel */
           AAC_DEC_MEMCPY(aac_dec_struc_ptr->pcm_out_r_ptr,  aac_dec_struc_ptr->pcm_out_l_ptr, 2048*sizeof(int16));
			
        } else 
        {
                retval = sbrDecodeSingleFramePS(hDecoder_ptr->sbr, 
                                                                      tmp_spec_coef1_ptr,  //  S16.0
                                                                      aac_dec_mem_ptr);
        }
        if (retval > 0)
            return retval;
    } 
    else  // out put the aac single channel data
#endif    
    {
        /* copy the left channel pcm data to right channel */
        AAC_DEC_MEMCPY(aac_dec_struc_ptr->pcm_out_r_ptr,  aac_dec_struc_ptr->pcm_out_l_ptr, 1024*sizeof(int16));
    }
    /* save window shape for next frame */
    hDecoder_ptr->window_shape_prev[sce->channel] = ics->window_shape;
    return 0;
}

//#define ANDROID_TEST

uint8 AAC_ReconstructChannelPair(NeAACDecHandle hDecoder_ptr, 
                                                        AAC_ICS_STREAM_T *ics1, 
                                                        AAC_ICS_STREAM_T *ics2,
                                                        AAC_ELEMENT_T *cpe_ptr, 
                                                        int32 *tmp_spec_coef1_ptr,
                                                        int32 *tmp_spec_coef2_ptr,
                                                        void   *aac_dec_mem_ptr)
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    uint8 retval;
    int16 *t_sample;
    int32 *tmp_aac_overlap;
    int16 aac_format_sign = AAC_LC;
#ifdef AAC_HE_AAC_EXTENSION
    uint8 ele = (uint8) (hDecoder_ptr->fr_ch_ele);
    if (hDecoder_ptr->element_alloced[ele] == 0)
    {
        hDecoder_ptr->sbr_alloced[ele]     = 1;		
        hDecoder_ptr->element_alloced[ele] = 1;
    }
#endif
    if (((hDecoder_ptr->sbr_present_flag == 1) || (hDecoder_ptr->forceUpSampling == 1))
        && hDecoder_ptr->sbr_alloced[ele]  && (1 == hDecoder_ptr->aacplus_decode_flag))
    {
        aac_format_sign = AAC_SBR;
    }
    /* pns decoding */
    if (ics1->ms_mask_present)
    {
        AAC_PnsDecode(ics1, ics2, tmp_spec_coef1_ptr, tmp_spec_coef2_ptr, hDecoder_ptr->frameLength, 1, hDecoder_ptr->object_type);
    } else 
    {
        AAC_PnsDecode(ics1, NULL, tmp_spec_coef1_ptr, NULL, hDecoder_ptr->frameLength, 0, hDecoder_ptr->object_type);
        AAC_PnsDecode(ics2, NULL, tmp_spec_coef2_ptr, NULL, hDecoder_ptr->frameLength, 0, hDecoder_ptr->object_type);
    }

#if 0//def GEN_TEST_DATA
    //if (g_frm_counter >= 54)
	{
		
		FILE *fp = fopen("test_iq_out0.txt", "wb");
		
		int32 i;
		for (i = 0; i < 1024; i+=2)
		{
			fprintf(fp, "frm: %4d, idx:%4d, left: %10d, %10d,  right: %10d, %10d\n", g_frm_counter, i/2, tmp_spec_coef1_ptr[i], tmp_spec_coef1_ptr[i+1], tmp_spec_coef2_ptr[i], tmp_spec_coef2_ptr[i+1]);
		}
		fclose(fp);
	}
#endif
    /* mid/side decoding */
    AAC_MsDecode(ics1, ics2, tmp_spec_coef1_ptr, tmp_spec_coef2_ptr);
    /* intensity stereo decoding */
    AAC_IsDecode(ics1, ics2, tmp_spec_coef1_ptr, tmp_spec_coef2_ptr);
    /* LTP processing */
    if (AAC_LTP == hDecoder_ptr->object_type)
    {
        int32 *pred_sample = aac_dec_struc_ptr->g_shared_buffer;
        aac_format_sign = AAC_LTP;
        /* left channel */
        AAC_DEC_LtpModel(tmp_spec_coef1_ptr,
                                           ics1,
                                           hDecoder_ptr->window_shape_prev[0],
                                           pred_sample, 
                                           pred_sample + 2048,                         
                                           (int16 *)aac_dec_struc_ptr->pcm_out_l_ptr,
                                           aac_dec_struc_ptr->g_aac_overlap_l);
        /* right channel */
        AAC_DEC_LtpModel(tmp_spec_coef2_ptr,
                                            ics2,
                                           (int16) (hDecoder_ptr->window_shape_prev[1]),
                                            pred_sample, 
                                            pred_sample + 2048,
                                           (int16 *) aac_dec_struc_ptr->pcm_out_r_ptr,
                                            aac_dec_struc_ptr->g_aac_overlap_r);            
        AAC_DEC_MEMCPY(ics1->ltp_data_s.last_pcm_data, aac_dec_struc_ptr->pcm_out_l_ptr, 1024 * sizeof(int16));
        AAC_DEC_MEMCPY(ics2->ltp_data_s.last_pcm_data, aac_dec_struc_ptr->pcm_out_r_ptr, 1024 * sizeof(int16));
    }
    
    /* tns decoding */
    AAC_TnsDecodeFrame(ics1,
                                          &(ics1->tns),
                                          hDecoder_ptr->sf_index, 
                                          hDecoder_ptr->object_type,
                                          tmp_spec_coef1_ptr);
    AAC_TnsDecodeFrame(ics2,
                                          &(ics2->tns),
                                           hDecoder_ptr->sf_index,
                                          hDecoder_ptr->object_type,
                                          tmp_spec_coef2_ptr);   

#if 0//def GEN_TEST_DATA
    //if (g_frm_counter >= 54)
	{

		FILE *fp = fopen("test_iq_out.txt", "wb");

		int32 i;
		for (i = 0; i < 1024; i+=2)
		{
			fprintf(fp, "frm: %4d, idx:%4d, left: %10d, %10d,  right: %10d, %10d\n", g_frm_counter, i/2, tmp_spec_coef1_ptr[i], tmp_spec_coef1_ptr[i+1], tmp_spec_coef2_ptr[i], tmp_spec_coef2_ptr[i+1]);
		}
		fclose(fp);
	}
#endif
#ifdef ANDROID_TEST
    {
        int16 i;
        int32 *ptr_l = (int32 *) aac_dec_struc_ptr->pcm_out_l_ptr + 2048;
        int32 *ptr_r = (int32 *) aac_dec_struc_ptr->pcm_out_r_ptr + 2048;
        for (i = 0; i < 1024; i++)
        {
            ptr_l[i] = tmp_spec_coef1_ptr[i];
            ptr_r[i] = tmp_spec_coef2_ptr[i];
        }
    }
    {
        int16 i;
        int32 *ptr_l = (int32 *) (aac_dec_struc_ptr->pcm_out_l_ptr + 2048 + 2048);
        int32 *ptr_r = (int32 *) (aac_dec_struc_ptr->pcm_out_r_ptr + 2048 + 2048);
        for (i = 0; i < 1024; i++)
        {
            ptr_l[i] = aac_dec_struc_ptr->g_aac_overlap_l[i];
            ptr_r[i] = aac_dec_struc_ptr->g_aac_overlap_r[i];
        }
    }
#endif    
    
    /* filter bank */
    tmp_aac_overlap = aac_dec_struc_ptr->g_aac_overlap_l;
    t_sample = (int16 *) aac_dec_struc_ptr->pcm_out_l_ptr;
    AAC_IfilterBank(
                             ics1->window_sequence, 
                             ics1->window_shape,
                             hDecoder_ptr->window_shape_prev[cpe_ptr->channel], 
                             tmp_spec_coef1_ptr,
                             tmp_aac_overlap,
                             t_sample,
                            aac_format_sign,
                            aac_dec_mem_ptr,
                            ics1->ltp_data_s.ltp_pred_sample
                            );
	

    t_sample = (int16 *) aac_dec_struc_ptr->pcm_out_r_ptr;
    tmp_aac_overlap = aac_dec_struc_ptr->g_aac_overlap_r;
    AAC_IfilterBank(
                            ics2->window_sequence, 
                            ics2->window_shape,
                            hDecoder_ptr->window_shape_prev[cpe_ptr->paired_channel], 
                            tmp_spec_coef2_ptr ,
                            tmp_aac_overlap,
                            t_sample,
                            aac_format_sign,
                            aac_dec_mem_ptr,
                            ics2->ltp_data_s.ltp_pred_sample
                            );
    /* save window shape for next frame */
    hDecoder_ptr->window_shape_prev[cpe_ptr->channel] = ics1->window_shape;
    hDecoder_ptr->window_shape_prev[cpe_ptr->paired_channel] = ics2->window_shape;
    
#if 0//def ANDROID_TEST
        {
        int16 i;
        int32 *ptr_l = (int32 *) (aac_dec_struc_ptr->pcm_out_l_ptr + 4096 + 2048);
            
        int32 *ptr_r = (int32 *) (aac_dec_struc_ptr->pcm_out_r_ptr + 4096 + 2048);
        for (i = 0; i < 1024; i++)
            {	
            ptr_l[i] = aac_dec_struc_ptr->g_aac_overlap_l[i];
            ptr_r[i] = aac_dec_struc_ptr->g_aac_overlap_r[i];
            }		
            
        }
#endif

#ifdef AAC_SBR_DEC
    if (AAC_SBR == aac_format_sign)  // SBR output
    {
        retval = (uint8)sbrDecodeCoupleFrame(hDecoder_ptr->sbr,     // SBR relative information
                                                                     tmp_spec_coef1_ptr,       // the left  channel data: S17.2
                                                                     tmp_spec_coef2_ptr,        // the left  channel data: S17.2
                                                                     aac_dec_mem_ptr
                                                                     );
        if (retval > 0)
            return retval;
    } 
    else if ((( hDecoder_ptr->sbr_present_flag == 1) || (hDecoder_ptr->forceUpSampling == 1)) && 
		!hDecoder_ptr->sbr_alloced[ele])
    {
        return 23;
    }
#endif
    return 0;
}

