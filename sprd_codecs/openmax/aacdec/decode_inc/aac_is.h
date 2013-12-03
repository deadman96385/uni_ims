/*************************************************************************
** File Name:      aac_is.h                                                                                          *
** Author:         Reed zhang                                                                                      *
** Date:           30/11/2005                                                                                      *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.                            *
** Description:    This file defines the IS model function interface                                  *
**                 
**                        Edit History                                                                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                                                              *
** 30/11/2005     Reed zhang       Create.                                                                 *
*************************************************************************/

#ifndef __AAC_IS_H__
#define __AAC_IS_H__

#ifdef __cplusplus
extern "C" {
#endif
	
void AAC_IsDecode(AAC_ICS_STREAM_T *icsl,    // the left channel information 
		AAC_ICS_STREAM_T *icsr,    // the right channel information 
		int32 *l_spec,     // the left channel data
		int32 *r_spec);    // the right channel data
	
static INLINE int8 is_intensity(uint8 sfb)
	{
		switch (sfb)
		{
		case AAC_INTENSITY_HCB:
			return 1;
		case AAC_INTENSITY_HCB2:
			return -1;
		default:
			return 0;
		}
	}

	
static INLINE int8 invert_intensity(AAC_ICS_STREAM_T *ics, uint8 group, uint8 sfb)
	{
		if (ics->ms_mask_present == 1)
        return (int8)(1-2*ics->ms_used[group][sfb]);
		return 1;
	}
	
#ifdef __cplusplus
}
#endif
#endif
