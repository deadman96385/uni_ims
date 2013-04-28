/******************************************************************************
 ** File Name:    mp4enc_interface.c										  *
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
#include "vsp_drv_sc8830.h"
#include "vsp_h264_enc.h"
#include "h264enc_or_fw.h"
#include "h264enc.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

FunctionType_BufCB VSP_bindCb = NULL;
FunctionType_BufCB VSP_unbindCb = NULL;
void *g_user_data = NULL;
FunctionType_MallocCB VSP_mallocCb = NULL;

H264ENC_SHARE_RAM s_h264enc_share_ram;

/*************************************/
/* functions needed for android platform */
/*************************************/
PUBLIC void ARM_VSP_BIND()
{
    uint32 buffer_header;
    int32 buffer_num,i;
    buffer_num =  (VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x70,"bind_buffer_number")) >> 16;
    if(buffer_num)
    {
	buffer_header = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x6c,"bind_buffer_header");
        (*VSP_bindCb)(g_user_data,(void *)buffer_header);
    }
}

PUBLIC void ARM_VSP_UNBIND()
{
    uint32 buffer_header;
    int32 buffer_num,i;
    buffer_num =  (VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x70,"unbind_buffer_number")) & 0xffff;
    for(i =0; i < buffer_num; i++)
    {
	buffer_header = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x74+i*4,"unbind_buffer_header");
        (*VSP_unbindCb)(g_user_data,(void *)buffer_header);
    }
}

