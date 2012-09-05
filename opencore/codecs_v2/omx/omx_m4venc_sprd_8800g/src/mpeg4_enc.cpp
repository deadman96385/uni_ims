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
#if !defined(CHIP_8810)	
#include "vsp_drv_sc8800g.h"
#else
#include "vsp_drv_sc8810.h"
#endif
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

#if 0
    /* Scalable profile */    /*8*/
    SIMPLE_SCALABLE_PROFILE_LEVEL0 = MAX_BASE_PROFILE + 1,
    SIMPLE_SCALABLE_PROFILE_LEVEL1,
    SIMPLE_SCALABLE_PROFILE_LEVEL2,
    CORE_SCALABLE_PROFILE_LEVEL1,
    CORE_SCALABLE_PROFILE_LEVEL2,
    CORE_SCALABLE_PROFILE_LEVEL3,
    MAX_SCALABLE_PROFILE = CORE_SCALABLE_PROFILE_LEVEL3
#endif

} ProfileLevelIdType;

const uint8 DEFAULT_VOL_HEADER[DEFAULT_VOL_HEADER_LENGTH] =
{
    0x00, 0x00, 0x01, 0xB0, 0x08, 0x00, 0x00, 0x01,
    0xB5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x20, 0x00, 0x84, 0x40, 0xFA, 0x28, 0x2C,
    0x20, 0x90, 0xA2, 0x1F
};

#define CLIP(x) ((x)<0) ? 0:(((x)>255)? 255:(x))
#define EXTRACT_RGB565(r, g, b, data) { r = (unsigned char)(((data) & 0x1F00) >> 8); b =\
             (unsigned char)((((data) & 0x7) << 3) | (((data) & 0xE000) >> 13)); g =\
             (unsigned char)(((data) & 0xF8) >> 3);}

#define EXTRACT_RGB565_(b, g, r, data) \
    r = ((data) & 31); \
    g = ((data >> 5) & 63); \
    b = ((data >> 11) & 31 );

#define EXTRACT_RGBA8888(r, g, b, data){ b = (unsigned char)(((data) & 0xff0000) >> 16); \
									      g = (unsigned char)(((data) & 0xff00) >> 8); \
									      r = (unsigned char)(((data) & 0xff));}
#define RGBA_32_SIZE 4
#define GENY16(r, g, b) CLIP(  ( ( (80593 * r)+(77855 * g)+(30728 * b)) >> 15))
#define GENU16(r, g, b) CLIP(128+ ( ( -(45483 * r)-(43936 * g)+(134771 * b)) >> 15 ))
#define GENV16(r, g, b) CLIP(128+ ( ( (134771 * r)-(55532 * g)-(21917 * b)) >> 15  ))

