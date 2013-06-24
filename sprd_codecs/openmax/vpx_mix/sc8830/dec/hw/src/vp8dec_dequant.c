
#include "video_common.h"
#include "vp8_quant_common.h"
#include "vp8_blockd.h"
#include "vp8_entropy.h"
#include "vp8dec_mode.h"
#include "vp8_idct.h" //weihu
//#include "vp8dec_dequant.h"//weihu
#include "sc8810_video_header.h"//weihu
#include "vp8dec_global.h"
#ifdef SIM_IN_WIN
	FILE* dct_in;
	FILE* dct_out;

void vp8_dequantize_b_c(BLOCKD *d)
{
    int i;
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;
    short *DQC = &d->dequant[0][0];

    for (i = 0; i < 16; i++)
    {
        DQ[i] = Q[i] * DQC[i];
    }
}

void vp8_dequant_idct_c(short *input, short *dq, short *output, int pitch)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        input[i] = dq[i] * input[i];
    }

    vp8_short_idct4x4llm_c(input, output, pitch);
    vpx_memset(input, 0, 32);
}

void vp8_dequant_dc_idct_c(short *input, short *dq, short *output, int pitch, int Dc)
{
    int i;

    input[0] = (short)Dc;

    for (i = 1; i < 16; i++)
    {
        input[i] = dq[i] * input[i];
    }

    vp8_short_idct4x4llm_c(input, output, pitch);
    vpx_memset(input, 0, 32);
}
#endif

void vp8cx_init_de_quantizer(VP8D_COMP *pbi)
{
    int r, c;
    int i;
    int Q;
    VP8_COMMON *const pc = & pbi->common;

    for (Q = 0; Q < QINDEX_RANGE; Q++)
    {
        pc->Y1dequant[Q][0][0] = (short)vp8_dc_quant(Q, pc->y1dc_delta_q);
        pc->Y2dequant[Q][0][0] = (short)vp8_dc2quant(Q, pc->y2dc_delta_q);
        pc->UVdequant[Q][0][0] = (short)vp8_dc_uv_quant(Q, pc->uvdc_delta_q);

#ifdef SIM_IN_WIN
        // all the ac values = ;
        for (i = 1; i < 16; i++)
        {
            int rc = vp8_default_zig_zag1d[i];
            r = (rc >> 2);
            c = (rc & 3);

            pc->Y1dequant[Q][r][c] = (short)vp8_ac_yquant(Q);
            pc->Y2dequant[Q][r][c] = (short)vp8_ac2quant(Q, pc->y2ac_delta_q);
            pc->UVdequant[Q][r][c] = (short)vp8_ac_uv_quant(Q, pc->uvac_delta_q);
        }
#else
		pc->Y1dequant[Q][0][1] = (short)vp8_ac_yquant(Q);
		pc->Y2dequant[Q][0][1] = (short)vp8_ac2quant(Q, pc->y2ac_delta_q);
		pc->UVdequant[Q][0][1] = (short)vp8_ac_uv_quant(Q, pc->uvac_delta_q);
#endif
    }
}


