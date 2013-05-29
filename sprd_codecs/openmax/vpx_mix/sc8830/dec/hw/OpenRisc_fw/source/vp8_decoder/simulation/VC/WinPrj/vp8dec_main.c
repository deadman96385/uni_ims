/******************************************************************************
 ** File Name:    mp4dec_main.c												  *
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
//#include "sc8800g_video_header.h"
//#if defined(_FPGA_AUTO_VERIFICATION_)
//#include "fpga_auto_vrf_def.h"
//#endif //_FPGA_AUTO_VERIFICATION_
//#include "sci_types.h"
//#include "video_common.h"
//#include "mmcodec.h"
#include "vp8dec_global.h"
#include "vp8dec_basic.h"
#include "vp8dec_mode.h"
#include "sc8810_video_header.h"
#ifdef SIM_IN_WIN
	#include "vp8_test_vectors.h" // derek
#endif

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


#ifdef SIM_IN_WIN
LOCAL int32 get_unit (uint8 *pInStream, MMDecVideoFormat *p_video_format)
{
//	int32 len = 0;
	uint8 *ptr;
//	uint8 data;
//	static int32 declen = 0;
//	int32 zero_num = 0;

	ptr = p_video_format->p_extra = pInStream;

	p_video_format->i_extra = (ptr[3]<<24) | (ptr[2]<<16) | (ptr[1]<<8) | ptr[0];
//	g_stream_offset += p_video_format->i_extra;

	if ( (g_stream_offset+p_video_format->i_extra) >= g_readFileSize)
	{
		return 1;
	}

	ptr += 12;
//	g_stream_offset += 12;
	p_video_format->p_extra = ptr;
#ifdef TV_OUT
	//Print_Stream_Offset( (g_stream_offset-p_video_format->i_extra), "Frame Header Start");
#endif
	return 0;
}
#endif

VP8D_PTR vp8dx_create_decompressor(VP8D_CONFIG *oxcf);
int vp8dx_receive_compressed_data(VP8D_PTR ptr, unsigned long size, const unsigned char *source, int64 time_stamp);

void vp8Dec_main(void)
{
//	MMDecRet ret;
#ifdef SIM_IN_WIN
	uint32 length;
	uint8 *input_bs_bfr_ptr, *bs_ptr;
#endif
	int32 file_is_ivf = 0;
//	MMDecRet decode_return;
	MMDecVideoFormat video_format;
	MMDecVideoFormat *video_format_ptr = &video_format;
//	MMDecInput *dec_input_ptr = NULL;
//	MMDecOutput *dec_output_ptr = NULL;
	MMCodecBuffer *dec_malloc_bfr_ptr = NULL;

//	VP8D_CONFIG oxcf;
    VP8D_PTR optr;

	int input_height;
	int input_width;

//	dec_input_ptr = (MMDecInput *)MallocInterMem(sizeof(MMDecInput)); //MallocExtraMem
//	SCI_ASSERT(NULL != dec_input_ptr);

//	dec_output_ptr = (MMDecOutput *)MallocExtraMem(sizeof(MMDecOutput)); //MallocExtraMem
//	SCI_ASSERT(NULL != dec_output_ptr);

	dec_malloc_bfr_ptr =  (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer)); //MallocExtraMem
	SCI_ASSERT(NULL != dec_malloc_bfr_ptr);

	#if defined(SIM_IN_WIN)
		/*load file into pStreamBfr*/
		bs_ptr = input_bs_bfr_ptr = (uint8 *)MallocExtraMem(g_readFileSize);  ///20Mbyte
		length = fread(input_bs_bfr_ptr, 1, g_readFileSize, s_pDec_input_bs_file);
		g_readFileSize = length;
	#else
		//input_bs_bfr_ptr = videoStream;//(uint8*)(SDRAM_START_ADDR);
		//g_pOutBfr = (uint8 *)MallocExtraMem(352*288*3/2 * 1); //50frame,~8Mbyte 
	#endif
	
	dec_malloc_bfr_ptr->int_size = total_inter_malloc_size - g_inter_malloced_size;	
	dec_malloc_bfr_ptr->int_buffer_ptr = (uint8 *)MallocInterMem(dec_malloc_bfr_ptr->int_size);	
	SCI_ASSERT(NULL != dec_malloc_bfr_ptr->int_buffer_ptr);
	// reset by hand ?
	//memset(dec_malloc_bfr_ptr->int_buffer_ptr, 0, dec_malloc_bfr_ptr->int_size);
