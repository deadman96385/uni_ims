/******************************************************************************
 ** File Name:    vp8dec.h                                                   *
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
#ifndef _VP8_DEC_H_
#define _VP8_DEC_H_

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

//#define VP8_DECODER_INTERNAL_BUFFER_SIZE (VP8DEC_OR_RUN_SIZE+VP8DEC_OR_INTER_MALLOC_SIZE)  //0x200000

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagVPXHandle
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
} VPXHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

void VP8DecSetCurRecPic(VPXHandle *vpxHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);

/*****************************************************************************/
//  Description: Init vpx decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet VP8DecDecode(VPXHandle *vpxHandle, MMDecInput *pInput, MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: Close vpx decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet VP8DecRelease(VPXHandle *vpxHandle);

void VP8DecReleaseRefBuffers(VPXHandle *vpxHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif //_VP8_DEC_H_
// End
