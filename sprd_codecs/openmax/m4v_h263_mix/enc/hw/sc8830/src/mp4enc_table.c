/******************************************************************************
 ** File Name:    Mp4Enc_table_internal.c                                   *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
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

const uint32 g_msk[33] =
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


uint32 g_vlc_hw_tbl [320*2*2] =
{
//#if 0 shark/dophin
    0x00000000, 0x80038003,
    0xc004f005, 0xf0055407,
    0x68062e08, 0x60061f09,
    0x5407128a, 0x4c07120a,
    0x4807084b, 0x2e08080b,
    0x1f0900ec, 0x1e0900cc,
    0x1d09040c, 0x128a8003,
    0x120ac004, 0x118ae005,
    0x108a6806, 0x084b6006,
    0x080b5806, 0x03cb4c07,
    0x038b4807, 0x00ec4407,
    0x00cc4007, 0x040c2c08,
    0x042c2a08, 0x050d2808,
    0x051d1c09, 0x052d1b09,
    0x0000108a, 0x7005100a,
    0x30070f8a, 0x16090f0a,
    0x0b8a0e8a, 0x018b0e0a,
    0x00ac0d8a, 0x008c0d0a,
    0x059d044c, 0x8003046c,
    0xe005056d, 0x5806057d,
    0x44077005, 0x40073c07,
    0x34073807, 0x24083407,
    0x28083007, 0x19092608,
    0x18092408, 0x17092208,
    0x0c8a2008, 0x0c0a1a09,
    0x01cb1909, 0x058d1809,
    0x70051709, 0x3c071609,
    0x38071509, 0x22081409,
    0x20081309, 0x26080c0a,
    0x15090b8a, 0x14090b0a,
    0x13090a8a, 0x1a090a0a,
    0x0a8a098a, 0x0a0a090a,
    0x098a088a, 0x090a01cb,
    0x088a018b, 0x04cc014b,
    0x04ec010b, 0x05cd048c,
    0x05dd04ac, 0x05ed04cc,
    0x05fd04ec, 0xc004058d,
    0x5007059d, 0x2a0805ad,
    0x1b0905bd, 0x110a05cd,
    0x0e0a05dd, 0x0d8a05ed,
    0x0d0a05fd, 0x024b0000,
    0x046cc004, 0x30075007,
    0x0b0a1e09, 0x010b03cb,
    0x048c042c, 0x04ac050d,
    0x05adf005, 0x05bd5007,
    0xf0051d09, 0x2c08118a,
    0x0f0a110a, 0x0e8a030b,
    0x028b02cb, 0x020b028b,
    0x054d024b, 0x057d020b,
    0x0000055d, 0xe0055407,
    0x50071e09, 0x2c08038b,
    0x1c09034b, 0x100a052d,
    0x0f8a053d, 0x034b054d,
    0x044c0000, 0x053de005,
    0x055d1d09, 0x0000038b,
    0x5806051d, 0x2a080000,
    0x0f0a7005, 0x030b0c8a,
    0x056d00ac, 0x00000000,
    0x44073c07, 0x1b09008c,
    0x0e8a0000, 0x02cb0000,
    0x16090000, 0x014b0000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0xf0310800, 0xd00a000b,
    0xf00a0000, 0xf0310800,
    0xe000005a, 0x70310000,
    0x300000b1, 0x70151010,
    0x30000041, 0x70100810,
    0xdc000109, 0xf0151003,
    0xf0151004, 0xe0000011,
    0xf0340001, 0x70000001,
    0xdc00000b, 0xf0150e05,
    0xe000005a, 0x70151000,
    0x30000041, 0x70111810,
    0xdc111932, 0xf0151001,
    0xf0151004, 0xe000001c,
    0xd00b1130, 0xe000001c,
    0xf0151401, 0xf0310e01,
    0x70321000, 0x300000c1,
    0x70505810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70575810, 0xdc0b1133,
    0xe0000048, 0xf0151602,
    0x70321000, 0x300000c1,
    0x70505810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70575810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70905810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70975810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70d05810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x70d75810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x71105810, 0xdc0b1133,
    0x70321000, 0x300000c1,
    0x71175810, 0xdc0b1133,
    0x70190000, 0x30000011,
    0x70180010, 0x30000011,
    0x70170010, 0x30000051,
    0x701a2010, 0x30000061,
    0x70102810, 0x70340004,
    0x30000075, 0x70063014,
    0x30000015, 0xf0151402,
    0x70001015, 0xdc00010f,
    0xe000005a, 0x00000000,
    0xf0140001, 0x50000000,
    0xdc00010e, 0x50000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x001c0002, 0x00000000,
    0x001c0005, 0x001c0005,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x001c000e,
    0x001c000e, 0x00000000,
    0x001c0011, 0x00000000,
    0x00000000, 0x001c0013,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x001c001a,
    0x001c001a, 0x00000000,
    0x00000000, 0x00000000,
    0x001c0027, 0x001c0027,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x001c0048,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x001c0057,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x001c005c, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
//#else tshark
    0xf0310800, 0x001c0002,
    0xd00a000b, 0x00000000,
    0xf00a0000, 0x001c0005,
    0xf0310800, 0x001c0005,
    0xe000005a, 0x00000000,
    0x70310000, 0x00000000,
    0x300000b1, 0x00000000,
    0x70151010, 0x00000000,
    0x30000041, 0x00000000,
    0x70100810, 0x00000000,
    0xdc000109, 0x00000000,
    0xf0151003, 0x001c000e,
    0xf0151004, 0x001c000e,
    0xe0000011, 0x00000000,
    0xf0340001, 0x001c0011,
    0x70000001, 0x00000000,
    0xdc00000b, 0x00000000,
    0xf0150e05, 0x001c0013,
    0xe000005a, 0x00000000,
    0x70151000, 0x00000000,
    0x30000041, 0x00000000,
    0x70111810, 0x00000000,
    0xdc111932, 0x00000000,
    0xf0151001, 0x001c001a,
    0xf0151004, 0x001c001a,
    0xe000001c, 0x00000000,
    0xd00b1130, 0x00000000,
    0xe000001c, 0x00000000,
    0xf0151401, 0x001c0027,
    0xf0310e01, 0x001c0027,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70505810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70575810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0xe0000048, 0x00000000,
    0xf0151602, 0x001c0048,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70505810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70575810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70905810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70975810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70d05810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x70d75810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x71105810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70321000, 0x00000000,
    0x300000c1, 0x00000000,
    0x71175810, 0x00000000,
    0xdc0b1133, 0x00000000,
    0x70190000, 0x00000000,
    0x30000011, 0x00000000,
    0x70180010, 0x00000000,
    0x30000011, 0x00000000,
    0x70170010, 0x00000000,
    0x30000051, 0x00000000,
    0x701a2010, 0x00000000,
    0x30000061, 0x00000000,
    0x70102810, 0x00000000,
    0x70340004, 0x00000000,
    0x30000075, 0x00000000,
    0x70063014, 0x00000000,
    0x30000015, 0x00000000,
    0xf0151402, 0x001c0057,
    0x70001015, 0x00000000,
    0xdc00010f, 0x00000000,
    0xe000005a, 0x00000000,
    0x00000000, 0x00000000,
    0xf0140001, 0x001c005c,
    0x50000000, 0x00000000,
    0xdc00010e, 0x00000000,
    0x50000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x80038003,
    0xc004f005, 0xf0055407,
    0x68062e08, 0x60061f09,
    0x5407128a, 0x4c07120a,
    0x4807084b, 0x2e08080b,
    0x1f0900ec, 0x1e0900cc,
    0x1d09040c, 0x128a8003,
    0x120ac004, 0x118ae005,
    0x108a6806, 0x084b6006,
    0x080b5806, 0x03cb4c07,
    0x038b4807, 0x00ec4407,
    0x00cc4007, 0x040c2c08,
    0x042c2a08, 0x050d2808,
    0x051d1c09, 0x052d1b09,
    0x0000108a, 0x7005100a,
    0x30070f8a, 0x16090f0a,
    0x0b8a0e8a, 0x018b0e0a,
    0x00ac0d8a, 0x008c0d0a,
    0x059d044c, 0x8003046c,
    0xe005056d, 0x5806057d,
    0x44077005, 0x40073c07,
    0x34073807, 0x24083407,
    0x28083007, 0x19092608,
    0x18092408, 0x17092208,
    0x0c8a2008, 0x0c0a1a09,
    0x01cb1909, 0x058d1809,
    0x70051709, 0x3c071609,
    0x38071509, 0x22081409,
    0x20081309, 0x26080c0a,
    0x15090b8a, 0x14090b0a,
    0x13090a8a, 0x1a090a0a,
    0x0a8a098a, 0x0a0a090a,
    0x098a088a, 0x090a01cb,
    0x088a018b, 0x04cc014b,
    0x04ec010b, 0x05cd048c,
    0x05dd04ac, 0x05ed04cc,
    0x05fd04ec, 0xc004058d,
    0x5007059d, 0x2a0805ad,
    0x1b0905bd, 0x110a05cd,
    0x0e0a05dd, 0x0d8a05ed,
    0x0d0a05fd, 0x024b0000,
    0x046cc004, 0x30075007,
    0x0b0a1e09, 0x010b03cb,
    0x048c042c, 0x04ac050d,
    0x05adf005, 0x05bd5007,
    0xf0051d09, 0x2c08118a,
    0x0f0a110a, 0x0e8a030b,
    0x028b02cb, 0x020b028b,
    0x054d024b, 0x057d020b,
    0x0000055d, 0xe0055407,
    0x50071e09, 0x2c08038b,
    0x1c09034b, 0x100a052d,
    0x0f8a053d, 0x034b054d,
    0x044c0000, 0x053de005,
    0x055d1d09, 0x0000038b,
    0x5806051d, 0x2a080000,
    0x0f0a7005, 0x030b0c8a,
    0x056d00ac, 0x00000000,
    0x44073c07, 0x1b09008c,
    0x0e8a0000, 0x02cb0000,
    0x16090000, 0x014b0000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
//#endif
};
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

