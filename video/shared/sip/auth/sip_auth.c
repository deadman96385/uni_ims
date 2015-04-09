/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29863 $ $Date: 2014-11-17 10:16:04 +0800 (Mon, 17 Nov 2014) $
 */

#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_clib.h"
#include "sip_hdrflds.h"
#include "sip_list.h"
#include "sip_parser_enc.h"
#include <auth_digcalc.h>
#include <auth_b64.h>
#include <milenage.h>
#include <sip_auth.h>
#include "sip_mem_pool.h"

/* static prototypes */

static vint _AUTH_Digest(
    tAUTH_Cred        *pCred,
    tUri              *pUri,
    tAuthorizationHFE *pChallenge, 
    tAuthorizationHFE *pResponse,
    char              *pMethod,
    char              *pAkaAuthResp,
    vint               akaAuthResLen,
    char              *pAkaAuthAuts);

static vint _AUTH_Basic(
    tAUTH_Cred        *pCred,
    tAuthorizationHFE *pResponse);

static vint _AUTH_aka(
    vint               version,
    tAUTH_Cred        *pCred,
    char              *pUri,
    char              *pNonce,
    char              *pRes,
    vint               resLen);

/* 
 *****************************************************************************
 * ================AUTH_Response()===================
 *
 * This function is the one and only interface used to oerform authentication
 * It will handle both Basic and Digest using MD5 authentication
 *
 * pUserName = a NULL terminated string of the username for this challenge
 * 
 * pCredentials = A pointer to a list of credentials
 *                (i.e. username, realm, password)
 *
 * pUri = The URI that was in the challenge.
 * 
 * pAuthChallenge = A pointer to the information in the 'WWW-Authenticate'
 *                  or 'Proxy-Authenticate' header fields.
 *
 * pMethod = A NULL temrinated string containing the name of the method.
 *           It's used when encoding the response.
 * 
 * pAuthResponse = A DLLIST that will contain all the responses to th 
 *                 challenges.  This ultimately added to the SIP response
 *                 is either the 'Authenticate' header field.
 *
 * akaAuthResLen = The length of response.
 *
 * pAkaAuthAuts = A pointer to the AKA AUTS.
 *
 * RETURNS:
 *     SIP_BADPARM: The function got a bad paramter passed in 
 *      SIP_FAILED: Could not successfully authenticate the challenges
 *          SIP_OK: The function was succesful and the pAuthResponse is 
 *                  properly populated.
 *****************************************************************************
 */
