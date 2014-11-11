/******************************************************************************
 ** File Name:    vp8dec_frame.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

LOCAL int get_delta_q(VPXDecObject *vo, vp8_reader *bc, int prev, int *q_update)
{
    int ret_val = 0;

    if (vp8_read_bit(bc))
    {
        ret_val = vp8_read_literal(vo, bc, 4);

        if (vp8_read_bit(bc))
            ret_val = -ret_val;
    }

    /* Trigger a quantizer update if the delta-q value has changed */
    if (ret_val != prev)
        *q_update = 1;

    return ret_val;
}

static void read_mvcontexts(VPXDecObject *vo, vp8_reader *bc, MV_CONTEXT *mvc)
{
    int i = 0;

    do
    {
        const vp8_prob *up = vp8_mv_update_probs[i].prob;
        vp8_prob *p = (vp8_prob *)(mvc + i);
        vp8_prob *const pstop = p + MVPcount;

        do
        {
            if (vp8_read(vo, bc, *up++))
            {
                const vp8_prob x = (vp8_prob)vp8_read_literal(vo, bc, 7);

                *p = x ? x << 1 : 1;
            }
        }
        while (++p < pstop);
    }
    while (++i < 2);
}

void vp8_decode_mode_mvs(VPXDecObject *vo)
{
    VP8_COMMON *const pc = & vo->common;
    vp8_reader *const bc = & vo->bc;
    VSP_FH_REG_T *fh_reg_ptr = vo->g_fh_reg_ptr;
    MV_CONTEXT *const mvc = pc->fc.mvc;

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false = 0;

    if (pc->mb_no_coeff_skip)
        prob_skip_false = (vp8_prob)vp8_read_literal(vo, bc, 8);

    prob_intra = (vp8_prob)vp8_read_literal(vo, bc, 8);
    prob_last  = (vp8_prob)vp8_read_literal(vo, bc, 8);
    prob_gf    = (vp8_prob)vp8_read_literal(vo, bc, 8);

    fh_reg_ptr->FH_CFG8 |= (prob_skip_false << 0);
    fh_reg_ptr->FH_CFG8 |= (prob_intra << 8);
    fh_reg_ptr->FH_CFG8 |= (prob_last << 16);
    fh_reg_ptr->FH_CFG8 |= (prob_gf << 24);

    if (vp8_read_bit(bc))
    {
        int i = 0;

        do
        {
            pc->fc.ymode_prob[i] = (vp8_prob) vp8_read_literal(vo, bc, 8);
        }
        while (++i < 4);
    }

    if (vp8_read_bit(bc))
    {
        int i = 0;

        do
        {
            pc->fc.uv_mode_prob[i] = (vp8_prob) vp8_read_literal(vo, bc, 8);
        }
        while (++i < 3);
    }
    read_mvcontexts(vo, bc, mvc);
}

void vp8_kfread_modes(VPXDecObject *vo)
{
    VP8_COMMON *const pc = & vo->common;
    vp8_reader *const bc = & vo->bc;
    vp8_prob prob_skip_false = 0;

    if (pc->mb_no_coeff_skip)
    {
        prob_skip_false = (vp8_prob)(vp8_read_literal(vo, bc, 8));
    }

    vo->g_fh_reg_ptr->FH_CFG8 |= (prob_skip_false << 0);
}

