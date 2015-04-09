/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30000 $ $Date: 2014-11-21 14:00:21 +0800 (Fri, 21 Nov 2014) $
 */

#include <audio_config.h>
#include <audio_app.h>

#include <osal.h>
#include "dsp.h"
#include "dsp_extern.h"
#include "dsp_tab.h"

static DspExt_Obj *DspExt_ptr = NULL;

static uint16   DspExt_amrNoData[32] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0x070F,
};

/*
 * ======== _DSPExt_volteReadDataCB() ========
 * This callback function is used to send network data to DSP for decoding
 *
 * Return Values:
 *
 */
void _DSPExt_volteReadDataCB(
    uint16 *pDest,
    uint32 *puiLength,
    void   *param)
{
    OSAL_semAcquire(DspExt_ptr->readMutex, OSAL_WAIT_FOREVER);

    *puiLength = DSPEXT_FRAME_MAX_SZ;

    if (OSAL_TRUE == DspExt_ptr->isRead) {
        /*
         * packet lose or SID no data.
         * If packet lose, repeat the previous data and change FT to 14.
         * If do not receive new RTP frame in SID mode, generate NO-DATA frame.
         */
        if (OSAL_TRUE == DspExt_ptr->isSid) {
            /* NO-DATA */
            OSAL_memCpy(pDest, DspExt_amrNoData, sizeof(DspExt_amrNoData));
            OSAL_semGive(DspExt_ptr->readMutex);
            return;
        }
        else {
            /* packet lose, so change FT to 14 */
            DspExt_ptr->frameXmit_arry[63] = (0x0F & 14);
        }
    }
    OSAL_memCpy(pDest, DspExt_ptr->frameXmit_arry,
            sizeof(DspExt_ptr->frameXmit_arry));

    DspExt_ptr->isRead = OSAL_TRUE;
    OSAL_semGive(DspExt_ptr->readMutex);
}

/*
 * ======== _DSPExt_volteWriteDataCB() ========
 * This callback function is used to get encode data from DSP
 *
 * Return Values:
 *
 */
void _DSPExt_volteWriteDataCB(
    uint16 *pSrc,
    uint32  uiLength,
    void   *param)
{
    OSAL_memCpy(DspExt_ptr->frameRecv_arry, pSrc,
            sizeof(DspExt_ptr->frameRecv_arry));

    OSAL_semGive(DspExt_ptr->frameSem);
}

/*
 * ======== _DSPExt_volteInit() ========
 * This private function is used to init DSP when stream start
 *
 * Return Values:
 *
 */
void _DSPExt_volteInit(
    void)
{
    AUDIO_VOLTE_COMMAND_T tPara = {0};

    tPara.eCommand                      = AUDIO_VOLTE_INIT;
    tPara.uPara.tInit.readdataClkFunc   = _DSPExt_volteReadDataCB;
    tPara.uPara.tInit.writedataClkFunc  = _DSPExt_volteWriteDataCB;
    AUDIO_VOLTE_EnableVoiceCodec(&tPara);
}

/*
 * ======== _DSPExt_volteDeinit() ========
 * This private function is used to de-init DSP when stream end
 *
 * Return Values:
 *
 */
void _DSPExt_volteDeinit(
    void)
{
    AUDIO_VOLTE_COMMAND_T tPara = {0};

    tPara.eCommand = AUDIO_VOLTE_DEINIT;
    AUDIO_VOLTE_EnableVoiceCodec(&tPara);
}

/*
 * ======== _DSPExt_volteStart() ========
 * This private function is used to start DSP when stream starting
 *
 * Return Values:
 *
 */
void _DSPExt_volteStart(
    void)
{
    AUDIO_VOLTE_COMMAND_T tPara = {0};

    tPara.eCommand                      = AUDIO_VOLTE_START;
    /* TODO: This is temp configuration */
    tPara.uPara.tEnableFlag.eEnableFlag = VOLTE_CALL_ENABLE | NORMAL_CALL_ENABLE;
    //tPara.uPara.tEnableFlag.eEnableFlag = VOLTE_CALL_ENABLE;
    AUDIO_VOLTE_EnableVoiceCodec(&tPara);
}

