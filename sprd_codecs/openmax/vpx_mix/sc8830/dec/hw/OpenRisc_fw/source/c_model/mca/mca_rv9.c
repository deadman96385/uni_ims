#include "sc8810_video_header.h"
#if 1
/*////////////////////////////////////////////////////////////////////////////// */
/*	 0 horizontal displacement */
/*	 0 vertical displacement */
/*	 No interpolation required, simple block copy. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
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
}	/* H00V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (1, -5, 52, 20, -5, 1) / 64 */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5; 

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
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((52*p2 + 20*p3 - 5*(p1 + p4) + p0 + p5 + 32) >> 6));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H01V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (1, -5, 20, 20, -5, 1) / 32 */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5; 

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
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255,((20*(p2 + p3) - 5*(p1 + p4) + p0 + p5 + 16) >> 5));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H02V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (1, -5, 20, 52, -5, 1) / 64 */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H03V00_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5; 

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
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255,((20*p2 + 52*p3 - 5*(p1 + p4) + p0 + p5 + 32) >> 6));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H03V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 horizontal displacement */
/*	1/3 vertical displacement */
/*	Use vertical filter (1, -5, 52, 20, -5, 1) / 64 */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth)  
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5; 

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
			y = IClip(y_min, y_max, y_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((p0 - 5*p1 + 52*p2 + 20*p3 -5*p4 + p5+ 32) >> 6));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H00V01 */

//common function, x != 0, y!= 0;
void RvDec_MC_Luma_HxVy_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth, int32 *pHCoeff, int32 *pVCoeff) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5;  
	int32 sum, tmp;
	int32 iHShift, iVshift;

	sum = (pHCoeff[0] + pHCoeff[1] + pHCoeff[2] + pHCoeff[3] + pHCoeff[4] + pHCoeff[5]);
	if(sum == 32)
	{
		iHShift = 5;
	}else if(sum == 64)
	{
		iHShift = 6;
	}else
	{
		PRINTF("ERROR!");
	}

	sum = (pVCoeff[0] + pVCoeff[1] + pVCoeff[2] + pVCoeff[3] + pVCoeff[4] + pVCoeff[5]);
	if(sum == 32)
	{
		iVshift = 5;
	}else if(sum == 64)
	{
		iVshift = 6;
	}else
	{
		PRINTF("ERROR!");
	}


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
			y = IClip(y_min, y_max, y_cor-2);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			sum = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum = sum * pVCoeff[0];

			y = IClip(y_min, y_max, y_cor-1);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			tmp = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum += tmp * pVCoeff[1];

			y = IClip(y_min, y_max, y_cor);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			tmp = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum += tmp * pVCoeff[2];

			y = IClip(y_min, y_max, y_cor+1);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			tmp = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum += tmp * pVCoeff[3];

			y = IClip(y_min, y_max, y_cor+2);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			tmp = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum += tmp * pVCoeff[4];

			y = IClip(y_min, y_max, y_cor+3);
			x = IClip(x_min, x_max, x_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];
			tmp = (uint8)IClip(0, 255, ((pHCoeff[0]*p0 + pHCoeff[1]*p1 + pHCoeff[2]*p2 + pHCoeff[3]*p3 + pHCoeff[4]*p4 + pHCoeff[5]*p5)+(1<<(iHShift-1)))>>iHShift);
			sum += tmp * pVCoeff[5];

			sum += (1<<(iVshift-1));	sum >>= iVshift;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}
/*////////////////////////////////////////////////////////////////////////////// */
/*	1/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (1,-5,52,20,-5,1) */
/*	Use vertical filter (1,-5,52,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1, -5, 52, 20, -5, 1};
	int32 iVCoeff[6] = {1,-5, 52, 20, -5, 1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);
}/* H01V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (1,-5,20,20,-5,1) */
/*	Use vertical filter (1,-5,52,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,20,20,-5,1};
	int32 iVCoeff[6] = {1,-5,52,20,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H02V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	3/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (1,-5,20,52,-5,1) */
/*	Use vertical filter (1,-5,52,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H03V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,20,52,-5,1};
	int32 iVCoeff[6] = {1,-5,52,20,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H03V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 horizontal displacement */
/*	2/4 vertical displacement */
/*	Use vertical filter (1,-5,20,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5;  

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
			y = IClip(y_min, y_max, y_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((p0 - 5*p1 + 20*p2  + 20*p3 -5*p4 + p5 + 16) >> 5));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H00V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (1,-5,52,20,-5,1) */
/*	Use vertical filter (1,-5,20,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,52,20,-5,1};
	int32 iVCoeff[6] = {1,-5,20,20,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H02V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (1,-5,20,20,-5,1) */
/*	Use vertical filter (1,-5,20,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,20,20,-5,1};
	int32 iVCoeff[6] = {1,-5,20,20,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}	/* H02V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	3/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (1,-5,20,52,-5,1) */