#ifdef SIM_IN_WIN
	dec_malloc_bfr_ptr->size = total_extra_malloc_size - g_extra_malloced_size;	
	dec_malloc_bfr_ptr->common_buffer_ptr = (uint8 *)MallocExtraMem(dec_malloc_bfr_ptr->size);
	SCI_ASSERT(NULL != dec_malloc_bfr_ptr->common_buffer_ptr);
	memset(dec_malloc_bfr_ptr->common_buffer_ptr, 0, dec_malloc_bfr_ptr->size);
#endif

	g_nFrame_dec = 0;
// 	g_dispFrmNum = 0;

	//            vp8dx_initialize();
	//
	//            oxcf.Width = ctx->si.w;
	//            oxcf.Height = ctx->si.h;
	//            oxcf.Version = 9;
	//            oxcf.postprocess = 0;
	//            oxcf.max_threads = ctx->cfg.threads;

	vp8dec_InitInterMem (dec_malloc_bfr_ptr);

	optr = vp8dx_create_decompressor(/*&oxcf*/NULL);
	g_fh_reg_ptr = (VSP_FH_REG_T *)vp8dec_InterMemAlloc(sizeof(VSP_FH_REG_T));

//	g_bsm_reg_ptr->TOTAL_BITS = 0;

#ifdef SIM_IN_WIN
	if(MMDEC_OK != VP8DecMemInit(dec_malloc_bfr_ptr))
	{
		PRINTF ("init extra memory failed!\n");
		return;
	}

	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x20, V_BIT_1|V_BIT_6, "ORSC: VSP_MODE: Set standard and work mode");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x60, bs_start_addr/8, "ORSC: BSM0_FRM_ADDR");
	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x6, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR");//clr BSM
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x04, 0x00000000|(0), "ORSC: BSM_CFG1 stream buffer offset & destuff disable");//byte align
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|(512)&0xfffffffc, "ORSC: BSM_CFG0 stream buffer size");//+16dw,注意word对齐

	if (bs_ptr[0] == 'D' && bs_ptr[1] == 'K' && bs_ptr[2] == 'I' && bs_ptr[3] == 'F')
	{
		uint32 val;
		uint32 fourcc;

		file_is_ivf = 1;

		bs_ptr += 4;

		val = (bs_ptr[1]<<8) | bs_ptr[0];
		if (val != 0)
		{
			printf("Error: Unrecognized IVF version!");
		}

		bs_ptr += 4;

		fourcc = (bs_ptr[3]<<24) | (bs_ptr[2]<<16) | (bs_ptr[1]<<8) | (bs_ptr[0]);
		fourcc &= 0x00ffffff;

		if (fourcc != 0x00385056)
		{
			printf("Error: not vp8 bitstream!\n");
		}	
		bs_ptr += 4;

		input_width = (bs_ptr[1]<<8 | bs_ptr[0]) & 0x3fff;
		bs_ptr += 2;

		input_height = (bs_ptr[1]<<8 | bs_ptr[0]) & 0x3fff;

		g_stream_offset = 32;
		
		for(val=0; val<8; val++)
			BitstreamReadBits(32);
	}
	else
	{
		//pc->error_flag=1;
		return;
	}
