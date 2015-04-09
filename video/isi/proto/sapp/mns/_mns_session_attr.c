
/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include "isi.h"
#include "isip.h"
#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>

#include "mns.h"
#include "_mns_neg.h"
#include "_mns_session_attr.h"
#include "_mns_precondition.h"

/*
 * ========_MNS_sessionSearchMedia()========
 * Search the media type in session.
 *
 * Return
 *   NULL: Cannot find media.
 *   Otherwise: The pointer of the media.
 */
tMedia* _MNS_sessionSearchMedia(
    tSession        *sess_ptr,
    tSdpMediaType    mediaType,
    vint            *idx_ptr)
{
    vint i;

    for (i = 0; i < sess_ptr->numMedia; i++) {
        if (mediaType == sess_ptr->media[i].mediaType) {
            *idx_ptr = i;
            return (&sess_ptr->media[i]);
        }
    }

    return (NULL);
} 

/*
 * ========_MNS_sessionSetMediaAttr()========
 * Set media related attributes from rmtSess_ptr to lclSess_ptr and
 * negotiate coder, security and direction based on rmtSess_ptr and
 * lclSess_ptr.
 *
 * Return
 *   MNS_OK: Successful negotiate media attributes.
 *   MNS_ERR: Failure on media attributes negotiation.
 *
 */
