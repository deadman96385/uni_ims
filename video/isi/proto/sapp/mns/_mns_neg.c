/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <auth_b64.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#include "../_sapp_coder_helper.h"
#include "_mns_neg.h"
#include "_mns_session_attr.h"

void _MNS_negDirection(
    tSdpAttrType in,
    tSdpAttrType *out_ptr)
{
    switch (in) {
    case eSdpAttrSendOnly:
        *out_ptr = eSdpAttrRecvOnly;
        break;
    case eSdpAttrRecvOnly:
        *out_ptr = eSdpAttrSendOnly;
        break;
    case eSdpAttrSendRecv:
        *out_ptr = eSdpAttrSendRecv;
        break;
    case eSdpAttrInactive:
    default:
        *out_ptr = in;
        break;
    }
}
/*
 * ======== _MNS_compareCoder() ========
 *
 * This function is used to find if the codes contains the audio or video code.
 * If the codes contains audio or video code return 1 else return 0
 *
 */
OSAL_Boolean _MNS_cptrCoder(char *tempPtr){
    if ((0 == OSAL_strncasecmp(_SAPP_neg_pcmu, tempPtr,
           sizeof(_SAPP_neg_pcmu)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_g729, tempPtr,
                   sizeof(_SAPP_neg_g729)))
           || (0 == OSAL_strncasecmp(_SAPP_neg_ilbc, tempPtr,
                   sizeof(_SAPP_neg_ilbc)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_silk_24k, tempPtr,
                   sizeof(_SAPP_neg_silk_24k)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_silk_16k, tempPtr,
                   sizeof(_SAPP_neg_silk_16k)))
           || (0 == OSAL_strncasecmp(_SAPP_neg_silk_8k, tempPtr,
                   sizeof(_SAPP_neg_silk_8k)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_silk, tempPtr,
                   sizeof(_SAPP_neg_silk)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_g722, tempPtr,
                   sizeof(_SAPP_neg_g722)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_g7221, tempPtr,
                   sizeof(_SAPP_neg_g7221)))
           || (0 == OSAL_strncasecmp(_SAPP_neg_amrnb, tempPtr,
                   sizeof(_SAPP_neg_amrnb)))
           ||(0 == OSAL_strncasecmp(_SAPP_neg_amrwb, tempPtr,
                                   sizeof(_SAPP_neg_amrwb)))
                           ||(0 == OSAL_strncasecmp(_SAPP_neg_pcma, tempPtr,
                                   sizeof(_SAPP_neg_pcma)))
                                   ){
        /*if the code is audio return 1*/
        return OSAL_TRUE;
    }
    else if ((0 == OSAL_strncasecmp(_SAPP_neg_h263_1998, tempPtr,
            sizeof(_SAPP_neg_h263_1998)))
            ||(0 == OSAL_strncasecmp(_SAPP_neg_h263_2000, tempPtr,
                    sizeof(_SAPP_neg_h263_2000)))
            || (0 == OSAL_strncasecmp(_SAPP_neg_h263, tempPtr,
                    sizeof(_SAPP_neg_h263)))
            ||(0 == OSAL_strncasecmp(_SAPP_neg_h264, tempPtr,
                    sizeof(_SAPP_neg_h264)))){
        /*if the code is video return 2*/
        return OSAL_TRUE;
    }
    else
        return OSAL_FALSE;
}

/*
 * ======== _MNS_negCovertModeSetToBitMask() ========
 *
 * Private functio to convert modeset string to bit mask.
 *
 * Return:
 *  MNS_ERR: Convert failed.
 *  MNS_OK: Convert successfully.
 */
static vint _MNS_negConvertModeSetToBitMask(
    char   *fmtp_ptr,
    vint   *modeSetBitMask_ptr)
{
    int     modeSet; // modeset bit mask
    char   *s_ptr;
    char    modesetStr[MAX_SESSION_MEDIA_LARGE_STR + 1];

    modeSet = 0;

    /* Find "modeset" in fmtp */
    if (NULL != OSAL_strncasescan(fmtp_ptr, OSAL_strlen(fmtp_ptr),
            _SAPP_neg_modeset)) {
        /* Found "modeset", then get the value. */
        OSAL_strcpy(modesetStr, OSAL_strchr(OSAL_strncasescan(fmtp_ptr,
                OSAL_strlen(fmtp_ptr), _SAPP_neg_modeset), '=') + 1);
        /* Find the end of mode-set if there are more parameters */
        if (NULL != OSAL_strchr(modesetStr, ';')) {
            modesetStr[OSAL_strlen(modesetStr) -
                    OSAL_strlen(OSAL_strchr(modesetStr, ';'))] = 0;
        }
        /* Put bit rate into bit mask */
        s_ptr = OSAL_strtok(modesetStr, ",");
        while (NULL != s_ptr) {
            modeSet |= (1 << OSAL_atoi(s_ptr));
            s_ptr = OSAL_strtok(NULL, ",");
        }
    }
    *modeSetBitMask_ptr = modeSet;

    return (MNS_OK);
}

