/*************************************************************************
** File Name:      syntax.c                                              *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is to do Reads the AAC bitstream as          *
**                 defined in 14496-3 (MPEG-4 Audio)					 *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_syntax.h"
#include "aac_specrec.h"
#include "aac_huffman.h"





/* Table 4.4.4 and */
/* Table 4.4.9 */
/* static function declarations */
static int16 AAC_DecodeSceLfe(NeAACDecHandle hDecoder_ptr,
						  
						   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                           uint8 id_syn_ele,
						   void           *aac_dec_mem_ptr,
						   AAC_ELEMENT_T                 *tmp_cpe_ptr);
static int16 AAC_DecodeCpe(NeAACDecHandle hDecoder_ptr, 
					   
					   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                       uint8 id_syn_ele,
					   void           *aac_dec_mem_ptr,
					   AAC_ELEMENT_T   *tmp_cpe_ptr);
static uint8 AAC_SingleLfeChannelElement(NeAACDecHandle hDecoder_ptr, 
										  AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                          uint8 channel,
										  uint8 *tag,
										  void           *aac_dec_mem_ptr,
										  AAC_ELEMENT_T                 *tmp_cpe_ptr);
static uint8 AAC_ChannelPairElement(NeAACDecHandle hDecoder_ptr, 
									AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                    uint8 channel, 
									uint8 *tag,
									void           *aac_dec_mem_ptr,
									AAC_ELEMENT_T   *tmp_cpe_ptr);
