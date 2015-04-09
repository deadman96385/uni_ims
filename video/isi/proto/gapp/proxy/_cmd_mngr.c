/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30347 $ $Date: 2014-12-11 12:16:45 +0800 (Thu, 11 Dec 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include <csm_event.h>
#include <rpm.h>
#include <pdu_hlpr.h>
#include <pdu_3gpp.h>

#include "proxy.h"
#include "_cmd_mngr.h"

/*
 * A lookup table that defines and translates AT commands related to calling
 * that are of interest to CSM.  AT commands in this table are routed to the
 * CSM module for further processing.  AT commands related to calling that are
 * not defined in this table end up getting routed directly to the underlying
 * GSM module (AT interface).
 */
static PRXY_CallIntExt _PRXY_AT_TO_REASON[] = {
    {   CSM_CALL_REASON_AT_CMD_DIAL, "CSM_CALL_REASON_AT_CMD_DIAL",
        "ATD", 3  },
    {   CSM_CALL_REASON_AT_CMD_REPORT, "CSM_CALL_REASON_AT_CMD_REPORT",
        "AT+CLCC", 7  },
    {   CSM_CALL_REASON_AT_CMD_ANSWER, "CSM_CALL_REASON_AT_CMD_ANSWER",
        "ATA",        3  },
    {   CSM_CALL_REASON_AT_CMD_END, "CSM_CALL_REASON_AT_CMD_END", "ATH",
        3  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 1", "AT+CHCCS",    8  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 1", "AT+CHLD=11", 10  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 2", "AT+CHLD=12", 10  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 3", "AT+CHLD=13", 10  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 4", "AT+CHLD=14", 10  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 5", "AT+CHLD=15", 10  },
    {   CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,
        "CSM_CALL_REASON_AT_CMD_RELEASE_AT_X 6", "AT+CHLD=16", 10  },
    {   CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE,
        "CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE", "AT+CHLD=1",  9  },
    {   CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING,
        "CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING",  "AT+CHLD=0",  9  },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 1", "AT+CHLD=21", 10 },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 2", "AT+CHLD=22", 10 },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 3", "AT+CHLD=23", 10 },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 4", "AT+CHLD=24", 10 },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 5", "AT+CHLD=25", 10 },
    {   CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
        "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X 6", "AT+CHLD=26", 10 },
    {   CSM_CALL_REASON_AT_CMD_SWAP, "CSM_CALL_REASON_AT_CMD_SWAP",
        "AT+CHLD=2", 9  },
    {   CSM_CALL_REASON_AT_CMD_DIGIT, "CSM_CALL_REASON_AT_CMD_DIGIT",
        "AT$VTS=", 7  },
    {   CSM_CALL_REASON_AT_CMD_DIGIT, "CSM_CALL_REASON_AT_CMD_DIGIT",
        "AT+VTS=", 7  },
    {   CSM_CALL_REASON_AT_CMD_END, "CSM_CALL_REASON_AT_CMD_END",
        "AT+CHUP", 7  },
    {   CSM_CALL_REASON_AT_CMD_CONFERENCE,
        "CSM_CALL_REASON_AT_CMD_CONFERENCE", "AT+CHLD=3", 9  },
    {   CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS,
        "CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS", "+CIREPH:1", 9  },
    {   CSM_CALL_REASON_AT_CMD_SRVCC_FAILED,
        "CSM_CALL_REASON_AT_CMD_SRVCC_FAILED", "+CIREPH:2", 9  },
    {   CSM_CALL_REASON_AT_CMD_DIAL, "CSM_CALL_REASON_AT_CMD_DIAL",
        "AT+CDU", 6  },
    {   CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL,
        "CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL",
        "AT+CCMMD", 8  },
    {   CSM_CALL_REASON_VE_AEC_CMD,
        "CSM_CALL_REASON_VE_AEC_CMD", "AT\%MEDIA=0", 10},
    {   CSM_CALL_REASON_VE_GAIN_CTRL,
        "CSM_CALL_REASON_VE_GAIN_CTRL", "AT\%MEDIA=1", 10},
    {   CSM_CALL_REASON_VE_CN_GAIN_CTRL,
        "CSM_CALL_REASON_VE_CN_GAIN_CTRL", "AT\%MEDIA=2", 10},
    {   CSM_CALL_REASON_AT_CMD_CONF_DIAL,
        "CSM_CALL_REASON_AT_CMD_CONF_DIAL", "AT%CGU=1", 8  },
    {   CSM_CALL_REASON_AT_CMD_CONF_ADD_PARTY,
        "CSM_CALL_REASON_AT_CMD_CONF_ADD_PARTY", "AT%CGU=4", 8  },
};

/*
 * A lookup table that defines and translates AT commands related to SMS
 * that are of interest to CSM.  AT commands in this table are routed to the
 * CSM module for further processing.  AT commands related to SMS that are
 * not defined in this table end up getting routed directly to the underlying
 * GSM module (AT interface).
 */
static PRXY_SmsIntExt _PRXY_SMS_AT_TO_REASON[] = {
    {   CSM_SMS_REASON_AT_CMD_SEND_MESSAGE, "CSM_EVENT_SMS_REASON_CMD_SEND",
        "AT+CMGS", 7  },
};

/*
 * A lookup table that defines and translates AT commands related to SUPSRV
 * that are of interest to CSM.  AT commands in this table are routed to the
 * CSM module for further processing.  AT commands related to SUPSRV that are
 * not defined in this table end up getting routed directly to the underlying
 * GSM module (AT interface).
 */
static PRXY_SupSrvIntExt _PRXY_SUPSRV_AT_TO_REASON[] = {
    /*
     * XXX Disable supplementary service at command sets
     * beause currently there is no AS to inter-op.
     * Filter those at command sets causes no cid on cs call.
     * Currently only enable it when doning unit test on pent0.
     */
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_OIP,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_OIP", "AT+CLIP", 7  },
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_OIR,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_OIR", "AT+CLIR", 7  },
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_TIP,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_TIP", "AT+COLP", 7  },
    /* AT+COLR is for testing TIR */
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_TIR,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_TIR", "AT+COLR", 7  },
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_CW,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_CW",  "AT+CCWA", 7  },
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_CF,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_CF",  "AT+CCFC", 7  },
    {   CSM_EVENT_SUPSRV_REASON_AT_CMD_CB,
        "CSM_EVENT_SUPSRV_REASON_AT_CMD_CB",  "AT+CLCK", 7  },
};

static PRXY_UssdIntExt _PRXY_USSD_AT_TO_REASON[] = {
    {   CSM_USSD_REASON_AT_CMD_SEND_USSD, "CSM_USSD_REASON_AT_CMD_SEND_USSD",
        "AT+CUSD", 7  },
};

static PRXY_ServiceIntExt _PRXY_SERVICE_AT_TO_REASON[] = {
    {   CSM_SERVICE_REASON_SET_IMPU,    "CSM_SERVICE_REASON_SET_IMPU",
        "AT%IMPU=", 8},
    {   CSM_SERVICE_REASON_SET_IMPI,    "CSM_SERVICE_REASON_SET_IMPI",
        "AT%IMPI=", 8},
    {   CSM_SERVICE_REASON_SET_DOMAIN,  "CSM_SERVICE_REASON_SET_DOMAIN",
        "AT\%DOMAIN=", 10},
    {   CSM_SERVICE_REASON_SET_PCSCF,   "CSM_SERVICE_REASON_SET_PCSCF",
        "AT%PCSCF=", 9},
    {   CSM_SERVICE_REASON_UPDATE_CGI,  "CSM_SERVICE_REASON_UPDATE_CGI",
        "AT\%CGI=", 7},
    {   CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER,
        "CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER",
        "AT\%AUDIOCONFSRV=", 16},
    {   CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER,
        "CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER",
        "AT\%VIDEOCONFSRV=", 16},
    {   CSM_SERVICE_REASON_IMS_ENABLE,  "CSM_SERVICE_REASON_IMS_ENABLE",
        "AT%IMSEN=1", 10},
    {   CSM_SERVICE_REASON_IMS_DISABLE, "CSM_SERVICE_REASON_IMS_DISABLE",
        "AT%IMSEN=0", 10},
    {   CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS,
        "CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS",
        "AT\%AKARESP=0", 12},
    {   CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE,
        "CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE",
        "AT\%AKARESP=1", 12},
    {   CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE,
        "CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE",
        "AT\%AKARESP=2", 12},
    {   CSM_SERVICE_REASON_SET_EMGCY_PCSCF,
        "CSM_SERVICE_REASON_SET_EMGCY_PCSCF",
        "AT\%EPCSCF=", 10},
};

static PRXY_RadioIntExt _PRXY_RADIO_AT_TO_REASON[] = {
    {   CSM_RADIO_REASON_IP_CHANGE,
        "CSM_RADIO_REASON_IP_CHANGE",
        "AT\%IPADDR=0,\"", 12},
    {   CSM_RADIO_REASON_IP_CHANGE_EMERGENCY,
        "CSM_RADIO_REASON_IP_CHANGE_EMERGENCY",
        "AT\%IPADDR=1,\"", 12},
    {   CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY,
        "CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY",
        "AT\%ECONF=", 9},
};

const char  _PRXY_hangupReason_002[] = "488 Not Acceptable Here";
const char  _PRXY_hangupReason_003[] = "486 Busy Here";

/**
 * ======== _PRXY_cmdMngrTokenFetch ========
 * This routine is to fetch string
 * from src_ptr to dst_ptr if token_ptr found.
 * 
 * src_ptr    A pointer to a NULL terminated string containing an AT command.
 * token_ptr  Token to be searched.
 * prefix_ptr Prefix string added to dst_ptr.
 * dst_ptr    Result string pointer.
 *
 * Returns:
 * OSAL_TRUE  Token found.
 * OSAL_FALSE otherwise case.
 */
static OSAL_Boolean _PRXY_cmdMngrTokenFetch(
    const char       *src_ptr,
    const char       *token_ptr,
    const char       *prefix_ptr,
    char             *dst_ptr) 
{
    char *content_ptr;
    char *contentEnd_ptr;

    /* Search starting point of content*/
    if ((content_ptr = OSAL_strscan(src_ptr, token_ptr)) == NULL) {
        return (OSAL_FALSE);
    }
    content_ptr += OSAL_strlen(token_ptr);
    
    /* Search ending point of content*/
    contentEnd_ptr = OSAL_strncasescan(content_ptr, 
            OSAL_strlen(content_ptr), "\\0D");
    if (contentEnd_ptr == NULL) {
        contentEnd_ptr = OSAL_strncasescan(content_ptr, 
                OSAL_strlen(content_ptr), "\\0A");
    }
    if (NULL == contentEnd_ptr) {
        // Can't find newline. The remaing string considered as content.
        contentEnd_ptr = content_ptr + OSAL_strlen(content_ptr);
    }

    /* Calculate space */
    if (OSAL_strlen(dst_ptr) + OSAL_strlen(prefix_ptr) + 
            (contentEnd_ptr - content_ptr) > CSM_CODER_DESCRIPTION_STRING_SZ) {
        return (OSAL_FALSE);
    }

    /* Copy content */
    OSAL_strcpy(dst_ptr + OSAL_strlen(dst_ptr), prefix_ptr);
    OSAL_strncpy(dst_ptr + OSAL_strlen(dst_ptr), content_ptr, 
            contentEnd_ptr - content_ptr + 1);
    OSAL_strcpy(dst_ptr + OSAL_strlen(dst_ptr), ";");
    return (OSAL_TRUE);
}

/**
 * ======== _PRXY_cmdMngrParaseSDPPreprocessing ========
 * Preprocessing AT command
 * 
 * mediaDesc_ptr A pointer to a NULL terminated string containing an AT command.
 *
 */
static void _PRXY_cmdMngrParaseSDPPreprocessing(
    char             *mediaDesc_ptr)
{
    char             *token_ptr;
    char             *tokenEnd_ptr;
    /* Remove SPACE after ; symbol */
    while ((token_ptr = OSAL_strscan(mediaDesc_ptr, "; ")) != 
            NULL) {
        tokenEnd_ptr = token_ptr + 2;
        token_ptr += 1;
        OSAL_memMove(token_ptr, tokenEnd_ptr, OSAL_strlen(tokenEnd_ptr));
        token_ptr[OSAL_strlen(tokenEnd_ptr)] = '\0';
    }
    /* Replace sprop-parameter-sets to sps */
    while ((token_ptr = OSAL_strscan(mediaDesc_ptr, "sprop-parameter-sets")) != 
            NULL) {
        tokenEnd_ptr = token_ptr + OSAL_strlen("sprop-parameter-sets");
        OSAL_memCpy(token_ptr, "sps", 3);
        token_ptr += 3;
        OSAL_memMove(token_ptr, tokenEnd_ptr, OSAL_strlen(tokenEnd_ptr));
        token_ptr[OSAL_strlen(tokenEnd_ptr)] = '\0';
    }
    /* Remove the last " symbol */
    if (mediaDesc_ptr[OSAL_strlen(mediaDesc_ptr) - 1] == '"') {
        mediaDesc_ptr[OSAL_strlen(mediaDesc_ptr) - 1] = '\0';
    }
}
/**
 * ======== _PRXY_cmdMngrParaseSDP ========
 * Parse SDP media description from CDEFMP AT command
 *
 * Returns:
 * PRXY_RETURN_OK:      OK
 * PRXY_RETURN_FAILED: Fail to parse
 */
