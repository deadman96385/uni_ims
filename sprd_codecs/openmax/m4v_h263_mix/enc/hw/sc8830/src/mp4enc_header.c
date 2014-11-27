/******************************************************************************
 ** File Name:    mp4enc_header.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************
 **	Name : 			Mp4Enc_InitVOPHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
MP4_LOCAL uint32 Mp4Enc_InitVOPHeader(Mp4EncObject*vo, int32 time_stamp)
{
    uint32 NumBits = 0;
    int32 modula_time_base;
    int32 vop_time_increment;
    int32 nbits_modula_time_base;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    NumBits += Mp4Enc_OutputBits(vo, START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
    NumBits += Mp4Enc_OutputBits(vo, ENC_VOP_START_CODE, NUMBITS_VOP_START_CODE);
    NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->VopPredType, NUMBITS_VOP_PRED_TYPE);

    modula_time_base = time_stamp / (int32)vol_mode_ptr->ClockRate;
    vop_time_increment = time_stamp - modula_time_base * (int32)vol_mode_ptr->ClockRate;

    nbits_modula_time_base = modula_time_base - vo->g_enc_last_modula_time_base;
    NumBits += Mp4Enc_OutputBits(vo, g_msk[nbits_modula_time_base] << 1, nbits_modula_time_base + 1);
    NumBits += Mp4Enc_OutputBits(vo,  (int32) 1, 1);
    NumBits += Mp4Enc_OutputBits(vo, vop_time_increment, vop_mode_ptr->time_inc_resolution_in_vol_length);

    vo->g_enc_last_modula_time_base = modula_time_base;
    NumBits += Mp4Enc_OutputBits(vo,  (int32) 1, 1);

    return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncH263PicHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
int32 Mp4Enc_EncH263PicHeader(Mp4EncObject*vo)
{
    int32 nBits = 0;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    nBits += Mp4Enc_OutputBits(vo,  1, 17);
    nBits += Mp4Enc_OutputBits(vo,  0, 5);
    nBits += Mp4Enc_OutputBits(vo,  vo->g_enc_tr, 8);

    /*PTYPE 13 bits*/
    nBits += Mp4Enc_OutputBits(vo,  1, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  vop_mode_ptr->H263SrcFormat, 3);
    nBits += Mp4Enc_OutputBits(vo,  vop_mode_ptr->VopPredType, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1); //unrestricted mv is OFF, modified from 'on' to 'off', 20100111
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  vop_mode_ptr->StepSize, 5);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);
    nBits += Mp4Enc_OutputBits(vo,  0, 1);

    vo->g_enc_tr += vo->g_enc_frame_skip_number + 1;
    vo->g_enc_tr = vo->g_enc_tr % 256;

    return nBits; //= 50
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncSequenceHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_EncSequenceHeader(Mp4EncObject*vo)
{
    uint32 NumBits = 0;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    BOOLEAN is_short_header = vop_mode_ptr->short_video_header;

    NumBits += Mp4Enc_OutputBits(vo,  START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
    NumBits += Mp4Enc_OutputBits(vo,  VSS_START_CODE, NUMBITS_START_CODE_SUFFIX);
    NumBits += Mp4Enc_OutputBits(vo,  vol_mode_ptr->ProfileAndLevel, NUMBITS_VSS_PROFILE);
    NumBits += Mp4Enc_OutputBits(vo,  START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
    NumBits += Mp4Enc_OutputBits(vo,  VSO_START_CODE, NUMBITS_START_CODE_SUFFIX);
    NumBits += Mp4Enc_OutputBits(vo,  0, 1);
    NumBits += Mp4Enc_OutputBits(vo,  VSO_TYPE, NUMBITS_VSO_TYPE);
    NumBits += Mp4Enc_OutputBits(vo,  0, 1);

    NumBits += Mp4Enc_ByteAlign(vo,  is_short_header);

    return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_EncVOHeader(Mp4EncObject*vo)
{
    uint32 NumBits = 0;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    NumBits += Mp4Enc_OutputBits(vo,  START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
    NumBits += Mp4Enc_OutputBits(vo,  ENC_VO_START_CODE, NUMBITS_VO_START_CODE); //plus 3 bits
    NumBits += Mp4Enc_OutputBits(vo,  0, NUMBITS_VO_ID);

    PRINTF("VO Overhead\t\t%d bits\n\n", NumBits);

    return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOLHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_EncVOLHeader(Mp4EncObject*vo)
{
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    uint32 NumBits = 0;
    ALPHA_USAGE_E alpha_usage;
    BOOLEAN is_short_header = vop_mode_ptr->short_video_header;

    NumBits += Mp4Enc_OutputBits(vo,  START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
    NumBits += Mp4Enc_OutputBits(vo,  ENC_VOL_START_CODE, NUMBITS_VOL_START_CODE);
    NumBits += Mp4Enc_OutputBits(vo, 0, NUMBITS_VOL_ID);
    NumBits += Mp4Enc_OutputBits(vo, 0, 1);
    NumBits += Mp4Enc_OutputBits(vo, 1, 8);
    NumBits += Mp4Enc_OutputBits(vo, 0, 1);
    NumBits += Mp4Enc_OutputBits(vo, 1, 4);
    NumBits += Mp4Enc_OutputBits(vo, 0, 1);//VOL_Control is disable.

    alpha_usage = vol_mode_ptr->fAUsage;
    if(2 == alpha_usage) // CD: 0 = rect, 1 = binary, 2 = shape only, 3 = grey alpha
    {
        alpha_usage = 3;
    }

    NumBits += Mp4Enc_OutputBits(vo, alpha_usage, NUMBITS_VOL_SHAPE);//0 is RECTANGLE.
    NumBits += Mp4Enc_OutputBits(vo, 1, 1);
    NumBits += Mp4Enc_OutputBits(vo,  vol_mode_ptr->ClockRate, NUMBITS_TIME_RESOLUTION);
    NumBits += Mp4Enc_OutputBits(vo, 1, 1);
    NumBits += Mp4Enc_OutputBits(vo, 0, 1);

    if(RECTANGLE == vol_mode_ptr->fAUsage)
    {
        NumBits += Mp4Enc_OutputBits(vo, MARKER_BIT, 1);
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->OrgFrameWidth, NUMBITS_VOP_WIDTH);
        NumBits += Mp4Enc_OutputBits(vo, MARKER_BIT, 1);
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->OrgFrameHeight, NUMBITS_VOP_HEIGHT);
        NumBits += Mp4Enc_OutputBits(vo, MARKER_BIT, 1);

        // 	NumBits += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT + 3;
    }

    NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->bInterlace, 1);
    NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->bAdvPredDisable, 1);
    NumBits += Mp4Enc_OutputBits(vo, 0, 1);
    NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->bNot8Bit, 1);
    NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->QuantizerType, 1);

    NumBits += Mp4Enc_OutputBits(vo, 1, 1);
    NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->bResyncMarkerDisable, 1);
    NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->bDataPartitioning, 1);

    if( vol_mode_ptr->bDataPartitioning )
    {
        NumBits += Mp4Enc_OutputBits(vo, vol_mode_ptr->bReversibleVlc, 1);
    }

    NumBits += Mp4Enc_OutputBits(vo, 0, 1);
    NumBits += Mp4Enc_ByteAlign(vo, is_short_header);

    return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOPHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_EncVOPHeader(Mp4EncObject*vo, int32 time_stamp)
{
    uint32 NumBits = 0;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    NumBits += Mp4Enc_InitVOPHeader(vo,  time_stamp);
    NumBits += Mp4Enc_OutputBits(vo, 1, 1);/* vop_coded */

    if(PVOP == vop_mode_ptr->VopPredType)
    {
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->RoundingControl, 1);
    }

    /*intra_dc_vlc_thr is */
    NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->IntraDcSwitchThr, 3);/* intra_dc_vlc_threshold */

    vop_mode_ptr->bAlternateScan = FALSE;

    /*output quantization of this frame*/
    NumBits += Mp4Enc_OutputBits(vo,vop_mode_ptr->StepSize, vop_mode_ptr->QuantPrecision);

    if(PVOP == vop_mode_ptr->VopPredType)
    {
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */
    }

    return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncGOBHeader
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_EncGOBHeader(Mp4EncObject*vo, int32 gob_quant)
{
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    uint32 NumBits = 0;
    uint32 gob_num;
    int32 gfid = 0;

    gob_num = (vop_mode_ptr->mb_y) / vop_mode_ptr->MBLineOneGOB;

    NumBits += Mp4Enc_OutputBits(vo, 1, 17);
    NumBits += Mp4Enc_OutputBits(vo, gob_num, 5);
    NumBits += Mp4Enc_OutputBits(vo, gfid, 2);
    NumBits += Mp4Enc_OutputBits(vo, gob_quant, 5);

    return NumBits;
}

