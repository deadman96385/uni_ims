/******************************************************************************
 ** File Name:    h264dec_table.c                                             *
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
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
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

DECLARE_ASM_CONST (4, int8, g_ICBP_TBL[6]) = {0, 16, 32, 15, 31, 47};

DECLARE_ASM_CONST (4, uint8, g_QP_SCALER_CR_TBL[52]) =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
	12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
	28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
	37,38,38,38,39,39,39,39		
};

/*cbp table for intra MB*/
DECLARE_ASM_CONST (4, uint8, g_cbp_intra_tbl [48]) = 
{
	47, 31, 15,  0, 23, 27, 29, 30,  7, 11, 13, 14,
	39, 43, 45, 46, 16,  3,  5, 10, 12, 19, 21, 26,
	28, 35, 37, 42, 44,  1,  2,  4,  8, 17, 18, 20, 
	24,  6,  9, 22, 25, 32, 33, 34, 36, 40, 38, 41,
};

/*cbp table for inter MB*/
DECLARE_ASM_CONST (4, uint8, g_cbp_inter_tbl [48]) = 
{
	 0, 16,  1,  2,  4,  8, 32,  3,
	 5, 10, 12, 15, 47,  7, 11, 13, 
	14,  6,  9, 31, 35, 37, 42, 44, 
	33, 34, 36, 40, 39, 43, 45, 46, 
	17, 18, 20, 24, 19, 21, 26, 28, 
	23, 27, 29, 30, 22, 25, 38, 41,
};

//qp_per<<8 | (qp_rem<<0)
DECLARE_ASM_CONST (4, uint16, g_qpPerRem_tbl [52]) = 
{
	((0<<8)|0), ((0<<8)|1), ((0<<8)|2), ((0<<8)|3), ((0<<8)|4), ((0<<8)|5),
	((1<<8)|0), ((1<<8)|1), ((1<<8)|2), ((1<<8)|3), ((1<<8)|4), ((1<<8)|5),
	((2<<8)|0), ((2<<8)|1), ((2<<8)|2), ((2<<8)|3), ((2<<8)|4), ((2<<8)|5),
	((3<<8)|0), ((3<<8)|1), ((3<<8)|2), ((3<<8)|3), ((3<<8)|4), ((3<<8)|5),
	((4<<8)|0), ((4<<8)|1), ((4<<8)|2), ((4<<8)|3), ((4<<8)|4), ((4<<8)|5),
	((5<<8)|0), ((5<<8)|1), ((5<<8)|2), ((5<<8)|3), ((5<<8)|4), ((5<<8)|5),
	((6<<8)|0), ((6<<8)|1), ((6<<8)|2), ((6<<8)|3), ((6<<8)|4), ((6<<8)|5),
	((7<<8)|0), ((7<<8)|1), ((7<<8)|2), ((7<<8)|3), ((7<<8)|4), ((7<<8)|5),
	((8<<8)|0), ((8<<8)|1), ((8<<8)|2), ((8<<8)|3)
};

/*block order map from decoder order to context cache order*/
DECLARE_ASM_CONST (4, uint8, g_blk_order_map_tbl[16+2 * 4]) = 
{
    	1 *12+4, 1 *12+5, 2 *12+4, 2 *12+5,  //first one block8x8
	1 *12+6, 1 *12+7, 2 *12+6, 2 *12+7, 
	3 *12+4, 3 *12+5, 4 *12+4, 4 *12+5, 
	3 *12+6, 3 *12+7, 4 *12+6, 4 *12+7, 

	6 *12+4, 6 *12+5, 7 *12+4, 7 *12+5,  //U's 4 block4x4
	6 *12+8, 6 *12+9, 7 *12+8, 7 *12+9, 
};

