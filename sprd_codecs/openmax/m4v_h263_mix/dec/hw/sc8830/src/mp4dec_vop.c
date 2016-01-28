/******************************************************************************
 ** File Name:    mp4dec_vop.c                                             *
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
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC void Mp4Dec_VSP_Cfg(Mp4DecObject *vo)
{
    SLICEINFO *slice_info = &(vo->SliceInfo);
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    uint32 cmd;

    cmd = ((0x0)<<31);
    cmd |= ((slice_info->VopQuant&0x3f)<<25);
    cmd |= (((slice_info->SliceNum&0x1ff)<<16));
    cmd |= ((slice_info->Max_MBX*slice_info->Max_MBy)<<3);
    cmd |= (slice_info->VOPCodingType & 0x7);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG0_OFF, cmd,"VSP_CFG0");

    cmd = ((slice_info->NumMbsInGob & 0x1ff)<<20);
    cmd |= ((slice_info->NumMbLineInGob & 0x7)<<29);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG1_OFF, cmd, "VSP_CFG1");

    cmd = (((BVOP == vop_mode_ptr->VopPredType) ? 0 : 1)<<31);
    cmd |= ((1-slice_info->VopRoundingType) << 30);
    cmd |= (((slice_info->Max_MBX*slice_info->FirstMBy+slice_info->FirstMBx)&0x1fff) << 16);
    cmd |= ((slice_info->FirstMBy & 0x7f) << 8);
    cmd |= (slice_info->FirstMBx & 0x7f);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG2_OFF, cmd, "VSP_CFG2");

    cmd = slice_info->ShortHeader;
    cmd |= (slice_info->DataPartition << 1);
    cmd |= ((slice_info->IsRvlc && (slice_info->VOPCodingType !=BVOP)) << 2);
    cmd |= (slice_info->IntraDCThr << 3);
    cmd |= (slice_info->VOPFcodeFwd << 6);
    cmd |= (slice_info->VOPFcodeBck << 9);
    cmd |= (0<<12);
    cmd |= (slice_info->QuantType<<13);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG3_OFF, cmd, "VSP_CFG3");

    cmd = ((vop_mode_ptr->time_pp & 0xffff) << 16);
    cmd |= (vop_mode_ptr->time_bp & 0xffff);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG4_OFF, cmd, "VSP_CFG4");

    if(vop_mode_ptr->time_pp == 0)
    {
        VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG5_OFF,0,"VSP_CFG5");
    } else
    {
        VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG5_OFF,((1<<29)/vop_mode_ptr->time_pp),"VSP_CFG5");
    }
}

PUBLIC void Mp4Dec_exit_picture(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;

    /*reorder reference frame list*/
    if(vop_mode_ptr->VopPredType != BVOP)
    {
        Mp4DecStorablePic * pframetmp;

        vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;
        if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL))
        {
            (*(vo->mp4Handle->VSP_bindCb))(vo->mp4Handle->userdata, vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader, 0);
        }

        /*the buffer for forward reference frame will no longer be a reference frame*/
        if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
        {
            if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL))
            {
                (*(vo->mp4Handle->VSP_unbindCb))(vo->mp4Handle->userdata,vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader,0);
                vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader = NULL;
            }

            vop_mode_ptr->pFrdRefFrame->pDecFrame->bRef = FALSE;
            vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
        }

        //exchange buffer address. bck->frd, current->bck, frd->current
        pframetmp = vop_mode_ptr->pFrdRefFrame;
        vop_mode_ptr->pFrdRefFrame = vop_mode_ptr->pBckRefFrame;
        vop_mode_ptr->pBckRefFrame = vop_mode_ptr->pCurRecFrame;
        vop_mode_ptr->pCurRecFrame = pframetmp;
    }
}

