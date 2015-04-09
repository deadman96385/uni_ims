/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
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

#include "_mns_precondition.h"

/*
 * ======== _MNS_preconditionGetDirByRsrcStatus()========
 * Get local resource reservation status
 *
 * Return:
 *  ePrecDirNone: No resource is reserved.
 *  ePrecDirSend: Send direction resource is reserved.
 *  ePrecDirRecv: Recv direction resource is reserved.
 *  ePrecDirSendRecv: Send and Recv direction resource is reserved.
 */
tPrecDir _MNS_preconditionGetDirByRsrcStatus(
    MNS_SessionObj *mns_ptr)
{
    /* Determine resource status by local resource ready */
    if (ISI_RESOURCE_STATUS_LOCAL_READY & mns_ptr->rsrcStatus) {
        /* Return sendrecv if local resource is ready */
        return (ePrecDirSendRecv);
    }
    /* Otherwise return ePrecDirNone */
    return (ePrecDirNone);
}

/*
 * ======== _MNS_preconditionFindStatus()========
 * Find precondition status by status and status type
 *
 * Return:
 *  -1          No such entry in precondition object
 *  otherwise   Index of status
 */
vint _MNS_preconditionFindStatus(
    tPrecondition   *p_ptr,
    tSdpAttrType     status,
    tPrecStatusType  statusType)
{
    vint mediaIdx;

    for (mediaIdx = 0; mediaIdx < p_ptr->numStatus; mediaIdx++) {
        if ((status == (tSdpAttrType)p_ptr->status[mediaIdx].status) &&
                (statusType == p_ptr->status[mediaIdx].statusType)) {
            return (mediaIdx);
        }
    }

    return (-1);
}

/*
 * ======== _MNS_preconditionSetLocalRsrcStatus()========
 * Update remote precondition to resource status.
 *
 * Return:
 *  None.
 */
void _MNS_preconditionSetLocalRsrcStatus(
    MNS_SessionObj *mns_ptr)
{
    tSession      *sess_ptr;
    tMedia        *media_ptr;
    tPrecondition *lclPrec_ptr;
    vint           media;
    vint           lclStatusIdx; 
    OSAL_Boolean   prevMetState;
    
    /* No need to set status if precondition is not used */
    if (!mns_ptr->usePrecondition) {
        return;
    }

    sess_ptr = &mns_ptr->session;
    /* Reset and update it below bases on each media. */
    prevMetState = mns_ptr->isPreconditionMet;
    mns_ptr->isPreconditionMet = OSAL_TRUE; 
    mns_ptr->isPreconditionChanged = OSAL_FALSE;

    for (media = 0; media < sess_ptr->numMedia; media++) {
        media_ptr = &sess_ptr->media[media];
        lclPrec_ptr = &media_ptr->precondition;
        /* Set failire if resource failure */
        if (ISI_RESOURCE_STATUS_FAILURE & mns_ptr->rsrcStatus) {
            /* Local resource failure, set strength to failure */
            if (-1 != (lclStatusIdx = _MNS_preconditionFindStatus(lclPrec_ptr,
                    eSdpAttrDes, ePrecStatusTypeLocal))) {
                /* Got status, set strength to failure */
                lclPrec_ptr->status[lclStatusIdx].strength =
                        ePrecStrengthFailure;
            }
            /* If failure then no need to set dir */
            continue;
        }
        /* Set local dir */
        if (-1 != (lclStatusIdx = _MNS_preconditionFindStatus(lclPrec_ptr,
                eSdpAttrCurr, ePrecStatusTypeLocal))) {
            /* Got status, set dir */
            lclPrec_ptr->status[lclStatusIdx].dir =
                    _MNS_preconditionGetDirByRsrcStatus(mns_ptr);
        }

        /* Update the precondition met result */
        _MNS_preconditionUpdateMet(lclPrec_ptr);
        /* Update precondion for the session. */
        mns_ptr->isPreconditionMet &= lclPrec_ptr->isMet;
    }

    if (prevMetState != mns_ptr->isPreconditionMet) {
        mns_ptr->isPreconditionChanged = OSAL_TRUE;
    }
    return;
}

/*
 * ========_MNS_preconditionUpdateRsrcStatus()========
 * Update remote precondition to resource status.
 *
 * Return:
 *  None.
 */
