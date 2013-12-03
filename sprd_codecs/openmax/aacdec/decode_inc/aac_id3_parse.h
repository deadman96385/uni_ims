/*************************************************************************
** File Name:      id3_parse.h                                           *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#ifndef ___D3_PARSE_H__
#define ___D3_PARSE_H__
#include "aac_common.h"

#ifdef __cplusplus
extern "C" {
#endif	
	
	

	

uint16 Id3Parse(
				 AUDIO_ID3_TAG_T  *s_ptr, 
				 uint8    *src_ptr,
				 uint16    in_buf_len);

#ifdef __cplusplus
}
#endif
#endif