/******************************************************************************
 ** File Name:    mp4dec_bfrctrl.c                                          *
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

PUBLIC MMDecRet Mp4Dec_InitYUVBfr(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    int32 id;
    int32 size_y, size_c;
    DEC_FRM_BFR *pDecFrame;

    size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
    size_c = size_y >> 2;

    memset(vo->g_FrmYUVBfr,0,sizeof(vo->g_FrmYUVBfr));
    pDecFrame = vo->g_FrmYUVBfr;

    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        pDecFrame->id = id;
        pDecFrame->bRef = FALSE;
        pDecFrame->bDisp = FALSE;

        // Malloc frame info buffer. 80 Bytes for each MB.
        pDecFrame->rec_info = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)((size_y >>8)*80), 256, HW_NO_CACHABLE);
        CHECK_MALLOC(pDecFrame->rec_info, "pDecFrame->rec_info");
        pDecFrame->rec_infoAddr = (uint32)Mp4Dec_MemV2P(vo, pDecFrame->rec_info, HW_NO_CACHABLE);

        pDecFrame++;
    }

    return MMDEC_OK;
}

/**********************************************************
check is there free frame buffer, if has, return the frame buffer ID
************************************************************/
/*find a buffer which has been displayed and will no longer be a reference frame from g_dec_frame,
and set the buffer to be reference frame, if current frame picture type is not B frame */
BOOLEAN Mp4Dec_GetOneFreeDecBfr(Mp4DecObject *vo, int32 *bfrId)
{
    int id;
    DEC_FRM_BFR *FrmYUVBfr = &(vo->g_FrmYUVBfr[0]);

    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        if((!FrmYUVBfr[id].bRef) && (!FrmYUVBfr[id].bDisp))
        {
            *bfrId = id;
            break;
        }
    }

    if(id == DEC_YUV_BUFFER_NUM)
    {
        return FALSE;
    } else
    {
        return TRUE;
    }
}

BOOLEAN Mp4Dec_GetOneFreeDispBfr(Mp4DecObject *vo, int32 *bfrId)
{
    int id;
    DEC_FRM_BFR *DispFrmYUVBfr = &(vo->g_DispFrmYUVBfr[0]);

    for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
    {
        if((!DispFrmYUVBfr[id].bRef) && (!DispFrmYUVBfr[id].bDisp))
        {
            *bfrId = id;
            break;
        }
    }

    if(id == DISP_YUV_BUFFER_NUM)
    {
        return FALSE;
    } else
    {
        return TRUE;
    }
}

/*allocate frame buffer for current frame before current frame decoding*/
BOOLEAN Mp4Dec_GetCurRecFrameBfr(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    int32 bfrId;
    DEC_FRM_BFR *pDecFrame;

    while(!Mp4Dec_GetOneFreeDecBfr(vo, &bfrId))
    {
        //wait for display free buffer
//		PRINTF("no buffer is available!\n");
        return FALSE;
    }

    pDecFrame = vop_mode_ptr->pCurRecFrame->pDecFrame = &(vo->g_FrmYUVBfr[bfrId]);
    vop_mode_ptr->pCurRecFrame->bfrId = bfrId;

    if(!vop_mode_ptr->post_filter_en)
    {
        int32 size_y;

        size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

        pDecFrame->imgY = vo->g_rec_buf.imgY;
        pDecFrame->imgU = pDecFrame->imgY + size_y;
        pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

        pDecFrame->imgYAddr = vo->g_rec_buf.imgYAddr;
        pDecFrame->imgUAddr = pDecFrame->imgYAddr + size_y;
        pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>2);

        vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader= vo->g_rec_buf.pBufferHeader;
    }

    return TRUE;
}

/*allocate frame buffer for current frame before current frame displaying*/
BOOLEAN Mp4Dec_GetCurDispFrameBfr(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    int32 bfrId;
    int32 size_y;
    DEC_FRM_BFR *pDecFrame;

    while(!Mp4Dec_GetOneFreeDispBfr(vo, &bfrId))
    {
        //wait for display free buffer
//		PRINTF("no buffer is available!\n");
        return FALSE;
    }

    pDecFrame = vop_mode_ptr->pCurDispFrame->pDecFrame = &(vo->g_DispFrmYUVBfr[bfrId]);
    vop_mode_ptr->pCurDispFrame->bfrId = bfrId;

    //recode it's corresponding decoding frame.
    pDecFrame->rec_imgY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;
    size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

    pDecFrame->imgY = vo->g_rec_buf.imgY;
    pDecFrame->imgU = pDecFrame->imgY + size_y;
    pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

    pDecFrame->imgYAddr = vo->g_rec_buf.imgYAddr;
    pDecFrame->imgUAddr = pDecFrame->imgYAddr + size_y;
    pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>2);

    vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader= vo->g_rec_buf.pBufferHeader;

    return TRUE;
}

/*get the display frame which is corresponding to reconstruction frame*/
DEC_FRM_BFR* Mp4Dec_GetDispFrameBfr(Mp4DecObject *vo, Mp4DecStorablePic *rec_pic)
{
    int32 i;
    uint8 *rec_imgY = PNULL;
    DEC_FRM_BFR *disp_frm = PNULL;
    DEC_FRM_BFR *DispFrmYUVBfr = &(vo->g_DispFrmYUVBfr[0]);

    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    vop_mode_ptr->pCurDispFrame->pDecFrame->bDisp = 1;
    vop_mode_ptr->pCurDispFrame->pDecFrame->bRef = 1;
    if((vop_mode_ptr->VopPredType != BVOP)&&(vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader!=NULL))
    {
        (*(vo->mp4Handle->VSP_bindCb))(vo->mp4Handle->userdata, vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader, 0);
    }

    if( (PNULL == rec_pic) || (PNULL == rec_pic->pDecFrame) )
    {
        return PNULL;
    }

    rec_imgY = rec_pic->pDecFrame->imgY;

    for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
    {
        if(DispFrmYUVBfr[i].bDisp && (rec_imgY == DispFrmYUVBfr[i].rec_imgY))
        {
            disp_frm = &DispFrmYUVBfr[i];
            break;
        }
    }

    if(DISP_YUV_BUFFER_NUM == i)
    {
        return PNULL;
    }

    disp_frm->bDisp = FALSE;
    disp_frm->bRef = FALSE;
    if((vop_mode_ptr->VopPredType != BVOP)&&(disp_frm->pBufferHeader!=NULL))
    {
        (*(vo->mp4Handle->VSP_unbindCb))(vo->mp4Handle->userdata,disp_frm->pBufferHeader, 0);
    }

    return disp_frm;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