static vint _PRXY_cmdMngrParaseSDP(
    PRXY_CommandMngr *cmdMngr_ptr, 
    CSM_InputEvent   *csmEvt_ptr, 
    vint              mpIdx, 
    char             *mediaDesc_ptr) 
{
    char             *rtpmap_ptr;
    char             *nextRtpmap_ptr;
    const char       *coderName_ptr;
    const char       *coderNameEnd_ptr;
    int               payloadType;
    int               coderNum = 0;
    CSM_InputService *serviceEvt_ptr;
    char              tokenBuf[PRXY_MEDIA_TOKEN_STRING_SZ_MAX + 1];
    char             *coderDesc_ptr;
    int               rate = 20;

    serviceEvt_ptr = &csmEvt_ptr->evt.service;
    serviceEvt_ptr->u.coder.coderNum = 0;

    /* Preprocessing the input AT command */
    _PRXY_cmdMngrParaseSDPPreprocessing(mediaDesc_ptr);
    
    /* Search media profile*/
    if (NULL != OSAL_strscan(mediaDesc_ptr, PRXY_MEDIA_PROFILE_STR_AUDIO)) {
        cmdMngr_ptr->mediaProfiles[mpIdx - 1] |= CSM_CALL_SESSION_TYPE_AUDIO;
    }
    if (NULL != OSAL_strscan(mediaDesc_ptr, PRXY_MEDIA_PROFILE_STR_VIDEO)) {
        cmdMngr_ptr->mediaProfiles[mpIdx - 1] |= CSM_CALL_SESSION_TYPE_VIDEO;
    }
    
    /*Search a=rtpmap*/
    rtpmap_ptr = OSAL_strscan(mediaDesc_ptr, PRXY_RTP_MAP_STR);
    if (rtpmap_ptr == NULL) {
        return (PRXY_RETURN_OK);
    }
    do {
        rtpmap_ptr += OSAL_strlen(PRXY_RTP_MAP_STR);
        /*
         * Search next rtpmap and 
         * add terminating null byte to seperate nextRtpmap_ptr and rtpmap_ptr.
         */
        nextRtpmap_ptr = OSAL_strscan(rtpmap_ptr, PRXY_RTP_MAP_STR);
        if (nextRtpmap_ptr != NULL) {
            *nextRtpmap_ptr = '\0';
        }

        /* Parse rtpmap */
        payloadType = OSAL_atoi(rtpmap_ptr);
        if (payloadType <= 0) {
             return (PRXY_RETURN_FAILED);
        }
        if (NULL == (coderName_ptr = OSAL_strchr(rtpmap_ptr, ' '))) {
            return (PRXY_RETURN_FAILED);
        }
        coderName_ptr++;
        if (NULL == (coderNameEnd_ptr = OSAL_strchr(coderName_ptr, '/'))) {
            return (PRXY_RETURN_FAILED);
        }
        OSAL_strncpy(serviceEvt_ptr->u.coder.coderName[coderNum], 
                coderName_ptr, coderNameEnd_ptr - coderName_ptr + 1);
        if (!OSAL_strncmp(coderName_ptr, "H264", 4)) {
            rate = 0;
        }
        coderDesc_ptr = serviceEvt_ptr->u.coder.coderDescription[coderNum];            
        OSAL_snprintf(coderDesc_ptr, CSM_CODER_DESCRIPTION_STRING_SZ, 
                "enum=%d;rate=%d;", payloadType, rate);
        serviceEvt_ptr->u.coder.coderPayloadType[coderNum] = payloadType;
        
        /* Parse a=fmtp*/
        OSAL_snprintf(tokenBuf, PRXY_MEDIA_TOKEN_STRING_SZ_MAX, 
                "%s%d ", PRXY_FORMAT_PARAMETER_STR, payloadType);
        _PRXY_cmdMngrTokenFetch(rtpmap_ptr, tokenBuf, "", coderDesc_ptr);

        /* Parse a=framesize */
        OSAL_snprintf(tokenBuf, PRXY_MEDIA_TOKEN_STRING_SZ_MAX, 
                "%s%d ", PRXY_FRAMESIZE_PARAMETER_STR, payloadType);
        _PRXY_cmdMngrTokenFetch(rtpmap_ptr, tokenBuf, "framesize=", 
                coderDesc_ptr);

        /* Parse a=framerate */
        OSAL_snprintf(tokenBuf, PRXY_MEDIA_TOKEN_STRING_SZ_MAX, 
                "%s", PRXY_FRAMERATE_PARAMETER_STR);
        _PRXY_cmdMngrTokenFetch(rtpmap_ptr, tokenBuf, "framerate=", 
                coderDesc_ptr);

        /* Remove seperate symbol on last character. */
        if (coderDesc_ptr[OSAL_strlen(coderDesc_ptr) - 1] == ';') {
            coderDesc_ptr[OSAL_strlen(coderDesc_ptr) - 1] = '\0';
        }
        coderNum++;
    } while((rtpmap_ptr = nextRtpmap_ptr) != NULL);
    serviceEvt_ptr->u.coder.coderNum = coderNum;
    return (PRXY_RETURN_OK);    
}
/**
 * ======== _PRXY_getHangUpReasonDesc ========
 * This routine is used to get reason description string of call termination from
 * cause index number defined in 27.007 or operator requirement.
 *
 * cause An index number of call termination.
 * reasonDesc A string pointer to store the termination reason string
 *
 * Returns:
 * PRXY_RETURN_OK The cause has translated into a string.
 */

static vint _PRXY_getHangUpReasonDesc(
    int      cause,
    char    *reasonDesc_ptr)
{
    switch(cause) {
        case 2:
            OSAL_strncpy(reasonDesc_ptr, _PRXY_hangupReason_002,
                CSM_EVENT_STRING_SZ);
            break;
        case 3:
            OSAL_strncpy(reasonDesc_ptr, _PRXY_hangupReason_003,
                CSM_EVENT_STRING_SZ);
            break;
        /* 0 and 1 is considered normal ending call and don't have reasonDesc */
        default:
            break;
    }
    return (PRXY_RETURN_OK);
}
/**
 * ======== _PRXY_cmdMngrIsEmergencyNumber ========
 * This routine is used to look for if the address is emergency number.
 *
 * Returns:
 * OSAL_TRUE: It is emergency number.
 * OSAL_FALSE: It is not emergency number.
 */
OSAL_Boolean _PRXY_cmdMngrIsEmergencyNumber(
    PRXY_CommandMngr   *cmdMngr_ptr,
    char               *address)
{
    vint    idx;
    char    number[PRXY_EMERGENCY_NUMBER_STR_SZ];

    if (RPM_RETURN_OK != RPM_getNameFromAddress(address, number,
            PRXY_EMERGENCY_NUMBER_STR_SZ)) {
        PRXY_dbgPrintf("Cannot find the number or name from %s\n", address);
        return (OSAL_FALSE);
    }


    /* Look for emergency number table */
    for (idx = 0; idx < PRXY_EMERGENCY_NUMBER_SZ; idx++) {
        if (0 == cmdMngr_ptr->emergencyNumbers[idx][0]) {
            continue;
        }
        if (0 == OSAL_strncmp(cmdMngr_ptr->emergencyNumbers[idx], number,
                OSAL_strlen(cmdMngr_ptr->emergencyNumbers[idx]))) {
            PRXY_dbgPrintf("%s is an emergency number.\n", address);
            return (OSAL_TRUE);
        }
    }
    return (OSAL_FALSE);
}

/**
 * ======== _PRXY_cmdMngrParseParticipants ========
 * This routine is used to parse the address list as participants list.
 *
 * addresses is assumed as "url1,url2,url3,..."
 * each url is similar to +CDU syntax and could be "tel:123;prm=value" or "sip:name@org"
 *
 * Returns:
 * PRXY_RETURN_OK:      OK
 * PRXY_RETURN_FAILED: Fail to parse
 */
vint _PRXY_cmdMngrParseParticipants(
    char               *addresses,
    CSM_InputCall     *callEvt_ptr)
{
    const char      *pos_ptr;
    char            *end_ptr;

    pos_ptr = addresses;
    if ('"' != *pos_ptr) {
        return (PRXY_RETURN_FAILED);
    }
    pos_ptr += 1; /* Advance '"' */
    if (NULL == (end_ptr = OSAL_strchr(pos_ptr, '"'))) {
        return (PRXY_RETURN_FAILED);
    }
        
    /* end the string. */
    *end_ptr = 0;
    OSAL_snprintf(callEvt_ptr->u.remoteParticipants, 
            CSM_EVENT_LONG_STRING_SZ, "%s", pos_ptr);
    *end_ptr = '"';
    return (PRXY_RETURN_OK);
}

