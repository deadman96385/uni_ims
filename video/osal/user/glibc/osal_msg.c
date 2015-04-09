/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7102 $ $Date: 2008-07-23 16:03:18 -0500 (Wed, 23 Jul 2008) $
 *
 */

#include <osal_msg.h>
#include <osal_mem.h>
#include <osal_string.h>
#include <osal_select.h>

char OSAL_msgQPathName[OSAL_MSG_PATH_NAME_SIZE_MAX] = {0};

/*
 * Private Group Q structure.
 */
typedef struct {
    OSAL_MsgQId qId[OSAL_MSG_GROUP_MAX_QUEUES];
    OSAL_Fid    fid[OSAL_MSG_GROUP_MAX_QUEUES];
    uint32      sz[OSAL_MSG_GROUP_MAX_QUEUES];
} _OSAL_MsgGrpQParams;

/*
 * Private Q structure.
 */
typedef struct {
    char    name[128];
    int     fid;
    uint32  sz;
} _OSAL_MsgQParams;

/*
 * ======== _OSAL_msgQSetBlocking() ========
 *
 * Set access to blocking 1, non blocking 0.
 *
 * Returns:
 *  OSAL_SUCCESS or OSAL_FAIL
 */
static OSAL_Status _OSAL_msgQSetBlocking(
    int          fid,
    OSAL_Boolean blocking)
{
    int flags;

    if ((flags = fcntl(fid,
            F_GETFL,
            0)) < 0) {
        flags = 0;
    }

    if (OSAL_TRUE == blocking) {
        flags &= ~O_NONBLOCK;
    }
    else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fid,
            F_SETFL,
            flags) < 0) {
        return(OSAL_FAIL);
    }

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_msgQCreate() ========
 *
 * This function is used to create a OSAL message queue.
 *
 * srcModId, dstModId, and msgStructId are not used here.
 *
 * Returns:
 * NULL if failed, q ID else.
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
    _OSAL_MsgQParams *q_ptr;

    /*
     * FIFO queues, ignore message numbers
     */
    if((NULL == name_ptr) || (lenOfMsg >= OSAL_MSG_SIZE_MAX)) {
        return(NULL);
    }

    if (NULL == (q_ptr = (_OSAL_MsgQParams *)OSAL_memCalloc(
           sizeof(_OSAL_MsgQParams),
           1,
           0))) {
        return(NULL);
    }

    if (0 == OSAL_msgQPathName[0]) {
        OSAL_snprintf(q_ptr->name,
                sizeof(q_ptr->name) - 1,
                "%s%s-%08x",
                OSAL_IPC_FOLDER,
                name_ptr,
                lenOfMsg);
    }
    else {
        OSAL_snprintf(q_ptr->name,
                sizeof(q_ptr->name) - 1,
                "%s/%s-%08x",
                OSAL_msgQPathName,
                name_ptr,
                lenOfMsg);
    }


    /*
     * If queue exists dont create it again
     */
    if (0 > access(q_ptr->name, F_OK)) {
        if (mknod(q_ptr->name, S_IFIFO | 0666, 0) < 0) {
            OSAL_memFree(q_ptr, 0);
            return(NULL);
        }
    }

    /*
     * open it RW
     */
    q_ptr->fid = open(q_ptr->name, O_RDWR);
    if (q_ptr->fid < 0) {
        OSAL_memFree(q_ptr, 0);
        return(NULL);
    }

    q_ptr->sz = lenOfMsg;

    return((OSAL_MsgQId)q_ptr);
}

