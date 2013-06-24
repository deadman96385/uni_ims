
#include "video_common.h"
#include "sci_types.h"
#include "vp8_mode.h"
#include "vp8_entropy.h"
#include "vp8_subpix.h"
#include "vp8_setup_intra_recon.h"
#include "vp8_alloc_common.h"
#include "vp8dec_mode.h"
#include "vp8dec_treereader.h"
#include "vp8dec_dboolhuff.h"
#include "vp8dec_mv.h"
#include "vp8dec_demode.h"
#include "vp8dec_dequant.h"//weihu
#include "vp8_entropy_mode.h"//weihu


#include "sc8810_video_header.h"

#ifdef SIM_IN_WIN
#include "vp8dec_detokenize.h"
#include "vp8dec_mb.h"
#include "vp8_reconinter.h"//weihu
#include "mca_global.h"//weihu
#include "vp8_extend.h"//weihu
#include "vp8_test_vectors.h" // derek
#endif

#ifdef SIM_IN_WIN
unsigned char* y_buffer;
unsigned char* u_buffer;
unsigned char* v_buffer;

void foo()
{
	return;
}

void vp8_decode_macroblock(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int eobtotal = 0;

    if (xd->mbmi.mb_skip_coeff)
    {
        vp8_reset_mb_tokens_context(xd);
    }
    else
    {
        eobtotal = vp8_decode_mb_tokens(pbi, xd);
    }

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
		Print_VLD_Out(xd);
#ifdef MCA_TV_SPLIT
		Print_MCA_In(xd, pbi->common.filter_type==0);
#endif
	}
#endif

    xd->mode_info_context->mbmi.dc_diff = 1;
	if (xd->segmentation_enabled)
        mb_init_dequantizer(pbi, xd);

    if (xd->mbmi.mode != B_PRED && xd->mbmi.mode != SPLITMV && eobtotal == 0)
    {
        xd->mode_info_context->mbmi.dc_diff = 0;

#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_dec==FRAME_X)
#endif
			Print_IQIT_Out(xd, 1);
#endif
        skip_recon_mb(pbi, xd);
#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_dec==FRAME_X)
#endif
		{
			Print_PRED_Out(xd, 1);
			Print_REC_Out(xd);
		}
#endif
        return;
    }

//    if (xd->segmentation_enabled)
//        mb_init_dequantizer(pbi, xd);

    de_quantand_idct(pbi, xd);

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
		Print_IQIT_Out(xd, 0);
#endif
    reconstruct_mb(pbi, xd);
#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
		Print_PRED_Out(xd, 0);
		Print_REC_Out(xd);
	}
#endif
}
#endif	// SIM_IN_WIN


static int get_delta_q(vp8_reader *bc, int prev, int *q_update)
{
    int ret_val = 0;

    if (vp8_read_bit(bc))
    {
        ret_val = vp8_read_literal(bc, 4);

        if (vp8_read_bit(bc))
            ret_val = -ret_val;
    }

    /* Trigger a quantizer update if the delta-q value has changed */
    if (ret_val != prev)
        *q_update = 1;

    return ret_val;
}


