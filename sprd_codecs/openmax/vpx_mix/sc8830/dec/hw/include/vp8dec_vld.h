
#ifndef _VP8DEC_VLD_H_
#define _VP8DEC_VLD_H_

#include "sci_types.h"
#include "vp8dec_basic.h"
#include "vp8dec_mode.h"


void vp8_default_coef_probs(VP8_COMMON *pc);
void vp8_init_mbmode_probs(VP8_COMMON *x);
void vp8_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES-1]);
void vp8_kf_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1]);//weihu

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_VP8_ENTROPY_H_