vint AUTH_Response(
    char              *pUserName,
    tDLList           *pCredentials,
    tUri              *pUri,
    tDLList           *pAuthChallenge,
    char              *pMethod,
    tDLList           *pAuthResponse,
    char              *pAkaAuthResp,
    vint               akaAuthResLen,
    char              *pAkaAuthAuts)
{
    tDLListEntry      *pListEntry;
    tDLListEntry      *pEntry;
    tAuthorizationHFE *pNewAuth;
    tAUTH_Cred        *pCred;
    tAUTH_Cred        *pUserCred;
    tAUTH_Cred         cred;
    vint               status;
    
    pListEntry = NULL;

    if (pUri == NULL || pAuthChallenge == NULL || pMethod == NULL ||
            pAuthResponse == NULL) {
        return (SIP_BADPARM);
    }
    /* clear out any old pAuthResponse entries */
    DLLIST_Empty(pAuthResponse, eSIP_OBJECT_AUTH_HF);
    
    while (DLLIST_GetNext(pAuthChallenge, &pListEntry)) {
        tAuthorizationHFE *pAuth = (tAuthorizationHFE *)pListEntry;
         
        /*
         * Bug 10154: It is invalid to get AUTHORICATION header in AUTH
         * response. Ignore it.
         */
        if (pAuth->hdrFld == eSIP_AUTHORIZATION_HF)
            continue;

        pEntry = NULL;
        pCred = NULL;
        pUserCred = NULL;
        while (pCredentials && DLLIST_GetNext(pCredentials, &pEntry)) {
            if (pUserName) {
                if (!OSAL_strcasecmp(((tAUTH_Cred*)pEntry)->szUsername,
                        pUserName)) {
                    pUserCred = (tAUTH_Cred*)pEntry;
                    /* found a username match */
                    if (!OSAL_strcasecmp(((tAUTH_Cred*)pEntry)->szRealm,
                            pAuth->szRealm)) {
                        /* found it */
                        pCred = (tAUTH_Cred*)pEntry;
                        break;
                    }
                }
            }
            else {
                /* we don't have a username so just try searching the realm */
                if (!OSAL_strcasecmp(((tAUTH_Cred*)pEntry)->szRealm,
                        pAuth->szRealm)) {
                    /* found it */
                    pCred = (tAUTH_Cred*)pEntry;
                    break;
                }
            }
        }
        
        /*
         * If there are no credentials or if credentials for the 
         * endpoint do not match the 'realm' in the challenge then try
         * defaults.
         */  
        if (!pCred) {
#ifndef USE_RFC3261_USERNAME_DEFAULT
            /* Then try the username and password from the 
             * first credential entry.
             */
            if (pUserCred) {
                OSAL_memCpy(&cred, pUserCred, sizeof(tAUTH_Cred));
                /* Replace the realm with the one in the challenge */
                OSAL_strcpy(cred.szRealm, pAuth->szRealm);
            }
            else {
                /* if we are here then there is no username or 
                 * realm found for this challenge.  So just try the first 
                 * thing this UA has configured.
                 */ 
                pEntry = NULL;
                if (pCredentials && DLLIST_GetNext(pCredentials, &pEntry)) {
                    pCred = (tAUTH_Cred*)pEntry; 
                    OSAL_memCpy(&cred, pCred, sizeof(tAUTH_Cred));
                    /* Replace the realm with the one in the challenge */
                    OSAL_strcpy(cred.szRealm, pAuth->szRealm);
                }
                else {
#endif
                    /* attempt a default this is from RFC3261 section 22.3 */
                    OSAL_strcpy(cred.szUsername, SIP_ANONYMOUS_NAME);
                    OSAL_strcpy(cred.szRealm, pAuth->szRealm);
                    cred.szPassword[0] = 0;
                    cred.types = SIP_AUTH_AKA_PASSWORD;
#ifndef USE_RFC3261_USERNAME_DEFAULT
                }
            }
#endif
            /* RFC3261 section 22.2 says password should be "" */
            pCred = &cred;
        }
        
        /* set up credentials */
        if (NULL == (pNewAuth =
                (tAuthorizationHFE *)SIP_memPoolAlloc(eSIP_OBJECT_AUTH_HF))) {
            DLLIST_Empty(pAuthResponse, eSIP_OBJECT_AUTH_HF);
            return (SIP_NO_MEM);
        }
        
        pNewAuth->hdrFld = pAuth->hdrFld;

        if (pAuth->type == eSIP_AUTH_TYPE_DIGEST) {
            status = _AUTH_Digest(pCred, pUri, pAuth, pNewAuth, pMethod,
                    pAkaAuthResp, akaAuthResLen, pAkaAuthAuts);
            if (status != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_AUTH_HF, (tDLListEntry *)pNewAuth);
                DLLIST_Empty(pAuthResponse, eSIP_OBJECT_AUTH_HF);
                return (status);
            }
            else {
                /* good stuff, insert in output list */
                DLLIST_InitEntry(&pNewAuth->dll);
                DLLIST_Enqueue(pAuthResponse, &pNewAuth->dll);
                return (SIP_OK);
            }
        } /* end of Digest Auth */
        else {
            if (_AUTH_Basic(pCred, pNewAuth) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_AUTH_HF, (tDLListEntry *)pNewAuth);
                DLLIST_Empty(pAuthResponse, eSIP_OBJECT_AUTH_HF);
                return (SIP_FAILED); 
            }
            else {
                /* good stuff, insert in output list */
                DLLIST_InitEntry(&pNewAuth->dll);
                DLLIST_Enqueue(pAuthResponse, &pNewAuth->dll);
                return (SIP_OK);
            }
        }
    } /* end of while */
    return SIP_FAILED;
}

