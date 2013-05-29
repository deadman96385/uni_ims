#include "rdbk_global.h"

uint8	reg_blk0[16];
uint8	reg_blk1[16];
uint8	reg_blk2[16];
uint8	reg_blk3[16];
uint8	reg_blk4[16];
uint8	reg_blk5[16];


/*registers array organization:
	reg_blk0	reg_blk2	reg_blk4		
	reg_blk1	reg_blk3	reg_blk5		*/


void UpdatePixArrPipe_rv9()
{
	// reg_blk0 <= reg_blk2
	// reg_blk1 <= reg_blk3
	// reg_blk2 <= reg_blk4
	// reg_blk3 <= reg_blk5
	// reg_blk4 <= reg_blk0
	// reg_blk5 <= reg_blk1


	uint8	reg_tmp0, reg_tmp1;
	int		i;
	for (i=0;i<16;i++)
	{
		reg_tmp0	= reg_blk0[i];
		reg_tmp1	= reg_blk1[i];
		reg_blk0[i] = reg_blk2[i];
		reg_blk1[i] = reg_blk3[i];
		reg_blk2[i] = reg_blk4[i];
		reg_blk3[i] = reg_blk5[i];
		reg_blk4[i] = reg_tmp0;
		reg_blk5[i] = reg_tmp1;
	}
}

void GetEightPixWrite_rv9(int line_id, uint8 pixel_write0[4], uint8 pixel_write1[4])
{
	if (line_id == 0)
	{
		pixel_write0[0] = reg_blk4[0];
		pixel_write0[1] = reg_blk4[1];
		pixel_write0[2] = reg_blk4[2];
		pixel_write0[3] = reg_blk4[3];

		pixel_write1[0] = reg_blk5[0];
		pixel_write1[1] = reg_blk5[1];
		pixel_write1[2] = reg_blk5[2];
		pixel_write1[3] = reg_blk5[3];
	}
	else if (line_id == 1)
	{
		pixel_write0[0] = reg_blk4[4];
		pixel_write0[1] = reg_blk4[5];
		pixel_write0[2] = reg_blk4[6];
		pixel_write0[3] = reg_blk4[7];

		pixel_write1[0] = reg_blk5[4];
		pixel_write1[1] = reg_blk5[5];
		pixel_write1[2] = reg_blk5[6];
		pixel_write1[3] = reg_blk5[7];
	}
	else if (line_id == 2)
	{
		pixel_write0[0] = reg_blk4[8];
		pixel_write0[1] = reg_blk4[9];
		pixel_write0[2] = reg_blk4[10];
		pixel_write0[3] = reg_blk4[11];

		pixel_write1[0] = reg_blk5[8];
		pixel_write1[1] = reg_blk5[9];
		pixel_write1[2] = reg_blk5[10];
		pixel_write1[3] = reg_blk5[11];
	}
	else if (line_id == 3)
	{
		pixel_write0[0] = reg_blk4[12];
		pixel_write0[1] = reg_blk4[13];
		pixel_write0[2] = reg_blk4[14];
		pixel_write0[3] = reg_blk4[15];

		pixel_write1[0] = reg_blk5[12];
		pixel_write1[1] = reg_blk5[13];
		pixel_write1[2] = reg_blk5[14];
		pixel_write1[3] = reg_blk5[15];
	}
}