DECLARE_ASM_CONST (4, uint32, g_huff_tab_token [69]) = 
{
	0x0041c701, 0x4100c641, 0x8200c500, 0x0000c400, 0x42c4c302, 0x01c38242, 0xc3824182, 0xc3820000, 
	0xc5c64503, 0x83838543, 0xc4434483, 0xc40184c3, 0xc6c54304, 0x84c5c844, 0x43428384, 0x024242c4, 
	0xc7c70305, 0x85848745, 0x44444785, 0x030202c5, 0xc8c8c906, 0x86858646, 0x45454686, 0x040301c6, 
	0xc9050707, 0x87860647, 0x46468987, 0x050405c7, 0x00c9ca08, 0x00878848, 0x00474888, 0x000604c8, 
	0x08cbcc09, 0x89898b49, 0x48494a89, 0x070809c9, 0xcacacb0a, 0x88888a4a, 0x4748498a, 0x060708ca, 
	0xcc0b0c0b, 0x8b8b8d4b, 0x4a4b4c8b, 0x0a0a0bcb, 0xcbcccd0c, 0x8a8a8c4c, 0x494a4b8c, 0x09090acc, 
	0xcece4f0d, 0x8d8d0e4d, 0x4c4dce8d, 0x0c0d8ecd, 0xcdcd4e0e, 0x8c8c0d4e, 0x4b4c4d8e, 0x0b0c4dce, 
	0xd04f500f, 0x8f0f0f4f, 0x4f8fcf8f, 0x0e4e8fcf, 0xcf8ed010, 0x8e8e9050, 0x4e0e1090, 0x0d0e00d0, 
	0x10d00000, 0x90900000, 0x50500000, 0x0f100000, 0x4dcf0000
};

DECLARE_ASM_CONST (4, uint16, g_incVlc_tbl[7]) = 
{
	0,	3,	6,	12,	24,	48,	32768
};

DECLARE_ASM_CONST (4, int8, g_total_zero_len_tbl[16]) = 
{
	0, 9, 6, 6, 5, 5, 6, 6, 6, 6, 5, 4, 4, 3, 2, 1
};

DECLARE_ASM_CONST (4, int8, g_run_before_len_tbl[16]) = 
{
	-1, 1, 2, 2, 3, 3, 3, 11, 11, 11, 11, 11, 11, 11, 11, 11
};

/*vlc table*/


/*chroma DC trailing one and total coeff table*/
DECLARE_ASM_CONST (4, uint16, g_coeffToken_chroma_0_tbl[8]) = 
{
	0xffff, (3<<8)|(2<<2)|2, (2<<8)|(0<<2)|0, (2<<8)|(0<<2)|0, (1<<8)|(1<<2)|1, (1<<8)|(1<<2)|1, (1<<8)|(1<<2)|1, (1<<8)|(1<<2)|1, 
};

