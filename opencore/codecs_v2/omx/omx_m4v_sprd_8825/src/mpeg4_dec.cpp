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

//#define MPEG4_ES_DUMP

#ifdef MPEG4_ES_DUMP
static FILE* fp_es = NULL;
static const char* const fn_es = "/data/video/input.mpeg4";
#endif


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
#define PVH263DEFAULTHEIGHT 288
#define PVH263DEFAULTWIDTH 352

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
//#include <binder/MemoryHeapPmem.h>
#include <linux/android_pmem.h>

#include "mmcodec.h"
#include "mpeg4dec.h"
#include "mp4_basic.h"
#include "ion_sprd.h"
// from m4v_config_parser.h
OSCL_IMPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *, int32 *);

using namespace android;

int VSP_bind_cb(void *userdata, void *pHeader,int flag)
{
	Mpeg4Decoder_OMX *pMpeg4Decoder_OMX = (Mpeg4Decoder_OMX *)userdata;
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
	OMX_MP4DEC_INFO ("%s: %x,%x,%d\n", __FUNCTION__, ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer,pHeader,pBCTRL->iRefCount);
        
        pBCTRL->iRefCount++;
	return 0;
}

int VSP_unbind_cb(void *userdata, void *pHeader,int flag)
{
    Mpeg4Decoder_OMX *pMpeg4Decoder_OMX = (Mpeg4Decoder_OMX *)userdata;
    BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    OMX_MP4DEC_INFO("%s: ref frame: 0x%x, %x; cnt=%d, isinQueue=%d", __FUNCTION__, 
            ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,
            pBCTRL->iRefCount,pBCTRL->iIsBufferInComponentQueue
    );

    if (pBCTRL->iRefCount  > 0)
    {   
        pBCTRL->iRefCount--;
    }

    if ( pBCTRL->iRefCount == 0  )
    {
        // if a buffer's ref count is 0 - it means that it is available.
        pMpeg4Decoder_OMX->ipOMXComponent->iNumAvailableOutputBuffers++;

        if(OMX_FALSE == pBCTRL->iIsBufferInComponentQueue)
        {
            // push back the buffer to queue.
            QueueType *pOutputQueue = pMpeg4Decoder_OMX->ipOMXComponent->GetOutputQueue();
            Queue(pOutputQueue,(void *)pHeader);
            pBCTRL->iIsBufferInComponentQueue = OMX_TRUE;
        }
    }

    return 0;
}

void Mpeg4Decoder_OMX::ReleaseReferenceBuffers()
{
    OMX_MP4DEC_INFO("%s, %d", __FUNCTION__, __LINE__); 
    MP4DecReleaseRefBuffers();
}

OMX_BOOL Mpeg4Decoder_OMX::FlushOutput_OMX( OMX_BUFFERHEADERTYPE **aOutBufferForRendering)
{ 
    OMX_MP4DEC_INFO("%s, %d", __FUNCTION__, __LINE__); 
    return (OMX_BOOL)MP4DecGetLastDspFrm((void **)aOutBufferForRendering);
}

void Mpeg4Decoder_OMX::ResetDecoder()
{
    OMX_MP4DEC_INFO("%s, %d", __FUNCTION__, __LINE__); 
    ReleaseReferenceBuffers();
}
	
Mpeg4Decoder_OMX::Mpeg4Decoder_OMX(OmxComponentBase *pComp)
{
    OMX_MP4DEC_INFO("%s, %d", __FUNCTION__, __LINE__); 
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
	
    iBufferAllocFail = OMX_FALSE;	

    iDecoder_sw_flag = 0;
    iDecoder_sw_vt_flag	 = 0;
}

/* Initialization routine */
OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecInit()
{
    OMX_MP4DEC_INFO ("%s, %d", __FUNCTION__, __LINE__); 
    Mpeg4InitCompleteFlag = OMX_FALSE;
    return OMX_ErrorNone;
}

