/******************************************************************************
 ** File Name:    mp4dec_interface.c										  *
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
#include "vsp_mp4_dec.h"
#include "mp4dec_or_fw.h"
#include "mpeg4dec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

FunctionType_MallocCB VSP_mallocCb = NULL;

MP4DEC_SHARE_RAM s_mp4dec_share_ram;

/*************************************/
/* functions needed for android platform */
/*************************************/
PUBLIC void ARM_VSP_BIND(MP4Handle *mp4Handle)
{
    uint32 buffer_header;
    int32 buffer_num,i;
    buffer_num =  (VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x70,"bind_buffer_number")) >> 16;
    if(buffer_num)
    {
	buffer_header = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x6c,"bind_buffer_header");
        (*mp4Handle->VSP_bindCb)(mp4Handle->userdata,(void *)buffer_header, 0);
    }
}

PUBLIC void ARM_VSP_UNBIND(MP4Handle *mp4Handle)
{
    uint32 buffer_header;
    int32 buffer_num,i;
    buffer_num =  (VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x70,"unbind_buffer_number")) & 0xffff;
    for(i =0; i < buffer_num; i++)
    {
	buffer_header = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x74+i*4,"unbind_buffer_header");
        (*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,(void *)buffer_header, 0);
    }
}

PUBLIC void MP4DecSetPostFilter(MP4Handle *mp4Handle, int en)
{
	// Shark VSP DO NOT support post filter for MPEG4 decoder.
}

PUBLIC void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;

	share_ram->rec_buf_Y=  (uint32)pFrameY;

	share_ram->rec_buf_Y_addr= (uint32)pFrameY_phy;

	share_ram->rec_buf_header =(uint32) pBufferHeader;
	
}

void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle)
{

	int ret;
	OR_VSP_RST();

	// Send H264DEC_RELEASE signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_RELEASE,"Call  MP4DecReleaseRefBuffers function.");	

	OR_VSP_START();

	// Call unbind callback function. 
	 ARM_VSP_UNBIND(mp4Handle);

        VSP_RELEASE_Dev();

}

int MP4DecGetLastDspFrm(MP4Handle *mp4Handle,void **pOutput)
{
	int ret;
	OR_VSP_RST();

	// Send GetLastDspFrm signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_GETLAST,"Call  H264DEC_GETLAST function.");	

	ret = OR_VSP_START();	//ret  is buffer header.

	
	// Get bool value of H264Dec_GetLastDspFrm function of OR.
	// If output is TRUE. 
	//Get address of unbind buffer.
	// Call unbind callback function. 
	// else return NULL
	if(ret)
	{
		* pOutput = (void *)ret;
		 ARM_VSP_UNBIND(mp4Handle);
		 return TRUE;
	}
	else
	{
		* pOutput = NULL;
		return FALSE;
	}

    VSP_RELEASE_Dev();

}

PUBLIC MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer * buffer_ptr)
{
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;
	
	share_ram->malloc_mem1_start_addr 	= (uint32)buffer_ptr->common_buffer_ptr_phy;
	share_ram->total_mem1_size 		= buffer_ptr->size;
		
	OR_VSP_RST();

	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x10, share_ram->malloc_mem1_start_addr, "SHARE_RAM_BASE_ADDR 0x10: malloc_mem1_start_addr");
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x14,share_ram->total_mem1_size,"SHARE_RAM_BASE_ADDR 0x14: total_mem1_size");

	// Send MPEG4DEC_MEMINIT signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_MEMINIT,"Call  MP4DecMemInit function.");	

	OR_VSP_START();

        VSP_RELEASE_Dev();


  	return MMDEC_OK;
}

PUBLIC void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height)
{
//    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
//	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;

	 *display_width = share_ram->pic_width;
	 *display_height = share_ram->pic_height;

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height) 
{
//    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
//	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;
	
    	*width = ((share_ram->pic_width + 15) >> 4) << 4;
    	*height = ((share_ram->pic_height+15) >> 4) << 4;

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}


MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * buffer_ptr)
{
	MMDecRet ret;
	uint32 * OR_addr_vitual_ptr;
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;

     ALOGE ("%s, %d", __FUNCTION__, __LINE__);


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
	memcpy(OR_addr_vitual_ptr, mp4dec_code, MP4DEC_OR_DATA_SIZE);

	
	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}


	//Function related share ram configuration.
	share_ram->malloc_mem0_start_addr=MP4DEC_OR_INTER_START_ADDR;	// Addr in Openrisc space. 
	share_ram->total_mem0_size=MP4DEC_OR_INTER_MALLOC_SIZE;	
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+8, share_ram->malloc_mem0_start_addr,"shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc, share_ram->total_mem0_size,"shareRAM c CODE_RUN_SIZE");//OPENRISC text+heap+stack

//	share_ram->standard = STREAM_ID_MPEG4;//pVideoFormat->video_std;
//	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0, share_ram->standard, "SHARE_RAM_BASE_ADDR 0x0: [3:0] video standard.");

        share_ram->bs_used_len=0;
        VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, share_ram->bs_used_len,"shareRAM 0x20 stream_len");//			
	
	// Send MP4DecInit signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_INIT,"Call  MP4DecInit function.");		

	ret = (MMDecRet)OR_VSP_START();

        VSP_RELEASE_Dev();

	return ret;
}


PUBLIC MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr)
{
        MMDecRet ret = MMDEC_OK;
	    MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;

            ALOGE ("%s, %d, video_format_ptr->video_std: %d", __FUNCTION__, __LINE__, video_format_ptr->video_std);

	share_ram->standard = video_format_ptr->video_std;
	
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != STREAM_ID_MPEG4)
	{
		ALOGE ("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{
            uint32 img_size_reg;

            ALOGE ("%s, %d", __FUNCTION__, __LINE__);

            if(OR_VSP_RST()<0)
            {
                return MMDEC_HW_ERROR;
            }

            // Bitstream.
            share_ram->bs_start_addr=((uint32)video_format_ptr->p_extra_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned. 
            share_ram->bs_buffer_size= video_format_ptr->i_extra;
            share_ram->bs_used_len=0;

            VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x18, share_ram->bs_start_addr,"shareRAM 0x18 STREAM_BUF_ADDR");//
            VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x1c, share_ram->bs_buffer_size,"shareRAM 0x1c STREAM_BUF_SIZE");
            VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, share_ram->bs_used_len,"shareRAM 0x20 stream_len");//			
    	
    	    // Send MP4DecInit signal to Openrisc.
            VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_VOLHEADER,"Call  MP4DecVolHeader function.");		

            ret = (MMDecRet)OR_VSP_START();

	    share_ram->video_size_get = 1;	

            img_size_reg = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4, "image size");
            share_ram->pic_width =(img_size_reg) & 0xfff;
            share_ram->pic_height=(img_size_reg >>12) & 0xfff;	
            ALOGE ("%s, %d, img_size_reg: %0x, frame_width: %0x,frame_height: %0x ", __FUNCTION__, __LINE__, img_size_reg, share_ram->pic_width, share_ram->pic_height);
            
//            share_ram->pic_width = 160;
//             share_ram->pic_height = 120;

            video_format_ptr->frame_width  = share_ram->pic_width;
            video_format_ptr->frame_height = share_ram->pic_height;


#if 0
                {
                    uint32 debug_tmp;
                    int32 ii;

                    for (ii = 0; ii < 20; ii++)
                    {
                            debug_tmp = VSP_READ_REG(SHARE_RAM_BASE_ADDR+ 0xb0 + ii*4,"");
                            ALOGE ("%s, %d, %0x ", __FUNCTION__, __LINE__,debug_tmp);
                        }                                                                               
                    }
#endif        	
            VSP_RELEASE_Dev();

	    
            //tmp
            mp4Handle->VSP_extMemCb(mp4Handle->userdata,  video_format_ptr->frame_width, video_format_ptr->frame_height);
	}

	return ret;
}

int16 MP4DecDecodeShortHeader(mp4StreamType *psBits,
                         int32 *width,
                         int32 *height,
                         int32 *display_width,
                         int32 *display_height);

