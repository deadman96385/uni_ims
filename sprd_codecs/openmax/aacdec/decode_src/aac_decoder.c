/*************************************************************************
** File Name:      decoder.c                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_decoder.h"
#include "aac_syntax.h"


//#define FILE_TEST
#define NULL 0
#define INPUTBUF_SIZE 4096

int16 AAC_FrameDecode(uint8           *buffer,         // the input stream
					  uint32           buffer_size,    // input stream size
					  uint16          *pcm_out_l,      // output left channel pcm
					  uint16          *pcm_out_r,      // output right channel pcm
					  uint16          *frm_pcm_len,    // frmae length 
					  void            *aac_dec_mem_ptr,
					  int16            aacplus_decode_flag,
					  int16		*decoded_bytes
					  )
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    AAC_BIT_FIFO_FORMAT_T *bit_fifo_ptr;
    int16  rel = 0;
    int16  exit_sign = 0;    
    NeAACDecHandle hDecoder_ptr;   // the input relative information
    int32 *tmp_shared_buffer_ptr = aac_dec_struc_ptr->g_shared_buffer;
    int32 *tmp_spec_coef1_ptr;
    AAC_ELEMENT_T *tmp_cpe_ptr;
	
    hDecoder_ptr = &aac_dec_struc_ptr->g_decoder;
    hDecoder_ptr->aacplus_decode_flag = aacplus_decode_flag;
    hDecoder_ptr->sbr = &aac_dec_struc_ptr->g_sbr_info;
    hDecoder_ptr->sbr->ps = (AAC_PS_INFO_T *)((int32 *)(aac_dec_struc_ptr->g_sbr_info.Xsbr) + 5120);
    hDecoder_ptr->sbr->qmfa[0] = aac_dec_struc_ptr->g_qmf_ana[0];
    hDecoder_ptr->sbr->qmfs[0] = aac_dec_struc_ptr->g_qmf_syn[0];	
    hDecoder_ptr->sbr->qmfs[1] = aac_dec_struc_ptr->g_qmf_syn[1];
    hDecoder_ptr->sbr->qmfa[1] = aac_dec_struc_ptr->g_qmf_ana[1]; 
	
    tmp_cpe_ptr = &aac_dec_struc_ptr->g_cpe;
    tmp_spec_coef1_ptr = (int32 *)(aac_dec_struc_ptr->g_sbr_info.Xsbr) + 5120 + 1024 + 3072;
    //tmp_cpe_ptr->ics1.sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr);
    //tmp_cpe_ptr->ics2.sect_sfb_offset = (uint16 (*)[120]) (tmp_spec_coef1_ptr + 480);
    
    tmp_cpe_ptr->ics1.sect_end        = (uint16 (*)[120]) (tmp_shared_buffer_ptr + 1024);	
    tmp_cpe_ptr->ics1.scale_factors   = (int16  (*)[51] ) (tmp_shared_buffer_ptr + 1504);	
    tmp_cpe_ptr->ics2.sect_end        = (uint16 (*)[120]) (tmp_shared_buffer_ptr + 2048);	
    tmp_cpe_ptr->ics2.scale_factors   = (int16  (*)[51] ) (tmp_shared_buffer_ptr + 2528);
	
    if (AAC_LTP == hDecoder_ptr->object_type)
    {
        tmp_cpe_ptr->ics1.ltp_data_s.ltp_pred_sample = (int16 *)aac_dec_struc_ptr->g_qmf_ana;
        tmp_cpe_ptr->ics2.ltp_data_s.ltp_pred_sample = tmp_cpe_ptr->ics1.ltp_data_s.ltp_pred_sample + 3072;
    }
    hDecoder_ptr->frameLength = 1024;
	
    aac_dec_struc_ptr->pcm_out_l_ptr = pcm_out_l;
    aac_dec_struc_ptr->pcm_out_r_ptr = pcm_out_r;  
	
    bit_fifo_ptr = &aac_dec_struc_ptr->g_bit_fifo;

    AAC_DEC_MEMSET(hDecoder_ptr->internal_channel, 0, AAC_DEC_MAX_CHANNEL*sizeof(hDecoder_ptr->internal_channel[0]));


    if (buffer_size > INPUTBUF_SIZE)
    {
        rel = 0x8001;
        goto AAC_DEC_ERROR;
    }
    AAC_DEC_MEMCPY(aac_dec_struc_ptr->inputbuf, buffer, buffer_size);
    *decoded_bytes = 0;
    
    /* initialize the bitstream */
    if (1 == hDecoder_ptr->latm_sign)
    {
        /* LATM format parsing */
        int16 real_unit_len = 0;
        int16 loop_counter = 0;
        int16 cw;        
        if (0 == hDecoder_ptr->unit_byte_consumed || hDecoder_ptr->latm_seek == AAC_DEC_LATM_SEEK)
        {
            AAC_InitBits(bit_fifo_ptr, aac_dec_struc_ptr->inputbuf, buffer_size);
            hDecoder_ptr->latm_unit_len      = (uint16) (buffer_size);  
            hDecoder_ptr->unit_byte_consumed = 0;   
           
            cw = (int16) (AAC_GetBits(bit_fifo_ptr, 8));
            while (0xFF == cw && loop_counter < AAC_DEC_LATMCOUNTER_LIMIT)
            {
                real_unit_len = (uint16) (real_unit_len +cw+1);
                cw = (int16) (AAC_GetBits(bit_fifo_ptr, 8));
                loop_counter++;
            }
            real_unit_len = (uint16) (real_unit_len +cw+1);  // count the frame byte consumed.

	    if (hDecoder_ptr->latm_seek == AAC_DEC_LATM_SEEK)
		hDecoder_ptr->latm_seek = 0;        

            if ((loop_counter >= AAC_DEC_LATMCOUNTER_LIMIT) || (real_unit_len != buffer_size))
            {
                /* error processing */
                goto AAC_DEC_ERROR;   
            }
        }   
    }else
    {
        /* other format(ADTS, ADIF, MP4)*/
        AAC_InitBits(bit_fifo_ptr, aac_dec_struc_ptr->inputbuf, buffer_size);     
        if (hDecoder_ptr->adts_header_present)
        {
            AAC_ADTS_HEADER_T adts;
            adts.old_format = hDecoder_ptr->config.useOldADTSFormat;
            if ((rel = AAC_AdtsFrameParsing(&adts, bit_fifo_ptr)) > 0)
                goto AAC_DEC_ERROR;
            hDecoder_ptr->sf_index = adts.sf_index;
            hDecoder_ptr->channelConfiguration = adts.channel_configuration;        
        }
    }
    
    /* decode the complete bitstream */
    rel = AAC_RawDataBlockParsing(hDecoder_ptr,				   
                                   bit_fifo_ptr, 
                                   &hDecoder_ptr->pce,
                                   aac_dec_mem_ptr,
                                   tmp_cpe_ptr);

    *decoded_bytes = AAC_GetProcessedBits(bit_fifo_ptr) >> 3; 
    if (*decoded_bytes > buffer_size)
        *decoded_bytes = buffer_size;

    if (rel > 0)
        goto AAC_DEC_ERROR;   

    /* no more bit reading after this */
    if (1 == hDecoder_ptr->latm_sign)
    {
        hDecoder_ptr->unit_byte_consumed = (AAC_GetProcessedBits(bit_fifo_ptr) >> 3);
        
        if (hDecoder_ptr->latm_unit_len == hDecoder_ptr->unit_byte_consumed)
        {
            /* unit parsing is end */
            hDecoder_ptr->unit_byte_consumed = 0;
            exit_sign = 0;
        }else if (hDecoder_ptr->latm_unit_len < hDecoder_ptr->unit_byte_consumed)
        {
            /* error processing */
            goto AAC_DEC_ERROR;   
        }else
        {
            /* unit is not end */
            exit_sign = 1;
        }
        
        
    }     
    if (bit_fifo_ptr->error)
    {
        rel = 14;
        goto AAC_DEC_ERROR;
    }    
    *frm_pcm_len              = 1024;
    if (((hDecoder_ptr->sbr_present_flag == 1) || (hDecoder_ptr->forceUpSampling == 1)) && (1 == aacplus_decode_flag))  // SBR output
    {
        *frm_pcm_len              = 2048;
    }
    hDecoder_ptr->frame++;        
    return exit_sign;
