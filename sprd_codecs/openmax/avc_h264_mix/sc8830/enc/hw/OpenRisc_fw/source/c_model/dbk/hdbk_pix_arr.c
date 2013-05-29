/*hdbk_pix_arr.c*/
#include "video_common.h"
#include "hdbk_mode.h"
#include "hdbk_global.h"

/************************************************************
					|
			reg_blk0|reg_blk2	
			--------|--------
			reg_blk1|reg_blk3
					|

 register array behavior:
 1: generate 8 pixels for horizontal and vertical filtering;
 2: updated by horizontal and vertical filtering;
 3: exchange left and right two blocks for reducing multiplex logic
 4: output 4 pixels for writing back to on-chip buffer
 5: recive pixels read from on-chip buffer
*************************************************************/

uint8		reg_blk0[16];
uint8		reg_blk1[16];
uint8		reg_blk2[16];
uint8		reg_blk3[16];


void GetFourPixWrite (int line_id, uint8 pix_write[4])
{
	if (line_id >= 4)
	{
		if (line_id == 7)
		{
			pix_write[0] = reg_blk3[12];
			pix_write[1] = reg_blk3[13];
			pix_write[2] = reg_blk3[14];
			pix_write[3] = reg_blk3[15];
		}
		else if (line_id == 6)
		{
			pix_write[0] = reg_blk3[8];
			pix_write[1] = reg_blk3[9];
			pix_write[2] = reg_blk3[10];
			pix_write[3] = reg_blk3[11];
		}
		else if (line_id == 5)
		{
			pix_write[0] = reg_blk3[4];
			pix_write[1] = reg_blk3[5];
			pix_write[2] = reg_blk3[6];
			pix_write[3] = reg_blk3[7];
		}
		else
		{
			pix_write[0] = reg_blk3[0];
			pix_write[1] = reg_blk3[1];
			pix_write[2] = reg_blk3[2];
			pix_write[3] = reg_blk3[3];
		}
	}
	else
	{
		if (line_id == 3)
		{			
			pix_write[0] = reg_blk2[12];
			pix_write[1] = reg_blk2[13];
			pix_write[2] = reg_blk2[14];
			pix_write[3] = reg_blk2[15];
		}
		else if (line_id == 2)
		{			
			pix_write[0] = reg_blk2[8];
			pix_write[1] = reg_blk2[9];
			pix_write[2] = reg_blk2[10];
			pix_write[3] = reg_blk2[11];
		}
		else if (line_id == 1)
		{
			pix_write[0] = reg_blk2[4];
			pix_write[1] = reg_blk2[5];
			pix_write[2] = reg_blk2[6];
			pix_write[3] = reg_blk2[7];
		}
		else
		{
			pix_write[0] = reg_blk2[0];
			pix_write[1] = reg_blk2[1];
			pix_write[2] = reg_blk2[2];
			pix_write[3] = reg_blk2[3];
		}
	}
}