/**
 * ======== _PRXY_cmdMngrParseAtCommand ========
 * This routine will determine in an AT command is related to calling
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 */
vint _PRXY_cmdMngrParseAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    vint             x;
    const char      *pos_ptr;
    char            *end_ptr;
    vint             mpIdx = 0;
    vint             negStatus;
    CSM_InputCall   *callEvt_ptr;
    PRXY_CallIntExt *command_ptr = NULL;
    vint             size = sizeof(_PRXY_AT_TO_REASON) / sizeof(PRXY_CallIntExt);
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_AT_TO_REASON[x].at_ptr, at_ptr, 
                _PRXY_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_AT_TO_REASON[x];
            break;
        }
    }
    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }
    /* Check if it's just polling for existence. */
    pos_ptr = at_ptr + command_ptr->atLen;
    if (*pos_ptr == '?' || NULL != OSAL_strscan(pos_ptr, "=?")) {
        return (PRXY_RETURN_FAILED);
    }

    callEvt_ptr = &csmEvt_ptr->evt.call;
    csmEvt_ptr->type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = command_ptr->reason;
    OSAL_strncpy(callEvt_ptr->reasonDesc, command_ptr->reasonDesc_ptr, 
            sizeof(callEvt_ptr->reasonDesc));
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->cidType = CSM_CALL_CID_TYPE_NONE;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    PRXY_dbgPrintf("%s %d: processing AT command:%s\n", __FUNCTION__,
            __LINE__, at_ptr);

    /* 
     * Let's look for 'reasons' that require more parsing and take 
     * care of those. 
     */
    switch (command_ptr->reason) {
        case CSM_CALL_REASON_AT_CMD_END:
            callEvt_ptr->reasonDesc[0] = '\0';
            break;
        case CSM_CALL_REASON_AT_CMD_DIGIT:
            /* Then let's get the digit. */
            pos_ptr = at_ptr + command_ptr->atLen;
            callEvt_ptr->u.digit = *pos_ptr;
            if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                /* Then we have a duration. */
                end_ptr++;
                callEvt_ptr->extraArgument = OSAL_atoi(end_ptr);
                if (1 == callEvt_ptr->extraArgument) {
                    callEvt_ptr->extraArgument = MAX_INT32;
                }
            }
            break;
        case CSM_CALL_REASON_AT_CMD_CONF_DIAL:
            /* Then let's get %CGU=action,"<URI1>,<URI2>, ..."*/
            pos_ptr = at_ptr + command_ptr->atLen + 1; /* plus 1 for ',' */
            _PRXY_cmdMngrParseParticipants((char *)pos_ptr, callEvt_ptr);
            /* Search end symbol of participants */
            pos_ptr = OSAL_strchr(pos_ptr+1, '"');

            /* Check media index */
            if ((pos_ptr = OSAL_strchr(pos_ptr, ',')) != NULL) {
                callEvt_ptr->callSessionType =
                    cmdMngr_ptr->mediaProfiles[OSAL_atoi(pos_ptr + 1) - 1];    
            }
            else {
                callEvt_ptr->callSessionType =
                        cmdMngr_ptr->mediaProfiles[0];
            }

            PRXY_dbgPrintf("group call to dial:%s sessionType:%d\n",
                    callEvt_ptr->u.remoteParticipants,
                    callEvt_ptr->callSessionType);
            break;
        case CSM_CALL_REASON_AT_CMD_CONF_ADD_PARTY:
            /* Then let's get %CGU=action,"<URI>"*/
            pos_ptr = at_ptr + command_ptr->atLen + 1; /* plus 1 for ',' */
            _PRXY_cmdMngrParseParticipants((char *)pos_ptr, callEvt_ptr);

            /* filter out illegal parameter, allow only one participant */
            if (NULL != OSAL_strchr(callEvt_ptr->u.remoteParticipants, ',')) {
                return (PRXY_RETURN_FAILED);
            }
            PRXY_dbgPrintf("group call to add another party:%s\n",
                    callEvt_ptr->u.remoteParticipants);
            break;
        case CSM_CALL_REASON_AT_CMD_DIAL:
            /* Check if it's AT+CDU */
            if (0 == OSAL_strncmp(PRXY_AT_CMD_CDU, _PRXY_AT_TO_REASON[x].at_ptr,
                    _PRXY_AT_TO_REASON[x].atLen)) {
                /* It's AT+CDU. Get media profile. */
                pos_ptr = at_ptr + command_ptr->atLen + 1; /* plus 1 for '=' */
                if ('1' != *pos_ptr) {
                    /* Only support dial */
                    return (PRXY_RETURN_FAILED);
                }

                pos_ptr += 3; /* Advance "1,/"" */
                if (NULL == (end_ptr = OSAL_strchr(pos_ptr, '"'))) {
                    return (PRXY_RETURN_FAILED);
                }
                    
                /* end the string. */
                *end_ptr = 0;
                OSAL_snprintf(callEvt_ptr->u.remoteAddress, 
                        CSM_EVENT_STRING_SZ, "%s", pos_ptr);

                /*
                 * Check if use alias for the call. Example:
                 * AT+CDU=1,"tel:1234567;useAlias"
                 */
                if (NULL != OSAL_strscan(pos_ptr, PRXY_URI_PARAM_USE_ALIAS)) {
                    callEvt_ptr->cidType = CSM_CALL_CID_TYPE_USE_ALIAS;
                }
                    
                pos_ptr = end_ptr + 1;
                if (NULL != (pos_ptr = OSAL_strchr(pos_ptr, ','))) {
                    pos_ptr++;
                    if (NULL != (pos_ptr = OSAL_strchr(pos_ptr, ','))) {
                        pos_ptr++;
                        /* There is mpIdx. */
                        /* Assume mpidx is only one digit for now. */
                        mpIdx = OSAL_atoi(pos_ptr);
                    }
                }
                if (0 == mpIdx) {
                    /* no media profile defined, used first one. */
                    mpIdx = 1;
                }

                PRXY_dbgPrintf("AT+CDU to:%s media profile id:%d\n",
                        callEvt_ptr->u.remoteAddress, mpIdx);

                /* Map mpIdx to session type. */
                callEvt_ptr->callSessionType =
                        cmdMngr_ptr->mediaProfiles[mpIdx - 1];

                /* Check range. */
                if ((1 > mpIdx) || (PRXY_MEDIA_PROFILE_SIZE < mpIdx)) {
                    /* Return error */
                    PRXY_dbgPrintf("media profile id exceeds range\n");
                    return (PRXY_RETURN_FAILED);
                }
                /* Check CLIR_OIR, with assertion that mpIdx is entered */
                if ((NULL != pos_ptr) && (NULL != (pos_ptr = OSAL_strchr(pos_ptr, ',')))) {
                    pos_ptr++;
                    if (1 == OSAL_atoi(pos_ptr)) {
                        callEvt_ptr->cidType = CSM_CALL_CID_TYPE_INVOCATION;
                    }
                    else if (2 == OSAL_atoi(pos_ptr)) {
                        callEvt_ptr->cidType = CSM_CALL_CID_TYPE_SUPPRESSION;
                    }
                }
            }
            else {
                /* It's ATD. */

                /* Then let's get the address and the privacy setting. */
                pos_ptr = at_ptr + command_ptr->atLen;
                end_ptr = OSAL_strchr(pos_ptr, 'I');
                if (NULL != end_ptr) {
                    /* Then set privacy. */
                    callEvt_ptr->cidType = CSM_CALL_CID_TYPE_INVOCATION;
                    *end_ptr = 0;
                }
                end_ptr = OSAL_strchr(pos_ptr, 'i');
                if (NULL != end_ptr) {
                    /* Then set privacy. */
                    callEvt_ptr->cidType = CSM_CALL_CID_TYPE_SUPPRESSION;
                    *end_ptr = 0;
                }
                end_ptr = OSAL_strchr(pos_ptr, ';');

                if (NULL != end_ptr) {
                    *end_ptr = 0;
                }
                OSAL_snprintf(callEvt_ptr->u.remoteAddress, 
                        CSM_EVENT_STRING_SZ, "%s", pos_ptr);
            }

            if (_PRXY_cmdMngrIsEmergencyNumber(cmdMngr_ptr,
                    callEvt_ptr->u.remoteAddress)) {
                callEvt_ptr->isEmergency = 1;
            }
            else {
                callEvt_ptr->isEmergency = 0;
            }
            PRXY_dbgPrintf("%s %d: %s to %s", __FUNCTION__, __LINE__,
                    _PRXY_AT_TO_REASON[x].at_ptr,                    
                    callEvt_ptr->u.remoteAddress);
            break;
        case CSM_CALL_REASON_AT_CMD_RELEASE_AT_X:
        case CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
            if (OSAL_strncmp(at_ptr, "AT+CHCCS", OSAL_strlen("AT+CHCCS")) == 0) {
                pos_ptr++;
                callEvt_ptr->u.callIndex = OSAL_atoi(pos_ptr);
                callEvt_ptr->reasonDesc[0] = '\0';
                if (callEvt_ptr->u.callIndex == 0) {
                    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_END;
                }
                if ((pos_ptr != NULL) && 
                            (NULL != (pos_ptr = OSAL_strchr(pos_ptr, ',')))) {
                        /* Then we have a cause index. */
                        _PRXY_getHangUpReasonDesc(OSAL_atoi(pos_ptr+1), 
                            callEvt_ptr->reasonDesc);
                }
            }
            else {
                /* Then let's get the call index. */
                pos_ptr = at_ptr + (command_ptr->atLen - 1);
                callEvt_ptr->u.callIndex = OSAL_atoi(pos_ptr);
                callEvt_ptr->reasonDesc[0] = '\0';
            }
            break;
        case CSM_CALL_REASON_AT_CMD_REPORT:
            /*
             * Set command issued for filtering out unsolicited call report.
             * XXX This should be removed once we have command id mechanisim,
             * we should check if there is command id in the call report event
             * for determining if it's unsolicited event.
             */
            cmdMngr_ptr->atClccIssued = OSAL_TRUE;
            break;
        case CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL:
            /* Ex: AT+CCMMD=1,2,"m=audio" */
            pos_ptr = at_ptr + command_ptr->atLen + 1;
            callEvt_ptr->u.callIndex = OSAL_atoi(pos_ptr);
            pos_ptr += 2;
            negStatus = OSAL_atoi(pos_ptr);
            callEvt_ptr->negStatus = negStatus;
            if ((1 == negStatus) || (2 == negStatus)) {
                /* Get media */
                pos_ptr += 3; /* Advance "1,/"" */
                if (NULL == (end_ptr = OSAL_strchr(pos_ptr, '"'))) {
                    return (PRXY_RETURN_FAILED);
                }
                callEvt_ptr->callSessionType = 0;
                if (NULL != OSAL_strscan(pos_ptr, PRXY_MEDIA_PROFILE_STR_AUDIO)) {
                    /* Set audio session type */
                    callEvt_ptr->callSessionType |= CSM_CALL_SESSION_TYPE_AUDIO;
                }
                if (NULL != OSAL_strscan(pos_ptr, PRXY_MEDIA_PROFILE_STR_VIDEO)) {
                    /* Set video session type */
                    callEvt_ptr->callSessionType |= CSM_CALL_SESSION_TYPE_VIDEO;
                }
            }
            break;
        case CSM_CALL_REASON_VE_AEC_CMD:
            pos_ptr = at_ptr + command_ptr->atLen;
            pos_ptr = OSAL_strchr(pos_ptr, ',');
            if (NULL == pos_ptr) {
                OSAL_logMsg("%s:%d Error There is no value\n",
                        __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            pos_ptr++;
            callEvt_ptr->u.aecEnable = OSAL_atoi(pos_ptr);
            break;
        case CSM_CALL_REASON_VE_GAIN_CTRL:
            pos_ptr = at_ptr + command_ptr->atLen;
            pos_ptr = OSAL_strchr(pos_ptr, ',');
            if (NULL == pos_ptr) {
                OSAL_logMsg("%s:%d Error There is tx gain value\n",
                        __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            pos_ptr++;
            callEvt_ptr->u.gain.tx = OSAL_atoi(pos_ptr);
            pos_ptr = OSAL_strchr(pos_ptr, ',');
            if (NULL == pos_ptr) {
                OSAL_logMsg("%s:%d Error There is rx gain value\n",
                        __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            pos_ptr++;
            callEvt_ptr->u.gain.rx = OSAL_atoi(pos_ptr);
            break;
        case CSM_CALL_REASON_VE_CN_GAIN_CTRL:
            pos_ptr = at_ptr + command_ptr->atLen;
            pos_ptr++;
            callEvt_ptr->u.gain.tx = OSAL_atoi(pos_ptr);
            break;
        default:
            break;
    }
    return (PRXY_RETURN_OK);
}

/**
 * ======== _PRXY_cmdMngrParseSmsAtCommand ========
 * This routine will determine in an AT command is related to SMS
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * cmdMngr_ptr A pointer to the PRXY_CommandMngr object that manages
 * AT command transactions related to SMS traffic'ing.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 * PRXY_RETURN_WAIT Indicates that more data pertaining to the AT
 * command needs to be read from the AT command interface. 'PRXY_RETURN_WAIT'
 * is typically needed when sending SMS as it may take a few reads of the AT
 * interface to get all the text associated with the AT command that sends SMS.
 */
vint _PRXY_cmdMngrParseSmsAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    vint x;
    const char      *pos_ptr;
    CSM_InputSms    *smsEvt_ptr;
    PRXY_SmsIntExt  *command_ptr = NULL;
    char            *address;
    vint size = sizeof(_PRXY_SMS_AT_TO_REASON) / sizeof(PRXY_SmsIntExt);

    if (PRXY_SMS_STATE_READ_PDU == cmdMngr_ptr->sms.state) {

        csmEvt_ptr->type = CSM_EVENT_TYPE_SMS;
        /* Populate the rest of the CSM Event */
        smsEvt_ptr = &csmEvt_ptr->evt.sms;
        smsEvt_ptr->reason = CSM_SMS_REASON_AT_CMD_SEND_MESSAGE;
        smsEvt_ptr->type = cmdMngr_ptr->sms.smsType;

        if (PRXY_SMS_PDU_MODE_TEXT == cmdMngr_ptr->sms.pduMode) {
            /* Check if the message size is too large */
            if ((vint)sizeof(smsEvt_ptr->message) <=
                    OSAL_strlen(smsEvt_ptr->message) + OSAL_strlen(at_ptr)) {
                /* PDU body is too large, response an error */
                /* Clean up command state */
                cmdMngr_ptr->sms.state = PRXY_SMS_STATE_NONE;
                return (PRXY_RETURN_FAILED);
            }

            /* Append data to pdu */
            OSAL_snprintf(smsEvt_ptr->message +
                    OSAL_strlen(smsEvt_ptr->message),
                    CSM_SMS_STRING_SZ - OSAL_strlen(smsEvt_ptr->message), "%s",
                    at_ptr);
        }
        else {
            /* Check if the pdu size is too large */
            if ((vint)sizeof(smsEvt_ptr->pdu) <=
                    OSAL_strlen(smsEvt_ptr->pdu) + OSAL_strlen(at_ptr)) {
                /* PDU body is too large, response an error */
                /* Clean up command state */
                cmdMngr_ptr->sms.state = PRXY_SMS_STATE_NONE;
                return (PRXY_RETURN_FAILED);
            }

            /* Append data to pdu */
            OSAL_snprintf(smsEvt_ptr->pdu + OSAL_strlen(smsEvt_ptr->pdu),
                    CSM_SMS_STRING_SZ - OSAL_strlen(smsEvt_ptr->pdu), "%s",
                    at_ptr);
        }
        if (PRXY_AT_RESPONSE_CTRL_Z != at_ptr[OSAL_strlen(at_ptr) - 1]) {
            PRXY_dbgPrintf("%s:%d Not get the end Ctrl-Z of PDU data yet. %s",
                    __FUNCTION__, __LINE__, at_ptr);
            /* Not the end of pdu, keep waiting */
            return (PRXY_RETURN_CONTINUE);
        }

        // Remove CTRL_Z symbol.
        smsEvt_ptr->message[OSAL_strlen(smsEvt_ptr->message) - 1] = '\0';
        
        if (PRXY_SMS_PDU_MODE_TEXT == cmdMngr_ptr->sms.pduMode) {
            smsEvt_ptr->msgLen = OSAL_strlen(smsEvt_ptr->message);
        }
        else {
            smsEvt_ptr->msgLen = OSAL_strlen(smsEvt_ptr->pdu);

            /* Get the remote address and message pdu is here. */
            PDU_3gppGetSubmitPduMessageAndAddress(smsEvt_ptr->pdu,
                    smsEvt_ptr->remoteAddress,
                    smsEvt_ptr->message, CSM_SMS_STRING_SZ);
        }

        PRXY_dbgPrintf("%s:%d Sending:%s", __FUNCTION__, __LINE__, 
                smsEvt_ptr->message);

        /* Set up reason */
        OSAL_strncpy(smsEvt_ptr->reasonDesc, "CSM_EVENT_SMS_REASON_CMD_SEND", 
                sizeof(smsEvt_ptr->reasonDesc));
        /* Clean up command state */
        cmdMngr_ptr->sms.state = PRXY_SMS_STATE_NONE;
        /* then we issued a command, let's wait for a response. */
        return (PRXY_RETURN_WAIT);
    }

    /* Other wise let's self we are interested in this command. */
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_SMS_AT_TO_REASON[x].at_ptr, at_ptr, 
                _PRXY_SMS_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_SMS_AT_TO_REASON[x];
            break;
        }
    }
    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }
    /* Check if it's just polling for existence. */
    pos_ptr = at_ptr + command_ptr->atLen;
    if (*pos_ptr == '?' || NULL != OSAL_strscan(pos_ptr, "=?")) {
        return (PRXY_RETURN_FAILED);
    }

    PRXY_dbgPrintf("%s %d: processing SMS AT command:%s", __FUNCTION__, 
            __LINE__, at_ptr);

    /* 
     * Let's look for 'reasons' that require more parsing and take 
     * care of those. 
     */
    switch (command_ptr->reason) {
        case CSM_SMS_REASON_AT_CMD_SEND_MESSAGE:
            PRXY_dbgPrintf("%s %d: returning OK", __FUNCTION__, __LINE__);
            /* If PDU mode is TEXT, fill in remote address */
            if (PRXY_SMS_PDU_MODE_TEXT == cmdMngr_ptr->sms.pduMode) {
                address = csmEvt_ptr->evt.sms.remoteAddress;
                //Search for prefix " symbol
                pos_ptr = OSAL_strchr(pos_ptr + 1, '\"');
                if (pos_ptr == NULL) {
                    return (PRXY_RETURN_FAILED);
                }
                OSAL_strncpy(address, pos_ptr + 1,
                        sizeof(csmEvt_ptr->evt.sms.remoteAddress));
                //Remove newline characters
                while (('\r' == address[OSAL_strlen(address) - 1]) ||
                        ('\n' == address[OSAL_strlen(address) - 1])) {
                    address[OSAL_strlen(address) - 1] = 0;
                }
                //Remove suffix " symbol
                address[OSAL_strlen(address) - 1] = '\0';
            }
            /* Clear pdu */
            csmEvt_ptr->evt.sms.pdu[0] = 0; 
            cmdMngr_ptr->sms.state = PRXY_SMS_STATE_READ_PDU;
            return (PRXY_RETURN_OK);
        default:
            break;
    }
    return (PRXY_RETURN_FAILED);
}

/**
 * ======== _PRXY_cmdMngrConstructSupsrvQueryResult ========
 * This routine will construct an AT queried response that came from CSM
 * back up through the AT interface.
 *
 * supSrv_ptr A pointer to a CSM_OutputSupSrv object.  This object contains the
 * details response to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated AT response will be
 * written. Ultimately the AT response written here is then sent up through
 * the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 */
