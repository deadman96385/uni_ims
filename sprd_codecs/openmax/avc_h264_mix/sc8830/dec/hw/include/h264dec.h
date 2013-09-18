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

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader);
typedef int (*FunctionType_MallocCB)(void* aUserData, unsigned int width,unsigned int height, unsigned int aNumBuffers);

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
    uint32 numRefFrames;
    uint32 has_b_frames;
} H264SwDecInfo;

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagAVCHandle
{
    /* The following fucntion pointer is copied to BitstreamDecVideo structure  */
    /*    upon initialization and never used again. */
//    int (*readBitstreamData)(uint8 *buf, int nbytes_required, void *appData);
//    applicationData appData;

//    uint8 *outputFrame;
    void *videoDecoderData;     /* this is an internal pointer that is only used */
    /* in the decoder library.   */
#ifdef PV_MEMORY_POOL
    int32 size;
#endif

    void *userdata;

    FunctionType_BufCB VSP_bindCb;
    FunctionType_BufCB VSP_unbindCb;
    FunctionType_MallocCB VSP_extMemCb;

} AVCHandle;

#ifdef _VSP_LINUX_
//typedef int (*FunctionType_Bind_CB)(void *userData/*, int32 index*/, uint8 **yuv);
//typedef void (*FunctionType_UnBind_CB)(void *userData, int32_t index);
//void H264Dec_RegBufferCB(FunctionType_Bind_CB bindCb,FunctionType_UnBind_CB unbindCb,void *userdata);
void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle);
MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, uint8 **pOutput, int32 *picId);
void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId);
//void H264Dec_RegSPSCB(FunctionType_SPS spsCb,void *userdata);
//void H264Dec_RegFlushCacheCB( FunctionType_FlushCache fluchCacheCb);


#endif

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
void H264GetBufferDimensions(AVCHandle *avcHandle, int32 *aligned_width, int32 *aligned_height) ;
MMDecRet H264DecGetInfo(AVCHandle *avcHandle, /*H264SwDecInst decInst, */H264SwDecInfo *pDecInfo);


/*****************************************************************************/
//  Description: Init h264 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecMemInit(AVCHandle *avcHandle, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264_DecReleaseDispBfr(AVCHandle *avcHandle, uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecRelease(AVCHandle *avcHandle);

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence:
//  Author:
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp
/*****************************************************************************/
BOOLEAN H264DEC_VSP_Available (AVCHandle *avcHandle);

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence:
//  Author:
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal
/*****************************************************************************/
void H264Dec_GetOneDspFrm (AVCHandle *avcHandle, MMDecOutput * pOutput, int req_transposed, int is_last_frame);


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
