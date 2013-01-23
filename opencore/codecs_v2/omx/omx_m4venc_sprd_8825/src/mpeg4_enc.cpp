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
static const char* const fn_es = "/data/video/output.mpeg4";
#endif


#include "mpeg4_enc.h"
#include "oscl_mem.h"

#if PROFILING_ON
#include "oscl_tickcount.h"
#endif

#include <utils/threads.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <media/hardware/MetadataBufferType.h>
//#include <ui/android_native_buffer.h>

#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

#include "omx_mpeg4enc_component.h"

#include <binder/MemoryHeapBase.h>


#include "mmcodec.h"
#include "mpeg4enc.h"
#include "video_common.h"

#include "mp4_basic.h"
#include "mp4enc_mode.h"


/* {SPL0, SPL1, SPL2, SPL3, SPL4a, SPL5, CPL1, CPL2, CPL2, CPL2} , SPL0: Simple Profile@Level0 , CPL1: Core Profile@Level1 */
/* {SSPL0, SSPL1, SSPL2, SSPL2, CSPL1, CSPL2, CSPL3, CSPL3} , SSPL0: Simple Scalable Profile@Level0, CPL1: Core Scalable Profile@Level1 */
typedef enum
{
    /* Non-scalable profile */
    SIMPLE_PROFILE_LEVEL0 = 0,
    SIMPLE_PROFILE_LEVEL1,      /*1 = 0b'0000,0001*/
    SIMPLE_PROFILE_LEVEL2,      /*2= 0b'0000,0010*/
    SIMPLE_PROFILE_LEVEL3,      /*3= 0b'0000,0011*/
    SIMPLE_PROFILE_LEVEL4A,     /*4= 0b'0000,0100*/
    SIMPLE_PROFILE_LEVEL5,      /*5= 0b'0000,0101*/
    CORE_PROFILE_LEVEL1,        /*6= 0b'0000,0110*/
    CORE_PROFILE_LEVEL2,        /*7= 0b'0000,0111*/
    MAX_BASE_PROFILE = CORE_PROFILE_LEVEL2,
} ProfileLevelIdType;


const uint8 DEFAULT_VOL_HEADER[DEFAULT_VOL_HEADER_LENGTH] =
{
    0x00, 0x00, 0x01, 0xB0, 0x08, 0x00, 0x00, 0x01,
    0xB5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x20, 0x00, 0x84, 0x40, 0xFA, 0x28, 0x2C,
    0x20, 0x90, 0xA2, 0x1F
};


Mpeg4Encoder_OMX::Mpeg4Encoder_OMX(class OmxComponentBase *pComp)
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mpeg4Encoder_OMX\n");

    ipOMXComponent = pComp;
	
    iModTimeInitialized = OMX_FALSE;
	
    iInitialized = OMX_FALSE;
    iYUVIn = NULL;
    iVIDInterBuf = NULL;
    iEncExtBuf = NULL;
	
    // Create a default VOL header
    oscl_memset(iVolHeader, 0, DEFAULT_VOL_HEADER_LENGTH);
    oscl_memcpy(iVolHeader, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH);
    iVolHeaderSize = DEFAULT_VOL_HEADER_LENGTH;

#if PROFILING_ON
    oscl_memset(&iProfileStats, 0, sizeof(PVEncNodeStats));
#endif

}



static OMX_ERRORTYPE  Mp4EncGetProfileLevelId(
        OMX_U32 mode,
        OMX_U32 omxProfile,
        OMX_U32 omxLevel,
        OMX_U8* pProfileLevelId) 
{
    uint8 profileLevelId = 0;

    if (mode == MODE_H263) {
        omxProfile = OMX_VIDEO_H263ProfileBaseline;

        switch (omxProfile) {
            case OMX_VIDEO_H263ProfileBaseline:
                if (omxLevel > OMX_VIDEO_H263Level45) {
                    return OMX_ErrorUnsupportedSetting;
                } else {
                    profileLevelId = CORE_PROFILE_LEVEL2;
                    break;
                }
                break;

            default:
                return OMX_ErrorUnsupportedSetting;
        }
    } else {  // MPEG4

        omxProfile = OMX_VIDEO_MPEG4ProfileSimple;

        switch (omxProfile) {
            case OMX_VIDEO_MPEG4ProfileSimple:
                switch (omxLevel) {
                    case OMX_VIDEO_MPEG4Level0b:
                        profileLevelId = SIMPLE_PROFILE_LEVEL0;
                        break;
                    case OMX_VIDEO_MPEG4Level1:
                        profileLevelId = SIMPLE_PROFILE_LEVEL1;
                        break;
                    case OMX_VIDEO_MPEG4Level2:
                        profileLevelId = SIMPLE_PROFILE_LEVEL2;
                        break;
                    case OMX_VIDEO_MPEG4Level3:
                        profileLevelId = SIMPLE_PROFILE_LEVEL3;
                        break;
                    case OMX_VIDEO_MPEG4Level4:
                    case OMX_VIDEO_MPEG4Level4a:
                        profileLevelId = SIMPLE_PROFILE_LEVEL4A;
                        break;
                    case OMX_VIDEO_MPEG4Level5:
                        profileLevelId = SIMPLE_PROFILE_LEVEL5;
                        break;
                    default:
                        return OMX_ErrorUnsupportedSetting;
                }
                break;

            default:
                return OMX_ErrorUnsupportedSetting;
        }
    }

    *pProfileLevelId = profileLevelId;
    return OMX_ErrorNone;
}

