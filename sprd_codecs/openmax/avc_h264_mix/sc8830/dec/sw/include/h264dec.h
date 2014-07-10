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
typedef int (*FunctionType_MallocCB)(void* aUserData, unsigned int size_extra);

typedef enum
{
    INTER_MEM = 0,  /*internal memory, only for software writing and reading and initialized when initialize decoder*/
    HW_NO_CACHABLE, /*physical continuous and no-cachable, only for VSP writing and reading */
    HW_CACHABLE,    /*physical continuous and cachable, for software writing and VSP reading */
    SW_CACHABLE,    /*only for software writing and reading*/
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle);
MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId);
void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY, uint8 *pFrameY_phy,void *pBufferHeader, int32 picId);
MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo);
MMDecRet H264GetCodecCapability(AVCHandle *avcHandle, int32 *max_width, int32 *max_height);
MMDecRet H264DecSetParameter(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat);

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
//  Description: Close mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet H264DecRelease(AVCHandle *avcHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif //_H264_DEC_H_
// End