uint32 Mp4Enc_ReSyncHeader(Mp4EncObject*vo, int32 time_stamp)
{
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    uint32 NumBits = 0;
    uint32 nBitsResyncmarker = 17;
    uint32 hec = 0; //now, not insert header information, 20091120
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    int32 mbnumEnc = vop_mode_ptr->mb_y*vop_mode_ptr->MBNumX/*+pVop_mode->mb_x*/;

#if 0   //VSP has made byte align
    NumBits += Mp4Enc_ByteAlign(pVop_mode->short_video_header);
#endif

    if (PVOP == vop_mode_ptr->VopPredType)
    {
        nBitsResyncmarker += vop_mode_ptr->mvInfoForward.FCode - 1;
    }
    NumBits += Mp4Enc_OutputBits(vo, 1, nBitsResyncmarker); //resync marker
    //only add resync header at the beginning of one MB Line
    //video packet
    NumBits += Mp4Enc_OutputBits(vo, mbnumEnc, vop_mode_ptr->MB_in_VOP_length);//,"macro_block_num"
    NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->StepSize, vop_mode_ptr->QuantPrecision);//,"quant_scale"
    NumBits += Mp4Enc_OutputBits(vo, hec, 1);//,"header_extension_code"

    if(hec)
    {
        int32 modula_time_base;
        int32 vop_time_increment;
        int32 nbits_modula_time_base;

        modula_time_base = time_stamp / (int32)vol_mode_ptr->ClockRate;
        vop_time_increment = time_stamp - modula_time_base * (int32)vol_mode_ptr->ClockRate;

        nbits_modula_time_base = modula_time_base - vo->g_enc_last_modula_time_base;
        NumBits += Mp4Enc_OutputBits(vo, g_msk[nbits_modula_time_base] << 1, nbits_modula_time_base + 1);
        NumBits += Mp4Enc_OutputBits(vo,  (int32) 1, 1);
        NumBits += Mp4Enc_OutputBits(vo, vop_time_increment, vop_mode_ptr->time_inc_resolution_in_vol_length);

        vo->g_enc_last_modula_time_base = modula_time_base;

        NumBits += Mp4Enc_OutputBits(vo, (int32) 1, 1);
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->VopPredType, NUMBITS_VOP_PRED_TYPE);
        NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->IntraDcSwitchThr, 3);

        if(PVOP == vop_mode_ptr->VopPredType)
        {
            NumBits += Mp4Enc_OutputBits(vo, vop_mode_ptr->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */

        }
    }

    return NumBits;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
