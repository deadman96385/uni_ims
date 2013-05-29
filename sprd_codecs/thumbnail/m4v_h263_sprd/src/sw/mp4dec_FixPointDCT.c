/******************************************************************************
 ** File Name:    mp4dec_fixpointdct.c	                                      *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/*dct  idct*/
//#include "mp4dec_basic.h"
#include "mp4dec_global.h"
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//#define FIXED(a)     (a*(1<<15))
//#define ROUND(x) ((x)+(1<<14))>>15
//#define NORMAL(x)  x//((x)+(1<<1))>>2   

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

#ifndef INTRA_IDCT_ASSEMBLY
void IDCTcFix16(int16 *block, uint8*rgchDst, int32 nWidthDst)
{
	int32 i;
	int16 *blk;
	int16*pDst;
	uint8 *blkDstC;
	int32 x0, x1, x2, x3, x4, x5, x6, x7, x8;
	int16 block_tmp[64];

	for(i=0; i<8; i++)
	{
		int32 _x4, _x2, _x6;
		
		blk  = block + i;
		pDst = block_tmp + 8 * i;		
		
		x2 = blk[8*6];
		x3 = blk[8*2];
		x4 = blk[8*1];
		x5 = blk[8*7];
		x6 = blk[8*5];
		x7 = blk[8*3];		
			
		/* first stage */
		_x4 = W7*x5 + W1*x4;
		x5 = W7*x4 - W1*x5;
		_x6 = W3*x7 +W5*x6;
		x7 = W3*x6 - W5*x7;
			
		/* second stage */
		_x2 = W6*x3 - W2*x2;
		x3 = W6*x2 + W2*x3;
			
		x0 = (blk[8*0]<<11) + 128; /* for proper rounding in the fourth stage */
		x1 = blk[8*4]<<11;
		x8 = x0 + x1;
		x0 = x0 - x1;
		x1 = _x4 + _x6;
		x4 = _x4 - _x6;
		x6 = x5 + x7;
		x5 = x5 - x7;
			
		/* third stage */
		x7 = x8 + x3;
		x8 = x8 - x3;
		x3 = x0 + _x2;
		x0 = x0 - _x2;
		x2 = (181*(x4+x5)+128)>>8;
		x4 = (181*(x4-x5)+128)>>8;
			
		/* fourth stage */
		pDst[1]  = (x3+x2)>>8;
		pDst[6]  = (x3-x2)>>8;
		pDst[2]  = (x0+x4)>>8;
		pDst[5]  = (x0-x4)>>8;
		pDst[0] = (x7+x1)>>8;
		pDst[7]  = (x7-x1)>>8;			
		pDst[3]  = (x8+x6)>>8;
		pDst[4]  = (x8-x6)>>8;
	}
	
	for(i=0; i<8; i++)
	{
		register int32 offset = 0;
		int32 _x4, _x2, _x6;
		
		blk = block_tmp + i;
		blkDstC = rgchDst + i;		
		
		x2 = blk[8*6];  
		x3 = blk[8*2];
		x4 = blk[8*1];  
		x5 = blk[8*7];
		x6 = blk[8*5];  
		x7 = blk[8*3];
		
		/* first stage */
		_x4 = (W7*x5 + W1*x4+4)>>3;
		x5 = (W7*x4 - W1*x5+4)>>3;
		_x6 = (W3*x7 +W5*x6+4)>>3;
		x7 = (W3*x6 - W5*x7+4)>>3;
				
		/* second stage */
		_x2 = (W6*x3 - W2*x2+4)>>3;
		x3 = (W6*x2 + W2*x3+4)>>3;

		x0 = (blk[8*0]<<8) + 8192;
		x1 = (blk[8*4]<<8);
		x8 = x0 + x1;
		x0 = x0 - x1;
		x1 = _x4 + _x6;
		x4 = _x4 - _x6;
		x6 = x5 + x7;
		x5 = x5 - x7;
		
		/* third stage */
		x7 = x8 + x3;
		x8 = x8 - x3;
		x3 = x0 + _x2;
		x0 = x0 - _x2;
		x2 = (181*(x4+x5)+128)>>8;
		x4 = (181*(x4-x5)+128)>>8;
		
		/* fourth stage */	
		blkDstC[offset]  = IClip(0, 255, (x7+x1)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x3+x2)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x0+x4)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x8+x6)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x8-x6)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x0-x4)>>14);  offset += nWidthDst;		
		blkDstC[offset]  = IClip(0, 255, (x3-x2)>>14);  offset += nWidthDst;
		blkDstC[offset]  = IClip(0, 255, (x7-x1)>>14);  offset += nWidthDst; 	
	}
}
#endif

