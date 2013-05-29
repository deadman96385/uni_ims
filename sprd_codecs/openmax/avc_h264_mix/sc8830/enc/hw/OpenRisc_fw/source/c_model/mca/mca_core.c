/******************************************************************************
 ** File Name:    mca_core.c												  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc6800x_video_header.h"
#include "hmea_mode.h"
#include "hmea_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

extern int32 s_MvRound16[16];
extern int32 s_MvRound4[4];

void Mp4_mc_xyfull_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	/*x and y mv is full pixel*/
	x_cor = start_x;
	y_cor = start_y;		

	for (j = MB_SIZE; j != 0; j--)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			
		for (i = 0; i < MB_SIZE; i++)
		{
			x = IClip(x_min, x_max, x_cor);
			pRec_mb[i] = pframe_ref[y*width + x];
			x_cor++;
		}

		y_cor++;
		pRec_mb += MB_SIZE;
	}
}

void Mp4_mc_xhalfyfull_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;	

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	/*x is half pixel, and y is full pixel*/ 
	x_cor = start_x;
	y_cor = start_y;		

	for (j = MB_SIZE; j != 0; j--)
	{		
		x_cor = start_x;
		y = IClip(y_min,y_max,y_cor);
		for (i = 0; i < MB_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			p0 = pframe_ref[y*width + x];

			x = IClip (x_min, x_max, x_cor+1);
			p1 = pframe_ref[y*width + x];

			pRec_mb[i] = (p0 + p1 + RndCtrl) >>1;	
					
			x_cor++;
		}

		y_cor++;		
		pRec_mb += MB_SIZE;
	}
}

void Mp4_mc_xfullyhalf_16x16(uint8 *pframe_ref, uint8 *pRec_mb,	int32 rounding_control, 
				int32 width, int32 height, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;	

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	/*y is half pixel, and x is full pixel*/
	x_cor = start_x;
	y_cor = start_y;		

	for (j = 0; j < MB_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < MB_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
					
			y = IClip(y_min,y_max, y_cor);
			p0 = pframe_ref[y*width + x];

			y = IClip(y_min, y_max, y_cor+1);
			p1 = pframe_ref[y*width + x];

			pRec_mb[i] = (p0 + p1 + RndCtrl)>>1; 

			x_cor++;
		}

		y_cor++;
		pRec_mb += MB_SIZE;
	}
}

void Mp4_mc_xyhalf_16x16(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3;
	int32 RndCtrl = 2 - rounding_control;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	/*both x and y are half pixel*/
	x_cor = start_x;
	y_cor = start_y;		

	for (j = 0; j < MB_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < MB_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			y = IClip(y_min,y_max,y_cor);
			p0 = pframe_ref[y*width + x];
					
			x = IClip(x_min,x_max,x_cor+1);
			p1 = pframe_ref[y*width + x];
					
			x = IClip (x_min, x_max, x_cor);
			y = IClip (y_min, y_max, y_cor + 1); 
			p2 = pframe_ref[y*width + x];
					
			x = IClip (x_min, x_max, x_cor+1);
			p3 = pframe_ref[y*width + x];
					
			pRec_mb [i] = (p0 + p1 + p2 + p3 + RndCtrl)>>2; 
					
			x_cor++;
		}
				
		y_cor++;
		pRec_mb += MB_SIZE;				
	}
}

