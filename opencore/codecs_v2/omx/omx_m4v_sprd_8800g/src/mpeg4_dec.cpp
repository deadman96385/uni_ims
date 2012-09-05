/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#include "mpeg4_dec.h"
#include "oscl_mem.h"
#include "omx_mpeg4_component.h"

#if PROFILING_ON
#include "oscl_tickcount.h"
#endif

#define MAX_LAYERS 1
#define PVH263DEFAULTHEIGHT 144
#define PVH263DEFAULTWIDTH 176

#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

#include <utils/threads.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <binder/MemoryHeapBase.h>

#include "mmcodec.h"
#include "mpeg4dec.h"
#include "video_common.h"
#if !defined(CHIP_8810)	
#include "vsp_drv_sc8800g.h"
#else
#include "vsp_drv_sc8810.h"
#endif
#include "mp4_basic.h"
// from m4v_config_parser.h
OSCL_IMPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *, int32 *);


//using android::sp;
using namespace android;

void little_to_big_endian(char *p,unsigned int  count)
{
       unsigned int *pInt =(unsigned int *) p;
       unsigned int value;   
       for(int i=0;i<(count+3)/4;i++)
	{
		value = pInt[i];
		value = ((value&0xff)<<24)|((value&0xff00)<<8)|((value&0xff0000)>>8)|((value&0xff000000)>>24);
		 pInt[i] = value;
       	}
}

	
int VSP_reset_cb(int fd)
{
//	SCI_TRACE_LOW("VSP_reset_cb\n");
//	ioctl(fd,VSP_ENABLE,NULL); //@remove by simon.wang @ 2012.07.12
	return 0;
}

static int VSP_Start_CQM_cb(int fd)
{
	SCI_TRACE_LOW("VSP_Start_CQM_cb: E\n");
	ioctl(fd,VSP_START,NULL);
	SCI_TRACE_LOW("VSP_Start_CQM_cb: X\n");
	return 0;
}

static int VSP_Acquaire_cb(int fd)
{
	int ret ;
//	SCI_TRACE_LOW("VSP_Acquaire_cb: E\n");

   
	ret =  ioctl(fd,VSP_ACQUAIRE,NULL);
	if(ret){
		SCI_TRACE_LOW("mp4dec VSP hardware timeout try again %d\n",ret);	
		ret =  ioctl(fd,VSP_ACQUAIRE,NULL);
		if(ret){
   			 SCI_TRACE_LOW("mp4dec VSP hardware timeout give up %d\n",ret);
		 	 return 1;
			}		 
	}	
	ioctl(fd,VSP_ENABLE,NULL);
	ioctl(fd,VSP_RESET,NULL);	 
//	SCI_TRACE_LOW("VSP_Acquaire_cb: X\n");
	return 0;
}
 

