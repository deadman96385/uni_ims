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
#ifndef MPEG4_DEC_H_INCLUDED
#define MPEG4_DEC_H_INCLUDED

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#include <sys/ioctl.h>
#include "sprd_vsp.h"

#include <binder/MemoryHeapIon.h>
#define SPRD_ION_DEV "/dev/ion"

#define LOG_TAG "VSP"
#include <utils/Log.h>

//#define  SCI_TRACE_LOW   LOGI
#define OMX_MP4DEC_DEBUG //LOGD
#define OMX_MP4DEC_INFO  ALOGI
#define OMX_MP4DEC_ERR  ALOGE


using namespace android;

#define MPEG4_DECODER_INTERNAL_BUFFER_SIZE 10*1024
#define MPEG4_DECODER_STREAM_BUFFER_SIZE 1024*1024



//#define PV_TRUE  1
//#define PV_FALSE 0

typedef enum
{
	H263_MODE = 0,MPEG4_MODE,
	FLV_MODE,
	UNKNOWN_MODE	
}MP4DecodingMode;

class Mpeg4Decoder_OMX
{
    public:
        static int g_mpeg4_dec_inst_num;
        ~Mpeg4Decoder_OMX() {};
        Mpeg4Decoder_OMX(class OmxComponentBase *pComp);

        OMX_S32 InitializeVideoDecode(OMX_S32* aWidth, OMX_S32* aHeight,
                                      OMX_U8** aBuffer, OMX_S32* aSize, OMX_S32 mode,
                                      OMX_BOOL aDeBlocking, OMX_BOOL *notSupport);

        OMX_ERRORTYPE Mp4DecInit();


        OMX_BOOL Mp4DecodeVideo(OMX_BOOL *need_new_pic,OMX_BUFFERHEADERTYPE * aOutBuffer, OMX_U32* aOutputLength,
                                OMX_U8** aInputBuf, OMX_U32* aInBufSize,
                                OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
                                OMX_BOOL aDeBlocking,
                                OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag,
                                OMX_BUFFERHEADERTYPE **aOutBufferForRendering,void *aPrivate_data, OMX_BOOL *notSupport);

        OMX_ERRORTYPE Mp4DecDeinit();

	void ReleaseReferenceBuffers();

	OMX_BOOL FlushOutput_OMX( OMX_BUFFERHEADERTYPE **aOutBufferForRendering);
		static int FlushCache_OMX(void* aUserData, int *vadr,int *paddr,int size);
	void ResetDecoder();

        OMX_S32 GetVideoHeader(int32 aLayer, uint8 *aBuf, int32 aMaxSize);
        void CheckPortReConfig(OMX_BUFFERHEADERTYPE* aOutBuffer, OMX_S32 OldWidth, OMX_S32 OldHeight, OMX_BOOL *aResizeFlag, OMX_S32* aFrameCount);

        OMX_BOOL getDisplayRect(int32 *display_left, int32 *display_top, int32 *display_width, int32 *display_height);

        OMX_BOOL Mpeg4InitCompleteFlag;
        OmxComponentBase *ipOMXComponent;

#if PROFILING_ON
        OMX_U32 iTotalTicks;
#endif

	int iDecoder_sw_flag;
	int iDecoder_sw_vt_flag;

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

	 OMX_BOOL iSkipToIDR;

        MP4DecodingMode CodecMode;
        OMX_BOOL iReferenceYUVWasSet;
        OMX_U32  iFrameSize;
        OMX_U32 *ipRefCtrPreviousReferenceBuffer;
        OMX_S32 iDisplay_Width, iDisplay_Height;
        OMX_S32 iData_Width, iData_Height;
        OMX_S32 iShortVideoHeader;

        OMX_U8 VO_START_CODE1[4];
        OMX_U8 VOSH_START_CODE1[4];
        OMX_U8 VOP_START_CODE1[4];
        OMX_U8 H263_START_CODE1[3];

	 OMX_BOOL iBufferAllocFail;	
};


#endif ///#ifndef MPEG4_DEC_H_INCLUDED
