/******************************************************************************
 ** File Name:      mp4enc_mode.h                                             *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the mode define of mp4 codec		      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_MODE_H_
#define _MP4ENC_MODE_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "mp4enc_basic.h"
#include "mpeg4enc.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

typedef enum {
    BASE_LAYER
}
VOL_TYPE_E;	/* will be generlized later*/

typedef struct Mp4Enc_storable_pic
{
    uint8 * imgY;
    uint8 * imgU;
    uint8 * imgV;

    uint_32or64 imgYAddr;
    uint_32or64 imgUAddr;
    uint_32or64 imgVAddr;
} Mp4EncStorablePic;

//for Video Object Layer(VOL) Mode
typedef struct vol_mode_tag
{
    VOL_TYPE_E VolType;	/* Which type of VOL*/
    uint8 VolVerid;
    uint8 NumBitsTimeIncr;

    BOOLEAN short_video_header;

    //for VOL size
    uint16 VolWidth;
    uint16 VolHeight;

    uint8 QuantPrecision;
    uint8 nBits;
    BOOLEAN bNot8Bit;

    /* time info*/
    uint32 ClockRate;	/*rate of clock used to count vops in Hz*/
    uint32 FrameHz;		/* Frame frequency (Hz), (floating point in case of 29.97 Hz) 	*/

    //for shape info
    ALPHA_USAGE_E fAUsage;	/*binary or no alpha (rectangle VO)*/

    /* motion info*/
    int32	 InitialRoundingType;
    BOOLEAN bOriginalForME;		/* flag indicating whether use the original previous VOP for ME*/
    BOOLEAN bAdvPredDisable;		/* No OBMC, (8x8 in the future).*/
    BOOLEAN bRoundingControlDisable;

    BOOLEAN intra_acdc_pred_disable;
    BOOLEAN bResyncMarkerDisable;	/* resync marker Disable*/

    BOOLEAN bVPBitTh;			/* Bit threshold for video packet spacing control*/
    BOOLEAN bDataPartitioning;	/* data partitioning*/
    BOOLEAN bReversibleVlc;	/* reversible VLC*/

    BOOLEAN bCodeSequenceHead;
    uint32 ProfileAndLevel;

    //for texture coding info
    QUANTIZER_E QuantizerType;		/* either H.263 or MPEG*/

    BOOLEAN bAllowSkippedPMBs;

    //for frame rate info
    uint16 PbetweenI;
    uint8 GOVperiod;	/*number of VOP from GOV header to next GOV header*/
    int32 TemporalRate;	/*no. of input frames between two encoded VOP's assuming 30Hz input*/

    int32 MVRadiusPerFrameAwayFromRef;	/* MV serach radius per frame away from reference VOP*/
} VOL_MODE_T;

//for VideoObjectPlane Mode
typedef struct enc_vop_mode_tag
{
    /*about the size and position*/
    int8 mb_x;
    int8 mb_y;
    int8 MBNumX;
    int8 MBNumY;

    int16 MBNum;
    int16 MBNumOneVP;

    int16 OrgFrameWidth;
    int16 OrgFrameHeight;

    int16 FrameWidth;
    int16 FrameHeight;

    /*for short video header*/
    BOOLEAN short_video_header;
    int32 H263SrcFormat;
    BOOLEAN GOBResync;
    int8 MBLineOneGOB;

//	int8 MBNumberOneGOB;
//	int8 GOBNumOneFrame;
    int8 intra_acdc_pred_disable;
    int8 StepSize;

// 	int		resync_dis;
    int		mbline_num_slice;
    int		intra_mb_dis;

    /* user specify, per VOP*/
    int8 StepI;	/* I-VOP stepsize for DCT*/
    int8 StepP;	/* P-VOP stepsize for DCT*/
    int8 QuantPrecision;
    int8 QuantizerType;

    int8 QP_last[8];
    int8 Need_MinQp_flag;

    int8 bCoded;
    int8 srv;
    uint8 MB_in_VOP_length;

    uint32 time_inc_resolution_in_vol_length;

    //for packet resync decoding
    int16 mbnumDec;
    int16 sliceNumber;

    /* user specify, per VOP*/
    VOP_PRED_TYPE_E  VopPredType;	/*whether IVOP, PVOP, BVOP*/
    int32  IntraDcSwitchThr;		/*threshold to code intraDC as with AC VLCs*/
    int32  RoundingControl;		/*rounding control*/
    int32 RoundingControlEncSwitch;

    /* motion search info*/
    MV_INFO_T	mvInfoForward;					/* motion search info*/

    BOOLEAN bInterlace;		/* interlace coding flag*/
    uint16 SearchRangeForward;/* maximum search range for motion estimation*/
    BOOLEAN  bAlternateScan;                   /* Alternate Scan*/

    uint8 *pZigzag;

    int32 iMaxVal;    //for clip iquant coeff

    //int32 uv_interleaved;
    Mp4EncStorablePic *pYUVSrcFrame;  //frame to be encoded
    Mp4EncStorablePic *pYUVRecFrame; //store forward reference frame
    Mp4EncStorablePic *pYUVRefFrame; //store backward reference frame

    //for rate control
    uint32  bInitRCSuceess;
    uint32	RateCtrlEnable;            // 0 : disable  1: enable
    uint32	targetBitRate;
    uint32  FrameRate;

    uint8 *pOneFrameBitstream;
    uint_32or64 OneFrameBitstream_addr_phy;
    uint32 OneframeStreamLen;
} ENC_VOP_MODE_T;

typedef struct
{
    uint32 enable_anti_shake;
    uint32 input_width;
    uint32 input_height;
    uint32 shift_x;
    uint32 shift_y;
} ENC_ANTI_SHAKE_T;

typedef struct
{
    int reaction_delay_factor;
    int averaging_period;
    int buffer;

    int bytes_per_sec;
    double target_framesize;

    double time;
    int total_size;
    int rtn_quant;

    double sequence_quality;
    double avg_framesize;
    double quant_error[31];

    double fq_error;
}
rc_single_t;

typedef struct
{
    int fincr;
    int fbase;

    int min_quant[3];
    int max_quant[3];

    int type;
    int quant;
    int length;
} xvid_plg_data_t;

typedef enum
{
    INTER_MEM = 0, /*physical continuous and no-cachable, constant length */
    EXTRA_MEM,   /*physical continuous and no-cachable, variable length, need allocated according to image resolution */
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint8* v_base;  //virtual address
    uint_32or64 p_base;  //physical address
} CODEC_BUF_T;

typedef struct tagMp4EncObject
{
    uint_32or64 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 vsp_freq_div;
    int32	error_flag;
    int32   vsp_capability;

    MP4Handle  *mp4Handle;

    int32 g_enc_last_modula_time_base;
    int32 g_enc_tr;
    BOOLEAN g_enc_is_prev_frame_encoded_success;
    int32 g_re_enc_frame_number;

    VOL_MODE_T *g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *g_enc_vop_mode_ptr;

    uint32 *g_vlc_hw_ptr;

//rate control
    rc_single_t *g_rc_ptr;
    xvid_plg_data_t *g_rc_data_ptr;
    int32 g_stat_rc_nvop_cnt;

    uint32 g_nFrame_enc;
    uint32 g_enc_frame_skip_number;
    uint16 g_enc_p_frame_count;
    int32 yuv_format;

    ENC_ANTI_SHAKE_T g_anti_shake;

    CODEC_BUF_T mem[MAX_MEM_TYPE];
} Mp4EncObject;
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif	//_MP4ENC_MODE_H_