int VSP_bind_cb(void *userdata, void *pHeader,int flag)
{
	//SCI_TRACE_LOW("VSP DPB::VSP_bind_cb %x,%x\n",((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader);
	Mpeg4Decoder_OMX *pMpeg4Decoder_OMX = (Mpeg4Decoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
        pBCTRL->iRefCount++;
	return 0;
}

int VSP_unbind_cb(void *userdata, void *pHeader,int flag)
{
	//SCI_TRACE_LOW("VSP DPB::VSP_unbind_cb %x,%x\n",((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader);
	Mpeg4Decoder_OMX *pMpeg4Decoder_OMX = (Mpeg4Decoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
        pBCTRL->iRefCount--;
	if ( pBCTRL->iRefCount == 0)
        {
            pMpeg4Decoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers++;
	    if(OMX_FALSE == pBCTRL->iIsBufferInComponentQueue)
	    {
	        //SCI_TRACE_LOW("VSP_unbind_cb  OMX_FALSE == pBCTRL->iIsBufferInComponentQueue %x\n",pHeader);
	    	QueueType *pOutputQueue = pMpeg4Decoder_OMX->ipOMXComponent->GetOutputQueue();
		Queue(pOutputQueue,(void *)pHeader);
		pBCTRL->iIsBufferInComponentQueue = OMX_TRUE;
	    }
            // posssibly reschedule the component?
        }
	return 0;
}

void Mpeg4Decoder_OMX::ReleaseReferenceBuffers()
{
	SCI_TRACE_LOW("Mpeg4Decoder_OMX::ReleaseReferenceBuffers\n");
	MP4DecReleaseRefBuffers();
}

OMX_BOOL Mpeg4Decoder_OMX::FlushOutput_OMX( OMX_BUFFERHEADERTYPE **aOutBufferForRendering)
{ 
	SCI_TRACE_LOW("Mpeg4Decoder_OMX::FlushOutput_OMX\n"); 
	return (OMX_BOOL)MP4DecGetLastDspFrm((void **)aOutBufferForRendering);
}

void Mpeg4Decoder_OMX::ResetDecoder()
{
	SCI_TRACE_LOW("Mpeg4Decoder_OMX::ResetDecoder\n");
	if(iVsp_fd>=0)
		VSP_reset_cb(iVsp_fd);
	ReleaseReferenceBuffers();
}
	

Mpeg4Decoder_OMX::Mpeg4Decoder_OMX(OmxComponentBase *pComp)
{
    SCI_TRACE_LOW("Mpeg4Decoder_OMX::Mpeg4Decoder_OMX\n");
    ipOMXComponent = pComp;
    CodecMode = MPEG4_MODE; // this can change
    iReferenceYUVWasSet = OMX_FALSE;
    ipRefCtrPreviousReferenceBuffer = NULL;
    Mpeg4InitCompleteFlag = OMX_FALSE;
    iExternalBufferWasSet = OMX_FALSE;
    iStreamBufferWasSet = OMX_FALSE;

	iSkipToIDR = OMX_TRUE;
	
    iFrameSize = 0;
    iDisplay_Width = 0;
    iDisplay_Height = 0;

    VO_START_CODE1[0] = 0x00;
    VO_START_CODE1[1] = 0x00;
    VO_START_CODE1[2] = 0x01;
    VO_START_CODE1[3] = 0x00;

    VOSH_START_CODE1[0] = 0x00;
    VOSH_START_CODE1[1] = 0x00;
    VOSH_START_CODE1[2] = 0x01;
    VOSH_START_CODE1[3] = 0xB0;

    VOP_START_CODE1[0] = 0x00;
    VOP_START_CODE1[1] = 0x00;
    VOP_START_CODE1[2] = 0x01;
    VOP_START_CODE1[3] = 0xB6;

    H263_START_CODE1[0] = 0x00;
    H263_START_CODE1[1] = 0x00;
    H263_START_CODE1[2] = 0x80;

#if PROFILING_ON
    iTotalTicks = 0;
#endif
    iDecoder_int_buffer_ptr = NULL;
    iDecoder_ext_cache_buffer_ptr = NULL;
	
    iVsp_fd = -1;
    iVsp_addr = NULL;

    iBufferAllocFail = OMX_FALSE;	

    iDecoder_sw_flag = 0;
}


/* Initialization routine */
OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecInit()
{
    SCI_TRACE_LOW("Mpeg4Decoder_OMX::Mp4DecInit\n");
    Mpeg4InitCompleteFlag = OMX_FALSE;
    return OMX_ErrorNone;
}


void dump_yuv( OMX_U8* pBuffer,OMX_U32 aInBufSize)
{
	FILE *fp = fopen("/data/video.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}
/*
#include <stdio.h>
#include <stdarg.h>
int g_frame_num =0;
extern "C"{
void  SCI_PRINTF(const char* format,...)
{
    FILE *log_fp;
    va_list args;

if(g_frame_num>=3)
	return;
    va_start(args, format);

 
    log_fp = fopen("/data/vsp_log.txt", "a+");
    if (log_fp == NULL)
	{
		printf("vsp can not open log file\n");
		return;
    	}

    vfprintf(log_fp, format, args);
    fclose(log_fp);
 
    va_end(args);
	
}
}
*/

extern int g_mpeg4_dec_err_flag;

OMX_BOOL  SearchMpeg4VOP(uint8_t *pStream, int strmLen)
{
    uint8_t *pStreamEnd = pStream + strmLen;
    uint8_t *tempPos = pStream;

    while (tempPos < pStreamEnd)
    {
	if (tempPos[0] == 0x00 && tempPos[1] == 0x00)
	{
		if (tempPos[2] == 0x01 && tempPos[3] == 0xB6) /* MPEG4 VOP start code */
		{
			return OMX_TRUE;
		}
	}
	tempPos++;
    }

    return OMX_FALSE;
}

/*Decode routine */
OMX_BOOL Mpeg4Decoder_OMX::Mp4DecodeVideo(OMX_BOOL *need_new_pic,OMX_BUFFERHEADERTYPE*aOutBuffer, OMX_U32* aOutputLength,
                                OMX_U8** aInputBuf, OMX_U32* aInBufSize,
                                OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
                                OMX_BOOL aDeBlocking,
                                OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag,
                                OMX_BUFFERHEADERTYPE **aOutBufferForRendering,void *aPrivate_data, OMX_BOOL *notSupport)
{
    OMX_BOOL Status = OMX_TRUE;
    OMX_S32 OldWidth, OldHeight;

    OldWidth = aPortParam->format.video.nFrameWidth;
    OldHeight = aPortParam->format.video.nFrameHeight;
    *aResizeFlag = OMX_FALSE;

     *need_new_pic = OMX_FALSE;
#ifdef _DEBUG
    static OMX_U32 FrameCount = 0;
#endif
    uint UseExtTimestamp = 0;
    uint32 TimeStamp;
    OMX_S32 InputSize, InitSize;

    if(iBufferAllocFail)
    {
    	*notSupport = OMX_TRUE;
	 iBufferAllocFail = OMX_FALSE;	
	 SCI_TRACE_LOW("Mpeg4Decoder_OMX dec not support because of buffer alloc fail\n");
	 return OMX_FALSE;
    }
	
   //SCI_TRACE_LOW("Mpeg4Decoder_OMX::Mp4DecodeVideo %d aInBufSize %5d marker %d::buffer %x,%x,%x,%x,%x,%x\n", (*aFrameCount),*aInBufSize,aMarkerFlag,(*aInputBuf)[0],(*aInputBuf)[1],(*aInputBuf)[2],(*aInputBuf)[3],(*aInputBuf)[4],(*aInputBuf)[5]);
	
    if ((Mpeg4InitCompleteFlag == OMX_FALSE) && (MPEG4_MODE == CodecMode))
    {
        if (!aMarkerFlag)
        {
            InitSize = GetVideoHeader(0, *aInputBuf, *aInBufSize);
        }
        else
        {
            InitSize = *aInBufSize;
        }	
	 
        if (PV_TRUE != InitializeVideoDecode(&iDisplay_Width, &iDisplay_Height,aInputBuf, (OMX_S32*)&InitSize, MODE_MPEG4, aDeBlocking))
        {
            *aInBufSize -= InitSize;
	     *aInputBuf += InitSize;
            return OMX_FALSE;
        }

        Mpeg4InitCompleteFlag = OMX_TRUE;

        if (CodecMode != MPEG4_MODE)
        {
            /* SVH mode is activated inside the decoder.
            ** Reset The init flag to get the width/height next frame.
            */
            Mpeg4InitCompleteFlag = OMX_FALSE;	
            return OMX_TRUE;
        }else
        {
            if(!SearchMpeg4VOP(*aInputBuf,*aInBufSize))
            {
            	*aInBufSize -= InitSize;
	     	*aInputBuf += InitSize;
            }
        }
#if 0
        aPortParam->format.video.nFrameWidth = iDisplay_Width;
        aPortParam->format.video.nFrameHeight = iDisplay_Height;
#else
        aPortParam->format.video.nFrameWidth =  ( (iDisplay_Width + 15) & (~15) );
        aPortParam->format.video.nFrameHeight =  ( (iDisplay_Height + 15) & (~15) );
#endif

        OMX_U32 min_stride = ((aPortParam->format.video.nFrameWidth + 15) & (~15));
        OMX_U32 min_sliceheight = ((aPortParam->format.video.nFrameHeight + 15) & (~15));


        aPortParam->format.video.nStride = min_stride;
        aPortParam->format.video.nSliceHeight = min_sliceheight;

        if (!((min_stride <= 720 && min_sliceheight <= 576) || (min_stride <= 576 && min_sliceheight <= 720)))
        {
            iDecoder_sw_flag = OMX_TRUE;

#ifdef YUV_THREE_PLANE
            aPortParam->format.video.nFrameWidth +=32;
            aPortParam->format.video.nFrameHeight +=32;
            aPortParam->format.video.nStride +=32;
            aPortParam->format.video.nSliceHeight +=32;
#endif
        }


        // finally, compute the new minimum buffer size.

        // Decoder components always output YUV420 format
        aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;

        iFrameSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride);
        iData_Width = aPortParam->format.video.nFrameWidth;
        iData_Height = aPortParam->format.video.nFrameHeight;

        // normally - in case of mp4 - the config parser will read the vol header ahead of time
        // and set the port values - so there'll be no port reconfig - but just in case ...

        CheckPortReConfig(aOutBuffer, OldWidth, OldHeight, aResizeFlag, aFrameCount);

        return OMX_TRUE;
    }


    if ((*(OMX_S32*)aInBufSize) <= 0)
    {
        return OMX_FALSE;
    }

    TimeStamp = 0xFFFFFFFF;
    InputSize = *aInBufSize;

    // in case of H263, read the 1st frame to find out the sizes (use the m4v_config)
    if ((OMX_FALSE == Mpeg4InitCompleteFlag) && (CodecMode != MPEG4_MODE))
    {
        int32 aligned_width, aligned_height;
        int32 display_width, display_height;

        if (iGetM4VConfigInfo(*aInputBuf, *aInBufSize, &aligned_width, &aligned_height, &display_width, &display_height))
        {
            return OMX_FALSE;
        }

        Mpeg4InitCompleteFlag = OMX_TRUE;
        iDisplay_Width = display_width;
        iDisplay_Height = display_height;
#if 0
        aPortParam->format.video.nFrameWidth = iDisplay_Width; // use non 16byte aligned values (display_width) for H263
        aPortParam->format.video.nFrameHeight = iDisplay_Height; // like in the case of M4V (PVGetVideoDimensions also returns display_width/height)
#else
        aPortParam->format.video.nFrameWidth =  ( (iDisplay_Width + 15) & (~15) );
        aPortParam->format.video.nFrameHeight =  ( (iDisplay_Height + 15) & (~15) );
#endif

        OMX_U32 min_stride = ((aPortParam->format.video.nFrameWidth + 15) & (~15));
        OMX_U32 min_sliceheight = ((aPortParam->format.video.nFrameHeight + 15) & (~15));

        aPortParam->format.video.nStride = min_stride;
        aPortParam->format.video.nSliceHeight = min_sliceheight;

        if (!((min_stride <= 720 && min_sliceheight <= 576) || (min_stride <= 576 && min_sliceheight <= 720)))
        {
            iDecoder_sw_flag = OMX_TRUE;

#ifdef YUV_THREE_PLANE
            aPortParam->format.video.nFrameWidth +=32;
            aPortParam->format.video.nFrameHeight +=32;
            aPortParam->format.video.nStride +=32;
            aPortParam->format.video.nSliceHeight +=32;
#endif
        }
		if (!((iDisplay_Width <= 720 && iDisplay_Height <= 576) || (iDisplay_Width <= 576 && iDisplay_Height <= 720)))
		{
			*aResizeFlag = OMX_TRUE;

			iDecoder_sw_flag = OMX_TRUE;

		#ifdef YUV_THREE_PLANE
			aPortParam->format.video.nStride +=32;
			aPortParam->format.video.nSliceHeight +=32;

			aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;
		#endif
		}
		

        // finally, compute the new minimum buffer size.

        // Decoder components always output YUV420 format
        aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;

        iFrameSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride);
        iData_Width = aPortParam->format.video.nFrameWidth;
        iData_Height = aPortParam->format.video.nFrameHeight;

        // in case of h263 - port reconfig is pretty common - we'll alos have to do SetReferenceYUV later

 //       CheckPortReConfig(aOutBuffer, OldWidth, OldHeight, aResizeFlag, aFrameCount);
        return OMX_TRUE;
    }


    if(!iExternalBufferWasSet)
    {
       if( -12 != getpriority(PRIO_PROCESS, 0)){
	  	setpriority(PRIO_PROCESS, 0, -12);//@jgdu
       }   
    	MMCodecBuffer codec_buf;
        uint32 Frm_width_align = aPortParam->format.video.nStride;
	uint32 Frm_height_align = aPortParam->format.video.nSliceHeight;
	uint32 mb_x = Frm_width_align/16;
        
	uint32 mb_y = Frm_height_align/16;
	uint32 total_mb_num = mb_x * mb_y;
	uint32 frm_size = (total_mb_num * 256);
	uint32 ext_buffer_size;
	 SCI_TRACE_LOW("Mpeg4Decoder_OMX::Mp4DecodeVideo width %d,height %d\n",aPortParam->format.video.nFrameWidth,aPortParam->format.video.nFrameHeight);

#if defined(CHIP_8810)
	if (iDecoder_sw_flag)
	{
		ext_buffer_size = 0;	//no used, xwluo@2012.07.03
	}else
	{
		ext_buffer_size = mb_x*4*8*sizeof(int16);	//pTopCoeff
		ext_buffer_size += 1000*1024;	//MP4DEC_FRM_STRM_BUF_SIZE
		//ext_buffer_size += frm_size*4;	//cmd data and info
		ext_buffer_size += frm_size*8;	//cmd data and info
		
		if(aDeBlocking)
		{
			ext_buffer_size += ((frm_size*3*3)>>1); //reconstruction frame buffer
		}
		ext_buffer_size += 10*1024;	//misc
	}
#else
	if(aDeBlocking)
	{
		ext_buffer_size = Frm_width_align*Frm_height_align*3/2*3 + mb_x*4*8*2  + 512;
	}else
	{
		ext_buffer_size =  mb_x*4*8*2  + 512;
	}
#endif

	if (ext_buffer_size)
	{
		iDecExtPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, ext_buffer_size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);	
	     	int fd = iDecExtPmemHeap->getHeapID();
		if(fd>=0)
		{
	 		int ret,phy_addr, buffer_size;
	 		ret = iDecExtPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 		if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
			
	    		iDecExtPhyAddr =(OMX_U32)phy_addr;
	    		SCI_TRACE_LOW("Mpeg4Decoder_OMX ext mem pmempool %x,%x,%x,%x\n",iDecExtPmemHeap->getHeapID(),iDecExtPmemHeap->base(),phy_addr,buffer_size);
	    	 	iDecExtVAddr = (OMX_U32)iDecExtPmemHeap->base();
	    		codec_buf.common_buffer_ptr =(uint8 *) iDecExtVAddr;
	    		codec_buf.common_buffer_ptr_phy = (void *)iDecExtPhyAddr;
	    		codec_buf.size = ext_buffer_size;

			MP4DecMemInit(&codec_buf);
		}else
		{
			iBufferAllocFail = OMX_TRUE;
			SCI_TRACE_LOW("Mpeg4Decoder_OMX ext mem pmempool  error\n");
	    		return OMX_FALSE;	
		}
	}	        
	
    	//alloc cacheable ext buf 
#if !defined(CHIP_8810)		
    	ext_buffer_size = mb_x*mb_y*6*4*2 +mb_x*mb_y+ mb_x*3*2+mb_x*mb_y*4 + mb_x*mb_y*4*6 + 512;
#else
	ext_buffer_size = total_mb_num * 6 * 2* sizeof(int32); 	//mb_info
	ext_buffer_size += 4 * 8 * sizeof(int16);				//pLeftCoeff
	ext_buffer_size += 6 * 64 * sizeof(int16);				//coef_block
	
	ext_buffer_size += 1024;		
	if (iDecoder_sw_flag)
	{
		int32 ext_size_y = (mb_x * 16 + 16*2) * (mb_y * 16 + 16*2);
		int32 ext_size_c = ext_size_y >> 2;
		int32 i;
	
		ext_buffer_size += 4*8*mb_x*sizeof(int16);	//pTopCoeff
		ext_buffer_size += ((( 64*4*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrY 
		ext_buffer_size += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrU
		ext_buffer_size += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrV 
	#ifndef YUV_THREE_PLANE	
		for (i = 0; i < 3; i++)
		{			
			ext_buffer_size += ((( ext_size_y + 255) >>8)<<8);	//imgYUV[0]
			ext_buffer_size += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[1]
			ext_buffer_size += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[2]
		}
	#endif		
	}else
	{
		ext_buffer_size += total_mb_num * 1 * sizeof(uint8);		//mbdec_stat_ptr
		ext_buffer_size += mb_x * 4 * sizeof(int16);			//pTopLeftDCLine
		ext_buffer_size += total_mb_num * (sizeof(uint32) + 6 *sizeof(uint32));	//dc_store,dp
		//ext_buffer_size += frm_size *4;			//cmd
		//ext_buffer_size += frm_size *8;			//ping pong cmd buffer
	}
#endif
    	iDecoder_ext_cache_buffer_ptr = oscl_malloc(ext_buffer_size+4);
    	if(iDecoder_ext_cache_buffer_ptr==NULL)
    	{
    		iBufferAllocFail = OMX_TRUE;    	
    		SCI_TRACE_LOW("Mpeg4Decoder_OMX iDecoder_ext_cache_buffer_ptr alloc  error\n");
		return OMX_FALSE;	    	
    	}
    	codec_buf.common_buffer_ptr =(uint8 *)( ((int)iDecoder_ext_cache_buffer_ptr+3)&(~0x3));
    	codec_buf.size = ext_buffer_size;

		MP4DecMemCacheInit(&codec_buf);
		iExternalBufferWasSet = OMX_TRUE;	
    }
	
    int picPhyAddr =  (static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->FindPhyAddr((uint32)aOutBuffer->pBuffer);	 
    if(!picPhyAddr)
    {
        SCI_TRACE_LOW("Mpeg4Decoder_OMX FindPhyAddr failed");
        return OMX_FALSE; 
    }	

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    if((static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX]){
    	int width = aPortParam->format.video.nStride;
    	int height = aPortParam->format.video.nSliceHeight;
    	Rect bounds(width, height);
    	void *vaddr;
        int usage;
        if(iDecoder_sw_flag){
		usage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
	}else{
		usage = GRALLOC_USAGE_SW_WRITE_RARELY;
	}
    	if(mapper.lock((const native_handle_t*)aOutBuffer->pBuffer, usage, bounds, &vaddr))
    	{
            SCI_TRACE_LOW("Mpeg4Decoder_OMX mapper.lock fail %x",aOutBuffer->pBuffer);
            return OMX_FALSE;			
    	}
	MP4DecSetCurRecPic((uint8*)(vaddr),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }else{	
    	//SCI_TRACE_LOW("VSP DPB::Mpeg4Decoder_OMX::Mp4DecodeVideo set cur pic  %x,%x,%x\n",aOutBuffer->pBuffer,aOutBuffer,picPhyAddr);
    	MP4DecSetCurRecPic((uint8*)(aOutBuffer->pBuffer),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }
#if PROFILING_ON
    OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

    MMDecInput dec_in;
    MMDecOutput dec_out;
	
    if(*aInBufSize > MPEG4_DECODER_STREAM_BUFFER_SIZE)
    {
    	    SCI_TRACE_LOW("Mpeg4Decoder_OMX input stream too big %d\n",*aInBufSize);
	    return OMX_FALSE;
    }

    oscl_memcpy((void *)iStream_buffer_ptr, *aInputBuf,*aInBufSize);
    dec_in.pStream = (uint8 *) iStream_buffer_ptr;
//    dec_in.pStream_phy = (uint8 *) iStreamPhyAddr;
    dec_in.dataLen = *aInBufSize;
    dec_in.beLastFrm = 0;
    dec_in.expected_IVOP = iSkipToIDR;
    dec_in.beDisplayed = 1;
    dec_in.err_pkt_num = 0;
  
    int ret;
   // ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
    //if(ret){
//		SCI_TRACE_LOW("mp4dec VSP hardware timeout try again %d\n",ret);	
//		ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
//		if(ret){
//   			 SCI_TRACE_LOW("mp4dec VSP hardware timeout give up %d\n",ret);
//		 	 return OMX_FALSE;
//		}		 
//    }	
//    ioctl(iVsp_fd,VSP_ENABLE,NULL);

    dec_out.VopPredType = -1;
    dec_out.frameEffective = 0;	
    MMDecRet decRet =	MP4DecDecode(&dec_in,&dec_out);
    Status =  (decRet == MMDEC_OK)?OMX_TRUE : OMX_FALSE;

//    ioctl(iVsp_fd,VSP_DISABLE,NULL);
//    ioctl(iVsp_fd,VSP_RELEASE,NULL);

    if((static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX]){
	if(mapper.unlock((const native_handle_t*)aOutBuffer->pBuffer))
	{
            SCI_TRACE_LOW("Mpeg4Decoder_OMX mapper.unlock fail %x",aOutBuffer->pBuffer);	
	}
    }

   if(aMarkerFlag)
   	    *aInBufSize = 0;
   else
   {
     	*aInBufSize = dec_in.dataLen;	//todo jgdu
   }
   // advance input buffer ptr
   *aInputBuf += (InputSize - *aInBufSize);
		
#if PROFILING_ON
    OMX_U32 EndTime = OsclTickCount::TickCount();
    iTotalTicks += (EndTime - StartTime);
#endif

    if (Status == OMX_TRUE)
    {
#ifdef _DEBUG
        //printf("Frame number %d\n", ++FrameCount);
#endif

	if(dec_out.frameEffective){
		*aOutBufferForRendering = (OMX_BUFFERHEADERTYPE *)dec_out.pBufferHeader;
	}	
	else
		*aOutBufferForRendering = NULL;
        *aOutputLength = (iFrameSize * 3) >> 1;
        (*aFrameCount)++;
       //SCI_TRACE_LOW("Mpeg4Decoder_OMX dec %d\n",(*aFrameCount));
	   
	if(dec_out.VopPredType!=NVOP)
	{
       *need_new_pic = OMX_TRUE;
	   	iSkipToIDR = OMX_FALSE;
	}
	//g_frame_num++;
#if 0	
	if((dec_out.VopPredType!=NVOP)&&((*aFrameCount)<=100))
	{
		SCI_TRACE_LOW("1---Mpeg4Decoder_OMX: aFrameCount = %d\n ",(*aFrameCount));
		dump_yuv(aOutBuffer->pBuffer,*aOutputLength);
	}
#endif
/*		
	if(dec_out.frameEffective)
	SCI_TRACE_LOW("VSP DPB::Mpeg4Decoder_OMX::Mp4DecodeVideo succeed %d,%x,%x,%d,%d\n",dec_out.frameEffective,(*aOutBufferForRendering)->pBuffer,*aOutBufferForRendering,(*aFrameCount), *aInBufSize);
	else
	SCI_TRACE_LOW("VSP DPB::Mpeg4Decoder_OMX::Mp4DecodeVideo succeed %d,%x,%x,%d,%d\n",dec_out.frameEffective,0,0,(*aFrameCount), *aInBufSize);	
*/	
    }
    else
    {
        *aOutBufferForRendering = NULL;
        //*aInBufSize = InputSize;
        *aOutputLength = 0;
        SCI_TRACE_LOW("Mpeg4Decoder_OMX dec err %d,%x",(*aFrameCount),g_mpeg4_dec_err_flag);	
    }
    return Status;
}

int Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num = 0;

OMX_S32 Mpeg4Decoder_OMX::InitializeVideoDecode(
    OMX_S32* aWidth, OMX_S32* aHeight, OMX_U8** aBuffer, OMX_S32* aSize, OMX_S32 mode, OMX_BOOL aDeBlocking)
{
    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;
    OMX_S32 notOK = PV_TRUE;
    CodecMode = MPEG4_MODE;
	
    if (mode == MODE_H263)
    {
        CodecMode = H263_MODE;
    }
	
    SCI_TRACE_LOW("Mpeg4Decoder_OMX::InitializeVideoDecode\n");

     if(!iStreamBufferWasSet)
     {
        iStream_buffer_ptr = oscl_malloc(MPEG4_DECODER_STREAM_BUFFER_SIZE);
        if(iStream_buffer_ptr==NULL)
        {
            iBufferAllocFail = OMX_TRUE;
            SCI_TRACE_LOW("AvcDecoder_OMX: iStream_buffer_ptr alloc  error\n");
            return OMX_FALSE;
        }
        iStreamBufferWasSet = OMX_TRUE;
    }
    if(iDecoder_int_buffer_ptr==NULL)
    {
    	iDecoder_int_buffer_ptr = oscl_malloc(MPEG4_DECODER_INTERNAL_BUFFER_SIZE+4);
    	if(iDecoder_int_buffer_ptr==NULL)
    	{
    		iBufferAllocFail = OMX_TRUE;    	
    		SCI_TRACE_LOW("Mpeg4Decoder_OMX iDecoder_int_buffer_ptr alloc  error\n");
		return PV_FALSE;	    	
    	}
    }
    codec_buf.int_buffer_ptr =(uint8 *)( ((int)iDecoder_int_buffer_ptr+3)&(~0x3));
    codec_buf.int_size = MPEG4_DECODER_INTERNAL_BUFFER_SIZE;
    video_format.i_extra = *aSize;
    if( video_format.i_extra>0)
    {
        oscl_memcpy((void *)iStream_buffer_ptr,*aBuffer,*aSize);
	video_format.p_extra =(void *) iStream_buffer_ptr;
//	video_format.p_extra_phy=(void *) iStreamPhyAddr;	
	uint32 first_32bits = ((*aBuffer)[0]<<24)|((*aBuffer)[1]<<16)|((*aBuffer)[2]<<8)|(*aBuffer)[3];
	if((first_32bits>>11) == 0x10)
	{
		if((first_32bits>>10) == 0x20)
			CodecMode = H263_MODE;
		else
			CodecMode = FLV_MODE;
	}
    }else{
	video_format.p_extra = NULL;
//	video_format.p_extra_phy = NULL;
    }

    SCI_TRACE_LOW("Mp4 stream  mode %d\n",CodecMode);	
	
    if(CodecMode == H263_MODE)
    {
    	video_format.video_std = ITU_H263;
    }else if(CodecMode == MPEG4_MODE)
    {
        video_format.video_std = MPEG4;
    }else
    {
    	video_format.video_std = FLV_H263;
    }
	
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    if (iVsp_fd == -1)
    {
    	if((iVsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
    	{
			return PV_FALSE;
    	}else
    	{
        	iVsp_addr = mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,iVsp_fd,0);
		}
    }

 //   if(video_format.video_std == MPEG4)
//    {
//    	int ret;
//        ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
//	if(ret){
//   		 SCI_TRACE_LOW("mp4dec VSP hardware timeout try again %d\n",ret);	
//		ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
//		if(ret){
//   			 SCI_TRACE_LOW("mp4dec VSP hardware timeout give up %d\n",ret);
//		 	return PV_FALSE;
//		}		 
//	}	
//        ioctl(iVsp_fd,VSP_ENABLE,NULL);
//    }    

    SCI_TRACE_LOW("Mpeg4Decoder_OMX::InitializeVideoDecode vsp addr %x\n",iVsp_addr);	
    VSP_SetVirtualBaseAddr((uint32)iVsp_addr);
    VSP_reg_reset_callback(VSP_reset_cb,iVsp_fd);
    VSP_reg_start_cqm_callback(VSP_Start_CQM_cb);
    VSP_reg_acquaire_callback(VSP_Acquaire_cb);

	MP4DecRegBufferCB(VSP_bind_cb,VSP_unbind_cb,(void *)this);
	
     	video_format.uv_interleaved = 1;// todo jgdu
	notOK = MP4DecInit(&codec_buf,&video_format);

	MP4DecSetPostFilter(aDeBlocking);

   //  if(video_format.video_std == MPEG4)
    {
        ioctl(iVsp_fd,VSP_DISABLE,NULL);
        ioctl(iVsp_fd,VSP_RELEASE,NULL);
	//*aBuffer += *aSize - video_format.i_extra;
	//*aSize = video_format.i_extra;// todo jgdu
	//*aBuffer += *aSize;
	//*aSize = 0;
    }    
    if(!notOK)
    {
    	if((CodecMode == H263_MODE)&&(video_format.frame_width == 0||video_format.frame_height == 0))
    	{
    	    *aWidth = PVH263DEFAULTWIDTH;
	    *aHeight = PVH263DEFAULTHEIGHT;
    	}else
    	{
    	    *aWidth = video_format.frame_width;
	    *aHeight = video_format.frame_height;
	    if(!(*aWidth)||!(*aHeight))
	    	return PV_FALSE;
    	}
	return PV_TRUE;
    }else
    {
    	return PV_FALSE;
    }
    
}

OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecDeinit()
{
    OMX_BOOL Status;

    SCI_TRACE_LOW("Mpeg4Decoder_OMX::Mp4DecDeinit\n");
    // at this point - the buffers have been freed already
    ipRefCtrPreviousReferenceBuffer = NULL;
    iReferenceYUVWasSet = OMX_FALSE;   

    if( iDecoder_int_buffer_ptr)
    {
	oscl_free(iDecoder_int_buffer_ptr);
	iDecoder_int_buffer_ptr = NULL;
    }
    if(iVsp_fd>=0)
    {
		munmap(iVsp_addr,SPRD_VSP_MAP_SIZE);    	
		ioctl(iVsp_fd,VSP_DISABLE,NULL); 	
		close(iVsp_fd);	
		iVsp_fd = -1;
    }
    if(iExternalBufferWasSet)
    {
        iDecExtPmemHeap.clear();
	 if(iDecoder_ext_cache_buffer_ptr)
	 {
	 	oscl_free(iDecoder_ext_cache_buffer_ptr);
		iDecoder_ext_cache_buffer_ptr = NULL;
	 }
	 iExternalBufferWasSet = OMX_FALSE;		
    }		
   if(iStreamBufferWasSet)
   {
	iStreamBufferWasSet = OMX_FALSE; 	

        if(iStream_buffer_ptr)
        {
            oscl_free(iStream_buffer_ptr);
            iStream_buffer_ptr = NULL;
        }
   }
    return OMX_ErrorNone;
}

OMX_S32 Mpeg4Decoder_OMX::GetVideoHeader(int32 aLayer, uint8* aBuf, int32 aMaxSize)
{
    OSCL_UNUSED_ARG(aLayer);

    int32 count = 0;
    char my_sc[4];

    uint8 *tmp_bs = aBuf;

    SCI_TRACE_LOW("Mpeg4Decoder_OMX::GetVideoHeader\n");
	
    oscl_memcpy(my_sc, tmp_bs, 4);
    my_sc[3] &= 0xf0;

    if (aMaxSize >= 4)
    {
        if (oscl_memcmp(my_sc, VOSH_START_CODE1, 4) && oscl_memcmp(my_sc, VO_START_CODE1, 4))
        {
            count = 0;
            iShortVideoHeader = OMX_TRUE;
        }
        else
        {
            count = 0;
            iShortVideoHeader = OMX_FALSE;
            while (oscl_memcmp(tmp_bs + count, VOP_START_CODE1, 4))
            {
                count++;
                if (count > 1000)
                {
                    iShortVideoHeader = OMX_TRUE;
                    break;
                }
            }
            if (iShortVideoHeader == OMX_TRUE)
            {
                count = 0;
                while (oscl_memcmp(tmp_bs + count, H263_START_CODE1, 3))
                {
                    count++;
                }
            }
        }
    }
    return count;
}


// Check for resizeflag and whether Reference YUV buff needs to set.
void Mpeg4Decoder_OMX::CheckPortReConfig(OMX_BUFFERHEADERTYPE* aOutBuffer, OMX_S32 OldWidth, OMX_S32 OldHeight, OMX_BOOL *aResizeFlag, OMX_S32* aFrameCount)
{
    SCI_TRACE_LOW("Mpeg4Decoder_OMX::CheckPortReConfig\n");
	
    if ((iData_Width != OldWidth) || (iData_Height != OldHeight))
    {
    	 SCI_TRACE_LOW("Mpeg4Decoder_OMX::CheckPortReConfig old [%d,%d] new [%d,%d]\n",OldWidth,OldHeight,iData_Width,iData_Height);
        *aResizeFlag = OMX_TRUE;
    }
    /*
    else if (NULL != aOutBuffer)
    {
        // if there'll be no port reconfig - the current output YUV buffer is good enough
        PVSetReferenceYUV(&VideoCtrl, (uint8*)(aOutBuffer->pBuffer));
        // take care of ref count for the buffer
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(aOutBuffer->pOutputPortPrivate);
        pBCTRL->iRefCount++;
        ipRefCtrPreviousReferenceBuffer = &(pBCTRL->iRefCount);

        iReferenceYUVWasSet = OMX_TRUE;
    }
    */
    *aFrameCount = 1;
}


/* ======================================================================== */
/*  Function    : getDisplayRect()      */
/*  Date        : 040505                        */
/*  Purpose     :                                   */
/*  In/out      :                                   */
/*  Return      : the display_width and display_height of       */
/*          the frame in the current layer.                                 */
/*  Note     : 
/*  Modified :                  */
/* ======================================================================== */
OMX_BOOL Mpeg4Decoder_OMX::getDisplayRect(int32 *display_left, int32 *display_top, int32 *display_width, int32 *display_height)
{
    if( (0 != iDisplay_Width) && (0 != iDisplay_Height) )
    {
        // decoder has been initialized and configured.
        *display_left = 0;
        *display_top = 0;
        if(iDecoder_sw_flag){
#ifdef      YUV_THREE_PLANE
        *display_left = 16;
        *display_top = 16;
#endif 
        }

        *display_width = iDisplay_Width;     // pOutPort->PortParam.format.video.nFrameWidth;
        *display_height = iDisplay_Height;  // pOutPort->PortParam.format.video.nFrameHeight ;
        return OMX_TRUE;
    }
    else
        return OMX_FALSE;
}


