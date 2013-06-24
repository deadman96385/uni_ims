#include "video_common.h"
#include "vpx_codec.h"
#include "vp8_init.h"
#include "vp8_swap_yv12buffer.h"
#include "vp8_yv12extend.h"
#include "vp8dec_basic.h"
#include "vp8dec_mode.h"
#include "vp8dec_malloc.h"
#include "vp8dec_dequant.h"//weihu
#include "sc8810_video_header.h"//weihu

#ifdef SIM_IN_WIN
#include "vp8_segmentation_common.h"
#include "common_global.h"//weihu
#include "vp8dbk_trace.h"//weihu
#include "vp8dbk_global.h"
#include "vp8_test_vectors.h"	//derek
#endif

extern int vp8_decode_frame(VP8D_COMP *pbi);//weihu

#ifdef SIM_IN_WIN
void vp8dx_initialize()
{
    static int init_done = 0;

    if (!init_done)
    {
		InitVp8DBKTrace();
        init_done = 1;
    }
}

LOCAL void write_out_frame(VP8_COMMON *common, FILE *fp, int input_width, int input_height)
{
    unsigned char *src = common->frame_to_show->y_buffer;
	int out_width;
	int out_height;
	
	//	input_width = (input_width + 15)&0xfffffff0;
	//	input_height = (input_height + 15)&0xfffffff0;
	
	if (input_width & 0xffff)
		out_width = input_width;
	else
		out_width = common->frame_to_show->y_width;
	
	if (input_height & 0xffff)
		out_height = input_height;
	else
		out_height = common->frame_to_show->y_height;

    do
    {
        fwrite(src, out_width, 1,  fp);
        src += common->frame_to_show->y_stride;
    }
    while (--out_height);
	
    src = common->frame_to_show->u_buffer;
	
	if (input_width & 0xffff)
		out_width = (input_width+1)>>1;
	else
		out_width = common->frame_to_show->uv_width;
	
	if (input_height & 0xffff)
		out_height = (input_height+1)>>1;
	else
		out_height = common->frame_to_show->uv_height;
	
    do
    {
        fwrite(src, out_width, 1,  fp);
        src += common->frame_to_show->uv_stride;
    }
    while (--out_height);
	
    src = common->frame_to_show->v_buffer;
	
	if (input_height & 0xffff)
		out_height = (input_height+1)>>1;
	else
		out_height = common->frame_to_show->uv_height;
	
    do
    {
        fwrite(src, out_width, 1, fp);
        src += common->frame_to_show->uv_stride;
    }
    while (--out_height);
    
}
#endif

