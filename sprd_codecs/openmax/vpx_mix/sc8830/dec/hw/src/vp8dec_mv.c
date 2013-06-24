
#include "vp8_entropy_mv.h"
#include "vp8_entropy_mode.h"
#include "vp8dec_treereader.h"
#include "vp8dec_mode.h"
#include "sc8810_video_header.h"
#include "vp8dec_demode.h"//weihu

#ifdef SIM_IN_WIN
#include "vp8_find_nearmv.h"
#include "vp8_reconinter.h"//weihu
#include "vp8_test_vectors.h"//derek
#endif

int read_mvcomponent(vp8_reader *r, const MV_CONTEXT *mvc)
{
    const vp8_prob *const p = (const vp8_prob *) mvc;
    int x = 0;

    if (vp8_read(r, p [mvpis_short]))  /* Large, 8 <= x <= 1023 */
    {
        int i = 0;

        do
        {
            x += vp8_read(r, p [MVPbits + i]) << i;
        }
        while (++i < 3);

        i = mvlong_width - 1;  /* Skip bit 3, which is sometimes implicit */

        do
        {
            x += vp8_read(r, p [MVPbits + i]) << i;
        }
        while (--i > 3);

        if (!(x & 0xFFF0)  ||  vp8_read(r, p [MVPbits + 3]))
            x += 8;
    }
    else   /* small, 0 <= x <= 7 */
        x = vp8_treed_read(r, vp8_small_mvtree, p + MVPshort);

    if (x  &&  vp8_read(r, p [MVPsign]))
        x = -x;

    return x;
}

#ifdef SIM_IN_WIN
static void read_mv(vp8_reader *r, MV *mv, const MV_CONTEXT *mvc)
{
    mv->row = (short)(read_mvcomponent(r,   mvc) << 1);
    mv->col = (short)(read_mvcomponent(r, ++mvc) << 1);
}
#endif


static void read_mvcontexts(vp8_reader *bc, MV_CONTEXT *mvc)
{
    int i = 0;

    do
    {
        const vp8_prob *up = vp8_mv_update_probs[i].prob;
        vp8_prob *p = (vp8_prob *)(mvc + i);
        vp8_prob *const pstop = p + MVPcount;

        do
        {
            if (vp8_read(bc, *up++))
            {
                const vp8_prob x = (vp8_prob)vp8_read_literal(bc, 7);

                *p = x ? x << 1 : 1;
            }
        }
        while (++p < pstop);
    }
    while (++i < 2);
}

#ifdef SIM_IN_WIN
static MB_PREDICTION_MODE read_mv_ref(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_mv_ref_tree, p);

    return (MB_PREDICTION_MODE)i;
}

static MB_PREDICTION_MODE sub_mv_ref(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_sub_mv_ref_tree, p);

    return (MB_PREDICTION_MODE)i;
}
unsigned int vp8_mv_cont_count[5][4] =
{
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }
};
#endif

