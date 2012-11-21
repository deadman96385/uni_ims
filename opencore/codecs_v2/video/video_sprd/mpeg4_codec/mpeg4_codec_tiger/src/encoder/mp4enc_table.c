/******************************************************************************
 ** File Name:    Mp4Enc_table_internal.c                                   *
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
#include "sci_types.h"
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
#define DECLARE_ASM_CONST(n,t,v)    const t v  __attribute__ ((aligned (n)))

DECLARE_ASM_CONST (4, uint32, g_lambda[32]) =
{
	 0,  1,  2,  3,  4,  5,  6,  7,
	 8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 
	24, 25, 26, 27, 28, 29, 30, 31	
};

DECLARE_ASM_CONST (4, uint32, g_msk[33]) =
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

DECLARE_ASM_CONST (4, uint16, g_quant_pa[32][2]) = 
{
	{0, 0x000}, {0, 0x800}, {1, 0x800}, {2, 0xaab}, {2, 0x800}, {3, 0xccd}, 
	{3, 0xaab}, {3, 0x925},	{3, 0x800}, {4, 0xe39}, {4, 0xccd}, {4, 0xba3}, 
	{4, 0xaab}, {4, 0x9d9}, {4, 0x925}, {4, 0x889},	{4, 0x800}, {5, 0xf0f},
	{5, 0xe39}, {5, 0xd79}, {5, 0xccd}, {5, 0xc31}, {5, 0xba3}, {5, 0xb21},
	{5, 0xaab}, {5, 0xa3d}, {5, 0x9d9}, {5, 0x97b}, {5, 0x925}, {5, 0x8d4}, 
	{5, 0x889}, {5, 0x842},
};

DECLARE_ASM_CONST (4, uint16, g_DC_scaler[47][2]) = 
{
	{0, 0,}, {0, 0,}, {0, 0,}, {0, 0,}, {0, 0,}, {0, 0,}, {0, 0,}, {0, 0,},
	{2, 0x800}, {3, 0xe39}, {3, 0xccd}, {3, 0xba3}, {3, 0xaab}, {3, 0x9d9},
	{3, 0x925}, {3, 0x889},	{3, 0x800}, {4, 0xf0f}, {4, 0xe39}, {4, 0xd79}, 
	{4, 0xccd}, {4, 0xc31}, {4, 0xba3}, {4, 0xb21},	{4, 0xaab}, {4, 0xa3d}, 
	{4, 0x9d9}, {4, 0x97b}, {4, 0x925}, {4, 0x8d4}, {4, 0x889}, {4, 0x842},
	{4, 0x800}, {5, 0xf84}, {5, 0xf0f}, {5, 0xea1}, {5, 0xe39}, {5, 0xdd6},
	{5, 0xd79}, {5, 0xd21},	{5, 0xccd}, {5, 0xc7d}, {5, 0xc31}, {5, 0xbe8}, 
	{5, 0xba3}, {5, 0xb61}, {5, 0xb21},	
};

/*MOMUSYS VLC TABLE*/
DECLARE_ASM_CONST (4, MCBPC_TABLE_CODE_LEN_T, g_mcbpc_intra_tab[15]) =
{
	{0x01,9}, {0x01,1}, {0x01,4}, {0x00,0},	{0x00,0}, {0x01,3}, {0x01,6}, 
	{0x00,0}, {0x00,0}, {0x02,3}, {0x02,6}, {0x00,0}, {0x00,0}, {0x03,3},
	{0x03,6}
};

DECLARE_ASM_CONST (4, MCBPC_TABLE_CODE_LEN_T, g_cbpy_tab[16]) =
{
	{3,4}, {5,5}, {4,5}, {9,4}, {3,5}, {7,4}, {2,6}, {11,4},
	{2,5}, {3,6}, {5,4}, {10,4}, {4,4}, {8,4}, {6,4}, {3,2}
};

