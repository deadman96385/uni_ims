/******************************************************************************
** File Name:      h264enc_rc.h	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/16/2011    Shangwen Li     Create.                                     *
*****************************************************************************/
#include "sc6800x_video_header.h"
#include <math.h>

#define INT_RC
//#define RC_DEBUG

#ifdef RC_DEBUG
	FILE * rc_fp;
#endif

int32 rc_init_GOP (RC_GOP_PARAS *rc_gop_paras)
{
	int32  GOP_init_QP;								//return value, the initial QP of the GOP
	uint32 target_rate = g_h264_enc_config->targetBitRate;
	uint32 rate_control_en = g_h264_enc_config->RateCtrlEnable;
	if(rate_control_en){
#ifdef INT_RC
	uint32 target_bitrate = (uint32)(target_rate-(target_rate>>8)-(target_rate>>10));
#else
	float  target_bitrate = (float)(target_rate*0.995);
//	uint16 frame_rate = INTRA_PERIOD;
#endif			
//	uint16 GOP_len = INTRA_PERIOD;					//no. of pictures in a GOP
//  uint16 GOP_P = INTRA_PERIOD-1;					//no. of P pictures in a GOP
//  uint16 GOP_I = GOP_len - GOP_P;					//no. of I pictures in a GOP
	uint32 GOP_no = g_nFrame_enc/INTRA_PERIOD;		//the GOP_no th GOP in the sequence
	uint32 GOP_alloc_bits;

	/*compute the total number of bits for the current GOP*/
#ifdef INT_RC
	GOP_alloc_bits = (uint32)target_bitrate;
#else
	GOP_alloc_bits = (uint32) floor(INTRA_PERIOD * target_bitrate / INTRA_PERIOD + 0.5);
#endif
	rc_gop_paras->rem_bits += GOP_alloc_bits;

	/*Compute InitialQp for each GOP*/
	if(GOP_no == 0)//the first GOP
	{
		GOP_init_QP = g_h264_enc_config->QP_IVOP;
	}
	else			//not the first GOP
	{
		rc_gop_paras->total_qp += g_h264_enc_config->QP_PVOP;
		GOP_init_QP = rc_gop_paras->total_qp / INTRA_PERIOD ;
		//GOP_init_QP = g_input->QP_PVOP-1;
	}

#ifdef RC_DEBUG
	if(g_nFrame_enc==0)
		rc_fp = fopen("rc.log", "w");
	fprintf(rc_fp, "rc_init_GOP\t: frame %d, target_bitrate = %d\n", g_nFrame_enc, target_bitrate);
	fprintf(rc_fp, "rc_init_GOP\t: frame %d, rc_gop_paras->rem_bits = %d\n", g_nFrame_enc, rc_gop_paras->rem_bits);
#endif
	}else{GOP_init_QP = g_h264_enc_config->QP_IVOP;}

	return GOP_init_QP;
}

