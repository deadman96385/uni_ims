/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */

#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_dbase_sys.h"
#include "sip_sdp_msg.h"
#include "sip_session.h"
#include "sip_mem_pool.h"

/* local prototypes */
static tAttribute* _AddFmtp      (uint8 payloadType, char* pArg);
static tAttribute* _AddExtmap (
        uint8 extmapId,
        char* extmapUri);
static void _PopulateRtpMap      (tMedia    *pMedia, tSdpMsg *pSdp);
static void _PopulateSdpMedia    (tSdpMedia *pSdpM,  tMedia *pMedia);
static void _PopulateSessionMedia(tSdpMedia *pSdpM,  tMedia *pMedia);
static tAttribute* _SearchAttrByPayloadType (
        uint8 payloadType,
        tSdpMediaType mediaType,
        tSdpMsg *pSdp,
        tSdpAttrType  attribute);


/*
 *****************************************************************************
 * ================SESSION_MakeSdp()===================
 *
 * This function builds an internal SDP message from the handle to the session
 * an internal sdp object.
 *
 * hSession  = A handle to the session object.
 *
 * RETURNS:
 *      NULL: There was an error creating an sdp object
 *
 *      tSdpMsg*:  A pointer to a sdp object.
 *
 ******************************************************************************
 */
tSdpMsg* SESSION_MakeSdp(
    tSipHandle   hSession)
{
    uint16           c;
    tSdpMsg         *pSdp;
    tAttribute      *pAttr;
    tNetworkAddress *pAddr;
    tSdpAttrType     lclDirection;
    tSessionEntry   *pSessionE;
    tSession        *pSession;
    tSdpMedia       *pSdpM;
    tSdpMedia       *pCurrM;


    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_MakeSdp: hSession:%X", (int)hSession, 0, 0);

    pSessionE = (tSessionEntry*)hSession;
    pSession = &pSessionE->sess;

    if (NULL == (pSdp = SDP_AllocMsg())) return (NULL);

    /* Set presence mask for mandatory fields */
    SDP_SET_PARM_PRESENT(pSdp, eSdpOrigin);
    SDP_SET_PARM_PRESENT(pSdp, eSdpName);
    SDP_SET_PARM_PRESENT(pSdp, eSdpTime);

    /* populate the originator */
    if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
        tSdpOrigin *pOrigin = &pAttr->value.x.origin;

        pAttr->id = eSdpPAttrOrigin;
        pAttr->value.valType = SIP_VAL_DATA;
        OSAL_strncpy(pOrigin->userName, pSessionE->szName, MAX_SDP_USERNAME);
        pOrigin->sessId = pSession->lclSessId;
        pOrigin->sessVersion = pSession->lclSessVersion;
        SDP_InsertAttr(&pSdp->pParmAttr, pAttr);
    }
    pAddr = &pSession->lclAddr;
    if (pAddr->addressType == eNwAddrLocal) {
        pSdp->connInfo.nwType = eNetworkLocal;
        pSdp->media.transpType= eTransportLocal;
    }
    else if (pAddr->addressType <= eNwAddrIPv6 &&
             pAddr->addressType > eNwAddrNonSpecified) {
        pSdp->connInfo.nwType = eNetworkIN;
    }
    else {
        SIP_DebugLog(SIP_DB_SESSION_LVL_2, "SESSION_MakeSdp: Warning no network address type specified in hSession:%X",
            (int)hSession, 0, 0);
        SDP_DeallocMsg(pSdp);
        return (NULL);
    }
    /* Fill in address & port */
    /*Port should be irrelevant here...it is a media (m=) specific attribute */
    pSdp->connInfo.nwAddress = *pAddr;

    /* set the session direction (if set, otherwise leave unpopulated indicates sendrecv) */
    if (0 != (lclDirection = pSession->lclDirection)) {
        if (lclDirection != eSdpAttrSendRecv) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = lclDirection;
                pAttr->value.valType = SIP_VAL_NONE;
                SDP_InsertAttr(&pSdp->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdp, eSdpAttr);
            }
        }
    }

    /* initialize the SDP media pointer */
    pSdpM = pCurrM = &pSdp->media;
    for (c = 0 ; c < pSession->numMedia && c < MAX_SESSION_MEDIA_STREAMS; c++) {
        if (!pCurrM) {
            /* then allocate a new one */
            pCurrM =  (tSdpMedia *)SIP_memPoolAlloc(eSIP_OBJECT_SDP_MEDIA);
            if (!pCurrM) {
                SIP_DebugLog(SIP_DB_SESSION_LVL_1, "SESSION_MakeSdp: Not enoungh memory hSession:%X",
                             (int)hSession, 0, 0);
                SDP_DeallocMsg(pSdp);
                return (NULL);
            }
            else {
                /* attach it */
                pSdpM->next = pCurrM;
                pSdpM = pCurrM;
            }
        }
        _PopulateSdpMedia(pCurrM, &pSession->media[c]);
        SDP_SET_PARM_PRESENT(pSdp, eSdpMedia);
        pCurrM = pCurrM->next;
    }

    /* Set presence mask */
    SDP_SET_PARM_PRESENT(pSdp, eSdpProtVersion);
    SDP_SET_PARM_PRESENT(pSdp, eSdpConnInfo);
    return pSdp;
}


/*
 *****************************************************************************
 * ================SESSION_Encode()===================
 *
 * This function is called when it's time to encode the LOCAL UA's SDP info
 * This function takes it's argument and ultimately
 * builds an sdp object to be used as the offer to the media session.
 *
 * hSession = A handle to a session entry object
 *
 * pSess = A pointer to a tSession object describing the media.
 *         If it's NULL, then the cached session data is encoded.
 *
 * RETURNS:
 *      NULL: There was an error creating an sdp object
 *
 *      tSdpMsg*:  A pointer to a sdp object.
 *
 ******************************************************************************
 */
tSdpMsg* SESSION_Encode(
    tSipHandle       hSession,
    tSession        *pSess)
{
    tSessionEntry   *pSessionE;
    tSession        *pSession;
    tMedia          *pMedia;
    vint             i, m;

    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_Encode: hSession:%X", (int)hSession, 0, 0);

    pSessionE = (tSessionEntry*)hSession;
    pSession = &pSessionE->sess;

    if (pSess) {
        pSession->numMedia = pSess->numMedia;
        for (m = 0 ; m < pSess->numMedia && m < MAX_SESSION_MEDIA_STREAMS ; m++) {
            pMedia = &pSess->media[m];
            pSession->media[m] = *pMedia;
            if (pMedia->mediaType != eSdpMediaMsMessage) {
                if ((0 == pSession->media[m].numCoders) &&
                        (eSdpMediaAudio == pMedia->mediaType)) {
                    /* use default only for audio media*/
                    pSession->media[m].numCoders = pSessionE->defaultNumCoders;
                    /* Copy the payload types */
                    for (i=0; i<pSession->media[m].numCoders; i++) {
                        pSession->media[m].aCoders[i].payloadType = pSessionE->defaultCoders[i];
                    }
                }
                else if (pSession->media[m].numCoders > SYSDB_MAX_NUM_CODERS) {
                     SIP_DebugLog(SIP_DB_SESSION_LVL_1, "SESSION_Encode: FAILED exceeded the max num of coders for hSession:%X hMedia:%X MAXCODERS:%d",
                            (int)hSession, (int)pMedia, SYSDB_MAX_NUM_CODERS);
                    return (NULL);
                }
            }
        } /* end of the for loop */

        pSession->lclAddr = pSess->lclAddr;
        if (pSess->lclDirection == 0)
            pSession->lclDirection = eSdpAttrSendRecv;
        else
            pSession->lclDirection = pSess->lclDirection;

        /* set up the session id number and session version number */
        if (pSession->lclSessId == 0)
            pSession->lclSessId = pSession->lclSessVersion = SIP_randInt(1, SIP_MAX_POSTIVE_INT);
        else
            pSession->lclSessVersion++;


        /* Set what the rmtDirection should be before receiving the answer */
        if (pSession->lclDirection == eSdpAttrRecvOnly)
            pSession->rmtDirection = eSdpAttrSendOnly;
        else if (pSession->lclDirection == eSdpAttrSendOnly)
            pSession->rmtDirection = eSdpAttrRecvOnly;
        else if (pSession->lclDirection == eSdpAttrInactive)
            pSession->rmtDirection = eSdpAttrInactive;
        else
            pSession->rmtDirection = eSdpAttrSendRecv;
    }

    /* now that the session object is all built up we can make a sdp msg */
    return SESSION_MakeSdp(hSession);
}

