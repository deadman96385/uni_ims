/******************************************************************************
 ** File Name:    h264dec_vld.c                                               *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
#include "tiger_video_header.h"
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
/*left most 6 bit denote total coeff, right most 2 bits denote trailing one*/
/*	(nBit<<8) | (totalCoeff<<2) | tralingOne   */
/*0 =< nC < 2*/
	
/*the leading zero is 0, 1, 2*/
DECLARE_ASM_CONST (4, uint16, g_vlc0_0_tbl [8]) = 
{
	0xffff, (3<<8)|(2<<2)|2, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (1<<8)|(0<<2)|0, (1<<8)|(0<<2)|0, (1<<8)|(0<<2)|0, (1<<8)|(0<<2)|0 
};
	
/*leading zero is 3, 4, 5*/
DECLARE_ASM_CONST (4, uint16, g_vlc0_1_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, (8<<8)|(6<<2)|3, (8<<8)|(4<<2)|2, (8<<8)|(3<<2)|1, (8<<8)|(2<<2)|0,   //leading zero is 5
	(7<<8)|(5<<2)|3, (7<<8)|(5<<2)|3, (7<<8)|(3<<2)|2, (7<<8)|(3<<2)|2, (6<<8)|(4<<2)|3, (6<<8)|(4<<2)|3, (6<<8)|(4<<2)|3, (6<<8)|(4<<2)|3,  //leading zero is 4
	(6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0,  //leading zero is 3
	(5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, (5<<8)|(3<<2)|3, 	
};
	
/*leading zero is 6, 7, 8*/
DECLARE_ASM_CONST (4, uint16, g_vlc0_2_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, (11<<8)|(9<<2)|3, (11<<8)|(7<<2)|2, (11<<8)|(6<<2)|1, (11<<8)|(5<<2)|0, 		//leading zero is 8
	(10<<8)|(8<<2)|3, (10<<8)|(8<<2)|3, (10<<8)|(6<<2)|2, (10<<8)|(6<<2)|2, (10<<8)|(5<<2)|1, (10<<8)|(5<<2)|1, (10<<8)|(4<<2)|0, (10<<8)|(4<<2)|0, 		//leading zero is 7
	(9<<8)|(7<<2)|3, (9<<8)|(7<<2)|3, (9<<8)|(7<<2)|3, (9<<8)|(7<<2)|3, (9<<8)|(5<<2)|2, (9<<8)|(5<<2)|2, (9<<8)|(5<<2)|2, (9<<8)|(5<<2)|2, 		//leading zero is 6
	(9<<8)|(4<<2)|1, (9<<8)|(4<<2)|1, (9<<8)|(4<<2)|1, (9<<8)|(4<<2)|1, (9<<8)|(3<<2)|0, (9<<8)|(3<<2)|0, (9<<8)|(3<<2)|0, (9<<8)|(3<<2)|0, 
};
	
/*leading zero is 9~14*/
DECLARE_ASM_CONST (4, uint16, g_vlc0_3_tbl[128]) = 
{
	0xffff, 0xffff, (15<<8)|(13<<2)|1, (15<<8)|(13<<2)|1, (16<<8)|(16<<2)|0, (16<<8)|(16<<2)|2, (16<<8)|(16<<2)|1, (16<<8)|(15<<2)|0,	//leading zero is 14(first 4), 13 
	(16<<8)|(16<<2)|3, (16<<8)|(15<<2)|2, (16<<8)|(15<<2)|1, (16<<8)|(14<<2)|0, (16<<8)|(15<<2)|3, (16<<8)|(14<<2)|2, (16<<8)|(14<<2)|1, (16<<8)|(13<<2)|0, //leading zero is 12
	(15<<8)|(14<<2)|3, (15<<8)|(14<<2)|3, (15<<8)|(13<<2)|2, (15<<8)|(13<<2)|2, (15<<8)|(12<<2)|1, (15<<8)|(12<<2)|1, (15<<8)|(12<<2)|0, (15<<8)|(12<<2)|0, //leading zero is 11
	(15<<8)|(13<<2)|3, (15<<8)|(13<<2)|3, (15<<8)|(12<<2)|2, (15<<8)|(12<<2)|2, (15<<8)|(11<<2)|1, (15<<8)|(11<<2)|1, (15<<8)|(11<<2)|0, (15<<8)|(11<<2)|0, 
	(14<<8)|(12<<2)|3, (14<<8)|(12<<2)|3, (14<<8)|(12<<2)|3, (14<<8)|(12<<2)|3, (14<<8)|(11<<2)|2, (14<<8)|(11<<2)|2, (14<<8)|(11<<2)|2, (14<<8)|(11<<2)|2, //leading zero is 10
	(14<<8)|(10<<2)|1, (14<<8)|(10<<2)|1, (14<<8)|(10<<2)|1, (14<<8)|(10<<2)|1, (14<<8)|(10<<2)|0, (14<<8)|(10<<2)|0, (14<<8)|(10<<2)|0, (14<<8)|(10<<2)|0, 
	(14<<8)|(11<<2)|3, (14<<8)|(11<<2)|3, (14<<8)|(11<<2)|3, (14<<8)|(11<<2)|3, (14<<8)|(10<<2)|2, (14<<8)|(10<<2)|2, (14<<8)|(10<<2)|2, (14<<8)|(10<<2)|2, 
	(14<<8)|( 9<<2)|1, (14<<8)|( 9<<2)|1, (14<<8)|( 9<<2)|1, (14<<8)|( 9<<2)|1, (14<<8)|( 9<<2)|0, (14<<8)|( 9<<2)|0, (14<<8)|( 9<<2)|0, (14<<8)|( 9<<2)|0,
	(13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, (13<<8)|( 8<<2)|0, //leading zero is 9
	(13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, (13<<8)|( 9<<2)|2, 
	(13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, (13<<8)|( 8<<2)|1, 
	(13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, (13<<8)|( 7<<2)|0, 
	(13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, (13<<8)|(10<<2)|3, 
	(13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, (13<<8)|( 8<<2)|2, 
	(13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, (13<<8)|( 7<<2)|1, 
	(13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, (13<<8)|( 6<<2)|0, 
};
		
/*2<=nC<4*/
	
/*leading zero is 0, 1, 2, 3*/
DECLARE_ASM_CONST (4, uint16, g_vlc1_0_tbl[64]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, (6<<8)|(7<<2)|3, (6<<8)|(4<<2)|2, (6<<8)|(4<<2)|1, (6<<8)|(2<<2)|0,    //leading zero 3
	(6<<8)|(6<<2)|3, (6<<8)|(3<<2)|2, (6<<8)|(3<<2)|1, (6<<8)|(1<<2)|0, (5<<8)|(5<<2)|3, (5<<8)|(5<<2)|3, (5<<8)|(2<<2)|1, (5<<8)|(2<<2)|1,    //leading zero 2
	(4<<8)|(4<<2)|3, (4<<8)|(4<<2)|3, (4<<8)|(4<<2)|3, (4<<8)|(4<<2)|3, (4<<8)|(3<<2)|3, (4<<8)|(3<<2)|3, (4<<8)|(3<<2)|3, (4<<8)|(3<<2)|3,    //leading zero 1
	(3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, (3<<8)|(2<<2)|2, 
	(2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1,    //leading zero 0
	(2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, (2<<8)|(1<<2)|1, 
	(2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, 
	(2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, 
};
	
/*leading zero is 4, 5, 6*/
DECLARE_ASM_CONST (4, uint16, g_vlc1_1_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff,  (9<<8)|(9<<2)|3, (9<<8)|(7<<2)|2, (9<<8)|(7<<2)|1, (9<<8)|(6<<2)|0,	   //leading zero is 6
	(8<<8)|(5<<2)|0, (8<<8)|(5<<2)|0, (8<<8)|(6<<2)|2, (8<<8)|(6<<2)|2, (8<<8)|(6<<2)|1, (8<<8)|(6<<2)|1, (8<<8)|(4<<2)|0, (8<<8)|(4<<2)|0,    //leading zero is 5
	(7<<8)|(8<<2)|3, (7<<8)|(8<<2)|3, (7<<8)|(8<<2)|3, (7<<8)|(8<<2)|3, (7<<8)|(5<<2)|2, (7<<8)|(5<<2)|2, (7<<8)|(5<<2)|2, (7<<8)|(5<<2)|2,    //leading zero is 4
	(7<<8)|(5<<2)|1, (7<<8)|(5<<2)|1, (7<<8)|(5<<2)|1, (7<<8)|(5<<2)|1, (7<<8)|(3<<2)|0, (7<<8)|(3<<2)|0, (7<<8)|(3<<2)|0, (7<<8)|(3<<2)|0, 
};
	
/*leading zero is 7, 8*/
DECLARE_ASM_CONST (4, uint16, g_vlc1_2_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	(12<<8)|(11<<2)|0, (12<<8)|(11<<2)|2, (12<<8)|(11<<2)|1, (12<<8)|(10<<2)|0, (12<<8)|(12<<2)|3, (12<<8)|(10<<2)|2, (12<<8)|(10<<2)|1, (12<<8)|(9<<2)|0, //leading zero is 8
	(11<<8)|(11<<2)|3, (11<<8)|(11<<2)|3, (11<<8)|( 9<<2)|2, (11<<8)|( 9<<2)|2, (11<<8)|( 9<<2)|1, (11<<8)|( 9<<2)|1, (11<<8)|( 8<<2)|0, (11<<8)|(8<<2)|0, //leading zero is 7
	(11<<8)|(10<<2)|3, (11<<8)|(10<<2)|3, (11<<8)|( 8<<2)|2, (11<<8)|( 8<<2)|2, (11<<8)|( 8<<2)|1, (11<<8)|( 8<<2)|1, (11<<8)|( 7<<2)|0, (11<<8)|(7<<2)|0,
};
	
/*leading zero is 9, 10, 11, 12*/
DECLARE_ASM_CONST (4, uint16, g_vlc1_3_tbl[32]) = 
{
	0xffff,		   0xffff, (13<<8)|(15<<2)|3, (13<<8)|(15<<2)|3, (14<<8)|(16<<2)|3, (14<<8)|(16<<2)|2, (14<<8)|(16<<2)|1, (14<<8)|(16<<2)|0, //leading zero is 12, 11
	(14<<8)|(15<<2)|1, (14<<8)|(15<<2)|0, (14<<8)|(15<<2)|2, (14<<8)|(14<<2)|1, (13<<8)|(14<<2)|2, (13<<8)|(14<<2)|2, (13<<8)|(14<<2)|0, (13<<8)|(14<<2)|0, //leading zero is 10
	(13<<8)|(14<<2)|3, (13<<8)|(14<<2)|3, (13<<8)|(13<<2)|2, (13<<8)|(13<<2)|2, (13<<8)|(13<<2)|1, (13<<8)|(13<<2)|1, (13<<8)|(13<<2)|0, (13<<8)|(13<<2)|0, //leading zero is 9
	(13<<8)|(13<<2)|3, (13<<8)|(13<<2)|3, (13<<8)|(12<<2)|2, (13<<8)|(12<<2)|2, (13<<8)|(12<<2)|1, (13<<8)|(12<<2)|1, (13<<8)|(12<<2)|0, (13<<8)|(12<<2)|0,
};
	
/*4=<nC<8*/
	
/*leading zero is 0, 1*/
DECLARE_ASM_CONST (4, uint16, g_vlc2_0_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	(5<<8)|(5<<2)|1, (5<<8)|(5<<2)|2, (5<<8)|(4<<2)|1, (5<<8)|(4<<2)|2, (5<<8)|(3<<2)|1, (5<<8)|(8<<2)|3, (5<<8)|(3<<2)|2, (5<<8)|(2<<2)|1,  //leading zero is 1
	(4<<8)|(7<<2)|3, (4<<8)|(7<<2)|3, (4<<8)|(6<<2)|3, (4<<8)|(6<<2)|3, (4<<8)|(5<<2)|3, (4<<8)|(5<<2)|3, (4<<8)|(4<<2)|3, (4<<8)|(4<<2)|3,  //leading zero is 0 
	(4<<8)|(3<<2)|3, (4<<8)|(3<<2)|3, (4<<8)|(2<<2)|2, (4<<8)|(2<<2)|2, (4<<8)|(1<<2)|1, (4<<8)|(1<<2)|1, (4<<8)|(0<<2)|0, (4<<8)|(0<<2)|0, 
};
	
/*leading zero is 2, 3*/
DECLARE_ASM_CONST (4, uint16, g_vlc2_1_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	(7<<8)|(7<<2)|0, (7<<8)|(6<<2)|0, (7<<8)|(9<<2)|2, (7<<8)|(5<<2)|0, (7<<8)|(10<<2)|3, (7<<8)|(8<<2)|2, (7<<8)|(8<<2)|1, (7<<8)|(4<<2)|0, //leading zero is 3
	(6<<8)|(3<<2)|0, (6<<8)|(3<<2)|0, (6<<8)|(7<<2)|2, (6<<8)|(7<<2)|2, (6<<8)|( 7<<2)|1, (6<<8)|(7<<2)|1, (6<<8)|(2<<2)|0, (6<<8)|(2<<2)|0, //leading zero is 2
	(6<<8)|(9<<2)|3, (6<<8)|(9<<2)|3, (6<<8)|(6<<2)|2, (6<<8)|(6<<2)|2, (6<<8)|( 6<<2)|1, (6<<8)|(6<<2)|1, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, 
};
	
/*leading zero is 4, 5*/
DECLARE_ASM_CONST (4, uint16, g_vlc2_2_tbl[32]) = 
{
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	(9<<8)|(12<<2)|0, (9<<8)|(13<<2)|2, (9<<8)|(12<<2)|1, (9<<8)|(11<<2)|0, (9<<8)|(13<<2)|3, (9<<8)|(12<<2)|2, (9<<8)|(11<<2)|1, (9<<8)|(10<<2)|0, //leading zero is 5
	(8<<8)|(12<<2)|3, (8<<8)|(12<<2)|3, (8<<8)|(11<<2)|2, (8<<8)|(11<<2)|2, (8<<8)|(10<<2)|1, (8<<8)|(10<<2)|1, (8<<8)|( 9<<2)|0, (8<<8)|( 9<<2)|0, //leading zero is 4
	(8<<8)|(11<<2)|3, (8<<8)|(11<<2)|3, (8<<8)|(10<<2)|2, (8<<8)|(10<<2)|2, (8<<8)|( 9<<2)|1, (8<<8)|( 9<<2)|1, (8<<8)|( 8<<2)|0, (8<<8)|( 8<<2)|0, 
};
	
/*leading zero is 6, 7, 8, 9*/
DECLARE_ASM_CONST (4, uint16, g_vlc2_3_tbl[16]) = 
{
	0xffff, (10<<8)|(16<<2)|0, (10<<8)|(16<<2)|3, (10<<8)|(16<<2)|2, (10<<8)|(16<<2)|1, (10<<8)|(15<<2)|0, (10<<8)|(15<<2)|3, (10<<8)|(15<<2)|2,   
	(10<<8)|(15<<2)|1, (10<<8)|(14<<2)|0, (10<<8)|(14<<2)|3, (10<<8)|(14<<2)|2, (10<<8)|(14<<2)|1, (10<<8)|(13<<2)|0, (9<<8)|(13<<2)|1, (9<<8)|(13<<2)|1, 
};
	
/*nC >= 8*/
DECLARE_ASM_CONST (4, uint16, g_vlc3_0_tbl[64]) = 
{
	(6<<8)|( 1<<2)|0, (6<<8)|( 1<<2)|1, 		  0xffff, (6<<8)|( 0<<2)|0, (6<<8)|( 2<<2)|0, (6<<8)|( 2<<2)|1, (6<<8)|( 2<<2)|2,		   0xffff, 
	(6<<8)|( 3<<2)|0, (6<<8)|( 3<<2)|1, (6<<8)|( 3<<2)|2, (6<<8)|( 3<<2)|3, (6<<8)|( 4<<2)|0, (6<<8)|( 4<<2)|1, (6<<8)|( 4<<2)|2, (6<<8)|( 4<<2)|3, 
	(6<<8)|( 5<<2)|0, (6<<8)|( 5<<2)|1, (6<<8)|( 5<<2)|2, (6<<8)|( 5<<2)|3, (6<<8)|( 6<<2)|0, (6<<8)|( 6<<2)|1, (6<<8)|( 6<<2)|2, (6<<8)|( 6<<2)|3,
	(6<<8)|( 7<<2)|0, (6<<8)|( 7<<2)|1, (6<<8)|( 7<<2)|2, (6<<8)|( 7<<2)|3, (6<<8)|( 8<<2)|0, (6<<8)|( 8<<2)|1, (6<<8)|( 8<<2)|2, (6<<8)|( 8<<2)|3, 
	(6<<8)|( 9<<2)|0, (6<<8)|( 9<<2)|1, (6<<8)|( 9<<2)|2, (6<<8)|( 9<<2)|3, (6<<8)|(10<<2)|0, (6<<8)|(10<<2)|1, (6<<8)|(10<<2)|2, (6<<8)|(10<<2)|3, 
	(6<<8)|(11<<2)|0, (6<<8)|(11<<2)|1, (6<<8)|(11<<2)|2, (6<<8)|(11<<2)|3, (6<<8)|(12<<2)|0, (6<<8)|(12<<2)|1, (6<<8)|(12<<2)|2, (6<<8)|(12<<2)|3,
	(6<<8)|(13<<2)|0, (6<<8)|(13<<2)|1, (6<<8)|(13<<2)|2, (6<<8)|(13<<2)|3, (6<<8)|(14<<2)|0, (6<<8)|(14<<2)|1, (6<<8)|(14<<2)|2, (6<<8)|(14<<2)|3, 
	(6<<8)|(15<<2)|0, (6<<8)|(15<<2)|1, (6<<8)|(15<<2)|2, (6<<8)|(15<<2)|3, (6<<8)|(16<<2)|0, (6<<8)|(16<<2)|1, (6<<8)|(16<<2)|2, (6<<8)|(16<<2)|3, 
};

void NumCoeffTrailingOnes (DEC_BS_T * stream, int nnz, int * numcoeff, int * numtrailingones)
{
	int val;
	int tmp;
	int nBits;

	/*nnz*/
	if (nnz < 2)
	{
		/*show next 16 bits, according to the value to look up different table*/
		val = BITSTREAMSHOWBITS (stream, 16);

		if (val >= (1<<13))
		{
			val = val >> 13;
			/*use val to index table g_table0*/
			tmp = g_vlc0_0_tbl [val];
		}
		else if (val >= (1<<10))
		{
			val = val >> 8;
			tmp = g_vlc0_1_tbl [val]; 
		}
		else if (val >= (1<<7))
		{
			val = val >> 5;
			tmp = g_vlc0_2_tbl [val];
		}
		else 
		{
			tmp = g_vlc0_3_tbl [val];
		}
	}
	else if (nnz < 4)
	{
		val = BITSTREAMSHOWBITS (stream, 14);

		if ( val >= (1<<10) )
		{
			val = val >> 8;
			tmp = g_vlc1_0_tbl [val];			
		}
		else if (val >= (1<<7))
		{
			val = val >> 5;
			tmp = g_vlc1_1_tbl [val];
		}
		else if (val >= (1<<5))
		{
			val = val >> 2;
			tmp = g_vlc1_2_tbl [val];
		}
		else
		{
			tmp = g_vlc1_3_tbl [val];
		}
	}
	else if (nnz < 8)
	{
		val = BITSTREAMSHOWBITS (stream, 10);

		if (val >= (1<<8)) //leading zero is 0, 1
		{
			val = val >> 5;

			tmp = g_vlc2_0_tbl [val];			
		}
		else if (val >= (1<<6))  //leading zero is 2, 3
		{
			val = val >> 3;

			tmp = g_vlc2_1_tbl [val];			
		}
		else if (val >= (1<<4))
		{
			val = val >> 1;

			tmp = g_vlc2_2_tbl [val];			
		}
		else
		{
			tmp = g_vlc2_3_tbl [val];
		}
	}
	else
	{
		val = BITSTREAMSHOWBITS (stream, 6);

		tmp = g_vlc3_0_tbl [val];	
	}

//	SCI_ASSERT (tmp != -1);
	if (tmp == -1)
	{
		g_image_ptr->error_flag = TRUE;
	}

	*numcoeff = (tmp >> 2) & 0x3f;
	*numtrailingones = tmp & 0x3;
	nBits = (tmp >> 8) & 0xff;
	BITSTREAMFLUSHBITS (stream, nBits);

	if ((*numcoeff > 16) || (*numcoeff < *numtrailingones) || (nBits > 32))
	{
		g_image_ptr->error_flag = TRUE;
		//printf ("" );
	}

	return;
}

void readLevel_VLC0(int * lev, int *pSign, DEC_BS_T * stream)
{
	int val;
	int tmp;
	int len = 1;
	int sign;
	int level;
	int code;
	uint32 msk = 0x80000000;

	//val = BitstreamShowBits (stream, 32);
	val = BITSTREAMSHOWBITS(stream, 32);

	/*get (leading zero number add 1)*/
#if _CMODEL_ //ndef _ARM926EJ_S_
	if (!val)
	{
		g_image_ptr->error_flag = TRUE;
		return;
	}
	while (!((uint32)val & msk))
	{
		len++;
		msk = msk >> 1;		
	}
#else
//	__asm {
//		clz len, val
//	}
	__asm__("clz %0, %1":"=&r"(len):"r"(val):"cc");
	len++;
#endif

	if (len < 15)			/*lint !e774 */
	{
		sign = (len - 1) & 1;
		level = (len-1) / 2 + 1;
	}
	else if (len == 15)		/*lint !e774 */
	{
		tmp = (val >> 13) & 0xf;		
		code = (1 << 4) | tmp;
		
		sign = (code & 1);
		level = ((code >> 1) & 0x7) + 8;

		len += 4;
	}
	else //len > 15
	{
		//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
		if (len >16)//because baseline (8-pixel precious) never go into here
		{
			g_image_ptr->error_flag = TRUE;
			*lev = 0;
			*pSign = 0;

			return;
		}else
		{
			int addbit;
			int offset;
			
			addbit = len - 16;
			code = (val >> (36 - len * 2)) & g_msk [len - 4];
			sign = code & 1;

			len += len - 4;

			offset = (2048 << addbit) + 16 - 2048;
			level = (code >> 1) + offset;
		}	
	}

	BITSTREAMFLUSHBITS(stream, len);

	*lev = level;
	*pSign = sign;

	return;
}

void readLevel_VLCN(int * lev, int * pSign, int suffixLen, DEC_BS_T * stream)
{
	int len;
	int val;
	int tmp;
	int sign;
	int addbit;
	int numPrefix = 0;
	int level;
	uint32 msk = 0x80000000;

	//val = BitstreamShowBits (stream, 32);
	val = BITSTREAMSHOWBITS(stream, 32);
	
	/*get (leading zero number)*/
#if _CMODEL_//ndef _ARM926EJ_S_
	if (!val)
	{
		g_image_ptr->error_flag = TRUE;
		return;
	}
	while (!((uint32)val & msk))
	{
		numPrefix++;
		msk = msk >> 1;
	}
#else
//	__asm {
//		clz numPrefix, val
//	}
	__asm__("clz %0, %1":"=&r"(numPrefix):"r"(val):"cc");
#endif
	//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
	if (numPrefix > 31)
	{
		g_image_ptr->error_flag = TRUE;
		*lev = 0;
		*pSign = 0;

		return;
	}

	len = numPrefix + 1;

	if (numPrefix < 15)	/*lint !e774 */
	{
		level = (numPrefix << (suffixLen - 1)) + 1;
		
		if (suffixLen > 1)
		{
			tmp = (val >> (32 - numPrefix - suffixLen)) & g_msk [suffixLen-1];
			level += tmp;
			len += suffixLen - 1;
		}
	
		len++;
		sign = (val >> (32 - len)) & 1;		
	}else
	{
		int offset;
		int escape = (15<<(suffixLen - 1))+1;
		
		addbit = numPrefix - 15;
		tmp = (val >> (32 - 11 - len - addbit)) & g_msk [11+addbit];
		len += 11 + addbit;

		offset = (2048<<addbit)+escape-2048;
		level = tmp + offset;

		/*sign*/
		len++;
		sign = (val >> (32 - len)) & 1;		
	}

	BITSTREAMFLUSHBITS (stream, len);

	*lev = level;
	*pSign = sign;

	return;
}

/*get all the value of none zero coefficient including trailing ones,
the result is put into levArr*/
void level_decode (DEC_BS_T * stream, int numcoeff, int numtrailingones)
{
	int k;
	int suffixLen = 0;
	int level, sign;
	int level_two_or_higher;

	/*sign of trailing ones*/
	BitstreamFlushBits(stream, numtrailingones);	

	/*each level except trailing ones*/
	level_two_or_higher = 1;
    if (numcoeff > 3 && numtrailingones == 3)
		level_two_or_higher = 0;
	
	if (numcoeff > 10 && numtrailingones < 3)
		suffixLen = 1;
	
    for (k = numcoeff - 1 - numtrailingones; k >= 0; k--)
    {
		if (suffixLen == 0)
			readLevel_VLC0(&level, &sign, stream);
		else
			readLevel_VLCN(&level, &sign, suffixLen, stream);
		
		if (level_two_or_higher) //for first non-trailing one coeff(numtrailingone < 3),
		{
			level++;
			level_two_or_higher = 0;
		}	

		if (level > g_incVlc_tbl[suffixLen])
			suffixLen++;

		if (k == numcoeff - 1 - numtrailingones && level>3)
			suffixLen = 2;		
    }

	return;
}

int read_total_zeros_CHROMA_DC (DEC_BS_T * stream, int vclnum)
{
	int len;
	int val;
	int totalZeros_nBits;
	int totalZeros;
	
	len = 4 - vclnum;
	val = BITSTREAMSHOWBITS (stream, len);
	
	totalZeros_nBits = g_totZero_Chroma_DC [vclnum-1][val];
	
	totalZeros = (totalZeros_nBits >> 4) & 0xf;
	len = totalZeros_nBits & 0xf;
	BITSTREAMFLUSHBITS (stream, len);
	
	return totalZeros;
}

#if 1
int total_zeros_totCoeff1 (int val)
{
	int totalZeros;

	if (val >= (1<<5))
	{
		val >>= 4;
		totalZeros = g_totZero1_0_tbl [val];
	}
	else
		totalZeros = g_totZero1_1_tbl [val];

	return totalZeros;
}

int total_zeros_totCoeff2 (int val)
{
	int totalZeros;

	if (val >= 8)
	{
		val >>= 2;
		totalZeros = g_totZero2_0_tbl [val];
	}
	else
		totalZeros = g_totZero2_1_tbl [val];

	return totalZeros;
}

int total_zeros_totCoeff3 (int val)
{
	int totalZeros;

	if (val >= 8)
	{
		val >>= 2;
		totalZeros = g_totZero3_0_tbl [val];
	}
	else
		totalZeros = g_totZero3_1_tbl [val];

	return totalZeros;
}

/*6, 7, 8, 9, 10*/
int total_zeros_totCoeff (int32 val, const uint8 * tab1, const uint8 * tab2)
{
	int totalZeros;
	
	if (val >= 4)
	{
		val >>= 2;
		totalZeros = tab1 [val];
	}
	else
		totalZeros = tab2 [val];
	
	return totalZeros;	
}

int read_total_zeros (DEC_BS_T * stream, int vclnum)
{
	int totalZeros;
	int nBits;
	int totalZeros_nBits;
	int val;
    int len;
	
	len = g_total_zero_len_tbl [vclnum];

	val = BITSTREAMSHOWBITS (stream, len);	

	/*@@@@ init at the start*/
	switch(vclnum)
	{
	case 1:
		totalZeros_nBits = total_zeros_totCoeff1 (val);	
		break;
	case 2:
		totalZeros_nBits = total_zeros_totCoeff2 (val);	
		break;
	case 3:
		totalZeros_nBits = total_zeros_totCoeff3 (val);
		break;
	case 4:
		totalZeros_nBits = g_totZero4_0_tbl [val];
		break;
	case 5:
		totalZeros_nBits = g_totZero5_0_tbl [val];
		break;
	case 6:
		totalZeros_nBits = total_zeros_totCoeff (val, g_totZero6_0_tbl, g_totZero6_1_tbl);
		break;
	case 7:
		totalZeros_nBits = total_zeros_totCoeff (val, g_totZero7_0_tbl, g_totZero7_1_tbl);
		break;
	case 8:
		totalZeros_nBits = total_zeros_totCoeff (val, g_totZero8_0_tbl, g_totZero8_1_tbl);
		break;
	case 9:
		totalZeros_nBits = total_zeros_totCoeff (val, g_totZero9_0_tbl, g_totZero9_1_tbl);
		break;
	case 10:
		totalZeros_nBits = total_zeros_totCoeff (val, g_totZero10_0_tbl, g_totZero10_1_tbl);
		break;
	case 11:
		totalZeros_nBits = g_totZero11_0_tbl [val];
		break;
	case 12:
		totalZeros_nBits = g_totZero12_0_tbl [val];
		break;
	case 13:
		totalZeros_nBits = g_totZero13_0_tbl [val];
		break;
	case 14:
		totalZeros_nBits = g_totZero14_0_tbl [val];
		break;

	default:
		totalZeros_nBits = g_totZero15_0_tbl [val];
		break;
	}

	totalZeros = (totalZeros_nBits >> 4) & 0xf;
	nBits = totalZeros_nBits & 0xf;

	BITSTREAMFLUSHBITS (stream, nBits);

	return totalZeros;
}
#else
#endif

int read_run_zeroLeftGt6 (DEC_BS_T * stream, int val)
{
	int nBits;
	int run_before;
	int run_nBits;
	int msk = (int)0x80000000;
	int leading_nZeros = 0;

	if (val >= 256)
	{
		val = val >> 8;
		
		//must! because BITSTRM may be error, xw@20100527
		if (val >= 8)
		{
			g_image_ptr->error_flag = TRUE;
			return 0;
		}

		run_nBits = g_run_zeroLeftGt6_tbl [val];
	}
	else
	{
		/*find leading zero*/
		val = val << 21;
#if _CMODEL_//ndef _ARM926EJ_S_
		if (!val)
		{
			g_image_ptr->error_flag = TRUE;
			return 0;
		}
		while (!(val & msk))
		{
			msk >>= 1;
			leading_nZeros++;
		}
#else
//		__asm {
//			clz leading_nZeros, val
//		}
		__asm__("clz %0, %1":"=&r"(leading_nZeros):"r"(val):"cc");
#endif

		//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
		if (leading_nZeros > 15)
		{
			g_image_ptr->error_flag = TRUE;
			return 0;
		}

		run_before = leading_nZeros + 4;
		nBits = leading_nZeros + 1;

		run_nBits = (run_before << 4) | nBits;
	}

	return run_nBits;
}

/*can use two dimensional array*/
int read_run_before (DEC_BS_T * stream, int zerosLeft)
{
	int len;
	int val;
	int run_nBit;
	int run_before;
	int nBits;

	len = g_run_before_len_tbl [zerosLeft];
	//val = BitstreamShowBits (stream, len);
	val = BITSTREAMSHOWBITS(stream, len);

	if (zerosLeft < 7)
	{
		if (zerosLeft < 1)
		{
			g_image_ptr->error_flag = TRUE;
			return 0;
		}
		run_nBit = g_run_zeroLeft [zerosLeft-1][val];
	}else
	{
		run_nBit = read_run_zeroLeftGt6 (stream, val);
	}

	run_before = (run_nBit >> 4) & 0xf;
	nBits = run_nBit & 0xf;

	BITSTREAMFLUSHBITS (stream, nBits);

	return run_before;	
}

void run_decode_CHROMA_DC (DEC_BS_T * stream, int numcoeff, int maxCoeff)
{
	int i;
	int totalZeros = 0;
	int zerosLeft;
	int runBefore;
	
	/*total number of zero coefficient*/
	if (numcoeff < maxCoeff)
		totalZeros = read_total_zeros_CHROMA_DC (stream, numcoeff);
	
	/*decode run before each coefficient*/
	zerosLeft = totalZeros;
	i = numcoeff - 1;

	while ((zerosLeft != 0) && (i != 0))
	{
		runBefore = read_run_before (stream, zerosLeft);
		zerosLeft -= runBefore;
		i--;
	}
	
	return;
}

void run_decode (DEC_BS_T * stream, int numcoeff, int maxCoeff)
{
	int i;
	int totalZeros = 0;
	int zerosLeft;
	int runBefore;

	/*total number of zero coefficient*/
	if (numcoeff < maxCoeff)
	{
		totalZeros = read_total_zeros (stream, numcoeff);
	}

	/*decode run before each coefficient*/
	zerosLeft = totalZeros;
	i = numcoeff - 1;

	while ((zerosLeft != 0) && (i != 0))
	{
		runBefore = read_run_before (stream, zerosLeft);		
		zerosLeft -= runBefore;
		
		i--;
	}

	return;
}

void NumCoeffTrailingOnesChromaDC (int * numcoeff, int * numtrailingones)
{
	int val;
	int tmp;
	int nBits;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;

	val = BITSTREAMSHOWBITS (stream, 8);

	if (val >= 32)
	{
		val = val >> 5;
		tmp = g_coeffToken_chroma_0_tbl [val];
	}
	else
	{
		tmp = g_coeffToken_chroma_1_tbl [val];
	}
	
	*numcoeff = (tmp >> 2) & 0x3f;
	*numtrailingones = tmp & 0x3;
	nBits = (tmp >> 8) & 0xff;
	BITSTREAMFLUSHBITS (stream, nBits);

	return;
}

int readCoeff4x4_CAVLC (DEC_MB_CACHE_T * mb_cache_ptr, int blkIndex, int maxCoeff)
{
	int pred_nnz;
	int leftNnz, topNnz;
	int blkStrIdx;
	int numcoeff;
	int numtrailingones;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	int8 *pNnzRef = mb_cache_ptr->nnz_cache;
	
	//compute nnz
	blkStrIdx = g_blk_order_map_tbl[blkIndex];
	leftNnz = pNnzRef [blkStrIdx - 1];
	topNnz  = pNnzRef [blkStrIdx - CTX_CACHE_WIDTH];	
	pred_nnz = leftNnz + topNnz;	
	if (pred_nnz < 64)
		pred_nnz = (pred_nnz+1) / 2;
	pred_nnz = pred_nnz & 31;
		
	NumCoeffTrailingOnes (stream, pred_nnz, &numcoeff, &numtrailingones);

	if (g_image_ptr->error_flag)
	{
		return 0;
	}
	
	pNnzRef [blkStrIdx] = numcoeff;
	
	if (numcoeff == 0)
		return 0;
	
	level_decode (stream, numcoeff, numtrailingones);
	run_decode (stream, numcoeff, maxCoeff);
	
	return 1;//numcoeff;
}

#define	LUMA_DC			0
#define LUMA_AC_I16		1
#define LUMA_AC			2
#define CHROMA_DC		3
#define CHROMA_AC		4

/******************************************************************
			derive nc according to block type, block id
*******************************************************************/
int32 GetNeighborCodedInfo_cabac (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T * mb_cache_ptr, int blk_type,  int	 blk_id )
{
	int		na;				//left block's nnz
	int		nb;				//top block's nnz
	int		lblk_avail;		//left block available
	int		tblk_avail;		//top block available
	int		coded_flag_a;
	int		coded_flag_b;

	int32 	lmb_avail	= mb_cache_ptr->mb_avail_a;//(g_hvld_info_ptr->mb_info >> 16) & 1;
	int32	tmb_avail	= mb_cache_ptr->mb_avail_b;//(g_hvld_info_ptr->mb_info >> 17) & 1;
	int		default_value = mb_info_ptr->is_intra ? 1 : 0;
	
	int32 	ctx_idx;

	switch (blk_type)
	{
		case LUMA_DC:
			if (lmb_avail)
			{
				coded_flag_a = (mb_cache_ptr->vld_dc_coded_flag >> 0) & 1;
			}
			else
			{
				coded_flag_a = default_value;
			}
			
			if (tmb_avail)
			{
				coded_flag_b = (mb_cache_ptr->vld_dc_coded_flag >> 4) & 1;
			}
			else
			{
				coded_flag_b = default_value;
			}
			break;

		case LUMA_AC_I16:
		//	break;

		case LUMA_AC:
			{
				int32 map_id = g_blk_order_map_tbl[blk_id];
				int8 *nnz_ref_ptr = mb_cache_ptr->nnz_cache;
				
				na = nnz_ref_ptr[map_id-1];
				nb = nnz_ref_ptr[map_id-CTX_CACHE_WIDTH];
				
				tblk_avail = ((blk_id == 0) || (blk_id == 1) || (blk_id == 4) || (blk_id == 5)) ? tmb_avail : 1;
				lblk_avail = ((blk_id == 0) || (blk_id == 2) || (blk_id == 8) || (blk_id == 10)) ? lmb_avail : 1;

				coded_flag_a = lblk_avail ? (na != 0) : default_value;
				coded_flag_b = tblk_avail ? (nb != 0) : default_value;
			}
			break;
		case CHROMA_DC:
			{
				int shift_bits;

				if (lmb_avail)
				{
					shift_bits = (blk_id == 0) ? 1 : 2;
					coded_flag_a = (mb_cache_ptr->vld_dc_coded_flag >> shift_bits) & 1;
				}else
				{
					coded_flag_a = default_value;
				}

				if (tmb_avail)
				{
					shift_bits = (blk_id == 0) ? 5 : 6;
					coded_flag_b = (mb_cache_ptr->vld_dc_coded_flag >> shift_bits) & 1;
				}else
				{
					coded_flag_b = default_value;
				}
			}
			break;

		case CHROMA_AC:

			{
				int32 map_id = g_blk_order_map_tbl[blk_id];
				int8 *nnz_ref_ptr = mb_cache_ptr->nnz_cache;

				blk_id = blk_id & 3;
				
				na = nnz_ref_ptr[map_id-1];
				nb = nnz_ref_ptr[map_id-CTX_CACHE_WIDTH];
				
				tblk_avail = ((blk_id == 0) || (blk_id == 1) ) ? tmb_avail : 1;
				lblk_avail = ((blk_id == 0) || (blk_id == 2) ) ? lmb_avail : 1;

				coded_flag_a = lblk_avail ? (na != 0) : default_value;
				coded_flag_b = tblk_avail ? (nb != 0) : default_value;
			}
			
			break;
			
	}

	ctx_idx = 2*coded_flag_b+coded_flag_a;

	return ctx_idx;
}

int32 read_and_store_CBP_block_bit (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T * mb_cache_ptr, int type, int blk_id)
{
	int		cbf;
	int		ctx_idx;
	
	uint8 map_tbl[5] = {0, 1, 4, 5, 6};

	ctx_idx = GetNeighborCodedInfo_cabac (mb_info_ptr, mb_cache_ptr, type, blk_id);

    //===== encode symbol =====
    cbf = biari_decode_symbol (g_image_ptr, g_image_ptr->curr_slice_ptr->tex_ctx->bcbp_contexts[map_tbl[type]] + ctx_idx);

	if (type == LUMA_DC)
	{
		mb_cache_ptr->vld_dc_coded_flag |=  ((cbf != 0) << 8);
	}
	else if (type == CHROMA_DC)
	{
		mb_cache_ptr->vld_dc_coded_flag |=  ((cbf != 0) << (9+blk_id));
	}
	
	return cbf;
}

int readCoeff4x4_CABAC (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int mb_type, int type, int blk_id)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_BS_T * stream =  img_ptr->bitstrm_ptr;
	int coeff[64];
	int coeff_ctr = -1;
	int pos = 0;
	int8 *pNnzRef = mb_cache_ptr->nnz_cache;
	int32 blkStrIdx;
	
  	//===== decode CBP-BIT =====
  	if ((coeff_ctr = read_and_store_CBP_block_bit(currMB, mb_cache_ptr, type, blk_id)))
  	{
  		//===== decode significance map =====
  		coeff_ctr = read_significance_map(img_ptr, currMB, mb_type, coeff);

		 //===== decode significant coefficients =====
		 read_significant_coefficients (img_ptr, currMB, mb_type, coeff);
  	}
	
	//compute nnz
	if (type != CHROMA_DC)
	{
		blkStrIdx = g_blk_order_map_tbl[blk_id];
		pNnzRef [blkStrIdx] = coeff_ctr;
	}
	
	return coeff_ctr;
}

int readCoeff_CAVLC_CHROMA_DC ()
{
	int numcoeff;
	int numtrailingones;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;

	NumCoeffTrailingOnesChromaDC (&numcoeff, &numtrailingones);
	
	if (numcoeff == 0)
		return 0;
	
	level_decode (stream, numcoeff, numtrailingones);
	run_decode_CHROMA_DC (stream, numcoeff, 4);
	
	return numcoeff;
}

void decode_LUMA_AC (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * pMBCache)
{
	int startPos = 0;
	int maxCoeff = 16;
	int blk4x4;
	int numCoeff;
	int cbp = currMB->cbp;
	
	if (currMB->mb_type == I16MB)
	{
		maxCoeff = 15;
		startPos = 1;
	}	

	for (blk4x4 = 0; blk4x4 < 16; blk4x4++)
	{
		if ( cbp & (1 << (blk4x4/4)) )
		{
			numCoeff = readCoeff4x4_CAVLC (pMBCache, blk4x4, maxCoeff);
			if(g_image_ptr->error_flag)
			{
				return;
			}
			pMBCache->cbp_luma_iqt |= (numCoeff<<blk4x4);
		}
		else
		{
			blk4x4 += 3;  //go to next 8x8 block 
		}
	}
	
	return;
}

void decode_CHROMA_AC (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * pMBCache)
{
	int blkIndex;
#if 0	
	int uv;
	int blk4x4_nr;
	for (uv = 0; uv < 2; uv++)
	{
		for (blk4x4_nr = 0; blk4x4_nr < 4; blk4x4_nr++)
		{	
			blkIndex = uv * 4 + blk4x4_nr + 16;
		
			if (readCoeff4x4_CAVLC (pMBCache, blkIndex, 15))
			{
				pMBCache->cbp_uv |= 0x1 << (uv*4 + blk4x4_nr);
			}
		}
	}
#else
	for (blkIndex = 0; blkIndex < 8; blkIndex++)
	{
		if (readCoeff4x4_CAVLC (pMBCache, blkIndex+16, 15))
		{
			pMBCache->cbp_uv |= 0x1 << blkIndex;
		}
	}
#endif

	return;
}

void decode_LUMA_DC_cabac (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr)
{
	int numCoeff;

	numCoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, MB_TYPE_LUMA_16DC, LUMA_DC, 0);

	return;
}

void decode_LUMA_AC_cabac (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr)
{
	int blk4x4;
	int numCoeff;
	int cbp = currMB->cbp;
	int32 blk_type;
	int32 mb_type;
	
	blk_type = (currMB->mb_type == I16MB) ? LUMA_AC_I16 : LUMA_AC;
	mb_type = (currMB->mb_type == I16MB) ? MB_TYPE_LUMA_16AC : MB_TYPE_LUMA_4x4;

	for (blk4x4 = 0; blk4x4 < 16; blk4x4+=4)
	{
		if ( cbp & (1 << (blk4x4/4)) )
		{
			numCoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, mb_type, blk_type, blk4x4);	mb_cache_ptr->cbp_luma_iqt |= ((numCoeff ? 1 : 0) << (blk4x4 + 0));
			numCoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, mb_type, blk_type, blk4x4+1);	mb_cache_ptr->cbp_luma_iqt |= ((numCoeff ? 1 : 0) << (blk4x4 + 1));
			numCoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, mb_type, blk_type, blk4x4+2);	mb_cache_ptr->cbp_luma_iqt |= ((numCoeff ? 1 : 0) << (blk4x4 + 2));
			numCoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, mb_type, blk_type, blk4x4+3);	mb_cache_ptr->cbp_luma_iqt |= ((numCoeff ? 1 : 0) << (blk4x4 + 3));
			if(g_image_ptr->error_flag)
			{
				return;
			}
		}
//		else
//		{
//			blk4x4 += 3;  //go to next 8x8 block 
//		}
	}
	
	return;
}

void decode_CHROMA_DC_cabac (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr)
{
//	int k;
	int uv;
	short DC[4];
//	short levArr[4];
//	char runArr[4];
	int numcoeff;
//	int coef_ctr;
//	short * pCoeff;

	/*dc coeff for uv*/
	for (uv = 0; uv < 2; uv++)
	{
//		pCoeff = mb_cache_ptr->coff_UV[uv];	
		
		/*initialize DC to 0*/
		((int *)DC) [0] = 0;
		((int *)DC) [1] = 0;
	
		numcoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, MB_TYPE_CHROMA_DC, CHROMA_DC, uv) ;

		if (numcoeff)
			mb_cache_ptr->cbp_uv |= 0xf << (4*uv);	
	}

	return;
}

void decode_CHROMA_AC_cabac (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr)
{
//	int k;
	int uv;
//	short levArr[16];
//	char  runArr[16]; 
	int numcoeff;
//	int coef_ctr;
	int blkIndex;
//	int level;
//	int index;
	int blk4x4_nr;
//	int16 * pCoeff;
	
	for (uv = 0; uv < 2; uv++)
	{
//		pCoeff = mb_cache_ptr->coff_UV[uv];
		
		for (blk4x4_nr = 0; blk4x4_nr < 4; blk4x4_nr++)
		{	
			blkIndex = uv * 4 + blk4x4_nr + 16;
		
			numcoeff = readCoeff4x4_CABAC (currMB, mb_cache_ptr, MB_TYPE_CHROMA_AC, CHROMA_AC, blkIndex) ;
			if (numcoeff)
			{
				mb_cache_ptr->cbp_uv |= 0x1 << (uv*4 + blk4x4_nr);
			}
			
//			pCoeff += 16;				
		}
	}

	return;
}


void H264Dec_ComputeCBPIqtMbc (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 	cbp		= mb_info_ptr->cbp;
	uint32	cbp_uv = mb_cache_ptr->cbp_uv;
	uint32	cbp_iqt , cbp_mbc;
	uint32	cbp_luma_iqt, cbp_luma_mbc;

	//calculate cbp26
	cbp_luma_iqt = mb_cache_ptr->cbp_luma_iqt;
	cbp_luma_mbc =  (cbp_luma_iqt & 0xc3c3) | 
					(((cbp_luma_iqt >>  2) & 0x303) <<  4) | 
					(((cbp_luma_iqt >>  4) & 0x303) <<  2);

	cbp_mbc = (cbp_uv << 16) | cbp_luma_mbc;
	cbp_iqt    = (cbp_uv << 16) | cbp_luma_iqt;

	if (cbp > 15)	cbp_iqt |= (1<<24);	
	if (cbp > 31)	cbp_iqt |= (1<<25);
		
	if (mb_info_ptr->mb_type == I16MB)
	{
		cbp_mbc |= 0xffff;
	}

	mb_cache_ptr->cbp_iqt = cbp_iqt;
	mb_cache_ptr->cbp_mbc = cbp_mbc;
}

void decode_mb_cabac (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_cache_ptr->vld_dc_coded_flag = (mb_cache_ptr->vld_dc_coded_flag & 0xff);

	if (mb_info_ptr->mb_type == I16MB)
	{
		decode_LUMA_DC_cabac(mb_info_ptr, mb_cache_ptr);
	}

	decode_LUMA_AC_cabac(mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->cbp > 15)
	{
		decode_CHROMA_DC_cabac(mb_info_ptr, mb_cache_ptr);
	}

	if (mb_info_ptr->cbp > 31)
	{
		decode_CHROMA_AC_cabac(mb_info_ptr, mb_cache_ptr);
	}
		
	mb_info_ptr->dc_coded_flag = (mb_cache_ptr->vld_dc_coded_flag >> 8) & 0x7;
}

void decode_mb_cavlc (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	if (mb_info_ptr->mb_type == I16MB)
	{
		readCoeff4x4_CAVLC (mb_cache_ptr,  0, 16);	//luma DC
	}

	decode_LUMA_AC (mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->cbp > 15)
	{//CHROMA_DC
		if (readCoeff_CAVLC_CHROMA_DC ())
			mb_cache_ptr->cbp_uv |= 0xf;	
		
		if (readCoeff_CAVLC_CHROMA_DC ())	
			mb_cache_ptr->cbp_uv |= 0xf0;	
	}

	if (mb_info_ptr->cbp > 31)
	{
		decode_CHROMA_AC (mb_info_ptr, mb_cache_ptr); 
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
