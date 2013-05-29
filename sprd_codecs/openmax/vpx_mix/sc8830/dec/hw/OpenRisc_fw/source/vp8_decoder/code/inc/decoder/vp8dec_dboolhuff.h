
#ifndef VP8DEC_DBOOLHUFF_H
#define VP8DEC_DBOOLHUFF_H

/* Size of the bool decoder backing storage
 *
 * This size was chosen to be greater than the worst case encoding of a
 * single macroblock. This was calcluated as follows (python):
 *
 *     def max_cost(prob):
 *         return max(prob_costs[prob], prob_costs[255-prob]) / 256;
 *
 *     tree_nodes_cost = 7 * max_cost(255)
 *     extra_bits_cost = sum([max_cost(bit) for bit in extra_bits])
 *     sign_bit_cost = max_cost(128)
 *     total_cost = tree_nodes_cost + extra_bits_cost + sign_bit_cost
 *
 * where the prob_costs table was taken from the C vp8_prob_cost table in
 * boolhuff.c and the extra_bits table was taken from the 11 extrabits for
 * a category 6 token as defined in vp8d_token_extra_bits2/detokenize.c
 *
 * This equation produced a maximum of 79 bits per coefficient. Scaling up
 * to the macroblock level:
 *
 *     79 bits/coeff * 16 coeff/block * 25 blocks/macroblock = 31600 b/mb
 *
 *     4096 bytes = 32768 bits > 31600
 */
#define VP8_BOOL_DECODER_SZ       (1024*800)//4096
#define VP8_BOOL_DECODER_MASK     (VP8_BOOL_DECODER_SZ-1)
#define VP8_BOOL_DECODER_PTR_MASK (~(unsigned int)(VP8_BOOL_DECODER_SZ))

#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))
typedef struct
{
//    unsigned int         lowvalue;
    unsigned int         range;
    unsigned int         value;
    int                  count;
    const unsigned char *user_buffer;
    unsigned int         user_buffer_sz;
    unsigned char       *decode_buffer;
    const unsigned char *read_ptr;
    unsigned char       *write_ptr;
#if CONFIG_RUNTIME_CPU_DETECT
    struct vp8_dboolhuff_rtcd_vtable *rtcd;
#endif
} BOOL_DECODER;

#define IF_RTCD(x) NULL

unsigned char *br_ptr_advance(const unsigned char *_ptr,
                                     unsigned int n);

int vp8dx_start_decode_c(BOOL_DECODER *br, const unsigned char *source,
                        unsigned int source_sz);

void vp8dx_stop_decode_c(BOOL_DECODER *bc);
//void vp8dx_bool_decoder_fill_c(BOOL_DECODER *br);
int vp8dx_decode_bool(BOOL_DECODER *br, int probability) ;
int vp8_decode_value(BOOL_DECODER *br, int bits);

#endif //VP8DEC_DBOOLHUFF_H