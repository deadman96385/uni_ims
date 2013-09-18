/******************************************************************************
 ** File Name:    h264enc_slice.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/18/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
//#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif


#if 0
PUBLIC int32 h264enc_slicetype_decide(ENC_IMAGE_PARAMS_T *img_ptr)
{
//	return SLICE_TYPE_I;

    if((vd->g_nFrame_enc % INTRA_PERIOD) == 0x0)
    {
        return SLICE_TYPE_I;
    } else
    {
        return SLICE_TYPE_P;
    }
}
#endif

//fill "default" values
LOCAL void h264enc_slice_header_init (ENC_IMAGE_PARAMS_T *img_ptr, ENC_SLICE_HEADER_T *sh, ENC_SPS_T *sps,
                                      ENC_PPS_T *pps, int32 i_type, int32 i_idr_pic_id, int32 i_frame, int32 i_qp)
{
    //first we fill all field
    sh->sps = sps;
    sh->pps = pps;

    sh->i_type = i_type;
    //sh->i_first_mb = 0;
    sh->i_last_mb = IClip(0, img_ptr->frame_size_in_mbs-1, (int32)(sh->i_first_mb+img_ptr->slice_mb-1));//img_ptr->sps->i_mb_width*img_ptr->sps->i_mb_height;
    sh->i_pps_id = pps->i_id;

    sh->i_frame_num = i_frame;

    sh->i_idr_pic_id = i_idr_pic_id;

    //poc stuff, fixed later
    sh->i_poc_lsb = 0;
    sh->i_delta_poc[0] = 0;
    sh->i_delta_poc[1] = 0;

    sh->i_redundant_pic_cnt = 0;

    sh->b_num_ref_idx_override = 0;
    sh->i_num_ref_idx_l0_active = 1;
    sh->i_num_ref_idx_l1_active = 1;

    sh->b_ref_pic_list_reordering_l0 = 0;

    sh->i_qp = i_qp;
    sh->i_qp_delta = i_qp - pps->i_pic_init_qp;

    sh->i_alpha_c0_offset	= 0;//3*2;	// -6 ~ 6, div2
    sh->i_beta_offset		= 0;//-3*2;	// -6 ~ 6, div2

    sh->i_disable_deblocking_filter_idc = 0; //tmp disable dbk
}

LOCAL void h264enc_slice_header_write (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr)
{
    int32 i;
    uint32 nal_header;
    ENC_SLICE_HEADER_T *sh = &(img_ptr->sh);

    //slice
    H264Enc_write_nal_start_code(vo);

    /* nal header, ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type; */
    nal_header = ( 0x00 << 7 ) | ( img_ptr->i_nal_ref_idc << 5 ) | img_ptr->i_nal_type;
    H264Enc_OutputBits (vo, nal_header, 8);

    WRITE_UE_V (sh->i_first_mb);
    WRITE_UE_V (sh->i_type+5); //same type things
    WRITE_UE_V (sh->i_pps_id);
    H264Enc_OutputBits (vo, sh->i_frame_num, sh->sps->i_log2_max_frame_num);

    if (sh->i_idr_pic_id >= 0) //NAL IDR
    {
        WRITE_UE_V (sh->i_idr_pic_id);
    }

    if (sh->sps->i_poc_type == 0)
    {
        H264Enc_OutputBits (vo, sh->i_poc_lsb, sh->sps->i_log2_max_poc_lsb);
    }

    if (sh->pps->b_redundant_pic_cnt)
    {
        WRITE_UE_V (sh->i_redundant_pic_cnt);
    }

    if (sh->i_type == SLICE_TYPE_P)
    {
        H264Enc_OutputBits (vo, sh->b_num_ref_idx_override, 1);

        if (sh->b_num_ref_idx_override)
        {
            WRITE_UE_V (sh->i_num_ref_idx_l0_active -1);
        }
    }

    //ref pic list reordering
    if (sh->i_type != SLICE_TYPE_I)
    {
        H264Enc_OutputBits (vo, sh->b_ref_pic_list_reordering_l0, 1);

        if (sh->b_ref_pic_list_reordering_l0)
        {
            for (i = 0; i < sh->i_num_ref_idx_l0_active; i++)
            {
                //WRITE_UE_V (sh->ref_pic_list_order[0][i].idc);
                //WRITE_UE_V (sh->ref_pic_list_order[0][i].arg);
            }
            WRITE_UE_V (3);
        }
    }

    if (img_ptr->i_nal_ref_idc != 0)
    {
        if(sh->i_idr_pic_id >= 0)
        {
            H264Enc_OutputBits (vo, 0, 1); //no output of prior pics flag
            H264Enc_OutputBits (vo, 0, 1); //long term reference flag
        } else
        {
            H264Enc_OutputBits (vo, 0, 1); //adaptive_ref_pic_marking_mode_flag
        }
    }

    WRITE_SE_V (sh->i_qp_delta); //slice qp delta

    if (sh->pps->b_deblocking_filter_control)
    {
        WRITE_UE_V(sh->i_disable_deblocking_filter_idc);

        if (sh->i_disable_deblocking_filter_idc != 1)
        {
            WRITE_SE_V (sh->i_alpha_c0_offset>>1);
            WRITE_SE_V (sh->i_beta_offset>>1);
        }
    }
}