/*
 * ======== _MNS_negModeSet() ========
 *
 * Private function to negotiate mode-set(i.e. bit rate) of AMR or AMR-WB.
 *
 * Returns:
 *    MNS_ERR: not match
 *    MNS_OK:  match
 */
static vint _MNS_negModeSet(
    char   *inFmtp_ptr,
    char   *outFmtp_ptr)
{
    int     inModeset, outModeset; // modeset bit mask
    char   *outModeset_ptr;
    char   *outModesetEnd_ptr;

    inModeset = 0;
    outModeset = 0;

    /* process modeset of inFmtp */
    if (MNS_ERR == _MNS_negConvertModeSetToBitMask(inFmtp_ptr, &inModeset)) {
        return (MNS_ERR);
    }

    if (MNS_ERR == _MNS_negConvertModeSetToBitMask(outFmtp_ptr, &outModeset)) {
        return (MNS_ERR);
    }

    /* If Remote doesn't set modeset, accept Local's modeset. */
    if ((0 == inModeset) && (0 != outModeset)) {
        outModeset_ptr = OSAL_strscan(outFmtp_ptr, _SAPP_neg_modeset);
        outModesetEnd_ptr = OSAL_strchr(outModeset_ptr, ';');
        inFmtp_ptr[OSAL_strlen(inFmtp_ptr)] = ';';            
        if (outModesetEnd_ptr == NULL) {
            OSAL_strcpy(inFmtp_ptr + OSAL_strlen(inFmtp_ptr), outModeset_ptr);
        }
        else {
            inFmtp_ptr[OSAL_strlen(inFmtp_ptr) + 
                    (outModesetEnd_ptr - outModeset_ptr) + 1] = '\0';
            OSAL_memCpy(inFmtp_ptr + OSAL_strlen(inFmtp_ptr), outModeset_ptr,
                    outModesetEnd_ptr - outModeset_ptr);
        }
    }
    else if ((0 != inModeset) && (0 != outModeset)) {
        if (inModeset != outModeset) {
            return (MNS_ERR);
        }
    }
    return (MNS_OK);
}

/*
 * ======== _MNS_negCoders() ========
 *
 * This function is used to determine is any of the coders defined in
 * "in_ptr" are supported by the coders defined in "out_ptr".
 *
 * This function is typically called when a call needs to negotiate a
 * list of coder types.  The coders in the "in_ptr" object will be compared to
 * all the coders in the "out_ptr".  Coders that exist in both objects are then
 * re-written to the "out_ptr" object.  Therefore the result of this function
 * is that "out_ptr" will contain a list of all coders that is mutually
 * supported.
 *
 * Returns:
 *   MNS_ERR : No coders were negotiated.  No 'matches' were found
 *              between "in_ptr" and "out_ptr".
 *   MNS_OK : The negotiation was successful.  "out_ptr" has the result of
 *             the comparison.
 */