/***********************************************************
motion compensation for 8x8 block; 1: 4mv for y, and for uv 
input: 
		xRef yRef: absolute coordinate in reference frame
		ref_frame: point to reference address in reference frame
		pPred:     store the reference block
		width:     width of reference frame
output:
		pPred:     store the reference block
************************************************************/
void Mp4_mc_xyfull_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64; //offset is 64 in MBPIX structure.

	if(dstWidth == BLOCK_SIZE)
	{
		is_uv = TRUE; //pRec_mb is U address.
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;		
		
	for (j = BLOCK_SIZE; j != 0; j--)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min, x_max, x_cor);

			if(is_uv)
			{
				pRec_mb[i] = pframe_ref[y*width*2 + 2*x]; 
				pRecV[i] = pframe_ref[y*width*2 + 2*x+1]; 
			}else
			{
				pRec_mb[i] = pframe_ref[y*width + x];
			}
			x_cor++;
		}

		y_cor++;

		pRec_mb += dstWidth;

		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xfullyhalf_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v;

	if(dstWidth == BLOCK_SIZE)
	{
		is_uv = TRUE;
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = 0; j < BLOCK_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
					
			y = IClip(y_min,y_max, y_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}

			y = IClip(y_min, y_max, y_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}

			pRec_mb[i] = (p0 + p1 + RndCtrl)>>1; 
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + RndCtrl)>>1; 
			}

			x_cor++;
		}

		y_cor++;
		pRec_mb += dstWidth;
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xhalfyfull_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;	
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v;

	if(dstWidth == BLOCK_SIZE)
	{
		is_uv = TRUE;
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = BLOCK_SIZE; j != 0; j--)
	{		
		x_cor = start_x;
		y = IClip(y_min,y_max,y_cor);
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}

			x = IClip (x_min, x_max, x_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}

			pRec_mb[i] = (p0 + p1 + RndCtrl) >>1;	
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + RndCtrl)>>1; 
			}
			
			x_cor++;
		}
		y_cor++;		
		pRec_mb += dstWidth;
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xyhalf_8x8(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3;
	int32 RndCtrl = 2 - rounding_control;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v, p2_v, p3_v;

	if(dstWidth == BLOCK_SIZE)
	{
		is_uv = TRUE;
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = 0; j < BLOCK_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			
			y = IClip(y_min,y_max,y_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}
			
			x = IClip(x_min,x_max,x_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}
					
			x = IClip (x_min, x_max, x_cor);
			y = IClip (y_min, y_max, y_cor + 1); 
			if(is_uv)
			{
				p2 = pframe_ref[y*width*2 + 2*x];	
				p2_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p2 = pframe_ref[y*width + x];
			}
					
			x = IClip (x_min, x_max, x_cor+1);
			if(is_uv)
			{
				p3 = pframe_ref[y*width*2 + 2*x];	
				p3_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p3 = pframe_ref[y*width + x];
			}
					
			pRec_mb[i] = (p0 + p1 + p2 + p3 + RndCtrl)>>2; 
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + p2_v + p3_v+ RndCtrl)>>2; 
			}
					
			x_cor++;
		}
				
		y_cor++;
		pRec_mb += dstWidth;	
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

/***********************************************************
motion compensation for 8x8 block; 1: 4mv for y, and for uv 
input: 
		xRef yRef: absolute coordinate in reference frame
		ref_frame: point to reference address in reference frame
		pPred:     store the reference block
		width:     width of reference frame
output:
		pPred:     store the reference block
************************************************************/
void Mp4_mc_xyfull_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64; //offset is 64 in MBPIX structure.

	if(dstWidth == BLOCK_SIZE)
	{
//		is_uv = TRUE; //pRec_mb is U address.
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;		
		
	for (j = BLOCK_SIZE; j != 0; j--)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min, x_max, x_cor);

			if(is_uv)
			{
				pRec_mb[i] = pframe_ref[y*width*2 + 2*x]; 
				pRecV[i] = pframe_ref[y*width*2 + 2*x+1]; 
			}else
			{
				pRec_mb[i] = pframe_ref[y*width + x];
			}
			x_cor++;
		}

		y_cor++;

		pRec_mb += dstWidth;

		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xfullyhalf_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v;

	if(dstWidth == BLOCK_SIZE)
	{
//		is_uv = TRUE;
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = 0; j < BLOCK_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
					
			y = IClip(y_min,y_max, y_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}

			y = IClip(y_min, y_max, y_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}

			pRec_mb[i] = (p0 + p1 + RndCtrl)>>1; 
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + RndCtrl)>>1; 
			}

			x_cor++;
		}

		y_cor++;
		pRec_mb += dstWidth;
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xhalfyfull_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control, 
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1;
	int32 RndCtrl = 1 - rounding_control;	
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v;

	if(dstWidth == BLOCK_SIZE)
	{
//		is_uv = TRUE;
	}

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = BLOCK_SIZE; j != 0; j--)
	{		
		x_cor = start_x;
		y = IClip(y_min,y_max,y_cor);
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}

			x = IClip (x_min, x_max, x_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}

			pRec_mb[i] = (p0 + p1 + RndCtrl) >>1;	
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + RndCtrl)>>1; 
			}
			
			x_cor++;
		}
		y_cor++;		
		pRec_mb += dstWidth;
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}

