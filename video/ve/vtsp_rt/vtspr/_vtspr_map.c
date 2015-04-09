/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */

/*
 * This file contains helper functions for interface, channel, and stream
 * number mapping.  These mappings are for between software layers and between
 * hardware-software interface.
 *
 * DTMF-digit to DTMF-enumeration mapping is also here.
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_infcToType() ========
 *
 * Convert interface index into audio channel index
 *
 * for DM1 Si:
 *  0 -> fxs
 *  1 -> fxo
 *
 * for DM1 Leg:
 *  0 -> fxs
 *  1 -> fxo
 *
 * for DM2 Inf:
 *  0 -> fxs #1
 *  1 -> fxs #2
 *  2 -> fxo
 *
 */
OSAL_INLINE vint _VTSPR_infcToType(
    uvint infc)
{
    /* cast to vint because LAST can be negative */
    if ((vint)infc <= _VTSP_INFC_FXS_LAST) {
        return (VTSPR_INFC_TYPE_FXS);
    }
    else if ((vint)infc <= _VTSP_INFC_FXO_LAST) { 
        return (VTSPR_INFC_TYPE_FXO);
    }
    else if ((vint)infc <= _VTSP_INFC_AUDIO_LAST) { 
        return (VTSPR_INFC_TYPE_AUDIO);
    }

    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return (0);
}

OSAL_INLINE vint _VTSPR_infcToTicInfcType(
    uvint infc)
{
    /* cast to vint because LAST can be negative */
    if ((vint)infc <= _VTSP_INFC_FXS_LAST) {
        return (TIC_INFCTYPE_FXS);
    }
    else if ((vint)infc <= _VTSP_INFC_FXO_LAST) { 
        return (TIC_INFCTYPE_FXO);
    }
    else if ((vint)infc <= _VTSP_INFC_AUDIO_LAST) { 
        return (TIC_INFCTYPE_AUDIO);
    }

    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return (0);
}

/*
 * ======== _VTSPR_infcToInfcPtr() ========
 *
 * XXX May require porting for new platforms
 */
OSAL_INLINE void *_VTSPR_infcToInfcPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc)
{
    vint    fxId;

    /* cast to vint because LAST can be negative */
#if _VTSP_INFC_FXS_NUM > 0
    if ((vint)infc <= _VTSP_INFC_FXS_LAST) { 
        /* is FXS */
        fxId = infc;
        return (&dsp_ptr->fxsInfc[fxId]);
    }
#endif
#if _VTSP_INFC_FXO_NUM > 0
    if ((vint)infc <= _VTSP_INFC_FXO_LAST) { 
        /* is FXO */
        fxId = infc - _VTSP_INFC_FXO_FIRST;
        return (&dsp_ptr->fxoInfc[fxId]);
    }
#endif
#if _VTSP_INFC_AUDIO_NUM > 0
    if ((vint)infc <= _VTSP_INFC_AUDIO_LAST) {
        /* is AUDIO */
        fxId = infc - _VTSP_INFC_AUDIO_FIRST;
        return (&dsp_ptr->audioInfc[fxId]);
    }
#endif
    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return (0);
}


/*
 * ======== _VTSPR_infcToTicPtr() ========
 *
 */
OSAL_INLINE void *_VTSPR_infcToTicPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc)
{
    VTSPR_FxsInfcObj    *fxs_ptr;
    VTSPR_FxoInfcObj    *fxo_ptr;
    VTSPR_AudioInfcObj  *audio_ptr;
    void                *infc_ptr;

    infc_ptr = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
    if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) { 
        fxs_ptr = infc_ptr;
        return (&fxs_ptr->fxs);
    }
    if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) { 
        fxo_ptr = infc_ptr;
        return (&fxo_ptr->fxo);
    }
    if (VTSPR_INFC_TYPE_AUDIO == _VTSPR_infcToType(infc)) { 
        audio_ptr = infc_ptr;
        return (&audio_ptr->audio);
    }
    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return (0);
}

