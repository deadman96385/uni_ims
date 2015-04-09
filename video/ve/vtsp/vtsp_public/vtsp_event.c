/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12979 $ $Date: 2010-09-21 04:21:52 +0800 (Tue, 21 Sep 2010) $
 *
 */


#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_infcLineStatus() ========
 */
VTSP_Return VTSP_infcLineStatus(
    uvint             infc)
{
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != _VTSP_isInfcValid(infc)) { 
        return (VTSP_E_INFC);
    }

    cmd.code = _VTSP_CMD_INFC_LINE_STATUS;
    cmd.infc = infc;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}


/*
 * ======== VTSP_getEvent() ========
 */
VTSP_Return VTSP_getEvent(
    vint            infc,      
    VTSP_EventMsg  *msg_ptr,
    uint32          timeout)
{
    VTSP_Return     e;
    vint            v;          /* must be signed */
    OSAL_MsgQId     qId;
    OSAL_MsgQGrpId  qGrpId;
    OSAL_Boolean    timeo;

    _VTSP_VERIFY_INIT;

#if defined(OSAL_PTHREADS) && (_VTSP_Q_CMD_TIMEOUT == OSAL_WAIT_FOREVER)
    /* Fix for some verions of PTHREADS which have bug with timed wait  */
    if (timeout != OSAL_NO_WAIT) { 
        timeout = OSAL_WAIT_FOREVER;
    }
#endif

    if (VTSP_INFC_ANY == infc) { 
        qGrpId = _VTSP_object_ptr->infcEventQGrp;

        v = OSAL_msgQGrpRecv(&qGrpId, (char *)msg_ptr, 
                sizeof(VTSP_EventMsg), timeout, &qId, &timeo);
    }
    else { 
        qId = NULL;
        if (VTSP_INFC_GLOBAL == infc) { 
            qId = _VTSP_object_ptr->globalEventQ;
        }
        else { 
            if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
                _VTSP_TRACE(__FILE__, __LINE__);
                return (e);
            }
            qId = _VTSP_object_ptr->infc[infc].eventQ;
        }

        if (NULL == qId) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_CONFIG);
        }

        /* OSAL_msgQRecv returns # bytes or -1 for error */
        v = OSAL_msgQRecv(qId, msg_ptr, 
                sizeof(VTSP_EventMsg), timeout, &timeo);
    }

    if (VTSP_EVENT_MSG_CODE_SHUTDOWN_VIDEO == msg_ptr->code) {
        _VTSP_TRACE(__FILE__, __LINE__);
        if (VTSP_EVENT_ACTIVE == msg_ptr->msg.shutdown.reason) {
            _VTSP_TRACE("Video engine ready\n", 0);
        }
        else {
            _VTSP_TRACE("Video engine shutdown\n", 0);
        }
    }

    /* If shutdown message, signal VTSP_shutdown() */
    if (VTSP_EVENT_MSG_CODE_SHUTDOWN == msg_ptr->code) {
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("SHUTDOWN: release semaphore", 0);
        OSAL_semGive(_VTSP_object_ptr->shutdownSemId);        
    }

    if (v <= 0) { 
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_CONFIG);
    }

    return (VTSP_OK);
}

/*
 * ======== VTSP_putEvent() ========
 */
VTSP_Return VTSP_putEvent(
    vint            infc,
    VTSP_EventMsg  *msg_ptr,
    uint32          timeout)
{
    VTSP_Return     e;
    vint            v;          /* must be signed */
    OSAL_MsgQId     qId;
    OSAL_Boolean    timeo;

    _VTSP_VERIFY_INIT;

    if (VTSP_INFC_GLOBAL == infc) { 
        qId = _VTSP_object_ptr->globalEventQ;
    }
    else { 
        if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (e);
        }
        qId = _VTSP_object_ptr->infc[infc].eventQ;
    }

    if (NULL == qId) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_CONFIG);
    }

    v = OSAL_msgQSend(qId, (char *)msg_ptr, 
            sizeof(VTSP_EventMsg), timeout, &timeo);
    if (OSAL_SUCCESS != v) { 
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_CONFIG);
    }

    return (VTSP_OK);
}
 