PUBLIC MMDecRet Mp4Dec_InitVop(Mp4DecObject *vo, MMDecInput *dec_input_ptr)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    uint_32or64 vld_table_addr;
    MMDecRet ret = MMDEC_ERROR;
    SLICEINFO *slice_info = &(vo->SliceInfo);

    vop_mode_ptr->sliceNumber	= 0;
    vop_mode_ptr->stop_decoding = FALSE;
    vop_mode_ptr->mbnumDec		= 0;
    vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
    vop_mode_ptr->pCurRecFrame->nTimeStamp = dec_input_ptr->nTimeStamp;

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + IMG_SIZE_OFF, ((vop_mode_ptr->MBNumY&0xff)<<8) |((vop_mode_ptr->MBNumX&0xff)),"IMG_SIZE");
    if (vop_mode_ptr->bDataPartitioning)
    {
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x10,vop_mode_ptr->data_partition_buffer_Addr>>3,"data partition buffer." );
    }

    if(vop_mode_ptr->bReversibleVlc&&vop_mode_ptr->VopPredType!=BVOP)
    {
        memcpy(vo->g_rvlc_tbl_ptr, g_rvlc_huff_tab,(sizeof(uint32)*146));
        vld_table_addr= Mp4Dec_MemV2P(vo, (uint8 *)(vo->g_rvlc_tbl_ptr), HW_NO_CACHABLE);
    } else
    {
        memcpy(vo->g_huff_tbl_ptr, g_mp4_dec_huff_tbl,(sizeof(uint32)*152));
        vld_table_addr= Mp4Dec_MemV2P(vo, (uint8 *)(vo->g_huff_tbl_ptr), HW_NO_CACHABLE);
    }
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xc, (vld_table_addr)/8,"ddr vlc table start addr");//qiangshen@2013_01_11
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_SIZE_SET_OFF, 0x100,"ddr VLC table size");

    memset(slice_info,0,sizeof(SLICEINFO));
    slice_info->video_std=vop_mode_ptr->video_std;
    slice_info->DataPartition=vop_mode_ptr->bDataPartitioning;
    slice_info->VOPCodingType=vop_mode_ptr->VopPredType;
    slice_info->ShortHeader=(STREAM_ID_H263 == vop_mode_ptr->video_std) ? 1: 0;
    slice_info->Max_MBX=vop_mode_ptr->MBNumX;
    slice_info->Max_MBy=vop_mode_ptr->MBNumY;
    slice_info->IsRvlc=vop_mode_ptr->bReversibleVlc;;
    slice_info->QuantType=vop_mode_ptr->QuantizerType;
    slice_info->PicHeight=vop_mode_ptr->FrameHeight;
    slice_info->PicWidth=vop_mode_ptr->FrameWidth;
    slice_info->NumMbsInGob=vop_mode_ptr->NumMBInGob;
    slice_info->NumMbLineInGob=vop_mode_ptr->num_mbline_gob;
    slice_info->VopQuant=vop_mode_ptr->StepSize;
    slice_info->VOPFcodeFwd=vop_mode_ptr->mvInfoForward.FCode;
    slice_info->VOPFcodeBck=vop_mode_ptr->mvInfoBckward.FCode;
    slice_info->FirstMBx=0;
    slice_info->FirstMBy=0;
    slice_info->SliceNum=0;
    slice_info->VopRoundingType =(slice_info->VOPCodingType==PVOP) ? vop_mode_ptr->RoundingControl : 0;
    slice_info->IntraDCThr=vop_mode_ptr->IntraDcSwitchThr;

    Mp4Dec_VSP_Cfg(vo);

    if (vo->is_need_init_vsp_quant_tab)
    {
        int32 inter_quant=0;//(vop_mode_ptr->InterQuantizerMatrix);
        int32 intra_quant=0;//(vop_mode_ptr->IntraQuantizerMatrix);
        int i;
        volatile int8 tmp1,tmp2,tmp3,tmp4;

        for(i = 0; i < 96; i++)
        {
            if(i<16)
            {
                tmp1 = vop_mode_ptr->InterQuantizerMatrix[4*i];
                tmp2 = vop_mode_ptr->InterQuantizerMatrix[4*i+1];
                tmp3 = vop_mode_ptr->InterQuantizerMatrix[4*i+2];
                tmp4 = vop_mode_ptr->InterQuantizerMatrix[4*i+3];

                inter_quant =((tmp4&0xff)<<24)|((tmp3&0xff)<<16)|((tmp2&0xff)<<8)|(tmp1&0xff);
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+4*i,inter_quant,"weightscale inter8x8");
            } else if(i<32)
            {
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+i*4,0,"zero");

            } else if(i<48)
            {
                tmp1=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)];
                tmp2=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+1];
                tmp3=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+2];
                tmp4=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+3];

                intra_quant=((tmp4&0xff)<<24)|((tmp3&0xff)<<16)|((tmp2&0xff)<<8)|(tmp1&0xff);
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR + 4*i, intra_quant, "weightscale intra8x8");
            } else
            {
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR + i*4, 0, "zero");
            }
        }

        //disable this code for bug449345
        //vo->is_need_init_vsp_quant_tab = FALSE;
    }

    /*init  current frame, forward reference frame, backward reference frame*/
    // NOTE: MPEG4 decoder DO NOT support post_filter in Shark.
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0,  vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr >> 3,"current Y addr");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 4,  vop_mode_ptr->pCurRecFrame->pDecFrame->imgUAddr >>3,"current UV addr");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 8,  vop_mode_ptr->pCurRecFrame->pDecFrame->rec_infoAddr >>3,"current info addr");

    if(IVOP != vop_mode_ptr->VopPredType)
    {
        if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
        {
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80,vo->g_tmp_buf.imgYAddr >>3,"ref L0 Y addr");
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100,vo->g_tmp_buf.imgUAddr >>3,"ref L0 UV addr");
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x180,vo->g_tmp_buf.rec_infoAddr>>3,"ref L0 info addr");
        } else
        {
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80,vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr >>3,"ref L0 Y addr");
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR +  0x100,vop_mode_ptr->pBckRefFrame->pDecFrame->imgUAddr >>3,"ref L0 UV addr");
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR +0x180,vop_mode_ptr->pBckRefFrame->pDecFrame->rec_infoAddr>>3,"ref L0 info addr");
        }

        if(BVOP == vop_mode_ptr->VopPredType)
        {
            if (vop_mode_ptr->pFrdRefFrame->pDecFrame == NULL)
            {
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80,vo->g_tmp_buf.imgYAddr >>3,"ref L0 Y addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100,vo->g_tmp_buf.imgUAddr >>3,"ref L0 UV addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x180,vo->g_tmp_buf.rec_infoAddr>>3,"ref L0 info addr");

                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xc0,vo->g_tmp_buf.imgYAddr >>3,"ref L0 Y addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x140,vo->g_tmp_buf.imgUAddr >>3,"ref L0 UV addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x1c0,vo->g_tmp_buf.rec_infoAddr>>3,"ref L0 info addr");
            } else
            {
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80,vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYAddr >>3,"ref L0 Y addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100,vop_mode_ptr->pFrdRefFrame->pDecFrame->imgUAddr >>3,"ref L0 UV addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x180,vop_mode_ptr->pFrdRefFrame->pDecFrame->rec_infoAddr>>3,"ref L0 info addr");

                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xc0,vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr >>3,"ref L1 Y addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x140,vop_mode_ptr->pBckRefFrame->pDecFrame->imgUAddr >>3,"ref L1 UV addr");
                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x1c0,vop_mode_ptr->pBckRefFrame->pDecFrame->rec_infoAddr>>3,"ref L1 info addr");
            }

            if (vop_mode_ptr->pBckRefFrame->nTimeStamp < vop_mode_ptr->pCurRecFrame->nTimeStamp) {
                uint64 nTimeStamp;

                if (vo->trace_enabled) {
                    SPRD_CODEC_LOGD ("%s, [Bck nTimeStamp: %lld], [Cur nTimeStamp: %lld]", __FUNCTION__, vop_mode_ptr->pBckRefFrame->nTimeStamp, vop_mode_ptr->pCurRecFrame->nTimeStamp);
                }
                nTimeStamp = vop_mode_ptr->pCurRecFrame->nTimeStamp;
                vop_mode_ptr->pCurRecFrame->nTimeStamp = vop_mode_ptr->pBckRefFrame->nTimeStamp;
                vop_mode_ptr->pBckRefFrame->nTimeStamp = nTimeStamp;
            }
        }
    }

    return MMDEC_OK;
}

