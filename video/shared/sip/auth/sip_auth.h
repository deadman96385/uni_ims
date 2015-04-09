/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29863 $ $Date: 2014-11-17 10:16:04 +0800 (Mon, 17 Nov 2014) $
 */

#ifndef _SIP_AUTH_H_
#define _SIP_AUTH_H_

/* Definitions needed for Mutual Authenication (AKA version 1 & 2). */
#define SIP_AUTH_AKA_RANDLEN    (16)

#define SIP_AUTH_AKA_AUTNLEN    (16)

#define SIP_AUTH_AKA_SQNLEN     (6)

#define SIP_AUTH_AKA_MACLEN     (8)

#define SIP_AUTH_AKA_RESLEN     (16)

#define SIP_AUTH_AKA_AKLEN      (6)

#define SIP_AUTH_AKA_CKLEN      (16)

#define SIP_AUTH_AKA_IKLEN      (16)

#define SIP_AUTH_AKA_AUTSLEN    (14)

/* 
 * Masks used to determine whether or not passwords or 
 * AKA subscription keys are used.
 */
#define SIP_AUTH_AKA_PASSWORD   0x00000001

#define SIP_AUTH_AKA_KEY        0x00000002

#define SIP_AUTH_AKAV2_PWD      "http-digest-akav2-password"

vint AUTH_Response(
    char       *pUserName,
    tDLList    *pCredentials,
    tUri       *pUri,
    tDLList    *pAuthChallenge, 
    char       *pMethod,
    tDLList    *pAuthResponse,
    char       *pAkaAuthResp,
    vint        akaAuthResLen,
    char       *pAkaAuthAuts);

#endif