void RGBA8888toYUV420(unsigned char *pIn, unsigned char *pOut, int height, int width)
{
	int   col, row;
    	unsigned char     *pu8_yn, *pu8_ys, *pu8_uv;
    	unsigned char     *pu8_y_data, *pu8_uv_data;
    	unsigned char     *pu8_rgbn_data, *pu8_rgbn;

       unsigned int   u32_pix1, u32_pix2, u32_pix3, u32_pix4;

   	int    i32_r00, i32_r01, i32_r10, i32_r11;
   	int    i32_g00, i32_g01, i32_g10, i32_g11;
    	int    i32_b00, i32_b01, i32_b10, i32_b11;

       int    i32_y00, i32_y01, i32_y10, i32_y11;
       int    i32_u00, i32_u01, i32_u10, i32_u11;
       int    i32_v00, i32_v01, i32_v10, i32_v11;
	
    	pu8_rgbn_data   = pIn;
	
    	pu8_y_data = pOut;
    	pu8_uv_data = pOut + height*width ;
	
	for(row = height; row != 0; row-=2 ){
		/* Current Y plane row pointers */
        	pu8_yn = pu8_y_data;
        	/* Next Y plane row pointers */
        	pu8_ys = pu8_yn + width;
        	/* Current U plane row pointer */
        	pu8_uv = pu8_uv_data;

        	pu8_rgbn = pu8_rgbn_data;
		
		for(col = width; col != 0; col-=2){
		 		/* Get four RGB 565 samples from input data */
            			u32_pix1 = *( (unsigned int *) pu8_rgbn);
            			u32_pix2 = *( (unsigned int *) (pu8_rgbn + RGBA_32_SIZE));
            			u32_pix3 = *( (unsigned int *) (pu8_rgbn + width*RGBA_32_SIZE));
            			u32_pix4 = *( (unsigned int *) (pu8_rgbn + width*RGBA_32_SIZE + RGBA_32_SIZE));
            			/* Unpack RGB565 to 8bit R, G, B */
            			/* (x,y) */
            			EXTRACT_RGBA8888(i32_r00,i32_g00,i32_b00,u32_pix1);
            			/* (x+1,y) */
            			EXTRACT_RGBA8888(i32_r10,i32_g10,i32_b10,u32_pix2);
            			/* (x,y+1) */
            			EXTRACT_RGBA8888(i32_r01,i32_g01,i32_b01,u32_pix3);
            			/* (x+1,y+1) */
            			EXTRACT_RGBA8888(i32_r11,i32_g11,i32_b11,u32_pix4);

				/* Convert RGB value to YUV */
            			i32_u00 = GENU16(i32_r00, i32_g00, i32_b00);
            			i32_v00 = GENV16(i32_r00, i32_g00, i32_b00);
            			/* luminance value */
            			i32_y00 = GENY16(i32_r00, i32_g00, i32_b00);

            			i32_u10 = GENU16(i32_r10, i32_g10, i32_b10);
            			i32_v10 = GENV16(i32_r10, i32_g10, i32_b10);
            			/* luminance value */
            			i32_y10 = GENY16(i32_r10, i32_g10, i32_b10);

            			i32_u01 = GENU16(i32_r01, i32_g01, i32_b01);
            			i32_v01 = GENV16(i32_r01, i32_g01, i32_b01);
            			/* luminance value */
            			i32_y01 = GENY16(i32_r01, i32_g01, i32_b01);

            			i32_u11 = GENU16(i32_r11, i32_g11, i32_b11);
            			i32_v11 = GENV16(i32_r11, i32_g11, i32_b11);
            			/* luminance value */
            			i32_y11 = GENY16(i32_r11, i32_g11, i32_b11);

            			/* Store luminance data */
            			pu8_yn[0] = (unsigned char)i32_y00;
            			pu8_yn[1] = (unsigned char)i32_y10;
            			pu8_ys[0] = (unsigned char)i32_y01;
            			pu8_ys[1] = (unsigned char)i32_y11;

            			/* Store chroma data */
            			pu8_uv[0] = (unsigned char)((i32_u00 + i32_u01 + i32_u10 + i32_u11 + 2) >> 2);
            			pu8_uv[1] = (unsigned char)((i32_v00 + i32_v01 + i32_v10 + i32_v11 + 2) >> 2);
						
           			 /* Prepare for next column */
            			pu8_rgbn += 2*RGBA_32_SIZE;
            			 /* Update current Y plane line pointer*/
            			pu8_yn += 2;
            			/* Update next Y plane line pointer*/
            			pu8_ys += 2;
            			/* Update U plane line pointer*/
            			pu8_uv +=2;										
			}
		/* Prepare pointers for the next row */
		pu8_y_data += width*2;
		pu8_uv_data += width;
		pu8_rgbn_data += width*2*RGBA_32_SIZE;
	}
		
}