static uint16 AAC_DataStreamElement(AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static uint8 AAC_ProgramConfigEement(AAC_PROGRAM_CONFIG_T *pce, 
									  AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static uint8 AAC_FillElement(NeAACDecHandle hDecoder_ptr, 
							AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static uint8 AAC_IndividualChannelStream(NeAACDecHandle         hDecoder_ptr, 
										 AAC_ELEMENT_T         *ele_ptr,
                                         AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr, 
										 AAC_ICS_STREAM_T      *ics_ptr,
                                         int32                 *spec_data_ptr,
                                         void                  *aac_dec_mem_ptr);
static uint8 AAC_IcsInfoPairParse(NeAACDecHandle hDecoder_ptr, 
						  AAC_ICS_STREAM_T *ics1, 
						  AAC_ICS_STREAM_T *ics2, 
						  AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
						  uint8 common_window);

static uint8 AAC_SectionData(NeAACDecHandle hDecoder_ptr, 
							AAC_ICS_STREAM_T *ics,
							AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);

static uint8 AAC_SpectralDataParsing(AAC_ICS_STREAM_T *ics, 
							 AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                             int32 *spectral_data,
                             int32   *tmp_buf_ptr);
static uint8 AAC_HuffPulseData(AAC_ICS_STREAM_T *ics,
						  AAC_PULSE_INFO_T *pul, 
						  AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static void AAC_TnsDataParsing(AAC_ICS_STREAM_T *ics, 
					 AAC_TNS_INFO_T *tns, 
					 AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static uint8 AAC_AdtsFixedHeaderParsing(AAC_ADTS_HEADER_T *adts,
								 AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static void AAC_AdtsVariableHeaderParsing(AAC_ADTS_HEADER_T *adts,
								 AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
static void AAC_AdtsErrorCheck(AAC_ADTS_HEADER_T *adts,
							 AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr);
#ifdef AAC_SBR_DEC
static uint8 AAC_SBRFillElement(NeAACDecHandle hDecoder_ptr, 
							   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,   
							   uint8 t,
							   int32  *tmp_ptr);


static uint16 AAC_ExtensionPayload(AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr, 
									 uint16 count);
									 
#endif									 
extern uint32 AAC_GetSampleRate(const uint8 sr_index);


static uint8 AAC_ProgramConfigEement(AAC_PROGRAM_CONFIG_T *pce_ptr, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    uint8 i;

    AAC_DEC_MEMSET(pce_ptr, 0, sizeof(AAC_PROGRAM_CONFIG_T));

    pce_ptr->channels = 0;

    pce_ptr->element_instance_tag = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

    pce_ptr->profile = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
    pce_ptr->sf_idx  = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    pce_ptr->num_front_channel_elements = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    pce_ptr->num_side_channel_elements  = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    pce_ptr->num_back_channel_elements  = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    pce_ptr->num_lfe_channel_elements   = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
    pce_ptr->num_assoc_data_elements    = (uint8)AAC_GetBits(bit_fifo_ptr, 3);
    pce_ptr->num_valid_cc_elements      = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

    pce_ptr->mono_mixdown_present = AAC_Get1Bit(bit_fifo_ptr);
    if (pce_ptr->mono_mixdown_present == 1)
    {
        pce_ptr->mono_mixdown_element_number = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    }

    pce_ptr->stereo_mixdown_present = AAC_Get1Bit(bit_fifo_ptr);
    if (pce_ptr->stereo_mixdown_present == 1)
    {
        pce_ptr->stereo_mixdown_element_number = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    }

    pce_ptr->matrix_mixdown_idx_present = AAC_Get1Bit(bit_fifo_ptr);
    if (pce_ptr->matrix_mixdown_idx_present == 1)
    {
        pce_ptr->matrix_mixdown_idx = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
        pce_ptr->pseudo_surround_enable = AAC_Get1Bit(bit_fifo_ptr);
    }

    for (i = 0; i < pce_ptr->num_front_channel_elements; i++)
    {
        pce_ptr->front_element_is_cpe[i] = AAC_Get1Bit(bit_fifo_ptr);
        pce_ptr->front_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

        if (pce_ptr->front_element_is_cpe[i] & 1)
        {
            pce_ptr->cpe_channel[pce_ptr->front_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->num_front_channels += 2;
            pce_ptr->channels += 2;
        } else {
            pce_ptr->sce_channel[pce_ptr->front_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->num_front_channels++;
            pce_ptr->channels++;
        }
    }

    for (i = 0; i < pce_ptr->num_side_channel_elements; i++)
    {
        pce_ptr->side_element_is_cpe[i] = AAC_Get1Bit(bit_fifo_ptr);
        pce_ptr->side_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

        if (pce_ptr->side_element_is_cpe[i] & 1)
        {
            pce_ptr->cpe_channel[pce_ptr->side_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->num_side_channels += 2;
            pce_ptr->channels += 2;
        } else {
            pce_ptr->sce_channel[pce_ptr->side_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->num_side_channels++;
            pce_ptr->channels++;
        }
    }

    for (i = 0; i < pce_ptr->num_back_channel_elements; i++)
    {
        pce_ptr->back_element_is_cpe[i] = AAC_Get1Bit(bit_fifo_ptr);
        pce_ptr->back_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

        if (pce_ptr->back_element_is_cpe[i] & 1)
        {
            pce_ptr->cpe_channel[pce_ptr->back_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->channels += 2;
            pce_ptr->num_back_channels += 2;
        } else {
            pce_ptr->sce_channel[pce_ptr->back_element_tag_select[i]] = pce_ptr->channels;
            pce_ptr->num_back_channels++;
            pce_ptr->channels++;
        }
    }

    for (i = 0; i < pce_ptr->num_lfe_channel_elements; i++)
    {
        pce_ptr->lfe_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

        pce_ptr->sce_channel[pce_ptr->lfe_element_tag_select[i]] = pce_ptr->channels;
        pce_ptr->num_lfe_channels++;
        pce_ptr->channels++;
    }

    for (i = 0; i < pce_ptr->num_assoc_data_elements; i++)
        pce_ptr->assoc_data_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);

    for (i = 0; i < pce_ptr->num_valid_cc_elements; i++)
    {
        pce_ptr->cc_element_is_ind_sw[i] = AAC_Get1Bit(bit_fifo_ptr);
        pce_ptr->valid_cc_element_tag_select[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    }

    AAC_ByteAlign(bit_fifo_ptr);

    pce_ptr->comment_field_bytes = (uint8)AAC_GetBits(bit_fifo_ptr, 8);

    for (i = 0; i < pce_ptr->comment_field_bytes; i++)
    {
        pce_ptr->comment_field_data[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 8);
    }
    pce_ptr->comment_field_data[i] = 0;

    if (pce_ptr->channels > AAC_DEC_MAX_CHANNEL)
        return AAC_ProgramConfigEement_ERROR;

    return 0;
}
/* Table 4.4.1 */
int8 GASpecificConfig(AAC_BIT_FIFO_FORMAT_T *ld,
                        AAC_mp4AudioSpecificConfig *mp4ASC,
                        AAC_PROGRAM_CONFIG_T *pce_out)
{
    AAC_PROGRAM_CONFIG_T pce;
    /* 1024 or 960 */
    mp4ASC->frameLengthFlag = AAC_Get1Bit(ld);
    mp4ASC->dependsOnCoreCoder = AAC_Get1Bit(ld);
    if (mp4ASC->dependsOnCoreCoder == 1)
    {
        mp4ASC->coreCoderDelay = (uint16)AAC_GetBits(ld, 14);
    }
    mp4ASC->extensionFlag = AAC_Get1Bit(ld );
    if (mp4ASC->channelsConfiguration == 0)
    {
        if (AAC_ProgramConfigEement(&pce, ld))
            return -3;
        //mp4ASC->channelsConfiguration = pce.channels;
        if (pce_out != NULL)
            AAC_DEC_MEMCPY(pce_out, &pce, sizeof(AAC_PROGRAM_CONFIG_T));
   }
    return 0;
}
static int16 AAC_DecodeSceLfe(NeAACDecHandle hDecoder_ptr,                           
						   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                           uint8 id_syn_ele,
						   void           *aac_dec_mem_ptr,
						   AAC_ELEMENT_T                 *tmp_cpe_ptr)
{
    uint8 channels = hDecoder_ptr->fr_channels;
    uint8 tag = 0;
	int16  rel = 0;

    if (channels+1 > AAC_DEC_MAX_CHANNEL)
    {
        
        return 12;
    }
    if (hDecoder_ptr->fr_ch_ele+1 > MAX_SYNTAX_ELEMENTS)
    {
         
        return 13;
    }


    /* save the syntax element id */
    hDecoder_ptr->element_id[hDecoder_ptr->fr_ch_ele] = id_syn_ele;

    /* decode the element */
    rel = AAC_SingleLfeChannelElement(hDecoder_ptr, bit_fifo_ptr, channels, &tag, aac_dec_mem_ptr,tmp_cpe_ptr);

    
    /* map output channels position to internal data channels */

    if (hDecoder_ptr->element_output_channels[hDecoder_ptr->fr_ch_ele] == 2)/*lint !e661 */ 
    {
        /* this might be faulty when pce_set is true */        

        /*lint -save -e661 */  
        hDecoder_ptr->internal_channel[channels] = channels;
        hDecoder_ptr->internal_channel[channels+1] = (uint8) (channels+1);
        /*lint -restore */  
    } else 
    {
        if (hDecoder_ptr->pce_set)
        {        
            hDecoder_ptr->internal_channel[hDecoder_ptr->pce.sce_channel[tag]] = channels;
        }
        else
        {
            hDecoder_ptr->internal_channel[channels] = channels;
        }
    }

    hDecoder_ptr->fr_channels = (uint8) (hDecoder_ptr->fr_channels +hDecoder_ptr->element_output_channels[hDecoder_ptr->fr_ch_ele]);
    hDecoder_ptr->fr_ch_ele++;
	return rel;
}

static int16 AAC_DecodeCpe(NeAACDecHandle hDecoder_ptr, 
					   
					   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                       uint8 id_syn_ele,
					   void           *aac_dec_mem_ptr,
					   AAC_ELEMENT_T   *tmp_cpe_ptr)
{
    uint8 channels = hDecoder_ptr->fr_channels;
    uint8 tag = 0;
    int16 rel = 0;
    if (channels+2 > AAC_DEC_MAX_CHANNEL)
    {         
        return 12;
    }
    if (hDecoder_ptr->fr_ch_ele+1 > MAX_SYNTAX_ELEMENTS)
    {
        return 13;
    }

    /* for CPE the number of output channels is always 2 */
    if (hDecoder_ptr->element_output_channels[hDecoder_ptr->fr_ch_ele] == 0)
    {
        /* element_output_channels not set yet */
        hDecoder_ptr->element_output_channels[hDecoder_ptr->fr_ch_ele] = 2;
    } else if (hDecoder_ptr->element_output_channels[hDecoder_ptr->fr_ch_ele] != 2) {
        /* element inconsistency */
        return 21;
    }
    /* save the syntax element id */
    hDecoder_ptr->element_id[hDecoder_ptr->fr_ch_ele] = id_syn_ele;

    /* decode the element */
    rel = AAC_ChannelPairElement(hDecoder_ptr, bit_fifo_ptr, channels, &tag, aac_dec_mem_ptr, tmp_cpe_ptr);

    /* map output channel position to internal data channels */
    if (hDecoder_ptr->pce_set)
    {
        hDecoder_ptr->internal_channel[hDecoder_ptr->pce.cpe_channel[tag]] = channels;
        hDecoder_ptr->internal_channel[hDecoder_ptr->pce.cpe_channel[tag]+1] = (uint8) (channels+1);
    } else {
        hDecoder_ptr->internal_channel[channels] = channels;
        hDecoder_ptr->internal_channel[channels+1] = (uint8) (channels+1);
    }

    hDecoder_ptr->fr_channels += 2;
    hDecoder_ptr->fr_ch_ele++;
    return rel;
}

//#define TEST_STREAM
#ifdef TEST_STREAM
int32 test_data[32];
#endif
int16 AAC_RawDataBlockParsing(NeAACDecHandle hDecoder_ptr, 					
                    AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr, 
					AAC_PROGRAM_CONFIG_T *pce,
					void   *aac_dec_mem_ptr,
					AAC_ELEMENT_T * tmp_cpe_ptr)
{
    uint8 id_syn_ele;
    int16 rel = 0;
    hDecoder_ptr->fr_channels     = 0;
    hDecoder_ptr->fr_ch_ele        = 0;
    hDecoder_ptr->first_syn_ele   = 25;
    hDecoder_ptr->has_lfe            = 0;
    hDecoder_ptr->frame_dec_id  = 1;
    /* Table 4.4.3: raw_data_block() */

    while (1)
    {
        id_syn_ele = (uint8)AAC_GetBits(bit_fifo_ptr, AAC_LEN_SE_ID);
        if(id_syn_ele == AAC_ID_END|| (bit_fifo_ptr->buffer_size <= bit_fifo_ptr->bytes_used)/* */)
        {
            break;
        }
        switch (id_syn_ele)
        {
        case AAC_ID_SCE:
            if (hDecoder_ptr->first_syn_ele == 25) hDecoder_ptr->first_syn_ele = id_syn_ele;
            rel = AAC_DecodeSceLfe(hDecoder_ptr, bit_fifo_ptr, id_syn_ele, aac_dec_mem_ptr,tmp_cpe_ptr);
            if (rel  > 0)
                return rel;
            hDecoder_ptr->fr_channels = 1;
            break;
        case AAC_ID_CPE:
            if (hDecoder_ptr->first_syn_ele == 25) hDecoder_ptr->first_syn_ele = id_syn_ele;
            rel = AAC_DecodeCpe(hDecoder_ptr, bit_fifo_ptr, id_syn_ele, aac_dec_mem_ptr, tmp_cpe_ptr);
            if (rel > 0)
                return rel;
            hDecoder_ptr->fr_channels = 2;
            break;
        case AAC_ID_LFE:
            hDecoder_ptr->has_lfe++;
            rel = AAC_DecodeSceLfe(hDecoder_ptr, bit_fifo_ptr, id_syn_ele, aac_dec_mem_ptr, tmp_cpe_ptr);
            if (rel > 0)
                return rel;
            hDecoder_ptr->fr_channels = 1;
            break;
        case AAC_ID_CCE: /* not implemented yet, but skip the bits */
            return 6;
        case AAC_ID_DSE:
            AAC_DataStreamElement(bit_fifo_ptr);
            break;
        case AAC_ID_PCE:
            /* 14496-4: 5.6.4.1.2.1.3: */
            /* program_configuration_element()'s in access units shall be ignored */
            AAC_ProgramConfigEement(pce, bit_fifo_ptr);                
            break;
        case AAC_ID_FIL:
            /* one sbr_info describes a channel_element not a channel! */
            /* if we encounter SBR data here: error */
            /* SBR data will be read directly in the SCE/LFE/CPE element */
            if ((rel = AAC_FillElement(hDecoder_ptr, 
                                                     bit_fifo_ptr)) > 0)
                return rel;
            default:
                break;
        }
    }
    /* new in corrigendum 14496-3:2002 */
    AAC_ByteAlign(bit_fifo_ptr);

    return 0;
}


static uint8 AAC_SingleLfeChannelElement(NeAACDecHandle hDecoder_ptr,
                                                                   AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                                                   uint8 channel,
                                                                   uint8 *tag,
                                                                   void  *aac_dec_mem_ptr,
                                                                   AAC_ELEMENT_T  *tmp_cpe_ptr)
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    uint8 retval = 0;	
    AAC_ICS_STREAM_T *ics = &(tmp_cpe_ptr->ics1);
    int32   sbr_sign, * spec_data_ptr1;
    int32 *tmp_spec_coef1_ptr; 
    spec_data_ptr1 = (int32 *)(aac_dec_struc_ptr->g_aac_overlap_r);
    ics->ltp_data_s.data_present = 0;    
    ics->ltp_data_s.sf_index = hDecoder_ptr->sf_index;  
    tmp_cpe_ptr->element_instance_tag = (uint8)AAC_GetBits(bit_fifo_ptr, AAC_LEN_TAG);

    tmp_cpe_ptr->common_window = 0;
    *tag = tmp_cpe_ptr->element_instance_tag;
    tmp_cpe_ptr->channel = channel;
    tmp_cpe_ptr->paired_channel = -1;
    tmp_spec_coef1_ptr = (int32 *)(hDecoder_ptr->sbr->Xsbr) + 5120 + 1024 + 3072;
    ics->sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr);
    retval = AAC_IndividualChannelStream(hDecoder_ptr, tmp_cpe_ptr, bit_fifo_ptr, ics, spec_data_ptr1, aac_dec_mem_ptr);
    if (retval > 0)
        return retval;
#if  0//def PSTEST
        if (g_frm_counter > 132)
        {
            FILE *fp = fopen("test_iq_out.txt", "wb");
            int32 i;
            for (i = 0; i < 1024; i++)
            {
                fprintf(fp, "frm: %4d, idx:%4d, left: %10d,\n", g_frm_counter, i, spec_data_ptr1[i]);
            }
            fclose(fp);
        }
#endif
#ifdef AAC_SBR_DEC
    /* check if next bitstream element is a fill element */
    /* if so, read it now so SBR decoding can be done in case of a file with SBR */
    if (0 == hDecoder_ptr->sbr->frame)
    {
        sbrDecodeInit(	hDecoder_ptr->frameLength,
                                hDecoder_ptr->element_id[hDecoder_ptr->fr_ch_ele], 
                                2*AAC_GetSampleRate(hDecoder_ptr->sf_index),
                                hDecoder_ptr->sbr
                                );
    }
    sbr_sign = (int16) (AAC_ShowBits(bit_fifo_ptr, 3));
    if (sbr_sign == AAC_ID_FIL)
    {
        AAC_FlushBits(bit_fifo_ptr, AAC_LEN_SE_ID);		
        /* one sbr_info describes a channel_element not a channel! */
        if ((retval = AAC_SBRFillElement(hDecoder_ptr, 
                                                             bit_fifo_ptr, 
                                                             (uint8)hDecoder_ptr->fr_ch_ele,
                                                             aac_dec_struc_ptr->g_shared_buffer)) > 0)
        {
            return retval;
        }
    }
#endif	
    /* noiseless coding is done, spectral reconstruction is done now */
    retval = AAC_ReconstructSingleChannel(hDecoder_ptr, ics, tmp_cpe_ptr, spec_data_ptr1, aac_dec_mem_ptr);
    if (retval > 0)
        return retval;
    return 0;
}

/* Table 4.4.5 */
static uint8 AAC_ChannelPairElement(NeAACDecHandle hDecoder_ptr, 
                                                          AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                                          uint8 channels, 
                                                          uint8 *tag,
                                                          void  *aac_dec_mem_ptr,
                                                          AAC_ELEMENT_T   *tmp_cpe_ptr)
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
#ifdef AAC_SBR_DEC
    uint8   sbr_sign = 0;
#endif	
    AAC_ICS_STREAM_T *ics1_ptr = &(tmp_cpe_ptr->ics1);
    AAC_ICS_STREAM_T *ics2_ptr = &(tmp_cpe_ptr->ics2);	
    uint8 result;
    int32 *spec_data_ptr1, *spec_data_ptr2;
    spec_data_ptr1 = (int32 *)(aac_dec_struc_ptr->g_sbr_info.Xsbr) + 5120 + 1024 + 2048;
    spec_data_ptr2 = spec_data_ptr1+1024;
    //AAC_DEC_MEMSET(spec_data_ptr1, 0, 1024 * sizeof(short));
    ics1_ptr->ltp_data_s.data_present = 0;
    ics2_ptr->ltp_data_s.data_present = 0;
	
    ics1_ptr->ltp_data_s.sf_index = hDecoder_ptr->sf_index;
    ics2_ptr->ltp_data_s.sf_index = hDecoder_ptr->sf_index;

    tmp_cpe_ptr->channel             = channels;
    tmp_cpe_ptr->paired_channel  = (uint8) (channels+1);

    tmp_cpe_ptr->element_instance_tag = (uint8)AAC_GetBits(bit_fifo_ptr, AAC_LEN_TAG);
    *tag = tmp_cpe_ptr->element_instance_tag;
	
    if ((tmp_cpe_ptr->common_window = AAC_Get1Bit(bit_fifo_ptr)) & 1)
    {
        /* both channels have common ics information */
        if ((result = AAC_IcsInfoPairParse(hDecoder_ptr, ics1_ptr, ics2_ptr, bit_fifo_ptr, tmp_cpe_ptr->common_window)) > 0)   // modify by reed
            return result;
        ics1_ptr->ms_mask_present = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
        ics2_ptr->ms_mask_present = ics1_ptr->ms_mask_present;
        if (ics1_ptr->ms_mask_present == 1)
        {
            uint8 g, sfb;
            for (g = 0; g < ics1_ptr->num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics1_ptr->max_sfb; sfb++)
                {
                    uint8 t;
                    t = AAC_Get1Bit(bit_fifo_ptr);
                    ics1_ptr->ms_used[g][sfb] = t;
                    ics2_ptr->ms_used[g][sfb] = t;
                }
            }
        }
    } else 
    {
        int32 *tmp_spec_coef1_ptr;
        ics1_ptr->ms_mask_present = 0;
        tmp_spec_coef1_ptr = (int32 *)(hDecoder_ptr->sbr->Xsbr) + 5120 + 1024 + 3072;
        ics1_ptr->sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr);
        ics2_ptr->sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr + 480);
    }
    if ((result = AAC_IndividualChannelStream(hDecoder_ptr,
                                                                      tmp_cpe_ptr, 
                                                                      bit_fifo_ptr, 
                                                                      ics1_ptr,											 
                                                                      spec_data_ptr1,
                                                                      aac_dec_mem_ptr)) > 0)
    {
        return result;
    }
    	
	
    if ((result = AAC_IndividualChannelStream(hDecoder_ptr,
                                                                      tmp_cpe_ptr, 
                                                                      bit_fifo_ptr, 
                                                                      ics2_ptr,
                                                                      spec_data_ptr2,
                                                                      aac_dec_mem_ptr)) > 0)
    {
        return result;
    }
#if 0
                    {
                        FILE *fp = fopen("ref_hufft_IQ.text", "a+");
                        int k;
                        fprintf(fp, "frm: %10d, \n", g_frm_counter);
                        for (k = 0; k < 1024; k++)
                        {
                            fprintf(fp, "%d, \n", spec_data_ptr1[k]);
                        }
                        for (k = 0; k < 1024; k++)
                        {
                            fprintf(fp, "%d, \n", spec_data_ptr2[k]);
                        }
                        fclose(fp);
                    }
#endif
#ifdef AAC_SBR_DEC
   if (0 == hDecoder_ptr->frame)
   {
        sbrDecodeInit(hDecoder_ptr->frameLength,
                               hDecoder_ptr->element_id[hDecoder_ptr->fr_ch_ele], 
                               2*AAC_GetSampleRate(hDecoder_ptr->sf_index),
                               hDecoder_ptr->sbr
                              );
    }
    sbr_sign = (uint8) (AAC_ShowBits(bit_fifo_ptr, AAC_LEN_SE_ID));
    if ((sbr_sign == AAC_ID_FIL))
    {
        AAC_FlushBits(bit_fifo_ptr, AAC_LEN_SE_ID);		
        /* one sbr_info describes a channel_element not a channel! */
        if ((result = AAC_SBRFillElement( hDecoder_ptr,
                                                              bit_fifo_ptr, 
                                                              (uint8) (hDecoder_ptr->fr_ch_ele),
                                                              aac_dec_struc_ptr->g_shared_buffer)) > 0)
        {
            return result;
        }
    }
#endif		
    /* noiseless coding is done, spectral reconstruction is done now */
    if ((result = AAC_ReconstructChannelPair( hDecoder_ptr,
                                                                      ics1_ptr, 
                                                                      ics2_ptr, 
                                                                      tmp_cpe_ptr,
                                                                      spec_data_ptr1,
                                                                      spec_data_ptr2,
                                                                      aac_dec_mem_ptr)) > 0)
    {
        return result;
    }
    return 0;
}

static uint8 AAC_IcsInfoPairParse(NeAACDecHandle hDecoder_ptr, 
                                                     AAC_ICS_STREAM_T *ics1, 
                                                     AAC_ICS_STREAM_T *ics2, 
                                                     AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                                     uint8 common_window)
{
    uint8 retval = 0;	
    int32 *tmp_spec_coef1_ptr;
    tmp_spec_coef1_ptr = (int32 *)(hDecoder_ptr->sbr->Xsbr) + 5120 + 1024 + 3072;
    ics1->sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr);
    ics2->sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr + 480);
    /* ics->ics_reserved_bit = */ AAC_Get1Bit(bit_fifo_ptr);
    ics1->window_sequence = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
    ics2->window_sequence = ics1->window_sequence;

    ics1->window_shape = AAC_Get1Bit(bit_fifo_ptr);
    ics2->window_shape = ics1->window_shape;
	
    if (ics1->window_sequence == AAC_EIGHT_SHORT_SEQUENCE)
    {
        ics1->max_sfb = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
        ics2->max_sfb = ics1->max_sfb;
        ics1->scale_factor_grouping = (uint8)AAC_GetBits(bit_fifo_ptr, 7);
        ics2->scale_factor_grouping = ics1->scale_factor_grouping;
    } else {
        ics1->max_sfb = (uint8)AAC_GetBits(bit_fifo_ptr, 6);
        ics2->max_sfb = ics1->max_sfb;
    }
	
    /* get the grouping information */
    if ((retval = AAC_WindowGroupingInfoPairParsing(hDecoder_ptr, ics1, ics2)) > 0)
        return retval;
	
    /* check the range of max_sfb */
    if (ics1->max_sfb > ics1->num_swb)
        return 16;
	
    if (ics1->window_sequence != AAC_EIGHT_SHORT_SEQUENCE)
    {
        if ((ics1->predictor_data_present = AAC_Get1Bit(bit_fifo_ptr)) & 1)
        {
            if (hDecoder_ptr->object_type == AAC_LTP) /* AAC_LTP mode side information parsing */
            {
                /* LTP mode */
                /* left channel */
                ics1->ltp_data_s.data_present = AAC_Get1Bit(bit_fifo_ptr);
                if (ics1->ltp_data_s.data_present)
                {
                    /* left channel ltp side information parsing */
                    AAC_DEC_LTPSideInfoParsing(ics1, bit_fifo_ptr);
                }
                if (common_window)
                {
                    /* right channel */
                    ics2->ltp_data_s.data_present = AAC_Get1Bit(bit_fifo_ptr);
                    if (ics2->ltp_data_s.data_present)
                    {
                        /* left channel ltp side information parsing */
                        AAC_DEC_LTPSideInfoParsing(ics2, bit_fifo_ptr);
                    }                    
                }
            }
        }
        ics2->predictor_data_present = ics1->predictor_data_present;   // reed add
    }	
    return retval;
}
static uint8 AAC_IcsInfoSingleParse(NeAACDecHandle hDecoder_ptr, 
                                                     AAC_ICS_STREAM_T *ics1, 
                                                     AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr,
                                                     uint8 common_window)
{
    uint8 retval = 0;	
    /* ics->ics_reserved_bit = */ AAC_Get1Bit(bit_fifo_ptr);
    ics1->window_sequence = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
    ics1->window_shape = AAC_Get1Bit(bit_fifo_ptr);	
    if (ics1->window_sequence == AAC_EIGHT_SHORT_SEQUENCE)
    {
        ics1->max_sfb = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
        ics1->scale_factor_grouping = (uint8)AAC_GetBits(bit_fifo_ptr, 7);
    } else 
    {
        ics1->max_sfb = (uint8)AAC_GetBits(bit_fifo_ptr, 6);
    }
    /* get the grouping information */
    if ((retval = AAC_WindowGroupingInfoSingleParsing(hDecoder_ptr, ics1)) > 0)
        return retval;	
    /* check the range of max_sfb */
    if (ics1->max_sfb > ics1->num_swb)
        return 16;
    if (ics1->window_sequence != AAC_EIGHT_SHORT_SEQUENCE)
    {
        if ((ics1->predictor_data_present = AAC_Get1Bit(bit_fifo_ptr)) & 1)
        {
            if (hDecoder_ptr->object_type == AAC_LTP) /* AAC_LTP mode side information parsing */
            {
                /* LTP mode */
                /* left channel */
                ics1->ltp_data_s.data_present = AAC_Get1Bit(bit_fifo_ptr);
                if (ics1->ltp_data_s.data_present)
                {
                    /* left channel ltp side information parsing */
                    AAC_DEC_LTPSideInfoParsing(ics1, bit_fifo_ptr);
                }
            }
        }
    }	
    return retval;
}

/* Table 4.4.7 */
static uint8 AAC_HuffPulseData(AAC_ICS_STREAM_T *ics, AAC_PULSE_INFO_T *pul, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    uint8 i;
    pul->number_pulse = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
    pul->pulse_start_sfb = (uint8)AAC_GetBits(bit_fifo_ptr, 6);
    /* check the range of pulse_start_sfb */
    if (pul->pulse_start_sfb > ics->num_swb)
        return 16;
    for (i = 0; i < pul->number_pulse+1; i++)
    {
        pul->pulse_offset[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 5);
        pul->pulse_amp[i] = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    }

    return 0;
}

/* Table 4.4.10 */
static uint16 AAC_DataStreamElement(AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    uint8 data_byte_align_flag;
    uint16 i, count;

    /* element_instance_tag = */ AAC_GetBits(bit_fifo_ptr, AAC_LEN_TAG);
    data_byte_align_flag = AAC_Get1Bit(bit_fifo_ptr);
    count = (uint16)AAC_GetBits(bit_fifo_ptr, 8);
    if (count == 255)
    {
        count = (uint16) (count +AAC_GetBits(bit_fifo_ptr, 8));
    }
    if (data_byte_align_flag)
        AAC_ByteAlign(bit_fifo_ptr);

    for (i = 0; i < count; i++)
    {
        AAC_GetBits(bit_fifo_ptr, AAC_LEN_BYTE);
    }

    return count;
}

/* Table 4.4.11 */
static uint8 AAC_FillElement(NeAACDecHandle hDecoder_ptr, 
                                             AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    int16 count;
    count = (int16)AAC_GetBits(bit_fifo_ptr, 4);
    if (count == 15)
    {
        count = (int16) (count +AAC_GetBits(bit_fifo_ptr, 8) - 1);
    }
    if (count > 0)
    {
        while (count > 0)
        {
            count = (int16) (count -AAC_ExtensionPayload(bit_fifo_ptr, (uint16)count));
        }
    }
    return 0;
}

extern int32 AAC_DEC_ScaleFactorHuffAsm(
                                        AAC_ICS_STREAM_T        *ics_ptr,
                                        AAC_BIT_FIFO_FORMAT_T   *ld_ptr
                                       );
/* Table 4.4.24 */
static uint8 AAC_IndividualChannelStream(NeAACDecHandle         hDecoder_ptr, 
                                          AAC_ELEMENT_T                            *ele_ptr,
                                          AAC_BIT_FIFO_FORMAT_T           *bit_fifo_ptr, 
                                          AAC_ICS_STREAM_T                       *ics_ptr,
                                          int32                                                      *spec_data_ptr,
                                          void                                                       *aac_dec_mem_ptr)
{
    uint8     result;
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    int32    *tmp_buf_ptr = aac_dec_struc_ptr->g_shared_buffer;
    hDecoder_ptr->frame_dec_id  = 0;
    ics_ptr->global_gain = (uint8)AAC_GetBits(bit_fifo_ptr, 8);
    if (!ele_ptr->common_window)
    {
        if ((result = AAC_IcsInfoSingleParse(hDecoder_ptr, ics_ptr, bit_fifo_ptr, ele_ptr->common_window)) > 0)
            return result;
    }
    if ((result = AAC_SectionData(hDecoder_ptr, ics_ptr, bit_fifo_ptr)) > 0)
        return result;
    result = (uint8) AAC_DEC_ScaleFactorHuffAsm(ics_ptr, bit_fifo_ptr);

    if (result > 0)
    {
        return result;
    }
    /* get pulse data */
    if ((ics_ptr->pulse_data_present = AAC_Get1Bit(bit_fifo_ptr)) & 1)
    {
        if ((result = AAC_HuffPulseData(ics_ptr, &(ics_ptr->pul), bit_fifo_ptr)) > 0)
            return result;
    }	
    /* get tns data */
    if ((ics_ptr->tns_data_present = AAC_Get1Bit(bit_fifo_ptr)) & 1)
    {
        AAC_TnsDataParsing(ics_ptr, &(ics_ptr->tns), bit_fifo_ptr);
    }	
    /* get gain control data */
    ics_ptr->gain_control_data_present = AAC_Get1Bit(bit_fifo_ptr);
		
    /* decode the spectral data */
    if ((result = AAC_SpectralDataParsing(ics_ptr, bit_fifo_ptr, spec_data_ptr, tmp_buf_ptr)) > 0)
    {
       return result;
    }	
    /* pulse coding reconstruction */
    if (ics_ptr->pulse_data_present)
    {
        if (ics_ptr->window_sequence != AAC_EIGHT_SHORT_SEQUENCE)
        {
            if ((result = AAC_PulseDecode(ics_ptr, spec_data_ptr)) > 0)
                return result;
        } else {
            return 2; /* pulse coding not allowed for short blocks */
        }
    }
    return 0;
}

/* Table 4.4.25 */
static uint8 AAC_SectionData(NeAACDecHandle  hDecoder_ptr, 
							AAC_ICS_STREAM_T      *ics,
							AAC_BIT_FIFO_FORMAT_T        *bit_fifo_ptr)
{
    int32 g;
    uint8 sect_esc_val, sect_bits;
    if (ics->window_sequence == AAC_EIGHT_SHORT_SEQUENCE)
        sect_bits = 3;
    else
        sect_bits = 5;
    sect_esc_val = (uint8) ((1<<sect_bits) - 1);

    for (g = 0; g < ics->num_window_groups; g++)
    {
        int32 k = 0;
        int32 i = 0;

        int32 max_sfb = ics->max_sfb;
        while (k < max_sfb && (i < max_sfb))
        {
            uint8    sfb;
            uint8    sect_len_incr;
            uint16  sect_len = 0;
            uint8    tmp;
            uint8  *tmp2;
            /* if "AAC_GetBits" detects error and returns "0", "k" is never
               incremented and we cannot leave the while loop */
            if ((bit_fifo_ptr->error != 0) || (bit_fifo_ptr->no_more_reading))
                return 14;
            sect_len_incr = (uint8) (AAC_GetBits(bit_fifo_ptr, 4));            
            ics->sect_cb[g][i] = (uint8) sect_len_incr;
            if (sect_len_incr == AAC_NOISE_HCB)
                ics->noise_used = 1;
            sect_len_incr = (uint8) (AAC_GetBits(bit_fifo_ptr, sect_bits));

            while ((sect_len_incr == sect_esc_val) /* &&
                (k+sect_len < ics->max_sfb)*/)
            {
                sect_len = (uint16) (sect_len +sect_len_incr);
                sect_len_incr = (uint8)AAC_GetBits(bit_fifo_ptr, sect_bits);
            }
            sect_len = (uint16) (sect_len +sect_len_incr);
            ics->sect_end[g][i] = (uint16) ( k + sect_len);
            if (k + sect_len >= 8*15)
                return 15;
            tmp  = ics->sect_cb[g][i];
            tmp2 = ics->sfb_cb[g] + k;
            for (sfb = k; sfb < k + sect_len; sfb++)
            {
               *(tmp2++) = tmp;
            }
            k =  (k +sect_len);
            i++;
        }
        ics->num_sec[g] = i;
    }
    return 0;
}




/* Table 4.4.27 */
static void AAC_TnsDataParsing(AAC_ICS_STREAM_T *ics,
					 AAC_TNS_INFO_T  *tns,
					 AAC_BIT_FIFO_FORMAT_T   *bit_fifo_ptr)
{
    int32 w, filt, i, start_coef_bits=0, coef_bits;
    int32 n_filt_bits = 2;
    int32 length_bits = 6;
    int32 order_bits = 5;

    if (ics->window_sequence == AAC_EIGHT_SHORT_SEQUENCE)
    {
        n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
    }

    for (w = 0; w < ics->num_windows; w++)
    {
        tns->n_filt[w] = (uint8)AAC_GetBits(bit_fifo_ptr, n_filt_bits);

        if (tns->n_filt[w])
        {
            if ((tns->coef_res[w] = AAC_Get1Bit(bit_fifo_ptr)) & 1)
            {
                start_coef_bits = 4;
            } else {
                start_coef_bits = 3;
            }
        }

        for (filt = 0; filt < tns->n_filt[w]; filt++)
        {
            tns->length[w][filt] = (uint8)AAC_GetBits(bit_fifo_ptr, length_bits);
            tns->order[w][filt]  = (uint8)AAC_GetBits(bit_fifo_ptr, order_bits);
            if (tns->order[w][filt])
            {
                tns->direction[w][filt] = AAC_Get1Bit(bit_fifo_ptr);
                tns->coef_compress[w][filt] = AAC_Get1Bit(bit_fifo_ptr);
                coef_bits = (uint8) (start_coef_bits - tns->coef_compress[w][filt]);
                for (i = 0; i < tns->order[w][filt]; i++)
                {
                    tns->coef[w][filt][i] = (uint8)AAC_GetBits(bit_fifo_ptr, coef_bits);
                }
            }
        }
    }
}
extern int32 AAC_DEC_HuffmanCB11Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );	
extern int32 AAC_DEC_HuffmanCB9Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );
extern int32 AAC_DEC_HuffmanCB34Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );      
extern int32 AAC_DEC_HuffmanCB1and2Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );
extern int32 AAC_DEC_HuffmanCB8and10Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );                
extern int32 AAC_DEC_HuffmanCB5Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );                
extern int32 AAC_DEC_HuffmanCB6Asm(
                                              int32                   *out_ptr,
                                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                                              int32                   *scalefac_ptr,
                                              int32                   *sect_sfb_offset_ptr
                                              );                
