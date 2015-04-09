/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
 *
 */        
#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_stunSend() ========
 */
VTSP_Return VTSP_stunSend(
    VTSP_Stun *stunMsg_ptr,
    uint32     timeout,
    vint       video)
{
    OSAL_MsgQId     qId;
    OSAL_Boolean    timeo;
    vint            osalStatus;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (NULL == stunMsg_ptr) {
        return (VTSP_E_ARG);
    }

    if (video) {
        qId = _VTSP_object_ptr->stunSendQVideo;
    }
    else {
        qId = _VTSP_object_ptr->stunSendQ;
    }

    osalStatus = OSAL_msgQSend(qId, (char *)stunMsg_ptr, sizeof(VTSP_Stun),
            timeout, &timeo);

    if (OSAL_SUCCESS != osalStatus) { 
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}


/*
 * ======== VTSP_stunRecv() ========
 */
VTSP_Return VTSP_stunRecv(
    VTSP_Stun *stunMsg_ptr,
    uint32     timeout,
    vint       video)
{
    OSAL_MsgQId     qId;
    OSAL_Boolean    timeo;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (NULL == stunMsg_ptr) {
        return (VTSP_E_ARG);
    }

    if (video) {
        qId = _VTSP_object_ptr->stunRecvQVideo;
    }
    else {
        qId = _VTSP_object_ptr->stunRecvQ;
    }

    if (OSAL_msgQRecv(qId, (char *)stunMsg_ptr,
            sizeof(VTSP_Stun), timeout, &timeo) != sizeof(VTSP_Stun)) {
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}
