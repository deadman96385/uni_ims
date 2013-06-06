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
#include "sc8810_video_header.h"
#if defined(_FPGA_AUTO_VERIFICATION_)
#include "fpga_auto_vrf_def.h"
#endif //_FPGA_AUTO_VERIFICATION_

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
#if 0



static int32 s_bFisrtUnit = TRUE;
volatile static int32 decLen;
extern int32 bs_buffer_length;
extern int32 g_not_first_reset;
extern int32 stream_buffer_update;
extern int32 video_size_get;
extern int32 video_buffer_malloced;

LOCAL int32 Mp4Dec_GetSeqenceType(uint8 *bs_ptr)
{
	uint8 *ptr = bs_ptr;
	
	if((ptr[0] == 0x00) && (ptr[1] == 0x00))
	{
		if((ptr[2]&0xFC) == 0x80)
		{
			return ITU_H263;
		}else if((ptr[2]&0xF8) == 0x80)
		{
			return FLV_H263;
		}
	}

	return VSP_MPEG4;
}

/*get a unit until to next picture start code*/
int g_readFileSize = 20*1024*1024;
int file_end = FALSE;

#if SIM_IN_WIN

LOCAL void Mp4Dec_GetUnit(uint8 *pInStream, MMDecVideoFormat *p_video_format)
{
	int32 len = 0;
	uint8 *ptr;
	int32 data = 0xffffffff;
	static int32 decLen = 0;

	ptr = p_video_format->p_extra = pInStream;

	if(p_video_format->video_std == VSP_MPEG4)
	{
		if(!s_bFisrtUnit)
		{
			ptr = (uint8 *)(pInStream + 4);
			len += 4;
			decLen += 4;
		}
		
		while(data != 0x1B6)
		{
			data = (data << 8) | (*ptr++);
			
			len++;
			decLen++;

			if (decLen >= g_readFileSize)
			{
			//	PRINTF ("file end!\n");
			//	exit (-1);
				file_end = TRUE;
				decLen = 4;
				len += 4;
				break;
			}
		}
		
		len = len - 4;	
		decLen = decLen -4;
		
		s_bFisrtUnit = FALSE;
	}else
	{
		uint32 msk = 0xfffffc;

		if(p_video_format->video_std == FLV_H263)
		{
			msk = 0xfffff8;
		}

		ptr = (uint8 *)(pInStream + 3);
		len += 3;	
		decLen += 3;
	
		while((data & msk) != 0x80)
		{ 
			data = (data << 8) | (*ptr++);
			len++;
			decLen++;
			if (decLen >= g_readFileSize)
			{
			//	PRINTF ("file end!\n");
			//	exit (-1);
				file_end = TRUE;
				decLen = 3;
				len += 3;
				break;
			}
		}
		
		len = len - 3;
		decLen = decLen -3;
	}	
	
	p_video_format->i_extra = len;

}
#else
LOCAL void Mp4Dec_GetUnit(MMDecVideoFormat *p_video_format)
{
	int32 len = 0;
	//uint8 *ptr;
	int32 data = 0xffffffff;
	//int32 data1;
    int32 len1=0;
	int tmp;
//	int i;

  //  OR1200_WRITE_REG(0x18000c, 0x00345600,"for FPGA debug 7");
   // uint8 *stream;
//	ptr = p_video_format->p_extra = pInStream;
    decLen=g_stream_offset;
	if(p_video_format->video_std == VSP_MPEG4)
	{
		if(!s_bFisrtUnit)
		{
			Mp4Dec_FlushBits(32);
			len += 4;
		}
		
		while(data != 0x1B6)
		{
			/*data1=Mp4Dec_ReadBits(32);

			for(i=3;i>-1;i--)
			{
				data=(data << 8)|(  ( data1&(0xff000000>>(8*i)) )>>24)

				if(data!=0x1B6)
				{
					len++;
				}else
				{
					break;
				}

			}*/
      //      OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x8c, 0x88888888,"debug");//weihu
			data = (data << 8) | (Mp4Dec_ReadBits( 8));
	//		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x8c, 0,"debug");//weihu
			len++;
			decLen++;
      //      OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x84, len,"debug");//weihu
			if(decLen>bs_buffer_length)
			{
				break;

			}
            //tmp=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
		    //OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, tmp&0xfffffeff,"shareRAM 0x0 clr stream_buffer_update");
		}
	//	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x84, 0x12340000|len,"debug");//weihu
		len1 = len - 4;	
		s_bFisrtUnit = FALSE;
	}else
	{
		uint32 msk = 0xfffffc;

		if(p_video_format->video_std == FLV_H263)
		{
			msk = 0xfffff8;
		}

		//ptr = (uint8 *)(pInStream + 3);
		Mp4Dec_ReadBits( 24);
		len += 3;	
		while((data & msk) != 0x80)
		{ 
			//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x8c, 0x88888888,"debug");//weihu
			data = (data << 8) | (Mp4Dec_ReadBits( 8));
			//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x8c, 0,"debug");//weihu
			len++;
			decLen++;
            //OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x88, len,"debug");//weihu
			if(decLen>bs_buffer_length)
			{
				break;
				
			}
		}
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x88, 0x12000000|len,"debug");//weihu
		len1 = len - 3;

	}
	//OR1200_WRITE_REG(0x18000c, 0x00005678,"for FPGA debug 7");
	//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x84, 0x55aa,"debug");//weihu
	//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x88, 0x55aa,"debug");//weihu

	p_video_format->i_extra = len1;

	//tmp=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
	//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, tmp&0xfffffeff,"shareRAM 0x0 clr stream_buffer_update");
}
#endif