/* this logic is found in RFC2617 section 2 */
static vint _AUTH_Basic(
    tAUTH_Cred        *pCred,
    tAuthorizationHFE *pResponse)
{
    char str[SIP_B64_USER_PW_STR_SIZE];
    int len;

    pResponse->type = eSIP_AUTH_TYPE_BASIC;

    /* build the string to encode in base64 */
    OSAL_strcpy(str, pCred->szUsername);
    len = OSAL_strlen(str);
    str[len] = ':';
    OSAL_strcpy((str + len + 1), pCred->szPassword);
    len = OSAL_strlen(str);

    if (!b64encode(str, pResponse->szB64UserPw, len))
        return (SIP_FAILED);

    HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_B64_USER_PW_HF_ARG);
    return (SIP_OK);
}

/* this logic is found in RFC2617 section 3 */
static vint _AUTH_Digest(
    tAUTH_Cred        *pCred,
    tUri              *pUri,
    tAuthorizationHFE *pChallenge, 
    tAuthorizationHFE *pResponse,
    char              *pMethod,
    char              *pAkaAuthResp, /*
                                      * pString is the sip method that the
                                      * authorization is getting included in
                                      */
    vint               akaAuthResLen,
    char              *pAkaAuthAuts)
{
    char    alg[16];
    char    qop[16];
    char    uriStr[SIP_URI_STRING_MAX_SIZE];
    char    res[SIP_AUTH_AKA_RESLEN + SIP_AUTH_AKA_IKLEN + SIP_AUTH_AKA_CKLEN];
    char    hmac_digest[16];
    uint32  hmacLen;
    uint32  resLen;
    uint32  uriStrLen = SIP_URI_STRING_MAX_SIZE;
    HASHHEX SessionKey;
    HASHHEX HEntity = ""; /* this is not used right now */
    HASHHEX Response;

    /* init objects */
    alg[0] = 0;
    qop[0] = 0;
    resLen = 0;
    OSAL_memSet(res, 0, sizeof(res));

    if (pChallenge->alg == eAUTH_ALG_MD5 || pChallenge->alg == eAUTH_ALG_NONE) {
        OSAL_strcpy(alg, SIP_AUTH_ALG_MD5_STR);
        pResponse->alg = eAUTH_ALG_MD5;
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_ALGORITHM_HF_ARG);
    }
    else if (pChallenge->alg == eAUTH_ALG_AKAV1_MD5) {
        /* If key is not set, pass nonce to app to try ISIM AKA */
        if (0 == (pCred->types & SIP_AUTH_AKA_KEY) && NULL == pAkaAuthResp) {
            return (SIP_AUTH_AKA_V1);
        }
        OSAL_strcpy(alg, SIP_AUTH_ALG_AKAV1_MD5_STR);
        pResponse->alg = eAUTH_ALG_AKAV1_MD5;
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_ALGORITHM_HF_ARG);
    }
    else if (pChallenge->alg == eAUTH_ALG_AKAV2_MD5) {
        /* If key is not set, pass nonce to app to try ISIM AKA */
        if (0 == (pCred->types & SIP_AUTH_AKA_KEY) && NULL == pAkaAuthResp) {
            return (SIP_AUTH_AKA_V2);
        }
        OSAL_strcpy(alg, SIP_AUTH_ALG_AKAV2_MD5_STR);
        pResponse->alg = eAUTH_ALG_AKAV2_MD5;
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_ALGORITHM_HF_ARG);
    }
    else {
        return (SIP_NOT_SUPPORTED);
    }
    
    pResponse->type = eSIP_AUTH_TYPE_DIGEST;
    
    OSAL_strcpy(pResponse->szUsername, pCred->szUsername);
    HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_USERNAME_HF_ARG);
    
    OSAL_strcpy(pResponse->szRealm, pCred->szRealm);
    HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_REALM_HF_ARG);

    OSAL_strcpy(pResponse->szNonce, pChallenge->szNonce);
    HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_NONCE_HF_ARG);

    if (HF_ARG_PRESENCE_EXISTS(&pChallenge->presence, eSIP_OPAQUE_HF_ARG)) {
        OSAL_strcpy(pResponse->szOpaque, pChallenge->szOpaque);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_OPAQUE_HF_ARG);
    }

    pResponse->uri = *pUri;
    HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_URI_HF_ARG);

    if (pChallenge->qop != eSIP_QOP_NONE) {
        /* GENERATE the CNONCE HERE */
        /* GENERATE THE NONCE COUNT HERE */
        SIP_DebugLog(SIP_DB_GENERAL_LVL_3,
                "_AUTH_Digest: qop is present qop:%d",
                (int)pChallenge->qop, 0, 0);

        HF_GenerateCNonce(pResponse->szCNonce);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_CNONCE_HF_ARG);

        HF_UpdateNC(pChallenge->szNC);
         /* copy to response */
        OSAL_strcpy(pResponse->szNC, pChallenge->szNC);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_NC_HF_ARG);

        if (pChallenge->qop ==  eSIP_QOP_AUTH) {
            OSAL_strcpy(qop, SIP_QOP_AUTH_STR);
            pResponse->qop = pChallenge->qop;
            HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_QOP_HF_ARG);
        }
        else if (pChallenge->qop ==  eSIP_QOP_AUTH_INT) {
            OSAL_strcpy(qop, SIP_QOP_AUTH_INT_STR);
        }
    }
    
    /* convert the uri to a string for encryption */
    ENC_Uri(pUri, uriStr, &uriStrLen, 0);
    uriStr[uriStrLen] = 0;
    
    if (pResponse->alg == eAUTH_ALG_MD5) {
        /* Check that we are configured with a password */
        if (0 == (pCred->types & SIP_AUTH_AKA_PASSWORD)) {
            /* No password configured */
            return (SIP_FAILED);
        }

        /* generate the response */
        DigestCalcHA1(alg, 
                pCred->szUsername, 
                pCred->szRealm, 
                pCred->szPassword,
                OSAL_strlen(pCred->szPassword),
                pResponse->szNonce,
                pResponse->szCNonce,
                SessionKey);

        DigestCalcResponse(
                SessionKey, 
                pResponse->szNonce,  
                pResponse->szNC,    /* 8 hex digits */
                pResponse->szCNonce,
                qop,                 
                pMethod,
                uriStr,                   
                HEntity,  /*
                           * H(entity body) if qop="auth-int"
                           * NOT SUPPORTED RIGHT NOW
                           */
                Response);

        OSAL_strcpy(pResponse->szResponse, Response);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_RESPONSE_HF_ARG);
        return (SIP_OK);
    }
    else if (pResponse->alg == eAUTH_ALG_AKAV1_MD5) {
        /* If AKA auth resp is given from app, use it */
        if (NULL != pAkaAuthResp) {

            if (((4 > akaAuthResLen) || (16 < akaAuthResLen)) &&
                    (NULL == pAkaAuthAuts)) {
                /* Network failure or invalid response length.*/
                pResponse->szResponse[0] = '\0';
                HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_RESPONSE_HF_ARG);
                return (SIP_OK);
            }
            else if (NULL != pAkaAuthAuts) {
                /*
                 * RFC 3310 3.4 Synchronization Failure
                 *  auts
                 *  A string carrying a base64 encoded AKA AUTS parameter.  This
                 *  directive is used to re-synchronize the server side SQN.
                 *  If the directive is present, the client doesn't use any
                 *  password when calculating its credentials.
                 *  Instead, the client MUST calculate its credentials using
                 *  an empty password (password of "").
                 */
                /* res and resLen already are 0  */

                if (!OSAL_cryptoB64Encode(pAkaAuthAuts, pResponse->szAuts,
                        SIP_AUTH_AKA_AUTSLEN)) {
                    return (SIP_FAILED);
                }
                HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_AUTS_HF_ARG);
            }
            else {
                /*
                 * Successful authentication.
                 * Set res for calculate digest.
                 */
                OSAL_memCpy(res, pAkaAuthResp, akaAuthResLen);
                resLen = akaAuthResLen;
            }
        }
        else {
            /* Construct the "res" */
            if (SIP_OK != _AUTH_aka(1, pCred, uriStr, pChallenge->szNonce, res,
                    sizeof(res))) {
                return (SIP_FAILED);
            }
        }
 
        /* Okay now create the response using the "res" as the password */
        DigestCalcHA1(alg, 
                pCred->szUsername, 
                pCred->szRealm, 
                res,
                resLen,
                pResponse->szNonce,
                pResponse->szCNonce,
                SessionKey);

        DigestCalcResponse(
                SessionKey, 
                pResponse->szNonce,  
                pResponse->szNC,    /* 8 hex digits */
                pResponse->szCNonce,
                qop,                 
                pMethod,
                uriStr,                   
                HEntity,  /* H(entity body) if qop="auth-int" NOT SUPPORTED RIGHT NOW */
                Response);

        OSAL_strcpy(pResponse->szResponse, Response);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_RESPONSE_HF_ARG);
        return (SIP_OK);
    }
    else if (pResponse->alg == eAUTH_ALG_AKAV2_MD5) {
        if (NULL != pAkaAuthResp) {
            if ((((4 + SIP_AUTH_AKA_IKLEN + SIP_AUTH_AKA_CKLEN) > akaAuthResLen)
                    || ((16 + SIP_AUTH_AKA_IKLEN + SIP_AUTH_AKA_CKLEN) <
                    akaAuthResLen)) && (NULL == pAkaAuthAuts)) {
                /* Network failure or invalid response length.*/
                pResponse->szResponse[0] = '\0';
                HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_RESPONSE_HF_ARG);
                return (SIP_OK);
            }
            else if (NULL != pAkaAuthAuts) {
                /*
                 * RFC 3310 3.4 Synchronization Failure
                 *  auts
                 *  A string carrying a base64 encoded AKA AUTS parameter.  This
                 *  directive is used to re-synchronize the server side SQN.
                 *  If the directive is present, the client doesn't use any
                 *  password when calculating its credentials.
                 *  Instead, the client MUST calculate its credentials using
                 *  an empty password (password of "").
                 */
                /* res and resLen already are 0  */

                if (!OSAL_cryptoB64Encode(pAkaAuthAuts, pResponse->szAuts,
                        SIP_AUTH_AKA_AUTSLEN)) {
                    return (SIP_FAILED);
                }
                HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_AUTS_HF_ARG);
            }
            else {
                /*
                 * For AKAv2, response is base64 encoded of 
                 * PRF(RES||IK||CK, "http-digest-akav2-password")
                 * pAkaAuthResp is already RES||IK||CK from application.
                 * Now do hamc_md5 and base64 encode.
                 */
                OSAL_cryptoHmac(pAkaAuthResp, akaAuthResLen,
                        OSAL_CRYPTO_MD_ALGO_MD5,
                        (const unsigned char *)SIP_AUTH_AKAV2_PWD, OSAL_strlen(SIP_AUTH_AKAV2_PWD),
                        (unsigned char *)hmac_digest, (size_t *)&hmacLen);
                resLen = OSAL_cryptoB64Encode(hmac_digest, res, hmacLen);
            }
        }
        else {
            /* Should not be here. */
            return (SIP_FAILED);
        }
 
        DigestCalcHA1(alg, 
                pCred->szUsername, 
                pCred->szRealm, 
                res,
                resLen,
                pResponse->szNonce,
                pResponse->szCNonce,
                SessionKey);

        DigestCalcResponse(
                SessionKey, 
                pResponse->szNonce,  
                pResponse->szNC,    /* 8 hex digits */
                pResponse->szCNonce,
                qop,                 
                pMethod,
                uriStr,                   
                HEntity,  /*
                           * H(entity body) if qop="auth-int"
                           * NOT SUPPORTED RIGHT NOW
                           */
                Response);

        OSAL_strcpy(pResponse->szResponse, Response);
        HF_ARG_SET_PRESENCE(&pResponse->presence, eSIP_RESPONSE_HF_ARG);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _AUTH_aka(
    vint               version,
    tAUTH_Cred        *pCred,
    char              *pUri,
    char              *pNonce,
    char              *pRes,
    vint               resLen)
{
    vint    len;
    vint    i;
    uint8   nonce[SIP_NONCE_ARG_STR_SIZE + 1];
    uint8  *rand_ptr;
    uint8  *sqn_ptr;
    uint8  *mac_ptr;
    uint8   k[SIP_AUTH_KEY_SIZE + 1];
    uint8   op[SIP_AUTH_OP_SIZE + 1];
    uint8   amf[SIP_AUTH_AMF_SIZE + 1];
    uint8   ck[SIP_AUTH_AKA_CKLEN + 1];
    uint8   ik[SIP_AUTH_AKA_IKLEN + 1];
    uint8   ak[SIP_AUTH_AKA_AKLEN + 1];
    uint8   sqn[SIP_AUTH_AKA_SQNLEN + 1];
    uint8   xmac[SIP_AUTH_AKA_MACLEN + 1];
    uint8   res[SIP_AUTH_AKA_RESLEN + 1];
    
    /* Zero the nonce */
    OSAL_memSet(nonce, 0, sizeof(nonce));
    
    if (0 == (len = b64decode(pNonce, OSAL_strlen(pNonce), (char*)nonce))) {
        /* Failed to decode the nonce */
        return (SIP_FAILED);
    }
    
    /* Verify the length of the decoded nonce */
    if (len < SIP_AUTH_AKA_RANDLEN + SIP_AUTH_AKA_AUTNLEN) {
        return (SIP_FAILED);
    }
    /* Get RAND, AUTN, and MAC */
    rand_ptr = nonce;
    sqn_ptr = nonce + SIP_AUTH_AKA_RANDLEN;
    mac_ptr = nonce + SIP_AUTH_AKA_RANDLEN + 
            SIP_AUTH_AKA_SQNLEN + SIP_AUTH_AMF_SIZE;

    /* Copy k. op, and amf */
    OSAL_memSet(k, 0, sizeof(k));
    OSAL_memSet(op, 0, sizeof(op));
    OSAL_memSet(amf, 0, sizeof(amf));
    
    /* Check that we are configured with a subscription key */
    if (0 == (pCred->types & SIP_AUTH_AKA_KEY)) {
        /* No key configured, try the 'password' of the credentials then */
        OSAL_snprintf((char*)k, sizeof(k), "%s", pCred->szPassword);
    }
    else {
        /* Then let's use the "k" value */
        OSAL_memCpy(k, pCred->aka.k, pCred->aka.kLen);
    }
    
    /* Optional 'op' code */
    if (0 != pCred->aka.opLen) {
        OSAL_memCpy(op, pCred->aka.op, pCred->aka.opLen);
    }
    if (0 != pCred->aka.amfLen) {
        OSAL_memCpy(amf, pCred->aka.amf, pCred->aka.amfLen);
    }


    /* Given key K and random challenge RAND, compute response RES,
     * confidentiality key CK, integrity key IK and anonymity key AK.
     */
    f2345(k, rand_ptr, res, ck, ik, ak, op);

    /* Compute sequence number SQN */
    for (i = 0 ; i < SIP_AUTH_AKA_SQNLEN ; ++i) {
        sqn[i] = (char) (sqn_ptr[i] ^ ak[i]);
    }

    /* Verify MAC in the challenge */
    f1(k, rand_ptr, sqn, amf, xmac, op);

    if (0 != OSAL_memCmp(mac_ptr, xmac, SIP_AUTH_AKA_MACLEN)) {
        return (SIP_FAILED);
    }
    
    if (1 == version) {
        /* Verify the length */
        if (resLen < SIP_AUTH_AKA_RESLEN) {
            return (SIP_FAILED);
        }
        /* Copy to target */
        OSAL_memCpy(pRes, res, SIP_AUTH_AKA_RESLEN);
    }
    else {
        /* Must be version 2, verify the length and copy to target */
        if (resLen < (SIP_AUTH_AKA_RESLEN + SIP_AUTH_AKA_IKLEN +
                SIP_AUTH_AKA_CKLEN)) {
            return (SIP_FAILED);
        }
        OSAL_memCpy(pRes, res, SIP_AUTH_AKA_RESLEN);
        OSAL_memCpy(pRes + SIP_AUTH_AKA_RESLEN, ik, SIP_AUTH_AKA_IKLEN);
        OSAL_memCpy(pRes + SIP_AUTH_AKA_RESLEN + SIP_AUTH_AKA_IKLEN, 
                ck, SIP_AUTH_AKA_CKLEN);
    }
    return (SIP_OK);
}