static void AAC_DEC_Reorder(AAC_ICS_STREAM_T *ics_ptr,
                            int32 *test_data_opt, 
                            int32 *spec_data_ptr)
{
    int32 g = 0, sfb = 0;
    int32 *spec_data_t_ptr = spec_data_ptr;
    int32 width;
    for (g = 0; g < ics_ptr->num_window_groups; g++)
    {
        int16 win;
        for (sfb = 0; sfb < ics_ptr->num_swb; sfb++)
        {            
            width = (uint8) (ics_ptr->swb_offset[sfb+1] - ics_ptr->swb_offset[sfb]);
            for (win = 0; win < ics_ptr->window_group_length[g]; win ++)
            {
                AAC_DEC_MEMCPY(spec_data_t_ptr+128 * win, test_data_opt+win * width, width * sizeof(int32));
            }
            spec_data_t_ptr += width;
            test_data_opt   += width * ics_ptr->window_group_length[g];
        }
        spec_data_ptr += 128 * ics_ptr->window_group_length[g];
        spec_data_t_ptr = spec_data_ptr;
    }
}
                
                                                                                                      
/* Table 4.4.29 */
static uint8 AAC_SpectralDataParsing(AAC_ICS_STREAM_T *ics_ptr,
                             AAC_BIT_FIFO_FORMAT_T   *bit_fifo_ptr,
                             int32   *spec_data_1_ptr,
                             int32   *tmp_buf_ptr)
{
    int32 *spec_data_0_ptr, *base_buf_ptr;
    int32 i = 0;
    int32 g = 0;
    int32 sect_strat=0, start_pos = 0, end_pos=0;
    int32 groups  = 0;
    int32 sect_cb = 0;    
    if (AAC_EIGHT_SHORT_SEQUENCE == ics_ptr->window_sequence)
    {
        base_buf_ptr = tmp_buf_ptr;
    }else
    {
        base_buf_ptr = spec_data_1_ptr;
    }
    AAC_DEC_MEMSET(base_buf_ptr, 0, 1024*4);
    for(g = 0; g < ics_ptr->num_window_groups; g++)
    {
        uint8 num_seg0 = ics_ptr->num_sec[g];
        spec_data_0_ptr   = base_buf_ptr + groups*128;
	    
        sect_strat = 0;
        start_pos  = 0;
        for (i = 0; i < num_seg0; i++)
        {
            sect_cb = ics_ptr->sect_cb[g][i];
            end_pos = ics_ptr->sect_sfb_offset[g][ics_ptr->sect_end[g][i]];			
            if (end_pos > 1024 
		 || ((end_pos - start_pos) & 0x1) 
	        || (bit_fifo_ptr->buffer_size <= bit_fifo_ptr->bytes_used)
	        || (end_pos <= start_pos)
	        || (end_pos <= 0)
	        || (start_pos < 0)
	        || (ics_ptr->sect_end[g][i] > ics_ptr->num_swb))
            {
                return 50;
            }
            switch (sect_cb)
            {
            case AAC_ZERO_HCB:
            case AAC_NOISE_HCB:
            case AAC_INTENSITY_HCB:
            case AAC_INTENSITY_HCB2:
                break;
            default:				
                switch (sect_cb)
                {
                case 8: 
                case 10:		    
                    spec_data_0_ptr[start_pos]   = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    if (8 == sect_cb)
                    {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb8;
                     }else
                    {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb10;
                     }
                    AAC_DEC_HuffmanCB8and10Asm(spec_data_0_ptr+start_pos, 
                                                                              bit_fifo_ptr, 
                                                                              (int32 *) (ics_ptr->scale_factors[g]+sect_strat),
                                                                              (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));						
                    break;
                case 6: 
                    spec_data_0_ptr[start_pos]   = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb6;
                    AAC_DEC_HuffmanCB6Asm(spec_data_0_ptr+start_pos, 
                                                                     bit_fifo_ptr, 
                                                                      (int32 *) (ics_ptr->scale_factors[g]+sect_strat), 
                                                                      (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));
                    break;
                case 11:		
                    spec_data_0_ptr[start_pos] = (int32)ics_ptr->sect_end[g][i]-sect_strat;	    
                    AAC_DEC_HuffmanCB11Asm(spec_data_0_ptr+start_pos, 
                                                                       bit_fifo_ptr, 
                                                                       (int32 *) (ics_ptr->scale_factors[g]+sect_strat),
                                                                       (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));
                    break;
                case 1: 
                case 2:
                    spec_data_0_ptr[start_pos]   = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    if (2 == sect_cb)
                    {
                         spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb2;
                    }else
                    {
                         spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb1;
                    }
                    AAC_DEC_HuffmanCB1and2Asm(spec_data_0_ptr+start_pos, 
                                                                            bit_fifo_ptr, 
                                                                            (int32 *) (ics_ptr->scale_factors[g]+sect_strat), 
                                                                            (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));					
                    break;
                case 3: 
                case 4:  
                    spec_data_0_ptr[start_pos]   = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    if (4 == sect_cb)
                   {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb4;
                    }else
                   {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb3;
                    }	                   
                    AAC_DEC_HuffmanCB34Asm(spec_data_0_ptr+start_pos, 
                                                                       bit_fifo_ptr, 
                                                                       (int32 *) (ics_ptr->scale_factors[g]+sect_strat), 
                                                                       (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));
                    break;	
                case 5: 
                    spec_data_0_ptr[start_pos]   = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb5;
                    AAC_DEC_HuffmanCB5Asm(spec_data_0_ptr+start_pos, 
                                                                     bit_fifo_ptr, 
                                                                     (int32 *) (ics_ptr->scale_factors[g]+sect_strat), 
                                                                     (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));
                    break;
                case 7: 						
                case 9: 
                    /* these table pre-read 4 bit */						
                    spec_data_0_ptr[start_pos] = (int32)ics_ptr->sect_end[g][i]-sect_strat;
                    if (9 == sect_cb)
                    {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb9;
                    }else
                    {
                        spec_data_0_ptr[start_pos+1] = (int32) AAC_DEC_HuffCb7;
                    }
                    AAC_DEC_HuffmanCB9Asm(spec_data_0_ptr+start_pos, 
                                                                     bit_fifo_ptr, 
                                                                     (int32 *) (ics_ptr->scale_factors[g]+sect_strat), 
                                                                     (int32 *) (ics_ptr->sect_sfb_offset[g]+sect_strat));
                    break;
                default:
                     /* Non existent codebook number, something went wrong */
                    return 11;              
                }
                 break;
            }
            sect_strat = ics_ptr->sect_end[g][i];
            start_pos  = end_pos;
        }
        groups = (uint8) (groups +ics_ptr->window_group_length[g]);
    }    
    if (AAC_EIGHT_SHORT_SEQUENCE == ics_ptr->window_sequence)
    {
        /* reorder for short block */
        AAC_DEC_Reorder(ics_ptr, base_buf_ptr, spec_data_1_ptr);
    }
    return 0;
}