VP8D_PTR vp8dx_create_decompressor(VP8D_CONFIG *oxcf)
{
    VP8D_COMP *pbi = vp8dec_InterMemAlloc(sizeof(VP8D_COMP));

    if (!pbi)
        return NULL;

//    vpx_memset(pbi, 0, sizeof(VP8D_COMP));
	vpx_memset(&pbi->mb, 0, sizeof(MACROBLOCKD));
	pbi->common.new_frame.buffer_alloc = 0;
	pbi->common.new_frame.addr_idx = 0;
	pbi->common.new_frame.pBufferHeader = NULL;
	pbi->common.last_frame.buffer_alloc = 0;
	pbi->common.last_frame.addr_idx = 0;
	pbi->common.last_frame.pBufferHeader = NULL;
	pbi->common.golden_frame.buffer_alloc = 0;
	pbi->common.golden_frame.addr_idx = 0;
	pbi->common.golden_frame.pBufferHeader = NULL;
	pbi->common.alt_ref_frame.buffer_alloc = 0;
	pbi->common.alt_ref_frame.addr_idx = 0;
	pbi->common.alt_ref_frame.pBufferHeader = NULL;
	pbi->common.FRAME_ADDR_4 = 0;
	pbi->common.ref_count[0] = 0;
	pbi->common.ref_count[1] = 0;
	pbi->common.ref_count[2] = 0;
	pbi->common.ref_count[3] = 0;

	pbi->common.buffer_pool [0] =NULL;
	pbi->common.buffer_pool [1] =NULL;
	pbi->common.buffer_pool [2] =NULL;
	pbi->common.buffer_pool [3] =NULL;

#ifdef SIM_IN_WIN
	pbi->common.above_context[Y1CONTEXT] = 0;
    pbi->common.above_context[UCONTEXT]  = 0;
    pbi->common.above_context[VCONTEXT]  = 0;
    pbi->common.above_context[Y2CONTEXT] = 0;
    pbi->common.mip = 0;
	pbi->common.y_rec = 0;
	pbi->common.uv_rec = 0;
#endif
	pbi->common.refresh_golden_frame = 0;
	pbi->common.refresh_alt_ref_frame = 0;
//	pbi->common.copy_buffer_to_gf = 0;
//	pbi->common.copy_buffer_to_arf = 0;
	pbi->common.y1dc_delta_q = 0;
	pbi->common.y2dc_delta_q = 0;
	pbi->common.uvdc_delta_q = 0;
	pbi->common.y2ac_delta_q = 0;
	pbi->common.uvac_delta_q = 0;
	pbi->common.Width = 0;
	pbi->common.Height = 0;

#ifdef SIM_IN_WIN
    vp8dx_initialize();
	VSP_Init_CModel();
#endif
    vp8_create_common(&pbi->common);

    //vp8cx_init_de_quantizer() is first called here. Add check in frame_init_dequantizer() to avoid
    // unnecessary calling of vp8cx_init_de_quantizer() for every frame.
    vp8cx_init_de_quantizer(pbi);
    return (VP8D_PTR) pbi;
}

extern void Get_MD5_Code(uint8 *img, uint32 datalen, uint8 *code);
int vp8dx_receive_compressed_data(VPXHandle *vpxHandle, unsigned long size, const unsigned char *source, int64 time_stamp)
{
    VP8D_COMP *pbi = (VP8D_COMP *) (vpxHandle->videoDecoderData);
    VP8_COMMON *cm = &pbi->common;
    int retcode = 0;

    pbi->Source = source;
    pbi->source_sz = size;

#ifdef MCA_TV_SPLIT
	MCASplitInit();
#endif

	// Find a  place in buffer pool to save pBufferHeader of new picture.
	{
		int buffer_index;
		for(buffer_index = 0; buffer_index <4; buffer_index ++)
		{
			if(cm->ref_count[buffer_index] == 0)
			{
				cm->buffer_pool[buffer_index] = cm->new_frame.pBufferHeader;
				break;
			}
		}

		if(buffer_index == 4)
			return -1;
	
	}
	
    retcode = vp8_decode_frame(pbi);

    if (retcode < 0)
    {
        return retcode;
    }

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
	{
#ifdef MCA_TV_SPLIT
		Print_REF_Frame(&cm->last_frame, g_fp_ref_frm0_tv);
		Print_REF_Frame(&cm->golden_frame, g_fp_ref_frm0_tv);
		Print_REF_Frame(&cm->alt_ref_frame, g_fp_ref_frm0_tv);
#else
//		Print_REF_Frame(&cm->last_frame, g_fp_ref_frm0_tv);
//		Print_REF_Frame(&cm->golden_frame, g_fp_ref_frm1_tv);
//		Print_REF_Frame(&cm->alt_ref_frame, g_fp_ref_frm2_tv);
#endif
	}
#endif


	// If any buffer copy / swapping is signaled it should be done here.
    if (cm->copy_buffer_to_arf != 0)
    {
        if (cm->copy_buffer_to_arf == 1)
			vp8_copy_yv12_buffer(vpxHandle,cm, &cm->last_frame, &cm->alt_ref_frame);
			//vp8_yv12_copy_frame(&cm->last_frame, &cm->alt_ref_frame);
        else if (cm->copy_buffer_to_arf == 2)
			vp8_copy_yv12_buffer(vpxHandle,cm, &cm->golden_frame, &cm->alt_ref_frame);
            //vp8_yv12_copy_frame(&cm->golden_frame, &cm->alt_ref_frame);
    }
	
    if (cm->copy_buffer_to_gf != 0)
    {
        if (cm->copy_buffer_to_gf == 1)
			vp8_copy_yv12_buffer(vpxHandle,cm, &cm->last_frame, &cm->golden_frame);
			//vp8_yv12_copy_frame(&cm->last_frame, &cm->golden_frame);
        else if (cm->copy_buffer_to_gf == 2)
			vp8_copy_yv12_buffer(vpxHandle,cm, &cm->alt_ref_frame, &cm->golden_frame);
            //vp8_yv12_copy_frame(&cm->alt_ref_frame, &cm->golden_frame);
    }
#if 0
    if (cm->refresh_last_frame == 1)
    {
        vp8_swap_yv12_buffer(&cm->last_frame, &cm->new_frame);
        cm->frame_to_show = &cm->last_frame;
    }
    else
    {
        cm->frame_to_show = &cm->new_frame;
    }
#endif

#ifdef SIM_IN_WIN
//	if (!pbi->b_multithreaded_lf)
    {
//        struct vpx_usec_timer lpftimer;
//        vpx_usec_timer_start(&lpftimer);
        // Apply the loop filter if appropriate.

        //if (cm->filter_level > 0)
        {
			vp8_dbk_frame(cm, &pbi->mb, cm->filter_level);
        }
    }
#endif // SIM_IN_WIN

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_dec==FRAME_X)
#endif
		Print_DBK_Frame(cm->frame_to_show);
