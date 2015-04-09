/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29374 $ $Date: 2014-10-16 14:57:45 +0800 (Thu, 16 Oct 2014) $
 */
#include "sip_cfg.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_sip_syntax.h"
#include "sip_voipnet.h"
#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_clib.h"
#include "sip_debug.h"
#include "sip_hdrflds.h"
#include "sip_mem_pool.h"

/* 
 *****************************************************************************
 * ================HF_IsEqualUri()===================
 *
 * This function works like an overloaded "=" operator.  It returns
 * true if two uri are equal as defined in RFC3261.
 *
 * pA = A pointer to a uri object
 *
 * pB = A pointer to a uri object
 *
 * RETURNS: 
 *         TRUE:  They are equal
 *         FALSE: These are different
 *         
 ******************************************************************************
 */
vint HF_IsEqualUri(
    tUri *pA, 
    tUri *pB)
{
    if (pA->host.addressType == pB->host.addressType) {
        if (pA->host.addressType == eNwAddrDomainName) {
            if(OSAL_strcasecmp(pA->host.x.domainName,
                    pB->host.x.domainName) != 0)
                return (FALSE);
        }
        else if (pA->host.addressType == eNwAddrIPv4) {
            if ((pA->host.x.ip.v4.ul != pB->host.x.ip.v4.ul) ||
                    (pA->maddr.v4.ul != pB->maddr.v4.ul)) {
                return (FALSE);
            }
        }
        else if (pA->host.addressType == eNwAddrIPv6) {
            if (OSAL_memCmp(pA->host.x.ip.v6, pB->host.x.ip.v6, sizeof(pA->host.x.ip.v6)) ||
                    (OSAL_memCmp(pA->maddr.v6, pB->maddr.v6, sizeof(pA->maddr.v6)))) {
                return (FALSE);
            }
        }
        /* we are looking good if we are here, check the rest */
        if (pA->host.port == pB->host.port &&
            pA->scheme == pB->scheme &&
           (!OSAL_strcmp(pA->user, pB->user)) &&
            pA->transport == pB->transport &&
            pA->lr == pB->lr &&
            pA->ttl == pB->ttl &&
            pA->method == pB->method &&
            pA->argType == pB->argType &&
            (!OSAL_strcmp(pA->szFtag, pB->szFtag)) &&
            (!OSAL_strcmp(pA->arg.szPsbr, pB->arg.szPsbr)) &&
            (!OSAL_strcmp(pA->szConf, pB->szConf)) &&
            (!OSAL_strcmp(pA->user, pB->user)) &&
            (!OSAL_strcmp(pA->szPhoneCxtParam, pB->szPhoneCxtParam))) {
                return (TRUE);
            }
    }
    return (FALSE);
}

/* 
 *****************************************************************************
 * ================HF_IsEqualUriPlus()===================
 *
 * This function works like an overloaded "=" operator.  It returns
 * true if two tUriPlus objects are equal.
 *
 * pA = A pointer to a tUriPlus object
 *
 * pB = A pointer to a tUriPlus object
 *
 * RETURNS: 
 *         TRUE:  They are equal
 *         FALSE: These are different
 *         
 ******************************************************************************
 */
vint HF_IsEqualUriPlus(
    tUriPlus *pA, 
    tUriPlus *pB)
{
    if (OSAL_strcmp(pA->szTag, pB->szTag) == 0 &&
            HF_IsEqualUri(&pA->uri, &pB->uri) &&
            OSAL_strcasecmp(pA->szDisplayName, pB->szDisplayName) == 0)
    {
        return (TRUE);
    }

    return (FALSE);
}

/* 
 *****************************************************************************
 * ================HF_IsEqualVia()===================
 *
 * This function works like an overloaded "=" operator.  It returns
 * true if two tViaHFE objects are equal.
 *
 * pA = A pointer to a tViaHFE object
 *
 * pB = A pointer to a tViaHFE object
 *
 * RETURNS: 
 *         TRUE:  They are equal
 *         FALSE: These are different
 *         
 ******************************************************************************
 */
vint HF_IsEqualVia(
    tViaHFE *pA, 
    tViaHFE *pB)
{
    if (HF_IsEqualUri(&pA->uri, &pB->uri) &&
        OSAL_strcmp(pA->szBranch, pB->szBranch) == 0 &&
        !(OSAL_strcasecmp(pA->szVersion, pB->szVersion)) &&
        pA->uri.transport == pB->uri.transport )
    {
        return TRUE;
    }
    return FALSE;
}

