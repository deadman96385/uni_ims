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
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#ifdef _VSP_LINUX_
void MP4DecSetPostFilter(int en);
typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);

void MP4DecRegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);
void MP4DecReleaseRefBuffers();
int MP4DecGetLastDspFrm(void **pOutput);
void MP4DecSetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
PUBLIC MMDecRet MP4DecMemCacheInit(MMCodecBuffer *pBuffer);
#endif
/*****************************************************************************/
//  Description: Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecInit(MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecMemInit(MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description:   Calculate the size of external memory for mpeg4 decoder
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecMemSizeCal(uint8 *pInStream, uint32 str_length, MMDecMemSize *mem_size);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecDecode(MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MPEG4_DecReleaseDispBfr(uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecRelease(void);

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
void mpeg4dec_GetOneDspFrm (MMDecOutput * pOutput, int req_transposed, int is_last_frame);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