/*
 * ======== _DSPExt_volteStop() ========
 * This private function is used to stop DSP when stream end
 *
 * Return Values:
 *
 */
void _DSPExt_volteStop(
    void)
{
    AUDIO_VOLTE_COMMAND_T tPara = {0};

    tPara.eCommand                      = AUDIO_VOLTE_STOP;
    tPara.uPara.tEnableFlag.eEnableFlag = VOLTE_CALL_ENABLE;
    AUDIO_VOLTE_EnableVoiceCodec(&tPara);
}

/*
 * ======== _DSPExt_volteParaSet() ========
 * This private function is used to set parameters of DSP.
 *
 * Return Values:
 *
 */
void _DSPExt_volteParaSet(
    DSPExt_CoderType    type,
    int                 ft,
    int                 vadEnable)
{
    AUDIO_VOLTE_COMMAND_T tPara = {0};

    tPara.eCommand = AUDIO_VOLTE_PARASET;
    if ((DSPEXT_CODER_TYPE_AMRNB_OA == type) ||
            (DSPEXT_CODER_TYPE_AMRNB_BE == type)) {
        tPara.uPara.tParaSet.uiformat = 0;
    }
    else if ((DSPEXT_CODER_TYPE_AMRWB_OA == type) ||
            (DSPEXT_CODER_TYPE_AMRWB_BE == type)) {
        tPara.uPara.tParaSet.uiformat = 1;
    }
    else {
        return;
    }
    /*0: 4.75kbps_for_amrnb/6.6kbps_for_amrwb*/
    tPara.uPara.tParaSet.uiFrameTypeIndex = ft;
    tPara.uPara.tParaSet.dtxFlag          = vadEnable;
    AUDIO_VOLTE_EnableVoiceCodec(&tPara);
    /* Speaker mode */
    AUDIO_SetDevMode(AUDIO_DEVICE_MODE_HANDFREE);
}

/*
 * ======== _DSPExt_gamrPackBitsOA() ========
 * This function is used to pack AMR packet.
 *
 * Return Values:
 *  packet length
 */