void _MNS_preconditionUpdateRsrcStatus(
    MNS_SessionObj *mns_ptr,
    tPrecondition   *p_ptr)
{
    /* Update local */
    if (_MNS_preconditionIsMet(p_ptr, ePrecStatusTypeLocal)) {
        mns_ptr->rsrcStatus |= ISI_RESOURCE_STATUS_LOCAL_READY;
    }

    /* Update remote */
    if (_MNS_preconditionIsMet(p_ptr, ePrecStatusTypeRemote)) {
        mns_ptr->rsrcStatus |= ISI_RESOURCE_STATUS_REMOTE_READY;
    }

    return;
}

/*
 * ========_MNS_preconditionIsDirMet()========
 * Check if current precondition direction is met desired condition direction
 *  
 * Return:
 *  OSAL_TRUE   Met
 *  OSAL_FALSE  Not met
 */ 
vint _MNS_preconditionIsDirMet(
    tPrecDir currDir,
    tPrecDir desDir)
{
    if ((desDir == currDir) ||
            (ePrecDirSendRecv == currDir) ||
            (ePrecDirNone == desDir)) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*
 * ==========_MNS_preconditionIsMet()========
 * Check if precondition is met or not by status type.
 *
 * Return:
 *  OSAL_TRUE   Met
 *  OSAL_FALSE  Not met
 */
OSAL_Boolean _MNS_preconditionIsMet(
    tPrecondition   *p_ptr,
    tPrecStatusType  statusType)
{
    vint desStatusIdx;
    vint currStatusIdx;

    if (-1 != (desStatusIdx = _MNS_preconditionFindStatus(p_ptr, eSdpAttrDes,
            statusType))) {
        /* if strength is not mandatory then don't care current status */
        if (ePrecStrengthMandatory == p_ptr->status[desStatusIdx].strength) {
            if (-1 != (currStatusIdx = _MNS_preconditionFindStatus(p_ptr,
                    eSdpAttrCurr, statusType))) {
                if (!_MNS_preconditionIsDirMet(p_ptr->status[currStatusIdx].dir,
                        p_ptr->status[desStatusIdx].dir)) {
                    return (OSAL_FALSE);
                }
            }
        }
    }

    return (OSAL_TRUE);
}

/*
 * ==========_MNS_preconditionAddStatus()========
 * Add a status to precondition object
 *
 * Return:
 *  -1          Add failed
 *  otherwise   Index of the new added status
 */
vint _MNS_preconditionAddStatus(
    tPrecondition  *p_ptr,
    tSdpAttrType    status,
    tPrecType       type,
    tPrecStatusType statusType,
    tPrecDir        dir,
    tPrecStrength   strength)
{
    vint numStatus;

    numStatus = p_ptr->numStatus;

    if (SIP_PREC_STATUS_SIZE_MAX <= numStatus) {
        return (-1);
    }

    p_ptr->status[numStatus].status      = status;
    p_ptr->status[numStatus].type        = type;
    /*
     * strength-tag is not required for current status,
     * it will be ignored in sip
     */
    p_ptr->status[numStatus].strength    = strength;
    p_ptr->status[numStatus].statusType  = statusType;
    p_ptr->status[numStatus].dir         = dir;

    return (vint)p_ptr->numStatus++;
}
 
/*
 * ==========_MNS_preconditionDelStatus()========
 * Del a status to precondition object
 *
 * Return:
 *  MNS_OK      Delete successfully
 *  MNS_ERR     No entry to delete
 */
vint _MNS_preconditionDelStatus(
    tPrecondition  *p_ptr,
    tSdpAttrType    status,
    tPrecStatusType statusType)
{
    vint  numStatus;
    vint  statusIdx;
    vint  i;

    numStatus = p_ptr->numStatus;

    if (-1 != (statusIdx = _MNS_preconditionFindStatus(p_ptr, status,
            statusType))) {
        /* Move rest ahead by 1 element */
        for (i = statusIdx; i < numStatus - 1; i++) {
            p_ptr->status[i] = p_ptr->status[i + 1];
        }
        p_ptr->numStatus--;
        return (MNS_OK);
    }

    return (MNS_ERR);
}

/*
 * ========_MNS_preconditionMapStatusType()========
 * Map local status type to remote 
 *
 * Return:
 *  Mapped status type
 */
tPrecStatusType _MNS_preconditionMapStatusType(
    tPrecStatusType statusType)
{
    switch (statusType) {
        case (ePrecStatusTypeLocal):
            return (ePrecStatusTypeRemote);
        case (ePrecStatusTypeRemote):
            return (ePrecStatusTypeLocal);
        default:
            break;
    }
    return (ePrecStatusTypeE2e);
}

/*
 * ========_MNS_preconditionMapDir()========
 * Map local direction to remote 
 *
 * Return:
 *  Mapped direction
 */
tPrecDir _MNS_preconditionMapDir(
    tPrecDir dir)
{
    switch (dir) {
        case (ePrecDirSend):
            return (ePrecDirRecv);
        case (ePrecDirRecv):
            return (ePrecDirSend);
        default:
            break;
    }

    /* here for sendrecv or none */
    return dir;
}

/*
 * ========_MNS_preconditionUpdateStrength()========
 * Update local strength bases on local and remote strength
 *
 * Return:
 *  None
 */
void _MNS_preconditionUpdateStrength(
    tPrecStrength  *lclStrength_ptr,
    tPrecStrength  *rmtStrength_ptr)
{
    /* upgarade the local strength */
    if (*lclStrength_ptr > *rmtStrength_ptr) {
        *lclStrength_ptr = *rmtStrength_ptr;
    }
}


/*
 * ========_MNS_preconditionUpdateDir()========
 * Update local dir from remote dir
 *
 * Return:
 *  None
 */
void _MNS_preconditionUpdateDir(
    tPrecondition  *p_ptr,
    tPrecDir       *lclDir_ptr,
    tPrecDir       *rmtDir_ptr)
{
    vint     lclSend = 0;
    vint     lclRecv = 0;
    vint     rmtSend = 0;
    vint     rmtRecv = 0;
    tPrecDir rmtDir  = _MNS_preconditionMapDir(*rmtDir_ptr);

    /* get send and recv for remote */
    switch (rmtDir) {
        case (ePrecDirSend):
            rmtSend = 1;
            break;
        case (ePrecDirRecv):
            rmtRecv = 1;
            break;
        case (ePrecDirSendRecv):
            rmtSend = 1;
            rmtRecv = 1;
            break;
        default:
            break;
    }

    /* get send and recv for local */
    switch (*lclDir_ptr) {
        case (ePrecDirSend):
            lclSend = 1;
            break;
        case (ePrecDirRecv):
            lclRecv = 1;
            break;
        case (ePrecDirSendRecv):
            lclSend = 1;
            lclRecv = 1;
            break;
        default:
            break;
    }

    /* update send direction */
    if ((0 == rmtSend) && (1 == lclSend)) {
        if (p_ptr->isConfReceived) {
            lclSend = 1;
        }
    }
    else {
        lclSend |= rmtSend;
    }

    /* update recv direction */
    if ((0 == rmtRecv) && (1 == lclRecv)) {
        if (p_ptr->isConfReceived) {
            lclRecv = 1;
        }
    }
    else {
        lclRecv |= rmtRecv;
    }

    /* mapping back to ePrecDir */
    if ((0 == lclSend) && (0 == lclRecv)) {
        *lclDir_ptr = ePrecDirNone;
    }
    else if ((1 == lclSend) && (0 == lclRecv)) {
        *lclDir_ptr = ePrecDirSend;
    }
    else if ((0 == lclSend) && (1 == lclRecv)) {
        *lclDir_ptr = ePrecDirRecv;
    }
    else {
        *lclDir_ptr = ePrecDirSendRecv;
    }

}

/*
 * ========_MNS_preconditionUpdateMet()========
 * Update isPreconditionMet based on current local and remote precondition
 * status
 *
 * Return:
 *  None
 */
void  _MNS_preconditionUpdateMet(
    tPrecondition   *p_ptr)
{
    /* check all status types */
    /* e2e */
    if (!_MNS_preconditionIsMet(p_ptr, ePrecStatusTypeE2e)) {
        p_ptr->isMet = 0;
        return;
    }

    /* local */
    if (!_MNS_preconditionIsMet(p_ptr, ePrecStatusTypeLocal)) {
        p_ptr->isMet = 0;
        return;
    }

    /* remote */
    if (!_MNS_preconditionIsMet(p_ptr, ePrecStatusTypeRemote)) {
        p_ptr->isMet = 0;
        return;
    }

    p_ptr->isMet = 1;
}

/*
 * ========_MNS_preconditionInit()========
 * Initialize precondition parameters
 *
 * Return:
 *  MNS_OK  Initialize successfully
 *  MNS_ERR Initialize failed.
 */
vint _MNS_preconditionInit(
    MNS_SessionObj *mns_ptr)
{
    tPrecondition *lclPrec_ptr;
    vint media;

    /* init precondition for all media */
    for (media = 0; media < mns_ptr->session.numMedia; media++) {
        /* Don't put precondition for unused media */
        if (0 == mns_ptr->session.media[media].lclRtpPort) {
            continue;
        }
        lclPrec_ptr = &mns_ptr->session.media[media].precondition;
        lclPrec_ptr->numStatus = 0;
        lclPrec_ptr->isMet = 0;
        lclPrec_ptr->isConfReceived = 0;
        /*
         * Set current status for local.
         * From TS 24.229, segmented type should be used.
         */
        _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrCurr, ePrecTypeQos,
                ePrecStatusTypeLocal,
                _MNS_preconditionGetDirByRsrcStatus(mns_ptr),
                ePrecStrengthMandatory); 

        /* set current status for remote */
        _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrCurr, ePrecTypeQos,
                ePrecStatusTypeRemote, ePrecDirNone, ePrecStrengthMandatory);

        /* 
         * Set desired status for local.
         * From TS 24.229 local strength-tag should set to Mandatory.
         */
        _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrDes, ePrecTypeQos,
                ePrecStatusTypeLocal, ePrecDirSendRecv,
                ePrecStrengthMandatory); 

        /*
         * Set desired status for remote.
         * From TS 24.229 remote strength-tag should set to Optional.
         */
        _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrDes, ePrecTypeQos,
                ePrecStatusTypeRemote, ePrecDirSendRecv, ePrecStrengthOptional);

        /*
         * TS 24.229: If local resource is not reserved yet, set media to
         * inactive unless we know precondition is supported by remote UE.
         * Since currenyly we are not using resource reservation,
         * it should never set to inactive.
         */
        if (!_MNS_preconditionIsMet(lclPrec_ptr, ePrecStatusTypeLocal)) {
            mns_ptr->session.media[media].lclDirection = eSdpAttrInactive;
        }
        else if (!_MNS_preconditionIsMet(lclPrec_ptr, ePrecStatusTypeRemote)) {
            /*
             * Local precondition met but remote noy. Need confirm from remote
             */
            _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrConf, ePrecTypeQos,
                    ePrecStatusTypeRemote, ePrecDirSendRecv, 
                    ePrecStrengthMandatory);
        }
    }

    /* set precondition use and not met */
    mns_ptr->usePrecondition = OSAL_TRUE;
    mns_ptr->isPreconditionMet = OSAL_FALSE;
    /* Set local resource not ready */
    mns_ptr->rsrcStatus = ISI_RESOURCE_STATUS_NOT_READY;

    return (MNS_OK);
}