vint _PRXY_cmdMngrConstructSupsrvQueryResult(
    const CSM_OutputSupSrv *supSrv_ptr,
    char                   *out_ptr,
    vint                    maxOutSize)
{
    vint              type;

    switch (supSrv_ptr->cmdType) {
        case CSM_SUPSRV_CMD_GET_OIP:
             OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    PRXY_AT_RESPONSE_OIP,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    PRXY_AT_RESPONSE_CR_LN,
                    PRXY_AT_RESPONSE_OK,
                    PRXY_AT_RESPONSE_CR_LN);
             break;
        case CSM_SUPSRV_CMD_GET_OIR:
             OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    PRXY_AT_RESPONSE_OIR,
                    supSrv_ptr->queryEnb.oirResStatus,
                    supSrv_ptr->prov.oirProv,
                    PRXY_AT_RESPONSE_CR_LN,
                    PRXY_AT_RESPONSE_OK,
                    PRXY_AT_RESPONSE_CR_LN);
             break;
        case CSM_SUPSRV_CMD_GET_TIP:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    PRXY_AT_RESPONSE_TIP,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    PRXY_AT_RESPONSE_CR_LN,
                    PRXY_AT_RESPONSE_OK,
                    PRXY_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_TIR:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    PRXY_AT_RESPONSE_TIR,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    PRXY_AT_RESPONSE_CR_LN,
                    PRXY_AT_RESPONSE_OK,
                    PRXY_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_CW:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%s%s%s%s",
                    PRXY_AT_RESPONSE_WAITING,
                    supSrv_ptr->queryEnb.genResStatus,
                    PRXY_AT_RESPONSE_CLASS,
                    PRXY_AT_RESPONSE_CR_LN,
                    PRXY_AT_RESPONSE_OK,
                    PRXY_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_CD:
            if (0 == supSrv_ptr->queryEnb.genResStatus) {
                OSAL_snprintf(out_ptr, maxOutSize,
                        "%s %d,%s%s%s%s",
                        PRXY_AT_RESPONSE_CF,
                        0,
                        PRXY_AT_RESPONSE_CLASS,
                        PRXY_AT_RESPONSE_CR_LN,
                        PRXY_AT_RESPONSE_OK,
                        PRXY_AT_RESPONSE_CR_LN);
            }
            else {
                if (NULL != supSrv_ptr->u.ruleParams.cfwNumber) {
                    /* default 145 when dialling string 
                     * includes international
                     * access code character "+", otherwise 129  
                     * (refer GSM 04.08)  
                     */
                    if (OSAL_strchr(supSrv_ptr->u.ruleParams.cfwNumber, '+')){
                        type = PRXY_AT_RESPONSE_CF_TYPE_145;
                    }
                    else {
                        type = PRXY_AT_RESPONSE_CF_TYPE_129;
                    }
                    if (CSM_EVENT_SUPSRV_CF_MODE_NOREPLY ==
                            supSrv_ptr->mode.cfMode) {
                        OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,%s,%s,%d,,,%d%s%s%s",
                            PRXY_AT_RESPONSE_CF,
                            1,
                            PRXY_AT_RESPONSE_CLASS,
                            supSrv_ptr->u.ruleParams.cfwNumber,
                            type,
                            supSrv_ptr->u.ruleParams.noReplyTimer,
                            PRXY_AT_RESPONSE_CR_LN,
                            PRXY_AT_RESPONSE_OK,
                            PRXY_AT_RESPONSE_CR_LN);
                    }
#if defined(PROVIDER_CMCC)
                    else if (CSM_EVENT_SUPSRV_CF_MODE_TIME ==
                            supSrv_ptr->mode.cfMode) {
                        OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,%s,%s,%d,,,\"%s\"%s%s%s",
                            PRXY_AT_RESPONSE_CF,
                            1,
                            PRXY_AT_RESPONSE_CLASS,
                            supSrv_ptr->u.ruleParams.cfwNumber,
                            type,
                            supSrv_ptr->u.ruleParams.timeRangeOfTheDay,
                            PRXY_AT_RESPONSE_CR_LN,
                            PRXY_AT_RESPONSE_OK,
                            PRXY_AT_RESPONSE_CR_LN);
                    }
#endif
                    else {
                        OSAL_snprintf(out_ptr, maxOutSize,
                                "%s %d,%s,%s,%d%s%s%s",
                                PRXY_AT_RESPONSE_CF,
                                1,
                                PRXY_AT_RESPONSE_CLASS,
                                supSrv_ptr->u.ruleParams.cfwNumber,
                                type,
                                PRXY_AT_RESPONSE_CR_LN,
                                PRXY_AT_RESPONSE_OK,
                                PRXY_AT_RESPONSE_CR_LN);
                    }
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,,,%s%s%s",
                            PRXY_AT_RESPONSE_CF,
                            1,
                            PRXY_AT_RESPONSE_CR_LN,
                            PRXY_AT_RESPONSE_OK,
                            PRXY_AT_RESPONSE_CR_LN);
                }
            }
            break;
        case CSM_SUPSRV_CMD_GET_CBOG:
        case CSM_SUPSRV_CMD_GET_CBOIC:
        case CSM_SUPSRV_CMD_GET_CBIC:
        case CSM_SUPSRV_CMD_GET_CBICR:
            OSAL_snprintf(out_ptr, maxOutSize,
                "%s %d%s%s%s",
                PRXY_AT_RESPONSE_CB,
                supSrv_ptr->queryEnb.genResStatus,
                PRXY_AT_RESPONSE_CR_LN,
                PRXY_AT_RESPONSE_OK,
                PRXY_AT_RESPONSE_CR_LN);
            break;
        default:
            break;
    }
    PRXY_dbgPrintf("\n%s %d SUPSRV AT output=%s\n", __FILE__, 
            __LINE__, out_ptr);
    return (PRXY_RETURN_OK);
}

/**
 * ======== _PRXY_cmdMngrParseSupSrvAtCommand ========
 * This routine will determine in an AT command is related to SUPSRV
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * cmdMngr_ptr A pointer to the PRXY_CommandMngr object that manages
 * AT command transactions related to SUPSRV traffic'ing.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 */
vint _PRXY_cmdMngrParseSupSrvAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    vint               x;
    const char        *pos_ptr;
    CSM_InputSupSrv   *supSrvEvt_ptr;
    char              *end_ptr;
    PRXY_SupSrvIntExt *command_ptr = NULL;
    CSM_SupSrvRuleParams *rp_ptr;

    vint size = sizeof(_PRXY_SUPSRV_AT_TO_REASON) / sizeof(PRXY_SupSrvIntExt);

    /* Get reason via AT command  */
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_SUPSRV_AT_TO_REASON[x].at_ptr,
                at_ptr, _PRXY_SUPSRV_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_SUPSRV_AT_TO_REASON[x];
            break;
        }
    }

    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }

    /* Check if it's just polling for existence. */
    pos_ptr = at_ptr + command_ptr->atLen;
    csmEvt_ptr->type = CSM_EVENT_TYPE_SUPSRV;
    /* Populate the rest of the CSM Event */
    supSrvEvt_ptr = &csmEvt_ptr->evt.supSrv;
    rp_ptr = &supSrvEvt_ptr->ruleParams;
    supSrvEvt_ptr->reason = command_ptr->reason;
    /* Set up reason */
    OSAL_strncpy(supSrvEvt_ptr->reasonDesc, command_ptr->reasonDesc_ptr,
            sizeof(supSrvEvt_ptr->reasonDesc));

    switch (command_ptr->reason) {
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_OIP:
            /* Query command */
            if (*pos_ptr == '?'){
                supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_QUERY;
            }
            else {
                pos_ptr++;
                supSrvEvt_ptr->status.genReqStatus = OSAL_atoi(pos_ptr);
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_OIR:
            /* Query command */
            if (*pos_ptr == '?'){
                supSrvEvt_ptr->status.oirReqStatus = CSM_SUPSRV_OIR_QUERY;
            }
            else {
                pos_ptr++;
                supSrvEvt_ptr->status.oirReqStatus = OSAL_atoi(pos_ptr);
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_TIP:
            if (*pos_ptr == '?'){
                supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_QUERY;
            }
            else{
                pos_ptr++;
                supSrvEvt_ptr->status.genReqStatus = OSAL_atoi(pos_ptr);
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_TIR:
            if (*pos_ptr == '?'){
                supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_QUERY;
            }
            else{
                pos_ptr++;
                supSrvEvt_ptr->status.genReqStatus = OSAL_atoi(pos_ptr);
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_CW:
            /*
             * Doesn't support TE status set and quert.
               Only support network interrogation, i.e.
               AT+CCWA=<n>,<mode>
               We don't care <n>, only process network status.
               AT+CCWA=<n>, 0 to disable CW.
               AT+CCWA=<n>, 1 to enable CW.
               AT+CCWA=<n>, 2 to query CW.
             */
            if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                /* Then we have status. */
                end_ptr++;
                if ('2' == *end_ptr) {
                    supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_QUERY;
                }
                else if ('0' == *end_ptr) {
                    supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_DISABLE;
                }
                else if ('1' == *end_ptr) {
                    supSrvEvt_ptr->status.genReqStatus = CSM_SUPSRV_ENABLE;
                }
                else {
                    return (PRXY_RETURN_FAILED);
                }
            }
            else {
                return (PRXY_RETURN_FAILED);
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_CF:
            /* Configure default value for CSM event */
            rp_ptr->cfwNumber[0] = '\0';
            rp_ptr->noReplyTimer = CSM_SUPSRV_NOREPLY_NONE;
            /*
             * +CCFC=<reason>,<mode>[,<number>[,<type>[,<class>[,<subaddr>
             *       [,<satype>[,<time>]]]]]]
#if defined(PROVIDER_CMCC)
             * extended timeRange parameter "hh:mm,hh:mm"
             * +CCFC=<reason>,<mode>[,<number>[,<type>[,<class>[,<subaddr>
             *       [,<satype>[,<time>[,<timeRange>]]]]]]]
#end
             */
            /* Look for <reason> */
            pos_ptr++;
            supSrvEvt_ptr->mode.cfMode = OSAL_atoi(pos_ptr);
            /* Look for <mode> */
            if (NULL == (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                return (PRXY_RETURN_FAILED);
            }
            pos_ptr += 2;
            supSrvEvt_ptr->status.genReqStatus = OSAL_atoi(pos_ptr);

            /* Look for <number> */
            if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                pos_ptr += 2;
                x = 0;
                while(*pos_ptr != ',' && *pos_ptr != '\r') {
                    rp_ptr->cfwNumber[x] = *pos_ptr;
                    pos_ptr++;
                    x++;
                }
                /* Look for <type> */
                if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                    pos_ptr++;
                    rp_ptr->addrType = OSAL_atoi(pos_ptr);

                    /* Look for <class> */
                    if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                        pos_ptr += (OSAL_strlen(pos_ptr) -
                                OSAL_strlen(end_ptr) + 1);
                        /* Look for <subaddr> */
                        if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                            pos_ptr += (OSAL_strlen(pos_ptr) -
                                    OSAL_strlen(end_ptr) + 1);
                            /* Look for <satype> */
                            if (NULL != (end_ptr = OSAL_strchr(pos_ptr,
                                    ','))) {
                                pos_ptr += (OSAL_strlen(pos_ptr) -
                                        OSAL_strlen(end_ptr) + 1);
                                /* Look for <time> */
                                if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ','))) {
                                    pos_ptr += (OSAL_strlen(pos_ptr) -
                                            OSAL_strlen(end_ptr) + 1);
                                    rp_ptr->noReplyTimer =
                                            OSAL_atoi(pos_ptr);
#if defined(PROVIDER_CMCC)
                                    /* look for "timeRangeStart,timeRangeEnd" */
                                    if (NULL != (end_ptr = OSAL_strchr(pos_ptr,','))) {
                                        pos_ptr += (OSAL_strlen(pos_ptr) -
                                                OSAL_strlen(end_ptr) + 1);
                                        pos_ptr += 1; /* skip " assuming "hh:mm,hh:mm" */
                                        end_ptr = OSAL_strchr(pos_ptr, '"');
                                        *end_ptr = '\0';
                                        OSAL_strcpy(rp_ptr->timeRangeOfTheDay, pos_ptr);
                                        *end_ptr = '"';
                                    }
#endif
                                }
                            }
                        }
                    }
                }
            }
            break;
        case CSM_EVENT_SUPSRV_REASON_AT_CMD_CB:
            /* skip '=' */
            pos_ptr++;
            if (OSAL_strscan(pos_ptr, PRXY_AT_CMD_CB_ALL_OUTGOING)){
                supSrvEvt_ptr->mode.cbMode = CSM_EVENT_SUPSRV_CB_MODE_BAOC;
            }
            else if (OSAL_strscan(pos_ptr,
                    PRXY_AT_CMD_CB_OUTGOING_INTERNATION)) {
                supSrvEvt_ptr->mode.cbMode = CSM_EVENT_SUPSRV_CB_MODE_BOIC;
            }
            else if (OSAL_strscan(pos_ptr,
                    PRXY_AT_CMD_CB_OUTGOING_INTERNATION_EXHC)) {
                supSrvEvt_ptr->mode.cbMode = CSM_EVENT_SUPSRV_CB_MODE_BOIC_EXHC;
            }
            else if (OSAL_strscan(pos_ptr, PRXY_AT_CMD_CB_ALL_INCOMING)) {
                supSrvEvt_ptr->mode.cbMode = CSM_EVENT_SUPSRV_CB_MODE_BAIC;
            }
            else if (OSAL_strscan(pos_ptr, PRXY_AT_CMD_CB_INCOMING_ROAMING)) {
                supSrvEvt_ptr->mode.cbMode = CSM_EVENT_SUPSRV_CB_MODE_BICR;
            }
            else {
                return (PRXY_RETURN_FAILED);
            }
            pos_ptr += 5;
            supSrvEvt_ptr->status.genReqStatus = OSAL_atoi(pos_ptr);

            break;
        default:
            return (PRXY_RETURN_FAILED);
    }
    return (PRXY_RETURN_OK);
    PRXY_dbgPrintf("%s %d: processing Supplementary Service AT command: \
            %s", __FUNCTION__, __LINE__, at_ptr);

}