vint _DSPExt_gamrPackBitsOA(
    DSPExt_CoderType    coderType,
    uint8              *src_ptr,
    uint8              *dst_ptr)
{
    vint    coder, txType, ft, i, posBit;
    uint8   toc, temp;

    /* Check amrwb or amrnb */
    if (DSPEXT_CODER_TYPE_AMRNB_OA == coderType) {
        coder = 0;
    }
    else {
        coder = 1;
    }

    txType = src_ptr[63] & 0x0F;

    /* XXX:
     * DSPEXT_TX_TYPE_ONSET,
     * DSPEXT_TX_TYPE_N_FRAMETYPES,
     * DSPEXT_TX_TYPE_SPEECH_LOST
     */
    if (DSPEXT_TX_TYPE_NO_DATA == txType) {
        if (DSPEXT_CODER_TYPE_AMRNB_OA == coderType) {
            ft = DSPEXT_AMRNB_FRAME_TYPE_NO_DATA;
        }
        else {
            ft = DSPEXT_AMRWB_FRAME_TYPE_NO_DATA;
        }
    }
    else if ((DSPEXT_TX_TYPE_SID_FIRST == txType) ||
            (DSPEXT_TX_TYPE_SID_UPDATA == txType) ||
            (DSPEXT_TX_TYPE_SID_BAD == txType)) {
        if (DSPEXT_CODER_TYPE_AMRNB_OA == coderType) {
            ft = DSPEXT_AMRNB_FRAME_TYPE_AMR_SID;
        }
        else {
            ft = DSPEXT_AMRWB_FRAME_TYPE_SID;
        }
    }
    else {
        ft = (src_ptr[63] >> 4) & 0x0F;
    }

    /* Get ToC */
    toc = 0;
    if ((DSPEXT_TX_TYPE_SID_BAD == txType) ||
            (DSPEXT_TX_TYPE_SPEECH_BAD == txType)) {
        /* Q is 0 */
        toc = ft << 3;
    }
    else {
        toc = ((ft << 3) | 0x04);
    }

    /* TS 26.201 define speech lose for AMR-WB, FT is 14 and Q is 0 */
    if ((DSPEXT_CODER_TYPE_AMRWB_OA == coderType) &&
            (DSPEXT_TX_TYPE_SPEECH_LOST == txType)) {
        ft = DSPEXT_AMRWB_FRAME_TYPE_SPEECH_LOST;
        /* Q is 0 */
        toc = ft << 3;
    }

    /* Insert default CMR(no mode request). */
    *dst_ptr = 0xF0;
    dst_ptr++;

    /* insert table of contents (ToC) byte at the beginning of the frame */
    *dst_ptr = toc;
    dst_ptr++;

    /* note that NO_DATA frames do not need further processing */
    if ((DSPEXT_CODER_TYPE_AMRNB_OA == coderType) &&
            (DSPEXT_AMRNB_FRAME_TYPE_NO_DATA == ft)) {
        return (2);
    }
    if ((DSPEXT_CODER_TYPE_AMRWB_OA == coderType) &&
            ((DSPEXT_AMRWB_FRAME_TYPE_NO_DATA == ft) ||
            (DSPEXT_AMRWB_FRAME_TYPE_SPEECH_LOST == ft))) {
        return (2);
    }

    temp = 0;
    /* sort and pack speech bits */
    for (i = 1; i < gamr_bit_size[coder][ft] + 1; i++) {
        posBit  = gamr_sort_ptr[coder][ft][i - 1];
        if ((src_ptr[posBit / 8] << (posBit % 8)) & 0x80) {
            temp++;
        }

        if (i % 8) {
            temp <<= 1;
        }
        else {
            *dst_ptr = temp;
            dst_ptr++;
            temp = 0;
        }
    }

    /* insert SID type indication and speech mode in case of SID frame */
    if ((DSPEXT_CODER_TYPE_AMRNB_OA == coderType) &&
            (DSPEXT_AMRNB_FRAME_TYPE_AMR_SID == ft)) {
        if (DSPEXT_TX_TYPE_SID_UPDATA == txType) {
            temp++;
        }
        /* AMR-NB */
        temp <<= 3;
        temp += ((DspExt_ptr->rateMode & 0x4) >> 2) |
                (DspExt_ptr->rateMode & 0x2) |
                ((DspExt_ptr->rateMode & 0x1) << 2);
        temp <<= 1;
    }
    else if ((DSPEXT_CODER_TYPE_AMRWB_OA == coderType) &&
            (DSPEXT_AMRWB_FRAME_TYPE_SID == ft)) {
        if (DSPEXT_TX_TYPE_SID_UPDATA == txType) {
            temp++;
        }
        /* AMR-WB */
        temp <<= 4;
        temp += DspExt_ptr->rateMode;
    }

    /* insert unused bits (zeros) at the tail of the last byte */
    temp <<= (gamr_unused_size_oa[coder][ft] - 1);
    *dst_ptr = temp;

    /* Size including CMR. */
    return (gamr_packed_size_oa[coder][ft]);
}

/*
 * ======== _DSPExt_gamrPackBitsBE() ========
 * This function is used to pack AMR packet.
 *
 * Return Values:
 *  packet length
 */
