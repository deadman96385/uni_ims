/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10497 $ $Date: 2009-10-06 17:59:33 +0800 (Tue, 06 Oct 2009) $
 *
 */

#include <osal_msg.h>
#include <osal_sem.h>
#include <osal_mem.h>
#include <osal_string.h>
#include <osal_log.h>
#include <osal_task.h>

#include <sdi_msg.h>
#include <sdi_cfg.h>
#include <sdi_cfg_entity.h>
#include <sdi_cfg_mod.h>
#include <ims_stack_msgs.h>

#define OSAL_MSG_Q_NAME_SIZE_MAX    (128)
#define OSAL_MSG_Q_MAX_SZ           (256)

typedef struct {
    OSAL_MsgQId qId[OSAL_MSG_GROUP_MAX_QUEUES];
    /* counting semaphore for group synch */
    OSAL_SemId  sId;
} OSAL_MsgGroup;

/*
 * Private Q structure.
 */
typedef struct {
    osa_msgqid      qId;
    uint32          maxNumMessages;
    uint32          maxMessageLen;
    char            qName[OSAL_MSG_Q_NAME_SIZE_MAX];
    vint            qSrcModId;
    vint            qDstModId;
    vint            qMsgStructId;
    OSAL_SemId      semId;
    OSAL_MsgGroup  *group;
} OSAL_MsgQ;

static OSAL_MsgQ *_osal_dev_mqDev[OSAL_MSG_Q_MAX_SZ] = {NULL};

/*
 * ======== _OSAL_msgQGetModId() ========
 * This function is used to mapping SPRD module id,
 * and the source module id is osal module id which is define in osal_constant.h
 * SPRD define module id in MS_Code/PS/sdi/export/inc/sdi_cfg_mod.h
 *
 * Return:
 *  SPRD module id
 */
vint _OSAL_msgQGetModId(
    vint    srcId)
{
    if (OSAL_MODULE_CSM_PUBLIC == srcId) {
        return (MOD_CSM);
    }
    else if (OSAL_MODULE_CSM_PRIVATE == srcId) {
        return (MOD_CSM_PRI);
    }
    else if (OSAL_MODULE_ISIM == srcId) {
        return (MOD_ISIM);
    }
    else if (OSAL_MODULE_MGT == srcId) {
        return (MOD_MGT);
    }
#if 0
    else if (OSAL_MODULE_ISI == srcId) {
        return ();
    }
    else if (OSAL_MODULE_ISI_MSG_RECV == srcId) {
        return ();
    }
    else if (OSAL_MODULE_MC == srcId) {
        return ();
    }
    else if (OSAL_MODULE_SAPP == srcId) {
        return ();
    }
    else if (OSAL_MODULE_MSRP == srcId) {
        return ();
    }
#endif
    else if (OSAL_MODULE_RIR == srcId) {
        return (MOD_RIR);
    }
#if 0
    else if (OSAL_MODULE_SIP == srcId) {
        return ();
    }
#endif
    else if (OSAL_MODULE_GBAM == srcId) {
        return (MOD_GBAM);
    }
    else if (OSAL_MODULE_XCAP == srcId) {
        return (MOD_XCAP);
    }
    else {
        return (MOD_DUMMY_IMS);
    }
}

/*
 * ======== _OSAL_msgQGetMsgId() ========
 * This function is used to look up message struct id for SPRD
 * SPRD define message id in
 * MS_Code/PS/stack/common/interfaces/common/include/ims_stack_msgs.h
 *
 * Retrun:
 *   SPRD's message id
 */
