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
#include "vsp_vp8_dec.h"
#include "vp8dec_or_fw.h"
#include "vp8dec.h"
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

VP8DEC_SHARE_RAM s_vp8dec_share_ram;

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



PUBLIC void VP8Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	VP8DEC_SHARE_RAM *share_ram = &s_vp8dec_share_ram;

	share_ram->rec_buf_Y=  (uint32)pFrameY;

	share_ram->rec_buf_Y_addr= (uint32)pFrameY_phy;

	share_ram->rec_buf_header =(uint32) pBufferHeader;
	
}

void VP8Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata)
{
	VSP_bindCb = bindCb;
	VSP_unbindCb = unbindCb;
	g_user_data = userdata;
}


void VP8Dec_RegMallocCB(FunctionType_MallocCB mallocCb)
{
	VSP_mallocCb = mallocCb;
}


MMDecRet VP8DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret;
	uint32 * OR_addr_vitual_ptr;
	VP8DEC_SHARE_RAM *share_ram = &s_vp8dec_share_ram;

	OR_addr_ptr = (uint32 *)(buffer_ptr->common_buffer_ptr_phy);
	OR_addr_vitual_ptr = (uint32 *)(buffer_ptr->common_buffer_ptr);
	
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
	memcpy(OR_addr_vitual_ptr, vp8dec_code, VP8DEC_OR_DATA_SIZE);

	
	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}


	//Function related share ram configuration.
	share_ram->malloc_mem0_start_addr=VP8DEC_OR_INTER_START_ADDR;	// Addr in Openrisc space. 
	share_ram->total_mem0_size=VP8DEC_OR_INTER_MALLOC_SIZE;	
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+8, share_ram->malloc_mem0_start_addr,"shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc, share_ram->total_mem0_size,"shareRAM c CODE_RUN_SIZE");//OPENRISC text+heap+stack

	
	// Send VP8DEC_INIT signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, VP8DEC_INIT,"Call  VP8DEC_INIT function.");		

	ret = (MMDecRet)OR_VSP_START();

	VSP_RELEASE_Dev();
	
	return ret;
}




MMDecRet VP8DecHeader(MMDecInput *dec_input_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret;
	int32 i;
	uint32 img_size_reg;
	VP8DEC_SHARE_RAM *share_ram = &s_vp8dec_share_ram;

	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}

	// Bitstream.
	share_ram->bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
	share_ram->bs_buffer_size=dec_input_ptr->dataLen;
	share_ram->bs_used_len=0;

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x18, share_ram->bs_start_addr,"shareRAM 0x18 STREAM_BUF_ADDR");//
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x1c, share_ram->bs_buffer_size,"shareRAM 0x1c STREAM_BUF_SIZE");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, share_ram->bs_used_len,"shareRAM 0x20 stream_len");//	



	// Send VP8DEC_DECODE signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, VP8DEC_HEADER,"Call  VP8DEC_DECODE function.");		

	// Get output of VP8DEC_DECODE function of OR.
	ret = (MMDecRet)OR_VSP_START();

	if(ret)
		return ret;


	img_size_reg = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4, "image size");
	share_ram->pic_width =(img_size_reg) & 0xfff;
	share_ram->pic_height=(img_size_reg >>12) & 0xfff;

	
	pVideoFormat->frame_width = share_ram->pic_width;
	pVideoFormat->frame_height = share_ram->pic_height;

	VSP_RELEASE_Dev();

 
	// Return output.		
	return ret;

}

LOCAL MMDecRet VP8DecFrameHeader(uint8 * bs_ptr)
{
	uint8 * data = bs_ptr;
	int32 frame_type;
	VP8DEC_SHARE_RAM *share_ram = &s_vp8dec_share_ram;
	
	 frame_type = (data[0] & 1);

   	 data += 3;

	  if (frame_type  == 0) // 0: KEY_FRAME
	 {
       		 int32 Width = share_ram->pic_width;
        	 int32 Height = share_ram->pic_height;


        	// vet via sync code
       		 if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
		{
			return MMDEC_STREAM_ERROR ;
		}

        	share_ram->pic_width = (data[3] | (data[4] << 8)) & 0x3fff;
      		 // pc->horiz_scale = data[4] >> 6;
       		share_ram->pic_height = (data[5] | (data[6] << 8)) & 0x3fff;
       		// pc->vert_scale = data[6] >> 6;
  	        data += 7;

		 if ( (Width !=share_ram->pic_width) || (Height !=share_ram->pic_height) )	// Resolution Changed
		 {
	        	 if (share_ram->pic_width <= 0)
	            	{
	             		 share_ram->pic_width = Width;
				return MMDEC_PARAM_ERROR ;
			 }

	  		 if (share_ram->pic_height <= 0)
	            	{
	                	share_ram->pic_height = Height;
				return MMDEC_PARAM_ERROR ;
			}
			
			return 	MMDEC_MEMORY_ALLOCED;
		 }     
   	 }

	  return MMDEC_OK;
}

PUBLIC MMDecRet VP8DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret;
	int32 i;
	uint32 img_size_reg;
	VP8DEC_SHARE_RAM *share_ram = &s_vp8dec_share_ram;

	ret =  VP8DecFrameHeader(dec_input_ptr->pStream);

	if(MMDEC_OK !=ret)
		return ret;

	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}

	// Bitstream.
	share_ram->bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
	share_ram->bs_buffer_size=dec_input_ptr->dataLen;
	share_ram->bs_used_len=0;

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x18, share_ram->bs_start_addr,"shareRAM 0x18 STREAM_BUF_ADDR");//
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x1c, share_ram->bs_buffer_size,"shareRAM 0x1c STREAM_BUF_SIZE");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, share_ram->bs_used_len,"shareRAM 0x20 stream_len");//	

	// Rec Buffer.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60,share_ram->rec_buf_Y,"shareRame 0x60: rec buffer virtual address.");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x64,share_ram->rec_buf_Y_addr,"shareRame 0x64: rec buffer physical address.");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x68,share_ram->rec_buf_header,"shareRame 0x68: rec buffer header.");

	// Send VP8DEC_DECODE signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, VP8DEC_DECODE,"Call  VP8DEC_DECODE function.");		

	// Get output of VP8DEC_DECODE function of OR.
	ret = (MMDecRet)OR_VSP_START();


	img_size_reg = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4, "image size");
	share_ram->pic_width =(img_size_reg) & 0xfff;
	share_ram->pic_height=(img_size_reg >>12) & 0xfff;
	share_ram->frameY_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x2c, "display frame_Y_addr");
	share_ram->frameUV_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x30, "display frame_UV_addr");
	share_ram->frame_output_en=(VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4c, "display en"))&0x1;

	dec_output_ptr->frameEffective = share_ram->frame_output_en;
	if(dec_output_ptr->frameEffective)
	{
		dec_output_ptr->pOutFrameY = (uint8 *)share_ram->frameY_addr;
		dec_output_ptr->pOutFrameU = (uint8 *)share_ram->frameUV_addr;
		dec_output_ptr->frame_width = share_ram->pic_width;
		dec_output_ptr->frame_height = share_ram->pic_height;
	}

	 ARM_VSP_BIND();
	 ARM_VSP_UNBIND();

	 VSP_RELEASE_Dev();

 
	// Return output.		
	return ret;
}



MMDecRet VP8DecRelease(void)
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
