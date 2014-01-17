/******************************************************************************
 ** File Name:    mp4dec_interface.c  		                                  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         08/25/2008                                                  *
 ** Copyright:    2008 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
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

PUBLIC void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_FRM_BFR *rec_buf_ptr = &(vo->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint32)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;

//    SCI_TRACE_LOW("%s: %s  %x\n", __FILE__, __FUNCTION__, pFrameY);
}

PUBLIC void MP4DecSetReferenceYUV(MP4Handle *mp4Handle, uint8 *pFrameY)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    uint32 frame_size = vop_mode_ptr->FrameExtendWidth*vop_mode_ptr->FrameExtendHeight;
    DEC_FRM_BFR *pDecFrame;

    pDecFrame = vop_mode_ptr->pBckRefFrame->pDecFrame = &vo->g_FrmYUVBfr[1];

    memset(pDecFrame->imgYUV[0], 16, sizeof(uint8)*frame_size);
    memset(pDecFrame->imgYUV[1], 128, sizeof(uint8)*frame_size / 4);
    memset(pDecFrame->imgYUV[2], 128, sizeof(uint8)*frame_size / 4);
}

void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    DEC_FRM_BFR *FrmYUVBfr = &(vo->g_FrmYUVBfr[0]);
    int i;

    if(!vop_mode_ptr)
        return;

    for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
    {
        if(vo->g_FrmYUVBfr[i].bRef)
        {
            FrmYUVBfr[i].bRef = FALSE;
            if((*mp4Handle->VSP_unbindCb) && FrmYUVBfr[i].pBufferHeader!=NULL) {
                (*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,FrmYUVBfr[i].pBufferHeader,0);
                //g_FrmYUVBfr[i].pBufferHeader = NULL;
            }
        }
    }

    vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
    vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
    vop_mode_ptr->g_nFrame_dec = 0;
}

int MP4DecGetLastDspFrm(MP4Handle *mp4Handle, void **pOutput)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
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
        *pOutput = frm_bfr->pBufferHeader;

        return TRUE;
    } else
    {
        return FALSE;
    }
}

PUBLIC void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    *display_width = vop_mode_ptr->OrgFrameWidth;
    *display_height = vop_mode_ptr->OrgFrameHeight;

    SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    *width = vop_mode_ptr->FrameWidth;
    *height = vop_mode_ptr->FrameHeight;

    SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}

PUBLIC MMDecRet MP4GetCodecCapability(MP4Handle *mp4Handle, int32 *max_width, int32 *max_height)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;

    //*codec_capability = 1/*vo->vsp_capability*/;
    *max_width = 1920;
    *max_height = 1088;
    return MMDEC_OK;
}

PUBLIC MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer *buffer_ptr)
{
    Mp4DecObject *vo;
    MMDecRet is_init_success = MMDEC_OK;
    MMCodecBuffer tmp_buffer;

    SCI_TRACE_LOW("libomx_m4vh263dec_sw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    vo = (Mp4DecObject *) (buffer_ptr->common_buffer_ptr);

    memset(vo, 0, sizeof(Mp4DecObject));
    mp4Handle->videoDecoderData = (void *) vo;
    vo->mp4Handle = mp4Handle;

    buffer_ptr->common_buffer_ptr += sizeof(Mp4DecObject);
    buffer_ptr->common_buffer_ptr_phy = (void *)((uint32)(buffer_ptr->common_buffer_ptr_phy) + sizeof(Mp4DecObject));
    buffer_ptr->size -= sizeof(Mp4DecObject);
    Mp4Dec_InitInterMem(vo, buffer_ptr);

    return Mp4Dec_InitGlobal(vo);
}

PUBLIC MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    MMDecRet ret = MMDEC_OK;

    vop_mode_ptr->uv_interleaved = video_format_ptr->uv_interleaved;
    vop_mode_ptr->video_std = video_format_ptr->video_std;

    /*judge h.263 or mpeg4*/
    if(video_format_ptr->video_std != MPEG4)
    {
        SCI_TRACE_LOW ("H263(ITU or Sorenson format) is detected");
    } else
    {
        if(video_format_ptr->i_extra > 0)
        {
            Mp4Dec_InitBitstream(vop_mode_ptr->bitstrm_ptr, video_format_ptr->p_extra, video_format_ptr->i_extra);

            ret = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);
            if(MMDEC_OK == ret)
            {
                video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
                video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;

                vop_mode_ptr->FrameWidth =  ((vop_mode_ptr->OrgFrameWidth  + 15)>>4)<<4;
                vop_mode_ptr->FrameHeight  = ((vop_mode_ptr->OrgFrameHeight + 15) >>4)<<4;
            }

            SCI_TRACE_LOW("%s, %d, ret: %d, org_width: %d, org_height: %d, width: %d, height: %d", __FUNCTION__, __LINE__,
                          ret, vop_mode_ptr->OrgFrameWidth, vop_mode_ptr->OrgFrameHeight, vop_mode_ptr->FrameWidth, vop_mode_ptr->FrameHeight);
        }
    }

    return ret;
}

