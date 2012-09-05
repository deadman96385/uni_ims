/******************************************************************************
 ** File Name:      mp4dec_mode.h		                                      *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    basic coding modes for VO, VOL, VOP and MB.			      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_MODE_H_
#define _MP4DEC_MODE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "sci_types.h"
#include "video_common.h"
#include "mp4_basic.h"
#include "mmcodec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//typedef enum {BASE_LAYER}VOL_TYPE_E;	/* will be generlized later*/
// typedef enum {INTRA,INTRAQ,INTER,INTERQ,INTER4V,MODE_STUFFING=7}DCT_MODE_E;	
typedef enum {DIRECT,INTERPOLATE,BACKWARD,FORWARD}MB_TYPE_E; 
typedef enum {UNKNOWN_DIR,HORIZONTAL,VERTICAL,DIAGONAL}DIRECTION_E;	
typedef enum {NOT_DECODED, DECODED_IN_ERR_PKT, DECODED_NOT_IN_ERR_PKT, DECODED_OK, ERR_CONCEALED}MB_DEC_STATUS_E;

typedef struct cbpy_table_code_len_tag
{
	uint8 code;	/* right justified *///the value of the variable length code
	uint8 len;	// the length of the variable length code
}CBPY_TABLE_CODE_LEN_T;

typedef struct dc_table_code_len_tag
{
	int8 code;	/* right justified *///the value of the variable length code, modified by xiaowei
	uint8 len;	// the length of the variable length code
}DC_TABLE_CODE_LEN_T;

#define DEC_BS_BFR_SIZE		5*1024

#define MODE_NOT_CODED	16
#define MODE_NOT_CODED_GMC	17

/* --- bframe specific --- */
#define MODE_DIRECT			0
#define MODE_INTERPOLATE	1
#define MODE_BACKWARD		2
#define MODE_FORWARD		3
#define MODE_DIRECT_NONE_MV	4
#define MODE_DIRECT_NO4V	5

/*define mb mca type*/
/*
NOTE: 
for P frmae: 
inter mb is MCA_BACKWARD, four_mv mb is MCA_BACKWARD_4V,
so reference id is 1, and use backward reference frame

for B frame: forward mb is MCA_FORWARD, backward mb is MCA_BACKWARD
*/

#define MCA_FORWARD			0			//only for b frame's forward MB
#define MCA_BACKWARD_4V		1			//for p frame's 4 mv mb type
#define MCA_BACKWARD		2			//for B frame's backward mb type and p frame's inter mb
#define MCA_BI_DRT			3			//for B frame's interpolate MB
#define MCA_BI_DRT_4V		4			//for B frame's direct MB

//for MacroBlock Mode
typedef struct _MBMode  //MacroBlock Mode*
{
	BOOLEAN	bIntra;
	int8	StepSize;	//qp for texture
	uint8	videopacket_num;
	int8	dctMd;		//is the Macroblock inter- or intra- coded

	BOOLEAN	bFirstMB_in_VP;
	int8	CBP;
	BOOLEAN	bSkip;	    //is the Macroblock skiped. = COD in H.263
	BOOLEAN	bACPrediction;// use AC prediction or not

	//must word align before here. xiaowei.luo@20090115
	MOTION_VECTOR_T mv[4];//for 4 block's mv
}DEC_MB_MODE_T;

//for macro block buffer
typedef struct dec_mb_buffer_tag
{
	int8	QpPred;
	BOOLEAN	bTopMBAvail;	//for intra prediction and motion vector prediction
	BOOLEAN	bLeftMBAvail;
	BOOLEAN	bLeftTopAvail;
	
	int8	rightAvail;
	int8	leftMBQP;
	int8	topMBQP;
	int8	leftTopMBQP;
	
	int16	pDCCache[4*3]; //current mb's top left dc
	int16	pLeftDCCache[3]; //left mb's top left dc
	int16	rev;//for word-aligned.

	BOOLEAN bCodeDcAsAc;  //code Intra DC with Ac VLC
	int8	mca_type;	
	int8	iDcScalerY;
	int8	iDcScalerC;
	int32	start_pos;	//to determine whether the MB is located in erroe domain
	int32	end_pos;

	MOTION_VECTOR_T fmv[4];//store forward mv for B frame decoding
	MOTION_VECTOR_T bmv[4];//store backward mv for B frame decoding	

	uint32	BS[8];
}DEC_MB_BFR_T;

