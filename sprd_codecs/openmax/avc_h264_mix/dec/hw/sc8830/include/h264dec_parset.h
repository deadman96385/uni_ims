/******************************************************************************
** File Name:      h264dec_parset.h                                          *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_PARSET_H_
#define _H264DEC_PARSET_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void H264Dec_use_parameter_set (H264DecObject *vo, int32 pps_id);
void H264Dec_InterpretSEIMessage (void);
void H264Dec_ProcessSPS (H264DecObject *vo);
void H264Dec_ProcessPPS (H264DecObject *vo);
void H264Dec_interpret_sps (H264DecObject *vo, DEC_SPS_T *sps_ptr);

#if _MVC_
void ProcessSubsetSPS (H264DecObject *vo);
void init_subset_sps_list(subset_seq_parameter_set_rbsp_t *subset_sps_list, int iSize);
MMDecRet mvc_vui_parameters_extension(H264DecObject *vo, MVCVUI_t *pMVCVUI);
MMDecRet seq_parameter_set_mvc_extension(H264DecObject *vo, subset_seq_parameter_set_rbsp_t *subset_sps);
void reset_subset_sps(subset_seq_parameter_set_rbsp_t *subset_sps);
void get_max_dec_frame_buf_size(H264DecObject *vo, DEC_SPS_T *sps);
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_PARSET_H_