/*
 * ========_MNS_preconditionUpdateStatus()========
 * Update local precondition according to remote precondition by status
 *
 * Return:
 *  None
 */
void _MNS_preconditionUpdateStatus(
    MNS_SessionObj  *mns_ptr,
    tPrecondition   *lclPrec_ptr,
    tPrecondition   *rmtPrec_ptr,
    tSdpAttrType    status,
    tPrecStatusType lclStatusType)
{
    vint lclStatusIdx; 
    vint rmtStatusIdx; 
    tPrecStatusType rmtStatusType;

    rmtStatusType = _MNS_preconditionMapStatusType(lclStatusType);
    
    if (-1 != (rmtStatusIdx = _MNS_preconditionFindStatus(rmtPrec_ptr, status,
            rmtStatusType))) {
        /* Found current remote status, update to local */
        if (-1 == (lclStatusIdx = _MNS_preconditionFindStatus(lclPrec_ptr,
                status, lclStatusType))) {
            /* no such status in local, add one */
            lclStatusIdx = _MNS_preconditionAddStatus(
                    lclPrec_ptr, status, ePrecTypeQos, lclStatusType,
                    ePrecDirNone, ePrecStrengthMandatory);
        }
        _MNS_preconditionUpdateDir(lclPrec_ptr,
                &lclPrec_ptr->status[lclStatusIdx].dir,
                &rmtPrec_ptr->status[rmtStatusIdx].dir);
        /* Update strength-tag anyway */
        _MNS_preconditionUpdateStrength(
                &lclPrec_ptr->status[lclStatusIdx].strength,
                &rmtPrec_ptr->status[rmtStatusIdx].strength);
    }
}


