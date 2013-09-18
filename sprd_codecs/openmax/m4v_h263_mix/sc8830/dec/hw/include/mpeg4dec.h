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
//#include "vsp_mp4_dec.h"

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

//#define MP4DEC_INTERNAL_BUFFER_SIZE (MP4DEC_OR_RUN_SIZE+MP4DEC_OR_INTER_MALLOC_SIZE)
//#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

typedef int (*FunctionType_BufCB)(void *userdata,void *pHeader,int flag);
typedef int (*FunctionType_MemAllocCB)(/*void *decCtrl,*/ void *userData, unsigned int width,unsigned int height);
typedef int (*FunctionType_MallocCB)(uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size);

typedef enum {IVOP, PVOP, BVOP, SVOP, NVOP} VOP_PRED_TYPE_E;



/* Application controls, this structed shall be allocated */
/*    and initialized in the application.                 */
typedef struct tagMP4Handle
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
    FunctionType_MemAllocCB VSP_extMemCb;

    int g_mpeg4_dec_err_flag;
} MP4Handle;

#define MP4_INVALID_VOL_PARAM -1

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void MP4DecSetPostFilter(MP4Handle *mp4Handle, int en);
void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle);
int MP4DecGetLastDspFrm(MP4Handle *mp4Handle,void **pOutput);
void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader);

void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height);
void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height);

/*****************************************************************************/
//  Description: Init mpeg4 decoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * pBuffer);

MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *pInput,MMDecOutput *pOutput);

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
MMDecRet MP4DecRelease(MP4Handle *mp4Handle);

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