#ifdef SIM_IN_WIN
void vp8_decode_mb_row(VP8D_COMP *pbi,
                       VP8_COMMON *pc,
                       int mb_row,
                       MACROBLOCKD *xd)
{

    int i;
    int recon_yoffset, recon_uvoffset;
    int mb_col;
    int recon_y_stride = pc->last_frame.y_stride;
    int recon_uv_stride = pc->last_frame.uv_stride;

    vpx_memset(pc->left_context, 0, sizeof(pc->left_context));
    recon_yoffset = mb_row * recon_y_stride * 16;
    recon_uvoffset = mb_row * recon_uv_stride * 8;
    // reset above block coeffs

    xd->above_context[Y1CONTEXT] = pc->above_context[Y1CONTEXT];
    xd->above_context[UCONTEXT ] = pc->above_context[UCONTEXT];
    xd->above_context[VCONTEXT ] = pc->above_context[VCONTEXT];
    xd->above_context[Y2CONTEXT] = pc->above_context[Y2CONTEXT];
    xd->up_available = (mb_row != 0);

	// Distance of Mb to the various image edges.
	// These specified to 8th pel as they are always compared to values that are in 1/8th pel units
    xd->mb_to_top_edge = -((mb_row * 16)) << 3;
    xd->mb_to_bottom_edge = ((pc->mb_rows - 1 - mb_row) * 16) << 3;

    for (mb_col = 0; mb_col < pc->mb_cols; mb_col++)
    {
		//if(mb_row == 0 && mb_col ==0 && g_nFrame_dec ==0x1 )
		//	printf("");

		VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, (mb_row << 8) | (mb_col << 0), "vsp global reg: configure current MB position");

        // Take a copy of the mode and Mv information for this macroblock into the xd->mbmi
        vpx_memcpy(&xd->mbmi, &xd->mode_info_context->mbmi, 33); //sizeof(MB_MODE_INFO) ); not all mbmi is used for xd->mbmi

        if (xd->mbmi.mode == SPLITMV || xd->mbmi.mode == B_PRED)
        {
            for (i = 0; i < 16; i++)
            {
                BLOCKD *d = &xd->block[i];
                vpx_memcpy(&d->bmi, &xd->mode_info_context->bmi[i], sizeof(B_MODE_INFO));
            }
        }
#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_dec==FRAME_X)
#endif
		{
			Write_BUF_REG(mb_row, mb_col, xd, pc);	// with reset
			Print_Partition_Buf(xd);
		}
#endif
        // Distance of Mb to the various image edges.
        // These specified to 8th pel as they are always compared to values that are in 1/8th pel units
        xd->mb_to_left_edge = -((mb_col * 16) << 3);
        xd->mb_to_right_edge = ((pc->mb_cols - 1 - mb_col) * 16) << 3;

        xd->dst.y_buffer = pc->new_frame.y_buffer + recon_yoffset;
        xd->dst.u_buffer = pc->new_frame.u_buffer + recon_uvoffset;
        xd->dst.v_buffer = pc->new_frame.v_buffer + recon_uvoffset;

        xd->left_available = (mb_col != 0);

        // Select the appropriate reference frame for this MB
		if (xd->mbmi.ref_frame == LAST_FRAME)
        {
            xd->pre.y_buffer = pc->last_frame.y_buffer + recon_yoffset;
            xd->pre.u_buffer = pc->last_frame.u_buffer + recon_uvoffset;
            xd->pre.v_buffer = pc->last_frame.v_buffer + recon_uvoffset;
        }
        else if (xd->mbmi.ref_frame == GOLDEN_FRAME)
        {
            // Golden frame reconstruction buffer
            xd->pre.y_buffer = pc->golden_frame.y_buffer + recon_yoffset;
            xd->pre.u_buffer = pc->golden_frame.u_buffer + recon_uvoffset;
            xd->pre.v_buffer = pc->golden_frame.v_buffer + recon_uvoffset;
        }
        else
        {
            // Alternate reference frame reconstruction buffer
            xd->pre.y_buffer = pc->alt_ref_frame.y_buffer + recon_yoffset;
            xd->pre.u_buffer = pc->alt_ref_frame.u_buffer + recon_uvoffset;
            xd->pre.v_buffer = pc->alt_ref_frame.v_buffer + recon_uvoffset;
        }

		// Derek 2012-08-03
		vp8_build_block_doffsets(xd);

		// For mca_core_vp8.c
		if (xd->mbmi.ref_frame == LAST_FRAME)
		{
			y_buffer =  pc->last_frame.y_buffer;
			u_buffer =  pc->last_frame.u_buffer;
			v_buffer =  pc->last_frame.v_buffer;
		}
		else if (xd->mbmi.ref_frame == GOLDEN_FRAME)
		{
			// Golden frame reconstruction buffer
			y_buffer =  pc->golden_frame.y_buffer;
			u_buffer =  pc->golden_frame.u_buffer;
			v_buffer =  pc->golden_frame.v_buffer;
		}
		else
		{
			// Alternate reference frame reconstruction buffer
			y_buffer =  pc->alt_ref_frame.y_buffer;
			u_buffer =  pc->alt_ref_frame.u_buffer;
			v_buffer =  pc->alt_ref_frame.v_buffer;
		}

        vp8_build_uvmvs(xd, pc->full_pixel);

        /*
        if(pbi->common.current_video_frame==0 &&mb_col==1 && mb_row==0)
        pbi->debugoutput =1;
        else
        pbi->debugoutput =0;
        */
        //vp8dx_bool_decoder_fill_c(xd->current_bc);
		/*if(mb_row == 0 && mb_col == 4)
		{
			foo();
		}*/

        vp8_decode_macroblock(pbi, xd);

#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_dec==FRAME_X)
#endif
		{
			g_isyn_buf_ptr->ISYN_CFG0 |= (xd->mode_info_context->mbmi.dc_diff << 24);
			// Print Parameter Buffers
			Print_ISYN_Buf();	// CBP is written in vp8_decode_macroblock( )
			g_ppal_buf_ptr->PPAL_CFG0 |= (((g_isyn_buf_ptr->ISYN_CFG1 >> 12)&0xf) << 3);	// CBP_BLK[6:3]
			Print_PPA_Line_Buf();
			g_mbc_reg_ptr->VP8_CFG1 |= (g_isyn_buf_ptr->ISYN_CFG1&0xffffff);
			Print_MBC_Buf();
			Print_MCA_Buf(xd);	// Related to Print_ISYN_Buf
			Print_DCT_Buf(xd, pc);
		}
#endif

        recon_yoffset += 16;
        recon_uvoffset += 8;

        ++xd->mode_info_context;  /* next mb */

        //xd->gf_active_ptr++;      // GF useage flag for next MB

        xd->above_context[Y1CONTEXT] += 4;
        xd->above_context[UCONTEXT ] += 2;
        xd->above_context[VCONTEXT ] += 2;
        xd->above_context[Y2CONTEXT] ++;

        pbi->current_mb_col_main = mb_col;
    }

    // adjust to the next row of mbs
    vp8_extend_mb_row(
        &pc->new_frame,
        xd->dst.y_buffer + 16, xd->dst.u_buffer + 8, xd->dst.v_buffer + 8
    );

    ++xd->mode_info_context;      /* skip prediction column */

    pbi->last_mb_row_decoded = mb_row;
}

