/******************************************************************************
** File Name:    mp4enc_ratecontrol.c										  *
** Author:       Xiaowei Luo                                                 *
** DATE:         12/14/2006                                                  *
** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
** Description:                                                              *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 12/14/2006    Xiaowei Luo     Create.                                     *
*****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

#define RC_PRINTF	//PRINTF
#define DEFAULT_INITIAL_QUANTIZER 8

#define DEFAULT_BITRATE 900000	/* 900kbps */
#define DEFAULT_DELAY_FACTOR 16
#define DEFAULT_AVERAGING_PERIOD 100
#define DEFAULT_BUFFER 100

static int
get_initial_quant(unsigned int bitrate)
{

#if defined(DEFAULT_INITIAL_QUANTIZER)
	return (DEFAULT_INITIAL_QUANTIZER);
#else
	int i;

	const unsigned int bitrate_quant[31] = {
		UINT_MAX
	};

	for (i = 30; i >= 0; i--) {
		if (bitrate > bitrate_quant[i])
			continue;
	}

	return (i + 1);
#endif
}

 int rc_single_create(ENC_VOP_MODE_T * vop_mode_ptr, rc_single_t * rc, xvid_plg_data_t * data)
{
//	xvid_plugin_single_t *param = (xvid_plugin_single_t *) create->param;
//	rc_single_t *rc;
	int i;

	/*
	 * single needs to caclculate the average frame size. In order to do that,
	 * we really need valid fps
	 */
//	if (create->fincr == 0) {
//		return XVID_ERR_FAIL;
//	}

	/* Allocate context struct */
//	if ((rc = malloc(sizeof(rc_single_t))) == NULL)
//		return (XVID_ERR_MEMORY);

	/* Constants */
	for(i=0;i<3;i++)
	{
		data->min_quant[i] = 1;
		data->max_quant[i] = 31;
	}

	data->fbase = vop_mode_ptr->FrameRate;
	data->fincr = 1;
	rc->bytes_per_sec =	(vop_mode_ptr->targetBitRate> 0) ? vop_mode_ptr->targetBitRate / 8 : DEFAULT_BITRATE / 8;
	rc->target_framesize =(double) rc->bytes_per_sec / ((double) data->fbase / data->fincr);
	rc->reaction_delay_factor =DEFAULT_DELAY_FACTOR;
	rc->averaging_period = DEFAULT_AVERAGING_PERIOD;
	rc->buffer = DEFAULT_BUFFER;

	rc->time = 0;
	rc->total_size = 0;
	rc->rtn_quant = get_initial_quant(vop_mode_ptr->targetBitRate);

	if(vop_mode_ptr->short_video_header)
	{
		rc->rtn_quant = 12;
	}
	/* Reset quant error accumulators */
	for (i = 0; i < 31; i++)
		rc->quant_error[i] = 0.0;

	/* Last bunch of variables */
	rc->sequence_quality = 2.0 / (double) rc->rtn_quant;
	rc->avg_framesize = rc->target_framesize;

	rc->fq_error = 0;

//	/* Bind the RC */
//	*handle = rc;

//	/* A bit of debug info */
//	DPRINTF(XVID_DEBUG_RC, "bytes_per_sec: %i\n", rc->bytes_per_sec);
//	DPRINTF(XVID_DEBUG_RC, "frame rate   : %f\n", (double) create->fbase / create->fincr);
//	DPRINTF(XVID_DEBUG_RC, "target_framesize: %f\n", rc->target_framesize);

	return (0);
}


static int
rc_single_destroy()
{
//	free(rc);
//	return (0);
return 0;
}