/*****************************************************************************/
//  Description:   Set  H264 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncSetConf(MMEncConfig *pConf)
{
	H264ENC_SHARE_RAM *share_ram = &s_h264enc_share_ram;

	share_ram->h264enc_FrameRate			= pConf->FrameRate;	
	share_ram->target_bitrate				= pConf->targetBitRate;
	share_ram->rate_ctr_en				= pConf->RateCtrlEnable;
	
	share_ram->h264enc_StepI				= pConf->QP_IVOP;
	share_ram->h264enc_StepP				= pConf->QP_PVOP;

	OR_VSP_RST();		
	
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x38, (share_ram->rate_ctr_en <<31)|share_ram->target_bitrate,"SHARE_RAM_BASE_ADDR 0x38: targetBitRate");

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x60, (share_ram->h264enc_StepP <<24)|(share_ram->h264enc_StepI<<16),"config");
	
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x64, (share_ram->h264enc_FrameRate),"SHARE_RAM_BASE_ADDR 0x64:h264enc_FrameRate ");


	// Send H264ENC_SETCONF signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264ENC_SETCONF,"Call  H264EncSetConf function.");	

	OR_VSP_START();	

        VSP_RELEASE_Dev();

	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get H264 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncGetConf(MMEncConfig *pConf)
{
	H264ENC_SHARE_RAM *share_ram = &s_h264enc_share_ram;
	
	OR_VSP_RST();	

	// Send H264ENC_GETCONF signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264ENC_GETCONF,"Call  H264EncGetConf function.");	

	OR_VSP_START();	

	{
	uint32 tmp;	
	
	tmp = VSP_READ_REG(SHARE_RAM_BASE_ADDR + 0x38, "SHARE_RAM_BASE_ADDR 0x38: targetBitRate");
	share_ram->rate_ctr_en = tmp >>31;
	share_ram->target_bitrate =( tmp & 0x7fffffff);

	tmp = VSP_READ_REG(SHARE_RAM_BASE_ADDR + 0x60, "config");
	share_ram->h264enc_StepI				= (tmp >>16)&0Xff;
	share_ram->h264enc_StepP				= (tmp >>24)&0xff;

	share_ram->h264enc_FrameRate			= VSP_READ_REG(SHARE_RAM_BASE_ADDR + 0x64, "SHARE_RAM_BASE_ADDR 0x64:h264enc_FrameRate ");

	}

	pConf->QP_IVOP					 =share_ram->h264enc_StepI;
	pConf->QP_PVOP					 = share_ram->h264enc_StepP;
	
	pConf->targetBitRate 				= share_ram->target_bitrate;
	pConf->FrameRate 				=share_ram->h264enc_FrameRate;	
	pConf->RateCtrlEnable 			= share_ram->rate_ctr_en; 
	
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncRelease(void)
{
#ifndef _FPGA_TEST_
	VSP_CLOSE_Dev();
#endif
	return MMENC_OK;
}

void H264Enc_close(void)
{
	return;
}



/*****************************************************************************/
//  Description:   Init h264 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
	MMDecRet ret;
	uint32 * OR_addr_vitual_ptr;
	H264ENC_SHARE_RAM *share_ram = &s_h264enc_share_ram;

	OR_addr_ptr = (uint32 *)(pInterMemBfr->common_buffer_ptr_phy);
	OR_addr_vitual_ptr = (uint32 *)(pInterMemBfr->common_buffer_ptr);
	
#ifndef _FPGA_TEST_
	// Open VSP device
	if(VSP_OPEN_Dev()<0)
	{
		return MMDEC_HW_ERROR;
	}	
#else
	TEST_VSP_ENABLE();	
#endif

	//Load firmware to ddr
	memcpy(OR_addr_vitual_ptr, h264enc_code, H264ENC_OR_DATA_SIZE);

	
	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}


	//Function related share ram configuration.
	share_ram->malloc_mem0_start_addr=H264ENC_OR_INTER_START_ADDR;	// Addr in Openrisc space. 
	share_ram->total_mem0_size=H264ENC_OR_INTER_MALLOC_SIZE;	
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+8, share_ram->malloc_mem0_start_addr,"shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc, share_ram->total_mem0_size,"shareRAM c CODE_RUN_SIZE");//OPENRISC text+heap+stack

	
	share_ram->malloc_mem1_start_addr = (uint32 )(pExtaMemBfr->common_buffer_ptr_phy);
	share_ram->total_mem1_size= pExtaMemBfr->size;
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x10, share_ram->malloc_mem1_start_addr,"shareRAM 0x10: VSP_MEM1_ST_ADDR");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x14, share_ram->total_mem1_size,"shareRAM 0x14: VSP_MEM1_SIZE");
	

	share_ram->bs_start_addr = (uint32 )pBitstreamBfr->common_buffer_ptr;
	share_ram->bs_buffer_size=pBitstreamBfr->size;
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x18, (uint32 )pBitstreamBfr->common_buffer_ptr_phy,"shareRAM 0x18: STREAM_BUF_ADDR");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x1c, share_ram->bs_buffer_size,"shareRAM 0x1c: bs_buffer_size");
	

	share_ram->standard = STREAM_ID_H264;
	share_ram->b_anti_shake = 0; // DISABLE anti-shark.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+ 0x0, (share_ram->b_anti_shake <<17)|(share_ram->standard),"SHARE_RAM_BASE_ADDR 0x0: config.");

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+ 0x4, (pVideoFormat->frame_height <<12)|(pVideoFormat->frame_width),"SHARE_RAM_BASE_ADDR 0x4: image size");


	// Send H264ENC_INIT signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264ENC_INIT,"Call  H264EncInit function.");		

	ret = (MMDecRet)OR_VSP_START();

        VSP_RELEASE_Dev();
	
	return ret;
}

/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncStrmEncode(MMEncIn *pInput, MMEncOut *pOutput)
{
	H264ENC_SHARE_RAM *share_ram = &s_h264enc_share_ram;
	
	OR_VSP_RST();	


	//Function related share ram configuration.
	share_ram->frameY_addr = (uint32)pInput->p_src_y_phy;
	share_ram->frameUV_addr = (uint32)pInput->p_src_u_phy;
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c,share_ram->frameY_addr,"SHARE_RAM_BASE_ADDR 0x2c:frameY_addr " );
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30,share_ram->frameUV_addr,"SHARE_RAM_BASE_ADDR 0x30:frameUV_addr " );
	

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x68,pInput->vopType, "SHARE_RAM_BASE_ADDR 0x68: h264enc_vopType");
	

	// Send H264ENC_ENCODE signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264ENC_ENCODE,"Call  H264EncStrmEncode function.");	

	OR_VSP_START();	

	share_ram->bs_used_len =  VSP_READ_REG(SHARE_RAM_BASE_ADDR +0x20, "SHARE_RAM_BASE_ADDR 0x20:bs_used_len ");

	pOutput->pOutBuf =  (uint8 *)share_ram->bs_start_addr ;
	pOutput->strmSize = share_ram->bs_used_len;

        VSP_RELEASE_Dev();
				
	return MMENC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
