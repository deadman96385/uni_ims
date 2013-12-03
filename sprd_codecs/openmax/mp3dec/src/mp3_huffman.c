/********************************************************************************
**  File Name: 	mp3_huffman.c               					                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:   huffman table                   						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/
# include "mp3_huffman.h"
# include "mp3_fixed.h"


#define MP3_DEC_QUAD_PTR(offs, bits)	{ { 0, bits, offs } }
#if defined(MP3_DEC_WORDS_BIGENDIAN)
#define MP3_DEC_QUAD_V(v, w, x, y, hlen)	{ { 1, hlen, (v << 3) | (w << 2) |  \
                                             (x <<  1) | (y <<  0) } }
#else
#define MP3_DEC_QUAD_V(v, w, x, y, hlen)	{ { 1, hlen, (v <<  0) | (w <<  1) |  \
                                             (x <<  2) | (y <<  3) } }
#endif



union MP3_DEC_HUFFQUAD_U const MP3_DEC_huff_tabA[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 2),
  /* 0001 */ MP3_DEC_QUAD_PTR(20, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(24, 1),
  /* 0011 */ MP3_DEC_QUAD_PTR(26, 1),
  /* 0100 */ MP3_DEC_QUAD_V(0, 0, 1, 0, 4),
  /* 0101 */ MP3_DEC_QUAD_V(0, 0, 0, 1, 4),
  /* 0110 */ MP3_DEC_QUAD_V(0, 1, 0, 0, 4),
  /* 0111 */ MP3_DEC_QUAD_V(1, 0, 0, 0, 4),
  /* 1000 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1001 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1010 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1011 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1100 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1101 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1110 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),
  /* 1111 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 1),

  /* 0000 ... */
  /* 00   */ MP3_DEC_QUAD_V(1, 0, 1, 1, 2),	/* 16 */
  /* 01   */ MP3_DEC_QUAD_V(1, 1, 1, 1, 2),
  /* 10   */ MP3_DEC_QUAD_V(1, 1, 0, 1, 2),
  /* 11   */ MP3_DEC_QUAD_V(1, 1, 1, 0, 2),

  /* 0001 ... */
  /* 00   */ MP3_DEC_QUAD_V(0, 1, 1, 1, 2),	/* 20 */
  /* 01   */ MP3_DEC_QUAD_V(0, 1, 0, 1, 2),
  /* 10   */ MP3_DEC_QUAD_V(1, 0, 0, 1, 1),
  /* 11   */ MP3_DEC_QUAD_V(1, 0, 0, 1, 1),

  /* 0010 ... */
  /* 0    */ MP3_DEC_QUAD_V(0, 1, 1, 0, 1),	/* 24 */
  /* 1    */ MP3_DEC_QUAD_V(0, 0, 1, 1, 1),

  /* 0011 ... */
  /* 0    */ MP3_DEC_QUAD_V(1, 0, 1, 0, 1),	/* 26 */
  /* 1    */ MP3_DEC_QUAD_V(1, 1, 0, 0, 1)
};


union MP3_DEC_HUFFQUAD_U const MP3_DEC_huff_tabB[] = {
  /* 0000 */ MP3_DEC_QUAD_V(1, 1, 1, 1, 4),
  /* 0001 */ MP3_DEC_QUAD_V(1, 1, 1, 0, 4),
  /* 0010 */ MP3_DEC_QUAD_V(1, 1, 0, 1, 4),
  /* 0011 */ MP3_DEC_QUAD_V(1, 1, 0, 0, 4),
  /* 0100 */ MP3_DEC_QUAD_V(1, 0, 1, 1, 4),
  /* 0101 */ MP3_DEC_QUAD_V(1, 0, 1, 0, 4),
  /* 0110 */ MP3_DEC_QUAD_V(1, 0, 0, 1, 4),
  /* 0111 */ MP3_DEC_QUAD_V(1, 0, 0, 0, 4),
  /* 1000 */ MP3_DEC_QUAD_V(0, 1, 1, 1, 4),
  /* 1001 */ MP3_DEC_QUAD_V(0, 1, 1, 0, 4),
  /* 1010 */ MP3_DEC_QUAD_V(0, 1, 0, 1, 4),
  /* 1011 */ MP3_DEC_QUAD_V(0, 1, 0, 0, 4),
  /* 1100 */ MP3_DEC_QUAD_V(0, 0, 1, 1, 4),
  /* 1101 */ MP3_DEC_QUAD_V(0, 0, 1, 0, 4),
  /* 1110 */ MP3_DEC_QUAD_V(0, 0, 0, 1, 4),
  /* 1111 */ MP3_DEC_QUAD_V(0, 0, 0, 0, 4)
};




#define MP3_DEC_PAIR_PTR(offs, bits)	{ { 0, bits, offs } }

#if defined(MP3_DEC_WORDS_BIGENDIAN)
#define MP3_DEC_PAIR_V(x, y, hlen)	{ { 1, hlen, (x << 6) | (y << 0) } }
#else
#define MP3_DEC_PAIR_V(x, y, hlen)	{ { 1, hlen, (x << 0) | (y << 4) } }
#endif


