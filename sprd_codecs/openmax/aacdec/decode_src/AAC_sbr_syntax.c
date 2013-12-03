/*************************************************************************
** File Name:      sbr_syntax.c                                          *
** Author:         Reed zhang                                            *
** Date:           09/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    this file is use to Calculate frequency band tables
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#ifdef AAC_SBR_DEC
#include "AAC_sbr_syntax.h"
#include "aac_syntax.h"
#include "AAC_sbr_huff.h"
#include "AAC_sbr_fbt.h"
#include "AAC_sbr_tf_grid.h"
#include "AAC_sbr_e_nf.h"
#include "aac_bits.h"
#ifdef AAC_PS_DEC
#include "AAC_ps_dec.h"
#endif

/* static function declarations */
extern const uint8 log2tab[];
static uint16 AAC_SbrExtension(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
							  AAC_SBR_INFO_T *sbr_ptr,
                              uint8 bs_extension_id, 
							  uint16 num_bits_left);
static uint8 AAC_SbrSingleChannelElement(AAC_BIT_FIFO_FORMAT_T *ld_ptr,
										  AAC_SBR_INFO_T *sbr_ptr);
static uint8 AAC_SbrChannelPairElement(AAC_BIT_FIFO_FORMAT_T *ld_ptr,
										AAC_SBR_INFO_T *sbr_ptr);

static void AAC_SinusoidalCoding(AAC_BIT_FIFO_FORMAT_T *ld_ptr,
							  AAC_SBR_INFO_T *sbr_ptr, 
							  uint8 ch);

static uint8 AAC_SbrGrid(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
						AAC_SBR_INFO_T *sbr_ptr,
						uint8 ch);
static void AAC_SbrDtdf(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
					 AAC_SBR_INFO_T *sbr_ptr, 
					 uint8 ch);

static void AAC_InvfMode(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
					  AAC_SBR_INFO_T *sbr_ptr, 
					  uint8 ch);
static void AAC_SbrHeader(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
					   AAC_SBR_INFO_T *sbr_ptr);
static uint8 AAC_SbrData(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
						AAC_SBR_INFO_T *sbr_ptr);

//////////////////////////////////////////////////////////////////////////
static void AAC_SbrReset(AAC_SBR_INFO_T *sbr_ptr)
{

    /* if these are different from the previous frame: Reset = 1 */
    if ((sbr_ptr->bs_start_freq  != sbr_ptr->bs_start_freq_prev) ||
        (sbr_ptr->bs_stop_freq   != sbr_ptr->bs_stop_freq_prev) ||
        (sbr_ptr->bs_freq_scale  != sbr_ptr->bs_freq_scale_prev) ||
        (sbr_ptr->bs_alter_scale != sbr_ptr->bs_alter_scale_prev))
    {
        sbr_ptr->Reset = 1;
    } else {
        sbr_ptr->Reset = 0;
    }

    if ((sbr_ptr->bs_start_freq  != sbr_ptr->bs_start_freq_prev) ||
        (sbr_ptr->bs_stop_freq   != sbr_ptr->bs_stop_freq_prev) ||
        (sbr_ptr->bs_freq_scale  != sbr_ptr->bs_freq_scale_prev) ||
        (sbr_ptr->bs_alter_scale != sbr_ptr->bs_alter_scale_prev) ||
        (sbr_ptr->bs_xover_band  != sbr_ptr->bs_xover_band_prev) ||
        (sbr_ptr->bs_noise_bands != sbr_ptr->bs_noise_bands_prev))
    {
        sbr_ptr->Reset = 1;
    } else {
        sbr_ptr->Reset = 0;
    }

    sbr_ptr->bs_start_freq_prev = sbr_ptr->bs_start_freq;
    sbr_ptr->bs_stop_freq_prev = sbr_ptr->bs_stop_freq;
    sbr_ptr->bs_freq_scale_prev = sbr_ptr->bs_freq_scale;
    sbr_ptr->bs_alter_scale_prev = sbr_ptr->bs_alter_scale;
    sbr_ptr->bs_xover_band_prev = sbr_ptr->bs_xover_band;
    sbr_ptr->bs_noise_bands_prev = sbr_ptr->bs_noise_bands;

    if (sbr_ptr->frame == 0)
    {
        sbr_ptr->Reset = 1;
    }
}