static unsigned int read_partition_size(const unsigned char *cx_size)
{
    const unsigned int size =
        cx_size[0] + (cx_size[1] << 8) + (cx_size[2] << 16);
    return size;
}

static void setup_token_decoder(VP8D_COMP *pbi,
                                const unsigned char *cx_data)
{
    int num_part;
    int i;
    VP8_COMMON          *pc = &pbi->common;
    const unsigned char *user_data_end = pbi->Source + pbi->source_sz;
    vp8_reader          *bool_decoder;
    const unsigned char *partition;

    /* Parse number of token partitions to use */
    pc->multi_token_partition = (TOKEN_PARTITION)vp8_read_literal(&pbi->bc, 2);	// log2_nbr_of_dct_partitions

	or1200_print = 0;

	g_fh_reg_ptr->FH_CFG0 |= (pc->multi_token_partition << 19);
    num_part = 1 << pc->multi_token_partition;

    /* Set up pointers to the first partition */
    partition = cx_data;
    bool_decoder = &pbi->bc2;
	
    if (num_part > 1)
    {
     //   CHECK_MEM_ERROR(pbi->mbc, vpx_malloc(num_part * sizeof(vp8_reader)));
		pbi->mbc = vpx_malloc(num_part * sizeof(vp8_reader));
        bool_decoder = pbi->mbc;
        partition += 3 * (num_part - 1);
    }

    for (i = 0; i < num_part; i++)
    {
        const unsigned char *partition_size_ptr = cx_data + i * 3;
        unsigned int         partition_size;
		//unsigned char s[30];

        /* Calculate the length of this partition. The last partition
         * size is implicit.
         */
        if (i < num_part - 1)
        {
            partition_size = read_partition_size(partition_size_ptr);
        }
        else
        {
            partition_size = user_data_end - partition;
        }

        if (partition + partition_size > user_data_end)
		{
			pc->error_flag = 1;
		}

        if (vp8dx_start_decode_c(bool_decoder, /*IF_RTCD(&pbi->dboolhuff),*/
                               partition, partition_size))
		{
			pc->error_flag = 1;
		}

//#ifdef TV_OUT
		//sprintf(s, "DCT Partition %d", i);
		//Print_Stream_Offset( ((g_stream_offset-pbi->source_sz)+(partition-pbi->Source)), s);
		if(i==0)
			g_fh_reg_ptr->FH_CFG11 = 12 + (partition-pbi->Source);
			//g_fh_reg_ptr->FH_CFG11 = ((g_stream_offset-pbi->source_sz)+(partition-pbi->Source));	// just offset from input_bs_bfr_ptr
//#endif
        /* Advance to the next partition */
        partition += partition_size;
        bool_decoder++;
    }

	or1200_print = 1;

    /* Clamp number of decoder threads */
//	if (pbi->decoding_thread_count > num_part - 1)
//		pbi->decoding_thread_count = num_part - 1;
}

static void stop_token_decoder(VP8D_COMP *pbi)
{
    int i;
    VP8_COMMON *pc = &pbi->common;

    if (pc->multi_token_partition != ONE_PARTITION)
    {
        int num_part = (1 << pc->multi_token_partition);

        for (i = 0; i < num_part; i++)
        {
            vp8dx_stop_decode_c(&pbi->mbc[i]);
        }

        vpx_free(pbi->mbc);
    }
    else
        vp8dx_stop_decode_c(& pbi->bc2);
}
#endif // SIM_IN_WIN


static void init_frame(VP8D_COMP *pbi)
{
    VP8_COMMON *const pc = & pbi->common;
    MACROBLOCKD *const xd  = & pbi->mb;

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
#ifdef SIM_IN_WIN
    else
    {
        // To enable choice of different interpolation filters
        if (!pc->use_bilinear_mc_filter)
        {
            xd->subpixel_predict      = vp8_sixtap_predict_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap4x4);
            xd->subpixel_predict8x4   = vp8_sixtap_predict8x4_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x4);
            xd->subpixel_predict8x8   = vp8_sixtap_predict8x8_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x8);
            xd->subpixel_predict16x16 = vp8_sixtap_predict16x16_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap16x16);
        }
        else
        {
            xd->subpixel_predict      = vp8_bilinear_predict4x4_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear4x4);
            xd->subpixel_predict8x4   = vp8_bilinear_predict8x4_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x4);
            xd->subpixel_predict8x8   = vp8_bilinear_predict8x8_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x8);
            xd->subpixel_predict16x16 = vp8_bilinear_predict16x16_c;//SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear16x16);
        }
    }

    xd->left_context = pc->left_context;
    xd->mode_info_context = pc->mi;
	xd->mode_info_stride = pc->mode_info_stride;
	xd->mbmi.mode = DC_PRED;
#endif
    xd->frame_type = pc->frame_type;
}


