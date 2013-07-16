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

PUBLIC MMDecRet Mp4Dec_InitYUVBfr(Mp4DecObject *vd)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
    int32 id;
    int32 size_y, size_c;
    int32 ext_size_y, ext_size_c;
    DEC_FRM_BFR *pDecFrame;
    int32 need_malloc_decY = 0, need_malloc_decYUV = 0, need_malloc_dispY = 0;

#ifdef _SIMULATION_
    need_malloc_decY = 1;
    need_malloc_decYUV = 1;
#else
    need_malloc_decYUV = 1;
#endif

    size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
    size_c = size_y >> 2;

    ext_size_y = vop_mode_ptr->FrameExtendWidth * vop_mode_ptr->FrameExtendHeigth;
    ext_size_c = ext_size_y >> 2;

    memset(vd->g_FrmYUVBfr,0,sizeof(vd->g_FrmYUVBfr));
    pDecFrame = vd->g_FrmYUVBfr;
    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        pDecFrame->id = id;
        pDecFrame->bRef = FALSE;
        pDecFrame->bDisp = FALSE;
        pDecFrame->need_extend = FALSE;

        if (need_malloc_decY)
        {
            pDecFrame->imgY = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)size_y * sizeof(uint8), 256);  //y
            pDecFrame->imgU = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)size_c * sizeof(uint8), 256);  //u
            pDecFrame->imgV = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)size_c * sizeof(uint8), 256);  //v

            if( (NULL == pDecFrame->imgY) || (NULL == pDecFrame->imgU) || (NULL == pDecFrame->imgV))
            {
                return MMDEC_MEMORY_ERROR;
            }
        }

        if (need_malloc_decYUV)
        {
            pDecFrame->imgYUV[0] = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)ext_size_y * sizeof(uint8), 256);  //y
            pDecFrame->imgYUV[1] = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)ext_size_c * sizeof(uint8), 256);  //u
            pDecFrame->imgYUV[2] = (uint8 *)Mp4Dec_ExtraMemAlloc(vd, (uint32)ext_size_c * sizeof(uint8), 256);  //v

            if( NULL == pDecFrame->imgYUV[0] ||(NULL == pDecFrame->imgYUV[1]) || (NULL == pDecFrame->imgYUV[2]))
            {
                return MMDEC_MEMORY_ERROR;
            }
        }

        pDecFrame++;
    }

    return MMDEC_OK;
}

/**********************************************************
check is there free frame buffer, if has, return the frame buffer ID
************************************************************/
/*find a buffer which has been displayed and will no longer be a reference frame from g_dec_frame,
and set the buffer to be reference frame, if current frame picture type is not B frame */
static BOOLEAN Mp4Dec_GetOneFreeDecBfr(Mp4DecObject *vd, int32 *bfrId)
{
    int id;

    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
    {
        if((!vd->g_FrmYUVBfr[id].bRef) && (!vd->g_FrmYUVBfr[id].bDisp))
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

static BOOLEAN Mp4Dec_GetOneFreeDispBfr(Mp4DecObject *vd, int32 *bfrId)
{
    int id;

    for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
    {
        if((!vd->g_DispFrmYUVBfr[id].bRef) && (!vd->g_DispFrmYUVBfr[id].bDisp))
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
BOOLEAN Mp4Dec_GetCurRecFrameBfr(Mp4DecObject *vd)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
    int32 bfrId;
    DEC_FRM_BFR *pDecFrame;
    int32 size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

    while(!Mp4Dec_GetOneFreeDecBfr(vd, &bfrId))
    {
        //wait for display free buffer
        SCI_TRACE_LOW("%s: no buffer is available!\n", __FUNCTION__);
        return FALSE;
    }

    vop_mode_ptr->pCurRecFrame->bfrId = bfrId;
    pDecFrame = vop_mode_ptr->pCurRecFrame->pDecFrame = &vd->g_FrmYUVBfr[bfrId];
    pDecFrame->pBufferHeader= vd->g_rec_buf.pBufferHeader;
    pDecFrame->imgY = vd->g_rec_buf.imgY;
    pDecFrame->imgU = pDecFrame->imgY + size_y; //g_rec_buf.imgU;
    pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);//g_rec_buf.imgV;

    return TRUE;
}

/****************************************************
input: the address of Y frame to be released which has been displayed
output: set the corresponding frame's display type to 0
****************************************************/
MMDecRet MPEG4_DecReleaseDispBfr(Mp4DecObject *vd, uint8 *pBfrAddr)
{
    int i;
    DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

    SCI_ASSERT(pBfrAddr != NULL);

    for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
    {
        if(pBfrAddr == vd->g_FrmYUVBfr[i].imgY)
        {
            vd->g_FrmYUVBfr[i].bDisp = 0;
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