#if SIM_IN_WIN 

/*out put the deblocking frame to file or other device*/
void write_out_frame(MMDecOutput *dec_output_ptr)
{
	int i=0;int j=0;

	FILE * pf_seq = s_pDec_disp_yuv_file;
	uint8 *p_frame;
	int width;
	int height;
	uint8 *pFrameUV;
	DEC_VOP_MODE_T *vop_mode_ptr = g_dec_vop_mode_ptr;
	
	width = vop_mode_ptr->FrameWidth;
	height = vop_mode_ptr->FrameHeight;

	/*write Y frame*/	
	p_frame = dec_output_ptr->pOutFrameY;

	for (i = 0; i < height; i++)
	{
	#ifndef _ARM_
		fwrite (p_frame, 1, width, pf_seq);
	#else
		memcpy (g_pOutBfr, p_frame, width);
		g_pOutBfr += width;
	#endif
		p_frame += width;
	}

	pFrameUV = dec_output_ptr->pOutFrameU;
	width = width / 2;
	height = height / 2;

#if 0 //three plane	
	/*write U frame*/
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
		#ifndef _ARM_
			fwrite(&pFrameUV[i*width*2+j*2], 1, 1, pf_seq);
		#else
			*g_pOutBfr++ = pFrameUV[i*width*2+j*2];
		#endif
		}
	}

	/*write V frame*/
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
		#ifndef _ARM_
			fwrite(&pFrameUV[i*width*2+j*2+1], 1, 1, pf_seq);
		#else
			*g_pOutBfr++ = pFrameUV[i*width*2+j*2+1];
		#endif
		}
	}
#else //two plane
	fwrite (pFrameUV, 1, width*height*2, pf_seq);
#endif

	MPEG4_DecReleaseDispBfr(dec_output_ptr->pOutFrameY);

	g_dispFrmNum++;
}

