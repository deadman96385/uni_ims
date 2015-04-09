/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */
#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_config() ========
 */
VTSP_Return VTSP_config(
    VTSP_TemplateCode    templateCode, 
    uvint                templateId, 
    void                *data_ptr)
{
    _VTSP_CmdMsg              cmd;
    uvint                  infc;
    uvint                  streamId;
    VTSP_RingTemplate     *templateRing_ptr;
    VTSP_ToneTemplate     *templateTone_ptr;
    uvint                  index;
    VTSP_ToneQuadTemplate *templateQuad_ptr;
    VTSP_EchoCancTemplate *ecConfig_ptr;
    VTSP_AecTemplate      *aecConfig_ptr;
    VTSP_Fr38Template     *fr38Config_ptr;
    VTSP_CnTemplate       *cnConfig_ptr;
    VTSP_FmtdTemplate     *fmtdConfig_ptr;
    VTSP_TicTemplate      *ticConfig_ptr;
    VTSP_JbTemplate       *jbConfig_ptr;
    VTSP_RtcpTemplate     *rtcpConfig_ptr;
    VTSP_RtpTemplate      *rtpConfig_ptr;
    VTSP_CidTemplate      *cidConfig_ptr;
    VTSP_Return            e;
    VTSP_UtdTemplate      *utdConfig_ptr;
    VTSP_DtmfTemplate     *dtmfConfig_ptr;
    VTSP_DebugTemplate    *debugConfig_ptr;
    _VTSP_VERIFY_INIT;

    if (NULL == data_ptr) { 
        return (VTSP_E_ARG);
    }
   
    infc = 0;

    cmd.code = _VTSP_CMD_CONFIG;
    cmd.msg.config.templCode = templateCode;

    switch (templateCode) { 
        case VTSP_TEMPL_CODE_NONE:
        case VTSP_TEMPL_CODE_CODEC:
        case VTSP_TEMPL_CODE_FIFO:
        case VTSP_TEMPL_CODE_DIGIT:
        case VTSP_TEMPL_CODE_DTMF_DET:
        case VTSP_TEMPL_CODE_SOFT_GAIN:
        case VTSP_TEMPL_CODE_REF_LEVEL:
        case VTSP_TEMPL_CODE_NFE:
        case VTSP_TEMPL_CODE_FSKS:
        case VTSP_TEMPL_CODE_FSKR:
        case VTSP_TEMPL_CODE_VAD:
            _VTSP_TRACE(__FILE__, __LINE__); /* XXX */
            break;
        case VTSP_TEMPL_CODE_FMTD:
            fmtdConfig_ptr = (VTSP_FmtdTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = (int16)fmtdConfig_ptr->control;
            switch (fmtdConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_INFC:
                case VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_PEER:
                    if ((-53 > fmtdConfig_ptr->powerMin) ||
                            (-33 < fmtdConfig_ptr->powerMin)) {
                        return (VTSP_E_ARG); /* powerMin out of range */
                    }
                    infc = VTSP_INFC_GLOBAL;
                    cmd.infc = infc;
                    cmd.msg.config.u.data[1] = (int16)fmtdConfig_ptr->powerMin;
                    break;
                case VTSP_TEMPL_CONTROL_FMTD_DETECT_MASK:
                    infc = templateId;
                    cmd.infc = infc;
                    cmd.msg.config.u.data[1] = (int16)fmtdConfig_ptr->detectMask;
                    break;
                default:
                    return (VTSP_E_TEMPL);
            }
            break;            
        case VTSP_TEMPL_CODE_CID:
            infc = templateId;
            cmd.infc = infc;
            cidConfig_ptr = (VTSP_CidTemplate *)data_ptr;
            cmd.msg.config.u.data[0] = cidConfig_ptr->control;
            switch (cidConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_CID_FORMAT:
                    cmd.msg.config.u.data[1] = cidConfig_ptr->format;
                    break;
                case VTSP_TEMPL_CONTROL_CID_MDID_TIMEOUT:
                    cmd.msg.config.u.data[1] = cidConfig_ptr->mdidTimeout;
                    break;
                case VTSP_TEMPL_CONTROL_CID_FSK_PWR:
                    cmd.msg.config.u.data[1] = cidConfig_ptr->fskPower;
                    break;
                case VTSP_TEMPL_CONTROL_CID_SAS_PWR:
                    cmd.msg.config.u.data[1] = cidConfig_ptr->sasPower;
                    break;
                case VTSP_TEMPL_CONTROL_CID_CAS_PWR:
                    cmd.msg.config.u.data[1] = cidConfig_ptr->casPower;
                    break;
                default:
                    return (VTSP_E_TEMPL);
            }
            break;  
        case VTSP_TEMPL_CODE_RING:
            if (templateId >= _VTSP_NUM_RING_TEMPL) {
                return (VTSP_E_TEMPL_NUM);
            }
            templateRing_ptr = data_ptr;
            infc = VTSP_INFC_GLOBAL;
            cmd.infc = infc;
            cmd.msg.config.u.data[0] = templateId;
            cmd.msg.config.u.data[1] = (int16)templateRing_ptr->cadences;
            cmd.msg.config.u.data[2] = (int16)(templateRing_ptr->make1/10);
            cmd.msg.config.u.data[3] = (int16)(templateRing_ptr->break1/10);
            cmd.msg.config.u.data[4] = (int16)(templateRing_ptr->make2/10);
            cmd.msg.config.u.data[5] = (int16)(templateRing_ptr->break2/10);
            cmd.msg.config.u.data[6] = (int16)(templateRing_ptr->make3/10);
            cmd.msg.config.u.data[7] = (int16)(templateRing_ptr->break3/10);
            cmd.msg.config.u.data[8] = (int16)(templateRing_ptr->cidBreakNum);
            break;
        case VTSP_TEMPL_CODE_TONE:   
            if (templateId >= _VTSP_NUM_TONE_TEMPL) {
                return (VTSP_E_TEMPL_NUM);
            }
            templateTone_ptr = data_ptr;
            infc = VTSP_INFC_GLOBAL;
            cmd.infc = infc;
            cmd.msg.config.u.data[0] = templateId;
            cmd.msg.config.u.data[1] = templateTone_ptr->freq1;
            cmd.msg.config.u.data[2] = templateTone_ptr->freq2;
            /* Tone power control is in 0.5db unit */
            cmd.msg.config.u.data[3] = templateTone_ptr->power1;
            cmd.msg.config.u.data[4] = templateTone_ptr->power2;
            cmd.msg.config.u.data[5] = templateTone_ptr->cadences;
            cmd.msg.config.u.data[6] = templateTone_ptr->make1;
            cmd.msg.config.u.data[7] = templateTone_ptr->break1;
            cmd.msg.config.u.data[8] = templateTone_ptr->repeat1;
            cmd.msg.config.u.data[9] = templateTone_ptr->make2;
            cmd.msg.config.u.data[10] = templateTone_ptr->break2;
            cmd.msg.config.u.data[11] = templateTone_ptr->repeat2;
            cmd.msg.config.u.data[12] = templateTone_ptr->make3;
            cmd.msg.config.u.data[13] = templateTone_ptr->break3;
            cmd.msg.config.u.data[14] = templateTone_ptr->repeat3;
            break;
        case VTSP_TEMPL_CODE_TONE_QUAD:
            if(templateId >= _VTSP_NUM_TONE_TEMPL) {
                return (VTSP_E_TEMPL_NUM);
            }
            templateQuad_ptr = data_ptr;
            infc = VTSP_INFC_GLOBAL;
            cmd.infc = infc;
            index = 0;
            cmd.msg.config.u.data[index++] = templateId;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->control;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.freq1;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.freq2;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.freq3;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.freq4;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.power1;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.power2;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.power3;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->tone.quad.power4;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->cadences;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->make1;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->break1;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->repeat1;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->make2;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->break2;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->repeat2;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->make3;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->break3;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->repeat3;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->delta;
            cmd.msg.config.u.data[index++] = templateQuad_ptr->decay;
            break;
        case VTSP_TEMPL_CODE_EC:
            infc = templateId;
            cmd.infc = infc;
            ecConfig_ptr = (VTSP_EchoCancTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = (uint16)ecConfig_ptr->control;
            switch (ecConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_EC_NORMAL:
                case VTSP_TEMPL_CONTROL_EC_BYPASS:
                case VTSP_TEMPL_CONTROL_EC_FREEZE:
                    break;
                case VTSP_TEMPL_CONTROL_EC_TAIL_LENGTH:
                    switch (ecConfig_ptr->tailLength) {
                        case VTSP_EC_TAIL_LENGTH_16MS:
                        case VTSP_EC_TAIL_LENGTH_32MS:
                        case VTSP_EC_TAIL_LENGTH_48MS:
                        case VTSP_EC_TAIL_LENGTH_64MS:
                        case VTSP_EC_TAIL_LENGTH_128MS:
                            cmd.msg.config.u.data[1] = 
                                    (uint16)(ecConfig_ptr->tailLength);
                        default:
                            return (VTSP_E_CONFIG);
                    }
                case VTSP_TEMPL_CONTROL_EC_FMTD:
                    cmd.msg.config.u.data[1] = 
                            (uint16)(ecConfig_ptr->silenceTime / 10);
            cmd.msg.config.u.data[2] = (uint16)ecConfig_ptr->faxSilRxDB;
                    cmd.msg.config.u.data[3] = (uint16)ecConfig_ptr->faxSilTxDB;                  default:
                    return (VTSP_E_TEMPL);
            }
            break;
       case VTSP_TEMPL_CODE_AEC:
            infc = templateId;
            cmd.infc = infc;
            aecConfig_ptr = (VTSP_AecTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = (uint16)aecConfig_ptr->control;
            switch (aecConfig_ptr->control) { 
                case VTSP_TEMPL_CONTROL_AEC_TAIL_LENGTH:
                    cmd.msg.config.u.data[1] = aecConfig_ptr->tailLen;
                    break;
                default:
                    break;
            }
            break;

        case VTSP_TEMPL_CODE_FR38:
            infc = templateId;
            cmd.infc = infc;
            fr38Config_ptr = (VTSP_Fr38Template *)data_ptr;
            cmd.msg.config.u.data[0] = fr38Config_ptr->control;
            switch (fr38Config_ptr->control) {
                case VTSP_TEMPL_CONTROL_FR38_RATE_MAX:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->maxRate;
                    break;
                case VTSP_TEMPL_CONTROL_FR38_JITTER_MAX:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->maxJitter;
                    break;
                case VTSP_TEMPL_CONTROL_FR38_ECC:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->eccMode;
                    cmd.msg.config.u.data[2] = fr38Config_ptr->numEccT30;
                    cmd.msg.config.u.data[3] = fr38Config_ptr->numEccData;
                    break;
                case VTSP_TEMPL_CONTROL_FR38_RATE_MGMT:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->rateMgmt;
                    break;
                case VTSP_TEMPL_CONTROL_FR38_ECM_MODE:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->ecmMode;
                    break;
                case VTSP_TEMPL_CONTROL_FR38_CED_LEN:
                    cmd.msg.config.u.data[1] = fr38Config_ptr->cedLen;
                    break;
                default:
                    return (VTSP_E_TEMPL);
            }
            break;
       case VTSP_TEMPL_CODE_CN:
            cmd.infc = templateId;
            cnConfig_ptr = (VTSP_CnTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = (uint16)cnConfig_ptr->control;
            switch (cnConfig_ptr->control) { 
                case VTSP_TEMPL_CONTROL_CN_POWER_ATTEN:
                    if ((_VTSP_CONFIG_CN_PWR_ATN_MAX > 
                                cnConfig_ptr->cnPwrAttenDb) ||
                        (_VTSP_CONFIG_CN_PWR_ATN_MIN <
                                 cnConfig_ptr->cnPwrAttenDb)) {
                        return (VTSP_E_ARG); /* power out of range */
                    }
                    cmd.msg.config.u.data[1] = cnConfig_ptr->cnPwrAttenDb;
                    break;
                default:
                    return (VTSP_E_TEMPL_NUM);
            }
            break;
        case VTSP_TEMPL_CODE_TIC:
            infc = templateId;
            cmd.infc = infc;
            ticConfig_ptr = (VTSP_TicTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = ticConfig_ptr->control;
            switch (ticConfig_ptr->control) { 
                case VTSP_TEMPL_CONTROL_TIC_SET_RELEASE_TIME:
                    cmd.msg.config.u.data[1] =
                            (uint16)(ticConfig_ptr->releaseTimeMin / 10);
                    break;
                case VTSP_TEMPL_CONTROL_TIC_SET_FLASH_TIME:
                    cmd.msg.config.u.data[1] =
                            (uint16)(ticConfig_ptr->flashTimeMin / 10);
                    cmd.msg.config.u.data[2] = 
                        (uint16)(ticConfig_ptr->flashTimeMax / 10);
                    break;
                case VTSP_TEMPL_CONTROL_TIC_SET_PDD_PARAMS:
                    cmd.msg.config.u.data[1] =
                            (uint16)(ticConfig_ptr->pddMakeMin);
                    cmd.msg.config.u.data[2] = 
                            (uint16)(ticConfig_ptr->pddMakeMax);
                    cmd.msg.config.u.data[3] =
                            (uint16)(ticConfig_ptr->pddBreakMin);
                    cmd.msg.config.u.data[4] = 
                            (uint16)(ticConfig_ptr->pddBreakMax);
                    cmd.msg.config.u.data[5] = 
                            (uint16)(ticConfig_ptr->pddInterDigitMin);
                    break;
                default: 
                    return (VTSP_E_CONFIG);
            }
            break;
        case VTSP_TEMPL_CODE_JB:
            jbConfig_ptr = (VTSP_JbTemplate *)(data_ptr);
            infc = templateId;
            cmd.infc = infc;
            streamId = jbConfig_ptr->streamId;
            if (streamId >= _VTSP_object_ptr->config.stream.numPerInfc) { 
                return (VTSP_E_STREAM_NUM);
            }
            cmd.msg.config.u.data[0] = jbConfig_ptr->control;
            cmd.msg.config.u.data[1] = streamId;
            /* Convert the units from millseconds to samples for JB */
            /* XXX warning -- hardcodes 80 samples per 10 ms */
            cmd.msg.config.u.data[2] = jbConfig_ptr->maxLevel * 8;
            cmd.msg.config.u.data[3] = jbConfig_ptr->initLevel * 8;
            break;
        case VTSP_TEMPL_CODE_RTCP:
            rtcpConfig_ptr = (VTSP_RtcpTemplate *)(data_ptr);
            infc = templateId;
            cmd.infc = infc;
            streamId = rtcpConfig_ptr->streamId;
            if (streamId >= _VTSP_object_ptr->config.stream.numPerInfc) { 
                return (VTSP_E_STREAM_NUM);
            }
            cmd.msg.config.u.data[0] = rtcpConfig_ptr->control;
            cmd.msg.config.u.data[1] = streamId;
            switch (rtcpConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_RTCP_INTERVAL:
                    cmd.msg.config.u.data[2] = rtcpConfig_ptr->interval;
                    break;
                case VTSP_TEMPL_CONTROL_RTCP_MASK:
                    cmd.msg.config.u.data[2] = rtcpConfig_ptr->mask;
                    break;
                case VTSP_TEMPL_CONTROL_RTCP_TOS:
                    cmd.msg.config.u.data[2] = rtcpConfig_ptr->tos;
                    break;
                default:
                    return (VTSP_E_TEMPL);                    
            }
            break;     
        case VTSP_TEMPL_CODE_UTD:
            utdConfig_ptr = (VTSP_UtdTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = utdConfig_ptr->control;
            switch (utdConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_UTD_MASK:
                    infc = templateId;
                    if (VTSP_OK != (e = _VTSP_isInfcFxo(infc))) { 
                        return (e);
                    }
                    cmd.infc = infc;
                    cmd.msg.config.u.data[1] = utdConfig_ptr->mask;
                    break;
                case VTSP_TEMPL_CONTROL_UTD_TONE:
                    if (templateId >= _VTSP_NUM_UTD_TEMPL) {
                        return (VTSP_E_TEMPL_NUM);
                    }
                    if (utdConfig_ptr->type > VTSP_TEMPL_UTD_TONE_TYPE_UNK) { 
                        return (VTSP_E_ARG);
                    }
                    infc = VTSP_INFC_GLOBAL;
                    cmd.infc = infc;
                    cmd.msg.config.u.data[1] = templateId;
                    /* 
                     * Note:  VTSP_UtdTemplate.dual is larger than .sit, 
                     * so copy .dual into config.u.data.
                     */
                    cmd.msg.config.u.data[2] = utdConfig_ptr->type;
                    if (VTSP_TEMPL_UTD_TONE_TYPE_SIT == utdConfig_ptr->type) {
                        cmd.msg.config.u.data[3]  = utdConfig_ptr->u.sit.control;
                        cmd.msg.config.u.data[4]  = utdConfig_ptr->u.sit.freq1;
                        cmd.msg.config.u.data[5]  = utdConfig_ptr->u.sit.freqDev1;
                        cmd.msg.config.u.data[6]  = utdConfig_ptr->u.sit.freq2;
                        cmd.msg.config.u.data[7]  = utdConfig_ptr->u.sit.freqDev2;
                        cmd.msg.config.u.data[8]  = utdConfig_ptr->u.sit.freq3;
                        cmd.msg.config.u.data[9]  = utdConfig_ptr->u.sit.freqDev3;
                        cmd.msg.config.u.data[10] = utdConfig_ptr->u.sit.freq4;
                        cmd.msg.config.u.data[11] = utdConfig_ptr->u.sit.freqDev4;
                        cmd.msg.config.u.data[12] = utdConfig_ptr->u.sit.freq5;
                        cmd.msg.config.u.data[13] = utdConfig_ptr->u.sit.freqDev5;
                        cmd.msg.config.u.data[14] = utdConfig_ptr->u.sit.shortMin;
                        cmd.msg.config.u.data[15] = utdConfig_ptr->u.sit.shortMax;
                        cmd.msg.config.u.data[16] = utdConfig_ptr->u.sit.longMin;
                        cmd.msg.config.u.data[17] = utdConfig_ptr->u.sit.longMax;
                        cmd.msg.config.u.data[18] = utdConfig_ptr->u.sit.power;
                    } 
                    else { /* Assume dual type */
                        cmd.msg.config.u.data[3]  = utdConfig_ptr->u.dual.control;
                        cmd.msg.config.u.data[4]  = utdConfig_ptr->u.dual.cadences;
                        cmd.msg.config.u.data[5]  = utdConfig_ptr->u.dual.freq1;
                        cmd.msg.config.u.data[6]  = utdConfig_ptr->u.dual.freqDev1;
                        cmd.msg.config.u.data[7]  = utdConfig_ptr->u.dual.freq2;
                        cmd.msg.config.u.data[8]  = utdConfig_ptr->u.dual.freqDev2;
                        cmd.msg.config.u.data[9]  = utdConfig_ptr->u.dual.minMake1;
                        cmd.msg.config.u.data[10] = utdConfig_ptr->u.dual.maxMake1;
                        cmd.msg.config.u.data[11] = utdConfig_ptr->u.dual.minBreak1;
                        cmd.msg.config.u.data[12] = utdConfig_ptr->u.dual.maxBreak1;
                        cmd.msg.config.u.data[13] = utdConfig_ptr->u.dual.minMake2;
                        cmd.msg.config.u.data[14] = utdConfig_ptr->u.dual.maxMake2;
                        cmd.msg.config.u.data[15] = utdConfig_ptr->u.dual.minBreak2;
                        cmd.msg.config.u.data[16] = utdConfig_ptr->u.dual.maxBreak2;
                        cmd.msg.config.u.data[17] = utdConfig_ptr->u.dual.minMake3;
                        cmd.msg.config.u.data[18] = utdConfig_ptr->u.dual.maxMake3;
                        cmd.msg.config.u.data[19] = utdConfig_ptr->u.dual.minBreak3;
                        cmd.msg.config.u.data[20] = utdConfig_ptr->u.dual.maxBreak3;
                        cmd.msg.config.u.data[21] = utdConfig_ptr->u.dual.power;
                    }
                    break;
                default:
                    return (VTSP_E_TEMPL);
            }
            break;
        case VTSP_TEMPL_CODE_RTP:
            rtpConfig_ptr = (VTSP_RtpTemplate *)(data_ptr);
            infc = templateId;
            cmd.infc = infc;
            streamId = rtpConfig_ptr->streamId;
            if (streamId >= _VTSP_object_ptr->config.stream.numPerInfc) { 
                return (VTSP_E_STREAM_NUM);
            }
            cmd.msg.config.u.data[0] = rtpConfig_ptr->control;
            cmd.msg.config.u.data[1] = streamId;
            switch (rtpConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_RTP_TOS:
                    cmd.msg.config.u.data[2] = rtpConfig_ptr->tos;
                    break;
                case VTSP_TEMPL_CONTROL_RTP_TS_INIT:
                    cmd.msg.config.u.data[2] = rtpConfig_ptr->randomEnable;
                    break;
                case VTSP_TEMPL_CONTROL_RTP_SN_INIT:
                    cmd.msg.config.u.data[2] = rtpConfig_ptr->randomEnable;
                    break;
                case VTSP_TEMPL_CONTROL_RTP_REDUNDANT:
                    cmd.msg.config.u.data[2] = rtpConfig_ptr->rdnLevel;
                    cmd.msg.config.u.data[3] = rtpConfig_ptr->rdnHop;
                    break;
                default:
                    return (VTSP_E_TEMPL);                    
            }
            break;  

        case VTSP_TEMPL_CODE_DTMF:
            dtmfConfig_ptr = (VTSP_DtmfTemplate *)(data_ptr);
            cmd.msg.config.u.data[0] = dtmfConfig_ptr->control;
            switch (dtmfConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_DTMF_POWER:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfPower;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_DURATION:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfDur;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_SILENCE:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfSil;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_HI_TWIST:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfHiTwist;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_LO_TWIST:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfLoTwist;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_FRE_DEV:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfDev;
                    break;
                case VTSP_TEMPL_CONTROL_DTMF_ERR_FRAMES:
                    cmd.msg.config.u.data[1] = dtmfConfig_ptr->dtmfFrames;
                    break;
            }
            break;
        case VTSP_TEMPL_CODE_DEBUG:
            debugConfig_ptr = (VTSP_DebugTemplate*)(data_ptr);
            cmd.msg.config.u.data[0] = debugConfig_ptr->control;
            infc = templateId;
            cmd.infc = infc;
            switch (debugConfig_ptr->control) {
                case VTSP_TEMPL_CONTROL_DEBUG_PCMDUMP:
                    cmd.msg.config.u.data[1] = debugConfig_ptr->debugEnable;
                    cmd.msg.config.u.data32[1] =
                            debugConfig_ptr->debugRemoteIP;
                    break;
                case VTSP_TEMPL_CONTROL_DEBUG_TRACE:
                    /* Do trace stuff */
                    break;
            }
            break;
        default:
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_OK);
    }
    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_configRing() ========
 */
VTSP_Return VTSP_configRing(
    uvint              templateId,
    VTSP_RingTemplate *templateData_ptr)
{
    _VTSP_VERIFY_INIT;

    if (templateId >= _VTSP_NUM_RING_TEMPL) {
        return (VTSP_E_TEMPL_NUM);
    }
    if (templateData_ptr == NULL) {
        return (VTSP_E_TEMPL);
    }
    return (VTSP_config(VTSP_TEMPL_CODE_RING, templateId, templateData_ptr));
}


/*
 * ========  VTSP_configToneQuad() ========
 */
VTSP_Return VTSP_configToneQuad(
    uvint                  templateId,
    VTSP_ToneQuadTemplate *templateData_ptr)
{
    _VTSP_VERIFY_INIT;

    if(templateId >= _VTSP_NUM_TONE_TEMPL) {
        return (VTSP_E_TEMPL_NUM);
    }
    if(templateData_ptr == NULL) {
        return (VTSP_E_TEMPL);
    }

    /* Error check template data  */
    switch (templateData_ptr->control) { 
        case VTSP_TEMPL_CONTROL_TONE_QUAD_QUAD:
        case VTSP_TEMPL_CONTROL_TONE_QUAD_MOD:
        case VTSP_TEMPL_CONTROL_TONE_QUAD_SIT:
        case VTSP_TEMPL_CONTROL_TONE_QUAD_BONG:
        case VTSP_TEMPL_CONTROL_TONE_QUAD_DELTA:
            /* Valid control words */
            break;
        default:
            return (VTSP_E_TEMPL);
    }

    /* Modify template data to conform to algorithm interface */
    return (VTSP_config(VTSP_TEMPL_CODE_TONE_QUAD, templateId,
            templateData_ptr));
}

/*
 * ========  VTSP_configTone() ========
 */
VTSP_Return VTSP_configTone(
    uvint              templateId,
    VTSP_ToneTemplate *templateData_ptr)
{
    _VTSP_VERIFY_INIT;

    if ((templateId >= _VTSP_NUM_TONE_TEMPL) || (0 == templateId)) {
        return (VTSP_E_TEMPL_NUM);
    }
    if (templateData_ptr == NULL) {
        return (VTSP_E_TEMPL);
    }
    return (VTSP_config(VTSP_TEMPL_CODE_TONE, templateId, templateData_ptr)
            );
}

/*
 * ======== VTSP_init() ========
 */
VTSP_Return VTSP_init(
    VTSP_Context       **context_ptr, 
    VTSP_ProfileHeader  *profile_ptr,
    VTSP_TaskConfig     *taskConfig_ptr)
{
    VTSP_Context       mem_ptr;
    _RTCP_TaskContext *rtcpContext_ptr;
    VTSP_Return        e;
#if defined(OSAL_KERNEL_EMULATION) || defined (OSAL_VXWORKS) || \
    defined(OSAL_WINCE) || defined(OSAL_THREADX)
    OSAL_TaskId        taskId;
#endif
    vint               infc;             /* must be signed */
#if defined(VTSP_ENABLE_PLAY) || defined(VTSP_ENABLE_RECORD)
    vint               flowIndex;
#endif
    _VTSP_InfcObj     *infc_ptr;
    vint               streamId;         /* must be signed */
    uvint              numStreamId;
    uvint              numInfc;
    char               qname[16];

    /*
     * Guard attempt to re-init without shutdown first
     */
    if (NULL != _VTSP_object_ptr) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_SHUTDOWN);
    }

    *context_ptr = NULL;
    numStreamId = 0;

    mem_ptr = OSAL_memCalloc(1, _VTSP_OBJECT_SZ, 0);

    if (NULL == mem_ptr) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INIT);
    }

    _VTSP_object_ptr = mem_ptr;

    _VTSP_object_ptr->start = VTSP_E_INIT;

    if (VTSP_OK != (e = _VTSP_setObj())) { 
        /*
         * Unallocate mem
         */
        _VTSP_TRACE(__FILE__, __LINE__);
        OSAL_memFree(_VTSP_object_ptr, 0);
        _VTSP_object_ptr = NULL;
        return (e);
    }

    if (NULL == (_VTSP_object_ptr->globalEventQ = 
                OSAL_msgQCreate(
                "vtsp-globalq",
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_GLOBAL_NUM_MSG,
                _VTSP_Q_GLOBAL_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }

    if (NULL == (_VTSP_object_ptr->cmdQ = 
                OSAL_msgQCreate(
                "vtsp-cmdq",
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }

    /*
     * Create eventQ group, for blocking on ANY
     * Use priority, to allow global events to always receive first.
     */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&_VTSP_object_ptr->infcEventQGrp)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    /* Add the globalEventQ to the EventQ Group first.  Highest priority. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VTSP_object_ptr->infcEventQGrp,
                _VTSP_object_ptr->globalEventQ)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    
    /*
     * Video queues
     */
    if (NULL == (_VTSP_object_ptr->cmdQVideo = 
                OSAL_msgQCreate(
                "vtsp-cmdqVideo",
                OSAL_MODULE_AUDIO_VE,
#ifdef INCLUDE_4G_PLUS
                OSAL_MODULE_VPR,
#else
                OSAL_MODULE_VIDEO_VE,
#endif
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    
    if (NULL == (_VTSP_object_ptr->evtQVideo = 
                OSAL_msgQCreate(
                "vtsp-evtqVideo",
#ifdef INCLUDE_4G_PLUS
                OSAL_MODULE_VPR,
#else
                OSAL_MODULE_VIDEO_VE,
#endif
                OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_EVENT_NUM_MSG,
                _VTSP_Q_EVENT_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    
    
    /* Add the evtQVideo to the EventQ Group */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VTSP_object_ptr->infcEventQGrp,
                _VTSP_object_ptr->evtQVideo)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }

    numInfc = _VTSP_object_ptr->config.hw.numInfc;
    for (infc = numInfc - 1; infc >= 0; infc--) { 
        /*
         * Create Event Q 
         */
        infc_ptr = &_VTSP_object_ptr->infc[infc];
        OSAL_snprintf(qname, sizeof(qname), "vtsp-infc%dq", infc);
        if (NULL == (infc_ptr->eventQ = OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT_VTSP_EventMsg,
                    _VTSP_Q_EVENT_NUM_MSG,
                    _VTSP_Q_EVENT_MSG_SZ,
                    0))) {
            _VTSP_TRACE(__FILE__, __LINE__);
            goto _error;
        }
        /* Add EventQ to the EventQGroup */
        if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VTSP_object_ptr->infcEventQGrp,
                infc_ptr->eventQ)) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            goto _error;
        }

#if defined(VTSP_ENABLE_CIDR) || defined(VTSP_ENABLE_CIDS)
        /*
         * Create CallerID Q
         * one msg is allowed in the Q
         */
        OSAL_snprintf(qname, sizeof(qname), "vtsp-cid%dq", infc);
        if (NULL == (infc_ptr->cidQ = OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT__VTSP_CIDData,
                    1,
                    sizeof(_VTSP_CIDData),
                    0))) {
            _VTSP_TRACE(__FILE__, __LINE__);
            goto _error;
        }
#endif

        /*
         * Create Flow Qs.
         *
         * NOTE:
         * flowId is the same as streamId.
         */
        numStreamId = _VTSP_object_ptr->config.stream.numPerInfc;
        for (streamId = numStreamId - 1; streamId >= 0; streamId--) { 
            /*
             * Note:
             * Flow index is the same as stream Index.
             */
#if defined(VTSP_ENABLE_PLAY) || defined(VTSP_ENABLE_RECORD)
            flowIndex = (infc * numStreamId) + streamId;
#endif

            /*
             * Add flow queues
             */
#ifdef VTSP_ENABLE_PLAY
            OSAL_snprintf(qname, sizeof(qname), "vtsp-flowp%dq", flowIndex);
            if (NULL == (infc_ptr->flowDataFromAppQ[streamId] =
                    OSAL_msgQCreate(
                        qname,
                        OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                        OSAL_DATA_STRUCT__VTSP_FlowMsg,
                        _VTSP_Q_FLOW_PUT_NUM_MSG,
                        _VTSP_Q_FLOW_PUT_DATA_SZ,
                        0))) {
                _VTSP_TRACE(__FILE__, __LINE__);
                goto _error;
            }
#endif
#ifdef VTSP_ENABLE_RECORD
            OSAL_snprintf(qname, sizeof(qname), "vtsp-flowr%dq", flowIndex);
            if (NULL == (infc_ptr->flowDataToAppQ[streamId] =
                    OSAL_msgQCreate(
                        qname,
                        OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                        OSAL_DATA_STRUCT__VTSP_FlowMsg,
                        _VTSP_Q_FLOW_GET_NUM_MSG,
                        _VTSP_Q_FLOW_GET_DATA_SZ,
                        0))) {
                _VTSP_TRACE(__FILE__, __LINE__);
                goto _error;
            }
#endif
        }
    }

    /* Add the packetSendQ and packetRecvQ */
    if (NULL == (_VTSP_object_ptr->stunSendQ =
            OSAL_msgQCreate(
            "vtsp-pktsendq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    if (NULL == (_VTSP_object_ptr->stunRecvQ =
            OSAL_msgQCreate(
            "vtsp-pktrecvq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    
    if (NULL == (_VTSP_object_ptr->stunSendQVideo =
            OSAL_msgQCreate(
            "vtsp-pktsendqVideo",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VIDEO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    if (NULL == (_VTSP_object_ptr->stunRecvQVideo =
            OSAL_msgQCreate(
            "vtsp-pktrecvqVideo",
            OSAL_MODULE_VIDEO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }
    

    /* Creat semaphore for use in VTSP_shutdown() */
    if (NULL == (_VTSP_object_ptr->shutdownSemId = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        goto _error;
    }

    _VTSP_object_ptr->taskPriority = _VTSP_TASK_PRIORITY;
    if (NULL != taskConfig_ptr) {
        _VTSP_object_ptr->taskAddStackSz = taskConfig_ptr->vtspAddStackSize;
    }
    else {
        _VTSP_object_ptr->taskAddStackSz = 0;
    }

#if defined(OSAL_KERNEL_EMULATION) || defined (OSAL_VXWORKS) || \
    defined(OSAL_WINCE) || defined(OSAL_THREADX)
    /* 
     * On RTOS, call the VTSP_RT init function directly.
     * On Unix, this function is not called.
     */
    if (0 == (taskId = VTSPR_init())) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("VTSPR init err", (vint)taskId);
        goto _error;
    }
#endif

#if defined(OSAL_PTHREADS) || defined(OSAL_VXWORKS) || \
    defined(OSAL_WINCE) || defined(OSAL_THREADX)
    /*
     * Initialize the user space RTCP code.
     */
    rtcpContext_ptr = &(_VTSP_object_ptr->rtcpTaskContext);
    rtcpContext_ptr->msgTaskContext.stackSize = _VTSP_TASK_RTCP_MSG_STACKSIZE;
    rtcpContext_ptr->sktTaskContext.stackSize = _VTSP_TASK_RTCP_SKT_STACKSIZE;
    if (NULL != taskConfig_ptr) {
        rtcpContext_ptr->msgTaskContext.stackSize
                += taskConfig_ptr->rtcpAddStackSize;
        rtcpContext_ptr->sktTaskContext.stackSize
                += taskConfig_ptr->rtcpAddStackSize;
        rtcpContext_ptr->controlPort = taskConfig_ptr->rtcpInternalPort;
    }
    else {
        rtcpContext_ptr->controlPort = OSAL_netHtons(_VTSP_TASK_RTCP_CONTROL_PORT);
    }
        
    if (VTSP_OK != _VTSP_rtcpInit(rtcpContext_ptr, numInfc, numStreamId)) {
        goto _error;
    }
#endif

    if (NULL != profile_ptr) {
        /*
         * Loop through template data in profile ptr and config each set of data
         *
         * XXX
         */
    }

    *context_ptr = mem_ptr;

    return (VTSP_OK);

_error:
    /*
     * unallocate anything previously allocated
     */
    _VTSP_TRACE(__FILE__, __LINE__);
    VTSP_shutdown();
    return (VTSP_E_INIT);

}

/*
 * ======== VTSP_query() ========
 */
VTSP_QueryData *VTSP_query(
    void)
{

    if (NULL == _VTSP_object_ptr) {
        return (NULL);
    }

    return (&_VTSP_object_ptr->config);
}

/*
 * ======== VTSP_shutdown() ========
 */
VTSP_Return VTSP_shutdown(void)
{
    _VTSP_InfcObj *infc_ptr;
    vint           infc;                   /* must be signed */
    vint           streamId;               /* must be signed */

    _VTSP_VERIFY_INIT;
    
#if defined(OSAL_PTHREADS)
    /*
     * Shutdown the user space RTCP code.
     */
    if (VTSP_OK != _VTSP_rtcpShutdown(&(_VTSP_object_ptr->rtcpTaskContext))) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }
#endif  

    /*
     * if started, send shutdown msg
     * wait for IPC response w/ timeout
     */
    if (NULL != _VTSP_object_ptr->cmdQ) { 
        _VTSP_CmdMsg   cmd;
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("Stopping VTSPR", 0);
        cmd.code = _VTSP_CMD_SHUTDOWN;
        cmd.infc = VTSP_INFC_GLOBAL;
        _VTSP_putCmd(VTSP_INFC_GLOBAL, &cmd, 0);
    }

    /* Block on shutdown semaphore from event task */
    if (OSAL_SUCCESS == OSAL_semAcquire(_VTSP_object_ptr->shutdownSemId,
            10000)) {
        OSAL_semDelete(_VTSP_object_ptr->shutdownSemId);
    }
    else
    {
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("OSAL_TIMEOUT shutdown semaphore", 0);
    }

    /* Delay to allow application to finish cleanup */
    OSAL_taskDelay(1000);

    _VTSP_object_ptr->start = VTSP_E_SHUTDOWN;   /* Block access to VTSP */
    
    if (NULL != _VTSP_object_ptr->globalEventQ) { 
        OSAL_msgQDelete(_VTSP_object_ptr->globalEventQ);
    }
    if (NULL != _VTSP_object_ptr->cmdQ) { /* Delete Q */
        OSAL_msgQDelete(_VTSP_object_ptr->cmdQ);
    }
    if (NULL != _VTSP_object_ptr->cmdQVideo) {
        OSAL_msgQDelete(_VTSP_object_ptr->cmdQVideo);
    }
    if (NULL != _VTSP_object_ptr->evtQVideo) {
        OSAL_msgQDelete(_VTSP_object_ptr->evtQVideo);
    }
    if (NULL != _VTSP_object_ptr->stunSendQ) {
        OSAL_msgQDelete(_VTSP_object_ptr->stunSendQ);
    }
    if (NULL != _VTSP_object_ptr->stunRecvQ) {
        OSAL_msgQDelete(_VTSP_object_ptr->stunRecvQ);
    }
    if (NULL != _VTSP_object_ptr->stunSendQVideo) {
        OSAL_msgQDelete(_VTSP_object_ptr->stunSendQVideo);
    }
    if (NULL != _VTSP_object_ptr->stunRecvQVideo) {
        OSAL_msgQDelete(_VTSP_object_ptr->stunRecvQVideo);
    }
    for (infc = _VTSP_INFC_NUM - 1; infc >= 0; infc--) { 
        /* Remove Q from Group first */
        infc_ptr = &_VTSP_object_ptr->infc[infc];
        if (NULL != infc_ptr->eventQ) { /* Delete Event Q */
            OSAL_msgQDelete(infc_ptr->eventQ);
        }
        if (NULL != infc_ptr->cidQ) { /* Delete CID Q */
            OSAL_msgQDelete(infc_ptr->cidQ);
        }
        /* Delete Stream Q's */
        for (streamId = _VTSP_object_ptr->config.stream.numPerInfc - 1;
                streamId >= 0; streamId--) { 
            if (NULL != infc_ptr->flowDataToAppQ[streamId]) { 
                OSAL_msgQDelete(infc_ptr->flowDataToAppQ[streamId]);
            }
#ifdef VTSP_ENABLE_PLAY
            if (NULL != infc_ptr->flowDataFromAppQ[streamId]) { 
                OSAL_msgQDelete(infc_ptr->flowDataFromAppQ[streamId]);
            }
#endif
#ifdef VTSP_ENABLE_RECORD
            if (NULL != infc_ptr->flowDataToAppQ[streamId]) { 
                OSAL_msgQDelete(infc_ptr->flowDataFromAppQ[streamId]);
            }
#endif
        }
    }
    /* Delete group queue */
    if (NULL != _VTSP_object_ptr->infcEventQGrp) {
        OSAL_msgQGrpDelete(&(_VTSP_object_ptr->infcEventQGrp));
    }

    OSAL_memFree(_VTSP_object_ptr, 0);

    _VTSP_object_ptr = NULL;

    _VTSP_TRACE("Exiting VTSP Shutdown", 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_start() ========
 */
VTSP_Return VTSP_start(void)
{
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;

    /* Protect against starting again */
    if (VTSP_OK == _VTSP_object_ptr->start) { 
        return (VTSP_E_SHUTDOWN);
    }

    cmd.code = _VTSP_CMD_START;
    cmd.infc = VTSP_INFC_GLOBAL;
    cmd.msg.arg.arg1 = 0;   /* reserved for future use */

    if(VTSP_E_RESOURCE == _VTSP_putCmd(VTSP_INFC_GLOBAL, &cmd, 0))
    { 
        return (VTSP_E_TIMEOUT);
    } 

    _VTSP_object_ptr->start = VTSP_OK;

    return (VTSP_OK);
}

/*
 * ======== VTSP_getVersion() ========
 */
VTSP_Return VTSP_getVersion(
    VTSP_Version   *data_ptr)
{
    _VTSP_CmdMsg cmd;

    /* This function does not require VTSP to be initialized first */
    if (NULL == data_ptr) { 
        return (VTSP_E_ARG);
    }

    OSAL_strncpy(data_ptr->string_ptr, D2_Release_VTSP, 
            sizeof(data_ptr->string_ptr));

    /* XXX must properly fill in type, major, minor, point */
    data_ptr->type = 'R';
    data_ptr->major = 2;
    data_ptr->minor = 1;
    data_ptr->point = 0;

    /* 
     * If VTSP is initialized and started, then send cmd to report 
     * VTSP_RT version.
     */
    if ((NULL != _VTSP_object_ptr) &&
            VTSP_OK == _VTSP_object_ptr->start) { 
        cmd.code = _VTSP_CMD_GET_VERSION;
        cmd.infc = VTSP_INFC_GLOBAL;
        cmd.msg.arg.arg1 = 0;   /* reserved for future use */

        if(VTSP_E_RESOURCE == _VTSP_putCmd(VTSP_INFC_GLOBAL, &cmd, 0))
        { 
            return (VTSP_E_TIMEOUT);
        }
    }

    return (VTSP_OK);
}