PUBLIC MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    Mp4DecObject *vo = (Mp4DecObject *) mp4Handle->videoDecoderData;
    MMDecRet ret = MMDEC_ERROR;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    SCI_TRACE_LOW("MP4DecDecode: E");

    mp4Handle->g_mpeg4_dec_err_flag = 0;

FLV_RE_DEC:
    vop_mode_ptr->error_flag = FALSE;
    vop_mode_ptr->return_pos = 0;
    vop_mode_ptr->return_pos1 = 0;
    vop_mode_ptr->return_pos2 = 0;

    Mp4Dec_InitBitstream(vop_mode_ptr->bitstrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);

    ret = Mp4Dec_VerifyBitstrm(vo,dec_input_ptr->pStream, dec_input_ptr->dataLen);
    if(ret != MMDEC_OK)
    {
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<0;
        return ret;
    }

    if(ITU_H263 == vop_mode_ptr->video_std)
    {
        ret = Mp4Dec_DecH263Header(vo);
    } else if(MPEG4== vop_mode_ptr->video_std)
    {
        vop_mode_ptr->find_vop_header  = 0;
        ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
        if(!vop_mode_ptr->find_vop_header)
        {
            dec_output_ptr->VopPredType = NVOP;
            return MMDEC_OK;
        }
    } else
    {
        ret = Mp4Dec_FlvH263PicHeader(vo);
    }

    SCI_TRACE_LOW("%s, %d, ret: %d, pic_type: %d", __FUNCTION__, __LINE__, ret, vop_mode_ptr->VopPredType);
    if(ret != MMDEC_OK)
    {
        //modified by xwluo, 20100511
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<1;
        return ret;
    } else if (!vop_mode_ptr->is_work_mode_set)
    {
        MP4Dec_JudgeDecMode(vop_mode_ptr);
        vop_mode_ptr->is_work_mode_set = 1;
    }

#if 0	//removed for bug211978, seek with fake IVOP
    if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
    {
        if (vop_mode_ptr->g_nFrame_dec)
        {
            vop_mode_ptr->g_nFrame_dec++;
        }
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<2;
        return MMDEC_FRAME_SEEK_IVOP;
    }