/*out put the rec frame to file or other device*/
void write_out_rec_frame(MMDecOutput *dec_output_ptr)
{
	int i, j;
	uint8 *p_frame;
	int width;
	int height;
	uint8 *pFrameUV;
	DEC_VOP_MODE_T *vop_mode_ptr = g_dec_vop_mode_ptr;
	DEC_FRM_BFR *rec_frm;
	FILE * pf_seq = s_pDec_recon_yuv_file;

	rec_frm = Mp4Dec_GetRecFrm(dec_output_ptr->pOutFrameY);
	
	if(PNULL == rec_frm)
	{
		return;
	}

	width = vop_mode_ptr->FrameWidth;
	height = vop_mode_ptr->FrameHeight;
	
	/*write Y frame*/	
	p_frame = rec_frm->imgY;

	for (i = 0; i < height; i++)
	{
		fwrite (p_frame, 1, width, pf_seq);
		p_frame += width;
	}

	pFrameUV = rec_frm->imgU;
	width = width / 2;
	height = height / 2;
	
#if 0 //three plane		
	/*write U frame*/
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
		#ifndef _ARM_
			fwrite(&pFrameUV[i*width*2+j*2], 1, 1, pf_seq);
		#else
			*g_pOutBfr++ = pFrameUV[i*width*2+j*2];
		#endif
		}
	}

	/*write V frame*/
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
		#ifndef _ARM_
			fwrite(&pFrameUV[i*width*2+j*2], 1, 1, pf_seq);
		#else
			*g_pOutBfr++ = pFrameUV[i*width*2+j*2];
		#endif
		}
	}
#else //two plane
	fwrite (pFrameUV, 1, width*height*2, pf_seq);
#endif
}

void GenErrInfo (
				 MMDecInput *	dec_input_ptr, 
				 int			frame_len, 
				 uint8		*	err_inf_ptr
				 )
{
	int i;
	int n_pack;
	int err_num = 0;

	n_pack = (frame_len+99) / 100;

	for (i = 0; i < n_pack; i++)
	{
		if (err_inf_ptr[i] == 1)
		{
			dec_input_ptr->err_pkt_pos[err_num].start_pos	= i*100;
			dec_input_ptr->err_pkt_pos[err_num].end_pos		= (i+1)*100 - 1;
			err_num++;
		}		
	}

	dec_input_ptr->err_pkt_num = err_num;	

	dec_input_ptr->err_pkt_num = 0; //added by xiaowei, for normal bitstream	

}

#endif

int g_skipped_frm = 0;
uint8 * err_inf_ptr;

int32 set_channel_quality(int32 err_MB_num)
{
	//0: good, 1: ok, 2: poor
	int32 channel_quality = 2; //poor;

	if (err_MB_num < 10)
	{
		channel_quality = 0; //good
	}else if (err_MB_num < 30)
	{
		channel_quality = 1;
	}else 
	{
		channel_quality = 2;
	}

	return channel_quality;
}



int32 g_slice_datalen;
int OR1200_Vaild=1;	 

void Mp4Dec_main(void)
{ 
		
	MMDecRet ret;
	MMDecRet decode_return;
	MMDecVideoFormat video_format;
	MMDecVideoFormat *video_format_ptr = &video_format;

	int32		channel_quality = 0; //default is (0).
	int tmp,i;
#if SIM_IN_WIN
    uint8 *input_bs_bfr_ptr;
	uint32 length;
#endif

	MMCodecBuffer *dec_malloc_bfr_ptr = NULL;
	MMDecInput *dec_input_ptr = NULL;
	MMDecOutput *dec_output_ptr = NULL;
	


	
#if SIM_IN_WIN	

	dec_input_ptr = (MMDecInput *)MallocExtraMem(sizeof(MMDecInput));
	//SCI_ASSERT(NULL != dec_input_ptr);

	dec_output_ptr = (MMDecOutput *)MallocExtraMem(sizeof(MMDecOutput));	
	//SCI_ASSERT(NULL != dec_output_ptr);

    dec_malloc_bfr_ptr =  (MMCodecBuffer *)MallocExtraMem(sizeof(MMCodecBuffer));
	/*load file into pStreamBfr*/

	input_bs_bfr_ptr = (uint8 *)MallocExtraMem(g_readFileSize);  ///20Mbyte
	length = fread(input_bs_bfr_ptr, 1, g_readFileSize, s_pDec_input_bs_file);
	g_readFileSize = length;
    
	dec_malloc_bfr_ptr->int_size = TOTAL_INTER_MALLOC_SIZE - g_inter_malloced_size;	
	dec_malloc_bfr_ptr->int_buffer_ptr = (uint8 *)MallocInterMem(dec_malloc_bfr_ptr->int_size);	
	SCI_ASSERT(NULL != dec_malloc_bfr_ptr->int_buffer_ptr);
	memset(dec_malloc_bfr_ptr->int_buffer_ptr, 0, dec_malloc_bfr_ptr->int_size);

	dec_malloc_bfr_ptr->size = TOTAL_EXTRA_MALLOC_SIZE - g_extra_malloced_size;	
	dec_malloc_bfr_ptr->common_buffer_ptr = (uint8 *)MallocExtraMem(dec_malloc_bfr_ptr->size);
	SCI_ASSERT(NULL != dec_malloc_bfr_ptr->common_buffer_ptr);
	memset(dec_malloc_bfr_ptr->common_buffer_ptr, 0, dec_malloc_bfr_ptr->size);

	g_nFrame_dec = 0;
	g_dispFrmNum = 0;
    
   video_format_ptr->video_std = Mp4Dec_GetSeqenceType(input_bs_bfr_ptr);
   
	Mp4Dec_InitGlobal();
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, video_format_ptr->video_std,"VSP_MODE");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    OR1200_WRITE_REG(0x80001810, 0x184000/8,"Partition_info_addr ");
    OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
	
    
	if(video_format_ptr->video_std ==VSP_MPEG4)
	{
		Mp4Dec_GetUnit(input_bs_bfr_ptr, video_format_ptr);
		input_bs_bfr_ptr += video_format_ptr->i_extra;
		
	}else
	{
		video_format_ptr->i_extra = 0;
	}
	
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x00000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//byte align
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(video_format_ptr->i_extra+128),"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐
	