#else
	while(1)
	{
		uint32 tmp;
		if(input_buffer_update == 1)//stream buffer update
		{
			g_stream_offset = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x20, "ORSC_SHARE: 0x20 stream_len");//初始偏移;//0
			bs_start_addr = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x18, "ORSC_SHARE: 0x18 STREAM_BUF_ADDR");			
			bs_buffer_length = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x1c, "ORSC_SHARE: 0x1c STREAM_BUF_SIZE");//buffer内容长度	,Byte	

			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x60, bs_start_addr/8, "ORSC: BSM0_FRM_ADDR");
			OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x6, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR");//clr BSM
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x04, 0x00000000|(g_stream_offset), "ORSC: BSM_CFG1 stream buffer offset & destuff disable");//byte align
			OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|((bs_buffer_length+0x30)&0xfffffffc), "ORSC: BSM_CFG0 stream buffer size");//+16dw,注意word对齐

			if( BitstreamReadBits(32) == 0x444b4946) // DKIF, byte0-3, big-endian
			{
				uint32 val;
				uint32 fourcc;
				
				file_is_ivf = 1;
				
				val = BitstreamReadBits(16); // byte4-5
				if (val != 0)
				{
					//printf("Error: Unrecognized IVF version!");
				}
				
				BitstreamReadBits(16); // byte6-7
				
				fourcc = BitstreamReadBits(32); // byte8-11
				fourcc &= 0xffffff00;
				
				if (fourcc != 0x56503800)
				{
					//printf("Error: not vp8 bitstream!\n");
				}
				
				val = (BitstreamReadBits(16) & 0xff3f); // byte12-13
				input_width = ((val&0xff00)>>8) | ((val&0xff)<<8);
				val = (BitstreamReadBits(16) & 0xff3f); // byte14-15
				input_height = ((val&0xff00)>>8) | ((val&0xff)<<8);
				
				BitstreamReadBits(32); // byte16-31
				BitstreamReadBits(32);
				BitstreamReadBits(32);
				BitstreamReadBits(32);
				g_stream_offset += 32; // OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS")/8
			}
			else
			{
				//pc->error_flag=1;
				return;
			}
			//input_buffer_update = 0;
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x0, tmp&0xfffffeff,"ORSC_SHARE: MODE_CFG clear input_buffer_update");
			break;
		}
		tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
		input_buffer_update = (tmp>>8)&1; //1
		video_buffer_malloced = (tmp>>16)&1; //0
		g_not_first_reset = (tmp>>29)&1; //0
		cpu_will_be_reset = (tmp>>30)&1; //arm根据情况设置;
	}
#endif

	while (1)
	{
		if (g_nFrame_dec >= (g_input->frame_num_dec))
		{
			//printf ("\nfrm_width:%d, frm_height:%d\n", dec_output_ptr->frame_width, dec_output_ptr->frame_height);
			break;
		}
#ifdef SIM_IN_WIN
		or1200_print = 1;

		g_bsm_reg_ptr->TOTAL_BITS = 0;
		printf ("frame No.%d\n", g_nFrame_dec);

		bs_ptr = input_bs_bfr_ptr + g_stream_offset;
		file_end = get_unit(bs_ptr, video_format_ptr);
		if (file_end)
			break;

		bs_start_addr = BS_START_ADDR + g_stream_offset;
		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x60, bs_start_addr/8, "ORSC: BSM0_FRM_ADDR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x6, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR");//clr BSM
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x04, 0x00000000|(g_stream_offset&0x7), "ORSC: BSM_CFG1 stream buffer offset & destuff disable");//byte align

		g_stream_offset += (12 + video_format_ptr->i_extra);
		bs_buffer_length = (video_format_ptr->i_extra+512); //g_readFileSize
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|(bs_buffer_length+0x30)&0xfffffffc, "ORSC: BSM_CFG0 stream buffer size");// Set each time after BSM clear, +16dw,注意word对齐
		BitstreamReadBits(32);
		BitstreamReadBits(32);
		BitstreamReadBits(32);

		//BSM_Init(video_format_ptr->i_extra);
		vp8dx_receive_compressed_data(optr, video_format_ptr->i_extra, video_format_ptr->p_extra, 0);
        g_nFrame_dec++;
//		if(((VP8D_COMP *)optr)->common.show_frame)
//			write_out_frame(((VP8D_COMP *)optr)->common,s_pDec_recon_yuv_file,((VP8D_COMP *)optr)->common.Width,((VP8D_COMP *)optr)->common.Height);
			//write_out_frame(((VP8D_COMP *)optr)->common,s_pDec_recon_yuv_file,input_width,input_height);
