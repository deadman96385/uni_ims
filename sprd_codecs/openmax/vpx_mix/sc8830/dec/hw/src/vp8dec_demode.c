
#include "vp8_mode.h"
#include "vp8_find_nearmv.h"
#include "vp8_entropy_mode.h"
#include "vp8dec_mode.h"
#include "vp8dec_treereader.h"
#include "vp8dec_demode.h"//weihu
#ifdef SIM_IN_WIN
	#include "vp8_test_vectors.h" //derek
#else
	#include "vp8dec_global.h"
#endif
#include <stdio.h>

int vp8_read_bmode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_bmode_tree, p);

    return i;
}


int vp8_read_ymode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_ymode_tree, p);

    return i;
}

int vp8_kfread_ymode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_kf_ymode_tree, p);

    return i;
}

int vp8_read_uv_mode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_uv_mode_tree, p);

    return i;
}

void vp8_read_mb_features(vp8_reader *r, MB_MODE_INFO *mi, MACROBLOCKD *x)
{
    // Is segmentation enabled
    if (x->segmentation_enabled && x->update_mb_segmentation_map)
    {
        // If so then read the segment id.
        if (vp8_read(r, x->mb_segment_tree_probs[0]))
            mi->segment_id = (unsigned char)(2 + vp8_read(r, x->mb_segment_tree_probs[2]));
        else
            mi->segment_id = (unsigned char)(vp8_read(r, x->mb_segment_tree_probs[1]));
    }
}
//#define mode_trace
void vp8_kfread_modes(VP8D_COMP *pbi)
{
    VP8_COMMON *const pc = & pbi->common;
    vp8_reader *const bc = & pbi->bc;

#ifdef SIM_IN_WIN
    MODE_INFO *m = pc->mi;
    const int ms = pc->mode_info_stride;
	int mb_row = -1;
#endif

    vp8_prob prob_skip_false = 0;

#ifdef mode_trace
	FILE * mode_trace_fp = fopen("D:/trace_mode.txt","wb");
	int j;
#endif
	
    if (pc->mb_no_coeff_skip)
        prob_skip_false = (vp8_prob)(vp8_read_literal(bc, 8));
	g_fh_reg_ptr->FH_CFG8 |= (prob_skip_false << 0);


	
#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
//		uint32 s_size;
		// Assume VP8_BOOL_DECODER_SZ is large enough (>2K Bytes) to prevent multiple write before MB parser
//		s_size = (g_stream_offset-pbi->source_sz) + 10 + (bc->read_ptr - bc->decode_buffer);	// relative to the file start
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
	//OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
	//OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|(bs_buffer_length+0x30)&0xfffffffc, "ORSC: BSM_CFG0 stream buffer size"); // Set each time after BSM clear
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x04, 0, "ORSC: VSP_INT_MASK: BSM_FRM_DONE"); // enable HW INT
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x30, 0xa, "ORSC: VSP_START: DECODE_START=1");
	//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
	//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));//MCU_SLEEP

#ifdef SIM_IN_WIN
	or1200_print = 1;
	FPRINTF_ORSC(g_vsp_glb_reg_fp, "//***********************************frame num=%d slice id=%d\n", g_nFrame_dec, 0);
    while (++mb_row < pc->mb_rows)
    {
        int mb_col = -1;

        while (++mb_col < pc->mb_cols)
        {
            MB_PREDICTION_MODE y_mode;

			if(mb_row == 52 && mb_col == 57)
				printf("");

            //vp8dx_bool_decoder_fill_c(bc);
            // Read the Macroblock segmentation map if it is being updated explicitly this frame (reset to 0 above by default)
            // By default on a key frame reset all MBs to segment 0
            m->mbmi.segment_id = 0;

            if (pbi->mb.update_mb_segmentation_map)
                vp8_read_mb_features(bc, &m->mbmi, &pbi->mb);

            // Read the macroblock coeff skip flag if this feature is in use, else default to 0
            if (pc->mb_no_coeff_skip)
                m->mbmi.mb_skip_coeff = vp8_read(bc, prob_skip_false);
            else
                m->mbmi.mb_skip_coeff = 0;

            y_mode = (MB_PREDICTION_MODE) vp8_kfread_ymode(bc, pc->kf_ymode_prob);

            m->mbmi.ref_frame = INTRA_FRAME;

            if ((m->mbmi.mode = y_mode) == B_PRED)
            {
                int i = 0;

                do
                {
                    const B_PREDICTION_MODE A = vp8_above_bmi(m, i, ms)->mode;
                    const B_PREDICTION_MODE L = vp8_left_bmi(m, i)->mode;

                    m->bmi[i].mode = (B_PREDICTION_MODE) vp8_read_bmode(bc, pc->kf_bmode_prob [A] [L]);
                }
                while (++i < 16);
            }
            else
            {
                int BMode;
                int i = 0;

                switch (y_mode)
                {
                case DC_PRED:
                    BMode = B_DC_PRED;
                    break;
                case V_PRED:
                    BMode = B_VE_PRED;
                    break;
                case H_PRED:
                    BMode = B_HE_PRED;
                    break;
                case TM_PRED:
                    BMode = B_TM_PRED;
                    break;
                default:
                    BMode = B_DC_PRED;
                    break;
                }

                do
                {
                    m->bmi[i].mode = (B_PREDICTION_MODE)BMode;
                }
                while (++i < 16);
            }
#ifdef mode_trace
			fprintf(mode_trace_fp,"mb_row:%d,mb_col:%d:\n",mb_row,mb_col);
			for ( j=0;j<16;j++)
			{
				fprintf(mode_trace_fp,"%d\n",m->bmi[j].mode);
			}

#endif
            (m++)->mbmi.uv_mode = (MB_PREDICTION_MODE)vp8_read_uv_mode(bc, pc->kf_uv_mode_prob);
        }

        m++; // skip the border
    }
#endif
}