/*
 * ======== _VTSPR_infcToChan() ========
 *
 * Convert interface index into audio channel index
 * Future interfaces may have multiple audio channels
 *
 * for DM1 Si:
 *  infc 0 -> chan 0
 *  infc 1 -> chan 1
 *
 * for DM1 Leg:
 *  infc 0 -> chan 0
 *  infc 1 -> chan 1
 *
 * for DM2 Inf:
 *  infc 0 -> chan 0 (fxs)
 *  infc 1 -> chan 1 (fxs)
 *  infc 2 -> chan 2 (fxo)
 *
 * derek1 infc:
 *  infc 0 -> chan 0 (fxs)
 *  infc 1 -> chan 1 (fxs)
 *  infc 2 -> chan 2 (fxo)
 */
OSAL_INLINE uvint _VTSPR_infcToChan(
    uvint infc)
{
    uvint chan;

    /* cast to vint because LAST can be negative */
    if ((vint)infc <= _VTSP_INFC_FXS_LAST) { 
        /* is FXS */
        chan = infc;
        return (chan);
    }
#ifdef VTSP_ENABLE_FXO
    if ((vint)infc <= _VTSP_INFC_FXO_LAST) { 
        /* is FXO */
        chan = infc;
        return (chan);
    }
#endif
    if ((vint)infc <= _VTSP_INFC_AUDIO_LAST) { 
        /* is AUDIO */
        chan = infc;
        return (chan);
    }

    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return (0);

}

/*
 * ======== _VTSPR_infcToChanPtr() ========
 *
 * XXX May require porting for new platforms
 */
OSAL_INLINE void *_VTSPR_infcToChanPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc)
{
    void   *chan_ptr;
    vint    chan;

    /* For channel obj, FXS & FXO are handled the same way */
    chan = _VTSPR_infcToChan(infc);
    chan_ptr = &dsp_ptr->chan[chan];

    return (chan_ptr);

}

/*
 * ======== _VTSPR_dtmfToChar() ========
 *
 * Convert DTMF enumeration into ascii char [0123456789*#ABCD], in that order
 *
 * See DTMF algorithm manual page
 */
unsigned char _VTSPR_dtmfToChar(
    uvint dig)
{
    /* common case */
    if (dig < 10) { 
        return (dig + '0');
    }

    if (10 == dig) { 
        return '*';
    }

    if (11 == dig) { 
        return '#';
    }

    if (dig < 16) {
        return dig - 12 + 'A';
    }

    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return 0;
}

/*
 * ======== _VTSPR_charToDtmf() ========
 *
 * Convert ascii char [0123456789*#ABCD] into index, in that order
 *
 * See DTMF algorithm manual page
 */
vint _VTSPR_charToDtmf(
    unsigned char c)
{
    uvint index;

    index = c - '0';
    if (index <= 9) { 
        /* Most common case */
        return index;
    }

    /* other common cases */
    if ('*' == c) { 
        return 10;
    }
    else if ('#' == c) { 
        return 11;
    }

    /* least common case */
    index = c - 'A';
    if (index <= ('D' - 'A')) { 
        return index + 12;
    }

    /* Should never get here */
    _VTSP_TRACE(__FILE__, __LINE__);
    return 0;
}

/*
 * ======== _VTSPR_localToDynamicEncoder() ========
 *
 * Convert a local encoder type to a dynamic encoder type for 
 * given stream param.
 */
uvint _VTSPR_localToDynamicEncoder(
    VTSP_Stream *streamParam_ptr,
    vint         local)
{
    if (local >= VTSP_ENCODER_NUM) {
        /* Should never get here */
        _VTSP_TRACE(__FILE__, __LINE__);
        return (0);
    }
    return (streamParam_ptr->encodeType[local]);
    
}

