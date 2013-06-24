/*
 *  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license and patent
 *  grant that can be found in the LICENSE file in the root of the source
 *  tree. All contributing project authors may be found in the AUTHORS
 *  file in the root of the source tree.
 */


#include "vp8dec_dboolhuff.h"
#include <stdio.h>
#include "vp8dec_global.h" //derek
//#include "vpx_ports/mem.h"
//#include "vpx_mem/vpx_mem.h"

const unsigned int vp8dx_bitreader_norm[256] =
{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#ifdef SIM_IN_WIN
unsigned char *br_ptr_advance(const unsigned char *_ptr,
                                     unsigned int n)
{
    unsigned int   ptr = (unsigned int )_ptr;

    ptr += n;
 //    ptr &= VP8_BOOL_DECODER_PTR_MASK;

    return (void *)ptr;
}

static void copy_in(BOOL_DECODER *br, unsigned int to_write)
{
    if (to_write > br->user_buffer_sz)
        to_write = br->user_buffer_sz;

    memcpy(br->write_ptr, br->user_buffer, to_write);
    br->user_buffer += to_write;
    br->user_buffer_sz -= to_write;
    br->write_ptr = br_ptr_advance(br->write_ptr, to_write);
}

void * vpx_memalign(size_t align, size_t size)
{
    void *addr,* x = NULL;


    addr = malloc(size + align - 1 + sizeof(size_t));


    if (addr)
    {
        x = align_addr((unsigned char *)addr + sizeof(size_t), (int)align);
        /* save the actual malloc address */
        ((size_t *)x)[-1] = (size_t)addr;
    }

    return x;
}
#endif	// SIM_IN_WIN


int vp8dx_start_decode_c(BOOL_DECODER *br, const unsigned char *source,
                        unsigned int source_sz)
{
//    br->lowvalue = 0;
    br->range    = 255;
    br->count    = 0;
#ifdef SIM_IN_WIN
    br->user_buffer    = source;
    br->user_buffer_sz = source_sz;

    if (source_sz && !source)
        return 1;

    /* Allocate the ring buffer backing store with alignment equal to the
     * buffer size*2 so that a single pointer can be used for wrapping rather
     * than a pointer+offset.
     */
    br->decode_buffer  = (unsigned char *)malloc(VP8_BOOL_DECODER_SZ);//vpx_memalign(VP8_BOOL_DECODER_SZ * 2, VP8_BOOL_DECODER_SZ);
//	br->decode_buffer  = vpx_memalign(VP8_BOOL_DECODER_SZ * 2,
//                                      VP8_BOOL_DECODER_SZ);

    if (!br->decode_buffer)
        return 1;

    /* Populate the buffer */
    br->read_ptr = br->decode_buffer;
    br->write_ptr = br->decode_buffer;
    copy_in(br, VP8_BOOL_DECODER_SZ);

    /* Read the first byte */
    br->value = (*br->read_ptr++) << 8;
	if(or1200_print)
		BitstreamReadBits(8);
#else
	br->value = (BitstreamReadBits(8) << 8);
#endif
    return 0;
}


/*void vp8dx_bool_decoder_fill_c(BOOL_DECODER *br)
{
    int          left, right;

    // Find available room in the buffer
    left = 0;
    right = br->read_ptr - br->write_ptr;

    if (right < 0)
    {
        // Read pointer is behind the write pointer. We can write from the
        // write pointer to the end of the buffer.
        right = VP8_BOOL_DECODER_SZ - (br->write_ptr - br->decode_buffer);
        left = br->read_ptr - br->decode_buffer;
    }

    if (right + left < 128)
        return;

    if (right)
        copy_in(br, right);

    if (left)
    {
        br->write_ptr = br->decode_buffer;
        copy_in(br, left);
    }

}*/


void vp8dx_stop_decode_c(BOOL_DECODER *bc)
{
#ifdef SIM_IN_WIN
     vpx_free(bc->decode_buffer);
#endif
//    bc->decode_buffer = 0;
}

#if 0
/*
 * Until optimized versions of these functions are available, we
 * keep the implementation in the header to allow inlining.
 *
 * The RTCD-style invocations are still in place so this can
 * be switched by just uncommenting these functions here and
 * the DBOOLHUFF_INVOKE calls in the header.
 */
int vp8dx_decode_bool_c(BOOL_DECODER *br, int probability)
{
    unsigned int bit=0;
    unsigned int split;
    unsigned int bigsplit;
    register unsigned int range = br->range;
    register unsigned int value = br->value;

    split = 1 + (((range-1) * probability) >> 8);
    bigsplit = (split<<8);

    range = split;
    if(value >= bigsplit)
    {
        range = br->range-split;
        value = value-bigsplit;
        bit = 1;
    }

    /*if(range>=0x80)
    {
        br->value = value;
        br->range = range;
        return bit;
    }*/

    {
        int count = br->count;
        register unsigned int shift = vp8dx_bitreader_norm[range];
        range <<= shift;
        value <<= shift;
        count -= shift;
        if(count <= 0)
        {
            value |= (*br->read_ptr++) << (-count);
            count += 8 ;
        }
        br->count = count;
    }
    br->value = value;
    br->range = range;
    return bit;
}

int vp8dx_decode_value_c(BOOL_DECODER *br, int bits)
{
    int z = 0;
    int bit;
    for ( bit=bits-1; bit>=0; bit-- )
    {
        z |= (vp8dx_decode_bool(br, 0x80)<<bit);
    }
    return z;
}
#endif


int vp8dx_decode_bool(BOOL_DECODER *br, int probability) 
{
  /*
   * Until optimized versions of this function are available, we
   * keep the implementation in the header to allow inlining.
   *
   *return DBOOLHUFF_INVOKE(br->rtcd, debool)(br, probability);
   */
    unsigned int bit = 0;
    unsigned int split;
    unsigned int bigsplit;
    /*register*/ unsigned int range = br->range;
   /* register*/ unsigned int value = br->value;

    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = (split << 8);

    range = split;

    if (value >= bigsplit)
    {
        range = br->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    /*if(range>=0x80)
    {
        br->value = value;
        br->range = range;
        return bit
    }*/

    {
        int count = br->count;
        /*register */unsigned int shift = vp8dx_bitreader_norm[range];
        range <<= shift;
        value <<= shift;
        count -= shift;

        if (count <= 0)
        {
#ifdef SIM_IN_WIN
            value |= (*br->read_ptr++) << (-count);
			if(or1200_print)
				BitstreamReadBits(8);
#else
			value |= (BitstreamReadBits(8) << (-count));
#endif
            count += 8 ;
        }

        br->count = count;
    }
    br->value = value;
    br->range = range;
    return bit;
}

int vp8_decode_value(BOOL_DECODER *br, int bits)
{
  /*
   * Until optimized versions of this function are available, we
   * keep the implementation in the header to allow inlining.
   *
   *return DBOOLHUFF_INVOKE(br->rtcd, devalue)(br, bits);
   */
    int z = 0;
    int bit;

    for (bit = bits - 1; bit >= 0; bit--)
    {
        z |= (vp8dx_decode_bool(br, 0x80) << bit);
    }

    return z;
}