LOCAL void init_frame(VPXDecObject *vo)
{
    VP8_COMMON *const pc = & vo->common;
    MACROBLOCKD *const xd  = & vo->mb;

    if (pc->frame_type == KEY_FRAME)
    {
        // Various keyframe initializations
        vpx_memcpy(pc->fc.mvc, vp8_default_mv_context, sizeof(vp8_default_mv_context));

        vp8_init_mbmode_probs(pc);

        vp8_default_coef_probs(pc);

        vp8_kf_default_bmode_probs(pc->kf_bmode_prob);
        // reset the segment feature data to 0 with delta coding (Default state).
        vpx_memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));
        xd->mb_segement_abs_delta = SEGMENT_DELTADATA;

        // reset the mode ref deltasa for loop filter
        vpx_memset(xd->ref_lf_deltas, 0, sizeof(xd->ref_lf_deltas));
        vpx_memset(xd->mode_lf_deltas, 0, sizeof(xd->mode_lf_deltas));

        // All buffers are implicitly updated on key frames.
        pc->refresh_golden_frame = 1;
        pc->refresh_alt_ref_frame = 1;
        pc->copy_buffer_to_gf = 0;
        pc->copy_buffer_to_arf = 0;

        // Note that Golden and Altref modes cannot be used on a key frame so
        // ref_frame_sign_bias[] is undefined and meaningless
        pc->ref_frame_sign_bias[GOLDEN_FRAME] = 0;
        pc->ref_frame_sign_bias[ALTREF_FRAME] = 0;
    }
    xd->frame_type = pc->frame_type;
}