/**
 * ======== _PRXY_cmdMngrParseNetworkRegAtCommand ========
 * This routine will determine in an AT command is related to 
 * Network Registration.
 * Need to pass-through AT command and process response Event.
 *
 * cmdMngr_ptr A pointer to the PRXY_CommandMngr object that manages
 * AT command transactions related to network registration.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is not related to network registration.
 * PRXY_RETURN_OK The AT command is related network registration, and need 
 * process the command result.(i.e. +CREG?)
 * PRXY_RETURN_CONTINUE The AT command is related network registration, and 
 * set it as pass-thru command.
 */
vint _PRXY_cmdMngrParseNetworkRegAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr)
{
    const char *pos_ptr;

    /* if AT command is not network registration, return failed. */
    if (NULL == OSAL_strscan(at_ptr, PRXY_AT_CMD_CREG)) {
        return (PRXY_RETURN_FAILED);
    }

    PRXY_dbgPrintf("%s %d: processing Network Registration AT command:%s\n", 
            __FUNCTION__, __LINE__, at_ptr);

    pos_ptr = at_ptr + (sizeof(PRXY_AT_CMD_CREG) - 1);
    if ('=' == *pos_ptr) {
        /* AT+CREG= , cache the enable mode. 
         * This AT command should be pass-thru.
         */
        pos_ptr++;
        if ('1' == *pos_ptr || '2' == *pos_ptr || '3' == *pos_ptr) {
            cmdMngr_ptr->networkReg.mode = OSAL_atoi(pos_ptr);
            return (PRXY_RETURN_CONTINUE);
        }
        else if ('?' == *pos_ptr) {
            return (PRXY_RETURN_CONTINUE);
        }
    }
    else if ('?' == *pos_ptr) {
        /* '+CREG?', need process its response. */
        return (PRXY_RETURN_OK);
    }
    return (PRXY_RETURN_FAILED);
}

/**
 * ======== _PRXY_cmdMngrParseNetworkRegAtCommand ========
 * This routine will determine in an AT command is related to media profile
 * definition.
 * 
 * cmdMngr_ptr A pointer to the PRXY_CommandMngr object that manages
 * AT command transactions related to network registration.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is not related to media profile definition
 * PRXY_RETURN_OK The AT command is related media profile definition and it's
 *    processed.
 */
vint _PRXY_cmdMngrParseMediaProfileAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    char       *pos_ptr;
    vint        mpIdx;
    char        atString[PRXY_AT_COMMAND_SIZE + 1];

    /* if AT command is not network registration, return failed. */
    if (NULL == OSAL_strscan(at_ptr, PRXY_AT_CMD_CDEFMP)) {
        return (PRXY_RETURN_FAILED);
    }

    PRXY_dbgPrintf("%s %d: processing media profile definition AT command:%s\n", 
            __FUNCTION__, __LINE__, at_ptr);
    csmEvt_ptr->type = CSM_EVENT_TYPE_SERVICE;
    csmEvt_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_CODERS;
    
    OSAL_strcpy(atString, at_ptr);
    /* Remove terminating symbols */
    while (('\r' == atString[OSAL_strlen(atString) - 1]) ||
            ('\n' == atString[OSAL_strlen(atString) - 1])) {
        atString[OSAL_strlen(atString) - 1] = 0;
    }

    pos_ptr = atString + (sizeof(PRXY_AT_CMD_CDEFMP) - 1);
    if ('=' == *pos_ptr) {
        /* AT+CDEFMP=' */
        pos_ptr++;
        if (',' == *pos_ptr) {
            /* No media index set, not support it currently. */
            return (PRXY_RETURN_FAILED);
        }
        else if ('?' == *pos_ptr) {
            /* 'AT+CDEFMP=?', not support it currently. */
            return (PRXY_RETURN_FAILED);
        }
        else {
            /* Get media profile index. */
            mpIdx = OSAL_atoi(pos_ptr);
            if ((1 > mpIdx) || (PRXY_MEDIA_PROFILE_SIZE < mpIdx)) {
                /*
                 * mpidx must greater than zero and less than the maximum size.
                 */
                return (PRXY_RETURN_FAILED);
            }
            /* Get media */
            pos_ptr++;
            cmdMngr_ptr->mediaProfiles[mpIdx - 1] = CSM_CALL_SESSION_TYPE_NONE;
            if (',' == *pos_ptr) {
                pos_ptr++;
                if (PRXY_RETURN_OK != _PRXY_cmdMngrParaseSDP(cmdMngr_ptr, 
                        csmEvt_ptr, mpIdx, pos_ptr)) {
                    return (PRXY_RETURN_FAILED);
                }
            }
            PRXY_dbgPrintf("Media profile set mpIdx:%d, type:0x%x, num=%d\n", 
                    mpIdx, cmdMngr_ptr->mediaProfiles[mpIdx - 1], 
                    csmEvt_ptr->evt.service.u.coder.coderNum);
            /* Generate OK response. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s%s",
                    PRXY_AT_RESPONSE_OK, PRXY_AT_RESPONSE_CR);
            return (PRXY_RETURN_OK);
        }
    }
    else if ('?' == *pos_ptr) {
        /* 'AT+CDEFMP?', not support it currently. */
        return (PRXY_RETURN_FAILED);
    }
    return (PRXY_RETURN_FAILED);
}

/**
 * ======== _PRXY_cmdMngrParseServiceAtCommand ========
 * This routine will determine in an AT command is related to service
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 */
vint _PRXY_cmdMngrParseServiceAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    vint                x;
    char                atString[PRXY_AT_COMMAND_SIZE + 1];
    const char         *pos_ptr;
    CSM_InputService   *serviceEvt_ptr;
    PRXY_ServiceIntExt *command_ptr = NULL;
    char               *auts_ptr;

    vint size = sizeof(_PRXY_SERVICE_AT_TO_REASON) / sizeof(PRXY_ServiceIntExt);

    /* Get reason via AT command  */
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_SERVICE_AT_TO_REASON[x].at_ptr,
                at_ptr, _PRXY_SERVICE_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_SERVICE_AT_TO_REASON[x];
            break;
        }
    }

    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }

    OSAL_strcpy(atString, at_ptr);
    /* Check if it's just polling for existence. */
    pos_ptr = atString + command_ptr->atLen;
    csmEvt_ptr->type = CSM_EVENT_TYPE_SERVICE;

    /* Populate the rest of the CSM Event */
    serviceEvt_ptr = &csmEvt_ptr->evt.service;
    serviceEvt_ptr->reason = command_ptr->reason;

    /* Set up reason */
    OSAL_strncpy(serviceEvt_ptr->reasonDesc, command_ptr->reasonDesc_ptr,
            sizeof(serviceEvt_ptr->reasonDesc));

    while (('\r' == atString[OSAL_strlen(atString) - 1]) ||
            ('\n' == atString[OSAL_strlen(atString) - 1])) {
        atString[OSAL_strlen(atString) - 1] = 0;
    }

    switch (command_ptr->reason) {
        case CSM_SERVICE_REASON_SET_IMPU:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.impu, pos_ptr,
                    sizeof(serviceEvt_ptr->u.impu));
            break;
        case CSM_SERVICE_REASON_SET_IMPI:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.impi, pos_ptr,
                    sizeof(serviceEvt_ptr->u.impu));
            break;
        case CSM_SERVICE_REASON_SET_DOMAIN:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.domain, pos_ptr,
                    sizeof(serviceEvt_ptr->u.domain));
            break;
        case CSM_SERVICE_REASON_SET_PCSCF:
        case CSM_SERVICE_REASON_SET_EMGCY_PCSCF:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.pcscf, pos_ptr,
                    sizeof(serviceEvt_ptr->u.pcscf));
            break;
        case CSM_SERVICE_REASON_UPDATE_CGI:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            /* check cgi type */
            switch (OSAL_atoi(pos_ptr)) {
                case PRXY_CGI_TYPE_3GPP_GERAN:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_3GPP_GERAN;
                    break;
                case PRXY_CGI_TYPE_3GPP_UTRAN_FDD:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_FDD;
                    break;
                case PRXY_CGI_TYPE_3GPP_UTRAN_TDD:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_TDD;
                    break;
                case PRXY_CGI_TYPE_3GPP_E_UTRAN_FDD:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_FDD;
                    break;
                case PRXY_CGI_TYPE_3GPP_E_UTRAN_TDD:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_TDD;
                    break;
                case PRXY_CGI_TYPE_IEEE_802_11:
                    serviceEvt_ptr->u.cgi.type =
                            CSM_SERVICE_ACCESS_TYPE_IEEE_802_11;
                    break;
                default:
                    OSAL_logMsg("%s:%d Error CGI type=%d\n", __FUNCTION__,
                            __LINE__, OSAL_atoi(pos_ptr));
                    return (PRXY_RETURN_FAILED);
                    break;
            }
            pos_ptr = OSAL_strchr(pos_ptr, ',');
            if (NULL == pos_ptr) {
                OSAL_logMsg("%s:%d Error AT format\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.cgi.id, pos_ptr + 1,
                    sizeof(serviceEvt_ptr->u.cgi.id));
            break;
        case CSM_SERVICE_REASON_IMS_ENABLE:
        case CSM_SERVICE_REASON_IMS_DISABLE:
            break;
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS:
            /* skip , symbol */
            pos_ptr++;
            if (OSAL_strlen(pos_ptr) > (CSM_AKA_RESP_STRING_SZ * 2)) {
                return (PRXY_RETURN_FAILED);
            }
            PDU_pduHexStringToBytes((char *)pos_ptr,
                    (unsigned char *)serviceEvt_ptr->u.aka.response);
            serviceEvt_ptr->u.aka.resLength = OSAL_strlen(pos_ptr) >> 1;
            break;
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
            serviceEvt_ptr->u.aka.resLength = 0;
            break;
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
            /* skip , symbol */
            pos_ptr++;
            /* search for auts */
            auts_ptr = OSAL_strchr(pos_ptr, ',');
            if (auts_ptr == NULL) {
                return (PRXY_RETURN_FAILED);
            }
            *auts_ptr = '\0';
            auts_ptr ++;
            if (OSAL_strlen(pos_ptr) > (CSM_AKA_RESP_STRING_SZ * 2) || 
                    OSAL_strlen(auts_ptr) > (CSM_AKA_AUTS_STRING_SZ * 2)) {
                return (PRXY_RETURN_FAILED);
            }
            /* Copy response */
            PDU_pduHexStringToBytes((char *)pos_ptr,
                    (unsigned char *)serviceEvt_ptr->u.aka.response);
            serviceEvt_ptr->u.aka.resLength = OSAL_strlen(pos_ptr) >> 1;
            /* Copy auts */
            PDU_pduHexStringToBytes((char *)auts_ptr,
                    (unsigned char *)serviceEvt_ptr->u.aka.auts);
            break;
        case CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.audioConfServer, pos_ptr,
                    sizeof(serviceEvt_ptr->u.audioConfServer));
            break;
        case CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER:
            if (0 == pos_ptr[0]) {
                /* Check AT command format */
                OSAL_logMsg("%s:%d Error AT format.\n", __FUNCTION__, __LINE__);
                return (PRXY_RETURN_FAILED);
            }
            OSAL_strncpy(serviceEvt_ptr->u.videoConfServer, pos_ptr,
                    sizeof(serviceEvt_ptr->u.videoConfServer));
            break;
        default:
            return (PRXY_RETURN_FAILED);
    }
    return (PRXY_RETURN_OK);
}

/*
 * ======== _PRXY_cmdMngrParseRadioAtCommand ========
 * This routine will determine in an AT command is related to radio
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * at_ptr A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 */
vint _PRXY_cmdMngrParseRadioAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)
{
    vint              x;
    char              atString[PRXY_AT_COMMAND_SIZE + 1];
    const char       *pos_ptr;
    char             *end_ptr;
    CSM_InputRadio   *radioEvt_ptr;
    PRXY_RadioIntExt *command_ptr = NULL;

    vint size = sizeof(_PRXY_RADIO_AT_TO_REASON) / sizeof(PRXY_RadioIntExt);

    /* Get reason via AT command  */
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_RADIO_AT_TO_REASON[x].at_ptr,
                at_ptr, _PRXY_RADIO_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_RADIO_AT_TO_REASON[x];
            break;
        }
    }

    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }

    OSAL_strcpy(atString, at_ptr);
    /* Check if it's just polling for existence. */
    pos_ptr = atString + command_ptr->atLen;
    csmEvt_ptr->type = CSM_EVENT_TYPE_RADIO;

    /* Populate the rest of the CSM Event */
    radioEvt_ptr = &csmEvt_ptr->evt.radio;
    radioEvt_ptr->reason = command_ptr->reason;

    while (('\r' == atString[OSAL_strlen(atString) - 1]) ||
            ('\n' == atString[OSAL_strlen(atString) - 1])) {
        atString[OSAL_strlen(atString) - 1] = 0;
    }

    switch (command_ptr->reason) {
        case CSM_RADIO_REASON_IP_CHANGE:
        case CSM_RADIO_REASON_IP_CHANGE_EMERGENCY:
            pos_ptr = at_ptr + command_ptr->atLen + 1;
            /* Get ip address string, find next quote/ */
            if (NULL == (end_ptr = OSAL_strchr(pos_ptr, '"'))) {
                PRXY_dbgPrintf("Set IP ADDR: Cannot find end quote!\n");
                return (PRXY_RETURN_FAILED);
            }
            /* Null terminate */
            *end_ptr = 0;
            radioEvt_ptr->networkType = CSM_RADIO_NETWORK_TYPE_LTE;
            OSAL_strncpy(radioEvt_ptr->address, pos_ptr,
                    sizeof(radioEvt_ptr->address));
            PRXY_dbgPrintf("Set IP ADDR reason:%d ip:%s\n", command_ptr->reason,
                    pos_ptr);
            break;
        case CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY:
            radioEvt_ptr->isEmergencyFailoverToCs =  OSAL_atoi(pos_ptr + 0);
            radioEvt_ptr->isEmergencyRegRequired =  OSAL_atoi(pos_ptr + 2);
            PRXY_dbgPrintf("Set reason:%d isFailoverToCs:%d, "
                    "isEmergencyRegRequired:%d\n", command_ptr->reason,
                    radioEvt_ptr->isEmergencyFailoverToCs,
                    radioEvt_ptr->isEmergencyRegRequired );
            break;
        default:
            return (PRXY_RETURN_FAILED);
    }
    return (PRXY_RETURN_OK);
}

