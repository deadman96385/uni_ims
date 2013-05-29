
#include "sc8810_video_header.h"
//#include "video_common.h"
#include "vp8_yv12config.h"

int
vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf)
{
    if (ybf != 0)
    {
#ifdef SIM_IN_WIN
        if (ybf->buffer_alloc)
        {
			vp8dec_ExtraMemFree(ybf->frame_size + (ybf->y_stride * 2) + 32);
            //vpx_free(ybf->buffer_alloc);
        }
#endif
        ybf->buffer_alloc = 0;
		ybf->addr_idx = 0;
    }
    else
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 *
 ****************************************************************************/
int
vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border_tmp, int addr_idx)
{
//NOTE:
#ifdef SIM_IN_WIN
	int border = border_tmp;
#else
	int border = 0;	// Padding is considered in HW
#endif
    int yplane_size = (height + 2 * border) * (width + 2 * border);
    int uvplane_size = ((1 + height) / 2 + border) * ((1 + width) / 2 + border);

    if (ybf != 0)
    {
        //vp8_yv12_de_alloc_frame_buffer(ybf);

		ybf->addr_idx = addr_idx;
        ybf->y_width  = width;
        ybf->y_height = height;
        ybf->y_stride = width + 2 * border;

        ybf->uv_width = (1 + width)>>1;
        ybf->uv_height = (1 + height)>>1;
        ybf->uv_stride = ybf->uv_width + border;

        ybf->border = border;
        ybf->frame_size = yplane_size + 2 * uvplane_size;

        // Added 2 extra lines to framebuffer so that copy12x12 doesn't fail
        // when we have a large motion vector in V on the last v block.
        // Note : We never use these pixels anyway so this doesn't hurt.
		ybf->buffer_alloc = (unsigned char *)vp8dec_ExtraMemAlloc(ybf->frame_size + (ybf->y_stride * 2) + 32);
//		vpx_memset(ybf->buffer_alloc, 0, ybf->frame_size + (ybf->y_stride * 2) + 32);
		//ybf->buffer_alloc = (unsigned char *)malloc(ybf->frame_size + (ybf->y_stride * 2) + 32); //duck_memalign(32,  ybf->frame_size + (ybf->y_stride * 2) + 32, 0);

        if (ybf->buffer_alloc == NULL)
            return -1;

        ybf->y_buffer = ybf->buffer_alloc + (border * ybf->y_stride) + border;

        if ((yplane_size & 0xf) !=0)
            yplane_size += 16 - (yplane_size & 0xf);

        ybf->u_buffer = ybf->buffer_alloc + yplane_size + ((border>>1)  * ybf->uv_stride) + (border>>1);
        ybf->v_buffer = ybf->buffer_alloc + yplane_size + uvplane_size + ((border>>1)  * ybf->uv_stride) + (border>>1);
    }
    else
    {
        return -2;
    }

    return 0;
}


/****************************************************************************
 *
 ****************************************************************************/
int
vp8_yv12_init_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border_tmp, int addr_idx)
{

    int yplane_size = (height ) * (width );
    int uvplane_size = yplane_size >>2;

    if (ybf != 0)
    {
       

//	ybf->addr_idx = addr_idx;
        ybf->y_width  = width;
        ybf->y_height = height;
        ybf->y_stride = width;

        ybf->uv_width = ( width)>>1;
        ybf->uv_height = ( height)>>1;
        ybf->uv_stride = ybf->uv_width ;

        ybf->border = 0;
        ybf->frame_size = yplane_size + 2 * uvplane_size;


    }
    else
    {
        return -2;
    }

    return 0;
}

/****************************************************************************
 *
 ****************************************************************************/
int
vp8_yv12_black_frame_buffer(YV12_BUFFER_CONFIG *ybf)
{
    if (ybf)
    {
        if (ybf->buffer_alloc)
        {
            memset(ybf->y_buffer, 0x0, ybf->y_stride * ybf->y_height);
            memset(ybf->u_buffer, 0x80, ybf->uv_stride * ybf->uv_height);
            memset(ybf->v_buffer, 0x80, ybf->uv_stride * ybf->uv_height);
        }

        return 0;
    }

    return -1;
}