/* 
 *****************************************************************************
 * ================HF_PresenceExists()===================
 *
 * This function checks if a header field presence bit is set.
 * Prsence bits are used to indictate which header fields are present
 * in a SIP an internal message.
 *
 * pBitMap = A bitmap of all the header fields
 *
 * hdrFldEnum = The header field value that we are checking for presence on.
 *
 * RETURNS: 
 *        TRUE:  The indictated header field's bit is set to '1' in the bipmap
 *        FALSE: The indictated header field's bit is set to '0' in the bipmap
 *         
 ******************************************************************************
 */
vint HF_PresenceExists(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld)
{
    uint32 mask;
    uint32 presence;
    
    SIP_DebugLog(SIP_DB_GENERAL_LVL_3,
            "Checking Presence High:%x, Low:%d HF:%d", 
            pBitMap->one, pBitMap->two, hdrFld);
    
    if (hdrFld < eSIP_HF_THIRTY_TWO)
    {
        mask = pBitMap->one;
        presence = (0x01 << hdrFld);
    }
    else
    {
        mask = pBitMap->two;
        presence = (0x01 << (hdrFld - eSIP_HF_THIRTY_TWO));
    }
    if (mask & presence)
        return TRUE;
    else
        return FALSE;
}

/* 
 *****************************************************************************
 * ================HF_SetPresence()===================
 *
 * This function sets a bit in a pBitMap bitmap indictating the header field 
 * specified in hdrFldEnum is present.
 *
 * pBitMap = A bitmap of all the header fields.
 *
 * hdrFldEnum = The header field value that we are setting.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_SetPresence(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld)
{
    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Setting Presence High:%x, Low:%d HF:%d", 
                pBitMap->one, pBitMap->two, hdrFld);
    
    if (hdrFld < eSIP_HF_THIRTY_TWO)
        pBitMap->one |= (0x01 << hdrFld);
    else
        pBitMap->two |= (0x01 << (hdrFld - eSIP_HF_THIRTY_TWO));
    return;
}

/* 
 *****************************************************************************
 * ================HF_ClrPresence()===================
 *
 * This function clears a bit in a pBitMap bitmap indictating the header field 
 * specified in hdrFldEnum has presence.
 *
 * pBitMap = A bitmap of all the header fields.
 *
 * hdrFldEnum = The header field value that we are setting.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_ClrPresence(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld)
{
    uint32 *puint32;
    uint32 presence;
    
    SIP_DebugLog(SIP_DB_GENERAL_LVL_3,
            "Clearing Presence High:%x, Low:%d HF:%d", 
            pBitMap->one, pBitMap->two, hdrFld);
    
    if (hdrFld < eSIP_HF_THIRTY_TWO) {
        puint32 = &((pBitMap)->one);
        presence = (0x01 << hdrFld);
    }
    else {
        puint32 = &((pBitMap)->two);
        presence = (0x01 << (hdrFld - eSIP_HF_THIRTY_TWO));
    }  
    (*puint32) &= ~presence;
}

/* these are used stricly for the random string genereators */
static char _randomNumberCharTable[]= ".0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static int _randomNumberCharTableSize = (sizeof(_randomNumberCharTable) - 1);
/* 
 ******************************************************************************
 * ================HF_GenerateCallId()===================
 *
 * This function generates a random callId.  If there is a domain name
 * provided in pDomain, then it will be included in the callid.  Adding the 
 * domain name in the callid is a RECOMMENDATION in RFC3261.
 *
 * pDomain = A string specifying a domain name.  If NULL then no domain is used
 *
 * pTargetStr = The target location to write the string.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_GenerateCallId(
    char *pDomain, 
    char *pTargetStr)
{
    int x;
    int len = 0;
    OSAL_NetAddress addr;
    /* 
     * First generate the length of the Call-ID random number
     * that will be before the domain name 
     */
    len = SIP_randInt(SIP_CALL_ID_HF_RAND_MIN_LEN,SIP_CALL_ID_HF_RAND_MAX_LEN);

    /* we have our length */
    for (x = 0 ; x < len ; x++) {
        /* get a location into the array of possible chars */
        int rand = SIP_randInt(0, _randomNumberCharTableSize);
        *pTargetStr = _randomNumberCharTable[rand];
        pTargetStr++;
    }

    if (pDomain && (pDomain[0] != 0)) {
        /* add the @ sign */
        *pTargetStr = '@';
        pTargetStr++;
        /* Check if it's ipv6 string. */
        if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)pDomain, &addr) &&
                OSAL_netIsAddrIpv6(&addr)) {
            /* Embrace ipv6 string with [] */
            /* XXX no string protection. It's bad. */
            *pTargetStr = '[';
            pTargetStr++;
            OSAL_strcpy(pTargetStr, pDomain);
            pTargetStr += OSAL_strlen(pDomain);
            *pTargetStr = ']';
            pTargetStr++;
            /* null terminate */
            *pTargetStr = 0;
        }
        else {
            /* now copy the domain */
            OSAL_strcpy(pTargetStr, pDomain);
        }
    }
    else {
        /* null terminate */
        *pTargetStr = 0;
    }

    return;
}