#ifdef SIM_IN_WIN
void de_quantand_idct(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int i,j;
    BLOCKD *b = &xd->block[24];
	char need_hadamard_trans;  

//	dct_in = fopen(".//dct_in.txt","ab+");
//	dct_out = fopen(".//dct_out.txt","ab+");
/*	for(i=0;i<25;i++)
	{
		for(j=0;j<16;j++)
		{
			xd->block[i].qcoeff[j];
//			fprintf(dct_in,"%d %x\n",j,(xd->block[i].qcoeff[j])&0xffff);
		}
	}*/

//    if(g_nFrame_dec==24)
//		fprintf(dct_in,"%d \n",g_nFrame_dec);
    need_hadamard_trans= (xd->mbmi.mode != B_PRED && xd->mbmi.mode != SPLITMV);

    if (need_hadamard_trans)
    {
        vp8_dequantize_b_c(b);

        // do 2nd order transform on the dc block
      //  if (b->eob > 1)//weihu
        {
            vp8_short_inv_walsh4x4_c(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
            ((int *)b->qcoeff)[1] = 0;
            ((int *)b->qcoeff)[2] = 0;
            ((int *)b->qcoeff)[3] = 0;
            ((int *)b->qcoeff)[4] = 0;
            ((int *)b->qcoeff)[5] = 0;
            ((int *)b->qcoeff)[6] = 0;
            ((int *)b->qcoeff)[7] = 0;
			
			for (i = 0; i < 16; i++)
			{
			    xd->block[i].qcoeff[0] = xd->block[24].diff[i];
#ifdef TV_OUT
				//g_mbc_reg_ptr->VP8_CFG1 |= ((xd->block[24].diff[i]!=0) << i);
				//g_isyn_buf_ptr->ISYN_CFG1 |= ((xd->block[24].diff[i]!=0) << i);
				//g_isyn_buf_ptr->ISYN_CFG1 |= ((xd->block[24].dqcoeff[i]!=0) << 24);
#endif
			}//weihu
        }

    /*    else
        {
            vp8_short_inv_walsh4x4_1_c(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
        }*///weihu


        /*for (i = 0; i < 16; i++)
        {

            b = &xd->block[i];

		    if (b->eob > 1)//weihu
            {
                 
				vp8_dequant_dc_idct_c(b->qcoeff, &b->dequant[0][0], b->diff, 32, xd->block[24].diff[i]);
            }
            else
            {
                vp8_dc_only_idct_c(xd->block[24].diff[i], b->diff, 32);
            }
        }

        for (i = 16; i < 24; i++)
        {
            b = &xd->block[i];

			

            if (b->eob > 1)//weihu
            {
                vp8_dequant_idct_c(b->qcoeff, &b->dequant[0][0], b->diff, 16);
            }
            else
            {
                vp8_dc_only_idct_c(b->qcoeff[0] * b->dequant[0][0], b->diff, 16);
                ((int *)b->qcoeff)[0] = 0;
            }
        }
    }
    else
    {
        for (i = 0; i < 24; i++)
        {

            b = &xd->block[i];

            //if (b->eob > 1)//weihu
            {
                vp8_dequant_idct_c(b->qcoeff, &b->dequant[0][0], b->diff, (32 - (i & 16)));
            }
            else
            {
                vp8_dc_only_idct_c(b->qcoeff[0] * b->dequant[0][0], b->diff, (32 - (i & 16)));
                ((int *)b->qcoeff)[0] = 0;
            }
        }*///weihu
    }

	for (i = 0; i < 24; i++)//for (i = 0; i < 16; i++)
	{
		
		b = &xd->block[i];
		
		//dequant
		{
			
			for (j = 0; j < 16; j++)
			{
			//	if((g_nFrame_dec==23) || (g_nFrame_dec==24))
			//		fprintf(dct_out,"%d %d %d %x\n",g_nFrame_dec,i,j,b->qcoeff[j]&0xffff);
				b->dqcoeff[j] = b->qcoeff[j] * ((i<16)&&(j==0) && need_hadamard_trans ? 1 : b->dequant[j/4][j%4] ) ;
				//if(g_nFrame_dec==24)
				//fprintf(dct_out,"%d %d %x\n",i,j,b->dqcoeff[j]&0xffff);
			}
			vpx_memset(b->qcoeff, 0, 32);
		}
		
		vp8_short_idct4x4llm_c(b->dqcoeff, b->diff, (32 - (i & 16)));

	}

	for (i = 0; i < 24; i++)//for (i = 0; i < 16; i++)
	{
    	for(j=0;j<16;j++)
		{
    //	 fprintf(dct_out,"%d %d %x\n",i,j,(xd->diff[i*16+j])&0xffff);
		}
	}
//	fclose(dct_in);
//	fclose(dct_out);
}

void mb_init_dequantizer(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int i;
    int QIndex;
    MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
    VP8_COMMON *const pc = & pbi->common;

    // Decide whether to use the default or alternate baseline Q value.
    if (xd->segmentation_enabled)
    {
        // Abs Value
        if (xd->mb_segement_abs_delta == SEGMENT_ABSDATA)
            QIndex = xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];

        // Delta Value
        else
        {
            QIndex = pc->base_qindex + xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];
            QIndex = (QIndex >= 0) ? ((QIndex <= MAXQ) ? QIndex : MAXQ) : 0;    // Clamp to valid range
        }
    }
    else
        QIndex = pc->base_qindex;

    // Set up the block level dequant pointers
    for (i = 0; i < 16; i++)
    {
        xd->block[i].dequant = pc->Y1dequant[QIndex];
    }

    for (i = 16; i < 24; i++)
    {
        xd->block[i].dequant = pc->UVdequant[QIndex];
    }

    xd->block[24].dequant = pc->Y2dequant[QIndex];

    //VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CFG1_OFF, pc->Y1dequant[QIndex][0][1]|(pc->Y1dequant[QIndex][0][0]<<8)|(pc->Y1dequant[QIndex][0][1]<<16)|(pc->Y1dequant[QIndex][0][0]<<24), "configure qp of y1 and UV");//weihu
//	VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CFG1_OFF, pc->Y1dequant[QIndex][0][1]|(pc->Y1dequant[QIndex][0][0]<<8)|(pc->UVdequant[QIndex][0][1]<<16)|(pc->UVdequant[QIndex][0][0]<<24), "configure qp of y1 and UV");//derek
//	VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CFG2_OFF, pc->Y2dequant[QIndex][0][1]|(pc->Y2dequant[QIndex][0][0]<<8), "configure qp of y2");//weihu
}
#endif	// SIM_IN_WIN