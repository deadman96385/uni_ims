/******************************************************************************
 ** File Name:    mp4dec_mv.c                                              *
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
#include "mp4dec_video_header.h"
/*lint -save -e744 -e767*/
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/*----------------------------------------------------------------------------*
**                            Mcaro Definitions                               *
**---------------------------------------------------------------------------*/
#define PIX_SORT(_a, _b) {if (_a > _b) PIX_SWAP(_a, _b);}
#define PIX_SWAP(_a, _b) {int32 temp = _a; _a = _b; _b = temp;}

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC  int32 Mp4_GetMedianofThree(int32 a0, int32 a1, int32 a2)
{
    PIX_SORT(a0,a1);
    PIX_SORT(a1,a2);
    PIX_SORT(a0,a1);

    return a1;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get16x16MVPred
 ** Description:	Get motion vector prediction in one mv condition.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_Get16x16MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
    int32 nInBound = 0;
    MOTION_VECTOR_T VecCand[3];
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;

    ((int32 *)VecCand)[0] = 0;
    ((int32 *)VecCand)[1] = 0;
    ((int32 *)VecCand)[2] = 0;

    if(mb_cache_ptr->bLeftMBAvail)
    {
        pMb_mode_Left = mb_mode_ptr - 1;
        VecCand[0] = pMb_mode_Left->mv[1];

        nInBound++;
    }

    if(mb_cache_ptr->bTopMBAvail)
    {
        pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
        VecCand[nInBound] = pMb_mode_Top->mv[2];
        nInBound++;
    }

    if(mb_cache_ptr->rightAvail)
    {
        pMb_mode_Right = mb_mode_ptr - TotalMbNumX + 1;
        VecCand[nInBound] = pMb_mode_Right->mv[2];

        nInBound++;
    }

    if(nInBound == 1)
    {
        ((int32 *)mvPred)[0] = ((int32 *)(VecCand))[0];
        return;
    }

    mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
    mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPredAtBndry
 ** Description:	Get motion vector prediction in 4mv condition when at boundary.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_Get8x8MVPredAtBndry(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
    int32 nInBound = 0;
    MOTION_VECTOR_T VecCand[3];//, zeroMv = {0, 0};
    DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
//	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

    ((int32 *)VecCand)[0] = 0;
    ((int32 *)VecCand)[1] = 0;
    ((int32 *)VecCand)[2] = 0;

    pMb_mode_Left = mb_mode_ptr - 1;
    pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
    pMb_mode_Right = pMb_mode_Top + 1;

    switch(blk_num)
    {
    case 0:
        if(mb_cache_ptr->bLeftMBAvail)
        {
            ((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[1];
            nInBound++;
        }

        if(mb_cache_ptr->bTopMBAvail)
        {
            ((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[2];
            nInBound++;
        }

        if(mb_cache_ptr->rightAvail)
        {
            ((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
            nInBound++;
        }
        break;

    case 1:
        ((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[0];
        nInBound++;

        if(mb_cache_ptr->bTopMBAvail)
        {
            ((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[3];
            nInBound++;
        }

        if(mb_cache_ptr->rightAvail)
        {
            ((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
            nInBound++;
        }

        break;

    case 2:
        if(mb_cache_ptr->bLeftMBAvail)
        {
            ((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[3];
            nInBound++;
        }

        ((int32 *)VecCand)[nInBound] = ((int32 *)(mb_mode_ptr->mv))[0];
        nInBound++;

        ((int32 *)VecCand)[nInBound] = ((int32 *)(mb_mode_ptr->mv))[1];
        nInBound++;

        break;

    case 3:
        ((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[2];
        ((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
        ((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];

        nInBound = 3;

        break;

    default:

        break;
    }

    if(nInBound == 1)
    {
        ((int32 *)mvPred)[0] = ((int32 *)VecCand)[0];
        return;
    }

    mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
    mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPredNotAtBndry
 ** Description:	Get motion vector prediction in 4mv condition when not at boundary.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_Get8x8MVPredNotAtBndry(DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
    MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
    DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;

    ((int32 *)VecCand)[0] = 0;
    ((int32 *)VecCand)[1] = 0;
    ((int32 *)VecCand)[2] = 0;

    pMb_mode_Left = mb_mode_ptr - 1;
    pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
    pMb_mode_Right = pMb_mode_Top + 1;

    switch(blk_num)
    {
    case 0:
        ((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[1];		//left
        ((int32 *)VecCand)[1] = ((int32 *)(pMb_mode_Top->mv))[2];		//top
        ((int32 *)VecCand)[2] = ((int32 *)(pMb_mode_Right->mv))[2];//top right
        break;

    case 1:
        ((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[0];
        ((int32 *)VecCand)[1] = ((int32 *)(pMb_mode_Top->mv))[3];
        ((int32 *)VecCand)[2] = ((int32 *)(pMb_mode_Right->mv))[2];
        break;

    case 2:
        ((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[3];
        ((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
        ((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];
        break;

    case 3:
        ((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[2];
        ((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
        ((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];
        break;

    default:
        break;
    }

    mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
    mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPred
 ** Description:	Get motion vector prediction in 4mv condition.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_Get8x8MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred)
{
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

    if(mb_cache_ptr->bLeftMBAvail && mb_cache_ptr->bTopMBAvail && mb_cache_ptr->rightAvail)
    {
        Mp4Dec_Get8x8MVPredNotAtBndry(mb_mode_ptr, blk_num, mvPred, vop_mode_ptr->MBNumX);
    } else
    {
        Mp4Dec_Get8x8MVPredAtBndry(mb_mode_ptr, mb_cache_ptr, blk_num, mvPred, vop_mode_ptr->MBNumX);
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DeScaleMVD
 ** Description:	DeScale motion vector.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DeScaleMVD(int32 long_vectors, int32 f_code, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pPredMv)
{
    int16 *pVector;
    int32 pred_vector;
    int32 comp;

    if (!long_vectors)
    {
        int32 r_size = f_code-1;
        int32 scale_factor = 1<<r_size;
        int32 range = 32*scale_factor;
        int32 low   = -range;
        int32 high  =  range-1;

        //x
        pVector = &(pMv->x);
        pred_vector = pPredMv->x;

        for (comp = 2; comp > 0; comp--)
        {
            *pVector = (int16)(pred_vector + *pVector);

            if (*pVector < low)
            {
                *pVector += (int16)(2*range);
            } else if (*pVector > high)
            {
                *pVector -= (int16)(2*range);
            }

            //y
            pVector = &(pMv->y);
            pred_vector = pPredMv->y;
        }
    } else
    {
        //x
        pVector = &(pMv->x);
        pred_vector = pPredMv->x;

        for (comp = 2; comp > 0; comp--)
        {
            if (*pVector > 31)
            {
                *pVector -= 64;
            }

            *pVector = (int16)(pred_vector + *pVector);

            if (pred_vector < -31 && *pVector < -63)
            {
                *pVector += 64;
            }

            if (pred_vector > 32 && *pVector > 63)
            {
                *pVector -= 64;
            }

            //y
            pVector = &(pMv->y);
            pred_vector = pPredMv->y;
        }
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecodeOneMVD
 ** Description:	Get one motion vector from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecodeOneMVD(DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *pMv, int32 fcode)
{
    int32 mvData;
    int32 mvResidual;
    int32 r_size = fcode-1;
    int32 scale_factor = 1<<r_size;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
    int32 sign;

    /*mv x*/
    mvData = Mp4Dec_VlcDecMV(vop_mode_ptr, bitstrm_ptr);

    if(fcode > 1 && mvData != 0)
    {
        mvResidual = (int32)Mp4Dec_ReadBits(bitstrm_ptr, (uint32)(fcode-1));//, "mv residual"
        sign = (mvData < 0) ? (-1) : 1;
        pMv->x = ((mvData - sign)<<r_size) + sign * (mvResidual + 1);
    } else
    {
        pMv->x = mvData;
    }

    /*mv y*/
    mvData = Mp4Dec_VlcDecMV(vop_mode_ptr, bitstrm_ptr);

    if(fcode > 1 && mvData != 0)
    {
        mvResidual = (int32)Mp4Dec_ReadBits(bitstrm_ptr, (uint32)(fcode-1));//, "mv residual"
        sign = (mvData < 0) ? (-1) : 1;
        pMv->y = ((mvData - sign)<<r_size) + sign * (mvResidual + 1);
    } else
    {
        pMv->y = mvData;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecodeOneMV
 ** Description:	Get one motion vector from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecodeOneMV(DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pPredMv, int32 fcode)
{
    Mp4Dec_DecodeOneMVD(vop_mode_ptr, pMv, fcode);	/*get mv difference*/
    Mp4Dec_DeScaleMVD(vop_mode_ptr->long_vectors, fcode, pMv, pPredMv);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV
 ** Description:	Get the motion vector of current macroblock from bitstream, PVOP.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
    MOTION_VECTOR_T Mv_cliped[4];

    if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
    {
        /*set the MB's vector to 0*/
        ((int32 *)pMv)[0] = 0;
        ((int32 *)pMv)[1] = 0;
        ((int32 *)pMv)[2] = 0;
        ((int32 *)pMv)[3] = 0;

        if (mb_mode_ptr->bSkip)
        {
            int32 width = vop_mode_ptr->FrameExtendWidth;

            if ((vop_mode_ptr->YUVRefFrame0[0] == NULL) ||
                    (vop_mode_ptr->YUVRefFrame0[1] == NULL) ||
                    (vop_mode_ptr->YUVRefFrame0[2] == NULL))
            {
                uint32 *p, i;

                width >>= 2; 	//word unit

                //y
                p = (uint32 *)(mb_cache_ptr->mb_addr[0]);
                for (i = 0; i < MB_SIZE; i++)
                {
                    p[0] = p[1] = p[2] = p[3] = 0x10101010;
                    p += width;
                }

                //u
                width >>= 1;
                p = (uint32 *)(mb_cache_ptr->mb_addr[1]);
                for (i = 0; i < MB_CHROMA_SIZE; i++)
                {
                    p[0] = p[1] = 0x80808080;
                    p += width;
                }

                //v
                p = (uint32 *)(mb_cache_ptr->mb_addr[2]);
                for (i = 0; i < MB_CHROMA_SIZE; i++)
                {
                    p[0] = p[1] = 0x80808080;
                    p += width;
                }
            } else
            {
                int32 offset;
                uint8* pRefFrm;

                //y
                offset = mb_cache_ptr->mb_addr[0] - vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0];
                pRefFrm = vop_mode_ptr->YUVRefFrame0[0] + offset;
                mc_xyfull_16x16(pRefFrm, mb_cache_ptr->mb_addr[0], width, width);

                //u and v
                offset = mb_cache_ptr->mb_addr[1] - vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1];
                width >>= 1;

                pRefFrm = vop_mode_ptr->YUVRefFrame0[1] + offset;
                mc_xyfull_8x8(pRefFrm, mb_cache_ptr->mb_addr[1], width, width);

                pRefFrm = vop_mode_ptr->YUVRefFrame0[2] + offset;
                mc_xyfull_8x8(pRefFrm, mb_cache_ptr->mb_addr[2], width, width);
            }

            mb_mode_ptr->dctMd = MODE_NOT_CODED;
        }
    } else
    {
        int32 blk_num;
        MOTION_VECTOR_T mvPred;
        int32 pos_x = vop_mode_ptr->mb_x;
        int32 pos_y = vop_mode_ptr->mb_y;
        int32 total_mb_num_x = vop_mode_ptr->MBNumX;
        int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;
        uint8 *pDstFrameY, *pDstFrameU, *pDstFrameV;
        int32 mv_x, mv_y;
        int32 mvc_x, mvc_y;
        int32 dst_width;

        //set mv range
        vop_mode_ptr->mv_x_max = (total_mb_num_x - pos_x) << 5;
        vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
        vop_mode_ptr->mv_y_max = (vop_mode_ptr->MBNumY - pos_y) << 5;
        vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;

        if (mb_mode_ptr->CBP)	//has resi coeff
        {
            pDstFrameY = mb_cache_ptr->pMBBfrY;
            pDstFrameU = mb_cache_ptr->pMBBfrU;
            pDstFrameV = mb_cache_ptr->pMBBfrV;
            dst_width = MB_SIZE;
        } else
        {
            pDstFrameY = mb_cache_ptr->mb_addr[0];
            pDstFrameU = mb_cache_ptr->mb_addr[1];
            pDstFrameV = mb_cache_ptr->mb_addr[2];
            dst_width = vop_mode_ptr->FrameExtendWidth;
        }

        mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
        mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
        mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);

        if(INTER4V != mb_mode_ptr->dctMd)/*has one MV*/
        {
            mb_cache_ptr->mca_type = MCA_BACKWARD;

            Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);	/*get mv predictor*/
            Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv, &mvPred, forward_fcode);

            ((int32 *)pMv)[1] = ((int32 *)pMv)[0];
            ((int32 *)pMv)[2] = ((int32 *)pMv)[0];
            ((int32 *)pMv)[3] = ((int32 *)pMv)[0];

            mv_x = pMv[0].x;
            mv_y = pMv[0].y;
            CLIP_MV(vop_mode_ptr, mv_x, mv_y);
            //Simon.Wang @20120820. Store cliped mvs.
            Mv_cliped[0].x = mv_x;
            Mv_cliped[0].y = mv_y;

            /*compute mv of uv*/
            mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
            mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];

            Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  Mv_cliped, pDstFrameY, dst_width, 1);
        } else   /*has 4 MV*/
        {
            int32 dmvcx = 0, dmvcy = 0;

            mb_cache_ptr->mca_type = MCA_BACKWARD_4V;

            for(blk_num = 0; blk_num < 4; blk_num++)
            {
                Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);/*get mv predictor*/
                Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv + blk_num, &mvPred, forward_fcode);

                mv_x = pMv[blk_num].x;
                mv_y = pMv[blk_num].y;
                CLIP_MV(vop_mode_ptr, mv_x, mv_y);
                dmvcx += mv_x;
                dmvcy += mv_y;
                //Simon.Wang @20120820. Store cliped mvs.
                Mv_cliped[blk_num].x = mv_x;
                Mv_cliped[blk_num].y = mv_y;
            }

            mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
            mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];

            Mp4Dec_motionCompY_fourMV(vop_mode_ptr,  Mv_cliped,pDstFrameY, dst_width, 1);
        }

        Mp4Dec_motionCompensationUV(vop_mode_ptr,  mvc_x, mvc_y, pDstFrameU, pDstFrameV, dst_width/2, 1);
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV_DP
 ** Description:	Get the motion vector of current macroblock from bitstream, DataPartition.
 ** Author:			Simon.Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV_DP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;

    if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
    {
        /*set the MB's vector to 0*/
        ((int32 *)pMv)[0] = 0;
        ((int32 *)pMv)[1] = 0;
        ((int32 *)pMv)[2] = 0;
        ((int32 *)pMv)[3] = 0;

    } else
    {
        int32 blk_num;
        MOTION_VECTOR_T mvPred;
        int32 pos_x = vop_mode_ptr->mb_x;
        int32 pos_y = vop_mode_ptr->mb_y;
        int32 total_mb_num_x = vop_mode_ptr->MBNumX;
        int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;

        mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
        mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
        mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);

        if(INTER4V != mb_mode_ptr->dctMd)/*has one MV*/
        {
            mb_cache_ptr->mca_type = MCA_BACKWARD;

            Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);	/*get mv predictor*/
            Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv, &mvPred, forward_fcode);

            ((int32 *)pMv)[1] = ((int32 *)pMv)[0];
            ((int32 *)pMv)[2] = ((int32 *)pMv)[0];
            ((int32 *)pMv)[3] = ((int32 *)pMv)[0];

        } else   /*has 4 MV*/
        {
            int32 dmvcx = 0, dmvcy = 0;

            mb_cache_ptr->mca_type = MCA_BACKWARD_4V;

            for(blk_num = 0; blk_num < 4; blk_num++)
            {
                Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);/*get mv predictor*/
                Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv + blk_num, &mvPred, forward_fcode);
            }
        }
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
