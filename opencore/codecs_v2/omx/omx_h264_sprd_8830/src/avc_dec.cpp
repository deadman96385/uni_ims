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

#define LOG_NDEBUG 0
#define LOG_TAG "omx"
#include <utils/Log.h>

//#define AVC_ES_DUMP

#ifdef AVC_ES_DUMP
static FILE* fp_es = NULL;
static const char* const fn_es = "/data/video/input.h264";
#endif


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
//#include "vsp_drv_sc8830.h"
using namespace android;

static int VSP_bind_cb(void *userdata, void *pHeader)
{
	AvcDecoder_OMX *pAvcDecoder_OMX = (AvcDecoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
	OMX_H264DEC_INFO ("%s: %x,%x,%d\n", __FUNCTION__, ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader,pBCTRL->iRefCount);

        pBCTRL->iRefCount++;	
	return 0;
}

static int VSP_unbind_cb(void *userdata, void *pHeader)
{
    AvcDecoder_OMX *pAvcDecoder_OMX = (AvcDecoder_OMX *)userdata;
    BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    OMX_H264DEC_INFO ("%s: ref frame: 0x%x, %x; cnt=%d, isinQueue=%d", __FUNCTION__, 
            ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,
            pBCTRL->iRefCount,pBCTRL->iIsBufferInComponentQueue
    );

    if (pBCTRL->iRefCount  > 0)
    {   
        pBCTRL->iRefCount--;
    }

    if ( pBCTRL->iRefCount == 0)
    {
         // if a buffer's ref count is 0 - it means that it is available.
        pAvcDecoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers++;

        if(OMX_FALSE == pBCTRL->iIsBufferInComponentQueue)
        {
            // push back the buffer to queue.
            QueueType *pOutputQueue = pAvcDecoder_OMX->ipOMXComponent->GetOutputQueue();
            Queue(pOutputQueue,(void *)pHeader);
            pBCTRL->iIsBufferInComponentQueue = OMX_TRUE;
        }
    }

    return 0;
}

int AvcDecoder_OMX::g_h264_dec_inst_num = 0;

int AvcDecoder_OMX::VSP_malloc_cb(void* aUserData, uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size)
{
    OMX_H264DEC_INFO ("%s: width: %d,%d,%d\n", __FUNCTION__, buffer_num, buffer_size);
	
    AvcDecoder_OMX* pAvcDecoder_OMX = (AvcDecoder_OMX*)aUserData;

    if (NULL == pAvcDecoder_OMX)
    {
        return 0;
    }

    int i;

    for (i = 0; i < 17; i++)
    {
        if (pAvcDecoder_OMX->iDecExtMalloced[i]== OMX_TRUE)
        {
            pAvcDecoder_OMX->iDecExtPmemHeap[i].clear();                
        }
    }
    	
    for(i =0; i < buffer_num; i++)
    {
        pAvcDecoder_OMX->iDecExtPmemHeap[i] = new MemoryHeapIon(SPRD_ION_DEV, buffer_size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
        int fd = pAvcDecoder_OMX->iDecExtPmemHeap[i]->getHeapID();
        if(fd>=0){
            int ret,phy_addr, size;
            ret = pAvcDecoder_OMX->iDecExtPmemHeap[i]->get_phy_addr_from_ion(&phy_addr, &size);
            if(ret)
            {
                OMX_H264DEC_ERR ("%s, %d: iDecExtPmemHeap get_phy_addr_from_ion fail", __FUNCTION__, __LINE__);
            }

            buffer_array[i] = (uint32)phy_addr;
            pAvcDecoder_OMX->iDecExtMalloced[i] = OMX_TRUE;
        }else
        {
            pAvcDecoder_OMX->iBufferAllocFail = OMX_TRUE;
            OMX_H264DEC_ERR ("%s, %d: ext mem pmempool  error\n", __FUNCTION__, __LINE__);
        return OMX_FALSE;	
        }
    }	

    pAvcDecoder_OMX->iNewSPSActivation = OMX_TRUE;

    return 1;
}

/* initialize video decoder */
OMX_BOOL AvcDecoder_OMX::InitializeVideoDecode_OMX()
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    
    MaxWidth = 0;
    MaxHeight = 0;
    MaxNumFs = 0;	

    int fd, ret,phy_addr, buffer_size;		

    buffer_size = (H264_DECODER_STREAM_BUFFER_SIZE+4);
    iStreamPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, buffer_size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
    fd = iStreamPmemHeap->getHeapID();
    if(fd>=0){
        ret = iStreamPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        if(ret){
            OMX_H264DEC_ERR("%s, %d: get_phy_addr_from_ion fail", __FUNCTION__, __LINE__);
        }

        iStreamBuf_p = (void *)phy_addr;
        iStreamBuf_v = (void *)iStreamPmemHeap->base();            
        OMX_H264DEC_DEBUG ("iStreamPmemHeap allocate successful!");
    }else
    {
        iBufferAllocFail = OMX_TRUE;
        OMX_H264DEC_ERR ("%s: cannot malloc VID internal buff", __FUNCTION__);
        return OMX_FALSE;	
    }

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;	

    buffer_size = H264_DECODER_INTERNAL_BUFFER_SIZE;
    iORbufferPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, buffer_size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
    fd = iORbufferPmemHeap->getHeapID();
    if(fd>=0){
        ret = iORbufferPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        if(ret){
            OMX_H264DEC_ERR("%s, %d: get_phy_addr_from_ion fail", __FUNCTION__, __LINE__);
        }

        iORBuf_v = (void *)iORbufferPmemHeap->base();
        codec_buf.common_buffer_ptr_phy = (void *)phy_addr;
        codec_buf.common_buffer_ptr =(uint8 *)iORBuf_v;
        codec_buf.size = buffer_size;
            
        OMX_H264DEC_DEBUG ("iORbufferPmemHeap allocate successful!");
    }else
    {
        iBufferAllocFail = OMX_TRUE;
        OMX_H264DEC_ERR ("%s: cannot malloc VID internal buff", __FUNCTION__);
        return OMX_FALSE;	
    }

//    video_format.video_std = STREAM_ID_H264;
    video_format.frame_width = 0;
    video_format.frame_height = 0;	
    video_format.p_extra = NULL;
    video_format.i_extra = 0;
    H264Dec_RegBufferCB(VSP_bind_cb,VSP_unbind_cb,(void *)this);
    H264Dec_RegMallocCB( AvcDecoder_OMX::VSP_malloc_cb);
    
#ifdef AVC_ES_DUMP
    fp_es = fopen(fn_es, "wb");
    if (fp_es != NULL)
    {
        fwrite(video_format.p_extra, 1, video_format.i_extra, fp_es);
    }
#endif

    MMDecRet InitRet = H264DecInit(&codec_buf,&video_format);
		
    if(InitRet!=MMDEC_OK)
        return OMX_FALSE;

    return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::FlushOutput_OMX(OMX_BUFFERHEADERTYPE **aOutBufferForRendering)
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    return (OMX_BOOL)H264Dec_GetLastDspFrm((void **)aOutBufferForRendering);
}

/* Initialization routine */
OMX_ERRORTYPE AvcDecoder_OMX::AvcDecInit_OMX()
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    if (OMX_FALSE == InitializeVideoDecode_OMX())
    {
        return OMX_ErrorInsufficientResources;
    }

    iAvcActiveFlag = OMX_FALSE;
    iSkipToIDR = OMX_TRUE ;//OMX_FALSE;
    iNewSPSActivation = OMX_FALSE;

    iReconfigWidth = 0;
    iReconfigHeight = 0;
    iReconfigStride = 0;
    iReconfigSliceHeight = 0;

    iStreamBuf_v = NULL;
    iORBuf_v = NULL;
    iBufferAllocFail = OMX_FALSE;

    for (int i = 0; i < 17; i++)
    {
        iDecExtMalloced[i] = OMX_FALSE;
    }
    
    return OMX_ErrorNone;
}

