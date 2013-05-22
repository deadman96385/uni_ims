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

#define VP8_DECODER_INTERNAL_BUFFER_SIZE (VP8DEC_OR_RUN_SIZE+VP8DEC_OR_INTER_MALLOC_SIZE)  //0x200000

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader);
typedef int (*FunctionType_MallocCB)(void* aUserData, uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

PUBLIC void VP8Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
void VP8Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);
void VP8Dec_RegMallocCB(FunctionType_MallocCB mallocCb);
MMDecRet VP8DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat);
MMDecRet VP8DecHeader(MMDecInput *dec_input_ptr,MMDecVideoFormat * pVideoFormat);
PUBLIC MMDecRet VP8DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr);
MMDecRet VP8DecRelease(void);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif //_VP8_DEC_H_
// End