#else
	{
	    int tmp;
        decLen=0;
		
		dec_input_ptr = (MMDecInput *)MallocInterMem(sizeof(MMDecInput));
		//SCI_ASSERT(NULL != dec_input_ptr);
		
		dec_output_ptr = (MMDecOutput *)MallocInterMem(sizeof(MMDecOutput));	
		//SCI_ASSERT(NULL != dec_output_ptr);
		
		dec_malloc_bfr_ptr =  (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer));
		
		dec_malloc_bfr_ptr->int_size = TOTAL_INTER_MALLOC_SIZE - g_inter_malloced_size;
		dec_malloc_bfr_ptr->int_buffer_ptr=(uint8*)(0x00080000 + g_inter_malloced_size);
		//dec_malloc_bfr_ptr->size = TOTAL_EXTRA_MALLOC_SIZE - g_extra_malloced_size;
		//dec_malloc_bfr_ptr =  (MMCodecBuffer *)MallocInterMem(sizeof(MMCodecBuffer));

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");

        OR1200_READ_REG_POLL(SHARE_RAM_BASE_ADDR+0, 0x80000000,0x80000000,"shareRAM 0x0 run");//polling until can run

		stream_buffer_update=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>8)&1;//1; //weihu
		if(stream_buffer_update)//stream buffer跟新
		{   
			g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");//初始偏移;//0
			bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");			
			bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//buffer内容长度	,Byte	
			//stream_buffer_update = FALSE;
			tmp=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
			OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, (tmp&0xfffffeff),"shareRAM 0x0 clr stream_buffer_update");
			
		}

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((0<<31)|g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//byte align
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐
		OR1200_WRITE_REG(0x80001810, (unsigned int)(0x184000+or_addr_offset)/8,"Partition_info_addr ");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (24<<24),"BSM_rd n bits");
		tmp=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
		
		if((tmp&0xffff00)==0)
		{
			if(((tmp&0xff)&0xFC) == 0x80)
			{
				video_format_ptr->video_std=ITU_H263;
			}else if(((tmp&0xff)&0xF8) == 0x80)
			{
				video_format_ptr->video_std=FLV_H263;
			}else
			{
				video_format_ptr->video_std=VSP_MPEG4;
			}
		}else
		{
			video_format_ptr->video_std=VSP_MPEG4;
		}

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, video_format_ptr->video_std,"VSP_MODE");
	}
    
	if(video_format_ptr->video_std ==VSP_MPEG4)
	{
		Mp4Dec_GetUnit(video_format_ptr);
	
	}else
	{
		video_format_ptr->i_extra = 0;
	}