int vp8_decode_frame(VP8D_COMP *pbi)
{
	const unsigned char *data		= (const unsigned char *)pbi->Source;
	const unsigned char *data_end	= (const unsigned char *)(data + pbi->source_sz);
#ifdef SIM_IN_WIN
	int cmd;
	int mb_row;
#else
	uint32 tmp;
#endif
	vp8_reader *const bc = & pbi->bc;
    VP8_COMMON *const pc = & pbi->common;
    MACROBLOCKD *const xd  = & pbi->mb;
    int first_partition_length_in_bytes;

    int i, j, k, l;
    const int *const mb_feature_data_bits = vp8_mb_feature_data_bits;

	pc->error_flag = 0;
	memset(g_fh_reg_ptr, 0, sizeof(VSP_FH_REG_T));

#if 1 //def SIM_IN_WIN
	// 3 byte header
    pc->frame_type = (FRAME_TYPE)(data[0] & 1);
    pc->version = (data[0] >> 1) & 7;
    pc->show_frame = (data[0] >> 4) & 1;
    first_partition_length_in_bytes =
        (data[0] | (data[1] << 8) | (data[2] << 16)) >> 5;
    data += 3;
	BitstreamReadBits(8*3);
	
#else
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG0 |= pc->frame_type;
	g_fh_reg_ptr->FH_CFG0 |= (pc->version << 1);
	g_fh_reg_ptr->FH_CFG0 |= (pc->show_frame << 4);
#endif

	init_mca_cmd();//jin 

	{
		tmp = BitstreamReadBits(8*3);
		tmp = ((tmp&0xff)<<16) | (tmp&0xff00) | ((tmp&0xff0000)>>16);
		pc->frame_type = (FRAME_TYPE)(tmp & 1);
		pc->version = (tmp >> 1) & 7;
		pc->show_frame = (tmp >> 4) & 1;
		first_partition_length_in_bytes = (tmp >> 5);
		data += 3;
	}
#endif

	if (data + first_partition_length_in_bytes > data_end)
	{
		pc->error_flag = 1;
		return MMDEC_STREAM_ERROR;
	}

    vp8_setup_version(pc);


    if (pc->frame_type == KEY_FRAME)
    {
        const int Width = pc->Width;
        const int Height = pc->Height;

#if 1//def SIM_IN_WIN
        // vet via sync code
        if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
		{
			pc->error_flag = 1;
			//printf ("Invalid frame sync code");
			return MMDEC_STREAM_ERROR;
		}

        pc->Width = (data[3] | (data[4] << 8)) & 0x3fff;
        pc->horiz_scale = data[4] >> 6;
        pc->Height = (data[5] | (data[6] << 8)) & 0x3fff;
        pc->vert_scale = data[6] >> 6;
        data += 7;
		BitstreamReadBits(8*3);
		BitstreamReadBits(8*2);
		BitstreamReadBits(8*2);
#else
		if(BitstreamReadBits(8*3) != 0x9d012a)
		{
			pc->error_flag = 1;
			return -1;
		}
		tmp = BitstreamReadBits(8*2);
		tmp = ((tmp&0xff)<<8) | ((tmp&0xff00)>>8);
		pc->Width = (tmp & 0x3fff);
		pc->horiz_scale = (tmp >> 14);
		tmp = BitstreamReadBits(8*2);
		tmp = ((tmp&0xff)<<8) | ((tmp&0xff00)>>8);
        pc->Height = (tmp & 0x3fff);
        pc->vert_scale = (tmp >> 14);
		data += 7;
#endif
	 if ( (Width != pc->Width) || (Height != pc->Height) )	// Resolution Changed
	 {
	             if (pc->Width <= 0)
	            {
	                pc->Width = Width;
					pc->error_flag = 1;
#ifdef SIM_IN_WIN
					printf ("Invalid frame width");
#endif
					return MMDEC_STREAM_ERROR;
	            }

	            if (pc->Height <= 0)
	            {
	                pc->Height = Height;
					pc->error_flag = 1;
#ifdef SIM_IN_WIN
					printf ("Invalid frame height");
#endif
					return MMDEC_STREAM_ERROR;
	            }

		vp8_init_frame_buffers(pc);		
				
//		 pc->mb_rows = (pc->Height + 15)  >> 4;
//		 pc->mb_cols = (pc->Width + 15) >> 4;
//		 pc->MBs = pc->mb_rows * pc->mb_cols;

		return 	MMDEC_MEMORY_ALLOCED;
	 	



	 }


//	 pc->new_frame.u_buffer = pc->new_frame.y_buffer + (pc->MBs <<8) ;
//	 pc->new_frame.u_buffer_virtual =  pc->new_frame.y_buffer_virtual + (pc->MBs <<8);
      
        

#if 0

        if ( (Width != pc->Width) || (Height != pc->Height) )	// Resolution Changed
        {
            if (pc->Width <= 0)
            {
                pc->Width = Width;
				pc->error_flag = 1;
#ifdef SIM_IN_WIN
				printf ("Invalid frame width");
#endif
				return -1;
            }

            if (pc->Height <= 0)
            {
                pc->Height = Height;
				pc->error_flag = 1;
#ifdef SIM_IN_WIN
				printf ("Invalid frame height");
#endif
				return -1;
            }

			// maybe not needed to re-allocate if size is smaller ?
			video_size_get = 1;
			//video_buffer_malloced = 0;

			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x04, (video_size_get<<31)|((pc->Height&0xfff)<<12)|((pc->Width&0xfff)<<0), "ORSC_SHARE: shareRAM 0x4 IMAGE_SIZE");//保存到shareram
			// Need Calculation?
			total_extra_malloc_size = TOTAL_EXTRA_MALLOC_SIZE;
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x14, total_extra_malloc_size, "ORSC_SHARE: shareRAM 0x14 VSP_MEM1_SIZE"); //供arm分配新空间大小
			frame_buf_size = (pc->Width*pc->Height)>>1;	// FRAME_BUF_SIZE*8
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x34, frame_buf_size, "ORSC_SHARE: shareRAM 0x34 UV_FRAME_BUF_SIZE");