/*
 * ======== OSAL_msgQDelete() ========
 *
 * This function is used to delete a OSAL message queue.
 * Note: queues on which tasks are pending should not be deleted without waking
 * up the pending tasks.
 *
 * Returns:
 * OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status OSAL_msgQDelete(
    OSAL_MsgQId qId)
{
    _OSAL_MsgQParams *q_ptr = (_OSAL_MsgQParams *)qId;

    if (NULL == q_ptr) {
        return(OSAL_FAIL);
    }

    if (0 != close(q_ptr->fid)) {
        return (OSAL_FAIL);
    }

    return(OSAL_memFree(q_ptr, 0));
}

/*
 * ======== OSAL_msgQSend() ========
 *
 * This function is used to put a message in a message queue.
 *
 * Returns:
 * OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status OSAL_msgQSend(
    OSAL_MsgQId   qId,
    void         *buffer_ptr,
    uint32        octets,
    uint32        msTimeout,
    OSAL_Boolean *timeout_ptr)
{
    OSAL_Status          status;
    OSAL_SelectSet       set;
    OSAL_SelectTimeval   time;
    OSAL_Boolean         test;
    _OSAL_MsgQParams    *q_ptr = (_OSAL_MsgQParams *)qId;

    status = OSAL_FAIL;
    if (NULL != timeout_ptr) {
        *timeout_ptr = OSAL_FALSE;
    }

    if ((NULL == q_ptr) || (octets >= OSAL_MSG_SIZE_MAX)) {
        return(status);
    }

    if (octets != q_ptr->sz) {
        return(status);
    }

    /*
     * Set to block or no block.
     */
    switch (msTimeout) {
        case OSAL_NO_WAIT:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    OSAL_FALSE)) {
                return(status);
            }
            break;
        default:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    OSAL_TRUE)) {
                return(status);
            }
            break;
    }

    switch (msTimeout) {
        case OSAL_WAIT_FOREVER:
        case OSAL_NO_WAIT:
            if (q_ptr->sz == write(q_ptr->fid,
                    buffer_ptr,
                    q_ptr->sz)) {
                status = OSAL_SUCCESS;
            }
            break;
        default:
            OSAL_selectSetInit(&set);
            OSAL_selectAddId(&q_ptr->fid, &set);
            time.sec  = msTimeout / 1000;
            msTimeout %= 1000;
            time.usec = msTimeout * 1000;
            test = OSAL_FALSE;
            if (OSAL_FAIL == OSAL_select((OSAL_SelectSet *)0,
                    &set,
                    &time,
                    &test)) {
                break;
            }
            if (NULL != timeout_ptr) {
                *timeout_ptr = test;
            }
            test = OSAL_FALSE;
            if (OSAL_FAIL == OSAL_selectIsIdSet(&q_ptr->fid,
                    &set,
                    &test)) {
                break;
            }
            if (OSAL_TRUE == test) {
                if (q_ptr->sz == write(q_ptr->fid,
                        buffer_ptr,
                        q_ptr->sz)) {
                    status = OSAL_SUCCESS;
                }
            }
            break;
    }

    return(status);
}

/*
 * ======== OSAL_msgQRecv() ========
 *
 * This function is used to get a message from a message queue.
 *
 * Returns:
 *  Number of octets read.
 */
int32 OSAL_msgQRecv(
    OSAL_MsgQId   qId,
    void         *buffer_ptr,
    uint32        bufferSz,
    uint32        msTimeout,
    OSAL_Boolean *timeout_ptr)
{
    int32                ret;
    int32                status;
    OSAL_SelectSet       set;
    OSAL_SelectTimeval   time;
    OSAL_Boolean         test;
    _OSAL_MsgQParams    *q_ptr = (_OSAL_MsgQParams *)qId;

    status = -1;
    if (NULL != timeout_ptr) {
        *timeout_ptr = OSAL_FALSE;
    }

    if (NULL == q_ptr) {
        return(status);
    }

    if (bufferSz < q_ptr->sz) {
        return(status);
    }

    /*
     * Set to block or no block.
     */
    switch (msTimeout) {
        case OSAL_NO_WAIT:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    OSAL_FALSE)) {
                return(status);
            }
            break;
        default:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    OSAL_TRUE)) {
                return(status);
            }
            break;
    }

    switch (msTimeout) {
        case OSAL_WAIT_FOREVER:
        case OSAL_NO_WAIT:
            ret = read(q_ptr->fid,
                    buffer_ptr,
                    q_ptr->sz);
            if (ret > 0) {
                status = ret;
            }
            break;
        default:
            OSAL_selectSetInit(&set);
            OSAL_selectAddId(&q_ptr->fid, &set);
            time.sec  = msTimeout / 1000;
            msTimeout %= 1000;
            time.usec = msTimeout * 1000;
            test = OSAL_FALSE;
            if (OSAL_FAIL == OSAL_select(&set,
                    (OSAL_SelectSet *)0,
                    &time,
                    &test)) {
                break;
            }
            if (NULL != timeout_ptr) {
                *timeout_ptr = test;
            }
            test = OSAL_FALSE;
            if (OSAL_FAIL == OSAL_selectIsIdSet(&q_ptr->fid,
                    &set,
                    &test)) {
                break;
            }
            if (OSAL_TRUE == test) {
                ret = read(q_ptr->fid,
                        buffer_ptr,
                        q_ptr->sz);
                if (ret > 0) {
                    status = ret;
                }
            }
            break;
    }
    return(status);
}