#endif

	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((0<<31)|g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");
	g_nFrame_dec = 0;
	g_dispFrmNum = 0;
	ret = MP4DecInit(dec_malloc_bfr_ptr, video_format_ptr);

   // g_stream_offset += video_format_ptr->i_extra;
#if SIM_IN_WIN
	
	 FPRINTF(g_fp_dctpara_tv, "//dct_mode=0 vsp_standard=%d scale_enable=0\n",video_format_ptr->video_std);//jzy
	 FPRINTF(g_fp_mbcpara_tv, "//STD=%d  ENC=0\n",video_format_ptr->video_std);

	 if(MMDEC_OK != ret)
	 {
		 PRINTF ("init failed!\n");
		 return;
	 }
	 
	 ret = MP4DecMemInit(dec_malloc_bfr_ptr);
	 if(MMDEC_OK != ret)
	 {
		 PRINTF ("init extra memory failed!\n");
		 return;
	 }
	 err_inf_ptr = g_err_inf;
#endif //_CMODEL_

   g_stream_offset += video_format_ptr->i_extra;
   OR1200_WRITE_REG(0x180000, (0x12340000+g_stream_offset),"for FPGA debug 1");


	while(1)
	{
#if SIM_IN_WIN
#else
		//第一次进入或重新进入或继续循环
		OR1200_READ_REG_POLL(SHARE_RAM_BASE_ADDR+0, 0x80000000,0x80000000,"shareRAM 0x0 run");//polling until can run
		
		
		
		g_not_first_reset=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>29)&1;//0;
		//如果reset后重新进入，需恢复现场寄存器        
		if(g_not_first_reset)
		{
			//恢复现场寄存器
			//g_image_ptr->is_need_init_vsp_hufftab = TRUE;
		};
		
		for(i=1;i<16;i++)
			OR1200_WRITE_REG(0x180000+i*4, 0,"for FPGA debug clr");
		
				
	
#endif
	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");	
#if SIM_IN_WIN
		PRINTF ("nFrame: %d,\t display %d Frame,\t", g_nFrame_dec, g_dispFrmNum);
		OR1200_Vaild=1;
	
		Mp4Dec_GetUnit(input_bs_bfr_ptr, video_format_ptr);
		
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x00000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//byte align
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(g_stream_offset+video_format_ptr->i_extra+128),"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐	
		{
			input_bs_bfr_ptr += video_format_ptr->i_extra;
			dec_input_ptr->pStream = video_format_ptr->p_extra;
			dec_input_ptr->dataLen = video_format_ptr->i_extra;
			dec_input_ptr->expected_IVOP = 0;//g_dec_vop_mode_ptr->error_flag?1:0;
			dec_input_ptr->beLastFrm = FALSE;
			dec_input_ptr->beDisplayed = 1;
			
			g_slice_datalen=dec_input_ptr->dataLen;
			if((g_nFrame_dec >= (g_input->frame_num_dec)) || file_end)
				dec_input_ptr->beLastFrm = TRUE;
		}		
			GenErrInfo (dec_input_ptr, video_format_ptr->i_extra, err_inf_ptr);
			err_inf_ptr += (video_format_ptr->i_extra + 99)/100;	
#else
		OR1200_WRITE_REG(0x180004, 0x12345678,"for FPGA debug 1");
		stream_buffer_update=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>8)&1;//1;
		if(stream_buffer_update)//stream buffer跟新
		{   
			
			g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");//初始偏移;//0
			bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");			
			bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//buffer内容长度	,Byte	
			
	

			//stream_buffer_update = FALSE;
			tmp=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
			OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, tmp&0xfffffeff,"shareRAM 0x0 clr stream_buffer_update");
		    OR1200_WRITE_REG(0x180008, 0x12345678,"for FPGA debug 7");		    
		}
	    OR1200_WRITE_REG(0x18000c, 0x12340000,"for FPGA debug 7");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((0<<31)|g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//byte align
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐
		
		//查找下一个start code//得到nal unit 长度
		{

			Mp4Dec_GetUnit(video_format_ptr);
		}

		OR1200_WRITE_REG(0x18000c, 0x12345678,"for FPGA debug 7");
		if(video_size_get&&video_buffer_malloced)
		{
			video_size_get=0;//避免再次进入
			extra_malloc_mem_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x10, "shareRAM 0x10 VSP_MEM1_ST_ADDR");//
			dec_malloc_bfr_ptr->common_buffer_ptr = (uint8 *)(extra_malloc_mem_start_addr);//(uint8 *)MallocExtraMem(dec_malloc_bfr_ptr->size);
			g_extra_malloced_size = 0;
			dec_malloc_bfr_ptr->size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x14,"shareRAM 0x14 VSP_MEM1_SIZE");
			
		//	ret = H264DecMemInit(dec_malloc_bfr_ptr);	
		}

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x00000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//byte align
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐
		
		
        dec_input_ptr->dataLen=video_format_ptr->i_extra;
		dec_input_ptr->expected_IVOP = 0;//g_dec_vop_mode_ptr->error_flag?1:0;
		dec_input_ptr->beLastFrm = FALSE;
		dec_input_ptr->beDisplayed = 1;//?for 显示buffer
		