PUBLIC void h264enc_slice_init (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr, int32 nal_type, int32 slice_type, int32 global_qp)
{
    //create slice header
    if (nal_type == NAL_SLICE_IDR)
    {
        h264enc_slice_header_init (img_ptr, &img_ptr->sh, img_ptr->sps, img_ptr->pps, slice_type,
                                   img_ptr->i_idr_pic_id, img_ptr->frame_num, global_qp);

        //increment id
        //img_ptr->i_idr_pic_id = (img_ptr->i_idr_pic_id + 1) % 65536;
    } else
    {
        h264enc_slice_header_init (img_ptr, &img_ptr->sh, img_ptr->sps, img_ptr->pps, slice_type, -1, img_ptr->frame_num, global_qp);

        //always set the real higher num of ref frame used
        img_ptr->sh.b_num_ref_idx_override = 1;
        img_ptr->sh.i_num_ref_idx_l0_active = 1/*(h->i_ref0 <= 0) ? 1 : h->i_ref0*/;
        //	g_slice_header_ptr->i_num_ref_idx_l1_active = (h->i_ref1 <= 0) ? 1 : img_ptr->i_ref1;
    }

    if (img_ptr->sps->i_poc_type == 0)
    {
        img_ptr->sh.i_poc_lsb = img_ptr->pYUVRecFrame->i_poc & ( ( 1<< img_ptr->sps->i_log2_max_poc_lsb) - 1);
        img_ptr->sh.i_delta_poc_bottom = 0; //won't work for field
    } else if (img_ptr->sps->i_poc_type == 1)
    {
        //nothing to do
    } else
    {
        //nothing to do
    }
}

PUBLIC int32 h264enc_slice_write (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr)
{
    int32 i_frame_size;
    uint32 slice_bits;
    uint32 tmp;

    img_ptr->slice_end = 0;
    slice_bits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "TOTAL_BITS");

    //slice header
    h264enc_slice_header_write(vo, img_ptr);

    img_ptr->qp = img_ptr->sh.i_qp;

    VSP_WRITE_REG(VSP_REG_BASE_ADDR + ARM_INT_MASK_OFF, V_BIT_2, "ARM_INT_MASK, only enable VSP ACC init");//enable int //
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, (V_BIT_1 | V_BIT_5), "VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, V_BIT_0, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_START_OFF, 0x5|((img_ptr->sh.i_first_mb==0)<<3), "VSP_START: ENCODE_START=1");

    tmp = VSP_POLL_COMPLETE((VSPObject *)vo);
    if(tmp & (V_BIT_4 | V_BIT_5))	// (VLC_ERR|TIME_OUT)
    {
        img_ptr->error_flag=1;

        if (tmp & V_BIT_4)
        {
            SCI_TRACE_LOW("%s, %d, VLC_ERR", __FUNCTION__, __LINE__);
        } else if (tmp & V_BIT_5)
        {
            SCI_TRACE_LOW("%s, %d, TIME_OUT", __FUNCTION__, __LINE__);
        }
    } else if(tmp & V_BIT_1)	// VLC_FRM_DONE
    {
        img_ptr->error_flag=0;
    }

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_27, 0x00000000, TIME_OUT_CLK, "Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, V_BIT_1, "BSM_OPERATE: BSM_CLR");
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "Polling BSM_DBG0: BSM inactive"); //check bsm is idle
    VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + BSM_DBG1_OFF, V_BIT_1, 0x0, TIME_OUT_CLK, "Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR
    VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + VSP_INT_RAW_OFF, V_BIT_2, V_BIT_2, TIME_OUT_CLK,  "Polling MBW_FMR_DONE"); //check MBW is done
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_CLR_OFF, V_BIT_2,"VSP_INT_CLR: clear MBW_FMR_DONE");

    i_frame_size = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF,"TOTAL_BITS");

    if( (img_ptr->sh.i_last_mb + 1) < img_ptr->frame_size_in_mbs)
    {
        img_ptr->sh.i_first_mb = img_ptr->sh.i_last_mb + 1;
    } else
    {
        img_ptr->sh.i_first_mb = 0;
    }

#ifdef RC_BU
    if(vo->g_h264_enc_config->RateCtrlEnable)
        vo->BU_bit_stat = (i_frame_size - slice_bits);
#endif

    return (i_frame_size-slice_bits);
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

