/******************************************************************************
 ** File Name:    h264enc_table.c                                             *
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
#include "h264enc_video_header.h"
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

uint32 g_vlc_hw_tbl [406*2] =
{
0x060b0605,0x0600060f,
0x02020201,0x0601040e,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x06070807,0x0604060b,
0x05070604,0x0605050f,
0x03030301,0x0606040d,
0x00000000,0x00000000,
0x07070907,0x06080608,
0x060a0806,0x0609050c,
0x06090705,0x060a050e,
0x04050503,0x060b040c,
0x08070a07,0x060c070f,
0x06060906,0x060d050a,
0x06050805,0x060e050b,
0x04040603,0x060f040b,
0x08040b07,0x0610070b,
0x07060a06,0x06110508,
0x07050905,0x06120509,
0x05060704,0x0613040a,
0x09070d0f,0x06140709,
0x08060b06,0x0615060e,
0x08050a05,0x0616060d,
0x06080804,0x06170409,
0x0b0f0d0b,0x06180708,
0x09060d0e,0x0619060a,
0x09050b05,0x061a0609,
0x06040904,0x061b0408,
0x0b0b0d08,0x061c080f,
0x0b0e0d0a,0x061d070e,
0x0b0d0d0d,0x061e070d,
0x07040a04,0x061f050d,
0x0c0f0e0f,0x0620080b,
0x0b0a0e0e,0x0621080e,
0x0b090d09,0x0622070a,
0x09040b04,0x0623060c,
0x0c0b0e0b,0x0624090f,
0x0c0e0e0a,0x0625080a,
0x0c0d0e0d,0x0626080d,
0x0b0c0d0c,0x0627070c,
0x0c080f0f,0x0628090b,
0x0c0a0f0e,0x0629090e,
0x0c090e09,0x062a0809,
0x0b080e0c,0x062b080c,
0x0d0f0f0b,0x062c0908,
0x0d0e0f0a,0x062d090a,
0x0d0d0f0d,0x062e090d,
0x0c0c0e08,0x062f0808,
0x0d0b100f,0x06300a0d,
0x0d0a0f01,0x06310907,
0x0d090f09,0x06320909,
0x0d0c0f0c,0x0633090c,
0x0d07100b,0x06340a09,
0x0e0b100e,0x06350a0c,
0x0d06100d,0x06360a0b,
0x0d080f08,0x06370a0a,
0x0e091007,0x06380a05,
0x0e08100a,0x06390a08,
0x0e0a1009,0x063a0a07,
0x0d01100c,0x063b0a06,
0x0e071004,0x063c0a01,
0x0e061006,0x063d0a04,
0x0e051005,0x063e0a03,
0x0e041008,0x063f0a02,
0x45371100,0x61614553,
0x40516161,0x10203040,
0x37363300,0x51514437,
0x41506041,0x11213141,
0x36353200,0x35374345,
0x31314151,0x00111121,
0x35344300,0x34363744,
0x32232333,0x00002111,
0x44334200,0x33353636,
0x11222223,0x00000031,
0x43455300,0x23343535,
0x33213122,0x00000000,
0x34445200,0x32333434,
0x00412132,0x00000000,
0x33436300,0x41323343,
0x00005131,0x00000000,
0x42426200,0x31414233,
0x00000060,0x00000000,
0x53537300,0x60315142,
0x00000000,0x00000000,
0x52527200,0x00604152,
0x00000000,0x00000000,
0x61638300,0x00005051,
0x00000000,0x00000000,
0x51628200,0x00000050,
0x00000000,0x00000000,
0x60619300,0x00000000,
0x00000000,0x00000000,
0x00609200,0x00000000,
0x00000000,0x00000000,
0x00009100,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00001011,0x00000000,
0x00000000,0x00000000,
0x00202111,0x00000000,
0x00000000,0x00000000,
0x20212223,0x00000000,
0x00000000,0x00000000,
0x31212223,0x00000030,
0x00000000,0x00000000,
0x32332223,0x00003031,
0x00000000,0x00000000,
0x33313023,0x00343532,
0x00000000,0x00000000,
0x34353637,0x41313233,
0x81716151,0x00b1a191,
0x42426200,0x31414233,
0x00000060,0x00000000,
0x53537300,0x60315142,
0x00000000,0x00000000,
0x52527200,0x00604152,
0x00000000,0x00000000,
0x61638300,0x00005051,
0x00000000,0x00000000,
0x51628200,0x00000050,
0x00000000,0x00000000,
0x60619300,0x00000000,
0x00000000,0x00000000,
0x00609200,0x00000000,
0x00000000,0x00000000,
0x00009100,0x00000000,
0x00000000,0x00000000,
0x01010607,0x00000000,
0x06060604,0x00000301,
0x07030603,0x06050702,
0x08030602,0x07000802,
0x06070807,0x0604060b,
0x05070604,0x0605050f,
0x03030301,0x0606040d,
0x00000000,0x00000000,
0x07070907,0x06080608,
0x060a0806,0x0609050c,
0x06090705,0x060a050e,
0x04050503,0x060b040c,
0x08070a07,0x060c070f,
0x06060906,0x060d050a,
0x06050805,0x060e050b,
0x04040603,0x060f040b,
0x11111100,0x10212100,
0x00203100,0x00003000,
0x00001011,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x70152808,0xf0310800,
0xf00a0000,0xf00a06ff,
0x67fffff0,0x67fffff0,
0x602ffff0,0x80000001,
0xf0140000,0xf0140000,
0x601ffff0,0x80000005,
0xe0000009,0xdc1a0830,
0x70000001,0x90000001,
0x70000005,0xe0000072,
0xf0140000,0x601ffff0,
0x80000005,0xe0000012,
0xdc1a0830,0x90000001,
0x70000001,0xf0152804,
0x6fb00ff8,0xe0000028,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x70004005,0xd0152834,
0xd0152830,0xf0152c18,
0xf0152e00,0xf0130000,
0xd413000b,0xc0000001,
0xc0009c17,0xc0020001,
0x90000000,0xf4000010,
0x70000002,0xdc00000b,
0xf4008600,0x70000003,
0xdc00001b,0xe8000033,
0x80000000,0xb0000020,
0xb0008080,0xd0280808,
0xe0000050,0xd0506861,
0xd0576061,0xe0000050,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x70000001,0xf0152c18,
0xf0152e00,0x70001001,
0x30000061,0x70102810,
0xdc000035,0xf0101800,
0xf0130000,0xf0152800,
0xd013000b,0xf0102e00,
0xf0152c18,0xf0152800,
0xd00b2831,0xe000006e,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x70000005,0xf0152c18,
0xf0152800,0x70001005,
0x70023008,0x30000079,
0x70063018,0xf00a0000,
0xf0140000,0x70040001,
0xe000007c,0x70140000,
0x30000061,0x70102810,
0xd00b0034,0xe000407e,
0x70000009,0x70000009,
0xf00a0000,0xf0140000,
0x70000001,0xe00040be,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0xf0128000,0x70019001,
0x70002005,0xd00b2834,
0x7001a001,0xd00b2834,
0xf0120800,0x70010001,
0x70001005,0x90000000,
0xf0000008,0xd00b2834,
0x601ffff0,0xe00040a0,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x70004005,0x6fb00ff8,
0xd0152834,0xd0152830,
0xd800100b,0xd800100b,
0xd800100b,0xd800100b,
0xd0506861,0xd0576061,
0xd0906861,0xd0976061,
0xd0d06861,0xd0d76061,
0xd1106861,0xd1176061,
0xe0004050,0x00000000,
0x00000000,0x00000000,
0xf0140001,0x50000000,
0xdc00010e,0x50000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x001c0028,
0x001c0012,0x401c0007,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x001c0010,0x401c000d,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x401c0016,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x001c00aa,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x601c003f,
0x601c003d,0x601c002f,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x201c003d,
0x00000000,0x00000000,
0x901c0039,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x901c0039,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x601c0055,
0x601c005b,0x00000000,
0x00000000,0x00000000,
0x00000000,0x001c005b,
0x001c005b,0x601c005b,
0x00000000,0x001c005e,
0x601c006e,0x601c006e,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x601c0072,
0x601c0072,0x00000000,
0x00000000,0x00000000,
0x00000000,0x001c0079,
0x001c007c,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x001c0082,0x001c00be,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x001c009c,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x001c00be,0x00000000,
0x00000000,0x00000000,
0x211c00be,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x011c00c0,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
0x00000000,0x00000000,
};

uint32 g_skipBlock_QP_table[52] =
{
0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /* 0-9*/
0,   0,   0,   0,   0,   0, 256, 256, 256, 256, /*10-19*/
256, 256, 256, 256, 256, 256, 512, 512, 512, 768, /*20-29*/
768, 768, 768, 768, 768,1024,1024,1024,1280,1280, /*30-39*/
1280,1280,1280,1280,1280,1280,1280,1280,1280,1280, /*40-49*/
1536,1536,                                         /*50-51*/
};

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