MMDecRet vp8_decode_frame(VPXDecObject *vo)
{
    const unsigned char *data		= (const unsigned char *)vo->Source;
    const unsigned char *data_end	= (const unsigned char *)(data + vo->source_sz);
    uint32 cmd;
    vp8_reader *const bc = & vo->bc;
    VP8_COMMON *const pc = & vo->common;
    MACROBLOCKD *const xd  = & vo->mb;
    int first_partition_length_in_bytes;
    int i, j, k, l;
    const int *const mb_feature_data_bits = vp8_mb_feature_data_bits;

    vo->error_flag = 0;
    memset(vo->g_fh_reg_ptr, 0, sizeof(VSP_FH_REG_T));

    // 3 byte header
    pc->frame_type = (FRAME_TYPE)(data[0] & 1);
    pc->version = (data[0] >> 1) & 7;
    pc->show_frame = (data[0] >> 4) & 1;
    first_partition_length_in_bytes = (data[0] | (data[1] << 8) | (data[2] << 16)) >> 5;
    data += 3;
    BitstreamReadBits(vo, 8*3);

    if (data + first_partition_length_in_bytes > data_end)
    {
        vo->error_flag |= ER_SREAM_ID;
        return MMDEC_STREAM_ERROR;
    }

    vp8_setup_version(pc);

    if (pc->frame_type == KEY_FRAME)
    {
        const int Width = pc->Width;
        const int Height = pc->Height;

        // vet via sync code
        if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
        {
            vo->error_flag |= ER_SREAM_ID;
            //printf ("Invalid frame sync code");
            return MMDEC_STREAM_ERROR;
        }

        pc->Width = (data[3] | (data[4] << 8)) & 0x3fff;
        pc->horiz_scale = data[4] >> 6;
        pc->Height = (data[5] | (data[6] << 8)) & 0x3fff;
        pc->vert_scale = data[6] >> 6;
        data += 7;
        BitstreamReadBits(vo, 8*3);
        BitstreamReadBits(vo, 8*2);
        BitstreamReadBits(vo, 8*2);
        if ( (Width != pc->Width) || (Height != pc->Height) || (!pc->bInitSuceess))	// Resolution Changed
        {
            if ((Width != pc->Width) || (Height != pc->Height))
            {
                pc->bInitSuceess = 0;
            }

            if (pc->Width <= 0)
            {
                pc->Width = Width;
                vo->error_flag |= ER_SREAM_ID;
                return MMDEC_STREAM_ERROR;
            }

            if (pc->Height <= 0)
            {
                pc->Height = Height;
                vo->error_flag |= ER_SREAM_ID;
                return MMDEC_STREAM_ERROR;
            }

            if (vp8_init_frame_buffers(vo, pc) != MMDEC_OK)
            {
                return MMDEC_MEMORY_ERROR;
            }

            if (!pc->bInitSuceess)
            {
                pc->bInitSuceess = 1;
                return MMDEC_MEMORY_ALLOCED;
            }
        }
    }
    pc->new_frame.u_buffer = pc->new_frame.y_buffer + (pc->MBs <<8) ;
    pc->new_frame.u_buffer_virtual =  pc->new_frame.y_buffer_virtual + (pc->MBs <<8);
    if (pc->Width == 0 || pc->Height == 0)
    {
        vo->error_flag |= ER_SREAM_ID;
        return MMDEC_STREAM_ERROR;
    }

    init_frame(vo);

    bc->range = 255;
    bc->count = 0;
    bc->value = (BitstreamReadBits(vo, 8) << 8);

    if (pc->frame_type == KEY_FRAME) {
        pc->clr_type    = (YUV_TYPE)vp8_read_bit(bc);
        pc->clamp_type  = (CLAMP_TYPE)vp8_read_bit(bc);
    }

    // Is segmentation enabled
    xd->segmentation_enabled = (unsigned char)vp8_read_bit(bc);

    if (xd->segmentation_enabled)
    {
        // Signal whether or not the segmentation map is being explicitly updated this frame.
        xd->update_mb_segmentation_map = (unsigned char)vp8_read_bit(bc);
        xd->update_mb_segmentation_data = (unsigned char)vp8_read_bit(bc);

        if (xd->update_mb_segmentation_data)
        {
            xd->mb_segement_abs_delta = (unsigned char)vp8_read_bit(bc);	// segment_feature_mode, 0:delta, 1: abs

            vpx_memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));

            // For each segmentation feature (Quant and loop filter level)
            for (i = 0; i < MB_LVL_MAX; i++)
            {
                for (j = 0; j < MAX_MB_SEGMENTS; j++)
                {
                    // Frame level data
                    if (vp8_read_bit(bc))
                    {
                        xd->segment_feature_data[i][j] = (signed char)vp8_read_literal(vo, bc, mb_feature_data_bits[i]);

                        if (vp8_read_bit(bc))
                        {
                            xd->segment_feature_data[i][j] = (signed char)-xd->segment_feature_data[i][j];
                        }
                    } else
                    {
                        xd->segment_feature_data[i][j] = 0;
                    }
                }
            }
        }

        if (xd->update_mb_segmentation_map)
        {
            // Which macro block level features are enabled
            vpx_memset(xd->mb_segment_tree_probs, 255, sizeof(xd->mb_segment_tree_probs));

            // Read the probs used to decode the segment id for each macro block.
            for (i = 0; i < MB_FEATURE_TREE_PROBS; i++)
            {
                // If not explicitly set value is defaulted to 255 by memset above
                if (vp8_read_bit(bc))
                {
                    xd->mb_segment_tree_probs[i] = (vp8_prob)vp8_read_literal(vo, bc, 8);
                }
            }
        }
    } else
    {
        xd->update_mb_segmentation_map = 0;
    }

    // Read the loop filter level and type
    pc->filter_type = (LOOPFILTERTYPE) vp8_read_bit(bc);
    pc->filter_level = vp8_read_literal(vo, bc, 6);
    pc->sharpness_level = vp8_read_literal(vo, bc, 3);

    // Read in loop filter deltas applied at the MB level based on mode or ref frame.
    xd->mode_ref_lf_delta_update = 0;
    xd->mode_ref_lf_delta_enabled = (unsigned char)vp8_read_bit(bc);	// loop_filter_adj_enable

    if (xd->mode_ref_lf_delta_enabled)
    {
        // Do the deltas need to be updated
        xd->mode_ref_lf_delta_update = (unsigned char)vp8_read_bit(bc);

        if (xd->mode_ref_lf_delta_update)
        {
            // Send update
            for (i = 0; i < MAX_REF_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bc))
                {
                    //sign = vp8_read_bit( bc );
                    xd->ref_lf_deltas[i] = (signed char)vp8_read_literal(vo, bc, 6);

                    if (vp8_read_bit(bc))        // Apply sign
                        xd->ref_lf_deltas[i] = (signed char)(xd->ref_lf_deltas[i] * -1);
                }
            }

            // Send update
            for (i = 0; i < MAX_MODE_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bc))
                {
                    //sign = vp8_read_bit( bc );
                    xd->mode_lf_deltas[i] = (signed char)vp8_read_literal(vo, bc, 6);

                    if (vp8_read_bit(bc))        // Apply sign
                    {
                        xd->mode_lf_deltas[i] = (signed char)(xd->mode_lf_deltas[i] * -1);
                    }
                }
            }
        }
    }

    pc->multi_token_partition = (TOKEN_PARTITION)vp8_read_literal(vo, bc, 2);	// log2_nbr_of_dct_partitions
    vo->g_fh_reg_ptr->FH_CFG0 |= (pc->multi_token_partition << 19);
    vo->g_fh_reg_ptr->FH_CFG11 =  ((data + first_partition_length_in_bytes + 3*((1<<pc->multi_token_partition)-1)) - vo->Source);

    // Read the default quantizers.
    {
        int Q, q_update;

        Q = vp8_read_literal(vo, bc, 7);  // AC 1st order Q = default
        pc->base_qindex = Q;
        q_update = 0;
        pc->y1dc_delta_q = get_delta_q(vo, bc, pc->y1dc_delta_q, &q_update);
        pc->y2dc_delta_q = get_delta_q(vo, bc, pc->y2dc_delta_q, &q_update);
        pc->y2ac_delta_q = get_delta_q(vo, bc, pc->y2ac_delta_q, &q_update);
        pc->uvdc_delta_q = get_delta_q(vo, bc, pc->uvdc_delta_q, &q_update);
        pc->uvac_delta_q = get_delta_q(vo, bc, pc->uvac_delta_q, &q_update);

        if (q_update)
            vp8cx_init_de_quantizer(vo);
    }

    // Determine if the golden frame or ARF buffer should be updated and how.
    // For all non key frames the GF and ARF refresh flags and sign bias
    // flags must be set explicitly.
    if (pc->frame_type != KEY_FRAME)
    {
        // Should the GF or ARF be updated from the current frame
        pc->refresh_golden_frame = vp8_read_bit(bc);
        pc->refresh_alt_ref_frame = vp8_read_bit(bc);

        // Buffer to buffer copy flags.
        pc->copy_buffer_to_gf = 0;

        if (!pc->refresh_golden_frame)
            pc->copy_buffer_to_gf = vp8_read_literal(vo, bc, 2);

        pc->copy_buffer_to_arf = 0;

        if (!pc->refresh_alt_ref_frame)
            pc->copy_buffer_to_arf = vp8_read_literal(vo, bc, 2);

        pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp8_read_bit(bc);
        pc->ref_frame_sign_bias[ALTREF_FRAME] = vp8_read_bit(bc);
    }

    pc->refresh_entropy_probs = vp8_read_bit(bc);
    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
    }

    pc->refresh_last_frame = pc->frame_type == KEY_FRAME  ||  vp8_read_bit(bc);

    // read coef probability tree
    for (i = 0; i < BLOCK_TYPES; i++)
    {
        for (j = 0; j < COEF_BANDS; j++)
        {
            for (k = 0; k < PREV_COEF_CONTEXTS; k++)
            {
                for (l = 0; l < MAX_ENTROPY_TOKENS - 1; l++)
                {
                    vp8_prob *const p = pc->fc.coef_probs [i][j][k] + l;

                    if (vp8_read(vo, bc, vp8_coef_update_probs [i][j][k][l]))
                    {
                        *p = (vp8_prob)vp8_read_literal(vo, bc, 8);
                    }
                }
            }
        }
    }

    // Read the mb_no_coeff_skip flag
    pc->mb_no_coeff_skip = (int)vp8_read_bit(bc);
    vo->g_fh_reg_ptr->FH_CFG0 |= (pc->mb_no_coeff_skip << 31);

    if (pc->frame_type == KEY_FRAME)
    {
        vp8_kfread_modes(vo);
    } else
    {
        vp8_decode_mode_mvs(vo);		//decode mv and modes
    }

    Vp8Dec_InitVSP(vo);
    Write_tbuf_Probs(vo);

    VSP_WRITE_REG(VSP_REG_BASE_ADDR + ARM_INT_MASK_OFF, V_BIT_2, "ARM_INT_MASK, only enable VSP ACC init");//enable int //
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, (V_BIT_2 | V_BIT_4 | V_BIT_5), "VSP_INT_MASK, enable mbw_slice_done, vld_err, time_out");//enable int //frame done/error/timeout
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, V_BIT_0, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_START_OFF, 0xa, "VSP_START: DECODE_START=1");

    cmd = VSP_POLL_COMPLETE((VSPObject *)vo);
    if(cmd & V_BIT_2)	// MBW_FMR_DONE
    {
        vo->error_flag = 0;
    } else if(cmd & (V_BIT_4 | V_BIT_5 | V_BIT_30 | V_BIT_31))	// (VLD_ERR|TIME_OUT)
    {
        vo->error_flag |= ER_HW_ID;

        if (cmd & V_BIT_4)
        {
            SCI_TRACE_LOW("%s, %d, VLD_ERR", __FUNCTION__, __LINE__);
        } else if (cmd & (V_BIT_5 | V_BIT_31))
        {
            SCI_TRACE_LOW("%s, %d, TIME_OUT", __FUNCTION__, __LINE__);
        } else //if (cmd & V_BIT_30)
        {
            SCI_TRACE_LOW("%s, %d, Broken by signal", __FUNCTION__, __LINE__);
        }
    } else
    {
        SCI_TRACE_LOW("%s, %d, should not be here!", __FUNCTION__, __LINE__);
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL");

    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->fc, &pc->lfc, sizeof(pc->fc));
    }

    return MMDEC_OK;
}