/*
 *****************************************************************************
 * ================SESSION_Decode()===================
 *
 * This function is called when SDP info has been received from a REMOTE UA
 * (SDP is received from the network)
 *
 * hSession = A handle to a session object
 *
 * pSdp = A pointer to the SDP object fromt he internal SIP object
 *
 * RETURNS:
 *      SIP_OK Always
 *
 ******************************************************************************
 */
vint SESSION_Decode(
    tSipHandle       hSession,
    tSdpMsg         *pSdp)
{
    tAttribute    *pAttr;
    tSessionEntry *pSessionE;
    tSession      *pSession;
    tSdpMedia     *pSdpM;
    vint           m;
    vint           cnt;


    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_Decode: hSession:%X", (int)hSession, 0, 0);

    pSessionE = (tSessionEntry*)hSession;
    pSession = &pSessionE->sess;

    /* look for origin information */
    if ((pAttr = SDP_FindAttr(pSdp->pParmAttr, eSdpPAttrOrigin)) != NULL) {
        /* if this is true then the Id and version should be the same */
        pSession->rmtSessId = pAttr->value.x.origin.sessId;
        pSession->rmtSessVersion = pAttr->value.x.origin.sessVersion;
        SIP_DebugLog(SIP_DB_SESSION_LVL_2, "SESSION_Decode: rmt sessionId= %d, version= %d for hSession:%X",
            (int)pSession->rmtSessId, (int)pSession->rmtSessVersion, (int)hSession);
    }

    /* set the remote address */
    if (eNwAddrNonSpecified == pSdp->connInfo.nwAddress.addressType) {
        /*
         * If there is no connInfo in SDP,
         * get address from the first connection info of sdpM.
         */
        pSdpM = &pSdp->media;
        while (pSdpM) {
            if (eNwAddrNonSpecified != pSdpM->connInfo.nwAddress.addressType) {
                pSession->rmtAddr = pSdpM->connInfo.nwAddress;
                break;
            }
            pSdpM = pSdpM->next;
        }
    }
    else {
        pSession->rmtAddr = pSdp->connInfo.nwAddress;
    }

    /* If still cannot get remote connection info, return failed. */
    if (eNwAddrNonSpecified == pSession->rmtAddr.addressType) {
        return (SIP_FAILED);
    }

    /* port is not used here */
    pSession->rmtAddr.port = 0;

    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_Decode: rmtAddr:%X rmtPort:%d",
            (int)pSession->rmtAddr.x.ip.v4.ul, (int)pSession->rmtAddr.port, 0);

    /* look for the direction */
    if ((pAttr = SDP_FindAttr(pSdp->pAttr, eSdpAttrRecvOnly)) != NULL)
        pSession->rmtDirection = eSdpAttrRecvOnly;
    else if ((pAttr = SDP_FindAttr(pSdp->pAttr, eSdpAttrSendOnly)) != NULL)
        pSession->rmtDirection = eSdpAttrSendOnly;
    else if ((pAttr = SDP_FindAttr(pSdp->pAttr, eSdpAttrInactive)) != NULL)
        pSession->rmtDirection = eSdpAttrInactive;
    else
        pSession->rmtDirection = eSdpAttrSendRecv;

    /* now get all the 'media' lines from the SDP */
    pSdpM = &pSdp->media;
    cnt = 0;
    while (pSdpM) {
        for (m = 0 ; m < MAX_SESSION_MEDIA_STREAMS ; m++) {
            /*
             * Try to match an existing media object type with the SDP one.
             * For example, try to match media with a type of "audio" to an
             * existing "audio" type that already exists.
             */
            if (pSdpM->mediaType == pSession->media[m].mediaType) {
                /* Found an existing media type */
                _PopulateSessionMedia(pSdpM, &pSession->media[m]);
                _PopulateRtpMap(&pSession->media[m], pSdp);
                cnt++;
                break;
            }
        }
        if (MAX_SESSION_MEDIA_STREAMS == m) {
            /*
             * Then we didn't find a match, so then copy the sdp media into
             * an unused spot in the array of media's inthe session object.
             */
            for (m = 0 ; m < MAX_SESSION_MEDIA_STREAMS ; m++) {
                /*
                 * Try to match an existing media object type with the SDP one.
                 * For example, try to match media with a type of "audio" to an
                 * existing "audio" type that already exists.
                 */
                if (eSdpMediaNone == pSession->media[m].mediaType) {
                    /* Found an available used spot in the array  */
                    _PopulateSessionMedia(pSdpM, &pSession->media[m]);
                    _PopulateRtpMap(&pSession->media[m], pSdp);
                    cnt++;
                    break;
                }
            }
        }
        /* Bug fix: to catch ip address in the "c=" parameter
         * that are nested inside "m=" params
         */
        if (eNwAddrIPv6 == pSession->rmtAddr.addressType) {
            if (OSAL_netIpv6IsAddrZero(pSession->rmtAddr.x.ip.v6)) {
                OSAL_netIpv6AddrCpy(pSession->rmtAddr.x.ip.v6, pSdpM->connInfo.nwAddress.x.ip.v6);
            }
        }
        else if (pSession->rmtAddr.x.ip.v4.ul == 0) {
            /* load the first IP connection info we can find then */
            pSession->rmtAddr.x.ip.v4 = pSdpM->connInfo.nwAddress.x.ip.v4;
        }
        pSdpM = pSdpM->next;
    }
    pSession->numMedia = cnt;

    /* Since we just received new SDP info then mark it as new */
    pSessionE->isNew = 1;

    return (SIP_OK);
}

/*
 *****************************************************************************
 * ================SESSION_Init()===================
 *
 * This function initializes a session object
 *
 * hSession: A handle to a session
 *
 * pName = The name of the endpoint, this gets populated in the o: param
 *         of the SDP payload. Examples are "sparrish" or "192.168.1.1"
 *
 * aCoders: An array containing the coder tpyes that zare supported
 *          The first byte in the array is the length of the array.
 *          The remaining values are the coder numbers supported by the stack
 *          NOTE: These values are specified in the RTP RFC
 *
 * pPrate = A pointer to a tPacketRate object.  These are the packet rate
 *          values that wil be used during session negociation.
 *
 * RETURNS:
 *      Nothing
 *
 ******************************************************************************
 */
