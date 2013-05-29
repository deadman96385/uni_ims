/******************************************************************************
 ** File Name:    h264dec.h                                                   *
 ** Author:                                     		                      *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2007 Spreadtrum, Incorporated. All Rights Reserved.         *
 ** Description:  define data structures for Video Codec                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 3/15/2007     			      Create.                                     *
 *****************************************************************************/
#ifndef _H264_DEC_H_
#define _H264_DEC_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mmcodec.h"
/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
    typedef struct
    {
        uint32 cropLeftOffset;
        uint32 cropOutWidth;
        uint32 cropTopOffset;
        uint32 cropOutHeight;
    } CropParams;


    typedef struct
    {
        uint32 profile;
        uint32 picWidth;
        uint32 picHeight;
        uint32 videoRange;
        uint32 matrixCoefficients;
        uint32 parWidth;
        uint32 parHeight;
        uint32 croppingFlag;
        CropParams cropParams;
    } H264SwDecInfo;


MMDecRet H264DecGetNALType(uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
void H264GetBufferDimensions(int32 *aligned_width, int32 *aligned_height) ;
MMDecRet H264DecGetInfo(/*H264SwDecInst decInst, */H264SwDecInfo *pDecInfo);


/*****************************************************************************/
//  Description: Init h264 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecInit(MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet H264DecMemInit(MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecDecode(MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264_DecReleaseDispBfr(uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecRelease(void);

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN H264DEC_VSP_Available (void);

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
void H264Dec_GetOneDspFrm (MMDecOutput * pOutput, int req_transposed, int is_last_frame);

#ifdef _VSP_LINUX_
typedef int (*FunctionType_Bind_CB)(void *userData/*, int32 index*/, uint8 **yuv);
typedef void (*FunctionType_UnBind_CB)(void *userData, int32_t index);
void H264Dec_RegBufferCB(FunctionType_Bind_CB bindCb,FunctionType_UnBind_CB unbindCb,void *userdata);
void H264Dec_ReleaseRefBuffers();
MMDecRet H264Dec_GetLastDspFrm(uint8 **pOutput, int32 *picId);
void H264Dec_SetCurRecPic(uint8	*pFrameY, int32 picId);
//typedef int (*FunctionType_SPS)(void* aUserData, uint width,uint height, uint aNumBuffers, uint profile);
typedef int (*FunctionType_SPS)(void* aUserData, unsigned int width,unsigned int height, unsigned int aNumBuffers);
void H264Dec_RegSPSCB(FunctionType_SPS spsCb,void *userdata);
MMDecRet H264DecMemCacheInit(MMCodecBuffer * pBuffer);

#endif

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif //_H264_DEC_H_
// End