typedef struct dec_buffer_seq_info_tag
{
	uint8 *imgY;		//should be 64 word alignment
	uint8 *imgU;
	uint8 *imgV;

	uint32 imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
	uint32 imgUAddr;	//imgUVAddr = ((uint32)imgU>>8)
	uint32 imgVAddr;	//imgUVAddr = ((uint32)imgV>>8)

	uint8	id;    	// buffer number 
	BOOLEAN bRef;   	// FALSE£¬not to be  ref frame 
					// TRUE, to  the ref frame
	BOOLEAN bDisp; 	// FALSE, not to be display frame
					// TRUE, to be the display frame

	uint8 *rec_imgY;
#ifdef _VSP_LINUX_					
	void *pBufferHeader;
#endif						
}DEC_FRM_BFR;

typedef struct Mp4Dec_storable_pic
{
	int		time;		//display time of the frame
	int		bfrId;		//yuv buffer index	

	DEC_FRM_BFR *pDecFrame;

} Mp4DecStorablePic;

//for VideoObjectPlane Mode
typedef struct dec_vop_mode_tag 
{
	/*about the size and position*/
	int8 mb_x;
	int8 mb_y;
	int8 MBNumX;
	int8 MBNumY;
	
	int16 MBNum;
	int16 OrgFrameWidth;

	int16 OrgFrameHeight;
	int16 FrameWidth;

	int16 FrameHeight;	
	uint16 NumMBInGob;

	int16 num_mbline_gob;
	int16 post_filter_en;

	int8 NumGobInVop;	/*for short video header*/
	int8 GobNum;
	int8 video_std;
	int8 intra_acdc_pred_disable;
	
	int8 StepSize;
	int8 QuantPrecision;
	int8 QuantizerType;	
	int8 bQuarter_pel;
	
	uint32 size_decode_flag; 
	
	BOOLEAN bCoded;
	BOOLEAN error_flag;
	uint8 MB_in_VOP_length;
	uint8 time_inc_resolution_in_vol_length;

	int32 frame_len;

	BOOLEAN bResyncMarkerDisable;
	uint8 NumBitsTimeIncr;
	BOOLEAN bInitSuceess;
	BOOLEAN bInterlace; //interlace coding flag

	//for packet resync decoding
	int32 mbnumDec;
	int32 sliceNumber;
	int32 stop_decoding;

	int32 pre_vop_type;
	int32 VopPredType;//whether IVOP, PVOP, BVOP
	int32 IntraDcSwitchThr;	 //threshold to code intraDC as with AC VLCs
	int32 RoundingControl;	 //rounding control

	MV_INFO_T mvInfoForward; //motion search info
	MV_INFO_T mvInfoBckward; //motion search info
	
    BOOLEAN  bAlternateScan;    //Alternate Scan
    BOOLEAN sprite_enable;/*gmc*/	
	BOOLEAN  bDataPartitioning;	/* data partitioning*/
	BOOLEAN  bReversibleVlc;		/* reversible VLC*/

	BOOLEAN bLoadIntraQMatrix;		/* flag indicating whether to load intra Q-matrix*/
	BOOLEAN bLoadInterQMatrix;		/* flag indicating whether to load inter Q-matrix*/
	uint8 IntraQuantizerMatrix[BLOCK_SQUARE_SIZE]; /* Intra Q-Matrix*/
	uint8 InterQuantizerMatrix[BLOCK_SQUARE_SIZE]; /* Inter Q-Matrix*/

	DEC_MB_MODE_T *pMbMode;			//for storing one picture's mb mode, located in external memory
	DEC_MB_MODE_T *pMbMode_prev;			//mb mode for previous frame	
	DEC_MB_MODE_T *pMbMode_B;    		//for storing one mb's mb mode, located in on-chip memory, only for BVOP
	DEC_MB_BFR_T  *mb_cache_ptr;

	int16 *pTopCoeff;
	int16 *pTopLeftDCLine;

	int32 uv_interleaved;
	Mp4DecStorablePic *pCurDispFrame;
	Mp4DecStorablePic *pCurRecFrame;
	Mp4DecStorablePic *pBckRefFrame;
	Mp4DecStorablePic *pFrdRefFrame;

	/*for huffman decoding*/
	const MCBPC_TABLE_CODE_LEN_T *pMCBPCtabintra;
	const MCBPC_TABLE_CODE_LEN_T *pMCBPCtab;
	const CBPY_TABLE_CODE_LEN_T *pCBPYtab;

	const MV_TABLE_CODE_LEN_T *pTMNMVtab0;
	const MV_TABLE_CODE_LEN_T *pTMNMVtab1;
	const MV_TABLE_CODE_LEN_T *pTMNMVtab2;

	const VLC_TABLE_CODE_LEN_T *pDCT3Dtab0;
	const VLC_TABLE_CODE_LEN_T *pDCT3Dtab1;
	const VLC_TABLE_CODE_LEN_T *pDCT3Dtab2;
	int32 sw_vld_flag;

	/*******************************************************/
	/*for computing time stamp for every frame*/
	int32 time;				/* for record time */
	int32 time_base;
	int32 last_time_base;
	int32 last_non_b_time;
	int32 time_pp;
	int32 time_bp;
	int32 time_inc_resolution;

	/*gmc*/	
//	int sprite_enable;
	int sprite_warping_points;
	int sprite_warping_accuracy;
	int sprite_brightness_change;

	//flv_h263
	int8 h263_flv;
	int8 h263_plus;
	int8 unrestricted_mv;
	int8 h263_long_vectors;
	int32 picture_number;

	/*error information*/
	int			err_num;
	int			err_left;
	ERR_POS_T *	err_pos_ptr;
	
	int32 err_MB_num;		//error MB number

	uint8 *mbdec_stat_ptr;	//indicate mb decoded status, 
							//0: not decoded, 
							//1: decoded, but in error packet, 
							//2: decoded, not in error packet,
							//3: error concealed

	int32 top_grad[16];
	int32 top_dir[16];
	int32 bot_grad[16];
	int32 bot_dir[16];
	int32 top_avg_grad;
	int32 top_var_grad;
	int32 bot_avg_grad;
	int32 bot_var_grad;

	//mv range
	int32 mv_x_max;
	int32 mv_x_min;
	int32 mv_y_max;
	int32 mv_y_min;
	
	//dct config
	uint32 intra_dct_cfg;
	uint32 inter_dct_cfg;

	int32 find_vop_header;
}DEC_VOP_MODE_T;

