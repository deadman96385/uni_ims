/******************************************************************************
 ** File Name:    h264dec_interface.c                                         *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vsp_drv_sc8830.h"
#include "vsp_h264_dec.h"
#include "h264dec_or_fw.h"
#include "h264dec.h"
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

H264DEC_SHARE_RAM s_h264dec_share_ram;

PUBLIC void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId)
{
    H264DEC_SHARE_RAM *share_ram = &s_h264dec_share_ram;

    share_ram->rec_buf_Y=  (uint32)pFrameY;
    share_ram->rec_buf_Y_addr= (uint32)pFrameY_phy;
    share_ram->rec_buf_header =(uint32) pBufferHeader;	
}

//void H264Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata)
//{
//    VSP_bindCb = bindCb;
//    VSP_unbindCb = unbindCb;
//    g_user_data = userdata;
//}

//void H264Dec_RegMallocCB(FunctionType_MallocCB mallocCb)
//{
//    VSP_mallocCb = mallocCb;
//}

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

void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    int ret;
    
    OR_VSP_RST();

    // Send H264DEC_RELEASE signal to Openrisc.
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264DEC_RELEASE,"Call  H264Dec_ReleaseRefBuffers function.");	

    OR_VSP_START();

    // Call unbind callback function. 
    ARM_VSP_UNBIND();

    VSP_RELEASE_Dev();
}

MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, uint8 **pOutput, int32 *picId)
{
    int ret;
    
    OR_VSP_RST();

    // Send GetLastDspFrm signal to Openrisc.
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264DEC_GETLAST,"Call  H264DEC_GETLAST function.");	

    ret = OR_VSP_START();	//ret  is buffer header.

    // Get bool value of H264Dec_GetLastDspFrm function of OR.
    // If output is TRUE. 
    //Get address of unbind buffer.
    // Call unbind callback function. 
    // else return NULL
    if(ret)
    {
	* pOutput = (void *)ret;
	 ARM_VSP_UNBIND();

             VSP_RELEASE_Dev();

	 return MMDEC_OK;
    }else
    {
	* pOutput = NULL;

            VSP_RELEASE_Dev();

	return MMDEC_ERROR;
    }
}

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
{
    int forbidden_zero_bit;

//    SCI_TRACE_LOW("%s, %d, data: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x", __FUNCTION__, __LINE__, 
//		bitstream[0],bitstream[1],bitstream[2],bitstream[3],bitstream[4],bitstream[5],bitstream[6],bitstream[7],bitstream[8]);

    if (size > 0)
    {
        forbidden_zero_bit = bitstream[0] >> 7;
        if (forbidden_zero_bit != 0)
            return MMDEC_ERROR;
        *nal_ref_idc = (bitstream[0] & 0x60) >> 5;
        *nal_type = bitstream[0] & 0x1F;
        return MMDEC_OK;
    }

    return MMDEC_ERROR;
}

MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo)
{
#if 1
    H264DEC_SHARE_RAM *share_ram = &s_h264dec_share_ram;

//    	AVCDecObject *vd = (AVCDecObject *) avcHandle->videoDecoderData;
//        DEC_SPS_T *sps_ptr = &(vd->g_sps_array_ptr[0]);
//        DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[0]);

//    storage_t *pStorage;

//    SCI_TRACE_LOW("H264DecGetInfo#");

    if (/*decInst == NULL ||*/ pDecInfo == NULL)
    {
        ALOGI("H264SwDecGetInfo# ERROR: decInst or pDecInfo is NULL");
        return(MMDEC_PARAM_ERROR);
    }


//    pStorage = &(((decContainer_t *)decInst)->storage);

//    LOGI("%s, %d, g_active_sps_ptr: %0x, g_active_pps_ptr: %0x", __FUNCTION__, __LINE__, g_active_sps_ptr, g_active_pps_ptr);

//    if (pStorage->activeSps == NULL || pStorage->activePps == NULL)
//    if (sps_ptr == NULL /*|| g_active_pps_ptr == NULL*/)
//    {
//        ALOGI("H264SwDecGetInfo# ERROR: Headers not decoded yet");
//        return(MMDEC_ERROR);
//    }