/* table 2 */
uint8 AAC_SbrExtensionData(AAC_BIT_FIFO_FORMAT_T *ld_ptr, AAC_SBR_INFO_T *sbr_ptr, uint16 cnt, int32 *tmp_buf_ptr)
{
    uint8 result = 0;
    int16 num_align_bits = 0;
    int16 num_sbr_bits = (int16)AAC_GetProcessedBits(ld_ptr);
    uint8 bs_extension_type = (uint8)AAC_GetBits(ld_ptr, 4);

    if (bs_extension_type == AAC_EXT_SBR_DATA_CRC)
    {
          sbr_ptr->bs_sbr_crc_bits = (uint16)AAC_GetBits(ld_ptr, 10);
    }

    sbr_ptr->bs_header_flag = AAC_Get1Bit(ld_ptr);

    if (sbr_ptr->bs_header_flag)
        AAC_SbrHeader(ld_ptr, sbr_ptr);

    /* Reset? */
    AAC_SbrReset(sbr_ptr);

    /* first frame should have a header */
    //if (!(sbr_ptr->frame == 0 && sbr_ptr->bs_header_flag == 0))
    if (sbr_ptr->header_count != 0)
    {
        if (sbr_ptr->Reset || (sbr_ptr->bs_header_flag && sbr_ptr->just_seeked))
        {
            uint8 k2, k0;
			int16 tmp;
			uint8 sr;

			sr = AAC_GetSrIndex(sbr_ptr->sample_rate);
            /* calculate the Master Frequency Table */
            k0 = AAC_QmfStartChannel(sbr_ptr->bs_start_freq,
									sbr_ptr->bs_samplerate_mode,
									sr);
			sbr_ptr->k0 = (uint8)k0;
            k2 = AAC_QmfStopChannel(sbr_ptr->bs_stop_freq,
								  sr, 
								  k0);
			tmp = k2 - k0;
            /* check k0 and k2 */
            if (sbr_ptr->sample_rate >= 48000)
            {
                if ((tmp) > 32)
                    result += 1;
            } else if (sbr_ptr->sample_rate <= 32000) {
                if ((tmp) > 48)
                    result += 1;
            } else { /* (sbr_ptr->sample_rate == 44100) */
                if ((tmp) > 45)
                    result += 1;
            }

            if (sbr_ptr->bs_freq_scale == 0)
            {
                result += AAC_MasterFrequencyTable_fs0(sbr_ptr, 
													 k0, 
													 k2,
													 sbr_ptr->bs_alter_scale,
													 tmp_buf_ptr);
            } else {
                result += AAC_MasterFrequencyTable(sbr_ptr,
												 k0, 
												 k2,
												 sbr_ptr->bs_freq_scale,
												 sbr_ptr->bs_alter_scale,
												 tmp_buf_ptr);
            }
            result += AAC_DerivedFrequencyTable(sbr_ptr,
											  sbr_ptr->bs_xover_band, 
											  k2);
  
            result = (result > 0) ? 1 : 0;
        }

        if (result == 0)
            result = AAC_SbrData(ld_ptr, sbr_ptr);
    } else {
        result = 1;
    }


    {
        num_sbr_bits = (uint16)AAC_GetProcessedBits(ld_ptr) - num_sbr_bits;
        /* -4 does not apply, bs_extension_type is re-read in this function */
        num_align_bits = 8*cnt /*- 4*/ - num_sbr_bits;

        while (num_align_bits > 7)
        {
            AAC_GetBits(ld_ptr, 8);
            num_align_bits -= 8;
        }
        AAC_GetBits(ld_ptr, num_align_bits);
    }

    return result;
}