/* 
 ******************************************************************************
 * ================HF_GenerateSeqNum()===================
 *
 * This function generates sequence number.
 *
 * RETURNS: 
 *        uint32: a random unsigned integer.
 *         
 ******************************************************************************
 */
uint32 HF_GenerateSeqNum(void)
{
    /* 
     * Generate a starting sequence number
     * for use with CSeq.  This is a random number
     * no bigger than (2^16 - 1).  Roll over is
     * not an issue as indictated in RFC3261
     */
    uint32 seq = (uint32)SIP_randInt(0, 0x0000ffff);
    return seq;
}

/* 
 ******************************************************************************
 * ================HF_GenerateTag()===================
 *
 * This function generates a random 'tag' value.  These are used in the "from"
 * and "to" header fields.
 *
 * pTargetStr = The target location to write the 'tag' string.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_GenerateTag(
    char *pTargetStr)
{
    int x;
    int len = 0;
    /* 
     * First generate the length of the Call-ID random number
     * that will be before the domain name 
     */
    len = SIP_randInt(SIP_TAG_HF_RAND_MIN_LEN,SIP_TAG_HF_RAND_MAX_LEN);

    /* we have our length */
    for (x = 0 ; x < len ; x++) {
        /* get a location into the array of possible chars */
        int rand = SIP_randInt(0, _randomNumberCharTableSize);
        *pTargetStr = _randomNumberCharTable[rand];
        pTargetStr++;
    }

    /* Null terminate it */
    *pTargetStr = 0;
    
    return;
}

/* 
 ******************************************************************************
 * ================HF_GenerateBranch()===================
 *
 * This function generates a random 'branch' value.  These are used in the 
 * "via" header field.
 *
 * pTargetStr = The target location to write the 'tag' string.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_GenerateBranch(
    char *pTargetStr)
{
    int x;
    int len = 0;
    
    
    /* first copy the cookie */
    OSAL_memCpy(pTargetStr, SIP_MAGIC_COOKIE_STR_DFLT,
            SIP_MAGIC_COOKIE_STR_LEN);
    pTargetStr = pTargetStr + SIP_MAGIC_COOKIE_STR_LEN;
        
    /* 
     * Generate the length of the Branch random number
     * that will be before the domain name 
     */
    len = SIP_randInt(SIP_BRANCH_HF_RAND_MIN_LEN,SIP_BRANCH_HF_RAND_MAX_LEN);
        
    /* we have our length */
    for (x = 0 ; x < len ; x++) {
        /* get a location into the array of possible chars */
        int rand = SIP_randInt(0, _randomNumberCharTableSize);
        *pTargetStr = _randomNumberCharTable[rand];
        pTargetStr++;
    }
    /* null terminate */
    *pTargetStr = 0;
    return;
}

/* 
 ******************************************************************************
 * ================HF_GenerateCNonce()===================
 *
 * This function generates a random 'CNonce' value for authentication.  
 *
 * pTargetStr = The target location to write the string.
 *
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void HF_GenerateCNonce(
    char *pTargetStr)
{
    int x;
    int len = 0;
    
    /* 
     * Generate the length of the Branch random number
     * that will be before the domain name 
     */
    len = SIP_randInt(SIP_CNONCE_RAND_MIN_LEN, SIP_CNONCE_RAND_MAX_LEN);
        
    /* we have our length */
    for (x = 0 ; x < len ; x++) {
        /* get a location into the array of possible chars */
        int rand = SIP_randInt(0, _randomNumberCharTableSize);
        *pTargetStr = _randomNumberCharTable[rand];
        pTargetStr++;
    }
    /* null terminate */
    *pTargetStr = 0;
    return;
}