/*
 * ======== _VTSPR_dynamicToLocalDecoder() ========
 *
 * Convert a dynamic decoder type to a local decoder type for 
 * given stream param.
 */
uvint _VTSPR_dynamicToLocalDecoder(
    VTSP_Stream *streamParam_ptr,
    vint         dynamic)
{
    vint  i;
    vint  count;
    vint *coder_ptr;
    
    coder_ptr = streamParam_ptr->decodeType;
    count     = VTSP_DECODER_NUM;
    
    /*
     * Now map dynamic type to local type.
     */
    for (i = 0; i < count; i++, coder_ptr++) {
        if (*coder_ptr == dynamic) {
            return (i);
        }
    }
    return (VTSP_CODER_CN);
}

/*
 * ======== _VTSPR_jbCoderToLocalCoder() ========
 *
 * Convert a JB coder type to a local coder type.
 */
uvint _VTSPR_jbCoderToLocalCoder(
    JB_Payload coder,
    vint       payloadSize)
{
    /*
     * Map. Return VTSP_CODER_UNAVAIL for unknown coder, or the one that cannot
     * be mapped.
     */
    switch (coder) {
        case JB_PT_G711U:
            return (VTSP_CODER_G711U);
        case JB_PT_G726_32:
            return (VTSP_CODER_G726_32K);
        case JB_PT_G711A:
            return (VTSP_CODER_G711A);
#ifdef VTSP_ENABLE_16K_MU            
        case JB_PT_16K_MU:
            return (VTSP_CODER_16K_MU);
#endif
#ifdef VTSP_ENABLE_G722            
        case JB_PT_G722_0:
            return (VTSP_CODER_G722);
#endif
#ifdef VTSP_ENABLE_G722P1            
        case JB_PT_G722_1:
            return (VTSP_CODER_G722P1_20MS);
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
        case JB_PT_GAMRNB_OA:
            return (VTSP_CODER_GAMRNB_20MS_OA);
        case JB_PT_GAMRNB_BE:
            return (VTSP_CODER_GAMRNB_20MS_BE);
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
        case JB_PT_GAMRWB_OA:
            return (VTSP_CODER_GAMRWB_20MS_OA);
        case JB_PT_GAMRWB_BE:
            return (VTSP_CODER_GAMRWB_20MS_BE);
#endif
#ifdef VTSP_ENABLE_SILK
        case JB_PT_SILK_8K:
            return (VTSP_CODER_SILK_20MS_8K);

        case JB_PT_SILK_16K:
            return (VTSP_CODER_SILK_20MS_16K);

        case JB_PT_SILK_24K:
            return (VTSP_CODER_SILK_20MS_24K);            
#endif
#ifdef VTSP_ENABLE_G711P1
        case JB_PT_G711P1U:
            return (VTSP_CODER_G711P1U);
            
        case JB_PT_G711P1A:
            return (VTSP_CODER_G711P1A);
#endif 

        case JB_PT_CNSE:
            return (VTSP_CODER_CN);
        case JB_PT_G729:
            return (VTSP_CODER_G729);
        case JB_PT_DTRLY: 
            return (VTSP_CODER_DTMF);
        case JB_PT_TONERLY: 
            return (VTSP_CODER_TONE);
        case JB_PT_G723:
            return (VTSP_CODER_G723_30MS);
#ifdef VTSP_ENABLE_ILBC
        case JB_PT_ILBC:
            if (VTSP_BLOCK_ILBC_20MS_SZ == payloadSize) {
                return (VTSP_CODER_ILBC_20MS);
            }
            return (VTSP_CODER_ILBC_30MS);
#endif
        default:
            return (VTSP_CODER_UNAVAIL);
    }
}

/*
 * ======== _VTSPR_localCoderToJbCoder() ========
 *
 * Convert a JB coder type to a local coder type.
 */