#endif

#ifdef SIM_IN_WIN
    vp8_yv12_extend_frame_borders(cm->frame_to_show);
#endif

    // If any buffer copy / swapping is signaled it should be done here.
    /*if (cm->copy_buffer_to_arf)
    {
        if (cm->copy_buffer_to_arf == 1)
        {
            if (cm->refresh_last_frame)
                vp8_yv12_copy_frame(&cm->new_frame, &cm->alt_ref_frame);
            else
                vp8_yv12_copy_frame(&cm->last_frame, &cm->alt_ref_frame);
        }
        else if (cm->copy_buffer_to_arf == 2)
            vp8_yv12_copy_frame(&cm->golden_frame, &cm->alt_ref_frame);
    }

    if (cm->copy_buffer_to_gf)
    {
        if (cm->copy_buffer_to_gf == 1)
        {
            if (cm->refresh_last_frame)
                vp8_yv12_copy_frame(&cm->new_frame, &cm->golden_frame);
            else
                vp8_yv12_copy_frame(&cm->last_frame, &cm->golden_frame);
        }
        else if (cm->copy_buffer_to_gf == 2)
            vp8_yv12_copy_frame(&cm->alt_ref_frame, &cm->golden_frame);
    }*/

	// Should the golden or alternate reference frame be refreshed?
    if (cm->refresh_golden_frame == 1)
		vp8_copy_yv12_buffer(vpxHandle,cm, &cm->new_frame, &cm->golden_frame);
		//vp8_copy_yv12_buffer(cm, cm->frame_to_show, &cm->golden_frame);
		//vp8_yv12_copy_frame(cm->frame_to_show, &cm->golden_frame);
	
    if (cm->refresh_alt_ref_frame == 1)
		vp8_copy_yv12_buffer(vpxHandle,cm, &cm->new_frame, &cm->alt_ref_frame);
        //vp8_yv12_copy_frame(cm->frame_to_show, &cm->alt_ref_frame);

		
    if (cm->refresh_last_frame == 1)
    {
    	vp8_copy_yv12_buffer(vpxHandle,cm, &cm->new_frame, &cm->last_frame);
   //     vp8_swap_yv12_buffer(&cm->last_frame, &cm->new_frame);
        cm->frame_to_show = &cm->last_frame;
    }
    else
    {
        cm->frame_to_show = &cm->new_frame;
    }
#ifdef SIM_IN_WIN
#ifndef _LIB
	if(cm->show_frame)
		write_out_frame(cm, s_pDec_recon_yuv_file, cm->Width, cm->Height);
#endif
#endif

#if 0
	if(cm->refresh_alt_ref_frame || cm->refresh_golden_frame || cm->refresh_last_frame)
	{
		if(cm->new_frame.pBufferHeader != NULL)
			{
				OR_VSP_BIND(cm->new_frame.pBufferHeader);
			}
	}
