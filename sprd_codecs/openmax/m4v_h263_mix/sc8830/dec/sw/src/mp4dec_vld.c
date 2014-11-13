/******************************************************************************
 ** File Name:    mp4dec_vld.c                                                *
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

#define FAST_VLD
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMCBPC_com_intra
 ** Description:	Get MCBPC of intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecMCBPC_com_intra(DEC_VOP_MODE_T *vop_mode_ptr)
{
    uint16 tmpval;
    const MCBPC_TABLE_CODE_LEN_T *tab = NULL;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    tmpval = (uint16)Mp4Dec_ShowBits(bitstrm_ptr, 9);

    if(1 == tmpval)
    {
        /* macroblock stuffing */
        Mp4Dec_FlushBits(bitstrm_ptr, 9);
        return 7;
    }

    if(tmpval < 8)
    {
        PRINTF("Invalid MCBPCintra code\n");
        vop_mode_ptr->error_flag = TRUE;
        vop_mode_ptr->return_pos2 |= (1<<16);
        return -1;
    }

    tmpval >>= 3;

    if(tmpval >= 32)
    {
        Mp4Dec_FlushBits(bitstrm_ptr, 1);
        return 3;
    }

    tab = &(vop_mode_ptr->pMCBPCtabintra[tmpval]);
    Mp4Dec_FlushBits(bitstrm_ptr, (uint32)(tab->len));

    return (int32)tab->code;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMCBPC_com_inter
 ** Description:	Get MCBPC of inter macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