JB_Payload _VTSPR_localCoderToJbCoder(
    vint coder)
{
    /*
     * Map. Return JB_PT_CNSE for unknown coder, or the one that cannot be
     * mapped.
     */
    switch (coder) {
        case VTSP_CODER_G711U:
            return (JB_PT_G711U);
        case VTSP_CODER_G726_32K:
            return (JB_PT_G726_32);
        case VTSP_CODER_G711A:
            return (JB_PT_G711A);

#ifdef VTSP_ENABLE_G722            
        case VTSP_CODER_G722:
            return (JB_PT_G722_0);
#endif
#ifdef VTSP_ENABLE_G722P1
        case VTSP_CODER_G722P1_20MS:
            return (JB_PT_G722_1);
#endif         
#ifdef VTSP_ENABLE_SILK
        case VTSP_CODER_SILK_20MS_8K:
            return (JB_PT_SILK_8K);

        case VTSP_CODER_SILK_20MS_16K:
            return (JB_PT_SILK_16K);

        case VTSP_CODER_SILK_20MS_24K:
            return (JB_PT_SILK_24K);
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
        case VTSP_CODER_GAMRNB_20MS_OA:
            return (JB_PT_GAMRNB_OA);
        case VTSP_CODER_GAMRNB_20MS_BE:
            return (JB_PT_GAMRNB_BE);
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
        case VTSP_CODER_GAMRWB_20MS_OA:
            return (JB_PT_GAMRWB_OA);
        case VTSP_CODER_GAMRWB_20MS_BE:
            return (JB_PT_GAMRWB_BE);
#endif
#ifdef VTSP_ENABLE_G711P1
        case VTSP_CODER_G711P1U:
            return (JB_PT_G711P1U);
            
        case VTSP_CODER_G711P1A:
            return (JB_PT_G711P1A);
#endif      
   
#ifdef VTSP_ENABLE_16K_MU            
        case VTSP_CODER_16K_MU: 
            return (JB_PT_16K_MU);
#endif
        case VTSP_CODER_CN:
            return (JB_PT_CNSE);
        case VTSP_CODER_G729:
            return (JB_PT_G729);
        case VTSP_CODER_DTMF: 
            return (JB_PT_DTRLY);
        case VTSP_CODER_TONE: 
            return (JB_PT_TONERLY);
        case VTSP_CODER_G723_30MS:
            return (JB_PT_G723);
#ifdef VTSP_ENABLE_ILBC
        case VTSP_CODER_ILBC_20MS:
        case VTSP_CODER_ILBC_30MS:
            return (JB_PT_ILBC);
#endif
        default:
            return (JB_PT_CNSE);
    }
    
}

/*
 * ======== _VTSPR_streamIdToStreamPtr() ========
 *
 * Get stream pointer from DSP object using interface and stream id.
 *
 * RETURN: stream_ptr
 */
OSAL_INLINE VTSPR_StreamObj *_VTSPR_streamIdToStreamPtr(
        VTSPR_DSP *dsp_ptr,
        vint       infc,
        vint       streamId)
{
    VTSPR_StreamObj *stream_ptr;
    vint             index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    stream_ptr = dsp_ptr->streamObj_ptr[index];
    
    return (stream_ptr);
}

/*
 * ======== _VTSPR_streamIdToFlowPtr() ========
 *
 * Get stream pointer from DSP object using interface and stream id.
 *
 * RETURN: _VTSPR_FlowObj *flow_ptr
 */
OSAL_INLINE _VTSPR_FlowObj *_VTSPR_streamIdToFlowPtr(
        VTSPR_DSP *dsp_ptr,
        vint       infc,
        vint       streamId)
{
    _VTSPR_FlowObj *flow_ptr;
    vint            index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    flow_ptr = &(dsp_ptr->flowObj[index]);
    
    return (flow_ptr);
}

/*
 * ======== _VTSPR_streamIdToRtcpPtr() ========
 *
 * Get RTCP pointer from NET object using interface and stream id.
 *
 * RETURN: _VTSPR_RtcpObject *rtcp_ptr
 */
