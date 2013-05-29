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
#include "sc8810_video_header.h"
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
#if defined(REAL_DEC)
	int i, uv;
	uint32 v1, v2, w;
	uint8 * pPixF, *pPixP;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
//int iRatio0 = g_mbc_reg_ptr->RV_RATIO_CTRL & 0x7fff; @ Simon, 20110530
//int iRatio1 = (g_mbc_reg_ptr->RV_RATIO_CTRL >>16) & 0x7fff;
	int iRatio1 = g_mbc_reg_ptr->RV_RATIO_CTRL & 0x7fff;
	int iRatio0 = (g_mbc_reg_ptr->RV_RATIO_CTRL >>16) & 0x7fff;

	if (iRatio0 != iRatio1)
	{
		/*Y*/
		pPixP = fw_mca_bfr;
		pPixF = bw_mca_bfr;
		for (i = 0; i < 256; i++)
		{
			v1 = pPixF[i] << 7;
			v2 = pPixP[i] << 7;

			w = (v1*iRatio0 >> 16) + (v2* iRatio1 >> 16);

			pPixP [i] = (uint8)((w + 0x10) >> 5);
		}

		/*UV*/
		for (uv = 0; uv < 2; uv++)
		{
			pPixP = (uv == 0) ? (fw_mca_bfr + (MB_SIZE*MB_SIZE)) : (fw_mca_bfr + (MB_SIZE*MB_SIZE) + (BLOCK_SIZE*BLOCK_SIZE));
			pPixF = (uv == 0) ? (bw_mca_bfr + (MB_SIZE*MB_SIZE)) : (bw_mca_bfr + (MB_SIZE*MB_SIZE) + (BLOCK_SIZE*BLOCK_SIZE));

			for (i = 0; i < 64; i++)
			{
				v1 = pPixF[i] << 7;
				v2 = pPixP[i] << 7;
				
				w = (v1*iRatio0 >> 16) + (v2* iRatio1 >> 16);
				
				pPixP [i] = (uint8)((w + 0x10) >> 5);
			}
		}
	}
	else
	{
		/*Y*/
		pPixP = fw_mca_bfr;
		pPixF = bw_mca_bfr;
		for (i = 0; i < 16; i++)
		{
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;

			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;			
		}
		
		/*UV*/
		for (uv = 0; uv < 2; uv++)
		{
			pPixP = (uv == 0) ? (fw_mca_bfr + (MB_SIZE*MB_SIZE)) : (fw_mca_bfr + (MB_SIZE*MB_SIZE) + (BLOCK_SIZE*BLOCK_SIZE));
			pPixF = (uv == 0) ? (bw_mca_bfr + (MB_SIZE*MB_SIZE)) : (bw_mca_bfr + (MB_SIZE*MB_SIZE) + (BLOCK_SIZE*BLOCK_SIZE));
			for (i = 0; i < 8; i++)
			{
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
				*pPixP++ = (*pPixF++ + *pPixP + 1) >> 1;
			}
		}
	}
#else
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
#endif
}

void h264_MC_GetAverage(int32 b8)
{
	int i;
	uint8 *pPred0, *pPred1;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;

	//for 8x8 block mc
	int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
	int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

	/*for Y*/
	pPred0 = fw_mca_bfr + b8_luma_offset[b8];
	pPred1 = bw_mca_bfr + b8_luma_offset[b8];
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

		pPred0 += 8;
		pPred1 += 8;
	}

	fw_mca_bfr += (MB_SIZE*MB_SIZE);
	bw_mca_bfr += (MB_SIZE*MB_SIZE);

	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 0; i < 4; i++)
	{
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;

		pPred0 += 4;
		pPred1 += 4;
	}

	fw_mca_bfr += (BLOCK_SIZE*BLOCK_SIZE);
	bw_mca_bfr += (BLOCK_SIZE*BLOCK_SIZE);

	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 0; i < 4; i++)
	{
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;
		*pPred0++ = (*pPred0 + (*pPred1++) + 1) / 2;

		pPred0 += 4;
		pPred1 += 4;
	}
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
