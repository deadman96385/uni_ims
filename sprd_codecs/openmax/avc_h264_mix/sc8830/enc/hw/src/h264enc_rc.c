/******************************************************************************
** File Name:      h264enc_rc.c	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2011 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/16/2011    Shangwen Li     Create.                                     *
** 06/18/2013    Xiaowei Luo     Modify.                                     *
*****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
#include <math.h>
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define INT_RC
//#define RC_DEBUG

#ifdef RC_DEBUG
FILE * rc_fp;
#endif

int32 g_RC_Qstep[52] = 
	{
	640,  704,  832,  896, 1024, 1152, 1280, 1408, 1664, 1792, /* 0-9*/
	2048, 2304, 2560, 2816, 3328, 3584, 4096, 4608, 5120, 5632, /*10-19*/
	6656, 7168, 8192, 9216,10240,11264,13312,14336,16384,18432, /*20-29*/
	20480,22528,26624,28672,32768,36864,40960,45056,53248,57344, /*30-39*/
	65536,73728,81920,90112,106496,   0,    0,    0,    0,    0, /*40-49*/
	0,    0,                                                     /*50-51*/
	};

int32 rc_init_GOP (H264EncObject *vo, RC_GOP_PARAS *rc_gop_paras)
{
    uint32 target_rate = vo->g_h264_enc_config->targetBitRate;
    uint32 rate_control_en = vo->g_h264_enc_config->RateCtrlEnable;
	uint32 frame_rate = vo->g_h264_enc_config->FrameRate;
    int32  GOP_init_QP;								//return value, the initial QP of the GOP

    if(rate_control_en) {
		// update rc_gop_paras->rem_bits
		if(0 == (vo->g_nFrame_enc % frame_rate))
		{
#ifdef INT_RC
			uint32 target_bitrate = (uint32)(target_rate-(target_rate>>8)-(target_rate>>10));
#else
			float  target_bitrate = (float)(target_rate*0.995);
#endif
			rc_gop_paras->rem_bits += target_bitrate;

			rc_gop_paras->gop_num_per_sec = (frame_rate + rc_gop_paras->intra_period - 1) / rc_gop_paras->intra_period;
		}
		else
		{
			rc_gop_paras->gop_num_per_sec--;
		}

		// Compute InitialQp for each GOP
		if (0 == vo->g_nFrame_enc) // the first GOP
		{
			GOP_init_QP = vo->g_h264_enc_config->QP_IVOP;
		}
		else
		{
			int32 nGOPMONE = rc_gop_paras->intra_period - 1;
			int32 nCalculateBitRate;
			int32 i;

			if (0 != nGOPMONE)
			{
				rc_gop_paras->last_P_frame_bits = rc_gop_paras->last_P_frame_bits / nGOPMONE;
				rc_gop_paras->last_P_QP = rc_gop_paras->last_P_QP / nGOPMONE;
				nCalculateBitRate = (rc_gop_paras->last_P_frame_bits / g_RC_Qstep[rc_gop_paras->last_I_QP] * g_RC_Qstep[rc_gop_paras->last_P_QP]);
				// for cr#211038, avoid div 0
				if (0 == nCalculateBitRate)
				{
					nCalculateBitRate = (rc_gop_paras->last_P_frame_bits * g_RC_Qstep[rc_gop_paras->last_P_QP] / g_RC_Qstep[rc_gop_paras->last_I_QP]);
					if (0 == nCalculateBitRate)
					{
						nCalculateBitRate = 1;
					}
				}
				rc_gop_paras->I_P_ratio = (rc_gop_paras->last_I_frame_bits + nCalculateBitRate / 2) / nCalculateBitRate;
			}
			
			for(i = -6; i <= 6; i++)
			{
				GOP_init_QP = rc_gop_paras->last_I_QP + i;
				GOP_init_QP = Clip3(0, 40, GOP_init_QP);
				if(GOP_init_QP >=0 && GOP_init_QP < 40)
				{
					nCalculateBitRate = rc_gop_paras->last_I_frame_bits / g_RC_Qstep[GOP_init_QP] * g_RC_Qstep[rc_gop_paras->last_I_QP];
					nCalculateBitRate += rc_gop_paras->last_P_frame_bits / g_RC_Qstep[GOP_init_QP] * g_RC_Qstep[rc_gop_paras->last_P_QP] * nGOPMONE;
				}
				else
				{
					nCalculateBitRate = 0x7fffffff;
				}
	
				if( GOP_init_QP >= 40)
				{
					GOP_init_QP = 40;
					break;
				}
				else if(nCalculateBitRate < (rc_gop_paras->rem_bits / rc_gop_paras->gop_num_per_sec))
				{
					break;
				}
			}
		}
		
		rc_gop_paras->rem_frame_num = rc_gop_paras->gop_num_per_sec * (rc_gop_paras->I_P_ratio + rc_gop_paras->intra_period - 1) - rc_gop_paras->I_P_ratio + 1;

#ifdef RC_DEBUG
        if(vo->g_nFrame_enc==0)
            rc_fp = fopen("rc.log", "w");
        fprintf(rc_fp, "rc_init_GOP\t: frame %d, target_bitrate = %d\n", vo->g_nFrame_enc, target_bitrate);
        fprintf(rc_fp, "rc_init_GOP\t: frame %d, rc_gop_paras->rem_bits = %d\n", vo->g_nFrame_enc, rc_gop_paras->rem_bits);
#endif
    } 
	else
    {
        GOP_init_QP = vo->g_h264_enc_config->QP_IVOP;
    }

    return GOP_init_QP;
}