/* Table 1.A.5 */
uint8 AAC_AdtsFrameParsing(AAC_ADTS_HEADER_T *adts, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    /* faad_byte_align(bit_fifo_ptr); */
    if (AAC_AdtsFixedHeaderParsing(adts, bit_fifo_ptr))
        return 5;
    AAC_AdtsVariableHeaderParsing(adts, bit_fifo_ptr);
    AAC_AdtsErrorCheck(adts, bit_fifo_ptr);
    return 0;
}

/* Table 1.A.6 */
extern int adts_sample_rates[];
static uint8 AAC_AdtsFixedHeaderParsing(AAC_ADTS_HEADER_T *adts, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    int32   code = AAC_GetBits(bit_fifo_ptr, 28);
    /* try to recover from sync errors */
    //AAC_GetBits(bit_fifo_ptr, 12);
    adts->id                             = (code >> 15) & 0x1;//AAC_Get1Bit(bit_fifo_ptr);
    adts->layer                         = (code >> 13) & 0x3;//(uint8)AAC_GetBits(bit_fifo_ptr, 2);
    adts->protection_absent     = (code >> 12) & 0x1;//AAC_Get1Bit(bit_fifo_ptr);
    
    adts->profile                      = (code >> 10) & 0x3;//(uint8)AAC_GetBits(bit_fifo_ptr, 2);
    adts->sf_index                   = (code >> 6) & 0xF;//(uint8)AAC_GetBits(bit_fifo_ptr, 4);
    adts->private_bit               = (code >> 5) & 0x1;//AAC_Get1Bit(bit_fifo_ptr);
    adts->channel_configuration = (code >> 2) & 0x7;//(uint8)AAC_GetBits(bit_fifo_ptr, 3);
    adts->original                      = (code >> 1) & 0x1;//AAC_Get1Bit(bit_fifo_ptr);
    adts->home                        = (code) & 0x1;//AAC_Get1Bit(bit_fifo_ptr);
    
    if (adts->old_format == 1)
    {
        /* Removed in corrigendum 14496-3:2002 */
        if (adts->id == 0)
        {
            adts->emphasis = (uint8)AAC_GetBits(bit_fifo_ptr, 2);
        }
    }
    return 0;
}