void Mp4_mc_xyhalf_8x8_me(uint8 *pframe_ref, uint8 *pRec_mb, int32 rounding_control,
				int32 width, int32 height,  int32 dstWidth, int32 start_x, int32 start_y)
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3;
	int32 RndCtrl = 2 - rounding_control;
	int32 is_uv = FALSE;
	uint8 *pRecV = pRec_mb + 64;
	int32 p0_v, p1_v, p2_v, p3_v;

	if(dstWidth == BLOCK_SIZE)
	{
//		is_uv = TRUE;
	}
	
	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	
	x_cor = start_x;
	y_cor = start_y;			

	for (j = 0; j < BLOCK_SIZE; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			
			y = IClip(y_min,y_max,y_cor);
			if(is_uv)
			{
				p0 = pframe_ref[y*width*2 + 2*x];	
				p0_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p0 = pframe_ref[y*width + x];
			}
			
			x = IClip(x_min,x_max,x_cor+1);
			if(is_uv)
			{
				p1 = pframe_ref[y*width*2 + 2*x];	
				p1_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p1 = pframe_ref[y*width + x];
			}
					
			x = IClip (x_min, x_max, x_cor);
			y = IClip (y_min, y_max, y_cor + 1); 
			if(is_uv)
			{
				p2 = pframe_ref[y*width*2 + 2*x];	
				p2_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p2 = pframe_ref[y*width + x];
			}
					
			x = IClip (x_min, x_max, x_cor+1);
			if(is_uv)
			{
				p3 = pframe_ref[y*width*2 + 2*x];	
				p3_v = pframe_ref[y*width*2 + 2*x+1];	
			}else
			{
				p3 = pframe_ref[y*width + x];
			}
					
			pRec_mb[i] = (p0 + p1 + p2 + p3 + RndCtrl)>>2; 
			if(is_uv)
			{
				pRecV[i] = (p0_v + p1_v + p2_v + p3_v+ RndCtrl)>>2; 
			}
					
			x_cor++;
		}
				
		y_cor++;
		pRec_mb += dstWidth;	
		if(is_uv)
		{
			pRecV += dstWidth; 
		}
	}
}


void Mp4Dec_MC_GetAverage()
{
	int i;
	uint8 *pPred0, *pPred1;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;

	/*for Y*/
	pPred0 = fw_mca_bfr;
	pPred1 = bw_mca_bfr;
	for (i = 0; i < 16; i++)
	{
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;

		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;    
	}

	fw_mca_bfr += (MB_SIZE*MB_SIZE);
	bw_mca_bfr += (MB_SIZE*MB_SIZE);

	pPred0 = fw_mca_bfr;
	pPred1 = bw_mca_bfr;
	for (i = 0; i < 8; i++)
	{
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
	}

	fw_mca_bfr += (BLOCK_SIZE*BLOCK_SIZE);
	bw_mca_bfr += (BLOCK_SIZE*BLOCK_SIZE);

	pPred0 = fw_mca_bfr;
	pPred1 = bw_mca_bfr;
	for (i = 0; i < 8; i++)
	{
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
	}
}