#endif
		
 		decode_return = MP4DecDecode(dec_input_ptr, dec_output_ptr);
 		OR1200_WRITE_REG(0x18002c, (0x12345678|g_dec_vop_mode_ptr->error_flag),"for FPGA debug 4");	
        
		g_stream_offset+=video_format_ptr->i_extra;




#if SIM_IN_WIN
		g_stream_offset += dec_input_ptr->dataLen;


		if(dec_output_ptr->pOutFrameY != PNULL)
		{
			write_out_frame(dec_output_ptr);
			
			write_out_rec_frame(dec_output_ptr); //for debug

			channel_quality = set_channel_quality(dec_output_ptr->err_MB_num);
		}

		OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+VSP_INT_SYS_OFF, 0x00000004,0x00000004,"BSM_frame done int");//check HW int	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x4,"clear BSM_frame done int");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	

		if(dec_input_ptr->beLastFrm)
		{
			if(dec_output_ptr->frameEffective)
			{
				mpeg4dec_GetOneDspFrm(dec_output_ptr, 0, 1);
				write_out_frame(dec_output_ptr);	
				write_out_rec_frame(dec_output_ptr); //for debug
			}

			if (file_end)
			{
				PRINTF ("Average bit rate: %d kbps!\n", (g_readFileSize * 25*8)/(1000*g_nFrame_dec));
				PRINTF ("file end!\n");
				
			}

			fprintf(g_fp_global_tv,"f_01234567_89abcdef");
			MP4DecRelease();
						
			break;
		}
#else
		{
			int tmp0;
			int cpu_will_be_reset;

			file_end=(decLen>bs_buffer_length);
		OR1200_WRITE_REG(0x180030, (0x12345678|g_dec_vop_mode_ptr->error_flag),"for FPGA debug 4");	
		video_buffer_malloced=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>16)&1;
		if(g_dec_vop_mode_ptr->error_flag||dec_output_ptr->frameEffective||file_end/*(g_stream_offset>=bs_buffer_length)*/||(video_size_get&&!video_buffer_malloced))//g_image_ptr->error_flag||
		{ 
			
			OR1200_WRITE_REG(0x180034, (0x12345678|g_dec_vop_mode_ptr->error_flag),"for FPGA debug 4");
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c, extra_malloc_mem_start_addr+frame_buf_size*16*(g_dec_vop_mode_ptr->current_addr),"display frame_Y_addr");//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgYAddr
		    	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30, extra_malloc_mem_start_addr+frame_buf_size*8*(g_dec_vop_mode_ptr->current_addr+34),"display frame_UV_addr");//0x480000+//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgUAddr
			    OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x34, ((g_dec_vop_mode_ptr->OrgFrameWidth+15)/16)*((g_dec_vop_mode_ptr->OrgFrameHeight+15)/16)*128,"display frame_UV_size");
			    OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4c, dec_output_ptr->frameEffective,"display en");
                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x80, g_nFrame_dec-1,"frame num");
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, ((video_size_get<<31)|((video_format_ptr->frame_height&0xff)<<16)|(video_format_ptr->frame_width&0xff)<<4),"shareRAM 0x4 IMAGE_SIZE");//保存到shareram
				
				if(video_size_get&&!video_buffer_malloced)
				{
					//计算total_extra_malloc_size=
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x14, total_extra_malloc_size,"shareRAM 0x14 VSP_MEM1_SIZE");////total_extra_malloc_size保存到shareram ////供arm分配新空间大小
					
				}
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, g_stream_offset,"shareRAM 0x20 stream_len");//g_stream_offset保存到shareram，//供arm判断是否更新bsm buffer
				
				cpu_will_be_reset=(OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0")>>30)&1;
				if(cpu_will_be_reset)//如果重新进入会reset，则需要保存现场寄存器
				{
					//保存现场寄存器
				}
				tmp0=OR1200_READ_REG(SHARE_RAM_BASE_ADDR, "shareRAM 0");
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0, tmp0&0x7fffffff,"shareRAM 0x0 stop");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
				//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));
				//返回arm，等待bsm buffer跟新或申请空间
				
			}
		}
