/******************************************************************************
 ** File Name:    mp4dec_error_handle.c                                       *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/24/2009                                                  *
 ** Copyright:    2009 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 11/24/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

LOCAL int32 GetHorGrad (uint8 *curr_blk_pix, int32 frame_width, int32 blk_size, int32 is_uv)
{
    int32 i;
    int32 mad = 0;
    int32 left_pos = (is_uv ? (-2) : (-1));

    for (i = 0; i < blk_size; i++)
    {
        mad += ABS(curr_blk_pix[0] - curr_blk_pix[left_pos]);

        curr_blk_pix += frame_width;
    }

    return mad;
}

LOCAL int32 GetVertGrad (uint8 *curr_blk_pix, int32 frame_width, int32 blk_size, int32 is_uv)
{
    int32 i;
    uint8 *top_pix = curr_blk_pix - frame_width;
    int32 mad = 0;
    int32 step = (is_uv ? (2) : (1));

    for (i = 0; i < blk_size; i += step)
    {
        mad += ABS(curr_blk_pix[i] - top_pix[i]);
    }

    return mad;
}

//only for uv_interleaved format, xwluo@20100927
LOCAL int32 GetAvgGrad (DEC_VOP_MODE_T *vop_mode_ptr, int32 nei_avail[], int32 avail_num, int32 component_num)
{
    int32 frame_width = vop_mode_ptr->FrameExtendWidth;
    int32 curr_blk_addr = 0;
    uint8 *blk_ptr = PNULL;
    int32 blk_width = MB_SIZE;
    int32 shift = 4;
    int32 avg_grad = 0;
    int32 is_uv = 0;

    SCI_ASSERT (component_num < 3);

    if (component_num == 0) //luma
    {
        curr_blk_addr = vop_mode_ptr->mb_y * MB_SIZE * frame_width + vop_mode_ptr->mb_x * MB_SIZE;
        blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY + curr_blk_addr;
        blk_width = MB_SIZE;
        shift = 4;
    } else if (component_num == 1) //chroma U
    {
        curr_blk_addr = vop_mode_ptr->mb_y * BLOCK_SIZE * frame_width/2 + vop_mode_ptr->mb_x * BLOCK_SIZE;
        blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV + curr_blk_addr;
        blk_width = BLOCK_SIZE;
        shift = 3;
        is_uv = 1;
    } else //chroma V
    {
        curr_blk_addr = vop_mode_ptr->mb_y * BLOCK_SIZE * frame_width/2 + vop_mode_ptr->mb_x * BLOCK_SIZE;
        blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV + curr_blk_addr;
        blk_width = BLOCK_SIZE;
        shift = 3;
        is_uv = 1;
    }

    if (nei_avail[0]) //left
    {
        avg_grad += GetHorGrad(blk_ptr, frame_width, blk_width, is_uv);
    }

    if (nei_avail[1]) //right
    {
        avg_grad += GetHorGrad(blk_ptr + MB_SIZE, frame_width, blk_width, is_uv);
    }

    if (nei_avail[2]) //top
    {
        avg_grad += GetVertGrad(blk_ptr, frame_width, MB_SIZE, is_uv);
    }

    if (nei_avail[3]) //bottom
    {
        avg_grad += GetVertGrad(blk_ptr + blk_width*frame_width, frame_width, MB_SIZE, is_uv);
    }

    if (avail_num)
    {
        avg_grad /= avail_num;
        avg_grad >>= shift;
    }

    return avg_grad;
}

#define THRES_MOTION(nei_mb_mode_ptr, mv_amp, intra_num, inter4v_num)\
{\
	if (nei_mb_mode_ptr->dctMd < INTER)\
	{\
		intra_num++;\
	}else\
	{\
		mv_amp += ABS (nei_mb_mode_ptr->mv[0].x);\
		mv_amp += ABS (nei_mb_mode_ptr->mv[0].y);\
		if (INTER4V == nei_mb_mode_ptr->dctMd)\
		{\
			inter4v_num++;\
		}\
	}\
}

LOCAL int32 GetThresMotion (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 nei_avail[])
{
    int32 intra_num = 0, inter4v_num = 0;
    int32 mv_amp = 0;
    int32 thres_mv;
    int32 thres_motion;

    if (vop_mode_ptr->VopPredType != IVOP)
    {
        if (nei_avail[0]) //left
        {
            THRES_MOTION ((mb_mode_ptr - 1), mv_amp, intra_num, inter4v_num);
        }

//		if (nei_avail[1]) //right
//		{
//			THRES_MOTION ((mb_mode_ptr + 1), mv_amp, intra_num, inter4v_num);
//		}

        if (nei_avail[2]) //top
        {
            THRES_MOTION ((mb_mode_ptr - vop_mode_ptr->MBNumX), mv_amp, intra_num, inter4v_num);
        }

//		if (nei_avail[3]) //bottom
//		{
//			THRES_MOTION ((mb_mode_ptr + vop_mode_ptr->MBNumX), mv_amp, intra_num, inter4v_num);
//		}

        thres_mv = (mv_amp > 10) ? 10 : mv_amp;

        if (intra_num)
        {
            thres_motion = 25;
        } else if (inter4v_num)
        {
            thres_motion = 10 + thres_mv;
        } else
        {
            thres_motion = thres_mv;
        }
    } else
    {
        thres_motion = 0;
    }

    return thres_motion;
}

LOCAL BOOLEAN DetectErrBMA (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
    int32 mb_x, mb_y;
    int32 avg_grad = 0;
    int32 nei_avail[4], avail_num = 0;
    int32 thres_qp = 0, thres_motion = 0, threshold = 0;
    int32 is_error = FALSE;
// 	int32 frame_width = vop_mode_ptr->FrameWidth;
// 	int32 curr_mb_addr = 0;
    int32 curr_mb_num = vop_mode_ptr->mbnumDec;
// 	uint8 *mb_y_ptr = PNULL;

    mb_x = vop_mode_ptr->mb_x;
    mb_y = vop_mode_ptr->mb_y;

    //judge neighbor's availability
    //left
    nei_avail[0] = ((mb_x > 0) && (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num -1] >= DECODED_IN_ERR_PKT));
    if (nei_avail[0])
    {
        avail_num++;
    }

    //right
    nei_avail[1] = ((mb_x < (vop_mode_ptr->MBNumX - 1)) &&
                    (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num+1] >= DECODED_IN_ERR_PKT));
    if (nei_avail[1])
    {
        avail_num++;
    }

    //top
    nei_avail[2] = ((mb_y > 0) && (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - vop_mode_ptr->MBNumX] >= DECODED_IN_ERR_PKT));
    if (nei_avail[2])
    {
        avail_num++;
    }

    //bottom
    nei_avail[3] = ((mb_y < (vop_mode_ptr->MBNumY-1))
                    && (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num + vop_mode_ptr->MBNumX]) >= DECODED_IN_ERR_PKT);
    if (nei_avail[3])
    {
        avail_num++;
    }

    //get average gradient, luma
    avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 0);

    //get motion threshold
    if (PVOP == vop_mode_ptr->pre_vop_type)
    {
        thres_motion = GetThresMotion (vop_mode_ptr, mb_mode_ptr, nei_avail);
    }

    //get qp threshold
    thres_qp = (vop_mode_ptr->StepSize/* << 1*/);

    threshold = thres_qp + thres_motion;

    //compare
    if (avg_grad > threshold)
    {
        is_error = TRUE;
    } else {
        threshold >>= 1;

        //get average gradient, chroma U
        avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 1);

        if (avg_grad > threshold)
        {
            is_error = TRUE;
        } else
        {
            //get average gradient, chroma V
            avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 2);

            if (avg_grad > threshold)
            {
                is_error = TRUE;
            }
        }

    }

    return is_error;
}