vint _DSPExt_gamrPackBitsBE(
    DSPExt_CoderType    coderType,
    uint8              *src_ptr,
    uint8              *dst_ptr)
{
    vint    coder, txType, ft, i, posBit;
    uint8   toc, temp;

    /* check amrwb or amrnb */
    if (DSPEXT_CODER_TYPE_AMRNB_BE == coderType) {
        coder = 0;
    }
    else {
        coder = 1;
    }

    txType = src_ptr[63] & 0x0F;

    /* XXX:
     * DSPEXT_TX_TYPE_ONSET,
     * DSPEXT_TX_TYPE_N_FRAMETYPES,
     * DSPEXT_TX_TYPE_SPEECH_LOST
     */
    if (DSPEXT_TX_TYPE_NO_DATA == txType) {
        if (DSPEXT_CODER_TYPE_AMRNB_BE == coderType) {
            ft = DSPEXT_AMRNB_FRAME_TYPE_NO_DATA;
        }
        else {
            ft = DSPEXT_AMRWB_FRAME_TYPE_NO_DATA;
        }
    }
    else if ((DSPEXT_TX_TYPE_SID_FIRST == txType) ||
            (DSPEXT_TX_TYPE_SID_UPDATA == txType) ||
            (DSPEXT_TX_TYPE_SID_BAD == txType)) {
        if (DSPEXT_CODER_TYPE_AMRNB_BE == coderType) {
            ft = DSPEXT_AMRNB_FRAME_TYPE_AMR_SID;
        }
        else {
            ft = DSPEXT_AMRWB_FRAME_TYPE_SID;
        }
    }
    else {
        ft = (src_ptr[63] >> 4) & 0x0F;
    }

    /* Get ToC */
    toc = 0;
    if ((DSPEXT_TX_TYPE_SID_BAD == txType) ||
            (DSPEXT_TX_TYPE_SPEECH_BAD == txType)) {
        /* Q is 0 */
        toc = ft << 3;
    }
    else {
        toc = ((ft << 3) | 0x04);
    }

    /* TS 26.201 define speech lose for AMR-WB, FT is 14 and Q is 0 */
    if ((DSPEXT_CODER_TYPE_AMRWB_BE == coderType) &&
            (DSPEXT_TX_TYPE_SPEECH_LOST == txType)) {
        ft = DSPEXT_AMRWB_FRAME_TYPE_SPEECH_LOST;
        /* Q is 0 */
        toc = ft << 3;
    }

    /* Insert default CMR(no mode request) and TOC */
    *dst_ptr = 0xF0 | (toc >> 4);
    dst_ptr++;

    /* note that NO_DATA frames do not need further processing */
    if ((DSPEXT_CODER_TYPE_AMRNB_OA == coderType) &&
            (DSPEXT_AMRNB_FRAME_TYPE_NO_DATA == ft)) {
        return (2);
    }
    if ((DSPEXT_CODER_TYPE_AMRWB_OA == coderType) &&
            ((DSPEXT_AMRWB_FRAME_TYPE_NO_DATA == ft) ||
            (DSPEXT_AMRWB_FRAME_TYPE_SPEECH_LOST == ft))) {
        return (2);
    }

    /* Get the rest of TOC. */
    temp = (toc & 0x0C) >> 1;

    /* sort and pack speech bits */
    for (i = 3; i < gamr_bit_size[coder][ft] + 3; i++) {
        posBit  = gamr_sort_ptr[coder][ft][i - 3];
        if ((src_ptr[posBit / 8] << (posBit % 8)) & 0x80) {
            temp++;
        }

        if (i % 8) {
            temp <<= 1;
        }
        else {
            *dst_ptr = temp;
            dst_ptr++;
            temp = 0;
        }
    }

    /* insert SID type indication and speech mode in case of SID frame */
    if ((DSPEXT_CODER_TYPE_AMRNB_BE == coderType) &&
            (DSPEXT_AMRNB_FRAME_TYPE_AMR_SID == ft)) {
        if (DSPEXT_TX_TYPE_SID_UPDATA == txType) {
            temp++;
        }
        /* Now we already packed 4+6+35+1=46 bits. */
        temp <<= 2;

        /* AMR-NB 3 bits mode */
        temp += ((DspExt_ptr->rateMode & 0x2) |
                ((DspExt_ptr->rateMode & 0x1) << 2));
        *dst_ptr = temp;
        dst_ptr++;
        temp = ((DspExt_ptr->rateMode & 0x4) >> 2);
        temp <<= 1;
    }
    else if ((DSPEXT_CODER_TYPE_AMRWB_BE == coderType) &&
            (DSPEXT_AMRWB_FRAME_TYPE_SID == ft)) {
        if (DSPEXT_TX_TYPE_SID_UPDATA == txType) {
            temp++;
        }
        /* Now we already packed 4+6+35+1=46 bits. */
        temp <<= 2;

        /* AMR-WB 4 bits mode */
        temp += (DspExt_ptr->rateMode >> 2);
        *dst_ptr = temp;
        dst_ptr++;
        temp = (DspExt_ptr->rateMode & 0x3);
    }

    /* insert unused bits (zeros) at the tail of the last byte */
    temp <<= (gamr_unused_size_be[coder][ft] - 1);
    *dst_ptr = temp;

    /* Size including CMR. */
    return (gamr_packed_size_be[coder][ft]);
}

