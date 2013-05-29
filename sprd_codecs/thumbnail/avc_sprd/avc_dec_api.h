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
//#include "mmcodec.h"
/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
typedef unsigned char		BOOLEAN;
//typedef unsigned char		Bool;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
//typedef unsigned int		uint;

typedef signed char			int8;
typedef signed short		int16;
typedef signed int			int32;


typedef enum
{
    MMDEC_OK = 0,
    MMDEC_ERROR = -1,
    MMDEC_PARAM_ERROR = -2,
	MMDEC_MEMORY_ERROR = -3,
	MMDEC_INVALID_STATUS = -4,
    MMDEC_STREAM_ERROR = -5,
    MMDEC_OUTPUT_BUFFER_OVERFLOW = -6,
    MMDEC_HW_ERROR = -7,
	MMDEC_NOT_SUPPORTED = -8,
	MMDEC_FRAME_SEEK_IVOP = -9,
	MMDEC_MEMORY_ALLOCED = -10
} MMDecRet;

/*standard*/
typedef enum {
		ITU_H263 = 0, 
		MPEG4,  
		JPEG,
		FLV_V1,
		H264,
		RV8,
		RV9
		}VIDEO_STANDARD_E;

// decoder video format structure
typedef struct 
{
	int32 	video_std;			//video standard, 0: VSP_ITU_H263, 1: VSP_MPEG4, 2: VSP_JPEG, 3: VSP_FLV_V1 		
	int32	frame_width;
	int32	frame_height;
	int32	i_extra;
	void 	*p_extra;
#ifdef _VSP_LINUX_					
//	void *p_extra_phy;
#endif		
}MMDecVideoFormat;

// Decoder buffer for decoding structure
typedef struct 
{
    uint8	*common_buffer_ptr;     // Pointer to buffer used when decoding
//#ifdef _VSP_LINUX_					
    void *common_buffer_ptr_phy;
//#endif	       
    uint32	size;            		// Number of bytes decoding buffer

	int32 	frameBfr_num;			//YUV frame buffer number
	
	uint8   *int_buffer_ptr;		// internal memory address
	int32 	int_size;				//internal memory size
}MMCodecBuffer;

typedef struct 
{
	uint16 start_pos;
	uint16 end_pos;
}ERR_POS_T;

#define MAX_ERR_PKT_NUM		30

// Decoder input structure
typedef struct
{
    uint8		*pStream;          	// Pointer to stream to be decoded
    uint32		dataLen;           	// Number of bytes to be decoded
	int32		beLastFrm;			// whether the frame is the last frame.  1: yes,   0: no

	int32		expected_IVOP;		// control flag, seek for IVOP,
	int32		pts;                // presentation time stamp

	int32		beDisplayed;		// whether the frame to be displayed    1: display   0: not //display

	int32		err_pkt_num;		// error packet number
	ERR_POS_T	err_pkt_pos[MAX_ERR_PKT_NUM];		// position of each error packet in bitstream
}MMDecInput;

// Decoder output structure
typedef struct
{
    uint8	*pOutFrameY;     //Pointer to the recent decoded picture
	uint8	*pOutFrameU;
	uint8	*pOutFrameV;
	
    uint32	frame_width;						
    uint32	frame_height;	

	int32   is_transposed;	//the picture is transposed or not, in 8800S4, it should always 0.
	
	int32	pts;            //presentation time stamp
	int32	frameEffective;

	int32	err_MB_num;		//error MB number
//#ifdef _VSP_LINUX_					
	void *pBufferHeader;
	int reqNewBuf;
        int32 mPicId;
//#endif	
}MMDecOutput;

typedef enum
{
    HW_NO_CACHABLE = 0, /*physical continuous and no-cachable, only for VSP writing and reading */
    HW_CACHABLE,    /*physical continuous and cachable, for software writing and VSP reading */
    SW_CACHABLE,    /*only for software writing and reading*/
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

    typedef struct
    {
        uint32 cropLeftOffset;
        uint32 cropOutWidth;
        uint32 cropTopOffset;
        uint32 cropOutHeight;
    } CropParams;

    typedef struct
    {
        uint32 profile;
        uint32 picWidth;
        uint32 picHeight;
        uint32 videoRange;
        uint32 matrixCoefficients;
        uint32 parWidth;
        uint32 parHeight;
        uint32 croppingFlag;
        CropParams cropParams;
    } H264SwDecInfo;

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
} VideoDecControls;
	
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

MMDecRet H264DecGetNALType(VideoDecControls *decCtrl, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc);
void H264GetBufferDimensions(VideoDecControls *decCtrl, int32 *aligned_width, int32 *aligned_height) ;
MMDecRet H264DecGetInfo(VideoDecControls *decCtrl, H264SwDecInfo *pDecInfo);


/*****************************************************************************/
//  Description: Init h264 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecInit(VideoDecControls *decCtrl, MMCodecBuffer * pBuffer,MMDecVideoFormat * pVideoFormat);

/*****************************************************************************/
//  Description: Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecMemInit(VideoDecControls *decCtrl, MMCodecBuffer *pBuffer);

/*****************************************************************************/
//  Description: Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecDecode(VideoDecControls *decCtrl, MMDecInput *pInput,MMDecOutput *pOutput);

/*****************************************************************************/
//  Description: frame buffer no longer used for display
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264_DecReleaseDispBfr(VideoDecControls *decCtrl, uint8 *pBfrAddr);

/*****************************************************************************/
//  Description: Close mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecRelease(VideoDecControls *decCtrl);

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN H264DEC_VSP_Available (VideoDecControls *decCtrl);

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
void H264Dec_GetOneDspFrm (VideoDecControls *decCtrl, MMDecOutput * pOutput, int req_transposed, int is_last_frame);

//#ifdef _VSP_LINUX_
typedef int (*FunctionType_Bind_CB)(void *userData/*, int32 index*/, uint8 **yuv);
typedef void (*FunctionType_UnBind_CB)(void *userData, int32_t index);
void H264Dec_RegBufferCB(VideoDecControls *decCtrl, FunctionType_Bind_CB bindCb,FunctionType_UnBind_CB unbindCb,void *userdata);
void H264Dec_ReleaseRefBuffers(VideoDecControls *decCtrl);
MMDecRet H264Dec_GetLastDspFrm(VideoDecControls *decCtrl, uint8 **pOutput, int32 *picId);
void H264Dec_SetCurRecPic(VideoDecControls *decCtrl, uint8 *pFrameY, int32 picId);
//typedef int (*FunctionType_SPS)(void* aUserData, uint width,uint height, uint aNumBuffers, uint profile);
typedef int (*FunctionType_SPS)(void* aUserData, unsigned int width,unsigned int height, unsigned int aNumBuffers);
typedef int (*FunctionType_FlushCache)(void* aUserData, int* vaddr,int* paddr,int size);
void H264Dec_RegSPSCB(VideoDecControls *decCtrl, FunctionType_SPS spsCb,void *userdata);
//MMDecRet H264DecMemCacheInit(VideoDecControls *decCtrl, MMCodecBuffer * pBuffer);

//#endif

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif //_H264_DEC_H_
// End