void vp8_decode_mode_mvs(VP8D_COMP *pbi)
{
    VP8_COMMON *const pc = & pbi->common;
    vp8_reader *const bc = & pbi->bc;

#ifdef SIM_IN_WIN
	const MV Zero = { 0, 0};
    MODE_INFO *mi = pc->mi;//, *ms;
    const int mis = pc->mode_info_stride;
	int mb_row = -1;
#ifdef MCA_TV_SPLIT
	uint32 intra_count = 0;
#endif
#endif

    MV_CONTEXT *const mvc = pc->fc.mvc;

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false = 0;

    if (pc->mb_no_coeff_skip)
        prob_skip_false = (vp8_prob)vp8_read_literal(bc, 8);

    prob_intra = (vp8_prob)vp8_read_literal(bc, 8);
    prob_last  = (vp8_prob)vp8_read_literal(bc, 8);
    prob_gf    = (vp8_prob)vp8_read_literal(bc, 8);

//#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG8 |= (prob_skip_false << 0);
	g_fh_reg_ptr->FH_CFG8 |= (prob_intra << 8);
	g_fh_reg_ptr->FH_CFG8 |= (prob_last << 16);
	g_fh_reg_ptr->FH_CFG8 |= (prob_gf << 24);
//#endif

    //ms = pc->mi - 1;

    if (vp8_read_bit(bc))
    {
        int i = 0;

        do
        {
            pc->fc.ymode_prob[i] = (vp8_prob) vp8_read_literal(bc, 8);
        }
        while (++i < 4);
    }

    if (vp8_read_bit(bc))
    {
        int i = 0;

        do
        {
            pc->fc.uv_mode_prob[i] = (vp8_prob) vp8_read_literal(bc, 8);
        }
        while (++i < 3);
    }
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG9 |= (pc->fc.ymode_prob[0] << 0);
	g_fh_reg_ptr->FH_CFG9 |= (pc->fc.ymode_prob[1] << 8);
	g_fh_reg_ptr->FH_CFG9 |= (pc->fc.ymode_prob[2] << 16);
	g_fh_reg_ptr->FH_CFG9 |= (pc->fc.ymode_prob[3] << 24);
	g_fh_reg_ptr->FH_CFG10 |= (pc->fc.uv_mode_prob[0] << 0);
	g_fh_reg_ptr->FH_CFG10 |= (pc->fc.uv_mode_prob[1] << 8);
	g_fh_reg_ptr->FH_CFG10 |= (pc->fc.uv_mode_prob[2] << 16);
#endif
    read_mvcontexts(bc, mvc);
#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
//		uint32 s_size;
		vpx_memcpy((void*)g_fh_reg_ptr->FH_MV_PROBS, pc->fc.mvc, sizeof(uint8)*2*MVPcount);
		// Assume VP8_BOOL_DECODER_SZ is large enough (>2K Bytes) to prevent multiple write before MB parser
//		s_size = (g_stream_offset-pbi->source_sz) + 3 + (bc->read_ptr - bc->decode_buffer);	// relative to the file start
//		Print_Stream_Offset(s_size, "Frame Header End");
		g_fh_reg_ptr->FH_CFG12 |= bc->range;
		g_fh_reg_ptr->FH_CFG12 |= (bc->value << 8);
		g_fh_reg_ptr->FH_CFG12 |= (bc->count << 24);
		//Print_Stream_Offset(bc->range, "Bool Decoder Range");
		//Print_Stream_Offset(bc->value, "Bool Decoder Value");
		//Print_Stream_Offset(bc->count, "Bool Decoder Count");
	}
#endif

	// OpenRISC Init
	ORSC_Init(pbi);
	Write_tbuf_Probs(pbi);

	//OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
	//OR1200_WRITE_REG(ORSC_BSM_CTRL_OF+0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
	//OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|(bs_buffer_length+0x30)&0xfffffffc, "ORSC: BSM_CFG0 stream buffer size"); // Set each time after BSM clear
#if 1
	OR1200_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_2 | V_BIT_4 | V_BIT_5,"VSP_INT_MASK, enable mbw_slice_done, vld_err, time_out");//enable int //frame done/error/timeout
#else	
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x04, 0, "ORSC: VSP_INT_MASK: BSM_FRM_DONE"); // enable HW INT
#endif
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x30, 0xa, "ORSC: VSP_START: DECODE_START=1");
	//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
	//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));//MCU_SLEEP