vint _OSAL_msgQGetMsgId(
    vint    msgStructId,
    void   *buf_ptr)
{
    vint    msgId;
    vint    type;

    switch (msgStructId) {
#if 0
        case OSAL_DATA_STRUCT_CSM_InputEvent:
        case OSAL_DATA_STRUCT_CSM_PrivateInputEvt:
        case OSAL_DATA_STRUCT_CSM_OutputEvent:
            type = *(vint *)(buf_ptr);
            switch (type) {
                case 0: /* CSM_EVENT_TYPE_CALL */
                case 1: /* CSM_EVENT_TYPE_SMS */
                case 2: /* CSM_EVENT_TYPE_SERVICE */
                case 3: /* CSM_EVENT_TYPE_RADIO */
                case 4: /* CSM_EVENT_TYPE_SUPSRV */
                case 5: /* CSM_EVENT_TYPE_USSD */
                    msgId = msgStructId;
                    break;
                default:
                    msgId = -1;
                    break;
            }
            break;
        case OSAL_DATA_STRUCT_ISIP_Message:
            type = *((vint *)buf_ptr + 1);
            switch (type) {
                case 1:  /* ISIP_CODE_SERVICE */
                case 2:  /* ISIP_CODE_CALL */
                case 3:  /* ISIP_CODE_MESSAGE */
                case 4:  /* ISIP_CODE_PRESENCE */
                case 5:  /* ISIP_CODE_TEL_EVENT */
                case 6:  /* ISIP_CODE_MEDIA */
                case 7:  /* ISIP_CODE_SYSTEM */
                case 8:  /* ISIP_CODE_CHAT */
                case 9:  /* ISIP_CODE_FILE */
                case 10: /* ISIP_CODE_DIAGNOSTIC */
                case 11: /* ISIP_CODE_USSD */
                    msgId = msgStructId;
                    break;
                default:
                    msgId = -1;
                    break;
            }
            break;
        case OSAL_DATA_STRUCT_MSRP_Event:
        case OSAL_DATA_STRUCT_VPR_Comm:
        case OSAL_DATA_STRUCT_RIR_EventMsg:
        case OSAL_DATA_STRUCT__VTSP_CmdMsg:
        case OSAL_DATA_STRUCT_VTSP_EventMsg:
        case OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg:
        case OSAL_DATA_STRUCT_VAPP_Msg:
        case OSAL_DATA_STRUCT_SIT_QueueMsg:
        case OSAL_DATA_STRUCT_GBA_Event:
        case OSAL_DATA_STRUCT_GBA_Command:
#endif
        case OSAL_DATA_STRUCT_CSM_InputEvent:
            msgId = MSG_ID_CSM_INPUT_EVENT;
            break;
        case OSAL_DATA_STRUCT_CSM_OutputEvent:
            msgId = MSG_ID_CSM_OUTPUTEVENT;
            break;
        case OSAL_DATA_STRUCT_int:
            msgId = MSG_ID_INTEGER;
            break;
        default:
            msgId = MSG_ID_UNKONWN;
            break;
    }
    return (msgId);
}

/*
 * ======== _OSAL_msgQFindQueue() ========
 * This is a private function used to loop up queue
 *
 * Return:
 *   OSAL_MsgQ*
 */
OSAL_MsgQ* _OSAL_msgQFindQueue(
    char   *qName_ptr)
{
    int idx;

    for (idx = 0; idx < OSAL_MSG_Q_MAX_SZ; idx++) {
        if (NULL != _osal_dev_mqDev[idx]) {
            if (0 == OSAL_strcmp(qName_ptr, _osal_dev_mqDev[idx]->qName)) {
                return (_osal_dev_mqDev[idx]);
            }
        }
    }
    return (NULL);
}

/*
 * ======== OSAL_msgQCreate() ========
 *
 * OS Independent Message Q Creation
 *
 * Params:
 *   name_ptr:    message queue name
 *   srcModId:    source module Id
 *   dstModId:    destination module Id
 *   msgStructId: message structure name
 *   maxMsgs:     message number of the queue
 *   lenOfMsg:    message size
 *   flags:       a bit mask which is currently unused
 *
 * Returns:
 *   OSAL_MsgQId: Which is to be used for subsequent msgQ-related calls.
 *                On success, this is non-zero. On failure, this is NULL
 *
 */