#ifdef SIM_IN_WIN
			//OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
#else
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
			g_not_first_reset = (tmp>>29)&1; //0
			cpu_will_be_reset = (tmp>>30)&1;
			if(cpu_will_be_reset == 1)//如果重新进入会reset，则需要保存现场寄存器
			{
				//保存现场寄存器
			}
//            OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x00, tmp&0x7fffffff, "ORSC_SHARE: MODE_CFG Stop");
//			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");
//			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
			//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));
			//返回arm，等待bsm buffer跟新或申请空间

			//polling until can run	
			OR1200_READ_REG_POLL(ORSC_SHARERAM_OFF+0, 0x80000000,0x80000000, "ORSC_SHARE: shareRAM 0x0 MODE_CFG run");
			//如果reset后重新进入，需恢复现场寄存器        
			if(g_not_first_reset == 1)	// var need update?
			{
				//恢复现场寄存器
			}
#endif
#ifdef SIM_IN_WIN
			OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
			OR1200_READ_REG(ORSC_SHARERAM_OFF+0x10, "ORSC_SHARE: shareRAM 0x10 VSP_MEM1_ST_ADDR");
#else
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
			input_buffer_update = (tmp>>8)&1; //1
			video_buffer_malloced = (tmp>>16)&1; //0
			g_not_first_reset = (tmp>>29)&1; //0
			cpu_will_be_reset = (tmp>>30)&1; //arm根据情况设置;
			if(video_buffer_malloced == 1)
			{
				MMCodecBuffer dec_malloc_bfr_ptr;

				video_size_get = 0;//避免再次进入
				extra_malloc_mem_start_addr = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x10, "ORSC_SHARE: shareRAM 0x10 VSP_MEM1_ST_ADDR");
				dec_malloc_bfr_ptr.common_buffer_ptr = (uint8 *)((uint32)extra_malloc_mem_start_addr-or_addr_offset);
				//g_extra_malloced_size = 0;
				dec_malloc_bfr_ptr.size = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x14, "ORSC_SHARE: shareRAM 0x14 VSP_MEM1_SIZE"); //total_extra_malloc_size;
				VP8DecMemInit(&dec_malloc_bfr_ptr);
			}
#endif
            if (vp8_alloc_frame_buffers(&pbi->common, pc->Width, pc->Height) < 0)
			{
				pc->error_flag = 1;
#ifdef SIM_IN_WIN
				printf ("Failed to allocate frame buffers");
#endif
				return -1;
			}
        }

#endif
    }
	 pc->new_frame.u_buffer = pc->new_frame.y_buffer + (pc->MBs <<8) ;
	 pc->new_frame.u_buffer_virtual =  pc->new_frame.y_buffer_virtual + (pc->MBs <<8);
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG1 |= (pc->Width);
	g_fh_reg_ptr->FH_CFG1 |= (pc->Height << 14);
	g_fh_reg_ptr->FH_CFG1 |= (pc->horiz_scale << 28);
	g_fh_reg_ptr->FH_CFG1 |= (pc->vert_scale << 30);
#endif
    if (pc->Width == 0 || pc->Height == 0)
    {
		pc->error_flag = 1;
        return MMDEC_STREAM_ERROR;
    }
	
#ifdef SIM_IN_WIN
	{
		int mb_rows = (pc->Height+15)/16;
		int mb_cols = (pc->Width + 15)/16;
		cmd = (JPEG_FW_YUV420 << 24)|(mb_rows << 12)|(mb_cols);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");
    }
#endif

    init_frame(pbi);

#ifdef SIM_IN_WIN
    if (vp8dx_start_decode_c(bc, /*IF_RTCD(&pbi->dboolhuff),*/
                           data, data_end - data))
	{
//        vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
//                           "Failed to allocate bool decoder 0");
	}
#else
	bc->range = 255;
    bc->count = 0;
	bc->value = (BitstreamReadBits(8) << 8);
#endif

    if (pc->frame_type == KEY_FRAME) {
        pc->clr_type    = (YUV_TYPE)vp8_read_bit(bc);
        pc->clamp_type  = (CLAMP_TYPE)vp8_read_bit(bc);
    }

#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG0 |= (pc->clr_type << 5);
	g_fh_reg_ptr->FH_CFG0 |= (pc->clamp_type << 6);
