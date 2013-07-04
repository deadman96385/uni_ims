/******************************************************************************
 ** File Name:    mpeg4enc.h                                                   *
 ** Author:                                     		                      *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2007 Spreadtrum, Incorporated. All Rights Reserved.           *
 ** Description:  define data structures for Video Codec                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 3/15/2007     			      Create.                                     *
 *****************************************************************************/
#ifndef MPEG4ENC_H
#define MPEG4ENC_H

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

//#define MP4ENC_INTERNAL_BUFFER_SIZE (MP4ENC_OR_RUN_SIZE+MP4ENC_OR_INTER_MALLOC_SIZE)
//#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

typedef struct tagMP4Handle
{
    void            *videoEncoderData;
//    int             videoEncoderInit;
    void *userData;

} MP4Handle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description:   Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncInit(MP4Handle *mp4Handle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat);
/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MP4Handle *mp4Handle, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncSetConf(MP4Handle *mp4Handle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGetConf(MP4Handle *mp4Handle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncStrmEncode (MP4Handle *mp4Handle, MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Close mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncRelease(MP4Handle *mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
