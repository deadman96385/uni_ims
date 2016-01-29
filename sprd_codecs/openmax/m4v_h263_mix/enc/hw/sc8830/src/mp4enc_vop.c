/******************************************************************************
 ** File Name:    mp4enc_vop.c		                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC void Mp4Enc_ReviseLumaData(uint8 *p_src_y, int32 img_width, int32 img_height)
{
    int32 i;
    int32 img_size = img_width  * img_height;

    for (i = 0; i < img_size; i++, p_src_y++)
    {
        if (*p_src_y == 0)
        {
            *p_src_y = 1;
        }
    }
}

#if 1   //not verified, removed by xiaowei@2013.09.16
PUBLIC int32 Mp4Enc_EncNVOP(Mp4EncObject *vo, int32 time_stamp)
{
    int32 Qp;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    BOOLEAN is_short_header = vop_mode_ptr->short_video_header;

    vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepI;
    vop_mode_ptr->MBNumOneVP = 0;
    vop_mode_ptr->bCoded = 0;

//	Mp4Enc_VspFrameInit(vop_mode_ptr);
#if 0
    if(is_short_header)
    {
        g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
    } else
#endif
    {
        /*g_rc_par.nbits_hdr_mv +=*/ (int)Mp4Enc_EncVOPHeader(vo, time_stamp);
    }

#if 0   //VSP has made byte align
    Mp4Enc_ByteAlign(vo,is_short_header);
#endif

    return 1;
}
#endif

PUBLIC int32 Mp4Enc_EncVOP(Mp4EncObject *vo, int32 time_stamp)
{
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    uint32 mb_pos_x, mb_pos_y;
    uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
    uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
    BOOLEAN is_short_header = vop_mode_ptr->short_video_header;
    int32 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
    int32 mea_start = 0;

    vop_mode_ptr->MBNumOneVP = 0;
    vop_mode_ptr->mb_x = 0;
    vop_mode_ptr->mb_y = 0;

    Mp4Enc_InitVSP(vo);
    Mp4Enc_InitBSM(vo);

    if(is_short_header)
    {
        Mp4Enc_EncH263PicHeader(vo);
    } else
    {
        Mp4Enc_EncVOPHeader(vo, time_stamp);
    }

    VSP_WRITE_REG(VSP_REG_BASE_ADDR + ARM_INT_MASK_OFF, V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, (V_BIT_1 | V_BIT_5),"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0x1, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_START_OFF, 0xd, "VSP_START: ENCODE_START=1");
    mea_start = 1;

    for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
    {
        vop_mode_ptr->mb_y = mb_pos_y;
        vop_mode_ptr->mb_x = 0;

        if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!vol_mode_ptr->bResyncMarkerDisable))
        {
            Mp4Enc_InitVSP(vo);

            if(is_short_header)
            {
                int32 GQUANT = vop_mode_ptr->StepSize;
                Mp4Enc_EncGOBHeader(vo, GQUANT);
            } else
            {
                Mp4Enc_ReSyncHeader(vo, time_stamp);
            }

            left_mbline_slice = vop_mode_ptr->mbline_num_slice;
            vop_mode_ptr->sliceNumber++;
            VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
            VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

            VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, V_BIT_0, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_START_OFF, (V_BIT_2 | V_BIT_0), "VSP_START: ENCODE_START=1");
            mea_start = 1;
        }

        left_mbline_slice--;

        if(mea_start)
        {
            int32 int_ret = VSP_POLL_COMPLETE((VSPObject *)vo);

            if(int_ret & V_BIT_1)	// VLC_FRM_DONE
            {
                vo->error_flag = 0;
            } else if(int_ret & (V_BIT_4 | V_BIT_5 | V_BIT_30 | V_BIT_31))	// (VLC_ERR|TIME_OUT)
            {
                vo->error_flag |= ER_HW_ID;
                if (int_ret & V_BIT_4)
                {
                    SPRD_CODEC_LOGE ("%s, %d, VLD_ERR\n", __FUNCTION__, __LINE__);
                } else if (int_ret & (V_BIT_5  | V_BIT_31))
                {
                    SPRD_CODEC_LOGE ("%s, %d, TIME_OUT\n", __FUNCTION__, __LINE__);
                } else //if (int_ret &  V_BIT_30)
                {
                    SPRD_CODEC_LOGE ("%s, %d, Broken by signal\n", __FUNCTION__, __LINE__);
                }
                goto VOP_EXIT;
            } else
            {
                SPRD_CODEC_LOGE ("%s, %d, should not be here!\n", __FUNCTION__, __LINE__);
            }
            mea_start = 0;
        }
    }

VOP_EXIT:

#if 0   //VSP has made byte align
    Mp4Enc_ByteAlign(is_short_header);
#endif

    if(vo->error_flag)
    {
        return 0;
    }

    return 1;
}

PUBLIC void Mp4Enc_UpdateRefFrame(ENC_VOP_MODE_T *vop_mode_ptr)
{
    Mp4EncStorablePic *pTmp = PNULL;

    pTmp = vop_mode_ptr->pYUVRefFrame;
    vop_mode_ptr->pYUVRefFrame = vop_mode_ptr->pYUVRecFrame;
    vop_mode_ptr->pYUVRecFrame = pTmp;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