DECLARE_ASM_CONST (4, uint16, g_coeffToken_chroma_1_tbl[32]) = 
{
	(7<<8)|(4<<2)|3, (7<<8)|(4<<2)|3, (8<<8)|(4<<2)|2, (8<<8)|(4<<2)|1, (7<<8)|(3<<2)|2, (7<<8)|(3<<2)|2, (7<<8)|(3<<2)|1, (7<<8)|(3<<2)|1, 
	(6<<8)|(4<<2)|0, (6<<8)|(4<<2)|0, (6<<8)|(4<<2)|0, (6<<8)|(4<<2)|0, (6<<8)|(3<<2)|0, (6<<8)|(3<<2)|0, (6<<8)|(3<<2)|0, (6<<8)|(3<<2)|0, 
	(6<<8)|(2<<2)|0, (6<<8)|(2<<2)|0, (6<<8)|(2<<2)|0, (6<<8)|(2<<2)|0, (6<<8)|(3<<2)|3, (6<<8)|(3<<2)|3, (6<<8)|(3<<2)|3, (6<<8)|(3<<2)|3, 
	(6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(2<<2)|1, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, (6<<8)|(1<<2)|0, 
};


/* total zeros table
	left four bit is number of total zeros, 
	right most 4 bits is bits of the syntax
    0x(totalZeros)|(nBits)
*/
/*total coefficient is 1*/
/*total zeros between 0 and 6*/
DECLARE_ASM_CONST (4, uint8, g_totZero1_0_tbl[32]) = 
{
	  0xff,   0xff, 0x65, 0x55, 0x44, 0x44, 0x34, 0x34, 0x23, 0x23, 0x23, 0x23, 0x13, 0x13, 0x13, 0x13, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
};
/*total zeros between 7 and 15*/
DECLARE_ASM_CONST (4, uint8, g_totZero1_1_tbl[32]) = 
{
	   0xff, 0xf9, 0xe9, 0xd9, 0xc8, 0xc8, 0xb8, 0xb8, 0xa7, 0xa7, 0xa7, 0xa7, 0x97, 0x97, 0x97, 0x97, 
	 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 
};

/*total coefficient is 2*/
DECLARE_ASM_CONST (4, uint8, g_totZero2_0_tbl[16]) = 
{
	0xff, 0xff, 0x84, 0x74, 0x64, 0x54, 0x43, 0x43, 0x33, 0x33, 0x23, 0x23, 0x13, 0x13, 0x03, 0x03,
};

DECLARE_ASM_CONST (4, uint8, g_totZero2_1_tbl[8]) = 
{
	0xe6, 0xd6, 0xc6, 0xb6, 0xa5, 0xa5, 0x95, 0x95
};

/*total coefficient is 3*/
DECLARE_ASM_CONST (4, uint8, g_totZero3_0_tbl[16]) = 
{
	0xff, 0xff, 0x84, 0x54, 0x44, 0x04, 0x73, 0x73, 0x63, 0x63, 0x33, 0x33, 0x23, 0x23, 0x13, 0x13, 
};

DECLARE_ASM_CONST (4, uint8, g_totZero3_1_tbl[8]) = 
{
	0xd6, 0xb6, 0xc5, 0xc5, 0xa5, 0xa5, 0x95, 0x95, 
};

/*total coefficient is 4*/
DECLARE_ASM_CONST (4, uint8, g_totZero4_0_tbl[32]) = 
{
	0xc5, 0xb5, 0xa5, 0x05, 0x94, 0x94, 0x74, 0x74, 0x34, 0x34, 0x24, 0x24, 0x83, 0x83, 0x83, 0x83, 
	0x63, 0x63, 0x63, 0x63, 0x53, 0x53, 0x53, 0x53, 0x43, 0x43, 0x43, 0x43, 0x13, 0x13, 0x13, 0x13,
};

/*total coefficient is 5*/
DECLARE_ASM_CONST (4, uint8, g_totZero5_0_tbl[32]) = 
{
	0xb5, 0x95, 0xa4, 0xa4, 0x84, 0x84, 0x24, 0x24, 0x14, 0x14, 0x04, 0x04, 0x73, 0x73, 0x73, 0x73,
	0x63, 0x63, 0x63, 0x63, 0x53, 0x53, 0x53, 0x53, 0x43, 0x43, 0x43, 0x43, 0x33, 0x33, 0x33, 0x33,
};

/*total coefficient is 6*/
DECLARE_ASM_CONST (4, uint8, g_totZero6_0_tbl[16]) = 
{
	0xff, 0x84, 0x93, 0x93, 0x73, 0x73, 0x63, 0x63, 0x53, 0x53, 0x43, 0x43, 0x33, 0x33, 0x23, 0x23, 
};

DECLARE_ASM_CONST (4, uint8, g_totZero6_1_tbl[4]) =
{
	0xa6, 0x06, 0x15, 0x15,
};

/*total coefficient is 7*/
DECLARE_ASM_CONST (4, uint8, g_totZero7_0_tbl[16]) = 
{
	0xff, 0x74, 0x83, 0x83, 0x63, 0x63, 0x43, 0x43, 0x33, 0x33, 0x23, 0x23, 0x52, 0x52, 0x52, 0x52,
};

DECLARE_ASM_CONST (4, uint8, g_totZero7_1_tbl[4]) = 
{
	0x96, 0x06, 0x15, 0x15
};

/*total coefficient is 8*/
DECLARE_ASM_CONST (4, uint8, g_totZero8_0_tbl[16]) = 
{
	0xff, 0x14, 0x73, 0x73, 0x63, 0x63, 0x33, 0x33, 0x52, 0x52, 0x52, 0x52, 0x42, 0x42, 0x42, 0x42,
};

DECLARE_ASM_CONST (4, uint8, g_totZero8_1_tbl[4]) = 
{
	0x86, 0x06, 0x25, 0x25, 
};

/*total coefficient is 9*/
DECLARE_ASM_CONST (4, uint8, g_totZero9_0_tbl[16]) =
{
	0xff, 0x24, 0x53, 0x53, 0x62, 0x62, 0x62, 0x62, 0x42, 0x42, 0x42, 0x42, 0x32, 0x32, 0x32, 0x32, 
};

DECLARE_ASM_CONST (4, uint8, g_totZero9_1_tbl[4]) = 
{
	0x16, 0x06, 0x75, 0x75,
};

/*total coefficient is 10*/
DECLARE_ASM_CONST (4, uint8, g_totZero10_0_tbl[8]) = 
{
	0xff, 0x23, 0x52, 0x52, 0x42, 0x42, 0x32, 0x32, 
};

DECLARE_ASM_CONST (4, uint8, g_totZero10_1_tbl[4]) = 
{
	0x15, 0x05, 0x64, 0x64, 
};

/*total coefficient is 11*/
DECLARE_ASM_CONST (4, uint8, g_totZero11_0_tbl[16]) = 
{
	0x04, 0x14, 0x23, 0x23, 0x33, 0x33, 0x53, 0x53, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 
};

/*total coefficient is 12*/
DECLARE_ASM_CONST (4, uint8, g_totZero12_0_tbl[16]) = 
{
	0x04, 0x14, 0x43, 0x43, 0x22, 0x22, 0x22, 0x22, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 
};

/*total coefficient is 13*/
DECLARE_ASM_CONST (4, uint8, g_totZero13_0_tbl[8]) = 
{
	0x03, 0x13, 0x32, 0x32, 0x21, 0x21, 0x21, 0x21,
};

/*total coefficient is 14*/
DECLARE_ASM_CONST (4, uint8, g_totZero14_0_tbl[4]) = 
{
	0x02, 0x12, 0x21, 0x21,
};

/*total coefficient is 15*/
DECLARE_ASM_CONST (4, uint8, g_totZero15_0_tbl[2]) = 
{
	0x01, 0x11,
};

/*total coefficient for chrominance DC*/
DECLARE_ASM_CONST (4, uint8, g_totZero_Chroma_DC1_tbl[8]) = 
{         //total coeff is 1
	0x33, 0x23, 0x12, 0x12, 0x01, 0x01, 0x01, 0x01, 
};

DECLARE_ASM_CONST (4, uint8, g_totZero_Chroma_DC2_tbl[4]) = 
{         //total coeff is 2
	0x22, 0x12, 0x01, 0x01,
};

DECLARE_ASM_CONST (4, uint8, g_totZero_Chroma_DC3_tbl[2]) = 
{         //total coeff is 3
	0x11, 0x01
};

/*run-before table*/
/*left most 4 bits is run_before, right most 4 bits is bits of the syntax*/
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft1_tbl[2]) = {0x11, 0x01};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft2_tbl[4]) = {0x22, 0x12, 0x01, 0x01};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft3_tbl[4]) = {0x32, 0x22, 0x12, 0x02};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft4_tbl[8]) = {0x43, 0x33, 0x22, 0x22, 0x12, 0x12, 0x02, 0x02};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft5_tbl[8]) = {0x53, 0x43, 0x33, 0x23, 0x12, 0x12, 0x02, 0x02};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeft6_tbl[8]) = {0x13, 0x23, 0x43, 0x33, 0x63, 0x53, 0x02, 0x02};
DECLARE_ASM_CONST (4, uint8, g_run_zeroLeftGt6_tbl[8]) = {0xff, 0x63, 0x53, 0x43, 0x33, 0x23, 0x13, 0x03};

DECLARE_ASM_CONST (4, int32, g_msk[33]) =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 