Mpeg4Encoder_OMX::Mpeg4Encoder_OMX(class OmxComponentBase *pComp)
{
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mpeg4Encoder_OMX\n");

    ipOMXComponent = pComp;
	
    iModTimeInitialized = OMX_FALSE;
	
    iInitialized = OMX_FALSE;
    iYUVIn = NULL;
    iVIDInterBuf = NULL;
    iEncExtBuf = NULL;
    iVsp_fd	 = -1;
    iLogCount	= 0;
	
    // Create a default VOL header
    oscl_memset(iVolHeader, 0, DEFAULT_VOL_HEADER_LENGTH);
    oscl_memcpy(iVolHeader, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH);
    iVolHeaderSize = DEFAULT_VOL_HEADER_LENGTH;

#if PROFILING_ON
    oscl_memset(&iProfileStats, 0, sizeof(PVEncNodeStats));
#endif

}

static int VSP_reset_cb(int fd)
{
	//SCI_TRACE_LOW("VSP_reset_cb\n");
	ioctl(fd,VSP_RESET,NULL);
	return 0;
}


static int  vsp_acqure(int iVsp_fd)
{
   // SCI_TRACE_LOW("vsp_acqure\n");
    int ret;
    ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
    if(ret){
		SCI_TRACE_LOW("mp4enc VSP hardware timeout try again %d\n",ret);	
		ret =  ioctl(iVsp_fd,VSP_ACQUAIRE,NULL);
		if(ret){
   			 SCI_TRACE_LOW("mp4enc VSP hardware timeout give up %d\n",ret);
			 return 1;
		}		 
    }	
    return 0;	
}