/*
 * ======== _DSPExt_gamrUnpackBitsOA() ========
 * This function is used to unpack AMR packet.
 *
 * Return Values:
 *
 */
DSPExt_RxType _DSPExt_gamrUnpackBitsOA(
    DSPExt_CoderType    coderType,
    uint8              *pktBits_ptr,
    uint8              *dst_ptr,
    uint8              *ft_ptr)
{
    uint16  i, posByte, posBit;
    uint8  *pkt_ptr, tmp;
    uint8   q, cmr, ftSid, ftNodata;
    vint    coder;

    /* Check amrwb or amrnb */
    if (DSPEXT_CODER_TYPE_AMRNB_OA == coderType) {
        coder = 0;
        ftSid = DSPEXT_AMRNB_FRAME_TYPE_AMR_SID;
        ftNodata = DSPEXT_AMRNB_FRAME_TYPE_NO_DATA;
    }
    else {
        coder = 1;
        ftSid = DSPEXT_AMRWB_FRAME_TYPE_SID;
        ftNodata = DSPEXT_AMRWB_FRAME_TYPE_NO_DATA;
    }

    pkt_ptr = pktBits_ptr;

    /* 1st byte CMR */
    tmp = *(pkt_ptr);
    cmr = ((tmp >> 4) & 0x0F);
    if ((cmr < ftSid) && (cmr != DspExt_ptr->rateMode)) {
        /* Change coder */
        _DSPExt_volteParaSet(coderType, cmr, DspExt_ptr->vadEnable);
        DspExt_ptr->rateMode = cmr;
    }

    /* 2nd byte ToC */
    tmp = *(pkt_ptr + 1);
    pkt_ptr += 2;

    /* Get frame type and quality value */
    *ft_ptr = (tmp >> 3) & 0x0F;
    q       = (tmp >> 2) & 0x01;

    /* real NO_DATA frame or unspecified frame type */
    if ((ftNodata == *ft_ptr) || (*ft_ptr > ftSid && *ft_ptr < ftNodata)) {
        /* NO-DATA */
        return (DSPEXT_RX_TYPE_NO_DATA);
    }

    /* order speech data */
    tmp = *pkt_ptr;
    pkt_ptr++;

    for (i = 1; i < (gamr_bit_size[coder][*ft_ptr] + 1); i++) {
        posBit  = gamr_sort_ptr[coder][*ft_ptr][i - 1];
        posByte = (posBit / 8);
        posBit  = 7 - (posBit % 8);
        if (tmp & 0x80) {
            dst_ptr[posByte] |= (1 << posBit);
        }
        else {
            dst_ptr[posByte] &= ~(1 << posBit);
        }

        if (i % 8) {
            tmp <<= 1;
        }
        else {
            tmp = *pkt_ptr;
            pkt_ptr++;
        }
    }

    if (ftSid == *ft_ptr) {
        /* receive SID */
        DspExt_ptr->isSid = OSAL_TRUE;

        if (q) {
            /* Get SID type */
            if (tmp & 0x80) {
                return (DSPEXT_RX_TYPE_UPDATA);
            }
            else {
                return (DSPEXT_RX_TYPE_SID_FIRST);
            }
        }
        else {
            return (DSPEXT_RX_TYPE_SID_BAD);
        }
    }
    else {
        DspExt_ptr->isSid = OSAL_FALSE;
        if (q) {
            /* speech good */
            return (DSPEXT_RX_TYPE_SPEECH_GOOD);
        }
        else {
            /* speech bad */
            return (DSPEXT_RX_TYPE_SPEECH_BAD);
        }
    }
}