OSAL_INLINE _VTSPR_RtcpObject *_VTSPR_streamIdToRtcpPtr(
        VTSPR_NetObj *net_ptr,
        uvint         infc,
        uvint         streamId)
{
    _VTSPR_RtcpObject *rtcp_ptr;
    vint               index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    rtcp_ptr = &(net_ptr->rtcpObj[index]);
    
    return (rtcp_ptr);
}

/*
 * ======== _VTSPR_streamIdToRtpPtr() ========
 *
 * Get RTP pointer from NET object using interface and stream id.
 *
 * RETURN: _VTSPR_RtpObject *rtp_ptr
 */
OSAL_INLINE _VTSPR_RtpObject *_VTSPR_streamIdToRtpPtr(
        VTSPR_NetObj *net_ptr,
        uvint         infc,
        uvint         streamId)
{
    _VTSPR_RtpObject *rtp_ptr;
    vint              index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    rtp_ptr = &(net_ptr->rtpObj[index]);
    
    return (rtp_ptr);
}

/*
 * ======== _VTSPR_streamIdToMultiEncPtr() ========
 *
 * Get multi-encode object pointer from DSP object using interface
 * and stream id.
 *
 * RETURN: stream_ptr
 */
OSAL_INLINE _VTSPR_MultiEncObj *_VTSPR_streamIdToMultiEncPtr(
        VTSPR_DSP *dsp_ptr,
        vint       infc,
        vint       streamId)
{
    _VTSPR_MultiEncObj *multiEncObj_ptr;
    vint                index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    multiEncObj_ptr = &(dsp_ptr->streamObj_ptr[index]->multiEncObj);
    
    return (multiEncObj_ptr);
}

/*
 * ======== _VTSPR_streamIdToMultiEncQId() ========
 *
 * Get multi-encode object pointer from DSP object using interface
 * and stream id.
 *
 * RETURN: stream_ptr
 */
OSAL_INLINE OSAL_MsgQId _VTSPR_streamIdToMultiEncQId(
        VTSPR_Queues *q_ptr,
        vint          infc,
        vint          streamId,
        vint          pktRate)
{
    OSAL_MsgQId encQId;
    vint        index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    if (_VTSPR_MULTI_CODER_PKT_RATE_20 == pktRate) {
        encQId = q_ptr->data20Enc[index];
    }
    else if (_VTSPR_MULTI_CODER_PKT_RATE_30 == pktRate){
        encQId = q_ptr->data30Enc[index];
    }
    else if (_VTSPR_MULTI_CODER_PKT_RATE_10 == pktRate) {
        encQId = q_ptr->data10Enc[index];
    }
    else {
        /* Incorrect multi-frame rate */
        _VTSP_TRACE(__FILE__, __LINE__); 
        encQId = (OSAL_MsgQId) -1;
    }
    
    return (encQId);
}

/*
 * ======== _VTSPR_streamIdToMultiDecPtr() ========
 *
 * Get multi-decode object pointer from DSP object using interface
 * and stream id.
 *
 * RETURN: stream_ptr
 */
OSAL_INLINE _VTSPR_MultiDecObj *_VTSPR_streamIdToMultiDecPtr(
        VTSPR_DSP *dsp_ptr,
        vint       infc,
        vint       streamId)
{
    _VTSPR_MultiDecObj *multiDecObj_ptr;
    vint                index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    multiDecObj_ptr = &(dsp_ptr->streamObj_ptr[index]->multiDecObj);
    
    return (multiDecObj_ptr);
}

/*
 * ======== _VTSPR_streamIdToMultiDecQId() ========
 *
 * Get multi-decode object pointer from DSP object using interface
 * and stream id.
 *
 * RETURN: stream_ptr
 */
