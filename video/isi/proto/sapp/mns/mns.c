/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"
#include "_mns.h"
#include "_mns_precondition.h"

#include "../_sapp.h"
#include "_mns_neg.h"

#ifdef MNS_DEBUG
static char *_MNS_StateStrings[MNS_STATE_LAST + 1] = {
    "MNS_STATE_IDLE",
    "MNS_STATE_EARLY_OFFER_SENT",
    "MNS_STATE_EARLY_OFFER_RECEIVED",
    "MNS_STATE_EARLY",
    "MNS_STATE_ACTIVE",
    "MNS_STATE_ACTIVE_OFFER_SENT",
    "MNS_STATE_ACTIVE_OFFER_RECEIVED",
    "MNS_STATE_LAST",
};
#endif

/**
 * @brief Initializes MNS Service Object.
 *
 * @param mnsService_ptr Pointer to MNS_ServiceObj to be initialized.
 *
 * @return MNS_OK object initialized.
 * @return MNS_ERR Failure initializing a resource.
 */
vint MNS_initService(
    MNS_ServiceObj *mnsService_ptr,
    vint            usePreconditon)
{
    MNS_dbgPrintf("%s:%d mnsService_ptr:%p\n", __FUNCTION__, __LINE__,
            mnsService_ptr);

    if (NULL == mnsService_ptr) {
        MNS_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
        return MNS_ERR;
    }
    OSAL_memSet(mnsService_ptr, 0, sizeof(MNS_ServiceObj));

    mnsService_ptr->sipConfig.usePrecondition = usePreconditon;
    return MNS_OK;
}

/*=========MNS_clearSession()============
 * Clear session
 *
 * Return:
 *  None
 */
void MNS_clearSession(
    MNS_SessionObj  *mns_ptr)
{
    MNS_dbgPrintf("%s:%d mns_ptr:%p\n", __FUNCTION__, __LINE__,
            mns_ptr);

    /* reset state */
    mns_ptr->state              = MNS_STATE_IDLE;
    mns_ptr->sess_ptr           = NULL;
    mns_ptr->usePrecondition    = OSAL_FALSE;
    mns_ptr->isPreconditionMet  = OSAL_FALSE;

    /* clear session object*/
    OSAL_memSet(&mns_ptr->session, 0, sizeof(tSession));
}

/*
 * ======== MNS_loadDefaultAudio========
 * 
 * Load default audio media from MNS service to MNS session
 *
 * Returns:
 *   Nothing.
 */
void MNS_loadDefaultAudio(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr)
{
    vint   *numMedia_ptr;

    MNS_dbgPrintf("%s:%d\n",  __FUNCTION__, __LINE__);

    numMedia_ptr = &mns_ptr->session.numMedia;

    /* Copy video media from service to session */
    OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
            &service_ptr->sipConfig.amedia, sizeof(tMedia));

    /* Set media port and control port */
    mns_ptr->session.media[*numMedia_ptr].lclRtpPort = service_ptr->aAddr.port;
    mns_ptr->session.media[*numMedia_ptr].lclRtcpPort = service_ptr->aRtcpPort;

    (*numMedia_ptr)++; 
}

/*
 * ======== MNS_loadDefaultVideo ========
 * 
 * Load default video media from MNS service to MNS session
 *
 * Returns:
 *   Nothing.
 */
void MNS_loadDefaultVideo(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr)
{
    vint   *numMedia_ptr;

    MNS_dbgPrintf("%s:%d\n",  __FUNCTION__, __LINE__);

    numMedia_ptr = &mns_ptr->session.numMedia;

    /* Copy video media from service to session */
    OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
            &service_ptr->sipConfig.vmedia, sizeof(tMedia));

    /* Set media port and control port */
    mns_ptr->session.media[*numMedia_ptr].lclRtpPort = service_ptr->vAddr.port;
    mns_ptr->session.media[*numMedia_ptr].lclRtcpPort = service_ptr->vRtcpPort;

    (*numMedia_ptr)++; 
}

/*
 * ========= MNS_isOfferReceived()===========
 * Check if SDP offer is received and waiting for sending SDP answer.
 *
 * Return:
 *  OSAL_TRUE: If SDP offer is received.
 *  OSAL_FALSE: Otherwise
 */
