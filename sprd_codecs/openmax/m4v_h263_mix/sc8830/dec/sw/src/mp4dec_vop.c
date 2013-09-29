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

PUBLIC void Mp4Dec_exit_picture(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    if (vop_mode_ptr->uv_interleaved)
    {
        Mp4Dec_ExtendFrame(vop_mode_ptr, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV);
    }

    /*reorder reference frame list*/
    if(vop_mode_ptr->VopPredType != BVOP)
    {
        Mp4DecStorablePic * pframetmp;

        vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;

        if((vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL) && ((*(vo->mp4Handle->VSP_bindCb)) != NULL))
        {
            (*(vo->mp4Handle->VSP_bindCb))(vo->mp4Handle->userdata, vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader, 0);
        }

        /*i frame, clean reference frame buf point*/
        if(vop_mode_ptr->VopPredType == IVOP)
        {
            if((vop_mode_ptr->pBckRefFrame->pDecFrame != NULL) && ((*(vo->mp4Handle->VSP_unbindCb)) != NULL))
            {
                if(vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader!=NULL)
                {
                    (*(vo->mp4Handle->VSP_unbindCb))(vo->mp4Handle->userdata,vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader,0);
                    vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader = NULL;
                }

                vop_mode_ptr->pBckRefFrame->pDecFrame->bRef = FALSE;
                vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
            }
        }

        /*the buffer for forward reference frame will no longer be a reference frame*/
        if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
        {
            if ((vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL) && ((*(vo->mp4Handle->VSP_unbindCb)) != NULL))
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

PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
    MMDecRet ret = MMDEC_OK;

    vop_mode_ptr->sliceNumber	= 0;
    vop_mode_ptr->stop_decoding = FALSE;
    vop_mode_ptr->mbnumDec		= 0;
    vop_mode_ptr->error_flag	= FALSE;
    vop_mode_ptr->return_pos = 0;
    vop_mode_ptr->return_pos1 = 0;
    vop_mode_ptr->return_pos2 = 0;
    vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
//    vop_mode_ptr->has_interMBs  = FALSE;

    if(IVOP != vop_mode_ptr->VopPredType && vop_mode_ptr->is_expect_IVOP  == FALSE)
    {
        DEC_FRM_BFR *pDecFrame = vop_mode_ptr->pBckRefFrame->pDecFrame;

        if (pDecFrame == NULL)
        {
            ret =  MMDEC_ERROR;
        }

        if(BVOP == vop_mode_ptr->VopPredType)
        {
            pDecFrame = vop_mode_ptr->pFrdRefFrame->pDecFrame;

            if (pDecFrame == NULL)
            {
                ret =  MMDEC_ERROR;
            }
        }
    }

    //init mc function according to rounding_control
    vop_mode_ptr->g_mp4dec_mc_16x16[0] = mc_xyfull_16x16;
    vop_mode_ptr->g_mp4dec_mc_8x8[0] = mc_xyfull_8x8;
    if (vop_mode_ptr->RoundingControl == 0)
    {
        vop_mode_ptr->g_mp4dec_mc_16x16[1] = mc_xfullyhalf_16x16_rnd0;
        vop_mode_ptr->g_mp4dec_mc_16x16[2] = mc_xhalfyfull_16x16_rnd0;
        vop_mode_ptr->g_mp4dec_mc_16x16[3] = mc_xyhalf_16x16_rnd0;

        vop_mode_ptr->g_mp4dec_mc_8x8[1] = mc_xfullyhalf_8x8_rnd0;
        vop_mode_ptr->g_mp4dec_mc_8x8[2] = mc_xhalfyfull_8x8_rnd0;
        vop_mode_ptr->g_mp4dec_mc_8x8[3] = mc_xyhalf_8x8_rnd0;
    } else
    {
        vop_mode_ptr->g_mp4dec_mc_16x16[1] = mc_xfullyhalf_16x16_rnd1;
        vop_mode_ptr->g_mp4dec_mc_16x16[2] = mc_xhalfyfull_16x16_rnd1;
        vop_mode_ptr->g_mp4dec_mc_16x16[3] = mc_xyhalf_16x16_rnd1;

        vop_mode_ptr->g_mp4dec_mc_8x8[1] = mc_xfullyhalf_8x8_rnd1;
        vop_mode_ptr->g_mp4dec_mc_8x8[2] = mc_xhalfyfull_8x8_rnd1;
        vop_mode_ptr->g_mp4dec_mc_8x8[3] = mc_xyhalf_8x8_rnd1;
    }

    return ret;
}

void Mp4Dec_ExchangeMBMode (DEC_VOP_MODE_T * vop_mode_ptr)
{
    DEC_MB_MODE_T *mb_mode_tmp_ptr;

    mb_mode_tmp_ptr				= vop_mode_ptr->pMbMode;
    vop_mode_ptr->pMbMode		= vop_mode_ptr->pMbMode_prev;
    vop_mode_ptr->pMbMode_prev	= mb_mode_tmp_ptr;
}

PUBLIC void Mp4Dec_output_one_frame (Mp4DecObject *vo, MMDecOutput *dec_output_ptr)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    Mp4DecStorablePic *pic = PNULL;
    VOP_PRED_TYPE_E VopPredType = vop_mode_ptr->VopPredType;//pre_vop_type;

    if (vop_mode_ptr->uv_interleaved)
    {
        write_display_frame_uvinterleaved(vop_mode_ptr,vop_mode_ptr->pCurRecFrame->pDecFrame);

        /*output frame for display*/
        if(VopPredType != BVOP)
        {
            if(vop_mode_ptr->g_nFrame_dec > 0)
            {
                /*send backward reference (the lastest reference frame) frame's YUV to display*/
                pic = vop_mode_ptr->pBckRefFrame;
            }
        } else
        {
            /*send current B frame's YUV to display*/
            pic = vop_mode_ptr->pCurRecFrame;
        }
    } else   //only for thumbnail
    {
        write_display_frame(vop_mode_ptr,vop_mode_ptr->pCurRecFrame->pDecFrame);
        pic = vop_mode_ptr->pCurRecFrame;
    }

    if(pic != PNULL)
    {
        dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
        dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
        dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
        dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
        dec_output_ptr->is_transposed = 0;
        dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
        dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
        dec_output_ptr->frameEffective = 1;
    } else
    {
        dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
        dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
        dec_output_ptr->frameEffective = 0;
    }
}

PUBLIC void write_display_frame(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame)
{
    int16 FrameWidth= vop_mode_ptr->FrameWidth;
    int16 FrameHeight= vop_mode_ptr->FrameHeight;
    int16 FrameExtendWidth= vop_mode_ptr->FrameExtendWidth;
    int16 FrameExtendHeigth= vop_mode_ptr->FrameExtendHeigth;
    int16 iStartInFrameUV= vop_mode_ptr->iStartInFrameUV;

    uint8 *pSrc_y, *pSrc_u, *pSrc_v;
    uint8 *pDst_y, *pDst_u, *pDst_v;
    int32 row;

    //y
    pSrc_y = pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
    pDst_y = pDecFrame->imgY;

    for (row = 0; row < FrameHeight; row++)
    {
        memcpy(pDst_y, pSrc_y, FrameWidth);

        pSrc_y += FrameExtendWidth;
        pDst_y += FrameWidth;
    }

    //u and v
    FrameHeight >>= 1;
    FrameWidth >>= 1;
    FrameExtendWidth >>= 1;

    pSrc_u = pDecFrame->imgYUV[1] + iStartInFrameUV;
    pSrc_v = pDecFrame->imgYUV[2] + iStartInFrameUV;
    pDst_u = pDecFrame->imgU;
    pDst_v = pDecFrame->imgV;

    if(!vop_mode_ptr->uv_interleaved)
    {
        for (row = 0; row < FrameHeight; row++)
        {
            memcpy(pDst_u, pSrc_u, FrameWidth);
            memcpy(pDst_v, pSrc_v, FrameWidth);

            pDst_u += FrameWidth;
            pDst_v += FrameWidth;
            pSrc_u += FrameExtendWidth;
            pSrc_v += FrameExtendWidth;
        }
    } else
    {
        uint8 *pDst = vop_mode_ptr->pCurRecFrame->pDecFrame->imgU;
        int32 col;
#if 1//ndef _NEON_OPT_ 
        for (row = 0; row < FrameHeight; row++)
        {
            for (col = 0; col < FrameWidth; col++)
            {
                pDst[col*2+0] = pSrc_u[col];
                pDst[col*2+1] = pSrc_v[col];
            }

            pDst += FrameWidth*2;
            pSrc_u += FrameExtendWidth;
            pSrc_v += FrameExtendWidth;
        }
#else
        uint8x8_t u_vec64, v_vec64;
        uint16x8_t u_vec128, v_vec128;
        uint16x8_t uv_vec128;

        for (row = 0; row < height; row++)
        {
            for (col = 0; col < width; col+=8)
            {
                u_vec64 = vld1_u8(pSrc_u);
                v_vec64 = vld1_u8(pSrc_v);

                u_vec128 = vmovl_u8(u_vec64);
                v_vec128 = vmovl_u8(v_vec64);

                uv_vec128 = vsliq_n_u16(u_vec128, v_vec128, 8);
                vst1q_u16((uint16*)pDst, uv_vec128);

                pSrc_u += 8;
                pSrc_v += 8;
                pDst += 16;
            }
            pSrc_u += (UV_EXTEND_SIZE*2);
            pSrc_v += (UV_EXTEND_SIZE*2);
        }
#endif
    }
}

PUBLIC void Mp4Dec_ExtendFrame(DEC_VOP_MODE_T *vop_mode_ptr, uint8**Frame )
{
    int32 i;
    int32 height, width, offset, extendWidth, copyWidth;
    uint8 *pSrc1, *pSrc2, *pDst1, *pDst2;
#ifdef _NEON_OPT_
    uint8x8_t vec64;
    uint8x16_t vec128;
#endif

    height      = vop_mode_ptr->FrameHeight;
    width       = vop_mode_ptr->FrameWidth;
    extendWidth = vop_mode_ptr->FrameExtendWidth;
    offset      = vop_mode_ptr->iStartInFrameY;

    pSrc1 = Frame[0] + offset;
    pDst1 = pSrc1 - YEXTENTION_SIZE;
    pSrc2 = pSrc1 + width - 1;
    pDst2 = pSrc2 + 1;
    copyWidth = YEXTENTION_SIZE;


    /*horizontal repeat Y*/
    for(i = 0; i < height; i++)
    {
#ifndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 * pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
        pIntDst[3] = intValue;

        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
        pIntDst[3] = intValue;

        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
#else
        //left
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        pSrc1 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst1, vec128);
        pDst1 += extendWidth;
        //right
        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        pSrc2 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst2, vec128);
        pDst2 += extendWidth;

#endif

    }

    /*horizontal repeat U*/
    extendWidth = extendWidth / 2;
    offset      = vop_mode_ptr->iStartInFrameUV;
    pSrc1       = Frame [1] + offset;
    pDst1       = pSrc1 - UVEXTENTION_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    copyWidth   = UVEXTENTION_SIZE;
    height = height / 2;

    for(i = 0; i < height; i++)
    {
#ifndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 *pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;


        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;

        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
#else
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        pSrc1 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst1, vec64);
        pDst1 += extendWidth;

        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        pSrc2 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst2, vec64);
        pDst2 += extendWidth;
