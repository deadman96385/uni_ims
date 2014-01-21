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
/*lint -save -e553 */
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC MMDecRet Mp4Dec_InitYUVBfr(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    int32 id;
    int32 size_y, size_c;
    int32 ext_size_y, ext_size_c;
    DEC_FRM_BFR *pDecFrame;
    int32 need_malloc_decY = 0, need_malloc_decYUV = 0;

#ifdef _SIMULATION_
    need_malloc_decY = 1;
#endif
    need_malloc_decYUV = 1;

    size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
    size_c = size_y >> 2;

    ext_size_y = vop_mode_ptr->FrameExtendWidth * vop_mode_ptr->FrameExtendHeight;
    ext_size_c = ext_size_y >> 2;

    memset(vo->g_FrmYUVBfr,0,sizeof(vo->g_FrmYUVBfr));
    pDecFrame = vo->g_FrmYUVBfr;
    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        pDecFrame->id = id;
        pDecFrame->bRef = FALSE;
        pDecFrame->bDisp = FALSE;

        if (need_malloc_decY)
        {
            pDecFrame->imgY = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)size_y * sizeof(uint8), 256, SW_CACHABLE);  //y
            CHECK_MALLOC(pDecFrame->imgY, "pDecFrame->imgY");
            if (vop_mode_ptr->uv_interleaved) //two plane
            {
                pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vo, 2*(uint32)size_c * sizeof(uint8), 256, SW_CACHABLE);  //uv
                CHECK_MALLOC(pDecFrame->imgU, "pDecFrame->imgU");

                pDecFrame->imgV = PNULL;
            } else //three plane
            {
                pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)size_c * sizeof(uint8), 256, SW_CACHABLE);  //u
                CHECK_MALLOC(pDecFrame->imgU, "pDecFrame->imgU");

                pDecFrame->imgV = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)size_c * sizeof(uint8), 256, SW_CACHABLE);  //v
                CHECK_MALLOC(pDecFrame->imgV, "pDecFrame->imgV");
            }
        }

        if (need_malloc_decYUV)
        {
            pDecFrame->imgYUV[0] = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)ext_size_y * sizeof(uint8), 256, SW_CACHABLE);  //y
            CHECK_MALLOC(pDecFrame->imgYUV[0], "pDecFrame->imgYUV[0]");

            pDecFrame->imgYUV[1] = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)ext_size_c * sizeof(uint8), 256, SW_CACHABLE);  //u
            CHECK_MALLOC(pDecFrame->imgYUV[1], "pDecFrame->imgYUV[1]");

            pDecFrame->imgYUV[2] = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)(ext_size_c + 8) * sizeof(uint8), 256, SW_CACHABLE);  //v, 8 extra byte for mc loading of V.
            CHECK_MALLOC(pDecFrame->imgYUV[2] , "pDecFrame->imgYUV[2]");

            //modify for bug#212454
            memset(pDecFrame->imgYUV[0], 128, (uint32)ext_size_y * sizeof(uint8));
            memset(pDecFrame->imgYUV[1], 128, (uint32)ext_size_c * sizeof(uint8));
            memset(pDecFrame->imgYUV[2], 128, (uint32)(ext_size_c + 8) * sizeof(uint8));
        }

        pDecFrame++;
    }

    if(vop_mode_ptr->post_filter_en)
    {
        vo->g_dbk_tmp_frm_ptr = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)ext_size_y * sizeof(uint8), 256, SW_CACHABLE);

        CHECK_MALLOC(vo->g_dbk_tmp_frm_ptr , "vo->g_dbk_tmp_frm_ptr");
    }

    return MMDEC_OK;
}

/**********************************************************
check is there free frame buffer, if has, return the frame buffer ID
************************************************************/
/*find a buffer which has been displayed and will no longer be a reference frame from g_dec_frame,
and set the buffer to be reference frame, if current frame picture type is not B frame */
static BOOLEAN Mp4Dec_GetOneFreeDecBfr(Mp4DecObject *vo, int32 *bfrId)
{
    int id;

    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        if((!vo->g_FrmYUVBfr[id].bRef) && (!vo->g_FrmYUVBfr[id].bDisp))
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

static BOOLEAN Mp4Dec_GetOneFreeDispBfr(Mp4DecObject *vo, int32 *bfrId)
{
    int id;

    for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
    {
        if((!vo->g_DispFrmYUVBfr[id].bRef) && (!vo->g_DispFrmYUVBfr[id].bDisp))
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
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    int32 bfrId;
    DEC_FRM_BFR *pDecFrame;
    int32 size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

    while(!Mp4Dec_GetOneFreeDecBfr(vo, &bfrId))
    {
        //wait for display free buffer
        SCI_TRACE_LOW("%s: no buffer is available!\n", __FUNCTION__);
        return FALSE;
    }

    vop_mode_ptr->pCurRecFrame->bfrId = bfrId;
    pDecFrame = vop_mode_ptr->pCurRecFrame->pDecFrame = &vo->g_FrmYUVBfr[bfrId];
    pDecFrame->pBufferHeader= vo->g_rec_buf.pBufferHeader;
    pDecFrame->imgY = vo->g_rec_buf.imgY;
    pDecFrame->imgU = pDecFrame->imgY + size_y; //g_rec_buf.imgU;
    pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);//g_rec_buf.imgV;

    return TRUE;
}

/****************************************************
input: the address of Y frame to be released which has been displayed
output: set the corresponding frame's display type to 0
****************************************************/
MMDecRet MPEG4_DecReleaseDispBfr(Mp4DecObject *vo, uint8 *pBfrAddr)
{
    int i;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    SCI_ASSERT(pBfrAddr != NULL);

    for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
    {
        if(pBfrAddr == vo->g_FrmYUVBfr[i].imgY)
        {
            vo->g_FrmYUVBfr[i].bDisp = 0;
            break;
        }
    }

    if(DEC_YUV_BUFFER_NUM == i)
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