/* table 3 */
static void AAC_SbrHeader(AAC_BIT_FIFO_FORMAT_T *ld_ptr, AAC_SBR_INFO_T *sbr_ptr)
{
    uint8 bs_header_extra_1, bs_header_extra_2;

    sbr_ptr->header_count++;

	// bs_amp_res:define the resolution of the envelope estimates as given table on pages 21 in 14496-3:2001
    sbr_ptr->bs_amp_res    = AAC_Get1Bit(ld_ptr);
    /* bs_start_freq and bs_stop_freq must define a fequency band that does
       not exceed 48 channels */
    sbr_ptr->bs_start_freq = (uint8)AAC_GetBits(ld_ptr, 4);
    sbr_ptr->bs_stop_freq  = (uint8)AAC_GetBits(ld_ptr, 4);

	// bs_xover_band: index to master frequency table
    sbr_ptr->bs_xover_band = (uint8)AAC_GetBits(ld_ptr, 3);
    AAC_GetBits(ld_ptr, 2);
	// bs_header_extra_1 & bs_header_extra_2: indicates whether the optional header part 1 & 2 is enabled.
    bs_header_extra_1  = (uint8)AAC_Get1Bit(ld_ptr);
    bs_header_extra_2  = (uint8)AAC_Get1Bit(ld_ptr);
    if (bs_header_extra_1)
    {
		// bs_freq_scale: define the envelope frequency band group as defined in table pages 21 in 14496-3:2001
        sbr_ptr->bs_freq_scale  = (uint8)AAC_GetBits(ld_ptr, 2); 
		// bs_alter_scale: further define the frequency envelope bands grouping
        sbr_ptr->bs_alter_scale = (uint8)AAC_Get1Bit(ld_ptr);
		//bs_noise_bands: defines the number of noise floor bands
        sbr_ptr->bs_noise_bands = (uint8)AAC_GetBits(ld_ptr, 2);
    } else {
        /* Default values */
        sbr_ptr->bs_freq_scale  = 2;
        sbr_ptr->bs_alter_scale = 1;
        sbr_ptr->bs_noise_bands = 2;
    }

    if (bs_header_extra_2)
    {
		// bs_limiter_bands: defines the number of limiter bands
        sbr_ptr->bs_limiter_bands  = (uint8)AAC_GetBits(ld_ptr, 2);
		// bs_limiter_gains: defines the maximum gain of the limiters
        sbr_ptr->bs_limiter_gains  = (uint8)AAC_GetBits(ld_ptr, 2);
		// bs_interpol_freq: defines if the frequency interpolation shall be applied
        sbr_ptr->bs_interpol_freq  = (uint8)AAC_Get1Bit(ld_ptr);
		// bs_smoothing_mode: defines if smoothing shall be applied 
        sbr_ptr->bs_smoothing_mode = (uint8)AAC_Get1Bit(ld_ptr);
    } else {
        /* Default values */
        sbr_ptr->bs_limiter_bands = 2;
        sbr_ptr->bs_limiter_gains = 2;
        sbr_ptr->bs_interpol_freq = 1;
        sbr_ptr->bs_smoothing_mode = 1;
    }
}

/* table 4 */
static uint8 AAC_SbrData(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
						AAC_SBR_INFO_T *sbr_ptr)
{
    uint8 result;
#if 0
    sbr_ptr->bs_samplerate_mode = AAC_Get1Bit(ld_ptr);
#endif

    sbr_ptr->rate = (sbr_ptr->bs_samplerate_mode) ? 2 : 1;

    switch (sbr_ptr->id_aac)
    {
    case AAC_ID_SCE:
		if ((result = AAC_SbrSingleChannelElement(ld_ptr,
												 sbr_ptr)) > 0)
			return result;
        break;
    case AAC_ID_CPE:
		if ((result = AAC_SbrChannelPairElement(ld_ptr, 
											   sbr_ptr)) > 0)
			return result;
        break;
    }

	return 0;
}