/*
 * ======== _DSPExt_gamrUnpackBitsBE() ========
 * This function is used to unpack AMR packet.
 *
 * Return Values:
 *
 */
DSPExt_RxType _DSPExt_gamrUnpackBitsBE(
    DSPExt_CoderType    coderType,
    uint8              *pktBits_ptr,
    uint8              *dst_ptr,
    uint8              *ft_ptr)
{
    uint16  i, posByte, posBit;
    uint8  *pkt_ptr, tmp;
    uint8   q, cmr, ftSid, ftNodata;
    vint    coder;

    /* Check amrwb or amrnb */
    if (DSPEXT_CODER_TYPE_AMRNB_BE == coderType) {
        coder = 0;
        ftSid = DSPEXT_AMRNB_FRAME_TYPE_AMR_SID;
        ftNodata = DSPEXT_AMRNB_FRAME_TYPE_NO_DATA;
    }
    else {
        coder = 1;
        ftSid = DSPEXT_AMRWB_FRAME_TYPE_SID;
        ftNodata = DSPEXT_AMRWB_FRAME_TYPE_NO_DATA;
    }

    pkt_ptr = pktBits_ptr;

    /* 1st byte CMR */
    tmp = *(pkt_ptr);
    cmr = ((tmp >> 4) & 0x0F);
    if ((cmr < ftSid) && (cmr != DspExt_ptr->rateMode)) {
        /* Change coder */
        _DSPExt_volteParaSet(coderType, cmr, DspExt_ptr->vadEnable);
        DspExt_ptr->rateMode = cmr;
    }

    /*
     * First 4 bits is CMR. 
     * read rest of the frame based on ToC byte
     */
    q       = (*(pkt_ptr + 1) >> 6) & 0x01;
    *ft_ptr = ((*(pkt_ptr) & 0x7) << 1) | (*(pkt_ptr + 1) >> 7);


    /* real NO_DATA frame or unspecified frame type */
    if ((ftNodata == *ft_ptr) || (*ft_ptr > ftSid && *ft_ptr < ftNodata)) {
        /* NO-DATA */
        return (DSPEXT_RX_TYPE_NO_DATA);
    }

    /* order speech data */
    pkt_ptr++;
    tmp = *pkt_ptr;
    pkt_ptr++;
    /* First two bits are parts of ToC, start from the third bit. */
    tmp <<= 2;

    for (i = 3; i < (gamr_bit_size[coder][*ft_ptr] + 3); i++) {
        posBit  = gamr_sort_ptr[coder][*ft_ptr][i - 3];
        posByte = (posBit / 8);
        posBit  = 7 - (posBit % 8);
        if (tmp & 0x80) {
            dst_ptr[posByte] |= (1 << posBit);
        }
        else {
            dst_ptr[posByte] &= ~(1 << posBit);
        }

        if (i % 8) {
            tmp <<= 1;
        }
        else {
            tmp = *pkt_ptr;
            pkt_ptr++;
        }
    }

    if (ftSid == *ft_ptr) {
        /* receive SID */
        DspExt_ptr->isSid = OSAL_TRUE;

        if (q) {
            /* Get SID type */
            if (tmp & 0x80) {
                return (DSPEXT_RX_TYPE_UPDATA);
            }
            else {
                return (DSPEXT_RX_TYPE_SID_FIRST);
            }
        }
        else {
            return (DSPEXT_RX_TYPE_SID_BAD);
        }
    }
    else {
        DspExt_ptr->isSid = OSAL_FALSE;
        if (q) {
            /* speech good */
            return (DSPEXT_RX_TYPE_SPEECH_GOOD);
        }
        else {
            /* speech bad */
            return (DSPEXT_RX_TYPE_SPEECH_BAD);
        }
    }
}

/*
 * ======== DSPExt_init() ========
 * Initialize the DSP External module
 *
 * Return Values:
 *  DSP_OK: successful init
 *  DSP_ERR: fail init
 *
 */
