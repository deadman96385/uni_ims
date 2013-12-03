/********************************************************************************
** File Name:      mp3_huffman.h                                         *
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

# ifndef MP3_DEC_HUFFMAN_H
# define MP3_DEC_HUFFMAN_H

# include "mp3_fixed.h"


union MP3_DEC_HUFFQUAD_U {
  struct {
    uint16 final  :  1;
    uint16 bits   :  3;
    uint16 offset : 12;
  } ptr;
#ifdef MP3_DEC_WORDS_BIGENDIAN
  struct {
    uint16 final  :  1;
    uint16 hlen   :  11; //3
    uint16 v      :  1;
    uint16 w      :  1;
    uint16 x      :  1;
    uint16 y      :  1;
  } value;
#else
  struct {
      uint16 final  :  1;
      uint16 hlen   :  3;
      uint16 v      :  1;
      uint16 w      :  1;
      uint16 x      :  1;
      uint16 y      :  1;
  } value;
#endif
  uint16 final    :  1;
};

union MP3_DEC_HUFFPAIR_U {
  struct {
    uint16 final  :  1;
    uint16 bits   :  3;
    uint16 offset : 12;
  } ptr;
#ifdef MP3_DEC_WORDS_BIGENDIAN
  struct {
    uint16 final  :  1;
    uint16 hlen   :  3;
    uint16 x      :  6;
    uint16 y      :  6;
  } value;
#else
  struct {
      uint16 final  :  1;
      uint16 hlen   :  3;
      uint16 x      :  4;
      uint16 y      :  4;
  } value;
#endif
  uint16 final    :  1;
};

typedef struct hufftable {
  union MP3_DEC_HUFFPAIR_U const *table;
  int32 linbits;
} MP3_DEC_HUFF_TABLE_T;

extern union MP3_DEC_HUFFQUAD_U const *const mp3_huff_quad_table[2];
extern struct hufftable const mp3_huff_pair_table[32];
extern uint8 const MP3_DEC_pre_tab[22];
extern int32 const MP3_DEC_BitmaskTable[];
extern const int32 MP3_DEC_iq_table[];
extern int32 const MP3_DEC_root_table[7];

#endif
