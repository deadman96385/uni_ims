/*************************************************************************
** File Name:      bits.c                                                *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is for aac bit-stream parsing
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_bits.h"

const uint32 AAC_DEC_bit_mask[] = {
    0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF,
    0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF,
    0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF,
    0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

void AAC_InitBits(AAC_BIT_FIFO_FORMAT_T *ld_ptr, uint8 *buffer_ptr, uint32 buffer_size)
{
	
    if (ld_ptr == NULL)
        return;
    AAC_DEC_MEMSET(ld_ptr, 0, sizeof(AAC_BIT_FIFO_FORMAT_T));
	
    if (buffer_size == 0 || buffer_ptr == NULL)
    {
        ld_ptr->error = 1;
        ld_ptr->no_more_reading = 1;
        return;
    }
#if 0
    AAC_DEC_MEMSET(ld_ptr->buffer, 0, (TMP_BUFFER_SIZE)*sizeof(uint8));        // (buffer_size+12) as (buffer_size)
	if (buffer_size > TMP_BUFFER_SIZE)
		buffer_size = TMP_BUFFER_SIZE;
    AAC_DEC_MEMCPY(ld_ptr->buffer, buffer_ptr, buffer_size*sizeof(uint8));
#else
	ld_ptr->buffer = buffer_ptr;    
#endif
    ld_ptr->buffer_size = buffer_size;
	
	
	
	ld_ptr->cur_stream_ptr = (uint32*)ld_ptr->buffer;
	
    ld_ptr->bits_left = 32;
	
    ld_ptr->bytes_used = 0;
    ld_ptr->no_more_reading = 0;
    ld_ptr->error = 0;
}





uint32 AAC_GetProcessedBits(AAC_BIT_FIFO_FORMAT_T *ld_ptr)
{
    return (uint32)(8 * ld_ptr->bytes_used + (32 - ld_ptr->bits_left));
}

uint8 AAC_ByteAlign(AAC_BIT_FIFO_FORMAT_T *ld_ptr)
{
    uint8 remainder = (uint8)((32 - ld_ptr->bits_left) % 8);
	
    if (remainder)
    {
        AAC_FlushBits(ld_ptr, (8 - remainder));
        return  (uint8)(8 - remainder);
    }
    return 0;
}



uint8 AAC_Get1Bit(AAC_BIT_FIFO_FORMAT_T *ld_ptr )
{
    uint8 r;
	
	r = (uint8) (AAC_GetBits(ld_ptr, 1));
	
    return r;
}