DSPExt_Return DSPExt_init(
    void)
{
    DspExt_ptr = OSAL_memCalloc(1, sizeof(DspExt_Obj), 0);
    OSAL_memSet(DspExt_ptr, 0, sizeof(DspExt_Obj));

    DspExt_ptr->frameSem  = OSAL_semCountCreate(0);
    DspExt_ptr->readMutex = OSAL_semMutexCreate();

    OSAL_logMsg("%s:%d Done.\n", __FUNCTION__, __LINE__);
    return (DSPEXT_OK);
}

/*
 * ======== DSPExt_shudown() ========
 * DSP External module shutdown
 *
 * Return Values:
 * NONE
 *
 */
void DSPExt_shutdown(
    void)
{
    OSAL_semDelete(DspExt_ptr->frameSem);
    OSAL_semDelete(DspExt_ptr->readMutex);

    OSAL_memFree(DspExt_ptr, 0);
    OSAL_logMsg("%s:%d Done.\n", __FUNCTION__, __LINE__);
}

/*
 * ======== DSPExt_codecStart() ========
 *
 * Start external DSP
 *
 * Return:
 *  DSPEXT_ERR: failed.
 *  DSPEXT_OK:  success.
 *
 */
DSPExt_Return DSPExt_codecStart(
    DSPExt_CoderType    coderType,
    int                 rateMode,
    int                 vadEnable)
{
    /* cache the rate mode and vad */
    DspExt_ptr->rateMode    = rateMode;
    DspExt_ptr->vadEnable   = vadEnable;

    if ((DSPEXT_CODER_TYPE_AMRNB_OA != coderType) &&
            (DSPEXT_CODER_TYPE_AMRNB_BE != coderType) &&
            (DSPEXT_CODER_TYPE_AMRWB_OA != coderType) &&
            (DSPEXT_CODER_TYPE_AMRWB_BE != coderType)) {
        OSAL_logMsg("%s:%d Unsupport coder type(%d).",
                __FUNCTION__, __LINE__, coderType);
        return (DSPEXT_ERR);
    }

    if (OSAL_TRUE != DspExt_ptr->isCodecStart) {
        /* init and start dsp */
        _DSPExt_volteInit();
        _DSPExt_volteParaSet(coderType, rateMode, vadEnable);
        _DSPExt_volteStart();
    }
    else {
        if (DspExt_ptr->currCoderType == coderType) {
            OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);
            return (DSPEXT_OK);
        }
        /* Change coder */
        _DSPExt_volteParaSet(coderType, rateMode, vadEnable);
    }

    /* Set gobal obj parameters */
    DspExt_ptr->isCodecStart  = OSAL_TRUE;
    DspExt_ptr->currCoderType = coderType;
    DspExt_ptr->app10msflag   = 0;
    DspExt_ptr->isRead        = OSAL_FALSE;

    /* Init as no data frame */
    OSAL_memCpy(DspExt_ptr->frameXmit_arry, DspExt_amrNoData,
            sizeof(DspExt_ptr->frameXmit_arry));

    OSAL_logMsg("%s:%d Done.\n", __FUNCTION__, __LINE__);

    return (DSPEXT_OK);
}

/*
 * ======== DSPExt_codecStop() ========
 *
 * Stop external DSP
 *
 * Return:
 *  DSPEXT_ERR: failed.
 *  DSPEXT_OK:  success.
 *
 */
DSPExt_Return DSPExt_codecStop(
    void)
{
    _DSPExt_volteStop();
    _DSPExt_volteDeinit();

    DspExt_ptr->currCoderType = DSPEXT_CODER_TYPE_UNKNOWN;
    DspExt_ptr->isCodecStart  = OSAL_FALSE;

    OSAL_logMsg("%s:%d Done.\n", __FUNCTION__, __LINE__);
    return (DSPEXT_OK);
}

/*
 * ======== DSPExt_Encode() ========
 *
 * Perform DSP encode function
 *
 * Return:
 *    block length.
 */