int32 rc_init_pict(H264EncObject *vo, RC_GOP_PARAS *rc_gop_paras, RC_PIC_PARAS *rc_pic_paras)//calculate the target bits for a picture, here the whole picture/frame is a slice
{
    uint32 rate_control_en = vo->g_h264_enc_config->RateCtrlEnable;

    if(rate_control_en) 
	{
        int32 frame_no_in_GOP = (int32)vo->g_nFrame_enc % rc_gop_paras->intra_period;
        int32 curr_qp;
        uint32 target_rate = vo->g_h264_enc_config->targetBitRate;
#ifdef INT_RC
        uint32 ratio;
#else
        double ratio;
#endif
        int32 prev_frame_qp;
#ifndef NO_BU_CHANGE
        //uint32 BU_cnt = (uint16)ceil(g_input->pic_width*g_input->pic_height/256/(double)BU_SIZE);
#endif

        if (0 == frame_no_in_GOP) // I frame, calculate the target bits for Basic unit level rate control
        {
			rc_pic_paras->target_bits = ((rc_gop_paras->rem_bits/rc_gop_paras->gop_num_per_sec)*rc_gop_paras->I_P_ratio)/(rc_gop_paras->intra_period-1+rc_gop_paras->I_P_ratio);
            rc_pic_paras->remain_bits = rc_pic_paras->target_bits;
            rc_pic_paras->slice_num = 1;
            rc_pic_paras->total_qp = rc_pic_paras->curr_pic_qp;
#ifdef RC_DEBUG
            fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_gop_paras->rem_bits = %d\n", vo->g_nFrame_enc, rc_gop_paras->rem_bits);
            fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->target_bits = %d\n", vo->g_nFrame_enc, rc_pic_paras->target_bits);
#endif
            return rc_pic_paras->curr_pic_qp;
        }
        else	// P Frame
        {
            rc_pic_paras->remain_bits -= vo->BU_bit_stat;
        }

#ifdef NO_BU_CHANGE
        //rc_pic_paras->total_qp += rc_pic_paras->curr_pic_qp;
        prev_frame_qp = Clip3(0, 40, rc_pic_paras->total_qp/rc_pic_paras->slice_num);
#else
		//prev_frame_qp = Clip3(0, 40, rc_pic_paras->total_qp/BU_cnt);
#endif
        //}

        //rc_gop_paras->total_qp += prev_frame_qp;
        rc_pic_paras->slice_num = 1;
        //rc_pic_paras->total_qp = 0;

        //calculate the target buffer level for the picture
		rc_pic_paras->delta_target_buf = rc_gop_paras->curr_buf_full/(rc_gop_paras->rem_frame_num-frame_no_in_GOP);
		
		//calculate the target bits for the picture
#ifdef INT_RC
		rc_pic_paras->target_bits = (int32)( rc_gop_paras->rem_bits/(rc_gop_paras->rem_frame_num-frame_no_in_GOP) );
		rc_pic_paras->target_bits = rc_pic_paras->target_bits - rc_pic_paras->delta_target_buf;
#else
		rc_pic_paras->target_bits = (int) floor( rc_gop_paras->rem_bits / (rc_gop_paras->intra_period-frame_no_in_GOP) + 0.5);
		temp  = MAX(0, (int) floor(target_bitrate/(rc_gop_paras->intra_period-1+rc_gop_paras->I_P_ratio) - 0.5*rc_pic_paras->delta_target_buf + 0.5));
		rc_pic_paras->target_bits = (int) floor(0.5 * rc_pic_paras->target_bits + 0.5 * temp + 0.5);
#endif

#ifdef RC_DEBUG
        fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->remain_bits = %d\n", vo->g_nFrame_enc, rc_pic_paras->remain_bits);
        fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_gop_paras->rem_bits = %d\n", vo->g_nFrame_enc, rc_gop_paras->rem_bits);
        fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->target_bits = %d\n", vo->g_nFrame_enc, rc_pic_paras->target_bits);
        fprintf(rc_fp, "rc_init_pict\t: frame %d, rc_pic_paras->delta_target_buf = %d\n", vo->g_nFrame_enc, rc_pic_paras->delta_target_buf);
#endif
        rc_pic_paras->remain_bits = rc_pic_paras->target_bits;

#ifdef INT_RC
		// for cr#211038, avoid div 0
		if (vo->g_nFrame_enc == 1 && rc_pic_paras->target_bits != 0)
		{
			ratio = ((uint32)(rc_gop_paras->prev_pic_bits<<10)/rc_pic_paras->target_bits);
			ratio = ratio/rc_gop_paras->I_P_ratio;
		}
		else if (rc_pic_paras->target_bits != 0) // for cr#211038, avoid div 0
		{
			if(frame_no_in_GOP == 1)
				ratio = ((uint32)(rc_gop_paras->last_P_frame_bits<<10)/rc_pic_paras->target_bits);
			else
				ratio = ((uint32)(rc_gop_paras->prev_pic_bits<<10)/rc_pic_paras->target_bits);
		}
#else
		ratio = ((double)rc_gop_paras->prev_pic_bits/rc_pic_paras->target_bits)*1024;
#endif

		if (rc_pic_paras->target_bits <= 0)//overdue)
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
    } 
	else 
	{
        return rc_pic_paras->curr_pic_qp;
    }
    //update QP for the picture
}