/* table 5 */
static uint8 AAC_SbrSingleChannelElement(AAC_BIT_FIFO_FORMAT_T *ld_ptr, AAC_SBR_INFO_T *sbr_ptr)
{
    uint8 result;
    if (AAC_Get1Bit(ld_ptr))
    {
        AAC_GetBits(ld_ptr, 4);
    }

    if ((result = AAC_SbrGrid(ld_ptr, sbr_ptr, 0)) > 0)
        return result;
    AAC_SbrDtdf(ld_ptr, sbr_ptr, 0);
    AAC_InvfMode(ld_ptr, sbr_ptr, 0);
    AAC_SbrEnvelope(ld_ptr, sbr_ptr, 0);
    AAC_SbrNoise(ld_ptr, sbr_ptr, 0);

    AAC_DEC_MEMSET(sbr_ptr->bs_add_harmonic[0], 0, 64*sizeof(uint8));

    sbr_ptr->bs_add_harmonic_flag[0] = AAC_Get1Bit(ld_ptr);
    if (sbr_ptr->bs_add_harmonic_flag[0])
        AAC_SinusoidalCoding(ld_ptr, sbr_ptr, 0);

    sbr_ptr->bs_extended_data = AAC_Get1Bit(ld_ptr);
    if (sbr_ptr->bs_extended_data)
    {
        int16 nr_bits_left;
        uint16 cnt = (uint16)AAC_GetBits(ld_ptr, 4);
        if (cnt == 15)
        {
            cnt += (uint16)AAC_GetBits(ld_ptr, 8);
        }

        nr_bits_left = 8 * cnt;
        while (nr_bits_left > 7)
        {
            sbr_ptr->bs_extension_id = (uint8)AAC_GetBits(ld_ptr, 2);
            nr_bits_left -= 2;
            nr_bits_left -= AAC_SbrExtension(ld_ptr, sbr_ptr, sbr_ptr->bs_extension_id, nr_bits_left);
        }

        /* Corrigendum */
        if (nr_bits_left > 0)
        {
            AAC_GetBits(ld_ptr, nr_bits_left);
        }
    }

    return 0;
}