#endif

    }

    /*horizontal repeat V*/
    pSrc1 = Frame [2] + offset;
    pDst1 = pSrc1 - UVEXTENTION_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    for (i = 0; i < height; i++)
    {
#ifndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 * pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;

        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;

        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
#else
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        pSrc1 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst1, vec64);
        pDst1 += extendWidth;

        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        pSrc2 += extendWidth;

        vec128 = vdupq_lane_u8(vec64, 0);
        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst2, vec64);
        pDst2 += extendWidth;

#endif

    }

    /*copy first row and last row*/
    /*vertical repeat Y*/
    height = vop_mode_ptr->FrameHeight;
    extendWidth  = vop_mode_ptr->FrameExtendWidth;
    offset = extendWidth * YEXTENTION_SIZE;
    pSrc1  = Frame[0] + offset;
    pDst1  = Frame[0];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < YEXTENTION_SIZE; i++)
    {
        memcpy(pDst1, pSrc1, extendWidth);
        memcpy(pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }

    /*vertical repeat U*/
    height = height / 2;
    extendWidth  = extendWidth / 2;
    offset = offset / 4;
    pSrc1  = Frame[1] + offset;
    pDst1  = Frame[1];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < UVEXTENTION_SIZE; i++)
    {
        memcpy (pDst1, pSrc1, extendWidth);
        memcpy (pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }

    /*vertical repeat V*/
    pSrc1  = Frame[2] + offset;
    pDst1  = Frame[2];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < UVEXTENTION_SIZE; i++)
    {
        memcpy (pDst1, pSrc1, extendWidth);
        memcpy (pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOP
 ** Description:	Decode the IVOP and error resilience is disable.
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 pos_y, pos_x;
    DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
    DEC_MB_BFR_T* mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    BOOLEAN is_itu_h263 = (ITU_H263 == vop_mode_ptr->video_std)?1:0;
    int32 total_mb_num_x = vop_mode_ptr->MBNumX;
    int32 total_mb_num_y = vop_mode_ptr->MBNumY;
    MMDecRet ret = MMDEC_OK;
    uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon

    ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
    ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
    ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

    for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
    {
        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
        mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
        mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
        vop_mode_ptr->mb_y = (int8)pos_y;

        if(is_itu_h263)
        {
            ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
        }

        /*decode one MB line*/
        for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
        {
            vop_mode_ptr->mb_x = (int8)pos_x;

            if(!vop_mode_ptr->bResyncMarkerDisable)
            {
                mb_mode_ptr->bFirstMB_in_VP = FALSE;

                if(Mp4Dec_CheckResyncMarker(vop_mode_ptr, 0))
                {
                    ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,  0);
                    pos_x = vop_mode_ptr->mb_x;
                    pos_y = vop_mode_ptr->mb_y;
                    mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
                    mb_mode_ptr->bFirstMB_in_VP = TRUE;
                }
            }

            if((0 == pos_y) && (0 == pos_x))
            {
                mb_mode_ptr->bFirstMB_in_VP = TRUE;
            }

            mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
            Mp4Dec_DecIntraMBHeader(vop_mode_ptr, mb_mode_ptr);

            Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

            ((int*)mb_mode_ptr->mv)[0] = 0;   //set to zero for B frame'mv prediction
            ((int*)mb_mode_ptr->mv)[1] = 0;
            ((int*)mb_mode_ptr->mv)[2] = 0;
            ((int*)mb_mode_ptr->mv)[3] = 0;
            mb_mode_ptr++;

            //updated for next mb
            mb_cache_ptr->mb_addr[0] += MB_SIZE;
            mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
            mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
            vop_mode_ptr->mbnumDec++;
        }

        ppxlcRecGobY += vop_mode_ptr->FrameExtendWidth * MB_SIZE;
        ppxlcRecGobU += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;
        ppxlcRecGobV += vop_mode_ptr->FrameExtendWidth * MB_SIZE / 4;

        vop_mode_ptr->GobNum++;
    }

    return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOP
 ** Description:	Decode the PVOP and error resilience is disable.
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecPVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 pos_y, pos_x;
    DEC_MB_MODE_T * mb_mode_ptr = vop_mode_ptr->pMbMode;
    DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == ITU_H263)?1:0;
    int32 total_mb_num_x = vop_mode_ptr->MBNumX;
    int32 total_mb_num_y = vop_mode_ptr->MBNumY;
    MMDecRet ret = MMDEC_OK;
    uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;//leon
