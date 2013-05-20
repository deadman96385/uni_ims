/******************************************************************************
 ** File Name:      mp4enc_mode.h                                             *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the mode define of mp4 codec		      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
  ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
#ifndef _MP4ENC_MODE_H_
#define _MP4ENC_MODE_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "mp4_basic.h"

#include "mmcodec.h"
#include "mpeg4enc.h"
//#include "mp4enc_ratecontrol.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
typedef enum {BASE_LAYER}VOL_TYPE_E;	/* will be generlized later*/

#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.

typedef struct enc_mb_mode_tag /* MacroBlock Mode*/
{
	BOOLEAN    bSkip;	    	// is the Macroblock skiped. = COD in H.263
	int8	dctMd;		// is the Macroblock inter- or intra- coded
	int8    intStepDelta;	// change of quantization stepsize = DQUANT in h.263
	int8    StepSize;		//qp for texture

	BOOLEAN	bIntra;
	int8	CBP;
	uint8	iPacketNumber;
	BOOLEAN	bACPrediction;
	
 	MOTION_VECTOR_T mvPred;
	MOTION_VECTOR_T mv [4];
}ENC_MB_MODE_T;

typedef struct enc_mb_buffer_tag
{
//	uint8 pMBBfrY[256];
//	uint8 pMBBfrU[64];
//	uint8 pMBBfrV[64];
//
//	uint8 pPredMBBfrY[256];
//	uint8 pPredMBBfrU[64];
//	uint8 pPredMBBfrV[64];
	
	int8 dc_scaler_y;
	int8 dc_scaler_c;

	BOOLEAN 	bTopMBAvail;	//for intra prediction and motion vector prediction
	BOOLEAN	bLeftMBAvail;
	
	BOOLEAN	bLeftTopAvail;
	int8	rightAvail;	
	int16	rev1;
	
// 	int16 pBlkDc[6];

	uint8 *pSrcMb[6];
	uint8 *pRefMb[6];
	
	/*for store six block's DCT coefficient*/
//	DCT_IO_BUFFER_T *pDct_io_bfr;
//	int16 *pDctCoef[6];
//	int16 *pVlcCoef[6];  //store coefficient whitch need be sent to vlc. 
}ENC_MB_BFR_T;

typedef struct vop_info_tag
{
	BOOLEAN bEnc_success;	//是否正确编码
	int32 length;		//码流的长度
	int32 vop_type;	//vop的类型  0 - I Frame    1 - P frame
}VOP_INFO_T;

typedef struct Mp4Enc_storable_pic
{
	uint8 * imgY;
	uint8 * imgU;
	uint8 * imgV;

	uint32 imgYAddr;
	uint32 imgUAddr;	
	uint32 imgVAddr;	
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
}VOL_MODE_T;

//for VideoObjectPlane Mode
typedef struct enc_vop_mode_tag 
{
	/*about the size and position*/
	int8 mb_x;
	int8 mb_y;
	int8 MBNumX;
	int8 MBNumY;

	int8 mb_x_me;
	int8 mb_y_me;
	int8 mb_x_fetch;
	int8 mb_y_fetch;
	
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
	int8 error_flag;
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

	ENC_MB_MODE_T *pMbModeAbv;
	ENC_MB_MODE_T *pMbModeCur;

	ENC_MB_BFR_T * pMBCache;

	int32 uv_interleaved;
	Mp4EncStorablePic *pYUVSrcFrame;  //frame to be encoded
	Mp4EncStorablePic *pYUVRecFrame; //store forward reference frame
	Mp4EncStorablePic *pYUVRefFrame; //store backward reference frame

	uint16 *pQuant_pa;
	uint16 *pDC_scaler;
 	MCBPC_TABLE_CODE_LEN_T *pMcbpc_intra_tab;
	MCBPC_TABLE_CODE_LEN_T *pCbpy_tab;
	MCBPC_TABLE_CODE_LEN_T *pMcbpc_inter_tab;
	MV_TABLE_CODE_LEN_T *pMvtab;

	//for rate control
	uint32  bInitRCSuceess;
	uint32	RateCtrlEnable;            // 0 : disable  1: enable
	uint32	targetBitRate;
	uint32  FrameRate;

	uint8 *pOneFrameBitstream;
	uint32 OneframeStreamLen;

	uint32 vbv_buf_size;

	uint16 big_size_flag;	//for above 352x288
	int16 is_need_init_vsp_huff_tab;
	//just for verification by reading yuv from file and dumping bitstream out.
	void* bits;
	void* yuv_in;
}ENC_VOP_MODE_T;

typedef struct enc_mbc_command_tag
{
	uint32 MBC_CMD_Reg0;
	uint32 MBC_CMD_Reg1;
}ENC_MBC_CMD_T;

typedef struct enc_mca_command_tag
{
	uint32 MCA_CMD[33];
	uint32 Valid_Cmd_Num;
}ENC_MCA_CMD_T;

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
}xvid_plg_data_t;

typedef struct tagMp4EncObject
{
	MP4EncHandle *mp4EncHandle;

	VOL_MODE_T *g_enc_vol_mode_ptr;
	ENC_VOP_MODE_T *g_enc_vop_mode_ptr;

	rc_single_t *g_rc_ptr;
	xvid_plg_data_t *g_rc_data_ptr;

	//old version rate control  
//	RCMode	g_stat_rc;		// Rate control mode status
//	int		g_stat;
//	RateCtrlPara g_rc_par;
	int32 g_stat_rc_nvop_cnt;
	
	uint8 *g_enc_yuv_src_frame[3];   // current source frame

	int32 g_enc_last_modula_time_base;
	int32 g_enc_tr;
	BOOLEAN g_enc_is_prev_frame_encoded_success;
//	int32 g_re_enc_frame_number;

	uint32 g_nFrame_enc;

	uint32 g_enc_frame_skip_number;

	//for mpeg-4 time
//	uint32 g_enc_first_frame;
//	uint32 g_enc_last_frame; //encoder first and last frame number
//	int32 g_enc_vop_time_incr;	
//	uint32 g_enc_bits_modulo_base;
//	int32 g_enc_modulo_base_disp;		//of the most recently displayed I/Pvop
//	int32 g_enc_modulo_base_decd;		//of the most recently decoded I/Pvop

//	uint8 *g_pEnc_output_bs_buffer;  //the pointer to the output encoded bistream buffer.

	uint16 g_enc_p_frame_count;

	uint32 g_ME_SearchCount;
	uint32 g_HW_CMD_START;

//	uint32 *g_cmd_data_ptr;
//	uint32 *g_cmd_info_ptr;
//	uint32 *g_cmd_data_base;
//	uint32 *g_cmd_info_base;

	
   //for  memory management


	uint32 s_Mp4EncExtraMemUsed;
	uint32 s_Mp4EncExtraMemSize ; // = 0x400000;  //16M

	uint32 s_Mp4EncInterMemUsed ;
	uint32 s_Mp4EncInterMemSize ; //50kbyte; // = 0x400000;


		
	uint8 *s_pEnc_Extra_buffer ; 
	uint8 *s_pEnc_Inter_buffer ;
	uint8 *s_extra_mem_bfr_phy_ptr;
	
	uint32 s_vsp_Vaddr_base ;
	int32 s_vsp_fd ;

#ifdef _DEBUG_TIME_
	struct timeval tpstart;
	struct timeval tpend1;
	struct timeval tpend2;
#endif

}Mp4EncObject;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif	//_MP4ENC_MODE_H_

