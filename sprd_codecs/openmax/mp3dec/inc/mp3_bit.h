/********************************************************************************
**  File Name: 	imdct.c											                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:                                  						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/

# ifndef MP3_DEC_BIT_H
# define MP3_DEC_BIT_H
# include "mp3_bit.h"
# include "mp3_fixed.h"

typedef struct _Mp3Bitptr 
{ 
#ifdef MP3_DEC_WORDS_BIGENDIAN
  uint16 cache;
  uint16 left;
#else
  uint16 left;
  uint16 cache;
#endif
  uint32 const *word_ptr;
  uint8  const *byte_ptr;
  
} MP3_DEC_BIT_POOL_T;

# define MP3_DEC_bit_finish(bitptr)		/* nothing */

void  MP3_DEC_BitPoolInit(MP3_DEC_BIT_POOL_T *bit_ptr, 
                          uint8 const *byte_ptr);

uint32 MP3_DEC_CalcBitLen(MP3_DEC_BIT_POOL_T const *begin_ptr,
                          MP3_DEC_BIT_POOL_T const *end_ptr);

uint8 const *MP3_DEC_BitNextByte(MP3_DEC_BIT_POOL_T const *bit_pool_ptr);

void   MP3_DEC_BitSkip(MP3_DEC_BIT_POOL_T *bitptr, 
                       uint32 len);
uint32 MP3_DEC_8BitRead(MP3_DEC_BIT_POOL_T *bitptr, 
                        uint32 len);
#if !defined(ASO_HUFFMAN)
uint32 MP3_DEC_32BitRead(MP3_DEC_BIT_POOL_T *bitptr,
                         uint32 len);
#endif
uint16 MP3_DEC_BIT_CRCCheck(MP3_DEC_BIT_POOL_T bit_pool, 
                            uint32 len,
                            uint16 init);


/* asm */
#if defined(ASO_HUFFMAN)
extern uint32 MP3_DEC_32BitReadAsm(MP3_DEC_BIT_POOL_T *bitptr, 
                                   int32 len);
#define MP3_DEC_32BitRead  MP3_DEC_32BitReadAsm
#endif

# endif