/*
 ******************************************************************************
 * ================HF_UpdateNC()===================
 *
 * This function update nc-value  string for authentication.
 *
 * pTargetStr = The target location to write the string.
 *
 * RETURNS:
 *        Nothing
 *
 ******************************************************************************
 */
void HF_UpdateNC(
    char *pTargetStr)
{
    uint32 nc;
    vint   i = 0;

    nc = 0;
    /* Convert hex string to uint */
    nc = OSAL_strtoul(pTargetStr, 0, 16);
    nc++;

    OSAL_snprintf(pTargetStr, SIP_NONCE_CNT_ARG_STR_SIZE, "%8x", nc);
    /* replace white space to '0' */
    while (' ' == pTargetStr[i]) {
        pTargetStr[i++] = '0';
    }
}

/* 
 *****************************************************************************
 * ================HF_MagicCookieExists()===================
 *
 * This function returns true if the majic cookie exists in the string pointed
 * To by pCookie.
 *
 * pCookie = A NULL terminated string containg (typically) the 'branch'
 *           param from the via header field
 *
 * RETURNS: 
 *         TRUE:  It's there
 *         FALSE: It's not
 *         
 ******************************************************************************
 */
vint HF_MagicCookieExists(
    char* pCookie)
{
    if (0 == OSAL_memCmp(pCookie, SIP_MAGIC_COOKIE_STR_DFLT,
            SIP_MAGIC_COOKIE_STR_LEN)) {
        return TRUE;
    }
    return FALSE;
}

/* 
 *****************************************************************************
 * ================HF_CleanUriPlus()===================
 *
 * This function cleans up a tUri object.  It's faster then having to 
 * initialize via a memset or bzero function
 *
 * pUri = A pointer to a tUri object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanUri(
    tUri *pUri)
{
    pUri->scheme = eURI_SCHEME_DUMMY;
    pUri->user[0] = 0;
    pUri->szUserParam[0] = 0;
    pUri->szPhoneCxtParam[0] = 0;
    pUri->szSessionParam[0] = 0;
    pUri->host.addressType = eNwAddrNonSpecified;
    pUri->host.port = 0;
    pUri->host.x.ip.v4.ul = 0;
    OSAL_memSet(pUri->host.x.ip.v6, 0, sizeof(pUri->host.x.ip.v6));
     /* parameters */
    pUri->method = eSIP_FIRST_METHOD;
    pUri->maddr.v4.ul = 0;
    OSAL_memSet(pUri->maddr.v6, 0, sizeof(pUri->maddr.v6));
    pUri->ttl = 0;
    pUri->transport = eTransportNone;
    pUri->lr = FALSE;
    pUri->argType = eURI_ARG_TYPE_PSBR;
    pUri->arg.szPsbr[0] = 0;
    pUri->szConf[0] = 0;
    pUri->szFtag[0] = 0;
    pUri->szGruu[0] = 0;
    pUri->szLskpmc[0] = 0;
    return;
}