#define XVID_TYPE_IVOP 1 /* intra frame*/
#define XVID_TYPE_PVOP 2 /* predicted frame*/
#define XVID_TYPE_BVOP 3 /* biderectionally encoded */


 int rc_single_before(rc_single_t * rc, xvid_plg_data_t * data)
{
//	 if (data->quant <= 0) {
//		if (data->zone && data->zone->mode == XVID_ZONE_QUANT) {
//			rc->fq_error += (double)data->zone->increment / (double)data->zone->base;
//			data->quant = (int)rc->fq_error;
//			rc->fq_error -= data->quant;
//		} else {
			int q = rc->rtn_quant;
			/* limit to min/max range 
			   we don't know frame type of the next frame, so we just use 
			   P-VOP's range... */
			if (q > data->max_quant[XVID_TYPE_PVOP-1])
				q = data->max_quant[XVID_TYPE_PVOP-1];
			else if (q < data->min_quant[XVID_TYPE_PVOP-1])
				q = data->min_quant[XVID_TYPE_PVOP-1];

		   	data->quant = q;
//		}
//	}
	return 0;
}


 int rc_single_after(rc_single_t * rc, xvid_plg_data_t * data)
{
	double deviation;
	int rtn_quant;
	double overflow;
	double averaging_period;
	double reaction_delay_factor;
	double quality_scale;
	double base_quality;
	double target_quality;

	/* Update internal values */
	rc->time += (double) data->fincr / data->fbase;
	rc->total_size += data->length;

	/* Compute the deviation from expected total size */
	deviation = (double)
		(rc->total_size - rc->bytes_per_sec * rc->time);

	averaging_period = (double) rc->averaging_period;

	/* calculate the sequence quality */
	rc->sequence_quality -= rc->sequence_quality / averaging_period;

	rc->sequence_quality +=
		2.0 / (double) data->quant / averaging_period;

	/* clamp the sequence quality to 10% to 100%
	 * to try to avoid using the highest
	 * and lowest quantizers 'too' much */
	if (rc->sequence_quality < 0.1)
		rc->sequence_quality = 0.1;
	else if (rc->sequence_quality > 1.0)
		rc->sequence_quality = 1.0;

	/* factor this frame's size into the average framesize
	 * but skip using ivops as they are usually very large
	 * and as such, usually disrupt quantizer distribution */
	if (data->type != XVID_TYPE_IVOP) {
		reaction_delay_factor = (double) rc->reaction_delay_factor;
		rc->avg_framesize -= rc->avg_framesize / reaction_delay_factor;
		rc->avg_framesize += data->length / reaction_delay_factor;
	}

	/* don't change the quantizer between pvops */
	if (data->type == XVID_TYPE_BVOP)
		return (0);

	/* calculate the quality_scale which will be used
	 * to drag the target quality up or down, depending
	 * on if avg_framesize is >= target_framesize */
	quality_scale =
		rc->target_framesize / rc->avg_framesize *
		rc->target_framesize / rc->avg_framesize;

	/* use the current sequence_quality as the
	 * base_quality which will be dragged around
	 * 
	 * 0.06452 = 6.452% quality (quant:31) */
	base_quality = rc->sequence_quality;
	if (quality_scale >= 1.0) {
		base_quality = 1.0 - (1.0 - base_quality) / quality_scale;
	} else {
		base_quality = 0.06452 + (base_quality - 0.06452) * quality_scale;
	}

	overflow = -((double) deviation / (double) rc->buffer);

	/* clamp overflow to 1 buffer unit to avoid very
	 * large bursts of bitrate following still scenes */
	if (overflow > rc->target_framesize)
		overflow = rc->target_framesize;
	else if (overflow < -rc->target_framesize)
		overflow = -rc->target_framesize;

	/* apply overflow / buffer to get the target_quality */
	target_quality =
		base_quality + (base_quality -
						0.06452) * overflow / rc->target_framesize;

	/* clamp the target_quality to quant 1-31
	 * 2.0 = 200% quality (quant:1) */
	if (target_quality > 2.0)
		target_quality = 2.0;
	else if (target_quality < 0.06452)
		target_quality = 0.06452;

	rtn_quant = (int) (2.0 / target_quality);

	/* accumulate quant <-> quality error and apply if >= 1.0 */
	if (rtn_quant > 0 && rtn_quant < 31) {
		rc->quant_error[rtn_quant - 1] += 2.0 / target_quality - rtn_quant;
		if (rc->quant_error[rtn_quant - 1] >= 1.0) {
			rc->quant_error[rtn_quant - 1] -= 1.0;
			rtn_quant++;
			rc->rtn_quant++;
		}
	}

	/* prevent rapid quantization change */
	if (rtn_quant > rc->rtn_quant + 1) {
		if (rtn_quant > rc->rtn_quant + 3)
			if (rtn_quant > rc->rtn_quant + 5)
				rtn_quant = rc->rtn_quant + 3;
			else
				rtn_quant = rc->rtn_quant + 2;
		else
			rtn_quant = rc->rtn_quant + 1;
	}
	else if (rtn_quant < rc->rtn_quant - 1) {
		if (rtn_quant < rc->rtn_quant - 3)
			if (rtn_quant < rc->rtn_quant - 5)
				rtn_quant = rc->rtn_quant - 3;
			else
				rtn_quant = rc->rtn_quant - 2;
		else
			rtn_quant = rc->rtn_quant - 1;
	}

	rc->rtn_quant = rtn_quant;

	return (0);
}

 void Update_lastQp(int8 *QP_last,int n)
{
	int i = n;

	for (i = 0;i< n-1;i++)
	{
		QP_last[i] = QP_last[i+1]; 
	}
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