MMDecRet vp8dx_receive_compressed_data(VPXDecObject *vo, unsigned long size, const unsigned char *source, int64 time_stamp)
{
    VP8_COMMON *cm = &vo->common;
    MMDecRet ret;

    vo->Source = source;
    vo->source_sz = size;

    // Find a  place in buffer pool to save pBufferHeader of new picture.
    {
        int buffer_index;
        for(buffer_index = 0; buffer_index < YUV_BUFFER_NUM; buffer_index++)
        {
            if(cm->ref_count[buffer_index] == 0)
            {
                cm->buffer_pool[buffer_index] = cm->new_frame.pBufferHeader;
                break;
            }
        }

        if(buffer_index == YUV_BUFFER_NUM)
        {
            return MMDEC_OUTPUT_BUFFER_OVERFLOW;
        }
    }

    ret = vp8_decode_frame(vo);
    if (ret != MMDEC_OK)
    {
        return ret;
    }

    // If any buffer copy / swapping is signaled it should be done here.
    if (cm->copy_buffer_to_arf != 0)
    {
        if (cm->copy_buffer_to_arf == 1)
            vp8_copy_yv12_buffer(vo,cm, &cm->last_frame, &cm->alt_ref_frame);
        else if (cm->copy_buffer_to_arf == 2)
            vp8_copy_yv12_buffer(vo,cm, &cm->golden_frame, &cm->alt_ref_frame);
    }

    if (cm->copy_buffer_to_gf != 0)
    {
        if (cm->copy_buffer_to_gf == 1)
            vp8_copy_yv12_buffer(vo,cm, &cm->last_frame, &cm->golden_frame);
        else if (cm->copy_buffer_to_gf == 2)
            vp8_copy_yv12_buffer(vo,cm, &cm->alt_ref_frame, &cm->golden_frame);
    }

    // Should the golden or alternate reference frame be refreshed?
    if (cm->refresh_golden_frame == 1)
        vp8_copy_yv12_buffer(vo,cm, &cm->new_frame, &cm->golden_frame);

    if (cm->refresh_alt_ref_frame == 1)
        vp8_copy_yv12_buffer(vo,cm, &cm->new_frame, &cm->alt_ref_frame);

    if (cm->refresh_last_frame == 1)
    {
        vp8_copy_yv12_buffer(vo,cm, &cm->new_frame, &cm->last_frame);
        cm->frame_to_show = &cm->last_frame;
    }
    else
    {
        cm->frame_to_show = &cm->new_frame;
    }

    return MMDEC_OK;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