OSAL_INLINE OSAL_MsgQId _VTSPR_streamIdToMultiDecQId(
        VTSPR_Queues *q_ptr,
        vint          infc,
        vint          streamId,
        vint          pktRate)
{
    OSAL_MsgQId decQId;
    vint        index;

    index = streamId + (infc * _VTSP_STREAM_PER_INFC);

    if (_VTSPR_MULTI_CODER_PKT_RATE_10 == pktRate) {
        decQId = q_ptr->data10Dec[index];
    }
    else if (_VTSPR_MULTI_CODER_PKT_RATE_20 == pktRate) {
        decQId = q_ptr->data20Dec[index];
    }
    else if (_VTSPR_MULTI_CODER_PKT_RATE_30 == pktRate){
        decQId = q_ptr->data30Dec[index];
    }
    else {
        /* Incorrect multi-frame rate */
        _VTSP_TRACE(__FILE__, __LINE__); 
        decQId = (OSAL_MsgQId) -1;
    }
    
    return (decQId);
}


/*
 * ======== _VTSPR_infcToVadStatus() ========
 *
 * Determine if VAD is active on this infc
 *
 * RETURN: true or false
 */
vint _VTSPR_infcToVadStatus(
        VTSPR_ChanObj *chan_ptr,
        vint           infc)
{   
#ifdef VTSP_ENABLE_ECSR    
    NLP_Obj *nlp_ptr;
#endif
#ifdef VTSP_ENABLE_AEC
    AEC_Obj *aec_ptr;
#else
#ifndef VTSP_ENABLE_MP_LITE
    BND_Obj *bnd_ptr;
#endif
#endif
    vint voiceActive;

    voiceActive = 1;

    switch (_VTSPR_infcToType(infc)) {
        case VTSPR_INFC_TYPE_FXO:
        case VTSPR_INFC_TYPE_FXS:
#ifdef VTSP_ENABLE_ECSR
            nlp_ptr = &(chan_ptr->ec_ptr->nlpObj);
            if (nlp_ptr->status & NLP_BLK_ACT) {
                voiceActive = 0;
            }
#endif
            break;

        case VTSPR_INFC_TYPE_AUDIO:
#ifdef VTSP_ENABLE_AEC
            aec_ptr = &(chan_ptr->aec_ptr->aecNearObj);
            voiceActive = aec_ptr->nearActive;
#else
#ifndef VTSP_ENABLE_MP_LITE
            bnd_ptr = chan_ptr->bndNear_ptr;
            voiceActive = bnd_ptr->voiceActive;
#endif
#endif           
            break;
        default:
            _VTSP_TRACE(__FILE__,__LINE__);
    }

    return (voiceActive);
}
/*
 * ======== _VTSPR_isCoderWb() ========
 *
 * Determine if encoder type is wideband coder or not
 * XXX Remove this function during cleanup XXX
 *
 * RETURN: true or false
 */
OSAL_Boolean _VTSPR_isCoderWb(uvint encType)
{    
    switch (encType) { 
#if defined(VTSP_ENABLE_16K_MU) || defined(VTSP_ENABLE_G722) || \
    defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_SILK) || \
    defined(VTSP_ENABLE_G711P1) || defined(VTSP_ENABLE_GAMRWB) || \
    defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
        case VTSP_CODER_16K_MU:
        case VTSP_CODER_G722:
        case VTSP_CODER_G722P1_20MS:
        case VTSP_CODER_SILK_20MS_16K:
//        case VTSP_CODER_SILK_20MS_24K:
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
        case VTSP_CODER_GAMRWB_20MS_OA:
        case VTSP_CODER_GAMRWB_20MS_BE:
            return (OSAL_TRUE);
#endif
       default:
            return (OSAL_FALSE);
    }
}

/*
 * ======== _VTSPR_isCoderPassthrough() ========
 *
 * Determine if encoder type is CN or DTMF, which require knowledge of the
 * previous voice encoder.  Should/could be a macro instead.
 * Return value: OSAL_TRUE if passthrough CN or DTMF, OSAL_FALSE if not.
 */