#ifdef MPEG4_ENC
void Mp4_Interpolation (MODE_DECISION_T * mode_dcs_ptr)
{
	int		mv_x;
	int		mv_y;
	int		mvc_x;
	int		mvc_y;
	int		dmvcx = 0;
	int		dmvcy = 0;
	int		start_x;
	int		start_y;
	int		dx;
	int		dy;
	int		x_offset;
	int		y_offset;
	int		blk_id;
	int		frame_width = g_mb_num_x * MB_SIZE;
	int		frame_height = g_mb_num_y * MB_SIZE;
	int		mb_x = mode_dcs_ptr->mb_x;
	int		mb_y = mode_dcs_ptr->mb_y;
	int		rounding_control = mode_dcs_ptr->rnd_ctr;
	uint8 * pRefFrmY = (uint8 *)(g_ref_frame[0]);
	uint8 * pRefFrmUV = (uint8 *)g_ref_frame[1];
	uint8 * mca_out_ptr;

	/*interpolation for Y*/
	if (mode_dcs_ptr->mb_type == INTER_MB_16X16)
	{
		mca_out_ptr = (uint8 *)vsp_mea_out_bfr;
		
		mv_x = mode_dcs_ptr->blk_mv[0].x >> 1;			//quarter pixel resolution to half pixel
		mv_y = mode_dcs_ptr->blk_mv[0].y >> 1;

		start_x = mb_x * MB_SIZE + (mv_x >> 1);
		start_y = mb_y * MB_SIZE + (mv_y >> 1);
		
		dx = mv_x & 1;
		dy = mv_y & 1;

		g_dec_mc_16x16[(dx<<1) | dy](pRefFrmY, mca_out_ptr, rounding_control, frame_width, frame_height, start_x, start_y);	
		
		mvc_x = (mv_x >> 1) + s_MvRound4[mv_x & 0x3];
		mvc_y = (mv_y >> 1) + s_MvRound4[mv_y & 0x3];
	}
	else
	{
		for (blk_id = 0; blk_id < 4; blk_id++)
		{
			x_offset = (blk_id & 1) * 8;
			y_offset = (blk_id >> 1)*8;
			
			mca_out_ptr = ((uint8 *)vsp_mea_out_bfr) + y_offset*MB_SIZE + x_offset;

			mv_x = mode_dcs_ptr->blk_mv[blk_id].x >> 1;	
			mv_y = mode_dcs_ptr->blk_mv[blk_id].y >> 1;

			start_x = mb_x * MB_SIZE + x_offset + (mv_x >> 1);
			start_y = mb_y * MB_SIZE + y_offset + (mv_y >> 1);

			dx = mv_x & 1;
			dy = mv_y & 1;

			g_dec_mc_8x8[(dx<<1) | dy](pRefFrmY, mca_out_ptr, rounding_control, frame_width, frame_height, MB_SIZE, start_x, start_y);	
			
			dmvcx += mv_x;
			dmvcy += mv_y;
		}

		mvc_x = (dmvcx >> 3) + s_MvRound16[dmvcx & 0xf];
		mvc_y = (dmvcy >> 3) + s_MvRound16[dmvcy & 0xf];
	}

	/*interpolation for UV*/
	start_x = mb_x * BLOCK_SIZE + (mvc_x>>1);
	start_y = mb_y * BLOCK_SIZE + (mvc_y>>1);
	
	dx = mvc_x & 1;
	dy = mvc_y & 1;
	
	mca_out_ptr = (uint8 *)vsp_mea_out_bfr + MB_SIZE*MB_SIZE;
	g_dec_mc_8x8[(dx<<1) | dy](pRefFrmUV, mca_out_ptr, rounding_control, frame_width/2, frame_height/2, BLOCK_SIZE, start_x, start_y);
	
}
#endif


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
