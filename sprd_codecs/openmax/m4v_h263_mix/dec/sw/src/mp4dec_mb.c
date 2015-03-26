/******************************************************************************
 ** File Name:    mp4dec_mb.c                                              *
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
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#ifndef INTRA_IDCT_ASSEMBLY
void IDCTcFix16(int16 *block, uint8*rgchDst, int32 nWidthDst);
#define	DCT8X8_INTRA	IDCTcFix16
#else
#define	DCT8X8_INTRA	arm_IDCTcFix
#endif

#ifndef INTER_IDCT_ASSEMBLY
void IDCTiFix(int16 *block, uint8 *rgchDst, int32 nWidthDst, uint8 *ref, int32 ref_width);
#define	DCT8X8_INTER	IDCTiFix
#else
#define	DCT8X8_INTER 	arm_IDCTiFix
#endif

LOCAL void Mp4Dec_GetLeftTopDC(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    int32 mb_x = vop_mode_ptr->mb_x;
    DEC_MB_MODE_T *pTopLeftMB;
    int16 *pDCCache = mb_cache_ptr->pDCCache;
    int16 *pLeftDCCache = mb_cache_ptr->pLeftDCCache;
    int16 *pTopDCACPred = vop_mode_ptr->pTopCoeff + (mb_x <<5);

    pDCCache[0] = DEFAULT_DC_VALUE;
    pDCCache[1] = DEFAULT_DC_VALUE;
    pDCCache[4] = DEFAULT_DC_VALUE;
    pDCCache[3] = DEFAULT_DC_VALUE;
    pDCCache[7] = DEFAULT_DC_VALUE;

    if(mb_cache_ptr->bTopMBAvail)
    {
        DEC_MB_MODE_T *pTopMB = mb_mode_ptr - vop_mode_ptr->MBNumX;
        if(pTopMB->bIntra)
        {
            pDCCache[1] = pTopDCACPred[0];
        }
    }

    if(mb_cache_ptr->bLeftTopAvail)
    {
        pTopLeftMB = mb_mode_ptr - vop_mode_ptr->MBNumX - 1;

        if(pTopLeftMB->bIntra)
        {
            if((mb_mode_ptr-1)->bIntra) //left mb has updated pTopLeftDCLine
            {
                pDCCache[0] = (uint16)pLeftDCCache[0];
                pDCCache[3] = (uint16)pLeftDCCache[1];
                pDCCache[7] = (uint16)pLeftDCCache[2];
            } else
            {
                pDCCache[0] = (uint16)pTopDCACPred[-24];
                pDCCache[3] = (uint16)pTopDCACPred[-16];
                pDCCache[7] = (uint16)pTopDCACPred[-8];
            }
        }
    }

    if(mb_cache_ptr->bLeftMBAvail)
    {
        DEC_MB_MODE_T *pLeftMB = mb_mode_ptr - 1;
        if(pLeftMB->bIntra)
        {
            pDCCache[4] = pDCCache[6];
        }
    }

    pLeftDCCache[0] = pTopDCACPred[8];  //Y copy top DC coeff as left top DC for next MB
    pLeftDCCache[1] = pTopDCACPred[16]; //U
    pLeftDCCache[2] = pTopDCACPred[24]; //V
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBTexture
 ** Description:	Get the texture of intra macroblock.
 ** Author:			Xiaowei Luo, Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    int32 blk_num;
    uint8 *rgchBlkDst;
    int32 iWidthDst;
    int16 *coef_block;
    GetIntraBlkTCoef_func GetIntraBlkTCoef;

    if(MPEG4 == vop_mode_ptr->video_std)
    {
        int32 QP = mb_mode_ptr->StepSize;

        mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[QP];
        mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[QP];
        mb_cache_ptr->bCodeDcAsAc = Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr);

        Mp4Dec_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
        Mp4Dec_GetLeftTopDC(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);

        GetIntraBlkTCoef = Mp4Dec_GetIntraBlkTCoef_Mpeg;
    } else
    {
        GetIntraBlkTCoef = Mp4Dec_GetIntraBlkTCoef_H263;
    }

    for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
    {
        coef_block = vop_mode_ptr->coef_block[blk_num];
        memset(coef_block, 0, 64 * sizeof(int16));
        GetIntraBlkTCoef(vop_mode_ptr, mb_mode_ptr, coef_block, blk_num);
    }

    //y
    iWidthDst = vop_mode_ptr->FrameExtendWidth;
    for(blk_num = 0; blk_num < (BLOCK_CNT-2); blk_num++)
    {
        coef_block = vop_mode_ptr->coef_block[blk_num];
        rgchBlkDst = mb_cache_ptr->mb_addr[0] + mb_cache_ptr->blk8x8_offset[blk_num];
        DCT8X8_INTRA(coef_block, rgchBlkDst, iWidthDst);
    }

    //u
    iWidthDst = vop_mode_ptr->FrameExtendWidth>>1;
    DCT8X8_INTRA(vop_mode_ptr->coef_block[4], mb_cache_ptr->mb_addr[1], iWidthDst);
    //v
    DCT8X8_INTRA(vop_mode_ptr->coef_block[5], mb_cache_ptr->mb_addr[2], iWidthDst);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBTexture
 ** Description:	Get the texture of inter macroblock.
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    int32 CBP = mb_mode_ptr->CBP;
    int32 iBlkIdx;
    int32 iQP = mb_mode_ptr->StepSize;
    int16* rgiBlkCurrQ;
    int32 iWidthCurrQ = MB_SIZE;
    uint8*refBlk;
    uint8 *dstImg;
    int32 dstImg_width;
    GetInterBlkTCoef_func GetInterBlkTCoef;

    if(MPEG4 == vop_mode_ptr->video_std)
    {
        if((!vop_mode_ptr->bReversibleVlc) || (BVOP == vop_mode_ptr->VopPredType))
        {
            GetInterBlkTCoef = Mp4Dec_VlcDecInterTCOEF_Mpeg;
        } else
        {
            GetInterBlkTCoef = Mp4Dec_RvlcInterTCOEF;
        }
    } else
    {
        GetInterBlkTCoef = Mp4Dec_VlcDecInterTCOEF_H263;
    }

    vop_mode_ptr->iCoefStart = 0;

    for (iBlkIdx = 0; iBlkIdx < BLOCK_CNT; iBlkIdx++)
    {
        rgiBlkCurrQ = vop_mode_ptr->coef_block[iBlkIdx];

        if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
        {
            memset(rgiBlkCurrQ, 0, 64 * sizeof(int16));
            GetInterBlkTCoef(vop_mode_ptr, rgiBlkCurrQ, iQP, vop_mode_ptr->bitstrm_ptr);
        }
    }

    //y
    for (iBlkIdx = 0; iBlkIdx < (BLOCK_CNT-2); iBlkIdx++)
    {
        rgiBlkCurrQ = vop_mode_ptr->coef_block[iBlkIdx];
        refBlk = mb_cache_ptr->pMBBfrY+ ((iBlkIdx>> 1) <<7)+( (iBlkIdx & 1) <<3);
        dstImg_width = vop_mode_ptr->FrameExtendWidth;
        dstImg = mb_cache_ptr->mb_addr[0] + mb_cache_ptr->blk8x8_offset[iBlkIdx];

        if(CBP & (32 >> iBlkIdx))
        {
            DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, iWidthCurrQ);
        } else
        {
            mc_xyfull_8x8(refBlk, dstImg, iWidthCurrQ, dstImg_width);
        }
    }

    //u
    dstImg_width = vop_mode_ptr->FrameExtendWidth>>1;
    rgiBlkCurrQ = vop_mode_ptr->coef_block[4];
    refBlk = mb_cache_ptr->pMBBfrU;
    dstImg = mb_cache_ptr->mb_addr[1];
    if(CBP & 0x2)
    {
        DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, BLOCK_SIZE);
    } else
    {
        mc_xyfull_8x8(refBlk, dstImg, BLOCK_SIZE, dstImg_width);
    }

    //v
    rgiBlkCurrQ = vop_mode_ptr->coef_block[5];
    refBlk = mb_cache_ptr->pMBBfrV;
    dstImg = mb_cache_ptr->mb_addr[2];
    if(CBP & 0x1)
    {
        DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, BLOCK_SIZE);
    } else
    {
        mc_xyfull_8x8(refBlk, dstImg, BLOCK_SIZE, dstImg_width);
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBTexture_DP
 ** Description:	Get the texture of inter macroblock. DataPartition.
 ** Author:			Simon.Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBTexture_DP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
    int32 iBlkIdx;
    int32 iQP = mb_mode_ptr->StepSize;
    int16* rgiBlkCurrQ;
    int32 iWidthCurrQ = MB_SIZE;
    uint8*refBlk;
    uint8 *dstImg;
    int32 dstImg_width;
    MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
    MOTION_VECTOR_T Mv_cliped[4];

    if (mb_mode_ptr->bSkip)
    {
        int32 offset;
        uint8* pRefFrm;
        int32 width = vop_mode_ptr->FrameExtendWidth;

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

        mb_mode_ptr->dctMd = MODE_NOT_CODED;
    } else
    {
        int32 blk_num;
        int32 pos_x = vop_mode_ptr->mb_x;
        int32 pos_y = vop_mode_ptr->mb_y;
        int32 total_mb_num_x = vop_mode_ptr->MBNumX;
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

            //Simon.Wang @20120820. Store cliped mvs.
            mv_x = pMv[0].x;
            mv_y = pMv[0].y;
            CLIP_MV(vop_mode_ptr, mv_x, mv_y);
            Mv_cliped[0].x = mv_x;
            Mv_cliped[0].y = mv_y;

            /*compute mv of uv*/
            mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
            mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];

            Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  Mv_cliped, pDstFrameY, dst_width,1);
        } else   /*has 4 MV*/
        {
            int32 dmvcx = 0, dmvcy = 0;

            mb_cache_ptr->mca_type = MCA_BACKWARD_4V;

            for(blk_num = 0; blk_num < 4; blk_num++)
            {
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

            Mp4Dec_motionCompY_fourMV(vop_mode_ptr,  Mv_cliped,pDstFrameY,dst_width, 1);
        }

        Mp4Dec_motionCompensationUV(vop_mode_ptr,  mvc_x, mvc_y, pDstFrameU, pDstFrameV,dst_width/2, 1);

        if (mb_mode_ptr->CBP)	//has resi coeff
        {
            Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
        }
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBHeader
 ** Description:	decode intra macroblock header.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
    int32 dctMd = INTRA;
    int32 MBtype;
    int32 StepSize;
    int32 iMCBPC, CBPC, CBPY;
    int32 dq_index;
    const int8 *dq_table = Mp4Dec_GetDqTable();
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    mb_mode_ptr->bSkip = FALSE;
    mb_mode_ptr->bIntra = TRUE;

    do
    {
        dctMd = INTRA;

        iMCBPC = Mp4Dec_VlcDecMCBPC_com_intra(vop_mode_ptr);

        if(vop_mode_ptr->error_flag)
        {
            PRINTF("Error decoding MCBPC of macroblock\n");
            vop_mode_ptr->return_pos2 |= (1<<6);
            return;
        }

        MBtype = iMCBPC & 7;
        if(4 == MBtype)
        {
            dctMd = INTRAQ;
        } else if(7 == MBtype)
        {
            dctMd = MODE_STUFFING;
        }

        if(MODE_STUFFING != dctMd)
        {
            CBPC = (iMCBPC >> 4) & 3;

            mb_mode_ptr->bACPrediction = FALSE;
            if(MPEG4 == vop_mode_ptr->video_std)
            {
                mb_mode_ptr->bACPrediction = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); // "ACpred_flag"
            }

            CBPY = Mp4Dec_VlcDecCBPY(vop_mode_ptr, TRUE);

            if(vop_mode_ptr->error_flag)
            {
                PRINTF ("Error decoding CBPY of macroblock 2 %d\n");
                vop_mode_ptr->return_pos2 |= (1<<7);
                return;
            }

            mb_mode_ptr->CBP = (int8)(CBPY << 2 | (CBPC));

            StepSize = vop_mode_ptr->StepSize;

            if(INTRAQ == dctMd)
            {
                dq_index = Mp4Dec_ReadBits(bitstrm_ptr, 2); // "DQUANT"
                StepSize += dq_table[dq_index];

                if(StepSize > 31 || StepSize < 1)
                {
                    PRINTF("QUANTIZER out of range 2!\n");
                    StepSize = mmax(1, mmin(31, (StepSize)));
                }
            }

            mb_mode_ptr->StepSize = vop_mode_ptr->StepSize = (int8)StepSize;
        }
    } while(MODE_STUFFING == dctMd);

    mb_mode_ptr->dctMd = (int8)dctMd;