/* Initialization routine */
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncInit(OMX_S32 iEncMode,
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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE* aProfileLevel)
{
    MMEncVideoInfo encInfo;
    MMEncConfig encConfig;	
    OMX_U32  profile = 0;
    OMX_U32  level =0;
    OMX_U8  profileLevelId = 0;
	
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4EncInit\n");
    
    iEncodeMode = iEncMode;
    switch (iEncMode)
    {
        case MODE_H263:
        {
	    profile = (OMX_U32)aH263Type.eProfile;
            level = (OMX_U32)aH263Type.eLevel;		
        }
        break;

        case MODE_MPEG4:
        {
	    profile = (OMX_U32)aEncodeMpeg4Param.eProfile;
            level = (OMX_U32)aEncodeMpeg4Param.eLevel;
        }
        break;

        default:
        {
	    OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit error iEncMode %d \n",iEncMode);
            return OMX_ErrorUnsupportedSetting;
        }
    }
	
    if(iEncodeMode==MODE_H263)
    {
	encInfo.is_h263 = 1;
	encConfig.h263En = 1;
    }else
    {
	encInfo.is_h263 = 0;
	encConfig.h263En = 0;	
    }

     //Set the profile level of encoder
    if( OMX_ErrorNone !=  Mp4EncGetProfileLevelId(iEncodeMode,profile,level, &profileLevelId) )
    {
         OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit, profile_level error, 0x%x,%x,%x \n", iEncodeMode,profile,level);
        return OMX_ErrorUnsupportedSetting;
    }
    encConfig.profileAndLevel = profileLevelId;
	
    iSrcWidth = aInputParam.nFrameWidth;
    iSrcHeight = aInputParam.nFrameHeight;
    iSrcFrameRate = aInputParam.xFramerate;

    encInfo.frame_height = ((iSrcHeight + 15) >> 4) << 4;
    encInfo.frame_width = ((iSrcWidth+ 15) >> 4) << 4; 
	
    if ((OMX_COLOR_FormatYUV420SemiPlanar == aInputParam.eColorFormat)||(OMX_COLOR_FormatAndroidOpaque == aInputParam.eColorFormat))
    {
        OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eColorFormat %d \n",aInputParam.eColorFormat);
        iVideoFormat = aInputParam.eColorFormat;
	encInfo.uv_interleaved = 1;
    }
    else
    {
        OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eColorFormat error %d \n",aInputParam.eColorFormat);
        return OMX_ErrorUnsupportedSetting;
    }

    //Verify the input compression format
    if (OMX_VIDEO_CodingUnused != aInputParam.eCompressionFormat)
    {
        OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eCompressionFormat error\n");
        //Input port must have no compression supported
        return OMX_ErrorUnsupportedSetting;
    }


    if (OMX_TRUE == iInitialized)
    {
        /* clean up before re-initialized */
	Mp4EncDeinit();
    }

    // allocate iYUVIn
    if (((iSrcWidth & 0xF) || (iSrcHeight & 0xF))) /* Not multiple of 16 */
    {
	iYUVInPmemHeap = new MemoryHeapIon(SPRD_ION_DEV,(((((iSrcWidth + 15) >> 4) * ((iSrcHeight + 15) >> 4)) * 3) << 7),MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
 	 int fd = iYUVInPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) OMX_MP4ENC_ERR("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
		
		iYUVIn_phy =(uint8 *)phy_addr;
		 OMX_MP4ENC_DEBUG("Mpeg4Encoder_OMX yuvin mem pmempool %x,%x,%x,%x\n",iYUVInPmemHeap->getHeapID(),iYUVInPmemHeap->base(),phy_addr,buffer_size);
		
	 	iYUVIn =(uint8 *) iYUVInPmemHeap->base();
    	}else
    	{
    		OMX_MP4ENC_ERR("Mpeg4Encoder_OMX yuvin mem pmempool  error\n");
		return OMX_ErrorInsufficientResources;	
    	}		
    }

    iBps = aEncodeParam.nBitrate;
    encConfig.targetBitRate = aEncodeParam.nBitrate;
    encConfig.FrameRate = iSrcFrameRate>>16;
    encConfig.QP_IVOP = aQuantType.nQpI;
    encConfig.QP_PVOP = aQuantType.nQpP;
		
    switch (aRateControlType.eControlRate)
    {
        case OMX_Video_ControlRateDisable:
        {
	    encConfig.RateCtrlEnable = 0;
	    encConfig.vbv_buf_size = encConfig.targetBitRate;
        }
        break;

        case OMX_Video_ControlRateConstant:
        {
	    encConfig.RateCtrlEnable = 1;
	    encConfig.vbv_buf_size = encConfig.targetBitRate*0.5;
        }
        break;

        case OMX_Video_ControlRateVariable:
        {
	    encConfig.RateCtrlEnable = 1;
	    encConfig.vbv_buf_size = encConfig.targetBitRate*0.5;
        }
        break;

        default:
        OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit aRateControlType.eControlRate error\n");
            return OMX_ErrorUnsupportedSetting;
    }

#if 1
    if(encConfig.FrameRate > 1)
    {
        iAverageFrameBits = encConfig.targetBitRate /encConfig.FrameRate;;
    }
    else
    {
        iAverageFrameBits = encConfig.targetBitRate/20;         //default 20fps.
    }
    iLastFrameBitsEncoded = iAverageFrameBits;
#endif

    if(encInfo.frame_width>176)
    {
        // the VBV delay = 1s, 2s.
        encConfig.vbv_buf_size = encConfig.targetBitRate;   //(encConfig.targetBitRate<<1);
    }
    iVBVSize = encConfig.vbv_buf_size;

    //IPPPPPPPPPP, indicates I-frame followed by all P-frames
    if (0xFFFFFFFF == aEncodeMpeg4Param.nPFrames)
    {
	//iIFrameInterval = -1;	
	iIFrameInterval = 15;	
    }
    else
    {
	iIFrameInterval = 	aEncodeMpeg4Param.nPFrames + 1;
    }

    //No support for B Frames
    if (aEncodeMpeg4Param.nBFrames > 0)
    {
        return OMX_ErrorUnsupportedSetting;
    }

    //Encoder support only I and P frames picture type
    if (0 == (aEncodeMpeg4Param.nAllowedPictureTypes &
              (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP)))
    {
        return OMX_ErrorUnsupportedSetting;
    }

    if (OMX_VIDEO_PictureTypeI == aEncodeMpeg4Param.nAllowedPictureTypes) // I-only
    {
        iIFrameInterval = 1;
    }


    if ((aEncodeMpeg4Param.nTimeIncRes << 16) > iSrcFrameRate)
    { 
        iTimeIncRes = aEncodeMpeg4Param.nTimeIncRes;
    } 
    else
    {
        //Default value
       iTimeIncRes = 1000;
    }
    encInfo.time_scale = iTimeIncRes;
	
    if (iSrcFrameRate > 0)
    {
         iTickPerSrc = (iTimeIncRes << 16) / iSrcFrameRate;
    }

   OMX_MP4ENC_DEBUG("Mpeg4Encoder_OMX::Mp4EncInit:iSrcFrameRate %d,iTimeIncRes %d,iTickPerSrc %d\n",iSrcFrameRate,iTimeIncRes,iTickPerSrc);

    //Checking the range of parameters that the encoder can't support here
    // and return OMX_ErrorUnsupportedSetting

    /***** Initlaize the encoder *****/

    uint32 vidInterBufSize = (encInfo.frame_width/16)*7*4*2+14*4+6*4*3+68   +200 +4;
    iVIDInterBuf = (uint8*) oscl_malloc(vidInterBufSize);
    if (NULL == iVIDInterBuf)
    {
       	     OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit iVIDInterBuf alloc error\n");       
            return OMX_ErrorInsufficientResources;
    }
    uint32 vidExtBufSize = encInfo.frame_width* encInfo.frame_height*3/2*2  + ONEFRAME_BITSTREAM_BFR_SIZE + 300;
    uint8*  iEncExtBuf_phy = NULL;
    {
		iEncExtPmemHeap = new MemoryHeapIon(SPRD_ION_DEV,vidExtBufSize,MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
 	 	int fd = iEncExtPmemHeap->getHeapID();
		 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iEncExtPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) OMX_MP4ENC_ERR("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
		
		iEncExtBuf_phy =(uint8 *)phy_addr;
		OMX_MP4ENC_DEBUG("Mpeg4Encoder_OMX ext mem pmempool %x,%x,%x,%x\n",iEncExtPmemHeap->getHeapID(),iEncExtPmemHeap->base(),phy_addr,buffer_size);

		iEncExtBuf =(uint8 *) iEncExtPmemHeap->base();
    	}else
    	{
    		OMX_MP4ENC_ERR("Mpeg4Encoder_OMX ext  mem pmempool  error\n");
		return OMX_ErrorInsufficientResources;	
    	}
    }
    MMCodecBuffer 	InterMemBfr,ExtaMemBfr;
	
    InterMemBfr.common_buffer_ptr = (uint8 *)(((int) iVIDInterBuf+3)&(~0x3));
    InterMemBfr.size = vidInterBufSize-4;
    ExtaMemBfr.common_buffer_ptr = iEncExtBuf;
    ExtaMemBfr.common_buffer_ptr_phy = iEncExtBuf_phy;
    ExtaMemBfr.size	= vidExtBufSize;
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4EncInit: h263 %d,width %d,height %d,tiemscale %d,iIFrameInterval %d\n",encInfo.is_h263,encInfo.frame_width,encInfo.frame_height,encInfo.time_scale,iIFrameInterval);  	
    
#ifdef MPEG4_ES_DUMP
    fp_es = fopen(fn_es, "wb");
#endif	
		
    MMEncRet ret =  MP4EncInit(&InterMemBfr, &ExtaMemBfr,&encInfo);
    int status;
    if(ret == MMENC_OK)
        status = 	OMX_TRUE;
    else
	status = 	OMX_FALSE;		
    if (OMX_FALSE == status)
    {

        iInitialized = OMX_FALSE;
        OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncInit MP4EncInit error\n");  		
        return OMX_ErrorBadParameter;
    }
    //encConfig.RateCtrlEnable = 0;
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4EncInit: RateCtrlEnable %d,targetBitRate %d,FrameRate %d,QP_IVOP %d,QP_PVOP %d,profileAndLevel %d\n",encConfig.RateCtrlEnable,encConfig.targetBitRate,encConfig.FrameRate,encConfig.QP_IVOP,encConfig.QP_PVOP,encConfig.profileAndLevel);  		
    MP4EncSetConf(&encConfig);
	
    iInitialized = OMX_TRUE;
    iNextModTime = 0;
	
    iEncSycFrame = OMX_TRUE;
    iFrameTypeCounter = 0;

    //Update the vol header for non-h263 modes
    if (iEncodeMode == MODE_MPEG4)
    {
        // M4V output, get VOL header
        iVolHeaderSize = 32; // Encoder requires that buffer size is greater than vol header size (28)

	MMEncOut encOut;

	MP4EncGenHeader(&encOut);
	
        iVolHeaderSize = encOut.strmSize;
        oscl_memcpy(iVolHeader, (OsclAny*)encOut.pOutBuf, iVolHeaderSize);

#ifdef MPEG4_ES_DUMP
    if (fp_es != NULL)
    {
        fwrite(encOut.pOutBuf, 1, encOut.strmSize, fp_es);
    }
#endif
    }
	

    //Updating the profile and level from the encoder after initialization
    if (MODE_MPEG4 == iEncMode)
    {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level0;
    }
    //mode H263
    else
    {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level10;
    }

    return OMX_ErrorNone;

}

