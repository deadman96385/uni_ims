/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28656 $ $Date: 2014-09-02 12:18:08 +0800 (Tue, 02 Sep 2014) $
 */

#ifndef _SIP_SIP_H_
#define _SIP_SIP_H_

#include "sip_list.h"
#include "sip_cfg.h"
#include "sip_port.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_voipnet.h"
#include "sip_sdp_msg.h"
#include "sip_hdrflds.h"

typedef struct sSipMsgBody {
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */
    char         msg[SIP_MAX_TEXT_MSG_SIZE];
} tSipMsgBody;

/* Currently tSipText is only used for pReasonPhrase */
typedef struct sSipText {
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */
    char         msg[SIP_MAX_REASON_PHRASE_STR_LEN];
} tSipText;

typedef struct sSipIntMsg
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    /* start line info */
    tSipMsgType         msgType;
    tSipMethod          method;
    tSipMsgCodes        code;
    tSipText           *pReasonPhrase;
    tUri                requestUri;

    tSipMsgCodes        internalErrorCode;
    vint                isCompactForm;
    
    struct 
    {
        tPres64Bits     DCPresenceMasks;
        tPres64Bits     ECPresenceMasks;
    }x;

    /* These are header fields that are immediately
     * decoded and used internally in the stack.
     * Note, all other header fields are NOT decoded and
     * are stored in a linked list in pHFList.  See Below
     */
    tContentTypeHF      ContentType;
    tCallIdHF           szCallId;
    tContentLengthHF    ContentLength;
    tExpiresHF          Expires;
    tFromHF             From;
    tToHF               To;
    tDLList             RecRouteList;
    tReplacesHF         Replaces;
    tReferToHF          ReferTo;
    tSubStateHF         SubState;
    tEventHF            Event;
    tDLList             AuthorizationList;
    tDLList             ViaList;
    tDLList             RouteList;
    tDLList             ContactList;
    tDLList             ServiceRouteList;
    tCSeq               CSeq;
    tRAck               RAck;
    tRSeqHF             RSeq;
    tSessionExpiresHF   SessionTimer;
    tMinExpiresHF       MinExpires;
    tMinSeHF            MinSE;
    tNwAccess           nwAccess;

    /* this is a pointer to the first element in a list of
     * elements that contains all header fields that are not 
     * used internally in the stack.  Therefore, there is no 
     * reason to decode them
     */
    tHdrFldList        *pHFList;

    /* SDP params */
    uint32              parmPresenceMask;
    tSdpMsg            *pSessDescr;

    /* used for the 'message/sipfrag' bodies -for NOTIFY Requests */
    tSipMsgCodes        sipfragCode;


    tBoundry            boundary;

    /* used to hold a text message payload */
    tSipMsgBody        *pMsgBody;

    uint32              retryAfterPeriod;

}tSipIntMsg;

typedef struct sSipIpcMsg 
{
    tSipIpcMsgType type;
    uint32         id;
    tSipHandle     hTransaction;
    tSipHandle     hOwner;
    tSipIntMsg    *pMsg;
    tSipHandle     hContext;
}tSipIpcMsg;

typedef struct {
    OSAL_NetAddress refreshAddr;
    OSAL_NetSockId  refreshFd;
} tSipKeepalives;

typedef vint (*tpfSipProxy)(vint isRegistration, char *pAor, char **ppFqdn);

typedef vint (*tpfSipDispatcher)(tSipHandle hContext, tSipIpcMsg *msg_ptr);

#include "sip_debug.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_token.h"
#include "sip_sip_syntax.h"
#include "sip_msgcodes.h"
#include "sip_abnfcore.h"

#endif
