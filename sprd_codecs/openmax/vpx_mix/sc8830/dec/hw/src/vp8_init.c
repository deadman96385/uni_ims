
#include "sci_types.h"
#include "video_common.h"
#include "vp8_entropy.h"
#include "vp8_entropy_mode.h"
#include "vp8_basic.h"
#include "vp8_reconintra.h"

void vp8_create_common(VP8_COMMON *oci)
{
//     vp8_machine_specific_config(oci);
//	vp8_build_intra_predictors_mby_ptr = vp8_build_intra_predictors_mby;
//    vp8_build_intra_predictors_mby_s_ptr = vp8_build_intra_predictors_mby_s;

    vp8_default_coef_probs(oci);
    vp8_init_mbmode_probs(oci);
    vp8_default_bmode_probs(oci->fc.bmode_prob);

    oci->mb_no_coeff_skip = 1;
    oci->no_lpf = 0;
    oci->simpler_lpf = 0;
    oci->use_bilinear_mc_filter = 0;
    oci->full_pixel = 0;
    oci->multi_token_partition = ONE_PARTITION;
    oci->clr_type = REG_YUV;
    oci->clamp_type = RECON_CLAMP_REQUIRED;
	oci->buffer_count = 0;

    // Initialise reference frame sign bias structure to defaults
    vpx_memset(oci->ref_frame_sign_bias, 0, sizeof(oci->ref_frame_sign_bias));

    // Default disable buffer to buffer copying
    oci->copy_buffer_to_gf = 0;
    oci->copy_buffer_to_arf = 0;
}
