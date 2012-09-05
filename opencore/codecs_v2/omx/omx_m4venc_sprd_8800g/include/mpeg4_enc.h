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
#ifndef MPEG4_ENC_H_INCLUDED
#define MPEG4_ENC_H_INCLUDED


#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif

#ifndef OSCL_INT64_UTILS_H_INCLUDED
#include "oscl_int64_utils.h"
#endif

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#include "mp4_basic.h"

#include <sys/ioctl.h>
#include "sprd_vsp.h"

#include <binder/MemoryHeapIon.h>
#define SPRD_ION_DEV "/dev/ion"

#define SPRD_VSP_DRIVER "/dev/sprd_vsp"

using namespace android;

#define PMEM_DRIVER "/dev/pmem_adsp"

const uint32 DEFAULT_VOL_HEADER_LENGTH = 28;

typedef unsigned char UChar;
typedef char Char;
typedef unsigned int UInt;
typedef int Int;
typedef unsigned short UShort;
typedef short Short;
typedef unsigned int Bool;
typedef unsigned long ULong;

#define PV_TRUE  1
#define PV_FALSE 0

enum
{
    MODE_H263 = 0,
    MODE_MPEG4
};

typedef enum
{
    SHORT_HEADER,
    SHORT_HEADER_WITH_ERR_RES,
    H263_MODE,
    H263_MODE_WITH_ERR_RES,
    DATA_PARTITIONING_MODE,
    COMBINE_MODE_NO_ERR_RES,
    COMBINE_MODE_WITH_ERR_RES

} MP4EncodingMode;

class Mpeg4Encoder_OMX
{
    public:

        Mpeg4Encoder_OMX(class OmxComponentBase *pComp);

        OMX_ERRORTYPE Mp4EncInit(OMX_S32 iEncMode,
                                 OMX_VIDEO_PORTDEFINITIONTYPE aInputParam,
                                 OMX_CONFIG_ROTATIONTYPE aInputOrientationType,
                                 OMX_VIDEO_PORTDEFINITIONTYPE aEncodeParam,
                                 OMX_VIDEO_PARAM_MPEG4TYPE aEncodeMpeg4Param,
                                 OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE aErrorCorrection,
                                 OMX_VIDEO_PARAM_BITRATETYPE aRateControlType,
                                 OMX_VIDEO_PARAM_QUANTIZATIONTYPE aQuantType,
                                 OMX_VIDEO_PARAM_MOTIONVECTORTYPE aSearchRange,
                                 OMX_VIDEO_PARAM_INTRAREFRESHTYPE aIntraRefresh,
                                 OMX_VIDEO_PARAM_H263TYPE aH263Type,
                                 OMX_VIDEO_PARAM_PROFILELEVELTYPE* aProfileLevel);


        OMX_BOOL Mp4EncodeVideo(OMX_U8*    aOutBuffer,
                                OMX_U32*   aOutputLength,
                                OMX_BOOL*  aBufferOverRun,
                                OMX_U8**   aOverBufferPointer,
                                OMX_U8*    aInBuffer,
                                 OMX_U8*    aInBuffer_phy,
                                OMX_U32    aInBufSize,
                                OMX_TICKS  aInTimeStamp,
                                OMX_TICKS* aOutTimeStamp,
                                OMX_BOOL*  aSyncFlag);

        OMX_ERRORTYPE Mp4RequestIFrame();
        OMX_BOOL Mp4UpdateBitRate(OMX_U32 aEncodedBitRate);
        OMX_BOOL Mp4UpdateFrameRate(OMX_U32 aEncodeFramerate);
        OMX_BOOL Mp4UpdateIFrameInterval(OMX_U32 aIntraPeriod);

        OMX_ERRORTYPE Mp4EncDeinit();

        OMX_ERRORTYPE Mp4OutBufferSize(OMX_U32 *aMaxVideoFrameSize);

        OMX_BOOL Mp4GetVolHeader(OMX_U8* aOutBuffer, OMX_U32* aOutputLength);

#if PROFILING_ON
        // Profile Statistics
        struct PVEncNodeStats
        {
            OMX_U32 iTotalNumFrames;
            OMX_U32 iNumFramesEncoded;
            OMX_U32 iDuration;  //in milli seconds
            OMX_U32 iTotalEncTime;
            OMX_U32 iColorConversionTime;
        };

        PVEncNodeStats iProfileStats;
#endif

        OmxComponentBase *ipOMXComponent;

    private:

        void CopyToYUVIn(uint8* YUV, int width, int height, int width_16, int height_16);


        OMX_BOOL             iInitialized;
        OMX_COLOR_FORMATTYPE iVideoFormat;

        int     iSrcWidth;
        int     iSrcHeight;
        int     iFrameOrientation;
        uint32  iSrcFrameRate;
        uint8*  iYUVIn;
        uint8*  iYUVIn_phy;	
	sp<MemoryHeapIon> iYUVInPmemHeap;

        uint8*  iVideoIn;
	uint8*  iVideoIn_phy;	

        OMX_TICKS iPreSrcDitherms;
        OMX_TICKS  iNextModTime;
	OMX_U32 iTimeIncRes;
	OMX_U32 iTickPerSrc;	

        MP4EncodingMode ENC_Mode;

        OMX_U8 iVolHeader[DEFAULT_VOL_HEADER_LENGTH]; /** Vol header */
        OMX_U32 iVolHeaderSize;
        OMX_BOOL iModTimeInitialized;

	OMX_BOOL iEncSycFrame;	
        OMX_U32 iIFrameInterval;	
	OMX_U32 iFrameTypeCounter;
	VOP_PRED_TYPE_E iPredType;	
	OMX_U32 iVBVSize;
	OMX_U32 iEncodeMode;
	
	OMX_S32 iBs_remain_len;	
	OMX_U32 iBps;
       OMX_TICKS  iLastPicTime;
	OMX_U32 iLastPicBytesEncoded;
		
	uint8*  iVIDInterBuf;

        uint8*  iEncExtBuf;
        uint8*  iEncExtBuf_phy;		
	sp<MemoryHeapIon> iEncExtPmemHeap;

 	OMX_S32 iVsp_fd;
        void *iVsp_addr;

	OMX_U32 iLogCount;		
};


#endif ///#ifndef MPEG4_ENC_H_INCLUDED
