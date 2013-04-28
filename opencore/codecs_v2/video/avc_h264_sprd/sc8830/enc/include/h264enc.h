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
#include "vsp_h264_enc.h"

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define H264ENC_INTERNAL_BUFFER_SIZE (H264ENC_OR_RUN_SIZE+H264ENC_OR_INTER_MALLOC_SIZE)  
#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader);
typedef int (*FunctionType_MallocCB)(uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

typedef enum {IVOP, PVOP, BVOP, SVOP, NVOP} VOP_PRED_TYPE_E;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description:   Init mpeg4 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat);

/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncGenHeader(MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncSetConf(MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncGetConf(MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncStrmEncode (MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncRelease(void);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