AAC_DEC_ERROR:
    *frm_pcm_len                                    = 2048;
    /* when error is checked, the pcm buffer is set 0 */
    hDecoder_ptr->unit_byte_consumed = 0;
    if (0 != rel) 
    {        
        AAC_DEC_MEMSET(pcm_out_l, 0, *frm_pcm_len * sizeof(int16));
        AAC_DEC_MEMSET(pcm_out_r, 0, *frm_pcm_len * sizeof(int16));
    }
    
    *decoded_bytes = AAC_GetProcessedBits(bit_fifo_ptr) >> 3;
    if (*decoded_bytes > buffer_size)
        *decoded_bytes = buffer_size;

    if ((1 == hDecoder_ptr->latm_sign) && (1 == rel))
        rel = 2;

    return rel;
}

int16 AAC_ParsingHeader(AAC_BIT_FIFO_FORMAT_T        *ld_ptr,
						  NeAACDecStruct *hDecoder_ptr)
{	
    int32 bits_to_decode = 0;
    int16 result = 0;
    uint8 object_type = 0;
    AAC_mp4AudioSpecificConfig mp4ASC;
    AAC_PROGRAM_CONFIG_T pce;
    bits_to_decode         = AAC_GetBits(ld_ptr, 13);
    object_type               = (uint8) ((bits_to_decode >> 8) & 0x1F);//(uint8) (AAC_GetBits(ld_ptr, 5));    
    hDecoder_ptr->sf_index                     = (uint8) ((bits_to_decode >> 4) & 0xF);//(uint8) (AAC_GetBits(ld_ptr, 4));	
    hDecoder_ptr->channelConfiguration  = (uint8) (bits_to_decode & 0xF);//(uint8) (AAC_GetBits(ld_ptr, 4));
    hDecoder_ptr->object_type = object_type;
    if (!((object_type == 2) ||
        (object_type == 4) ||
        (object_type == 5)))   /*AAC-LC   */     /*AAC-LTP */    /*AAC-SBR */    
    {
        return 2;  // format not support
    }
    hDecoder_ptr->sbr_present_flag = 127;    
     /* get GASpecificConfig */
    if (object_type == 2)
    {        
        mp4ASC.channelsConfiguration = hDecoder_ptr->channelConfiguration;
        result = GASpecificConfig(ld_ptr, &mp4ASC, &pce);
        /* for SBR checking */
        bits_to_decode = (int8)(ld_ptr->buffer_size*8 - AAC_GetProcessedBits(ld_ptr));	
        if ((hDecoder_ptr->object_type != 5) )
        {
            int16 syncExtensionType = (int16)AAC_GetBits(ld_ptr, 11);		
            if (syncExtensionType == 0x2b7)
            {          			
                hDecoder_ptr->object_type = (uint8)AAC_GetBits(ld_ptr, 5);			
                if (hDecoder_ptr->object_type == 5)
                {
                    hDecoder_ptr->sbr_present_flag = (uint8)AAC_GetBits(ld_ptr, 1);                		
                }
            }
        }
    }  	   
    return 0;
}

