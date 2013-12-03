/*************************************************************************
** File Name:      tns.h                                                 *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:                                                          *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __TNS_H__
#define __TNS_H__

#ifdef __cplusplus
extern "C" {
#endif


#define TNS_MAX_ORDER 20

    
void AAC_TnsDecodeFrame(AAC_ICS_STREAM_T *ics, 
					  AAC_TNS_INFO_T *tns,
					  uint8 sr_index,
                      uint8 object_type, 
					  int32 *spec);
					  
int16 AAC_DEC_TNSFrmAnalysisFilter(int32              *spec_ptr,
                                   AAC_DEC_LTP_INFO_T *ltp_info_ptr,
                                   AAC_ICS_STREAM_T   *ics_ptr
                                  );					  
#ifdef __cplusplus
}
#endif
#endif