LOCAL int32 GetHorMAD_BMA (uint8 *left_pix_ptr, uint8 *ref_frame_y_ptr, int32 y_ref, int32 x_ref, int32 frame_width, int32 frame_height, int32 blk_size)
{
    int32 i;
    int32 dx, dy;
    int32 x_ref0, x_ref1;
    int32 y_ref0, y_ref1;
    int32 y_cor;
    uint8 pix_rec, pix_ref;
    int32 mad_bma = 0;

    dx = x_ref & 1;
    dy = y_ref & 1;

    x_ref >>= 1;
    y_ref >>= 1;

    //clip x cordination, two parameter for half pixel in horizontal direction
    x_ref0 = IClip(0, frame_width-1, x_ref);
    x_ref1 = IClip(0, frame_width-1, x_ref+1);

    for (i = 0; i < blk_size; i++)
    {
        y_cor = y_ref + i;

        y_ref0 = IClip(0, frame_height-1, y_cor);

        if (dy == 0)
        {
            if (dx == 0) //
            {
                pix_ref = ref_frame_y_ptr[y_ref0*frame_width+x_ref0];
            } else
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref0*frame_width+x_ref1] + 1) >> 1);
            }
        } else
        {
            y_ref1 = IClip(0, frame_height-1, y_cor+1);

            if (dx == 0) //
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref1*frame_width+x_ref0] + 1) >>1);
            } else
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref0*frame_width+x_ref1] +
                            ref_frame_y_ptr[y_ref1*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref1*frame_width+x_ref1] + 2) >> 2);
            }
        }

        pix_rec = left_pix_ptr[0];

        mad_bma += ABS(pix_rec - pix_ref);

        left_pix_ptr += frame_width;
    }

    return mad_bma;
}

