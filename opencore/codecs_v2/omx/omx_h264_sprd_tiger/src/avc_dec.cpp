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
#include "oscl_types.h"
#include "avc_dec.h"
#include "omx_avc_component.h"

#if PROFILING_ON
#include "oscl_tickcount.h"
#endif

#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

#include <utils/threads.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include "mmcodec.h"
#include "h264dec.h"
#include "video_common.h"
#if defined(CHIP_8800G)
#include "vsp_drv_sc8800g.h"
#elif defined(CHIP_8810)
#include "vsp_drv_sc8810.h"
#elif defined(CHIP_TIGER)
#include "vsp_drv_tiger.h"
#endif
using namespace android;

/*************************************/
/* functions needed for video engine */
/*************************************/
static int VSP_reset_cb(int fd)
{
	//SCI_TRACE_LOW("VSP_reset_cb\n");
	ioctl(fd,VSP_RESET,NULL);
	return 0;
}
 
static int VSP_bind_cb(void *userdata, void *pHeader)
{
	AvcDecoder_OMX *pAvcDecoder_OMX = (AvcDecoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
	//SCI_TRACE_LOW("VSP DPB::VSP_bind_cb %x,%x,%d\n",((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader,pBCTRL->iRefCount);
/*	
        if (pBCTRL->iRefCount == 0)
        {
	    	if(OMX_TRUE == pBCTRL->iIsBufferInComponentQueue)
	    	{
	        	SCI_TRACE_LOW("VSP_bind_cb  OMX_TRUE == pBCTRL->iIsBufferInComponentQueue\n");
	    		QueueType *pOutputQueue = pAvcDecoder_OMX->ipOMXComponent->GetOutputQueue();
			while(1)
			{
				OMX_BUFFERHEADERTYPE *pHeaderTmp = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
				if(pHeaderTmp == (OMX_BUFFERHEADERTYPE *)pHeader )
				{
					break;
				}else
				{
					Queue(pOutputQueue,(void *)pHeaderTmp);
				}
			}
			pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;

			if( pAvcDecoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers)
	        	{
       				pAvcDecoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers--;
	        	}			
	    	}			 
    	}	
 */   	
        pBCTRL->iRefCount++;	
	return 0;
}

static int VSP_unbind_cb(void *userdata, void *pHeader)
{
	AvcDecoder_OMX *pAvcDecoder_OMX = (AvcDecoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
        pBCTRL->iRefCount--;
	//SCI_TRACE_LOW("VSP DPB::VSP_unbind_cb %x,%x,%d\n",((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader,pBCTRL->iRefCount);	
	if(pAvcDecoder_OMX->pCurrentBufferHdr!=pHeader)
	{
		if ( pBCTRL->iRefCount == 0)
        	{
            		pAvcDecoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers++;
	    		if(OMX_FALSE == pBCTRL->iIsBufferInComponentQueue)
	    		{
	        		SCI_TRACE_LOW("VSP_unbind_cb  OMX_FALSE == pBCTRL->iIsBufferInComponentQueue\n");
	    			QueueType *pOutputQueue = pAvcDecoder_OMX->ipOMXComponent->GetOutputQueue();
				Queue(pOutputQueue,(void *)pHeader);
				pBCTRL->iIsBufferInComponentQueue = OMX_TRUE;
	    		}
		}			
                // posssibly reschedule the component?
        }
	return 0;
}

int AvcDecoder_OMX::g_h264_dec_inst_num = 0;

int AvcDecoder_OMX::ActivateSPS_OMX(void* aUserData, uint width,uint height, uint aNumBuffers)
{
    SCI_TRACE_LOW("ActivateSPS_OMX %d,%d,%d\n",width,height,aNumBuffers);
	
    AvcDecoder_OMX* pAvcDecoder_OMX = (AvcDecoder_OMX*)aUserData;

    if (NULL == pAvcDecoder_OMX)
    {
        return 0;
    }

    pAvcDecoder_OMX->ReleaseReferenceBuffers();

   if((pAvcDecoder_OMX->iReconfigWidth!=width) || (pAvcDecoder_OMX->iReconfigHeight != height))
   {
     	if( -12 != getpriority(PRIO_PROCESS, 0)){
		 setpriority(PRIO_PROCESS, 0, -12);//@jgdu
    	}  
        if(pAvcDecoder_OMX->iExternalBufferWasSet)
    	{
        	pAvcDecoder_OMX->iDecExtPmemHeap.clear();
		if(pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr)
		{
			oscl_free(pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr);
			pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr = NULL;
		}
		pAvcDecoder_OMX->iExternalBufferWasSet = OMX_FALSE;			
    	}	
   	MMCodecBuffer codec_buf;
	uint32 mb_x = width/16;
	uint32 mb_y = height/16;
	uint32 ext_buffer_size;
        SCI_TRACE_LOW("AvcDecoder_OMX::ActivateSPS_OMX width %d,height %d\n",width,height);
#if defined(CHIP_8800G)		
		ext_buffer_size =  mb_x*32 + 512 ;// +mb_x*mb_y*62*4 + mb_x*mb_y*1+ (16+1)*(3+7+18)*4 + 1024 ;
#else //defined(CHIP_8810)	|defined(CHIP_TIGER)
	if(mb_x*mb_y< 100){//qcif
		ext_buffer_size = mb_x*mb_y*256*8 + 4000*4 + mb_x*32 + 1024*1024+ 10*1024;
	}
	else{
		ext_buffer_size = mb_x*mb_y*256*4 + 4000*4 + mb_x*32 + 1024*1024+ 10*1024;
	}
#endif
     	SCI_TRACE_LOW("s_extra_mem_size %x",ext_buffer_size);
#if 0
	sp<MemoryHeapBase> masterHeap = new MemoryHeapBase(PMEM_DRIVER,ext_buffer_size,MemoryHeapBase::NO_CACHING);
        pAvcDecoder_OMX->iDecExtPmemHeap = new MemoryHeapPmem(masterHeap,MemoryHeapBase::NO_CACHING);
	//sp<MemoryHeapBase> masterHeap = new MemoryHeapBase(PMEM_DRIVER,ext_buffer_size,0);
       //pAvcDecoder_OMX->iDecExtPmemHeap = new MemoryHeapPmem(masterHeap,0);
	 
 	 int fd = pAvcDecoder_OMX->iDecExtPmemHeap->getHeapID();
	 if(fd>=0){
	        pAvcDecoder_OMX->iDecExtPmemHeap->slap();
		masterHeap.clear();
	        struct pmem_region region;
	        ::ioctl(fd,PMEM_GET_PHYS,&region);
		pAvcDecoder_OMX->iDecExtPhyAddr =(OMX_U32)region.offset;					
		SCI_TRACE_LOW("AvcDecoder_OMX ext mem pmempool %x,%x,%x,%x\n",pAvcDecoder_OMX->iDecExtPmemHeap->getHeapID(),pAvcDecoder_OMX->iDecExtPmemHeap->base(),region.offset,region.len);
		pAvcDecoder_OMX->iDecExtVAddr = (OMX_U32)pAvcDecoder_OMX->iDecExtPmemHeap->base();
		codec_buf.common_buffer_ptr =(uint8 *)pAvcDecoder_OMX-> iDecExtVAddr;
		codec_buf.common_buffer_ptr_phy = (void *)pAvcDecoder_OMX->iDecExtPhyAddr;
		codec_buf.size = ext_buffer_size;
		H264DecMemInit(&codec_buf);
    	}
#else
 	pAvcDecoder_OMX->iDecExtPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, ext_buffer_size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);

 	 int fd = pAvcDecoder_OMX->iDecExtPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = pAvcDecoder_OMX->iDecExtPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);

		pAvcDecoder_OMX->iDecExtPhyAddr =(OMX_U32)phy_addr;
		SCI_TRACE_LOW("AvcDecoder_OMX ext mem pmempool %x,%x,%x,%x\n",pAvcDecoder_OMX->iDecExtPmemHeap->getHeapID(),pAvcDecoder_OMX->iDecExtPmemHeap->base(),phy_addr,buffer_size);
	 	pAvcDecoder_OMX->iDecExtVAddr = (OMX_U32)pAvcDecoder_OMX->iDecExtPmemHeap->base();
		codec_buf.common_buffer_ptr =(uint8 *)pAvcDecoder_OMX-> iDecExtVAddr;
		codec_buf.common_buffer_ptr_phy = (void *)pAvcDecoder_OMX->iDecExtPhyAddr;
		codec_buf.size = ext_buffer_size;
		H264DecMemInit(&codec_buf);
    	}
#endif
	 else
    	{
    		pAvcDecoder_OMX->iBufferAllocFail = OMX_TRUE;
    		SCI_TRACE_LOW("AvcDecoder_OMX ext mem pmempool  error\n");
		return OMX_FALSE;	
    	}
#if defined(CHIP_8800G)		
	ext_buffer_size = mb_x*mb_y*62*4 + mb_x*mb_y*1+ (16+1)*(3+7+18)*4 + 1024;
#else ////defined(CHIP_8810)	|defined(CHIP_TIGER)
	if(mb_x*mb_y< 100){//qcif
		ext_buffer_size = /*mb_x*mb_y*256*8 + */4000*4 + mb_x*32 + (2+mb_y)*mb_x*352 + 
			17*4 + 17*4 + 17*4 + 17*(7*4+ 20496 + mb_x*mb_y*16*(4+4+1+1+4+4)) + mb_x*mb_y*1 + 10*1024;
	}
	else{
		ext_buffer_size =/* mb_x*mb_y*256*4 + */4000*4 + mb_x*32 + (2+mb_y)*mb_x*352 + 
			17*4 + 17*4 + 17*4 + 17*(7*4+ 20496 + mb_x*mb_y*16*(4+4+1+1+4+4)) + mb_x*mb_y*1 + 10*1024;
	}
#endif
    	pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr = oscl_malloc(ext_buffer_size+4);
	SCI_TRACE_LOW("iDecoder_ext_cache_buffer_ptr %x,%x",pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr,ext_buffer_size);
    	if(pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr==NULL)
    	{
    		pAvcDecoder_OMX->iBufferAllocFail = OMX_TRUE;    	
    		SCI_TRACE_LOW("AvcDecoder_OMX iDecoder_ext_cache_buffer_ptr alloc  error\n");
		return OMX_FALSE;	    	
    	}
	pAvcDecoder_OMX->iExternalBufferWasSet = OMX_TRUE;	
    	codec_buf.common_buffer_ptr =(uint8 *)( ((int)pAvcDecoder_OMX->iDecoder_ext_cache_buffer_ptr+3)&(~0x3));
    	codec_buf.size = ext_buffer_size;
//#if !defined(CHIP_8810)			
	H264DecMemCacheInit(&codec_buf);		
//#endif
    }	
   
    pAvcDecoder_OMX->FrameSize = width*height*3/2;
    pAvcDecoder_OMX->MaxNumFs = aNumBuffers;

    pAvcDecoder_OMX->iReconfigWidth = width;
    pAvcDecoder_OMX->iReconfigHeight = height;

    pAvcDecoder_OMX->iReconfigStride = width;
    pAvcDecoder_OMX->iReconfigSliceHeight = height;

    if (pAvcDecoder_OMX->MaxWidth <width)
    {
        pAvcDecoder_OMX->MaxWidth = width;
    }
    if (pAvcDecoder_OMX->MaxHeight < height)
    {
        pAvcDecoder_OMX->MaxHeight = height;
    }

    pAvcDecoder_OMX->iNewSPSActivation = OMX_TRUE;
    return 1;
}

