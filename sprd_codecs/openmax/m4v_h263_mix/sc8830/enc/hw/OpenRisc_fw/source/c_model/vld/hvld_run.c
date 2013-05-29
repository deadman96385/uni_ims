/*hvld_run.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "bsm_global.h"

extern int	g_run_coeff_reg[15];

/*register definition*/
static int g_index_reg; 
static int g_zeros_left_reg;

int GetTotalZeros (
				   int		total_coeff, 
				   int		blk_type, 
				   int *	total_zeros_ptr
				   )
{	
	uint32	bsm_run_data;
	int		total_zeros;
	int		code_len;
	int		total_zeros_err = 0;

	bsm_run_data = show_nbits (32);
	
	/*total zero*/
	if (blk_type == CHROMA_DC)
	{
		if (total_coeff == 1)
		{
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else if ((bsm_run_data >> 30) & 1)
			{
				total_zeros = 1;
				code_len = 2;
			}
			else if ((bsm_run_data >> 29) & 1)
			{
				total_zeros = 2;
				code_len = 3;
			}
			else
			{
				total_zeros = 3;
				code_len = 3;
			}
		}
		else if (total_coeff == 2)
		{			
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else if ((bsm_run_data >> 30) & 1)
			{
				total_zeros = 1;
				code_len = 2;
			}
			else
			{
				total_zeros = 2;
				code_len = 2;
			}
		}
		else
		{			
			if ((bsm_run_data >> 31) & 1)
			{
				total_zeros = 0;
				code_len = 1;
			}
			else
			{
				total_zeros = 1;
				code_len = 1;
			}
		}		
	}
	else
	{
		/*for block not be chroma_dc*/
		 if (total_coeff < 8)
		 {
			 if (total_coeff < 4)
			 {
				 if (total_coeff < 2)    //total_coeff: 1
				 {
					 if (bsm_run_data & 0xf0000000)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 0;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 2;
							 code_len = 3;							 
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 3 : 4;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 5 : 6;
							 code_len = 5;
						 }
					 }
					 else
					 {
						 if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 7 : 8;
							 code_len = 6;							 
						 }
						 else if (bsm_run_data & 0x04000000)
						 {
							 total_zeros = (bsm_run_data & 0x02000000) ? 9 : 10;
							 code_len = 7;
						 }
						 else if (bsm_run_data & 0x02000000)
						 {
							 total_zeros = (bsm_run_data & 0x01000000) ? 11 : 12;
							 code_len = 8;
						 }
						 else if (bsm_run_data & 0x01000000)
						 {
							 total_zeros = (bsm_run_data & 0x00800000) ? 13 : 14;
							 code_len = 9;
						 }
						 else if (bsm_run_data & 0x00800000)
						 {
							 total_zeros = 15;
							 code_len = 9;
						 }
						 else
						 {
							 total_zeros = 0;
							 code_len = 0;
							 total_zeros_err = 1;
						 }						 
					 }
				 }
				 else
				 {
					 if (total_coeff == 2)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;

							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 0 : 1;							
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 4;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 5 : 6;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 7 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 10;	
							 code_len = 5;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 11 : 12;	
							 code_len = 6;
						 }
						 else
						 {							 
							 total_zeros = (bsm_run_data & 0x04000000) ? 13 : 14;	
							 code_len = 6;
						 }
					 }
					 else		//total_coeff: 3
					 {
						 if (bsm_run_data & 0x80000000)
						 {							 
							 code_len = 3;
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 2;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 7;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 0 : 4;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 5 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 10;	
							 code_len = 5;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 12;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 11 : 13;
							 code_len = 6;							 
						 }
					 }
				 }
			 }
			 else
			 {
				 if (total_coeff < 6)
				 {
					 if (total_coeff == 4)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;

							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 4;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 8;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 2 : 3;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 7 : 9;	
							 code_len = 4;							 
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 0 : 10;	
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 11 : 12;	
							 code_len = 5;
						 }						 
					 }
					 else    //total_coeff: 5
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 4;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 6;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 if (bsm_run_data & 0x20000000)
							 {
								 total_zeros = 7;
								 code_len = 3;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x10000000) ? 0 : 1;	
								 code_len = 4;
							 }
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 2 : 8;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 10;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 9 : 11;	
							 code_len = 5;							 
						 }
					 }
				 }
				 else
				 {
					 if (total_coeff == 6)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 code_len = 3;
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 4 : 5;	
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 6 : 7;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 9;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 8;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 1;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 10;	
							 code_len = 6;
						 }
					 }
					 else   //total_coeff: 7
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 
							 if (bsm_run_data & 0x40000000)
							 {
								 total_zeros = 5;	
								 code_len = 2;
							 }
							 else
							 {
								 total_zeros = (bsm_run_data & 0x20000000) ? 2 : 3;	
								 code_len = 3;
							 }
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 4 : 6;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 8;	
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 7;	
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 1;	
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 9;	
							 code_len = 6;							 
						 }	
					 }
				 }
			 }
		 }
		 else
		 {
			 if (total_coeff < 12)
			 {
				 if (total_coeff < 10)
				 {
					 if (total_coeff == 8)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 4 : 5;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 3 : 6;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 7;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 1;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 2;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 8;	
							 code_len = 6;
						 }						 
					 }
					 else   //total coeff: 9
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 3 : 4;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 6;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 5;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 2;
							 code_len = 4;
						 }
						 else if (bsm_run_data & 0x08000000)
						 {
							 total_zeros = 7;
							 code_len = 5;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x04000000) ? 0 : 1;	
							 code_len = 6;
						 }
					 }
				 }
				 else
				 {
					 if (total_coeff == 10)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 3 : 4;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 5;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 2;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x10000000)
						 {
							 total_zeros = 6;
							 code_len = 4;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x08000000) ? 0 : 1;	
							 code_len = 5;
						 }
					 }
					 else  //total_coeff: 11
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 4;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 5 : 3;
							 code_len = 3;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 2;
							 code_len = 3;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 1 : 0;
							 code_len = 4;
						 }						 
					 }
				 }
			 }
			 else
			 {
				 if (total_coeff < 14)
				 {
					 if (total_coeff == 12)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 3;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 2;
							 code_len = 2;
						 }
						 else if (bsm_run_data & 0x20000000)
						 {
							 total_zeros = 4;
							 code_len = 3;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x10000000) ? 1 : 0;
							 code_len = 4;
						 }
					 }
					 else   //total_coeff: 13
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 2;
							 code_len = 1;
						 }
						 else if (bsm_run_data & 0x40000000)
						 {
							 total_zeros = 3;
							 code_len = 2;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x20000000) ? 1 : 0;
							 code_len = 3;
						 }						 
					 }
				 }
				 else
				 {
					 if (total_coeff == 14)
					 {
						 if (bsm_run_data & 0x80000000)
						 {
							 total_zeros = 2;
							 code_len = 1;
						 }
						 else
						 {
							 total_zeros = (bsm_run_data & 0x40000000) ? 1 : 0;
							 code_len = 2;
						 }
					 }
					 else   //total_coeff: 15
					 {
						 total_zeros = (bsm_run_data & 0x80000000) ? 1 : 0;
						 code_len = 1;
					 }
				 }
			 }
		 }
	}

	flush_nbits(code_len);