DECLARE_ASM_CONST (4, MCBPC_TABLE_CODE_LEN_T, g_mcbpc_inter_tab[29]) =
{
	{1,1}, {3,3}, {2,3}, {3,5}, {4,6}, {1,9}, {0,0}, {0,0},	{3,4}, {7,7}, 
	{5,7}, {4,8}, {4,9}, {0,0}, {0,0}, {0,0}, {2,4}, {6,7}, {4,7}, {3,8}, 
	{3,9}, {0,0}, {0,0}, {0,0},	{5,6}, {5,9}, {5,8}, {3,7}, {2,9}
};

DECLARE_ASM_CONST (4, MV_TABLE_CODE_LEN_T, g_mv_tab[33]) =
{
	{1,1}, {1,2}, {1,3}, {1,4}, {3,6}, {5,7}, {4,7}, {3,7},	{11,9}, {10,9}, 
	{9,9}, {17,10}, {16,10}, {15,10}, {14,10}, {13,10},	{12,10}, {11,10}, 
	{10,10}, {9,10}, {8,10}, {7,10}, {6,10}, {5,10},{4,10}, {7,11}, {6,11}, 
	{5,11}, {4,11}, {3,11}, {2,11}, {3,12},	{2,12}
};


DECLARE_ASM_CONST (4, uint32, g_mp4_enc_huff_tbl[128]) = 
{
	0x00000000, 0x80038003, 0xc004f005, 0xf0055407, 
	0x68062e08, 0x60061f09, 0x5407128a, 0x4c07120a, 
	0x4807084b, 0x2e08080b, 0x1f0900ec, 0x1e0900cc, 
	0x1d09040c, 0x128a8003, 0x120ac004, 0x118ae005, 
	0x108a6806, 0x084b6006, 0x080b5806, 0x03cb4c07, 
	0x038b4807, 0x00ec4407, 0x00cc4007, 0x040c2c08, 
	0x042c2a08, 0x050d2808, 0x051d1c09, 0x052d1b09, 
	0x0000108a, 0x7005100a, 0x30070f8a, 0x16090f0a, 
	0x0b8a0e8a, 0x018b0e0a, 0x00ac0d8a, 0x008c0d0a, 
	0x059d044c, 0x8003046c, 0xe005056d, 0x5806057d, 
	0x44077005, 0x40073c07, 0x34073807, 0x24083407, 
	0x28083007, 0x19092608, 0x18092408, 0x17092208, 
	0x0c8a2008, 0x0c0a1a09, 0x01cb1909, 0x058d1809, 
	0x70051709, 0x3c071609, 0x38071509, 0x22081409, 
	0x20081309, 0x26080c0a, 0x15090b8a, 0x14090b0a, 
	0x13090a8a, 0x1a090a0a, 0x0a8a098a, 0x0a0a090a, 
	0x098a088a, 0x090a01cb, 0x088a018b, 0x04cc014b, 
	0x04ec010b, 0x05cd048c, 0x05dd04ac, 0x05ed04cc, 
	0x05fd04ec, 0xc004058d, 0x5007059d, 0x2a0805ad, 
	0x1b0905bd, 0x110a05cd, 0x0e0a05dd, 0x0d8a05ed, 
	0x0d0a05fd, 0x024b0000, 0x046cc004, 0x30075007, 
	0x0b0a1e09, 0x010b03cb, 0x048c042c, 0x04ac050d, 
	0x05adf005, 0x05bd5007, 0xf0051d09, 0x2c08118a,
	0x0f0a110a, 0x0e8a030b, 0x028b02cb, 0x020b028b, 
	0x054d024b, 0x057d020b, 0x0000055d, 0xe0055407, 
	0x50071e09, 0x2c08038b, 0x1c09034b, 0x100a052d, 
	0x0f8a053d, 0x034b054d, 0x044c0000, 0x053de005, 
	0x055d1d09, 0x0000038b, 0x5806051d, 0x2a080000, 
	0x0f0a7005, 0x030b0c8a, 0x056d00ac, 0x00000000, 
	0x44073c07, 0x1b09008c, 0x0e8a0000, 0x02cb0000, 
	0x16090000, 0x014b0000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000 
};


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 


