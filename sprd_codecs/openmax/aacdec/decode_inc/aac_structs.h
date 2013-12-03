/*************************************************************************
** File Name:      structs.h                                             *
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

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#ifdef __cplusplus
extern "C" {
#endif

//#include "cfft.h"
#include "aac_common.h"
#ifdef AAC_HE_AAC_EXTENSION
#include "AAC_sbr_dec.h"
#endif
	

#define AAC_DEC_MAX_CHANNEL         2
#define MAX_SYNTAX_ELEMENTS  2
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB              51

#define AAC_SCE_CHANNEL_MAX 2
/* used to save the prediction state */
typedef struct {
    int16 r[2];
    int16 COR[2];
    int16 VAR[2];
} pred_state;

/* LTP mode */
#define  AAC_DEC_LTP_MAX_SFB 40
#define  AAC_DEC_LTP_PREDICTION_STAT 1024 * 3
typedef struct AAC_DEC_LTP_INFO_STRUC
{
    int16  last_band;
    int16  data_present;    
    int8   long_used[AAC_DEC_LTP_MAX_SFB];    
    int16  lag;
    int16  lag_update;
    int16  coef;
    int8   sf_index;
    int8   object_type;
        
    int8   short_used[8];
    int8   short_lag_present[8];
    int8   short_lag[8];
    int16  last_pcm_data[1024];
    int16  *ltp_pred_sample;//[AAC_DEC_LTP_PREDICTION_STAT];
} AAC_DEC_LTP_INFO_T;


typedef struct
{
    uint8 element_instance_tag;
    uint8 profile;
    uint8 sf_idx;
    uint8 num_front_channel_elements;
    uint8 num_side_channel_elements;
    uint8 num_back_channel_elements;
    uint8 num_lfe_channel_elements;
    uint8 num_assoc_data_elements;
    uint8 num_valid_cc_elements;
    uint8 mono_mixdown_present;
    uint8 mono_mixdown_element_number;
    uint8 stereo_mixdown_present;
    uint8 stereo_mixdown_element_number;
    uint8 matrix_mixdown_idx_present;
    uint8 pseudo_surround_enable;
    uint8 matrix_mixdown_idx;
    uint8 front_element_is_cpe[16];
    uint8 front_element_tag_select[16];
    uint8 side_element_is_cpe[16];
    uint8 side_element_tag_select[16];
    uint8 back_element_is_cpe[16];
    uint8 back_element_tag_select[16];
    uint8 lfe_element_tag_select[16];
    uint8 assoc_data_element_tag_select[16];
    uint8 cc_element_is_ind_sw[16];
    uint8 valid_cc_element_tag_select[16];
    uint8 channels;
    uint8 comment_field_bytes;
    uint8 comment_field_data[257];

    /* extra added values */
    uint8 num_front_channels;
    uint16 num_side_channels;
    uint16 num_back_channels;
    uint16 num_lfe_channels;    
    uint8 sce_channel[AAC_SCE_CHANNEL_MAX];
    uint8 cpe_channel[16];
} AAC_PROGRAM_CONFIG_T;

typedef struct
{
    uint16 syncword;
    uint16 aac_frame_length;
    uint16 adts_buffer_fullness;
    uint16 crc_check;
    uint8 id;
    uint8 layer;
    uint8 protection_absent;
    uint8 profile;
    uint8 sf_index;
    uint8 private_bit;
    uint8 channel_configuration;
    uint8 original;
    uint8 home;
    uint8 emphasis;
    uint8 copyright_identification_bit;
    uint8 copyright_identification_start;    
    uint8 no_raw_data_blocks_in_frame;
    /* control param */
    uint8 old_format;
} AAC_ADTS_HEADER_T;


typedef struct
{
    uint16 number_pulse;
    uint16 pulse_start_sfb;
    uint8 pulse_offset[4];
    uint8 pulse_amp[4];
} AAC_PULSE_INFO_T;

typedef struct
{
    uint8 n_filt[8];
    uint8 coef_res[8];
    uint8 length[8][4];
    uint8 order[8][4];
    uint8 direction[8][4];
    uint8 coef_compress[8][4];
    uint8 coef[8][4][32];
} AAC_TNS_INFO_T;

typedef uint16 u16_complex120[120];
typedef int16  s16_complex51[51];

typedef struct
{
    u16_complex120 *sect_sfb_offset;
    u16_complex120 *sect_start0;
    u16_complex120 *sect_end;	
    s16_complex51  *scale_factors;    
    uint8    num_swb;      // 16 byte
    uint8    num_window_groups;
    uint8    num_windows;
    uint8    window_sequence;    
    
    uint8    max_sfb;      // 20 bytes
    uint8    window_shape;
    uint8    scale_factor_grouping;
    uint8    global_gain;    
    
    uint8    ms_mask_present;        // 24 bytes 
    uint8    noise_used;
    uint8    pulse_data_present;
    uint8    tns_data_present;
    
    uint8    gain_control_data_present;     // 28 bytes
    uint8    predictor_data_present;
    uint8    t0;
    uint8    t1;
    
    uint8    window_group_length[8];     // 32 bytes
    uint16   swb_offset[52]; // 40 byte
    uint8    sfb_cb [8][51];	
    uint8    sect_cb[8][51];    
    uint8    ms_used[MAX_WINDOW_GROUPS][MAX_SFB];
    uint8    num_sec[8]; /* number of sections in a group */
    AAC_PULSE_INFO_T pul;
    AAC_TNS_INFO_T   tns;
    /* LTP mode */
    AAC_DEC_LTP_INFO_T ltp_data_s;
} AAC_ICS_STREAM_T; /* individual channel stream */