void SESSION_Init(
    tSipHandle   hSession,
    char        *pName,
    char        *aCoders,
    tPacketRate *pPrate)
{
    int size;
    tSessionEntry *pSessionE;

    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_Init: hSession:%X", (int)hSession, 0, 0);

    pSessionE = (tSessionEntry*)hSession;

    if (pName != NULL) {
        SIP_DebugLog(SIP_DB_SESSION_LVL_2, "SESSION_Init: setting default name for hSession:%X", (int)hSession, 0, 0);
        OSAL_strncpy(pSessionE->szName, pName, SIP_USERNAME_ARG_STR_SIZE);
    }
    if (aCoders != NULL) {
        SIP_DebugLog(SIP_DB_SESSION_LVL_2, "SESSION_Init: setting default coders for hSession:%X", (int)hSession, 0, 0);
        size = aCoders[0];
        OSAL_memCpy(pSessionE->defaultCoders, &aCoders[1], size);
        pSessionE->defaultNumCoders = size;
    }
    if (pPrate != NULL) {
        SIP_DebugLog(SIP_DB_SESSION_LVL_2, "SESSION_Init: setting default packet rate for hSession:%X", (int)hSession, 0, 0);
        pSessionE->defaultPrate = *pPrate;
    }
}

/*
 *****************************************************************************
 * ================SESSION_Destroy()===================
 *
 * This function destroys an session object (cleans of members).
 *
 * hSession = A handle to a session
 *
 * RETURNS:
 *      Nothing
 *
 ******************************************************************************
 */
void SESSION_Destroy(tSipHandle hSession)
{
    tSessionEntry *pSessionE;

    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "SESSION_Destroy: hSession:%X", (int)hSession, 0, 0);

    pSessionE = (tSessionEntry*)hSession;
    pSessionE->szName[0] = 0;

    /* zero out the session object (yes every byte) */
    OSAL_memSet(&pSessionE->sess, 0, sizeof (tSession));
    return;
}

/*
 *****************************************************************************
 * ================SESSION_ResetIsNew()===================
 *
 * This function resets the 'isNew' field whcih indicates whether the data in
 * the session object is new.
 *
 * hSession = A handle to a session
 *
 * RETURNS:
 *      Nothing
 *
 ******************************************************************************
 */
void SESSION_ResetIsNew(tSipHandle hSession)
{
    tSessionEntry *pSessionE;
    pSessionE = (tSessionEntry*)hSession;

    pSessionE->isNew = 0;
    return;
}

static tAttribute* _SearchAttrByPayloadType(
    uint8         payloadType,
    tSdpMediaType mediaType,
    tSdpMsg      *pSdp, 
    tSdpAttrType  attribute)
{
    tAttribute *pAttr;
    tAttribute *pCurr;
    tSdpMedia  *pM;
    uint8       AttrPayloadType;

    /*
     * First try to find the attribute globally (not nested within a media line)
     */
    pCurr = pSdp->pAttr;
    while (NULL != pCurr) {
        if (NULL != (pAttr = SDP_FindAttr(pCurr, attribute))) {
            switch (attribute) {
                case eSdpAttrFmtp:
                    AttrPayloadType = pAttr->value.x.fmtp.payloadType;
                    break;
                case eSdpAttrRtpMap:
                    AttrPayloadType = pAttr->value.x.rtpMap.payloadType;
                    break;
                case eSdpAttrFramesize:
                    AttrPayloadType = pAttr->value.x.framesize.payloadType;
                    break;
                default:
                    return NULL;
            }
            if (AttrPayloadType == payloadType) {
                SIP_DebugLog(SIP_DB_SESSION_LVL_3, 
                        "_SearchAttrByPayloadType: Found an attribute", 0, 0, 0);
                /* found it */
                return (pAttr);
            }
            pCurr = pAttr->next;
        }
        else {
            /* Then there's no more rtpmaps, break out */
            break;
        }
    }

    /*
     * If we here then we didn't find any rtpmaps yet, let's search the
     * attributes in all the media lines.
     */
    pM = &pSdp->media;
    while (NULL != pM) {
        /* Only search the same media type */
        if (mediaType == pM->mediaType) {
            pCurr = pM->pAttr;
            while (NULL != pCurr) {
                if (NULL != (pAttr = SDP_FindAttr(pCurr, attribute))) {
                    switch (attribute) {
                        case eSdpAttrFmtp:
                            AttrPayloadType = pAttr->value.x.fmtp.payloadType;
                            break;
                        case eSdpAttrRtpMap:
                            AttrPayloadType = pAttr->value.x.rtpMap.payloadType;
                            break;
                        case eSdpAttrFramesize:
                            AttrPayloadType = pAttr->value.x.framesize.payloadType;
                            break;
                        default:
                            return NULL;
                    }
                    if (AttrPayloadType == payloadType) {
                        SIP_DebugLog(SIP_DB_SESSION_LVL_3, 
                                "_SearchAttrByPayloadType: Found an fmtp", 0, 0, 0);
                        /* found it */
                        return (pAttr);
                    }
                    pCurr = pAttr->next;
                }
                else {
                    /* Then there's no more rtpmaps, break out */
                    break;
                }
            }
        }
        pM = pM->next;
    }

    return (NULL);
}

/*
 * ======== _PopulateBandwidth() ========
 * Private function to populate bandwidth information.
 *
 * Return:
 *  None.
 */
static void _PopulateBandwidth(
    tMedia    *pMedia,
    tSdpMedia *pSdpM)
{
    tAttribute *pCurr;
    tSdpBw     *pBw;

    pCurr = pSdpM->pParmAttr;
    while (NULL != pCurr) {
        if (eSdpPAttrBwInfo == pCurr->id) {
            pBw = &pCurr->value.x.bwInfo;
            if (eSdpBwAS == pBw->bwModifier) {
                pMedia->bwAs = pBw->bw;
            }
            else if (eSdpBwRR == pBw->bwModifier) {
                pMedia->bwRr = pBw->bw;
            }
            else if (eSdpBwRS == pBw->bwModifier) {
                pMedia->bwRs = pBw->bw;
            }
        }
        pCurr = pCurr->next;
    }
}

static void _PopulatePrecondition(
    tMedia    *pMedia,
    tSdpMedia *pSdpM)
{
    tAttribute *pCurr;
    uint16     *pNumStatus;
    

    pNumStatus = &pMedia->precondition.numStatus;
    *pNumStatus = 0;

    pCurr = pSdpM->pAttr;
    while (NULL != pCurr) {
        if ((eSdpAttrCurr == pCurr->id) || (eSdpAttrDes == pCurr->id) || (eSdpAttrConf == pCurr->id)) {
            pMedia->precondition.status[*pNumStatus].status     = (tPrecStatus)pCurr->id;
            pMedia->precondition.status[*pNumStatus].type       = pCurr->value.x.precondition.type;
            /* pouplate strength anyway even it's not desired status */
            pMedia->precondition.status[*pNumStatus].strength   = pCurr->value.x.precondition.strength;
            pMedia->precondition.status[*pNumStatus].statusType = pCurr->value.x.precondition.statusType;
            pMedia->precondition.status[*pNumStatus].dir        = pCurr->value.x.precondition.dir;
            (*pNumStatus)++;
        }
        pCurr = pCurr->next;
        if (SIP_PREC_STATUS_SIZE_MAX <= *pNumStatus) {
            SIP_DebugLog(SIP_DB_SESSION_LVL_3, "_PopulatePrecondition: status entries exceeds maximum size", 0, 0, 0);
            return;
        } 
    }
 
}

