/******************************************************************************
 ** File Name:    mp4enc_vop.c		                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
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

    for (i = 0; i < img_size; i++)
    {
        if (*p_src_y == 0)
        {
            *p_src_y ++ = 1;
        }
    }
}

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

PUBLIC int32 Mp4Enc_EncIVOP(Mp4EncObject *vo, int32 time_stamp)
{
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    int32 Qp;
    uint32 mb_pos_x;
    uint32 mb_pos_y;
    uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
    uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
    BOOLEAN is_short_header = vop_mode_ptr->short_video_header;
    int	 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
    int mea_start = 0;

    vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepI;
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
    VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0xd, "ORSC: VSP_START: ENCODE_START=1");
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

                /*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncGOBHeader(vo, GQUANT);
            } else
            {
                /*g_rc_par.nbits_hdr_mv += */Mp4Enc_ReSyncHeader(vo, Qp, time_stamp);
            }
            left_mbline_slice = vop_mode_ptr->mbline_num_slice;
            vop_mode_ptr->sliceNumber++;
            VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
            VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0x5, "ORSC: VSP_START: ENCODE_START=1");
            mea_start = 1;
        }

        left_mbline_slice--;

        if(mea_start)
        {
            uint32 tmp = VSP_POLL_COMPLETE((VSPObject *)vo);
            if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
            {
                vop_mode_ptr->error_flag=1;
            } else if((tmp&V_BIT_1)==V_BIT_1)	// VLC_FRM_DONE
            {
                vop_mode_ptr->error_flag=0;
            }
            mea_start = 0;
        }
    }

#if 0   //VSP has made byte align
    Mp4Enc_ByteAlign(is_short_header);
#endif

    return 1;
}

PUBLIC void Mp4Enc_UpdateRefFrame(ENC_VOP_MODE_T *vop_mode_ptr)
{
    Mp4EncStorablePic *pTmp = PNULL;

    pTmp = vop_mode_ptr->pYUVRefFrame;
    vop_mode_ptr->pYUVRefFrame = vop_mode_ptr->pYUVRecFrame;
    vop_mode_ptr->pYUVRecFrame = pTmp;
}

PUBLIC int32 Mp4Enc_EncPVOP(Mp4EncObject *vo, int32 time_stamp)
{
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    int32  Qp;
    uint32 mb_pos_x;
    uint32 mb_pos_y;
    uint32 mb_pos;
    uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
    uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
    BOOLEAN  is_short_header = vop_mode_ptr->short_video_header;
    int32 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
    int32 left_intramb_dis = vop_mode_ptr->intra_mb_dis;
    int mea_start = 0;

    vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepP;
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

#if 0
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0xd, "ORSC: VSP_START: ENCODE_START=1");
#else
    VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0xd, "ORSC: VSP_START: ENCODE_START=1");
#endif
    mea_start = 1;

    for(mb_pos = 0, mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
    {
        vop_mode_ptr->mb_y = mb_pos_y;
        vop_mode_ptr->mb_x = 0;
        /*code gob header if need*/
        if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!vol_mode_ptr->bResyncMarkerDisable))
        {
            Mp4Enc_InitVSP(vo);
            if(is_short_header)
            {
                int32 GQUANT = vop_mode_ptr->StepSize;

                /*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncGOBHeader(vo, GQUANT);
            } else
            {
                /*g_rc_par.nbits_hdr_mv += */Mp4Enc_ReSyncHeader(vo, Qp, time_stamp);
            }

            left_mbline_slice = vop_mode_ptr->mbline_num_slice;
            vop_mode_ptr->sliceNumber++;
#if 0
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0x5, "ORSC: VSP_START: ENCODE_START=1");
#else
            VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
            VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0x5, "ORSC: VSP_START: ENCODE_START=1");
#endif
            mea_start = 1;

        }

        left_mbline_slice--;

        if(mea_start)
        {
#if 0
            uint32 tmp = VSP_READ_REG(GLB_REG_BASE_ADDR+0x0C, "ORSC: Check VSP_INT_RAW");
            while ((tmp&0x34)==0)	// not (MBW_FMR_DONE|VLC_ERR|TIME_OUT)
                tmp = VSP_READ_REG(GLB_REG_BASE_ADDR+0x0C, "ORSC: Check VSP_INT_RAW");
            VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_1, V_BIT_1, "ORSC: Polling VSP_INT_RAW: VLC_FRM_DONE "); //check vlc frame done
            if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
            {
                pVop_mode->error_flag=TRUE;
                VSP_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x2c,"ORSC: VSP_INT_CLR: clear BSM_frame error int"); // (MBW_FMR_DONE|PPA_FRM_DONE|TIME_OUT)
            }
            else if((tmp&0x00000004)==0x00000004)	// MBW_FMR_DONE
            {
                pVop_mode->error_flag=FALSE;
                VSP_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x6,"ORSC: VSP_INT_CLR: clear MBW_FMR_DONE");
            }
#else
            uint32 tmp = VSP_POLL_COMPLETE((VSPObject *)vo);
            if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
            {
                vop_mode_ptr->error_flag=1;
            } else if((tmp&V_BIT_1)==V_BIT_1)	// VLC_FRM_DONE
            {
                vop_mode_ptr->error_flag=0;
            }
#endif
            mea_start = 0;

        }
    }
    /*stuffing to byte align*/
#if 0   //VSP has made byte align
    Mp4Enc_ByteAlign(is_short_header);
#endif

    return 1;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