#ifdef _DEBUG_TIME_
    struct timeval tpstart,tpend;
    long long  cur_time;
    gettimeofday(&tpstart,NULL);
#endif

    Mp4Dec_ExchangeMBMode (vop_mode_ptr);

    //ref frame
    if(vop_mode_ptr->is_expect_IVOP == FALSE)
    {
        vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
        vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
        vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];
    }

    ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
    ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
    ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

    for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
    {
        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
        mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
        mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
        vop_mode_ptr->mb_y = (int8)pos_y;

        if(is_itu_h263)
        {
            /*decode GOB header*/
            Mp4Dec_DecGobHeader(vop_mode_ptr);
        }

        /*decode one MB line*/
        for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
        {
#if _DEBUG_
            if((pos_x == 0)&&(pos_y == 0)&&(vop_mode_ptr->g_nFrame_dec == 1))
            {
                foo();
            }
#endif //_DEBUG_
            vop_mode_ptr->mb_x = (int8)pos_x;

            if(!vop_mode_ptr->bResyncMarkerDisable)
            {
                mb_mode_ptr->bFirstMB_in_VP = FALSE;

                if(Mp4Dec_CheckResyncMarker(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1))
                {
                    ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

                    pos_x = vop_mode_ptr->mb_x;
                    pos_y = vop_mode_ptr->mb_y;
                    mb_mode_ptr->bFirstMB_in_VP = TRUE;
                }
            }

            if((0 == pos_y) && (0 == pos_x))
            {
                mb_mode_ptr->bFirstMB_in_VP = TRUE;
            }

            mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
            Mp4Dec_DecInterMBHeader(vop_mode_ptr, mb_mode_ptr);
            if (vop_mode_ptr->is_expect_IVOP == TRUE &&  mb_mode_ptr->bIntra == FALSE)
            {
                return MMDEC_STREAM_ERROR;
            }

            Mp4Dec_DecMV(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

            if(mb_mode_ptr->bIntra)
            {
                Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
            } else if (mb_mode_ptr->CBP)
            {
                //vop_mode_ptr->has_interMBs = TRUE;
                if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
                {
                    return MMDEC_STREAM_ERROR;
                }
                Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
            }

            //updated for next mb
            mb_cache_ptr->mb_addr[0] += MB_SIZE;
            mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
            mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
            vop_mode_ptr->mbnumDec++;
            mb_mode_ptr++;
        }

        ppxlcRecGobY += (vop_mode_ptr->FrameExtendWidth <<4);//* MB_SIZE;
        ppxlcRecGobU += (vop_mode_ptr->FrameExtendWidth <<2);//* MB_SIZE / 4;
        ppxlcRecGobV += (vop_mode_ptr->FrameExtendWidth <<2);//MB_SIZE / 4;

        vop_mode_ptr->GobNum++;
    }

#ifdef _DEBUG_TIME_
    gettimeofday(&tpend,NULL);
    //recCrop_time +=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    cur_time = tpend.tv_usec-tpstart.tv_usec;
    if(cur_time < 0)
    {
        cur_time += 1000000;
    }
    SCI_TRACE_LOW("cur frame % dec time %lld",vop_mode_ptr->g_nFrame_dec,cur_time);
#endif
    return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecBVOP
 ** Description:	Decode the PVOP and error resilience is disable.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecBVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
    MMDecRet ret = MMDEC_OK;
    return ret;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