void Get8PixFiltering_rv9(int second_phase, int edge_id, int line_id,
					  uint8 pix_p[4],uint8 pix_q[4])
{
// For pix_p and pix_q
	if (second_phase)
	{
		if (edge_id == 3)
		{
			if (line_id == 0)
			{
				pix_p[0]	= reg_blk2[12];
				pix_p[1]	= reg_blk2[8];
				pix_p[2]	= reg_blk2[4];
				pix_p[3]	= reg_blk2[0];

				pix_q[0]	= reg_blk3[0];
				pix_q[1]	= reg_blk3[4];
				pix_q[2]	= reg_blk3[8];
				pix_q[3]	= reg_blk3[12];
			}
			else if (line_id == 1)
			{
				pix_p[0]	= reg_blk2[13];
				pix_p[1]	= reg_blk2[9];
				pix_p[2]	= reg_blk2[5];
				pix_p[3]	= reg_blk2[1];

				pix_q[0]	= reg_blk3[1];
				pix_q[1]	= reg_blk3[5];
				pix_q[2]	= reg_blk3[9];
				pix_q[3]	= reg_blk3[13];
			}
			else if (line_id == 2)
			{
				pix_p[0]	= reg_blk2[14];
				pix_p[1]	= reg_blk2[10];
				pix_p[2]	= reg_blk2[6];
				pix_p[3]	= reg_blk2[2];

				pix_q[0]	= reg_blk3[2];
				pix_q[1]	= reg_blk3[6];
				pix_q[2]	= reg_blk3[10];
				pix_q[3]	= reg_blk3[14];
			}
			else if (line_id == 3)
			{
				pix_p[0]	= reg_blk2[15];
				pix_p[1]	= reg_blk2[11];
				pix_p[2]	= reg_blk2[7];
				pix_p[3]	= reg_blk2[3];

				pix_q[0]	= reg_blk3[3];
				pix_q[1]	= reg_blk3[7];
				pix_q[2]	= reg_blk3[11];
				pix_q[3]	= reg_blk3[15];
			}
		}
		else // 2 or 4
		{
			if (line_id == 0)
			{
				pix_p[0]	= reg_blk1[3];
				pix_p[1]	= reg_blk1[2];
				pix_p[2]	= reg_blk1[1];
				pix_p[3]	= reg_blk1[0];

				pix_q[0]	= reg_blk3[0];
				pix_q[1]	= reg_blk3[1];
				pix_q[2]	= reg_blk3[2];
				pix_q[3]	= reg_blk3[3];
			}
			else if (line_id == 1)
			{
				pix_p[0]	= reg_blk1[7];
				pix_p[1]	= reg_blk1[6];
				pix_p[2]	= reg_blk1[5];
				pix_p[3]	= reg_blk1[4];

				pix_q[0]	= reg_blk3[4];
				pix_q[1]	= reg_blk3[5];
				pix_q[2]	= reg_blk3[6];
				pix_q[3]	= reg_blk3[7];
			}
			else if (line_id == 2)
			{
				pix_p[0]	= reg_blk1[11];
				pix_p[1]	= reg_blk1[10];
				pix_p[2]	= reg_blk1[9];
				pix_p[3]	= reg_blk1[8];

				pix_q[0]	= reg_blk3[8];
				pix_q[1]	= reg_blk3[9];
				pix_q[2]	= reg_blk3[10];
				pix_q[3]	= reg_blk3[11];
			}
			else if (line_id == 3)
			{
				pix_p[0]	= reg_blk1[15];
				pix_p[1]	= reg_blk1[14];
				pix_p[2]	= reg_blk1[13];
				pix_p[3]	= reg_blk1[12];

				pix_q[0]	= reg_blk3[12];
				pix_q[1]	= reg_blk3[13];
				pix_q[2]	= reg_blk3[14];
				pix_q[3]	= reg_blk3[15];
			}
		}
	}
	else //first phase
	{
		if (edge_id == 1) // same as edge3 in second phase
		{
			if (line_id == 0)
			{
				pix_p[0]	= reg_blk2[12];
				pix_p[1]	= reg_blk2[8];
				pix_p[2]	= reg_blk2[4];
				pix_p[3]	= reg_blk2[0];

				pix_q[0]	= reg_blk3[0];
				pix_q[1]	= reg_blk3[4];
				pix_q[2]	= reg_blk3[8];
				pix_q[3]	= reg_blk3[12];
			}
			else if (line_id == 1)
			{
				pix_p[0]	= reg_blk2[13];
				pix_p[1]	= reg_blk2[9];
				pix_p[2]	= reg_blk2[5];
				pix_p[3]	= reg_blk2[1];

				pix_q[0]	= reg_blk3[1];
				pix_q[1]	= reg_blk3[5];
				pix_q[2]	= reg_blk3[9];
				pix_q[3]	= reg_blk3[13];
			}
			else if (line_id == 2)
			{
				pix_p[0]	= reg_blk2[14];
				pix_p[1]	= reg_blk2[10];
				pix_p[2]	= reg_blk2[6];
				pix_p[3]	= reg_blk2[2];

				pix_q[0]	= reg_blk3[2];
				pix_q[1]	= reg_blk3[6];
				pix_q[2]	= reg_blk3[10];
				pix_q[3]	= reg_blk3[14];
			}
			else if (line_id == 3)
			{
				pix_p[0]	= reg_blk2[15];
				pix_p[1]	= reg_blk2[11];
				pix_p[2]	= reg_blk2[7];
				pix_p[3]	= reg_blk2[3];

				pix_q[0]	= reg_blk3[3];
				pix_q[1]	= reg_blk3[7];
				pix_q[2]	= reg_blk3[11];
				pix_q[3]	= reg_blk3[15];
			}
		}
		else // 2 or 4
		{
			if (line_id == 0)
			{
				pix_p[0]	= reg_blk0[3];
				pix_p[1]	= reg_blk0[2];
				pix_p[2]	= reg_blk0[1];
				pix_p[3]	= reg_blk0[0];

				pix_q[0]	= reg_blk2[0];
				pix_q[1]	= reg_blk2[1];
				pix_q[2]	= reg_blk2[2];
				pix_q[3]	= reg_blk2[3];
			}
			else if (line_id == 1)
			{
				pix_p[0]	= reg_blk0[7];
				pix_p[1]	= reg_blk0[6];
				pix_p[2]	= reg_blk0[5];
				pix_p[3]	= reg_blk0[4];

				pix_q[0]	= reg_blk2[4];
				pix_q[1]	= reg_blk2[5];
				pix_q[2]	= reg_blk2[6];
				pix_q[3]	= reg_blk2[7];
			}
			else if (line_id == 2)
			{
				pix_p[0]	= reg_blk0[11];
				pix_p[1]	= reg_blk0[10];
				pix_p[2]	= reg_blk0[9];
				pix_p[3]	= reg_blk0[8];

				pix_q[0]	= reg_blk2[8];
				pix_q[1]	= reg_blk2[9];
				pix_q[2]	= reg_blk2[10];
				pix_q[3]	= reg_blk2[11];
			}
			else if (line_id == 3)
			{
				pix_p[0]	= reg_blk0[15];
				pix_p[1]	= reg_blk0[14];
				pix_p[2]	= reg_blk0[13];
				pix_p[3]	= reg_blk0[12];

				pix_q[0]	= reg_blk2[12];
				pix_q[1]	= reg_blk2[13];
				pix_q[2]	= reg_blk2[14];
				pix_q[3]	= reg_blk2[15];
			}
		}
	}
}