#if _TRACE_
    FPRINTF (g_fp_trace_fw, "mbtype: %d, qp: %d, cbp: %d\n", MBtype, mb_mode_ptr->StepSize, mb_mode_ptr->CBP);
#endif //_TRACE_	

    return;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBHeader
 ** Description:	decode inter macroblock header.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
    int32 dctMd = INTER;
    int32 len;
    int32 bIntra = FALSE;
    uint32 code;
    int32 temp, flush_bits;
    int32 COD;
    int32 MBtype;
    int32 DQUANT, quant;
    int32 iMCBPC, CBPC, CBPY;
    BOOLEAN is_mpeg4 = (vop_mode_ptr->video_std == MPEG4)?1:0;
    static int8 MBTYPE[8] = {INTER, INTERQ, INTER4V, INTRA, INTRAQ, 5, 5, MODE_STUFFING};
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    mb_mode_ptr->bSkip = FALSE;
    mb_mode_ptr->bIntra = FALSE;

    quant = vop_mode_ptr->StepSize;

    do
    {
        dctMd = INTER;

        //COD = Mp4Dec_ReadBits (1); //"COD"
        code = Mp4Dec_Show32Bits(bitstrm_ptr);
#if _TRACE_
//		FPRINTF (g_fp_trace_fw, "code: %d\n", code);
#endif //_TRACE_
        COD = code >> 31;
        code = code << 1;
        flush_bits = 1;

        if(COD)
        {
            mb_mode_ptr->bSkip = TRUE;
            mb_mode_ptr->StepSize = vop_mode_ptr->StepSize;
            mb_mode_ptr->CBP = 0x00;
        } else
        {
            mb_mode_ptr->bSkip = FALSE;

            /*mcbpc*/
            temp = code >> 23;
            if(1 == temp)
            {
                code = code << 9;
                flush_bits += 9;
                iMCBPC = MODE_STUFFING;
            } else if(0 == temp)
            {
                PRINTF ("Invalid MCBPC code\n");
                vop_mode_ptr->error_flag = TRUE;
                vop_mode_ptr->return_pos2 |= (1<<8);
                return;
            } else if(temp >= 256)
            {
                code = code << 1;
                flush_bits += 1;
                iMCBPC = 0;
            } else
            {
                len = vop_mode_ptr->pMCBPCtab[temp].len;
                code = code << len;
                flush_bits += len;
                iMCBPC = vop_mode_ptr->pMCBPCtab[temp].code;
            }

            MBtype = iMCBPC & 7;
            dctMd = MBTYPE[MBtype];
            bIntra = ((dctMd == INTRA)||(dctMd == INTRAQ))? TRUE: FALSE;

            if(dctMd  != MODE_STUFFING)
            {
                CBPC = (iMCBPC >> 4) & 3;
#if 0  //for gmc
                if((vop_mode_ptr->be_gmc_warp) && ((mb_mode_ptr->dctMd == INTER) || (mb_mode_ptr->dctMd == INTERQ)))
                {
                    mb_cache_ptr->mcsel = (char)(code >> 31);
                    code = code << 1;
                    flush_bits += 1;
                } else
                {
                    mb_cache_ptr->mcsel = FALSE;
                }
#endif //0

                mb_mode_ptr->bACPrediction = FALSE;
                if(is_mpeg4  && bIntra && (!vop_mode_ptr->intra_acdc_pred_disable))
                {
                    mb_mode_ptr->bACPrediction = (int8)(code >> 31);
                    code = code << 1;
                    flush_bits += 1;
                }

                /*cbpy*/
                temp = code >> 26;
                if(temp < 2)
                {
                    PRINTF("Invalid CBPY4 code\n");
                    vop_mode_ptr->error_flag = TRUE;
                    vop_mode_ptr->return_pos2 |= (1<<9);
                    return;
                } else if(temp >= 48)
                {
                    code = code << 2;
                    flush_bits += 2;
                    CBPY = 15;
                } else
                {
                    len = vop_mode_ptr->pCBPYtab[temp].len;
                    code = code << len;
                    flush_bits += len;
                    CBPY = vop_mode_ptr->pCBPYtab[temp].code;
                }

                if(!bIntra)
                {
                    CBPY = 15-CBPY;
                }

                mb_mode_ptr->CBP = (int8)(CBPY << 2 | (CBPC));

                if(INTRAQ == dctMd|| INTERQ == dctMd)
                {
                    const int8 *dq_table = Mp4Dec_GetDqTable();

                    DQUANT = code >> 30;
                    code = code << 2;
                    flush_bits += 2;
                    quant += dq_table[DQUANT];

                    quant = (quant > 31) ? 31 : ((quant < 1) ? 1 : quant);

                    vop_mode_ptr->StepSize = (int8)quant;
                }

                mb_mode_ptr->StepSize = vop_mode_ptr->StepSize;
            }
        }

        Mp4Dec_FlushBits(bitstrm_ptr, flush_bits);

    } while(MODE_STUFFING == dctMd);

    mb_mode_ptr->dctMd = (int8)dctMd;
    mb_mode_ptr->bIntra = (BOOLEAN)bIntra;

    if ((dctMd == INTER) || (dctMd == INTERQ))
    {
        mb_cache_ptr->mca_type = MCA_BACKWARD;
    } else if (dctMd == INTER4V)
    {
        mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
    }
}