/* Table 1.A.7 */
static void AAC_AdtsVariableHeaderParsing(AAC_ADTS_HEADER_T *adts, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    int32   code = AAC_GetBits(bit_fifo_ptr, 28);
    adts->copyright_identification_bit        = code >> 27;                       //AAC_Get1Bit(bit_fifo_ptr);
    adts->copyright_identification_start     = (code >> 26) & 0x1;          //AAC_Get1Bit(bit_fifo_ptr);
    adts->aac_frame_length                      = (code >> 13) & 0x1FFF;   //(uint16)AAC_GetBits(bit_fifo_ptr, 13);
    adts->adts_buffer_fullness                   = (code >> 2) & 0x7FF;       //(uint16)AAC_GetBits(bit_fifo_ptr, 11);
    adts->no_raw_data_blocks_in_frame  = (code) & 0x3;                    //(uint8)AAC_GetBits(bit_fifo_ptr, 2);
}

/* Table 1.A.8 */
static void AAC_AdtsErrorCheck(AAC_ADTS_HEADER_T *adts, AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr)
{
    if (adts->protection_absent == 0)
    {
        adts->crc_check = (uint16)AAC_GetBits(bit_fifo_ptr, 16);
    }
}

#ifdef AAC_SBR_DEC
/************************************************************************/
// reed add at 2005-12-31
// SBR DEOCDER stream parsing and get the information for SBR reconstruct
/************************************************************************/