LOCAL int32 GetVertMAD_BMA (uint8 *curr_pix_ptr, uint8 *ref_frame_y_ptr, int32 y_ref, int32 x_ref, int32 frame_width, int32 frame_height, int32 blk_size)
{
    int32 i;
    int32 dx, dy;
    int32 x_cor;
    int32 x_ref0, x_ref1;
    int32 y_ref0, y_ref1;
    uint8 pix_rec, pix_ref;
    int32 mad_bma = 0;
// 	int32 x_max = frame_width - 1;

    dx = x_ref & 1;
    dy = y_ref & 1;

    x_ref >>= 1;
    y_ref >>= 1;

    //clip y cordination, two parameter for half pixel in vertical direction
    y_ref0 = IClip(0, frame_height -1, y_ref);
    y_ref1 = IClip(0, frame_height -1, y_ref+1);

    for (i = 0; i < blk_size; i++)
    {
        x_cor = x_ref + i;

        x_ref0 = IClip(0, frame_width-1, x_cor);

        if (dx == 0)
        {
            if (dy == 0)
            {
                pix_ref = ref_frame_y_ptr[y_ref0*frame_width + x_ref0];
            } else
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width + x_ref0] +
                            ref_frame_y_ptr[y_ref1*frame_width + x_ref0] + 1) >> 1);
            }
        } else
        {
            x_ref1 = IClip(0, frame_width-1, x_cor + 1);

            if (dy == 0)
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width + x_ref0] +
                            ref_frame_y_ptr[y_ref0*frame_width + x_ref1] + 1) >> 1);
            } else
            {
                pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref0*frame_width+x_ref1] +
                            ref_frame_y_ptr[y_ref1*frame_width+x_ref0] +
                            ref_frame_y_ptr[y_ref1*frame_width+x_ref1] + 2) >> 2);
            }
        }

        pix_rec = curr_pix_ptr[i];

        mad_bma += ABS(pix_rec - pix_ref);
    }

    return mad_bma;
}


LOCAL int32 GetMAD_OBMA (DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *mv_ptr, int32 nei_avail, int32 blk_size, int32 blk_idx)
{
    int32 mb_x = vop_mode_ptr->mb_x;
    int32 mb_y = vop_mode_ptr->mb_y;
    int32 mv_x = mv_ptr->x;	//use integer pixels
    int32 mv_y = mv_ptr->y;
    int32 mad_bma = 0;
    int32 x_ref, y_ref;
    int32 blk_x_offet, blk_y_offset;
    uint8 *curr_blk_ptr;
    int32 frame_width = vop_mode_ptr->FrameExtendWidth;
    int32 frame_height = vop_mode_ptr->FrameExtendHeight;
    uint8 *ref_frame_y_ptr  = vop_mode_ptr->YUVRefFrame0[0] + vop_mode_ptr->iStartInFrameY;


    blk_x_offet  = (blk_idx & 1 ) * BLOCK_SIZE;
    blk_y_offset = (blk_idx >> 1) * BLOCK_SIZE;

    x_ref = mb_x * MB_SIZE * 2 + blk_x_offet * 2 + mv_x;  //integer resolution
    y_ref = mb_y * MB_SIZE * 2 + blk_y_offset * 2 + mv_y;

    curr_blk_ptr = vop_mode_ptr->mb_cache_ptr->mb_addr[0] + blk_y_offset * frame_width + blk_x_offet;

    //left mad (obma)
    if (nei_avail & 1)
    {
        uint8 *left_pix_ptr = curr_blk_ptr - 1;

        mad_bma += GetHorMAD_BMA (left_pix_ptr, ref_frame_y_ptr, y_ref, x_ref - 2, frame_width, frame_height, blk_size);
    }

    //top mad (obma)
    if (nei_avail & 2)
    {
        uint8 *top_pix_ptr = curr_blk_ptr - frame_width;

        mad_bma += GetVertMAD_BMA (top_pix_ptr, ref_frame_y_ptr, y_ref - 2, x_ref, frame_width, frame_height, blk_size);
    }

    //bottom mad (obma)
    if (nei_avail & 8)
    {
        uint8 *bot_pix_ptr = curr_blk_ptr + frame_width * blk_size;

        mad_bma += GetVertMAD_BMA (bot_pix_ptr, ref_frame_y_ptr, y_ref + (blk_size<<1), x_ref, frame_width, frame_height, blk_size);
    }

    return mad_bma;
}