/* for decode B-frame mb type */
LOCAL int32 Mp4Dec_GetMbType(DEC_BS_T *bitstrm_ptr)
{
    int32 mb_type;

    for(mb_type = 0; mb_type <= 3; mb_type++)
    {
        if(Mp4Dec_ReadBits(bitstrm_ptr, 1))
        {
            return mb_type;
        }
    }

    return -1;
}

/* for decode B-frame dbquant */
LOCAL int32 Mp4Dec_GetDBQuant(DEC_BS_T *bitstrm_ptr)
{
    if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))/*  '0' */
    {
        return (0);
    } else if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))/* '10' */
    {
        return (-2);
    } else	/* '11' */
    {
        return (2);
    }
}

const char s_mbmode_mcatype_map [5] =
{
    MCA_BI_DRT_4V, MCA_BI_DRT, MCA_BACKWARD, MCA_FORWARD, MCA_BI_DRT_4V
};
/*****************************************************************************
 **	Name : 			Mp4Dec_DecMBHeaderBVOP
 ** Description:	decode inter macroblock header, BVOP.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMBHeaderBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
    int32 mbmode;
    int32 cbp;
    int32 dquant;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    int32 quant = vop_mode_ptr->StepSize;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    mb_mode_ptr->bSkip = FALSE;
    mb_mode_ptr->bIntra = FALSE;

    if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))
    {
        /* modb=='0' */
        const uint8 modb2 = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 1);

        mbmode = Mp4Dec_GetMbType(bitstrm_ptr);

        if(!modb2)/* modb=='00' */
        {
            cbp = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 6);
        } else
        {
            cbp = 0;
        }

        if(mbmode && cbp)
        {
            dquant = Mp4Dec_GetDBQuant(bitstrm_ptr);
            quant = quant + dquant;
            quant = IClip(1, 31, quant);
        }

        if(vop_mode_ptr->bInterlace)
        {
            PRINTF ("interlacing is not supported now!\n");
            vop_mode_ptr->error_flag = TRUE;
        }
    } else
    {
        mbmode = MODE_DIRECT_NONE_MV;
        cbp = 0;
    }

    mb_mode_ptr->dctMd = (int8)mbmode;
    mb_mode_ptr->CBP = (int8)cbp;
    vop_mode_ptr->StepSize = mb_mode_ptr->StepSize = (int8)quant;

    mb_cache_ptr->mca_type = s_mbmode_mcatype_map[mbmode];

