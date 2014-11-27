/******************************************************************************
 ** File Name:    vp8dec_bfrctrl.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void vp8_setup_version(VP8_COMMON *cm)
{
    //int cmd;
    switch (cm->version)
    {
    case 0:
        cm->no_lpf = 0;
        cm->simpler_lpf = 0;
        cm->use_bilinear_mc_filter = 0;
        cm->full_pixel = 0;
        break;
    case 1:
        cm->no_lpf = 0;
        cm->simpler_lpf = 1;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 0;
        break;
    case 2:
        cm->no_lpf = 1;
        cm->simpler_lpf = 0;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 0;
        break;
    case 3:
        cm->no_lpf = 1;
        cm->simpler_lpf = 1;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 1;
        break;
    default:
        //4,5,6,7 are reserved for future use
        cm->no_lpf = 0;
        cm->simpler_lpf = 0;
        cm->use_bilinear_mc_filter = 0;
        cm->full_pixel = 0;
        break;
    }
}

int32 vp8_yv12_init_frame_buffer(YV12_BUFFER_CONFIG *ybf, int32 width, int32 height)
{
    int32 yplane_size = (height ) * (width );
    int32 uvplane_size = yplane_size >>2;

    if (ybf != 0)
    {
        ybf->y_width  = width;
        ybf->y_height = height;
        ybf->y_stride = width;

        ybf->uv_width = ( width)>>1;
        ybf->uv_height = ( height)>>1;
        ybf->uv_stride = ybf->uv_width ;

        ybf->border = 0;
        ybf->frame_size = yplane_size + 2 * uvplane_size;
    } else
    {
        return -2;
    }

    return 0;
}

void vp8_copy_yv12_buffer(VPXDecObject *vo, VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame, YV12_BUFFER_CONFIG *dst_frame)
{
    int32 buffer_index;

    // Bind
    if(src_frame->pBufferHeader != NULL)
    {
        for(buffer_index = 0; buffer_index < YUV_BUFFER_NUM; buffer_index ++)
        {
            if(cm->buffer_pool[buffer_index] ==  src_frame->pBufferHeader)
            {
                break;
            }
        }

        if(buffer_index < YUV_BUFFER_NUM)
        {
            if(cm->ref_count[buffer_index] == 0)
            {
                (*(vo->vpxHandle->VSP_bindCb))(vo->vpxHandle->userdata,(void *)(src_frame->pBufferHeader), 0);
            }

            cm->ref_count[buffer_index]++;
        }
    }

    //UnBind
    if(dst_frame->pBufferHeader != NULL)
    {
        for(buffer_index = 0; buffer_index < YUV_BUFFER_NUM; buffer_index ++)
        {
            if(cm->buffer_pool[buffer_index] ==  dst_frame->pBufferHeader)
            {
                break;
            }
        }


        if(buffer_index < YUV_BUFFER_NUM)
        {
            cm->ref_count[buffer_index]--;

            if(cm->ref_count[buffer_index] == 0)
            {
                (*(vo->vpxHandle->VSP_unbindCb))(vo->vpxHandle->userdata,(void *)(dst_frame->pBufferHeader), 0);
                cm->buffer_pool[buffer_index] = NULL;
            }
        }
    }

    dst_frame->buffer_alloc = src_frame->buffer_alloc;
    dst_frame->y_buffer = src_frame->y_buffer;
    dst_frame->u_buffer = src_frame->u_buffer;
    dst_frame->v_buffer = src_frame->v_buffer;
    dst_frame->addr_idx = src_frame->addr_idx;

    dst_frame->y_buffer_virtual = src_frame->y_buffer_virtual;
    dst_frame->u_buffer_virtual = src_frame->u_buffer_virtual;

    dst_frame->pBufferHeader = src_frame->pBufferHeader;
}

MMDecRet vp8_init_frame_buffers(VPXDecObject *vo, VP8_COMMON *oci)
{
    int32 width = oci->Width;
    int32 height = oci->Height;

    // our internal buffers are always multiples of 16
    if ((width & 0xf) != 0)
    {
        width += (16 - (width & 0xf));
    }

    if ((height & 0xf) != 0)
    {
        height += (16 - (height & 0xf));
    }

    // FRAME_ADDR_4 for HW, 8160*9*64 + 512 bits
    if (oci->FRAME_ADDR_4 == PNULL)
    {
        oci->FRAME_ADDR_4 = Vp8Dec_MemAlloc(vo, 8160*9*8+64, 8, EXTRA_MEM);
        CHECK_MALLOC(oci->FRAME_ADDR_4, "oci->FRAME_ADDR_4");
    }

    if (vp8_yv12_init_frame_buffer(&oci->new_frame, width, height) < 0)
    {
        return MMENC_MEMORY_ERROR;
    }

    if (vp8_yv12_init_frame_buffer(&oci->last_frame, width, height) < 0)
    {
        return MMENC_MEMORY_ERROR;
    }

    if (vp8_yv12_init_frame_buffer(&oci->golden_frame, width, height) < 0)
    {
        return MMENC_MEMORY_ERROR;
    }

    if (vp8_yv12_init_frame_buffer(&oci->alt_ref_frame, width, height) < 0)
    {
        return MMENC_MEMORY_ERROR;
    }

    oci->mb_rows = height >> 4;
    oci->mb_cols = width >> 4;
    oci->MBs = oci->mb_rows * oci->mb_cols;

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