void GetLeftRightArr_rv9(int edge_id, int second_phase,
					 uint8 left_arr[12],uint8 right_arr[12])
{
	// left_arr and righ_arr
	/*left_arr[12] = L32 L31 L30 L22 L21 L20 L12 L11 L10 L02 L01 L00
	right_arr[12]= R32 R31 R30 R22 R21 R20 R12 R11 R10 R02 R01 R00
	which organized as 
	L02 L01 L00 | R00 R01 R02
	L12 L11 L10 | R10 R11 R12
	L22 L21 L20 | R20 R21 R22
	L32 L31 L30 | R30 R31 R32*/
	if (second_phase)
	{
		if (edge_id == 3)
		{
			left_arr[0]		= reg_blk2[12];
			left_arr[1]		= reg_blk2[8];
			left_arr[2]		= reg_blk2[4];
			left_arr[3]		= reg_blk2[13];
			left_arr[4]		= reg_blk2[9];
			left_arr[5]		= reg_blk2[5];
			left_arr[6]		= reg_blk2[14];
			left_arr[7]		= reg_blk2[10];
			left_arr[8]		= reg_blk2[6];
			left_arr[9]		= reg_blk2[15];
			left_arr[10]	= reg_blk2[11];	
			left_arr[11]	= reg_blk2[7];

			right_arr[0]	= reg_blk3[0];
			right_arr[1]	= reg_blk3[4];
			right_arr[2]	= reg_blk3[8];
			right_arr[3]	= reg_blk3[1];
			right_arr[4]	= reg_blk3[5];
			right_arr[5]	= reg_blk3[9];
			right_arr[6]	= reg_blk3[2];
			right_arr[7]	= reg_blk3[6];
			right_arr[8]	= reg_blk3[10];
			right_arr[9]	= reg_blk3[3];
			right_arr[10]	= reg_blk3[7];
			right_arr[11]	= reg_blk3[11];
		}
		else // 2 or 4
		{
			left_arr[0]		= reg_blk1[3];
			left_arr[1]		= reg_blk1[2];
			left_arr[2]		= reg_blk1[1];
			left_arr[3]		= reg_blk1[7];
			left_arr[4]		= reg_blk1[6];
			left_arr[5]		= reg_blk1[5];
			left_arr[6]		= reg_blk1[11];
			left_arr[7]		= reg_blk1[10];
			left_arr[8]		= reg_blk1[9];
			left_arr[9]		= reg_blk1[15];
			left_arr[10]	= reg_blk1[14];
			left_arr[11]	= reg_blk1[13];

			right_arr[0]	= reg_blk3[0];
			right_arr[1]	= reg_blk3[1];
			right_arr[2]	= reg_blk3[2];
			right_arr[3]	= reg_blk3[4];
			right_arr[4]	= reg_blk3[5];
			right_arr[5]	= reg_blk3[6];
			right_arr[6]	= reg_blk3[8];
			right_arr[7]	= reg_blk3[9];
			right_arr[8]	= reg_blk3[10];
			right_arr[9]	= reg_blk3[12];
			right_arr[10]	= reg_blk3[13];
			right_arr[11]	= reg_blk3[14];
		}
	}
	else //first phase
	{
		if (edge_id == 1) // same as edge3 in second phase
		{
			left_arr[0]		= reg_blk2[12];
			left_arr[1]		= reg_blk2[8];
			left_arr[2]		= reg_blk2[4];
			left_arr[3]		= reg_blk2[13];
			left_arr[4]		= reg_blk2[9];
			left_arr[5]		= reg_blk2[5];
			left_arr[6]		= reg_blk2[14];
			left_arr[7]		= reg_blk2[10];
			left_arr[8]		= reg_blk2[6];
			left_arr[9]		= reg_blk2[15];
			left_arr[10]	= reg_blk2[11];	
			left_arr[11]	= reg_blk2[7];

			right_arr[0]	= reg_blk3[0];
			right_arr[1]	= reg_blk3[4];
			right_arr[2]	= reg_blk3[8];
			right_arr[3]	= reg_blk3[1];
			right_arr[4]	= reg_blk3[5];
			right_arr[5]	= reg_blk3[9];
			right_arr[6]	= reg_blk3[2];
			right_arr[7]	= reg_blk3[6];
			right_arr[8]	= reg_blk3[10];
			right_arr[9]	= reg_blk3[3];
			right_arr[10]	= reg_blk3[7];
			right_arr[11]	= reg_blk3[11];
		}
		else // 2 or 4
		{
			left_arr[0]		= reg_blk0[3];
			left_arr[1]		= reg_blk0[2];
			left_arr[2]		= reg_blk0[1];
			left_arr[3]		= reg_blk0[7];
			left_arr[4]		= reg_blk0[6];
			left_arr[5]		= reg_blk0[5];
			left_arr[6]		= reg_blk0[11];
			left_arr[7]		= reg_blk0[10];
			left_arr[8]		= reg_blk0[9];
			left_arr[9]		= reg_blk0[15];
			left_arr[10]	= reg_blk0[14];
			left_arr[11]	= reg_blk0[13];

			right_arr[0]	= reg_blk2[0];
			right_arr[1]	= reg_blk2[1];
			right_arr[2]	= reg_blk2[2];
			right_arr[3]	= reg_blk2[4];
			right_arr[4]	= reg_blk2[5];
			right_arr[5]	= reg_blk2[6];
			right_arr[6]	= reg_blk2[8];
			right_arr[7]	= reg_blk2[9];
			right_arr[8]	= reg_blk2[10];
			right_arr[9]	= reg_blk2[12];
			right_arr[10]	= reg_blk2[13];
			right_arr[11]	= reg_blk2[14];
		}
	}
}