/* table 6 */
static uint8 AAC_SbrChannelPairElement(AAC_BIT_FIFO_FORMAT_T *ld_ptr,   // the sbr_ptr stream information
										AAC_SBR_INFO_T *sbr_ptr) // the sbr_ptr relative parameter structure information for Huffman decoder or dequantisation.
{
	uint8 bs_add_harmonic_flag;
    uint8 n, result;
	uint8 ibs_coupling;

    if (AAC_Get1Bit(ld_ptr))
    {
        AAC_GetBits(ld_ptr, 4);
        AAC_GetBits(ld_ptr, 4);
    }

	/************************************************************************/
	/*  bs_coupling: Indicates whether the stereo information between       */
	/*               the two channels is coupled or not                     */
	/************************************************************************/
	ibs_coupling = AAC_Get1Bit(ld_ptr);
    sbr_ptr->bs_coupling = ibs_coupling;
    if (ibs_coupling) 
    {
        if ((result = AAC_SbrGrid(ld_ptr,
							   sbr_ptr, 
							   0)) > 0)
            return result;

        /* need to copy some data from left to right */
        sbr_ptr->bs_frame_class[1] = sbr_ptr->bs_frame_class[0];
        sbr_ptr->L_E[1] = sbr_ptr->L_E[0];
        sbr_ptr->L_Q[1] = sbr_ptr->L_Q[0];
        sbr_ptr->bs_pointer[1] = sbr_ptr->bs_pointer[0];

        for (n = 0; n <= sbr_ptr->L_E[0]; n++)
        {
            sbr_ptr->t_E[1][n] = sbr_ptr->t_E[0][n];
            sbr_ptr->f[1][n] = sbr_ptr->f[0][n];
        }
        for (n = 0; n <= sbr_ptr->L_Q[0]; n++)
            sbr_ptr->t_Q[1][n] = sbr_ptr->t_Q[0][n];

        AAC_SbrDtdf(ld_ptr, 
				 sbr_ptr,
				 0);
        AAC_SbrDtdf(ld_ptr, 
				 sbr_ptr,
				 1);
        AAC_InvfMode(ld_ptr,
				  sbr_ptr,
				  0);

        /* more copying */
        for (n = 0; n < sbr_ptr->N_Q; n++)
            sbr_ptr->bs_invf_mode[1][n] = sbr_ptr->bs_invf_mode[0][n];

		/************************************************************************/
		/*                     sbr_ptr envelope huffman decoder                     */
		/************************************************************************/
        AAC_SbrEnvelope(ld_ptr, 
					 sbr_ptr, 
					 0);
		/************************************************************************/
		/*                       sbr_ptr noise huffman decoder                      */
		/************************************************************************/
        AAC_SbrNoise(ld_ptr, 
				  sbr_ptr, 
				  0);
        AAC_SbrEnvelope(ld_ptr, 
					 sbr_ptr, 
					 1);
        AAC_SbrNoise(ld_ptr, 
				  sbr_ptr,
				  1);
        AAC_DEC_MEMSET(sbr_ptr->bs_add_harmonic[0], 0, 64*sizeof(uint8));
        AAC_DEC_MEMSET(sbr_ptr->bs_add_harmonic[1], 0, 64*sizeof(uint8));
		bs_add_harmonic_flag = AAC_Get1Bit(ld_ptr);
        sbr_ptr->bs_add_harmonic_flag[0] = bs_add_harmonic_flag;
        if (bs_add_harmonic_flag)
            AAC_SinusoidalCoding(ld_ptr, sbr_ptr, 0);

		/************************************************************************/
		/* bs_add_harmonic_flag: Defines whether any additional sinsoids        */
		/*                        should be used                                */
		/************************************************************************/
		bs_add_harmonic_flag = AAC_Get1Bit(ld_ptr);
        sbr_ptr->bs_add_harmonic_flag[1] = bs_add_harmonic_flag;
        if (bs_add_harmonic_flag)
            AAC_SinusoidalCoding(ld_ptr, sbr_ptr, 1);
    } else 
	{
        if ((result = AAC_SbrGrid(ld_ptr, sbr_ptr, 0)) > 0)
            return result;
        if ((result = AAC_SbrGrid(ld_ptr, sbr_ptr, 1)) > 0)
            return result;
        AAC_SbrDtdf(ld_ptr, sbr_ptr, 0);
        AAC_SbrDtdf(ld_ptr, sbr_ptr, 1);
        AAC_InvfMode(ld_ptr, sbr_ptr, 0);
        AAC_InvfMode(ld_ptr, sbr_ptr, 1);
        AAC_SbrEnvelope(ld_ptr, sbr_ptr, 0);
        AAC_SbrEnvelope(ld_ptr, sbr_ptr, 1);
        AAC_SbrNoise(ld_ptr, sbr_ptr, 0);
        AAC_SbrNoise(ld_ptr, sbr_ptr, 1);

        AAC_DEC_MEMSET(sbr_ptr->bs_add_harmonic[0], 0, 64*sizeof(uint8));
        AAC_DEC_MEMSET(sbr_ptr->bs_add_harmonic[1], 0, 64*sizeof(uint8));

        sbr_ptr->bs_add_harmonic_flag[0] = AAC_Get1Bit(ld_ptr);
        if (sbr_ptr->bs_add_harmonic_flag[0])
            AAC_SinusoidalCoding(ld_ptr, sbr_ptr, 0);

        sbr_ptr->bs_add_harmonic_flag[1] = AAC_Get1Bit(ld_ptr);
        if (sbr_ptr->bs_add_harmonic_flag[1])
            AAC_SinusoidalCoding(ld_ptr, sbr_ptr, 1);
    }
    sbr_ptr->bs_extended_data = AAC_Get1Bit(ld_ptr);
    if (sbr_ptr->bs_extended_data)
    {
        uint16 nr_bits_left;
        uint16 cnt = (uint16)AAC_GetBits(ld_ptr, 4);
        if (cnt == 15)
        {
            cnt += (uint16)AAC_GetBits(ld_ptr, 8);
        }
        nr_bits_left = 8 * cnt;
        while (nr_bits_left > 7)
        {
            sbr_ptr->bs_extension_id = (uint8)AAC_GetBits(ld_ptr, 2);
            nr_bits_left         -= 2;
            AAC_SbrExtension(ld_ptr, sbr_ptr, sbr_ptr->bs_extension_id, nr_bits_left);
        }

        /* Corrigendum */
        if (nr_bits_left > 0)
        {
            AAC_GetBits(ld_ptr, nr_bits_left);
        }
    }

    return 0;
}

/* integer log[2](x): input range [0,10) */
static uint8 AAC_SbrLog2(uint8 val)
{
    
    if (val < 10 && val > 0)
        return log2tab[val];
    else
        return 0;
}