int DSPExt_Encode(
    DSPExt_CoderType    coderType,
    uint8              *dst_ptr)
{
    vint    i;
    uint8   tmp;

    if (0 == DspExt_ptr->app10msflag) {
        DspExt_ptr->app10msflag = 1;
        return (0);
    }
    else {
        DspExt_ptr->app10msflag = 0;
        /*
         * Wait for MVS blocking 20ms read to complete.
         */
        if (OSAL_SUCCESS != OSAL_semAcquire(DspExt_ptr->frameSem,
                OSAL_WAIT_FOREVER)) {
            OSAL_logMsg("ERROR semAquire() FAIL\n");
        }

        /* swap byte order, because memory copy from uint16 to uint8. */
        for (i = 0; i < DSPEXT_FRAME_MAX_SZ; i += 2) {
            tmp = DspExt_ptr->frameRecv_arry[i];
            DspExt_ptr->frameRecv_arry[i] = DspExt_ptr->frameRecv_arry[i + 1];
            DspExt_ptr->frameRecv_arry[i + 1] = tmp;
        }

        if ((DSPEXT_CODER_TYPE_AMRNB_OA == coderType) ||
                (DSPEXT_CODER_TYPE_AMRWB_OA == coderType)) {
            return (_DSPExt_gamrPackBitsOA(coderType,
                    DspExt_ptr->frameRecv_arry, dst_ptr));
        }
        else {
            return (_DSPExt_gamrPackBitsBE(coderType,
                    DspExt_ptr->frameRecv_arry, dst_ptr));
        }
    }
}

/*
 * ======== DSPExt_Decode() ========
 *
 * Perform DSP decode function
 *
 * Return:
 */
void DSPExt_Decode(
    DSPExt_CoderType    coderType,
    uint8              *src_ptr,
    uvint               pSize)
{
    DSPExt_RxType   rxType;
    vint            i;
    uint8           ft, tmp;

    if ((DSPEXT_CODER_TYPE_AMRNB_OA != coderType) &&
            (DSPEXT_CODER_TYPE_AMRNB_BE != coderType) &&
            (DSPEXT_CODER_TYPE_AMRWB_OA != coderType) &&
            (DSPEXT_CODER_TYPE_AMRWB_BE != coderType)) {
        OSAL_logMsg("%s:%d Unsupport coder type(%d).",
                __FUNCTION__, __LINE__, coderType);
        return;
    }

    OSAL_semAcquire(DspExt_ptr->readMutex, OSAL_WAIT_FOREVER);

    if (0 != pSize) {
        /* Get frame type and rx type */
        if ((DSPEXT_CODER_TYPE_AMRNB_OA == coderType) ||
                (DSPEXT_CODER_TYPE_AMRWB_OA == coderType)) {
            rxType = _DSPExt_gamrUnpackBitsOA(coderType, src_ptr,
                    DspExt_ptr->frameXmit_arry, &ft);
        }
        else {
            rxType = _DSPExt_gamrUnpackBitsBE(coderType, src_ptr,
                    DspExt_ptr->frameXmit_arry, &ft);
        }

        /* Fill in FT */
        DspExt_ptr->frameXmit_arry[62] = (0x0F & ft);
        /* Fill in Enc_rate */
        DspExt_ptr->frameXmit_arry[63] = (DspExt_ptr->rateMode << 4);
        /* Fill in RX_Type */
        DspExt_ptr->frameXmit_arry[63] &= (0xF0);
        DspExt_ptr->frameXmit_arry[63] |= ((uint8)rxType & 0x0F);

        if (DSPEXT_RX_TYPE_NO_DATA == rxType) {
            for (i = 0; i < 61; i++) {
                DspExt_ptr->frameXmit_arry[i] = 0;
            }
        }
        DspExt_ptr->isRead = OSAL_FALSE;

        /* swap byte order, because it will memory copy from uint8 to uint16. */
        for (i = 0; i < DSPEXT_FRAME_MAX_SZ; i += 2) {
            tmp = DspExt_ptr->frameXmit_arry[i];
            DspExt_ptr->frameXmit_arry[i] = DspExt_ptr->frameXmit_arry[i+1];
            DspExt_ptr->frameXmit_arry[i + 1] = tmp;
        }

    }
    OSAL_semGive(DspExt_ptr->readMutex);
}

