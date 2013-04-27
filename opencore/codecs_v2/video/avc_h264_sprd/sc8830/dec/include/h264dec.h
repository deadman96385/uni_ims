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
#include "vsp_h264_dec.h"
/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define H264_DECODER_INTERNAL_BUFFER_SIZE (H264DEC_OR_RUN_SIZE+H264DEC_OR_INTER_MALLOC_SIZE)  //0x200000

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader);
typedef int (*FunctionType_MallocCB)(void* aUserData, uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description: Init h264 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecMemInit(MMCodecBuffer pBuffer[]);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet H264DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr);


/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264_DecReleaseDispBfr(uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecRelease(void);


/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
void H264Dec_GetOneDspFrm (MMDecOutput * pOutput, int req_transposed, int is_last_frame);


void H264Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);
void H264Dec_ReleaseRefBuffers();
int H264Dec_GetLastDspFrm(void **pOutput);


void H264Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);

void H264Dec_RegMallocCB(FunctionType_MallocCB mallocCb);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif //_H264_DEC_H_
// End
