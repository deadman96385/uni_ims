/*************************************************************************
** File Name:      sbr_fbt.h                                             *
** Author:         Reed zhang                                            *
** Date:           09/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    this file is use to Calculate frequency band tables
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_FBT_H__
#define __SBR_FBT_H__

#ifdef __cplusplus
extern "C" {
#endif

uint8 AAC_QmfStartChannel(uint8 bs_start_freq,      // start t of the master frequency table 
						  uint8 bs_samplerate_mode, // define whether multi-rate or single-rate SBR mode is used
                          uint8 sr_index);          // the SBR sample rate index

uint8 AAC_QmfStopChannel(uint8 bs_stop_freq,        // stop t of the master frequency table 
						 uint8 sr_index,
                         uint8 k0); 
        
uint8 AAC_MasterFrequencyTable_fs0(AAC_SBR_INFO_T *sbr_ptr,     // the SBR decoder  structure
								   uint8 k0,
								   uint8 k2,
                                   uint8 bs_alter_scale,
								   int32 *tmp_buf_ptr);   // define the frequency envelope bands grouping

uint8 AAC_MasterFrequencyTable(AAC_SBR_INFO_T *sbr_ptr, 
							   uint8 k0, 
							   uint8 k2,
                               uint8 bs_freq_scale,
							   uint8 bs_alter_scale,
							   int32 *tmp_buf_ptr);

uint8 AAC_DerivedFrequencyTable(AAC_SBR_INFO_T *sbr_ptr, 
								uint8 bs_xover_band,
                                uint8 k2);
void AAC_LimiterFrequencyTable(AAC_SBR_INFO_T *sbr_ptr,
							   int32 *tmp_buf_ptr);


#ifdef __cplusplus
}
#endif
#endif