vint _MNS_negCoders(
    tMedia         *in_ptr,
    tMedia         *out_ptr,
    MNS_NegType     negType)
{
    vint    x;
    vint    y;
    uint32  whichCoders;
    int     foundMark;
    int     tempCpr;
    char   *inName_ptr;
    char   *outName_ptr;
    char   *inFmtp_ptr;
    char   *outFmtp_ptr;
    uint32  inClockRate;
    uint32  outClockRate;
    vint    chosenCoder;

    foundMark = 0;
    whichCoders = 0;
    chosenCoder = 0;

    /*
     * Loop through all 'in' coders and mark which
     * from 'in' match the ones from 'out'
     */
    for (x = 0 ; x < in_ptr->numCoders ; x++) {
        inName_ptr = in_ptr->aCoders[x].encodingName;
        inClockRate = in_ptr->aCoders[x].clockRate;
        tempCpr = _MNS_cptrCoder(inName_ptr);
        if (foundMark == 0 || tempCpr == 0){
            /* Compare inName_ptr to all those coders in 'out_ptr' */
            for (y = 0 ; y < out_ptr->numCoders ; y++) {
                outName_ptr = out_ptr->aCoders[y].encodingName;
                outClockRate = out_ptr->aCoders[y].clockRate;
                /* The '19' is dictated by a magic number in SIP */
                if (OSAL_strncasecmp(inName_ptr, outName_ptr, 19) == 0) {
                    /* Match mode for AMR NB and AMR-WB */
                    if ((0 == OSAL_strncasecmp(_SAPP_neg_amrnb, inName_ptr,
                            sizeof(_SAPP_neg_amrnb)))
                            || (0 == OSAL_strncasecmp(_SAPP_neg_amrwb,
                            inName_ptr, sizeof(_SAPP_neg_amrwb)))) {
                        /* Check if both fmtp contain "octet-align=1" or not */
                        inFmtp_ptr = in_ptr->aCoders[x].fmtp;
                        outFmtp_ptr = out_ptr->aCoders[y].fmtp;
                        if ((NULL == OSAL_strncasescan(
                                inFmtp_ptr, sizeof(in_ptr->aCoders[x].fmtp),
                                _MNS_NEG_OCTET_ALIGN))
                                != (NULL == OSAL_strncasescan(
                                outFmtp_ptr, sizeof(out_ptr->aCoders[x].fmtp),
                                _MNS_NEG_OCTET_ALIGN))) {
                            continue;
                        }
                        if (MNS_ERR == _MNS_negModeSet(inFmtp_ptr,
                                outFmtp_ptr)) {
                            continue;
                        }
                    }
                    if (OSAL_strncasecmp(_SAPP_neg_h264, inName_ptr, sizeof(_SAPP_neg_h264)) == 0 || 
                            OSAL_strncasecmp(_SAPP_neg_h263, inName_ptr, sizeof(_SAPP_neg_h263)) == 0) {
                        //Check frame size if both side provide the vales.
                        if ( in_ptr->aCoders[x].width != 0 && out_ptr->aCoders[y].width != 0) {
                            if (in_ptr->aCoders[x].width != out_ptr->aCoders[y].width ||
                            in_ptr->aCoders[x].height != out_ptr->aCoders[y].height)
                                continue;
                        }
                    }
                            
                    /* Check if clock rate is matched */
                    if (inClockRate != outClockRate) {
                        continue;
                    }
                    if (tempCpr == 1) {
                        foundMark++;
                        /* Cache which coder will be in SDP ANSWER */
                        chosenCoder = x;
                    }
                    /* They match, remember the index from 'in' */
                    whichCoders |= (1 << x);

                    /*
                     * Set decode payload type for symmetric payload type.
                     * If negotiate offer, encode/decode payload type use offer.
                     * If negotiate answer, encode type use answer.
                     */
                    out_ptr->aCoders[y].payloadType =
                            in_ptr->aCoders[x].payloadType;
                    if (MNS_NEGTYPE_OFFER == negType) {
                        out_ptr->aCoders[y].decodePayloadType =
                                in_ptr->aCoders[x].payloadType;
                    }

                    in_ptr->aCoders[x].decodePayloadType =
                            out_ptr->aCoders[y].decodePayloadType;
                    break;
                }
            }
        }
    }

    /*
     * 'whichCoders now contains a bitmap of all indexes into the 'in'
     * coders that both parties support.  So copy the valid ones
     * from 'in' to 'out'
     */
    x = 0;
    y = 0;
    while (whichCoders) {
        if (whichCoders & 0x00000001) {
            /*
             * Get audio coder clock rate and select what telephone-event is
             * sent in SDP ANSWER.
             */
            if (0 == OSAL_strncasecmp(_SAPP_neg_tel_evt,
                    in_ptr->aCoders[x].encodingName,
                    sizeof(_SAPP_neg_tel_evt))) {
                if (in_ptr->aCoders[x].clockRate ==
                        in_ptr->aCoders[chosenCoder].clockRate) {
                    out_ptr->aCoders[y] = in_ptr->aCoders[x];
                    y++;
                }
            }
            else {
                out_ptr->aCoders[y] = in_ptr->aCoders[x];
                y++;
            }
        }
        x++;
        whichCoders >>= 1;
    }
    /* update the number of valid coder in the 'out' */
    if (y != 0) {
        out_ptr->numCoders = y;
        return (MNS_OK);
    }
    /* Then no coder was negotiated return an error */
    return (MNS_ERR);
}

/*
 * ======== _MNS_negSecurity() ========
 *
 * This function is used to determine if a security mechanism has been
 * negotiated
 *
 * Returns:
 *   MNS_ERR : Could not negotiate security, the call should be terminated
 *   MNS_OK : The negotiation was successful.  "lcl_ptr" has the result of
 *             the comparison.
 */
