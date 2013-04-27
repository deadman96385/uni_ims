/******************************************************************************
 ** File Name:    mpeg4dec.h                                                   *
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
#ifndef _MPEG4_DEC_H_
#define _MPEG4_DEC_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mmcodec.h"
#include "vsp_mp4_dec.h"

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define MP4DEC_INTERNAL_BUFFER_SIZE (MP4DEC_OR_RUN_SIZE+MP4DEC_OR_INTER_MALLOC_SIZE)  
#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
typedef int (*FunctionType_MallocCB)(uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

typedef enum {IVOP, PVOP, BVOP, SVOP, NVOP} VOP_PRED_TYPE_E;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void MP4DecSetPostFilter(int en);
void MP4DecRegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);
void MP4DecReleaseRefBuffers();
int MP4DecGetLastDspFrm(void **pOutput);
void MP4DecSetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
//PUBLIC MMDecRet MP4DecMemCacheInit(MMCodecBuffer *pBuffer);


/*****************************************************************************/
//  Description: Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecMemInit(MMCodecBuffer *buffer_ptr);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr);

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
//BOOLEAN MPEG4DEC_VSP_Available (void);

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
//void mpeg4dec_GetOneDspFrm (MMDecOutput * pOutput, int req_transposed, int is_last_frame);


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
