/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
  ** 06/26/2012   Leon Li             Modify.                                                                                      *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
//#define PERFORMANCE_TEST
#ifdef PERFORMANCE_TEST
#include <sys/time.h>
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
 void dump_cmd_info()
{	
  	FILE* info = fopen("/data/info.txt","wb");
	FILE* data = fopen("/data/data.txt","wb");  

	FILE* info_vsp= fopen("/data/info_vsp.txt","wb");
	FILE* data_vsp = fopen("/data/data_vsp.txt","wb");  
	
        int length = 0;
	int i = 0;

	length = g_cmd_info_ptr - g_cmd_info_base;
//	for(i = 0;i< length;i+=4)
 //       {
//		fprintf(info,"%x, \n",*(unsigned int *)(g_cmd_info_base + i));
//	}
	SCI_TRACE_LOW("dump info lenght %d",length);
  	fwrite(g_cmd_info_base,1,length*4,info);
  	fwrite(g_vsp_cmd_info_base,1,length*4,info_vsp);

	length = g_cmd_data_ptr - g_cmd_data_base;
//	for(i = 0;i< length;i+=4)
 //      {
//		fprintf(data,"%x, \n",*(unsigned int *)(g_cmd_data_base + i));
//}
	SCI_TRACE_LOW("dump data lenght %d",length);		
  	fwrite(g_cmd_data_base,1,length*4,data);
  	fwrite(g_vsp_cmd_data_base,1,length*4,data_vsp);
	
	fclose(info);
	fclose(data);
	fclose(info_vsp);
	fclose(data_vsp);
}

FILE* output_yuv = NULL;
void dump_output_yuv()
{
	
        int length = 0;
	int i = 0;
        int width = g_image_ptr->frame_width_in_mbs<<4;
	int height = g_image_ptr->frame_height_in_mbs<<4;
	unsigned char * src_y = g_dec_picture_ptr->imgY;
	unsigned char * src_u= src_y+width*height;
	unsigned char * src_v = src_u+width*height/4;
	
	if(g_nFrame_dec_h264 == 0)
	{
		output_yuv = fopen("/data/out.yuv","wb");  
	}

	SCI_TRACE_LOW("dump_output_yuv width %d,height %d,src_y  %x %x %x",width,height,src_y,src_u,src_v);
	if(output_yuv != NULL)
	{
        	fwrite(src_y,1,width*height,output_yuv);
		fwrite(src_u,1,width*height/4,output_yuv);
		fwrite(src_v,1,width*height/4,output_yuv);
	}
		
	if(g_nFrame_dec_h264 == 54)
	{
		fclose(output_yuv );
	}
}

PUBLIC void H264Dec_SeqLevelConfig (DEC_IMAGE_PARAMS_T *img_ptr)
{
	return;
}

PUBLIC MMDecRet h264Dec_PicLevelSendRefAddressCommmand (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;
	
    if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
	{
		img_ptr->error_flag = TRUE;
		return MMDEC_ERROR;
	}

	if(VSP_READ_REG_POLL(VSP_AXIM_REG_BASE+AXIM_STS_OFF, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
		return MMDEC_HW_ERROR;
	}

	open_vsp_iram();

	VSP_WRITE_REG(pTableAddr + 0, MAPaddr(g_dec_picture_ptr->imgYAddr), "configure reconstruct frame Y");

#if _CMODEL_ //for RTL simulation
	VSP_WRITE_REG(pTableAddr + 4, ((uint32)INTRA_PREDICTION_ADDR)>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
	VSP_WRITE_REG(pTableAddr + 4*3, ((uint32)VLD_CABAC_TBL_ADDR)>>2, "vld cabac buffer address");
#else
	#ifdef _VSP_LINUX_
		VSP_WRITE_REG(pTableAddr + 4, ((uint32)H264Dec_ExtraMem_V2Phy(img_ptr->ipred_top_line_buffer))>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
		VSP_WRITE_REG(pTableAddr + 4*3, ((uint32)H264Dec_ExtraMem_V2Phy(img_ptr->vld_cabac_table_ptr))>>2, "vld cabac buffer address");
	#else
		VSP_WRITE_REG(pTableAddr + 4, ((uint32)img_ptr->ipred_top_line_buffer)>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
		VSP_WRITE_REG(pTableAddr + 4*3, ((uint32)img_ptr->vld_cabac_table_ptr)>>2, "vld cabac buffer address");
	#endif
#endif
	
	close_vsp_iram();
	
	return MMDEC_OK;
}

PUBLIC void H264Dec_mb_level_sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
#if _CMODEL_
	mbc_module();
#endif

	VSP_READ_REG_POLL_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, 5, 1, 1, "MBC: polling mbc done");	
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, (0<<30), "hold next mb's mbc by setting mb type to be intra");

	VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (3<<24) |(MBC_CMD0_WOFF<<16) |(MBC_ST0_WOFF<<8)|((1<<7)|MBC_ST0_WOFF));

#if _CMODEL_
	dbk_module();
#endif	

	return;
}
struct timeval start;
struct timeval end;
long long time_use;
PUBLIC MMDecRet H264Dec_Picture_Level_Sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 cmd, cmd1;
	uint32 ret;
	uint32 length= 0;