#endif
	
//	vp8_check_yv12_buffer(cm, &cm->new_frame);	// Must check here, derek

	// Compute MD5
#ifdef SIM_IN_WIN
	// copy frame, cause buffer has borders
	{
		int32 i, j;
		for(i=0; i<cm->frame_to_show->y_height; i++)
		{
			memcpy(&cm->y_rec[i*cm->frame_to_show->y_width], &cm->frame_to_show->y_buffer[i*cm->frame_to_show->y_stride], cm->frame_to_show->y_width);
		}
		for(j=0; j<cm->frame_to_show->uv_height; j++)
		{
			for(i=0; i<cm->frame_to_show->uv_width; i++)
			{
				cm->uv_rec[2*i+j*cm->frame_to_show->y_width] = cm->frame_to_show->u_buffer[i+j*cm->frame_to_show->uv_stride];
				cm->uv_rec[2*i+1+j*cm->frame_to_show->y_width] = cm->frame_to_show->v_buffer[i+j*cm->frame_to_show->uv_stride];
			}
		}
		//Get_MD5_Code(cm->y_rec, cm->frame_to_show->y_width*cm->frame_to_show->y_height, cm->ycode);
		//Get_MD5_Code(cm->uv_rec, cm->frame_to_show->uv_width*cm->frame_to_show->uv_height*2, cm->uvcode);
		/*{
			FILE *fp_md5 = fopen("md5_result.txt", "a");
			fprintf(fp_md5, "%016I64x", *((__int64*)&cm->ycode[8]));
			fprintf(fp_md5, "%016I64x\n", *((__int64*)&cm->ycode[0]));
			fprintf(fp_md5, "%016I64x", *((__int64*)&cm->uvcode[8]));
			fprintf(fp_md5, "%016I64x\n", *((__int64*)&cm->uvcode[0]));
			fclose(fp_md5);
		}*/
	}
#else
	//Get_MD5_Code(cm->frame_to_show->y_buffer, cm->frame_to_show->y_width*cm->frame_to_show->y_height, cm->ycode);
	//Get_MD5_Code(cm->frame_to_show->u_buffer, cm->frame_to_show->uv_width*cm->frame_to_show->uv_height*2, cm->uvcode);
#ifdef FPGA_AUTO_VERIFICATION
	{
		uint32 tmp;
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x4c, 1, "ORSC_SHARE: Output_en = 1");//weihu
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x2c, cm->ycode+or_addr_offset, "ORSC_SHARE: Frame_Y_ADDR");
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x30, cm->uvcode+or_addr_offset, "ORSC_SHARE: Frame_UV_ADDR");
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x2c, (uint32)cm->frame_to_show->y_buffer+or_addr_offset, "ORSC_SHARE: Frame_Y_ADDR");
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x30, (uint32)cm->frame_to_show->u_buffer+or_addr_offset, "ORSC_SHARE: Frame_UV_ADDR");
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x34, cm->frame_to_show->uv_width*cm->frame_to_show->uv_height*2, "ORSC_SHARE: UV_FRAME_BUF_SIZE");//weihu
		//tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
		//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x00, tmp&0x7fffffff, "ORSC_SHARE: MODE_CFG Stop");
		//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");
		//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
	}
#endif
#endif

    // Update data structures that monitors GF useage
    //vpx_memset(cm->gf_active_flags, 1, (cm->mb_rows * cm->mb_cols));
    //cm->gf_active_count = cm->mb_rows * cm->mb_cols;

#ifdef MCA_TV_SPLIT
	MCASplitDeinit();
#endif
    return retcode;
}


PUBLIC void OR_VSP_BIND(VPXHandle *vpxHandle, void *pHeader)
{
	 (*vpxHandle->VSP_bindCb)(vpxHandle->userdata,(void *)pHeader, 0);
}

PUBLIC void OR_VSP_UNBIND(VPXHandle *vpxHandle, void *pHeader)
{
	(*vpxHandle->VSP_unbindCb)(vpxHandle->userdata,(void *)pHeader, 0);
}