/*	Use vertical filter (1,-5,20,20,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H03V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,20,52,-5,1};
	int32 iVCoeff[6] = {1,-5,20,20,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H03V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	0 horizontal displacement */
/*	3/4 vertical displacement */
/*	Use vertical filter (1,-5,20,52,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H00V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	uint32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3, p4, p5;  

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
			y = IClip(y_min, y_max, y_cor-2);	p0 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor-1);	p1 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor);		p2 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+1);	p3 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+2);	p4 = pFrm_ref[y*uRefWidth + x];
			y = IClip(y_min, y_max, y_cor+3);	p5 = pFrm_ref[y*uRefWidth + x];

			pPred[j] = (uint8)IClip(0, 255, ((p0 - 5*p1 + 20*p2  + 52*p3 -5*p4 + p5 + 32) >> 6));

			x_cor++;
		}
		
		y_cor++;
		pPred += MB_SIZE;
	}
}/* H00V03 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	1/4 pel horizontal displacement */
/*	3/4 vertical displacement */
/*	Use horizontal filter (1,-5,52,20,-5,1) */
/*	Use vertical filter (1,-5,20,52,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H01V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,52,20,-5,1};
	int32 iVCoeff[6] = {1,-5,20,52,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H01V03 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	2/4 pel horizontal displacement */
/*	3/4 vertical displacement */
/*	Use horizontal filter (1,-5,20,20,-5,1) */
/*	Use vertical filter (1,-5,20,52,-5,1) */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H02V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[6] = {1,-5,20,20,-5,1};
	int32 iVCoeff[6] = {1,-5,20,52,-5,1};
	
	RvDec_MC_Luma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight, start_x, start_y, uBlkWidth, iHCoeff, iVCoeff);

}/* H02V03 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	3/4 pel horizontal displacement */
/*	3/4 vertical displacement */
/*	Use horizontal filter (-1,5,20,52,5,-1) */
/*	Use vertical filter (-1,5,20,52,5,-1) */
//Notes: It is only use simple average algorithm. not above coefficent array.
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Luma_H03V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 i,j;
	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_cor,y_cor;
	int32 x,y;
	int32 p0, p1, p2, p3;

	x_min = 0; x_max = uRefWidth - 1;
	y_min = 0; y_max = uRefHeight - 1;
	
	/*both x and y are half pixel*/
	x_cor = start_x;
	y_cor = start_y;		

	for (j = 0; j < (int32)uBlkWidth; j++)
	{	
		x_cor = start_x;
		for (i = 0; i < (int32)uBlkWidth; i++)
		{
			x = IClip(x_min,x_max,x_cor);
			y = IClip(y_min,y_max,y_cor);
			p0 = pFrm_ref[y*uRefWidth + x];
					
			x = IClip(x_min,x_max,x_cor+1);
			p1 = pFrm_ref[y*uRefWidth + x];
					
			x = IClip (x_min, x_max, x_cor);
			y = IClip (y_min, y_max, y_cor + 1); 
			p2 = pFrm_ref[y*uRefWidth + x];
					
			x = IClip (x_min, x_max, x_cor+1);
			p3 = pFrm_ref[y*uRefWidth + x];
					
			pPred [i] = (p0 + p1 + p2 + p3 + 2)>>2; 
					
			x_cor++;
		}
				
		y_cor++;
		pPred += MB_SIZE;				
	}
}/* H03V03 */

/* chroma functions */
/* Block size is 4x4 for all. */