void dump_yuv( OMX_U8* pBuffer,OMX_U32 aInBufSize)
{
	FILE *fp = fopen("/data/video.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}
/*Decode routine */
OMX_BOOL AvcDecoder_OMX::AvcDecodeVideo_OMX(OMX_BOOL *need_new_pic,OMX_BUFFERHEADERTYPE *aOutBuffer, // empty output buffer that the component provides to the decoder
        OMX_U32* aOutputLength,OMX_BUFFERHEADERTYPE **aOutBufferForRendering, // output buffer for rendering. Initially NULL, but if there is output to be flushed out
        // this ptr will be updated to point to buffer that needs to be rendered
        OMX_U8** aInputBuf, OMX_U32* aInBufSize,
        OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
        OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag, OMX_BOOL *notSupport)
{

   OMX_H264DEC_DEBUG ("%s: %d,%d: %x,%x,%x,%x,%x,%x\n",__FUNCTION__, *aInBufSize,aMarkerFlag,(*aInputBuf)[0],(*aInputBuf)[1],(*aInputBuf)[2],(*aInputBuf)[3],(*aInputBuf)[4],(*aInputBuf)[5]);
	
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
	 OMX_H264DEC_ERR ("%s: dec not support because of buffer alloc fail", __FUNCTION__);
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
                return OMX_FALSE;
        }
    }else
    {
        pNalBuffer = *aInputBuf+ offset;
        NalSize = *aInBufSize- offset;
        //Assuming that the buffer with marker bit contains one full NAL
        *aInBufSize = 0;
    }
    
    int nal_unit_type = pNalBuffer[0] & 0x1f;

    int picPhyAddr = (static_cast<OpenmaxAvcAO * > (ipOMXComponent))->FindPhyAddr((uint32)aOutBuffer->pBuffer);
    if(!picPhyAddr)
    {
        OMX_H264DEC_ERR ("AvcDecoder_OMX FindPhyAddr failed");
        return OMX_FALSE; 
    }

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    if((static_cast<OpenmaxAvcAO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
    {
    	int width = aPortParam->format.video.nStride;
    	int height = aPortParam->format.video.nSliceHeight;
    	Rect bounds(width, height);
    	void *vaddr;

    	if(mapper.lock((const native_handle_t*)aOutBuffer->pBuffer, GRALLOC_USAGE_SW_WRITE_RARELY, bounds, &vaddr))
    	{
            OMX_H264DEC_ERR ("%s: mapper.lock fail %x", __FUNCTION__, aOutBuffer->pBuffer);
            return OMX_FALSE;			
    	}	
	H264Dec_SetCurRecPic((uint8*)(vaddr),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }else
    {	
       OMX_H264DEC_DEBUG ("%s: set cur pic  %x,%x,%x\n", __FUNCTION__, aOutBuffer->pBuffer,aOutBuffer,picPhyAddr);
       H264Dec_SetCurRecPic((uint8*)(aOutBuffer->pBuffer),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }

    MMDecInput dec_in;
    MMDecOutput dec_out;
	
    if(NalSize > H264_DECODER_STREAM_BUFFER_SIZE)
    {
    	    OMX_H264DEC_ERR ("%s: input stream too big %d\n", __FUNCTION__, NalSize);
	    return OMX_FALSE;
    }
    
    ((uint8 *) iStreamBuf_v)[0] = 0x0;
    ((uint8 *) iStreamBuf_v)[1] = 0x0;
    ((uint8 *) iStreamBuf_v)[2] = 0x0;
    ((uint8 *) iStreamBuf_v)[3] = 0x1;
    oscl_memcpy((void *)iStreamBuf_v+4, pNalBuffer,NalSize);
    dec_in.pStream_phy= (uint8 *) iStreamBuf_p;
    dec_in.dataLen = NalSize+4;
    dec_in.beLastFrm = 0;
    dec_in.expected_IVOP = iSkipToIDR;
    dec_in.beDisplayed = 1;
    dec_in.err_pkt_num = 0;

    dec_out.frameEffective = 0;	

#if PROFILING_ON
        OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

#ifdef AVC_ES_DUMP
    if (fp_es != NULL)
    {
        fwrite(dec_in.pStream, 1, dec_in.dataLen, fp_es);
    }
#endif


    OMX_U32 Start_decode = OsclTickCount::TickCount();
    Status = H264DecDecode(&dec_in,&dec_out);
    OMX_U32 Stop_decode = OsclTickCount::TickCount();
    OMX_H264DEC_DEBUG("H264DecDecode (0x%x -- %d) return %d, consumed %d ms\n", dec_in.pStream_phy, dec_in.dataLen, Status, Stop_decode-Start_decode);

#if PROFILING_ON
        OMX_U32 EndTime = OsclTickCount::TickCount();
        iTotalTicks += (EndTime - StartTime);
#endif

    if((static_cast<OpenmaxAvcAO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX]){
	if(mapper.unlock((const native_handle_t*)aOutBuffer->pBuffer))
	{
            OMX_H264DEC_ERR ("%s: mapper.unlock fail %x", __FUNCTION__, aOutBuffer->pBuffer);	
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
        //iSkipToIDR = OMX_FALSE;
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
            iSkipToIDR = OMX_FALSE;
        }

        return OMX_TRUE;
    }else
    {
       iSkipToIDR = OMX_TRUE;
       OMX_H264DEC_ERR ("%s: dec err %d\n", __FUNCTION__, (*iFrameCount));    
       if(Status == MMDEC_NOT_SUPPORTED)
        {
            OMX_H264DEC_ERR ("%s: dec format not support", __FUNCTION__);
            *notSupport = OMX_TRUE;
            return  OMX_FALSE;
       }

    if(7 == nal_unit_type)//sps
    	{
		aPortParam->format.video.nFrameWidth = dec_out.frame_width;
		aPortParam->format.video.nFrameHeight = dec_out.frame_height;
                aPortParam->format.video.nStride =  dec_out.frame_width;
                aPortParam->format.video.nSliceHeight = dec_out.frame_height;
	        
		 // finally, compute the new minimum buffer size.
		 // Decoder components always output YUV420 format
		aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;

			OMX_H264DEC_INFO ("%d, %d",aPortParam->format.video.nStride,aPortParam->format.video.nSliceHeight);
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
    	}
	
    	return  OMX_FALSE;
    }
}

OMX_ERRORTYPE AvcDecoder_OMX::AvcDecDeinit_OMX()
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    
    H264DecRelease();

    if(iORBuf_v)
    {
        iORbufferPmemHeap.clear();
        iORBuf_v = NULL;
    }

    for (int i = 0; i < 17; i++)
    {
        if (iDecExtMalloced[i]== OMX_TRUE)
        {
            iDecExtPmemHeap[i].clear();                
        }
    }
    		

    if(iStreamBuf_v)
    {
        iStreamPmemHeap.clear();
        iStreamBuf_v = NULL;
    }
    
#ifdef AVC_ES_DUMP
    fclose(fp_es);
#endif
   
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
static  AVCDec_Status PVAVCAnnexBGetNALUnit(uint8 *bitstream, uint8 **nal_unit, int *size)
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
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
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
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

    return AVCDEC_SUCCESS;
}


void AvcDecoder_OMX::ResetDecoder()
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    ReleaseReferenceBuffers();
}

void AvcDecoder_OMX::ReleaseReferenceBuffers()
{
    OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
    H264Dec_ReleaseRefBuffers();
}

void AvcDecoder_OMX::CalculateBufferParameters(OMX_PARAM_PORTDEFINITIONTYPE* aPortParam)
{
   OMX_H264DEC_INFO ("%s, %d", __FUNCTION__, __LINE__);
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

    return ;
}