PUBLIC MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret;
	int32 i;
	uint32 img_size_reg;
	MP4DEC_SHARE_RAM *share_ram = &s_mp4dec_share_ram;


	if(!share_ram->video_size_get)
	{
		int32 width, height, display_width, display_height;
		mp4StreamType sBits;	
		sBits.data = dec_input_ptr->pStream;
		sBits.numBytes = dec_input_ptr->dataLen;
		sBits.bitBuf = 0;
		sBits.bitPos = 32;
		sBits.bytePos = 0;
		sBits.dataBitPos = 0;
		
		MP4DecDecodeShortHeader(&sBits,&width,&height,&display_width,&display_height);

		share_ram->video_size_get = 1;

                share_ram->pic_width = width;
                share_ram->pic_height = height;
  		
		//tmp
	        mp4Handle->VSP_extMemCb(mp4Handle->userdata,  width, height);

                return MMDEC_MEMORY_ALLOCED;	
	}

	if(OR_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}



	//Function related share ram configuration.		

	//expected_IVOP
	share_ram->expected_IVOP = (dec_input_ptr->expected_IVOP)? 1: 0;
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x0, (share_ram->expected_IVOP<<28) | (share_ram->standard),"shareRAM 0x0 bit 28: expected_IVOP");//expected_IVOP

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

	// Send MP4DecDecode signal to Openrisc.
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x58, MPEG4DEC_DECODE,"Call  MP4DecDecode function.");		

	// Get output of H264DecDecode function of OR.
	ret = (MMDecRet)OR_VSP_START();


	img_size_reg = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4, "image size");
	share_ram->pic_width =(img_size_reg) & 0xfff;
	share_ram->pic_height=(img_size_reg >>12) & 0xfff;
	share_ram->frameY_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x2c, "display frame_Y_addr");
	share_ram->frameUV_addr=VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x30, "display frame_UV_addr");
	share_ram->frame_output_en=(VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x4c, "display en"))&0x1;
        share_ram->pBufferHeader = (VSP_READ_REG(SHARE_RAM_BASE_ADDR+0xb4, "pBufferHeader"));

	dec_output_ptr->frameEffective = share_ram->frame_output_en;
	if(dec_output_ptr->frameEffective)
	{
	        dec_output_ptr->pBufferHeader = (void *)share_ram->pBufferHeader;
		dec_output_ptr->pOutFrameY = (uint8 *)share_ram->frameY_addr;
		dec_output_ptr->pOutFrameU = (uint8 *)share_ram->frameUV_addr;
		dec_output_ptr->frame_width = share_ram->pic_width;
		dec_output_ptr->frame_height = share_ram->pic_height;
	}

	 ARM_VSP_BIND(mp4Handle);
	 ARM_VSP_UNBIND(mp4Handle);

    VSP_RELEASE_Dev();

	// Return output.		
	return ret;







}

PUBLIC MMDecRet MPEG4_DecReleaseDispBfr(uint8 *pBfrAddr)
{
	return MMDEC_OK;
}

MMDecRet MP4DecRelease(MP4Handle *mp4Handle)
{
#ifndef _FPGA_TEST_
	VSP_CLOSE_Dev();
#endif
	return MMDEC_OK;
}


static const uint32 mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};


int16 ShowBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,          /* nr of bits to read */
    uint32 *pulOutData      /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;

    uint i;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3; /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        if (dataBytePos > pStream->numBytes - 4)
        {
            pStream->bitBuf = 0;
            for (i = 0; i < pStream->numBytes - dataBytePos; i++)
            {
                pStream->bitBuf |= pStream->data[dataBytePos+i];
                pStream->bitBuf <<= 8;
            }
            pStream->bitBuf <<= 8 * (3 - i);
        }
        else
        {
            bits = &pStream->data[dataBytePos];
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        pStream->bitPos = bitPos;
    }

    bitPos += ucNBits;

    *pulOutData = (pStream->bitBuf >> (32 - bitPos)) & mask[(uint16)ucNBits];


    return 0;
}

