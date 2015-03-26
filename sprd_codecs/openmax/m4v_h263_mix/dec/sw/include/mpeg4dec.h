/******************************************************************************
 ** File Name:    mpeg4dec.h                                                  *
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
#ifndef _MPEG4_DEC_H_
#define _MPEG4_DEC_H_

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

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
typedef int (*FunctionType_MemAllocCB)(void *userData, unsigned int extra_mem_size);

typedef enum
{
    INTER_MEM = 0,  /*internal memory, only for software writing and reading and initialized when initialize decoder*/
    HW_NO_CACHABLE, /*physical continuous and no-cachable, only for VSP writing and reading */
    HW_CACHABLE,    /*physical continuous and cachable, for software writing and VSP reading */
    SW_CACHABLE,    /*only for software writing and reading*/
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

/*standard*/
typedef enum {
    ITU_H263 = 0,
    MPEG4,
    JPEG,
    FLV_V1,
    H264,
    RV8,
    RV9
} VIDEO_STANDARD_E;

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagMP4Handle
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
//    int nLayers;
    /* pointers to VOL data for frame-based decoding. */
//    uint8 *volbuf[2];           /* maximum of 2 layers for now */
//    int32 volbuf_size[2];

    void *userdata;

    FunctionType_BufCB VSP_bindCb;
    FunctionType_BufCB VSP_unbindCb;
    FunctionType_MemAllocCB VSP_extMemCb;

    int g_mpeg4_dec_err_flag;
} MP4Handle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle);
int MP4DecGetLastDspFrm(MP4Handle *mp4Handle, void **pOutput);
void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
void MP4DecSetReferenceYUV(MP4Handle *mp4Handle, uint8 *pFrameY);

void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height);
void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height);
MMDecRet MP4GetCodecCapability(MP4Handle *mp4Handle, int32 *max_width, int32 *max_height);
MMDecRet MP4DecSetParameter(MP4Handle *mp4Handle, MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * pBuffer);

MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: Close mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecRelease(MP4Handle *mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