typedef struct
{
    
    int8 paired_channel;
    uint8 channel;
    uint8 element_instance_tag;
    uint8 common_window;

    AAC_ICS_STREAM_T ics1;
    AAC_ICS_STREAM_T ics2;
} AAC_ELEMENT_T; /* syntax element (SCE, CPE, LFE) */

typedef struct NeAACDecConfiguration
{
    uint8 defObjectType;    
    uint8 outputFormat;
    uint8 downMatrix;
    uint8 useOldADTSFormat;
    
    uint8 dontUpSampleImplicitSBR;
    uint8 res0;
    uint8 res1;
    uint8 res2;        
    
    uint32 defSampleRate;
} NeAACDecConfiguration, *NeAACDecConfigurationPtr;

typedef struct
{
    uint8 adts_header_present;
    uint8 adif_header_present;
    uint8 latm_sign;
    uint8 sf_index;
    
    uint8 object_type;
    uint8 channelConfiguration;    
    uint8 postSeekResetFlag;
    uint8 downMatrix;
    
    uint8 upMatrix;
    uint8 first_syn_ele;
    uint8 has_lfe;
    /* number of channels in current frame */
    uint8 fr_channels;    
    
    /* element_output_channels:
       determines the number of channels the element will output
    */
    uint8 element_output_channels[MAX_SYNTAX_ELEMENTS];
    uint8 element_alloced[MAX_SYNTAX_ELEMENTS];
    /* alloced_channels:
       determines the number of channels where output data is allocated for
    */
    uint8 alloced_channels;
    uint8 pce_set;   
    uint8 window_shape_prev[AAC_DEC_MAX_CHANNEL];      
     
    
    uint8 element_id[AAC_DEC_MAX_CHANNEL];
    uint8 internal_channel[AAC_DEC_MAX_CHANNEL];
    /* Configuration data */
    
	
    uint16 frameLength;
    uint16 unit_byte_consumed;
    
    uint16 latm_unit_len;
    /* number of elements in current frame */
    uint16 fr_ch_ele;
    
    uint32 frame;
    uint32 frame_rate;    
    

    uint8  stereo_mono;   // 1, for the true stereo, 0, for the false stereo.
    uint8  frame_dec_id;     // 1, indicate the current frame is not decoded, or for 0
    uint8  ps_used[MAX_SYNTAX_ELEMENTS];
    
     int16  aacplus_decode_flag;
    int16   sbr_present_flag;
    
    int16   forceUpSampling;
    uint8  sbr_alloced[MAX_SYNTAX_ELEMENTS];    /* determines whether SBR data is allocated for the gives element */
    AAC_PROGRAM_CONFIG_T pce;
    NeAACDecConfiguration config;
    uint32 latm_seek;
    AAC_SBR_INFO_T *sbr;
    
} NeAACDecStruct, *NeAACDecHandle;
typedef struct AAC_mp4AudioSpecificConfig
{
    /* Audio Specific Info */
    uint8  objectTypeIndex;
    uint8  samplingFrequencyIndex;    
    uint8  channelsConfiguration;
    /* GA Specific Info */
    uint8  frameLengthFlag;
    uint8  dependsOnCoreCoder;    
    uint8  extensionFlag;
    uint8  aacSectionDataResilienceFlag;
    uint8  aacScalefactorDataResilienceFlag;
    uint8  aacSpectralDataResilienceFlag;
    uint8  epConfig;
    uint8  sbr_present_flag;
    uint8  forceUpSampling;
    uint8  downSampledSBR;
    uint8  res;
    uint16 coreCoderDelay;
    uint32 samplingFrequency;
} AAC_mp4AudioSpecificConfig;
//////////////////////////////////////////////////////////////////////////
typedef struct
{
        int32   g_aac_overlap_l[1024];
        int32   g_aac_overlap_r[1024];
        NeAACDecStruct       g_decoder;   // when pause, the parametr is need to be saved for variant mode
        int32                            g_qmf_ana[2][320];
        int32                            g_qmf_syn[2][1280];
        AAC_ELEMENT_T   g_cpe;
        int32                            g_shared_buffer[5376];
        AAC_SBR_INFO_T  g_sbr_info;
        AAC_BIT_FIFO_FORMAT_T  g_bit_fifo;
        uint16                                        *pcm_out_l_ptr;
        uint16                                        *pcm_out_r_ptr;
	 uint8	*inputbuf;
} AAC_PLUS_DATA_STRUC_T;
typedef struct
{
       int32   *overlap_data_ptr;   // 0
       int32   *pcm_filter_ptr;     // 4
       int32   *overlap_filter_ptr; // 8
       int32   *input_data1_ptr;    // 12
       int32   *tmp_buf_ptr;        // 16
}AAC_DATA_PARA_STRUC_T;
/**/
typedef union _AAC_DEC_U64 {
    int64 w64;
    struct {
            /* ARM ADS = little endian */
              uint32 low32;
              int32     high32;
        } part;
} AAC_DEC_U64;

#ifdef __cplusplus
}
#endif
#endif