/* initialize video decoder */
OMX_BOOL AvcDecoder_OMX::InitializeVideoDecode_OMX()
{
    SCI_TRACE_LOW("AvcDecoder_OMX::InitializeVideoDecode_OMX\n");
    MaxWidth = 0;
    MaxHeight = 0;
    MaxNumFs = 0;	
		
     if(!iStreamBufferWasSet)
     {
#if 0     
    	 sp<MemoryHeapBase> masterHeap = new MemoryHeapBase(PMEM_DRIVER,H264_DECODER_STREAM_BUFFER_SIZE,MemoryHeapBase::NO_CACHING);
     	iStreamPmemHeap = new MemoryHeapPmem(masterHeap,MemoryHeapBase::NO_CACHING);
     	int fd = iStreamPmemHeap->getHeapID();
    	 if(fd>=0){
	       	iStreamPmemHeap->slap();
		masterHeap.clear();
	        struct pmem_region region;
	        ::ioctl(fd,PMEM_GET_PHYS,&region);
		iStreamPhyAddr =(OMX_U32)region.offset;					
		SCI_TRACE_LOW("AvcDecoder_OMX stream mem pempool %x,%x,%x,%x\n",iStreamPmemHeap->getHeapID(),iStreamPmemHeap->base(),region.offset,region.len);
	 	iStreamVAddr = (OMX_U32)iStreamPmemHeap->base();
		SCI_TRACE_LOW("AvcDecoder_OMX stream mem pmempool  sucessful \n");
		iStreamBufferWasSet = OMX_TRUE;
    	}
#else
	iStreamPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, H264_DECODER_STREAM_BUFFER_SIZE, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);

 	 int fd = iStreamPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iStreamPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);

		iStreamPhyAddr =(OMX_U32)phy_addr;
		SCI_TRACE_LOW("AvcDecoder_OMX ext mem pmempool %x,%x,%x,%x\n",iStreamPmemHeap->getHeapID(),iStreamPmemHeap->base(),phy_addr,buffer_size);
	 	iStreamVAddr = (OMX_U32)iStreamPmemHeap->base();
		SCI_TRACE_LOW("AvcDecoder_OMX stream mem pmempool  sucessful \n");
		iStreamBufferWasSet = OMX_TRUE;
    	}