PUBLIC void VP8DecSetCurRecPic(VPXHandle *vpxHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	VP8D_COMP *pbi = (VP8D_COMP *)(vpxHandle->videoDecoderData) ;
    	VP8_COMMON *cm = &pbi->common;
	 YV12_BUFFER_CONFIG *rec_frame = &cm->new_frame;

        rec_frame->buffer_alloc =   pFrameY_phy;		
        rec_frame->y_buffer = rec_frame->buffer_alloc ;

	rec_frame->y_buffer_virtual = (uint32)pFrameY;	
	
	rec_frame->pBufferHeader = pBufferHeader;
	
}


MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer * buffer_ptr)
{
#ifndef _FPGA_TEST_
	// Open VSP device
	if(VSP_OPEN_Dev()<0)
	{
		return MMDEC_HW_ERROR;
	}	
#else
	TEST_VSP_ENABLE();	
#endif

	vp8dec_InitInterMem (buffer_ptr);


	g_fh_reg_ptr = (VSP_FH_REG_T *)vp8dec_InterMemAlloc(sizeof(VSP_FH_REG_T));
				
	vpxHandle->videoDecoderData= vp8dx_create_decompressor(/*&oxcf*/NULL);
	
	return MMDEC_OK;
}


PUBLIC MMDecRet VP8DecDecode(VPXHandle *vpxHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{

	MMDecRet ret;
	uint32 bs_buffer_length, bs_start_addr;
	VP8D_COMP *pbi = (VP8D_COMP *) (vpxHandle->videoDecoderData);
    	VP8_COMMON *cm = &pbi->common;


	  if(ARM_VSP_RST()<0)
	 {
		return MMDEC_HW_ERROR;
	  }

	  SCI_TRACE_LOW("%s, %d",__FUNCTION__, __LINE__);
	  SCI_TRACE_LOW("pBufferHeader %x,pOutFrameY %x,frame_width %d,frame_height %d", dec_output_ptr->pBufferHeader, dec_output_ptr->pOutFrameY, dec_output_ptr->frame_width,dec_output_ptr->frame_height );

	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF,V_BIT_6 | STREAM_ID_VP8,"VSP_MODE");

	  // Bitstream.
   	 bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
  	 bs_buffer_length=dec_input_ptr->dataLen;
  	 g_stream_offset=0;


	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, /*(g_stream_offset)*/0,"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.


	ret = vp8dx_receive_compressed_data(vpxHandle,  bs_buffer_length,(uint8 *)dec_input_ptr->pStream , 0);

	SCI_TRACE_LOW("%s, %d, ret is %d",__FUNCTION__, __LINE__, ret);


	dec_output_ptr->frameEffective = (cm->show_frame && (ret ==MMDEC_OK ) );
	dec_output_ptr->frame_width =  (((cm->Width+ 15)>>4)<<4);
	dec_output_ptr->frame_height = (((cm->Height+ 15)>>4)<<4);

	SCI_TRACE_LOW("%s, %d, frameEffective is %d,frame_width %d,frame_height %d",__FUNCTION__, __LINE__, dec_output_ptr->frameEffective, dec_output_ptr->frame_width,dec_output_ptr->frame_height);
	if(dec_output_ptr->frameEffective)
	{
	        dec_output_ptr->pBufferHeader = (void *)(cm->frame_to_show->pBufferHeader);
		dec_output_ptr->pOutFrameY = (uint8 *)(cm->frame_to_show->y_buffer_virtual);
		dec_output_ptr->pOutFrameU = (uint8 *)(cm->frame_to_show->u_buffer_virtual);
	}

	SCI_TRACE_LOW("pBufferHeader %x,pOutFrameY %x", dec_output_ptr->pBufferHeader, dec_output_ptr->pOutFrameY );

	 VSP_RELEASE_Dev();

 	// Return output.		
	return ret;
}



MMDecRet VP8DecRelease(VPXHandle *vpxHandle)
{
#ifndef _FPGA_TEST_
	VSP_CLOSE_Dev();
#endif
	return MMDEC_OK;
}