#ifdef H264DEC_TRACE
    sprintf(((decContainer_t*)decInst)->str,
        "H264SwDecGetInfo# decInst %p  pDecInfo %p", decInst, (void*)pDecInfo);
    ALOGI(((decContainer_t*)decInst)->str);
#endif

    /* h264bsdPicWidth and -Height return dimensions in macroblock units,
     * picWidth and -Height in pixels */
    pDecInfo->picWidth        = share_ram->pic_width;
    pDecInfo->picHeight       = share_ram->pic_height;
//    pDecInfo->videoRange      = h264bsdVideoRange(pStorage);
//    pDecInfo->matrixCoefficients = h264bsdMatrixCoefficients(pStorage);

#if 1
//    h264bsdCroppingParams(pStorage,
//        &pDecInfo->croppingFlag,
//        &pDecInfo->cropParams.cropLeftOffset,
//        &pDecInfo->cropParams.cropOutWidth,
//        &pDecInfo->cropParams.cropTopOffset,
//      &pDecInfo->cropParams.cropOutHeight);
        OR_VSP_RST();

        // Send H264DecInit signal to Openrisc.
        VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264DEC_CROP_PARAM,"Call  H264DEC_CROP_PARAM function.");		

        OR_VSP_START();

        pDecInfo->croppingFlag = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xb4, "croppingFlag");
        pDecInfo->cropParams.cropLeftOffset = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xb8, "leftOffset");
        pDecInfo->cropParams.cropOutWidth = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xbc, "width");
        pDecInfo->cropParams.cropTopOffset = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xc0, "topOffset");
        pDecInfo->cropParams.cropOutHeight = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xc4, "height");
           
        VSP_RELEASE_Dev();

    /* sample aspect ratio */
//    h264bsdSampleAspectRatio(pStorage,
//                             &pDecInfo->parWidth,
//                             &pDecInfo->parHeight);
#endif
    /* profile */
//    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);

//    SCI_TRACE_LOW("H264DecGetInfo# OK");

#else
    pDecInfo->picWidth        = 320;
    pDecInfo->picHeight       = 240;
     pDecInfo->profile = 0x42;
#endif

    return(MMDEC_OK);

}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    MMDecRet ret;
    uint32 * OR_addr_vitual_ptr;
    H264DEC_SHARE_RAM *share_ram = &s_h264dec_share_ram;

    ALOGE("%s, %d", __FUNCTION__, __LINE__);

VSP_mallocCb = avcHandle->VSP_extMemCb;
VSP_bindCb = avcHandle->VSP_bindCb;
VSP_unbindCb = avcHandle->VSP_unbindCb;
g_user_data = avcHandle->userdata;

    OR_addr_ptr = (uint32 *)(buffer_ptr->common_buffer_ptr_phy);
    OR_addr_vitual_ptr = (uint32 *)(buffer_ptr->common_buffer_ptr);
	
#ifndef _FPGA_TEST_
    if(VSP_OPEN_Dev()<0)
    {
	return MMDEC_HW_ERROR;
    }	