void PixArrUptFilt_rv9(	uint8	L1_f, uint8 L2_f, uint8 L3_f,
					uint8	R1_f, uint8 R2_f, uint8 R3_f,
					int	L1_upt, int L2_upt, int L3_upt,
					int	R1_upt, int R2_upt, int R3_upt,
					int edge_id, int line_id, int second_phase)
{
	if (second_phase)
	{
		if (edge_id == 3)
		{
			if (line_id == 0)
			{
				if (L1_upt)
					reg_blk2[12]	= L1_f;

				if (L2_upt)
					reg_blk2[8]		= L2_f;

				if (L3_upt)
					reg_blk2[4]		= L3_f;

				if (R1_upt)
					reg_blk3[0]		= R1_f;

				if (R2_upt)
					reg_blk3[4]		= R2_f;

				if (R3_upt)
					reg_blk3[8]		= R3_f;
			}
			else if (line_id == 1)
			{
				if (L1_upt)
					reg_blk2[13]	= L1_f;

				if (L2_upt)
					reg_blk2[9]		= L2_f;

				if (L3_upt)
					reg_blk2[5]		= L3_f;

				if (R1_upt)
					reg_blk3[1]		= R1_f;

				if (R2_upt)
					reg_blk3[5]		= R2_f;

				if (R3_upt)
					reg_blk3[9]		= R3_f;
			}
			else if (line_id == 2)
			{
				if (L1_upt)
					reg_blk2[14]	= L1_f;

				if (L2_upt)
					reg_blk2[10]	= L2_f;

				if (L3_upt)
					reg_blk2[6]		= L3_f;

				if (R1_upt)
					reg_blk3[2]		= R1_f;

				if (R2_upt)
					reg_blk3[6]		= R2_f;

				if (R3_upt)
					reg_blk3[10]	= R3_f;
			}
			else if (line_id == 3)
			{
				if (L1_upt)
					reg_blk2[15]	= L1_f;

				if (L2_upt)
					reg_blk2[11]	= L2_f;

				if (L3_upt)
					reg_blk2[7]		= L3_f;

				if (R1_upt)
					reg_blk3[3]		= R1_f;

				if (R2_upt)
					reg_blk3[7]		= R2_f;

				if (R3_upt)
					reg_blk3[11]	= R3_f;
			}
		}
		else // 2 or 4
		{
			if (line_id == 0)
			{
				if (L1_upt)
					reg_blk1[3]		= L1_f;

				if (L2_upt)
					reg_blk1[2]		= L2_f;

				if (L3_upt)
					reg_blk1[1]		= L3_f;

				if (R1_upt)
					reg_blk3[0]		= R1_f;

				if (R2_upt)
					reg_blk3[1]		= R2_f;

				if (R3_upt)
					reg_blk3[2]		= R3_f;
			}
			else if (line_id == 1)
			{
				if (L1_upt)
					reg_blk1[7]		= L1_f;

				if (L2_upt)
					reg_blk1[6]		= L2_f;

				if (L3_upt)
					reg_blk1[5]		= L3_f;

				if (R1_upt)
					reg_blk3[4]		= R1_f;

				if (R2_upt)
					reg_blk3[5]		= R2_f;

				if (R3_upt)
					reg_blk3[6]		= R3_f;
			}
			else if (line_id == 2)
			{
				if (L1_upt)
					reg_blk1[11]	= L1_f;

				if (L2_upt)
					reg_blk1[10]	= L2_f;

				if (L3_upt)
					reg_blk1[9]		= L3_f;

				if (R1_upt)
					reg_blk3[8]		= R1_f;

				if (R2_upt)
					reg_blk3[9]		= R2_f;

				if (R3_upt)
					reg_blk3[10]	= R3_f;
			}
			else if (line_id == 3)
			{
				if (L1_upt)
					reg_blk1[15]	= L1_f;

				if (L2_upt)
					reg_blk1[14]	= L2_f;

				if (L3_upt)
					reg_blk1[13]	= L3_f;

				if (R1_upt)
					reg_blk3[12]	= R1_f;

				if (R2_upt)
					reg_blk3[13]	= R2_f;

				if (R3_upt)
					reg_blk3[14]	= R3_f;
			}
		}
	}
	else //first phase
	{
		if (edge_id == 1) // same as edge3 in second phase
		{
			if (line_id == 0)
			{
				if (L1_upt)
					reg_blk2[12]	= L1_f;

				if (L2_upt)
					reg_blk2[8]		= L2_f;

				if (L3_upt)
					reg_blk2[4]		= L3_f;

				if (R1_upt)
					reg_blk3[0]		= R1_f;

				if (R2_upt)
					reg_blk3[4]		= R2_f;

				if (R3_upt)
					reg_blk3[8]		= R3_f;
			}
			else if (line_id == 1)
			{
				if (L1_upt)
					reg_blk2[13]	= L1_f;

				if (L2_upt)
					reg_blk2[9]		= L2_f;

				if (L3_upt)
					reg_blk2[5]		= L3_f;

				if (R1_upt)
					reg_blk3[1]		= R1_f;

				if (R2_upt)
					reg_blk3[5]		= R2_f;

				if (R3_upt)
					reg_blk3[9]		= R3_f;
			}
			else if (line_id == 2)
			{
				if (L1_upt)
					reg_blk2[14]	= L1_f;

				if (L2_upt)
					reg_blk2[10]	= L2_f;

				if (L3_upt)
					reg_blk2[6]		= L3_f;

				if (R1_upt)
					reg_blk3[2]		= R1_f;

				if (R2_upt)
					reg_blk3[6]		= R2_f;

				if (R3_upt)
					reg_blk3[10]	= R3_f;
			}
			else if (line_id == 3)
			{
				if (L1_upt)
					reg_blk2[15]	= L1_f;

				if (L2_upt)
					reg_blk2[11]	= L2_f;

				if (L3_upt)
					reg_blk2[7]		= L3_f;

				if (R1_upt)
					reg_blk3[3]		= R1_f;

				if (R2_upt)
					reg_blk3[7]		= R2_f;

				if (R3_upt)
					reg_blk3[11]	= R3_f;
			}
		}
		else // 2 or 4
		{
			if (line_id == 0)
			{
				if (L1_upt)
					reg_blk0[3]		= L1_f;

				if (L2_upt)
					reg_blk0[2]		= L2_f;

				if (L3_upt)
					reg_blk0[1]		= L3_f;

				if (R1_upt)
					reg_blk2[0]		= R1_f;

				if (R2_upt)
					reg_blk2[1]		= R2_f;

				if (R3_upt)
					reg_blk2[2]		= R3_f;
			}
			else if (line_id == 1)
			{
				if (L1_upt)
					reg_blk0[7]		= L1_f;

				if (L2_upt)
					reg_blk0[6]		= L2_f;

				if (L3_upt)
					reg_blk0[5]		= L3_f;

				if (R1_upt)
					reg_blk2[4]		= R1_f;

				if (R2_upt)
					reg_blk2[5]		= R2_f;

				if (R3_upt)
					reg_blk2[6]		= R3_f;
			}
			else if (line_id == 2)
			{
				if (L1_upt)
					reg_blk0[11]	= L1_f;

				if (L2_upt)
					reg_blk0[10]	= L2_f;

				if (L3_upt)
					reg_blk0[9]		= L3_f;

				if (R1_upt)
					reg_blk2[8]		= R1_f;

				if (R2_upt)
					reg_blk2[9]		= R2_f;

				if (R3_upt)
					reg_blk2[10]	= R3_f;
			}
			else if (line_id == 3)
			{
				if (L1_upt)
					reg_blk0[15]	= L1_f;

				if (L2_upt)
					reg_blk0[14]	= L2_f;

				if (L3_upt)
					reg_blk0[13]	= L3_f;

				if (R1_upt)
					reg_blk2[12]	= R1_f;

				if (R2_upt)
					reg_blk2[13]	= R2_f;

				if (R3_upt)
					reg_blk2[14]	= R3_f;
			}
		}
	}	
	
}