/*
 * ======== OSAL_msgQGrpCreate() ========
 *
 * This function is used to init a message queue group.
 *
 * Returns:
 *  OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status OSAL_msgQGrpCreate(
    OSAL_MsgQGrpId *g_ptr)
{
    vint                    q;
    _OSAL_MsgGrpQParams    *group_ptr;

    if (NULL == (group_ptr = (_OSAL_MsgGrpQParams *) OSAL_memCalloc(1,
            (int32) sizeof(_OSAL_MsgGrpQParams),0))) {
        *g_ptr = NULL;
        return (OSAL_FAIL);
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        group_ptr->fid[q] = -1;
        group_ptr->qId[q] = 0;
        group_ptr->sz[q]  = 0;
    }

    *g_ptr = (OSAL_MsgQGrpId)group_ptr;
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_msgQGrpDelete() ========
 *
 * This function is used to delete a message queue group.
 *
 * Returns:
 *  OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status OSAL_msgQGrpDelete(
    OSAL_MsgQGrpId *g_ptr)
{
    _OSAL_MsgGrpQParams    *group_ptr;

    group_ptr = (_OSAL_MsgGrpQParams *) (*g_ptr);

    if ((NULL == g_ptr) || (NULL == group_ptr)) {
        return (OSAL_FAIL);
    }

    return (OSAL_memFree(group_ptr, 0));
}

/*
 * ======== OSAL_msgQGrpAddQ() ========
 *
 * This function is used to add a queue to a message queue group.
 * Dynamically remove message queues from a group is not allowed.
 * To change a q group queues, simply delete it and make a new one.
 *
 * Returns:
 *  OSAL_SUCCESS or OSAL_FAIL
 */