int32 rc_init_pict(RC_GOP_PARAS *rc_gop_paras, RC_PIC_PARAS *rc_pic_paras)//calculate the target bits for a picture, here the whole picture/frame is a slice
{
	uint32 rate_control_en = g_h264_enc_config->RateCtrlEnable;
	if(rate_control_en){
//	uint16 GOP_len = INTRA_PERIOD;
	int32 frame_no_in_GOP = (int32)g_nFrame_enc % INTRA_PERIOD;
//	uint32 temp;
	int32 curr_qp;
	uint32 target_rate = g_h264_enc_config->targetBitRate;
#ifdef INT_RC
	uint32 target_bitrate = (uint32)(target_rate-(target_rate>>8)-(target_rate>>10));
	uint32 ratio;
#else
	float  target_bitrate = (float)(target_rate*0.995);
	double ratio;
#endif
//  uint32 frame_rate = 30;
	int32 overdue = FALSE; //could try to use the average QP of last frame
	int32 prev_frame_qp;
#ifndef NO_BU_CHANGE
	//uint32 BU_cnt = (uint16)ceil(g_input->pic_width*g_input->pic_height/256/(double)BU_SIZE);
#endif

	if (frame_no_in_GOP == 0) // I frame, calculate the target bits for Basic unit level rate control
	{
		rc_pic_paras->target_bits = (rc_gop_paras->rem_bits*I_P_RATIO)/(INTRA_PERIOD-1+I_P_RATIO);
		rc_pic_paras->remain_bits = rc_pic_paras->target_bits;
		rc_pic_paras->slice_num = 1;
		rc_pic_paras->total_qp = rc_pic_paras->curr_pic_qp;
		rc_gop_paras->total_qp = 0;
#ifdef RC_DEBUG
		fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_gop_paras->rem_bits = %d\n", g_nFrame_enc, rc_gop_paras->rem_bits);
		fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->target_bits = %d\n", g_nFrame_enc, rc_pic_paras->target_bits);
#endif
		return rc_pic_paras->curr_pic_qp;
	}
#ifdef CONST_QP_I
	if (frame_no_in_GOP >1)	// Intra frame is not rate-controlled
#else
	else	// P Frame
#endif
	{
		rc_pic_paras->remain_bits -= BU_bit_stat;
		if (rc_pic_paras->remain_bits < 0)  // the last frame overused its budget, need to increase QP
		{
			overdue = TRUE;
		}
	}

	//if(rc_pic_paras->total_qp == 0)
	//{
	//	prev_frame_qp = rc_pic_paras->curr_pic_qp;
	//}
	//else
	//{
#ifndef NO_BU_CHANGE
		//prev_frame_qp = Clip3(0, 40, rc_pic_paras->total_qp/BU_cnt);
#else
		//rc_pic_paras->total_qp += rc_pic_paras->curr_pic_qp;
		prev_frame_qp = Clip3(0, 40, rc_pic_paras->total_qp/rc_pic_paras->slice_num);
#endif
	//}

	rc_gop_paras->total_qp += prev_frame_qp;
	rc_pic_paras->slice_num = 1;
	//rc_pic_paras->total_qp = 0;

	//calculate the target buffer level for the picture
	rc_pic_paras->delta_target_buf = rc_gop_paras->curr_buf_full/(INTRA_PERIOD-frame_no_in_GOP);
	
	//calculate the target bits for the picture
#ifdef INT_RC
	rc_pic_paras->target_bits = (int)( rc_gop_paras->rem_bits / (INTRA_PERIOD-frame_no_in_GOP) );
    //temp  = MAX(0, (int)(target_bitrate/(INTRA_PERIOD-1+I_P_RATIO) - (rc_pic_paras->delta_target_buf>>1)));
    //rc_pic_paras->target_bits = (int)( (rc_pic_paras->target_bits>>1) + (temp>>1) + 1 );
	rc_pic_paras->target_bits = rc_pic_paras->target_bits - rc_pic_paras->delta_target_buf;
#else
	rc_pic_paras->target_bits = (int) floor( rc_gop_paras->rem_bits / (INTRA_PERIOD-frame_no_in_GOP) + 0.5);
    temp  = MAX(0, (int) floor(target_bitrate/(INTRA_PERIOD-1+I_P_RATIO) - 0.5*rc_pic_paras->delta_target_buf + 0.5));
    rc_pic_paras->target_bits = (int) floor(0.5 * rc_pic_paras->target_bits + 0.5 * temp + 0.5);
#endif

#ifdef RC_DEBUG
	fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->remain_bits = %d\n", g_nFrame_enc, rc_pic_paras->remain_bits);
	fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_gop_paras->rem_bits = %d\n", g_nFrame_enc, rc_gop_paras->rem_bits);
	fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->target_bits = %d\n", g_nFrame_enc, rc_pic_paras->target_bits);
	fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->delta_target_buf = %d\n", g_nFrame_enc, rc_pic_paras->delta_target_buf);
#endif
	rc_pic_paras->remain_bits = rc_pic_paras->target_bits;

#ifdef INT_RC
	ratio = ((uint32)(rc_gop_paras->prev_pic_bits<<10)/rc_pic_paras->target_bits);
#else
	ratio = ((double)rc_gop_paras->prev_pic_bits/rc_pic_paras->target_bits)*1024;
#endif

	if (frame_no_in_GOP == 1)
	{
		ratio = ratio/I_P_RATIO;
	}

	if (overdue)
	{
		curr_qp = Clip3(0, 40, prev_frame_qp+2); //rc_pic_paras->curr_pic_qp + 2;
	}
	else if (ratio >= 1126)
	{
		curr_qp = Clip3(0, 40, prev_frame_qp+2);//rc_pic_paras->curr_pic_qp+2;
	}
	else if (ratio < 1126 && ratio >= 1065)
	{
		curr_qp = Clip3(0, 40, prev_frame_qp+1);//rc_pic_paras->curr_pic_qp+1;
	}
	else if (ratio < 1065 && ratio >= 819)
	{
		curr_qp = Clip3(0, 40, prev_frame_qp);//rc_pic_paras->curr_pic_qp;
	}
	else if (ratio < 819 && ratio >= 683)
	{
		curr_qp = Clip3(0, 40, prev_frame_qp-1);//rc_pic_paras->curr_pic_qp-1;
	}
	else
	{
		curr_qp = Clip3(0, 40, prev_frame_qp-2);//rc_pic_paras->curr_pic_qp-2;
	}

	rc_pic_paras->total_qp = curr_qp;
	return curr_qp;
  }else{return rc_pic_paras->curr_pic_qp;}
	//update QP for the picture
}