/* 
 *****************************************************************************
 * ================HF_CleanUriPlus()===================
 *
 * This function cleans up a tUriPlus object.  It's faster then having to 
 * initialize via a memset or bzero function
 *
 * pUri = A pointer to a tUriPlus object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanUriPlus(
    tUriPlus *pUri)
{
    HF_CleanUri(&pUri->uri);
    pUri->szDisplayName[0] = 0;
    pUri->szTag[0] = 0;
    pUri->szUser[0] = 0;
    return;
}

/* 
 *****************************************************************************
 * ================HF_CleanVia()===================
 *
 * This function cleans up a tViaHFE object.  It's faster then having to 
 * initialize via a memset or bzero function
 *
 * pVia = A pointer to a tViaHFE object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanVia(
    tViaHFE *pVia)
{
    DLLIST_InitEntry(&pVia->dll);
    OSAL_memSet(pVia->received.v6, 0, sizeof(pVia->received.v6));
    pVia->received.v4.ul = 0;
    pVia->rport          = 0;
    pVia->szBranch[0]    = 0;
    pVia->szVersion[0]   = 0;
    pVia->keep           = OSAL_FALSE;
    pVia->keepaliveFeq   = 0;
    HF_CleanUri(&pVia->uri);
    return;
}

/* 
 *****************************************************************************
 * ================HF_CleanRoute()===================
 *
 * This function cleans up a tRouteHFE object.  It's faster then having to 
 * initialize via a memset or bzero function
 *
 * pRoute = A pointer to a tRouteHFE object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanRoute(
    tRouteHFE *pRoute)
{
    DLLIST_InitEntry(&pRoute->dll);
    HF_CleanUri(&pRoute->uri);
    return;
}

/* 
 *****************************************************************************
 * ================HF_CleanContact()===================
 *
 * This function cleans up a tContactHFE object.  It's faster then having to 
 * initialize via a memset or bzero function
 *
 * pContact = A pointer to a tContactHFE object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanContact(
    tContactHFE *pContact)
{
    DLLIST_InitEntry(&pContact->dll);
    pContact->expires = 0;
    pContact->q[0] = 0;
    pContact->szDisplayName[0] = 0;
    pContact->szUser[0] = 0;
    pContact->isImSession = 0;
    pContact->capabilitiesBitmap = 0;
    pContact->isFocus = 0;
    pContact->szInstance[0] = 0;
    pContact->szPublicGruu[0] = 0;
    HF_CleanUri(&pContact->uri);
    return;
}

/* 
 *****************************************************************************
 * ================HF_CleanAuthorization()===================
 *
 * This function cleans up a tAuthorizationHFE object.  It's faster then having
 * to initialize via a memset or bzero function
 *
 * pAuth = A pointer to a tAuthorizationHFE object 
 *
 * RETURNS: 
 *         Nothing
 *         
 ******************************************************************************
 */
void HF_CleanAuthorization(
    tAuthorizationHFE *pAuth)
{
    DLLIST_InitEntry(&pAuth->dll);
    HF_CleanUri(&pAuth->uri);
    pAuth->szRealm[0] = 0;
    pAuth->domain[0] = 0;
    pAuth->szNonce[0] = 0;
    pAuth->szOpaque[0] = 0;
    pAuth->szUsername[0] = 0;
    pAuth->szCNonce[0] = 0;
    pAuth->szResponse[0] = 0;
    pAuth->szB64UserPw[0] = 0;
    pAuth->szNC[0] = 0;
    pAuth->presence = 0;
    pAuth->hdrFld = eSIP_HF_START;
    pAuth->qop = eSIP_QOP_NONE;
    pAuth->alg = eAUTH_ALG_NONE;
    pAuth->type = eSIP_AUTH_TYPE_BASIC;
    pAuth->stale = FALSE;
    return;
}

/* 
 *****************************************************************************
 * ================HF_MakeVia()===================
 *
 * This function is used to help initalize a via header field.
 * If successful the object pointed to by pViaList will be initialized and 
 * populated with branch and received parameters.
 *
 * pViaList = the list the a new via header field is added to if the function
 *            is successful.
 *
 * pUri = A pointer to a tUri object specifying the uri that should 
 *        be populated in the via, note that for via we only need the 'host' 
 *        portion.
 * 
 * pBranch = A pointer to the branch to populate in the via.  
 *           If NULL then the function will produce it's own 'branch' param
 *
 * received = The IP address to include in the via.  If zero, nothing will be 
 *            populated in the 'received' param in the via header field.
 *
 * keep = The parameter that explicitly indicate willingness to send keep-alives
 *        towards its adjacent downstream SIP entity
 *
 * RETURNS: 
 *         SIP_OK: The function was succesful
 *         SIP_NO_MEM: Could not allocate a new via object.
 *         
 ******************************************************************************
 */
