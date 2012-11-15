/******************************************************************************
 ** File Name:      mp4enc_ratecontrol.h                                      *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of rate control*
 **                 of mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/

#ifndef _RATECONTROL_H_
#define _RATECONTROL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define RC_MAX_SLIDING_WINDOW	8

//unit is ()%
#define RC_SAFETY_MARGIN 		10

#define RC_MAX_Q_INCREASE		25
#define	RC_MAX_Q_I_INCREASE		25

#define	RC_MAX_QUANT			28
#define	RC_MIN_QUANT			4

#define	RC_MAX_QP_I				28
#define	RC_MIN_QP_I				6

#define	IVOP_SKIP_MARGIN		50
#define	PVOP_SKIP_MARGIN		80
#define	BUFFER_FULL_LEVEL		30

#define	SCENE_CUT_SAD			20

#define	COMPLEX_RATIO			50

#define	RE_ENC_SHEME

typedef struct
{
	/*for statictics the bits used in header_mv coding or texture coding in one frame*/
	int32		nbits_hdr_mv;
	int32		nbits_texture;
	int32		nbits_total;

	int32		sad;

	/*for statictics the bits used in header_mv coding or texture coding in sequence*/
	int32		nbit_hdr_mv_seq;
	int32		nbits_texture_seq;
	int32		nbits_total_seq;

//	double	dSNRY;
//	double	dSNRU;
//	double	dSNRV;

	int32		skip_cnt;
	int32		total_bits; //for whole sequence
//	int32		is_pfrm_skipped;
	int32		p_count;

	int32		bit_rate;
	int32		frame_rate;
	int32		rc_ena;
	int32		qp_i;
	int32		qp_p;
	int32		p_between_i;

}RateCtrlPara;

typedef struct _RCMode  /* for rate control */
{
	//these parameter for re-encode one frame after analyze the encoded result,
	//when the one pass model is failed
	BOOLEAN		be_re_enc;		//the frame should be re-encoded;
	BOOLEAN		be_scene_cut;	//is scene change;
	BOOLEAN		be_skip_frame;	//the frame should be skipped;
	int8		Qp_type;		//according to encoder result, to judge the QP type, large, small or too small
	int32		Ec_actual_Q8;	//actual Ec of current frame;
	
	int32		X1;				// 1st order coefficient
	int32		X2;				// 2nd order coefficient
	int32		Rs;				// bit rate for sequence. e.g. 24000 bits/sec	
	int32		Rf;				// bits used for the first frame, e.g. 10000 bits
	int32		Rc;				// bits used for the current frame after encoding
	int32		Rp;				// bits to be removed from the buffer per picture
//	double		Ts;				// number of seconds for the sequence, e.g. 10 sec

//	double		Ec;				// mean absolute difference for the current frame after motion compensation
	int32		Ec_Q8;			// fix point of Ec with Q value 8

//	double		Ep;				// mean absolute difference for the previous frame after motion compensation
	int32		Ep_Q8;			// fix point of Ep with Q value 8
	
	int32		Qc;				// quantization level used for the current frame
	int32		Qp;				// quantization level used for the previous frame
//	int32		Nr;				// number of P frames remaining for encoding
	int32		Nc;				// number of P frames coded
	int32		Nc_prev_gop;	// number of P frames in previous gop
//	int32		Ns;				// distance between encoded frames
//	int32		Rr;				// number of bits remaining for encoding this sequence 
	int32		T;				// target bit to be used for the current frame
	int32		S;				// number of bits used for encoding the previous frame
	int32		Hc;				// header and motion vector bits used in the current frame
	int32		Hp;				// header and motion vector bits used in the previous frame
	int32		Bs;				// buffer size 
	int32		B;				// current buffer level
//	Bool		NrFlag;
	BOOLEAN		skipNextFrame;						// TRUE if buffer is 80% full
	
	int			is_pfrm_skipped;

	BOOLEAN		FirstGOP;
	int32		iPVopQP;
	int32		rgQp[RC_MAX_SLIDING_WINDOW];		// quantization levels for the past frames
	
//	double		rgRp[RC_MAX_SLIDING_WINDOW];		// scaled encoding complexity used for the past frames;
	int32		rgRp[RC_MAX_SLIDING_WINDOW];		

	BOOLEAN		rgRejected[RC_MAX_SLIDING_WINDOW];	// outliers
	int32		QpMeanLastGOP;
	
//	double		EcI[10];							//store the last 5 I frame's complexity.
	int32		EcP_Q8[RC_MAX_SLIDING_WINDOW];
}RCMode;

PUBLIC void Mp4Enc_InitRateCtrl(RateCtrlPara *rc_par_ptr, RCMode *rc_mode_ptr);
PUBLIC int32 Mp4Enc_JudgeFrameType(RateCtrlPara *rc_par_ptr, RCMode *rc_mode_ptr);
PUBLIC void Mp4Enc_ResetRCModel(ENC_VOP_MODE_T *vop_mode_ptr, RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr); 
PUBLIC void Mp4Enc_UpdateRCModel(ENC_VOP_MODE_T *vop_mode_ptr,	RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr, int32 Ec_Q8);
PUBLIC void Mp4Enc_UpdatePVOP_StepSize(ENC_VOP_MODE_T *vop_mode_ptr, RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr);
PUBLIC void Mp4Enc_UpdateIVOP_StepSize(ENC_VOP_MODE_T *vop_mode_ptr, RCMode * pRCMode);
PUBLIC void Mp4Enc_AnalyzeEncResult(RCMode *rc_mode_ptr, int32 total_bits_cur, int32 vop_type, int32 Ec_Q8);

PUBLIC void Mp4Enc_InitRCFrame(RateCtrlPara	*rc_par_ptr);
PUBLIC void Mp4Enc_InitRCGOP(RateCtrlPara *rc_par_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_RATECONTROL_H_