void Get8PixFiltering (int line_id, uint8 pix_p[4], uint8 pix_q[4])
{
	if (line_id >= 4)
	{
		/*horizontal filtering*/
		if (line_id == 7)
		{
			pix_p[0] = reg_blk1[15];
			pix_p[1] = reg_blk1[14];
			pix_p[2] = reg_blk1[13];
			pix_p[3] = reg_blk1[12];

			pix_q[0] = reg_blk3[12];
			pix_q[1] = reg_blk3[13];
			pix_q[2] = reg_blk3[14];
			pix_q[3] = reg_blk3[15];
		}
		else if (line_id == 6)
		{
			pix_p[0] = reg_blk1[11];
			pix_p[1] = reg_blk1[10];
			pix_p[2] = reg_blk1[9];
			pix_p[3] = reg_blk1[8];
			
			pix_q[0] = reg_blk3[8];
			pix_q[1] = reg_blk3[9];
			pix_q[2] = reg_blk3[10];
			pix_q[3] = reg_blk3[11];
		}
		else if (line_id == 5)
		{
			pix_p[0] = reg_blk1[7];
			pix_p[1] = reg_blk1[6];
			pix_p[2] = reg_blk1[5];
			pix_p[3] = reg_blk1[4];
			
			pix_q[0] = reg_blk3[4];
			pix_q[1] = reg_blk3[5];
			pix_q[2] = reg_blk3[6];
			pix_q[3] = reg_blk3[7];
		}
		else
		{			
			pix_p[0] = reg_blk1[3];
			pix_p[1] = reg_blk1[2];
			pix_p[2] = reg_blk1[1];
			pix_p[3] = reg_blk1[0];
			
			pix_q[0] = reg_blk3[0];
			pix_q[1] = reg_blk3[1];
			pix_q[2] = reg_blk3[2];
			pix_q[3] = reg_blk3[3];
		}
	}
	else
	{
		/*vertical filtering*/
		if (line_id == 0)
		{
			pix_p[0] = reg_blk0[12];
			pix_p[1] = reg_blk0[8];
			pix_p[2] = reg_blk0[4];
			pix_p[3] = reg_blk0[0];
			
			pix_q[0] = reg_blk1[0];
			pix_q[1] = reg_blk1[4];
			pix_q[2] = reg_blk1[8];
			pix_q[3] = reg_blk1[12];
		}
		else if (line_id == 1)
		{
			pix_p[0] = reg_blk0[13];
			pix_p[1] = reg_blk0[9];
			pix_p[2] = reg_blk0[5];
			pix_p[3] = reg_blk0[1];
			
			pix_q[0] = reg_blk1[1];
			pix_q[1] = reg_blk1[5];
			pix_q[2] = reg_blk1[9];
			pix_q[3] = reg_blk1[13];
		}
		else if (line_id == 2)
		{
			pix_p[0] = reg_blk0[14];
			pix_p[1] = reg_blk0[10];
			pix_p[2] = reg_blk0[6];
			pix_p[3] = reg_blk0[2];
			
			pix_q[0] = reg_blk1[2];
			pix_q[1] = reg_blk1[6];
			pix_q[2] = reg_blk1[10];
			pix_q[3] = reg_blk1[14];
		}
		else
		{			
			pix_p[0] = reg_blk0[15];
			pix_p[1] = reg_blk0[11];
			pix_p[2] = reg_blk0[7];
			pix_p[3] = reg_blk0[3];
			
			pix_q[0] = reg_blk1[3];
			pix_q[1] = reg_blk1[7];
			pix_q[2] = reg_blk1[11];
			pix_q[3] = reg_blk1[15];
		}
	}
}

void PixBlkExchange ()
{
	int		i;
	int		tmp;

	for (i = 0; i < 16; i++)
	{
		tmp = reg_blk2[i];
		reg_blk2[i] = reg_blk0[i];
		reg_blk0[i] = tmp;

		tmp = reg_blk3[i];
		reg_blk3[i] = reg_blk1[i];
		reg_blk1[i] = tmp;
	}
}

