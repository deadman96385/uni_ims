/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal.h>

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

/*
 *========_MNS_initOutboundSession()=======
 * Initialize session for outbound call.
 *
 * Return:
 *  MNS_OK  Initialize successfully
 */
vint _MNS_initOutboundSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr)
{
    /* if precondition is enabled then set precondition parameters */
    if (service_ptr->sipConfig.usePrecondition) {
        _MNS_preconditionInit(mns_ptr);
    }

    /*
     * Other session parameters are constructed in sapp.
     * could be moved in mns in the future.
     */

    return (MNS_OK);
}

/*
 *========_MNS_initInboundSession()=======
 * Initialize session for inbound call.
 *
 * Return:
 *  MNS_OK  Initialize successfully
 */
vint _MNS_initInboundSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr)
{

    vint    media;
    tMedia *media_ptr;
    vint   *numMedia_ptr;

    /* set local ip address and port for session */
    if (OSAL_netIsAddrIpv6(&service_ptr->aAddr)) {
        mns_ptr->session.lclAddr.addressType = eNwAddrIPv6;
        OSAL_netIpv6AddrCpy(mns_ptr->session.lclAddr.x.ip.v6,
                service_ptr->aAddr.ipv6);

    }
    else {
        mns_ptr->session.lclAddr.addressType = eNwAddrIPv4;
        mns_ptr->session.lclAddr.x.ip.v4.ul = service_ptr->aAddr.ipv4;
    }
    mns_ptr->session.lclAddr.port = service_ptr->aAddr.port;
    
    /* copy default media to session */
    numMedia_ptr = &mns_ptr->session.numMedia;
    *numMedia_ptr = 0;
    for (media = 0; media < rmtSess_ptr->numMedia; media++) {

        media_ptr = &rmtSess_ptr->media[media];

        switch (media_ptr->mediaType) {
            case (eSdpMediaAudio):
                /* copy audio media from service to session */
                OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
                        &service_ptr->sipConfig.amedia, sizeof(tMedia));
        
                /* set media port and control port */
                mns_ptr->session.media[*numMedia_ptr].lclRtpPort =
                        service_ptr->aAddr.port;
                mns_ptr->session.media[*numMedia_ptr].lclRtcpPort =
                        service_ptr->aRtcpPort;
                
                (*numMedia_ptr)++;
                break;
            case (eSdpMediaVideo):
                /* copy video media from service to session */
                OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
                        &service_ptr->sipConfig.vmedia, sizeof(tMedia));

                /* set media port and control port */
                mns_ptr->session.media[*numMedia_ptr].lclRtpPort =
                        service_ptr->vAddr.port;
                mns_ptr->session.media[*numMedia_ptr].lclRtcpPort =
                        service_ptr->vRtcpPort;
                
                (*numMedia_ptr)++;
                break;
            default:
                /* other media type */
                break;
        }
    }

    /* if precondition is enabled then set precondition parameters */
    if (service_ptr->sipConfig.usePrecondition) {
        _MNS_preconditionInit(mns_ptr);
    }

    return (MNS_OK);
}

/*
 *========_MNS_callHold()=======
 * Return:
 *     None
 */
void _MNS_callHold(
    MNS_SessionObj  *mns_ptr)
{
    /*
     * Nothing to do here for now. The session direction
     * was already set by ISI.
     */
}

/*
 *========_MNS_callResume()=======
 * Return:
 *    None
 */
void _MNS_callResume(
    MNS_SessionObj  *mns_ptr)
{
    /*
     * Nothing to do here for now. The session direction
     * was already set by ISI.
     */
}

/*
 *========_MNS_setSessionPtr()=======
 * Set sess_ptr to &session for sdp offer or answer.
 * This function is to handle SDP offer/answer model for sapp
 * to assign correct session pointer to sip stack.
 *
 * Return:
 *    None
 */
void _MNS_setSessionPtr(
    MNS_SessionObj  *mns_ptr)
{
    /* set sess_ptr to &session */
    mns_ptr->sess_ptr = &mns_ptr->session;
}

