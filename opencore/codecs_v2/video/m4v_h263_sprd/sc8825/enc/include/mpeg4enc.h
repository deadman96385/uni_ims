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

/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagMP4EncHandle
{

    void *videoEncoderData;     /* this is an internal pointer that is only used */
    /* in the encoder library.   */
#ifdef PV_MEMORY_POOL
    int32 size;
#endif


        void *userdata;




        int g_mpeg4_enc_err_flag;
} MP4EncHandle;



/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description:   Init mpeg4 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncInit(MP4EncHandle* mp4Handle,MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr, MMEncVideoInfo *pVideoFormat);

/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MP4EncHandle* mp4Handle,MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncSetConf(MP4EncHandle* mp4Handle,MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGetConf(MP4EncHandle* mp4Handle,MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncStrmEncode (MP4EncHandle* mp4Handle,MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncRelease(MP4EncHandle* mp4Handle);

/*****************************************************************************/
//  Description: check whether VSP can used for video encoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN MPEG4ENC_VSP_Available (MP4EncHandle* mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
