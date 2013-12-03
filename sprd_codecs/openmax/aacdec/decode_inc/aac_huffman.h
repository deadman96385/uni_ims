/*************************************************************************
** File Name:      huffman.h                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is for AAC huffamn decoder
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
#define AAC_DEC_HUFF_ADDR(offs, bits)	{ { 0, bits, offs } }

#if 1//def AAC_DEC_BIG_ENDIAN

#define AAC_DEC_HUFF_VALUE(w, x, y, z, hlen)	{ { 1, hlen, (w << 6) | (x << 4) |  \
(y <<  2) | (z <<  0) } }
#else

#define AAC_DEC_HUFF_VALUE(w, x, y, z, hlen)	{ { 1, hlen, (w <<  0) | (x <<  2) |  \
(y <<  4) | (z <<  6) } }
#endif




#define AAC_DEC_SINGLE_VALUE(x, hlen)	{ { 1, hlen, x } }



union AAC_DEC_HUFFSINGLE_U 
{
    struct {
        uint16 final  :  1;
        uint16 bits   :  3;
        uint16 offset : 12;
    } ptr;

    struct {
        uint16 final  :  1;
        uint16 hlen   :  3;
        uint16 x      :  12;
    } value;

        uint16 final  :  1;
};





union AAC_DEC_HUFF_QUAD {
    struct {
        uint16 final  :  1;
        uint16 bits   :  3;
        uint16 offset : 12;
    } addr;
    struct {
        uint16 final  :  1;
        uint16 cblen  :  3;
        uint16 w      :  2;
        uint16 x      :  2;
        uint16 y      :  2;
        uint16 z      :  2;
    } value;
    uint16 final    :  1;
};



union AAC_DEC_HUFF_PAIR {
    struct {
        uint16 final  :  1;
        uint16 bits   :  3;
        uint16 offset : 12;
    } addr;
    struct {
        uint16 final  :  1;
        uint16 cblen  :  3;
        uint16 x      :  6;
        uint16 y      :  6;
    } value;
    uint16 final    :  1;
};



	
                            
extern union AAC_DEC_HUFF_QUAD const AAC_DEC_HuffCb1[176];                                 
extern union AAC_DEC_HUFF_QUAD const AAC_DEC_HuffCb2[162];                                 
extern union AAC_DEC_HUFF_QUAD const AAC_DEC_HuffCb3[138];
extern union AAC_DEC_HUFF_QUAD const AAC_DEC_HuffCb4[138]; 
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb5[122];
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb6[140];
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb7[112];     
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb8[114];
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb9[230];
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb10[248];
extern union AAC_DEC_HUFF_PAIR const AAC_DEC_HuffCb11[462];



                                                                                         
#ifdef __cplusplus
}
#endif
#endif
