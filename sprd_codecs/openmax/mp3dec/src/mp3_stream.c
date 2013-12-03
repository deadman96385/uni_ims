/********************************************************************************
**  File Name: 	mp3_stream.c									                *
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

//# include "mp3_global.h"
# include "mp3_bit.h"
# include "mp3_stream.h"

/*
 * FUNCTION NAME:	MP3_DEC_StreamInit
 * FUNCTION DESCRIPTION:	initialize stream_ptr struct
 */
void MP3_DEC_StreamInit(MP3_STREAM_T *stream_ptr)
{
  stream_ptr->buffer     = 0;
  stream_ptr->bufend     = 0;
  stream_ptr->skiplen    = 0;
  stream_ptr->sync       = 0;
  stream_ptr->freerate   = 0;
  stream_ptr->this_frame = 0;
  stream_ptr->next_frame = 0;
  MP3_DEC_BitPoolInit(&stream_ptr->ptr, 0);
  MP3_DEC_BitPoolInit(&stream_ptr->anc_ptr, 0);
  stream_ptr->anc_bitlen = 0;
//  stream_ptr->main_data  = 0;
  stream_ptr->md_len     = 0;
  stream_ptr->options    = 0;
  stream_ptr->error      = MP3_ERROR_NONE;
}


/*
 * FUNCTION NAME:	MP3_DEC_StreamBufferSet
 * FUNCTION DESCRIPTION:	set stream_ptr buffer pointers
 */
void MP3_DEC_StreamBufferSet(MP3_STREAM_T *stream_ptr,
		                     uint8 const *buffer_ptr, 
                             uint32 length)
{
  stream_ptr->buffer = buffer_ptr;
  stream_ptr->bufend = buffer_ptr + length;

  stream_ptr->this_frame = buffer_ptr;
  stream_ptr->next_frame = buffer_ptr;

  stream_ptr->sync = 1;

  MP3_DEC_BitPoolInit(&stream_ptr->ptr, buffer_ptr);
}


/*
 * FUNCTION NAME:	MP3_DEC_StreamSync
 * FUNCTION DESCRIPTION:	locate the next stream_ptr sync word

int32 MP3_DEC_StreamSync(MP3_DEC_STREAM_T *stream_ptr)
{
  uint8 const *ptr, *end;

  ptr = MP3_DEC_BitNextByte(&stream_ptr->ptr);
  end = stream_ptr->bufend;

  while (ptr < end - 1 &&!(ptr[0] == 0xff && (ptr[1] & 0xe0) == 0xe0))
    ++ptr;

  if (end - ptr < MP3_BUFFER_GUARD)
    return -1;

  MP3_DEC_BitPoolInit(&stream_ptr->ptr, ptr);

  return 0;
} */