#endif

    // Is segmentation enabled
    xd->segmentation_enabled = (unsigned char)vp8_read_bit(bc);

#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG2 |= (xd->segmentation_enabled << 0);
#endif
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
                        xd->segment_feature_data[i][j] = (signed char)vp8_read_literal(bc, mb_feature_data_bits[i]);

                        if (vp8_read_bit(bc))
                            xd->segment_feature_data[i][j] = (signed char)-xd->segment_feature_data[i][j];
                    }
                    else
                        xd->segment_feature_data[i][j] = 0;
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
                    xd->mb_segment_tree_probs[i] = (vp8_prob)vp8_read_literal(bc, 8);
            }
        }
    }
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG2 |= (xd->update_mb_segmentation_map << 1);
	g_fh_reg_ptr->FH_CFG0 |= (xd->mb_segement_abs_delta << 7);
	g_fh_reg_ptr->FH_CFG2 |= ((xd->segment_feature_data[0][0]&0x7f) << 2);
	g_fh_reg_ptr->FH_CFG2 |= ((xd->segment_feature_data[0][1]&0x7f) << 9);
	g_fh_reg_ptr->FH_CFG2 |= ((xd->segment_feature_data[0][2]&0x7f) << 16);
	g_fh_reg_ptr->FH_CFG2 |= ((xd->segment_feature_data[0][3]&0x7f) << 23);
	g_fh_reg_ptr->FH_CFG3 |= ((xd->segment_feature_data[1][0]&0x7f) << 0);
	g_fh_reg_ptr->FH_CFG3 |= ((xd->segment_feature_data[1][1]&0x7f) << 6);
	g_fh_reg_ptr->FH_CFG3 |= ((xd->segment_feature_data[1][2]&0x7f) << 12);
	g_fh_reg_ptr->FH_CFG3 |= ((xd->segment_feature_data[1][3]&0x7f) << 18);
	g_fh_reg_ptr->FH_CFG4 |= (xd->mb_segment_tree_probs[0] << 0);
	g_fh_reg_ptr->FH_CFG4 |= (xd->mb_segment_tree_probs[1] << 8);
	g_fh_reg_ptr->FH_CFG4 |= (xd->mb_segment_tree_probs[2] << 16);
#endif
    // Read the loop filter level and type
    pc->filter_type = (LOOPFILTERTYPE) vp8_read_bit(bc);
    pc->filter_level = vp8_read_literal(bc, 6);
    pc->sharpness_level = vp8_read_literal(bc, 3);

    // Read in loop filter deltas applied at the MB level based on mode or ref frame.
    xd->mode_ref_lf_delta_update = 0;
    xd->mode_ref_lf_delta_enabled = (unsigned char)vp8_read_bit(bc);	// loop_filter_adj_enable

#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG0 |= (pc->filter_type << 8);
	g_fh_reg_ptr->FH_CFG0 |= (pc->filter_level << 9);
	g_fh_reg_ptr->FH_CFG0 |= (pc->sharpness_level << 15);
	g_fh_reg_ptr->FH_CFG0 |= (xd->mode_ref_lf_delta_enabled << 18);
#endif
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
                    xd->ref_lf_deltas[i] = (signed char)vp8_read_literal(bc, 6);

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
                    xd->mode_lf_deltas[i] = (signed char)vp8_read_literal(bc, 6);

                    if (vp8_read_bit(bc))        // Apply sign
                        xd->mode_lf_deltas[i] = (signed char)(xd->mode_lf_deltas[i] * -1);
                }
            }
        }
    }
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG5 |= ((xd->ref_lf_deltas[0]&0x3f) << 0);
	g_fh_reg_ptr->FH_CFG5 |= ((xd->ref_lf_deltas[1]&0x3f) << 6);
	g_fh_reg_ptr->FH_CFG5 |= ((xd->ref_lf_deltas[2]&0x3f) << 12);
	g_fh_reg_ptr->FH_CFG5 |= ((xd->ref_lf_deltas[3]&0x3f) << 18);
	g_fh_reg_ptr->FH_CFG6 |= ((xd->mode_lf_deltas[0]&0x3f) << 0);
	g_fh_reg_ptr->FH_CFG6 |= ((xd->mode_lf_deltas[1]&0x3f) << 6);
	g_fh_reg_ptr->FH_CFG6 |= ((xd->mode_lf_deltas[2]&0x3f) << 12);
	g_fh_reg_ptr->FH_CFG6 |= ((xd->mode_lf_deltas[3]&0x3f) << 18);
#endif
#ifdef SIM_IN_WIN
    setup_token_decoder(pbi, data + first_partition_length_in_bytes);	// data = pbi->Source + 3 or 10
	xd->current_bc = &pbi->bc2;
#else

	pc->multi_token_partition = (TOKEN_PARTITION)vp8_read_literal(bc, 2);	// log2_nbr_of_dct_partitions
	g_fh_reg_ptr->FH_CFG0 |= (pc->multi_token_partition << 19);
	//g_fh_reg_ptr->FH_CFG11 = 12 + ((data + first_partition_length_in_bytes + 3*((1<<pc->multi_token_partition)-1)) - pbi->Source);
	g_fh_reg_ptr->FH_CFG11 =  ((data + first_partition_length_in_bytes + 3*((1<<pc->multi_token_partition)-1)) - pbi->Source);
