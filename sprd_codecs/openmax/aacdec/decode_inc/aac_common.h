/*************************************************************************
** File Name:      huffman.c                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is for AAC huffamn decoder
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

//#ifdef AAC_TEST   // for AAC test
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#else
//#include "sfs.h"
//#include "audio_api.h"
//#include "asm.h"
//#include "aac_adp.h"
//#endif

#define AAC_DEC_LITTLE_ENDIAN
/************************************************************************/
/*                                                                      */
/************************************************************************/
#if 0//ndef AAC_TEST   // for AAC test

#define AAC_DEC_MEMSET(aac_data, aac_n, aac_m)       SCI_MEMSET(aac_data, aac_n, aac_m)
#define AAC_DEC_MEMCPY(aac_des, aac_src, aac_m)      SCI_MEMCPY(aac_des, aac_src, aac_m)
#define AAC_DEC_FREE(file_ptr)						 SCI_FREE(file_ptr)
#define AAC_DEC_MALLOC(aac_m)						 AUDIO_ALLOC(aac_m)
#define AAC_DEC_MEMMOVE(tar, src, m)                 memmove(tar, src, m)	
#else  // for PC mode
#define SCI_NULL                                 0
#define PNULL                                    0
#define AAC_DEC_MEMSET(aac_data, aac_n, aac_m)       memset(aac_data, aac_n, aac_m)
#define AAC_DEC_MEMCPY(aac_des, aac_src, aac_m)      memcpy(aac_des, aac_src, aac_m)
#define AAC_DEC_FREE(x)						              {if(x) {free(x); x = NULL;}}
#define AAC_DEC_MALLOC(aac_m)						             malloc(aac_m)
#define   SCI_MEMSET AAC_DEC_MEMSET
#define AAC_DEC_MEMMOVE(tar, src, m)                 memmove(tar, src, m)
#endif


//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* global variant                                                       */
/************************************************************************/


/* object types for AAC */
#define AAC_MAIN       1
#define AAC_LC         2
#define AAC_SSR        3
#define AAC_LTP        4
#define AAC_SBR        5


/* Bitstream */
#define AAC_LEN_SE_ID 3
#define AAC_LEN_TAG   4
#define AAC_LEN_BYTE  8

#define AAC_EXT_FIL            0
#define AAC_EXT_FILL_DATA      1
#define AAC_EXT_DATA_ELEMENT   2
#define AAC_EXT_DYNAMIC_RANGE 11
#define AAC_ANC_DATA           0

/* Syntax elements */
#define AAC_ID_SCE 0x0
#define AAC_ID_CPE 0x1
#define AAC_ID_CCE 0x2
#define AAC_ID_LFE 0x3
#define AAC_ID_DSE 0x4
#define AAC_ID_PCE 0x5
#define AAC_ID_FIL 0x6
#define AAC_ID_END 0x7

#define AAC_ONLY_LONG_SEQUENCE   0x0
#define AAC_LONG_START_SEQUENCE  0x1
#define AAC_EIGHT_SHORT_SEQUENCE 0x2
#define AAC_LONG_STOP_SEQUENCE   0x3

#define AAC_ZERO_HCB       0
#define AAC_FIRST_PAIR_HCB 5
#define AAC_ESC_HCB        11
#define AAC_QUAD_LEN       4
#define AAC_PAIR_LEN       2
#define AAC_NOISE_HCB      13
#define AAC_INTENSITY_HCB2 14
#define AAC_INTENSITY_HCB  15

#define AAC_INVALID_SBR_ELEMENT 255


#define INLINE __inline
#define ALIGN


#ifndef AAC_DEC_MAX
#define AAC_DEC_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef AAC_DEC_MIN
#define AAC_DEC_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define FIXED_POINT
#define QMF_RE(A) RE(A)
#define QMF_IM(A) IM(A)




typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long    int64;
typedef long           int32;
typedef short          int16;
typedef signed char int8;



typedef int32 aac_complex[2];
typedef int16 sh_complex_t[2];
#define RE(A) A[0]
#define IM(A) A[1]

//////////////////////////////////////////////////////////////////////////
#define AAC_HE_AAC_EXTENSION
#define AAC_PS_DEC
#define AAC_SBR_DEC
#define AAC_FIXFIX 0
#define AAC_FIXVAR 1
#define AAC_VARFIX 2
#define AAC_VARVAR 3
/* PS MODEL */
#define _AAC_PS_BASE_LINE_

#define AAC_LO_RES 0
#define AAC_HI_RES 1
#define AAC_T_HFGEN 8
#define AAC_T_HFADJ 2

#define AAC_NO_TIME_SLOTS_960 15
#define AAC_NO_TIME_SLOTS     16
#define AAC_RATE              2
#define AAC_NOISE_FLOOR_OFFSET 6

#define SBR_CLIP(val, min, max) ((val>max) ? (val = max):((val < min) ? (val = min) : (val)))

#define AAC_INV_TAB_BIT 12

#define AAC_EXT_SBR_DATA     13
#define AAC_EXT_SBR_DATA_CRC 14



		
	

 /*! \typedef AAC_CALLBACK_PFUNC
 *  \brief AAC callback function prototype. 
 */
typedef void  (* AAC_CALLBACK_PFUNC) ( 
    signed short *data_buf_ptr,    /*!<the left channel data after decoding.*/  
    signed short *data_buf_ptr_r,  /*!<the right channel data after decoding.*/
    unsigned long int cur_samplerate  /*!<the samplerate of this frame data.*/
    );

uint8 AAC_MaxPredSfb(const uint8 sr_index);
uint8 AAC_MaxTnsSfb(const uint8 sr_index, 
	             const uint8 object_type,
                    const uint8 is_short);
uint32 AAC_GetSampleRate(const uint8 sr_index);
uint8 AAC_GetSrIndex(const uint32 samplerate);


extern int32 g_frm_counter;
/* ERROR MACRO DEFINE */
#define AAC_ProgramConfigEement_ERROR       0x0001	

#define AAC_DEC_LATMCOUNTER_LIMIT 10
#define AAC_DEC_LATM_SEEK         0xFF  // 0xFF mean LATM seek mode

//#define PS_TEST_DATA
#define  TEST_FRAME 0
//#define AAC_TEST_DATA
//#define GEN_TEST_DATA
#ifdef __cplusplus
}
#endif
#endif