OSAL_MsgQId OSAL_msgQCreate(
    char   *name_ptr,
    vint    srcModId,
    vint    dstModId,
    vint    msgStructId,
    uint32  maxMsgs,
    uint32  lenOfMsg,
    uint32  flags)
{
    OSAL_MsgQ  *q_ptr, *qCheck_ptr;
    vint        idx;

    /* Check input parameters. */
    if(NULL == name_ptr) {
        OSAL_logMsg("%s:%d ERROR: queue name is empty.\n",
                __FUNCTION__, __LINE__);
        return(NULL);
    }

    if (0 == maxMsgs) {
        OSAL_logMsg("%s:%d ERROE: maxNumMessage=0\n", __FUNCTION__, __LINE__);
        return (NULL);
    }

    if(OSAL_MSG_SIZE_MAX <= lenOfMsg) {
        OSAL_logMsg("%s:%d ERROR: message size is over %d\n",
                __FUNCTION__, __LINE__, OSAL_MSG_SIZE_MAX);
        return(NULL);
    }

    /* Start to create message queue. */
    if (NULL == (q_ptr = (OSAL_MsgQ *)OSAL_memCalloc(
           sizeof(OSAL_MsgQ), 1, 0))) {
        OSAL_logMsg("%s:%d ERROR: Cannot allocate memory.\n",
                __FUNCTION__, __LINE__);
        return(NULL);
    }

    q_ptr->maxNumMessages = maxMsgs;
    q_ptr->maxMessageLen  = lenOfMsg;
    q_ptr->group          = NULL;
    /* Get source module ID */
    q_ptr->qSrcModId      = srcModId;
    /* Get dest module ID */
    q_ptr->qDstModId      = dstModId;
    /* Get message event struct ID */
    q_ptr->qMsgStructId   = msgStructId;
    OSAL_strcpy(q_ptr->qName, name_ptr);

    /* Create queue now. */
    if (NULL == (qCheck_ptr = _OSAL_msgQFindQueue(q_ptr->qName))) {
        /* Create count sem */
        if (NULL == (q_ptr->semId = OSAL_semCountCreate(maxMsgs))) {
            OSAL_memFree(q_ptr, 0);
            return (OSAL_FAIL);
        }

        if (NULL == (q_ptr->qId = osa_create_fix_size_msg_q(ENTITY_DEFAULT,
                MEM_HDL_INFRA_DEFAULT, q_ptr->qName, sizeof(sdi_msg_struct),
                (uint16)maxMsgs))) {
            OSAL_logMsg("%s:%d Message queue create failed.\n",
                    __FUNCTION__, __LINE__);
            OSAL_memFree(q_ptr, 0);
            return (NULL);
        }
    }
    else {
        q_ptr->qId      = qCheck_ptr->qId;
        q_ptr->group    = qCheck_ptr->group;
        q_ptr->semId    = qCheck_ptr->semId;
        q_ptr->maxNumMessages = qCheck_ptr->maxNumMessages;
    }

    /* Insert queue to queue table to record queue id */
    for (idx = 0; idx < OSAL_MSG_Q_MAX_SZ; idx++) {
        if (NULL == _osal_dev_mqDev[idx]) {
            _osal_dev_mqDev[idx] = q_ptr;
            break;
        }
    }

    return((OSAL_MsgQId)q_ptr);
}

/*
 * ======== OSAL_msgQDelete() ========
 *
 * OS Independent Message Queue Deletion.
 *
 * Params:
 *   msgQId: the id returned from OSAL_msgQCreate()
 *
 * Returns:
 *   OSAL_Status indicating success or failure
 *
 */
OSAL_Status OSAL_msgQDelete(
    OSAL_MsgQId qId)
{
    OSAL_MsgQ  *q_ptr = (OSAL_MsgQ *)qId;
    vint        idx;

    if (NULL == q_ptr) {
        return (OSAL_FAIL);
    }

    osa_delete_msg_q(q_ptr->qId);

    /* remove queue from queue table */
    for (idx = 0; idx < OSAL_MSG_Q_MAX_SZ; idx++) {
        if (NULL != _osal_dev_mqDev[idx]) {
            if (0 == OSAL_strcmp(q_ptr->qName, _osal_dev_mqDev[idx]->qName)) {
                _osal_dev_mqDev[idx] = NULL;
            }
        }
    }

    return (OSAL_memFree(q_ptr, 0));
}