#endif
		
	}
	
}

#if SIM_IN_WIN

double ComputePSNRFrame (
						 uint8	*	src_frame_ptr,
						 uint8	*	rec_frame_ptr,
						 int		frame_width,
						 int		frame_height
						 )
{
	int		y;
	int		x;
	int32	diff;
	int32	sqr_diff = 0;
	double	psnr;

	for (y = 0; y < frame_height; y++)
	{
		for (x = 0; x < frame_width; x++)
		{
			diff = (src_frame_ptr [x]) - (rec_frame_ptr [x]);
			sqr_diff += diff * diff;			
		}

		src_frame_ptr += frame_width;
		rec_frame_ptr += frame_width;
	}

	if(sqr_diff == 0)
	{
		psnr = 10000.0;
	}
	else
	{
		psnr = (log10 ((255*255*frame_width*frame_height) /(double) sqr_diff) * 10.0);
	}

	return psnr;
}

void ComputePSNR ()
{
	int					frame_width;
	int					frame_height;
	int					frame_size;
	double				psnr_y;
// 	double				psnr_u;
// 	double				psnr_v;
	DEC_VOP_MODE_T	*	vop_mode_ptr = g_dec_vop_mode_ptr;
	uint8			*	src_frame_ptr = NULL;
	uint8				*src_y, *src_u, *src_v;

	frame_width		= vop_mode_ptr->FrameWidth;
	frame_height	= vop_mode_ptr->FrameHeight;
	frame_size		= frame_width * frame_height;

	src_frame_ptr = (uint8 *)malloc(frame_size*3/2);
	src_y = src_frame_ptr;
	src_u = src_y + frame_size;
	src_v = src_u + (frame_size/4);

	/*read source frame*/
	fread (src_frame_ptr, 1, frame_size*3/2, s_pEnc_output_recon_file);

	/*compute psnr*/

	psnr_y = ComputePSNRFrame (
							src_y, 
							vop_mode_ptr->pCurRecFrame->pDecFrame->imgY, 
							frame_width, 
							frame_height
							);

//	g_psnr[0] += psnr_y;

//	psnr_u = ComputePSNRFrame (
//							src_u, 
//							vop_mode_ptr->pCurRecFrame->pDecFrame->imgUV, 
//							frame_width/2, 
//							frame_height/2
//							);
//	g_psnr[1] += psnr_u;

//	psnr_v = ComputePSNRFrame (
//							src_v, 
//							vop_mode_ptr->pCurRecFrame->pDecFrame->imgV, 
//							frame_width/2, 
//							frame_height/2
//							);
//	g_psnr[2] += psnr_v;

//	PRINTF ("\tpnsr_y: %f, psnr_u: %f, psnr_v: %f", psnr_y, psnr_u, psnr_v);

	free(src_frame_ptr);
}
#endif


#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