uint32 AAC_RetrieveSampleRate(void     *aac_buf_ptr)
{
	AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_buf_ptr;
	NeAACDecHandle      hDecoder_ptr = &aac_dec_struc_ptr->g_decoder;
	return hDecoder_ptr->frame_rate;
}




int16 AAC_DecInit(int8  *headerstream_ptr,  // input header info stream
                    int16  head_length,       // header stream length
                    int32  sample_rate,       // sample rate
                    int16  sign,              // 1: input sample rate, other: inout header stream 
                    void *aac_buf_ptr)      // allocate memory
{
    AAC_BIT_FIFO_FORMAT_T ld;
    int32 tmp;
    int16 rel = 0;
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_buf_ptr;
    NeAACDecHandle      hDecoder_ptr;   // the input relative information	
    int8_t *latm_buf_ptr = (int8 *)headerstream_ptr;
    hDecoder_ptr = &aac_dec_struc_ptr->g_decoder;	
    if (1 == sign)
    {
        if (latm_buf_ptr[0] == 'L' && latm_buf_ptr[1] == 'A' && latm_buf_ptr[2] == 'T' && latm_buf_ptr[3] == 'M')
        {
            hDecoder_ptr->latm_sign = 1;
            if (0x1 == latm_buf_ptr[4])
            {
                hDecoder_ptr->sbr_present_flag = 1;
            }
        }
        hDecoder_ptr->sf_index = AAC_GetSrIndex(sample_rate);
        hDecoder_ptr->frame_rate = sample_rate;
        return 0;
    }else
    {
        uint32  bufa;
        AAC_InitBits(&ld, 
                             (uint8*)headerstream_ptr,
                              head_length);         
        bufa = getdword((uint32*)ld.buffer);	
        tmp = (bufa) >> 20;
        if (0xFFF == tmp)  // ADTS
       {
            AAC_ADTS_HEADER_T adts_strc;
            hDecoder_ptr->adts_header_present = 1;
            AAC_AdtsFrameParsing(&adts_strc, &ld);
            hDecoder_ptr->sf_index = adts_strc.sf_index;
    	}
        else if (bufa == 0x41444946)// ADIF format, now not support	    
    	{
             rel = 1;
    	}
        else
    	{
             rel = AAC_ParsingHeader(&ld, hDecoder_ptr);	
    	}
        hDecoder_ptr->frame_rate = AAC_GetSampleRate(hDecoder_ptr->sf_index);
        if (AAC_GetSampleRate(hDecoder_ptr->sf_index) < 32000)
    	{
            hDecoder_ptr->forceUpSampling = 1;
    	}
        if (1 == hDecoder_ptr->forceUpSampling || (1 == hDecoder_ptr->sbr_present_flag))
        {
            hDecoder_ptr->frame_rate *= 2;
        }	
        return rel;	
    }	
}