void ReadPixUptArr_rv9 ( int	line_id, uint8 pixel_read0[4], uint8 pixel_read1[4])
{
	if (line_id == 0)
	{
		reg_blk4[0]  = pixel_read0[0];
		reg_blk4[1]  = pixel_read0[1];
		reg_blk4[2]  = pixel_read0[2];
		reg_blk4[3]  = pixel_read0[3];

		reg_blk5[0]  = pixel_read1[0];
		reg_blk5[1]  = pixel_read1[1];
		reg_blk5[2]  = pixel_read1[2];
		reg_blk5[3]  = pixel_read1[3];
	}
	else if (line_id == 1)
	{
		reg_blk4[4]  = pixel_read0[0];
		reg_blk4[5]  = pixel_read0[1];
		reg_blk4[6]  = pixel_read0[2];
		reg_blk4[7]  = pixel_read0[3];

		reg_blk5[4]  = pixel_read1[0];
		reg_blk5[5]  = pixel_read1[1];
		reg_blk5[6]  = pixel_read1[2];
		reg_blk5[7]  = pixel_read1[3];
	}
	else if (line_id == 2)
	{
		reg_blk4[8]  = pixel_read0[0];
		reg_blk4[9]  = pixel_read0[1];
		reg_blk4[10]  = pixel_read0[2];
		reg_blk4[11]  = pixel_read0[3];

		reg_blk5[8]  = pixel_read1[0];
		reg_blk5[9]  = pixel_read1[1];
		reg_blk5[10]  = pixel_read1[2];
		reg_blk5[11]  = pixel_read1[3];
	}
	else if (line_id == 3)
	{
		reg_blk4[12]  = pixel_read0[0];
		reg_blk4[13]  = pixel_read0[1];
		reg_blk4[14]  = pixel_read0[2];
		reg_blk4[15]  = pixel_read0[3];

		reg_blk5[12]  = pixel_read1[0];
		reg_blk5[13]  = pixel_read1[1];
		reg_blk5[14]  = pixel_read1[2];
		reg_blk5[15]  = pixel_read1[3];
	}
}