/* table 7 */
static uint8 AAC_SbrGrid(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
						AAC_SBR_INFO_T *sbr_ptr, 
						uint8 ch)
{
    uint8 i, env, rel, result;
    uint8 bs_abs_bord, bs_abs_bord_1;
    uint8 bs_num_env = 0;
	uint8 bs_frame_class;

    bs_frame_class          = (uint8)AAC_GetBits(ld_ptr, 2);
	sbr_ptr->bs_frame_class[ch] = bs_frame_class;
    switch (bs_frame_class)
    {
    case AAC_FIXFIX:
        i          = (uint8)AAC_GetBits(ld_ptr, 2);
        bs_num_env = AAC_DEC_MIN(1 << i, 5);
        i          = (uint8)AAC_Get1Bit(ld_ptr);
        for (env = 0; env < bs_num_env; env++)
		{
            sbr_ptr->f[ch][env] = i;
		}
        sbr_ptr->abs_bord_lead[ch]  = 0;
        sbr_ptr->abs_bord_trail[ch] = sbr_ptr->numTimeSlots;
        sbr_ptr->n_rel_lead[ch]     = bs_num_env - 1;
        sbr_ptr->n_rel_trail[ch]    = 0;
        break;

    case AAC_FIXVAR:
        bs_abs_bord = (uint8)AAC_GetBits(ld_ptr, 2) + sbr_ptr->numTimeSlots;
        bs_num_env  = (uint8)AAC_GetBits(ld_ptr, 2) + 1;

        for (rel = 0; rel < bs_num_env-1; rel++)
        {
            sbr_ptr->bs_rel_bord[ch][rel] = 2 * (uint8)AAC_GetBits(ld_ptr, 2) + 2;
        }
        i                   = AAC_SbrLog2((uint8)(bs_num_env + 1));
        sbr_ptr->bs_pointer[ch] = (uint8) AAC_GetBits(ld_ptr, i);

        for (env = 0; env < bs_num_env; env++)
        {
            sbr_ptr->f[ch][bs_num_env - env - 1] = (uint8)AAC_Get1Bit(ld_ptr);
        }

        sbr_ptr->abs_bord_lead[ch]  = 0;
        sbr_ptr->abs_bord_trail[ch] = bs_abs_bord;
        sbr_ptr->n_rel_lead[ch]     = 0;
        sbr_ptr->n_rel_trail[ch]    = bs_num_env - 1;
        break;

    case AAC_VARFIX:
        bs_abs_bord = (uint8)AAC_GetBits(ld_ptr, 2);
        bs_num_env  = (uint8)AAC_GetBits(ld_ptr, 2) + 1;

        for (rel = 0; rel < bs_num_env-1; rel++)
        {
            sbr_ptr->bs_rel_bord[ch][rel] = 2 * (uint8)AAC_GetBits(ld_ptr, 2) + 2;
        }
        i                   =  AAC_SbrLog2((uint8)(bs_num_env + 1));
        sbr_ptr->bs_pointer[ch] = (uint8) AAC_GetBits(ld_ptr, i);

        for (env = 0; env < bs_num_env; env++)
        {
            sbr_ptr->f[ch][env] = (uint8)AAC_Get1Bit(ld_ptr);
        }

        sbr_ptr->abs_bord_lead[ch]  = bs_abs_bord;
        sbr_ptr->abs_bord_trail[ch] = sbr_ptr->numTimeSlots;
        sbr_ptr->n_rel_lead[ch]     = bs_num_env - 1;
        sbr_ptr->n_rel_trail[ch]    = 0;
        break;

    case AAC_VARVAR:
        bs_abs_bord           = (uint8)AAC_GetBits(ld_ptr, 2);
        bs_abs_bord_1         = (uint8)AAC_GetBits(ld_ptr, 2) + sbr_ptr->numTimeSlots;
        sbr_ptr->bs_num_rel_0[ch] = (uint8)AAC_GetBits(ld_ptr, 2);
        sbr_ptr->bs_num_rel_1[ch] = (uint8)AAC_GetBits(ld_ptr, 2);
        bs_num_env            = AAC_DEC_MIN(5, sbr_ptr->bs_num_rel_0[ch] + sbr_ptr->bs_num_rel_1[ch] + 1);

        for (rel = 0; rel < sbr_ptr->bs_num_rel_0[ch]; rel++)
        {
            sbr_ptr->bs_rel_bord_0[ch][rel] = 2 * (uint8)AAC_GetBits(ld_ptr, 2) + 2;
        }
        for(rel = 0; rel < sbr_ptr->bs_num_rel_1[ch]; rel++)
        {
            sbr_ptr->bs_rel_bord_1[ch][rel] = 2 * (uint8)AAC_GetBits(ld_ptr, 2) + 2;
        }
        i                   = AAC_SbrLog2((uint8) (sbr_ptr->bs_num_rel_0[ch] + sbr_ptr->bs_num_rel_1[ch] + 2));
        sbr_ptr->bs_pointer[ch] = (uint8) AAC_GetBits(ld_ptr, i);

        for (env = 0; env < bs_num_env; env++)
        {
            sbr_ptr->f[ch][env] = (uint8)AAC_Get1Bit(ld_ptr);
        }

        sbr_ptr->abs_bord_lead[ch]  = bs_abs_bord;
        sbr_ptr->abs_bord_trail[ch] = bs_abs_bord_1;
        sbr_ptr->n_rel_lead[ch]     = sbr_ptr->bs_num_rel_0[ch];
        sbr_ptr->n_rel_trail[ch]    = sbr_ptr->bs_num_rel_1[ch];
        break;
    }

    if (sbr_ptr->bs_frame_class[ch] == AAC_VARVAR)
        sbr_ptr->L_E[ch] = AAC_DEC_MIN(bs_num_env, 5);
    else
        sbr_ptr->L_E[ch] = AAC_DEC_MIN(bs_num_env, 4);
        
    if (sbr_ptr->L_E[ch] > 1)
        sbr_ptr->L_Q[ch] = 2;
    else
        sbr_ptr->L_Q[ch] = 1;

    /* TODO: this code can probably be integrated into the code above! */
    if ((result = AAC_EnvelopeTimeBorderVector(sbr_ptr, ch)) > 0)
        return result;
    AAC_NoiseFloorTimeBorderVector(sbr_ptr, ch);

    return 0;
}