int16 AAC_MemoryAlloc(void **aac_mem_ptr)            // memory constrct
{
	AAC_PLUS_DATA_STRUC_T *temp_ptr;
	
	*aac_mem_ptr = (AAC_PLUS_DATA_STRUC_T*) AAC_DEC_MALLOC(sizeof(AAC_PLUS_DATA_STRUC_T));
       if(NULL ==(*aac_mem_ptr))
       {
	    return 1;
       }

	AAC_DEC_MEMSET(*aac_mem_ptr, 0, sizeof(AAC_PLUS_DATA_STRUC_T));
	temp_ptr = (AAC_PLUS_DATA_STRUC_T*) (*aac_mem_ptr);
	temp_ptr->inputbuf = NULL;
	temp_ptr->inputbuf = (uint8 *) AAC_DEC_MALLOC(INPUTBUF_SIZE*sizeof(uint8));
	if(NULL ==(temp_ptr->inputbuf))
    	{
	    return 1;
    	}

    return 0;
}

int16 AAC_MemoryFree(void ** const aac_mem_ptr)            // memory deconstrct

{
	AAC_PLUS_DATA_STRUC_T *temp_ptr;
	temp_ptr = (AAC_PLUS_DATA_STRUC_T*) (*aac_mem_ptr);
	AAC_DEC_FREE(temp_ptr->inputbuf);
	AAC_DEC_FREE(*aac_mem_ptr);
	    
	return 0;
}

int16 AAC_DecStreamBufferUpdate(
                    int16     seek_sign,         // 1: seek mode, 
					void     *aac_buf_ptr)      // allocate memory
{
	AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_buf_ptr;
	NeAACDecHandle      hDecoder_ptr;   // the input relative information
	hDecoder_ptr = &aac_dec_struc_ptr->g_decoder;	
	if (1 == seek_sign)
	{
	    hDecoder_ptr->latm_seek = AAC_DEC_LATM_SEEK;
	}

	return 0;
}