PUBLIC void Mp4Dec_output_one_frame (Mp4DecObject *vo, MMDecOutput *dec_output_ptr)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    Mp4DecStorablePic *pic = PNULL;
    VOP_PRED_TYPE_E VopPredType = vop_mode_ptr->VopPredType;//pre_vop_type;

    /*output frame for display*/
    if(VopPredType != BVOP)
    {
        if(vo->g_nFrame_dec > 0)
        {
            /*send backward reference (the lastest reference frame) frame's YUV to display*/
            pic = vop_mode_ptr->pBckRefFrame;
        }
    } else
    {
        /*send current B frame's YUV to display*/
        pic = vop_mode_ptr->pCurRecFrame;
    }

    if (vop_mode_ptr->post_filter_en)
    {
        DEC_FRM_BFR *display_frame = PNULL;

        /*get display frame from display queue*/
        display_frame = Mp4Dec_GetDispFrameBfr(vo, pic);
        if(PNULL != display_frame)
        {
            dec_output_ptr->pOutFrameY = display_frame->imgY;
            dec_output_ptr->pOutFrameU = display_frame->imgU;
            dec_output_ptr->pOutFrameV = display_frame->imgV;
            dec_output_ptr->is_transposed = 0;
            dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
            dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
            dec_output_ptr->frameEffective = 1;
            dec_output_ptr->err_MB_num = 0;
            dec_output_ptr->pBufferHeader = display_frame->pBufferHeader;
        } else
        {
            dec_output_ptr->frameEffective = 0;
        }
    } else
    {
        if(pic != PNULL && pic->pDecFrame != PNULL)
        {
            dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
            dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
            dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
            dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
            dec_output_ptr->is_transposed = 0;
            dec_output_ptr->pts = pic->nTimeStamp;

            dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
            dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
            dec_output_ptr->frameEffective = 1;
            dec_output_ptr->err_MB_num = 0;
        } else
        {
            dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
            dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
            dec_output_ptr->frameEffective = 0;
        }
    }
}

