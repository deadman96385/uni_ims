/******************************************************************************
 ** File Name:    h264enc.h                                                   *
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
#ifndef H264ENC_H
#define H264ENC_H

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
//#define H264ENC_INTERNAL_BUFFER_SIZE (H264ENC_OR_RUN_SIZE+H264ENC_OR_INTER_MALLOC_SIZE)
//#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

/**
This structure has to be allocated and maintained by the user of the library.
This structure is used as a handle to the library object.
*/
typedef struct tagAVCHandle
{
    /** A pointer to the internal data structure. Users have to make sure that this value
        is NULL at the beginning.
    */
    void        *videoEncoderData;

    /** A pointer to user object which has the following member functions used for
    callback purpose.  !!! */
    void        *userData;

    /** Flag to enable debugging */
    uint32  debugEnable;
} AVCHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description:   Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncInit(AVCHandle *avcHandle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat);

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncSetConf(AVCHandle *avcHandle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncGetConf(AVCHandle *avcHandle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncStrmEncode (AVCHandle *avcHandle, MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   generate sps or pps header
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncGenHeader(AVCHandle *avcHandle, MMEncOut *pOutput, int is_sps);

/*****************************************************************************/
//  Description:   Close mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncRelease(AVCHandle *avcHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