/*
 *========_MNS_clearSessionPtr()=======
 * This function is to clear sess_ptr
 * This function is to handle SDP offer/answer model for sapp
 * to assign correct session pointer to sip stack.
 *
 * Return:
 *    None
 */
void _MNS_clearSessionPtr(
    MNS_SessionObj  *mns_ptr)
{
    /* set sess_ptr to NULL */
    mns_ptr->sess_ptr = NULL;
}

/*
 *========= _MNS_setSessionInactive() ==============
 * This function is to set session to inactive.
 *
 */
void _MNS_setSessionInactive(
    MNS_SessionObj *mns_ptr)
{
    mns_ptr->session.lclDirection = eSdpAttrInactive;
}

/*
 *========= _MNS_setSessionActive() ==============
 * This function is to set session to active.
 *
 */
void _MNS_setSessionActive(
    MNS_SessionObj *mns_ptr)
{
    mns_ptr->session.lclDirection = eSdpAttrSendRecv;
}

/*
 *========= _MNS_updateDirection() ==============
 * This function is to update session and media direction to set active if
 * precondition changed and met.
 *
 */
void _MNS_updateDirection(
    MNS_SessionObj *mns_ptr)
{
    tSession *sess_ptr;
    int       idx;

    sess_ptr = &mns_ptr->session;
    if (mns_ptr->isPreconditionChanged && mns_ptr->isPreconditionMet) {
        /* Set session. */
        sess_ptr->lclDirection = eSdpAttrSendRecv;
        /* Set media */
        for (idx = 0; idx < sess_ptr->numMedia; idx++) {
            sess_ptr->media[idx].lclDirection = eSdpAttrSendRecv;
        }
        
    }
}
    
/*
 *========= _MNS_loadDefaultSession() ==============
 * This function is to load default session to session object.
 *
 */
vint _MNS_loadDefaultSession(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr)
{
    vint   *numMedia_ptr;

    MNS_dbgPrintf("%s:%d\n",  __FUNCTION__, __LINE__);

    /* Set local ip address and port for session */
    if (OSAL_netIsAddrIpv6(&service_ptr->aAddr)) {
        mns_ptr->session.lclAddr.addressType = eNwAddrIPv6;
        OSAL_netIpv6AddrCpy(mns_ptr->session.lclAddr.x.ip.v6,
        service_ptr->aAddr.ipv6);

    }
    else {
        mns_ptr->session.lclAddr.addressType = eNwAddrIPv4;
        mns_ptr->session.lclAddr.x.ip.v4.ul = service_ptr->aAddr.ipv4;
    }
    mns_ptr->session.lclAddr.port = service_ptr->aAddr.port;

    /* Copy default media to session */
    numMedia_ptr = &mns_ptr->session.numMedia;
    *numMedia_ptr = 0;

    /* Copy audio media from service to session */
    OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
            &service_ptr->sipConfig.amedia, sizeof(tMedia));

    /* Set media port and control port */
    mns_ptr->session.media[*numMedia_ptr].lclRtpPort = service_ptr->aAddr.port;
    mns_ptr->session.media[*numMedia_ptr].lclRtcpPort = service_ptr->aRtcpPort;

    (*numMedia_ptr)++;

    /* Copy video media from service to session */
    OSAL_memCpy(&mns_ptr->session.media[*numMedia_ptr],
            &service_ptr->sipConfig.vmedia, sizeof(tMedia));

    /* Set media port and control port */
    mns_ptr->session.media[*numMedia_ptr].lclRtpPort = service_ptr->vAddr.port;
    mns_ptr->session.media[*numMedia_ptr].lclRtcpPort = service_ptr->vRtcpPort;

    (*numMedia_ptr)++;

    /*
     * Currently only audio and video media, may need add more media
     * in the future.
     */

    /* if precondition is enabled then set precondition parameters */
    if (service_ptr->sipConfig.usePrecondition) {
        _MNS_preconditionInit(mns_ptr);
    }

    return (MNS_OK);
}