vint HF_MakeVia(
    tDLList        *pViaList,
    tUri           *pUri,
    char           *pBranch,
    tIPAddr         received,
    OSAL_Boolean    keep)
{
    tViaHFE *pVia;
    
    /* build a via */
    pVia = (tViaHFE *)SIP_memPoolAlloc(eSIP_OBJECT_VIA_HF);
    if (pVia) {
        HF_CleanVia(pVia);
        OSAL_strcpy(pVia->szVersion, SIP_VERSION_STR);
        /* copy or make a branch */
        if (pBranch) {
            OSAL_strcpy(pVia->szBranch, pBranch);
        }
        else {
            HF_GenerateBranch(pVia->szBranch);
        }
        /* copy a uri if one exists */
        if (pUri) {
            /* copy in the uri but the 'host portion only. 
             * Via's can't have user names etc, that's why 
             * we only need the 'host' portion */
            pVia->uri.host = pUri->host;
        }
        /* copy the received IP address */
        pVia->received = received;
        /* copy if turn on 'keep' */
        pVia->keep = keep;
        /* enqueue the via */
        DLLIST_Enqueue(pViaList, &pVia->dll);
        return (SIP_OK);
    }
    else {
        return (SIP_NO_MEM);
    }
}

/* 
 *****************************************************************************
 * ================HF_MakeViaBranch()===================
 *
 * This function is used to make a new via 'branch' for a via object that's
 * already in use.
 *
 * pViaList = The list the via header fields 
 *
 * pBranch = A pointer to the 'branch' to populate in the via.  
 *           If NULL then the function will produce it's own 'branch' param.
 *
 * RETURNS: 
 *         SIP_BADPARM: The function receivied a bad parameter
 *         SIP_OK: The function was succesful
 *         
 ******************************************************************************
 */
vint HF_MakeViaBranch(
    tDLList *pViaList, 
    char    *pBranch)
{
    tViaHFE *pVia;
    tDLListEntry *pEntry;
    
    /* get the topmost via and change the branch parameter */
    pEntry = NULL;
    if (DLLIST_GetNext(pViaList, &pEntry)) {
        pVia = (tViaHFE *)pEntry;
        /* copy or make a branch */
        if (pBranch) {
            OSAL_strcpy(pVia->szBranch, pBranch);
        }
        else {
            HF_GenerateBranch(pVia->szBranch);
        }
        return (SIP_OK);
    }
    else {
        return (SIP_BADPARM);
    }
}

void HF_CatViaBranch(
    char    *pBranch)
{
    uint32 checkSum;
    vint   size;
    char   str[SIP_MAX_BASETEN_NUM_STRING + 2];

    size = 0;
    checkSum = 0;
    while (pBranch[size] != 0) {
        checkSum += ((uint32)pBranch[size]);
        size++;
    }
    /* now we have the checksum, so turn it to a string 
     * but also place a'-' in front 
     */
    str[0] = '-';
    OSAL_itoa(checkSum, &str[1], SIP_MAX_BASETEN_NUM_STRING);
    pBranch += size;
    /* Get the difference so we don't overflow any buffers */
    size = (SIP_BRANCH_HF_STR_SIZE - size);
    OSAL_strncpy(pBranch, str, size);
}

void HF_Insert(
    tHdrFldList **ppHead, 
    tHdrFldList  *pItem)
{
    if (!(*ppHead)) {
        /* then there's nothing there */
        *ppHead = pItem;
    }
    else {
        pItem->pNext = *ppHead;
        *ppHead = pItem;
        /*pItem is now first */
    }
}

tHdrFldList* HF_Find(
    tHdrFldList **ppHead, 
    tHdrFld       hf)
{
    tHdrFldList  *pCurr;
    
    pCurr = *ppHead;
    while (pCurr) {
        if (pCurr->hf == hf) {
            /* found it */
            return pCurr;
        }
        pCurr = pCurr->pNext;
    }
    return NULL;
}

vint HF_CopyInsert(
    tHdrFldList **ppHead,
    tHdrFld       hf,
    char         *pStr,
    uint32        offset)
{
    tHdrFldList *pItem;
    if (!ppHead || pStr == NULL) {
        return (SIP_FAILED);
    }
    else {
        pItem = (tHdrFldList *)SIP_memPoolAlloc(eSIP_OBJECT_HF_LIST);
        if (pItem) {
            pItem->hf = hf;
            pItem->pNext = NULL;
            OSAL_strncpy(pItem->pStart, pStr, sizeof(pItem->pStart));
            pItem->pField = pItem->pStart + offset;
            HF_Insert(ppHead, pItem);
            return (SIP_OK); 
        }
        else {
            return (SIP_NO_MEM);
        }
    }
}