OSAL_Boolean MNS_isOfferReceived(
    MNS_SessionObj  *mns_ptr)
{
    if ((MNS_STATE_EARLY_OFFER_RECEIVED == mns_ptr->state) ||
            (MNS_STATE_ACTIVE_OFFER_RECEIVED == mns_ptr->state)) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*
 * ========= MNS_isSessionActive()===========
 * If current state is ACTIVE.
 *
 * Return:
 *  OSAL_TRUE: The session is in acitve state.
 *  OSAL_FALSE: The session is not in active state.
 */
OSAL_Boolean MNS_isSessionActive(
    MNS_SessionObj  *mns_ptr)
{
    return ((MNS_STATE_ACTIVE == mns_ptr->state)? OSAL_TRUE : OSAL_FALSE);
}

/*========= MNS_isSessionEarly()===========
 * Function to check if a session is in early state.
 *
 * Return:
 *  OSAL_TRUE: The session is in early state.
 *  OSAL_FALSE: The session is not in early state.
 */
OSAL_Boolean MNS_isSessionEarly(
    MNS_SessionObj  *mns_ptr)
{
    if ((MNS_STATE_EARLY == mns_ptr->state) ||
        (MNS_STATE_EARLY_OFFER_SENT == mns_ptr->state) ||
        (MNS_STATE_EARLY_OFFER_RECEIVED == mns_ptr->state)) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*========= MNS_isSessionEarlyIdle()===========
 * Function to check if a session is in early state and
 * SDP is negotiated.
 *
 * Return:
 *  OSAL_TRUE: The session is in early state.
 *  OSAL_FALSE: The session is not in early state.
 */
OSAL_Boolean MNS_isSessionEarlyIdle(
    MNS_SessionObj  *mns_ptr)
{
    return (MNS_STATE_EARLY == mns_ptr->state);
}

/*========= MNS_isPreconditiopnUsed()===========
 * If current precondition is used
 *
 * Return:
 *  OSAL_TRUE: Precondition is used.
 *  OSAL_FALSE: Precondition is not used.
 */
OSAL_Boolean MNS_isPreconditionUsed(
    MNS_SessionObj  *mns_ptr)
{
    return (mns_ptr->usePrecondition);
}

/*========= MNS_isPreconditiopnMet()===========
 * If current precondition is met.
 *
 * Return:
 *  OSAL_TRUE: Precondition is met or precondition is not used.
 *  OSAL_FALSE: Precondition is not met.
 */
OSAL_Boolean MNS_isPreconditionMet(
    MNS_SessionObj  *mns_ptr)
{
    return (mns_ptr->isPreconditionMet);
}

/*
 *============MNS_ip2ipUpdate()============
 * Update ip address
 *
 */
vint MNS_ip2ipUpdate(
    MNS_SessionObj  *mns_ptr,
    OSAL_NetAddress *addr_ptr)
{
    MNS_dbgPrintf("%s:%d mns_ptr:%p\n", __FUNCTION__, __LINE__,
            mns_ptr);

    /* update ip address */
    OSAL_netIpv6AddrCpy(mns_ptr->session.lclAddr.x.ip.v6, addr_ptr->ipv6);
    mns_ptr->session.lclAddr.x.ip.v4.ul = addr_ptr->ipv4;

    return (MNS_OK);
}

/**
 * @brief Negotiates coders and validates consistency and correctness of
 * incoming sdp for offer/answer events, and resets to idle upon terminate
 * event.
 *
 * @param mns_ptr Pointer to MNS handle.
 * @param session_ptr Pointer to session given by UA event. Coders will be
 * negotiated, modifying the session in-place to reflect the result.
 *
 * @return MNS_OK   Valid negotiation successful.
 * @return MNS_ERR  Invalid/inconsistent session.
 */
vint MNS_processEvent(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr,
    tUaEvtType      event,
    tUaAppEvent    *uaEvt_ptr,
    ISIP_Message   *isi_ptr)
{
    vint retVal;
    retVal = MNS_ERR;

    MNS_dbgPrintf("%s:%d mns_ptr:%p state:%s event:%d\n",
            __FUNCTION__, __LINE__,
            mns_ptr, _MNS_StateStrings[mns_ptr->state], uaEvt_ptr->header.type);

    if (NULL == mns_ptr || NULL == uaEvt_ptr) {
        MNS_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
        return (MNS_ERR);
    }

    /* 
     * Filter No SDP cases. Below case without SDP is valid.
     *  Case 1) 200 OK without SDP.
     *  Case 2) Re-Invite failure.
     */
    if (!uaEvt_ptr->sessNew &&
            !((eUA_ANSWERED == event) || 
            ((eUA_MEDIA_INFO == event) && (uaEvt_ptr->resp.respCode >= 300)))) {
        MNS_dbgPrintf("%s:%d Error: No sdp.\n",  __FUNCTION__, __LINE__);
        return (MNS_ERR);
    }

    _MNS_clearSessionPtr(mns_ptr);

    /* Here comes a SDP from remote */
    switch (mns_ptr->state) {
        case MNS_STATE_IDLE:
            /* It's a new inboud call */

            /* Offer received, set next state */
            mns_ptr->state = MNS_STATE_EARLY_OFFER_RECEIVED;

            /* init inboud session bases on remote session attributes */
            if (MNS_OK != (retVal = _MNS_initInboundSession(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                MNS_dbgPrintf("%s:%d Init inbound session failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }

            /* negotiate the offer */
            if (MNS_OK != (retVal = _MNS_negOffer(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                /* negotiation failure */
                MNS_dbgPrintf("%s:%d SDP offer negitiation failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }
            
            break;
        case MNS_STATE_EARLY_OFFER_SENT:
            /* It's a resp of a outbound call */

            if (MNS_OK != (retVal = _MNS_negAnswer(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                /* negotiation failure */
                MNS_dbgPrintf("%s:%d SDP answer negotiation failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }

            /* set next state based on precondition */
            if (mns_ptr->isPreconditionMet && (eUA_ANSWERED == event)) {
                /* Remote answer the call, goes to ACTIVE state */
                mns_ptr->state = MNS_STATE_ACTIVE;
            }
            else {
                /* Got sdp answer but still in early state */
                mns_ptr->state = MNS_STATE_EARLY;
            }

            break;
        case MNS_STATE_EARLY_OFFER_RECEIVED:
            /*
             * Receivd SDP offer in MNS_STATE_EARLY_OFFER_RECEIVED state.
             * Should be be here.
             */
            MNS_dbgPrintf("%s:%d Receivd SDP offer in "
                    "MNS_STATE_EARLY_OFFER_RECEIVED state\n",
                     __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            /* Keep in the same state */
            break;
        case MNS_STATE_EARLY:
            /* Update from remote */
            if (eUA_ANSWERED == event) {
                /*
                 * It's 200 OK and we already got SDP answer, however
                 * for the case that server doesn't follow RFC and update the
                 * sdp answer in 200 OK, it's better to re-negotiate sdp answer
                 * in 200 OK again.
                 */
                if (!uaEvt_ptr->sessNew) {
                    /* Negotiate SDP answer again. */
                    MNS_dbgPrintf("%s:%d Received 200 OK with SDP when SDP is "
                            "negitiated\n", __FUNCTION__, __LINE__);
                    if (MNS_OK != (retVal = _MNS_negAnswer(service_ptr, mns_ptr,
                            &uaEvt_ptr->msgBody.session))) {
                        /* negotiation failure */
                        MNS_dbgPrintf("%s:%d SDP answer negotiation failed\n",
                                __FUNCTION__, __LINE__);
                        break;
                    }
                }
                mns_ptr->state = MNS_STATE_ACTIVE;
                retVal = MNS_OK;
                break;
            }

            /* set next state */
            mns_ptr->state = MNS_STATE_EARLY_OFFER_RECEIVED;

            /* negotiate the offer */
            if (MNS_OK != (retVal = _MNS_negOffer(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                /* negotiation failure */
                MNS_dbgPrintf("%s:%d SDP offer negitiation failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }
            break;
        case MNS_STATE_ACTIVE:
            /* Re-invite */
            mns_ptr->state = MNS_STATE_ACTIVE_OFFER_RECEIVED;

            /* Store previous session */
            service_ptr->sessStore = *mns_ptr;

            /* 
             * Treat re-INVITE as new inbound session.
             * Init inboud session bases on remote session attributes
             */
            if (MNS_OK != (retVal = _MNS_initInboundSession(service_ptr,
                    mns_ptr, &uaEvt_ptr->msgBody.session))) {
                MNS_dbgPrintf("%s:%d Init inbound session failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }

            /* negotiate the offer */
            if (MNS_OK != (retVal = _MNS_negOffer(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                /* negotiation failure */
                MNS_dbgPrintf("%s:%d SDP offer negitiation failed\n",
                        __FUNCTION__, __LINE__);
                /* Re-store session */
                *mns_ptr = service_ptr->sessStore;
                break;
            }

            break;
        case MNS_STATE_ACTIVE_OFFER_SENT:
            /*
             * 1: Response to Re-invite
             * 2: If respCode is 0 and event is eUA_MEDIA_INFO,
             *    receive re-INVITE and we should change state to ACTIVE.
             *    This is 491 request pending case.
             */
            if ((eUA_MEDIA_INFO == event) &&
                    ((uaEvt_ptr->resp.respCode >= 300) ||
                    (0 == uaEvt_ptr->resp.respCode))) {
                /* Re-Invite failure. */
                mns_ptr->state = MNS_STATE_ACTIVE;
                MNS_dbgPrintf("%s:%d Re-Invite failure. Back to ACTIVE state.\n",
                        __FUNCTION__, __LINE__);
                retVal = MNS_OK;
                break;
            }

            /* negotiate the answer */
            if (MNS_OK != (retVal = _MNS_negAnswer(service_ptr, mns_ptr,
                    &uaEvt_ptr->msgBody.session))) {
                /* negotiation failure */
                MNS_dbgPrintf("%s:%d SDP answer negotiation failed\n",
                        __FUNCTION__, __LINE__);
                break;
            }

            /* set next state */
            mns_ptr->state = MNS_STATE_ACTIVE;
            break;
        case MNS_STATE_ACTIVE_OFFER_RECEIVED:
            /*
             * Receivd SDP offer in MNS_STATE_ACTIVE_OFFER_RECEIVED state.
             * Should be be here.
             */
            MNS_dbgPrintf("%s:%d Receivd SDP offer in "
                     "MNS_STATE_ACITVE_OFFER_RECEIVED state\n",
                     __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            /* Keep in the same state */
            break;
        default:
            /* should not be here */
            MNS_dbgPrintf("%s:%d Invalid state\n", __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            break;
    }

    MNS_dbgPrintf("%s:%d mns_ptr:%p new state:%s result:%d\n",
            __FUNCTION__, __LINE__,
            mns_ptr, _MNS_StateStrings[mns_ptr->state], retVal);

    return (retVal);
}

/**
 * @brief Negotiates coders and validates consistency and correctness of
 * incoming sdp for offer/answer commands, and resets to idle upon terminate
 * command. Coders will be negotiated, modifying the mns_ptr->session
 * in-place to reflect the result.
 *
 * @param mns_ptr Pointer to MNS Session handle.
 * @param m_ptr Pointer to the ISIP_Message object containing the command.
 *
 * @return MNS_OK Valid negotiation successful.
 * @return MNS_ERR Invalid/inconsistent session.
 */
vint MNS_processCommand(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr,
    ISIP_Message   *cmd_ptr)
{
    vint reason;
    vint retVal;

    reason = -1;

    if (NULL == mns_ptr) {
        MNS_dbgPrintf("%s %d\n",__FUNCTION__, __LINE__);
        return MNS_ERR;
    }

    MNS_dbgPrintf("%s:%d mns_ptr:%p state:%s\n", __FUNCTION__, __LINE__,
            mns_ptr, _MNS_StateStrings[mns_ptr->state]);

    if (NULL != cmd_ptr) {
        reason = cmd_ptr->msg.call.reason;
    }

    /* Here means there is a new media comes from ISI */
    switch (mns_ptr->state) {
        case MNS_STATE_IDLE:
            /* 
             * Prepare SDP offer.
             */
            if (NULL == cmd_ptr) {
                /*
                 * Not a command from ISI.
                 * Should be a response to INVITE w/o SDP.
                 */

                /* Load session with our capabilities. */
                _MNS_loadDefaultSession(service_ptr, mns_ptr);

                _MNS_setSessionInactive(mns_ptr);
            }
            _MNS_initOutboundSession(service_ptr, mns_ptr);

            /* Set session for sdp offer */
            _MNS_setSessionPtr(mns_ptr);
            /* set next state */
            mns_ptr->state = MNS_STATE_EARLY_OFFER_SENT;

            retVal = MNS_OK;
            break;
        case MNS_STATE_EARLY_OFFER_SENT:
            /*
             * Send offer in MNS_STATE_EARLY_OFFER_SENT state.
             * Should be be here.
             */
            MNS_dbgPrintf("%s:%d Send SDP offer in "
                    "MNS_STATE_EARLY_OFFER_SENT state\n",
                     __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            /* Keep in the same state */
            break;
        case MNS_STATE_EARLY_OFFER_RECEIVED:
            /* Set precondition */
            _MNS_preconditionSetLocalRsrcStatus(mns_ptr);
            /* Update direction base on precondition status. */
            _MNS_updateDirection(mns_ptr);

            if (!mns_ptr->isPreconditionMet) {
                /* Precondition not met */
                mns_ptr->state = MNS_STATE_EARLY;
            }
            else if (ISIP_CALL_REASON_ACCEPT == reason ||
                    (ISIP_CALL_REASON_ACCEPT_ACK == reason)) {
                /*
                 * Precondition met and local answers.
                 * SDP answer might be in ACK when the SDP offer is provided
                 * in 200 OK.
                 */
                mns_ptr->state = MNS_STATE_ACTIVE;
            }
            else {
                /* 
                 * Precondition met and local update the call before
                 * answers.
                 */
                mns_ptr->state = MNS_STATE_EARLY;
            }

            /* Set session for sdp answer */
            _MNS_setSessionPtr(mns_ptr);

            retVal = MNS_OK;
            break;
        case MNS_STATE_EARLY:
            if (ISIP_CALL_REASON_MODIFY == reason) {
                /* Set precondition */
                _MNS_preconditionSetLocalRsrcStatus(mns_ptr);
                /* Update direction base on precondition status. */
                _MNS_updateDirection(mns_ptr);
                /* Set session for sdp offer */
                _MNS_setSessionPtr(mns_ptr);
                /* set next state */
                mns_ptr->state = MNS_STATE_EARLY_OFFER_SENT;
            }
            else if (ISIP_CALL_REASON_ACCEPT == reason) {
                /*
                 * Accept the call and sdp had already negotiated,
                 * no need to populate SDP in 200 OK.
                 */
                mns_ptr->sess_ptr = NULL;
                mns_ptr->state = MNS_STATE_ACTIVE;
            }
            retVal = MNS_OK;
            break;
        case MNS_STATE_ACTIVE:
            /* media update */
            if (ISIP_CALL_REASON_HOLD == reason) {
                /* Call hold */
                _MNS_callHold(mns_ptr);
            }
            else if (ISIP_CALL_REASON_RESUME == reason) {
                /*
                 * Call resume.
                 * Set direction of session and media
                 */
                _MNS_callResume(mns_ptr);
            }
            else if (ISIP_CALL_REASON_MODIFY == reason) {
                /* Re-INVITE, do nothing here */
            }
            else if (ISIP_CALL_REASON_ACCEPT == reason) {
                /* Accept call in active state, need to set session to active */
                _MNS_setSessionActive(mns_ptr);
            }
            else {
                /*
                 * Not a command from ISI
                 * Should be a response to Re-INVITE w/o SDP
                 */

                /* Load session with our capabilities. */
                _MNS_loadDefaultSession(service_ptr, mns_ptr);
            }

            /* Set session for sdp answer */
            _MNS_setSessionPtr(mns_ptr);

            mns_ptr->state = MNS_STATE_ACTIVE_OFFER_SENT;

            retVal = MNS_OK;
            break;
        case MNS_STATE_ACTIVE_OFFER_SENT:
           /*
            * Send offer in MNS_STATE_ACTIVE_OFFER_SENT state.
            * Should be be here.
            */
            MNS_dbgPrintf("%s:%d Send SDP offer in "
                    "MNS_STATE_ACITVE_OFFER_SENT state\n",
                    __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            /* Keep in the same state */
            break;
        case MNS_STATE_ACTIVE_OFFER_RECEIVED:
            /* Accept the call */

            /* Set session for sdp answer */
            _MNS_setSessionPtr(mns_ptr);
            mns_ptr->state = MNS_STATE_ACTIVE;

            retVal = MNS_OK;
            break;
        default:
            /* should not be here */
            MNS_dbgPrintf("%s:%d Invalid state\n", __FUNCTION__, __LINE__);

            retVal = MNS_ERR;
            break;
    }

    MNS_dbgPrintf("%s:%d mns_ptr:%p new state:%s result:%d\n",
            __FUNCTION__, __LINE__,
            mns_ptr, _MNS_StateStrings[mns_ptr->state], retVal);

    return (retVal);
}