/**
 * ======== _PRXY_cmdMngrConstructAtResult ========
 * This routine will construct an AT solicited response that came from CSM
 * back up through the AT interface.
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details response to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated AT response will be
 * written. Ultimately the AT response written here is then sent up through
 * the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * PRXY_RETURN_OK The AT response was constructed in out_ptr.
 * PRXY_RETURN_FAILED The details of the response indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _PRXY_cmdMngrConstructAtResult(
    PRXY_CommandMngr      *cmdMngr_ptr,
    const CSM_OutputEvent *response_ptr,
    char                  *out_ptr,
    vint                  maxOutSize)
{
    const CSM_OutputCall   *call_ptr;
    const CSM_OutputSms    *sms_ptr;
    const CSM_OutputSupSrv *supSrv_ptr;
    const CSM_CallReport   *rpt_ptr;
    const CSM_CallSummary  *sum_ptr;
    const CSM_OutputUssd   *ussd_ptr;
    const CSM_OutputService *service_ptr;
    vint                    x;
    vint                    bytes;
    vint                    bytesLeft;
    vint                    errorCode;
    char                    sdpMd[PRXY_MEDIA_DESC_STRING_SZ_MAX];
    vint                    negStatusPresent;
     
    if (CSM_EVENT_TYPE_CALL == response_ptr->type) {
        call_ptr = &response_ptr->evt.call;
        switch (call_ptr->reason) {
            case CSM_OUTPUT_REASON_OK:        
                /* '0' or 'OK' response to a command. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        PRXY_AT_RESPONSE_OK, PRXY_AT_RESPONSE_CR);
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_ERROR:     
                /* +CME ERROR response to a command. 
                 * The error code will be in the payload. */
                if (call_ptr->u.errorCode >= CSM_ERROR_CODE_SIP_START && 
                        call_ptr->u.errorCode <= CSM_ERROR_CODE_SIP_END) {
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d,\"%s\"%s",
                            PRXY_AT_RESPONSE_ERROR, PRXY_AT_RESPONSE_ERROR_SIP, 
                            call_ptr->reasonDesc, PRXY_AT_RESPONSE_CR);
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                            PRXY_AT_RESPONSE_ERROR, call_ptr->u.errorCode, 
                            PRXY_AT_RESPONSE_CR);
                }
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_CALL_LIST: 
                /* Check if it's report responds to AT+CLCC. */
                if ((!cmdMngr_ptr->extDialCmdEnabled) &&
                        (OSAL_TRUE != cmdMngr_ptr->atClccIssued)) {
                    /* Ignore unsolicited call report for +CLCC. */
                    break;
                }
                cmdMngr_ptr->atClccIssued = OSAL_FALSE;

                /* +CLCC response to a AT+CLCC command. */
                rpt_ptr = &call_ptr->u.report;
                /* Loop and set all the states of the calls. */

                /* Let's save space for the last '\r\n0\r' in the report. */
                bytesLeft = maxOutSize - 4;
                for (x = 0 ; x < rpt_ptr->numCalls ; x++) {
                    sum_ptr = &rpt_ptr->calls[x];
                    /* Generate +CLCCS */
                    if (cmdMngr_ptr->extDialCmdEnabled) {
                        negStatusPresent = 0;
                        sdpMd[0] = 0;
                        /* Populate negStatusPresent. */
                        if (0 != sum_ptr->negStatus) {
                            negStatusPresent = 1;
                            /* Populate sdpMd. */
                            if (CSM_CALL_SESSION_TYPE_AUDIO ==
                                    sum_ptr->callSessionType) {
                                /* Audio only */
                                OSAL_snprintf(sdpMd, sizeof(sdpMd), "%s",
                                        PRXY_MEDIA_PROFILE_STR_AUDIO);
                            }
                            else if ((CSM_CALL_SESSION_TYPE_AUDIO |
                                    CSM_CALL_SESSION_TYPE_VIDEO) ==
                                    sum_ptr->callSessionType) {
                                /* Audio and video */
                                OSAL_snprintf(sdpMd, sizeof(sdpMd), "%s%%xD%%xA%s",
                                        PRXY_MEDIA_PROFILE_STR_AUDIO,
                                        PRXY_MEDIA_PROFILE_STR_VIDEO);
                            }
                        }

                        bytes = OSAL_snprintf(out_ptr, bytesLeft,
                            "%s%s %d,%d,%d,%d,\"%s\",%d,%d,%d,%d,%d,\"%s\"%s",
                            PRXY_AT_RESPONSE_CR_LN,
                            PRXY_AT_RESPONSE_REPORT_EXT,
                            sum_ptr->idx,
                            sum_ptr->direction, 
                            negStatusPresent,
                            sum_ptr->negStatus,
                            sdpMd,
                            sum_ptr->mode,
                            sum_ptr->status,
                            sum_ptr->isMultiParty,
                            sum_ptr->numberType,
                            0,
                            sum_ptr->number,
                            PRXY_AT_RESPONSE_CR_LN);
                    }
                    else {
                        /* +CLCC */
                        if (0 != sum_ptr->alpha[0]) {
                            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                                "%s %d,%d,%d,%d,%d,\"%s\",%d,\"%s\"%s",
                                PRXY_AT_RESPONSE_REPORT,
                                sum_ptr->idx, sum_ptr->direction, 
                                sum_ptr->state,
                                sum_ptr->mode, sum_ptr->isMultiParty,
                                sum_ptr->number, sum_ptr->type, 
                                sum_ptr->alpha,
                                PRXY_AT_RESPONSE_CR_LN);
                        }
                        else if (0 != sum_ptr->number[0]) {
                            /* Then don't include the alpha */
                            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                                "%s %d,%d,%d,%d,%d,\"%s\",%d%s",
                                PRXY_AT_RESPONSE_REPORT,
                                sum_ptr->idx, sum_ptr->direction, 
                                sum_ptr->state,
                                sum_ptr->mode, sum_ptr->isMultiParty,
                                sum_ptr->number, sum_ptr->type,
                                PRXY_AT_RESPONSE_CR_LN);
                        }
                        else {
                            /* Then don't include a phone number or alpha */
                            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                                "%s %d,%d,%d,%d,%d%s",
                                PRXY_AT_RESPONSE_REPORT,
                                sum_ptr->idx, sum_ptr->direction, 
                                sum_ptr->state,
                                sum_ptr->mode, sum_ptr->isMultiParty,
                                PRXY_AT_RESPONSE_CR_LN);
                        }
                    }
                    if (bytes > bytesLeft) {
                        /*
                         * Then we are at the end! let's break out with
                         * a truncated report but still end the report with 
                         * an '\r\n0\r'.
                         */
                        out_ptr += bytesLeft;
                        bytesLeft = 4;
                        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s", 
                                PRXY_AT_RESPONSE_CR_LN);
                        out_ptr += bytes;
                        bytesLeft -= bytes;
                        break;
                    }
                    out_ptr += bytes;
                    bytesLeft -= bytes;
                }
                /* Don't return OK if no active call. */
                if (0 < rpt_ptr->numCalls) {
                    return (PRXY_RETURN_OK);
                }
            case CSM_OUTPUT_REASON_CALL_INDEX: 
                /*
                 * It's the resposnse of dial command. Generate AT event only
                 * when extended dial command enabled.
                 */
                if (cmdMngr_ptr->extDialCmdEnabled) {
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                            PRXY_AT_RESPONSE_CDU, call_ptr->u.callIdx,
                            PRXY_AT_RESPONSE_CR_LN);
                    return (PRXY_RETURN_OK);
                }
                break;
            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SMS == response_ptr->type) {
        sms_ptr = &response_ptr->evt.sms;
        switch (sms_ptr->reason) {
            case CSM_OUTPUT_REASON_SMS_SENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s%s%s",
                        PRXY_AT_RESPONSE_SMS_SENT, sms_ptr->mr,
                        PRXY_AT_RESPONSE_CR_LN, PRXY_AT_RESPONSE_OK, 
                        PRXY_AT_RESPONSE_CR);
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_ERROR:
                /* +CMS ERROR response to a command.
                 * The error code will be in the payload. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d, %s%s",
                        PRXY_AT_RESPONSE_SMS_ERROR, sms_ptr->u.errorCode,
                        sms_ptr->reasonDesc,
                        PRXY_AT_RESPONSE_CR);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED:
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s%s%s",
                        PRXY_AT_RESPONSE_SMS_SATAUS_REPORT, 
                        OSAL_strlen(sms_ptr->u.msg.body) / 2,
                        PRXY_AT_RESPONSE_CR_LN, sms_ptr->u.msg.body, 
                        PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_OK:
                 /* '0' or 'OK' response to a command. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        PRXY_AT_RESPONSE_OK, PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            default:
                break;
        }   
    }
    else if (CSM_EVENT_TYPE_SUPSRV == response_ptr->type) {
        supSrv_ptr = &response_ptr->evt.supSrv;
        switch (supSrv_ptr->reason) {
            case CSM_OUTPUT_REASON_OK:
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        PRXY_AT_RESPONSE_OK,
                        PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_ERROR:
                switch (supSrv_ptr->errorCode) {
                    case CSM_SUPSRV_ERROR_NO_NET_SERVICE:
                        errorCode = 30; /* No network */
                        break;
                    case CSM_SUPSRV_ERROR_NET_TIMEOUT:
                        errorCode = 31; /* Network timeout*/
                        break;
                    case CSM_SUPSRV_ERROR_HTTP:
                        PRXY_dbgPrintf("CSM_SUPSRV_ERROR_HTTP: %s\n",
                                supSrv_ptr->reasonDesc);
                    case CSM_SUPSRV_ERROR_NONE:
                    case CSM_SUPSRV_ERROR_UNKNOWN:
                    case CSM_SUPSRV_ERROR_XCAP_CONFLICT:
                    case CSM_SUPSRV_ERROR_XML_PARSING:
                    default:
                        errorCode = 100; /* Unknown */
                        break;
                }
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        PRXY_AT_RESPONSE_ERROR, errorCode,
                        PRXY_AT_RESPONSE_CR);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT:
                return (_PRXY_cmdMngrConstructSupsrvQueryResult(
                        supSrv_ptr, out_ptr, maxOutSize));
            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_USSD == response_ptr->type) {
            ussd_ptr = &response_ptr->evt.ussd;
            switch (ussd_ptr->reason) {
                case CSM_OUTPUT_REASON_ERROR:
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        PRXY_AT_RESPONSE_ERROR, ussd_ptr->errorCode,
                        PRXY_AT_RESPONSE_CR);
                    return (PRXY_RETURN_OK);
                case CSM_OUTPUT_REASON_OK:
                    OSAL_snprintf(out_ptr, maxOutSize, "%s%s", 
                        PRXY_AT_RESPONSE_STR_OK, PRXY_AT_RESPONSE_CR_LN);
                    return (PRXY_RETURN_OK);
                default:
                    return (PRXY_RETURN_FAILED);
            }
    }
    else if (CSM_EVENT_TYPE_SERVICE == response_ptr->type) {
        service_ptr = &response_ptr->evt.service;
        switch (service_ptr->reason) {
            case CSM_OUTPUT_REASON_ERROR:
                if ((CSM_ERROR_CODE_NO_NETWORK_AVAILABLE ==
                        service_ptr->errorCode) ||
                        (CSM_ERROR_CODE_XPORT_INIT_FAILURE ==
                        service_ptr->errorCode) ||
                        (service_ptr->errorCode >= CSM_ERROR_CODE_SIP_START &&
                        service_ptr->errorCode <= CSM_ERROR_CODE_SIP_END)) {
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d,\"%s\"%s",
                            PRXY_AT_RESPONSE_ERROR, PRXY_AT_RESPONSE_ERROR_SIP, 
                            service_ptr->reasonDesc, PRXY_AT_RESPONSE_CR);
                }
                return (PRXY_RETURN_OK);
            default:
                return (PRXY_RETURN_FAILED);
        }        
    }
    return (PRXY_RETURN_FAILED);
}