void dump_yuv( OMX_U8* pBuffer,OMX_U32 aInBufSize)
{
	FILE *fp = fopen("/data/video.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}
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
#if 0 //buzilla 108794, 138373
	 iBufferAllocFail = OMX_FALSE;
#endif
	 OMX_MP4DEC_ERR ("%s: dec not support because of buffer alloc fail", __FUNCTION__);
	 return OMX_FALSE;
    }
	
   OMX_MP4DEC_DEBUG ("%s: %d aInBufSize %5d marker %d::buffer %x,%x,%x,%x,%x,%x\n", __FUNCTION__, (*aFrameCount),*aInBufSize,aMarkerFlag,(*aInputBuf)[0],(*aInputBuf)[1],(*aInputBuf)[2],(*aInputBuf)[3],(*aInputBuf)[4],(*aInputBuf)[5]);
	
    if ((Mpeg4InitCompleteFlag == OMX_FALSE) && (MPEG4_MODE == CodecMode))
    {
        if (!aMarkerFlag)
        {
            InitSize = GetVideoHeader(0, *aInputBuf, *aInBufSize);
        }else
        {
            InitSize = *aInBufSize;
        }	
	 
        if (OMX_TRUE != InitializeVideoDecode(&iDisplay_Width, &iDisplay_Height,aInputBuf, (OMX_S32*)&InitSize, MODE_MPEG4, aDeBlocking, notSupport))
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
        aPortParam->format.video.nFrameWidth =  ( (iDisplay_Width + 15) & (~15) );
        aPortParam->format.video.nFrameHeight =  ( (iDisplay_Height + 15) & (~15) );

        OMX_U32 min_stride = ((aPortParam->format.video.nFrameWidth + 15) & (~15));
        OMX_U32 min_sliceheight = ((aPortParam->format.video.nFrameHeight + 15) & (~15));

        aPortParam->format.video.nStride = min_stride;
        aPortParam->format.video.nSliceHeight = min_sliceheight;

#ifndef MP4CODEC_NO_PMEM
        if (!((min_stride <= 1280 && min_sliceheight <= 720) || (min_stride <= 720 && min_sliceheight <= 1280)))
        {
            iDecoder_sw_flag = OMX_TRUE;
        }else if((min_stride <= 320 && min_sliceheight <= 240) ||(min_stride <= 240 && min_sliceheight <= 320) )
        {
            iDecoder_sw_vt_flag = OMX_TRUE;
        }
#else
        if((min_stride <= 320 && min_sliceheight <= 240) ||(min_stride <= 240 && min_sliceheight <= 320) )
        {
            iDecoder_sw_vt_flag = OMX_TRUE;
        } else 
        {
            iDecoder_sw_flag = OMX_TRUE;
        }
#endif

        // finally, compute the new minimum buffer size.

        // Decoder components always output YUV420 format
#ifdef YUV_THREE_PLANE
        if(iDecoder_sw_flag)
        {
            aPortParam->format.video.nFrameWidth +=32;
            aPortParam->format.video.nFrameHeight +=32;
            aPortParam->format.video.nStride +=32;
            aPortParam->format.video.nSliceHeight +=32;
        }
#endif
        aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1 ;
 #ifdef YUV_THREE_PLANE
        if(iDecoder_sw_flag )
        {
            aPortParam->nBufferSize +=  8;// 8 extra byte for mc loading of V.
        }
 #endif
 
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
        aPortParam->format.video.nFrameWidth =  ( (iDisplay_Width + 15) & (~15) );
        aPortParam->format.video.nFrameHeight =  ( (iDisplay_Height + 15) & (~15) );

        OMX_U32 min_stride = ((aPortParam->format.video.nFrameWidth + 15) & (~15));
        OMX_U32 min_sliceheight = ((aPortParam->format.video.nFrameHeight + 15) & (~15));

        aPortParam->format.video.nStride = min_stride;
        aPortParam->format.video.nSliceHeight = min_sliceheight;

#ifndef MP4CODEC_NO_PMEM
        if (!((min_stride <= 1280 && min_sliceheight <= 720) || (min_stride <= 720 && min_sliceheight <= 1280)))
        {
            iDecoder_sw_flag = OMX_TRUE;
        }else if((min_stride <= 320 && min_sliceheight <= 240) ||(min_stride <= 240 && min_sliceheight <= 320) )
        {
            iDecoder_sw_vt_flag = OMX_TRUE;
        }
#else
        if((min_stride <= 320 && min_sliceheight <= 240) ||(min_stride <= 240 && min_sliceheight <= 320) )
        {
            iDecoder_sw_vt_flag = OMX_TRUE;
        }else
        {
            iDecoder_sw_flag = OMX_TRUE;
        }
#endif

        // finally, compute the new minimum buffer size.

        // Decoder components always output YUV420 format
#ifdef YUV_THREE_PLANE
        if(iDecoder_sw_flag)
        {
            aPortParam->format.video.nFrameWidth +=32;
            aPortParam->format.video.nFrameHeight +=32;
            aPortParam->format.video.nStride +=32;
            aPortParam->format.video.nSliceHeight +=32;
        }
#endif
        aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1 ;
 #ifdef YUV_THREE_PLANE
        if(iDecoder_sw_flag )
        {
            aPortParam->nBufferSize +=  8;// 8 extra byte for mc loading of V.
        }
 #endif

        iFrameSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride);
        iData_Width = aPortParam->format.video.nFrameWidth;
        iData_Height = aPortParam->format.video.nFrameHeight;

        // in case of h263 - port reconfig is pretty common - we'll alos have to do SetReferenceYUV later

        CheckPortReConfig(aOutBuffer, OldWidth, OldHeight, aResizeFlag, aFrameCount);
        return OMX_TRUE;
    }

    if(!iExternalBufferWasSet)
    {
        if( -12 != getpriority(PRIO_PROCESS, 0)){
            setpriority(PRIO_PROCESS, 0, -12);//@jgdu
        }
        
        OMX_MP4DEC_INFO ("%s: width %d,height %d, sw_vt_flag %d, sw_flag %d\n", __FUNCTION__, aPortParam->format.video.nFrameWidth,aPortParam->format.video.nFrameHeight, iDecoder_sw_vt_flag, iDecoder_sw_flag);

        MMCodecBuffer extra_mem[3];
	uint32 extra_mem_size[3];
	uint32 mb_num_x = (aPortParam->format.video.nStride + 15)>>4;
	uint32 mb_num_y = (aPortParam->format.video.nSliceHeight + 15)>>4;
	uint32 mb_num_total = mb_num_x * mb_num_y;
	uint32 frm_size = (mb_num_total * 256);    

        if (iDecoder_sw_flag ||iDecoder_sw_vt_flag)
	{
            extra_mem_size[HW_NO_CACHABLE] = 0;	
            extra_mem_size[HW_CACHABLE] = 0;
	}else
	{
            extra_mem_size[HW_NO_CACHABLE] = mb_num_x*4*8*sizeof(int16);	//pTopCoeff
            extra_mem_size[HW_CACHABLE] = 1000*1024;	//MP4DEC_FRM_STRM_BUF_SIZE
            //extra_mem_size[HW_CACHABLE] += frm_size*8;	//cmd data and info

	/////////////////////CMD BUFFER
	if(mb_num_x*mb_num_y< 100){//qcif
		extra_mem_size[HW_CACHABLE] += frm_size*8 *2;/*vsp_cmd_data, vsp_cmd_info*/

	}
	else{
		extra_mem_size[HW_CACHABLE] += frm_size*4*2;/*vsp_cmd_data, vsp_cmd_info*/
	}
	
		
            if(aDeBlocking)
            {
		extra_mem_size[HW_NO_CACHABLE] += ((frm_size*3*3)>>1); //reconstruction frame buffer
            }
            extra_mem_size[HW_NO_CACHABLE] += 100*1024;	//rsv
            extra_mem_size[HW_CACHABLE] += 100*1024;	//rsv
	}

        //NO CACHABLE
	if (extra_mem_size[HW_NO_CACHABLE])
	{
            iDecExtPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, extra_mem_size[HW_NO_CACHABLE], MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);	
            int fd = iDecExtPmemHeap->getHeapID();
            if(fd>=0)
            {
                int ret,phy_addr, buffer_size;
                ret = iDecExtPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) 
                {
                    OMX_MP4DEC_ERR ("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
                }
			
	    	iDecExtPhyAddr =(OMX_U32)phy_addr;
	    	OMX_MP4DEC_DEBUG ("%s: ext mem pmempool %x,%x,%x,%x\n", __FUNCTION__, iDecExtPmemHeap->getHeapID(),iDecExtPmemHeap->base(),phy_addr,buffer_size);
                iDecExtVAddr = (OMX_U32)iDecExtPmemHeap->base();
                extra_mem[HW_NO_CACHABLE].common_buffer_ptr =(uint8 *) iDecExtVAddr;
                extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy = (void *)iDecExtPhyAddr;
                extra_mem[HW_NO_CACHABLE].size = extra_mem_size[HW_NO_CACHABLE];
            }else
            {
		iBufferAllocFail = OMX_TRUE;
		OMX_MP4DEC_ERR ("%s: ext mem pmempool  error", __FUNCTION__);
	    	return OMX_FALSE;	
            }
        }

    //CACHABLE
        if (extra_mem_size[HW_CACHABLE])
        {
            iCMDbufferPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, extra_mem_size[HW_CACHABLE], 0, (1<<31)|ION_HEAP_CARVEOUT_MASK);	
            int fd = iCMDbufferPmemHeap->getHeapID();
            if(fd>=0)
            {
                int ret,phy_addr, buffer_size;
                ret = iCMDbufferPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if (ret)
                {
                    OMX_MP4DEC_ERR ("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
                }
			
                iCMDbufferPhyAddr =(OMX_U32)phy_addr;
	    	OMX_MP4DEC_DEBUG ("%s: ext mem pmempool %x,%x,%x,%x", __FUNCTION__, iCMDbufferPmemHeap->getHeapID(),iCMDbufferPmemHeap->base(),phy_addr,buffer_size);
                iCMDbufferVAddr = (OMX_U32)iCMDbufferPmemHeap->base();
                extra_mem[HW_CACHABLE].common_buffer_ptr =(uint8 *) iCMDbufferVAddr;
                extra_mem[HW_CACHABLE].common_buffer_ptr_phy = (void *)iCMDbufferPhyAddr;
                extra_mem[HW_CACHABLE].size = extra_mem_size[HW_CACHABLE];
            }else
            {
                iBufferAllocFail = OMX_TRUE;
                OMX_MP4DEC_ERR ("%s: ext mem pmempool  error", __FUNCTION__);
                return OMX_FALSE;	
            }
	}	        
	
    	//alloc cacheable ext buf 
	extra_mem_size[SW_CACHABLE] = mb_num_total * 6 * 2* sizeof(int32); 	//mb_info
	extra_mem_size[SW_CACHABLE] += 4 * 8 * sizeof(int16);				//pLeftCoeff
	extra_mem_size[SW_CACHABLE] += 6 * 64 * sizeof(int16);				//coef_block	
	extra_mem_size[SW_CACHABLE] += 100*1024; //rsv	
	if (iDecoder_sw_flag)
	{
            int32 ext_size_y = (mb_num_x * 16 + 16*2) * (mb_num_y * 16 + 16*2);
            int32 ext_size_c = ext_size_y >> 2;
            int32 i;
	
            extra_mem_size[SW_CACHABLE] += 4*8*mb_num_x*sizeof(int16);	//pTopCoeff
            extra_mem_size[SW_CACHABLE] += ((( 64*4*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrY 
            extra_mem_size[SW_CACHABLE] += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrU
            extra_mem_size[SW_CACHABLE] += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrV 
#ifndef YUV_THREE_PLANE	
            for (i = 0; i < 3; i++)
            {			
		extra_mem_size[SW_CACHABLE] += ((( ext_size_y + 255) >>8)<<8);	//imgYUV[0]
		extra_mem_size[SW_CACHABLE] += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[1]
		extra_mem_size[SW_CACHABLE] += ((( ext_size_c + 8 +255) >>8)<<8);	//imgYUV[2]// 8 extra byte for mc loading of V.  
            }
#endif	
	}else if ( iDecoder_sw_vt_flag)
	{
            int32 ext_size_y = (mb_num_x * 16 + 16*2) * (mb_num_y * 16 + 16*2);
            int32 ext_size_c = ext_size_y >> 2;
            int32 i;

            extra_mem_size[SW_CACHABLE] += mb_num_total * 1 * sizeof(uint8);		//mbdec_stat_ptr @Simon.Wang for vt
            extra_mem_size[SW_CACHABLE] += 4*8*mb_num_x*sizeof(int16);	//pTopCoeff
            extra_mem_size[SW_CACHABLE] += mb_num_total * (sizeof(uint32) + 6 *sizeof(uint32));	//dc_store,dp
            extra_mem_size[SW_CACHABLE] += ((( 64*4*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrY 
            extra_mem_size[SW_CACHABLE] += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrU
            extra_mem_size[SW_CACHABLE] += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrV 

            for (i = 0; i < 5; i++)	// 3+2. 2 for disp YUV.
            {			
		extra_mem_size[SW_CACHABLE] += ((( ext_size_y + 255) >>8)<<8);	//imgYUV[0]
		extra_mem_size[SW_CACHABLE] += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[1]
		extra_mem_size[SW_CACHABLE] += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[2]
            }	

            //if(aDeBlocking)
            if (1)
            {
		extra_mem_size[SW_CACHABLE] +=  ((( ext_size_y + 255) >>8)<<8);	//reconstruction frame buffer
            }
	}else
	{
            extra_mem_size[SW_CACHABLE] += mb_num_total * 1 * sizeof(uint8);		//mbdec_stat_ptr
            extra_mem_size[SW_CACHABLE] += mb_num_x * 4 * sizeof(int16);			//pTopLeftDCLine
            extra_mem_size[SW_CACHABLE] += mb_num_total * (sizeof(uint32) + 6 *sizeof(uint32));	//dc_store,dp
	}
    
    	iDecoder_ext_cache_buffer_ptr = oscl_malloc(extra_mem_size[SW_CACHABLE]+4);
    	if(iDecoder_ext_cache_buffer_ptr==NULL)
    	{
            iBufferAllocFail = OMX_TRUE;    	
            OMX_MP4DEC_ERR ("%s: iDecoder_ext_cache_buffer_ptr alloc  error", __FUNCTION__);
            return OMX_FALSE;	    	
    	}
    	extra_mem[SW_CACHABLE].common_buffer_ptr =(uint8 *)( ((int)iDecoder_ext_cache_buffer_ptr+3)&(~0x3));
    	extra_mem[SW_CACHABLE].size = extra_mem_size[SW_CACHABLE];

        MP4DecMemInit(extra_mem);
        iExternalBufferWasSet = OMX_TRUE;	
    }
	
    int picPhyAddr =  (static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->FindPhyAddr((uint32)aOutBuffer->pBuffer);	 
    if(!picPhyAddr)
    {
        OMX_MP4DEC_ERR ("Mpeg4Decoder_OMX FindPhyAddr failed");
        return OMX_FALSE; 
    }	

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    if((static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
    {
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
            OMX_MP4DEC_ERR ("%s: mapper.lock fail %x", __FUNCTION__, aOutBuffer->pBuffer);
            return OMX_FALSE;			
    	}
	MP4DecSetCurRecPic((uint8*)(vaddr),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }else
    {	
    	MP4DecSetCurRecPic((uint8*)(aOutBuffer->pBuffer),(uint8*)picPhyAddr,(void *)(aOutBuffer)); 
    }
#if PROFILING_ON
    OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

    MMDecInput dec_in;
    MMDecOutput dec_out;
	
    if(*aInBufSize > MPEG4_DECODER_STREAM_BUFFER_SIZE)
    {
        OMX_MP4DEC_ERR ("%s: input stream too big %d\n", __FUNCTION__, *aInBufSize);
        return OMX_FALSE;
    }

    oscl_memcpy((void *)iStream_buffer_ptr, *aInputBuf,*aInBufSize);
    dec_in.pStream = (uint8 *) iStream_buffer_ptr;
    dec_in.dataLen = *aInBufSize;
    dec_in.beLastFrm = 0;
    dec_in.expected_IVOP = iSkipToIDR;
    dec_in.beDisplayed = 1;
    dec_in.err_pkt_num = 0;

#ifdef MPEG4_ES_DUMP
    if (fp_es != NULL)
    {
        fwrite(dec_in.pStream, 1, dec_in.dataLen, fp_es);
    }
#endif


    int ret;

    dec_out.VopPredType = -1;
    dec_out.frameEffective = 0;
    OMX_U32 Start_decode = OsclTickCount::TickCount();
	
    MMDecRet decRet =	MP4DecDecode(&dec_in,&dec_out);
    Status =  (decRet == MMDEC_OK)?OMX_TRUE : OMX_FALSE;
	
    OMX_U32 Stop_decode = OsclTickCount::TickCount();
    OMX_MP4DEC_DEBUG("MP4DecDecode (0x%x -- %d) return %d, consumed %d ms\n", dec_in.pStream, dec_in.dataLen, Status, Stop_decode-Start_decode);
    
    if((static_cast<OpenmaxMpeg4AO * > (ipOMXComponent))->iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
    {
	if(mapper.unlock((const native_handle_t*)aOutBuffer->pBuffer))
	{
            OMX_MP4DEC_ERR ("%s: mapper.unlock fail %x", __FUNCTION__, aOutBuffer->pBuffer);	
	}
    }

    if(aMarkerFlag)
    {
   	    *aInBufSize = 0;
    }else
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

	if(dec_out.frameEffective)
        {
            *aOutBufferForRendering = (OMX_BUFFERHEADERTYPE *)dec_out.pBufferHeader;
	}else
        {
		*aOutBufferForRendering = NULL;
        }
        *aOutputLength = (iFrameSize * 3) >> 1;
        (*aFrameCount)++;
       OMX_MP4DEC_DEBUG ("%s: dec %d\n", __FUNCTION__, (*aFrameCount));
	   
	if(dec_out.VopPredType!=NVOP)
	{
            *need_new_pic = OMX_TRUE;
            iSkipToIDR = OMX_FALSE;
	}
	//g_frame_num++;
    } else
    {
        *aOutBufferForRendering = NULL;
        //*aInBufSize = InputSize;
        *aOutputLength = 0;
        OMX_MP4DEC_ERR ("%s: dec err %d,%x", __FUNCTION__, (*aFrameCount),g_mpeg4_dec_err_flag);	
    }
    return Status;
}

int Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num = 0;

int Mpeg4Decoder_OMX::FlushCache_OMX (void* aUserData,int* vaddr,int* paddr,int size)
{
#if (ION_DRIVER_VERSION == 1)
    //for bug#134992 : in kernel 3.4 , ION_DRIVER_VERSION == 1
    return 0; 
#else 
    OMX_MP4DEC_DEBUG ("%s: vaddr: 0x%x, paddr: 0x%x, size: 0x%x \n",__FUNCTION__, vaddr,paddr,size);

    Mpeg4Decoder_OMX* pMpeg4Decoder_OMX = (Mpeg4Decoder_OMX*)aUserData;
    int ret = pMpeg4Decoder_OMX->iCMDbufferPmemHeap->flush_ion_buffer((void *)vaddr,(void* )paddr,size);

    return ret;
#endif
}

OMX_S32 Mpeg4Decoder_OMX::InitializeVideoDecode(
    OMX_S32* aWidth, OMX_S32* aHeight, OMX_U8** aBuffer, OMX_S32* aSize, OMX_S32 mode, OMX_BOOL aDeBlocking , OMX_BOOL *notSupport)
{
    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;
    OMX_S32 notOK = OMX_TRUE;
    CodecMode = MPEG4_MODE;
	
    if (mode == MODE_H263)
    {
        CodecMode = H263_MODE;
    }
	
    OMX_MP4DEC_INFO ("%s: iDecoder_sw_flag: %d", __FUNCTION__, iDecoder_sw_flag);

     if(!iStreamBufferWasSet)
     {
        iStream_buffer_ptr = oscl_malloc(MPEG4_DECODER_STREAM_BUFFER_SIZE);
        if(iStream_buffer_ptr==NULL)
        {
            iBufferAllocFail = OMX_TRUE;
            OMX_MP4DEC_ERR ("%s: iStream_buffer_ptr alloc  error", __FUNCTION__);
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
    		OMX_MP4DEC_ERR ("%s: iDecoder_int_buffer_ptr alloc  error", __FUNCTION__);
		return OMX_FALSE;	    	
    	}
    }
    codec_buf.int_buffer_ptr =(uint8 *)( ((int)iDecoder_int_buffer_ptr+3)&(~0x3));
    codec_buf.int_size = MPEG4_DECODER_INTERNAL_BUFFER_SIZE;
    video_format.i_extra = *aSize;
    if( video_format.i_extra>0)
    {
        oscl_memcpy((void *)iStream_buffer_ptr,*aBuffer,*aSize);
	video_format.p_extra =(void *) iStream_buffer_ptr;
	uint32 first_32bits = ((*aBuffer)[0]<<24)|((*aBuffer)[1]<<16)|((*aBuffer)[2]<<8)|(*aBuffer)[3];
	if((first_32bits>>11) == 0x10)
	{
            if((first_32bits>>10) == 0x20)
            {
		CodecMode = H263_MODE;
            }else
            {
			CodecMode = FLV_MODE;
            } 
	}
    }else
    {
	video_format.p_extra = NULL;
    }

    OMX_MP4DEC_INFO ("Mp4 stream  mode %d\n",CodecMode);	

#ifdef MPEG4_ES_DUMP
    fp_es = fopen(fn_es, "wb");
    if (fp_es != NULL)
    {
        fwrite(video_format.p_extra, 1, video_format.i_extra, fp_es);
    }
#endif

    if(CodecMode == H263_MODE)
    {
    	video_format.video_std = ITU_H263;
    }else if(CodecMode == MPEG4_MODE)
    {
        video_format.video_std = MPEG4;
    }else
    {
    	video_format.video_std = FLV_V1;
    }
	
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    
    MP4DecRegBufferCB(VSP_bind_cb,VSP_unbind_cb,(void *)this);
    MP4Dec_RegFlushCacheCB( Mpeg4Decoder_OMX::FlushCache_OMX);
	
    video_format.uv_interleaved = 1;// todo jgdu
    notOK = MP4DecInit(&codec_buf,&video_format);

    MP4DecSetPostFilter(aDeBlocking);

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
	    	return OMX_FALSE;
    	}
	return OMX_TRUE;
    }else
    {
    	if (notOK == MMDEC_NOT_SUPPORTED )
    	{
    		*notSupport = OMX_TRUE;
    	}
    	return OMX_FALSE;
    }
}

OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecDeinit()
{
    OMX_BOOL Status;

    OMX_MP4DEC_INFO ("%s", __FUNCTION__);
    // at this point - the buffers have been freed already
    ipRefCtrPreviousReferenceBuffer = NULL;
    iReferenceYUVWasSet = OMX_FALSE;  
    
    MP4DecRelease();

    if( iDecoder_int_buffer_ptr)
    {
	oscl_free(iDecoder_int_buffer_ptr);
	iDecoder_int_buffer_ptr = NULL;
    }

    
#ifdef MPEG4_ES_DUMP
    fclose(fp_es);
#endif

    if(iExternalBufferWasSet)
    {
        if (!(iDecoder_sw_flag ||iDecoder_sw_vt_flag))
        {
            iDecExtPmemHeap.clear();
            if(iDecExtVAddr)
            {
                iDecExtVAddr = NULL;
            }
                
            iCMDbufferPmemHeap.clear();
            if(iCMDbufferVAddr)
            {
                iCMDbufferVAddr = NULL;
            }
        }
            
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

    OMX_MP4DEC_INFO ("%s", __FUNCTION__);
	
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
    OMX_MP4DEC_INFO ("%s", __FUNCTION__);
	
    if ((iData_Width != OldWidth) || (iData_Height != OldHeight))
    {
    	 OMX_MP4DEC_INFO ("%s: old [%d,%d] new [%d,%d]", __FUNCTION__, OldWidth,OldHeight,iData_Width,iData_Height);
        *aResizeFlag = OMX_TRUE;
    }
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


