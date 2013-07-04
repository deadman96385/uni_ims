/******************************************************************************
 ** File Name:    vp8dec_dequant.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int vp8_dc_quant(int QIndex, int Delta)
{
    int retval;

    QIndex = QIndex + Delta;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = dc_qlookup[ QIndex ];
    return retval;
}

int vp8_dc2quant(int QIndex, int Delta)
{
    int retval;

    QIndex = QIndex + Delta;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = (dc_qlookup[ QIndex ] << 1);
    return retval;

}
int vp8_dc_uv_quant(int QIndex, int Delta)
{
    int retval;

    QIndex = QIndex + Delta;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = dc_qlookup[ QIndex ];

    if (retval > 132)
        retval = 132;

    return retval;
}

int vp8_ac_yquant(int QIndex)
{
    int retval;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = ac_qlookup[ QIndex ];
    return retval;
}

int vp8_ac2quant(int QIndex, int Delta)
{
    int retval;

    QIndex = QIndex + Delta;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = (ac_qlookup[ QIndex ] * 6349) >> 12;

    if (retval < 8)
        retval = 8;

    return retval;
}
int vp8_ac_uv_quant(int QIndex, int Delta)
{
    int retval;

    QIndex = QIndex + Delta;

    if (QIndex > 127)
        QIndex = 127;
    else if (QIndex < 0)
        QIndex = 0;

    retval = ac_qlookup[ QIndex ];
    return retval;
}

void vp8cx_init_de_quantizer(VPXDecObject *vo)
{
    int r, c;
    int i;
    int Q;
    VP8_COMMON *const pc = & vo->common;

    for (Q = 0; Q < QINDEX_RANGE; Q++)
    {
        pc->Y1dequant[Q][0][0] = (short)vp8_dc_quant(Q, pc->y1dc_delta_q);
        pc->Y2dequant[Q][0][0] = (short)vp8_dc2quant(Q, pc->y2dc_delta_q);
        pc->UVdequant[Q][0][0] = (short)vp8_dc_uv_quant(Q, pc->uvdc_delta_q);

        pc->Y1dequant[Q][0][1] = (short)vp8_ac_yquant(Q);
        pc->Y2dequant[Q][0][1] = (short)vp8_ac2quant(Q, pc->y2ac_delta_q);
        pc->UVdequant[Q][0][1] = (short)vp8_ac_uv_quant(Q, pc->uvac_delta_q);
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