/* Table 4.4.11 */
static uint8 AAC_SBRFillElement(NeAACDecHandle hDecoder_ptr, 
                                                    AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr
#ifdef AAC_SBR_DEC
                                                   ,uint8 sbr_ele
#endif
                                                   ,int32  *tmp_ptr
                                                   )
{
    uint16 count;
#ifdef AAC_SBR_DEC
    uint8 bs_extension_type;
#endif
    count = (uint16)AAC_GetBits(bit_fifo_ptr, 4);
    if (count == 15)
    {
        count = (uint16) (count +AAC_GetBits(bit_fifo_ptr, 8) - 1);
    }
    if (count > 0)
    {
#ifdef AAC_SBR_DEC
        bs_extension_type = (uint8) (AAC_ShowBits(bit_fifo_ptr, 4));
        if ((bs_extension_type == AAC_EXT_SBR_DATA) ||
            (bs_extension_type == AAC_EXT_SBR_DATA_CRC))
        {
            if (sbr_ele == AAC_INVALID_SBR_ELEMENT)
                return 24;
            hDecoder_ptr->sbr_present_flag = 1;
            /* parse the SBR data */
            hDecoder_ptr->sbr->ret = AAC_SbrExtensionData(bit_fifo_ptr,
                                                                                             hDecoder_ptr->sbr, 
                                                                                             count,
                                                                                             tmp_ptr);
#ifdef AAC_PS_DEC
            if (hDecoder_ptr->sbr->ps_used)
            {
                hDecoder_ptr->ps_used[sbr_ele] = 1;
            }
#endif
        } else {
#endif
            while (count > 0)
            {
                count = (uint16) (count -AAC_ExtensionPayload( bit_fifo_ptr, count));
            }
#ifdef AAC_SBR_DEC
        }
#endif
    }
    return 0;
}