/*only for one directional interpolation MB*/
LOCAL void Mp4Dec_StartMcaOneDir (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv)
{
    int16 mv_x, mv_y;
    int16 mvc_x, mvc_y;
    MOTION_VECTOR_T mv;
    int32 pos_x = vop_mode_ptr->mb_x;
    int32 pos_y = vop_mode_ptr->mb_y;
    int32 total_mb_num_x = vop_mode_ptr->MBNumX;
    int32 mca_type = mb_cache_ptr->mca_type;
    uint8 *pDstFrameY, *pDstFrameU, *pDstFrameV;
    int32 dst_width;

    //set mv range
    vop_mode_ptr->mv_x_max = ((total_mb_num_x - pos_x) << 5)-1;
    vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
    vop_mode_ptr->mv_y_max = ((vop_mode_ptr->MBNumY - pos_y) << 5)-1;
    vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;

    //Directly output to memory.
    pDstFrameY = mb_cache_ptr->mb_addr[0];
    pDstFrameU = mb_cache_ptr->mb_addr[1];
    pDstFrameV = mb_cache_ptr->mb_addr[2];
    dst_width = vop_mode_ptr->FrameExtendWidth;

    /*configure Y block infor*/
    if (mca_type != MCA_BACKWARD_4V)
    {
        mv = pMv[0];
        CLIP_MV(vop_mode_ptr, mv.x, mv.y);

        /*compute mv of uv*/
        mv_x = mv.x;
        mv_y = mv.y;
        mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
        mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];

        Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  pMv, pDstFrameY, dst_width,1);
    } else
    {
        int iBlk;
        int dmvcx = 0, dmvcy = 0;

        for (iBlk = 4; iBlk > 0; iBlk--)
        {
            mv = pMv[0];
            CLIP_MV(vop_mode_ptr, mv.x, mv.y);

            dmvcx += mv.x;
            dmvcy += mv.y;
            pMv++;
        }

        mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
        mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];

        Mp4Dec_motionCompY_fourMV(vop_mode_ptr,  pMv,pDstFrameY,dst_width, 1);
    }

    Mp4Dec_motionCompensationUV(vop_mode_ptr,  mvc_x, mvc_y, pDstFrameU, pDstFrameV, dst_width/2,1);
}