#if _TRACE_
    FPRINTF (g_fp_trace_fw, "mbtype: %d, qp: %d, cbp: %d\n", mbmode, quant, cbp);
#endif //_TRACE_		
}

/*****************************************************************************
 **	Name : 			Mp4Dec_IsIntraDCSwitch
 ** Description:	Judge if need intra DC switch or not.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC BOOLEAN Mp4Dec_IsIntraDCSwitch(DEC_MB_MODE_T *mb_mode_ptr, int32 intra_dc_vlc_thr)
{
    int32 is_switched = FALSE;

    if(0 == intra_dc_vlc_thr)
    {
        is_switched = FALSE;
    } else if(7 == intra_dc_vlc_thr)
    {
        is_switched = TRUE;
    } else
    {
        uint8 QP;// = mb_mode_ptr->StepSize;

        if(mb_mode_ptr->bFirstMB_in_VP)
        {
            QP = mb_mode_ptr->StepSize; //current mb's qp
        } else
        {
            QP = (mb_mode_ptr-1)->StepSize; //left mb's qp.
        }

        if(QP >= (intra_dc_vlc_thr * 2 + 11))
        {
            is_switched = TRUE;
        }
    }

    return (BOOLEAN)is_switched;
}

PUBLIC void Mp4Dec_GetNeighborMBPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
    int32 MBPosX = vop_mode_ptr->mb_x;
    int32 MBPosY = vop_mode_ptr->mb_y;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    DEC_MB_MODE_T *pLeftMB, *pTopMB, *pLeftTopMB;
    int32 currMBPacketNumber = mb_mode_ptr->videopacket_num;

    mb_cache_ptr->bTopMBAvail = FALSE;
    mb_cache_ptr->bLeftTopAvail = FALSE;
    mb_cache_ptr->bLeftMBAvail = FALSE;

    if(MBPosY > 0)
    {
        pTopMB = mb_mode_ptr - vop_mode_ptr->MBNumX;
        if(pTopMB->bIntra && (currMBPacketNumber == pTopMB->videopacket_num))
        {
            mb_cache_ptr->topMBQP = pTopMB->StepSize;
            mb_cache_ptr->bTopMBAvail = TRUE;
        }

        if(MBPosX > 0)
        {
            pLeftTopMB = pTopMB - 1;
            if(pLeftTopMB->bIntra && (currMBPacketNumber == pLeftTopMB->videopacket_num))
            {
                mb_cache_ptr->leftTopMBQP = pLeftTopMB->StepSize;
                mb_cache_ptr->bLeftTopAvail = TRUE;
            }
        }
    }

    if(MBPosX > 0)
    {
        pLeftMB = mb_mode_ptr - 1;
        if(pLeftMB->bIntra && (currMBPacketNumber == pLeftMB->videopacket_num))
        {
            mb_cache_ptr->leftMBQP = pLeftMB->StepSize;
            mb_cache_ptr->bLeftMBAvail = TRUE;
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