//	SCI_TRACE_LOW ("in  H264Dec_Picture_Level_Sync!\n");	
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
	VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec_h264, "VSP_TST: configure frame_cnt to debug register for end of picture");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (2<<24) | (VSP_TST_WOFF<<8)|((1<<7)|VSP_DBG_WOFF));
		
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
	VSP_WRITE_CMD_INFO(0x12345678);
#if 1//_DEBUG_		
	SCI_TRACE_LOW("H264Dec_Picture_Level_Sync: 0, frame num %0x\n", g_nFrame_dec_h264);		
#endif

	H264Dec_VSPInit ();
#if 0
	length = (uint32)g_cmd_info_ptr - (uint32)g_cmd_info_base;
	memcpy(g_vsp_cmd_info_base,g_cmd_info_base,length);
	length = (uint32)g_cmd_data_ptr - (uint32)g_cmd_data_base;
	memcpy(g_vsp_cmd_data_base,g_cmd_data_base,length);
#endif
//SCI_TRACE_LOW("g_cmd_data_ptr length %d",length);
	if (img_ptr->is_need_init_vsp_hufftab /*&& img_ptr->is_new_pic*/)
	{
		configure_huff_tab (g_huff_tab_token, 69);
		//img_ptr->is_need_init_vsp_hufftab = FALSE;
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<15)|(1<<10), "DCAM_INT_RAW: enable CMD DONE INT bit");
	
//	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "read endian sel offset");		
//	cmd |= ((img_ptr->width*img_ptr->height/4) ); //word unit @modified 20120810
	cmd = ((img_ptr->width*img_ptr->height/4) ); //word unit
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, ((img_ptr->height << 16) | img_ptr->width), "configure frame width");
			
	//configure command infor and data address into register, and enable cmd-exe
#if _CMODEL_
 	VSP_WRITE_REG(VSP_MEMO10_ADDR+20*4, ((uint32)CMD_CONTROL_INFO_ADDR)>>2, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+21*4, ((uint32)CMD_CONTROL_DATA_ADDR)>>2, "config CMD control data buffer address");
#else
	VSP_WRITE_REG(VSP_MEMO10_ADDR+20*4, ((uint32)H264Dec_ExtraMem_V2Phy(g_cmd_info_base))>>2, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+21*4, ((uint32)H264Dec_ExtraMem_V2Phy(g_cmd_data_base))>>2, "config CMD control data buffer address");
#endif

#if 0 //_DEBUG_
	SCI_TRACE_LOW ("%s,%d!\n",__FUNCTION__,__LINE__);
	SCI_TRACE_LOW ("g_vsp_cmd_info_base %x,cmd_data%x!\n",g_vsp_cmd_info_base,g_vsp_cmd_data_base);
	SCI_TRACE_LOW ("cmd_info %x,cmd_data%x!\n",VSP_READ_REG(VSP_MEMO10_ADDR+20*4,""),VSP_READ_REG(VSP_MEMO10_ADDR+21*4,""));

	if(2 == g_nFrame_dec_h264)
	   	dump_cmd_info();

	SCI_TRACE_LOW ("stream_len %d,stream_len_left %d,buffA :%x buffB:%x",g_image_ptr->bitstrm_ptr->stream_len,\
	g_image_ptr->bitstrm_ptr->stream_len_left,g_image_ptr->bitstrm_ptr->bufa,g_image_ptr->bitstrm_ptr->bufb);
#endif

#ifdef PERFORMANCE_TEST
	gettimeofday(&start,NULL);
#endif

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd = cmd & (~0x10);
	cmd = cmd & (~0x200);
	cmd |= (1<<8);
//	SCI_TRACE_LOW ("b4 enable cmd-exe!\n");

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	
 //      SCI_TRACE_LOW ("aft enable cmd-exe!\n");

	ret = VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), TIME_OUT_CLK/*0x1fffffff*/, "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
#if 0//_DEBUG_		
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, "");
	SCI_TRACE_LOW("H264Dec_Picture_Level_Sync, cmd done, %d\n", cmd);
	
#ifdef PERFORMANCE_TEST
	gettimeofday(&end,NULL);
	time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us
	SCI_TRACE_LOW("g_nFrame_dec_h264 %d time_use is %lld\n",g_nFrame_dec_h264,time_use);
#endif

//	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+CMD_DEBUG0_OFFSET, "");
//	cmd1 = VSP_READ_REG(VSP_AHBM_REG_BASE+CMD_DEBUG1_OFFSET, "");
//	SCI_TRACE_LOW("cmd dbg0 %0x, cmd dbg1 %0x\n", cmd, cmd1);		
#endif

#if 0//_DEBUG_
	SCI_TRACE_LOW ("stream_len %d,stream_len_left %d,buffA :%x buffB:%x",g_image_ptr->bitstrm_ptr->stream_len,\
		g_image_ptr->bitstrm_ptr->stream_len_left,g_image_ptr->bitstrm_ptr->bufa,g_image_ptr->bitstrm_ptr->bufb);

	SCI_TRACE_LOW("g_stream_offset %d",g_stream_offset);
	SCI_TRACE_LOW("bitstream phy-addr %x,reg_value%x",H264Dec_ExtraMem_V2Phy(img_ptr->frame_bistrm_ptr),VSP_READ_REG(VSP_MEMO10_ADDR+8,""));

         if(55> g_nFrame_dec_h264)
         {
        	dump_output_yuv();
         }
#endif

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");

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