OSAL_Boolean _VTSPR_isCoderPassthrough(uvint cod)
{
    return (((VTSP_CODER_DTMF == cod) || (VTSP_CODER_CN == cod)) ?
            OSAL_TRUE : OSAL_FALSE);
}

/*
 * ======== _VTSPR_isPacketCn() ========
 * Determine if the specific payload size of the encoder type is CN/SID or not
 * INPIT:  coder --  coder type
 *         bytes --  payload size in bytes
 * RETURN: true or false
 */
OSAL_Boolean  _VTSPR_isPacketCn(
        vint coder,
        vint bytes)
{
    switch (coder) {
        case VTSP_CODER_G729:
            if (VTSP_BLOCK_G729_SID_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_G726_32K:
            if (VTSP_BLOCK_G726_32K_SID_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_G723_30MS:
            if (VTSP_BLOCK_G723_SID_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_GAMRNB_20MS_OA:
            if (VTSP_BLOCK_GAMRNB_20MS_OA_MRDTX_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_GAMRNB_20MS_BE:
            if (VTSP_BLOCK_GAMRNB_20MS_BE_MRDTX_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_GAMRWB_20MS_OA:
            if (VTSP_BLOCK_GAMRWB_20MS_OA_MRDTX_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        case VTSP_CODER_GAMRWB_20MS_BE:
            if (VTSP_BLOCK_GAMRWB_20MS_BE_MRDTX_SZ == bytes) {
                return (OSAL_TRUE);
            }
            break;
        default:
            if (VTSP_BLOCK_G711_CN_SZ == bytes) {
                return (OSAL_TRUE);
            }
    }
    return (OSAL_FALSE);
}

/*
 * ======== _VTSPR_get10msSize() ========
 * Determine the min. payload size for specific payload type.
 * INPIT:  coder --  coder type
 * RETURN: payload size for packet rate = 10ms.
 */
vint _VTSPR_get10msSize(
        vint coder)
{
    switch (coder) {
        case VTSP_CODER_G729:
            return (VTSP_BLOCK_G729_10MS_SZ);
        case VTSP_CODER_G726_32K:
            return (VTSP_BLOCK_G726_32K_10MS_SZ);
        case VTSP_CODER_G723_30MS:
            return (VTSP_BLOCK_G723_30MS_63_SZ);
        default:
            return (VTSP_BLOCK_G711_10MS_SZ);
    }
}

/*
 * ======== _VTSPR_infcLineToTicCode() ========
 *
 * Map VTSP_ControlInfcLine codes to TIC codes
 */
TIC_ControlCode _VTSPR_infcLineToTicCode(
        VTSP_ControlInfcLine code)
{
    switch (code) {
        case VTSP_CONTROL_INFC_POWER_OPEN:
            return (TIC_CONTROL_POWER_OPEN);

        case VTSP_CONTROL_INFC_POWER_TEST:
            return (TIC_CONTROL_POWER_TEST);

        case VTSP_CONTROL_INFC_POWER_ACTIVE:
            return (TIC_CONTROL_POWER_ACTIVE);

        case VTSP_CONTROL_INFC_POWER_DOWN:
            return (TIC_CONTROL_POWER_DOWN);

        case VTSP_CONTROL_INFC_POLARITY_FWD:
             return (TIC_CONTROL_POLARITY_FWD);

        case VTSP_CONTROL_INFC_POLARITY_REV:
            return (TIC_CONTROL_POLARITY_REV);

        case VTSP_CONTROL_INFC_ONHOOK:
             return (TIC_CONTROL_ONHOOK);

        case VTSP_CONTROL_INFC_OFFHOOK:
             return (TIC_CONTROL_OFFHOOK);

        case VTSP_CONTROL_INFC_FLASH:
            return (TIC_CONTROL_HOOK_FLASH);

        default:
            _VTSP_TRACE(__FILE__,__LINE__);
            return(TIC_CONTROL_UNKNOWN_CODE);
    }
}

/*
 * ======== _VTSPR_infcIOToTicCode() ========
 *
 * Map VTSP_infcLine codes to TIC codes
 */
TIC_ControlCode _VTSPR_infcIOToTicCode(
        VTSP_ControlInfcIO code)
{
    switch (code) {
        case VTSP_CONTROL_INFC_IO_FXS_TO_PSTN:
            return (TIC_CONTROL_RELAY_FXS_TO_PSTN);

        case VTSP_CONTROL_INFC_IO_LED:
            return (TIC_CONTROL_LED);

        case VTSP_CONTROL_INFC_IO_GAIN_TX:
            return (TIC_CONTROL_GAIN_TX);

        case VTSP_CONTROL_INFC_IO_GAIN_RX:
            return (TIC_CONTROL_GAIN_RX);

        case VTSP_CONTROL_INFC_IO_INPUT_HANDSET_MIC:
            return (TIC_CONTROL_INPUT_HANDSET);

        case VTSP_CONTROL_INFC_IO_INPUT_HEADSET_MIC:
            return (TIC_CONTROL_INPUT_HEADSET);

        case VTSP_CONTROL_INFC_IO_OUTPUT_SPEAKER:
            return (TIC_CONTROL_OUTPUT_SPEAKER);

        case VTSP_CONTROL_INFC_IO_OUTPUT_HANDSET:
            return (TIC_CONTROL_OUTPUT_HANDSET);

        case VTSP_CONTROL_INFC_IO_OUTPUT_HEADSET:
            return (TIC_CONTROL_OUTPUT_HEADSET);

        case VTSP_CONTROL_INFC_IO_MUTE_INPUT_HW:
            return (TIC_CONTROL_MUTE_INPUT);

        case VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_HW:
            return (TIC_CONTROL_MUTE_OUTPUT);

        case VTSP_CONTROL_INFC_IO_LOOPBACK_DIGITAL:
            return (TIC_CONTROL_LOOPBACK_DIGITAL);

        case VTSP_CONTROL_INFC_IO_LOOPBACK_ANALOG:
            return (TIC_CONTROL_LOOPBACK_ANALOG);

        case VTSP_CONTROL_INFC_IO_PULSE_GENERATE:
            return (TIC_CONTROL_PULSE_GENERATE);
        
        case VTSP_CONTROL_INFC_IO_AUDIO_ATTACH:
            return (TIC_CONTROL_AUDIO_ATTACH);

        case VTSP_CONTROL_INFC_IO_GR909:
            return (TIC_CONTROL_GR909_DIAG);

        case VTSP_CONTROL_INFC_IO_BALANCED_RINGING:
            return (TIC_CONTROL_BALANCED_RINGING);

        case VTSP_CONTROL_INFC_IO_GENERIC:
            return (TIC_CONTROL_GENERIC);

        default:
            _VTSP_TRACE(__FILE__,__LINE__);
            return(TIC_CONTROL_UNKNOWN_CODE);
    }
}

#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
/*
 * ======== _VTSPR_samples10msToUdsMode() ========
 *
 * Map VTSPR number samples 10ms to UDS mode
 */
vint _VTSPR_samples10msToUdsMode(
    vint nSamples10ms)
{
    switch (nSamples10ms) {
        case VTSPR_NSAMPLES_10MS_8K:
            return (UDS_SAMPLERATE_8K);

        case VTSPR_NSAMPLES_10MS_16K:
            return (UDS_SAMPLERATE_16K);

        case VTSPR_NSAMPLES_10MS_24K:
            return (UDS_SAMPLERATE_24K);

        case VTSPR_NSAMPLES_10MS_32K:
            return (UDS_SAMPLERATE_32K);

        case VTSPR_NSAMPLES_10MS_48K:
            return (UDS_SAMPLERATE_48K);

        default:
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
    }
}
#endif
#endif