#ifdef SIM_IN_WIN
	or1200_print = 0;
	FPRINTF_ORSC(g_vsp_glb_reg_fp, "//***********************************frame num=%d slice id=%d\n", g_nFrame_dec, 0);
    while (++mb_row < pc->mb_rows)
    {
        int mb_col = -1;

        while (++mb_col < pc->mb_cols)
        {
            MB_MODE_INFO *const mbmi = & mi->mbmi;
            MV *const mv = & mbmi->mv.as_mv;
            VP8_COMMON *const pc = &pbi->common;
            MACROBLOCKD *xd = &pbi->mb;

            //vp8dx_bool_decoder_fill_c(bc);

            // Distance of Mb to the various image edges.
            // These specified to 8th pel as they are always compared to MV values that are in 1/8th pel units
            xd->mb_to_left_edge = -((mb_col * 16) << 3);
            xd->mb_to_right_edge = ((pc->mb_cols - 1 - mb_col) * 16) << 3;
            xd->mb_to_top_edge = -((mb_row * 16)) << 3;
            xd->mb_to_bottom_edge = ((pc->mb_rows - 1 - mb_row) * 16) << 3;

            // If required read in new segmentation data for this MB
            if (pbi->mb.update_mb_segmentation_map)
                vp8_read_mb_features(bc, mbmi, &pbi->mb);

            // Read the macroblock coeff skip flag if this feature is in use, else default to 0
            if (pc->mb_no_coeff_skip)
                mbmi->mb_skip_coeff = vp8_read(bc, prob_skip_false);
            else
                mbmi->mb_skip_coeff = 0;

            mbmi->uv_mode = DC_PRED;

            if ((mbmi->ref_frame = (MV_REFERENCE_FRAME) vp8_read(bc, prob_intra)))    /* inter MB */
            {
                int rct[4];
                vp8_prob mv_ref_p [VP8_MVREFS-1];
                MV nearest, nearby, best_mv;

                if (vp8_read(bc, prob_last))
                {
                    mbmi->ref_frame = (MV_REFERENCE_FRAME)((int)mbmi->ref_frame + (int)(1 + vp8_read(bc, prob_gf)));
                }

                vp8_find_near_mvs(xd, mi, &nearest, &nearby, &best_mv, rct, mbmi->ref_frame, pbi->common.ref_frame_sign_bias);

                vp8_mv_ref_probs(mv_ref_p, rct);

                switch (mbmi->mode = read_mv_ref(bc, mv_ref_p))
                {
                case SPLITMV:
                {
                    const int s = mbmi->partitioning = vp8_treed_read(
                                                           bc, vp8_mbsplit_tree, vp8_mbsplit_probs
                                                       );
                    const int num_p = vp8_mbsplit_count [s];
                    const int *const  L = vp8_mbsplits [s];
                    int j = 0;

                    do  /* for each subset j */
                    {
                        B_MODE_INFO *const bmi = mbmi->partition_bmi + j;
                        MV *const mv = & bmi->mv.as_mv;

                        int k = -1;  /* first block in subset j */
                        int mv_contz;

                        while (j != L[++k])
                            if (k >= 16)
#if CONFIG_DEBUG
                                assert(0);
#else
                                ;
#endif

                        mv_contz = vp8_mv_cont(&(vp8_left_bmi(mi, k)->mv.as_mv), &(vp8_above_bmi(mi, k, mis)->mv.as_mv));

                        switch (bmi->mode = (B_PREDICTION_MODE) sub_mv_ref(bc, vp8_sub_mv_ref_prob2 [mv_contz])) //pc->fc.sub_mv_ref_prob))
                        {
                        case NEW4X4:
                            read_mv(bc, mv, (const MV_CONTEXT *) mvc);
#ifdef TV_OUT
							//bmi->mvd = *mv;
#endif
                            mv->row += best_mv.row;
                            mv->col += best_mv.col;
							vp8_clamp_mv(mv, xd);	// derek
#ifdef VPX_MODE_COUNT
                            vp8_mv_cont_count[mv_contz][3]++;
#endif
                            break;
                        case LEFT4X4:
                            *mv = vp8_left_bmi(mi, k)->mv.as_mv;
#ifdef VPX_MODE_COUNT
                            vp8_mv_cont_count[mv_contz][0]++;
#endif
                            break;
                        case ABOVE4X4:
                            *mv = vp8_above_bmi(mi, k, mis)->mv.as_mv;
#ifdef VPX_MODE_COUNT
                            vp8_mv_cont_count[mv_contz][1]++;
#endif
                            break;
                        case ZERO4X4:
                            *mv = Zero;
#ifdef VPX_MODE_COUNT
                            vp8_mv_cont_count[mv_contz][2]++;
#endif
                            break;
                        default:
                            break;
                        }

                        /* Fill (uniform) modes, mvs of jth subset.
                           Must do it here because ensuing subsets can
                           refer back to us via "left" or "above". */
                        do
                            if (j == L[k])
                                mi->bmi[k] = *bmi;
                        while (++k < 16);

						switch(s)
						{
						case 0://16x8
							{
								int32 int_type = pc->use_bilinear_mc_filter;
								int32 ref_blk_end = FALSE;
								int32 ref_bir_blk = FALSE;
								int32 ref_cmd_type = 3;//vp8
								int32 ref_bw_frame_id = 0;
								int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
								int32 ref_blk_id = j*2;
								int32 ref_blk_size = MC_BLKSIZE_8x8;
								int32 mc_blk_info;
								int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
								mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
									(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
							}
							{
								int32 int_type = pc->use_bilinear_mc_filter;
								int32 ref_blk_end = (j==1)?TRUE:FALSE;
								int32 ref_bir_blk = FALSE;
								int32 ref_cmd_type = 3;//vp8
								int32 ref_bw_frame_id = 0;
								int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
								int32 ref_blk_id =j*2 +1;
								int32 ref_blk_size = MC_BLKSIZE_8x8;
								int32 mc_blk_info;
								int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
								mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
									(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
							}
							break;
						case 1://8x16
							{
								int32 int_type = pc->use_bilinear_mc_filter;
								int32 ref_blk_end = FALSE;
								int32 ref_bir_blk = FALSE;
								int32 ref_cmd_type = 3;//vp8
								int32 ref_bw_frame_id = 0;
								int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
								int32 ref_blk_id = j;
								int32 ref_blk_size = MC_BLKSIZE_8x8;
								int32 mc_blk_info;
								int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
								mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
							}
							{
								int32 int_type = pc->use_bilinear_mc_filter;
								int32 ref_blk_end = (j==1)?TRUE:FALSE;
								int32 ref_bir_blk = FALSE;
								int32 ref_cmd_type = 3;//vp8
								int32 ref_bw_frame_id = 0;
								int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
								int32 ref_blk_id = j+2;
								int32 ref_blk_size = MC_BLKSIZE_8x8;
								int32 mc_blk_info;
								int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
								mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
							}
							break;
						case 2://8x8
							{
								{
									int32 int_type = pc->use_bilinear_mc_filter;
									int32 ref_blk_end = (j==3)?TRUE:FALSE;
									int32 ref_bir_blk = FALSE;
									int32 ref_cmd_type = 3;//vp8
									int32 ref_bw_frame_id = 0;
									int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
									int32 ref_blk_id = j;
									int32 ref_blk_size = MC_BLKSIZE_8x8;
									int32 mc_blk_info;
									int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
									mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
									VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
									VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
								}
							}
							break;
						case 3://4x4
							{
								{
									int32 int_type = pc->use_bilinear_mc_filter;
									int32 ref_blk_end = (j==15)?TRUE:FALSE;
									int32 ref_bir_blk = FALSE;
									int32 ref_cmd_type = 3;//vp8
									int32 ref_bw_frame_id = 0;
									int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
									int32 ref_blk_id = j;
									int32 ref_blk_size = MC_BLKSIZE_4x4;
									int32 mc_blk_info;
									int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
									mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
									VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
									VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
								}
							}
							break;
						}
                    }
                    while (++j < num_p);
                }
                *mv = mi->bmi[15].mv.as_mv;
                break;  /* done with SPLITMV */

                case NEARMV:
                    *mv = nearby;
                    // Clip "next_nearest" so that it does not extend to far out of image
					//vp8_clamp_mv(mv, xd);	// derek
                    goto propagate_mv;

                case NEARESTMV:
                    *mv = nearest;
                    // Clip "next_nearest" so that it does not extend to far out of image
                    //vp8_clamp_mv(mv, xd);	// derek					
                    goto propagate_mv;
					
                case ZEROMV:
                    *mv = Zero;
                    goto propagate_mv;
					
                case NEWMV:
                    read_mv(bc, mv, (const MV_CONTEXT *) mvc);
#ifdef TV_OUT
					//mbmi->mvd = *mv;
#endif
                    mv->row += best_mv.row;
                    mv->col += best_mv.col;
                    /* Encoder should not produce invalid motion vectors, but since
                     * arbitrary length MVs can be parsed from the bitstream, we
                     * need to clamp them here in case we're reading bad data to
                     * avoid a crash.
                     */
					vp8_clamp_mv(mv, xd);	// derek
#if CONFIG_DEBUG
                    assert(mv->col >= (xd->mb_to_left_edge - LEFT_TOP_MARGIN));
                    assert(mv->col <= (xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN));
                    assert(mv->row >= (xd->mb_to_top_edge - LEFT_TOP_MARGIN));
                    assert(mv->row <= (xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN));
#endif

                propagate_mv:  /* same MV throughout */
                    {
                        //int i=0;
                        //do
                        //{
                        //  mi->bmi[i].mv.as_mv = *mv;
                        //}
                        //while( ++i < 16);

                        mi->bmi[0].mv.as_mv = *mv;
                        mi->bmi[1].mv.as_mv = *mv;
                        mi->bmi[2].mv.as_mv = *mv;
                        mi->bmi[3].mv.as_mv = *mv;
                        mi->bmi[4].mv.as_mv = *mv;
                        mi->bmi[5].mv.as_mv = *mv;
                        mi->bmi[6].mv.as_mv = *mv;
                        mi->bmi[7].mv.as_mv = *mv;
                        mi->bmi[8].mv.as_mv = *mv;
                        mi->bmi[9].mv.as_mv = *mv;
                        mi->bmi[10].mv.as_mv = *mv;
                        mi->bmi[11].mv.as_mv = *mv;
                        mi->bmi[12].mv.as_mv = *mv;
                        mi->bmi[13].mv.as_mv = *mv;
                        mi->bmi[14].mv.as_mv = *mv;
                        mi->bmi[15].mv.as_mv = *mv;
#ifdef TV_OUT
						/*mi->bmi[0].mvd = mbmi->mvd;
                        mi->bmi[1].mvd = mbmi->mvd;
                        mi->bmi[2].mvd = mbmi->mvd;
                        mi->bmi[3].mvd = mbmi->mvd;
                        mi->bmi[4].mvd = mbmi->mvd;
                        mi->bmi[5].mvd = mbmi->mvd;
                        mi->bmi[6].mvd = mbmi->mvd;
                        mi->bmi[7].mvd = mbmi->mvd;
                        mi->bmi[8].mvd = mbmi->mvd;
                        mi->bmi[9].mvd = mbmi->mvd;
                        mi->bmi[10].mvd = mbmi->mvd;
                        mi->bmi[11].mvd = mbmi->mvd;
                        mi->bmi[12].mvd = mbmi->mvd;
                        mi->bmi[13].mvd = mbmi->mvd;
                        mi->bmi[14].mvd = mbmi->mvd;
                        mi->bmi[15].mvd = mbmi->mvd;*/
#endif
                    }
#if 1
			{
				int32 int_type = pc->use_bilinear_mc_filter;
				int32 ref_blk_end = TRUE;
				int32 ref_bir_blk = FALSE;
				int32 ref_cmd_type = 3;//vp8
				int32 ref_bw_frame_id = 0;
				int32 ref_fw_frame_id = (mi->mbmi.ref_frame-1);
				int32 ref_blk_id = 0;
				int32 ref_blk_size = MC_BLKSIZE_16x16;
				int32 mc_blk_info;
				int32 mv_cmd = (mv->row << 16)|(mv->col & 0xffff);
				mc_blk_info = (int_type << 28) | (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
			}
#endif
                    break;

                default:;
#if CONFIG_DEBUG
                    assert(0);
#endif
                }

            }
            else
            {
                /* MB is intra coded */

                int j = 0;

#ifdef MCA_TV_SPLIT
				intra_count++;
#endif

                do
                {
                    mi->bmi[j].mv.as_mv = Zero;
                }
                while (++j < 16);

                *mv = Zero;

                if ((mbmi->mode = (MB_PREDICTION_MODE) vp8_read_ymode(bc, pc->fc.ymode_prob)) == B_PRED)
                {
                    int j = 0;

                    do
                    {
                        mi->bmi[j].mode = (B_PREDICTION_MODE)vp8_read_bmode(bc, pc->fc.bmode_prob);
                    }
                    while (++j < 16);
                }

                mbmi->uv_mode = (MB_PREDICTION_MODE)vp8_read_uv_mode(bc, pc->fc.uv_mode_prob);
            }
			//vp8_build_uvmvs(xd, pc->full_pixel);

            mi++;       // next macroblock
        }	// end of while (++mb_col < pc->mb_cols)

        mi++;           // skip left predictor each row
    }		// end of while (++mb_row < pc->mb_rows)
#endif

#ifdef MCA_TV_SPLIT
	if( (g_nFrame_dec%FRAME_X) == 0 )
	{
		unsigned char s[100];
		FILE *dir_config;
		sprintf(s, "../../test_vectors/mca/frame_%03d/config.txt", g_nFrame_dec);
		assert(NULL != (dir_config = fopen(s, "w")));
		fprintf (dir_config, "//vsp_standard\n");
		fprintf (dir_config, "// h263 =0 , mp4=1, vp8=2,flv=3,h264=4,real8=5,real9=6\n");
		fprintf (dir_config, "2\n");
		fprintf (dir_config, "//block number\n");
		fprintf (dir_config, "%x\n", (pc->MBs-intra_count));
		fprintf (dir_config, "//pic x size\n");
		fprintf (dir_config, "%x\n", (pc->Width+15)&0xfffffff0);
		fprintf (dir_config, "//pic y size\n");
		fprintf (dir_config, "%x\n", (pc->Height+15)&0xfffffff0);
		fprintf (dir_config, "//weight enable\n");
		fprintf (dir_config, "0\n");
		fclose(dir_config);
	}
#endif
}