#endif

    // Read the default quantizers.
    {
        int Q, q_update;

        Q = vp8_read_literal(bc, 7);  // AC 1st order Q = default
        pc->base_qindex = Q;
        q_update = 0;
        pc->y1dc_delta_q = get_delta_q(bc, pc->y1dc_delta_q, &q_update);
        pc->y2dc_delta_q = get_delta_q(bc, pc->y2dc_delta_q, &q_update);
        pc->y2ac_delta_q = get_delta_q(bc, pc->y2ac_delta_q, &q_update);
        pc->uvdc_delta_q = get_delta_q(bc, pc->uvdc_delta_q, &q_update);
        pc->uvac_delta_q = get_delta_q(bc, pc->uvac_delta_q, &q_update);

        if (q_update)
            vp8cx_init_de_quantizer(pbi);

#ifdef SIM_IN_WIN
        // MB level dequantizer setup
        mb_init_dequantizer(pbi, &pbi->mb);
#endif
    }
#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG7 |= (pc->base_qindex << 0);
	g_fh_reg_ptr->FH_CFG7 |= ((pc->y1dc_delta_q&0xf) << 7);
	g_fh_reg_ptr->FH_CFG7 |= ((pc->y2dc_delta_q&0xf) << 11);
	g_fh_reg_ptr->FH_CFG7 |= ((pc->y2ac_delta_q&0xf) << 15);
	g_fh_reg_ptr->FH_CFG7 |= ((pc->uvdc_delta_q&0xf) << 19);
	g_fh_reg_ptr->FH_CFG7 |= ((pc->uvac_delta_q&0xf) << 23);
#endif

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
            pc->copy_buffer_to_gf = vp8_read_literal(bc, 2);

        pc->copy_buffer_to_arf = 0;

        if (!pc->refresh_alt_ref_frame)
            pc->copy_buffer_to_arf = vp8_read_literal(bc, 2);

        pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp8_read_bit(bc);
        pc->ref_frame_sign_bias[ALTREF_FRAME] = vp8_read_bit(bc);
    }

    pc->refresh_entropy_probs = vp8_read_bit(bc);
    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
    }

    pc->refresh_last_frame = pc->frame_type == KEY_FRAME  ||  vp8_read_bit(bc);



#ifdef TV_OUT
	g_fh_reg_ptr->FH_CFG0 |= (pc->refresh_entropy_probs << 21);
	g_fh_reg_ptr->FH_CFG0 |= (pc->refresh_golden_frame << 22);
	g_fh_reg_ptr->FH_CFG0 |= (pc->refresh_alt_ref_frame << 23);
	g_fh_reg_ptr->FH_CFG0 |= (pc->copy_buffer_to_gf << 24);
	g_fh_reg_ptr->FH_CFG0 |= (pc->copy_buffer_to_arf << 26);
	g_fh_reg_ptr->FH_CFG0 |= (pc->ref_frame_sign_bias[GOLDEN_FRAME] << 28);
	g_fh_reg_ptr->FH_CFG0 |= (pc->ref_frame_sign_bias[ALTREF_FRAME] << 29);
	g_fh_reg_ptr->FH_CFG0 |= (pc->refresh_last_frame << 30);
#endif
//    if (0)
//    {
//        FILE *z = fopen("decodestats.stt", "a");
//        fprintf(z, "%6d F:%d,G:%d,A:%d,L:%d,Q:%d\n",
//                pc->current_video_frame,
//                pc->frame_type,
//                pc->refresh_golden_frame,
//                pc->refresh_alt_ref_frame,
//                pc->refresh_last_frame,
//                pc->base_qindex);
//        fclose(z);
//    }

    //vp8dx_bool_decoder_fill_c(bc);
    {
        // read coef probability tree
        for (i = 0; i < BLOCK_TYPES; i++)
            for (j = 0; j < COEF_BANDS; j++)
                for (k = 0; k < PREV_COEF_CONTEXTS; k++)
                    for (l = 0; l < MAX_ENTROPY_TOKENS - 1; l++)
                    {
                        vp8_prob *const p = pc->fc.coef_probs [i][j][k] + l;

                        if (vp8_read(bc, vp8_coef_update_probs [i][j][k][l]))
                        {
                            *p = (vp8_prob)vp8_read_literal(bc, 8);

                        }
                    }
    }
#ifdef TV_OUT
	vpx_memcpy((void*)g_fh_reg_ptr->FH_COEFF_PROBS, pc->fc.coef_probs, sizeof(uint8)*BLOCK_TYPES*COEF_BANDS*PREV_COEF_CONTEXTS*(vp8_coef_tokens-1));
#endif
#ifdef SIM_IN_WIN
    vpx_memcpy(&xd->pre, &pc->last_frame, sizeof(YV12_BUFFER_CONFIG));
    vpx_memcpy(&xd->dst, &pc->new_frame, sizeof(YV12_BUFFER_CONFIG));
#endif


#ifdef SIM_IN_WIN
	// set up frame new frame for intra coded blocks
    vp8_setup_intra_recon(&pc->new_frame);

    vp8_setup_block_dptrs(xd);