int16 FlushBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits                      /* number of bits to flush */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos, byteLeft;


    if ((dataBitPos + ucNBits) > (uint32)(pStream->numBytes << 3))
        return (-2); // Buffer over run

    dataBitPos += ucNBits;
    bitPos     += ucNBits;

    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        byteLeft = pStream->numBytes - dataBytePos; // Byte Lefts
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        if (byteLeft > 3)
        {
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        else
        {
            uint32 lShift = 24;
            uint32 tmpBuff = 0;
            while (byteLeft)
            {
                tmpBuff |= (*bits << lShift);
                bits++;
                byteLeft--;
                lShift -= 8;
            }
            pStream->bitBuf = tmpBuff;
        }
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 ReadBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,                     /* nr of bits to read */
    uint32 *pulOutData                 /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos, byteLeft;

    if ((dataBitPos + ucNBits) > (pStream->numBytes << 3))
    {
        *pulOutData = 0;
        return (-2); // Buffer over run
    }

    //  dataBitPos += ucNBits;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        byteLeft = pStream->numBytes - dataBytePos; // Byte Lefts
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        if (byteLeft > 3)
        {
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        else
        {
            uint32 lShift = 24;
            uint32 tmpBuff = 0;
            while (byteLeft)
            {
                tmpBuff |= (*bits << lShift);
                bits++;
                byteLeft--;
                lShift -= 8;
            }
            pStream->bitBuf = tmpBuff;
        }
    }

    pStream->dataBitPos += ucNBits;
    pStream->bitPos      = (unsigned char)(bitPos + ucNBits);

    *pulOutData = (pStream->bitBuf >> (32 - pStream->bitPos)) & mask[(uint16)ucNBits];

    return 0;
}


int16 MP4DecDecodeShortHeader(mp4StreamType *psBits,
                         int32 *width,
                         int32 *height,
                         int32 *display_width,
                         int32 *display_height)
{
    uint32 codeword;
    int32   extended_PTYPE = 0;
    int32 UFEP = 0;
    int32 custom_PFMT = 0;

ALOGI("%s, %d", __FUNCTION__, __LINE__);

    ShowBits(psBits, 22, &codeword);

    if (codeword !=  0x20)
    {
        return MP4_INVALID_VOL_PARAM;
    }
    FlushBits(psBits, 22);
    ReadBits(psBits, 8, &codeword);

    ReadBits(psBits, 1, &codeword);
    if (codeword == 0) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    /* source format */
    ReadBits(psBits, 3, &codeword);
    switch (codeword)
    {
        case 1:
            *width = 128;
            *height = 96;
            break;

        case 2:
            *width = 176;
            *height = 144;
            break;

        case 3:
            *width = 352;
            *height = 288;
            break;

        case 4:
            *width = 704;
            *height = 576;
            break;

        case 5:
            *width = 1408;
            *height = 1152;
            break;

        case 7:
            extended_PTYPE = 1;
            break;
        default:
            /* Msg("H.263 source format not legal\n"); */
            return MP4_INVALID_VOL_PARAM;
    }

    if (extended_PTYPE == 0)
    {
        *display_width = *width;
        *display_height = *height;
        return 0;
    }
    /* source format */
    ReadBits(psBits, 3, &codeword);
    UFEP = codeword;
    if (UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        switch (codeword)
        {
            case 1:
                *width = 128;
                *height = 96;
                break;

            case 2:
                *width = 176;
                *height = 144;
                break;

            case 3:
                *width = 352;
                *height = 288;
                break;

            case 4:
                *width = 704;
                *height = 576;
                break;

            case 5:
                *width = 1408;
                *height = 1152;
                break;

            case 6:
                custom_PFMT = 1;
                break;
            default:
                /* Msg("H.263 source format not legal\n"); */
                return MP4_INVALID_VOL_PARAM;
        }
        if (custom_PFMT == 0)
        {
            *display_width = *width;
            *display_height = *height;
            return 0;
        }
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 3, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;             /* RPS, ISD, AIV */
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 4, &codeword);
        if (codeword != 8) return MP4_INVALID_VOL_PARAM;
    }
    if (UFEP == 0 || UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        if (codeword > 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
    }
    else
    {
        return MP4_INVALID_VOL_PARAM;
    }
    ReadBits(psBits, 1, &codeword);
    if (codeword) return MP4_INVALID_VOL_PARAM; /* CPM */
    if (custom_PFMT == 1 && UFEP == 1)
    {
	uint32 CP_PAR_code;

        ReadBits(psBits, 4, &CP_PAR_code);
        if (CP_PAR_code == 0) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        *display_width = (codeword + 1) << 2;
        *width = (*display_width + 15) & -16;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        *display_height = codeword << 2;
        *height = (*display_height + 15) & -16;

	if (CP_PAR_code == 0xf)
        {
            ReadBits(psBits, 8, &codeword);
            ReadBits(psBits, 8, &codeword);
        }
    }

    return 0;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