#endif

    vop_mode_ptr->is_expect_IVOP  = dec_input_ptr->expected_IVOP;
    dec_output_ptr->frameEffective = FALSE;
    dec_output_ptr->pOutFrameY = PNULL;
    dec_output_ptr->pOutFrameU = PNULL;
    dec_output_ptr->pOutFrameV = PNULL;

    if(!vop_mode_ptr->bInitSuceess)
    {
        if( MMDEC_OK != Mp4Dec_InitSessionDecode(vo) )
        {
            mp4Handle->g_mpeg4_dec_err_flag |= 1<<3;
            return MMDEC_MEMORY_ERROR;
        }
        vop_mode_ptr->bInitSuceess = TRUE;
        return MMDEC_MEMORY_ALLOCED;
    }

    if(vop_mode_ptr->VopPredType != NVOP)
    {
        //sorenson H263 and itu h.263 dont support B frame.
        if((BVOP == vop_mode_ptr->VopPredType) &&  (MPEG4 != vop_mode_ptr->video_std) )
        {
            dec_output_ptr->VopPredType = NVOP;
            return MMDEC_OK;
        }

        if(!Mp4Dec_GetCurRecFrameBfr(vo))
        {
            mp4Handle->g_mpeg4_dec_err_flag |= 1<<4;
            return MMDEC_OUTPUT_BUFFER_OVERFLOW;
        }
    } else //NVOP
    {
        SCI_TRACE_LOW ("MP4DecDecode: frame not coded!\n");

        {
            uint32 ByteConsumed = Mp4Dec_ByteConsumed(vop_mode_ptr);
            if(ByteConsumed > dec_input_ptr->dataLen)
                dec_input_ptr->dataLen = 0;
            else
                dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;
            dec_output_ptr->VopPredType = NVOP;
        }

        return MMDEC_OK;
    }

    ret = Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);

    if(ret != MMDEC_OK)
    {
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<6;
        return ret;
    }


    if(IVOP == vop_mode_ptr->VopPredType)
    {
        SCI_TRACE_LOW ("\t I VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,vop_mode_ptr->g_nFrame_dec);
#if _TRACE_
        FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", vop_mode_ptr->g_nFrame_dec);
#endif //_TRACE_	

        ret = vo->g_Mp4Dec_IVOP(vop_mode_ptr);
    } else if(PVOP == vop_mode_ptr->VopPredType)
    {
        SCI_TRACE_LOW ("\t P VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,vop_mode_ptr->g_nFrame_dec);
#if _TRACE_
        FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", vop_mode_ptr->g_nFrame_dec);
#endif //_TRACE_	

        ret = vo->g_Mp4Dec_PVOP(vop_mode_ptr);

//        if (vop_mode_ptr->g_nFrame_dec == 0 && vop_mode_ptr->has_interMBs)
//        {
//            return MMDEC_FRAME_SEEK_IVOP;
//        }

        if (ret != MMDEC_OK && vop_mode_ptr->is_expect_IVOP  == TRUE)
        {
            return MMDEC_FRAME_SEEK_IVOP;
        }
    } else if(SVOP == vop_mode_ptr->VopPredType)
    {
        SCI_TRACE_LOW ("\t S VOP, Don't SUPPORTED!\n");

#if _TRACE_
        FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: SVOP\n", vop_mode_ptr->g_nFrame_dec);
#endif //_TRACE_	
//		vop_mode_ptr->be_gmc_wrap = TRUE;
//		Mp4Dec_DecPVOP(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
    } else if(NVOP == vop_mode_ptr->VopPredType)
    {
        SCI_TRACE_LOW ("frame not coded!\n");
        MPEG4_DecReleaseDispBfr(vo, vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
        return MMDEC_OK;
    } else
    {
        SCI_TRACE_LOW ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
#if _TRACE_
        FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: BVOP\n", vop_mode_ptr->g_nFrame_dec);
#endif //_TRACE_	
        vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
        vop_mode_ptr->err_MB_num = 0;
        ret = vo->g_Mp4Dec_BVOP(vop_mode_ptr);
    }

    if (vop_mode_ptr->VT_used)
    {
        //modify for bug#232314 & 222962 & 250022
        if (vop_mode_ptr->OrgFrameWidth != vop_mode_ptr->PreOrgFrameWidth ||
                vop_mode_ptr->OrgFrameHeight != vop_mode_ptr->PreOrgFrameHeight)
        {
            SCI_TRACE_LOW("%s, %d, error concealment for VT mode", __FUNCTION__, __LINE__);

            vop_mode_ptr->OrgFrameWidth = vop_mode_ptr->PreOrgFrameWidth;
            vop_mode_ptr->OrgFrameHeight = vop_mode_ptr->PreOrgFrameHeight;

            vop_mode_ptr->FrameWidth =  ((vop_mode_ptr->OrgFrameWidth  + 15)>>4)<<4;
            vop_mode_ptr->FrameHeight  = ((vop_mode_ptr->OrgFrameHeight + 15) >>4)<<4;
        }

        if(vop_mode_ptr->err_MB_num)
        {
            SCI_TRACE_LOW ("MP4DecDecode: Detect error bitstream, try to conceal!\n");

            if(IVOP == vop_mode_ptr->VopPredType)
            {
                Mp4Dec_EC_IVOP(vop_mode_ptr);
            } else if(PVOP == vop_mode_ptr->VopPredType)
            {
                Mp4Dec_EC_PVOP(vop_mode_ptr);
            }
        }

        if(vop_mode_ptr->post_filter_en)
        {
            SCI_TRACE_LOW("%s, deblock", __FUNCTION__);
            Mp4Dec_Deblock_vop(vo);
        }

        ret = MMDEC_OK;
    }

    Mp4Dec_output_one_frame (vo, dec_output_ptr);
    Mp4Dec_exit_picture(vo);

    vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
    vop_mode_ptr->g_nFrame_dec++;

    {
        uint32 ByteConsumed = Mp4Dec_ByteConsumed(vop_mode_ptr);

        SCI_TRACE_LOW ("%s, %d, ByteConsumed: %d, dec_input_ptr->dataLen:%d", __FUNCTION__, __LINE__, ByteConsumed, dec_input_ptr->dataLen);

        if(ByteConsumed > dec_input_ptr->dataLen)
            dec_input_ptr->dataLen = 0;
        else
            dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

    }

    SCI_TRACE_LOW("MP4DecDecode: X");

    if(ret != MMDEC_OK)
    {
        mp4Handle->g_mpeg4_dec_err_flag |= 1<<10;
    }

    return ret;
}

MMDecRet MP4DecRelease(MP4Handle *mp4Handle)
{
    Mp4DecObject *vo = mp4Handle->videoDecoderData;

    MP4DecReleaseRefBuffers(mp4Handle);

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
