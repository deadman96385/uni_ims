/********************************************************************************
** File Name:      mp3_stream.h                                          *
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

# ifndef MP3_DEC_STREAM_H
# define MP3_DEC_STREAM_H

# include "mp3_bit.h"

# define MP3_BUFFER_GUARD	8
# define MP3_BUFFER_MDLEN	1940//(511 + 2048 + MP3_BUFFER_GUARD)
#  define MP3_DEC_CHAR_BIT  8


#define MP3_DEC_MEMSET(ptr, m, size)  memset(ptr, m, size)

/* we must take care that sz >= bits and sz < sizeof(long) lest bits == 0 */
# define MP3_DEC_MASK(cache, sz, bits)	\
(((cache) >> ((sz) - (bits))) & MP3_DEC_BitmaskTable[bits]) //((1 << (bits)) - 1))
# define MP3_DEC_MASK1BIT(cache, sz)  \
((cache) & (1 << ((sz) - 1)))

#define        MP3_DEC_COUNT1TABLE_SELECT  0x01
#define        MP3_DEC_SCLAE_FACTOR                0x02
#define        MP3_DEC_PRE_FLAG	                    0x04
#define        MP3_MIXED_BLOCK_FLAG               0x08
#define  MP3_ERROR_NONE	        0x0000	/* no error */
#define  MP3_ERROR_BUFLEN	        0x0100	/* input buffer too small (or EOF) */




#define  MP3_ERROR_BUFPTR	        0x0100	/* invalid (null) buffer pointer */
  //MP3_ERROR_NOMEM	       = 0x0031,	/* not enough memory */
#define  MP3_ERROR_LOSTSYNC	0x0101	/* lost synchronization */
#define  MP3_ERROR_BADLAYER	0x0102	/* reserved header layer value */
#define  MP3_ERROR_BADBITRATE	0x0103	/* forbidden bitrate value */
#define  MP3_ERROR_BADSAMPLERATE    0x0104	/* reserved sample frequency value */
#define  MP3_ERROR_BADEMPHASIS	  0x0105	/* reserved emphasis value */
#define  MP3_ERROR_BADCRC	                  0x0201	/* CRC check failed */
#define  MP3_ERROR_BADBITALLOC	  0x0211	/* forbidden bit allocation value */
#define  MP3_ERROR_BADSCALEFACTOR 0x0221	/* bad scalefactor index */
#define  MP3_ERROR_BADMODE                 0x0222	/* bad bitrate/mode combination */
#define  MP3_ERROR_BADFRAMELEN	  0x0231	/* bad frame length */
#define  MP3_ERROR_BADBIGVALUES       0x0232	/* bad big_values count */
#define  MP3_ERROR_BADBLOCKTYPE      0x0233	/* reserved block_type */
#define  MP3_ERROR_BADSCFSI	          0x0234	/* bad scalefactor selection info */
#define  MP3_ERROR_BADDATAPTR	          0x0235	/* bad main_data_begin pointer */
#define  MP3_ERROR_BADPART3LEN	  0x0236	/* bad audio data length */
#define  MP3_ERROR_BADHUFFTABLE       0x0237	/* bad Huffman table select */
#define  MP3_ERROR_BADHUFFDATA	   0x0238	/* Huffman data overrun */
#define  MP3_ERROR_BADSTEREO	           0x0239	/* incompatible block_type for JS */
//  MP3_ERROR_FREQ  	       = 0x0240,	/* freq data overrun */

# define MP3_RECOVERABLE(error)	((error) & 0xff00)

typedef struct _Mp3Stream
{
  uint8 const *buffer;		/* input bitstream buffer */
  uint8 const *bufend;		/* end of buffer */
  uint32 skiplen;		/* bytes to skip before next frame */

  int sync;				/* stream sync found */
  uint32 freerate;		/* free bitrate (fixed) */

  uint8 const *this_frame;	/* start of current frame */
  uint8 const *next_frame;	/* start of next frame */
  MP3_DEC_BIT_POOL_T ptr;		/* current processing bit pointer */

  MP3_DEC_BIT_POOL_T anc_ptr;		/* ancillary bits pointer */
  uint32 anc_bitlen;		/* number of ancillary bits */

  uint8 main_data[MP3_BUFFER_MDLEN];
					/* Layer III main_data() */
  uint32 md_len;			/* bytes in main_data */

  int32 options;				/* decoding options (see below) */
  int32 error;			/* error code (see above) */
} MP3_STREAM_T;



void MP3_DEC_StreamInit(MP3_STREAM_T *stream_ptr);


void MP3_DEC_StreamBufferSet(MP3_STREAM_T *stream_ptr,
                             uint8 const *buffer, 
                             uint32 length);


# endif