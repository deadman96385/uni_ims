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

#define VP8_DECODER_INTERNAL_BUFFER_SIZE (VP8DEC_OR_RUN_SIZE+VP8DEC_OR_INTER_MALLOC_SIZE)  //0x200000

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
//typedef int (*FunctionType_MallocCB)(void* aUserData, uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

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
//    int nLayers;
    /* pointers to VOL data for frame-based decoding. */
//    uint8 *volbuf[2];           /* maximum of 2 layers for now */
//    int32 volbuf_size[2];

        void *userdata;

	FunctionType_BufCB VSP_bindCb;
	FunctionType_BufCB VSP_unbindCb;
//        FunctionType_MemAllocCB VSP_extMemCb;
//	void *g_user_data;


} VPXHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

void VP8DecSetCurRecPic(VPXHandle *vpxHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
//void VP8Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata);

/*****************************************************************************/
//  Description: Init vpx decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer * pBuffer);

//MMDecRet VP8DecHeader(VPXHandle *vpxHandle, MMDecVideoFormat *pVideoFormat);

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


typedef void (*FT_VPXDecSetCurRecPic)(VPXHandle *vpxHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);
typedef MMDecRet (*FT_VPXDecInit)(VPXHandle *vpxHandle, MMCodecBuffer * pBuffer);
typedef MMDecRet (*FT_VPXDecDecode)(VPXHandle *vpxHandle, MMDecInput *pInput,MMDecOutput *pOutput);
typedef MMDecRet (*FT_VPXDecRelease)(VPXHandle *vpxHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif //_VP8_DEC_H_
// End