PUBLIC void Mp4Dec_EC_IVOP (DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 mb_x, mb_y;
    int32 curr_mb_dec_status = NOT_DECODED;
    int32 curr_mb_num = 0;
    int32 is_error = FALSE;
    int32 prev_pvop_valid = (PVOP == vop_mode_ptr->pre_vop_type) ? TRUE : FALSE;
// 	int32 mc_check_result = FALSE;
    int32 total_mb_num_y = vop_mode_ptr->MBNumY;
    int32 total_mb_num_x = vop_mode_ptr->MBNumX;
    DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
    DEC_MB_MODE_T *co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev;
    uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
    DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

    ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
    ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
    ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

    for (mb_y = 0; mb_y < total_mb_num_y; mb_y++)
    {
        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
        mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
        mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
        vop_mode_ptr->mb_y = mb_y;

        for (mb_x = 0; mb_x < total_mb_num_x; mb_x++)
        {
            vop_mode_ptr->mb_x = mb_x;
            vop_mode_ptr->mbnumDec = curr_mb_num;
            curr_mb_dec_status = vop_mode_ptr->mbdec_stat_ptr[curr_mb_num];

            if (curr_mb_dec_status < DECODED_NOT_IN_ERR_PKT) //not coded or coded but in error packet
            {
                mb_mode_ptr->bIntra = FALSE;
                mb_mode_ptr->CBP	= 0;

                if (NOT_DECODED == curr_mb_dec_status)
                {
                    is_error = TRUE;
                } else
                {
                    is_error = DetectErrBMA (vop_mode_ptr, mb_mode_ptr);

                    if (is_error)
                    {
                        vop_mode_ptr->err_MB_num++;
                    }
                }

                if (is_error) //then do error concealment
                {

                    if (prev_pvop_valid) //try temporal interpolation
                    {
                        co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev + curr_mb_num;

                        Mp4Dec_StartMcaOneDir (vop_mode_ptr, mb_cache_ptr,co_mb_mode_ptr->mv);
                    }

                    vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = ERR_CONCEALED;
                } else
                {
                    vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
                }
            } else
            {
                vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
            }

            mb_cache_ptr->mb_addr[0] += MB_SIZE;
            mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
            mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
            mb_mode_ptr++;
            curr_mb_num++;
        }
        ppxlcRecGobY += (vop_mode_ptr->FrameExtendWidth <<4);//* MB_SIZE;
        ppxlcRecGobU += (vop_mode_ptr->FrameExtendWidth <<2);//* MB_SIZE / 4;
        ppxlcRecGobV += (vop_mode_ptr->FrameExtendWidth <<2);//MB_SIZE / 4;

    }
}

/*****************************************************************************
  neighbor	MB
		|  B1  |  C2
  ------|------|------
	    |  cur |
	A0	|  MB  |
  ------|------|------
        |   D3 |

  neighbor MB: left, top, top right and bottom MB
******************************************************************************/
LOCAL int32 GetNeiAvail (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 *nei_intra_num, int32 *nei_avail)
{
    int32 mb_x = vop_mode_ptr->mb_x;
    int32 mb_y = vop_mode_ptr->mb_y;
    int32 mb_num_x = vop_mode_ptr->MBNumX;
    int32 curr_mb_num = mb_y * mb_num_x + mb_x;
    int32 avail_num = 0;

    if (mb_y)
    {
        DEC_MB_MODE_T *top_mb_mode_ptr = mb_mode_ptr - mb_num_x;
        int32 top_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - mb_num_x] != NOT_DECODED);
        avail_num += top_avail;
        *nei_avail |= (top_avail << 1);

        if ((top_avail) && (top_mb_mode_ptr->bIntra))
        {
            *nei_intra_num++;
        }

        if (mb_x < mb_num_x -1)
        {
// 			DEC_MB_MODE_T *tr_mb_mode_ptr = top_mb_mode_ptr + 1; //top right
            int32 tr_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - mb_num_x + 1] != NOT_DECODED);
            avail_num += tr_avail;
            *nei_avail |= (tr_avail << 2);
        }
    }

    if (mb_x > 0)
    {
// 		DEC_MB_MODE_T *left_mb_mode_ptr = mb_mode_ptr - 1; //left
        int32 left_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - 1] != NOT_DECODED);

        avail_num += left_avail;
        *nei_avail |= (left_avail << 0);
    }

    if (mb_y < (vop_mode_ptr->MBNumY -1))
    {
        DEC_MB_MODE_T *bot_mb_mode_ptr = mb_mode_ptr + mb_num_x; //bottom
        int32 bot_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num + mb_num_x] != NOT_DECODED);

        avail_num += bot_avail;
        *nei_avail |= (bot_avail << 3);

        if ((bot_avail) && (bot_mb_mode_ptr->bIntra))
        {
            *nei_intra_num++;
        }
    }

    return avail_num;
}

