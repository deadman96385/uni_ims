
#include "sc8810_video_header.h"
//#include "video_common.h"
#include "vp8_mode.h"

void vp8_update_mode_info_border(MODE_INFO *mi, int rows, int cols)
{
    int i;
    vpx_memset(mi - cols - 2, 0, sizeof(MODE_INFO) * (cols + 1));

    for (i = 0; i < rows; i++)
    {
        //vpx_memset(&mi[i*cols-1], 0, sizeof(MODE_INFO));
		vpx_memset(&mi[i*(cols+1)-1], 0, sizeof(MODE_INFO));	// stride is (cols+1) ?
    }
}
void vp8_de_alloc_frame_buffers(VP8_COMMON *oci)
{
//    vp8_yv12_de_alloc_frame_buffer(&oci->temp_scale_frame);
	if(oci->ref_count[oci->new_frame.addr_idx] > 0)
	{
		if(oci->ref_count[oci->new_frame.addr_idx] == 1)
			vp8_yv12_de_alloc_frame_buffer(&oci->new_frame);
		else
			oci->ref_count[oci->new_frame.addr_idx] --;
	}
	if(oci->ref_count[oci->last_frame.addr_idx] > 0)
	{
		if(oci->ref_count[oci->last_frame.addr_idx] == 1)
			vp8_yv12_de_alloc_frame_buffer(&oci->last_frame);
		else
			oci->ref_count[oci->last_frame.addr_idx] --;
	}
	if(oci->ref_count[oci->golden_frame.addr_idx] > 0)
	{
		if(oci->ref_count[oci->golden_frame.addr_idx] == 1)
			vp8_yv12_de_alloc_frame_buffer(&oci->golden_frame);
		else
			oci->ref_count[oci->golden_frame.addr_idx] --;
	}
	if(oci->ref_count[oci->alt_ref_frame.addr_idx] > 0)
	{
		if(oci->ref_count[oci->alt_ref_frame.addr_idx] == 1)
			vp8_yv12_de_alloc_frame_buffer(&oci->alt_ref_frame);
		else
			oci->ref_count[oci->alt_ref_frame.addr_idx] --;
	}
//    vp8_yv12_de_alloc_frame_buffer(&oci->post_proc_buffer);

	while(oci->buffer_count != 0)
	{
	//	vp8dec_ExtraMemFree(oci->new_frame.frame_size + (oci->new_frame.y_stride * 2) + 32);
		oci->buffer_count--;
	}

#ifdef SIM_IN_WIN
	if(oci->mip !=0)
		vp8dec_ExtraMemFree((oci->mb_cols + 1) * (oci->mb_rows + 1) * sizeof(MODE_INFO));
	if(oci->above_context[Y1CONTEXT] != 0)
		vp8dec_ExtraMemFree(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 4);
	if(oci->above_context[UCONTEXT] != 0)
		vp8dec_ExtraMemFree(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
	if(oci->above_context[VCONTEXT] != 0)
		vp8dec_ExtraMemFree(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
	if(oci->above_context[Y2CONTEXT] != 0)
		vp8dec_ExtraMemFree(sizeof(ENTROPY_CONTEXT) * oci->mb_cols);
	//vpx_free(oci->mip);
    //vpx_free(oci->above_context[Y1CONTEXT]);
	//vpx_free(oci->above_context[UCONTEXT]);
	//vpx_free(oci->above_context[VCONTEXT]);
	//vpx_free(oci->above_context[Y2CONTEXT]);

    oci->above_context[Y1CONTEXT] = 0;
    oci->above_context[UCONTEXT]  = 0;
    oci->above_context[VCONTEXT]  = 0;
    oci->above_context[Y2CONTEXT] = 0;
    oci->mip = 0;
#endif
	oci->buffer_count = 0;

#ifdef SIM_IN_WIN
    // Structure used to minitor GF useage
    //if (oci->gf_active_flags != 0)
    //    vpx_free(oci->gf_active_flags);
#endif
    //oci->gf_active_flags = 0;
}

int vp8_alloc_frame_buffers(VP8_COMMON *oci, int width, int height)
{
    vp8_de_alloc_frame_buffers(oci);
	if ( oci->FRAME_ADDR_4 != 0)
	{
	//	vp8dec_ExtraMemFree(8160*9*8+64);
		oci->FRAME_ADDR_4 = 0;
	}

    // our internal buffers are always multiples of 16
    if ((width & 0xf) != 0)
        width += 16 - (width & 0xf);

    if ((height & 0xf) != 0)
        height += 16 - (height & 0xf);

#ifdef SIM_IN_WIN
	if(oci->y_rec != 0)
		vp8dec_ExtraMemFree(oci->last_frame.y_width*oci->last_frame.y_height);
	if(oci->uv_rec != 0)
		vp8dec_ExtraMemFree(oci->last_frame.uv_width*oci->last_frame.uv_height*2);
	oci->y_rec = vp8dec_ExtraMemAlloc(width*height);
	oci->uv_rec = vp8dec_ExtraMemAlloc(((width+1)>>1)*((height+1)>>1)*2);
#endif

    /*if (vp8_yv12_alloc_frame_buffer(&oci->temp_scale_frame,   width, 16, VP8BORDERINPIXELS) < 0)
    {
        vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }*/

	// FRAME_ADDR_4 for HW, 8160*9*64 + 512 bits
	oci->FRAME_ADDR_4 = vp8dec_ExtraMemAlloc(8160*9*8+64);
	if ( oci->FRAME_ADDR_4 == 0)
        return ALLOC_FAILURE;

    if (vp8_yv12_alloc_frame_buffer(&oci->new_frame, width, height, VP8BORDERINPIXELS, 0) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
	oci->ref_count[0] = 1;

    if (vp8_yv12_alloc_frame_buffer(&oci->last_frame, width, height, VP8BORDERINPIXELS, 1) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
	oci->ref_count[1] = 1;

    if (vp8_yv12_alloc_frame_buffer(&oci->golden_frame, width, height, VP8BORDERINPIXELS, 2) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
	oci->ref_count[2] = 1;

    if (vp8_yv12_alloc_frame_buffer(&oci->alt_ref_frame, width, height, VP8BORDERINPIXELS, 3) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
	oci->ref_count[3] = 1;

    /*if (vp8_yv12_alloc_frame_buffer(&oci->post_proc_buffer, width, height, VP8BORDERINPIXELS) < 0)
    {
        vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }*/

    oci->mb_rows = height >> 4;
    oci->mb_cols = width >> 4;
    oci->MBs = oci->mb_rows * oci->mb_cols;
#ifdef SIM_IN_WIN
	oci->mode_info_stride = oci->mb_cols + 1;
	oci->mip = (MODE_INFO*)vp8dec_ExtraMemAlloc((oci->mb_cols + 1) * (oci->mb_rows + 1) * sizeof(MODE_INFO));
	memset(oci->mip, 0, (oci->mb_cols + 1) * (oci->mb_rows + 1) * sizeof(MODE_INFO));
    //oci->mip = vpx_calloc((oci->mb_cols + 1) * (oci->mb_rows + 1), sizeof(MODE_INFO));

    if (!oci->mip)
    {
        //vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

    oci->mi = oci->mip + oci->mode_info_stride + 1;

	oci->above_context[Y1CONTEXT] = (ENTROPY_CONTEXT *)vp8dec_ExtraMemAlloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 4);
	memset(oci->above_context[Y1CONTEXT], 0, sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 4);
    //oci->above_context[Y1CONTEXT] = vpx_calloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 4 , 1);

    if (!oci->above_context[Y1CONTEXT])
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

	oci->above_context[UCONTEXT]  = (ENTROPY_CONTEXT *)vp8dec_ExtraMemAlloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
	memset(oci->above_context[UCONTEXT], 0, sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
    //oci->above_context[UCONTEXT]  = vpx_calloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2 , 1);

    if (!oci->above_context[UCONTEXT])
    {
        //vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

	oci->above_context[VCONTEXT]  = (ENTROPY_CONTEXT *)vp8dec_ExtraMemAlloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
	memset(oci->above_context[VCONTEXT], 0, sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2);
    //oci->above_context[VCONTEXT]  = vpx_calloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols * 2 , 1);

    if (!oci->above_context[VCONTEXT])
    {
        //vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

	oci->above_context[Y2CONTEXT] = (ENTROPY_CONTEXT *)vp8dec_ExtraMemAlloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols);
	memset(oci->above_context[Y2CONTEXT], 0, sizeof(ENTROPY_CONTEXT) * oci->mb_cols);
    //oci->above_context[Y2CONTEXT] = vpx_calloc(sizeof(ENTROPY_CONTEXT) * oci->mb_cols     , 1);

    if (!oci->above_context[Y2CONTEXT])
    {
        //vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

    vp8_update_mode_info_border(oci->mi, oci->mb_rows, oci->mb_cols);
#endif

#ifdef SIM_IN_WIN
    // Structures used to monitor GF usage
    //if (oci->gf_active_flags != 0)
    //    vpx_free(oci->gf_active_flags);
#endif

	/*oci->gf_active_flags = (unsigned char *)vp8dec_ExtraMemAlloc(oci->mb_rows * oci->mb_cols);
	memset(oci->gf_active_flags, 0, oci->mb_rows * oci->mb_cols);
    //oci->gf_active_flags = (unsigned char *)vpx_calloc(oci->mb_rows * oci->mb_cols, 1);

    if (!oci->gf_active_flags)
    {
        vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }

    oci->gf_active_count = oci->mb_rows * oci->mb_cols;*/

    return 0;
}

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
	
//	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "VP8 mc mode");
}

void vp8_remove_common(VP8_COMMON *oci)
{
    vp8_de_alloc_frame_buffers(oci);
}


int vp8_init_frame_buffers(VP8_COMMON *oci)
{
	int width = oci->Width;
	int height = oci->Height;

    // our internal buffers are always multiples of 16
    if ((width & 0xf) != 0)
        width += 16 - (width & 0xf);

    if ((height & 0xf) != 0)
        height += 16 - (height & 0xf);

	// FRAME_ADDR_4 for HW, 8160*9*64 + 512 bits
	if ( oci->FRAME_ADDR_4 == 0)
	oci->FRAME_ADDR_4 = vp8dec_InterMemAlloc(8160*9*8+64);
	
	if ( oci->FRAME_ADDR_4 == 0)
        return ALLOC_FAILURE;

    if (vp8_yv12_init_frame_buffer(&oci->new_frame, width, height, VP8BORDERINPIXELS, 0) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
//	oci->ref_count[0] = 0;

    if (vp8_yv12_init_frame_buffer(&oci->last_frame, width, height, VP8BORDERINPIXELS, 1) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
//	oci->ref_count[1] = 0;

    if (vp8_yv12_init_frame_buffer(&oci->golden_frame, width, height, VP8BORDERINPIXELS, 2) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
//	oci->ref_count[2] = 0;

    if (vp8_yv12_init_frame_buffer(&oci->alt_ref_frame, width, height, VP8BORDERINPIXELS, 3) < 0)
    {
		//vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }
//	oci->ref_count[3] = 0;

    /*if (vp8_yv12_alloc_frame_buffer(&oci->post_proc_buffer, width, height, VP8BORDERINPIXELS) < 0)
    {
        vp8_de_alloc_frame_buffers(oci);
        return ALLOC_FAILURE;
    }*/

    oci->mb_rows = height >> 4;
    oci->mb_cols = width >> 4;
    oci->MBs = oci->mb_rows * oci->mb_cols;

    return 0;
}