#ifndef NO_BU_CHANGE
/*uint8 rc_init_BU(RC_PIC_PARAS *rc_pic_paras, RC_BU_PARAS *rc_bu_paras, uint16 BU_bit_stat, uint32 BU_nr)
{
	double ratio = 0;
	int32 overdue = FALSE;
	uint32 BU_cnt = (uint16)ceil(g_input->pic_width*g_input->pic_height/256/(double)BU_SIZE);
#ifdef PIPELINE_BU_RC
	uint8 prev_MB_qp = rc_bu_paras->curr_BU_QP;
#else
	uint8 prev_MB_qp = rc_pic_paras->curr_pic_qp;
#endif
	uint8 curr_MB_qp;
	uint16 pred_BU_bit_stat = (uint16)(((uint32)BU_bit_stat*20+15)/16);  //since currently the BU_bit_stat only store the first (BU_nr-PIPE_MB) MBs' bits, so need to rescale;

	if (BU_nr == 1)
	{
#ifdef PIPELINE_BU_RC
		rc_bu_paras->prev_target_bits = (int) floor((rc_pic_paras->target_bits)/BU_cnt);
		rc_pic_paras->pred_remain_bits = rc_pic_paras->target_bits-pred_BU_bit_stat;
#else
		rc_bu_paras->prev_target_bits = (int) floor((rc_pic_paras->target_bits)/BU_cnt);
		rc_pic_paras->remain_bits = rc_pic_paras->target_bits-BU_bit_stat;
#endif
	}
	else
	{
		rc_bu_paras->prev_target_bits = rc_bu_paras->target_bits;
#ifdef PIPELINE_BU_RC
		rc_pic_paras->pred_remain_bits = rc_pic_paras->remain_bits - pred_BU_bit_stat;
#else
		rc_pic_paras->remain_bits -= BU_bit_stat;
#endif
	}
#ifdef PIPELINE_BU_RC
	if (rc_bu_paras->prev_target_bits < pred_BU_bit_stat)
#else
	if (rc_bu_paras->prev_target_bits < BU_bit_stat) // the last basic unit overused its budget
#endif
	{
		overdue = TRUE;
	}

	rc_pic_paras->total_qp += prev_MB_qp;
#ifdef PIPELINE_BU_RC
	rc_bu_paras->target_bits = (int) floor((rc_pic_paras->pred_remain_bits)/(double)(BU_cnt-BU_nr));
#else
	rc_bu_paras->target_bits = (int) floor((rc_pic_paras->remain_bits)/(double)(BU_cnt-BU_nr));
#endif

	if (rc_bu_paras->BU_skip_MB>0)
	{
#ifdef PIPELINE_BU_RC
		pred_BU_bit_stat= (uint16)(((double)(pred_BU_bit_stat-rc_bu_paras->BU_skip_MB))/(BU_SIZE-rc_bu_paras->BU_skip_MB-PIPE_MB)*(BU_SIZE-PIPE_MB));
#else
		BU_bit_stat= (uint16)(((double)(BU_bit_stat-rc_bu_paras->BU_skip_MB))/(BU_SIZE-rc_bu_paras->BU_skip_MB)*BU_SIZE);
#endif
	}

	rc_bu_paras->BU_skip_MB = 0;

	if(overdue)
	{
		curr_MB_qp = Clip3(0, 40, prev_MB_qp+2);

		if (BU_nr == BU_cnt-1)
		{
			rc_pic_paras->total_qp += curr_MB_qp;
		}

		return curr_MB_qp; 
	}		

#ifdef PIPELINE_BU_RC
	ratio = (double)pred_BU_bit_stat/rc_bu_paras->target_bits;
#else
	ratio = (double)BU_bit_stat/rc_bu_paras->target_bits;
#endif

#ifdef PIPELINE_BU_RC
	if (rc_pic_paras->pred_remain_bits >= 0)
#else
	if (rc_pic_paras->remain_bits >= 0)
#endif
	{
		if (ratio >= 1.2)
		{
			curr_MB_qp = Clip3(0, 40, prev_MB_qp+2);
		}
		else if (ratio < 1.2 && ratio >= 1.04)
		{
			curr_MB_qp = Clip3(0, 40, prev_MB_qp+1);
		}
		else if (ratio < 1.04 && ratio >= 0.77)
		{
			curr_MB_qp = Clip3(0, 40, prev_MB_qp);
		}
		else if (ratio < 0.77 && ratio >= 0.67)
		{
			curr_MB_qp = Clip3(0, 40, prev_MB_qp-1);
		}
		else
		{
			curr_MB_qp = Clip3(0, 40, prev_MB_qp-2);
		}
	}
	else
	{
		curr_MB_qp = Clip3(0, 40, prev_MB_qp+2);
	}
	
	if (BU_nr == BU_cnt-1)
	{
		rc_pic_paras->total_qp += curr_MB_qp;
	}

	return curr_MB_qp;
}*/
#endif