static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab0[16] = {
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),
        MP3_DEC_PAIR_V(0, 0, 0),


};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab1[] =
{
    /*0000*/    MP3_DEC_PAIR_V(1, 1, 3), 
    /*0001*/    MP3_DEC_PAIR_V(1, 1, 3), 
    /*0010*/    MP3_DEC_PAIR_V(0, 1, 3), 
    /*0011*/    MP3_DEC_PAIR_V(0, 1, 3), 
    /*0100*/    MP3_DEC_PAIR_V(1, 0, 2), 
    /*0101*/    MP3_DEC_PAIR_V(1, 0, 2), 
    /*0110*/    MP3_DEC_PAIR_V(1, 0, 2), 
    /*0111*/    MP3_DEC_PAIR_V(1, 0, 2), 
    /*1000*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1001*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1010*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1011*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1100*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1101*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1110*/    MP3_DEC_PAIR_V(0, 0, 1), 
    /*1111*/    MP3_DEC_PAIR_V(0, 0, 1), 

};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab2[] = {
    /*0000*/      MP3_DEC_QUAD_PTR(16, 2), 
    /*0001*/      MP3_DEC_QUAD_PTR(20, 1), 
    /*0010*/      MP3_DEC_PAIR_V(1, 1, 3), 
    /*0011*/      MP3_DEC_PAIR_V(1, 1, 3), 
    /*0100*/      MP3_DEC_PAIR_V(0, 1, 3), 
    /*0101*/      MP3_DEC_PAIR_V(0, 1, 3), 
    /*0110*/      MP3_DEC_PAIR_V(1, 0, 3), 
    /*0111*/      MP3_DEC_PAIR_V(1, 0, 3), 
    /*1000*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1001*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1010*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1011*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1100*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1101*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1110*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*1111*/      MP3_DEC_PAIR_V(0, 0, 1), 
    /*0000 00*/   MP3_DEC_PAIR_V(2, 2, 2), 
    /*0000 01*/   MP3_DEC_PAIR_V(0, 2, 2), 
    /*0000 10*/   MP3_DEC_PAIR_V(1, 2, 1), 
    /*0000 11*/   MP3_DEC_PAIR_V(1, 2, 1), 
    /*0001 0*/    MP3_DEC_PAIR_V(2, 1, 1), 
    /*0001 1*/    MP3_DEC_PAIR_V(2, 0, 1), 

};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab3[] = {
    /*0000*/      MP3_DEC_QUAD_PTR(16, 2),
    /*0001*/      MP3_DEC_QUAD_PTR(20, 1),
    /*0010*/      MP3_DEC_PAIR_V(1, 0, 3),
    /*0011*/      MP3_DEC_PAIR_V(1, 0, 3),
    /*0100*/      MP3_DEC_PAIR_V(1, 1, 2),
    /*0101*/      MP3_DEC_PAIR_V(1, 1, 2),
    /*0110*/      MP3_DEC_PAIR_V(1, 1, 2),
    /*0111*/      MP3_DEC_PAIR_V(1, 1, 2),
    /*1000*/      MP3_DEC_PAIR_V(0, 1, 2),
    /*1001*/      MP3_DEC_PAIR_V(0, 1, 2),
    /*1010*/      MP3_DEC_PAIR_V(0, 1, 2),
    /*1011*/      MP3_DEC_PAIR_V(0, 1, 2),
    /*1100*/      MP3_DEC_PAIR_V(0, 0, 2),
    /*1101*/      MP3_DEC_PAIR_V(0, 0, 2),
    /*1110*/      MP3_DEC_PAIR_V(0, 0, 2),
    /*1111*/      MP3_DEC_PAIR_V(0, 0, 2),
    /*0000 00*/   MP3_DEC_PAIR_V(2, 2, 2),
    /*0000 01*/   MP3_DEC_PAIR_V(0, 2, 2),
    /*0000 10*/   MP3_DEC_PAIR_V(1, 2, 1),
    /*0000 11*/   MP3_DEC_PAIR_V(1, 2, 1),
    /*0001 0*/    MP3_DEC_PAIR_V(2, 1, 1),
    /*0001 1*/    MP3_DEC_PAIR_V(2, 0, 1),

};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab5[] = {
    /*0000*/      MP3_DEC_QUAD_PTR(16, 4),  
    /*0001*/      MP3_DEC_QUAD_PTR(32, 2),  
    /*0010*/      MP3_DEC_PAIR_V(1, 1, 3),  
    /*0011*/      MP3_DEC_PAIR_V(1, 1, 3),  
    /*0100*/      MP3_DEC_PAIR_V(0, 1, 3),  
    /*0101*/      MP3_DEC_PAIR_V(0, 1, 3),  
    /*0110*/      MP3_DEC_PAIR_V(1, 0, 3),  
    /*0111*/      MP3_DEC_PAIR_V(1, 0, 3),  
    /*1000*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1001*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1010*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1011*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1100*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1101*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1110*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*1111*/      MP3_DEC_PAIR_V(0, 0, 1),  
    /*0000 0000*/ MP3_DEC_PAIR_V(3, 3, 4),  
    /*0000 0001*/ MP3_DEC_PAIR_V(2, 3, 4),  
    /*0000 0010*/ MP3_DEC_PAIR_V(3, 2, 3),  
    /*0000 0011*/ MP3_DEC_PAIR_V(3, 2, 3),  
    /*0000 0100*/ MP3_DEC_PAIR_V(3, 1, 2),  
    /*0000 0101*/ MP3_DEC_PAIR_V(3, 1, 2),  
    /*0000 0110*/ MP3_DEC_PAIR_V(3, 1, 2),  
    /*0000 0111*/ MP3_DEC_PAIR_V(3, 1, 2),  
    /*0000 1000*/ MP3_DEC_PAIR_V(1, 3, 3),  
    /*0000 1001*/ MP3_DEC_PAIR_V(1, 3, 3),  
    /*0000 1010*/ MP3_DEC_PAIR_V(0, 3, 3),  
    /*0000 1011*/ MP3_DEC_PAIR_V(0, 3, 3),  
    /*0000 1100*/ MP3_DEC_PAIR_V(3, 0, 3),  
    /*0000 1101*/ MP3_DEC_PAIR_V(3, 0, 3),  
    /*0000 1110*/ MP3_DEC_PAIR_V(2, 2, 3),  
    /*0000 1111*/ MP3_DEC_PAIR_V(2, 2, 3),  
    /*0001 00*/ MP3_DEC_PAIR_V(1, 2, 2)  ,  
    /*0001 01*/ MP3_DEC_PAIR_V(2, 1, 2)  ,  
    /*0001 10*/ MP3_DEC_PAIR_V(0, 2, 2)  ,  
    /*0001 11*/ MP3_DEC_PAIR_V(2, 0, 2)  ,  

};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab6[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 3),
  /* 0001 */ MP3_DEC_QUAD_PTR(24, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(26, 1),
  /* 0011 */ MP3_DEC_PAIR_V(1, 2, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 1, 4),
  /* 0101 */ MP3_DEC_PAIR_V(2, 0, 4),
  /* 0110 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 0111 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1000 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 1001 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 1010 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 1011 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 1100 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1101 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 3),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 3),

  /* 0000 ... */
  /* 000  */ MP3_DEC_PAIR_V(3, 3, 3),	/* 16 */
  /* 001  */ MP3_DEC_PAIR_V(0, 3, 3),
  /* 010  */ MP3_DEC_PAIR_V(2, 3, 2),
  /* 011  */ MP3_DEC_PAIR_V(2, 3, 2),
  /* 100  */ MP3_DEC_PAIR_V(3, 2, 2),
  /* 101  */ MP3_DEC_PAIR_V(3, 2, 2),
  /* 110  */ MP3_DEC_PAIR_V(3, 0, 2),
  /* 111  */ MP3_DEC_PAIR_V(3, 0, 2),

  /* 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 3, 1),	/* 24 */
  /* 1    */ MP3_DEC_PAIR_V(3, 1, 1),

  /* 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 2, 1),	/* 26 */
  /* 1    */ MP3_DEC_PAIR_V(0, 2, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab7[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 2),
  /* 0011 */ MP3_DEC_PAIR_V(1, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 0101 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 0110 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1001 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1010 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1011 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 1),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(52, 2),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(56, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(58, 1),
  /* 0011 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 0100 */ MP3_DEC_PAIR_V(5, 1, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(60, 1),
  /* 0110 */ MP3_DEC_PAIR_V(5, 0, 4),
  /* 0111 */ MP3_DEC_QUAD_PTR(62, 1),
  /* 1000 */ MP3_DEC_PAIR_V(2, 4, 4),
  /* 1001 */ MP3_DEC_PAIR_V(4, 2, 4),
  /* 1010 */ MP3_DEC_PAIR_V(1, 4, 3),
  /* 1011 */ MP3_DEC_PAIR_V(1, 4, 3),
  /* 1100 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 1101 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 1110 */ MP3_DEC_PAIR_V(4, 0, 3),
  /* 1111 */ MP3_DEC_PAIR_V(4, 0, 3),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(0, 4, 4),	/* 32 */
  /* 0001 */ MP3_DEC_PAIR_V(2, 3, 4),
  /* 0010 */ MP3_DEC_PAIR_V(3, 2, 4),
  /* 0011 */ MP3_DEC_PAIR_V(0, 3, 4),
  /* 0100 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 0101 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 0110 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 0111 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1000 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1001 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1010 */ MP3_DEC_PAIR_V(2, 2, 3),
  /* 1011 */ MP3_DEC_PAIR_V(2, 2, 3),
  /* 1100 */ MP3_DEC_PAIR_V(1, 2, 2),
  /* 1101 */ MP3_DEC_PAIR_V(1, 2, 2),
  /* 1110 */ MP3_DEC_PAIR_V(1, 2, 2),
  /* 1111 */ MP3_DEC_PAIR_V(1, 2, 2),

  /* 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(2, 1, 1),	/* 48 */
  /* 01   */ MP3_DEC_PAIR_V(2, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0000 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 5, 2),	/* 52 */
  /* 01   */ MP3_DEC_PAIR_V(4, 5, 2),
  /* 10   */ MP3_DEC_PAIR_V(5, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(5, 3, 2),

  /* 0000 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 5, 1),	/* 56 */
  /* 1    */ MP3_DEC_PAIR_V(4, 4, 1),

  /* 0000 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 5, 1),	/* 58 */
  /* 1    */ MP3_DEC_PAIR_V(5, 2, 1),

  /* 0000 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 5, 1),	/* 60 */
  /* 1    */ MP3_DEC_PAIR_V(3, 4, 1),

  /* 0000 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 3, 1),	/* 62 */
  /* 1    */ MP3_DEC_PAIR_V(3, 3, 1)
};


static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab8[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_PAIR_V(1, 2, 4),
  /* 0011 */ MP3_DEC_PAIR_V(2, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 0101 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 0110 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 0111 */ MP3_DEC_PAIR_V(1, 1, 2),
  /* 1000 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1011 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 2),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(48, 3),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(56, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(60, 1),
  /* 0011 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 0100 */ MP3_DEC_PAIR_V(5, 1, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(62, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(64, 1),
  /* 0111 */ MP3_DEC_PAIR_V(2, 4, 4),
  /* 1000 */ MP3_DEC_PAIR_V(4, 2, 4),
  /* 1001 */ MP3_DEC_PAIR_V(1, 4, 4),
  /* 1010 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 4, 4),
  /* 1101 */ MP3_DEC_PAIR_V(4, 0, 4),
  /* 1110 */ MP3_DEC_PAIR_V(2, 3, 4),
  /* 1111 */ MP3_DEC_PAIR_V(3, 2, 4),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(1, 3, 4),	/* 32 */
  /* 0001 */ MP3_DEC_PAIR_V(3, 1, 4),
  /* 0010 */ MP3_DEC_PAIR_V(0, 3, 4),
  /* 0011 */ MP3_DEC_PAIR_V(3, 0, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 0101 */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 0110 */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 0111 */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 1000 */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 1001 */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 1010 */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 1011 */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 1100 */ MP3_DEC_PAIR_V(2, 0, 2),
  /* 1101 */ MP3_DEC_PAIR_V(2, 0, 2),
  /* 1110 */ MP3_DEC_PAIR_V(2, 0, 2),
  /* 1111 */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0000 0000 ... */
  /* 000  */ MP3_DEC_PAIR_V(5, 5, 3),	/* 48 */
  /* 001  */ MP3_DEC_PAIR_V(5, 4, 3),
  /* 010  */ MP3_DEC_PAIR_V(4, 5, 2),
  /* 011  */ MP3_DEC_PAIR_V(4, 5, 2),
  /* 100  */ MP3_DEC_PAIR_V(5, 3, 1),
  /* 101  */ MP3_DEC_PAIR_V(5, 3, 1),
  /* 110  */ MP3_DEC_PAIR_V(5, 3, 1),
  /* 111  */ MP3_DEC_PAIR_V(5, 3, 1),

  /* 0000 0001 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 5, 2),	/* 56 */
  /* 01   */ MP3_DEC_PAIR_V(4, 4, 2),
  /* 10   */ MP3_DEC_PAIR_V(2, 5, 1),
  /* 11   */ MP3_DEC_PAIR_V(2, 5, 1),

  /* 0000 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 2, 1),	/* 60 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1),

  /* 0000 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 4, 1),	/* 62 */
  /* 1    */ MP3_DEC_PAIR_V(4, 3, 1),

  /* 0000 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 0, 1),	/* 64 */
  /* 1    */ MP3_DEC_PAIR_V(3, 3, 1)
};


static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab9[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(40, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(44, 2),
  /* 0100 */ MP3_DEC_QUAD_PTR(48, 1),
  /* 0101 */ MP3_DEC_PAIR_V(1, 2, 4),
  /* 0110 */ MP3_DEC_PAIR_V(2, 1, 4),
  /* 0111 */ MP3_DEC_PAIR_V(2, 0, 4),
  /* 1000 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1101 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 3),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 3),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(50, 1),	/* 16 */
  /* 0001 */ MP3_DEC_PAIR_V(3, 5, 4),
  /* 0010 */ MP3_DEC_PAIR_V(5, 3, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(52, 1),
  /* 0100 */ MP3_DEC_PAIR_V(4, 4, 4),
  /* 0101 */ MP3_DEC_PAIR_V(2, 5, 4),
  /* 0110 */ MP3_DEC_PAIR_V(5, 2, 4),
  /* 0111 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 1000 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(3, 4, 3),
  /* 1011 */ MP3_DEC_PAIR_V(3, 4, 3),
  /* 1100 */ MP3_DEC_PAIR_V(4, 3, 3),
  /* 1101 */ MP3_DEC_PAIR_V(4, 3, 3),
  /* 1110 */ MP3_DEC_PAIR_V(5, 0, 4),
  /* 1111 */ MP3_DEC_PAIR_V(0, 4, 4),

  /* 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(2, 4, 3),	/* 32 */
  /* 001  */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 010  */ MP3_DEC_PAIR_V(3, 3, 3),
  /* 011  */ MP3_DEC_PAIR_V(4, 0, 3),
  /* 100  */ MP3_DEC_PAIR_V(1, 4, 2),
  /* 101  */ MP3_DEC_PAIR_V(1, 4, 2),
  /* 110  */ MP3_DEC_PAIR_V(4, 1, 2),
  /* 111  */ MP3_DEC_PAIR_V(4, 1, 2),

  /* 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(2, 3, 2),	/* 40 */
  /* 01   */ MP3_DEC_PAIR_V(3, 2, 2),
  /* 10   */ MP3_DEC_PAIR_V(1, 3, 1),
  /* 11   */ MP3_DEC_PAIR_V(1, 3, 1),

  /* 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 1, 1),	/* 44 */
  /* 01   */ MP3_DEC_PAIR_V(3, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 0, 2),

  /* 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 2, 1),	/* 48 */
  /* 1    */ MP3_DEC_PAIR_V(0, 2, 1),

  /* 0000 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 5, 1),	/* 50 */
  /* 1    */ MP3_DEC_PAIR_V(4, 5, 1),

  /* 0000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 4, 1),	/* 52 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab10[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 2),
  /* 0011 */ MP3_DEC_PAIR_V(1, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 0101 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 0110 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1001 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1010 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1011 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 1),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(52, 3),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(60, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(64, 3),
  /* 0011 */ MP3_DEC_QUAD_PTR(72, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(74, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(78, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(82, 2),
  /* 0111 */ MP3_DEC_PAIR_V(1, 7, 4),
  /* 1000 */ MP3_DEC_PAIR_V(7, 1, 4),
  /* 1001 */ MP3_DEC_QUAD_PTR(86, 1),
  /* 1010 */ MP3_DEC_QUAD_PTR(88, 2),
  /* 1011 */ MP3_DEC_QUAD_PTR(92, 2),
  /* 1100 */ MP3_DEC_PAIR_V(1, 6, 4),
  /* 1101 */ MP3_DEC_PAIR_V(6, 1, 4),
  /* 1110 */ MP3_DEC_PAIR_V(6, 0, 4),
  /* 1111 */ MP3_DEC_QUAD_PTR(96, 1),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(98, 1),	/* 32 */
  /* 0001 */ MP3_DEC_QUAD_PTR(100, 1),
  /* 0010 */ MP3_DEC_PAIR_V(1, 4, 4),
  /* 0011 */ MP3_DEC_PAIR_V(4, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(4, 0, 4),
  /* 0101 */ MP3_DEC_PAIR_V(2, 3, 4),
  /* 0110 */ MP3_DEC_PAIR_V(3, 2, 4),
  /* 0111 */ MP3_DEC_PAIR_V(0, 3, 4),
  /* 1000 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 1001 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 1010 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1101 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1110 */ MP3_DEC_PAIR_V(2, 2, 3),
  /* 1111 */ MP3_DEC_PAIR_V(2, 2, 3),

  /* 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(1, 2, 2),	/* 48 */
  /* 01   */ MP3_DEC_PAIR_V(2, 1, 2),
  /* 10   */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0000 0000 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 7, 3),	/* 52 */
  /* 001  */ MP3_DEC_PAIR_V(6, 7, 3),
  /* 010  */ MP3_DEC_PAIR_V(7, 6, 3),
  /* 011  */ MP3_DEC_PAIR_V(5, 7, 3),
  /* 100  */ MP3_DEC_PAIR_V(7, 5, 3),
  /* 101  */ MP3_DEC_PAIR_V(6, 6, 3),
  /* 110  */ MP3_DEC_PAIR_V(4, 7, 2),
  /* 111  */ MP3_DEC_PAIR_V(4, 7, 2),

  /* 0000 0001 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 4, 2),	/* 60 */
  /* 01   */ MP3_DEC_PAIR_V(5, 6, 2),
  /* 10   */ MP3_DEC_PAIR_V(6, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 7, 2),

  /* 0000 0010 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 3, 2),	/* 64 */
  /* 001  */ MP3_DEC_PAIR_V(7, 3, 2),
  /* 010  */ MP3_DEC_PAIR_V(4, 6, 2),
  /* 011  */ MP3_DEC_PAIR_V(4, 6, 2),
  /* 100  */ MP3_DEC_PAIR_V(5, 5, 3),
  /* 101  */ MP3_DEC_PAIR_V(5, 4, 3),
  /* 110  */ MP3_DEC_PAIR_V(6, 3, 2),
  /* 111  */ MP3_DEC_PAIR_V(6, 3, 2),

  /* 0000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 7, 1),	/* 72 */
  /* 1    */ MP3_DEC_PAIR_V(7, 2, 1),

  /* 0000 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 4, 2),	/* 74 */
  /* 01   */ MP3_DEC_PAIR_V(0, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(7, 0, 1),
  /* 11   */ MP3_DEC_PAIR_V(7, 0, 1),

  /* 0000 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 2, 1),	/* 78 */
  /* 01   */ MP3_DEC_PAIR_V(6, 2, 1),
  /* 10   */ MP3_DEC_PAIR_V(4, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 5, 2),

  /* 0000 0110 ... */
  /* 00   */ MP3_DEC_PAIR_V(0, 6, 1),	/* 82 */
  /* 01   */ MP3_DEC_PAIR_V(0, 6, 1),
  /* 10   */ MP3_DEC_PAIR_V(5, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(4, 4, 2),

  /* 0000 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 6, 1),	/* 86 */
  /* 1    */ MP3_DEC_PAIR_V(2, 6, 1),

  /* 0000 1010 ... */
  /* 00   */ MP3_DEC_PAIR_V(2, 5, 2),	/* 88 */
  /* 01   */ MP3_DEC_PAIR_V(5, 2, 2),
  /* 10   */ MP3_DEC_PAIR_V(1, 5, 1),
  /* 11   */ MP3_DEC_PAIR_V(1, 5, 1),

  /* 0000 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 1, 1),	/* 92 */
  /* 01   */ MP3_DEC_PAIR_V(5, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(3, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(4, 3, 2),

  /* 0000 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 5, 1),	/* 96 */
  /* 1    */ MP3_DEC_PAIR_V(5, 0, 1),

  /* 0001 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 4, 1),	/* 98 */
  /* 1    */ MP3_DEC_PAIR_V(4, 2, 1),

  /* 0001 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 3, 1),	/* 100 */
  /* 1    */ MP3_DEC_PAIR_V(0, 4, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab11[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(64, 3),
  /* 0100 */ MP3_DEC_PAIR_V(1, 2, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(72, 1),
  /* 0110 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1011 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 2),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 2),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(74, 2),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(78, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(86, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(90, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(92, 2),
  /* 0101 */ MP3_DEC_PAIR_V(2, 7, 4),
  /* 0110 */ MP3_DEC_PAIR_V(7, 2, 4),
  /* 0111 */ MP3_DEC_QUAD_PTR(96, 1),
  /* 1000 */ MP3_DEC_PAIR_V(7, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(7, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(1, 7, 4),
  /* 1011 */ MP3_DEC_PAIR_V(7, 0, 4),
  /* 1100 */ MP3_DEC_PAIR_V(3, 6, 4),
  /* 1101 */ MP3_DEC_PAIR_V(6, 3, 4),
  /* 1110 */ MP3_DEC_PAIR_V(6, 0, 4),
  /* 1111 */ MP3_DEC_QUAD_PTR(98, 1),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(100, 1),	/* 32 */
  /* 0001 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 0010 */ MP3_DEC_PAIR_V(6, 2, 3),
  /* 0011 */ MP3_DEC_PAIR_V(6, 2, 3),
  /* 0100 */ MP3_DEC_PAIR_V(2, 6, 4),
  /* 0101 */ MP3_DEC_PAIR_V(0, 6, 4),
  /* 0110 */ MP3_DEC_PAIR_V(1, 6, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 6, 3),
  /* 1000 */ MP3_DEC_PAIR_V(6, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(6, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(5, 1, 4),
  /* 1011 */ MP3_DEC_PAIR_V(3, 4, 4),
  /* 1100 */ MP3_DEC_PAIR_V(5, 0, 4),
  /* 1101 */ MP3_DEC_QUAD_PTR(102, 1),
  /* 1110 */ MP3_DEC_PAIR_V(2, 4, 4),
  /* 1111 */ MP3_DEC_PAIR_V(4, 2, 4),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(1, 4, 4),	/* 48 */
  /* 0001 */ MP3_DEC_PAIR_V(4, 1, 4),
  /* 0010 */ MP3_DEC_PAIR_V(0, 4, 4),
  /* 0011 */ MP3_DEC_PAIR_V(4, 0, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 3, 3),
  /* 0101 */ MP3_DEC_PAIR_V(2, 3, 3),
  /* 0110 */ MP3_DEC_PAIR_V(3, 2, 3),
  /* 0111 */ MP3_DEC_PAIR_V(3, 2, 3),
  /* 1000 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1001 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1010 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1011 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1100 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1101 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1110 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1111 */ MP3_DEC_PAIR_V(3, 1, 2),

  /* 0011 ... */
  /* 000  */ MP3_DEC_PAIR_V(0, 3, 3),	/* 64 */
  /* 001  */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 010  */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(2, 1, 1),
  /* 101  */ MP3_DEC_PAIR_V(2, 1, 1),
  /* 110  */ MP3_DEC_PAIR_V(2, 1, 1),
  /* 111  */ MP3_DEC_PAIR_V(2, 1, 1),

  /* 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 2, 1),	/* 72 */
  /* 1    */ MP3_DEC_PAIR_V(2, 0, 1),

  /* 0000 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 7, 2),	/* 74 */
  /* 01   */ MP3_DEC_PAIR_V(6, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(7, 6, 2),
  /* 11   */ MP3_DEC_PAIR_V(7, 5, 2),

  /* 0000 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(6, 6, 2),	/* 78 */
  /* 001  */ MP3_DEC_PAIR_V(6, 6, 2),
  /* 010  */ MP3_DEC_PAIR_V(4, 7, 2),
  /* 011  */ MP3_DEC_PAIR_V(4, 7, 2),
  /* 100  */ MP3_DEC_PAIR_V(7, 4, 2),
  /* 101  */ MP3_DEC_PAIR_V(7, 4, 2),
  /* 110  */ MP3_DEC_PAIR_V(5, 7, 3),
  /* 111  */ MP3_DEC_PAIR_V(5, 5, 3),

  /* 0000 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 6, 2),	/* 86 */
  /* 01   */ MP3_DEC_PAIR_V(6, 5, 2),
  /* 10   */ MP3_DEC_PAIR_V(3, 7, 1),
  /* 11   */ MP3_DEC_PAIR_V(3, 7, 1),

  /* 0000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 3, 1),	/* 90 */
  /* 1    */ MP3_DEC_PAIR_V(4, 6, 1),

  /* 0000 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(4, 5, 2),	/* 92 */
  /* 01   */ MP3_DEC_PAIR_V(5, 4, 2),
  /* 10   */ MP3_DEC_PAIR_V(3, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(5, 3, 2),

  /* 0000 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 4, 1),	/* 96 */
  /* 1    */ MP3_DEC_PAIR_V(0, 7, 1),

  /* 0000 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 4, 1),	/* 98 */
  /* 1    */ MP3_DEC_PAIR_V(2, 5, 1),

  /* 0001 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 2, 1),	/* 100 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1),

  /* 0001 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 3, 1),	/* 102 */
  /* 1    */ MP3_DEC_PAIR_V(3, 3, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab12[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(64, 2),
  /* 0100 */ MP3_DEC_QUAD_PTR(68, 3),
  /* 0101 */ MP3_DEC_QUAD_PTR(76, 1),
  /* 0110 */ MP3_DEC_PAIR_V(1, 2, 4),
  /* 0111 */ MP3_DEC_PAIR_V(2, 1, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(78, 1),
  /* 1001 */ MP3_DEC_PAIR_V(0, 0, 4),
  /* 1010 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1101 */ MP3_DEC_PAIR_V(0, 1, 3),
  /* 1110 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1111 */ MP3_DEC_PAIR_V(1, 0, 3),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(80, 2),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(84, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(86, 1),
  /* 0011 */ MP3_DEC_QUAD_PTR(88, 1),
  /* 0100 */ MP3_DEC_PAIR_V(5, 6, 4),
  /* 0101 */ MP3_DEC_PAIR_V(3, 7, 4),
  /* 0110 */ MP3_DEC_QUAD_PTR(90, 1),
  /* 0111 */ MP3_DEC_PAIR_V(2, 7, 4),
  /* 1000 */ MP3_DEC_PAIR_V(7, 2, 4),
  /* 1001 */ MP3_DEC_PAIR_V(4, 6, 4),
  /* 1010 */ MP3_DEC_PAIR_V(6, 4, 4),
  /* 1011 */ MP3_DEC_PAIR_V(1, 7, 4),
  /* 1100 */ MP3_DEC_PAIR_V(7, 1, 4),
  /* 1101 */ MP3_DEC_QUAD_PTR(92, 1),
  /* 1110 */ MP3_DEC_PAIR_V(3, 6, 4),
  /* 1111 */ MP3_DEC_PAIR_V(6, 3, 4),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(4, 5, 4),	/* 32 */
  /* 0001 */ MP3_DEC_PAIR_V(5, 4, 4),
  /* 0010 */ MP3_DEC_PAIR_V(4, 4, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(94, 1),
  /* 0100 */ MP3_DEC_PAIR_V(2, 6, 3),
  /* 0101 */ MP3_DEC_PAIR_V(2, 6, 3),
  /* 0110 */ MP3_DEC_PAIR_V(6, 2, 3),
  /* 0111 */ MP3_DEC_PAIR_V(6, 2, 3),
  /* 1000 */ MP3_DEC_PAIR_V(6, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(6, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(1, 6, 4),
  /* 1011 */ MP3_DEC_PAIR_V(6, 0, 4),
  /* 1100 */ MP3_DEC_PAIR_V(3, 5, 4),
  /* 1101 */ MP3_DEC_PAIR_V(5, 3, 4),
  /* 1110 */ MP3_DEC_PAIR_V(2, 5, 4),
  /* 1111 */ MP3_DEC_PAIR_V(5, 2, 4),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(1, 5, 3),	/* 48 */
  /* 0001 */ MP3_DEC_PAIR_V(1, 5, 3),
  /* 0010 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 0011 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 0100 */ MP3_DEC_PAIR_V(3, 4, 3),
  /* 0101 */ MP3_DEC_PAIR_V(3, 4, 3),
  /* 0110 */ MP3_DEC_PAIR_V(4, 3, 3),
  /* 0111 */ MP3_DEC_PAIR_V(4, 3, 3),
  /* 1000 */ MP3_DEC_PAIR_V(5, 0, 4),
  /* 1001 */ MP3_DEC_PAIR_V(0, 4, 4),
  /* 1010 */ MP3_DEC_PAIR_V(2, 4, 3),
  /* 1011 */ MP3_DEC_PAIR_V(2, 4, 3),
  /* 1100 */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 1101 */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 1110 */ MP3_DEC_PAIR_V(1, 4, 3),
  /* 1111 */ MP3_DEC_PAIR_V(1, 4, 3),

  /* 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 3, 2),	/* 64 */
  /* 01   */ MP3_DEC_PAIR_V(4, 1, 2),
  /* 10   */ MP3_DEC_PAIR_V(2, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 2, 2),

  /* 0100 ... */
  /* 000  */ MP3_DEC_PAIR_V(4, 0, 3),	/* 68 */
  /* 001  */ MP3_DEC_PAIR_V(0, 3, 3),
  /* 010  */ MP3_DEC_PAIR_V(3, 0, 2),
  /* 011  */ MP3_DEC_PAIR_V(3, 0, 2),
  /* 100  */ MP3_DEC_PAIR_V(1, 3, 1),
  /* 101  */ MP3_DEC_PAIR_V(1, 3, 1),
  /* 110  */ MP3_DEC_PAIR_V(1, 3, 1),
  /* 111  */ MP3_DEC_PAIR_V(1, 3, 1),

  /* 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 1, 1),	/* 76 */
  /* 1    */ MP3_DEC_PAIR_V(2, 2, 1),

  /* 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 2, 1),	/* 78 */
  /* 1    */ MP3_DEC_PAIR_V(2, 0, 1),

  /* 0000 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 7, 2),	/* 80 */
  /* 01   */ MP3_DEC_PAIR_V(6, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(7, 6, 1),
  /* 11   */ MP3_DEC_PAIR_V(7, 6, 1),

  /* 0000 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 7, 1),	/* 84 */
  /* 1    */ MP3_DEC_PAIR_V(7, 5, 1),

  /* 0000 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 6, 1),	/* 86 */
  /* 1    */ MP3_DEC_PAIR_V(4, 7, 1),

  /* 0000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 4, 1),	/* 88 */
  /* 1    */ MP3_DEC_PAIR_V(6, 5, 1),

  /* 0000 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 3, 1),	/* 90 */
  /* 1    */ MP3_DEC_PAIR_V(5, 5, 1),

  /* 0000 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 7, 1),	/* 92 */
  /* 1    */ MP3_DEC_PAIR_V(7, 0, 1),

  /* 0001 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 6, 1),	/* 94 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab13[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(64, 2),
  /* 0100 */ MP3_DEC_PAIR_V(1, 1, 4),
  /* 0101 */ MP3_DEC_PAIR_V(0, 1, 4),
  /* 0110 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1001 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1010 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1011 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 1),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(68, 4),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(84, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(100, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(116, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(132, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(148, 4),
  /* 0110 */ MP3_DEC_QUAD_PTR(164, 3),
  /* 0111 */ MP3_DEC_QUAD_PTR(172, 3),
  /* 1000 */ MP3_DEC_QUAD_PTR(180, 3),
  /* 1001 */ MP3_DEC_QUAD_PTR(188, 3),
  /* 1010 */ MP3_DEC_QUAD_PTR(196, 3),
  /* 1011 */ MP3_DEC_QUAD_PTR(204, 3),
  /* 1100 */ MP3_DEC_QUAD_PTR(212, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(214, 2),
  /* 1110 */ MP3_DEC_QUAD_PTR(218, 3),
  /* 1111 */ MP3_DEC_QUAD_PTR(226, 1),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(228, 2),	/* 32 */
  /* 0001 */ MP3_DEC_QUAD_PTR(232, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(236, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(240, 2),
  /* 0100 */ MP3_DEC_PAIR_V(8, 1, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(244, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(246, 1),
  /* 0111 */ MP3_DEC_QUAD_PTR(248, 1),
  /* 1000 */ MP3_DEC_QUAD_PTR(250, 2),
  /* 1001 */ MP3_DEC_QUAD_PTR(254, 1),
  /* 1010 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 1011 */ MP3_DEC_PAIR_V(5, 1, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(256, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(258, 1),
  /* 1110 */ MP3_DEC_QUAD_PTR(260, 1),
  /* 1111 */ MP3_DEC_PAIR_V(1, 4, 4),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(4, 1, 3),	/* 48 */
  /* 0001 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 0010 */ MP3_DEC_PAIR_V(0, 4, 4),
  /* 0011 */ MP3_DEC_PAIR_V(4, 0, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 3, 4),
  /* 0101 */ MP3_DEC_PAIR_V(3, 2, 4),
  /* 0110 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 1000 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(0, 3, 3),
  /* 1011 */ MP3_DEC_PAIR_V(0, 3, 3),
  /* 1100 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1101 */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 1110 */ MP3_DEC_PAIR_V(2, 2, 3),
  /* 1111 */ MP3_DEC_PAIR_V(2, 2, 3),

  /* 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(1, 2, 2),	/* 64 */
  /* 01   */ MP3_DEC_PAIR_V(2, 1, 2),
  /* 10   */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0000 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(262, 4),	/* 68 */
  /* 0001 */ MP3_DEC_QUAD_PTR(278, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(294, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(310, 3),
  /* 0100 */ MP3_DEC_QUAD_PTR(318, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(322, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(326, 3),
  /* 0111 */ MP3_DEC_QUAD_PTR(334, 2),
  /* 1000 */ MP3_DEC_QUAD_PTR(338, 1),
  /* 1001 */ MP3_DEC_QUAD_PTR(340, 2),
  /* 1010 */ MP3_DEC_QUAD_PTR(344, 2),
  /* 1011 */ MP3_DEC_QUAD_PTR(348, 2),
  /* 1100 */ MP3_DEC_QUAD_PTR(352, 2),
  /* 1101 */ MP3_DEC_QUAD_PTR(356, 2),
  /* 1110 */ MP3_DEC_PAIR_V(1, 15, 4),
  /* 1111 */ MP3_DEC_PAIR_V(15, 1, 4),

  /* 0000 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(15, 0, 4),	/* 84 */
  /* 0001 */ MP3_DEC_QUAD_PTR(360, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(362, 1),
  /* 0011 */ MP3_DEC_QUAD_PTR(364, 1),
  /* 0100 */ MP3_DEC_PAIR_V(14, 2, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(366, 1),
  /* 0110 */ MP3_DEC_PAIR_V(1, 14, 4),
  /* 0111 */ MP3_DEC_PAIR_V(14, 1, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(368, 1),
  /* 1001 */ MP3_DEC_QUAD_PTR(370, 1),
  /* 1010 */ MP3_DEC_QUAD_PTR(372, 1),
  /* 1011 */ MP3_DEC_QUAD_PTR(374, 1),
  /* 1100 */ MP3_DEC_QUAD_PTR(376, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(378, 1),
  /* 1110 */ MP3_DEC_PAIR_V(12, 6, 4),
  /* 1111 */ MP3_DEC_PAIR_V(3, 13, 4),

  /* 0000 0010 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(380, 1),	/* 100 */
  /* 0001 */ MP3_DEC_PAIR_V(2, 13, 4),
  /* 0010 */ MP3_DEC_PAIR_V(13, 2, 4),
  /* 0011 */ MP3_DEC_PAIR_V(1, 13, 4),
  /* 0100 */ MP3_DEC_PAIR_V(11, 7, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(382, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(384, 1),
  /* 0111 */ MP3_DEC_PAIR_V(12, 3, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(386, 1),
  /* 1001 */ MP3_DEC_PAIR_V(4, 11, 4),
  /* 1010 */ MP3_DEC_PAIR_V(13, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(13, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 13, 4),
  /* 1101 */ MP3_DEC_PAIR_V(13, 0, 4),
  /* 1110 */ MP3_DEC_PAIR_V(8, 10, 4),
  /* 1111 */ MP3_DEC_PAIR_V(10, 8, 4),

  /* 0000 0011 ... */
  /* 0000 */ MP3_DEC_PAIR_V(4, 12, 4),	/* 116 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 4, 4),
  /* 0010 */ MP3_DEC_PAIR_V(6, 11, 4),
  /* 0011 */ MP3_DEC_PAIR_V(11, 6, 4),
  /* 0100 */ MP3_DEC_PAIR_V(3, 12, 3),
  /* 0101 */ MP3_DEC_PAIR_V(3, 12, 3),
  /* 0110 */ MP3_DEC_PAIR_V(2, 12, 3),
  /* 0111 */ MP3_DEC_PAIR_V(2, 12, 3),
  /* 1000 */ MP3_DEC_PAIR_V(12, 2, 3),
  /* 1001 */ MP3_DEC_PAIR_V(12, 2, 3),
  /* 1010 */ MP3_DEC_PAIR_V(5, 11, 3),
  /* 1011 */ MP3_DEC_PAIR_V(5, 11, 3),
  /* 1100 */ MP3_DEC_PAIR_V(11, 5, 4),
  /* 1101 */ MP3_DEC_PAIR_V(8, 9, 4),
  /* 1110 */ MP3_DEC_PAIR_V(1, 12, 3),
  /* 1111 */ MP3_DEC_PAIR_V(1, 12, 3),

  /* 0000 0100 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 1, 3),	/* 132 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 1, 3),
  /* 0010 */ MP3_DEC_PAIR_V(9, 8, 4),
  /* 0011 */ MP3_DEC_PAIR_V(0, 12, 4),
  /* 0100 */ MP3_DEC_PAIR_V(12, 0, 3),
  /* 0101 */ MP3_DEC_PAIR_V(12, 0, 3),
  /* 0110 */ MP3_DEC_PAIR_V(11, 4, 4),
  /* 0111 */ MP3_DEC_PAIR_V(6, 10, 4),
  /* 1000 */ MP3_DEC_PAIR_V(10, 6, 4),
  /* 1001 */ MP3_DEC_PAIR_V(7, 9, 4),
  /* 1010 */ MP3_DEC_PAIR_V(3, 11, 3),
  /* 1011 */ MP3_DEC_PAIR_V(3, 11, 3),
  /* 1100 */ MP3_DEC_PAIR_V(11, 3, 3),
  /* 1101 */ MP3_DEC_PAIR_V(11, 3, 3),
  /* 1110 */ MP3_DEC_PAIR_V(8, 8, 4),
  /* 1111 */ MP3_DEC_PAIR_V(5, 10, 4),

  /* 0000 0101 ... */
  /* 0000 */ MP3_DEC_PAIR_V(2, 11, 3),	/* 148 */
  /* 0001 */ MP3_DEC_PAIR_V(2, 11, 3),
  /* 0010 */ MP3_DEC_PAIR_V(10, 5, 4),
  /* 0011 */ MP3_DEC_PAIR_V(6, 9, 4),
  /* 0100 */ MP3_DEC_PAIR_V(10, 4, 3),
  /* 0101 */ MP3_DEC_PAIR_V(10, 4, 3),
  /* 0110 */ MP3_DEC_PAIR_V(7, 8, 4),
  /* 0111 */ MP3_DEC_PAIR_V(8, 7, 4),
  /* 1000 */ MP3_DEC_PAIR_V(9, 4, 3),
  /* 1001 */ MP3_DEC_PAIR_V(9, 4, 3),
  /* 1010 */ MP3_DEC_PAIR_V(7, 7, 4),
  /* 1011 */ MP3_DEC_PAIR_V(7, 6, 4),
  /* 1100 */ MP3_DEC_PAIR_V(11, 2, 2),
  /* 1101 */ MP3_DEC_PAIR_V(11, 2, 2),
  /* 1110 */ MP3_DEC_PAIR_V(11, 2, 2),
  /* 1111 */ MP3_DEC_PAIR_V(11, 2, 2),

  /* 0000 0110 ... */
  /* 000  */ MP3_DEC_PAIR_V(1, 11, 2),	/* 164 */
  /* 001  */ MP3_DEC_PAIR_V(1, 11, 2),
  /* 010  */ MP3_DEC_PAIR_V(11, 1, 2),
  /* 011  */ MP3_DEC_PAIR_V(11, 1, 2),
  /* 100  */ MP3_DEC_PAIR_V(0, 11, 3),
  /* 101  */ MP3_DEC_PAIR_V(11, 0, 3),
  /* 110  */ MP3_DEC_PAIR_V(9, 6, 3),
  /* 111  */ MP3_DEC_PAIR_V(4, 10, 3),

  /* 0000 0111 ... */
  /* 000  */ MP3_DEC_PAIR_V(3, 10, 3),	/* 172 */
  /* 001  */ MP3_DEC_PAIR_V(10, 3, 3),
  /* 010  */ MP3_DEC_PAIR_V(5, 9, 3),
  /* 011  */ MP3_DEC_PAIR_V(9, 5, 3),
  /* 100  */ MP3_DEC_PAIR_V(2, 10, 2),
  /* 101  */ MP3_DEC_PAIR_V(2, 10, 2),
  /* 110  */ MP3_DEC_PAIR_V(10, 2, 2),
  /* 111  */ MP3_DEC_PAIR_V(10, 2, 2),

  /* 0000 1000 ... */
  /* 000  */ MP3_DEC_PAIR_V(1, 10, 2),	/* 180 */
  /* 001  */ MP3_DEC_PAIR_V(1, 10, 2),
  /* 010  */ MP3_DEC_PAIR_V(10, 1, 2),
  /* 011  */ MP3_DEC_PAIR_V(10, 1, 2),
  /* 100  */ MP3_DEC_PAIR_V(0, 10, 3),
  /* 101  */ MP3_DEC_PAIR_V(6, 8, 3),
  /* 110  */ MP3_DEC_PAIR_V(10, 0, 2),
  /* 111  */ MP3_DEC_PAIR_V(10, 0, 2),

  /* 0000 1001 ... */
  /* 000  */ MP3_DEC_PAIR_V(8, 6, 3),	/* 188 */
  /* 001  */ MP3_DEC_PAIR_V(4, 9, 3),
  /* 010  */ MP3_DEC_PAIR_V(9, 3, 2),
  /* 011  */ MP3_DEC_PAIR_V(9, 3, 2),
  /* 100  */ MP3_DEC_PAIR_V(3, 9, 3),
  /* 101  */ MP3_DEC_PAIR_V(5, 8, 3),
  /* 110  */ MP3_DEC_PAIR_V(8, 5, 3),
  /* 111  */ MP3_DEC_PAIR_V(6, 7, 3),

  /* 0000 1010 ... */
  /* 000  */ MP3_DEC_PAIR_V(2, 9, 2),	/* 196 */
  /* 001  */ MP3_DEC_PAIR_V(2, 9, 2),
  /* 010  */ MP3_DEC_PAIR_V(9, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(9, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(5, 7, 3),
  /* 101  */ MP3_DEC_PAIR_V(7, 5, 3),
  /* 110  */ MP3_DEC_PAIR_V(3, 8, 2),
  /* 111  */ MP3_DEC_PAIR_V(3, 8, 2),

  /* 0000 1011 ... */
  /* 000  */ MP3_DEC_PAIR_V(8, 3, 2),	/* 204 */
  /* 001  */ MP3_DEC_PAIR_V(8, 3, 2),
  /* 010  */ MP3_DEC_PAIR_V(6, 6, 3),
  /* 011  */ MP3_DEC_PAIR_V(4, 7, 3),
  /* 100  */ MP3_DEC_PAIR_V(7, 4, 3),
  /* 101  */ MP3_DEC_PAIR_V(5, 6, 3),
  /* 110  */ MP3_DEC_PAIR_V(6, 5, 3),
  /* 111  */ MP3_DEC_PAIR_V(7, 3, 3),

  /* 0000 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 9, 1),	/* 212 */
  /* 1    */ MP3_DEC_PAIR_V(9, 1, 1),

  /* 0000 1101 ... */
  /* 00   */ MP3_DEC_PAIR_V(0, 9, 2),	/* 214 */
  /* 01   */ MP3_DEC_PAIR_V(9, 0, 2),
  /* 10   */ MP3_DEC_PAIR_V(4, 8, 2),
  /* 11   */ MP3_DEC_PAIR_V(8, 4, 2),

  /* 0000 1110 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 2, 2),	/* 218 */
  /* 001  */ MP3_DEC_PAIR_V(7, 2, 2),
  /* 010  */ MP3_DEC_PAIR_V(4, 6, 3),
  /* 011  */ MP3_DEC_PAIR_V(6, 4, 3),
  /* 100  */ MP3_DEC_PAIR_V(2, 8, 1),
  /* 101  */ MP3_DEC_PAIR_V(2, 8, 1),
  /* 110  */ MP3_DEC_PAIR_V(2, 8, 1),
  /* 111  */ MP3_DEC_PAIR_V(2, 8, 1),

  /* 0000 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 2, 1),	/* 226 */
  /* 1    */ MP3_DEC_PAIR_V(1, 8, 1),

  /* 0001 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 7, 2),	/* 228 */
  /* 01   */ MP3_DEC_PAIR_V(2, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(1, 7, 1),
  /* 11   */ MP3_DEC_PAIR_V(1, 7, 1),

  /* 0001 0001 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 1, 1),	/* 232 */
  /* 01   */ MP3_DEC_PAIR_V(7, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(5, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(0, 7, 2),

  /* 0001 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 0, 2),	/* 236 */
  /* 01   */ MP3_DEC_PAIR_V(3, 6, 2),
  /* 10   */ MP3_DEC_PAIR_V(6, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(4, 5, 2),

  /* 0001 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 4, 2),	/* 240 */
  /* 01   */ MP3_DEC_PAIR_V(2, 6, 2),
  /* 10   */ MP3_DEC_PAIR_V(6, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 5, 2),

  /* 0001 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 8, 1),	/* 244 */
  /* 1    */ MP3_DEC_PAIR_V(8, 0, 1),

  /* 0001 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 6, 1),	/* 246 */
  /* 1    */ MP3_DEC_PAIR_V(6, 1, 1),

  /* 0001 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 6, 1),	/* 248 */
  /* 1    */ MP3_DEC_PAIR_V(6, 0, 1),

  /* 0001 1000 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 3, 2),	/* 250 */
  /* 01   */ MP3_DEC_PAIR_V(4, 4, 2),
  /* 10   */ MP3_DEC_PAIR_V(2, 5, 1),
  /* 11   */ MP3_DEC_PAIR_V(2, 5, 1),

  /* 0001 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 2, 1),	/* 254 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1),

  /* 0001 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 4, 1),	/* 256 */
  /* 1    */ MP3_DEC_PAIR_V(4, 3, 1),

  /* 0001 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 0, 1),	/* 258 */
  /* 1    */ MP3_DEC_PAIR_V(2, 4, 1),

  /* 0001 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 2, 1),	/* 260 */
  /* 1    */ MP3_DEC_PAIR_V(3, 3, 1),

  /* 0000 0000 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(388, 3),	/* 262 */
  /* 0001 */ MP3_DEC_PAIR_V(15, 15, 4),
  /* 0010 */ MP3_DEC_PAIR_V(14, 15, 4),
  /* 0011 */ MP3_DEC_PAIR_V(13, 15, 4),
  /* 0100 */ MP3_DEC_PAIR_V(14, 14, 4),
  /* 0101 */ MP3_DEC_PAIR_V(12, 15, 4),
  /* 0110 */ MP3_DEC_PAIR_V(13, 14, 4),
  /* 0111 */ MP3_DEC_PAIR_V(11, 15, 4),
  /* 1000 */ MP3_DEC_PAIR_V(15, 11, 4),
  /* 1001 */ MP3_DEC_PAIR_V(12, 14, 4),
  /* 1010 */ MP3_DEC_PAIR_V(13, 12, 4),
  /* 1011 */ MP3_DEC_QUAD_PTR(396, 1),
  /* 1100 */ MP3_DEC_PAIR_V(14, 12, 3),
  /* 1101 */ MP3_DEC_PAIR_V(14, 12, 3),
  /* 1110 */ MP3_DEC_PAIR_V(13, 13, 3),
  /* 1111 */ MP3_DEC_PAIR_V(13, 13, 3),

  /* 0000 0000 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(15, 10, 4),	/* 278 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 13, 4),
  /* 0010 */ MP3_DEC_PAIR_V(11, 14, 3),
  /* 0011 */ MP3_DEC_PAIR_V(11, 14, 3),
  /* 0100 */ MP3_DEC_PAIR_V(14, 11, 3),
  /* 0101 */ MP3_DEC_PAIR_V(14, 11, 3),
  /* 0110 */ MP3_DEC_PAIR_V(9, 15, 3),
  /* 0111 */ MP3_DEC_PAIR_V(9, 15, 3),
  /* 1000 */ MP3_DEC_PAIR_V(15, 9, 3),
  /* 1001 */ MP3_DEC_PAIR_V(15, 9, 3),
  /* 1010 */ MP3_DEC_PAIR_V(14, 10, 3),
  /* 1011 */ MP3_DEC_PAIR_V(14, 10, 3),
  /* 1100 */ MP3_DEC_PAIR_V(11, 13, 3),
  /* 1101 */ MP3_DEC_PAIR_V(11, 13, 3),
  /* 1110 */ MP3_DEC_PAIR_V(13, 11, 3),
  /* 1111 */ MP3_DEC_PAIR_V(13, 11, 3),

  /* 0000 0000 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(8, 15, 3),	/* 294 */
  /* 0001 */ MP3_DEC_PAIR_V(8, 15, 3),
  /* 0010 */ MP3_DEC_PAIR_V(15, 8, 3),
  /* 0011 */ MP3_DEC_PAIR_V(15, 8, 3),
  /* 0100 */ MP3_DEC_PAIR_V(12, 12, 3),
  /* 0101 */ MP3_DEC_PAIR_V(12, 12, 3),
  /* 0110 */ MP3_DEC_PAIR_V(10, 14, 4),
  /* 0111 */ MP3_DEC_PAIR_V(9, 14, 4),
  /* 1000 */ MP3_DEC_PAIR_V(8, 14, 3),
  /* 1001 */ MP3_DEC_PAIR_V(8, 14, 3),
  /* 1010 */ MP3_DEC_PAIR_V(7, 15, 4),
  /* 1011 */ MP3_DEC_PAIR_V(7, 14, 4),
  /* 1100 */ MP3_DEC_PAIR_V(15, 7, 2),
  /* 1101 */ MP3_DEC_PAIR_V(15, 7, 2),
  /* 1110 */ MP3_DEC_PAIR_V(15, 7, 2),
  /* 1111 */ MP3_DEC_PAIR_V(15, 7, 2),

  /* 0000 0000 0011 ... */
  /* 000  */ MP3_DEC_PAIR_V(13, 10, 2),	/* 310 */
  /* 001  */ MP3_DEC_PAIR_V(13, 10, 2),
  /* 010  */ MP3_DEC_PAIR_V(10, 13, 3),
  /* 011  */ MP3_DEC_PAIR_V(11, 12, 3),
  /* 100  */ MP3_DEC_PAIR_V(12, 11, 3),
  /* 101  */ MP3_DEC_PAIR_V(15, 6, 3),
  /* 110  */ MP3_DEC_PAIR_V(6, 15, 2),
  /* 111  */ MP3_DEC_PAIR_V(6, 15, 2),

  /* 0000 0000 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(14, 8, 2),	/* 318 */
  /* 01   */ MP3_DEC_PAIR_V(5, 15, 2),
  /* 10   */ MP3_DEC_PAIR_V(9, 13, 2),
  /* 11   */ MP3_DEC_PAIR_V(13, 9, 2),

  /* 0000 0000 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(15, 5, 2),	/* 322 */
  /* 01   */ MP3_DEC_PAIR_V(14, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 12, 2),
  /* 11   */ MP3_DEC_PAIR_V(11, 11, 2),

  /* 0000 0000 0110 ... */
  /* 000  */ MP3_DEC_PAIR_V(4, 15, 2),	/* 326 */
  /* 001  */ MP3_DEC_PAIR_V(4, 15, 2),
  /* 010  */ MP3_DEC_PAIR_V(15, 4, 2),
  /* 011  */ MP3_DEC_PAIR_V(15, 4, 2),
  /* 100  */ MP3_DEC_PAIR_V(12, 10, 3),
  /* 101  */ MP3_DEC_PAIR_V(14, 6, 3),
  /* 110  */ MP3_DEC_PAIR_V(15, 3, 2),
  /* 111  */ MP3_DEC_PAIR_V(15, 3, 2),

  /* 0000 0000 0111 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 15, 1),	/* 334 */
  /* 01   */ MP3_DEC_PAIR_V(3, 15, 1),
  /* 10   */ MP3_DEC_PAIR_V(8, 13, 2),
  /* 11   */ MP3_DEC_PAIR_V(13, 8, 2),

  /* 0000 0000 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 15, 1),	/* 338 */
  /* 1    */ MP3_DEC_PAIR_V(15, 2, 1),

  /* 0000 0000 1001 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 14, 2),	/* 340 */
  /* 01   */ MP3_DEC_PAIR_V(9, 12, 2),
  /* 10   */ MP3_DEC_PAIR_V(0, 15, 1),
  /* 11   */ MP3_DEC_PAIR_V(0, 15, 1),

  /* 0000 0000 1010 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 9, 2),	/* 344 */
  /* 01   */ MP3_DEC_PAIR_V(5, 14, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 11, 1),
  /* 11   */ MP3_DEC_PAIR_V(10, 11, 1),

  /* 0000 0000 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 13, 2),	/* 348 */
  /* 01   */ MP3_DEC_PAIR_V(13, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(4, 14, 1),
  /* 11   */ MP3_DEC_PAIR_V(4, 14, 1),

  /* 0000 0000 1100 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 8, 2),	/* 352 */
  /* 01   */ MP3_DEC_PAIR_V(13, 6, 2),
  /* 10   */ MP3_DEC_PAIR_V(3, 14, 1),
  /* 11   */ MP3_DEC_PAIR_V(3, 14, 1),

  /* 0000 0000 1101 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 9, 1),	/* 356 */
  /* 01   */ MP3_DEC_PAIR_V(11, 9, 1),
  /* 10   */ MP3_DEC_PAIR_V(9, 11, 2),
  /* 11   */ MP3_DEC_PAIR_V(10, 10, 2),

  /* 0000 0001 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(11, 10, 1),	/* 360 */
  /* 1    */ MP3_DEC_PAIR_V(14, 5, 1),

  /* 0000 0001 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(14, 4, 1),	/* 362 */
  /* 1    */ MP3_DEC_PAIR_V(8, 12, 1),

  /* 0000 0001 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 13, 1),	/* 364 */
  /* 1    */ MP3_DEC_PAIR_V(14, 3, 1),

  /* 0000 0001 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 14, 1),	/* 366 */
  /* 1    */ MP3_DEC_PAIR_V(0, 14, 1),

  /* 0000 0001 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(14, 0, 1),	/* 368 */
  /* 1    */ MP3_DEC_PAIR_V(5, 13, 1),

  /* 0000 0001 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(13, 5, 1),	/* 370 */
  /* 1    */ MP3_DEC_PAIR_V(7, 12, 1),

  /* 0000 0001 1010 ... */
  /* 0    */ MP3_DEC_PAIR_V(12, 7, 1),	/* 372 */
  /* 1    */ MP3_DEC_PAIR_V(4, 13, 1),

  /* 0000 0001 1011 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 11, 1),	/* 374 */
  /* 1    */ MP3_DEC_PAIR_V(11, 8, 1),

  /* 0000 0001 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(13, 4, 1),	/* 376 */
  /* 1    */ MP3_DEC_PAIR_V(9, 10, 1),

  /* 0000 0001 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(10, 9, 1),	/* 378 */
  /* 1    */ MP3_DEC_PAIR_V(6, 12, 1),

  /* 0000 0010 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(13, 3, 1),	/* 380 */
  /* 1    */ MP3_DEC_PAIR_V(7, 11, 1),

  /* 0000 0010 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 12, 1),	/* 382 */
  /* 1    */ MP3_DEC_PAIR_V(12, 5, 1),

  /* 0000 0010 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 9, 1),	/* 384 */
  /* 1    */ MP3_DEC_PAIR_V(7, 10, 1),

  /* 0000 0010 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(10, 7, 1),	/* 386 */
  /* 1    */ MP3_DEC_PAIR_V(9, 7, 1),

  /* 0000 0000 0000 0000 ... */
  /* 000  */ MP3_DEC_PAIR_V(15, 14, 3),	/* 388 */
  /* 001  */ MP3_DEC_PAIR_V(15, 12, 3),
  /* 010  */ MP3_DEC_PAIR_V(15, 13, 2),
  /* 011  */ MP3_DEC_PAIR_V(15, 13, 2),
  /* 100  */ MP3_DEC_PAIR_V(14, 13, 1),
  /* 101  */ MP3_DEC_PAIR_V(14, 13, 1),
  /* 110  */ MP3_DEC_PAIR_V(14, 13, 1),
  /* 111  */ MP3_DEC_PAIR_V(14, 13, 1),

  /* 0000 0000 0000 1011 ... */
  /* 0    */ MP3_DEC_PAIR_V(10, 15, 1),	/* 396 */
  /* 1    */ MP3_DEC_PAIR_V(14, 9, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab15[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(64, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(80, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(96, 3),
  /* 0110 */ MP3_DEC_QUAD_PTR(104, 3),
  /* 0111 */ MP3_DEC_QUAD_PTR(112, 2),
  /* 1000 */ MP3_DEC_QUAD_PTR(116, 1),
  /* 1001 */ MP3_DEC_QUAD_PTR(118, 1),
  /* 1010 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(1, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 1, 4),
  /* 1101 */ MP3_DEC_PAIR_V(1, 0, 4),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 3),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 3),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(120, 4),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(136, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(152, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(168, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(184, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(200, 3),
  /* 0110 */ MP3_DEC_QUAD_PTR(208, 3),
  /* 0111 */ MP3_DEC_QUAD_PTR(216, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(232, 3),
  /* 1001 */ MP3_DEC_QUAD_PTR(240, 3),
  /* 1010 */ MP3_DEC_QUAD_PTR(248, 3),
  /* 1011 */ MP3_DEC_QUAD_PTR(256, 3),
  /* 1100 */ MP3_DEC_QUAD_PTR(264, 2),
  /* 1101 */ MP3_DEC_QUAD_PTR(268, 3),
  /* 1110 */ MP3_DEC_QUAD_PTR(276, 3),
  /* 1111 */ MP3_DEC_QUAD_PTR(284, 2),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(288, 2),	/* 32 */
  /* 0001 */ MP3_DEC_QUAD_PTR(292, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(296, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(300, 2),
  /* 0100 */ MP3_DEC_QUAD_PTR(304, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(308, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(312, 2),
  /* 0111 */ MP3_DEC_QUAD_PTR(316, 2),
  /* 1000 */ MP3_DEC_QUAD_PTR(320, 1),
  /* 1001 */ MP3_DEC_QUAD_PTR(322, 1),
  /* 1010 */ MP3_DEC_QUAD_PTR(324, 1),
  /* 1011 */ MP3_DEC_QUAD_PTR(326, 2),
  /* 1100 */ MP3_DEC_QUAD_PTR(330, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(332, 1),
  /* 1110 */ MP3_DEC_QUAD_PTR(334, 2),
  /* 1111 */ MP3_DEC_QUAD_PTR(338, 1),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(340, 1),	/* 48 */
  /* 0001 */ MP3_DEC_QUAD_PTR(342, 1),
  /* 0010 */ MP3_DEC_PAIR_V(9, 1, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(344, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(346, 1),
  /* 0101 */ MP3_DEC_QUAD_PTR(348, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(350, 1),
  /* 0111 */ MP3_DEC_QUAD_PTR(352, 1),
  /* 1000 */ MP3_DEC_PAIR_V(2, 8, 4),
  /* 1001 */ MP3_DEC_PAIR_V(8, 2, 4),
  /* 1010 */ MP3_DEC_PAIR_V(1, 8, 4),
  /* 1011 */ MP3_DEC_PAIR_V(8, 1, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(354, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(356, 1),
  /* 1110 */ MP3_DEC_QUAD_PTR(358, 1),
  /* 1111 */ MP3_DEC_QUAD_PTR(360, 1),

  /* 0011 ... */
  /* 0000 */ MP3_DEC_PAIR_V(2, 7, 4),	/* 64 */
  /* 0001 */ MP3_DEC_PAIR_V(7, 2, 4),
  /* 0010 */ MP3_DEC_PAIR_V(6, 4, 4),
  /* 0011 */ MP3_DEC_PAIR_V(1, 7, 4),
  /* 0100 */ MP3_DEC_PAIR_V(5, 5, 4),
  /* 0101 */ MP3_DEC_PAIR_V(7, 1, 4),
  /* 0110 */ MP3_DEC_QUAD_PTR(362, 1),
  /* 0111 */ MP3_DEC_PAIR_V(3, 6, 4),
  /* 1000 */ MP3_DEC_PAIR_V(6, 3, 4),
  /* 1001 */ MP3_DEC_PAIR_V(4, 5, 4),
  /* 1010 */ MP3_DEC_PAIR_V(5, 4, 4),
  /* 1011 */ MP3_DEC_PAIR_V(2, 6, 4),
  /* 1100 */ MP3_DEC_PAIR_V(6, 2, 4),
  /* 1101 */ MP3_DEC_PAIR_V(1, 6, 4),
  /* 1110 */ MP3_DEC_QUAD_PTR(364, 1),
  /* 1111 */ MP3_DEC_PAIR_V(3, 5, 4),

  /* 0100 ... */
  /* 0000 */ MP3_DEC_PAIR_V(6, 1, 3),	/* 80 */
  /* 0001 */ MP3_DEC_PAIR_V(6, 1, 3),
  /* 0010 */ MP3_DEC_PAIR_V(5, 3, 4),
  /* 0011 */ MP3_DEC_PAIR_V(4, 4, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 5, 3),
  /* 0101 */ MP3_DEC_PAIR_V(2, 5, 3),
  /* 0110 */ MP3_DEC_PAIR_V(5, 2, 3),
  /* 0111 */ MP3_DEC_PAIR_V(5, 2, 3),
  /* 1000 */ MP3_DEC_PAIR_V(1, 5, 3),
  /* 1001 */ MP3_DEC_PAIR_V(1, 5, 3),
  /* 1010 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 5, 4),
  /* 1101 */ MP3_DEC_PAIR_V(5, 0, 4),
  /* 1110 */ MP3_DEC_PAIR_V(3, 4, 3),
  /* 1111 */ MP3_DEC_PAIR_V(3, 4, 3),

  /* 0101 ... */
  /* 000  */ MP3_DEC_PAIR_V(4, 3, 3),	/* 96 */
  /* 001  */ MP3_DEC_PAIR_V(2, 4, 3),
  /* 010  */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 011  */ MP3_DEC_PAIR_V(3, 3, 3),
  /* 100  */ MP3_DEC_PAIR_V(4, 1, 2),
  /* 101  */ MP3_DEC_PAIR_V(4, 1, 2),
  /* 110  */ MP3_DEC_PAIR_V(1, 4, 3),
  /* 111  */ MP3_DEC_PAIR_V(0, 4, 3),

  /* 0110 ... */
  /* 000  */ MP3_DEC_PAIR_V(2, 3, 2),	/* 104 */
  /* 001  */ MP3_DEC_PAIR_V(2, 3, 2),
  /* 010  */ MP3_DEC_PAIR_V(3, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(3, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(4, 0, 3),
  /* 101  */ MP3_DEC_PAIR_V(0, 3, 3),
  /* 110  */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 111  */ MP3_DEC_PAIR_V(1, 3, 2),

  /* 0111 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 1, 2),	/* 112 */
  /* 01   */ MP3_DEC_PAIR_V(3, 0, 2),
  /* 10   */ MP3_DEC_PAIR_V(2, 2, 1),
  /* 11   */ MP3_DEC_PAIR_V(2, 2, 1),

  /* 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 2, 1),	/* 116 */
  /* 1    */ MP3_DEC_PAIR_V(2, 1, 1),

  /* 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 2, 1),	/* 118 */
  /* 1    */ MP3_DEC_PAIR_V(2, 0, 1),

  /* 0000 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(366, 1),	/* 120 */
  /* 0001 */ MP3_DEC_QUAD_PTR(368, 1),
  /* 0010 */ MP3_DEC_PAIR_V(14, 14, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(370, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(372, 1),
  /* 0101 */ MP3_DEC_QUAD_PTR(374, 1),
  /* 0110 */ MP3_DEC_PAIR_V(15, 11, 4),
  /* 0111 */ MP3_DEC_QUAD_PTR(376, 1),
  /* 1000 */ MP3_DEC_PAIR_V(13, 13, 4),
  /* 1001 */ MP3_DEC_PAIR_V(10, 15, 4),
  /* 1010 */ MP3_DEC_PAIR_V(15, 10, 4),
  /* 1011 */ MP3_DEC_PAIR_V(11, 14, 4),
  /* 1100 */ MP3_DEC_PAIR_V(14, 11, 4),
  /* 1101 */ MP3_DEC_PAIR_V(12, 13, 4),
  /* 1110 */ MP3_DEC_PAIR_V(13, 12, 4),
  /* 1111 */ MP3_DEC_PAIR_V(9, 15, 4),

  /* 0000 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(15, 9, 4),	/* 136 */
  /* 0001 */ MP3_DEC_PAIR_V(14, 10, 4),
  /* 0010 */ MP3_DEC_PAIR_V(11, 13, 4),
  /* 0011 */ MP3_DEC_PAIR_V(13, 11, 4),
  /* 0100 */ MP3_DEC_PAIR_V(8, 15, 4),
  /* 0101 */ MP3_DEC_PAIR_V(15, 8, 4),
  /* 0110 */ MP3_DEC_PAIR_V(12, 12, 4),
  /* 0111 */ MP3_DEC_PAIR_V(9, 14, 4),
  /* 1000 */ MP3_DEC_PAIR_V(14, 9, 4),
  /* 1001 */ MP3_DEC_PAIR_V(7, 15, 4),
  /* 1010 */ MP3_DEC_PAIR_V(15, 7, 4),
  /* 1011 */ MP3_DEC_PAIR_V(10, 13, 4),
  /* 1100 */ MP3_DEC_PAIR_V(13, 10, 4),
  /* 1101 */ MP3_DEC_PAIR_V(11, 12, 4),
  /* 1110 */ MP3_DEC_PAIR_V(6, 15, 4),
  /* 1111 */ MP3_DEC_QUAD_PTR(378, 1),

  /* 0000 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 11, 3),	/* 152 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 11, 3),
  /* 0010 */ MP3_DEC_PAIR_V(15, 6, 3),
  /* 0011 */ MP3_DEC_PAIR_V(15, 6, 3),
  /* 0100 */ MP3_DEC_PAIR_V(8, 14, 4),
  /* 0101 */ MP3_DEC_PAIR_V(14, 8, 4),
  /* 0110 */ MP3_DEC_PAIR_V(5, 15, 4),
  /* 0111 */ MP3_DEC_PAIR_V(9, 13, 4),
  /* 1000 */ MP3_DEC_PAIR_V(15, 5, 3),
  /* 1001 */ MP3_DEC_PAIR_V(15, 5, 3),
  /* 1010 */ MP3_DEC_PAIR_V(7, 14, 3),
  /* 1011 */ MP3_DEC_PAIR_V(7, 14, 3),
  /* 1100 */ MP3_DEC_PAIR_V(14, 7, 3),
  /* 1101 */ MP3_DEC_PAIR_V(14, 7, 3),
  /* 1110 */ MP3_DEC_PAIR_V(10, 12, 3),
  /* 1111 */ MP3_DEC_PAIR_V(10, 12, 3),

  /* 0000 0011 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 10, 3),	/* 168 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 10, 3),
  /* 0010 */ MP3_DEC_PAIR_V(11, 11, 3),
  /* 0011 */ MP3_DEC_PAIR_V(11, 11, 3),
  /* 0100 */ MP3_DEC_PAIR_V(13, 9, 4),
  /* 0101 */ MP3_DEC_PAIR_V(8, 13, 4),
  /* 0110 */ MP3_DEC_PAIR_V(4, 15, 3),
  /* 0111 */ MP3_DEC_PAIR_V(4, 15, 3),
  /* 1000 */ MP3_DEC_PAIR_V(15, 4, 3),
  /* 1001 */ MP3_DEC_PAIR_V(15, 4, 3),
  /* 1010 */ MP3_DEC_PAIR_V(3, 15, 3),
  /* 1011 */ MP3_DEC_PAIR_V(3, 15, 3),
  /* 1100 */ MP3_DEC_PAIR_V(15, 3, 3),
  /* 1101 */ MP3_DEC_PAIR_V(15, 3, 3),
  /* 1110 */ MP3_DEC_PAIR_V(13, 8, 3),
  /* 1111 */ MP3_DEC_PAIR_V(13, 8, 3),

  /* 0000 0100 ... */
  /* 0000 */ MP3_DEC_PAIR_V(14, 6, 3),	/* 184 */
  /* 0001 */ MP3_DEC_PAIR_V(14, 6, 3),
  /* 0010 */ MP3_DEC_PAIR_V(2, 15, 3),
  /* 0011 */ MP3_DEC_PAIR_V(2, 15, 3),
  /* 0100 */ MP3_DEC_PAIR_V(15, 2, 3),
  /* 0101 */ MP3_DEC_PAIR_V(15, 2, 3),
  /* 0110 */ MP3_DEC_PAIR_V(6, 14, 4),
  /* 0111 */ MP3_DEC_PAIR_V(15, 0, 4),
  /* 1000 */ MP3_DEC_PAIR_V(1, 15, 3),
  /* 1001 */ MP3_DEC_PAIR_V(1, 15, 3),
  /* 1010 */ MP3_DEC_PAIR_V(15, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(15, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(9, 12, 3),
  /* 1101 */ MP3_DEC_PAIR_V(9, 12, 3),
  /* 1110 */ MP3_DEC_PAIR_V(12, 9, 3),
  /* 1111 */ MP3_DEC_PAIR_V(12, 9, 3),

  /* 0000 0101 ... */
  /* 000  */ MP3_DEC_PAIR_V(5, 14, 3),	/* 200 */
  /* 001  */ MP3_DEC_PAIR_V(10, 11, 3),
  /* 010  */ MP3_DEC_PAIR_V(11, 10, 3),
  /* 011  */ MP3_DEC_PAIR_V(14, 5, 3),
  /* 100  */ MP3_DEC_PAIR_V(7, 13, 3),
  /* 101  */ MP3_DEC_PAIR_V(13, 7, 3),
  /* 110  */ MP3_DEC_PAIR_V(4, 14, 3),
  /* 111  */ MP3_DEC_PAIR_V(14, 4, 3),

  /* 0000 0110 ... */
  /* 000  */ MP3_DEC_PAIR_V(8, 12, 3),	/* 208 */
  /* 001  */ MP3_DEC_PAIR_V(12, 8, 3),
  /* 010  */ MP3_DEC_PAIR_V(3, 14, 3),
  /* 011  */ MP3_DEC_PAIR_V(6, 13, 3),
  /* 100  */ MP3_DEC_PAIR_V(13, 6, 3),
  /* 101  */ MP3_DEC_PAIR_V(14, 3, 3),
  /* 110  */ MP3_DEC_PAIR_V(9, 11, 3),
  /* 111  */ MP3_DEC_PAIR_V(11, 9, 3),

  /* 0000 0111 ... */
  /* 0000 */ MP3_DEC_PAIR_V(2, 14, 3),	/* 216 */
  /* 0001 */ MP3_DEC_PAIR_V(2, 14, 3),
  /* 0010 */ MP3_DEC_PAIR_V(10, 10, 3),
  /* 0011 */ MP3_DEC_PAIR_V(10, 10, 3),
  /* 0100 */ MP3_DEC_PAIR_V(14, 2, 3),
  /* 0101 */ MP3_DEC_PAIR_V(14, 2, 3),
  /* 0110 */ MP3_DEC_PAIR_V(1, 14, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 14, 3),
  /* 1000 */ MP3_DEC_PAIR_V(14, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(14, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(0, 14, 4),
  /* 1011 */ MP3_DEC_PAIR_V(14, 0, 4),
  /* 1100 */ MP3_DEC_PAIR_V(5, 13, 3),
  /* 1101 */ MP3_DEC_PAIR_V(5, 13, 3),
  /* 1110 */ MP3_DEC_PAIR_V(13, 5, 3),
  /* 1111 */ MP3_DEC_PAIR_V(13, 5, 3),

  /* 0000 1000 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 12, 3),	/* 232 */
  /* 001  */ MP3_DEC_PAIR_V(12, 7, 3),
  /* 010  */ MP3_DEC_PAIR_V(4, 13, 3),
  /* 011  */ MP3_DEC_PAIR_V(8, 11, 3),
  /* 100  */ MP3_DEC_PAIR_V(13, 4, 2),
  /* 101  */ MP3_DEC_PAIR_V(13, 4, 2),
  /* 110  */ MP3_DEC_PAIR_V(11, 8, 3),
  /* 111  */ MP3_DEC_PAIR_V(9, 10, 3),

  /* 0000 1001 ... */
  /* 000  */ MP3_DEC_PAIR_V(10, 9, 3),	/* 240 */
  /* 001  */ MP3_DEC_PAIR_V(6, 12, 3),
  /* 010  */ MP3_DEC_PAIR_V(12, 6, 3),
  /* 011  */ MP3_DEC_PAIR_V(3, 13, 3),
  /* 100  */ MP3_DEC_PAIR_V(13, 3, 2),
  /* 101  */ MP3_DEC_PAIR_V(13, 3, 2),
  /* 110  */ MP3_DEC_PAIR_V(13, 2, 2),
  /* 111  */ MP3_DEC_PAIR_V(13, 2, 2),

  /* 0000 1010 ... */
  /* 000  */ MP3_DEC_PAIR_V(2, 13, 3),	/* 248 */
  /* 001  */ MP3_DEC_PAIR_V(0, 13, 3),
  /* 010  */ MP3_DEC_PAIR_V(1, 13, 2),
  /* 011  */ MP3_DEC_PAIR_V(1, 13, 2),
  /* 100  */ MP3_DEC_PAIR_V(7, 11, 2),
  /* 101  */ MP3_DEC_PAIR_V(7, 11, 2),
  /* 110  */ MP3_DEC_PAIR_V(11, 7, 2),
  /* 111  */ MP3_DEC_PAIR_V(11, 7, 2),

  /* 0000 1011 ... */
  /* 000  */ MP3_DEC_PAIR_V(13, 1, 2),	/* 256 */
  /* 001  */ MP3_DEC_PAIR_V(13, 1, 2),
  /* 010  */ MP3_DEC_PAIR_V(5, 12, 3),
  /* 011  */ MP3_DEC_PAIR_V(13, 0, 3),
  /* 100  */ MP3_DEC_PAIR_V(12, 5, 2),
  /* 101  */ MP3_DEC_PAIR_V(12, 5, 2),
  /* 110  */ MP3_DEC_PAIR_V(8, 10, 2),
  /* 111  */ MP3_DEC_PAIR_V(8, 10, 2),

  /* 0000 1100 ... */
  /* 00   */ MP3_DEC_PAIR_V(10, 8, 2),	/* 264 */
  /* 01   */ MP3_DEC_PAIR_V(4, 12, 2),
  /* 10   */ MP3_DEC_PAIR_V(12, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 11, 2),

  /* 0000 1101 ... */
  /* 000  */ MP3_DEC_PAIR_V(11, 6, 2),	/* 268 */
  /* 001  */ MP3_DEC_PAIR_V(11, 6, 2),
  /* 010  */ MP3_DEC_PAIR_V(9, 9, 3),
  /* 011  */ MP3_DEC_PAIR_V(0, 12, 3),
  /* 100  */ MP3_DEC_PAIR_V(3, 12, 2),
  /* 101  */ MP3_DEC_PAIR_V(3, 12, 2),
  /* 110  */ MP3_DEC_PAIR_V(12, 3, 2),
  /* 111  */ MP3_DEC_PAIR_V(12, 3, 2),

  /* 0000 1110 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 10, 2),	/* 276 */
  /* 001  */ MP3_DEC_PAIR_V(7, 10, 2),
  /* 010  */ MP3_DEC_PAIR_V(10, 7, 2),
  /* 011  */ MP3_DEC_PAIR_V(10, 7, 2),
  /* 100  */ MP3_DEC_PAIR_V(10, 6, 2),
  /* 101  */ MP3_DEC_PAIR_V(10, 6, 2),
  /* 110  */ MP3_DEC_PAIR_V(12, 0, 3),
  /* 111  */ MP3_DEC_PAIR_V(0, 11, 3),

  /* 0000 1111 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 2, 1),	/* 284 */
  /* 01   */ MP3_DEC_PAIR_V(12, 2, 1),
  /* 10   */ MP3_DEC_PAIR_V(2, 12, 2),
  /* 11   */ MP3_DEC_PAIR_V(5, 11, 2),

  /* 0001 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 5, 2),	/* 288 */
  /* 01   */ MP3_DEC_PAIR_V(1, 12, 2),
  /* 10   */ MP3_DEC_PAIR_V(8, 9, 2),
  /* 11   */ MP3_DEC_PAIR_V(9, 8, 2),

  /* 0001 0001 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 1, 2),	/* 292 */
  /* 01   */ MP3_DEC_PAIR_V(4, 11, 2),
  /* 10   */ MP3_DEC_PAIR_V(11, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 10, 2),

  /* 0001 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(3, 11, 2),	/* 296 */
  /* 01   */ MP3_DEC_PAIR_V(7, 9, 2),
  /* 10   */ MP3_DEC_PAIR_V(11, 3, 1),
  /* 11   */ MP3_DEC_PAIR_V(11, 3, 1),

  /* 0001 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(9, 7, 2),	/* 300 */
  /* 01   */ MP3_DEC_PAIR_V(8, 8, 2),
  /* 10   */ MP3_DEC_PAIR_V(2, 11, 2),
  /* 11   */ MP3_DEC_PAIR_V(5, 10, 2),

  /* 0001 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 2, 1),	/* 304 */
  /* 01   */ MP3_DEC_PAIR_V(11, 2, 1),
  /* 10   */ MP3_DEC_PAIR_V(10, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(1, 11, 2),

  /* 0001 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 1, 1),	/* 308 */
  /* 01   */ MP3_DEC_PAIR_V(11, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(11, 0, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 9, 2),

  /* 0001 0110 ... */
  /* 00   */ MP3_DEC_PAIR_V(9, 6, 2),	/* 312 */
  /* 01   */ MP3_DEC_PAIR_V(4, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(7, 8, 2),

  /* 0001 0111 ... */
  /* 00   */ MP3_DEC_PAIR_V(8, 7, 2),	/* 316 */
  /* 01   */ MP3_DEC_PAIR_V(3, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 3, 1),
  /* 11   */ MP3_DEC_PAIR_V(10, 3, 1),

  /* 0001 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 9, 1),	/* 320 */
  /* 1    */ MP3_DEC_PAIR_V(9, 5, 1),

  /* 0001 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 10, 1),	/* 322 */
  /* 1    */ MP3_DEC_PAIR_V(10, 2, 1),

  /* 0001 1010 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 10, 1),	/* 324 */
  /* 1    */ MP3_DEC_PAIR_V(10, 1, 1),

  /* 0001 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(0, 10, 2),	/* 326 */
  /* 01   */ MP3_DEC_PAIR_V(10, 0, 2),
  /* 10   */ MP3_DEC_PAIR_V(6, 8, 1),
  /* 11   */ MP3_DEC_PAIR_V(6, 8, 1),

  /* 0001 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 6, 1),	/* 330 */
  /* 1    */ MP3_DEC_PAIR_V(4, 9, 1),

  /* 0001 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 4, 1),	/* 332 */
  /* 1    */ MP3_DEC_PAIR_V(3, 9, 1),

  /* 0001 1110 ... */
  /* 00   */ MP3_DEC_PAIR_V(9, 3, 1),	/* 334 */
  /* 01   */ MP3_DEC_PAIR_V(9, 3, 1),
  /* 10   */ MP3_DEC_PAIR_V(7, 7, 2),
  /* 11   */ MP3_DEC_PAIR_V(0, 9, 2),

  /* 0001 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 8, 1),	/* 338 */
  /* 1    */ MP3_DEC_PAIR_V(8, 5, 1),

  /* 0010 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 9, 1),	/* 340 */
  /* 1    */ MP3_DEC_PAIR_V(6, 7, 1),

  /* 0010 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 6, 1),	/* 342 */
  /* 1    */ MP3_DEC_PAIR_V(9, 2, 1),

  /* 0010 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 9, 1),	/* 344 */
  /* 1    */ MP3_DEC_PAIR_V(9, 0, 1),

  /* 0010 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 8, 1),	/* 346 */
  /* 1    */ MP3_DEC_PAIR_V(8, 4, 1),

  /* 0010 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 7, 1),	/* 348 */
  /* 1    */ MP3_DEC_PAIR_V(7, 5, 1),

  /* 0010 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 8, 1),	/* 350 */
  /* 1    */ MP3_DEC_PAIR_V(8, 3, 1),

  /* 0010 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 6, 1),	/* 352 */
  /* 1    */ MP3_DEC_PAIR_V(4, 7, 1),

  /* 0010 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 4, 1),	/* 354 */
  /* 1    */ MP3_DEC_PAIR_V(0, 8, 1),

  /* 0010 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 0, 1),	/* 356 */
  /* 1    */ MP3_DEC_PAIR_V(5, 6, 1),

  /* 0010 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 5, 1),	/* 358 */
  /* 1    */ MP3_DEC_PAIR_V(3, 7, 1),

  /* 0010 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 3, 1),	/* 360 */
  /* 1    */ MP3_DEC_PAIR_V(4, 6, 1),

  /* 0011 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 7, 1),	/* 362 */
  /* 1    */ MP3_DEC_PAIR_V(7, 0, 1),

  /* 0011 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 6, 1),	/* 364 */
  /* 1    */ MP3_DEC_PAIR_V(6, 0, 1),

  /* 0000 0000 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(15, 15, 1),	/* 366 */
  /* 1    */ MP3_DEC_PAIR_V(14, 15, 1),

  /* 0000 0000 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(15, 14, 1),	/* 368 */
  /* 1    */ MP3_DEC_PAIR_V(13, 15, 1),

  /* 0000 0000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(15, 13, 1),	/* 370 */
  /* 1    */ MP3_DEC_PAIR_V(12, 15, 1),

  /* 0000 0000 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(15, 12, 1),	/* 372 */
  /* 1    */ MP3_DEC_PAIR_V(13, 14, 1),

  /* 0000 0000 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(14, 13, 1),	/* 374 */
  /* 1    */ MP3_DEC_PAIR_V(11, 15, 1),

  /* 0000 0000 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(12, 14, 1),	/* 376 */
  /* 1    */ MP3_DEC_PAIR_V(14, 12, 1),

  /* 0000 0001 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(10, 14, 1),	/* 378 */
  /* 1    */ MP3_DEC_PAIR_V(0, 15, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab16[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(64, 2),
  /* 0100 */ MP3_DEC_PAIR_V(1, 1, 4),
  /* 0101 */ MP3_DEC_PAIR_V(0, 1, 4),
  /* 0110 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 0, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1001 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1010 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1011 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1100 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1101 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1110 */ MP3_DEC_PAIR_V(0, 0, 1),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 1),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(68, 3),	/* 16 */
  /* 0001 */ MP3_DEC_QUAD_PTR(76, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(84, 2),
  /* 0011 */ MP3_DEC_PAIR_V(15, 15, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(88, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(92, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(94, 4),
  /* 0111 */ MP3_DEC_PAIR_V(15, 2, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(110, 1),
  /* 1001 */ MP3_DEC_PAIR_V(1, 15, 4),
  /* 1010 */ MP3_DEC_PAIR_V(15, 1, 4),
  /* 1011 */ MP3_DEC_QUAD_PTR(112, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(128, 4),
  /* 1101 */ MP3_DEC_QUAD_PTR(144, 4),
  /* 1110 */ MP3_DEC_QUAD_PTR(160, 4),
  /* 1111 */ MP3_DEC_QUAD_PTR(176, 4),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(192, 4),	/* 32 */
  /* 0001 */ MP3_DEC_QUAD_PTR(208, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(216, 3),
  /* 0011 */ MP3_DEC_QUAD_PTR(224, 3),
  /* 0100 */ MP3_DEC_QUAD_PTR(232, 3),
  /* 0101 */ MP3_DEC_QUAD_PTR(240, 3),
  /* 0110 */ MP3_DEC_QUAD_PTR(248, 3),
  /* 0111 */ MP3_DEC_QUAD_PTR(256, 3),
  /* 1000 */ MP3_DEC_QUAD_PTR(264, 2),
  /* 1001 */ MP3_DEC_QUAD_PTR(268, 2),
  /* 1010 */ MP3_DEC_QUAD_PTR(272, 1),
  /* 1011 */ MP3_DEC_QUAD_PTR(274, 2),
  /* 1100 */ MP3_DEC_QUAD_PTR(278, 2),
  /* 1101 */ MP3_DEC_QUAD_PTR(282, 1),
  /* 1110 */ MP3_DEC_PAIR_V(5, 1, 4),
  /* 1111 */ MP3_DEC_QUAD_PTR(284, 1),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(286, 1),	/* 48 */
  /* 0001 */ MP3_DEC_QUAD_PTR(288, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(290, 1),
  /* 0011 */ MP3_DEC_PAIR_V(1, 4, 4),
  /* 0100 */ MP3_DEC_PAIR_V(4, 1, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(292, 1),
  /* 0110 */ MP3_DEC_PAIR_V(2, 3, 4),
  /* 0111 */ MP3_DEC_PAIR_V(3, 2, 4),
  /* 1000 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 1001 */ MP3_DEC_PAIR_V(1, 3, 3),
  /* 1010 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1011 */ MP3_DEC_PAIR_V(3, 1, 3),
  /* 1100 */ MP3_DEC_PAIR_V(0, 3, 4),
  /* 1101 */ MP3_DEC_PAIR_V(3, 0, 4),
  /* 1110 */ MP3_DEC_PAIR_V(2, 2, 3),
  /* 1111 */ MP3_DEC_PAIR_V(2, 2, 3),

  /* 0011 ... */
  /* 00   */ MP3_DEC_PAIR_V(1, 2, 2),	/* 64 */
  /* 01   */ MP3_DEC_PAIR_V(2, 1, 2),
  /* 10   */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0000 0000 ... */
  /* 000  */ MP3_DEC_PAIR_V(14, 15, 3),	/* 68 */
  /* 001  */ MP3_DEC_PAIR_V(15, 14, 3),
  /* 010  */ MP3_DEC_PAIR_V(13, 15, 3),
  /* 011  */ MP3_DEC_PAIR_V(15, 13, 3),
  /* 100  */ MP3_DEC_PAIR_V(12, 15, 3),
  /* 101  */ MP3_DEC_PAIR_V(15, 12, 3),
  /* 110  */ MP3_DEC_PAIR_V(11, 15, 3),
  /* 111  */ MP3_DEC_PAIR_V(15, 11, 3),

  /* 0000 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(10, 15, 2),	/* 76 */
  /* 001  */ MP3_DEC_PAIR_V(10, 15, 2),
  /* 010  */ MP3_DEC_PAIR_V(15, 10, 3),
  /* 011  */ MP3_DEC_PAIR_V(9, 15, 3),
  /* 100  */ MP3_DEC_PAIR_V(15, 9, 3),
  /* 101  */ MP3_DEC_PAIR_V(15, 8, 3),
  /* 110  */ MP3_DEC_PAIR_V(8, 15, 2),
  /* 111  */ MP3_DEC_PAIR_V(8, 15, 2),

  /* 0000 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 15, 2),	/* 84 */
  /* 01   */ MP3_DEC_PAIR_V(15, 7, 2),
  /* 10   */ MP3_DEC_PAIR_V(6, 15, 2),
  /* 11   */ MP3_DEC_PAIR_V(15, 6, 2),

  /* 0000 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 15, 2),	/* 88 */
  /* 01   */ MP3_DEC_PAIR_V(15, 5, 2),
  /* 10   */ MP3_DEC_PAIR_V(4, 15, 1),
  /* 11   */ MP3_DEC_PAIR_V(4, 15, 1),

  /* 0000 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(15, 4, 1),	/* 92 */
  /* 1    */ MP3_DEC_PAIR_V(15, 3, 1),

  /* 0000 0110 ... */
  /* 0000 */ MP3_DEC_PAIR_V(15, 0, 1),	/* 94 */
  /* 0001 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0010 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0011 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0100 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0101 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0110 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 0111 */ MP3_DEC_PAIR_V(15, 0, 1),
  /* 1000 */ MP3_DEC_PAIR_V(3, 15, 2),
  /* 1001 */ MP3_DEC_PAIR_V(3, 15, 2),
  /* 1010 */ MP3_DEC_PAIR_V(3, 15, 2),
  /* 1011 */ MP3_DEC_PAIR_V(3, 15, 2),
  /* 1100 */ MP3_DEC_QUAD_PTR(294, 4),
  /* 1101 */ MP3_DEC_QUAD_PTR(310, 3),
  /* 1110 */ MP3_DEC_QUAD_PTR(318, 3),
  /* 1111 */ MP3_DEC_QUAD_PTR(326, 3),

  /* 0000 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 15, 1),	/* 110 */
  /* 1    */ MP3_DEC_PAIR_V(0, 15, 1),

  /* 0000 1011 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(334, 2),	/* 112 */
  /* 0001 */ MP3_DEC_QUAD_PTR(338, 2),
  /* 0010 */ MP3_DEC_QUAD_PTR(342, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(346, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(348, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(352, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(356, 1),
  /* 0111 */ MP3_DEC_QUAD_PTR(358, 2),
  /* 1000 */ MP3_DEC_QUAD_PTR(362, 2),
  /* 1001 */ MP3_DEC_QUAD_PTR(366, 2),
  /* 1010 */ MP3_DEC_QUAD_PTR(370, 2),
  /* 1011 */ MP3_DEC_PAIR_V(14, 3, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(374, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(376, 1),
  /* 1110 */ MP3_DEC_QUAD_PTR(378, 1),
  /* 1111 */ MP3_DEC_QUAD_PTR(380, 1),

  /* 0000 1100 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(382, 1),	/* 128 */
  /* 0001 */ MP3_DEC_QUAD_PTR(384, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(386, 1),
  /* 0011 */ MP3_DEC_PAIR_V(0, 13, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(388, 1),
  /* 0101 */ MP3_DEC_QUAD_PTR(390, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(392, 1),
  /* 0111 */ MP3_DEC_PAIR_V(3, 12, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(394, 1),
  /* 1001 */ MP3_DEC_PAIR_V(1, 12, 4),
  /* 1010 */ MP3_DEC_PAIR_V(12, 0, 4),
  /* 1011 */ MP3_DEC_QUAD_PTR(396, 1),
  /* 1100 */ MP3_DEC_PAIR_V(14, 2, 3),
  /* 1101 */ MP3_DEC_PAIR_V(14, 2, 3),
  /* 1110 */ MP3_DEC_PAIR_V(2, 14, 4),
  /* 1111 */ MP3_DEC_PAIR_V(1, 14, 4),

  /* 0000 1101 ... */
  /* 0000 */ MP3_DEC_PAIR_V(13, 3, 4),	/* 144 */
  /* 0001 */ MP3_DEC_PAIR_V(2, 13, 4),
  /* 0010 */ MP3_DEC_PAIR_V(13, 2, 4),
  /* 0011 */ MP3_DEC_PAIR_V(13, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(3, 11, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(398, 1),
  /* 0110 */ MP3_DEC_PAIR_V(1, 13, 3),
  /* 0111 */ MP3_DEC_PAIR_V(1, 13, 3),
  /* 1000 */ MP3_DEC_PAIR_V(12, 4, 4),
  /* 1001 */ MP3_DEC_PAIR_V(6, 11, 4),
  /* 1010 */ MP3_DEC_PAIR_V(12, 3, 4),
  /* 1011 */ MP3_DEC_PAIR_V(10, 7, 4),
  /* 1100 */ MP3_DEC_PAIR_V(2, 12, 3),
  /* 1101 */ MP3_DEC_PAIR_V(2, 12, 3),
  /* 1110 */ MP3_DEC_PAIR_V(12, 2, 4),
  /* 1111 */ MP3_DEC_PAIR_V(11, 5, 4),

  /* 0000 1110 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 1, 4),	/* 160 */
  /* 0001 */ MP3_DEC_PAIR_V(0, 12, 4),
  /* 0010 */ MP3_DEC_PAIR_V(4, 11, 4),
  /* 0011 */ MP3_DEC_PAIR_V(11, 4, 4),
  /* 0100 */ MP3_DEC_PAIR_V(6, 10, 4),
  /* 0101 */ MP3_DEC_PAIR_V(10, 6, 4),
  /* 0110 */ MP3_DEC_PAIR_V(11, 3, 3),
  /* 0111 */ MP3_DEC_PAIR_V(11, 3, 3),
  /* 1000 */ MP3_DEC_PAIR_V(5, 10, 4),
  /* 1001 */ MP3_DEC_PAIR_V(10, 5, 4),
  /* 1010 */ MP3_DEC_PAIR_V(2, 11, 3),
  /* 1011 */ MP3_DEC_PAIR_V(2, 11, 3),
  /* 1100 */ MP3_DEC_PAIR_V(11, 2, 3),
  /* 1101 */ MP3_DEC_PAIR_V(11, 2, 3),
  /* 1110 */ MP3_DEC_PAIR_V(1, 11, 3),
  /* 1111 */ MP3_DEC_PAIR_V(1, 11, 3),

  /* 0000 1111 ... */
  /* 0000 */ MP3_DEC_PAIR_V(11, 1, 3),	/* 176 */
  /* 0001 */ MP3_DEC_PAIR_V(11, 1, 3),
  /* 0010 */ MP3_DEC_PAIR_V(0, 11, 4),
  /* 0011 */ MP3_DEC_PAIR_V(11, 0, 4),
  /* 0100 */ MP3_DEC_PAIR_V(6, 9, 4),
  /* 0101 */ MP3_DEC_PAIR_V(9, 6, 4),
  /* 0110 */ MP3_DEC_PAIR_V(4, 10, 4),
  /* 0111 */ MP3_DEC_PAIR_V(10, 4, 4),
  /* 1000 */ MP3_DEC_PAIR_V(7, 8, 4),
  /* 1001 */ MP3_DEC_PAIR_V(8, 7, 4),
  /* 1010 */ MP3_DEC_PAIR_V(10, 3, 3),
  /* 1011 */ MP3_DEC_PAIR_V(10, 3, 3),
  /* 1100 */ MP3_DEC_PAIR_V(3, 10, 4),
  /* 1101 */ MP3_DEC_PAIR_V(5, 9, 4),
  /* 1110 */ MP3_DEC_PAIR_V(2, 10, 3),
  /* 1111 */ MP3_DEC_PAIR_V(2, 10, 3),

  /* 0001 0000 ... */
  /* 0000 */ MP3_DEC_PAIR_V(9, 5, 4),	/* 192 */
  /* 0001 */ MP3_DEC_PAIR_V(6, 8, 4),
  /* 0010 */ MP3_DEC_PAIR_V(10, 1, 3),
  /* 0011 */ MP3_DEC_PAIR_V(10, 1, 3),
  /* 0100 */ MP3_DEC_PAIR_V(8, 6, 4),
  /* 0101 */ MP3_DEC_PAIR_V(7, 7, 4),
  /* 0110 */ MP3_DEC_PAIR_V(9, 4, 3),
  /* 0111 */ MP3_DEC_PAIR_V(9, 4, 3),
  /* 1000 */ MP3_DEC_PAIR_V(4, 9, 4),
  /* 1001 */ MP3_DEC_PAIR_V(5, 7, 4),
  /* 1010 */ MP3_DEC_PAIR_V(6, 7, 3),
  /* 1011 */ MP3_DEC_PAIR_V(6, 7, 3),
  /* 1100 */ MP3_DEC_PAIR_V(10, 2, 2),
  /* 1101 */ MP3_DEC_PAIR_V(10, 2, 2),
  /* 1110 */ MP3_DEC_PAIR_V(10, 2, 2),
  /* 1111 */ MP3_DEC_PAIR_V(10, 2, 2),

  /* 0001 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(1, 10, 2),	/* 208 */
  /* 001  */ MP3_DEC_PAIR_V(1, 10, 2),
  /* 010  */ MP3_DEC_PAIR_V(0, 10, 3),
  /* 011  */ MP3_DEC_PAIR_V(10, 0, 3),
  /* 100  */ MP3_DEC_PAIR_V(3, 9, 3),
  /* 101  */ MP3_DEC_PAIR_V(9, 3, 3),
  /* 110  */ MP3_DEC_PAIR_V(5, 8, 3),
  /* 111  */ MP3_DEC_PAIR_V(8, 5, 3),

  /* 0001 0010 ... */
  /* 000  */ MP3_DEC_PAIR_V(2, 9, 2),	/* 216 */
  /* 001  */ MP3_DEC_PAIR_V(2, 9, 2),
  /* 010  */ MP3_DEC_PAIR_V(9, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(9, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(7, 6, 3),
  /* 101  */ MP3_DEC_PAIR_V(0, 9, 3),
  /* 110  */ MP3_DEC_PAIR_V(1, 9, 2),
  /* 111  */ MP3_DEC_PAIR_V(1, 9, 2),

  /* 0001 0011 ... */
  /* 000  */ MP3_DEC_PAIR_V(9, 1, 2),	/* 224 */
  /* 001  */ MP3_DEC_PAIR_V(9, 1, 2),
  /* 010  */ MP3_DEC_PAIR_V(9, 0, 3),
  /* 011  */ MP3_DEC_PAIR_V(4, 8, 3),
  /* 100  */ MP3_DEC_PAIR_V(8, 4, 3),
  /* 101  */ MP3_DEC_PAIR_V(7, 5, 3),
  /* 110  */ MP3_DEC_PAIR_V(3, 8, 3),
  /* 111  */ MP3_DEC_PAIR_V(8, 3, 3),

  /* 0001 0100 ... */
  /* 000  */ MP3_DEC_PAIR_V(6, 6, 3),	/* 232 */
  /* 001  */ MP3_DEC_PAIR_V(2, 8, 3),
  /* 010  */ MP3_DEC_PAIR_V(8, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(8, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(4, 7, 3),
  /* 101  */ MP3_DEC_PAIR_V(7, 4, 3),
  /* 110  */ MP3_DEC_PAIR_V(1, 8, 2),
  /* 111  */ MP3_DEC_PAIR_V(1, 8, 2),

  /* 0001 0101 ... */
  /* 000  */ MP3_DEC_PAIR_V(8, 1, 2),	/* 240 */
  /* 001  */ MP3_DEC_PAIR_V(8, 1, 2),
  /* 010  */ MP3_DEC_PAIR_V(8, 0, 2),
  /* 011  */ MP3_DEC_PAIR_V(8, 0, 2),
  /* 100  */ MP3_DEC_PAIR_V(0, 8, 3),
  /* 101  */ MP3_DEC_PAIR_V(5, 6, 3),
  /* 110  */ MP3_DEC_PAIR_V(3, 7, 2),
  /* 111  */ MP3_DEC_PAIR_V(3, 7, 2),

  /* 0001 0110 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 3, 2),	/* 248 */
  /* 001  */ MP3_DEC_PAIR_V(7, 3, 2),
  /* 010  */ MP3_DEC_PAIR_V(6, 5, 3),
  /* 011  */ MP3_DEC_PAIR_V(4, 6, 3),
  /* 100  */ MP3_DEC_PAIR_V(2, 7, 2),
  /* 101  */ MP3_DEC_PAIR_V(2, 7, 2),
  /* 110  */ MP3_DEC_PAIR_V(7, 2, 2),
  /* 111  */ MP3_DEC_PAIR_V(7, 2, 2),

  /* 0001 0111 ... */
  /* 000  */ MP3_DEC_PAIR_V(6, 4, 3),	/* 256 */
  /* 001  */ MP3_DEC_PAIR_V(5, 5, 3),
  /* 010  */ MP3_DEC_PAIR_V(0, 7, 2),
  /* 011  */ MP3_DEC_PAIR_V(0, 7, 2),
  /* 100  */ MP3_DEC_PAIR_V(1, 7, 1),
  /* 101  */ MP3_DEC_PAIR_V(1, 7, 1),
  /* 110  */ MP3_DEC_PAIR_V(1, 7, 1),
  /* 111  */ MP3_DEC_PAIR_V(1, 7, 1),

  /* 0001 1000 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 1, 1),	/* 264  */
  /* 01   */ MP3_DEC_PAIR_V(7, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(7, 0, 2),
  /* 11   */ MP3_DEC_PAIR_V(3, 6, 2),

  /* 0001 1001 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 3, 2),	/* 268 */
  /* 01   */ MP3_DEC_PAIR_V(4, 5, 2),
  /* 10   */ MP3_DEC_PAIR_V(5, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 6, 2),

  /* 0001 1010 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 2, 1),	/* 272 */
  /* 1    */ MP3_DEC_PAIR_V(1, 6, 1),

  /* 0001 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 1, 1),	/* 274 */
  /* 01   */ MP3_DEC_PAIR_V(6, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 6, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 0, 2),

  /* 0001 1100 ... */
  /* 00   */ MP3_DEC_PAIR_V(5, 3, 1),	/* 278 */
  /* 01   */ MP3_DEC_PAIR_V(5, 3, 1),
  /* 10   */ MP3_DEC_PAIR_V(3, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(4, 4, 2),

  /* 0001 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 5, 1),	/* 282 */
  /* 1    */ MP3_DEC_PAIR_V(5, 2, 1),

  /* 0001 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 5, 1),	/* 284 */
  /* 1    */ MP3_DEC_PAIR_V(0, 5, 1),

  /* 0010 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 4, 1),	/* 286 */
  /* 1    */ MP3_DEC_PAIR_V(4, 3, 1),

  /* 0010 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 0, 1),	/* 288 */
  /* 1    */ MP3_DEC_PAIR_V(2, 4, 1),

  /* 0010 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 2, 1),	/* 290 */
  /* 1    */ MP3_DEC_PAIR_V(3, 3, 1),

  /* 0010 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 4, 1),	/* 292 */
  /* 1    */ MP3_DEC_PAIR_V(4, 0, 1),

  /* 0000 0110 1100 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 14, 4),	/* 294 */
  /* 0001 */ MP3_DEC_QUAD_PTR(400, 1),
  /* 0010 */ MP3_DEC_PAIR_V(13, 14, 3),
  /* 0011 */ MP3_DEC_PAIR_V(13, 14, 3),
  /* 0100 */ MP3_DEC_PAIR_V(14, 9, 3),
  /* 0101 */ MP3_DEC_PAIR_V(14, 9, 3),
  /* 0110 */ MP3_DEC_PAIR_V(14, 10, 4),
  /* 0111 */ MP3_DEC_PAIR_V(13, 9, 4),
  /* 1000 */ MP3_DEC_PAIR_V(14, 14, 2),
  /* 1001 */ MP3_DEC_PAIR_V(14, 14, 2),
  /* 1010 */ MP3_DEC_PAIR_V(14, 14, 2),
  /* 1011 */ MP3_DEC_PAIR_V(14, 14, 2),
  /* 1100 */ MP3_DEC_PAIR_V(14, 13, 3),
  /* 1101 */ MP3_DEC_PAIR_V(14, 13, 3),
  /* 1110 */ MP3_DEC_PAIR_V(14, 11, 3),
  /* 1111 */ MP3_DEC_PAIR_V(14, 11, 3),

  /* 0000 0110 1101 ... */
  /* 000  */ MP3_DEC_PAIR_V(11, 14, 2),	/* 310 */
  /* 001  */ MP3_DEC_PAIR_V(11, 14, 2),
  /* 010  */ MP3_DEC_PAIR_V(12, 13, 2),
  /* 011  */ MP3_DEC_PAIR_V(12, 13, 2),
  /* 100  */ MP3_DEC_PAIR_V(13, 12, 3),
  /* 101  */ MP3_DEC_PAIR_V(13, 11, 3),
  /* 110  */ MP3_DEC_PAIR_V(10, 14, 2),
  /* 111  */ MP3_DEC_PAIR_V(10, 14, 2),

  /* 0000 0110 1110 ... */
  /* 000  */ MP3_DEC_PAIR_V(12, 12, 2),	/* 318 */
  /* 001  */ MP3_DEC_PAIR_V(12, 12, 2),
  /* 010  */ MP3_DEC_PAIR_V(10, 13, 3),
  /* 011  */ MP3_DEC_PAIR_V(13, 10, 3),
  /* 100  */ MP3_DEC_PAIR_V(7, 14, 3),
  /* 101  */ MP3_DEC_PAIR_V(10, 12, 3),
  /* 110  */ MP3_DEC_PAIR_V(12, 10, 2),
  /* 111  */ MP3_DEC_PAIR_V(12, 10, 2),

  /* 0000 0110 1111 ... */
  /* 000  */ MP3_DEC_PAIR_V(12, 9, 3),	/* 326 */
  /* 001  */ MP3_DEC_PAIR_V(7, 13, 3),
  /* 010  */ MP3_DEC_PAIR_V(5, 14, 2),
  /* 011  */ MP3_DEC_PAIR_V(5, 14, 2),
  /* 100  */ MP3_DEC_PAIR_V(11, 13, 1),
  /* 101  */ MP3_DEC_PAIR_V(11, 13, 1),
  /* 110  */ MP3_DEC_PAIR_V(11, 13, 1),
  /* 111  */ MP3_DEC_PAIR_V(11, 13, 1),

  /* 0000 1011 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(9, 14, 1),	/* 334 */
  /* 01   */ MP3_DEC_PAIR_V(9, 14, 1),
  /* 10   */ MP3_DEC_PAIR_V(11, 12, 2),
  /* 11   */ MP3_DEC_PAIR_V(12, 11, 2),

  /* 0000 1011 0001 ... */
  /* 00   */ MP3_DEC_PAIR_V(8, 14, 2),	/* 338 */
  /* 01   */ MP3_DEC_PAIR_V(14, 8, 2),
  /* 10   */ MP3_DEC_PAIR_V(9, 13, 2),
  /* 11   */ MP3_DEC_PAIR_V(14, 7, 2),

  /* 0000 1011 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 11, 2),	/* 342 */
  /* 01   */ MP3_DEC_PAIR_V(8, 13, 2),
  /* 10   */ MP3_DEC_PAIR_V(13, 8, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 14, 2),

  /* 0000 1011 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(14, 6, 1),	/* 346 */
  /* 1    */ MP3_DEC_PAIR_V(9, 12, 1),

  /* 0000 1011 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(10, 11, 2),	/* 348 */
  /* 01   */ MP3_DEC_PAIR_V(11, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(14, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(13, 7, 2),

  /* 0000 1011 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(4, 14, 1),	/* 352 */
  /* 01   */ MP3_DEC_PAIR_V(4, 14, 1),
  /* 10   */ MP3_DEC_PAIR_V(14, 4, 2),
  /* 11   */ MP3_DEC_PAIR_V(8, 12, 2),

  /* 0000 1011 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(12, 8, 1),	/* 356 */
  /* 1    */ MP3_DEC_PAIR_V(3, 14, 1),

  /* 0000 1011 0111 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 13, 1),	/* 358 */
  /* 01   */ MP3_DEC_PAIR_V(6, 13, 1),
  /* 10   */ MP3_DEC_PAIR_V(13, 6, 2),
  /* 11   */ MP3_DEC_PAIR_V(9, 11, 2),

  /* 0000 1011 1000 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 9, 2),	/* 362 */
  /* 01   */ MP3_DEC_PAIR_V(10, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(14, 1, 1),
  /* 11   */ MP3_DEC_PAIR_V(14, 1, 1),

  /* 0000 1011 1001 ... */
  /* 00   */ MP3_DEC_PAIR_V(13, 4, 1),	/* 366 */
  /* 01   */ MP3_DEC_PAIR_V(13, 4, 1),
  /* 10   */ MP3_DEC_PAIR_V(11, 8, 2),
  /* 11   */ MP3_DEC_PAIR_V(10, 9, 2),

  /* 0000 1011 1010 ... */
  /* 00   */ MP3_DEC_PAIR_V(7, 11, 1),	/* 370 */
  /* 01   */ MP3_DEC_PAIR_V(7, 11, 1),
  /* 10   */ MP3_DEC_PAIR_V(11, 7, 2),
  /* 11   */ MP3_DEC_PAIR_V(13, 0, 2),

  /* 0000 1011 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 14, 1),	/* 374 */
  /* 1    */ MP3_DEC_PAIR_V(14, 0, 1),

  /* 0000 1011 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 13, 1),	/* 376 */
  /* 1    */ MP3_DEC_PAIR_V(13, 5, 1),

  /* 0000 1011 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 12, 1),	/* 378 */
  /* 1    */ MP3_DEC_PAIR_V(12, 7, 1),

  /* 0000 1011 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 13, 1),	/* 380 */
  /* 1    */ MP3_DEC_PAIR_V(8, 11, 1),

  /* 0000 1100 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 10, 1),	/* 382 */
  /* 1    */ MP3_DEC_PAIR_V(6, 12, 1),

  /* 0000 1100 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(12, 6, 1),	/* 384 */
  /* 1    */ MP3_DEC_PAIR_V(3, 13, 1),

  /* 0000 1100 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 12, 1),	/* 386 */
  /* 1    */ MP3_DEC_PAIR_V(12, 5, 1),

  /* 0000 1100 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 10, 1),	/* 388 */
  /* 1    */ MP3_DEC_PAIR_V(10, 8, 1),

  /* 0000 1100 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 9, 1),	/* 390 */
  /* 1    */ MP3_DEC_PAIR_V(4, 12, 1),

  /* 0000 1100 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(11, 6, 1),	/* 392 */
  /* 1    */ MP3_DEC_PAIR_V(7, 10, 1),

  /* 0000 1100 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 11, 1),	/* 394 */
  /* 1    */ MP3_DEC_PAIR_V(8, 9, 1),

  /* 0000 1100 1011 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 8, 1),	/* 396 */
  /* 1    */ MP3_DEC_PAIR_V(7, 9, 1),

  /* 0000 1101 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 7, 1),	/* 398 */
  /* 1    */ MP3_DEC_PAIR_V(8, 8, 1),

  /* 0000 0110 1100 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(14, 12, 1),	/* 400 */
  /* 1    */ MP3_DEC_PAIR_V(13, 13, 1)
};

static
union MP3_DEC_HUFFPAIR_U const MP3_DEC_huff_tab24[] = {
  /* 0000 */ MP3_DEC_QUAD_PTR(16, 4),
  /* 0001 */ MP3_DEC_QUAD_PTR(32, 4),
  /* 0010 */ MP3_DEC_QUAD_PTR(48, 4),
  /* 0011 */ MP3_DEC_PAIR_V(15, 15, 4),
  /* 0100 */ MP3_DEC_QUAD_PTR(64, 4),
  /* 0101 */ MP3_DEC_QUAD_PTR(80, 4),
  /* 0110 */ MP3_DEC_QUAD_PTR(96, 4),
  /* 0111 */ MP3_DEC_QUAD_PTR(112, 4),
  /* 1000 */ MP3_DEC_QUAD_PTR(128, 4),
  /* 1001 */ MP3_DEC_QUAD_PTR(144, 4),
  /* 1010 */ MP3_DEC_QUAD_PTR(160, 3),
  /* 1011 */ MP3_DEC_QUAD_PTR(168, 2),
  /* 1100 */ MP3_DEC_PAIR_V(1, 1, 4),
  /* 1101 */ MP3_DEC_PAIR_V(0, 1, 4),
  /* 1110 */ MP3_DEC_PAIR_V(1, 0, 4),
  /* 1111 */ MP3_DEC_PAIR_V(0, 0, 4),

  /* 0000 ... */
  /* 0000 */ MP3_DEC_PAIR_V(14, 15, 4),	/* 16 */
  /* 0001 */ MP3_DEC_PAIR_V(15, 14, 4),
  /* 0010 */ MP3_DEC_PAIR_V(13, 15, 4),
  /* 0011 */ MP3_DEC_PAIR_V(15, 13, 4),
  /* 0100 */ MP3_DEC_PAIR_V(12, 15, 4),
  /* 0101 */ MP3_DEC_PAIR_V(15, 12, 4),
  /* 0110 */ MP3_DEC_PAIR_V(11, 15, 4),
  /* 0111 */ MP3_DEC_PAIR_V(15, 11, 4),
  /* 1000 */ MP3_DEC_PAIR_V(15, 10, 3),
  /* 1001 */ MP3_DEC_PAIR_V(15, 10, 3),
  /* 1010 */ MP3_DEC_PAIR_V(10, 15, 4),
  /* 1011 */ MP3_DEC_PAIR_V(9, 15, 4),
  /* 1100 */ MP3_DEC_PAIR_V(15, 9, 3),
  /* 1101 */ MP3_DEC_PAIR_V(15, 9, 3),
  /* 1110 */ MP3_DEC_PAIR_V(15, 8, 3),
  /* 1111 */ MP3_DEC_PAIR_V(15, 8, 3),

  /* 0001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(8, 15, 4),	/* 32 */
  /* 0001 */ MP3_DEC_PAIR_V(7, 15, 4),
  /* 0010 */ MP3_DEC_PAIR_V(15, 7, 3),
  /* 0011 */ MP3_DEC_PAIR_V(15, 7, 3),
  /* 0100 */ MP3_DEC_PAIR_V(6, 15, 3),
  /* 0101 */ MP3_DEC_PAIR_V(6, 15, 3),
  /* 0110 */ MP3_DEC_PAIR_V(15, 6, 3),
  /* 0111 */ MP3_DEC_PAIR_V(15, 6, 3),
  /* 1000 */ MP3_DEC_PAIR_V(5, 15, 3),
  /* 1001 */ MP3_DEC_PAIR_V(5, 15, 3),
  /* 1010 */ MP3_DEC_PAIR_V(15, 5, 3),
  /* 1011 */ MP3_DEC_PAIR_V(15, 5, 3),
  /* 1100 */ MP3_DEC_PAIR_V(4, 15, 3),
  /* 1101 */ MP3_DEC_PAIR_V(4, 15, 3),
  /* 1110 */ MP3_DEC_PAIR_V(15, 4, 3),
  /* 1111 */ MP3_DEC_PAIR_V(15, 4, 3),

  /* 0010 ... */
  /* 0000 */ MP3_DEC_PAIR_V(3, 15, 3),	/* 48 */
  /* 0001 */ MP3_DEC_PAIR_V(3, 15, 3),
  /* 0010 */ MP3_DEC_PAIR_V(15, 3, 3),
  /* 0011 */ MP3_DEC_PAIR_V(15, 3, 3),
  /* 0100 */ MP3_DEC_PAIR_V(2, 15, 3),
  /* 0101 */ MP3_DEC_PAIR_V(2, 15, 3),
  /* 0110 */ MP3_DEC_PAIR_V(15, 2, 3),
  /* 0111 */ MP3_DEC_PAIR_V(15, 2, 3),
  /* 1000 */ MP3_DEC_PAIR_V(15, 1, 3),
  /* 1001 */ MP3_DEC_PAIR_V(15, 1, 3),
  /* 1010 */ MP3_DEC_PAIR_V(1, 15, 4),
  /* 1011 */ MP3_DEC_PAIR_V(15, 0, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(172, 3),
  /* 1101 */ MP3_DEC_QUAD_PTR(180, 3),
  /* 1110 */ MP3_DEC_QUAD_PTR(188, 3),
  /* 1111 */ MP3_DEC_QUAD_PTR(196, 3),

  /* 0100 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(204, 4),	/* 64 */
  /* 0001 */ MP3_DEC_QUAD_PTR(220, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(228, 3),
  /* 0011 */ MP3_DEC_QUAD_PTR(236, 3),
  /* 0100 */ MP3_DEC_QUAD_PTR(244, 2),
  /* 0101 */ MP3_DEC_QUAD_PTR(248, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(252, 2),
  /* 0111 */ MP3_DEC_QUAD_PTR(256, 2),
  /* 1000 */ MP3_DEC_QUAD_PTR(260, 2),
  /* 1001 */ MP3_DEC_QUAD_PTR(264, 2),
  /* 1010 */ MP3_DEC_QUAD_PTR(268, 2),
  /* 1011 */ MP3_DEC_QUAD_PTR(272, 2),
  /* 1100 */ MP3_DEC_QUAD_PTR(276, 2),
  /* 1101 */ MP3_DEC_QUAD_PTR(280, 3),
  /* 1110 */ MP3_DEC_QUAD_PTR(288, 2),
  /* 1111 */ MP3_DEC_QUAD_PTR(292, 2),

  /* 0101 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(296, 2),	/* 80 */
  /* 0001 */ MP3_DEC_QUAD_PTR(300, 3),
  /* 0010 */ MP3_DEC_QUAD_PTR(308, 2),
  /* 0011 */ MP3_DEC_QUAD_PTR(312, 3),
  /* 0100 */ MP3_DEC_QUAD_PTR(320, 1),
  /* 0101 */ MP3_DEC_QUAD_PTR(322, 2),
  /* 0110 */ MP3_DEC_QUAD_PTR(326, 2),
  /* 0111 */ MP3_DEC_QUAD_PTR(330, 1),
  /* 1000 */ MP3_DEC_QUAD_PTR(332, 2),
  /* 1001 */ MP3_DEC_QUAD_PTR(336, 1),
  /* 1010 */ MP3_DEC_QUAD_PTR(338, 1),
  /* 1011 */ MP3_DEC_QUAD_PTR(340, 1),
  /* 1100 */ MP3_DEC_QUAD_PTR(342, 1),
  /* 1101 */ MP3_DEC_QUAD_PTR(344, 1),
  /* 1110 */ MP3_DEC_QUAD_PTR(346, 1),
  /* 1111 */ MP3_DEC_QUAD_PTR(348, 1),

  /* 0110 ... */
  /* 0000 */ MP3_DEC_QUAD_PTR(350, 1),	/* 96 */
  /* 0001 */ MP3_DEC_QUAD_PTR(352, 1),
  /* 0010 */ MP3_DEC_QUAD_PTR(354, 1),
  /* 0011 */ MP3_DEC_QUAD_PTR(356, 1),
  /* 0100 */ MP3_DEC_QUAD_PTR(358, 1),
  /* 0101 */ MP3_DEC_QUAD_PTR(360, 1),
  /* 0110 */ MP3_DEC_QUAD_PTR(362, 1),
  /* 0111 */ MP3_DEC_QUAD_PTR(364, 1),
  /* 1000 */ MP3_DEC_QUAD_PTR(366, 1),
  /* 1001 */ MP3_DEC_QUAD_PTR(368, 1),
  /* 1010 */ MP3_DEC_QUAD_PTR(370, 2),
  /* 1011 */ MP3_DEC_QUAD_PTR(374, 1),
  /* 1100 */ MP3_DEC_QUAD_PTR(376, 2),
  /* 1101 */ MP3_DEC_PAIR_V(7, 3, 4),
  /* 1110 */ MP3_DEC_QUAD_PTR(380, 1),
  /* 1111 */ MP3_DEC_PAIR_V(7, 2, 4),

  /* 0111 ... */
  /* 0000 */ MP3_DEC_PAIR_V(4, 6, 4),	/* 112 */
  /* 0001 */ MP3_DEC_PAIR_V(6, 4, 4),
  /* 0010 */ MP3_DEC_PAIR_V(5, 5, 4),
  /* 0011 */ MP3_DEC_PAIR_V(7, 1, 4),
  /* 0100 */ MP3_DEC_PAIR_V(3, 6, 4),
  /* 0101 */ MP3_DEC_PAIR_V(6, 3, 4),
  /* 0110 */ MP3_DEC_PAIR_V(4, 5, 4),
  /* 0111 */ MP3_DEC_PAIR_V(5, 4, 4),
  /* 1000 */ MP3_DEC_PAIR_V(2, 6, 4),
  /* 1001 */ MP3_DEC_PAIR_V(6, 2, 4),
  /* 1010 */ MP3_DEC_PAIR_V(1, 6, 4),
  /* 1011 */ MP3_DEC_PAIR_V(6, 1, 4),
  /* 1100 */ MP3_DEC_QUAD_PTR(382, 1),
  /* 1101 */ MP3_DEC_PAIR_V(3, 5, 4),
  /* 1110 */ MP3_DEC_PAIR_V(5, 3, 4),
  /* 1111 */ MP3_DEC_PAIR_V(4, 4, 4),

  /* 1000 ... */
  /* 0000 */ MP3_DEC_PAIR_V(2, 5, 4),	/* 128 */
  /* 0001 */ MP3_DEC_PAIR_V(5, 2, 4),
  /* 0010 */ MP3_DEC_PAIR_V(1, 5, 4),
  /* 0011 */ MP3_DEC_QUAD_PTR(384, 1),
  /* 0100 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 0101 */ MP3_DEC_PAIR_V(5, 1, 3),
  /* 0110 */ MP3_DEC_PAIR_V(3, 4, 4),
  /* 0111 */ MP3_DEC_PAIR_V(4, 3, 4),
  /* 1000 */ MP3_DEC_PAIR_V(2, 4, 3),
  /* 1001 */ MP3_DEC_PAIR_V(2, 4, 3),
  /* 1010 */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 1011 */ MP3_DEC_PAIR_V(4, 2, 3),
  /* 1100 */ MP3_DEC_PAIR_V(3, 3, 3),
  /* 1101 */ MP3_DEC_PAIR_V(3, 3, 3),
  /* 1110 */ MP3_DEC_PAIR_V(1, 4, 3),
  /* 1111 */ MP3_DEC_PAIR_V(1, 4, 3),

  /* 1001 ... */
  /* 0000 */ MP3_DEC_PAIR_V(4, 1, 3),	/* 144 */
  /* 0001 */ MP3_DEC_PAIR_V(4, 1, 3),
  /* 0010 */ MP3_DEC_PAIR_V(0, 4, 4),
  /* 0011 */ MP3_DEC_PAIR_V(4, 0, 4),
  /* 0100 */ MP3_DEC_PAIR_V(2, 3, 3),
  /* 0101 */ MP3_DEC_PAIR_V(2, 3, 3),
  /* 0110 */ MP3_DEC_PAIR_V(3, 2, 3),
  /* 0111 */ MP3_DEC_PAIR_V(3, 2, 3),
  /* 1000 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1001 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1010 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1011 */ MP3_DEC_PAIR_V(1, 3, 2),
  /* 1100 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1101 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1110 */ MP3_DEC_PAIR_V(3, 1, 2),
  /* 1111 */ MP3_DEC_PAIR_V(3, 1, 2),

  /* 1010 ... */
  /* 000  */ MP3_DEC_PAIR_V(0, 3, 3),	/* 160 */
  /* 001  */ MP3_DEC_PAIR_V(3, 0, 3),
  /* 010  */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 011  */ MP3_DEC_PAIR_V(2, 2, 2),
  /* 100  */ MP3_DEC_PAIR_V(1, 2, 1),
  /* 101  */ MP3_DEC_PAIR_V(1, 2, 1),
  /* 110  */ MP3_DEC_PAIR_V(1, 2, 1),
  /* 111  */ MP3_DEC_PAIR_V(1, 2, 1),

  /* 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(2, 1, 1),	/* 168 */
  /* 01   */ MP3_DEC_PAIR_V(2, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 0, 2),

  /* 0010 1100 ... */
  /* 000  */ MP3_DEC_PAIR_V(0, 15, 1),	/* 172 */
  /* 001  */ MP3_DEC_PAIR_V(0, 15, 1),
  /* 010  */ MP3_DEC_PAIR_V(0, 15, 1),
  /* 011  */ MP3_DEC_PAIR_V(0, 15, 1),
  /* 100  */ MP3_DEC_PAIR_V(14, 14, 3),
  /* 101  */ MP3_DEC_PAIR_V(13, 14, 3),
  /* 110  */ MP3_DEC_PAIR_V(14, 13, 3),
  /* 111  */ MP3_DEC_PAIR_V(12, 14, 3),

  /* 0010 1101 ... */
  /* 000  */ MP3_DEC_PAIR_V(14, 12, 3),	/* 180 */
  /* 001  */ MP3_DEC_PAIR_V(13, 13, 3),
  /* 010  */ MP3_DEC_PAIR_V(11, 14, 3),
  /* 011  */ MP3_DEC_PAIR_V(14, 11, 3),
  /* 100  */ MP3_DEC_PAIR_V(12, 13, 3),
  /* 101  */ MP3_DEC_PAIR_V(13, 12, 3),
  /* 110  */ MP3_DEC_PAIR_V(10, 14, 3),
  /* 111  */ MP3_DEC_PAIR_V(14, 10, 3),

  /* 0010 1110 ... */
  /* 000  */ MP3_DEC_PAIR_V(11, 13, 3),	/* 188 */
  /* 001  */ MP3_DEC_PAIR_V(13, 11, 3),
  /* 010  */ MP3_DEC_PAIR_V(12, 12, 3),
  /* 011  */ MP3_DEC_PAIR_V(9, 14, 3),
  /* 100  */ MP3_DEC_PAIR_V(14, 9, 3),
  /* 101  */ MP3_DEC_PAIR_V(10, 13, 3),
  /* 110  */ MP3_DEC_PAIR_V(13, 10, 3),
  /* 111  */ MP3_DEC_PAIR_V(11, 12, 3),

  /* 0010 1111 ... */
  /* 000  */ MP3_DEC_PAIR_V(12, 11, 3),	/* 196 */
  /* 001  */ MP3_DEC_PAIR_V(8, 14, 3),
  /* 010  */ MP3_DEC_PAIR_V(14, 8, 3),
  /* 011  */ MP3_DEC_PAIR_V(9, 13, 3),
  /* 100  */ MP3_DEC_PAIR_V(13, 9, 3),
  /* 101  */ MP3_DEC_PAIR_V(7, 14, 3),
  /* 110  */ MP3_DEC_PAIR_V(14, 7, 3),
  /* 111  */ MP3_DEC_PAIR_V(10, 12, 3),

  /* 0100 0000 ... */
  /* 0000 */ MP3_DEC_PAIR_V(12, 10, 3),	/* 204 */
  /* 0001 */ MP3_DEC_PAIR_V(12, 10, 3),
  /* 0010 */ MP3_DEC_PAIR_V(11, 11, 3),
  /* 0011 */ MP3_DEC_PAIR_V(11, 11, 3),
  /* 0100 */ MP3_DEC_PAIR_V(8, 13, 3),
  /* 0101 */ MP3_DEC_PAIR_V(8, 13, 3),
  /* 0110 */ MP3_DEC_PAIR_V(13, 8, 3),
  /* 0111 */ MP3_DEC_PAIR_V(13, 8, 3),
  /* 1000 */ MP3_DEC_PAIR_V(0, 14, 4),
  /* 1001 */ MP3_DEC_PAIR_V(14, 0, 4),
  /* 1010 */ MP3_DEC_PAIR_V(0, 13, 3),
  /* 1011 */ MP3_DEC_PAIR_V(0, 13, 3),
  /* 1100 */ MP3_DEC_PAIR_V(14, 6, 2),
  /* 1101 */ MP3_DEC_PAIR_V(14, 6, 2),
  /* 1110 */ MP3_DEC_PAIR_V(14, 6, 2),
  /* 1111 */ MP3_DEC_PAIR_V(14, 6, 2),

  /* 0100 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(6, 14, 3),	/* 220 */
  /* 001  */ MP3_DEC_PAIR_V(9, 12, 3),
  /* 010  */ MP3_DEC_PAIR_V(12, 9, 2),
  /* 011  */ MP3_DEC_PAIR_V(12, 9, 2),
  /* 100  */ MP3_DEC_PAIR_V(5, 14, 2),
  /* 101  */ MP3_DEC_PAIR_V(5, 14, 2),
  /* 110  */ MP3_DEC_PAIR_V(11, 10, 2),
  /* 111  */ MP3_DEC_PAIR_V(11, 10, 2),

  /* 0100 0010 ... */
  /* 000  */ MP3_DEC_PAIR_V(14, 5, 2),	/* 228 */
  /* 001  */ MP3_DEC_PAIR_V(14, 5, 2),
  /* 010  */ MP3_DEC_PAIR_V(10, 11, 3),
  /* 011  */ MP3_DEC_PAIR_V(7, 13, 3),
  /* 100  */ MP3_DEC_PAIR_V(13, 7, 2),
  /* 101  */ MP3_DEC_PAIR_V(13, 7, 2),
  /* 110  */ MP3_DEC_PAIR_V(14, 4, 2),
  /* 111  */ MP3_DEC_PAIR_V(14, 4, 2),

  /* 0100 0011 ... */
  /* 000  */ MP3_DEC_PAIR_V(8, 12, 2),	/* 236 */
  /* 001  */ MP3_DEC_PAIR_V(8, 12, 2),
  /* 010  */ MP3_DEC_PAIR_V(12, 8, 2),
  /* 011  */ MP3_DEC_PAIR_V(12, 8, 2),
  /* 100  */ MP3_DEC_PAIR_V(4, 14, 3),
  /* 101  */ MP3_DEC_PAIR_V(2, 14, 3),
  /* 110  */ MP3_DEC_PAIR_V(3, 14, 2),
  /* 111  */ MP3_DEC_PAIR_V(3, 14, 2),

  /* 0100 0100 ... */
  /* 00   */ MP3_DEC_PAIR_V(6, 13, 2),	/* 244 */
  /* 01   */ MP3_DEC_PAIR_V(13, 6, 2),
  /* 10   */ MP3_DEC_PAIR_V(14, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(9, 11, 2),

  /* 0100 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 9, 2),	/* 248 */
  /* 01   */ MP3_DEC_PAIR_V(10, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(14, 2, 2),
  /* 11   */ MP3_DEC_PAIR_V(1, 14, 2),

  /* 0100 0110 ... */
  /* 00   */ MP3_DEC_PAIR_V(14, 1, 2),	/* 252 */
  /* 01   */ MP3_DEC_PAIR_V(5, 13, 2),
  /* 10   */ MP3_DEC_PAIR_V(13, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(7, 12, 2),

  /* 0100 0111 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 7, 2),	/* 256 */
  /* 01   */ MP3_DEC_PAIR_V(4, 13, 2),
  /* 10   */ MP3_DEC_PAIR_V(8, 11, 2),
  /* 11   */ MP3_DEC_PAIR_V(11, 8, 2),

  /* 0100 1000 ... */
  /* 00   */ MP3_DEC_PAIR_V(13, 4, 2),	/* 260 */
  /* 01   */ MP3_DEC_PAIR_V(9, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 9, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 12, 2),

  /* 0100 1001 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 6, 2),	/* 264 */
  /* 01   */ MP3_DEC_PAIR_V(3, 13, 2),
  /* 10   */ MP3_DEC_PAIR_V(13, 3, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 13, 2),

  /* 0100 1010 ... */
  /* 00   */ MP3_DEC_PAIR_V(13, 2, 2),	/* 268 */
  /* 01   */ MP3_DEC_PAIR_V(1, 13, 2),
  /* 10   */ MP3_DEC_PAIR_V(7, 11, 2),
  /* 11   */ MP3_DEC_PAIR_V(11, 7, 2),

  /* 0100 1011 ... */
  /* 00   */ MP3_DEC_PAIR_V(13, 1, 2),	/* 272 */
  /* 01   */ MP3_DEC_PAIR_V(5, 12, 2),
  /* 10   */ MP3_DEC_PAIR_V(12, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(8, 10, 2),

  /* 0100 1100 ... */
  /* 00   */ MP3_DEC_PAIR_V(10, 8, 2),	/* 276 */
  /* 01   */ MP3_DEC_PAIR_V(9, 9, 2),
  /* 10   */ MP3_DEC_PAIR_V(4, 12, 2),
  /* 11   */ MP3_DEC_PAIR_V(12, 4, 2),

  /* 0100 1101 ... */
  /* 000  */ MP3_DEC_PAIR_V(6, 11, 2),	/* 280 */
  /* 001  */ MP3_DEC_PAIR_V(6, 11, 2),
  /* 010  */ MP3_DEC_PAIR_V(11, 6, 2),
  /* 011  */ MP3_DEC_PAIR_V(11, 6, 2),
  /* 100  */ MP3_DEC_PAIR_V(13, 0, 3),
  /* 101  */ MP3_DEC_PAIR_V(0, 12, 3),
  /* 110  */ MP3_DEC_PAIR_V(3, 12, 2),
  /* 111  */ MP3_DEC_PAIR_V(3, 12, 2),

  /* 0100 1110 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 3, 2),	/* 288 */
  /* 01   */ MP3_DEC_PAIR_V(7, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(10, 7, 2),
  /* 11   */ MP3_DEC_PAIR_V(2, 12, 2),

  /* 0100 1111 ... */
  /* 00   */ MP3_DEC_PAIR_V(12, 2, 2),	/* 292 */
  /* 01   */ MP3_DEC_PAIR_V(5, 11, 2),
  /* 10   */ MP3_DEC_PAIR_V(11, 5, 2),
  /* 11   */ MP3_DEC_PAIR_V(1, 12, 2),

  /* 0101 0000 ... */
  /* 00   */ MP3_DEC_PAIR_V(8, 9, 2),	/* 296 */
  /* 01   */ MP3_DEC_PAIR_V(9, 8, 2),
  /* 10   */ MP3_DEC_PAIR_V(12, 1, 2),
  /* 11   */ MP3_DEC_PAIR_V(4, 11, 2),

  /* 0101 0001 ... */
  /* 000  */ MP3_DEC_PAIR_V(12, 0, 3),	/* 300 */
  /* 001  */ MP3_DEC_PAIR_V(0, 11, 3),
  /* 010  */ MP3_DEC_PAIR_V(3, 11, 2),
  /* 011  */ MP3_DEC_PAIR_V(3, 11, 2),
  /* 100  */ MP3_DEC_PAIR_V(11, 0, 3),
  /* 101  */ MP3_DEC_PAIR_V(0, 10, 3),
  /* 110  */ MP3_DEC_PAIR_V(1, 10, 2),
  /* 111  */ MP3_DEC_PAIR_V(1, 10, 2),

  /* 0101 0010 ... */
  /* 00   */ MP3_DEC_PAIR_V(11, 4, 1),	/* 308 */
  /* 01   */ MP3_DEC_PAIR_V(11, 4, 1),
  /* 10   */ MP3_DEC_PAIR_V(6, 10, 2),
  /* 11   */ MP3_DEC_PAIR_V(10, 6, 2),

  /* 0101 0011 ... */
  /* 000  */ MP3_DEC_PAIR_V(7, 9, 2),	/* 312 */
  /* 001  */ MP3_DEC_PAIR_V(7, 9, 2),
  /* 010  */ MP3_DEC_PAIR_V(9, 7, 2),
  /* 011  */ MP3_DEC_PAIR_V(9, 7, 2),
  /* 100  */ MP3_DEC_PAIR_V(10, 0, 3),
  /* 101  */ MP3_DEC_PAIR_V(0, 9, 3),
  /* 110  */ MP3_DEC_PAIR_V(9, 0, 2),
  /* 111  */ MP3_DEC_PAIR_V(9, 0, 2),

  /* 0101 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(11, 3, 1),	/* 320 */
  /* 1    */ MP3_DEC_PAIR_V(8, 8, 1),

  /* 0101 0101 ... */
  /* 00   */ MP3_DEC_PAIR_V(2, 11, 2),	/* 322 */
  /* 01   */ MP3_DEC_PAIR_V(5, 10, 2),
  /* 10   */ MP3_DEC_PAIR_V(11, 2, 1),
  /* 11   */ MP3_DEC_PAIR_V(11, 2, 1),

  /* 0101 0110 ... */
  /* 00   */ MP3_DEC_PAIR_V(10, 5, 2),	/* 326 */
  /* 01   */ MP3_DEC_PAIR_V(1, 11, 2),
  /* 10   */ MP3_DEC_PAIR_V(11, 1, 2),
  /* 11   */ MP3_DEC_PAIR_V(6, 9, 2),

  /* 0101 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(9, 6, 1),	/* 330 */
  /* 1    */ MP3_DEC_PAIR_V(10, 4, 1),

  /* 0101 1000 ... */
  /* 00   */ MP3_DEC_PAIR_V(4, 10, 2),	/* 332 */
  /* 01   */ MP3_DEC_PAIR_V(7, 8, 2),
  /* 10   */ MP3_DEC_PAIR_V(8, 7, 1),
  /* 11   */ MP3_DEC_PAIR_V(8, 7, 1),

  /* 0101 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 10, 1),	/* 336 */
  /* 1    */ MP3_DEC_PAIR_V(10, 3, 1),

  /* 0101 1010 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 9, 1),	/* 338 */
  /* 1    */ MP3_DEC_PAIR_V(9, 5, 1),

  /* 0101 1011 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 10, 1),	/* 340 */
  /* 1    */ MP3_DEC_PAIR_V(10, 2, 1),

  /* 0101 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(10, 1, 1),	/* 342 */
  /* 1    */ MP3_DEC_PAIR_V(6, 8, 1),

  /* 0101 1101 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 6, 1),	/* 344 */
  /* 1    */ MP3_DEC_PAIR_V(7, 7, 1),

  /* 0101 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 9, 1),	/* 346 */
  /* 1    */ MP3_DEC_PAIR_V(9, 4, 1),

  /* 0101 1111 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 9, 1),	/* 348 */
  /* 1    */ MP3_DEC_PAIR_V(9, 3, 1),

  /* 0110 0000 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 8, 1),	/* 350 */
  /* 1    */ MP3_DEC_PAIR_V(8, 5, 1),

  /* 0110 0001 ... */
  /* 0    */ MP3_DEC_PAIR_V(2, 9, 1),	/* 352 */
  /* 1    */ MP3_DEC_PAIR_V(6, 7, 1),

  /* 0110 0010 ... */
  /* 0    */ MP3_DEC_PAIR_V(7, 6, 1),	/* 354 */
  /* 1    */ MP3_DEC_PAIR_V(9, 2, 1),

  /* 0110 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(1, 9, 1),	/* 356 */
  /* 1    */ MP3_DEC_PAIR_V(9, 1, 1),

  /* 0110 0100 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 8, 1),	/* 358 */
  /* 1    */ MP3_DEC_PAIR_V(8, 4, 1),

  /* 0110 0101 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 7, 1),	/* 360 */
  /* 1    */ MP3_DEC_PAIR_V(7, 5, 1),

  /* 0110 0110 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 8, 1),	/* 362 */
  /* 1    */ MP3_DEC_PAIR_V(8, 3, 1),

  /* 0110 0111 ... */
  /* 0    */ MP3_DEC_PAIR_V(6, 6, 1),	/* 364 */
  /* 1    */ MP3_DEC_PAIR_V(2, 8, 1),

  /* 0110 1000 ... */
  /* 0    */ MP3_DEC_PAIR_V(8, 2, 1),	/* 366 */
  /* 1    */ MP3_DEC_PAIR_V(1, 8, 1),

  /* 0110 1001 ... */
  /* 0    */ MP3_DEC_PAIR_V(4, 7, 1),	/* 368 */
  /* 1    */ MP3_DEC_PAIR_V(7, 4, 1),

  /* 0110 1010 ... */
  /* 00   */ MP3_DEC_PAIR_V(8, 1, 1),	/* 370 */
  /* 01   */ MP3_DEC_PAIR_V(8, 1, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 8, 2),
  /* 11   */ MP3_DEC_PAIR_V(8, 0, 2),

  /* 0110 1011 ... */
  /* 0    */ MP3_DEC_PAIR_V(5, 6, 1),	/* 374 */
  /* 1    */ MP3_DEC_PAIR_V(6, 5, 1),

  /* 0110 1100 ... */
  /* 00   */ MP3_DEC_PAIR_V(1, 7, 1),	/* 376 */
  /* 01   */ MP3_DEC_PAIR_V(1, 7, 1),
  /* 10   */ MP3_DEC_PAIR_V(0, 7, 2),
  /* 11   */ MP3_DEC_PAIR_V(7, 0, 2),

  /* 0110 1110 ... */
  /* 0    */ MP3_DEC_PAIR_V(3, 7, 1),	/* 380  */
  /* 1    */ MP3_DEC_PAIR_V(2, 7, 1),

  /* 0111 1100 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 6, 1),	/* 382 */
  /* 1    */ MP3_DEC_PAIR_V(6, 0, 1),

  /* 1000 0011 ... */
  /* 0    */ MP3_DEC_PAIR_V(0, 5, 1),	/* 384 */
  /* 1    */ MP3_DEC_PAIR_V(5, 0, 1)
};


/* external tables */

union MP3_DEC_HUFFQUAD_U const *const mp3_huff_quad_table[2] = { MP3_DEC_huff_tabA, MP3_DEC_huff_tabB };

MP3_DEC_HUFF_TABLE_T const mp3_huff_pair_table[32] = {
  /*  0 */ { MP3_DEC_huff_tab0,   0},
  /*  1 */ { MP3_DEC_huff_tab1,   0}, //
  /*  2 */ { MP3_DEC_huff_tab2,   0}, //
  /*  3 */ { MP3_DEC_huff_tab3,   0}, //
  /*  4 */ { 0 /* not used*/},
  /*  5 */ { MP3_DEC_huff_tab5,   0}, //
  /*  6 */ { MP3_DEC_huff_tab6,   0},
  /*  7 */ { MP3_DEC_huff_tab7,   0},
  /*  8 */ { MP3_DEC_huff_tab8,   0},
  /*  9 */ { MP3_DEC_huff_tab9,   0},
  /* 10 */ { MP3_DEC_huff_tab10,  0},
  /* 11 */ { MP3_DEC_huff_tab11,  0},
  /* 12 */ { MP3_DEC_huff_tab12,  0},
  /* 13 */ { MP3_DEC_huff_tab13,  0},
  /* 14 */ { 0 /* not used*/},
  /* 15 */ { MP3_DEC_huff_tab15,  0},
  /* 16 */ { MP3_DEC_huff_tab16,  1},
  /* 17 */ { MP3_DEC_huff_tab16,  2},
  /* 18 */ { MP3_DEC_huff_tab16,  3},
  /* 19 */ { MP3_DEC_huff_tab16,  4},
  /* 20 */ { MP3_DEC_huff_tab16,  6},
  /* 21 */ { MP3_DEC_huff_tab16,  8},
  /* 22 */ { MP3_DEC_huff_tab16, 10},
  /* 23 */ { MP3_DEC_huff_tab16, 13},
  /* 24 */ { MP3_DEC_huff_tab24,  4},
  /* 25 */ { MP3_DEC_huff_tab24,  5},
  /* 26 */ { MP3_DEC_huff_tab24,  6},
  /* 27 */ { MP3_DEC_huff_tab24,  7},
  /* 28 */ { MP3_DEC_huff_tab24,  8},
  /* 29 */ { MP3_DEC_huff_tab24,  9},
  /* 30 */ { MP3_DEC_huff_tab24, 11},
  /* 31 */ { MP3_DEC_huff_tab24, 13}
};

uint8 const MP3_DEC_pre_tab[22] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0
};


int32 const MP3_DEC_BitmaskTable[] = 
{
    
    0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF,
    0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF,
    0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF,
    0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, -1//0xFFFFFFFF        
};/**/


const int32 MP3_DEC_iq_table[] 
=
{
	0,      2048,	5161, 	8861, 	13004, 	17510, 	22329, 	27424, 
		32768, 	38340, 	44123, 	50102, 	56265, 	62602, 	69104, 	75762, 
		82570, 	89522, 	96611, 	103833, 111183, 118656, 126249, 133958, 
		141779, 149710, 157747, 165888, 174130, 182471, 190908, 199440, 
		208064, 216778, 225581, 234470, 243444, 252502, 261642, 270863, 
		280162, 289540, 298994, 308523, 318127, 327803, 337552, 347371, 
		357260, 367219, 377245, 387338, 397498, 407722, 418012, 428365, 
		438781, 449259, 459798, 470399, 481059, 491779, 502557, 513394, 
		524288, 535239, 546246, 557309, 568428, 579600, 590827, 602108, 
		613442, 624828, 636266, 647756, 659297, 670889, 682532, 694224, 
		705965, 717756, 729595, 741482, 753417, 765400, 777430, 789506, 
		801629, 813798, 826013, 838272, 850577, 862927, 875320, 887758, 
		900240, 912765, 925333, 937944, 950597, 963293, 976031, 988810, 
		1001631, 1014493, 1027396, 1040339, 1053323, 1066347, 1079411, 1092515, 
		1105658, 1118840, 1132061, 1145321, 1158619, 1171955, 1185330, 1198742, 
		1212193, 1225680, 1239205, 1252766, 1266365, 1280000, 1293672, 1307379, 
		1321123, 1334903, 1348718, 1362569, 1376455, 1390376, 1404332, 1418322, 
		1432348, 1446408, 1460502, 1474630, 1488792, 1502988, 1517217, 1531480, 
		1545776, 1560105, 1574468, 1588863, 1603291, 1617751, 1632244, 1646768, 
		1661326, 1675915, 1690535, 1705188, 1719872, 1734587, 1749334, 1764112, 
		1778921, 1793760, 1808631, 1823532, 1838464, 1853426, 1868418, 1883441, 
		1898493, 1913575, 1928687, 1943829, 1959001, 1974201, 1989431, 2004691, 
		2019979, 2035296, 2050643, 2066017, 2081421, 2096853, 2112314, 2127803, 
		2143320, 2158866, 2174439, 2190040, 2205669, 2221326, 2237011, 2252723, 
		2268462, 2284229, 2300023, 2315845, 2331693, 2347568, 2363471, 2379400, 
		2395355, 2411338, 2427347, 2443382, 2459443, 2475531, 2491645, 2507786, 
		2523952, 2540144, 2556362, 2572606, 2588875, 2605170, 2621491, 2637837, 
		2654208, 2670605, 2687026, 2703473, 2719945, 2736442, 2752964, 2769511, 
		2786083, 2802679, 2819299, 2835945, 2852614, 2869309, 2886027, 2902770, 
		2919537, 2936328, 2953143, 2969982, 2986845, 3003731, 3020642, 3037576, 
		3054534, 3071515, 3088520, 3105549, 3122600, 3139675, 3156774, 3173895, 
		3191040, 3208207, 3225398, 3242611, 3259848, 3277107, 3294389, 3311694, 3329021
};


/*
* fractional powers of two
* used for requantization and joint stereo decoding
*
* root_table[3 + x] = 2^(x/4)
*/

int32 const MP3_DEC_root_table[7] = {
	MP3_F(0x09837f05) /* 2^(-3/4) == 0.59460355750136 */,
		MP3_F(0x0b504f33) /* 2^(-2/4) == 0.70710678118655 */,
		MP3_F(0x0d744fcd) /* 2^(-1/4) == 0.84089641525371 */,
		MP3_F(0x10000000) /* 2^( 0/4) == 1.00000000000000 */,
		MP3_F(0x1306fe0a) /* 2^(+1/4) == 1.18920711500272 */,
		MP3_F(0x16a09e66) /* 2^(+2/4) == 1.41421356237310 */,
		MP3_F(0x1ae89f99) /* 2^(+3/4) == 1.68179283050743 */
};