/* Table 4.4.30 */
static uint16 AAC_ExtensionPayload(AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr, uint16 count)
{
    uint16 i, dataElementLength;
    uint8 dataElementLengthPart;
    uint8 align = 4, data_element_version, loopCounter;
    uint8 extension_type = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
    switch (extension_type)
    {
    case AAC_EXT_FILL_DATA:
        /* fill_nibble = */ AAC_GetBits(bit_fifo_ptr, 4); /* must be ?000?*/
        for (i = 0; i < count-1; i++)
        {
            /* fill_byte[i] = */ AAC_GetBits(bit_fifo_ptr, 8); /* must be ?0100101?*/
        }
        return count;
    case AAC_EXT_DATA_ELEMENT:
        data_element_version = (uint8)AAC_GetBits(bit_fifo_ptr, 4);
        switch (data_element_version)
        {
        case AAC_ANC_DATA:
            loopCounter = 0;
            dataElementLength = 0;
            do {
                dataElementLengthPart = (uint8)AAC_GetBits(bit_fifo_ptr, 8);
                dataElementLength = (uint16) (dataElementLength +dataElementLengthPart);
                loopCounter++;

            } while (dataElementLengthPart == 255);
			
            for (i = 0; i < dataElementLength; i++)
            {
                /* data_element_byte[i] = */ AAC_GetBits(bit_fifo_ptr, 8);
                return (uint16) (dataElementLength+loopCounter+1);
            }
            break;
        default:
            align = 0;
            break;
        }
    //case EXT_FIL:
    default:/*lint !e825 */
        AAC_GetBits(bit_fifo_ptr, align);
        for (i = 0; i < count-1; i++)
        {
            AAC_GetBits(bit_fifo_ptr, 8);
        }
        return count;
    }
}

#endif