int32 rc_init_slice (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr, RC_PIC_PARAS *rc_pic_paras)
{
    uint32 rate_control_en = vo->g_h264_enc_config->RateCtrlEnable;

    if(rate_control_en) {
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
        fprintf(rc_fp, "rc_init_slice\t: frame %d, rc_pic_paras->target_bits = %d\n", vo->g_nFrame_enc, rc_pic_paras->target_bits);
        fprintf(rc_fp, "rc_init_slice\t: frame %d, use_bits = %d\n", vo->g_nFrame_enc, use_bits);
        fprintf(rc_fp, "rc_init_slice\t: frame %d, img_ptr->pic_sz = %d\n", vo->g_nFrame_enc, img_ptr->pic_sz);
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
        rc_pic_paras->remain_bits -= vo->BU_bit_stat;
        rc_pic_paras->total_qp += curr_qp;
        return curr_qp;
    } else {
        return rc_pic_paras->curr_pic_qp;
    }
}

void rc_update_pict(H264EncObject *vo, int32 bits, RC_GOP_PARAS *rc_gop_paras)// store the results of current picture and update the model
{
    uint32 target_rate = vo->g_h264_enc_config->targetBitRate;
    uint32 rate_control_en = vo->g_h264_enc_config->RateCtrlEnable;
	uint32 frame_rate = vo->g_h264_enc_config->FrameRate;

    if(rate_control_en) 
	{
#ifdef INT_RC
        uint32 target_bitrate = (uint32)(target_rate-(target_rate>>8)-(target_rate>>10));
#else
        float  target_bitrate = (float)(target_rate*0.995);
#endif

        int32 frame_no_in_GOP = (int32)vo->g_nFrame_enc % rc_gop_paras->intra_period;
        int	 delta_bits;

		//update buffer fullness
#ifdef INT_RC
		delta_bits = bits - (int)(target_bitrate/frame_rate);
#else
		if(frame_no_in_GOP==0)
			delta_bits = bits - (int)floor((target_bitrate*rc_gop_paras->I_P_ratio)/(rc_gop_paras->intra_period-1+rc_gop_paras->I_P_ratio) + 0.5F);
		else
			delta_bits = bits - (int)floor(target_bitrate/(rc_gop_paras->intra_period-1+rc_gop_paras->I_P_ratio) + 0.5F);
#endif


        rc_gop_paras->rem_bits -= bits;

		if (frame_no_in_GOP != 0)
		{
			if(frame_no_in_GOP == 1)
			{
				rc_gop_paras->last_P_frame_bits = bits;
				rc_gop_paras->last_P_QP = vo->g_enc_image_ptr->qp;
			}
			else
			{
				rc_gop_paras->last_P_frame_bits += bits;
				rc_gop_paras->last_P_QP += vo->g_enc_image_ptr->qp;
			}
		}
		else
		{
			rc_gop_paras->last_I_frame_bits = bits;
			rc_gop_paras->last_I_QP = vo->g_enc_image_ptr->qp;
		}

        rc_gop_paras->curr_buf_full += delta_bits;
#ifdef RC_DEBUG
        fprintf(rc_fp, "rc_update_pict\t: frame %d, rc_gop_paras->prev_pic_bits = %d\n", vo->g_nFrame_enc, bits);
        fprintf(rc_fp, "rc_update_pict\t: frame %d, rc_gop_paras->curr_buf_full = %d\n", vo->g_nFrame_enc, rc_gop_paras->curr_buf_full);
#endif

        rc_gop_paras->prev_pic_bits = bits;
    }

    return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