/*
 * ======== OSAL_msgQSend() ========
 *
 * Send a message on a OSAL Message queue
 *
 * Params:
 *   qId:         a descriptor of the msgQId returned from OSAL_msgQCreate()
 *   buffer_ptr:  a pointer to a user's buffer to xmit
 *   octets:      how many bytes in the user's buffer is valid
 *   msTimeout:   this is the maximum number of milliseconds that
 *                this task will pend for if the queue is full on send.
 *                This value may be OSAL_NO_WAIT, OSAL_WAIT_FOREVER,
 *                or a positive value
 *   timeout_ptr: set to OSAL_TRUE if the request timed out.
 *
 * Returns:
 *   OSAL_SUCCESS or OSAL_FAIL
 *
 */
OSAL_Status OSAL_msgQSend(
    OSAL_MsgQId     qId,
    void           *buffer_ptr,
    uint32          octets,
    uint32          msTimeout,
    OSAL_Boolean   *timeout_ptr)
{
    OSAL_MsgQ      *q_ptr = (OSAL_MsgQ *)qId;
    sdi_msg_struct  msg;
    void           *buffer_copy_ptr;

    if (NULL != timeout_ptr) {
        *timeout_ptr = OSAL_FALSE;
    }

    if ((NULL == q_ptr) || (octets >= OSAL_MSG_SIZE_MAX)) {
        return (OSAL_FAIL);
    }

    if (octets != q_ptr->maxMessageLen) {
        return (OSAL_FAIL);
    }

    buffer_copy_ptr = OSAL_memAlloc(octets, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    OSAL_memCpy(buffer_copy_ptr, buffer_ptr, octets);

    /* Makeup simple SDI message */
    OSAL_memSet((void *)&msg, 0, sizeof(msg));
    msg.src_entity_id  = ENTITY_IMS_STACK;
    msg.dest_entity_id = ENTITY_IMS_STACK;
    msg.src_mod_id     = _OSAL_msgQGetModId(q_ptr->qSrcModId);
    msg.dest_mod_id    = _OSAL_msgQGetModId(q_ptr->qDstModId);
    msg.sap_id         = IMS_SAP;
    msg.msg_id         = _OSAL_msgQGetMsgId(q_ptr->qMsgStructId, buffer_ptr);
    msg.user_data_ptr  = buffer_copy_ptr;

    /*
     * If msTimeout is OSAL_WAIT_FOREVER, OSAL_semAcquire will wait
     * OSA_INFINITE_WAIT. Otherwise sem will wait OSA_NO_WAIT.
     * see OSAL_semAcquire in osal_sem.c
     */
    if (OSAL_FAIL == OSAL_semAcquire(q_ptr->semId, msTimeout)) {
        OSAL_memFree(buffer_copy_ptr, 1);
        return (OSAL_FAIL);
    }

    /* Enque into message queue */
    if (OSA_SUCCESS != osa_enque_fix_size_msg_q(q_ptr->qId, &msg,
            OSA_NO_WAIT)) {
        /* Assertion */
        OSAL_memFree(buffer_copy_ptr, 1);
        return (OSAL_FAIL);
    }

    if (q_ptr->group) {
        OSAL_semGive(q_ptr->group->sId);
    }

    /* TODO: Trace msg to tool side */
    //mta_trace(&msg);

    return (OSAL_SUCCESS);
}


/*
 * ======== OSAL_msgQRecv() ========
 *
 * Receive a message from a OSAL Message queue
 *
 * Params:
 *   qId:          a descriptor of the msgQId returned from OSAL_msgQCreate()
 *   buffer_ptr:   a pointer to a user's buffer to receive message into
 *   bufferSz:     how many bytes can the user's buffer receive
 *   msTimeout:    this is the maximum number of milliseconds that
 *                 this task will pend for if the queue is full on send.
 *                 This value may be OSAL_NO_WAIT, OSAL_WAIT_FOREVER,
 *                 or a positive value
 *   timeout_ptr:  set to OSAL_TRUE if the request timed out.
 *
 * Returns:
 *   numBytes: to indicate number of bytes actually received on
 *             this message or ?? to indicate failure.
 *
 */
int32 OSAL_msgQRecv(
    OSAL_MsgQId    qId,
    void          *buffer_ptr,
    uint32         bufferSz,
    uint32         msTimeout,
    OSAL_Boolean  *timeout_ptr)
{
    int32           status;
    OSAL_MsgQ      *q_ptr = (OSAL_MsgQ *)qId;
    sdi_msg_struct  msg = {0};
    osa_wait_mode   timeout;

    status = -1;

    if (NULL != timeout_ptr) {
        *timeout_ptr = OSAL_FALSE;
    }

    if (NULL == q_ptr) {
        return(status);
    }

    if (bufferSz < q_ptr->maxMessageLen) {
        return(status);
    }

    if (OSAL_WAIT_FOREVER == msTimeout) {
        timeout = OSA_INFINITE_WAIT;
    }
    else {
        timeout = OSA_NO_WAIT;
    }

    if (OSA_SUCCESS == osa_deque_fix_size_msg_q(q_ptr->qId, &msg,
            timeout)) {
        if (NULL != msg.user_data_ptr) {
            status = bufferSz;
            OSAL_memCpy(buffer_ptr, msg.user_data_ptr, bufferSz);
            OSAL_memFree(msg.user_data_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        }
        OSAL_semGive(q_ptr->semId);
    }

    return(status);
}

/*
 * ======== OSAL_msgQGrpCreate() ========
 *
 * To enable a single caller to pend on multiple queues
 * simultaneously, the user can create a msgQGroup using this
 * routine, then add any message queues created with
 * OSAL_msgQCreate() to this and do a single OSAL_msgQGrpRecv() to
 * receive a message on any of the queues.
 *
 * Params:   type - OSAL_MGrp_type is either OSAL_MGRP_PRIORITY or
 *                 OSAL_MGRP_ROUND_ROBIN.
 *
 * Returns:  OSAL_MsgQGrpId - a descriptor of the message queue group
 *
 * Notes:
 *        OSAL Message Queue Groups can be defined as either priority-based or
 *        round-rob based. In priority based queueing, message queues that are
 *        added to a group are added in priority order. The first q added has
 *        the highest priority. this means that when the group is pended for a
 *        group receive, the first queue is preferred before the second and so
 *        on. This means that lower priority queues can be starved and senders
 *        have a higher potential to block on queue full. In ROUND_ROBIN
 *        messaging, all queues have equal weight and message group receive
 *        always begins with the queue added after the message queue delivered
 *        the previous group receive.
 */
OSAL_Status OSAL_msgQGrpCreate(
    OSAL_MsgQGrpId *g_ptr)
{
    vint            q;
    OSAL_MsgGroup  *group_ptr;

    group_ptr = (OSAL_MsgGroup *) OSAL_memAlloc((int32)sizeof(OSAL_MsgGroup),
            OSAL_MEM_ARG_STATIC_ALLOC);

    if (NULL == group_ptr) {
        g_ptr = NULL;
        return (OSAL_FAIL);
    }

    if (NULL == (group_ptr->sId = OSAL_semCountCreate(0))) {
        OSAL_memFree(group_ptr, 0);
        g_ptr = NULL;
        return (OSAL_FAIL);
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        group_ptr->qId[q] = 0;
    }

    *g_ptr = (OSAL_MsgQGrpId)group_ptr;
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_msgQGrpDelete() ========
 *
 * Delete a Message Group
 *
 * Params:   OSAL_MsgQGrpId - a descriptor of the msgQGrpId returned from 
 *                                OSAL_msgQGrpCreate()
 *
 * Returns:  OSAL_Status indicating success or failure
 *
 */
OSAL_Status OSAL_msgQGrpDelete(
    OSAL_MsgQGrpId *g_ptr)
{
    OSAL_MsgGroup  *group_ptr;
    OSAL_MsgQ      *queue_ptr;
    vint            q, idx;

    if (NULL == g_ptr) {
        return (OSAL_FAIL);
    }

    group_ptr = (OSAL_MsgGroup*) *g_ptr;

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        queue_ptr = (OSAL_MsgQ *)group_ptr->qId[q];
        if (NULL == queue_ptr) {
            continue;
        }
        for (idx = 0; idx < OSAL_MSG_Q_MAX_SZ; idx++) {
            if (NULL == _osal_dev_mqDev[idx]) {
                continue;
            }
            if (queue_ptr->qId == _osal_dev_mqDev[idx]->qId) {
                _osal_dev_mqDev[idx]->group = NULL;
            }
        }
    }

    OSAL_memFree(group_ptr, 0);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_msgQGrpAddQ() ========
 *
 * Add a Message Queue to a Message Queue Group
 *
 * Params:
 *   g_ptr: a descriptor of the msgQGrpId
 *   qId:     a descriptor of the msgQId that is being added
 *            to the msgQGrpId group
 *
 * Returns:  OSAL_Status indicating success or failure
 *
 */
OSAL_Status OSAL_msgQGrpAddQ(
    OSAL_MsgQGrpId *g_ptr,
    OSAL_MsgQId     qId)
{
    OSAL_MsgQ      *queue_ptr = (OSAL_MsgQ*) qId;
    OSAL_MsgGroup  *group_ptr;
    vint            q, idx;

    if ((NULL == queue_ptr) || (NULL == g_ptr)) {
        return (OSAL_FAIL);
    }

    group_ptr = (OSAL_MsgGroup *) *g_ptr;

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        /* Already in. */
        if (group_ptr->qId[q] == qId) {
            return (OSAL_SUCCESS);
        }
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        /* Find free location. */
        if (NULL == group_ptr->qId[q]) {
            /* Add */
            group_ptr->qId[q] = queue_ptr;
            queue_ptr->group  = group_ptr;
            for (idx = 0; idx < OSAL_MSG_Q_MAX_SZ; idx++) {
                if (NULL == _osal_dev_mqDev[idx]) {
                    continue;
                }
                if (queue_ptr->qId == _osal_dev_mqDev[idx]->qId) {
                    _osal_dev_mqDev[idx]->group = group_ptr;
                }
            }
            return (OSAL_SUCCESS);
        }
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_msgQGrpRecv() ========
 *
 * Allows the caller to receive from a group of message queues
 *                simultaneously.
 *
 * Params:
 *   groupId:   a descriptor of the msgQGrpId
 *   buf_ptr:   the user's buffer that the msg is to be receive into
 *   bufferSz:  that the user's buf_ptr can hold
 *   msTimeout: how long to pend until reception
 *   qId_ptr:   what Msg QId was the message transmitted on (if NULL,
 *              user doesn't care)
 *
 * Returns:
 *   int32 indicating number of bytes read (or -1 on ERROR)
 *
 */
int32 OSAL_msgQGrpRecv(
    OSAL_MsgQGrpId *g_ptr,
    void           *buffer_ptr,
    uint32          bufferSz,
    uint32          msTimeout,
    OSAL_MsgQId    *qId_ptr,
    OSAL_Boolean   *timeout_ptr)
{
    OSAL_MsgGroup  *group_ptr;
    OSAL_MsgQ      *queue_ptr;
    vint            q, len;
    sdi_msg_struct  msg;

    if (NULL == g_ptr) {
        return (-1);
    }

    *qId_ptr  = NULL;
    group_ptr = (OSAL_MsgGroup *) *g_ptr;

    /* wait (for the specified timeout) until there's a message... */
    if (OSAL_FAIL == OSAL_semAcquire(group_ptr->sId, msTimeout)) {
        return (-1);
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        queue_ptr = (OSAL_MsgQ *)group_ptr->qId[q];
        if (NULL == queue_ptr) {
            continue;
        }
        if (OSA_SUCCESS == osa_deque_fix_size_msg_q(queue_ptr->qId, &msg,
                OSA_NO_WAIT)) {
            len = (queue_ptr->maxMessageLen > bufferSz)?
                    bufferSz : queue_ptr->maxMessageLen;
            OSAL_memCpy(buffer_ptr, msg.user_data_ptr, len);
            OSAL_memFree(msg.user_data_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
            *qId_ptr = group_ptr->qId[q];
            OSAL_semGive(queue_ptr->semId);
            return (len);
        }
    }

    return (-1);
}