int32 rc_init_slice(ENC_IMAGE_PARAMS_T *img_ptr, RC_PIC_PARAS *rc_pic_paras)
{
	uint32 rate_control_en = g_h264_enc_config->RateCtrlEnable;
	if(rate_control_en){
	int32 curr_qp;
#ifdef INT_RC
	int32 use_bits = ((rc_pic_paras->target_bits * ((img_ptr->sh.i_first_mb<<10) / img_ptr->frame_size_in_mbs))>>10);
	uint32 ratio = (uint32)(img_ptr->pic_sz<<10)/(++use_bits);
#else
	int32 use_bits = (int32)(rc_pic_paras->target_bits * (img_ptr->sh.i_first_mb/(double)img_ptr->frame_size_in_mbs));
	double ratio = ((double)img_ptr->pic_sz/use_bits)*1024;
#endif
	int32 overdue = (img_ptr->pic_sz > rc_pic_paras->target_bits);

#ifdef RC_DEBUG
	fprintf(rc_fp, "rc_init_slice\t: frame %d, rc_pic_paras->target_bits = %d\n", g_nFrame_enc, rc_pic_paras->target_bits);
	fprintf(rc_fp, "rc_init_slice\t: frame %d, use_bits = %d\n", g_nFrame_enc, use_bits);
	fprintf(rc_fp, "rc_init_slice\t: frame %d, img_ptr->pic_sz = %d\n", g_nFrame_enc, img_ptr->pic_sz);
#endif

	if(overdue)
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp+2); 
	}		
	else if (ratio >= 1126)
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp+2);
	}
	else if (ratio < 1126 && ratio >= 1065)
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp+1);
	}
	else if (ratio < 1065 && ratio >= 819)
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp);
	}
	else if (ratio < 819 && ratio >= 683)
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp-1);
	}
	else
	{
		curr_qp = Clip3(0, 40, rc_pic_paras->curr_pic_qp-2);
	}

	rc_pic_paras->slice_num++;
	rc_pic_paras->remain_bits -= BU_bit_stat;
	rc_pic_paras->total_qp += curr_qp;
	return curr_qp;
	}else{return rc_pic_paras->curr_pic_qp;}
}

void rc_update_pict(int32 bits, RC_GOP_PARAS *rc_gop_paras)// store the results of current picture and update the model
{
	uint32 target_rate = g_h264_enc_config->targetBitRate;
	uint32 rate_control_en = g_h264_enc_config->RateCtrlEnable;
	if(rate_control_en){
#ifdef INT_RC
	uint32 target_bitrate = (uint32)(target_rate-(target_rate>>8)-(target_rate>>10));
#else
	float  target_bitrate = (float)(target_rate*0.995);
#endif
//  uint16 frame_rate = INTRA_PERIOD;
//  uint16 GOP_len = INTRA_PERIOD;
  int32 frame_no_in_GOP = (int32)g_nFrame_enc % INTRA_PERIOD;
  int	 delta_bits;

  //update buffer fullness
#ifdef INT_RC
  if(frame_no_in_GOP==0)
	  delta_bits = bits - (int)((target_bitrate*I_P_RATIO)/(INTRA_PERIOD-1+I_P_RATIO));
  else
	  delta_bits = bits - (int)(target_bitrate/(INTRA_PERIOD-1+I_P_RATIO));
#else
  if(frame_no_in_GOP==0)
	  delta_bits = bits - (int)floor((target_bitrate*I_P_RATIO)/(INTRA_PERIOD-1+I_P_RATIO) + 0.5F);
  else
	  delta_bits = bits - (int)floor(target_bitrate/(INTRA_PERIOD-1+I_P_RATIO) + 0.5F);
#endif

  rc_gop_paras->rem_bits -= bits; 

//  if (rc_gop_paras->rem_bits < 0)
//  {
	  //bits = bits;
//	  rc_gop_paras->rem_bits = 0;
//  }

  rc_gop_paras->curr_buf_full += delta_bits;
#ifdef RC_DEBUG
  fprintf(rc_fp, "rc_update_pict\t: frame %d, rc_gop_paras->prev_pic_bits = %d\n", g_nFrame_enc, bits);
  fprintf(rc_fp, "rc_update_pict\t: frame %d, rc_gop_paras->curr_buf_full = %d\n", g_nFrame_enc, rc_gop_paras->curr_buf_full);
#endif

  rc_gop_paras->prev_pic_bits = bits;}
  return;
}