/*
 * ========_MNS_preconditionUpdate()========
 * Update local precondition according to remote precondition.
 * And also update if precondition is met.
 *
 * Return:
 *  MNS_OK      Update status successfully
 *  MNS_ERR     Failed
 */
vint _MNS_preconditionUpdate(
    MNS_SessionObj *mns_ptr,
    tPrecondition  *lclPrec_ptr,
    tPrecondition  *rmtPrec_ptr)
{
    vint lclStatusIdx; 

    /*
     * If remote doesn't use precondtion, ignore precondition and
     * unset usePrecondition.
     */
    if (0 == rmtPrec_ptr->numStatus) {
        MNS_dbgPrintf("%s:%d Remote media doesn't use precondition.\n",
                __FILE__, __LINE__);
        /* always set precondition met if precondition is not used */
        lclPrec_ptr->isMet     = 1;
        lclPrec_ptr->numStatus = 0;
        /* Set rsrc reday */
        mns_ptr->rsrcStatus = (ISI_ResourceStatus)(
                ISI_RESOURCE_STATUS_REMOTE_READY |
                ISI_RESOURCE_STATUS_LOCAL_READY);
        return (MNS_ERR);
    }

    /* Check if local failure */
    if (ISI_RESOURCE_STATUS_FAILURE & mns_ptr->rsrcStatus) {
        /* Local resource failure, set strength to failure */
        if (-1 == (lclStatusIdx = _MNS_preconditionFindStatus(lclPrec_ptr,
                eSdpAttrDes, ePrecStatusTypeLocal))) {
            /* cannot find local des status, add new one */
            _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrDes, ePrecTypeQos,
                    ePrecStatusTypeLocal,
                    _MNS_preconditionGetDirByRsrcStatus(mns_ptr),
                    ePrecStrengthFailure);
        }
        else {
            /* Got status, set strength to failure */
            lclPrec_ptr->status[lclStatusIdx].strength = ePrecStrengthFailure;
        }
    }

    /* Check if we got confirm status from remote side */
    /* first update local resource reservation status */
    if (-1 != (lclStatusIdx = _MNS_preconditionFindStatus(rmtPrec_ptr,
            eSdpAttrConf, ePrecStatusTypeRemote))) {
        /* we got conf status */
        lclPrec_ptr->isConfReceived = 1;
    }
    
    /*
     * Even though e2e is not required in TS 24.229, here still update current
     * e2e status in case remote side required it.
     */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrCurr, ePrecStatusTypeE2e);
    /* Update current local status */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrCurr, ePrecStatusTypeLocal);
    /* Update current remote status */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrCurr, ePrecStatusTypeRemote);
    /*
     * Even though e2e is not required in TS 24.229, here still update desired
     * e2e status in case remote side required it.
     */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrDes, ePrecStatusTypeE2e);
    /* Update desired local status */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrDes, ePrecStatusTypeLocal);
    /* Update desired remote status */
    _MNS_preconditionUpdateStatus(mns_ptr, lclPrec_ptr, rmtPrec_ptr,
            eSdpAttrDes, ePrecStatusTypeRemote);
    
    /* Update the precondition met result */
    _MNS_preconditionUpdateMet(lclPrec_ptr);

    /* Check if need to send confirm status */
    if (_MNS_preconditionIsMet(lclPrec_ptr, ePrecStatusTypeLocal)) {
        if (!_MNS_preconditionIsMet(lclPrec_ptr, ePrecStatusTypeRemote)) {
            _MNS_preconditionAddStatus(lclPrec_ptr, eSdpAttrConf, ePrecTypeQos,
                    ePrecStatusTypeRemote, ePrecDirSendRecv, 
                    ePrecStrengthMandatory);
        }
        else {
            /* Remove confirm status */
            _MNS_preconditionDelStatus(lclPrec_ptr, eSdpAttrConf,
                    ePrecStatusTypeRemote);
        }
    }

    /* Update remote precondition to rsrcStatus */
    _MNS_preconditionUpdateRsrcStatus(mns_ptr, lclPrec_ptr);

    return (MNS_OK);
}