#endif
	else
    	{
    		iBufferAllocFail = OMX_TRUE;      	
    		SCI_TRACE_LOW("AvcDecoder_OMX stream mem pmempool  error\n");
		return OMX_FALSE;	
    	}
    }
	 
 //   SCI_TRACE_LOW("b4 open SPRD_VSP_DRIVER\n");
    if((iVsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
    {
    	SCI_TRACE_LOW("open SPRD_VSP_DRIVER error\n");
	return OMX_FALSE;
    }else
    {
        iVsp_addr = mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,iVsp_fd,0);
    }
    SCI_TRACE_LOW("AvcDecoder_OMX::InitializeVideoDecode vsp addr %x\n",iVsp_addr);	

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;	
    if(!iDecoder_int_buffer_ptr)
    {
        iDecoder_int_buffer_ptr = oscl_malloc(H264_DECODER_INTERNAL_BUFFER_SIZE+4);
	if(iDecoder_int_buffer_ptr==NULL)
	{
    		iBufferAllocFail = OMX_TRUE;  	
   		SCI_TRACE_LOW("AvcDecoder_OMX::InitializeVideoDecode cannot malloc VID internal buff \n");	
		return OMX_FALSE;
	}
    }		
    codec_buf.int_buffer_ptr = (uint8 *)( ((int)iDecoder_int_buffer_ptr+3)&(~0x3));
    codec_buf.int_size = H264_DECODER_INTERNAL_BUFFER_SIZE;

    video_format.video_std = H264;
    video_format.frame_width = 0;
    video_format.frame_height = 0;	
    video_format.p_extra = NULL;
    video_format.p_extra_phy = NULL;	
    video_format.i_extra = 0;
    VSP_SetVirtualBaseAddr((uint32)iVsp_addr);
    VSP_reg_reset_callback(VSP_reset_cb,iVsp_fd);
    H264Dec_RegBufferCB(VSP_bind_cb,VSP_unbind_cb,(void *)this);
    H264Dec_RegSPSCB( AvcDecoder_OMX::ActivateSPS_OMX);

    MMDecRet ret = H264DecInit(&codec_buf,&video_format);
		
    if(ret!=MMDEC_OK)
    {
    		SCI_TRACE_LOW("H264DecInit failed");
		return OMX_FALSE;
    }
  //  SCI_TRACE_LOW("AvcDecoder_OMX::InitializeVideoDecode_OMX successful \n");
    return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::FlushOutput_OMX(OMX_BUFFERHEADERTYPE **aOutBufferForRendering)
{
   	 SCI_TRACE_LOW("AvcDecoder_OMX::FlushOutput_OMX\n");
	return (OMX_BOOL)H264Dec_GetLastDspFrm((void **)aOutBufferForRendering);
}


/* Initialization routine */
OMX_ERRORTYPE AvcDecoder_OMX::AvcDecInit_OMX()
{
    SCI_TRACE_LOW("AvcDecoder_OMX::AvcDecInit_OMX\n");
    if (OMX_FALSE == InitializeVideoDecode_OMX())
    {
    	 SCI_TRACE_LOW("!!!!AvcDecoder_OMX::AvcDecInit_OMX failed!!!!\n");
        return OMX_ErrorInsufficientResources;
    }

    iAvcActiveFlag = OMX_FALSE;
    iSkipToIDR = OMX_FALSE;
    iNewSPSActivation = OMX_FALSE;

    iReconfigWidth = 0;
    iReconfigHeight = 0;
    iReconfigStride = 0;
    iReconfigSliceHeight = 0;

    iBufferAllocFail = OMX_FALSE;	
//SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
    return OMX_ErrorNone;
}


/*Decode routine */
OMX_BOOL AvcDecoder_OMX::AvcDecodeVideo_OMX(OMX_BOOL *need_new_pic,OMX_BUFFERHEADERTYPE *aOutBuffer, // empty output buffer that the component provides to the decoder
        OMX_U32* aOutputLength,OMX_BUFFERHEADERTYPE **aOutBufferForRendering, // output buffer for rendering. Initially NULL, but if there is output to be flushed out
        // this ptr will be updated to point to buffer that needs to be rendered
        OMX_U8** aInputBuf, OMX_U32* aInBufSize,
        OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
        OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag, OMX_BOOL *notSupport)
{

    SCI_TRACE_LOW("AvcDecoder_OMX::AvcDecodeVideo_OMX %d,%d: %x,%x,%x,%x,%x,%x\n",*aInBufSize,aMarkerFlag,(*aInputBuf)[0],(*aInputBuf)[1],(*aInputBuf)[2],(*aInputBuf)[3],(*aInputBuf)[4],(*aInputBuf)[5]);
	
    MMDecRet Status;
    uint8* pNalBuffer;
    int NalSize, NalType, NalRefId;

    *aResizeFlag = OMX_FALSE;
    *need_new_pic = OMX_FALSE;	
    OMX_S32 OldStride;
    OMX_U32 OldSliceHeight;

    OldStride =  aPortParam->format.video.nStride; // actual buffer width
    OldSliceHeight = aPortParam->format.video.nSliceHeight; //actual buffer height

    *aOutBufferForRendering = NULL; // init this to NULL. If there is output to be flushed out - we'll update this value
    *aOutputLength = 0;
    pCurrentBufferHdr = aOutBuffer; // save the ptr to the empty output buffer we received from component

    if(iBufferAllocFail)
    {
    	*notSupport = OMX_TRUE;
	 iBufferAllocFail = OMX_FALSE;	
	 SCI_TRACE_LOW("AvcDecoder_OMX dec not support because of buffer alloc fail\n");
	 return OMX_FALSE;
    }
		
	
    int offset = 0;
    if(*(*aInputBuf) == 0 && *(*aInputBuf+1) == 0)
    {
    	if(*(*aInputBuf+2)==1)
    	{
    	  offset = 3 ;
    	}
		if(*(*aInputBuf+2)==0&& *(*aInputBuf+3)==1)
		{
	      offset = 4 ;
		}
    }
    if (!aMarkerFlag)
    {
        // if no more NALs - try to flush the output
        if (AVCDEC_FAIL == GetNextFullNAL_OMX(&pNalBuffer, &NalSize, *aInputBuf+offset, aInBufSize- offset))
        {    
        	SCI_TRACE_LOW("AVCDEC_FAIL %s,%d\n",__FUNCTION__,__LINE__);
                return OMX_FALSE;
        }
    }
    else
    {
        pNalBuffer = *aInputBuf+ offset;
        NalSize = *aInBufSize- offset;
        //Assuming that the buffer with marker bit contains one full NAL
        *aInBufSize = 0;
    }
//SCI_TRACE_LOW("aOutBuffer,->pBuffer %x,%x",aOutBuffer,aOutBuffer->pBuffer);
	int picPhyAddr =  (static_cast<OpenmaxAvcAO * > (ipOMXComponent))->FindPhyAddr((uint32)aOutBuffer->pBuffer);
    if(!picPhyAddr)
    {
        SCI_TRACE_LOW("AvcDecoder_OMX FindPhyAddr failed");
        return OMX_FALSE; 
    }



    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    if((static_cast<OpenmaxAvcAO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX]){
    	int width = aPortParam->format.video.nStride;
    	int height = aPortParam->format.video.nSliceHeight;
    	Rect bounds(width, height);
    	void *vaddr;
    	if(mapper.lock((const native_handle_t*)aOutBuffer->pBuffer, GRALLOC_USAGE_SW_WRITE_RARELY, bounds, &vaddr))
    	{
            SCI_TRACE_LOW("AvcDecoder_OMX mapper.lock fail %x",aOutBuffer->pBuffer);
            return OMX_FALSE;			
    	}
	SCI_TRACE_LOW("iUseAndroidNativeBuffer set cur pic  %x,%x,%x\n",aOutBuffer->pBuffer,aOutBuffer,picPhyAddr);
	H264Dec_SetCurRecPic((uint8*)(vaddr),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }else{	
       SCI_TRACE_LOW("VSP DPB::AvcDecoder_OMX::AvcDecodeVideo_OMX set cur pic  %x,%x,%x\n",aOutBuffer->pBuffer,aOutBuffer,picPhyAddr);
       H264Dec_SetCurRecPic((uint8*)(aOutBuffer->pBuffer),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }

    MMDecInput dec_in;
    MMDecOutput dec_out;
	
    if(NalSize>H264_DECODER_STREAM_BUFFER_SIZE)
    {
    	    SCI_TRACE_LOW("AvcDecoder_OMX input stream too big %d\n",NalSize);
	    return OMX_FALSE;
    }
//#if !defined(CHIP_8810)	
    ((uint8 *) iStreamVAddr)[0] = 0x0;
    ((uint8 *) iStreamVAddr)[1] = 0x0;
    ((uint8 *) iStreamVAddr)[2] = 0x0;
    ((uint8 *) iStreamVAddr)[3] = 0x1;	
    oscl_memcpy((void *)iStreamVAddr+4, pNalBuffer,NalSize);
    dec_in.pStream = (uint8 *) iStreamVAddr;
    dec_in.pStream_phy = (uint8 *) iStreamPhyAddr;
    dec_in.dataLen = NalSize+4;
//#else
//    oscl_memcpy((void *)iStreamVAddr, pNalBuffer,NalSize);
//    dec_in.pStream = (uint8 *) pNalBuffer;
//    dec_in.pStream_phy = (uint8 *) iStreamPhyAddr;
//    dec_in.dataLen = NalSize;
//#endif

    dec_in.beLastFrm = 0;
    dec_in.expected_IVOP = iSkipToIDR;
    dec_in.beDisplayed = 1;
    dec_in.err_pkt_num = 0;

    if(!iHold_VSP)
    {
    	int ret  =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
    	if(ret){
		SCI_TRACE_LOW("avcdec VSP hardware timeout try again %d\n",ret);	
		ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
		if(ret){
   			 SCI_TRACE_LOW("avcdec VSP hardware timeout give up %d\n",ret);
		 	 return OMX_FALSE;
		}		 
    	}
	iHold_VSP = 1;
	ioctl(iVsp_fd,VSP_ENABLE,NULL);
	ioctl(iVsp_fd,VSP_RESET,NULL);	
    }
    //ioctl(iVsp_fd,VSP_ENABLE,NULL);

    dec_out.frameEffective = 0;	

#if PROFILING_ON
        OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

    Status = H264DecDecode(&dec_in,&dec_out);

#if PROFILING_ON
        OMX_U32 EndTime = OsclTickCount::TickCount();
        iTotalTicks += (EndTime - StartTime);
#endif

   // ioctl(iVsp_fd,VSP_DISABLE,NULL);
   // ioctl(iVsp_fd,VSP_RELEASE,NULL);	

    if((static_cast<OpenmaxAvcAO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX]){
	if(mapper.unlock((const native_handle_t*)aOutBuffer->pBuffer))
	{
            SCI_TRACE_LOW("AvcDecoder_OMX mapper.unlock fail %x",aOutBuffer->pBuffer);	
	}
    }
	
    if ((MMDEC_OK== Status) && (OMX_TRUE == iNewSPSActivation))
    {
	// SPS NAL has been activated, set the new output port parameters and
	 // signal the port reconfiguration if required

	aPortParam->format.video.nFrameWidth = iReconfigWidth;
	aPortParam->format.video.nFrameHeight = iReconfigHeight;
	aPortParam->format.video.nStride = iReconfigStride;
	aPortParam->format.video.nSliceHeight = iReconfigSliceHeight;

	 // finally, compute the new minimum buffer size.
	 // Decoder components always output YUV420 format
	aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;


	if ((OldStride != aPortParam->format.video.nStride) || (OldSliceHeight !=  aPortParam->format.video.nSliceHeight))
	{
		  *aResizeFlag = OMX_TRUE;
	}

	// make sure that the number of buffers that the decoder requires is less than what we allocated
	// we should have at least 2 more than the decoder (one for omx component and one for downstream)
	if (MaxNumFs + 2 > aPortParam->nBufferCountActual)
	{
		 // even if buffers are the correct size - we must do port reconfig because of buffer count
		 *aResizeFlag = OMX_TRUE;
		// adjust only buffer count min - nBufferCountActual still needs to be preserved until port becomes disabled
		aPortParam->nBufferCountMin = MaxNumFs + 2;
	}
        iNewSPSActivation = OMX_FALSE;
     }
 


    if (Status == MMDEC_OK)
    {
    	 iSkipToIDR = OMX_FALSE;
        if (!iAvcActiveFlag)
            iAvcActiveFlag = OMX_TRUE;
			
	if(dec_out.frameEffective)
	{
		*aOutBufferForRendering = (OMX_BUFFERHEADERTYPE *)dec_out.pBufferHeader;
		*aOutputLength = FrameSize;
	}
        (*iFrameCount)++;

	if(dec_out.reqNewBuf)	
	{
            *need_new_pic = OMX_TRUE;

	     iHold_VSP = 0;
	     ioctl(iVsp_fd,VSP_DISABLE,NULL);
             ioctl(iVsp_fd,VSP_RELEASE,NULL);
	}
	return OMX_TRUE;
    }else
    {
       iSkipToIDR = OMX_TRUE;
       SCI_TRACE_LOW("AvcDecoder_OMX dec err %d\n",(*iFrameCount));    
       if(Status == MMDEC_NOT_SUPPORTED){
		*notSupport = OMX_TRUE;
		SCI_TRACE_LOW("AvcDecoder_OMX dec format not support\n");
       }
    	return  OMX_FALSE;
    }

//SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
}


OMX_ERRORTYPE AvcDecoder_OMX::AvcDecDeinit_OMX()
{
    SCI_TRACE_LOW("AvcDecoder_OMX::AvcDecDeinit_OMX\n");
    if( iDecoder_int_buffer_ptr){
	oscl_free(iDecoder_int_buffer_ptr);
	iDecoder_int_buffer_ptr = NULL;
    }
    if(iHold_VSP&&(iVsp_fd>=0))	
    {
 	ioctl(iVsp_fd,VSP_DISABLE,NULL);
        ioctl(iVsp_fd,VSP_RELEASE,NULL);	   	
    }
    if(iVsp_fd>=0)
    {
       munmap(iVsp_addr,SPRD_VSP_MAP_SIZE);	
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
        iStreamPmemHeap.clear();	
	iStreamBufferWasSet = OMX_FALSE;		
   }
  // SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
    return OMX_ErrorNone;
}


/* ======================================================================== */
/*  Function : PVAVCAnnexBGetNALUnit()                                      */
/*  Purpose  : Parse a NAL from byte stream format.                         */
/*  In/out   :                                                              */
/*  Return   : AVCDEC_SUCCESS if succeed, AVC_FAIL if fail.                 */
/* ======================================================================== */
/**
@pseudocode "
    byte_stream_nal_unit(NumBytesInNalunit){
    while(next_bits(24) != 0x000001)
        zero_byte
    if(more_data_in_byte_stream()){
        start_code_prefix_one_3bytes // equal 0x000001
        nal_unit(NumBytesInNALunit)
    }
   }"
*/
static  AVCDec_Status PVAVCAnnexBGetNALUnit(uint8 *bitstream, uint8 **nal_unit,
        int *size)
{
    //SCI_TRACE_LOW(" PVAVCAnnexBGetNALUnit\n");
    int i, j, FoundStartCode = 0;
    int end;

    i = 0;
    while (bitstream[i] == 0 && i < *size)
    {
        i++;
    }
    if (i >= *size)
    {
        *nal_unit = bitstream;
        return AVCDEC_FAIL; /* cannot find any start_code_prefix. */
    }
    else if (bitstream[i] != 0x1)
    {
        i = -1;  /* start_code_prefix is not at the beginning, continue */
    }

    i++;
    *nal_unit = bitstream + i; /* point to the beginning of the NAL unit */

    j = end = i;
    while (!FoundStartCode)
    {
        while ((j + 1 < *size) && (bitstream[j] != 0 || bitstream[j+1] != 0))  /* see 2 consecutive zero bytes */
        {
            j++;
        }
        end = j;   /* stop and check for start code */
        while (j + 2 < *size && bitstream[j+2] == 0) /* keep reading for zero byte */
        {
            j++;
        }
        if (j + 2 >= *size)
        {
            *size -= i;
            return AVCDEC_NO_NEXT_SC;  /* cannot find the second start_code_prefix */
        }
        if (bitstream[j+2] == 0x1)
        {
            FoundStartCode = 1;
        }
        else
        {
            /* could be emulation code 0x3 */
            j += 2; /* continue the search */
        }
    }

    *size = end - i;

    return AVCDEC_SUCCESS;
}

AVCDec_Status AvcDecoder_OMX::GetNextFullNAL_OMX(uint8** aNalBuffer, int* aNalSize, OMX_U8* aInputBuf, OMX_U32* aInBufSize)
{
   // SCI_TRACE_LOW(" AvcDecoder_OMX::GetNextFullNAL_OMX\n");
    uint8* pBuff = aInputBuf;
    OMX_U32 InputSize;

    *aNalSize = *aInBufSize;
    InputSize = *aInBufSize;

    AVCDec_Status ret_val  = PVAVCAnnexBGetNALUnit(pBuff, aNalBuffer, aNalSize);

    if (ret_val == AVCDEC_FAIL)
    {
        return AVCDEC_FAIL;
    }

    InputBytesConsumed = ((*aNalSize) + (int32)(*aNalBuffer - pBuff));
    aInputBuf += InputBytesConsumed;
    *aInBufSize = InputSize - InputBytesConsumed;
 //  SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
    return AVCDEC_SUCCESS;
}


void AvcDecoder_OMX::ResetDecoder()
{
    SCI_TRACE_LOW("AvcDecoder_OMX::ResetDecoder\n");
    ReleaseReferenceBuffers();
//   SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
}

void AvcDecoder_OMX::ReleaseReferenceBuffers()
{
        SCI_TRACE_LOW("AvcDecoder_OMX::ReleaseReferenceBuffers\n");
	H264Dec_ReleaseRefBuffers();
//	SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
}

void AvcDecoder_OMX::CalculateBufferParameters(OMX_PARAM_PORTDEFINITIONTYPE* aPortParam)
{
    SCI_TRACE_LOW("AvcDecoder_OMX::CalculateBufferParameters\n");
    // If decoder has decoded the parameters, retain the updated values from the decoder
    // and do not recalculate
    if (OMX_FALSE == iAvcActiveFlag)
    {
        OMX_VIDEO_PORTDEFINITIONTYPE *pVideoformat = &(aPortParam->format.video);

        // check if stride needs to be adjusted - stride should be at least the 16 byte aligned width
        OMX_U32 MinStride = ((pVideoformat->nFrameWidth + 15) & (~15));
        OMX_U32 MinSliceHeight = ((pVideoformat->nFrameHeight + 15) & (~15));

        pVideoformat->nStride = MinStride;
        pVideoformat->nSliceHeight = MinSliceHeight;

        // finally, compute the new minimum buffer size
        aPortParam->nBufferSize = (pVideoformat->nSliceHeight * pVideoformat->nStride * 3) >> 1;
    }
//SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);
    return ;
}