static OMX_ERRORTYPE  Mp4EncGetProfileLevelId(
        OMX_U32 mode,
        OMX_U32 omxProfile,
        OMX_U32 omxLevel,
        OMX_U8* pProfileLevelId) 
{
    uint8 profileLevelId = 0;

    SCI_TRACE_LOW("Mp4EncGetProfileLevelId: %d/%d/%d", mode, omxProfile, omxLevel);
    
    if (mode == H263_MODE) {

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

    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit\n");
    iEncodeMode = iEncMode;
    switch (iEncMode)
    {
        case MODE_H263:
        {
            if (aH263Type.nGOBHeaderInterval > 0)
            {
                ENC_Mode = H263_MODE_WITH_ERR_RES;
            }
            else
            {
                ENC_Mode = H263_MODE;
            }
            profile = (OMX_U32)aH263Type.eProfile;
            level = (OMX_U32)aH263Type.eLevel;
        }
        break;

        case MODE_MPEG4:
        {
            if (OMX_TRUE == aEncodeMpeg4Param.bSVH)
            {
                if (aH263Type.nGOBHeaderInterval > 0)
                {
                    ENC_Mode = SHORT_HEADER_WITH_ERR_RES;
                }
                else
                {
                    ENC_Mode = SHORT_HEADER;
                }
            }
            else
            {
                if (OMX_TRUE == aErrorCorrection.bEnableDataPartitioning)
                {
                    ENC_Mode = DATA_PARTITIONING_MODE;
                }
                else if (OMX_TRUE == aErrorCorrection.bEnableResync)
                {
                    ENC_Mode = COMBINE_MODE_WITH_ERR_RES;
                }
                else
                {
                    ENC_Mode = COMBINE_MODE_NO_ERR_RES;
                }
            }
            profile = (OMX_U32)aEncodeMpeg4Param.eProfile;
            level = (OMX_U32)aEncodeMpeg4Param.eLevel;
        }
        break;

        default:
        {
	    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit error iEncMode %d \n",iEncMode);
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
         SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit, profile_level error, 0x%x,%x,%x \n",
            iEncodeMode,profile,level);
        return OMX_ErrorUnsupportedSetting;
    }
    encConfig.profileAndLevel = profileLevelId;

    iSrcWidth = aInputParam.nFrameWidth;
    iSrcHeight = aInputParam.nFrameHeight;
    iSrcFrameRate = aInputParam.xFramerate;
    iFrameOrientation = aInputOrientationType.nRotation;

    encInfo.frame_height = ((iSrcHeight + 15) >> 4) << 4;
    encInfo.frame_width = ((iSrcWidth+ 15) >> 4) << 4; 
	
    if ((OMX_COLOR_FormatYUV420SemiPlanar == aInputParam.eColorFormat)||(OMX_COLOR_FormatAndroidOpaque == aInputParam.eColorFormat))
    {
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eColorFormat %d \n",aInputParam.eColorFormat);
        iVideoFormat = aInputParam.eColorFormat;
	encInfo.uv_interleaved = 1;
    }
    else
    {
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eColorFormat error %d \n",aInputParam.eColorFormat);
        return OMX_ErrorUnsupportedSetting;
    }

    //Verify the input compression format
    if (OMX_VIDEO_CodingUnused != aInputParam.eCompressionFormat)
    {
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aInputParam.eCompressionFormat error\n");
        //Input port must have no compression supported
        return OMX_ErrorUnsupportedSetting;
    }


    if (OMX_TRUE == iInitialized)
    {
        /* clean up before re-initialized */
	Mp4EncDeinit();
    }

    // allocate iYUVIn
    if (1)//((iSrcWidth & 0xF) || (iSrcHeight & 0xF)) ) /* Not multiple of 16 */
    {
	 iYUVInPmemHeap = new MemoryHeapIon(SPRD_ION_DEV,(((((iSrcWidth + 15) >> 4) * ((iSrcHeight + 15) >> 4)) * 3) << 7),MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
 	 int fd = iYUVInPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
		
		iYUVIn_phy =(uint8 *)phy_addr;
		SCI_TRACE_LOW("Mpeg4Encoder_OMX yuvin mem pmempool %x,%x,%x,%x\n",iYUVInPmemHeap->getHeapID(),iYUVInPmemHeap->base(),phy_addr,buffer_size);
	 	iYUVIn =(uint8 *) iYUVInPmemHeap->base();
    	}else
    	{
    		SCI_TRACE_LOW("Mpeg4Encoder_OMX yuvin mem pmempool  error\n");
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
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aRateControlType.eControlRate error\n");
            return OMX_ErrorUnsupportedSetting;
    }

     if(encInfo.frame_width>176)
	 	encConfig.vbv_buf_size = encConfig.targetBitRate;
	 	
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
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aEncodeMpeg4Param.nBFrames error\n");
        return OMX_ErrorUnsupportedSetting;
    }

    //Encoder support only I and P frames picture type
    if (0 == (aEncodeMpeg4Param.nAllowedPictureTypes &
              (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP)))
    {
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit aEncodeMpeg4Param.nAllowedPictureTypes error\n");    
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

   SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit:iSrcFrameRate %d,iTimeIncRes %d,iTickPerSrc %d\n",iSrcFrameRate,iTimeIncRes,iTickPerSrc);


    //Checking the range of parameters that the encoder can't support here
    // and return OMX_ErrorUnsupportedSetting

    /***** Initlaize the encoder *****/


    if((iVsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
    {
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit open vsp error\n");   
	return OMX_ErrorInsufficientResources;
    }else
    {
        iVsp_addr = mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,iVsp_fd,0);
	SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit  vsp addr %x\n",iVsp_addr);
    }
	
    VSP_SetVirtualBaseAddr((uint32)iVsp_addr);
    VSP_reg_reset_callback(VSP_reset_cb,iVsp_fd);
	
    uint32 vidInterBufSize = (encInfo.frame_width/16)*7*4*2+14*4+6*4*3+68   +200 +4;
    iVIDInterBuf = (uint8*) oscl_malloc(vidInterBufSize);
    if (NULL == iVIDInterBuf)
    {
       	     SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit iVIDInterBuf alloc error\n");       
            return OMX_ErrorInsufficientResources;
    }
    uint32 vidExtBufSize = encInfo.frame_width* encInfo.frame_height*3/2*2  + ONEFRAME_BITSTREAM_BFR_SIZE + 300;
    {
	 iEncExtPmemHeap = new MemoryHeapIon(SPRD_ION_DEV,vidExtBufSize,MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
 	 int fd = iEncExtPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iEncExtPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);
		
		iEncExtBuf_phy =(uint8 *)phy_addr;
		SCI_TRACE_LOW("Mpeg4Encoder_OMX ext mem pmempool %x,%x,%x,%x\n",iEncExtPmemHeap->getHeapID(),iEncExtPmemHeap->base(),phy_addr,buffer_size);
	 	iEncExtBuf =(uint8 *) iEncExtPmemHeap->base();
    	}else
    	{
    		SCI_TRACE_LOW("Mpeg4Encoder_OMX ext  mem pmempool  error\n");
		return OMX_ErrorInsufficientResources;	
    	}
    }
    MMCodecBuffer 	InterMemBfr,ExtaMemBfr;
	
    InterMemBfr.common_buffer_ptr = (uint8 *)(((int) iVIDInterBuf+3)&(~0x3));
    InterMemBfr.size = vidInterBufSize-4;
    ExtaMemBfr.common_buffer_ptr = iEncExtBuf;
    ExtaMemBfr.common_buffer_ptr_phy = iEncExtBuf_phy;
    ExtaMemBfr.size	= vidExtBufSize;
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit: h263 %d,width %d,height %d,tiemscale %d,iIFrameInterval %d\n",encInfo.is_h263,encInfo.frame_width,encInfo.frame_height,encInfo.time_scale,iIFrameInterval);  	
    
    int vsp_stat = vsp_acqure(iVsp_fd);
    if(vsp_stat)
    {
	return OMX_ErrorInsufficientResources;
     }
    ioctl(iVsp_fd,VSP_ENABLE,NULL);	
    ioctl(iVsp_fd,VSP_RESET,NULL);		
		
    MMEncRet ret =  MP4EncInit(&InterMemBfr, &ExtaMemBfr,&encInfo);
    int status;
    if(ret == MMENC_OK)
        status = 	PV_TRUE;
    else
	status = 	PV_FALSE;		
    if (PV_FALSE == status)
    {
        ioctl(iVsp_fd,VSP_DISABLE,NULL);
        ioctl(iVsp_fd,VSP_RELEASE,NULL);
        iInitialized = OMX_FALSE;
        SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit MP4EncInit error\n");  		
        return OMX_ErrorBadParameter;
    }
    //encConfig.RateCtrlEnable = 0;
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit: RateCtrlEnable %d,targetBitRate %d,FrameRate %d,QP_IVOP %d,QP_PVOP %d,profileAndLevel %d\n",encConfig.RateCtrlEnable,encConfig.targetBitRate,encConfig.FrameRate,encConfig.QP_IVOP,encConfig.QP_PVOP,encConfig.profileAndLevel);  		
    MP4EncSetConf(&encConfig);
	
    iInitialized = OMX_TRUE;
    iNextModTime = 0;
	
    iEncSycFrame = OMX_TRUE;
    iFrameTypeCounter = 0;

    //Update the vol header for non-h263 modes
    if ((DATA_PARTITIONING_MODE == ENC_Mode) ||
            (COMBINE_MODE_WITH_ERR_RES == ENC_Mode) ||
            (COMBINE_MODE_NO_ERR_RES == ENC_Mode) ||
            (SHORT_HEADER == ENC_Mode) ||
            (SHORT_HEADER_WITH_ERR_RES == ENC_Mode))
    {
        // M4V output, get VOL header
        iVolHeaderSize = 32; // Encoder requires that buffer size is greater than vol header size (28)

	MMEncOut encOut;

	MP4EncGenHeader(&encOut);
	
        iVolHeaderSize = encOut.strmSize;
        oscl_memcpy(iVolHeader, (OsclAny*)encOut.pOutBuf, iVolHeaderSize);
    }
	
    ioctl(iVsp_fd,VSP_DISABLE,NULL);
    ioctl(iVsp_fd,VSP_RELEASE,NULL);

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
	
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncInit ok\n");  
    return OMX_ErrorNone;

}

/* Request I frame routine */
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4RequestIFrame()
{
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4RequestIFrame\n");
    iEncSycFrame = OMX_TRUE;
    return OMX_ErrorNone;
}

/* Request Update BitRate routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateBitRate(OMX_U32 aEncodedBitRate)
{
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4UpdateBitRate %d\n",aEncodedBitRate);
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
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4UpdateFrameRate %d\n",aEncodeFramerate);
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
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4UpdateIFrameInterval %d\n",aIntraPeriod);
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
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4OutBufferSize %d\n",*aMaxVideoFrameSize);
    *aMaxVideoFrameSize = ONEFRAME_BITSTREAM_BFR_SIZE;
    return OMX_ErrorNone;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4GetVolHeader(OMX_U8* aOutBuffer, OMX_U32* aOutputLength)
{
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4GetVolHeader\n");
    //Send the VOL Header only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
        //Copy the vol header in the output buffer in case of m4v format
        if ((DATA_PARTITIONING_MODE == ENC_Mode) ||
                (COMBINE_MODE_WITH_ERR_RES == ENC_Mode) ||
                (COMBINE_MODE_NO_ERR_RES == ENC_Mode) ||
                (SHORT_HEADER == ENC_Mode) ||
                (SHORT_HEADER_WITH_ERR_RES == ENC_Mode))
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

   // SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo  %x,%x,%d,%d\n",aInBuffer,aInBuffer_phy,aInBufSize,(int32) (aInTimeStamp / 1000));	
	
    if (OMX_FALSE == iModTimeInitialized)
    {
        if( -12 != getpriority(PRIO_PROCESS, 0)){
	  	setpriority(PRIO_PROCESS, 0, -12);//@jgdu
        }   
		
        iNextModTime = aInTimeStamp/1000;
        iModTimeInitialized = OMX_TRUE;
	 iBs_remain_len = 0;
	 iLastPicBytesEncoded = 0;
	 iLastPicTime = aInTimeStamp;
        iPreSrcDitherms = 0;
    }

    iBs_remain_len += iLastPicBytesEncoded*8 - (aInTimeStamp-iLastPicTime)*MAX((iBps-2000),2000)/1000000;
    if(iLogCount%4==0){	
    	SCI_TRACE_LOW("Mp4Enc [%d,%lld,%d][%lld,%lld] %d",iLastPicBytesEncoded*8,(aInTimeStamp-iLastPicTime)*MAX((iBps-2000),2000)/1000000,iBs_remain_len,aInTimeStamp,(aInTimeStamp-iLastPicTime),iFrameTypeCounter);
    }
    iLogCount++;
	
    if(iBs_remain_len>=(OMX_S32)iVBVSize) 
		iBs_remain_len = iVBVSize;
    if(iBs_remain_len<0)
		iBs_remain_len = 0;
/*	
    if((((iSrcWidth + 15) >> 4) << 4)>176)
		iBs_remain_len =  iVBVSize/5;
*/	

    /* Input Buffer Size Check
     * Input buffer size should be equal to one frame, otherwise drop the frame
     * as it is a corrupt data and don't encode it */
    if (OMX_COLOR_FormatYUV420SemiPlanar == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)((iSrcWidth * iSrcHeight * 3) >> 1))
        {
            *aOutputLength = 0;
            SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo aInBufSize<image size\n");
            return OMX_FALSE;
        }
    }else
    {
    	 if((OMX_COLOR_FormatAndroidOpaque == iVideoFormat)&&(static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent)->iStoreMetaDataBuffer))
    	 {
    	 	//SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo OMX_COLOR_FormatAndroidOpaque\n");
    	 }else
    	 {
        	*aOutputLength = 0;
   	 	SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo iVideoFormat err %d\n",iVideoFormat);
    	 	return OMX_FALSE;
    	 }
    }

    //Now encode the input buffer
    MMEncIn  vid_in;
    MMEncOut vid_out;
    Int Size;
    Bool status;

    // iNextModTime is in milliseconds (although it's a 64 bit value) whereas
    // aInTimestamp is in microseconds
 
    OMX_U32 intervalms = (iTickPerSrc*1000/iTimeIncRes);	