#ifndef INTER_IDCT_ASSEMBLY
void IDCTiFix(int16 *block, uint8 *rgchDst, int32 nWidthDst, uint8 *ref, int32 ref_width)
{
	int32 i;
	int16 *blk;
	int16 *pDst;
	uint8 *blkDstI;
	uint8 *blkRef;
	int32 x0, x1, x2, x3, x4, x5, x6, x7, x8;
	int16 block_tmp[64];

	for(i=0; i<8; i++)
	{
		int32 _x4, _x2, _x6;
		
		blk  = block + i;
		pDst = block_tmp + (8*i);		
		
		x2 = blk[8*6];
		x3 = blk[8*2];
		x4 = blk[8*1];
		x5 = blk[8*7];
		x6 = blk[8*5];
		x7 = blk[8*3];		
			
		/* first stage */
		_x4 = W7*x5 + W1*x4;
		x5 = W7*x4 - W1*x5;
		_x6 = W3*x7 +W5*x6;
		x7 = W3*x6 - W5*x7;
			
		/* second stage */
		_x2 = W6*x3 - W2*x2;
		x3 = W6*x2 + W2*x3;
			
		x0 = (blk[8*0]<<11) + 128; /* for proper rounding in the fourth stage */
		x1 = blk[8*4]<<11;
		x8 = x0 + x1;
		x0 = x0 - x1;
		x1 = _x4 + _x6;
		x4 = _x4 - _x6;
		x6 = x5 + x7;
		x5 = x5 - x7;
			
		/* third stage */
		x7 = x8 + x3;
		x8 = x8 - x3;
		x3 = x0 + _x2;
		x0 = x0 - _x2;
		x2 = (181*(x4+x5)+128)>>8;
		x4 = (181*(x4-x5)+128)>>8;
			
		/* fourth stage */
		pDst[1]  = (x3+x2)>>8;
		pDst[6]  = (x3-x2)>>8;
		pDst[2]  = (x0+x4)>>8;
		pDst[5]  = (x0-x4)>>8;
		pDst[0] = (x7+x1)>>8;
		pDst[7]  = (x7-x1)>>8;			
		pDst[3]  = (x8+x6)>>8;
		pDst[4]  = (x8-x6)>>8;
	}
	
	for(i=0; i<8; i++)
	{
		int32 offset = 0;
		int32 offset_ref = 0;
		int32 _x4, _x2, _x6;
		
		blk = block_tmp + i;
		blkDstI= rgchDst + i;		
		blkRef = ref + i;
		
		x2 = blk[8*6];  
		x3 = blk[8*2];
		x4 = blk[8*1];  
		x5 = blk[8*7];
		x6 = blk[8*5];  
		x7 = blk[8*3];
		
		/* first stage */
		_x4 = (W7*x5 + W1*x4+4)>>3;
		x5 = (W7*x4 - W1*x5+4)>>3;
		_x6 = (W3*x7 +W5*x6+4)>>3;
		x7 = (W3*x6 - W5*x7+4)>>3;
				
		/* second stage */
		_x2 = (W6*x3 - W2*x2+4)>>3;
		x3 = (W6*x2 + W2*x3+4)>>3;

		x0 = (blk[8*0]<<8) + 8192;
		x1 = (blk[8*4]<<8);
		x8 = x0 + x1;
		x0 = x0 - x1;
		x1 = _x4 + _x6;
		x4 = _x4 - _x6;
		x6 = x5 + x7;
		x5 = x5 - x7;
		
		/* third stage */
		x7 = x8 + x3;
		x8 = x8 - x3;
		x3 = x0 + _x2;
		x0 = x0 - _x2;
		x2 = (181*(x4+x5)+128)>>8;
		x4 = (181*(x4-x5)+128)>>8;
		
		/* fourth stage */
		blkDstI[offset] = IClip(0, 255, ((x7+x1)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x3+x2)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x0+x4)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x8+x6)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x8-x6)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x0-x4)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x3-x2)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
		blkDstI[offset] = IClip(0, 255, ((x7-x1)>>14) + blkRef[offset_ref]); offset += nWidthDst; offset_ref += ref_width;
	}
}
#endif //INTER_IDCT_ASSEMBLY

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
 
