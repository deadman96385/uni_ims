#include "sc8810_video_header.h"

#define INTERP_FACTOR 4
#define INTERP_FACTOR_RV8 3
#define INTERP_ROUND  1

void RvDec_MC_Luma_H00V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			pPred[j] = pFrm_ref[y*uRefWidth + x];

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/3 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (-1,12,6,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((-p0 + 12*p1 + 6*p2 - p3 + 8) >> 4));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/3 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (-1,6,12,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((-p0 + 6*p1 + 12*p2 - p3 + 8) >> 4));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 horizontal displacement */
/*	1/3 vertical displacement */
/*	Use vertical filter (-1,12,6,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth)  
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			y = IClip(y_min, y_max, y_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((-p0 + 12*p1 + 6*p2 - p3 + 8) >> 4));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/3 pel horizontal displacement */
/*	1/3 vertical displacement */
/*	Use horizontal filter (-1,12,6,-1) */
/*	Use vertical filter (-1,12,6,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			y = IClip(y_min, y_max, y_cor-1);
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			sum = (-p0 + 12*p1 + 6*p2 - p3) * (-1);

			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * 12;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * 6;

			y = IClip(y_min, y_max, y_cor+2);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * (-1);

			sum += 128;	sum >>= 8;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H01V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/3 pel horizontal displacement */
/*	1/3 vertical displacement */
/*	Use horizontal filter (-1,6,12,-1) */
/*	Use vertical filter (-1,12,6,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth)  
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			y = IClip(y_min, y_max, y_cor-1);
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			sum = (-p0 + 6*p1 + 12*p2 - p3) * (-1);

			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 6*p1 + 12*p2 - p3;	sum += tmp * 12;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 6*p1 + 12*p2 - p3;	sum += tmp * 6;

			y = IClip(y_min, y_max, y_cor+2);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 6*p1 + 12*p2 - p3;	sum += tmp * (-1);

			sum += 128;	sum >>= 8;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H02V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 horizontal displacement */
/*	2/3 vertical displacement */
/*	Use vertical filter (-1,6,12,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 
//	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			y = IClip(y_min, y_max, y_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((-p0 + 6*p1 + 12*p2 - p3 + 8) >> 4));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H00V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (-1,12,6,-1) */
/*	Use vertical filter (-1,6,12,-1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			y = IClip(y_min, y_max, y_cor-1);
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			sum = (-p0 + 12*p1 + 6*p2 - p3) * (-1);

			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * 6;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * 12;

			y = IClip(y_min, y_max, y_cor+2);	
			x = IClip(x_min, x_max, x_cor-1);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p3 = pFrm_ref[y*uRefWidth + x];
			tmp = -p0 + 12*p1 + 6*p2 - p3;	sum += tmp * (-1);

			sum += 128;	sum >>= 8;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H01V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (6,9,1) */
/*	Use vertical filter (6,9,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x;
	y_cor = start_y;		

	for(i = 0; i < uBlkWidth; i++)
	{
		x_cor = start_x;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < uBlkWidth; j++)
		{
			y = IClip(y_min, y_max, y_cor);
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p2 = pFrm_ref[y*uRefWidth + x];
			sum = 36*p0 + 54*p1 + 6*p2;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p2 = pFrm_ref[y*uRefWidth + x];
			tmp = 6*p0 + 9*p1 + 1*p2;	sum += tmp * 9;

			y = IClip(y_min, y_max, y_cor+2);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p2 = pFrm_ref[y*uRefWidth + x];
			tmp = 6*p0 + 9*p1 + 1*p2;	sum += tmp;

			sum += 128;	sum >>= 8;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H02V02 */

/* chroma functions */
/* Block size is 4x4 for all. */

/*////////////////////////////////////////////////////////////////////////////// */
/*	 0 horizontal displacement */
/*	 0 vertical displacement */
/*	 No interpolation required, simple block copy. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			pPred[j] = pFrm_ref[y*uRefWidth + x];

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H00V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/3 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (5,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((5*p0 + 3*p1 + 4) >> 3));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H01V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/3 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (3,5) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V00_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((3*p0 + 5*p1 + 4) >> 3));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H02V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 pel horizontal displacement */
/*	1/3 vertical displacement */
/*	Use vertical filter (5,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth)  
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			y = IClip(y_min, y_max, y_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p1 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((5*p0 + 3*p1 + 4) >> 3));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H00V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use vertical filter (3,5) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			x = IClip(x_min, x_max, x_cor);
			y = IClip(y_min, y_max, y_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p1 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((3*p0 + 5*p1 + 4) >> 3));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H00V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/3 pel horizontal displacement */
/*	1/3 vertical displacement */
/*	Use horizontal filter (5,3) */
/*	Use vertical filter (5,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			sum = (5*p0 + 3*p1)*5;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			tmp = 5*p0 + 3*p1;	sum += tmp * 3;

			sum += 32;	sum >>= 6;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H01V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/3 pel horizontal displacement */
/*	1/3 vertical displacement */
/*	Use horizontal filter (3,5) */
/*	Use vertical filter (5,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V01_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			sum = (3*p0 + 5*p1)*5;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			tmp = 3*p0 + 5*p1;	sum += tmp * 3;

			sum += 32;	sum >>= 6;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H02V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	1/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (5,3) */
/*	Use vertical filter (3,5) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			sum = (5*p0 + 3*p1)*3;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			tmp = 5*p0 + 3*p1;	sum += tmp * 5;

			sum += 32;	sum >>= 6;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H01V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	2/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (3,5) */
/*	Use vertical filter (3,5) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V02_R8(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1; 
	int32 sum, tmp;

	x_min = 0; x_max = uRefWidth - 2;
	y_min = 0; y_max = uRefHeight - 1;
	
	x_cor = start_x * 2;
	y_cor = start_y;		

	for(i = 0; i < 4; i++)
	{
		x_cor = start_x * 2;
		y = IClip(y_min, y_max, y_cor);			

		for(j = 0; j < 4; j++)
		{
			y = IClip(y_min, y_max, y_cor);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			sum = (3*p0 + 5*p1)*3;

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			tmp = 3*p0 + 5*p1;	sum += tmp * 5;

			sum += 32;	sum >>= 6;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H02V02 */




