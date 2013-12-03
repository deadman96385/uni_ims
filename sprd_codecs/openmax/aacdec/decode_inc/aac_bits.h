/*************************************************************************
** File Name:      bits.h                                                *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __BITS_H__
#define __BITS_H__

#ifdef __cplusplus
extern "C" {
#endif



#define TMP_BUFFER_SIZE 1536

#define BYTE_NUMBIT 8
#define bit2byte(a) ((a+7)/BYTE_NUMBIT)


typedef struct _bitfile
{
    /* bit input */
    uint32  * cur_stream_ptr;
    uint16  bits_left;
    uint16  bytes_used;
    uint32  buffer_size; /* size of the buffer in bytes */
    uint8  *buffer;//[TMP_BUFFER_SIZE];   
    //uint32 *tail;
//uint32 *start;
    uint8   no_more_reading;
    uint8   error;
    uint16  res0;
} AAC_BIT_FIFO_FORMAT_T;





extern const uint32 AAC_DEC_bit_mask[];

void AAC_InitBits(AAC_BIT_FIFO_FORMAT_T *ld, uint8 *buffer, uint32 buffer_size);


/* circumvent memory alignment errors on ARM */


#ifdef AAC_DEC_LITTLE_ENDIAN
#define AAC_DEC_BSWAP(a) \
    ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))
static INLINE uint32 getdword(void *mem)
{
    uint32 tmp;
    tmp = *(uint32*)mem;
    AAC_DEC_BSWAP(tmp);
    return tmp;
}
#else
#define getdword(mem) (*(uint32*)mem) // for arm platform
#endif



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

uint8 AAC_ByteAlign(AAC_BIT_FIFO_FORMAT_T *ld_ptr);
uint32 AAC_GetProcessedBits(AAC_BIT_FIFO_FORMAT_T *ld_ptr);
//void faad_flushbits_ex(AAC_BIT_FIFO_FORMAT_T *ld_ptr, uint32 bits);





	
//////////////////////////////////////////////////////////////////////////
/* for ARM */
extern int32 AAC_ShowBitsAsm(AAC_BIT_FIFO_FORMAT_T *ld_ptr, int16 n);

#ifdef AAC_DEC_LITTLE_ENDIAN

#define AAC_ShowBits AAC_ShowBitsAsm
#else
#define AAC_ShowBits(ld_ptr, bits) \
	((bits <= ld_ptr->bits_left) ? \
	((*ld_ptr->cur_stream_ptr) << (32 - ld_ptr->bits_left) >> (32 - bits)) \
	: (((*ld_ptr->cur_stream_ptr) & AAC_DEC_bit_mask[ld_ptr->bits_left]) << (bits - ld_ptr->bits_left)) \
| (((ld_ptr->cur_stream_ptr[1])) >> (32 - (bits - ld_ptr->bits_left))))\


#endif


#define faad_flushbits_ex(ld_ptr, bits)		   \
	ld_ptr->bits_left += (uint16)(32 - bits);	       \
	ld_ptr->cur_stream_ptr++ ;                  \
	ld_ptr->bytes_used = (uint16)(ld_ptr->bytes_used+4);      \



#define AAC_FlushBits(ld_ptr, bits)		    \
	if (0 == ld_ptr->error )                \
{                                   \
	if (bits < ld_ptr->bits_left)       \
{								\
				ld_ptr->bits_left = (uint16)(ld_ptr->bits_left- bits);		\
} else {						\
				faad_flushbits_ex(ld_ptr, bits);\
}	                            \
}									\

/* return next n bits (right adjusted) */

//uint32 AAC_GetBits(AAC_BIT_FIFO_FORMAT_T *ld_ptr, uint32 n );
extern __inline int32 AAC_GetBits(AAC_BIT_FIFO_FORMAT_T *ld_ptr, uint32 n );


uint8 AAC_Get1Bit(AAC_BIT_FIFO_FORMAT_T *ld_ptr );

#ifdef __cplusplus
}
#endif
#endif