void PixBlkUptFilter (
					  int		line_id,
					  uint8		p0,
					  uint8		p1,
					  uint8		p2,
					  int		p0_upt,
					  int		p1_upt,
					  int		p2_upt,
					  uint8		q0,
					  uint8		q1,
					  uint8		q2,
					  int		q0_upt,
					  int		q1_upt,
					  int		q2_upt
					  )
{
	if (line_id >= 4)
	{
		if (line_id == 7)
		{
			if (p0_upt)
				reg_blk1[15] = p0;

			if (p1_upt)
				reg_blk1[14] = p1;

			if (p2_upt)
				reg_blk1[13] = p2;

			if (q0_upt)
				reg_blk3[12] = q0;

			if (q1_upt)
				reg_blk3[13] = q1;

			if (q2_upt)
				reg_blk3[14] = q2;
		}
		else if (line_id == 6)
		{
			if (p0_upt)
				reg_blk1[11] = p0;
			
			if (p1_upt)
				reg_blk1[10] = p1;
			
			if (p2_upt)
				reg_blk1[9] = p2;
			
			if (q0_upt)
				reg_blk3[8] = q0;
			
			if (q1_upt)
				reg_blk3[9] = q1;
			
			if (q2_upt)
				reg_blk3[10] = q2;
		}
		else if (line_id == 5)
		{
			if (p0_upt)
				reg_blk1[7] = p0;
			
			if (p1_upt)
				reg_blk1[6] = p1;
			
			if (p2_upt)
				reg_blk1[5] = p2;
			
			if (q0_upt)
				reg_blk3[4] = q0;
			
			if (q1_upt)
				reg_blk3[5] = q1;
			
			if (q2_upt)
				reg_blk3[6] = q2;
		}
		else
		{			
			if (p0_upt)
				reg_blk1[3] = p0;
			
			if (p1_upt)
				reg_blk1[2] = p1;
			
			if (p2_upt)
				reg_blk1[1] = p2;
			
			if (q0_upt)
				reg_blk3[0] = q0;
			
			if (q1_upt)
				reg_blk3[1] = q1;
			
			if (q2_upt)
				reg_blk3[2] = q2;
		}
	}
	else
	{
		if (line_id == 0)
		{
			if (p0_upt)
				reg_blk0[12] = p0;
			
			if (p1_upt)
				reg_blk0[8] = p1;
			
			if (p2_upt)
				reg_blk0[4] = p2;
			
			if (q0_upt)
				reg_blk1[0] = q0;
			
			if (q1_upt)
				reg_blk1[4] = q1;
			
			if (q2_upt)
				reg_blk1[8] = q2;
		}
		else if (line_id == 1)
		{
			if (p0_upt)
				reg_blk0[13] = p0;
			
			if (p1_upt)
				reg_blk0[9] = p1;
			
			if (p2_upt)
				reg_blk0[5] = p2;
			
			if (q0_upt)
				reg_blk1[1] = q0;
			
			if (q1_upt)
				reg_blk1[5] = q1;
			
			if (q2_upt)
				reg_blk1[9] = q2;
		}
		else if (line_id == 2)
		{			
			if (p0_upt)
				reg_blk0[14] = p0;
			
			if (p1_upt)
				reg_blk0[10] = p1;
			
			if (p2_upt)
				reg_blk0[6] = p2;
			
			if (q0_upt)
				reg_blk1[2] = q0;
			
			if (q1_upt)
				reg_blk1[6] = q1;
			
			if (q2_upt)
				reg_blk1[10] = q2;
		}
		else
		{
			if (p0_upt)
				reg_blk0[15] = p0;
			
			if (p1_upt)
				reg_blk0[11] = p1;
			
			if (p2_upt)
				reg_blk0[7] = p2;
			
			if (q0_upt)
				reg_blk1[3] = q0;
			
			if (q1_upt)
				reg_blk1[7] = q1;
			
			if (q2_upt)
				reg_blk1[11] = q2;
		}
	}
}

void PixBlkUptWrite (
					 int	line_id, 
					 uint8	pix0,
					 uint8	pix1,
					 uint8	pix2,
					 uint8	pix3
					 )
{
	if (line_id >= 4)
	{
		if (line_id == 7)
		{
			reg_blk3[12] = pix0;
			reg_blk3[13] = pix1;
			reg_blk3[14] = pix2;
			reg_blk3[15] = pix3;
		}
		else if (line_id == 6)
		{
			reg_blk3[8] = pix0;
			reg_blk3[9] = pix1;
			reg_blk3[10] = pix2;
			reg_blk3[11] = pix3;
		}
		else if (line_id == 5)
		{
			reg_blk3[4] = pix0;
			reg_blk3[5] = pix1;
			reg_blk3[6] = pix2;
			reg_blk3[7] = pix3;
		}
		else
		{
			reg_blk3[0] = pix0;
			reg_blk3[1] = pix1;
			reg_blk3[2] = pix2;
			reg_blk3[3] = pix3;
		}
	}
	else
	{
		if (line_id == 3)
		{
			reg_blk2[12] = pix0;
			reg_blk2[13] = pix1;
			reg_blk2[14] = pix2;
			reg_blk2[15] = pix3;
		}
		else if (line_id == 2)
		{
			reg_blk2[8] = pix0;
			reg_blk2[9] = pix1;
			reg_blk2[10] = pix2;
			reg_blk2[11] = pix3;
		}
		else if (line_id == 1)
		{
			reg_blk2[4] = pix0;
			reg_blk2[5] = pix1;
			reg_blk2[6] = pix2;
			reg_blk2[7] = pix3;
		}
		else
		{
			reg_blk2[0] = pix0;
			reg_blk2[1] = pix1;
			reg_blk2[2] = pix2;
			reg_blk2[3] = pix3;
		}
	}
}