#else
		//第一次进入或重新进入或继续循环
		OR1200_READ_REG_POLL(ORSC_SHARERAM_OFF+0, 0x80000000,0x80000000, "ORSC_SHARE: shareRAM 0x0 MODE_CFG run"); //polling until can run	
		//如果reset后重新进入，需恢复现场寄存器
        g_not_first_reset =((OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG") >> 29)&1);
		if(g_not_first_reset == 1)	// var need update ?
		{
			//恢复现场寄存器
		}

		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

		input_buffer_update = ((OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG") >> 8)&1);
		if(input_buffer_update == 1)//stream buffer update
		{
			uint32 tmp;
			g_stream_offset = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x20, "ORSC_SHARE: 0x20 stream_len");//初始偏移;//0
			bs_start_addr = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x18, "ORSC_SHARE: 0x18 STREAM_BUF_ADDR");			
			bs_buffer_length = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x1c, "ORSC_SHARE: 0x1c STREAM_BUF_SIZE");//buffer内容长度	,Byte	
			// input_buffer_update = 0;
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x0, "ORSC_SHARE: MODE_CFG");
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x0, tmp&0xfffffeff,"ORSC_SHARE: MODE_CFG clear input_buffer_update");
		}
		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x60, (bs_start_addr/8+g_stream_offset/8), "ORSC: BSM0_FRM_ADDR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x6, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR");//clr BSM
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x04, 0x00000000|((g_stream_offset&0x7)), "ORSC: BSM_CFG1 stream buffer offset & destuff disable");//byte align
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, 0x80000000|((bs_buffer_length+0x30)&0xfffffffc), "ORSC: BSM_CFG0 stream buffer size");//打开BSM load slice nalu sidata//真实sps/pps/ze+16dw,注意word对齐

		if((g_stream_offset+12) < bs_buffer_length)
		{
			uint32 temp = BitstreamReadBits(32);
			video_format_ptr->i_extra = ((temp&0xff)<<24) | ((temp&0xff00)<<8) | ((temp&0xff0000)>>8) | ((temp&0xff000000)>>24);
			BitstreamReadBits(32);
			BitstreamReadBits(32);
			g_stream_offset += 12;
			video_format_ptr->p_extra = (void*)g_stream_offset;
			g_stream_offset += video_format_ptr->i_extra;			
			//file_end = (g_stream_offset < (bs_buffer_length-1)) ? 0 : 1;
			
		}
		else
			file_end = 1;

        if(!file_end)
		     vp8dx_receive_compressed_data(optr, video_format_ptr->i_extra, video_format_ptr->p_extra, 0);
		
		
		{
			uint32 tmp;
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x20, g_stream_offset, "ORSC_SHARE: 0x20 stream_len"); // Provide Arm to decide if read in new data
			
			//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x2c, (uint32)optr->common->frame_to_show->y_buffer+or_addr_offset, "ORSC_SHARE: Frame_Y_ADDR");
			//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x30, (uint32)optr->common->frame_to_show->u_buffer+or_addr_offset, "ORSC_SHARE: Frame_UV_ADDR");
			//OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x34, frame_buf_size, "ORSC_SHARE: shareRAM 0x34 UV_FRAME_BUF_SIZE");
			OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x4c, 1, "ORSC_SHARE: shareRAM 0x4c FRAME_effective");//!file_end
            OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x80, g_nFrame_dec, "ORSC_SHARE: 0x80 g_nFrame_dec");
			g_nFrame_dec++;
			file_end = 0;
			// Wait ARM update
			tmp = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x00, "ORSC_SHARE: MODE_CFG");
			cpu_will_be_reset = (tmp>>30)&1;
			if(cpu_will_be_reset==1)//如果重新进入会reset，则需要保存现场寄存器
			{
				//保存现场寄存器
			}
            OR1200_WRITE_REG(ORSC_SHARERAM_OFF+0x00, tmp&0x7fffffff, "ORSC_SHARE: MODE_CFG Stop");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 1,"ORSC: VSP_INT_GEN Done_int_gen");
			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x2c, 0,"ORSC: VSP_INT_GEN Done_int_gen");

			
			//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));
		}
#endif
		//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");//weihu
		//g_nFrame_dec++;
	}
#ifdef SIM_IN_WIN
	FPRINTF_ORSC(g_vsp_glb_reg_fp, "f_01234567_89abcdef\n");
	/*{
		FILE * pic_size_fp = fopen("..\\..\\..\\..\\pic_size.txt","w");
		fprintf(pic_size_fp,"%d\n",((VP8D_COMP *)optr)->common.frame_to_show->y_width);
		fprintf(pic_size_fp,"%d\n",((VP8D_COMP *)optr)->common.frame_to_show->y_height);
		fclose(pic_size_fp);
	}*/
#endif
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
