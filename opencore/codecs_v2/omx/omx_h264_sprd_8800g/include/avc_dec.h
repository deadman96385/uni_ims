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
#ifndef AVC_DEC_H_INCLUDED
#define AVC_DEC_H_INCLUDED

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif
 
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#include <sys/ioctl.h>
#include "sprd_vsp.h"
#include "ion_sprd.h"

#include <binder/MemoryHeapIon.h>
#define SPRD_ION_DEV "/dev/ion"

#define LOG_TAG "VSP"
#include <utils/Log.h>

//#define  SCI_TRACE_LOW   LOGI
#define OMX_H264DEC_DEBUG //LOGD
#define OMX_H264DEC_INFO  ALOGI
#define OMX_H264DEC_ERR  ALOGE

using namespace android;

	
#define H264_DECODER_INTERNAL_BUFFER_SIZE (70*1024+256*512*480/256)//170*1024 

#define H264_DECODER_STREAM_BUFFER_SIZE 1024*1024


/**
 This enumeration is used for the status returned from the library interface.
*/
typedef enum
{
    /**
    The followings are fail with details. Their values are negative.
    */
    AVCDEC_NO_DATA = -4,
    AVCDEC_NOT_SUPPORTED = -3,
    /**
    Fail information
    */
    AVCDEC_NO_BUFFER = -2, /* no output picture buffer available */
    AVCDEC_MEMORY_FAIL = -1, /* memory allocation failed */
    AVCDEC_FAIL = 0,
    /**
    Generic success value
    */
    AVCDEC_SUCCESS = 1,
    AVCDEC_PICTURE_OUTPUT_READY = 2,
    AVCDEC_PICTURE_READY = 3,

    /**
    The followings are success with warnings. Their values are positive integers.
    */
    AVCDEC_NO_NEXT_SC = 4,
    AVCDEC_REDUNDANT_FRAME = 5,
    AVCDEC_CONCEALED_FRAME = 6  /* detect and conceal the error */
} AVCDec_Status;

#define MAX_NUM_REF_FRAMES_PLUS_ONE 17


class AvcDecoder_OMX
{
    public:
        static int g_h264_dec_inst_num;
        AvcDecoder_OMX(class OmxComponentBase *pComp)
        {
            // initialize members

            FrameSize = 0;
            MaxNumFs = 0;
            MaxWidth = 0;
            MaxHeight = 0;

            CurrInputTimestamp = 0;
            InputBytesConsumed = 0;
            iAvcActiveFlag = OMX_FALSE;
            iSkipToIDR = OMX_FALSE;

            ipOMXComponent = pComp;
            iNewSPSActivation = OMX_FALSE;
            iReconfigWidth = 0;
            iReconfigHeight = 0;
            iReconfigStride = 0;
            iReconfigSliceHeight = 0;

#if PROFILING_ON
            iTotalTicks = 0;
#endif

            iExternalBufferWasSet = OMX_FALSE;
            iStreamBufferWasSet = OMX_FALSE;

            iDecoder_int_buffer_ptr = NULL;
	     iDecoder_ext_cache_buffer_ptr = NULL;
	 	iStream_buffer_ptr = NULL;
         iDecH264WasSw = OMX_FALSE;
        };

        ~AvcDecoder_OMX() { };

        uint32          FrameSize;
        OMX_U32         MaxNumFs;
        OMX_S32         MaxWidth;
        OMX_S32         MaxHeight;

        OMX_TICKS       CurrInputTimestamp;
        OMX_U32         InputBytesConsumed;
        OMX_BOOL        iAvcActiveFlag;
        OMX_BOOL        iSkipToIDR;
        OMX_BOOL        iNewSPSActivation;
        OMX_S32         iReconfigWidth;
        OMX_S32         iReconfigHeight;
        OMX_S32         iReconfigStride;
        OMX_S32         iReconfigSliceHeight;

#if PROFILING_ON
        OMX_U32 iTotalTicks;
#endif

        OMX_ERRORTYPE AvcDecInit_OMX();

        OMX_BOOL AvcDecodeVideo_OMX(OMX_BOOL *need_new_pic,OMX_BUFFERHEADERTYPE *aOutBuffer, OMX_U32* aOutputLength,OMX_BUFFERHEADERTYPE ** aOutBufferForRendering,
                                    OMX_U8** aInputBuf, OMX_U32* aInBufSize,
                                    OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
                                    OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag,
                                    OMX_BOOL *aResizeFlag,OMX_BOOL *notSupport);

        OMX_ERRORTYPE AvcDecDeinit_OMX();

        OMX_BOOL InitializeVideoDecode_OMX();

        OMX_BOOL FlushOutput_OMX(OMX_BUFFERHEADERTYPE **aOutBuffer);

        AVCDec_Status GetNextFullNAL_OMX(uint8** aNalBuffer, int* aNalSize, OMX_U8* aInputBuf, OMX_U32* aInBufSize);

        static int ActivateSPS_OMX(void* aUserData, uint width,uint height, uint aNumBuffers);
        static int FlushCache_OMX(void* aUserData, int *vadr,int *paddr,int size);
        void ResetDecoder(); // for repositioning

        void ReleaseReferenceBuffers();

        void CalculateBufferParameters(OMX_PARAM_PORTDEFINITIONTYPE* aPortParam);

        OmxComponentBase *ipOMXComponent;
        OMX_BUFFERHEADERTYPE *pCurrentBufferHdr;
        OMX_BOOL iDecH264WasSw;
private:
       void *iDecoder_int_buffer_ptr;
	void *iDecoder_ext_cache_buffer_ptr;
	void *iStream_buffer_ptr;
       OMX_BOOL iExternalBufferWasSet;
       OMX_BOOL iStreamBufferWasSet;

         sp<MemoryHeapIon> iDecExtPmemHeap;
         OMX_U32  iDecExtVAddr;
         OMX_U32  iDecExtPhyAddr;

         sp<MemoryHeapIon> iCMDbufferPmemHeap;
         OMX_U32  iCMDbufferVAddr;
         OMX_U32  iCMDbufferPhyAddr;

	 OMX_BOOL iBufferAllocFail;	 
};

typedef class AvcDecoder_OMX AvcDecoder_OMX;

#endif  //#ifndef AVC_DEC_H_INCLUDED