static void _PopulateRtpMap(
    tMedia  *pMedia,
    tSdpMsg *pSdp)
{
    tAttribute *pAttr;
    tRtpAvp    *profile;
    vint        c;
    SIP_DebugLog(SIP_DB_SESSION_LVL_3, "_PopulateRtpMap: Searching for rtpmaps", 0, 0, 0);

    for (c = 0 ; c < pMedia->numCoders ; c++) {
        /* first we will see if they're are any fmtp entries for this coder type value */
        pAttr = _SearchAttrByPayloadType(pMedia->aCoders[c].payloadType, pMedia->mediaType, pSdp, eSdpAttrFmtp);
        if (pAttr) {
            if (pAttr->value.x.fmtp.szArg[0] != 0)
                OSAL_strncpy(pMedia->aCoders[c].fmtp, pAttr->value.x.fmtp.szArg,
                        MAX_SESSION_MEDIA_LARGE_STR);
        }
        /* check if this payloadType is static */
        if (pMedia->aCoders[c].payloadType >= DYNAMIC_PAYLOAD_TYPE_START) {
            SIP_DebugLog(SIP_DB_SESSION_LVL_3, "_PopulateRtpMap: looking for dynamic payload type", 0, 0, 0);
            /* then we have a dynamic coder type search for an rtpmap */
            pAttr = _SearchAttrByPayloadType(pMedia->aCoders[c].payloadType, pMedia->mediaType, pSdp, eSdpAttrRtpMap);
            if (pAttr) {
                /* found it */
                pMedia->aCoders[c].clockRate = pAttr->value.x.rtpMap.clockRate;
                if (pAttr->value.x.rtpMap.szEncodingName[0] != 0)
                    OSAL_strncpy(pMedia->aCoders[c].encodingName,
                            pAttr->value.x.rtpMap.szEncodingName, MAX_SESSION_MEDIA_STR);
            }
            pAttr = _SearchAttrByPayloadType(pMedia->aCoders[c].payloadType, pMedia->mediaType, pSdp, eSdpAttrFramesize);
            if (pAttr) {
                /* found it */
                pMedia->aCoders[c].width = pAttr->value.x.framesize.width;
                pMedia->aCoders[c].height = pAttr->value.x.framesize.height;
            }
            pMedia->aCoders[c].framerate = pMedia->framerate;
            /* else do nothing, just give the app the payload value and
             * see if they can do anything with it
             */
        }
        else {
            /* it's Static so search the payload table */
            SIP_DebugLog(SIP_DB_SESSION_LVL_3, "_PopulateRtpMap: looking for static payload type", 0, 0, 0);
            profile = SDP_staticAudioPTtoRtpAvp(pMedia->aCoders[c].payloadType);
            if (profile) {
                pMedia->aCoders[c].clockRate = profile->clockRate;
                if (profile->encodingName[0] != 0) {
                    OSAL_strncpy(pMedia->aCoders[c].encodingName,
                            profile->encodingName, MAX_SESSION_MEDIA_STR);
                }
                SIP_DebugLog(SIP_DB_SESSION_LVL_3, "_PopulateRtpMap: static payload type found", 0, 0, 0);
            }
            pAttr = _SearchAttrByPayloadType(pMedia->aCoders[c].payloadType, pMedia->mediaType, pSdp, eSdpAttrFramesize);
            if (pAttr) {
                /* found it */
                pMedia->aCoders[c].width = pAttr->value.x.framesize.width;
                pMedia->aCoders[c].height = pAttr->value.x.framesize.height;
            }
            pMedia->aCoders[c].framerate = pMedia->framerate;
        }
    }
}

