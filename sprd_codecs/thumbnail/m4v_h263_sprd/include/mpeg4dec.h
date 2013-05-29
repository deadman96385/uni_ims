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
/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagvideoDecControls
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

	int g_mpeg4_dec_err_flag;
} VideoDecControls;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#ifdef _VSP_LINUX_
void MP4DecSetPostFilter(VideoDecControls *decCtrl, int en);
typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
typedef int (*FunctionType_MemAllocCB)(void *decCtrl, void *userData, unsigned int width,unsigned int height);

void Mp4DecRegMemAllocCB (VideoDecControls *decCtrl, void *userdata, FunctionType_MemAllocCB extMemCb);
void MP4DecRegBufferCB(VideoDecControls *decCtrl, FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);
void MP4DecReleaseRefBuffers(VideoDecControls *decCtrl);
int MP4DecGetLastDspFrm(VideoDecControls *decCtrl,void **pOutput);
void MP4DecSetCurRecPic(VideoDecControls *decCtrl, uint8	*pFrameY);
void MP4DecSetReferenceYUV(VideoDecControls *decCtrl, uint8 *pFrameY);
PUBLIC MMDecRet MP4DecMemCacheInit(VideoDecControls *decCtrl, MMCodecBuffer *pBuffer);
#endif

void Mp4GetVideoDimensions(VideoDecControls *decCtrl, int32 *display_width, int32 *display_height);
void Mp4GetBufferDimensions(VideoDecControls *decCtrl, int32 *width, int32 *height); 

/*****************************************************************************/
//  Description: Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecInit(VideoDecControls *decCtrl, MMCodecBuffer * pBuffer);

MMDecRet MP4DecVolHeader(VideoDecControls *decCtrl, MMDecVideoFormat *video_format_ptr);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecMemInit(VideoDecControls *decCtrl, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecDecode(VideoDecControls *decCtrl, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
//MMDecRet MPEG4_DecReleaseDispBfr(uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecRelease(VideoDecControls *decCtrl);

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN MPEG4DEC_VSP_Available (void);

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
void mpeg4dec_GetOneDspFrm (VideoDecControls *decCtrl, MMDecOutput * pOutput, int req_transposed, int is_last_frame);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