/*
 * ======== _PRXY_cmdMngrConstructExtDialMonitorEvent ========
 * This routine will construct an AT unsolicited monitor events that came from CSM
 * back up through the AT interface CMCCSI reporting.
 *
 * xxx should also support CMCCSN to turn on/off the reporting
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details monitor call event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * PRXY_RETURN_OK The AT event was constructed in out_ptr.
 * PRXY_RETURN_FAILED The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _PRXY_cmdMngrConstructExtDialMonitorEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall   *call_ptr;
    const CSM_CallReport   *rpt_ptr;
    const CSM_CallSummary  *sum_ptr;
    vint                    bytes;
    vint                    bytesLeft;
    vint                    x;
    char                    sdpMd[PRXY_MEDIA_DESC_STRING_SZ_MAX];
    vint                    negStatusPresent;
    
    call_ptr = &response_ptr->evt.call;
    /* +CMCCSI unsolicited report */
    rpt_ptr = &call_ptr->u.report;
    /* Loop and set all the states of the calls. */
    /* Let's save space for the last '\r\n0\r' in the report. */
    bytesLeft = maxOutSize - 4;
    for (x = 0 ; x < rpt_ptr->numCalls ; x++) {
        sum_ptr = &rpt_ptr->calls[x];
        /* Generate +CMCCSI */
        negStatusPresent = 0;
        sdpMd[0] = 0;
        /* Populate negStatusPresent. */
        if (0 != sum_ptr->negStatus) {
            negStatusPresent = 1;
            /* Populate sdpMd. */
            if (CSM_CALL_SESSION_TYPE_AUDIO ==
                    sum_ptr->callSessionType) {
                /* Audio only */
                OSAL_snprintf(sdpMd, sizeof(sdpMd), "%s",
                        PRXY_MEDIA_PROFILE_STR_AUDIO);
            }
            else if ((CSM_CALL_SESSION_TYPE_AUDIO |
                    CSM_CALL_SESSION_TYPE_VIDEO) ==
                    sum_ptr->callSessionType) {
                /* Audio and video */
                OSAL_snprintf(sdpMd, sizeof(sdpMd), "%s\\0D\\0A%s\\0D\\0A%s",
                        PRXY_MEDIA_PROFILE_STR_AUDIO,
                        PRXY_MEDIA_PROFILE_STR_VIDEO,
                        sum_ptr->coderSdpMd);
            }
        }
        /* xxx exittype, exitcause */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d,%d,\"%s\",%d,%d,%d,%d,%d,\"%s\",0,0%s",
            PRXY_AT_RESPONSE_MONITOR_EXT,
            sum_ptr->idx,
            sum_ptr->direction,
            negStatusPresent,
            sum_ptr->negStatus,
            sdpMd,
            sum_ptr->mode,
            sum_ptr->status,
            sum_ptr->isMultiParty,
            sum_ptr->numberType,
            0,
            sum_ptr->normalizedAddress,
            PRXY_AT_RESPONSE_CR_LN);

        if (bytes > bytesLeft) {
            /*
             * Then we are at the end! let's break out with
             * a truncated report but still end the report with
             * an '\r\n0\r'.
             */
            out_ptr += bytesLeft;
            bytesLeft = 4;
            bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s",
                    PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
            break;
        }
        out_ptr += bytes;
        bytesLeft -= bytes;
    }
    return (PRXY_RETURN_OK);
}

typedef enum {
    PRXY_AT_CMCCSS_CFU = 8,
    PRXY_AT_CMCCSS_CFB = 9,
    PRXY_AT_CMCCSS_CFNR = 10,
    PRXY_AT_CMCCSS_CFNRc = 11,
    PRXY_AT_CMCCSS_CFNL = 12,
    PRXY_AT_CMCCSS_CW = 14,
    PRXY_AT_CMCCSS_HOLD = 15,
    PRXY_AT_CMCCSS_CONFERENCE = 16,
} Prxy_AtExtDialSupsrvCallEventCode;

/*
 * ======== _PRXY_cmdMngrConstructExtDialSupsrvCallEvent ========
 * This routine will construct an AT unsolicited supsrv events that came from CSM
 * back up through the AT interface CMCCSSx reporting.
 *
 * xxx should also support CMCCSN to turn on/off the reporting
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * PRXY_RETURN_OK The AT event was constructed in out_ptr.
 * PRXY_RETURN_FAILED The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _PRXY_cmdMngrConstructExtDialSupsrvCallEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall     *call_ptr;
    const CSM_CallSupsrvInfo *supsrvInfo_ptr;
    vint                      bytes;
    vint                      bytesLeft;

    call_ptr = &response_ptr->evt.call;

    supsrvInfo_ptr = &call_ptr->u.supsrvInfo;
    /* Let's save space for the last '\r\n0\r' in the report. */
    bytesLeft = maxOutSize - 4;

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_BEING_FORWARDED) {
        /*
         * Event indicating that the MO call is being forwarded.
         * +CMCCSS1: 1,0,8 
         * +CMCCSSEND
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_ORIGINATED,
            PRXY_AT_CMCCSS_CFU,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_IS_WAITING) {
        /*
         * Event indicating that the MO call is being waited.
         * +CMCCSS1: 1,0,14 
         * +CMCCSSEND
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_ORIGINATED,
            PRXY_AT_CMCCSS_CW,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_IS_FORWARDED) {
        /*
         * Event indicating that the MT call is forwarded to me.
         * cause code is from history-info, e.g. CFB 486
         * +CMCCSS1: 1,1,9 
         * +CMCCSS2: 1,0,"sip:user-number-that-was-busy@lte-lguplus.co.kr" 
         * +CMCCSSEND
         */
        if (supsrvInfo_ptr->numHistories > 0) {
            const CSM_CallHistory    *hi_ptr;
            Prxy_AtExtDialSupsrvCallEventCode cfCode;

            /* care only the last retarget at this moment and get the cause code */
            hi_ptr = &supsrvInfo_ptr->callHistories[supsrvInfo_ptr->numHistories-1];
            switch (hi_ptr->cause) {
                case CSM_CALL_CAUSE_CODE_INVALID:
                case CSM_CALL_CAUSE_CODE_NONE:
                case CSM_CALL_CAUSE_CODE_UNKNOWN:
                    cfCode = PRXY_AT_CMCCSS_CFU;
                    break;
                case CSM_CALL_CAUSE_CODE_BUSY:
                    cfCode = PRXY_AT_CMCCSS_CFB;
                    break;
                case CSM_CALL_CAUSE_CODE_NOREPLY:
                    cfCode = PRXY_AT_CMCCSS_CFNR;
                    break;
                case CSM_CALL_CAUSE_CODE_UNCONDITIONAL:
                    cfCode = PRXY_AT_CMCCSS_CFU;
                    break;
                case CSM_CALL_CAUSE_CODE_NOTREACHABLE:
                    cfCode = PRXY_AT_CMCCSS_CFNRc;
                    break;
                default:
                    cfCode = PRXY_AT_CMCCSS_CFU;
                    break;
            }
            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                "%s %d,%d,%d%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT1,
                supsrvInfo_ptr->idx,
                CSM_CALL_DIR_MOBILE_TERMINATED,
                cfCode,
                PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                "%s %d,%d,\"%s\"%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT2,
                CSM_CALL_NUMBER_TYPE_URI,
                0,
                hi_ptr->number,
                PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
            bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                    PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
        } else {
            /* no history-info, so report simple CFU code only */
            bytes = OSAL_snprintf(out_ptr, bytesLeft,
                "%s %d,%d,%d%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT1,
                supsrvInfo_ptr->idx,
                CSM_CALL_DIR_MOBILE_TERMINATED,
                PRXY_AT_CMCCSS_CFU,
                PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
            bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                    PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
            out_ptr += bytes;
            bytesLeft -= bytes;
        }
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_ALIAS_URI) {
        /*
         * Event indicating that the MT call is called to alias URI.
         * +CMCCSS1: 1,1,256
         * +CMCCSS2: 1,0,"sip:uri2@domain" 
         * +CMCCSSEND
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_TERMINATED,
            PRXY_AT_RESPONSE_SUPSRV_TYPE_ALIAS_URI,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%s%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT2,
            CSM_CALL_NUMBER_TYPE_URI,
            0,
            call_ptr->reasonDesc,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification & CSM_SUPSRV_MO_CALL_VIRTUAL_RING) {
        /*
         * Event indicating that the MO call is get virtual ring
         * +CMCCSS1: 1,0,257
         * +CMCCSSEND
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_ORIGINATED,
            PRXY_AT_RESPONSE_SUPSRV_TYPE_VIRTUAL_RING,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if ((supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_MEDIA_CONVERT) ||
            (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_MEDIA_CONVERT)) {
        /*
         * Event indicating that media convert
         * +CMCCSS1: 1,0,258
         * +CMCCSSEND
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_ORIGINATED,
            PRXY_AT_RESPONSE_SUPSRV_TYPE_MEDIA_CONVERT,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_CALL_PARTICIPANT_STATUS) {
        /*
         * Event indicating that the conference participant status.
         * +CMCCSS1: 1,0,259
         * +CMCCSS2: 1,0,"+821022330215",0
         * +CMCCSSEND
         *
         * The last number of CMCCSS2 is D2's private defination which indicate
         * the participant status.
         * 0: connected
         * 1: disconnected
         * 2: on-hold
         * 3: muted-via-focus
         * 4: pending
         * 5: alerting
         * 6: dialing-in
         * 7: disconnecting
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT1,
            supsrvInfo_ptr->idx,
            CSM_CALL_DIR_MOBILE_ORIGINATED,
            PRXY_AT_RESPONSE_SUPSRV_TYPE_PARTI_STATUS,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d,%d,%s,%d%s",
            PRXY_AT_RESPONSE_SUPSRV_EXT2,
            CSM_CALL_NUMBER_TYPE_URI,
            0,
            supsrvInfo_ptr->participant.number,
            supsrvInfo_ptr->participant.status,
            PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
        bytes = OSAL_snprintf(out_ptr, bytesLeft, "%s%s",
                PRXY_AT_RESPONSE_SUPSRV_EXT_END, PRXY_AT_RESPONSE_CR_LN);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    return (PRXY_RETURN_OK);
}

typedef enum {
    PRXY_AT_CSSI_CODE_BEING_FORWARDED = 2,
    PRXY_AT_CSSI_CODE_BEING_WAITING = 3,
    PRXY_AT_CSSU_CODE_FORWARDED_CALL = 0,
    PRXY_AT_CSSU_CODE_BEING_HOLD = 2,
    PRXY_AT_CSSU_CODE_BEING_UNHOLD = 3,
    PRXY_AT_CSSU_CODE_JOIN_CONFCALL = 4,
} Prxy_AtSupsrvCallEventCode;

/*
 * ======== _PRXY_cmdMngrConstructSupsrvCallEvent ========
 * This routine will construct an AT unsolicited supsrv events that came from CSM
 * back up through the AT interface CSSI/CSSU reporting.
 *
 * xxx should also support CSSN to turn on/off the reporting
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * PRXY_RETURN_OK The AT event was constructed in out_ptr.
 * PRXY_RETURN_FAILED The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _PRXY_cmdMngrConstructSupsrvCallEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall     *call_ptr;
    const CSM_CallSupsrvInfo *supsrvInfo_ptr;
    const CSM_CallHistory    *historyInfo_ptr;
    vint                      bytes;
    vint                      bytesLeft;
    vint                      x;

    call_ptr = &response_ptr->evt.call;
    if ((CSM_EVENT_TYPE_CALL != response_ptr->type) ||
            (CSM_OUTPUT_REASON_CALL_EXTRA_INFO != call_ptr->reason)) {
        return (PRXY_RETURN_FAILED);
    }

    supsrvInfo_ptr = &call_ptr->u.supsrvInfo;
    /* Let's save space for the last '\r\n0\r' in the report. */
    bytesLeft = maxOutSize - 4;

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_BEING_FORWARDED) {
        /*
         * Event indicating that the MO call is being forwarded.
         * +CSSI: 2
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            PRXY_AT_RESPONSE_SUPSRV_MO_CALL,
            PRXY_AT_CSSI_CODE_BEING_FORWARDED,
            PRXY_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_IS_WAITING) {
        /*
         * Event indicating that the MO call is being waited.
         * +CSSI: 3
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            PRXY_AT_RESPONSE_SUPSRV_MO_CALL,
            PRXY_AT_CSSI_CODE_BEING_WAITING,
            PRXY_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_IS_FORWARDED) {
        /*
         * Event indicating that the MT call is forwarded to me.
         * +CSSU: 0
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            PRXY_AT_RESPONSE_SUPSRV_MT_CALL,
            PRXY_AT_CSSU_CODE_FORWARDED_CALL,
            PRXY_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;

        /* do our proprietary command to show the history info */
        if (0 != supsrvInfo_ptr->numHistories) {
            /*
             * Event listing the history info using new at command
             * +CSSH: hi-idx1, cause1, number1, type1[, alpha1]
             * +CSSH: hi-idx2, cause2, number2, type2[, alpha2]
             * ...
             */

            for (x = 0 ; x < supsrvInfo_ptr->numHistories ; x++) {
                historyInfo_ptr = &supsrvInfo_ptr->callHistories[x];
                if (0 != historyInfo_ptr->alpha[0]) {
                    bytes = OSAL_snprintf(out_ptr, bytesLeft,
                            "%s %s,%d,\"%s\",%d,\"%s\"%s",
                            PRXY_AT_RESPONSE_SUPSRV_HISTORY,
                            historyInfo_ptr->hiIndex,
                            historyInfo_ptr->cause,
                            historyInfo_ptr->number,
                            historyInfo_ptr->type,
                            historyInfo_ptr->alpha,
                            PRXY_AT_RESPONSE_CR);
                }
                else if (0 != historyInfo_ptr->number[0]) {
                    /* Then don't include the alpha */
                    bytes = OSAL_snprintf(out_ptr, bytesLeft,
                            "%s %s,%d,\"%s\",%d%s",
                            PRXY_AT_RESPONSE_SUPSRV_HISTORY,
                            historyInfo_ptr->hiIndex,
                            historyInfo_ptr->cause,
                            historyInfo_ptr->number,
                            historyInfo_ptr->type,
                            PRXY_AT_RESPONSE_CR);
                }
                else {
                    /*
                     * Then don't include a phone number or alpha,
                     * illegal case
                     */
                }

                out_ptr += bytes;
                bytesLeft -= bytes;
            }
        }
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_IS_HELD) {
        /*
         * Event indicating that the call is held by remote.
         * +CSSU: 2
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            PRXY_AT_RESPONSE_SUPSRV_MT_CALL,
            PRXY_AT_CSSU_CODE_BEING_HOLD,
            PRXY_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_IS_UNHELD) {
        /*
         * Event indicating that the call is unheld by remote.
         * +CSSU: 3
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            PRXY_AT_RESPONSE_SUPSRV_MT_CALL,
            PRXY_AT_CSSU_CODE_BEING_UNHOLD,
            PRXY_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }
    return (PRXY_RETURN_OK);
}

/*
 * ======== _PRXY_cmdMngrConstructAtEvent ========
 * This routine will construct an AT unsolicited events that came from CSM
 * back up through the AT interface.
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * PRXY_RETURN_OK The AT event was constructed in out_ptr.
 * PRXY_RETURN_FAILED The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _PRXY_cmdMngrConstructAtEvent(
    PRXY_CommandMngr      *cmdMngr_ptr,
    const CSM_OutputEvent *response_ptr,
    char                  *out_ptr,
    vint                   maxOutSize)
{
    const CSM_OutputCall    *call_ptr;
    const CSM_OutputSms     *sms_ptr;
    const CSM_OutputUssd    *ussd_ptr;
    const CSM_OutputService *service_ptr;
    char                     smscLenBuf[PRXY_SMSC_ADDR_LENGTH_FIELD_SZ + 1];
    unsigned char            smscLen;
    char                     randString[CSM_AKA_RAND_STRING_SZ * 2 + 1];
    char                     autnString[CSM_AKA_AUTN_STRING_SZ * 2 + 1];

    if (CSM_EVENT_TYPE_CALL == response_ptr->type) {
        call_ptr = &response_ptr->evt.call;
        switch (call_ptr->reason) {

            case CSM_OUTPUT_REASON_DISCONNECT_EVENT:
                if (cmdMngr_ptr->extDialCmdEnabled) {
                    /* Ignore this AT Event in extDial mode */
                    out_ptr[0] = '\0';
                    return (PRXY_RETURN_OK);
                }
                /* Event indicating that a remote party has disconnected. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        PRXY_AT_RESPONSE_REMOTE_DISCONNECT,
                        PRXY_AT_RESPONSE_CR);
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_INCOMING_EVENT:
                /* 
                 * Event indicating that a new incoming call is being 
                 * requested. 
                 */
                // EXample...
                //+CRING: VOICE
                //+CLIP: "13122099351",131,,,""
                if ((CSM_CALL_SESSION_TYPE_AUDIO | CSM_CALL_SESSION_TYPE_VIDEO) == 
                        call_ptr->u.clipReport.callSessionType) {
                    OSAL_snprintf(out_ptr, maxOutSize, 
                            "%s \"%s\",%d,,,\"%s\"\r\n+CNAP:\"%s\"%s",
                            PRXY_AT_RESPONSE_INCOMING_VIDEO,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            call_ptr->u.clipReport.alpha,
                            call_ptr->u.clipReport.alpha,
                            PRXY_AT_RESPONSE_CR);
                }
                else if (0 != call_ptr->u.clipReport.alpha[0]) {
                    OSAL_snprintf(out_ptr, maxOutSize, 
                            "%s \"%s\",%d,,,\"%s\"\r\n+CNAP:\"%s\"%s",
                            PRXY_AT_RESPONSE_INCOMING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            call_ptr->u.clipReport.alpha,
                            call_ptr->u.clipReport.alpha,
                            PRXY_AT_RESPONSE_CR);
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s \"%s\",%d,,,%s",
                            PRXY_AT_RESPONSE_INCOMING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            PRXY_AT_RESPONSE_CR);
                }
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_WAITING_EVENT:
                /* 
                 * Event indicating that a new incoming call waiting is 
                 * being requested.
                */
                // EXample...
                //+CCWA: "13122099351",131,,,""
                if (0 != call_ptr->u.clipReport.alpha[0]) {
                    OSAL_snprintf(out_ptr, maxOutSize, 
                            "%s \"%s\",%d,,,\"%s\"%s",
                            PRXY_AT_RESPONSE_WAITING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            call_ptr->u.clipReport.alpha,
                            PRXY_AT_RESPONSE_CR);
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize, 
                            "%s \"%s\",%d,,,%s",
                            PRXY_AT_RESPONSE_WAITING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            PRXY_AT_RESPONSE_CR);
                }
                return (PRXY_RETURN_OK);

            case CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT:
                /* We don't handle SRVCC result in GAPP. */
                PRXY_dbgPrintf("SRVCC, result:%d call_id:%d\n",
                        call_ptr->u.srvcc.result, call_ptr->u.srvcc.callId);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_CALL_DTMF_DETECT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s:%c%s",
                        PRXY_AT_RESPONSE_DTMF_DETECT,
                        call_ptr->u.digit, PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_CALL_MONITOR:
                if (!cmdMngr_ptr->extDialCmdEnabled) {
                    /* Ignore unsolicited call report for +CMCCSI. */
                    return (PRXY_RETURN_FAILED);
                    break;
                }
                _PRXY_cmdMngrConstructExtDialMonitorEvent(response_ptr, 
                        out_ptr, maxOutSize);
                return (PRXY_RETURN_OK);
                break;
            case CSM_OUTPUT_REASON_CALL_EXTRA_INFO:
                if (cmdMngr_ptr->extDialCmdEnabled) {
                    _PRXY_cmdMngrConstructExtDialSupsrvCallEvent(response_ptr, 
                        out_ptr, maxOutSize);
                } else {
                    _PRXY_cmdMngrConstructSupsrvCallEvent(response_ptr, 
                        out_ptr, maxOutSize);
                }
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY:
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                       PRXY_AT_RESPONSE_VIDEO_REQUEST_KEY,
                       PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            default:
                return (PRXY_RETURN_FAILED);
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SMS == response_ptr->type) {
        sms_ptr = &response_ptr->evt.sms;
        switch (sms_ptr->reason) {
            case CSM_OUTPUT_REASON_SMS_RECEIVED:
                /* Copy the smsc address length(2 characters) */
                OSAL_snprintf(smscLenBuf, sizeof(smscLenBuf), "%s", sms_ptr->u.msg.body);
                /* Convert to byte */
                PDU_pduHexStringToBytes(smscLenBuf, &smscLen);
                if (PRXY_SMS_PDU_MODE_TEXT == cmdMngr_ptr->sms.pduMode) {
                    /*
                     * 3GPP TS 27.005 3.4.1
                     * TEXT mode:
                     * +CMT: <oa>,[<alpha>],<scts>[,<tooa>,<fo>,<pid>,<dcs>,
                     *       <sca>,<tosca>,<length>]<CR><LF><data>
                     */
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s \"%s\",,\"%s\"%s%s%s",
                            PRXY_AT_RESPONSE_SMS_RECV, sms_ptr->address,
                            sms_ptr->u.msg.scts, PRXY_AT_RESPONSE_CR_LN,
                            sms_ptr->u.msg.body, PRXY_AT_RESPONSE_CR_LN);
                }
                else {
                    /*
                     * The length shall exclude smsc address part, A.K.A smsc
                     * address length + 1(address length field itself)
                     */
                    OSAL_snprintf(out_ptr, maxOutSize, "%s ,%d%s%s%s",
                            PRXY_AT_RESPONSE_SMS_RECV, 
                            OSAL_strlen(sms_ptr->u.msg.body) / 2 - (smscLen + 1),
                            PRXY_AT_RESPONSE_CR_LN, sms_ptr->u.msg.body, 
                            PRXY_AT_RESPONSE_CR_LN);
                }
                return (PRXY_RETURN_OK);

            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_USSD == response_ptr->type) {
        ussd_ptr = &response_ptr->evt.ussd;
        switch (ussd_ptr->reason) {
            case CSM_OUTPUT_REASON_USSD_REQUEST_EVENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s1,\"%s\",72%s%s%s",
                    PRXY_AT_RESPONSE_USSD_SENT, ussd_ptr->message,
                    PRXY_AT_RESPONSE_CR_LN, PRXY_AT_RESPONSE_STR_OK, 
                    PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s2,\"%s\",72%s%s%s",
                    PRXY_AT_RESPONSE_USSD_SENT, ussd_ptr->message,
                    PRXY_AT_RESPONSE_CR_LN, PRXY_AT_RESPONSE_STR_OK, 
                    PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SERVICE == response_ptr->type) {
        service_ptr = &response_ptr->evt.service;
        switch (service_ptr->reason) {
            case CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE:
                PDU_pduBytesToHexString(
                        (unsigned char *)service_ptr->u.aka.akaRand,
                        CSM_AKA_RAND_STRING_SZ, randString);
                PDU_pduBytesToHexString(
                        (unsigned char *)service_ptr->u.aka.akaAutn,
                        CSM_AKA_AUTN_STRING_SZ, autnString);
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s,%s%s",
                        PRXY_AT_RESPONSE_AKA_CLG, randString, autnString,
                        PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            case CSM_OUTPUT_REASON_SERVICE_NOTIFY:
                OSAL_snprintf(out_ptr, maxOutSize, "%s%d,%s%s",
                        PRXY_AT_RESPONSE_IMS_SRV_NOT,
                        service_ptr->u.notify.type,
                        service_ptr->reasonDesc,
                        PRXY_AT_RESPONSE_CR_LN);
                return (PRXY_RETURN_OK);
            default:
                break;
        }
    }
    return (PRXY_RETURN_FAILED);
}