//    vp8_build_block_doffsets(xd);

    // clear out the coeff buffer
    vpx_memset(xd->qcoeff, 0, sizeof(xd->qcoeff));
#endif

    // Read the mb_no_coeff_skip flag
    pc->mb_no_coeff_skip = (int)vp8_read_bit(bc);
	g_fh_reg_ptr->FH_CFG0 |= (pc->mb_no_coeff_skip << 31);


    if (pc->frame_type == KEY_FRAME)
        vp8_kfread_modes(pbi);
    else
        vp8_decode_mode_mvs(pbi);		//decode mv and modes

	// Error Concealment
	/*if( (uint32)(bc->read_ptr - bc->decode_buffer) != first_partition_length_in_bytes)
	{
		pc->error_flag = 1;
		return -1;
	}*/

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
		Print_Frame_Header_CFG();
		Print_PPA_CFG(xd, pc);
		Print_tbuf_Probs(pbi);
	}
#endif

#ifdef SIM_IN_WIN
    // reset since these guys are used as iterators
    vpx_memset(pc->above_context[Y1CONTEXT], 0, sizeof(ENTROPY_CONTEXT) * pc->mb_cols * 4);
    vpx_memset(pc->above_context[UCONTEXT ], 0, sizeof(ENTROPY_CONTEXT) * pc->mb_cols * 2);
    vpx_memset(pc->above_context[VCONTEXT ], 0, sizeof(ENTROPY_CONTEXT) * pc->mb_cols * 2);
    vpx_memset(pc->above_context[Y2CONTEXT], 0, sizeof(ENTROPY_CONTEXT) * pc->mb_cols);

    //xd->gf_active_ptr = (signed char *)pc->gf_active_flags;     // Point to base of GF active flags data structure

    vpx_memcpy(&xd->block[0].bmi, &xd->mode_info_context->bmi[0], sizeof(B_MODE_INFO));

//    if (pbi->b_multithreaded_lf && pbi->common.filter_level != 0)
//        vp8_start_lfthread(pbi);

//    if (pbi->b_multithreaded_rd && pbi->common.multi_token_partition != ONE_PARTITION)
//    {
//        vp8_mtdecode_mb_rows(pbi, xd);
//    }
//    else
    {
        int ibc = 0;
        int num_part = 1 << pbi->common.multi_token_partition;

        // Decode the individual macro block
        for (mb_row = 0; mb_row < pc->mb_rows; mb_row++)
        {
            if (num_part > 1)
            {
                xd->current_bc = & pbi->mbc[ibc];
                ibc++;

                if (ibc == num_part)
                    ibc = 0;
            }

            vp8_decode_mb_row(pbi, pc, mb_row, xd);
        }

        pbi->last_mb_row_decoded = mb_row;
    }


    stop_token_decoder(pbi);

    vp8dx_stop_decode_c(bc);

	OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, V_BIT_2, "ORSC: VSP_INT_CLR: clear MBW_FMR_DONE"); // clear HW INT
	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
#else
	{
		//中断唤醒
#if 1
		uint32 tmp = VSP_POLL_COMPLETE();
		 SCI_TRACE_LOW("%s, %d, tmp1: %0x", __FUNCTION__, __LINE__, tmp);
#else
		
		uint32 tmp = OR1200_READ_REG(ORSC_VSP_GLB_OFF+0x0C, "ORSC: Check VSP_INT_RAW");
		while ((tmp&0x35)==0)	// not (MBW_FMR_DONE|VLC_ERR|TIME_OUT)
			tmp = OR1200_READ_REG(ORSC_VSP_GLB_OFF+0x0C, "ORSC: Check VSP_INT_RAW");	
#endif
		if(tmp&0x31)	// (VLC_ERR|TIME_OUT)
		{
            pc->error_flag=1;
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, 0x2c,"ORSC: VSP_INT_CLR: clear BSM_frame error int"); // (MBW_FMR_DONE|PPA_FRM_DONE|TIME_OUT)


		}
		else if((tmp&0x00000004)==0x00000004)	// MBW_FMR_DONE
		{
			pc->error_flag=0;
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, V_BIT_2, "ORSC: VSP_INT_CLR: clear MBW_FMR_DONE");
		}
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
	}
#endif

    // vpx_log("Decoder: Frame Decoded, Size Roughly:%d bytes  \n",bc->pos+pbi->bc2.pos);

    // If this was a kf or Gf note the Q used
    //if ((pc->frame_type == KEY_FRAME) || (pc->refresh_golden_frame) || pbi->common.refresh_alt_ref_frame)
    //    pc->last_kf_gf_q = pc->base_qindex;

    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->fc, &pc->lfc, sizeof(pc->fc));
    }

#ifdef PACKET_TESTING
    {
        FILE *f = fopen("decompressor.VP8", "ab");
        unsigned int size = pbi->bc2.pos + pbi->bc.pos + 8;
        fwrite((void *) &size, 4, 1, f);
        fwrite((void *) pbi->Source, size, 1, f);
        fclose(f);
    }
#endif

    return MMDEC_OK;
}