//only for uv-interleaved format
LOCAL void BI_SpatialConceal(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T  * mb_cache_ptr, int32 nei_avail)
{
    int32 mb_x = vop_mode_ptr->mb_x;
    int32 mb_y = vop_mode_ptr->mb_y;
    int32 top_avail = ((nei_avail>>1) & 0x1);
    int32 bot_avail = ((nei_avail>>1) & 0x3);
    int32 frame_width = vop_mode_ptr->FrameExtendWidth;
    uint8 *curr_pix_ptr_y = mb_cache_ptr->mb_addr[0];
    uint8 *curr_pix_ptr_u = mb_cache_ptr->mb_addr[1];
    uint8 *curr_pix_ptr_v = mb_cache_ptr->mb_addr[2];
    uint8 *top_pix_ptr_y = curr_pix_ptr_y - frame_width;
    uint8 *top_pix_ptr_u = curr_pix_ptr_u - frame_width/2;
    uint8 *top_pix_ptr_v = curr_pix_ptr_v - frame_width/2;
    uint8 *bot_pix_ptr_y = curr_pix_ptr_y + MB_SIZE * frame_width;
    uint8 *bot_pix_ptr_u = curr_pix_ptr_u + 4 * frame_width;
    uint8 *bot_pix_ptr_v = curr_pix_ptr_v + 4 * frame_width;
    int32 i, j;

    if (!top_avail)
    {
        top_pix_ptr_y = bot_pix_ptr_y;
        top_pix_ptr_u = bot_pix_ptr_u;
        top_pix_ptr_v = bot_pix_ptr_v;
    }

    if (!bot_avail)
    {
        bot_pix_ptr_y = top_pix_ptr_y;
        bot_pix_ptr_u = top_pix_ptr_u;
        bot_pix_ptr_v = top_pix_ptr_v;
    }

    //y
    for (i = 0; i < MB_SIZE; i++)
    {
        uint8 top_y, bot_y;

        for (j = 0; j < MB_SIZE; j++)
        {
            top_y = top_pix_ptr_y[j];
            bot_y = bot_pix_ptr_y[j];

            curr_pix_ptr_y[j] = ((MB_SIZE - i) * top_y + i * bot_y) >> 4;
        }

        curr_pix_ptr_y += frame_width;
    }

    //uv
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        uint8 top_u, bot_u;
        uint8 top_v, bot_v;

        for (j = 0; j < BLOCK_SIZE; j++)
        {
            top_u = top_pix_ptr_u[j];
            bot_u = bot_pix_ptr_u[j];
            top_v = top_pix_ptr_v[j];
            bot_v = bot_pix_ptr_v[j];

            curr_pix_ptr_u[j] = ((BLOCK_SIZE - i) * top_u + i * bot_u) >> 3;
            curr_pix_ptr_v[j] = ((BLOCK_SIZE - i) * top_v + i * bot_v) >> 3;
        }

        curr_pix_ptr_u += (frame_width/2);
    }
}

LOCAL void TemporalConceal (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *curr_mb_mode_ptr, int32 nei_avail)
{
    MOTION_VECTOR_T zero_mv = {0, 0};
    MOTION_VECTOR_T mv_cand = zero_mv;
    int32 min_mad_obma = GetMAD_OBMA (vop_mode_ptr, &zero_mv, nei_avail, MB_SIZE, 0);
    int32 left_avail = nei_avail & 0x1;
    int32 top_avail  = ((nei_avail>>1) & 0x1);
    int32 tr_avail   = ((nei_avail>>2) & 0x1);
    int32 bot_avail  = ((nei_avail>>3) & 0x1);
    DEC_MB_MODE_T *left_mb_mode_ptr = curr_mb_mode_ptr -1;
    DEC_MB_MODE_T *top_mb_mode_ptr = curr_mb_mode_ptr - vop_mode_ptr->MBNumX;
    DEC_MB_MODE_T *tr_mb_mode_ptr = top_mb_mode_ptr + 1;
    DEC_MB_MODE_T *bot_mb_mode_ptr = curr_mb_mode_ptr + vop_mode_ptr->MBNumX;
    DEC_MB_MODE_T *co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev+vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX+vop_mode_ptr->mb_x;
    int32 mad_obma = 0;

    //left mad
    if (left_avail && !(left_mb_mode_ptr->bIntra))
    {
        mad_obma = GetMAD_OBMA (vop_mode_ptr, &left_mb_mode_ptr->mv[1], nei_avail, MB_SIZE, 0);

        if (mad_obma < min_mad_obma)
        {
            min_mad_obma = mad_obma;
            mv_cand = left_mb_mode_ptr->mv[1];
        }
    }

    //top mad
    if (top_avail && !top_mb_mode_ptr->bIntra)
    {
        mad_obma = GetMAD_OBMA (vop_mode_ptr, &top_mb_mode_ptr->mv[2], nei_avail, MB_SIZE, 0);

        if (mad_obma < min_mad_obma)
        {
            min_mad_obma = mad_obma;
            mv_cand = top_mb_mode_ptr->mv[2];
        }
    }

    //bot mad
    if (bot_avail && !bot_mb_mode_ptr->bIntra)
    {
        mad_obma = GetMAD_OBMA (vop_mode_ptr, &bot_mb_mode_ptr->mv[0], nei_avail, MB_SIZE, 0);

        if (mad_obma < min_mad_obma)
        {
            min_mad_obma = mad_obma;
            mv_cand = bot_mb_mode_ptr->mv[0];
        }
    }

    //top right mad
    if (tr_avail && !tr_mb_mode_ptr->bIntra)
    {
        mad_obma = GetMAD_OBMA (vop_mode_ptr, &tr_mb_mode_ptr->mv[2], nei_avail, MB_SIZE, 0);

        if (mad_obma < min_mad_obma)
        {
            min_mad_obma = mad_obma;
            mv_cand = tr_mb_mode_ptr->mv[2];
        }
    }

    if (!co_mb_mode_ptr->bIntra)
    {
        mad_obma = GetMAD_OBMA (vop_mode_ptr, &co_mb_mode_ptr->mv[0], nei_avail, MB_SIZE, 0);

        if (mad_obma < min_mad_obma)
        {
            min_mad_obma = mad_obma;
            mv_cand = co_mb_mode_ptr->mv[0];
        }
    }

    Mp4Dec_StartMcaOneDir (vop_mode_ptr, vop_mode_ptr->mb_cache_ptr,&mv_cand);

    return;
}