vint HF_CopyAll(
    tHdrFldList **ppTo,
    tHdrFldList  *pFrom,
    tPres64Bits  *pBitMap)
{
    tHdrFldList  *pCurr;
    
    pCurr = pFrom;
    while (pCurr) {
        if (HF_CopyInsert(ppTo, pCurr->hf, pCurr->pStart,
                (pCurr->pField - pCurr->pStart)) == SIP_OK) {
            if (pBitMap) {
                /* then enable the header field in the bit in the bitmask */
                HF_SetPresence(pBitMap, pCurr->hf);
            }
        }
        else {
            return (SIP_FAILED);
        }
        pCurr = pCurr->pNext;
    }

    return (SIP_OK);
}

void HF_DeleteAll(
    tHdrFldList **ppHFList)
{
    tHdrFldList *pOld;
    tHdrFldList *pCurr;

    /* free all the strings */
    pCurr = *ppHFList;
    while (pCurr) {
        pOld = pCurr;
        pCurr = pCurr->pNext;
        /* release the holder */
        SIP_memPoolFree(eSIP_OBJECT_HF_LIST, (tDLListEntry *)pOld);
    }
    *ppHFList = 0;
}

void HF_Delete(
    tHdrFldList **ppHead, 
    tHdrFld       hf)
{
    tHdrFldList  *pCurr;
    tHdrFldList  *pPrev;
    tHdrFldList  *pOld;
    
    /* Search and free any instance of the header field */
    pPrev = pCurr = *ppHead;
    while (pCurr) {
        if (pCurr->hf == hf) {
            /* Found it, let's delete it */
            pOld = pCurr;
            /* Advance the pCurr, pPrev, and head if need be */
            if (pCurr == *ppHead) {
                *ppHead = pCurr->pNext;
                 pPrev = pCurr->pNext;
            }
            else {
                pPrev->pNext = pCurr->pNext;
            }
            pCurr = pCurr->pNext;
            SIP_memPoolFree(eSIP_OBJECT_HF_LIST, (tDLListEntry *)pOld);
        }
        else {
            pPrev = pCurr;
            pCurr = pCurr->pNext;
        }
    }
    return;
}

vint HF_CheckRequiredHF(tHdrFldList **ppHead, char *value_ptr)
{
    tHdrFldList *requireHf_ptr;

    if (NULL == (requireHf_ptr = HF_Find(ppHead, eSIP_REQUIRE_HF))) {
        /* Check for 'timer' */
        return (SIP_FAILED);
    }
    /*
     * Found the require header field, now let's look for the value,
     * for example 'timer'.
     */
     if (NULL != OSAL_strscan(requireHf_ptr->pField, value_ptr)) {
        /* Found it */
         return (SIP_OK);
     }
    return (SIP_FAILED);
}

vint HF_CheckSupportedHF(tHdrFldList **ppHead, char *value_ptr)
{
    tHdrFldList *requireHf_ptr;

    if (NULL == (requireHf_ptr = HF_Find(ppHead, eSIP_SUPPORTED_HF))) {
        /* Check for 'timer' */
        return (SIP_FAILED);
    }
    /*
     * Found the require header field, now let's look for the value,
     * for example 'timer'.
     */
     if (NULL != OSAL_strscan(requireHf_ptr->pField, value_ptr)) {
        /* Found it */
         return (SIP_OK);
     }
    return (SIP_FAILED);
}

/* 
 *****************************************************************************
 * ================HF_MakeViaBranch()===================
 *
 * To check whether a header field exist on Last HF or not
 *
 * ppHead = header fields 
 *
 * hdr_ptr = A pointer to the header field
 *
 * RETURNS: 
 *         SIP_FAILED: Can't find the header field
 *         SIP_OK: Found header field
 *         
 ******************************************************************************
 */

vint HF_CheckUnKnownHF(tHdrFldList **ppHead, char *hdr_ptr)
{
    tHdrFldList *lastHf_ptr;

    /* Get LAST header field list*/
    if (NULL == (lastHf_ptr = HF_Find(ppHead, eSIP_LAST_HF))) {
        return (SIP_FAILED);
    }
    /* Check hdr_ptr */
    while (lastHf_ptr != NULL) {
        if (OSAL_strncasecmp(lastHf_ptr->pStart, hdr_ptr, 
            OSAL_strlen(hdr_ptr)) == 0) {
            return (SIP_OK);
        }
        lastHf_ptr = lastHf_ptr->pNext;
    }
    return (SIP_FAILED);
}