/* Request I frame routine */
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4RequestIFrame()
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4RequestIFrame\n");
    iEncSycFrame = OMX_TRUE;
    return OMX_ErrorNone;
}

/* Request Update BitRate routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateBitRate(OMX_U32 aEncodedBitRate)
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4UpdateBitRate %d\n",aEncodedBitRate);
    OMX_BOOL Status = OMX_FALSE;

    //Update the bit rate only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
    	MMEncConfig config;
   	MP4EncGetConf(&config);	
    	config.targetBitRate = aEncodedBitRate;
	iBps = aEncodedBitRate;
	int vid_pitch = ((iSrcWidth + 15) >> 4) << 4;	
	if(vid_pitch>176){	
		config.vbv_buf_size = aEncodedBitRate;
		iVBVSize  = aEncodedBitRate;
	}else{
		config.vbv_buf_size = aEncodedBitRate*0.5;
		iVBVSize  = aEncodedBitRate*0.5;
	}
    	MP4EncSetConf(&config);
	Status = OMX_TRUE;	
    }
    return  Status;
}

/* Request Update FrameRate routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateFrameRate(OMX_U32 aEncodeFramerate)
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4UpdateFrameRate %d\n",aEncodeFramerate);
    OMX_BOOL Status = OMX_FALSE;

    //Update the frame rate only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
        MMEncConfig config;
   	MP4EncGetConf(&config);	
        config.FrameRate = (aEncodeFramerate / (1 << 16));
    	MP4EncSetConf(&config);
	Status = OMX_TRUE;	
    }
    return Status;

}

/* Request Update I-FrameInterval routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateIFrameInterval(OMX_U32 aIntraPeriod)
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4UpdateIFrameInterval %d\n",aIntraPeriod);
    OMX_BOOL Status = OMX_FALSE;
    if (OMX_TRUE == iInitialized)
    {
         iIFrameInterval = aIntraPeriod;
	 Status = OMX_TRUE;	
    }
    return Status;

}

// This function gives the maximum output buffer requirement and
// can only be called after encoder's initialization
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4OutBufferSize(OMX_U32 *aMaxVideoFrameSize)
{
    *aMaxVideoFrameSize = ONEFRAME_BITSTREAM_BFR_SIZE;
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4OutBufferSize %d\n",*aMaxVideoFrameSize);
    return OMX_ErrorNone;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4GetVolHeader(OMX_U8* aOutBuffer, OMX_U32* aOutputLength)
{
    OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4GetVolHeader\n");
    //Send the VOL Header only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
        //Copy the vol header in the output buffer in case of m4v format
        if (iEncodeMode == MODE_MPEG4)
        {
            oscl_memcpy(aOutBuffer, iVolHeader, iVolHeaderSize);
            *aOutputLength = iVolHeaderSize;
            return OMX_TRUE;
        }
    }

    return OMX_FALSE;
}

#define MAX(a,b) ((a)>(b) ? (a):(b))
/*Encode routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4EncodeVideo(OMX_U8*    aOutBuffer,
        OMX_U32*   aOutputLength,
        OMX_BOOL*  aBufferOverRun,
        OMX_U8**   aOverBufferPointer,
        OMX_U8*    aInBuffer,
        OMX_U8*    aInBuffer_phy,
        OMX_U32    aInBufSize,
        OMX_TICKS  aInTimeStamp,
        OMX_TICKS* aOutTimeStamp,
        OMX_BOOL*  aSyncFlag)
{
    *aSyncFlag = OMX_FALSE;
    *aBufferOverRun = OMX_FALSE;

    if (OMX_FALSE == iModTimeInitialized)
    {
        if( -12 != getpriority(PRIO_PROCESS, 0)){
	  	setpriority(PRIO_PROCESS, 0, -12);//@jgdu
        }   
		
        iNextModTime = aInTimeStamp/1000;
        iModTimeInitialized = OMX_TRUE;
        iLastPicTime = aInTimeStamp;
        iPreSrcDitherms = 0;
        iFlagAdjustSrcDither = OMX_FALSE;
        //
        iCycleAdaptFrmCnt = 0;
        iCycleAdaptInterval = 0;
        iSrcCycleAdaptms = ( iSrcFrameRate > 0)? ( (1000<<16)/iSrcFrameRate):50;  //default 20fps.

        iBs_remain_len =  (OMX_S32)(iVBVSize>>1);  // init vbv buffer half full to avoid larger frame at begin.
    }

    /* Input Buffer Size Check
     * Input buffer size should be equal to one frame, otherwise drop the frame
     * as it is a corrupt data and don't encode it */
     iVideoFormat = static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent)->Get_eColorFormat_of_input();
    if (OMX_COLOR_FormatYUV420SemiPlanar == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)((iSrcWidth * iSrcHeight * 3) >> 1))
        {
            *aOutputLength = 0;
            OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo aInBufSize<image size\n");
            return OMX_FALSE;
        }
    }else
    {
    	 if((OMX_COLOR_FormatAndroidOpaque == iVideoFormat)&&(static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent)->iStoreMetaDataBuffer))
    	 {
    	 }else
    	 {
        	*aOutputLength = 0;
   	 	OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo iVideoFormat err %d\n",iVideoFormat);
    	 	return OMX_FALSE;
    	 }
    }

    //Now encode the input buffer
    MMEncIn  vid_in;
    MMEncOut vid_out;
    int Size;
    bool status;

    OMX_U32 intervalms =  (OMX_U32)(iSrcCycleAdaptms);
    OMX_S32 bits_consumed  = 0;