int16 Mp4Dec_VlcDecMCBPC_com_inter(DEC_VOP_MODE_T *vop_mode_ptr)
{
    uint32 code;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    code = Mp4Dec_ShowBits(bitstrm_ptr, 9);

    if(code == 1)
    {
        /* macroblock stuffing */
        Mp4Dec_FlushBits(bitstrm_ptr, 9);
        return 7; /* HHI for Macroblock stuffing */
    }

    if(code == 0)
    {
        PRINTF ("Invalid MCBPC code\n");
        vop_mode_ptr->error_flag = TRUE;
        vop_mode_ptr->return_pos2 |= (1<<17);
        return -1;
    }

    if(code >= 256)
    {
        Mp4Dec_FlushBits(bitstrm_ptr, 1);
        return 0;
    }

    Mp4Dec_FlushBits(bitstrm_ptr, (uint8)vop_mode_ptr->pMCBPCtab[code].len);

    return vop_mode_ptr->pMCBPCtab[code].code;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecCBPY
 ** Description:	Get CBPY of macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecCBPY(DEC_VOP_MODE_T *vop_mode_ptr, BOOLEAN is_intra_mb)
{
    int32 tmpval;
    int32  cbpy;
    const CBPY_TABLE_CODE_LEN_T *tab = NULL;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    tmpval = (int32)Mp4Dec_ShowBits(bitstrm_ptr, 6);

    if(tmpval < 2)
    {
        PRINTF("Invalid CBPY4 code\n");
        vop_mode_ptr->error_flag = TRUE;
        vop_mode_ptr->return_pos2 |= (1<<18);
        return -1;
    }

    if(tmpval >= 48)
    {
        Mp4Dec_FlushBits(bitstrm_ptr, 2);
        cbpy = 15;
    } else
    {
        tab = &(vop_mode_ptr->pCBPYtab[tmpval]);
        Mp4Dec_FlushBits(bitstrm_ptr, (uint32)(tab->len));
        cbpy = tab->code;
    }

    if(!is_intra_mb)
    {
        cbpy = 15-cbpy;
    }

    return cbpy;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecPredIntraDCSize
 ** Description:	Get DC size of prediction intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL uint8 Mp4Dec_VlcDecPredIntraDCSize(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
{
    uint8 dc_size=0;
    uint16 code;
    uint8 code_len;/*add by zhang zheng*/
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    if(blk_num < 4) /* luminance block */
    {
        code = (uint16)Mp4Dec_ShowBits(bitstrm_ptr, 11);
        code_len = 11;

        while((1 != code) && (code_len > 3))
        {
            code >>= 1;
            code_len--;
        }

        if(code_len > 3)
        {
            dc_size = code_len + 1;
            Mp4Dec_FlushBits(bitstrm_ptr, code_len);
            return dc_size;
        }

        if(3 == code_len)
        {
            if(1 == code)
            {
                dc_size = 4;
                Mp4Dec_FlushBits(bitstrm_ptr, 3);

                return dc_size;
            } else if(2 == code)
            {
                dc_size = 3;
                Mp4Dec_FlushBits(bitstrm_ptr, 3);

                return dc_size;
            } else if(3 == code)
            {
                dc_size = 0;
                Mp4Dec_FlushBits(bitstrm_ptr, 3);

                return dc_size;
            }
            code_len--;
            code >>= 1;
        }

        if(2 == code_len)
        {
            if(2 == code)
            {
                dc_size = 2;
            } else if(3 == code)
            {
                dc_size =1;
            }

            Mp4Dec_FlushBits(bitstrm_ptr, 2);
        }

        return dc_size;
    } else /* chrominance block */
    {
        code = (uint16)Mp4Dec_ShowBits(bitstrm_ptr, 12);
        code_len = 12;

        while( (code_len > 2) && ( 1 != code ))
        {
            code>>=1;
            code_len--;
        }

        dc_size = code_len;

        if(2 == code_len)
        {
            dc_size = 3 - code;
        }

        Mp4Dec_FlushBits(bitstrm_ptr, code_len);

        return dc_size;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecPredIntraDC
 ** Description:	Get DC of prediction intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecPredIntraDC(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
{
    uint8 dc_size;
    uint32 tmp_var;
    int32 intra_dc_delta;
    int first_bit;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    /* read DC size 2 - 8 bits */
    dc_size = Mp4Dec_VlcDecPredIntraDCSize(vop_mode_ptr, blk_num);

    if(vop_mode_ptr->error_flag)
    {
        PRINTF("Error decoding INTRADC pred. size\n");
        vop_mode_ptr->return_pos2 |= (1<<19);
        return -1;
    }

    if(0 == dc_size)
    {
        intra_dc_delta = 0;
    } else
    {
        /* read delta DC 0 - 8 bits */
        tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, dc_size);//"DC coeff"

        first_bit = (tmp_var >> (dc_size-1));

        if(0 == first_bit)
        {
            /* negative delta INTRA DC */
            intra_dc_delta  = (-1) * (int32)(tmp_var ^ ((1 << dc_size) - 1));
        } else
        {
            /* positive delta INTRA DC */
            intra_dc_delta = tmp_var;
        }

        if(dc_size > 8)
        {
            Mp4Dec_ReadBits(bitstrm_ptr, 1);//"Marker bit"
        }
    }

    return intra_dc_delta;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecMV
 ** Description:	Get motion vector.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC int16 Mp4Dec_VlcDecMV(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr)
{
    uint32 code;
    int32  tmp;
    const MV_TABLE_CODE_LEN_T *tab = PNULL;

    if(Mp4Dec_ReadBits(bitstrm_ptr, 1))//"motion_code"
    {
        return 0; /* Vector difference = 0 */
    }

    if((code = Mp4Dec_ShowBits(bitstrm_ptr, 12)) >= 512)
    {
        code = (code >> 8) - 2;
        tab = &vop_mode_ptr->pTMNMVtab0[code];

        Mp4Dec_FlushBits(bitstrm_ptr, (uint8)(tab->len));
        return tab->code;
    }

    if(code >= 128)
    {
        code = (code >> 2) - 32;
        tab  = &vop_mode_ptr->pTMNMVtab1[code];

        Mp4Dec_FlushBits(bitstrm_ptr, (uint8)(tab->len));
        return tab->code;
    }

    tmp = code - 4;

    if(tmp < 0)
    {
        PRINTF("Invalid motion_vector code\n");
        vop_mode_ptr->error_flag = TRUE;
        vop_mode_ptr->return_pos2 |= (1<<20);
        return -1;
    }

    code -= 4;

    tab = &vop_mode_ptr->pTMNMVtab2[code];

    Mp4Dec_FlushBits(bitstrm_ptr, (uint8)(tab->len));

    return tab->code;
}

//#define FAST_VLD
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#if  1// WIN32
/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecIntraTCOEF
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:			vld, inverse scan, for MPEG-4
 *****************************************************************************/
PUBLIC int32 Mp4Dec_VlcDecIntraTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iCoefQ, int32 iCoefStart,char *pNonCoeffPos)
{
    int32 len;
    int32 temp;
    uint32 code;
    int32 nonCoeffNum = 0;
    int32 flush_bits;
    int32 i = iCoefStart, index;
    int32 level, run, sign, last = 0;
    const VLC_TABLE_CODE_LEN_T *tab = PNULL;
    const uint8 * rgiZigzag = vop_mode_ptr->pZigzag;
    DEC_BS_T *pBitstrm = vop_mode_ptr->bitstrm_ptr;

    while(!last)
    {
        //code = Mp4Dec_ShowBits_sw(pBitstrm, 32);
        code = Mp4Dec_Show32Bits(pBitstrm);
        temp = code >> 20;

        if(temp >= 512)
        {
            tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
        } else if(temp >= 128)
        {
            tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
        } else if(temp >= 8)
        {
            tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
        } else
        {
            PRINTF("Invalid Huffman code 1\n");
            vop_mode_ptr->error_flag = TRUE;
            vop_mode_ptr->return_pos2 |= (1<<21);

            return 0;
        }

        //Mp4Dec_FlushBits (pBitstrm, tab->len);
        len = tab->len;
        flush_bits = len;
        code = code << len;

        if(ESCAPE == tab->code)
        {
            int32 level_offset;

            //level_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC level offset"
            level_offset = code >> 31;
            flush_bits += 1;
            code = code << 1;

            /*first escape mode, level is offset*/
            if(!level_offset)
            {
                //code = Mp4Dec_ShowBits (pBitstrm, 12);
                temp = code >> 20;

                if(temp >= 512)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
                } else if(temp >= 128)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
                } else if(temp >= 8)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
                } else
                {
                    PRINTF ("Invalid Huffman code 2\n");
                    vop_mode_ptr->error_flag = TRUE;
                    vop_mode_ptr->return_pos2 |= (1<<22);
                    return 0;
                }

                //Mp4Dec_FlushBits (pBitstrm, tab->len);
                len = tab->len;
                flush_bits += len;
                code = code << len;

                run = (tab->code >> 8) & 63;
                level = tab->code & 255;
                last = (tab->code >> 14) & 1;

                level += vop_mode_ptr->pIntra_max_level[(last <<6) + run];
                //sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"

                sign = code >> 31;
                flush_bits += 1;
                code = code << 1;
            } else
            {
                /* second escape mode. run is offset */
                int32 run_offset;

                //run_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC run offset"
                run_offset = code >> 31;
                flush_bits += 1;
                code = code << 1;

                if(!run_offset)
                {
                    //code = Mp4Dec_ShowBits (pBitstrm, 12);
                    temp = code >> 20;

                    if(temp >= 512)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab3[(temp >> 5) - 16];
                    } else if(temp >= 128)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab4[(temp >> 2) - 32];
                    } else if(temp >= 8)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab5[(temp >> 0) - 8];
                    } else
                    {
                        PRINTF("Invalid Huffman code 3\n");
                        vop_mode_ptr->error_flag = TRUE;
                        vop_mode_ptr->return_pos2 |= (1<<23);
                        return 0;
                    }

                    //Mp4Dec_FlushBits (pBitstrm, tab->len);
                    len = tab->len;
                    flush_bits += len;
                    code = code << len;

                    run = (tab->code >> 8) & 63;
                    level = tab->code & 255;
                    last = (tab->code >> 14) & 1;

                    run = run + vop_mode_ptr->pIntra_max_run [(last <<5) + level] + 1;

                    //sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
                    sign = code >> 31;
                    flush_bits += 1;
                    code = code << 1;
                } else
                {
                    int32 run_levle_last;

                    /* third escape mode*/
                    //run_levle_last = Mp4Dec_ReadBits (pBitstrm, 21);
                    run_levle_last = code >> 11;
                    flush_bits += 21;
                    code = code << 21;

                    last = run_levle_last >> 20;
                    run = (run_levle_last >> 14) & 0x3f;
                    level = (run_levle_last >> 1) & 0xfff;

                    if(level >= 2048)
                    {
                        sign = 1;
                        level = 4096 - level;
                    } else
                    {
                        sign = 0;
                    }
                }
            }
        } else
        {
            run = (tab->code >> 8) & 63;
            level = tab->code & 255;
            last = (tab->code >> 14) & 1;

            //tcoef.sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
            sign = code >> 31;
            flush_bits += 1;
            code = code << 1;
        }

        Mp4Dec_FlushBits(pBitstrm, flush_bits);

        i += run;

        if(i >= 64)
        {
            vop_mode_ptr->error_flag = TRUE;
        }
        index = rgiZigzag[i];
        //statistics which coefficients are non-zero for inverse quantization
        pNonCoeffPos[nonCoeffNum] = index;

        if(sign)
        {
            iCoefQ [index] = -level;
        } else
        {
            iCoefQ [index] = level;
        }

        i++;
        nonCoeffNum++;
    }

    //return -1;
    return nonCoeffNum;
}
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecInterTCOEF_H263
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_VlcDecInterTCOEF_H263(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP, DEC_BS_T *pBitstrm)
{
    uint32 code;
    uint32 temp;
    const VLC_TABLE_CODE_LEN_T *tab = PNULL;
    int32 run, level, sign = 0,last = 0;
    int32 flush_bits = 0;
    int32 index;
    int32 i = vop_mode_ptr->iCoefStart;
    const uint8 * rgiZigzag = vop_mode_ptr->pStandardZigzag;
    int32 iquant;
    int32 QPModify = (iQP & 0x01) - 1;
    int32 qadd = (iQP - 1) | 0x1;
    int32 qmul = iQP << 1;

    last = 0;

    while(last != 1)
    {
        flush_bits = 0;

        code = Mp4Dec_Show32Bits(pBitstrm);
        temp = code >> 20;

        if(temp >= 512)
        {
            tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
        } else if(temp >= 128)
        {
            tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
        } else if(temp >= 8)
        {
            tab = &vop_mode_ptr->pDCT3Dtab2[(temp >> 0) - 8];
        } else
        {
            PRINTF("Invalid Huffman code\n");
            vop_mode_ptr->error_flag = TRUE;
            vop_mode_ptr->return_pos2 |= (1<<24);
            return;
        }

        //Mp4Dec_FlushBits (pBitstrm, tab->len);
        code = code << tab->len;
        flush_bits += tab->len;

        if(ESCAPE == tab->code)
        {
            int32 run_level_last;

            if(ITU_H263 == vop_mode_ptr->video_std)
            {
                //run_level_last = Mp4Dec_ReadBits (pBitstrm, 15);
                run_level_last = code >> 17;
                code = code << 15;
                flush_bits += 15;

                last = run_level_last >> 14;
                run = (run_level_last >> 8) & 0x3f;
                level = run_level_last & 0xff;

                if(level > 128)
                {
                    sign = 1;
                    level = 256 - level;
                } else if(level == 128)
                {
                    PRINTF ("Illegal LEVEL for ESCAPE mode 4: 128\n");
                    vop_mode_ptr->error_flag = TRUE;
                    vop_mode_ptr->return_pos2 |= (1<<25);
                    return;
                } else if(level > 0)
                {
                    sign = 0;
                } else if(level == 0)
                {
                    PRINTF ("Illegal LEVEL for ESCAPE mode 4: 0\n");
                    vop_mode_ptr->error_flag = TRUE;
                    vop_mode_ptr->return_pos2 |= (1<<26);
                    return;
                }
            } else //sorenson h.263
            {
                int isl1;

                //run_level_last = Mp4Dec_ReadBits (pBitstrm, 8);
                run_level_last = code >> 24;
                code = code << 8;
                flush_bits += 8;

                isl1 = (run_level_last >> 7);
                last = (run_level_last >> 6) & 0x01;
                run = run_level_last & 0x3f;

                if(isl1)
                {
                    flush_bits += 11;
                    level = code >>21;
                    if(level >= 1024)
                    {
                        sign = 1;
                        level = 2048 - level;
                    } else
                    {
                        sign = 0;
                    }
                } else
                {
                    flush_bits += 7;
                    level = code >> 25;
                    if(level > 64)
                    {
                        sign = 1;
                        level = 128 - level;
                    } else
                    {
                        sign = 0;
                    }
                }
            }
        } else
        {
            run = (tab->code >> 4) & 255;
            last = (tab->code >> 12) & 1;
            level = tab->code & 15;

            /*sign = Mp4Dec_ReadBits (pBitstrm, 1); */
            sign = code >> 31;
            code = code << 1;
            flush_bits += 1;
        }

        Mp4Dec_FlushBits(pBitstrm, flush_bits);

        i += run;
        if(i >= 64)
        {
            vop_mode_ptr->error_flag = TRUE;
            vop_mode_ptr->return_pos2 |= (1<<27);
            return;
        }
        index = rgiZigzag[i];

        iquant = level * qmul + qadd;

        if(sign == 1)
        {
            iquant = -iquant;
        }

        iDCTCoef[index] = iquant;
        i++;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_VlcDecInterTCOEF_Mpeg
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:     vld do deScan.
 *****************************************************************************/
#if  1// WIN32
PUBLIC void Mp4Dec_VlcDecInterTCOEF_Mpeg(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP,DEC_BS_T * pBitstrm)
{
    uint32 code;
    int32 i = 0, index;
    const VLC_TABLE_CODE_LEN_T *tab = PNULL;
    int32 run, level, sign, last = 0;
    int32 flush_bits;
    int32 temp;
    const uint8 * rgiZigzag = vop_mode_ptr->pStandardZigzag;

    /*for iquantization*/
    int32 iquant;
    int32 fQuantizer = vop_mode_ptr->QuantizerType;
    char *piQuantizerMatrix;
    int32 iSum = 0;
    BOOLEAN bCoefQAllZero = TRUE;
    int32 qadd, qmul;

    if(fQuantizer == Q_H263)
    {
        qadd = (iQP - 1) | 0x1;
        qmul = iQP << 1;
    } else
    {
        piQuantizerMatrix = vop_mode_ptr->InterQuantizerMatrix;
    }

    while(last == 0)
    {
        code = Mp4Dec_Show32Bits(pBitstrm);

        temp = code >> 20;
        if(temp >= 512)
        {
            tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
        } else if(temp >= 128)
        {
            tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
        } else if(temp >= 8)
        {
            tab = &vop_mode_ptr->pDCT3Dtab2[(temp >> 0) - 8];
        } else
        {
            PRINTF("Invalid Huffman code\n");
            vop_mode_ptr->error_flag = TRUE;
            vop_mode_ptr->return_pos2 |= (1<<28);
            return;
        }

        //Mp4Dec_FlushBits (pBitstrm, tab->len);
        code = code << tab->len;
        flush_bits = tab->len;

        if(ESCAPE == tab->code)
        {
            int32 level_offset;

            //level_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC level offset"
            level_offset = code >> 31;
            code = code << 1;
            flush_bits += 1;

            /*first escape mode, level is offset*/
            if(!level_offset)
            {
                //code = Mp4Dec_ShowBits (pBitstrm, 12);
                temp = code >> 20;

                if(temp >= 512)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
                } else if (temp >= 128)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
                } else if (temp >= 8)
                {
                    tab = &vop_mode_ptr->pDCT3Dtab2[temp - 8];
                } else
                {
                    PRINTF("Invalid Huffman code\n");
                    vop_mode_ptr->error_flag = TRUE;
                    vop_mode_ptr->return_pos2 |= (1<<29);
                    return;
                }

                //Mp4Dec_FlushBits (pBitstrm, tab->len);
                code = code << tab->len;
                flush_bits += tab->len;

                run = (tab->code >> 4) & 255;
                last = (tab->code >> 12) & 1;
                level = tab->code & 15;

                level += vop_mode_ptr->pInter_max_level[(last <<6) + run];
                //sign = Mp4Dec_ReadBits (pBitstrm, 1); 	//, "SIGN"
                sign = code >> 31;
                code = code << 1;
                flush_bits += 1;
            } else
            {
                /* second escape mode. run is offset */
                int32 run_offset;

                //run_offset = Mp4Dec_ReadBits (pBitstrm, 1);//, "ESC run offset"
                run_offset = code >> 31;
                code = code << 1;
                flush_bits += 1;

                if(!run_offset)
                {
                    //code = Mp4Dec_ShowBits (pBitstrm, 12);
                    temp = code >> 20;

                    if(temp >= 512)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab0[(temp >> 5) - 16];
                    } else if(temp >= 128)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab1[(temp >> 2) - 32];
                    } else if(temp >= 8)
                    {
                        tab = &vop_mode_ptr->pDCT3Dtab2[temp  - 8];
                    } else
                    {
                        PRINTF("Invalid Huffman code\n");
                        vop_mode_ptr->error_flag = TRUE;
                        vop_mode_ptr->return_pos2 |= (1<<30);
                        return;
                    }

                    //Mp4Dec_FlushBits (pBitstrm, tab->len);
                    code = code << tab->len;
                    flush_bits += tab->len;

                    run = (tab->code >> 4) & 255;
                    last = (tab->code >> 12) & 1;
                    level = tab->code & 15;
                    run = run + vop_mode_ptr->pInter_max_run[(last <<4) + level] + 1;

                    //sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
                    sign = code >> 31;
                    code = code << 1;
                    flush_bits += 1;
                } else
                {
                    //int32_t mark;
                    int32 run_levle_last;//

                    /* third escape mode*/
                    //run_levle_last = Mp4Dec_ReadBits (pBitstrm, 21);
                    run_levle_last = code >> 11;
                    code = code << 21;
                    flush_bits += 21;

                    last = run_levle_last >> 20;
                    run = (run_levle_last >> 14) & 0x3f;
                    level = (run_levle_last >> 1) & 0xfff;

                    if(level >= 2048)
                    {
                        sign = 1;
                        level = 4096 - level;
                    } else
                    {
                        sign = 0;
                    }
                }
            }
        } else
        {
            run = (tab->code >> 4) & 255;
            level = tab->code & 15;
            last = (tab->code >> 12) & 1;

            //tcoef.sign = Mp4Dec_ReadBits (pBitstrm, 1); //, "SIGN"
            sign = code >> 31;
            code = code << 1;
            flush_bits += 1;
        }

        Mp4Dec_FlushBits(pBitstrm, flush_bits);

        /*inverse quantization*/
        i += run;
        if(i >= 64)
        {
            vop_mode_ptr->error_flag = TRUE;
            vop_mode_ptr->return_pos2 |= (1<<31);
            return;
        }
        index = rgiZigzag[i];

        if(fQuantizer == Q_H263)
        {
            //	iquant = (iQP * ((level << 1) + 1) + QPModify);
            iquant = level * qmul + qadd;
        } else
        {
            iquant = iQP * (level * 2  + 1) * piQuantizerMatrix [index] >> 4;

            iSum ^= iquant;
            bCoefQAllZero = FALSE;
        }

        if(sign == 1)
        {
            iquant = -iquant;
        }

        iDCTCoef[index] = iquant;
        i++;
    }

    if(fQuantizer != Q_H263)
    {
        if(!bCoefQAllZero)
        {
            if((iSum & 0x00000001) == 0)
            {
                iDCTCoef [63] ^= 0x00000001;
            }
        }
    }
}
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