//	PrintfTotalZero (blk_type, bsm_run_data, total_coeff, total_zeros, code_len);

	*total_zeros_ptr = total_zeros;

	return total_zeros_err;
}

int ReadRunBefor (int zeros_left, int * run_before_ptr)
{
	int     code_len;
	int		run_before;
	int		error = 0;
	uint32  bsm_run_data;

	bsm_run_data = show_nbits (32);

	if (zeros_left < 5)
	{
		if (zeros_left == 1)
		{
			code_len = 1;
			run_before = (bsm_run_data & 0x80000000) ? 0 : 1;
		}
		else if (zeros_left == 2)
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 1;
				run_before = 0;
			}
			else
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 1 : 2;
			}
		}
		else if (zeros_left == 3)
		{
			code_len = 2;

			if (bsm_run_data & 0x80000000)
			{
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else
			{
				run_before = (bsm_run_data & 0x40000000) ? 2 : 3;	
			}
		}
		else	//zero_lefe 4
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 2;
				run_before = 2;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 3 : 4;
			}			
		}
	}
	else
	{
		if (zeros_left == 5)
		{
			if (bsm_run_data & 0x80000000)
			{
				code_len = 2;
				run_before = (bsm_run_data & 0x40000000) ? 0 : 1;				
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 2 : 3;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 4 : 5;
			}
		}
		else if (zeros_left == 6)
		{
			if (bsm_run_data & 0x80000000)
			{
				if (bsm_run_data & 0x40000000)
				{
					code_len = 2;
					run_before = 0;
				}
				else
				{
					code_len = 3;
					run_before = (bsm_run_data & 0x20000000) ? 5 : 6;
				}
			}
			else if (bsm_run_data & 0x40000000)
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 3 : 4;
			}
			else
			{
				code_len = 3;
				run_before = (bsm_run_data & 0x20000000) ? 2 : 1;
			}
		}
		else
		{
			if (bsm_run_data & 0xf0000000)
			{
				if (bsm_run_data & 0x80000000)
				{
					if (bsm_run_data & 0x40000000)
					{
						code_len = 3;
						run_before = (bsm_run_data & 0x20000000) ? 0 : 1;
					}
					else
					{
						code_len = 3;
						run_before = (bsm_run_data & 0x20000000) ? 2 : 3;
					}
				}
				else if (bsm_run_data & 0x40000000)
				{
					code_len = 3;
					run_before = (bsm_run_data & 0x20000000) ? 4 : 5;
				}
				else if (bsm_run_data & 0x20000000)
				{
					code_len = 3;
					run_before = 6;
				}
				else
				{
					code_len = 4;
					run_before = 7;
				}
			}
			else
			{
				if (bsm_run_data & 0x08000000)
				{
					code_len = 5;
					run_before = 8;
				}
				else if (bsm_run_data & 0x04000000)
				{
					code_len = 6;
					run_before = 9;
				}
				else if (bsm_run_data & 0x02000000)
				{
					code_len = 7;
					run_before = 10;
				}
				else if (bsm_run_data & 0x01000000)
				{
					code_len = 8;
					run_before = 11;
				}
				else if (bsm_run_data & 0x00800000)
				{
					code_len = 9;
					run_before = 12;
				}
				else if (bsm_run_data & 0x00400000)
				{
					code_len = 10;
					run_before = 13;
				}
				else if (bsm_run_data & 0x00200000)
				{
					code_len = 11;
					run_before = 14;
				}
				else
				{
					code_len = 0;
					run_before = 0;
					error = 1;					
				}
			}
		}
	}

	flush_nbits(code_len);

	*run_before_ptr = run_before;