#if 1
    // Auto adapt to get the frame cycle of source.
    if(iCycleAdaptFrmCnt < 10)
    {
        OMX_TICKS curInterval = aInTimeStamp - iLastPicTime;

        if( (curInterval < iCycleAdaptInterval + 3000) &&
            (curInterval + 3000 >  iCycleAdaptInterval)
          )
        {
            iCycleAdaptFrmCnt++;
            if(iCycleAdaptFrmCnt >= 10)
            {
                // Cycle adapt is ok. adjust the cycle.
                uint32 i_new_src_cycle_ms;
                i_new_src_cycle_ms  = (OMX_U32)((aInTimeStamp - iCycleAdaptFirstFrmTime)/iCycleAdaptFrmCnt/1000);
                if(i_new_src_cycle_ms > iSrcCycleAdaptms)
                {
                    iSrcCycleAdaptms = i_new_src_cycle_ms;
                }

                OMX_MP4ENC_INFO("Mpeg4Encoder_OMX::Mp4EncodeVideo, iSrcCycleAdaptms=%ld",iSrcCycleAdaptms );
            }
        }
        else
        {
            // start the new auto adaptor.
            iCycleAdaptFirstFrmTime = aInTimeStamp;
            iCycleAdaptInterval = curInterval;
            iCycleAdaptFrmCnt = 0;
        }
    }
