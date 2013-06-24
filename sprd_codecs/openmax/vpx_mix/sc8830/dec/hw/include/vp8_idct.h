

#ifndef VP8_IDCT_H
#define VP8_IDCT_H

void vp8_short_idct4x4llm_c(short *input, short *output, int pitch);
void vp8_short_idct4x4llm_1_c(short *input, short *output, int pitch);
void vp8_short_inv_walsh4x4_c(short *input, short *output);
void vp8_short_inv_walsh4x4_1_c(short *input, short *output);
void vp8_dc_only_idct_c(short input_dc, short *output, int pitch);

#endif //VP8_IDCT_H