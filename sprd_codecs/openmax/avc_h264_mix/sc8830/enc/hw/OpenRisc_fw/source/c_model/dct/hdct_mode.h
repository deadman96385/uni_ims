#ifndef _HDCT_MODE_H_
#define _HDCT_MODE_H_

/************************************************************************/
/* 
	DCT/IO buffer organization for H264 dct coefficient
	coefficient:
	blk0~blk23 4x4 AC coefficient: [0, 191], one block uses 8 word
	luma 4x4 DC coefficient: [192, 199]
	U 2x2 DC coefficient: [200, 201]
	V 2x2 DC coefficient: [202: 203]
	
	non_zero coefficient flag: 
	blk0~blk23: [204, 227], one block uses one word
	luma 4x4 DC: 228
	U 2x2 DC: 229
	V 2x2 DC: 230
                                                                     */
/************************************************************************/
#define HDCT_COEFF_LUMA_AC_BASE		0
#define HDCT_COEFF_CHROMA_AC_BASE	128
#define HDCT_COEFF_LUMA_DC_BASE		192
#define HDCT_COEFF_CHROMA_DC_BASE	200
//#define COEFF_CR_DC_BASE		202

#endif //_HDCT_MODE_H_