/* table 8 */
static void AAC_SbrDtdf(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
					 AAC_SBR_INFO_T *sbr_ptr, 
					 uint8 ch)  // the channel No.
{
    uint8 i;

    for (i = 0; i < sbr_ptr->L_E[ch]; i++)   // bs_num_env
    {
        sbr_ptr->bs_df_env[ch][i] = AAC_Get1Bit(ld_ptr);
    }

    for (i = 0; i < sbr_ptr->L_Q[ch]; i++)   // bs_num_noise
    {
        sbr_ptr->bs_df_noise[ch][i] = AAC_Get1Bit(ld_ptr);
    }
}

/* table 9 */
static void AAC_InvfMode(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
					  AAC_SBR_INFO_T *sbr_ptr,
					  uint8 ch)
{
    uint8 n;

    for (n = 0; n < sbr_ptr->N_Q; n++)   // bs_num_noise_bands
    {
        sbr_ptr->bs_invf_mode[ch][n] = (uint8)AAC_GetBits(ld_ptr, 2);
    }
}

static uint16 AAC_SbrExtension(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
							  AAC_SBR_INFO_T *sbr_ptr,
                              uint8 bs_extension_id, 
							  uint16 num_bits_left)
{
	uint8 header;
	uint16 ret;
    switch (bs_extension_id)
    {
#ifdef AAC_PS_DEC  // reed modify at 2006-04-10
    case AAC_PS_EXTENSION_ID_PS:
        if (0 == sbr_ptr->ps->ps_init_sign)
        {
            AAC_PsInit(AAC_GetSrIndex(sbr_ptr->sample_rate), sbr_ptr->ps);
			sbr_ptr->ps->ps_init_sign = 1;
        }
        ret = AAC_PsData(sbr_ptr->ps, ld_ptr, &header);
		
        /* enable PS if and only if: a header has been decoded */
        if (sbr_ptr->ps_used == 0 && header == 1)
        {
            sbr_ptr->ps_used = 1;
        }
		
        return ret;
#endif

    default:
        sbr_ptr->bs_extension_data = (uint8)AAC_GetBits(ld_ptr, 6);
        return 6;
    }
}

/* table 12 */
static void AAC_SinusoidalCoding(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
							  AAC_SBR_INFO_T *sbr_ptr,
							  uint8 ch)
{
    uint8 n;

    for (n = 0; n < sbr_ptr->N_high; n++)
    {
        sbr_ptr->bs_add_harmonic[ch][n] = AAC_Get1Bit(ld_ptr);
    }
}


#endif /* AAC_SBR_DEC */
