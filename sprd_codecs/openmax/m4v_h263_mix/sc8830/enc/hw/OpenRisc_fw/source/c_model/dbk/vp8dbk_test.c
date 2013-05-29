#include "vp8dbk_global.h"


void Vp8Dec_BS_and_Para(VP8_COMMON *cm,MACROBLOCKD *mbd, int *baseline_filter_level, int mb_row, int mb_col)
{
	int alt_flt_enabled = mbd->segmentation_enabled;
	int Segment = (alt_flt_enabled) ? mbd->mode_info_context->mbmi.segment_id : 0;
	int filter_level = baseline_filter_level[Segment];
    MB_MODE_INFO *mbmi = &mbd->mode_info_context->mbmi;
	int filter_type = cm->filter_type;
	int dc_diff = (mbd->mode_info_context->mbmi.dc_diff > 0) ? 1 : 0;

    loop_filter_info *lfi = cm->lf_info;
    LOOPFILTERTYPE lft = cm->filter_type;
    int sharpness_lvl = cm->sharpness_level;
    int frame_type = cm->frame_type;

    int block_inside_limit = 0;
    int HEVThresh;
    const int yhedge_boost  = 2;

	int b_1st_col = (mb_col == 0) ? 1: 0;
	int b_1st_row = (mb_row == 0) ? 1: 0;


    if (mbd->mode_ref_lf_delta_enabled)
    {
        // Aplly delta for reference frame
        filter_level += mbd->ref_lf_deltas[mbmi->ref_frame];

        // Apply delta for mode
        if (mbmi->ref_frame == INTRA_FRAME)
        {
            // Only the split mode BPRED has a further special case
            if (mbmi->mode == B_PRED)
                filter_level +=  mbd->mode_lf_deltas[0];
        }
        else
        {
            // Zero motion mode
            if (mbmi->mode == ZEROMV)
                filter_level +=  mbd->mode_lf_deltas[1];

            // Split MB motion mode
            else if (mbmi->mode == SPLITMV)
                filter_level +=  mbd->mode_lf_deltas[3];

            // All other inter motion modes (Nearest, Near, New)
            else
                filter_level +=  mbd->mode_lf_deltas[2];
        }

        // Range check
        if (filter_level > MAX_LOOP_FILTER)
            filter_level = MAX_LOOP_FILTER;
        else if (filter_level < 0)
            filter_level = 0;
	}

    if (frame_type == KEY_FRAME)
    {
        if (filter_level >= 40)
            HEVThresh = 2;
        else if (filter_level >= 15)
            HEVThresh = 1;
        else
            HEVThresh = 0;
    }
    else
    {
        if (filter_level >= 40)
            HEVThresh = 3;
        else if (filter_level >= 20)
            HEVThresh = 2;
        else if (filter_level >= 15)
            HEVThresh = 1;
        else
            HEVThresh = 0;
    }

    // Set loop filter parameters that control sharpness.
    block_inside_limit = filter_level >> (sharpness_lvl > 0);
    block_inside_limit = block_inside_limit >> (sharpness_lvl > 4);

	if (sharpness_lvl > 0)
    {
        if (block_inside_limit > (9 - sharpness_lvl))
            block_inside_limit = (9 - sharpness_lvl);
    }

    if (block_inside_limit < 1)
        block_inside_limit = 1;

// 	        lfi[i].lim[j] = block_inside_limit;
//             lfi[i].mbflim[j] = filt_lvl + yhedge_boost;
//             lfi[i].mbthr[j] = HEVThresh;
//             lfi[i].flim[j] = filt_lvl;
//             lfi[i].thr[j] = HEVThresh;

	g_dbk_reg_ptr->VP8DBK_CFG0 = ((filter_level + yhedge_boost) << 24)
								|(block_inside_limit << 16)
								|(filter_level << 8)
								|(HEVThresh << 4)
								|(b_1st_row << 3)
								|(b_1st_col << 2)
								|(dc_diff <<1)
								|(filter_type);
											//[31:24]	mbflim
											//[21:16]	block_inside_limint. lim
											//[13:8]	loop filter level. flim
											//[5:4]		HEVThreshold. mbthr
											//[3]		b_1st_row. 
											//[2]		b_1st_col
											//[1]		Dc diff. 1, nonzero. 0, zero
											//[0]		filter type. NORMAL_LOOPFILTER = 0,SIMPLE_LOOPFILTER = 1.


}

void vp8_dbk_frame
(
    VP8_COMMON *cm,
    MACROBLOCKD *mbd,
    int default_filt_lvl
)
{
    YV12_BUFFER_CONFIG *post = cm->frame_to_show;
    loop_filter_info *lfi = cm->lf_info;
    int frame_type = cm->frame_type;

    int mb_row;
    int mb_col;


    int baseline_filter_level[MAX_MB_SEGMENTS];
    int filter_level;
    int alt_flt_enabled = mbd->segmentation_enabled;

    int i;
    unsigned char *y_ptr, *u_ptr, *v_ptr;

    mbd->mode_info_context = cm->mi;          // Point at base of Mb MODE_INFO list

    // Note the baseline filter values for each segment
    if (alt_flt_enabled)
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {
            // Abs value
            if (mbd->mb_segement_abs_delta == SEGMENT_ABSDATA)
                baseline_filter_level[i] = mbd->segment_feature_data[MB_LVL_ALT_LF][i];
            // Delta Value
            else
            {
                baseline_filter_level[i] = default_filt_lvl + mbd->segment_feature_data[MB_LVL_ALT_LF][i];
                baseline_filter_level[i] = (baseline_filter_level[i] >= 0) ? ((baseline_filter_level[i] <= MAX_LOOP_FILTER) ? baseline_filter_level[i] : MAX_LOOP_FILTER) : 0;  // Clamp to valid range
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
            baseline_filter_level[i] = default_filt_lvl;
    }

    // Set up the buffer pointers
    y_ptr = post->y_buffer;
    u_ptr = post->u_buffer;
    v_ptr = post->v_buffer;

    // vp8_filter each macro block
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
	
			if (g_nFrame_dec == 1 && mb_col == 0 && mb_row == 0)
			{
				PRINTF("");
			}

			g_glb_reg_ptr->VSP_CTRL0 = (mb_row << 8) | mb_col;
			g_glb_reg_ptr->VSP_CFG1  = (cm->mb_rows << 12) | cm->mb_cols;

			Vp8Dec_BS_and_Para(cm,mbd, baseline_filter_level,mb_row,mb_col);
			
// 			fprintf(g_dbk_trace_fp,"mbx = %d mby=%d, filter_level = %d \n",mb_row,mb_col);
// 			fprintf(g_dbk_trace_fp,"lim = %d flim = %d thr = %d mbflim = %d mbthr = %d \n",
// 				lfi[filter_level].lim[0], lfi[filter_level].flim[0],lfi[filter_level].thr[0],lfi[filter_level].mbflim[0],lfi[filter_level].mbthr[0]);
 	
			dbk_module(y_ptr, u_ptr, v_ptr, post->y_stride, post->uv_stride);

            y_ptr += 16;
            u_ptr += 8;
            v_ptr += 8;

            mbd->mode_info_context++;     // step to next MB
        }

        y_ptr += post->y_stride  * 16 - post->y_width;
        u_ptr += post->uv_stride *  8 - post->uv_width;
        v_ptr += post->uv_stride *  8 - post->uv_width;

        mbd->mode_info_context++;         // Skip border mb
    }
}