#endif
    iLastPicTime = aInTimeStamp;


    // iNextModTime is in milliseconds (although it's a 64 bit value) whereas
    // aInTimestamp is in microseconds
    //if((iNextModTime * 1000) < aInTimeStamp + intervalms*1000)
    //if((iNextModTime * 1000) < aInTimeStamp + 15*1000)
    if( (iNextModTime * 1000) <= aInTimeStamp + (iPreSrcDitherms + 15)*1000 )
    {
        // estimate the bit consumed by APP.
        bits_consumed =  (OMX_S32)( intervalms*MAX(iBps,2000)/1000);
        // bits_consumed =  (OMX_S32)( (aInTimeStamp-iLastPicTime)*MAX(iBps,2000)/1000000); 
        iBs_remain_len -=  bits_consumed;
        if(iBs_remain_len<0)
        {
            iBs_remain_len = 0;
        }

#if PROFILING_ON
        OMX_U32 Start = OsclTickCount::TickCount();
#endif
	OMX_MP4ENC_INFO("aInBuffer %x, aInBuffer_phy %x,aInBuffer_size %d",aInBuffer,aInBuffer_phy,aInBufSize);
        if((static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent)->iStoreMetaDataBuffer))
        {
		GraphicBufferMapper &mapper = GraphicBufferMapper::get();        
		OMX_U32 type = *(OMX_U32 *) aInBuffer;
		if(type == kMetadataBufferTypeGrallocSource){
	            GraphicBufferMapper &mapper = GraphicBufferMapper::get();
		buffer_handle_t buf = *((buffer_handle_t *) (aInBuffer + 4));	
    		int width = ((iSrcWidth + 15) >> 4) << 4;
    		int height = ((iSrcHeight + 15) >> 4) << 4;
    		Rect bounds(width, height);
    		void *vaddr;
    		if(mapper.lock(buf, GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_NEVER, bounds, &vaddr))
    		{
            		OMX_MP4ENC_ERR("Mpeg4Encoder_OMX mapper.lock fail %x",buf);
            		return OMX_FALSE;			
    		}
		//todo format conversion for MetaDataBuffer
		OMX_MP4ENC_INFO("OMX_COLOR_FormatAndroidOpaque %x",vaddr);
		CopyToYUVIn((uint8 *)vaddr, iSrcWidth, iSrcHeight,((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
		if(mapper.unlock(buf))
		{
            		OMX_MP4ENC_ERR("Mpeg4Encoder_OMX mapper.unlock fail %x",buf);	
			return OMX_FALSE;			
		}
              iVideoIn = iYUVIn;
		iVideoIn_phy = iYUVIn_phy;	
        }else if(type == kMetadataBufferTypeCameraSource){	             
	            	iVideoIn = (uint8*)(*((int *) aInBuffer + 2));
			iVideoIn_phy = (uint8*)(*((int *) aInBuffer + 1));
			OMX_MP4ENC_ERR("wxz: Mp4EncodeVideo in: 0x%x, in phy: 0x%x.", (int)iVideoIn, (int)iVideoIn_phy);
	            }else{
	            OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo kMetadataBufferTypeGrallocSource err %d\n",type);
	                 return OMX_FALSE;
	            }
        }
        else
        {
        	if((iSrcWidth & 0xF) || (iSrcHeight & 0xF) ||(!(static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent))->iNumberOfPmemBuffers)) { /* iSrcWidth or iSrcHeight is not multiple of 16 or buffer allocated not by component*/
                	CopyToYUVIn(aInBuffer, iSrcWidth, iSrcHeight,((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
                	iVideoIn = iYUVIn;
			iVideoIn_phy = iYUVIn_phy;					
        	}else{		
              	iVideoIn = aInBuffer;
				iVideoIn_phy = (uint8*)(static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent))->FindPhyAddr((uint32)aInBuffer);				
        	    }
         }


#if PROFILING_ON
        //End ticks for color conversion time
        OMX_U32 Stop = OsclTickCount::TickCount();
        iProfileStats.iColorConversionTime += (Stop - Start);

        //Start ticks for encoding time
        ++iProfileStats.iTotalNumFrames;
        OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

        if(iEncSycFrame)
        {
            iFrameTypeCounter = 0;
            iEncSycFrame = OMX_FALSE;
        }
        if(iFrameTypeCounter%iIFrameInterval == 0)
        {
            iPredType = IVOP;
        }else
        {
            iPredType = PVOP;		
        }
        /* with backward-P or B-Vop this timestamp must be re-ordered */
        *aOutTimeStamp = aInTimeStamp;

	int vid_height =  ((iSrcHeight + 15) >> 4) << 4;
	int vid_pitch = ((iSrcWidth + 15) >> 4) << 4;
       vid_in.time_stamp =(int32) (aInTimeStamp / 1000);
	vid_in.vopType = iPredType;
	vid_in.bs_remain_len = iBs_remain_len;
	vid_in.channel_quality = 1;
	vid_in.p_src_y = iVideoIn;
	vid_in.p_src_u = iVideoIn + vid_height * vid_pitch;
	vid_in.p_src_v = vid_in.p_src_u + (vid_height * vid_pitch>>2);
	vid_in.p_src_y_phy = iVideoIn_phy;
	vid_in.p_src_u_phy = iVideoIn_phy + vid_height * vid_pitch;
	vid_in.p_src_v_phy = vid_in.p_src_u_phy + (vid_height * vid_pitch>>2);	

        OMX_U32 Start_encode = OsclTickCount::TickCount();
        MMEncRet ret =  MP4EncStrmEncode(&vid_in, &vid_out);
        OMX_U32 Stop_encode = OsclTickCount::TickCount();
        OMX_MP4ENC_INFO("MP4EncStrmEncode consumed %dms, return %d, size = %d\n", Stop_encode-Start_encode, ret, vid_out.strmSize);
        
#ifdef MPEG4_ES_DUMP
    if (fp_es != NULL)
    {
        fwrite(vid_out.pOutBuf, 1, vid_out.strmSize, fp_es);
    }
#endif
    
	Size =  vid_out.strmSize;
	if(ret == MMENC_OK)
		status = 	OMX_TRUE;
	else
		status = 	OMX_FALSE;

#if PROFILING_ON
        OMX_U32 EndTime = OsclTickCount::TickCount();
        iProfileStats.iTotalEncTime += (EndTime - StartTime);

        if ((PV_TRUE == status) && (Size > 0))
        {
            ++iProfileStats.iNumFramesEncoded;
            iProfileStats.iDuration = vid_out.timestamp;
        }
#endif

        if (status == OMX_TRUE)
        {
            if(iCycleAdaptFrmCnt >= 10)
            {
                // adapt success, use modetime.
                *aOutTimeStamp = iNextModTime*1000;
            }
            else
            {
                // adapt failed, use real source time.
                *aOutTimeStamp = aInTimeStamp;
            }

            // iPreSrcDitherms:  if pre src is later than the expect time, >0; else, <0.
            iPreSrcDitherms = (aInTimeStamp/1000 - iNextModTime);
            if( iPreSrcDitherms > (OMX_TICKS)(intervalms>>1) )
            {
                /* iNextModTime it's too large., setup the flag adjust the modetime.*/
                 iFlagAdjustSrcDither = OMX_TRUE;
            }
            else if( iPreSrcDitherms <= 1 )
            {
                iFlagAdjustSrcDither = OMX_FALSE;
            }

            if(iFlagAdjustSrcDither > 0)
            {
#if 1
                if( iPreSrcDitherms > (OMX_TICKS)(intervalms>>1) )
                {
                    // adjust the mode time quickly,  the timestamp of source should be checked.
                    // or some frames dropped.
                    //iPreSrcDitherms = 0;
                    //iNextModTime = (OMX_TICKS)((OMX_S32)( aInTimeStamp/1000)  + intervalms);
                    iNextModTime += (OMX_TICKS)( intervalms + (intervalms>>2) );
                }
                else
#endif
                {
                    // adjust the src dither slowly. For source dither or  slight frame drop.
                    iNextModTime += (OMX_TICKS)(intervalms) + 1; //2
                }
            }
            else
            {
                 // this is time in milliseconds
                iNextModTime += (OMX_TICKS)(intervalms);
            }

            iFrameTypeCounter++;
            if (Size > 0)
            {
            	if ((OMX_U32) Size > *aOutputLength) // overrun buffer is used by the encoder
            	{
            		OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo buffer overrun:%x,%d,%d\n",vid_out.pOutBuf,Size,*aOutputLength);    
                	*aOverBufferPointer = vid_out.pOutBuf;
                	*aBufferOverRun = OMX_TRUE;
            	}else
            	{
            		oscl_memcpy(aOutBuffer, vid_out.pOutBuf,Size);
            	}

            }
           // if (Size > 0)
            {
               // *aOutTimeStamp = ((OMX_TICKS) vid_out.timestamp * 1000);  //converting millisec to microsec
                if (iPredType == IVOP)
                {
                    //Its an I Frame, mark the sync flag as true
                    *aSyncFlag = OMX_TRUE;
                }
		*aOutputLength = Size;		
            }
            //SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo encode ok %d\n",Size);   

            /*  increase the level of vbv buffer pool.*/
            iBs_remain_len += (Size<<3);
            if(iBs_remain_len >= (OMX_S32)iVBVSize) 
            {
                iBs_remain_len = iVBVSize;
            }
            iLastFrameBitsEncoded = (Size<<3);

            return OMX_TRUE;
        }
        else
        {
            *aOutputLength = 0;
    	    OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo encode error\n");        
            return OMX_FALSE;
        }
    }
    else /* if(aInTimeStamp >= iNextModTime) */
    {
        *aOutputLength = 0;
	 OMX_MP4ENC_ERR("Mpeg4Encoder_OMX::Mp4EncodeVideo frame skip %lld ms,%lld us,%d ms\n",iNextModTime,aInTimeStamp,intervalms);
        return OMX_TRUE;
    }
}



OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncDeinit()
{
    OMX_MP4ENC_DEBUG("Mpeg4Encoder_OMX::Mp4EncDeinit\n");
    
#ifdef MPEG4_ES_DUMP
    fclose(fp_es);
#endif

    if (OMX_TRUE == iInitialized)
    {
        MP4EncRelease();
        iInitialized = OMX_FALSE;

        if (iYUVIn)
        {
            iYUVInPmemHeap.clear();
            iYUVIn = NULL;
        }

	if(iEncExtBuf)
	{
	    iEncExtPmemHeap.clear();
	    iEncExtBuf = NULL;
	}

	if(iVIDInterBuf)
	{
	    oscl_free(iVIDInterBuf);
	    iVIDInterBuf = NULL;
	}
    }
    return OMX_ErrorNone;
}


/* COLOUR CONVERSION ROUTINES ARE WRITTEN BELOW*/

/* ///////////////////////////////////////////////////////////////////////// */
/* Copy from YUV input to YUV frame inside M4VEnc lib                       */
/* When input is not YUV, the color conv will write it directly to iVideoInOut. */
/* ///////////////////////////////////////////////////////////////////////// */

void Mpeg4Encoder_OMX::CopyToYUVIn(uint8 *YUV, int width, int height, int width_16, int height_16)
{
    OMX_MP4ENC_DEBUG("Mpeg4Encoder_OMX::CopyToYUVIn\n");
    if((width==width_16)&&(height==height_16))
    {
	oscl_memcpy(iYUVIn, YUV, width_16 * height_16*3/2);
	return ;
    }

    uint8 *y_src = YUV;
    uint8 *y_dst = iYUVIn;	
    for(int i=0;i<height;i++)
    {
    	oscl_memcpy(y_dst, y_src, width);
	y_src += width;
	y_dst += width_16;
    }

    uint8 *uv_src = YUV +width*height ;
    uint8 *uv_dst = iYUVIn + width_16*height_16;	
    for(int i=0;i<height/2;i++)
    {
    	oscl_memcpy(uv_dst, uv_src, width);
	uv_src += width;
	uv_dst += width_16;
    }
    return ;
}