#endif

    //Load firmware to ddr
    memcpy(OR_addr_vitual_ptr, H264dec_OR_data, H264dec_OR_DATA_SIZE);

    if(OR_VSP_RST()<0)
    {
	return MMDEC_HW_ERROR;
    }

    //Function related share ram configuration.
    share_ram->malloc_mem0_start_addr=H264DEC_OR_INTER_START_ADDR;	// Addr in Openrisc space. 
    share_ram->total_mem0_size=H264DEC_OR_INTER_MALLOC_SIZE;	
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+8, share_ram->malloc_mem0_start_addr,"shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc, share_ram->total_mem0_size,"shareRAM c CODE_RUN_SIZE");//OPENRISC text+heap+stack

    // Set video_buffer_malloced to 0 before decoding.
    share_ram->video_buffer_malloced = 0;
    share_ram->video_size_got = 0;
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x0, (share_ram->video_buffer_malloced) <<16, "shareRAM 0 video_buffer_malloced");
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, (share_ram->video_size_got) <<31, "shareRAM 4 video_size_get");
		
    // Send H264DecInit signal to Openrisc.
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264DEC_INIT,"Call  H264DEC_INIT function.");		

    ret = (MMDecRet)OR_VSP_START();

        VSP_RELEASE_Dev();
	
    return ret;
}

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret;
    int32 i;
    uint32 img_size_reg;
    H264DEC_SHARE_RAM *share_ram = &s_h264dec_share_ram;

    if(OR_VSP_RST()<0)
    {
	return MMDEC_HW_ERROR;
    }

    //Function related share ram configuration.		
    if(share_ram->video_size_got && !share_ram->video_buffer_malloced)
    {
	//Send direct mb info address to OR.
	for( i =0; i<17; i++)
	{
	    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x70 + i*4, share_ram->direct_mb_info_addr[i], "direct_mb_info_addr");
        }

	share_ram->video_buffer_malloced = 1;
    }

    //expected_IVOP
    share_ram->expected_IVOP = (dec_input_ptr->expected_IVOP)? 1: 0;
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x0, (share_ram->expected_IVOP<<28)|(share_ram->video_buffer_malloced<<16),"shareRAM 0x0 bit 28: expected_IVOP");//expected_IVOP

    // Bitstream.
    share_ram->bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
    share_ram->bs_buffer_size=dec_input_ptr->dataLen;
    share_ram->bs_used_len=0;

    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x18, share_ram->bs_start_addr,"shareRAM 0x18 STREAM_BUF_ADDR");//
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x1c, share_ram->bs_buffer_size,"shareRAM 0x1c STREAM_BUF_SIZE");
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, share_ram->bs_used_len,"shareRAM 0x20 stream_len");//	

    // Rec Buffer.
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60, share_ram->rec_buf_Y,"shareRame 0x60: rec buffer virtual address.");
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x64, share_ram->rec_buf_Y_addr,"shareRame 0x64: rec buffer physical address.");
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x68, share_ram->rec_buf_header,"shareRame 0x68: rec buffer header.");

    // Send H264DecDecode signal to Openrisc.
    VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, H264DEC_DECODE,"Call  H264DecDecode function.");		

    // Get output of H264DecDecode function of OR.
    ret = (MMDecRet)OR_VSP_START();

    img_size_reg = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4, "image size");
    share_ram->pic_width =(img_size_reg) & 0xfff;
    share_ram->pic_height=(img_size_reg >>12) & 0xfff;
    share_ram->video_size_got = (img_size_reg>>31) & 0x1;
    share_ram->frameY_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x2c, "display frame_Y_addr");
    share_ram->frameUV_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x30, "display frame_UV_addr");
    share_ram->frame_output_en=(VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4c, "display en"))&0x1;

    dec_output_ptr->frameEffective = share_ram->frame_output_en;
 //   if(dec_output_ptr->frameEffective)
    {
	dec_output_ptr->pOutFrameY = (uint8 *)share_ram->frameY_addr;
	dec_output_ptr->pOutFrameU = (uint8 *)share_ram->frameUV_addr;
	dec_output_ptr->frame_width = share_ram->pic_width;
	dec_output_ptr->frame_height = share_ram->pic_height;

ALOGE("%s, %d", __FUNCTION__, __LINE__);
ALOGE("pic_width : %d, pic_height :%d", share_ram->pic_width,share_ram->pic_height);
    }

    ARM_VSP_BIND();
    ARM_VSP_UNBIND();

    if(share_ram->video_size_got && !share_ram->video_buffer_malloced)
    {
        // Malloc direct mb info buffers
	uint32 malloc_buffer_num;
	uint32 malloc_buffer_size;

	malloc_buffer_num = VSP_READ_REG(SHARE_RAM_BASE_ADDR+ 0x10, "Number of direct mb info buffer when parsing sps. W by or");
	malloc_buffer_size = VSP_READ_REG(SHARE_RAM_BASE_ADDR+ 0x14,"buffer size of each direct mb info buffer when parsing sps. W by or.");

    ALOGE("%s, %d", __FUNCTION__, __LINE__);

	VSP_mallocCb (g_user_data, share_ram->direct_mb_info_addr, malloc_buffer_num, malloc_buffer_size);
    }

            VSP_RELEASE_Dev();
 
    // Return output.		
    return ret;
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(AVCHandle *avcHandle, uint8 *pBfrAddr)
{
    return MMDEC_OK;
}

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
#ifndef _FPGA_TEST_
    VSP_CLOSE_Dev();
#endif
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