/**
 * ======== _PRXY_cmdMngrParseUssdAtCommand ========
 * This routine will determine in an AT command is related to USSD
 * and of interest to CSM and will populate a CSM_Event to send to CSM
 * for further processing.
 *
 * cmdMngr_ptr A pointer to the PRXY_CommandMngr object that manages
 * AT command transactions related to USSD traffic.
 *
 * at_ptr: A pointer to a NULL terminated string containing an AT command.
 *
 * csmEvt_ptr: A pointer to a CSM_Event. If the AT command is of interest to
 * CSM then a CSM_Event object is populated and then later sent to CSM for
 * processing.
 *
 * Returns:
 * PRXY_RETURN_FAILED The AT command is of no interest to CSM.
 * PRXY_RETURN_OK The AT command needs to be processed by CSM.
 * The CSM_Event object pointed to by csmEvt_ptr will be
 * populated for CSM to process.
 */

vint _PRXY_cmdMngrParseUssdAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr)

{
    vint x;
    const char      *pos_ptr, *end_ptr;
    CSM_InputUssd    *ussdEvt_ptr;
    PRXY_UssdIntExt  *command_ptr = NULL;
    vint size = sizeof(_PRXY_USSD_AT_TO_REASON) / sizeof(PRXY_UssdIntExt);
    vint at_n;
    unsigned char at_str[CSM_USSD_STRING_SZ + 1];
    vint at_dcs;
    /* Other wise let's self we are interested in this command. */
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_PRXY_USSD_AT_TO_REASON[x].at_ptr, at_ptr, 
                _PRXY_USSD_AT_TO_REASON[x].atLen)) {
            command_ptr = &_PRXY_USSD_AT_TO_REASON[x];
            break;
        }
    }
    if (NULL == command_ptr) {
        /* then no match was found. This is not for D2 software to process. */
        return (PRXY_RETURN_FAILED);
    }
    /* Check if it's just polling for existence. */
    pos_ptr = at_ptr + command_ptr->atLen;
    if (*pos_ptr == '?' || NULL != OSAL_strscan(pos_ptr, "=?")) {
        return (PRXY_RETURN_FAILED);
    }
    PRXY_dbgPrintf("%s %d: processing USSD AT command:%s", __FUNCTION__, 
            __LINE__, at_ptr);

    /* skip '=' */
    pos_ptr++;
    at_str[0] = '\0';
    at_dcs = -1;
    at_n = OSAL_atoi(pos_ptr);
    if (pos_ptr != NULL && NULL != (pos_ptr = OSAL_strchr(pos_ptr, ','))) {
        /* Then we have a ussd string. */
        pos_ptr = OSAL_strchr(pos_ptr, '"');
        if (pos_ptr == NULL || OSAL_strlen(pos_ptr) == 1) {
            return (PRXY_RETURN_FAILED);
        }
        pos_ptr++;
        end_ptr = pos_ptr + (OSAL_strlen(pos_ptr) - 1);
        for(;end_ptr != pos_ptr;end_ptr--) {
            if (*end_ptr == '"')
                break;
        }
        if (end_ptr == pos_ptr) {
            return (PRXY_RETURN_FAILED);
        }
        OSAL_memCpy(at_str, pos_ptr, end_ptr - pos_ptr);
        at_str[end_ptr - pos_ptr] = '\0';
        pos_ptr = end_ptr + 1;

        if (NULL != (pos_ptr = OSAL_strchr(pos_ptr, ','))) {
            /* Get dcs */
            at_dcs = OSAL_atoi(pos_ptr+1);
        }
    }
    
    if ( at_n != 2 && OSAL_strlen((char*)at_str) == 0) {
        return PRXY_RETURN_CONTINUE;
    }
    /* 
     * Let's look for 'reasons' that require more parsing and take 
     * care of those. 
     */
    switch (command_ptr->reason) {
        case CSM_USSD_REASON_AT_CMD_SEND_USSD:
            csmEvt_ptr->type = CSM_EVENT_TYPE_USSD;
            /* Populate the rest of the CSM Event */
            ussdEvt_ptr = &csmEvt_ptr->evt.ussd;
            ussdEvt_ptr->message[0] = '\0';
            if (at_n == 2) {
                ussdEvt_ptr->reason = CSM_USSD_REASON_AT_CMD_DISCONNECT_USSD;
            }
            else {
                ussdEvt_ptr->reason = CSM_USSD_REASON_AT_CMD_SEND_USSD;
            }
            if (at_dcs == 72) {
                ussdEvt_ptr->encType= CSM_USSD_ENCTYPE_UCS2;
                OSAL_strcpy(ussdEvt_ptr->message, (char*)at_str);
            }
            else {
                ussdEvt_ptr->encType= CSM_USSD_ENCTYPE_ASCII;
                PDU_pduBytesToHexString(at_str, OSAL_strlen((char*)at_str), 
                    ussdEvt_ptr->message);
            }
            return (PRXY_RETURN_OK);
        default:
            break;
    }
    return (PRXY_RETURN_FAILED);
}