//	PrintfRunBefore (bsm_run_data, zeros_left, run_before, code_len);

	return error;	
}

int H264VldRunDec (
				   int	total_coeff,
				   int	blk_type,
				   int	max_coeff_num
				   )
{
	int		total_zeros = 0;	
	int		run_befor;
	int		error = 0;
	int		run_num = 0;

	if (total_coeff < max_coeff_num)
	{
		error = GetTotalZeros (total_coeff, blk_type, &total_zeros);
	}

	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		VLD_FPRINTF (g_hvld_trace_fp, "total_zeros: %d\n", total_zeros);
	}

	if (error)
		return 1;

	/*run before each coefficient are stored in register in hvld_blk module*/
	g_zeros_left_reg = total_zeros;
	g_index_reg		 = total_coeff - 1;
	
	while ((g_zeros_left_reg != 0) && (g_index_reg != 0))
	{
		if (run_num == 0)
			printf ("");

		error = ReadRunBefor (g_zeros_left_reg, &run_befor);

		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			VLD_FPRINTF (g_hvld_trace_fp, "run: %d\n", run_befor);
		}
		run_num++;		

		if (error)
			return 1;
		
		g_zeros_left_reg = g_zeros_left_reg - run_befor;
		
		g_run_coeff_reg[g_index_reg] = run_befor;
		
		g_index_reg--;

	}
	
	if (g_zeros_left_reg != 0)
	{
//		PrintfRunBefore (0, 0, g_zeros_left_reg, 0);

		g_run_coeff_reg[0] = g_zeros_left_reg;
	}

	return 0;
}