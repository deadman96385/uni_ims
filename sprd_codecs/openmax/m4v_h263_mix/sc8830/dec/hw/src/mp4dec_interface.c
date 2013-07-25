/******************************************************************************
 ** File Name:    mp4dec_interface.c										  *
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

PUBLIC void MP4DecSetPostFilter(MP4Handle *mp4Handle, int en)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;

    // Shark VSP DO NOT support post filter for MPEG4 decoder.
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    vop_mode_ptr->post_filter_en = 0;
}

PUBLIC void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_FRM_BFR *rec_buf_ptr = &(vo->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint32)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;
}

void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;

    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    DEC_FRM_BFR *FrmYUVBfr = &(vo->g_FrmYUVBfr[0]);

    int i;

    if(!vop_mode_ptr)
        return;

    for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
    {
        if(FrmYUVBfr[i].bRef)
        {
            FrmYUVBfr[i].bRef = FALSE;
            if((*mp4Handle->VSP_unbindCb) && FrmYUVBfr[i].pBufferHeader!=NULL) {
                (*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,FrmYUVBfr[i].pBufferHeader,0);
                //g_FrmYUVBfr[i].pBufferHeader = NULL;
            }
        }
    }

    if (vop_mode_ptr->post_filter_en)
    {
        DEC_FRM_BFR *DispFrmYUVBfr = &(vo->g_DispFrmYUVBfr[0]);

        for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
        {
            if(DispFrmYUVBfr[i].bRef)
            {
                DispFrmYUVBfr[i].bRef = FALSE;
                DispFrmYUVBfr[i].bDisp = FALSE;
                if((*mp4Handle->VSP_unbindCb) && DispFrmYUVBfr[i].pBufferHeader!=NULL) {
                    (*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,DispFrmYUVBfr[i].pBufferHeader,0);
                    //g_DispFrmYUVBfr[i].pBufferHeader = NULL;
                }
            }
        }
    }
#if 0 //bug127759    
    if (!vop_mode_ptr->post_filter_en)
#endif
    {
        vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
        vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
    }
    vo->g_nFrame_dec = 0;
}

int MP4DecGetLastDspFrm(MP4Handle *mp4Handle,void **pOutput)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    DEC_FRM_BFR *frm_bfr;
    DEC_FRM_BFR *disp_frm = PNULL;
    uint8 *rec_imgY = PNULL;
    int i;

    *pOutput = NULL;
    if(!vop_mode_ptr)
        return FALSE;
    
    if(!vop_mode_ptr->pBckRefFrame)
        return FALSE;
    
    frm_bfr = vop_mode_ptr->pBckRefFrame->pDecFrame;
    vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
    if(NULL != frm_bfr)
    {
        if (!vop_mode_ptr->post_filter_en)
        {
            *pOutput = frm_bfr->pBufferHeader;

        } else
        {
            rec_imgY = frm_bfr->imgY;
            DEC_FRM_BFR *DispFrmYUVBfr = &(vo->g_DispFrmYUVBfr[0]);

            for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
            {
                if(DispFrmYUVBfr[i].bDisp && (rec_imgY == DispFrmYUVBfr[i].rec_imgY))
                {
                    disp_frm = &DispFrmYUVBfr[i];
                    break;
                }
            }
            if(disp_frm)
            {
                *pOutput = disp_frm->pBufferHeader;
                disp_frm->bDisp = FALSE;
            } else
            {
                return FALSE;
            }
        }
        return TRUE;
    } else
    {
        return FALSE;
    }
}

PUBLIC void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;

    *display_width = vop_mode_ptr->OrgFrameWidth;
    *display_height =  vop_mode_ptr->OrgFrameHeight;

    SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;

    *width =( (vop_mode_ptr->OrgFrameWidth + 15) >>4) <<4;
    *height = ( (vop_mode_ptr->OrgFrameHeight + 15) >>4) <<4;

    SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}

MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * buffer_ptr)
{
    Mp4DecObject*vo;
    MMDecRet is_init_success = MMDEC_OK;
    DEC_VOP_MODE_T *vop_mode_ptr = NULL;
    H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr = NULL;

    SCI_TRACE_LOW("libomx_m4vh263dec_hw_sprd.so is built on %s %s, Copyright (C) Spreatrum, Inc.", __DATE__, __TIME__);

    vo = (Mp4DecObject *) (buffer_ptr->common_buffer_ptr);
    memset(vo, 0, sizeof(Mp4DecObject));
    mp4Handle->videoDecoderData = (void *) vo;
    vo->mp4Handle = mp4Handle;

    buffer_ptr->common_buffer_ptr += sizeof(Mp4DecObject);
    buffer_ptr->common_buffer_ptr_phy = (void *)((uint32)(buffer_ptr->common_buffer_ptr_phy) + sizeof(Mp4DecObject));
    buffer_ptr->size -= sizeof(Mp4DecObject);

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    if(VSP_OPEN_Dev((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    Mp4Dec_InitInterMem(vo, buffer_ptr);

    vo->g_dec_vop_mode_ptr = vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_VOP_MODE_T), 8, INTER_MEM);
    SCI_ASSERT(NULL != vop_mode_ptr);

    vo->g_nFrame_dec = 0;
    vo->g_dec_is_first_frame = TRUE;
    vo->g_dec_is_stop_decode_vol = FALSE;
    vo->g_dec_is_changed_format = FALSE;
    vo->g_dec_pre_vop_format = IVOP;
    vo->is_need_init_vsp_quant_tab= FALSE;
    vop_mode_ptr->error_flag = FALSE;

    //for H263 plus header
    vo->g_h263_plus_head_info_ptr = h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_MemAlloc(vo, sizeof(H263_PLUS_HEAD_INFO_T), 8, INTER_MEM);
    Mp4Dec_InitDecoderPara(vo);

    vo->g_rvlc_tbl_ptr = (uint32*)Mp4Dec_MemAlloc(vo, sizeof(uint32)*146, 8, INTER_MEM);
    vo->g_huff_tbl_ptr= (uint32*)Mp4Dec_MemAlloc(vo, sizeof(uint32)*152, 8, INTER_MEM);

    memcpy(vo->g_rvlc_tbl_ptr, g_rvlc_huff_tab,(sizeof(uint32)*146));
    memcpy(vo->g_huff_tbl_ptr, g_mp4_dec_huff_tbl,(sizeof(uint32)*152));


    return is_init_success;
}

PUBLIC MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    MMDecRet ret = MMDEC_OK;

    vop_mode_ptr->video_std = video_format_ptr->video_std;

    /*judge h.263 or mpeg4*/
    if(video_format_ptr->video_std != STREAM_ID_MPEG4)
    {
        ALOGE ("\nH263(ITU or Sorenson format) is detected!\n");
    } else
    {
        if(video_format_ptr->i_extra > 0)
        {
            MMDecRet is_init_success;
            uint32 bs_buffer_length, bs_start_addr;

            // Bitstream.
            bs_start_addr=((uint32)video_format_ptr->p_extra_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
            bs_buffer_length= video_format_ptr->i_extra;

            if(ARM_VSP_RST(vo)<0)
            {
                return MMDEC_HW_ERROR;
            }
            SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);

            VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
            VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_MPEG4,"VSP_MODE");

            VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000, TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
            VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
            VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

            VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (0),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
            VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

            is_init_success = Mp4Dec_DecMp4Header(vo, video_format_ptr->i_extra);
            if(MMDEC_OK == is_init_success)
            {
                video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
                video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
            }

            VSP_RELEASE_Dev((VSPObject *)vo);


//          		 mp4Handle->VSP_extMemCb(mp4Handle->userdata,  video_format_ptr->frame_width, video_format_ptr->frame_height);
        }
    }
    return ret;
}

