/********************************************************************************
**  File Name: 	mp3_bit.c   									                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:  bit process                     						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/

#define MP3_DEC_CHAR_BIT  8
#define MP3_DEC_CRC_POLY  0x8005
#include "mp3_bit.h"

//CRC-check look-up table X^16 + X^15 + X^2 + 1
static uint16 const MP3_DEC_crc_tab[256] = {
  0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
  0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
  0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
  0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
  0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
  0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
  0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
  0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,

  0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
  0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
  0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
  0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
  0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
  0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
  0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,

  0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
  0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
  0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
  0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
  0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
  0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
  0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
  0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,

  0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
  0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
  0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
  0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
  0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
  0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
  0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
  0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202
};




/*****************************************************************/
// 	Description : initialize bit pool pointer struct
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
void MP3_DEC_BitPoolInit(MP3_DEC_BIT_POOL_T *bit_ptr, uint8 const *byte_ptr)
{
    bit_ptr->byte_ptr = byte_ptr;
    bit_ptr->cache    = 0;
    bit_ptr->left     = MP3_DEC_CHAR_BIT;
}


/*****************************************************************/
// 	Description : return length of bits between start and end points
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
uint32 MP3_DEC_CalcBitLen(MP3_DEC_BIT_POOL_T const *begin_ptr,
                          MP3_DEC_BIT_POOL_T const *end_ptr)
{
    return begin_ptr->left + MP3_DEC_CHAR_BIT * (end_ptr->byte_ptr - (begin_ptr->byte_ptr + 1)) + (MP3_DEC_CHAR_BIT - end_ptr->left);
}

/*****************************************************************/
// 	Description : return pointer to next unprocessed byte
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
uint8 const *MP3_DEC_BitNextByte(MP3_DEC_BIT_POOL_T const *bit_pool_ptr)
{
    return bit_pool_ptr->left == MP3_DEC_CHAR_BIT ? bit_pool_ptr->byte_ptr : bit_pool_ptr->byte_ptr + 1;
}



/*****************************************************************/
// 	Description : skip bit and advance bit pointer
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
void MP3_DEC_BitSkip(MP3_DEC_BIT_POOL_T *bit_pool_ptr, 
                     uint32 len)
{
    bit_pool_ptr->byte_ptr += len / MP3_DEC_CHAR_BIT;
    bit_pool_ptr->left -= (uint16)(len % MP3_DEC_CHAR_BIT);
    
    if (bit_pool_ptr->left > MP3_DEC_CHAR_BIT) {
        bit_pool_ptr->byte_ptr++;
        bit_pool_ptr->left += MP3_DEC_CHAR_BIT;
    }
    
    if (bit_pool_ptr->left < MP3_DEC_CHAR_BIT)
        bit_pool_ptr->cache = *bit_pool_ptr->byte_ptr;
}

extern int32 MP3_DEC_BitmaskTable[33];


/*****************************************************************/
// 	Description : read an arbitrary number of bits and return their 8bit value
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
uint32 MP3_DEC_8BitRead(MP3_DEC_BIT_POOL_T *bit_pool_ptr, 
                        uint32 len)
{
    uint32 value;
    
    if (bit_pool_ptr->left == MP3_DEC_CHAR_BIT)
        bit_pool_ptr->cache = *bit_pool_ptr->byte_ptr;
    
    value = bit_pool_ptr->cache & MP3_DEC_BitmaskTable[bit_pool_ptr->left];
    
    if (len < bit_pool_ptr->left)
    {
        value        = value >> (bit_pool_ptr->left - len);
        bit_pool_ptr->left = (uint16) (bit_pool_ptr->left - len);
        return value;
    }
    
    /* remaining bits in current byte */    
    len  -= bit_pool_ptr->left;    
    bit_pool_ptr->byte_ptr++;
    bit_pool_ptr->left = MP3_DEC_CHAR_BIT;
    
    /* more bytes */
    while (len >= MP3_DEC_CHAR_BIT) 
    {
        value = (value << MP3_DEC_CHAR_BIT) | *bit_pool_ptr->byte_ptr++;
        len  -= MP3_DEC_CHAR_BIT;
    }
    
    if (len > 0) 
    {
        bit_pool_ptr->cache = *bit_pool_ptr->byte_ptr;        
        value = (value << len) | (bit_pool_ptr->cache >> (MP3_DEC_CHAR_BIT - len));
        bit_pool_ptr->left = (uint16) (MP3_DEC_CHAR_BIT - len);
    }
    
    return value;
}


