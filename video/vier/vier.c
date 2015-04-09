/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 27616 $ $Date: 2014-07-18 11:02:53 +0800 (Fri, 18 Jul 2014) $
 */

#include <osal.h>
#include <osal_net.h>
#include "vpr_comm.h"
#include "vpad_vpmd.h"
#include "vier_net.h"
#include "vier.h"
#include "_vier.h"

extern VIER_Obj *_VIER_Obj_ptr;

/*
 * This file implements the VIER_netXxx() interfaces used by
 * video engine when compiled with VPR(vPort4G+) support.
 */

/*
 * ======== VIER_init() ========
 * Public function to initialize ViER.
 *
 * Returns: 
 * OSAL_SUCCESS: ViER successfully initialized.
 * OSAL_FAIL: Error in ViER initialization.
 */
OSAL_Status VIER_init(
    void)
{
    vint idx;
    char qName[OSAL_MSG_PATH_NAME_SIZE_MAX];

    /* Allocate memory for ViER_Obj */
    if (NULL == (_VIER_Obj_ptr = OSAL_memCalloc(1, sizeof(VIER_Obj), 0))) {
        OSAL_logMsg("%s %d Failed to allocate memory for global VIER_Obj\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Initialize VIER_Socket array */
    for (idx = 0; idx < VIER_MAX_VIDEO_STREAMS; idx++) {
        _VIER_Obj_ptr->socket[idx].socketId = VIER_SOCKET_ID_NONE;        

        /*
         * Create msg queues for each video stream.
         * These queues work as sockets for video rtp, and it returns to VE when
         * VIER_netSocket() is called.
         */
        OSAL_snprintf(qName, sizeof(qName), "%s%d", VIER_SOCKET_Q_NAME, idx);
        if (0 == (_VIER_Obj_ptr->socket[idx].qId =
                OSAL_msgQCreate(qName,
                OSAL_MODULE_VIER, OSAL_MODULE_VIER, OSAL_DATA_STRUCT_VPR_Comm,
                VIER_SOCKET_Q_DEPTH,
                sizeof(VPR_Comm), 0))) { 
            OSAL_logMsg("%s %d Failed creating %s queue\n",
                    __FUNCTION__, __LINE__, qName);
            return (OSAL_FAIL);
        }
    }
    _VIER_Obj_ptr->queue.evtQVideo = NULL;
    /* create video command and event message queue */
    if (NULL == (_VIER_Obj_ptr->queue.cmdQVideo = OSAL_msgQCreate(
                VIDEO_VTSP_CMD_Q_NAME,
                OSAL_MODULE_VIER, OSAL_MODULE_VIDEO_VE,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VIDEO_VTSP_CMD_Q_NAME);
        return (OSAL_FAIL);
    }
    if (NULL == (_VIER_Obj_ptr->queue.evtQVideo = OSAL_msgQCreate(
                VIDEO_VTSP_EVT_Q_NAME,
                OSAL_MODULE_VIDEO_VE, OSAL_MODULE_VIER,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_EVENT_NUM_MSG,
                _VTSP_Q_EVENT_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VIDEO_VTSP_EVT_Q_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp msg queue */
    if (NULL == (_VIER_Obj_ptr->queue.rtcpCmdQId = OSAL_msgQCreate(
                VIDEO_VTSP_RTCP_MSG_Q_NAME,
                OSAL_MODULE_VIDEO_VE, OSAL_MODULE_VIER,
                OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
                _VTSP_Q_RTCP_NUM_MSG,
                _VTSP_Q_RTCP_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VIDEO_VTSP_RTCP_MSG_Q_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp evt queue */
    if (NULL == (_VIER_Obj_ptr->queue.rtcpEvtQId = OSAL_msgQCreate(
                VIDEO_VTSP_RTCP_EVT_Q_NAME,
                OSAL_MODULE_VIER, OSAL_MODULE_VIDEO_VE,
                OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
                _VTSP_Q_RTCP_NUM_EVENT,
                _VTSP_Q_RTCP_EVENT_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VIDEO_VTSP_RTCP_EVT_Q_NAME);
        return (OSAL_FAIL);
    }
    /* Start vier task */
    if (0 == (_VIER_Obj_ptr->taskId = OSAL_taskCreate( 
            VIER_TASK_NAME, 
            OSAL_TASK_PRIO_VTSPR,
            VIER_TASK_STACK_BYTES, 
            _VIER_daemon, 
            (void *)_VIER_Obj_ptr))) { 
        OSAL_logMsg("Failed creating %s daemon\n", VIER_TASK_NAME); 
        return (OSAL_FAIL); 
    } 

    return (OSAL_SUCCESS);
}

/* 
 * ======== VIER_shutdown() ========
 * Public function to shutdown ViER.
 *
 * Returns: 
 * OSAL_SUCCESS: ViER shutdown successfully 
 * OSAL_FAIL: Error in ViER shutdown
 */
OSAL_Status VIER_shutdown(
    void)
{
    vint idx;

    /* Delete vier task */
    OSAL_taskDelete(_VIER_Obj_ptr->taskId);

    /* Destroy Qs */
    for (idx = 0; idx < VIER_MAX_VIDEO_STREAMS; idx++) {
        OSAL_msgQDelete(_VIER_Obj_ptr->socket[idx].qId);
    }

    /* delete video command/event queue */
    if (NULL != _VIER_Obj_ptr->queue.cmdQVideo) {
        OSAL_msgQDelete(_VIER_Obj_ptr->queue.cmdQVideo);
    }
    if (NULL != _VIER_Obj_ptr->queue.evtQVideo) {
        OSAL_msgQDelete(_VIER_Obj_ptr->queue.evtQVideo);
    }

    /* Free memory */
    if (NULL != _VIER_Obj_ptr) {
        OSAL_memFree(_VIER_Obj_ptr, 0);
        _VIER_Obj_ptr = NULL;
    }
    return (OSAL_SUCCESS);
}

/* 
 * ======== VIER_netSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type)
{
    if (VPR_NETWORK_MODE_WIFI == _VIER_Obj_ptr->networkMode) {
        return OSAL_netSocket(socket_ptr, type);
    }
    else {
        if (OSAL_SUCCESS != _VIER_netSocket(socket_ptr, type)) {
            return (OSAL_FAIL);
        }
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== VIER_netCloseSocket() ========
 *
 * Wrapper function of net socket close function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netCloseSocket(
    OSAL_NetSockId *socket_ptr)
{
    if (OSAL_TRUE == _VIER_isVprHostedSocket(socket_ptr)) {
        return _VIER_netCloseSocket(socket_ptr);
    }
    return OSAL_netCloseSocket(socket_ptr);
}

/*
 * ========_VIER_netBindSocket() ========
 *
 * Wrapper function of net socket bind function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    if (OSAL_TRUE == _VIER_isVprHostedSocket(socket_ptr)) {
        return _VIER_netBindSocket(socket_ptr, address_ptr);
    }
    return OSAL_netBindSocket(socket_ptr, address_ptr);
}

/*
 * ======== VIER_netSocketReceiveFrom() ========
 *
 * Wrapper function of net socket recvfrom function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    if (OSAL_TRUE == _VIER_isVprHostedSocket(socket_ptr)) {
        return _VIER_netSocketReceiveFrom(socket_ptr, buf_ptr, size_ptr,
                address_ptr);
    }
    return OSAL_netSocketReceiveFrom(socket_ptr, buf_ptr, size_ptr,
            address_ptr);
}

/*
 * ======== VIER_netSocketSendTo() ========
 *
 * Wrapper function of net socket sendto function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    if (OSAL_TRUE == _VIER_isVprHostedSocket(socket_ptr)) {
        return _VIER_netSocketSendTo(socket_ptr, buf_ptr, size_ptr,
                address_ptr);
    }
    return OSAL_netSocketSendTo(socket_ptr, buf_ptr, size_ptr, address_ptr);
}

/*
 * ======== VIER_netSetSocketOptions() ========
 *
 * Wrapper function of net set socket options function.
 * It calls OSAL net function or _VIER net function depends on wifi mode or
 * 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VIER_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value)
{
    if (OSAL_TRUE == _VIER_isVprHostedSocket(socket_ptr)) {
        return _VIER_netSetSocketOptions(socket_ptr, option, value);
    }
    return OSAL_netSetSocketOptions(socket_ptr, option, value);
}