OSAL_Status OSAL_msgQGrpAddQ(
    OSAL_MsgQGrpId *g_ptr,
    OSAL_MsgQId     qId)
{
    _OSAL_MsgQParams       *q_ptr = (_OSAL_MsgQParams *)qId;
    _OSAL_MsgGrpQParams    *group_ptr;
    int                     q;

    if ((NULL == q_ptr) || (NULL == g_ptr)) {
        return (OSAL_FAIL);
    }

    group_ptr = (_OSAL_MsgGrpQParams *) (*g_ptr);
    if (NULL == group_ptr) {
        return (OSAL_FAIL);
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        /*
         * Already in.
         */
        if (group_ptr->qId[q] == qId) {
            return (OSAL_SUCCESS);
        }
    }

    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        /*
         * Find free location.
         */
        if (NULL == group_ptr->qId[q]) {
            /*
             * Add
             */
            group_ptr->qId[q] = q_ptr;
            group_ptr->fid[q] = q_ptr->fid;
            group_ptr->sz[q]  = q_ptr->sz;
            return (OSAL_SUCCESS);
        }
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_msgQGrpRecv() ========
 *
 * This function is used to get a message from a message queue group.
 *
 * Returns:
 *  Number of octets read. Also return qId for queue from which octets
 *  received.
 */
int32 OSAL_msgQGrpRecv(
    OSAL_MsgQGrpId *g_ptr,
    void           *buffer_ptr,
    uint32          bufferSz,
    uint32          msTimeout,
    OSAL_MsgQId    *qId_ptr,
    OSAL_Boolean   *timeout_ptr)
{
    int                  ret;
    int32                status;
    OSAL_SelectSet       set;
    OSAL_SelectTimeval   time;
    OSAL_Boolean         test;
    int                  q;
    int                  fid;
    _OSAL_MsgGrpQParams *group_ptr;

    status = -1;
    if (NULL != timeout_ptr) {
        *timeout_ptr = OSAL_FALSE;
    }

    if (NULL == g_ptr) {
        return(status);
    }

    group_ptr = (_OSAL_MsgGrpQParams *) (*g_ptr);
    if (NULL == group_ptr) {
        return (OSAL_FAIL);
    }

    OSAL_selectSetInit(&set);

    /*
     * Run loop on each q.
     */
    ret = 0;
    for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
        fid = group_ptr->fid[q];
        if (0 > fid) {
            /*
             * This fid is invalid, but we may want to receive on others
             */
            continue;
        }

        /*
         * Set to block or no block.
         */
        switch (msTimeout) {
            case OSAL_NO_WAIT:
                if (OSAL_FAIL == _OSAL_msgQSetBlocking(fid,
                        OSAL_FALSE)) {
                    continue;
                }
                break;
            default:
                if (OSAL_FAIL == _OSAL_msgQSetBlocking(fid,
                        OSAL_TRUE)) {
                    continue;
                }
                break;
        }
        ret++;
        OSAL_selectAddId(&fid, &set);
    }

    /*
     * no queues in group. Return error.
     */
    if (0 == ret) {
        return(status);
    }

    switch (msTimeout) {
        case OSAL_WAIT_FOREVER:
            if (OSAL_FAIL == OSAL_select(&set,
                    (OSAL_SelectSet *)0,
                    NULL,
                    NULL)) {
                break;
            }
            for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
                fid = group_ptr->fid[q];
                if (0 > fid) {
                    continue;
                }
                test = OSAL_FALSE;
                if (OSAL_FAIL == OSAL_selectIsIdSet(&fid,
                    &set,
                    &test)) {
                    continue;
                }
                if (OSAL_TRUE == test) {
                    if (bufferSz < group_ptr->sz[q]) {
                        continue;
                    }
                    ret = read(fid,
                            buffer_ptr,
                            group_ptr->sz[q]);
                    if (ret > 0) {
                        status = ret;
                        *qId_ptr = group_ptr->qId[q];
                        break;
                    }
                }
            }
            break;
        case OSAL_NO_WAIT:
            /*
             * non blocking for no wait
             */
            for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
                fid = group_ptr->fid[q];
                if (0 > fid) {
                    continue;
                }
                if (bufferSz < group_ptr->sz[q]) {
                    continue;
                }
                ret = read(fid,
                        buffer_ptr,
                        group_ptr->sz[q]);
                if (ret > 0) {
                    status = ret;
                    *qId_ptr = group_ptr->qId[q];
                    break;
                }
            }
            break;
        default:
            time.sec  = msTimeout / 1000;
            msTimeout %= 1000;
            time.usec = msTimeout * 1000;
            test = OSAL_FALSE;
            if (OSAL_FAIL == OSAL_select(&set,
                    (OSAL_SelectSet *)0,
                    &time,
                    &test)) {
                break;
            }
            if (NULL != timeout_ptr) {
                *timeout_ptr = test;
            }
            for (q = 0; q < OSAL_MSG_GROUP_MAX_QUEUES; q++) {
                fid = group_ptr->fid[q];
                if (0 > fid) {
                    continue;
                }
                test = OSAL_FALSE;
                if (OSAL_FAIL == OSAL_selectIsIdSet(&fid,
                    &set,
                    &test)) {
                    continue;
                }
                if (OSAL_TRUE == test) {
                    if (bufferSz < group_ptr->sz[q]) {
                        continue;
                    }
                    ret = read(fid,
                            buffer_ptr,
                            group_ptr->sz[q]);
                    if (ret > 0) {
                        status = ret;
                        *qId_ptr = group_ptr->qId[q];
                        break;
                    }
                }
            }
            break;
    }

    return(status);
}