#if 0
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo, iNextModTime=%lld,aInTimeStamp=%lld,iPreSrcDitherms=%lld, interval=%d. \n",
        iNextModTime,aInTimeStamp/1000,iPreSrcDitherms, intervalms);
#endif

    //if((iNextModTime * 1000) < aInTimeStamp + intervalms*1000)
    //if((iNextModTime * 1000) < aInTimeStamp + 15*1000)
    if( (iNextModTime * 1000) <= aInTimeStamp + iPreSrcDitherms*1000 )
    {

#if PROFILING_ON
        OMX_U32 Start = OsclTickCount::TickCount();
#endif

        if((static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent)->iStoreMetaDataBuffer))
        {
		GraphicBufferMapper &mapper = GraphicBufferMapper::get();        
		OMX_U32 type = *(OMX_U32 *) aInBuffer;
		if(type != kMetadataBufferTypeGrallocSource)
		{
			SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo kMetadataBufferTypeGrallocSource err %d\n",type);
    	 		return OMX_FALSE;			
		}
		buffer_handle_t buf = *((buffer_handle_t *) (aInBuffer + 4));	
    		int width = ((iSrcWidth + 15) >> 4) << 4;
    		int height = ((iSrcHeight + 15) >> 4) << 4;
    		Rect bounds(width, height);
    		void *vaddr;
    		if(mapper.lock(buf, GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_NEVER, bounds, &vaddr))
    		{
            		SCI_TRACE_LOW("Mpeg4Encoder_OMX mapper.lock fail %x",buf);
            		return OMX_FALSE;			
    		}
		//todo format conversion for MetaDataBuffer
		SCI_TRACE_LOW("OMX_COLOR_FormatAndroidOpaque %x",vaddr);
		CopyToYUVIn((uint8 *)vaddr, iSrcWidth, iSrcHeight,((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
		if(mapper.unlock(buf))
		{
            		SCI_TRACE_LOW("Mpeg4Encoder_OMX mapper.unlock fail %x",buf);	
			return OMX_FALSE;			
		}
              iVideoIn = iYUVIn;
		iVideoIn_phy = iYUVIn_phy;		
        }
        else
        {
        	if((iSrcWidth & 0xF) || (iSrcHeight & 0xF)){ /* iSrcWidth or iSrcHeight is not multiple of 16 */
                	CopyToYUVIn(aInBuffer, iSrcWidth, iSrcHeight,((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
                	iVideoIn = iYUVIn;
			iVideoIn_phy = iYUVIn_phy;					
        	}else{		
              	iVideoIn = aInBuffer;
			if((static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent))->iNumberOfPmemBuffers){//buffer allocated by component
				iVideoIn_phy = (uint8*)(static_cast<OmxComponentMpeg4EncAO * > (ipOMXComponent))->FindPhyAddr((uint32)aInBuffer);				
			}else{
				//LOGI("aInBuffer_phy %x",aInBuffer_phy);
				iVideoIn_phy = aInBuffer_phy;
			}
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
	        //SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo IVOP %d,%d,%d\n",iFrameTypeCounter,iIFrameInterval,(int32) (aInTimeStamp / 1000));
	}else
	{
		iPredType = PVOP;		
	        //SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo PVOP %d,%d,%d\n",iFrameTypeCounter,iIFrameInterval,(int32) (aInTimeStamp / 1000));		
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

	int vsp_stat = vsp_acqure(iVsp_fd);
	if(vsp_stat)
	{
	    *aOutputLength = 0;
            return OMX_FALSE;
	}
        ioctl(iVsp_fd,VSP_ENABLE,NULL);
        ioctl(iVsp_fd,VSP_RESET,NULL);		
        MMEncRet ret =  MP4EncStrmEncode(&vid_in, &vid_out);
        ioctl(iVsp_fd,VSP_DISABLE,NULL);
        ioctl(iVsp_fd,VSP_RELEASE,NULL);
	
	Size =  vid_out.strmSize;
	if(ret == MMENC_OK)
		status = 	PV_TRUE;
	else
		status = 	PV_FALSE;

#if PROFILING_ON
        OMX_U32 EndTime = OsclTickCount::TickCount();
        iProfileStats.iTotalEncTime += (EndTime - StartTime);

        if ((PV_TRUE == status) && (Size > 0))
        {
            ++iProfileStats.iNumFramesEncoded;
            iProfileStats.iDuration = vid_out.timestamp;
        }
#endif

        if (status == PV_TRUE)
        {
            // iPreSrcDitherms:  if pre src is later than the expect time, >0; else, <0.
            iPreSrcDitherms = (aInTimeStamp/1000 - iNextModTime);
            if( ( iPreSrcDitherms > (OMX_TICKS)(intervalms<<1) ) || 
                ( iPreSrcDitherms + (OMX_TICKS)(intervalms<<1) < 0) )
            {
                /* reset the dither if it's too large. */
                SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo,iPreSrcDitherms=%lld, interval=%d. \n",
                                                iPreSrcDitherms, intervalms);
                iPreSrcDitherms = 0;
                iNextModTime = (OMX_TICKS)((OMX_S32)( aInTimeStamp/1000)  + intervalms);
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
            		SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo buffer overrun:%x,%d,%d\n",vid_out.pOutBuf,Size,*aOutputLength);    
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
	     iLastPicTime = aInTimeStamp;
	     iLastPicBytesEncoded = *aOutputLength;	 	
            return OMX_TRUE;
        }
        else
        {
            *aOutputLength = 0;
//	     iLastPicTime = aInTimeStamp;
	     iLastPicBytesEncoded = *aOutputLength;				
    	    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo encode error\n");        
            return OMX_FALSE;
        }
    }
    else /* if(aInTimeStamp >= iNextModTime) */
    {
        *aOutputLength = 0;
//	 iLastPicTime = aInTimeStamp;
	 iLastPicBytesEncoded = *aOutputLength;			
	 SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncodeVideo frame skip %lld ms,%lld us,%d ms\n",iNextModTime,aInTimeStamp,intervalms);
        return OMX_TRUE;
    }
}



OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncDeinit()
{
    SCI_TRACE_LOW("Mpeg4Encoder_OMX::Mp4EncDeinit\n");
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

	 if(iVsp_fd>=0)
	 {
	     munmap(iVsp_addr,SPRD_VSP_MAP_SIZE); 
	     close(iVsp_fd);
	     iVsp_fd = -1;
	 }

    }
    return OMX_ErrorNone;
}


/* COLOUR CONVERSION ROUTINES ARE WRITTEN BELOW*/

/* ///////////////////////////////////////////////////////////////////////// */
/* Copy from YUV input to YUV frame inside M4VEnc lib                       */
/* When input is not YUV, the color conv will write it directly to iVideoInOut. */
/* ///////////////////////////////////////////////////////////////////////// */

void Mpeg4Encoder_OMX::CopyToYUVIn(uint8 *YUV, Int width, Int height, Int width_16, Int height_16)
{
   // SCI_TRACE_LOW("Mpeg4Encoder_OMX::CopyToYUVIn\n");
    if((width==width_16)&&(height==height_16))
    {
	oscl_memcpy(iYUVIn, YUV, width_16 * height_16*3/2);
	return ;
    }

    UChar *y_src = YUV;
    UChar *y_dst = iYUVIn;	
    for(int i=0;i<height;i++)
    {
    	oscl_memcpy(y_dst, y_src, width);
	y_src += width;
	y_dst += width_16;
    }

    UChar *uv_src = YUV +width*height ;
    UChar *uv_dst = iYUVIn + width_16*height_16;	
    for(int i=0;i<height/2;i++)
    {
    	oscl_memcpy(uv_dst, uv_src, width);
	uv_src += width;
	uv_dst += width_16;
    }
    return ;
}