#if !defined(ASO_HUFFMAN)

/*****************************************************************/
// 	Description : read an arbitrary number of bits and return their 32bit value
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
//////////////////////////////////////////////////////////////////////////
uint32 MP3_DEC_32BitRead(MP3_DEC_BIT_POOL_T *bit_pool_ptr,
                         uint32 len)
{
    uint32 a;
    uint32 b;
    uint32 value;       
    
    a = (bit_pool_ptr->word_ptr[0]);
    
    BSWAP(a);
    
    if (len <= bit_pool_ptr->left)
    {
        value = (a << ((32 - bit_pool_ptr->left)) >> (32 - len));
        bit_pool_ptr->left -= (uint16) (len);
    }else
    {
        b = (bit_pool_ptr->word_ptr[1]);
        BSWAP(b);
        value = ((a & MP3_DEC_BitmaskTable[bit_pool_ptr->left]) << (len - bit_pool_ptr->left)) | (b >> (32 - (len - bit_pool_ptr->left)));
        bit_pool_ptr->left = (uint16) (32 - (len - bit_pool_ptr->left));
        bit_pool_ptr->word_ptr++;
    }
    return value;
}
#endif


/*****************************************************************/
// 	Description : calculate CRC-check sign word
//	Global resource dependence : 
//   Author: Tan. Li
//	Note:
/*****************************************************************/
uint16 MP3_DEC_BIT_CRCCheck(MP3_DEC_BIT_POOL_T bit_pool, 
                            uint32 len,
                            uint16 init)
{
    uint32 crc;    
    for (crc = init; len >= 32; len -= 32) 
    {
        uint32 data;        
        data = MP3_DEC_8BitRead(&bit_pool, 32);        
        crc = (crc << 8) ^ MP3_DEC_crc_tab[((crc >> 8) ^ (data >> 24)) & 0xff];
        crc = (crc << 8) ^ MP3_DEC_crc_tab[((crc >> 8) ^ (data >> 16)) & 0xff];
        crc = (crc << 8) ^ MP3_DEC_crc_tab[((crc >> 8) ^ (data >>  8)) & 0xff];
        crc = (crc << 8) ^ MP3_DEC_crc_tab[((crc >> 8) ^ (data >>  0)) & 0xff];
    }
    
    switch (len / 8) 
    {
    case 3: crc = (crc << 8) ^
                MP3_DEC_crc_tab[((crc >> 8) ^ MP3_DEC_8BitRead(&bit_pool, 8)) & 0xff];
    case 2: crc = (crc << 8) ^      /*lint !e616 !e825*/
                MP3_DEC_crc_tab[((crc >> 8) ^ MP3_DEC_8BitRead(&bit_pool, 8)) & 0xff];
    case 1: crc = (crc << 8) ^      /*lint !e616 !e825*/
                MP3_DEC_crc_tab[((crc >> 8) ^ MP3_DEC_8BitRead(&bit_pool, 8)) & 0xff];
        
        len %= 8;
        
    case 0: break;    /*lint !e616 !e825*/
    default:break;
    }
    
    while (len--) 
    {
        unsigned int msb;
        
        msb = MP3_DEC_8BitRead(&bit_pool, 1) ^ (crc >> 15);
        
        crc <<= 1;
        if (msb & 1)
            crc ^= MP3_DEC_CRC_POLY;
    }
    
    return (uint16)(crc & 0xffff);
}