/*****************************************************************************/
//  Description:   firware Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
PUBLIC MMDecRet Mp4Dec_decode_vop(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    int32 cmd;
    int32 pic_end = 0;
    int32 VopPredType=vop_mode_ptr->VopPredType;
    int32 resyn_bits=VopPredType ? (vop_mode_ptr->mvInfoForward.FCode - 1) : 0;

    VSP_WRITE_REG(VSP_REG_BASE_ADDR + ARM_INT_MASK_OFF, V_BIT_2, "VSP_INT_MASK");//enable int //frame done/error/timeout

    while (!pic_end)
    {
        cmd = VSP_READ_REG(GLB_REG_BASE_ADDR + VSP_DBG_STS0_OFF, "read mbx mby");
        vop_mode_ptr->mb_y = (cmd & 0xff);
        vop_mode_ptr->mb_x = ((cmd >> 8) & 0xff);

        //SCI_TRACE_LOW("%s, %d, vop_mode_ptr->mb_y: %d, vop_mode_ptr->mb_x: %d", __FUNCTION__, __LINE__, vop_mode_ptr->mb_y, vop_mode_ptr->mb_x);

        if(vop_mode_ptr->mb_y == (vop_mode_ptr->MBNumY-1) && vop_mode_ptr->mb_x==(vop_mode_ptr->MBNumX-1))
        {
            if (vo->trace_enabled) {
                SPRD_CODEC_LOGD ("%s, %d, finished decoding one frame", __FUNCTION__, __LINE__);
            }
            pic_end = 1;
        } else
        {
            if(vop_mode_ptr->video_std != STREAM_ID_H263)
            {
                if(!vop_mode_ptr->bResyncMarkerDisable)
                {
                    if(Mp4Dec_CheckResyncMarker(vo, resyn_bits))
                    {
                        Mp4Dec_GetVideoPacketHeader(vo, resyn_bits);
                    }
                }
            } else if(VopPredType != BVOP)
            {
                Mp4Dec_DecGobHeader(vo);
                vo->SliceInfo.GobNum++;
            }

            VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, (V_BIT_2 | V_BIT_5), "VSP_INT_MASK, enable mbw_slice_done, time_out");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, V_BIT_0, "RAM_ACC_SEL");//change ram access to vsp hw
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_START_OFF, 0xa|1, "VSP_START");//start vsp   vld/vld_table//load_vld_table_en

            cmd = VSP_POLL_COMPLETE((VSPObject *)vo);
            if(cmd & V_BIT_2)
            {
                vo->error_flag = 0;
            } else if(cmd & (V_BIT_4 | V_BIT_5 | V_BIT_30 | V_BIT_31))
            {
                pic_end = 1;

                if (cmd & V_BIT_4)
                {
                    SPRD_CODEC_LOGE ("%s, %d, VLD_ERR", __FUNCTION__, __LINE__);
                } else if (cmd & (V_BIT_5  | V_BIT_31))
                {
                    SPRD_CODEC_LOGE ("%s, %d, TIME_OUT", __FUNCTION__, __LINE__);
                } else //if (cmd &  V_BIT_30)
                {
                    SPRD_CODEC_LOGE ("%s, %d, Broken by signal", __FUNCTION__, __LINE__);
                }
                vo->error_flag |= ER_HW_ID;
            } else
            {
                SPRD_CODEC_LOGE ("%s, %d, should not be here!", __FUNCTION__, __LINE__);
            }
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
        }
    }

    if(vo->error_flag)
    {
        return MMDEC_ERROR;
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