vint _MNS_sessionSetMediaAttr(
    MNS_SessionObj  *mns_ptr,
    tSession        *lclSess_ptr,
    tSession        *rmtSess_ptr,
    MNS_NegType      negType)
{
    int           media;
    tMedia       *rmtMedia_ptr;
    tMedia       *lclMedia_ptr;
    OSAL_Boolean  prevMetState;
    tSdpMediaType mediaType;
    uint32        whichMedias; /* Indicate which media is negotiated. */
    vint          negMediaIdx;
    vint          mediaIdx;

    prevMetState = mns_ptr->isPreconditionMet;
    mns_ptr->isPreconditionChanged = OSAL_FALSE;
    mns_ptr->isPreconditionMet = OSAL_TRUE;
    mns_ptr->usePrecondition = OSAL_FALSE;

    whichMedias = 0;

    for (media = 0; media < rmtSess_ptr->numMedia; media++) {

        rmtMedia_ptr = &rmtSess_ptr->media[media];
        mediaType    = rmtMedia_ptr->mediaType;
        lclMedia_ptr = _MNS_sessionSearchMedia(lclSess_ptr, mediaType, &mediaIdx);

        if (NULL == lclMedia_ptr) {
            /* Cannot find media in local */
            if (eSdpMediaAudio == mediaType) {
                MNS_dbgPrintf("%s %d: No audio media in local session\n",
                        __FUNCTION__, __LINE__);
                return (MNS_ERR);
            }
            continue;
        }

        /* if media is disable, no need to negotiate */
        if (0 == rmtMedia_ptr->rmtRtpPort) {
            lclMedia_ptr->lclRtpPort  = 0;
            lclMedia_ptr->lclRtcpPort = 0;
            lclMedia_ptr->rmtRtpPort  = 0;
            lclMedia_ptr->rmtRtcpPort = 0;
            lclMedia_ptr->precondition.numStatus = 0;

            /* The media matches, remember the index from local session */
            whichMedias |= (1 << mediaIdx);
            continue;
        }

        /* Negotiate coder */
        if (MNS_OK != _MNS_negCoders(rmtMedia_ptr, lclMedia_ptr, negType)) {
            /*
             * If no video coder is available, do not return error but disable
             * video by set port to zero.
             */
            if (eSdpMediaVideo == mediaType) {
                lclMedia_ptr->numCoders = 0;
                lclMedia_ptr->lclRtpPort = 0;
                lclMedia_ptr->lclRtcpPort = 0;
                lclMedia_ptr->precondition.numStatus = 0;
                /* The media matches, remember the index from local session */
                whichMedias |= (1 << mediaIdx);
                continue;
            }
            return (MNS_ERR);
        }

        /* build security attributes */
        if (MNS_OK != _MNS_negSecurity(rmtMedia_ptr, lclMedia_ptr)) {
            return (MNS_ERR);
        }

        /* build rtcp feedback attributes */
        if (MNS_OK != _MNS_negRtcpFb(rmtMedia_ptr, lclMedia_ptr)) {
            return (MNS_ERR);
        }

        lclMedia_ptr->rmtRtpPort = rmtMedia_ptr->rmtRtpPort;
        lclMedia_ptr->rmtRtcpPort = rmtMedia_ptr->rmtRtcpPort;
        lclMedia_ptr->rmtDirection = rmtMedia_ptr->rmtDirection;
        lclMedia_ptr->framerate = rmtMedia_ptr->framerate;
        if (rmtMedia_ptr->rmtAsBwKbps > 0) {
            lclMedia_ptr->rmtAsBwKbps = rmtMedia_ptr->rmtAsBwKbps;
        }

        /* if remote doesn't include rtcp port, assume as rtp port + 1 */
        if (0 == rmtMedia_ptr->rmtRtcpPort) {
            lclMedia_ptr->rmtRtcpPort = lclMedia_ptr->rmtRtpPort + 1;
        }

        /* Set the direction based on what the remote end said */
        _MNS_negDirection(lclMedia_ptr->rmtDirection,
                &lclMedia_ptr->lclDirection);

        /*
         * Remote may not have media-level direction(default is sendrecv)
         * but session-level direction. So overwrite media-level direction
         * if it's sendrecv.
         */
        if (eSdpAttrSendRecv == lclMedia_ptr->lclDirection) {
            lclMedia_ptr->lclDirection = mns_ptr->session.lclDirection;
        }

        /* Update precondition table */
        if (MNS_OK == _MNS_preconditionUpdate(
                mns_ptr, &lclMedia_ptr->precondition,
                &rmtMedia_ptr->precondition)) {
            /* set usePrecondition if there is any media use precondition */
            mns_ptr->usePrecondition = OSAL_TRUE;

            /*
             * Seesion's precondition is met when all media's preconditions
             * are met
             */
            mns_ptr->isPreconditionMet &= lclMedia_ptr->precondition.isMet;
        }

        /* The media matches, remember the index from local session */
        whichMedias |= (1 << mediaIdx);
    }

    /*
     * 'whichMedias now contains a bitmap of all indexes into the local session
     * medias that both parties support.  So copy the valid ones
     * from 'local session to itself.
     */
    negMediaIdx = 0;
    mediaIdx = 0;
    while (whichMedias) {
        if (whichMedias & 0x00000001) {
            /* No need to copy if it's same location. */
            if (negMediaIdx != mediaIdx) {
                lclSess_ptr->media[negMediaIdx] = lclSess_ptr->media[mediaIdx];
            }
            negMediaIdx++;
        }
        mediaIdx++;
        whichMedias >>= 1;
        if (MAX_SESSION_MEDIA_STREAMS <= mediaIdx) {
            /* Break if it exceeds maximum media. */
            break;
        }
    }
    /* Check media number, return fail if 0 */
    if (0 == negMediaIdx) {
        return (MNS_ERR);
    }
    /* Set media number. */
    lclSess_ptr->numMedia = negMediaIdx;

    if (prevMetState != mns_ptr->isPreconditionMet) {
        mns_ptr->isPreconditionChanged = OSAL_TRUE;
    }

    return (MNS_OK);

}

/*
 * ========_MNS_sessionSetSessionAttr()========
 * Set session related attributes from rmtSess_ptr to lclSess_ptr.
 *
 * Return
 *   MNS_OK
 */
vint _MNS_sessionSetSessionAttr(
    tSession        *lclSess_ptr,
    tSession        *rmtSess_ptr)
{

    /* Cache the remote RTP interface info */
    lclSess_ptr->rmtAddr.addressType = rmtSess_ptr->rmtAddr.addressType;
    lclSess_ptr->rmtAddr.x.ip.v4.ul = rmtSess_ptr->rmtAddr.x.ip.v4.ul;
    OSAL_netIpv6AddrCpy(lclSess_ptr->rmtAddr.x.ip.v6,
            rmtSess_ptr->rmtAddr.x.ip.v6);

    lclSess_ptr->rmtDirection = rmtSess_ptr->rmtDirection;

    /* Set the direction based on what the remote end said */
    _MNS_negDirection(lclSess_ptr->rmtDirection, &lclSess_ptr->lclDirection);

    return (MNS_OK);
}