static void _PopulateSdpMedia(
    tSdpMedia *pSdpM,
    tMedia    *pMedia)
{
    tSdpAttrType lclDirection;
    tRtpAvp     *profile;
    tAttribute  *pAttr;
    uint16       c;
    uint32       bandwidth;
    char         scratch1[SIP_MAX_BASETEN_NUM_STRING] = {0};
    int          availableMaxFps = 0;
    /* Fill in audio media. That's what we support */
    if (pMedia->mediaType == eSdpMediaNone) {
        /* do a default here if the developer screws up */
        pSdpM->mediaType = eSdpMediaAudio;
    }
    else {
        pSdpM->mediaType = pMedia->mediaType;
    }

    /* now set the transport type */
    if (pSdpM->mediaType == eSdpMediaMsMessage) {
        if (0 == OSAL_strncmp(pMedia->path, "msrps", OSAL_strlen("msrps"))) {
            pSdpM->transpType = eTransportMsrpTls;
        }
        else {
            pSdpM->transpType = eTransportMsrpTcp;
        }
    }
    else if (pSdpM->mediaType == eSdpMediaImage) {
        pSdpM->transpType = eTransportUdptl;
    }
    else if (pMedia->useAVPF) {
        pSdpM->transpType = (pMedia->useSrtp) ? eTransportRtpSavpf : eTransportRtpAvpf;
    }
    else {
        pSdpM->transpType = (pMedia->useSrtp) ? eTransportRtpSavp : eTransportRtpAvp;
    }

    pSdpM->port = pMedia->lclRtpPort;
    bandwidth   = 0;

    if (pSdpM->transpType != eTransportUdptl) {
        /* Set all media specific attributes */
        if (pMedia->useSrtp) {
            if (0 == OSAL_strncmp(pMedia->cryptoSuite,
                    "AES_CM_128_HMAC_SHA1_80", MAX_SESSION_MEDIA_STR)) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id                 = eSdpAttrCrypto;
                    pAttr->value.valType      = SIP_VAL_DATA;
                    pAttr->value.x.crypto.tag = 1;
                    OSAL_strncpy(pAttr->value.x.crypto.cryptoSuite,
                            "AES_CM_128_HMAC_SHA1_80", MAX_SESSION_MEDIA_STR);
                    OSAL_strncpy(pAttr->value.x.crypto.keyParamsAes80,
                            pMedia->srtpKeyParamsAes80,
                            sizeof(pAttr->value.x.crypto.keyParamsAes80));
                    SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                }
            }
            else if (0 == OSAL_strncmp(pMedia->cryptoSuite,
                    "AES_CM_128_HMAC_SHA1_32", MAX_SESSION_MEDIA_STR)) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id                 = eSdpAttrCrypto;
                    pAttr->value.valType      = SIP_VAL_DATA;
                    pAttr->value.x.crypto.tag = 1;                    
                    OSAL_strncpy(pAttr->value.x.crypto.cryptoSuite,
                            "AES_CM_128_HMAC_SHA1_32", MAX_SESSION_MEDIA_STR);
                    OSAL_strncpy(pAttr->value.x.crypto.keyParamsAes32,
                            pMedia->srtpKeyParamsAes32,
                            sizeof(pAttr->value.x.crypto.keyParamsAes32));
                    SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                }
            }
            else if (0 == OSAL_strcmp(pMedia->cryptoSuite, "")) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id                 = eSdpAttrCrypto;
                    pAttr->value.valType      = SIP_VAL_DATA;
                    pAttr->value.x.crypto.tag = 1;
                    OSAL_strncpy(pAttr->value.x.crypto.cryptoSuite,
                            "AES_CM_128_HMAC_SHA1_80", MAX_SESSION_MEDIA_STR);
                    OSAL_strncpy(pAttr->value.x.crypto.keyParamsAes80,
                            pMedia->srtpKeyParamsAes80,
                            sizeof(pAttr->value.x.crypto.keyParamsAes80));
                    SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                }
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id                 = eSdpAttrCrypto;
                    pAttr->value.valType      = SIP_VAL_DATA;
                    pAttr->value.x.crypto.tag = 2;
                    OSAL_strncpy(pAttr->value.x.crypto.cryptoSuite,
                            "AES_CM_128_HMAC_SHA1_32", MAX_SESSION_MEDIA_STR);
                    OSAL_strncpy(pAttr->value.x.crypto.keyParamsAes32,
                            pMedia->srtpKeyParamsAes32,
                            sizeof(pAttr->value.x.crypto.keyParamsAes32));
                    SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                }
            }
        }        
     
        /* Add RTP/AVPF attributes if not use avpf only.*/  
        if ((1 == pMedia->supportAVPF) && (0 == pMedia->useAVPF) &&
                (pSdpM->mediaType == eSdpMediaVideo)) {
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrTcap;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.tcap.tcapId  = 1;

                if (pMedia->supportSrtp) {
                    OSAL_strncpy(pAttr->value.x.tcap.tcapStr,
                            STR_RTCP_FB_RTP_SAVPF, MAX_RTPTYPE_NAME_LEN);
                }
                else {
                    OSAL_strncpy(pAttr->value.x.tcap.tcapStr,
                            STR_RTCP_FB_RTP_AVPF, MAX_RTPTYPE_NAME_LEN);
                }
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrPcfg;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.pcfg.pcfgId  = 1;
                OSAL_strncpy(pAttr->value.x.pcfg.pcfgStr,
                        "t=1", MAX_RTPTYPE_NAME_LEN);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        if (pMedia->supportAVPF || pMedia->useAVPF) {
            /* add rtcp-fb attributes here */
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrRtcpfb;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.rtcpfb.rtcpfbId  = 1;
                OSAL_strncpy(pAttr->value.x.rtcpfb.rtcpfbStr,
                        STR_RTCP_FB_NACK, MAX_RTPTYPE_NAME_LEN);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
            /* add rtcp-fb attributes here */
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrRtcpfb;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.rtcpfb.rtcpfbId  = 1;
                OSAL_strncpy(pAttr->value.x.rtcpfb.rtcpfbStr,
                        STR_RTCP_FB_NACK_PLI, MAX_RTPTYPE_NAME_LEN);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
            /* add rtcp-fb attributes here */
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrRtcpfb;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.rtcpfb.rtcpfbId  = 1;
                OSAL_strncpy(pAttr->value.x.rtcpfb.rtcpfbStr,
                        STR_RTCP_FB_CCM_TMMBR, MAX_RTPTYPE_NAME_LEN);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
            /* add rtcp-fb attributes here */
            if ((pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                   = eSdpAttrRtcpfb;
                pAttr->value.valType        = SIP_VAL_DATA;
                pAttr->value.x.rtcpfb.rtcpfbId  = 1;
                OSAL_strncpy(pAttr->value.x.rtcpfb.rtcpfbStr,
                        STR_RTCP_FB_CCM_FIR, MAX_RTPTYPE_NAME_LEN);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* Set remaining attributes */
        for (c = 0 ; c < pMedia->numCoders && c < SYSDB_MAX_NUM_CODERS ; c++) {
            pSdpM->formats[c] = pMedia->aCoders[c].decodePayloadType;
            if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrRtpMap;
                pAttr->value.valType = SIP_VAL_DATA;
                if (pMedia->aCoders[c].payloadType >= DYNAMIC_PAYLOAD_TYPE_START) {
                    /* then we have a dynamic coder type */
                    pAttr->value.x.rtpMap.payloadType =
                            pMedia->aCoders[c].decodePayloadType;
                    pAttr->value.x.rtpMap.clockRate = pMedia->aCoders[c].clockRate;
                    OSAL_strncpy(pAttr->value.x.rtpMap.szEncodingName,
                            pMedia->aCoders[c].encodingName, MAX_RTPTYPE_NAME_LEN);
                    if (pMedia->aCoders[c].fmtp[0] != '\0') {
                        SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                        SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                        pAttr = _AddFmtp(pMedia->aCoders[c].decodePayloadType, pMedia->aCoders[c].fmtp);
                        if (!pAttr) {
                            continue;
                        }
                    }
                    /* Add extmap attribute if present. */
                    if (pMedia->aCoders[c].extmap.uri[0] != '\0') {
                        SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                        SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                        pAttr = _AddExtmap(pMedia->aCoders[c].extmap.id, pMedia->aCoders[c].extmap.uri);
                        if (!pAttr) {
                            continue;
                        }
                    }
                }
                else {
                    /* If this is a static payload type we can provide the mapping information directly */
                    profile = SDP_staticAudioPTtoRtpAvp(pMedia->aCoders[c].payloadType);
                    if (profile) {
                        pAttr->value.x.rtpMap.payloadType = pMedia->aCoders[c].payloadType;
                        pAttr->value.x.rtpMap.clockRate = profile->clockRate;
                        pAttr->value.x.rtpMap.numChan = profile->numChan;
                        if (pMedia->aCoders[c].encodingName[0] != '\0') {
                        /* use the user supplied encoding name */
                            OSAL_strncpy(pAttr->value.x.rtpMap.szEncodingName,
                                    pMedia->aCoders[c].encodingName,
                                    MAX_RTPTYPE_NAME_LEN);
                        }
                        else {
                            /* Otherwise, use the default from the profile table */
                            OSAL_strncpy(pAttr->value.x.rtpMap.szEncodingName,
                                    profile->encodingName, MAX_RTPTYPE_NAME_LEN);
                        }
                    }
                    else {
                        SIP_DebugLog(SIP_DB_SESSION_LVL_1, "SESSION_MakeSdp: Warning -payloadType:%d not supported",
                        (int)pMedia->aCoders[c].payloadType, 0, 0);
                        SIP_memPoolFree(eSIP_OBJECT_ATTRIBUTE, (tDLListEntry *)pAttr);
                        continue;
                    }
                    if (pMedia->aCoders[c].fmtp[0] != '\0') {
                        SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                        SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                        pAttr = _AddFmtp(pMedia->aCoders[c].payloadType, pMedia->aCoders[c].fmtp);
                        if (!pAttr) {
                            continue;
                        }
                    }
                    /* Add extmap attribute if present. */
                    if (pMedia->aCoders[c].extmap.uri[0] != '\0') {
                        SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                        SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                        pAttr = _AddExtmap(pMedia->aCoders[c].extmap.id, pMedia->aCoders[c].extmap.uri);
                        if (!pAttr) {
                            continue;
                        }
                    }
                }
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                if (pMedia->aCoders[c].width != 0 && pMedia->aCoders[c].height != 0) {
                    if (NULL != (pAttr = 
                                (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                        pAttr->id = eSdpAttrFramesize;
                        pAttr->value.valType = SIP_VAL_DATA;
                        pAttr->value.x.framesize.payloadType = pMedia->aCoders[c].decodePayloadType;
                        pAttr->value.x.framesize.width = pMedia->aCoders[c].width;
                        pAttr->value.x.framesize.height = pMedia->aCoders[c].height;
                        SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                        SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                    }   
                }
                /* Find maximum frame rate through all available coders. */
                if (pMedia->aCoders[c].framerate > 0) {
                    if (pMedia->aCoders[c].framerate > availableMaxFps) {
                        availableMaxFps = pMedia->aCoders[c].framerate;
                    }
                }
                /* Set media bandwidth to the bigger one */
                if (pMedia->aCoders[c].bandwidth > bandwidth) {
                    bandwidth = pMedia->aCoders[c].bandwidth;
                }
                /*
                 * Only for Audio, Set media bandwidth to the bigger one.
                 * Video bandwidth is already set by:
                 * _SAPP_updateMediaSapp2Sip or _SAPP_encodeVideoSapp2Sip
                 */
                if (eSdpMediaAudio == pMedia->mediaType) {
                    if (pMedia->aCoders[c].bandwidth > bandwidth) {
                        bandwidth = pMedia->aCoders[c].bandwidth;
                        pMedia->lclAsBwKbps = bandwidth;
                    }
                }

            } /* end of if */
        } /* end of for loop */

        /* Only set maximum frame rate to SDP. */
        if (availableMaxFps > 0) {
            if (NULL != (pAttr =
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrFrameRate;
                pAttr->value.valType = SIP_VAL_DATA;
                pAttr->value.x.uparm = availableMaxFps;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }
        if (0 != pMedia->lclAsBwKbps) {
            pSdpM->bwInfo.bwModifier = eSdpBwAS;
            pSdpM->bwInfo.bw = pMedia->lclAsBwKbps;
            SDP_SET_PARM_PRESENT(pSdpM, eSdpBWInfo);
        }

        /* Populate bandwidth modifier only for AVP(F) or SAVP(F). */
        if ((eTransportRtpSavp == pSdpM->transpType) ||
                (eTransportRtpAvp == pSdpM->transpType) ||
                (eTransportRtpSavpf == pSdpM->transpType) ||
                (eTransportRtpAvpf == pSdpM->transpType)) {
            /* b=AS: */
            if (0 != bandwidth) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(
                        eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id = eSdpPAttrBwInfo;
                    pAttr->value.valType = SIP_VAL_DATA;
                    pAttr->value.x.bwInfo.bwModifier = eSdpBwAS;
                    pAttr->value.x.bwInfo.bw = bandwidth;
                    SDP_InsertAttr(&pSdpM->pParmAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpBWInfo);
                }
            }
            /* b=RS: */
            if (0 <= pMedia->bwRs) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(
                        eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id = eSdpPAttrBwInfo;
                    pAttr->value.valType = SIP_VAL_DATA;
                    pAttr->value.x.bwInfo.bwModifier = eSdpBwRS;
                    pAttr->value.x.bwInfo.bw = pMedia->bwRs;
                    SDP_InsertAttr(&pSdpM->pParmAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpBWInfo);
                }
            }
            /* b=RR: */
            if (0 <= pMedia->bwRr) {
                if (NULL != (pAttr = (tAttribute *)SIP_memPoolAlloc(
                        eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id = eSdpPAttrBwInfo;
                    pAttr->value.valType = SIP_VAL_DATA;
                    pAttr->value.x.bwInfo.bwModifier = eSdpBwRR;
                    pAttr->value.x.bwInfo.bw = pMedia->bwRr;
                    SDP_InsertAttr(&pSdpM->pParmAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpBWInfo);
                }
            }
        }

        pSdpM->nFormats = (uint16)c;

        /* set the media ptime (if set) */
        if (pMedia->packetRate) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrPTime;
                pAttr->value.valType = SIP_VAL_DATA;
                pAttr->value.x.uparm = pMedia->packetRate;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* set the media ptime (if set) */
        if (pMedia->maxPacketRate) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrMaxPTime;
                pAttr->value.valType = SIP_VAL_DATA;
                pAttr->value.x.uparm = pMedia->maxPacketRate;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* set any media specific direction (if set) */
        if (0 != (lclDirection = pMedia->lclDirection)) {
            if (lclDirection != eSdpAttrSendRecv) {
                if (NULL != (pAttr = 
                            (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                    pAttr->id = lclDirection;
                    pAttr->value.valType = SIP_VAL_NONE;
                    SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                    SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
                }
            }
        }

        /*
         * Set the RTCP port (if set).
         */
        if (pMedia->lclRtcpPort) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrRtcp;
                pAttr->value.valType = SIP_VAL_DATA;
                pAttr->value.x.uparm = pMedia->lclRtcpPort;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }
        if (0 != pMedia->acceptTypes[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrAcceptTypes;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->acceptTypes,
                        sizeof(pAttr->value.x.szStr));
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        if (0 != pMedia->acceptWrappedTypes[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrAcceptWrappedTypes;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->acceptWrappedTypes,
                        sizeof(pAttr->value.x.szStr));
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* fingerprint */
        if (0 != pMedia->fingerprint.fingerprint[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrFingerprint;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.fingerprint.fingerprint,
                        pMedia->fingerprint.fingerprint,
                        sizeof(pAttr->value.x.fingerprint.fingerprint));

                pAttr->value.x.fingerprint.hash = pMedia->fingerprint.hash;
                /* Then it was truncated */
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* setup */
        if (pMedia->setupRole) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrSetup;
                pAttr->value.valType = SIP_VAL_DATA;
                pAttr->value.x.setup.role = pMedia->setupRole;
                /* Then it was truncated */
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /*
         * fill in the RFC 5547 attributes for file transfer, if present
         * verify that both the name and type strings exist */
        if ((0 != pMedia->fileSelectorName[0]) &&
                (0 != pMedia->fileSelectorType[0])) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrFileSelector;
                pAttr->value.valType = SIP_VAL_DATA;

                /* format string and make sure it all fits */
                OSAL_itoa(pMedia->fileSelectorSize, scratch1,
                                    SIP_MAX_BASETEN_NUM_STRING);
                /*
                 * XXX: presently we do not generate the hash value so
                 * it is not used in the SDP
                OSAL_itoa(pMedia->fileSelectorHash, scratch2,
                                    SIP_MAX_BASETEN_NUM_STRING);
                */

                /*
                 * successfully converted length of file to a string
                 * now, format and fill */
                c = OSAL_snprintf(pAttr->value.x.szStr, MAX_SDP_ATTR_STR_LEN,
                        "name:\"%s\" type:%s size:%s",
                        pMedia->fileSelectorName, pMedia->fileSelectorType,
                        scratch1);
                /*
                 * OSAL_snprintf() will truncate as it formats the string, but
                 * returns what WOULD HAVE BEEN copied so it's necessary to
                 * (possibly) adjust this length in the tAttribute entry
                 */
                c = CALC_MIN(c, MAX_SDP_ATTR_STR_LEN);
                pAttr->value.valLength = c;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        if (0 != pMedia->fileSubject[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpPAttrInfo;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->fileSubject,
                        sizeof(pAttr->value.x.szStr));
                /* Then it was truncated */
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pParmAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpInfo);
            }
        }

        if (0 != pMedia->fileDisposition[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrFileDisposition;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->fileDisposition,
                        sizeof(pAttr->value.x.szStr));
                /* Then it was truncated */
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        if (0 != pMedia->fileTransferId[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrFileTransferId;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->fileTransferId,
                        sizeof(pAttr->value.x.szStr));
                /* Then it was truncated */
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        if (0 != pMedia->path[0]) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id = eSdpAttrPath;
                pAttr->value.valType = SIP_VAL_DATA;
                OSAL_strncpy(pAttr->value.x.szStr, pMedia->path,
                        sizeof(pAttr->value.x.szStr));
                /* Then it was truncated */
                pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }

        /* precondition */
        for (c = 0; c < pMedia->precondition.numStatus; c++) {
            if (NULL != (pAttr = 
                        (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
                pAttr->id                              = pMedia->precondition.status[c].status;
                pAttr->value.valType                   = SIP_VAL_DATA;
                pAttr->value.x.precondition.type       = pMedia->precondition.status[c].type;
                pAttr->value.x.precondition.statusType = pMedia->precondition.status[c].statusType;
                pAttr->value.x.precondition.strength   = pMedia->precondition.status[c].strength;
                pAttr->value.x.precondition.dir        = pMedia->precondition.status[c].dir;
                SDP_InsertAttr(&pSdpM->pAttr, pAttr);
                SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
            }
        }
    }
    else {

        /*
         * If UDPTL, then set up T.38 attributes.
         */
        pSdpM->nFormats = (uint16)0;

        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxVersion;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxVersion;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38MaxBitRate;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38MaxBitRate;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxFillBitRemoval;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxFillBitRemoval;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxTranscodingMMR;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxTranscodingMMR;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxTranscodingJBIG;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxTranscodingJBIG;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxRateManagement;
            pAttr->value.valType = SIP_VAL_DATA;
            OSAL_strncpy(pAttr->value.x.szStr,
                    pMedia->t38.T38FaxRateManagement, MAX_SDP_ATTR_STR_LEN);
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxMaxBuffer;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxMaxBuffer;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxMaxDatagram;
            pAttr->value.valType = SIP_VAL_DATA;
            pAttr->value.x.uparm = pMedia->t38.T38FaxMaxDatagram;
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
        if (NULL != (pAttr = 
                    (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
            pAttr->id = eSdpAttrT38FaxUdpEC;
            pAttr->value.valType = SIP_VAL_DATA;
            OSAL_strncpy(pAttr->value.x.szStr, pMedia->t38.T38FaxUdpEC,
                    MAX_SDP_ATTR_STR_LEN);
            SDP_InsertAttr(&pSdpM->pAttr, pAttr);
            SDP_SET_PARM_PRESENT(pSdpM, eSdpAttr);
        }
    }
}

static tAttribute* _AddFmtp(
    uint8 payloadType,
    char* pArg)
{
    tAttribute *pAttr;
    if (NULL != (pAttr = 
                (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
        pAttr->id = eSdpAttrFmtp;
        pAttr->value.valType = SIP_VAL_DATA;
        pAttr->value.x.fmtp.payloadType = payloadType;
        OSAL_strncpy(pAttr->value.x.fmtp.szArg, pArg,
                sizeof(pAttr->value.x.fmtp.szArg));
        return pAttr;
    }
    return NULL;
}

static tAttribute* _AddExtmap(
        uint8 extmapId,
        char* extmapUri)
{
    tAttribute *pAttr;
    if (NULL != (pAttr = 
                (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
        pAttr->id = eSdpAttrExtMap;
        pAttr->value.valType = SIP_VAL_DATA;
        pAttr->value.x.extMap.extmapId = extmapId;
        OSAL_strncpy(pAttr->value.x.extMap.extmapUri, extmapUri,
                sizeof(pAttr->value.x.extMap.extmapUri));
        return pAttr;
    }
    return NULL;
}

static vint _ParseFileSelector(
    char             *fileSelector_ptr,
    char             *fileName,
    vint              maxFileNameLen,
    char             *fileType,
    vint              maxFileTypeLen,
    vint             *fileSize)
{
    char          *c_ptr;
    OSAL_Boolean   success = OSAL_FALSE;

    static const char name[] = "name:";
    static const char type[] = "type:";
    static const char size[] = "size:";

    c_ptr = OSAL_strtok(fileSelector_ptr, " ");
    while (c_ptr != NULL) {

        if (0 == OSAL_strncmp(c_ptr, name, sizeof(name) - 1)) {
            /*
             * advance pointer past the fileSelectorName header, plus
             * another to skip the beginning quotation mark (")
             */
            OSAL_strncpy(fileName, c_ptr + sizeof(name), maxFileNameLen);
            /*
             * now remove the ending quote by null terminating one character
             * before the original end
             */
            fileName[OSAL_strlen(fileName) - 1] = 0;
            success = OSAL_TRUE;
        }
        else if (0 == OSAL_strncmp(c_ptr, type, sizeof(type) - 1)) {
            OSAL_strncpy(fileType, c_ptr + (sizeof(type) - 1), maxFileTypeLen);
            success = OSAL_TRUE;
        }
        else if (0 == OSAL_strncmp(c_ptr, size, sizeof(size) - 1)) {
            *fileSize = OSAL_atoi(c_ptr + (sizeof(size) - 1));
            success = OSAL_TRUE;
        }
        c_ptr = OSAL_strtok(NULL, " ");
    }
    if (OSAL_TRUE == success) {
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static void _PopulateSessionMedia(
    tSdpMedia *pSdpM,
    tMedia    *pMedia)
{
    uint16      nFormats;
    uint16      c;
    tAttribute *pAttr;

    nFormats = pSdpM->nFormats;
    pMedia->rmtRtpPort = pSdpM->port;
    pMedia->mediaType = pSdpM->mediaType;

    if (pSdpM->transpType != eTransportUdptl) {
        /* get Extmap attribute corresponding to this media */
        pAttr = SDP_FindAttr(pSdpM->pAttr, eSdpAttrExtMap);

        for (c = 0; c < nFormats && c < SYSDB_MAX_NUM_CODERS; c++) {
            pMedia->aCoders[c].payloadType = (uint8)pSdpM->formats[c];
            if (pAttr) {
                /* If extmap is present cache the info for all coders corresponding to this media. */
                pMedia->aCoders[c].extmap.id = pAttr->value.x.extMap.extmapId;
                if (pAttr->value.x.extMap.extmapUri[0] != 0) {
                    OSAL_strncpy(pMedia->aCoders[c].extmap.uri, pAttr->value.x.extMap.extmapUri,
                            MAX_SDP_ATTR_STR_LEN);
                }
            }
        }
        pMedia->numCoders = (uint8)c;

        /* get MAX packet rate */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrMaxPTime))) {
            pMedia->maxPacketRate = pAttr->value.x.uparm;
        }

         /* look for the direction  */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrRecvOnly))) {
            pMedia->rmtDirection = eSdpAttrRecvOnly;
            pMedia->lclDirection = eSdpAttrSendOnly;
        }
        else if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrSendOnly))) {
            pMedia->rmtDirection = eSdpAttrSendOnly;
            pMedia->lclDirection = eSdpAttrRecvOnly;
        }
        else if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrInactive))) {
            pMedia->rmtDirection = eSdpAttrInactive;
            pMedia->lclDirection = eSdpAttrInactive;
        }
        else {
            pMedia->rmtDirection = eSdpAttrSendRecv;
            pMedia->lclDirection = eSdpAttrSendRecv;
        }

        /* get packet rate */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrPTime))) {
            pMedia->packetRate = pAttr->value.x.uparm;
        }
        else {
            pMedia->packetRate = 0;
        }
        /* get RTCP port */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrRtcp))) {
            pMedia->rmtRtcpPort = pAttr->value.x.uparm;
        }
        else {
            pMedia->rmtRtcpPort = 0;
        }
        /* get framerate */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrFrameRate))) {
            pMedia->framerate = pAttr->value.x.uparm;
        }
        else {
            pMedia->framerate = 0;
        }
        
        /* get SRTP info */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrCrypto))) {
            if (0 == OSAL_strncmp(pAttr->value.x.crypto.cryptoSuite,
                    "AES_CM_128_HMAC_SHA1_80", MAX_SESSION_MEDIA_STR)) {
                if (OSAL_strlen(pAttr->value.x.crypto.keyParamsAes80) == 40) {
                    pMedia->useSrtp = 1; 
                    OSAL_strncpy(pMedia->srtpKeyParamsAes80,
                            pAttr->value.x.crypto.keyParamsAes80,
                            MAX_SESSION_SRTP_PARAMS);
                    OSAL_strncpy(pMedia->cryptoSuite,
                            pAttr->value.x.crypto.cryptoSuite, MAX_SESSION_MEDIA_STR);
                }
                else {
                    pMedia->useSrtp = 0;
                }            
            }
            else if (0 == OSAL_strncmp(pAttr->value.x.crypto.cryptoSuite,
                    "AES_CM_128_HMAC_SHA1_32", MAX_SESSION_MEDIA_STR)) {
                if (OSAL_strlen(pAttr->value.x.crypto.keyParamsAes32) == 40) {
                    pMedia->useSrtp = 1; 
                    OSAL_strncpy(pMedia->srtpKeyParamsAes32,
                            pAttr->value.x.crypto.keyParamsAes32,
                            MAX_SESSION_SRTP_PARAMS);
                    OSAL_strncpy(pMedia->cryptoSuite,
                            pAttr->value.x.crypto.cryptoSuite,
                            MAX_SESSION_MEDIA_STR);
                }           
            }
        }
        else {
            if (pSdpM->transpType == eTransportRtpSavp || 
                pSdpM->transpType == eTransportRtpSavpf) {
                pMedia->useSrtp = 1;
            }
            else {
                pMedia->useSrtp = 0;
            }
        }

        /* Get AVPF support info. */
        if ((pAttr = SDP_FindAttr(pSdpM->pAttr, eSdpAttrTcap))) { 
            pMedia->useAVPF = 0;
            if (0 == pMedia->useSrtp) {
                if (0 == OSAL_strncmp(pAttr->value.x.tcap.tcapStr,
                    STR_RTCP_FB_RTP_AVPF, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->useAVPF = 1;
                } 
            }
            else {
                if (0 == OSAL_strncmp(pAttr->value.x.tcap.tcapStr,
                    STR_RTCP_FB_RTP_SAVPF, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->useAVPF = 1;
                } 
            }
        }
        else {
            if (pSdpM->transpType == eTransportRtpAvpf || 
                pSdpM->transpType == eTransportRtpSavpf) {
                pMedia->useAVPF = 1;
            }
            else {
                pMedia->useAVPF = 0;
            }
        }

        if (pMedia->useAVPF) {
            pAttr = pSdpM->pAttr;
            pMedia->use_FB_NACK = 0;
            pMedia->use_FB_PLI = 0;
            pMedia->use_FB_TMMBR = 0;
            pMedia->use_FB_TMMBN = 0;
            pMedia->use_FB_FIR = 0;
            while (NULL != (pAttr = SDP_FindAttr(pAttr, eSdpAttrRtcpfb))) {
                if (0 == OSAL_strncmp(pAttr->value.x.rtcpfb.rtcpfbStr,
                    STR_RTCP_FB_NACK, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->use_FB_NACK = 1;
                }
                else if (0 == OSAL_strncmp(pAttr->value.x.rtcpfb.rtcpfbStr,
                    STR_RTCP_FB_NACK_PLI, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->use_FB_NACK = 1;
                    pMedia->use_FB_PLI = 1;
                }
                else if (0 == OSAL_strncmp(pAttr->value.x.rtcpfb.rtcpfbStr,
                    STR_RTCP_FB_CCM_TMMBR, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->use_FB_TMMBR = 1;
                    pMedia->use_FB_TMMBN = 1;
                }
                else if (0 == OSAL_strncmp(pAttr->value.x.rtcpfb.rtcpfbStr,
                    STR_RTCP_FB_CCM_FIR, MAX_RTPTYPE_NAME_LEN)) {
                    pMedia->use_FB_FIR = 1;
                }
                
                pAttr = pAttr->next; 
            }
        }
        pMedia->rmtAsBwKbps = pSdpM->bwInfo.bw;

        /* Find any acceptTypes and/or path info */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrPath))) {
            OSAL_strncpy(pMedia->path, 
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_LARGE_STR);
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrAcceptTypes))) {
            OSAL_strncpy(pMedia->acceptTypes, 
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_LARGE_STR);
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrAcceptWrappedTypes))) {
            OSAL_strncpy(pMedia->acceptWrappedTypes, 
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_LARGE_STR);
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrFingerprint))) {
            OSAL_strncpy(pMedia->fingerprint.fingerprint,
                    pAttr->value.x.fingerprint.fingerprint,
                    sizeof(pMedia->fingerprint.fingerprint));
            pMedia->fingerprint.hash = pAttr->value.x.fingerprint.hash;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrSetup))) {
            pMedia->setupRole = pAttr->value.x.setup.role;
        }

        /* look for the file disposition attribute */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrFileDisposition))) {
            OSAL_strncpy(pMedia->fileDisposition, 
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_LARGE_STR);
        }

        /* look for the file selector attribute */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrFileSelector))) {
            /* Parse out the individual values */
            if (SIP_OK != _ParseFileSelector(pAttr->value.x.szStr,
                    pMedia->fileSelectorName, MAX_SESSION_MEDIA_LARGE_STR,
                    pMedia->fileSelectorType, MAX_SESSION_MEDIA_LARGE_STR,
                    &pMedia->fileSelectorSize)) {
                /* Could not understand. Let's use some default values */
                OSAL_strcpy(pMedia->fileSelectorName, "Filename Details Unknown");
            }
        }

        /* look for the file transfer ID attribute */
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrFileTransferId))) {
            OSAL_strncpy(pMedia->fileTransferId, 
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_LARGE_STR);
        }

        /* populate precondition if any */
        _PopulatePrecondition(pMedia, pSdpM);

        /* Populate bandwidth modifiers */
        _PopulateBandwidth(pMedia, pSdpM);
            
    }
    else {

        /*
         * UDPTL/T38 only.
         */
        pMedia->numCoders = 0;

        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxVersion))) {
            pMedia->t38.T38FaxVersion = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxVersion = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38MaxBitRate))) {
            pMedia->t38.T38MaxBitRate = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38MaxBitRate = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxFillBitRemoval))) {
            pMedia->t38.T38FaxFillBitRemoval = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxFillBitRemoval = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxTranscodingMMR))) {
            pMedia->t38.T38FaxTranscodingMMR = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxTranscodingMMR = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxTranscodingJBIG))) {
            pMedia->t38.T38FaxTranscodingJBIG = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxTranscodingJBIG = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxRateManagement))) {
            OSAL_strncpy(pMedia->t38.T38FaxRateManagement,
                    pAttr->value.x.szStr, MAX_SESSION_MEDIA_STR);
        }
        else {
            pMedia->t38.T38FaxRateManagement[0] = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxMaxBuffer))) {
            pMedia->t38.T38FaxMaxBuffer = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxMaxBuffer = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxMaxDatagram))) {
            pMedia->t38.T38FaxMaxDatagram = pAttr->value.x.uparm;
        }
        else {
            pMedia->t38.T38FaxMaxDatagram = 0;
        }
        if (NULL != (pAttr = 
                    SDP_FindAttr(pSdpM->pAttr, eSdpAttrT38FaxUdpEC))) {
            OSAL_strncpy(pMedia->t38.T38FaxUdpEC, pAttr->value.x.szStr,
                    MAX_SESSION_MEDIA_STR);
        }
        else {
            pMedia->t38.T38FaxUdpEC[0] = 0;
        }
    }
}