PUBLIC void Mp4Dec_EC_PVOP (DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 mb_x, mb_y;
    int32 curr_mb_dec_status = NOT_DECODED;
    int32 curr_mb_num = 0;
    int32 is_error = FALSE;
    int32 total_mb_num_x = vop_mode_ptr->MBNumX;
    int32 need_load_data = TRUE;
    int32 bUseRecFrmAsDispFrm = FALSE;
    DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
    uint8 *ppxlcRecGobY, *ppxlcRecGobU, *ppxlcRecGobV;
    DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

    //ref frame
    vop_mode_ptr->YUVRefFrame0[0] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[0];
    vop_mode_ptr->YUVRefFrame0[1] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[1];
    vop_mode_ptr->YUVRefFrame0[2] = vop_mode_ptr->pBckRefFrame->pDecFrame->imgYUV[2];

    ppxlcRecGobY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
    ppxlcRecGobU = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1] + vop_mode_ptr->iStartInFrameUV;
    ppxlcRecGobV = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[2] + vop_mode_ptr->iStartInFrameUV;

    vop_mode_ptr->mb_cache_ptr->mca_type = MCA_BACKWARD;

    for (mb_y = 0; mb_y < vop_mode_ptr->MBNumY; mb_y++)
    {
        mb_cache_ptr->mb_addr[0] = ppxlcRecGobY;
        mb_cache_ptr->mb_addr[1] = ppxlcRecGobU;
        mb_cache_ptr->mb_addr[2] = ppxlcRecGobV;
        vop_mode_ptr->mb_y = mb_y;

        for (mb_x = 0; mb_x < vop_mode_ptr->MBNumX; mb_x++)
        {
            int32 nei_intra_num = 0;
            int32 nei_avail = 0;
            int32 avail_num = 0;

            vop_mode_ptr->mb_x = mb_x;
            vop_mode_ptr->mbnumDec = curr_mb_num;
            curr_mb_dec_status = vop_mode_ptr->mbdec_stat_ptr[curr_mb_num];

            if (curr_mb_dec_status < DECODED_NOT_IN_ERR_PKT) //not coded or coded but in error packet
            {
                mb_mode_ptr->bIntra = FALSE;
                mb_mode_ptr->CBP	= 0;

                if (NOT_DECODED == curr_mb_dec_status)
                {
                    is_error = TRUE;
                } else
                {
                    is_error = DetectErrBMA (vop_mode_ptr, mb_mode_ptr);

                    if (is_error)
                    {
                        vop_mode_ptr->err_MB_num++;
                    }
                }

                if (is_error)
                {
                    avail_num = GetNeiAvail (vop_mode_ptr, mb_mode_ptr, &nei_intra_num, &nei_avail);

                    if (nei_intra_num > 1)
                    {
                        BI_SpatialConceal (vop_mode_ptr, mb_mode_ptr, mb_cache_ptr, nei_avail);
                    } else
                    {
                        if(vop_mode_ptr->pBckRefFrame->pDecFrame != NULL)
                        {
                            TemporalConceal (vop_mode_ptr, mb_mode_ptr, nei_avail);
                        }
                    }

                    vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = ERR_CONCEALED;
                } else
                {
                    vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
                }
            } else
            {
                vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
            }
            mb_cache_ptr->mb_addr[0] += MB_SIZE;
            mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
            mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;
            mb_mode_ptr++;
            curr_mb_num++;
        }
        ppxlcRecGobY += (vop_mode_ptr->FrameExtendWidth <<4);//* MB_SIZE;
        ppxlcRecGobU += (vop_mode_ptr->FrameExtendWidth <<2);//* MB_SIZE / 4;
        ppxlcRecGobV += (vop_mode_ptr->FrameExtendWidth <<2);//MB_SIZE / 4;
    }

}