PUBLIC MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    MMDecRet ret = MMDEC_ERROR;
    int32 i;
    uint32 bs_buffer_length, bs_start_addr;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;

    vop_mode_ptr->error_flag = FALSE;
    mp4Handle->g_mpeg4_dec_err_flag = 0;

    if(!Mp4Dec_GetCurRecFrameBfr(vo))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
        mp4Handle->g_mpeg4_dec_err_flag |= 1;
        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }

    if(ARM_VSP_RST(vo) < 0)
    {
        ret = MMDEC_HW_ERROR;

        goto DECODER_DONE;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, vop_mode_ptr->video_std,"VSP_MODE");

    // Bitstream.
    bs_start_addr=((uint32)dec_input_ptr->pStream_phy);	// bistream start address
    bs_buffer_length= dec_input_ptr->dataLen;//bitstream length .

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000, TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (0),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

     ret = Mp4Dec_VerifyBitstrm(vo,dec_input_ptr->pStream, dec_input_ptr->dataLen);
   if(ret != MMDEC_OK)
    {
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<6;       
        goto DECODER_DONE;
    }    	
   
    if(STREAM_ID_H263 == vop_mode_ptr->video_std)
    {
        ret = Mp4Dec_DecH263Header(vo);
    }else if(STREAM_ID_MPEG4== vop_mode_ptr->video_std)
    {
        vop_mode_ptr->find_vop_header  = 0;
        ret = Mp4Dec_DecMp4Header(vo, dec_input_ptr->dataLen);
        if(!vop_mode_ptr->find_vop_header)
        {
            dec_output_ptr->VopPredType = NVOP;
            return MMDEC_OK;
        }
    }else
    {
        ret = Mp4Dec_FlvH263PicHeader(vo);
    }

    if(ret != MMDEC_OK)
    {
        //modified by xwluo, 20100511
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<1;

        if (vop_mode_ptr->VopPredType == BVOP)
        {
            //here, error occur may be NO refereance frame for BVOP, so we skip this B frame.
            dec_output_ptr->VopPredType = NVOP;
            ret = MMDEC_OK;
        }
        
        goto DECODER_DONE;
    }

    if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
    {
        if (vo->g_nFrame_dec)
        {
            vo->g_nFrame_dec++;
        }
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<2;
        ret = MMDEC_FRAME_SEEK_IVOP;

        goto DECODER_DONE;
    }

    dec_output_ptr->frameEffective = FALSE;//weihu
    dec_output_ptr->pOutFrameY = PNULL;
    dec_output_ptr->pOutFrameU = PNULL;
    dec_output_ptr->pOutFrameV = PNULL;

    if(!vop_mode_ptr->bInitSuceess)
    {
        if( MMDEC_OK != Mp4Dec_InitSessionDecode(vo) )
        {
            ret = MMDEC_MEMORY_ERROR;
        }else
        {
            vop_mode_ptr->bInitSuceess = TRUE;
            ret = MMDEC_MEMORY_ALLOCED;
        }

        goto DECODER_DONE;
    }

    if(vop_mode_ptr->VopPredType != NVOP)
    {
        //sorenson H263 and itu h.263 dont support B frame.
        if((BVOP == vop_mode_ptr->VopPredType) &&  (STREAM_ID_MPEG4 != vop_mode_ptr->video_std) )
        {
            dec_output_ptr->VopPredType = NVOP;
            ret= MMDEC_OK;

            goto DECODER_DONE;
        }

        if(!Mp4Dec_GetCurRecFrameBfr(vo))
        {
            mp4Handle->g_mpeg4_dec_err_flag |= 1<<4;
            ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;

            goto DECODER_DONE;
        }

        if (vop_mode_ptr->post_filter_en)
        {
            if(dec_input_ptr->beDisplayed)
            {
                if(!Mp4Dec_GetCurDispFrameBfr(vo))
                {
                    mp4Handle->g_mpeg4_dec_err_flag |= 1<<5;
                    ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;

                    goto DECODER_DONE;
                }
            }
        } else //for fpga verification
        {
            vop_mode_ptr->pCurDispFrame = vop_mode_ptr->pCurRecFrame;
        }
    } else //NVOP
    {
        uint32 BitConsumed,ByteConsumed;
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001, TIME_OUT_CLK, "BSM_rdy");
        BitConsumed = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "Read out BitConsumed. ") ;
        ByteConsumed = (BitConsumed + 7) >>3;

        if(ByteConsumed > dec_input_ptr->dataLen)
            dec_input_ptr->dataLen = 0;
        else
            dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

        dec_output_ptr->VopPredType = NVOP;

        PRINTF ("frame not coded!\n");
        ret = MMDEC_OK;

        goto DECODER_DONE;
    }

    Mp4Dec_InitVop(vo, dec_input_ptr);
    Mp4Dec_decode_vop(vo);//wait hw return
    Mp4Dec_output_one_frame (vo, dec_output_ptr);
    Mp4Dec_exit_picture(vo);

    vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
    vo->g_nFrame_dec++;

DECODER_DONE:
    VSP_RELEASE_Dev((VSPObject *)vo);
    SCI_TRACE_LOW("%s : error flag is 0x%x.", __FUNCTION__,mp4Handle->g_mpeg4_dec_err_flag );

    return ret;
}

MMDecRet MP4DecRelease(MP4Handle *mp4Handle)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;

    VSP_CLOSE_Dev((VSPObject *)vo);

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
