/******************************************************************************
 ** File Name:    mmcodec.h                                                   *
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
 ** 5/19/2009     Xiaowei.Luo     Modification.                               *
 *****************************************************************************/
#ifndef __MMCODEC_H__
#define __MMCODEC_H__
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
/**---------------------------------------------------------------------------*
**                             Compiler Flag                                  *
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#ifdef SIM_NO_VSP
typedef unsigned char	uint8; 
typedef signed char		int8; 
//typedef unsigned char	Bool; 
typedef signed short	int16;
typedef unsigned short	uint16;
typedef signed int		int32;
typedef unsigned int	uint32;
#else
#include "sci_types.h"

#endif
/**---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/

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
	MMDEC_FRAME_SEEK_IVOP = -9
} MMDecRet;

typedef enum
{
    MMENC_OK = 0,
    MMENC_ERROR = -1,
    MMENC_PARAM_ERROR = -2,
    MMENC_MEMORY_ERROR = -3,
    MMENC_INVALID_STATUS = -4,
    MMENC_OUTPUT_BUFFER_OVERFLOW = -5,
    MMENC_HW_ERROR = -6 
} MMEncRet;

// decoder video format structure
typedef struct 
{
	int32 	video_std;			//video standard, 0: ITU_H263, 1: MPEG4, 2: JPEG, 3: FLV_H263 		
	int32	frame_width;
	int32	frame_height;
	int32	i_extra;
	void 	*p_extra;
#ifdef _VSP_LINUX_					
	void *p_extra_phy;
#endif		
}MMDecVideoFormat;

// Decoder buffer for decoding structure
typedef struct 
{
    uint8	*common_buffer_ptr;     // Pointer to buffer used when decoding
#ifdef _VSP_LINUX_					
    void *common_buffer_ptr_phy;
#endif	         
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
#ifdef _VSP_LINUX_					
    uint8    *pStream_phy;
#endif             
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
#ifdef _VSP_LINUX_					
	void *pBufferHeader;
	int reqNewBuf;
#endif	
}MMDecOutput;

// Encoder video format structure
typedef struct 
{
	int32	is_h263;					// 1 : H.263, 0 : MP4
	int32	frame_width;				//frame width
	int32	frame_height;				//frame Height
	int32	time_scale;
}MMEncVideoInfo;

// Encoder config structure 
typedef struct
{
    uint32	RateCtrlEnable;            // 0 : disable  1: enable
    uint32	targetBitRate;             // 400 ~  (bit/s)
    uint32  FrameRate;
    uint32	QP_IVOP;     				// first I frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled
    uint32	QP_PVOP;     				// first P frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled
    
    uint32	h263En;            			// 1 : H.263, 0 : MP4

    uint32	profileAndLevel;   		
}MMEncConfig;

// Encoder input structure
typedef struct
{
    uint8	*pInBuf;					//YUV buffer to be encoded
    uint32	inBufSize;					//YUV buffer size to be encoded
    int32	vopType;					//vopµƒ¿‡–Õ  0 - I Frame    1 - P frame
    int32	time_stamp;					//time stamp
	int32   bs_remain_len;				//remained bitstream length
	int32 	channel_quality;			//0: good, 1: ok, 2: poor
} MMEncIn;

// Encoder output structure
typedef struct
{
    uint8	*pOutBuf;					//Output buffer
    int32	strmSize;					//encoded stream size, if 0, should skip this frame.
} MMEncOut;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif

#endif //__MMCODEC_H__