/*////////////////////////////////////////////////////////////////////////////// */
/* C_MCCopyChroma_H00V00 */
/* */
/*	 0 horizontal displacement */
/*	 0 vertical displacement */
/*	 No interpolation required, simple block copy. */
/* */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V00_R9(uint8 *pFrm_ref, uint8 *pPred4x4, uint32 uRefWidth, uint32 uRefHeight, 
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
			pPred4x4[j] = pFrm_ref[y*uRefWidth + x];

			x_cor+=2;
		}
		
		y_cor++;
		pPred4x4 += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma_H00V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	1/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (3,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V00_R9(uint8 *pFrm_ref, uint8 *pPred4x4, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred4x4[j] = (uint8)IClip(0, 255, ((3*p0 + 1*p1 + 1) >> 2));

			x_cor+=2;
		}
		
		y_cor++;
		pPred4x4 += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H01V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	2/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V00_R9(uint8 *pFrm_ref, uint8 *pPred4x4, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred4x4[j] = (uint8)IClip(0, 255, ((p0 + p1 + 1) >> 1));

			x_cor+=2;
		}
		
		y_cor++;
		pPred4x4 += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H02V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	3/4 pel horizontal displacement */
/*	0 vertical displacement */
/*	Use horizontal filter (1,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H03V00_R9(uint8 *pFrm_ref, uint8 *pPred4x4, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred4x4[j] = (uint8)IClip(0, 255, ((p0 + 3*p1 + 1) >> 2));

			x_cor+=2;
		}
		
		y_cor++;
		pPred4x4 += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H03V00 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	0 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use vertical filter (3,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred[j] = (uint8)IClip(0, 255, ((3*p0 + 1*p1 + 2) >> 2));

			x_cor+= 2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H00V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	0 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use vertical filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred[j] = (uint8)IClip(0, 255, ((p0 + p1) >> 1));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H00V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	0 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use vertical filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H00V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
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

			pPred[j] = (uint8)IClip(0, 255, ((p0 + 3*p1 + 2) >> 2));

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}/* C_MCCopyChroma4_H00V03 */

//common function, x != 0, y!= 0;
void RvDec_MC_Chroma_HxVy_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth, int32 *pHCoeff, int32 *pVCoeff, int32 iShiftBits, int32 iShiftOffset ) 
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
			sum = (pHCoeff[0]*p0 + pHCoeff[1]*p1)*pVCoeff[0];

			y = IClip(y_min, y_max, y_cor+1);	
			x = IClip(x_min, x_max, x_cor);		p0 = pFrm_ref[y*uRefWidth + x];
			x = IClip(x_min, x_max, x_cor+2);	p1 = pFrm_ref[y*uRefWidth + x];
			tmp = (pHCoeff[0]*p0 + pHCoeff[1]*p1);	sum += tmp * pVCoeff[1];

			sum += iShiftOffset;	sum >>= iShiftBits;
			pPred[j] = (uint8)IClip(0, 255, sum);

			x_cor+=2;
		}
		
		y_cor++;
		pPred += MB_CHROMA_SIZE;
	}
}

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated chroma 4x4 block copy. */
/*	1/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (3,1) */
/*	Use vertical filter (3,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {3, 1};
	int32 iVCoeff[2] = {3, 1};
	int32 iShiftBits = 4;
	int32 iShiftOffset = 7;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H01V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	2/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (1,1) */
/*	Use vertical filter (3,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 1};
	int32 iVCoeff[2] = {3, 1};
	int32 iShiftBits = 3;
	int32 iShiftOffset = 4;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H02V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	3/4 pel horizontal displacement */
/*	1/4 vertical displacement */
/*	Use horizontal filter (1,3) */
/*	Use vertical filter (3,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H03V01_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 3};
	int32 iVCoeff[2] = {3, 1};
	int32 iShiftBits = 4;
	int32 iShiftOffset = 7;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H03V01 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	1/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (3,1) */
/*	Use vertical filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {3, 1};
	int32 iVCoeff[2] = {1, 1};
	int32 iShiftBits = 3;
	int32 iShiftOffset = 4;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}	/* C_MCCopyChroma4_H01V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	2/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (1,1) */
/*	Use vertical filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 1};
	int32 iVCoeff[2] = {1, 1};
	int32 iShiftBits = 2;
	int32 iShiftOffset = 1;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H02V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	3/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (1,3) */
/*	Use vertical filter (1,1) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H03V02_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 3};
	int32 iVCoeff[2] = {1, 1};
	int32 iShiftBits = 3;
	int32 iShiftOffset = 4;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H01V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	1/4 pel horizontal displacement */
/*	3/4 vertical displacement */
/*	Use horizontal filter (3,1) */
/*	Use vertical filter (1,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H01V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {3, 1};
	int32 iVCoeff[2] = {1, 3};
	int32 iShiftBits = 4;
	int32 iShiftOffset = 7;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H01V02 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	2/3 pel horizontal displacement */
/*	2/3 vertical displacement */
/*	Use horizontal filter (1,1) */
/*	Use vertical filter (1,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H02V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 1};
	int32 iVCoeff[2] = {1, 3};
	int32 iShiftBits = 3;
	int32 iShiftOffset = 4;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H02V03 */

/*////////////////////////////////////////////////////////////////////////////// */
/*	Motion compensated 4x4 chroma block copy. */
/*	3/4 pel horizontal displacement */
/*	2/4 vertical displacement */
/*	Use horizontal filter (1,3) */
/*	Use vertical filter (1,3) */
/*	Dst pitch is DEST_PITCH. */
/*////////////////////////////////////////////////////////////////////////////// */
void RvDec_MC_Chroma_H03V03_R9(uint8 *pFrm_ref, uint8 *pPred, uint32 uRefWidth, uint32 uRefHeight, 
							  int32 start_x, int32 start_y, uint32 uBlkWidth) 
{
	int32 iHCoeff[2] = {1, 3};
	int32 iVCoeff[2] = {1, 3};
	int32 iShiftBits = 4;
	int32 iShiftOffset = 7;
	
	RvDec_MC_Chroma_HxVy_R9(pFrm_ref, pPred, uRefWidth, uRefHeight,  start_x, start_y, uBlkWidth, iHCoeff, iVCoeff, iShiftBits, iShiftOffset ); 
}/* C_MCCopyChroma4_H03V03 */
#endif //0