typedef struct h263_plus_header_info_tag 
{
	int32 UFEP;
	int32 source_format;
	int32 mv_outside_frame;
	int32 long_vectors;
	int32 syntax_arith_coding;
	int32 adv_pred_mode;
	int32 overlapping_MC;
	int32 use_4mv;
	int32 pb_frame;

	/* Following variables are used in H.263+ bitstream decoding */
	int32 plus_type;           /* indicates if extended PTYPE exists or not. */
	int32 optional_custom_PCF;
	int32 advanced_intra_coding;
	int32 deblocking_filter_mode;
	int32 slice_structured_mode;
	int32 reference_picture_selection_mode;
	int32 independently_segmented_decoding_mode;
	int32 alternative_inter_VLC_mode;
	int32 modified_quantization_mode;
	int32 reduced_resolution_update_mode;
	int32 reference_picture_resampling_mode;
	int32 rtype;
 	int32 post_filter;
	int32 unlimited_unrestricted_motion_vectors;
	int32 concealment;

	int32 CP_clock_frequency;
	int32 temp_ref;
	int32 prev_non_disposable_temp_ref;
	int32 next_non_disposable_temp_ref;

	int32 trd;
	int32 trb;
	int32 bscan;
	int32 bquant;
	int32 true_b_trb;
}H263_PLUS_HEAD_INFO_T;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif	//_MP4DEC_MODE_H_