vint _MNS_negSecurity(
    tMedia       *rmt_ptr,
    tMedia       *lcl_ptr)
{
    vint  securityDesired;

    securityDesired = rmt_ptr->useSrtp;

    /* Check if we want security and they don't then we must refuse */
    if (lcl_ptr->useSrtp != 0) {
        if (0 == securityDesired) {
            return (MNS_ERR);
        }
    }
    lcl_ptr->useSrtp = securityDesired;

    if (0 != securityDesired) {

        if ((eSdpMediaAudio != rmt_ptr->mediaType) && 
                (eSdpMediaVideo != rmt_ptr->mediaType)) {
            /*
             * We don't understand this media type when processing srtp so
             * ignore
             */
            return (MNS_OK);
        }

        if (0 == OSAL_strncmp(rmt_ptr->cryptoSuite, SRTP_AES_80_STR,
                sizeof(rmt_ptr->cryptoSuite))) {

            OSAL_strncpy(lcl_ptr->cryptoSuite, SRTP_AES_80_STR,
                    sizeof(lcl_ptr->cryptoSuite));

        }
        else if (0 == OSAL_strncmp(rmt_ptr->cryptoSuite, SRTP_AES_32_STR,
                sizeof(rmt_ptr->cryptoSuite))) {

            OSAL_strncpy(lcl_ptr->cryptoSuite, SRTP_AES_32_STR,
                    sizeof(lcl_ptr->cryptoSuite));

        }
    }
    return (MNS_OK);
}

/*
 * ======== _MNS_negRtcpFb() ========
 *
 * This function is used to determine if a rtcp feedback messages has been
 * negotiated
 *
 * Returns:
 *   MNS_OK : The negotiation was successful.  "lcl_ptr" has the result of
 *             the comparison.
 */
vint _MNS_negRtcpFb(
    tMedia       *rmt_ptr,
    tMedia       *lcl_ptr)
{
    lcl_ptr->useAVPF = rmt_ptr->useAVPF;
    lcl_ptr->use_FB_NACK = rmt_ptr->use_FB_NACK;
    lcl_ptr->use_FB_TMMBR = rmt_ptr->use_FB_TMMBR;
    lcl_ptr->use_FB_TMMBN = rmt_ptr->use_FB_TMMBN;
    lcl_ptr->use_FB_PLI = rmt_ptr->use_FB_PLI;
    lcl_ptr->use_FB_FIR = rmt_ptr->use_FB_FIR;
    return (MNS_OK);
}

/*
 * ======== _MNS_negAnswer() ========
 * Negotiate sdp answer
 *
 * Return Values:
 *   MNS_ERR: Could not negotiate answer, the call should be terminated
 *   MNS_OK: The negotiation was successful.
 */
vint _MNS_negAnswer(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr)
{
    MNS_dbgPrintf("%s:%d Negotiate SDP answer. mns_ptr:%p\n",
            __FUNCTION__, __LINE__, mns_ptr);

    /* set session attributes, i.e. ip address   */
    if (MNS_OK != _MNS_sessionSetSessionAttr(&mns_ptr->session, rmtSess_ptr)) {
        /* set session attributes error */
        MNS_dbgPrintf("%s:%d mns_ptr:%p\n",
                __FUNCTION__, __LINE__, mns_ptr);
        return (MNS_ERR);
    }

    /* set media attributes */
    if (MNS_OK != _MNS_sessionSetMediaAttr(mns_ptr, &mns_ptr->session,
            rmtSess_ptr, MNS_NEGTYPE_ANSWER)) {
        /* no media available */
        MNS_dbgPrintf("%s:%d Set media attributes failed. mns_ptr:%p\n",
                __FUNCTION__, __LINE__, mns_ptr);
        return (MNS_ERR);
    }

    return (MNS_OK);
}

/*
 * ======== _MNS_negOffer() ========
 * * Negotiate sdp offer
 *
 * Return Values:
 *   MNS_ERR: Could not negotiate offer, the call should be terminated
 *   MNS_OK: The negotiation was successful.
 */
vint _MNS_negOffer(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr)
{
    MNS_dbgPrintf("%s:%d: Negotiate SDP offer. mns_ptr:%p\n",
            __FUNCTION__, __LINE__ , mns_ptr);

    /* set session attributes, i.e. ip address   */
    if (MNS_OK != _MNS_sessionSetSessionAttr(&mns_ptr->session, rmtSess_ptr)) {
        /* set session attributes error */
        MNS_dbgPrintf("%s:%d Set session attributes failed. mns_ptr:%p\n",
                __FUNCTION__, __LINE__, mns_ptr);
        return (MNS_ERR);
    }

    /*
     * set media attributes, includes dir, audio port, video port, strp, coder,
     * prate
     */
    if (MNS_OK != _MNS_sessionSetMediaAttr(mns_ptr, &mns_ptr->session,
            rmtSess_ptr, MNS_NEGTYPE_OFFER)) {
        /* no media available */
        MNS_dbgPrintf("%s:%d mns_ptr:%p\n",
                __FUNCTION__, __LINE__, mns_ptr);
        return (MNS_ERR);
    }

    return (MNS_OK);
}