//update error information when resync point is founded
PUBLIC void Mp4Dec_UpdateErrInfo (DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 i;
    int32 bitstream_pos;
    ERR_POS_T *err_pos_ptr_tmp;

    bitstream_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;
    err_pos_ptr_tmp = vop_mode_ptr->err_pos_ptr;

    for (i = 0; i < vop_mode_ptr->err_left; i++)
    {
        if (bitstream_pos <= err_pos_ptr_tmp->end_pos)
        {
            break;
        } else
        {
            err_pos_ptr_tmp++;
        }
    }

    vop_mode_ptr->err_left	  -= i;
    vop_mode_ptr->err_pos_ptr += i;
}

//MB from dbk indicated to synchronization are not decoded, use zero-mv inter MBs to fill them.
//use MCA and DBK hardware module.
PUBLIC MMDecRet Mp4Dec_EC_FillMBGap (DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 ref_frm_valid = (vop_mode_ptr->pBckRefFrame->pDecFrame != PNULL) ? TRUE : FALSE;

    if (ref_frm_valid)
    {
        int32 error_mb;
        DEC_MB_MODE_T *mb_mode_ptr,*co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev;
        int32 total_mb_num = vop_mode_ptr->MBNum;
        int32 pos_x, pos_y;
#if 0	//disabled in CQM mode
        uint32 mb_pos = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, "readout current mb index in dbk module");

        pos_x = (int32)(mb_pos&0xffff)+1;
        pos_y = (int32)((mb_pos>>16)&0xffff);
#else
        pos_x = vop_mode_ptr->mb_x;
        pos_y = vop_mode_ptr->mb_y;
#endif

        if (pos_x == vop_mode_ptr->MBNumX)
        {
            pos_x = 0;
            pos_y++;
        }

        error_mb = pos_y * vop_mode_ptr->MBNumX + pos_x;
        vop_mode_ptr->mbdec_stat_ptr[error_mb-1] = NOT_DECODED;//previous MB maybe error, set "NOT DECODED" for EC later
        for(; (error_mb < vop_mode_ptr->mbnumDec) && (error_mb < total_mb_num); error_mb++)
        {
            mb_mode_ptr = vop_mode_ptr->pMbMode + error_mb;
            mb_mode_ptr->bIntra = FALSE; //let this mb not available in mb pred
            mb_mode_ptr->CBP	= 0;

#if 0	//disabled in CQM mode
            co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev + error_mb;
            Mp4Dec_VspMBInit (pos_x, pos_y);
            Mp4Dec_MBC_DBK_Cmd (vop_mode_ptr, mb_mode_ptr);
            Mp4Dec_StartMcaOneDir (vop_mode_ptr, vop_mode_ptr->mb_cache_ptr, co_mb_mode_ptr->mv);
            Mp4Dec_CheckMBCStatus (vop_mode_ptr);
            if (vop_mode_ptr->error_flag)
            {
                PRINTF ("MBC time out error!\n");
                return MMDEC_HW_ERROR;
            }

            //set mb status
            vop_mode_ptr->mbdec_stat_ptr[error_mb] = ERR_CONCEALED;
#endif
            //next mb
            pos_x++;
            if (pos_x == vop_mode_ptr->MBNumX)
            {
                pos_x = 0;
                pos_y++;
            }
        }
    } else
    {
        return MMDEC_ERROR;
    }

    return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
