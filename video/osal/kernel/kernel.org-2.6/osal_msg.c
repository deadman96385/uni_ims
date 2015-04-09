/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004,2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 7595 $ $Date: 2008-09-11 09:53:05 -0400 (Thu, 11 Sep 2008) $
 *
 */

#include <osal_msg.h>
#include <osal_mem.h>

/*
 * Private Q structure.
 */
typedef struct {
    char   name[128];
    int    fid;
    uint32 sz;
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
    int fid,
    int blocking)
{
    int flags;
    mm_segment_t fs;

    fs = get_fs();
    set_fs(KERNEL_DS);

    if ((flags = OSAL_syscall3(__NR_fcntl,
            (long)fid,
            (long)F_GETFL,
            (long)0)) < 0) {
        flags = 0;
    }
    
    if (blocking) {
        flags &= ~O_NONBLOCK;
    }
    else {
        flags |= O_NONBLOCK;
    }

    if (OSAL_syscall3(__NR_fcntl,
            (long)fid,
            (long)F_SETFL,
            (long)flags) < 0) {
        set_fs(fs);
        return(OSAL_FAIL);
    }

    set_fs(fs);
    return(OSAL_SUCCESS);
}

/* 
 * ======== OSAL_msgQCreate() ========
 *
 * This function is used to create a OSAL message queue.
 *
 * srcName_ptr, dstName_ptr, and msgStructId are not used here.
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
    mm_segment_t      fs;
    int ret;
    
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

    snprintf(q_ptr->name,
            sizeof(q_ptr->name) - 1,
            "%s%s-%08x",
            OSAL_IPC_FOLDER,
            name_ptr,
            lenOfMsg);

    fs = get_fs();
    set_fs(KERNEL_DS);

    /*
     * If queue exists dont create it again
     */
    if (0 > OSAL_syscall3(__NR_access, (long)q_ptr->name, (long)0, (long)0)) {
        if ((ret = OSAL_syscall3(__NR_mknod,
                (long)q_ptr->name,
                (long)(S_IFIFO | 0666),
                (long)0)) < 0) {
            set_fs(fs);
            OSAL_memFree(q_ptr, 0);
            return(NULL);
        }
    }
   
    /*
     * open it RW 
     */
    q_ptr->fid = OSAL_syscall3(__NR_open,
            (long)q_ptr->name,
            (long)O_RDWR,
            (long)0);
    if (q_ptr->fid < 0) {
        set_fs(fs);
        OSAL_memFree(q_ptr, 0);
        return(NULL);
    }

    set_fs(fs);

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
    OSAL_MsgQId msgQId)
{
    _OSAL_MsgQParams *q_ptr = (_OSAL_MsgQParams *)msgQId;
    mm_segment_t               fs;
    
    if (NULL == q_ptr) {
        return(OSAL_FAIL);
    }
    
    fs = get_fs();
    set_fs(KERNEL_DS);
 
    if (0 != OSAL_syscall3(__NR_close, (long)q_ptr->fid, (long)0, (long)0)) {
        set_fs(fs);
        return(OSAL_FAIL);
    }

    set_fs(fs);

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
    OSAL_Status       status;
    mm_segment_t      fs;
    _OSAL_MsgQParams *q_ptr = (_OSAL_MsgQParams *)qId;
   
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
                    0)) {
                return(status);
            }
            
            fs = get_fs();
            set_fs(KERNEL_DS);
            if (q_ptr->sz == OSAL_syscall3(__NR_write, 
                    (long)q_ptr->fid,
                    (long)buffer_ptr,
                    (long)q_ptr->sz)) {
                status = OSAL_SUCCESS;
            }
            set_fs(fs);
            break;
        case OSAL_WAIT_FOREVER:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    1)) {
                return(status);
            }
            fs = get_fs();
            set_fs(KERNEL_DS);
            if (q_ptr->sz == OSAL_syscall3(__NR_write, 
                    (long)q_ptr->fid,
                    (long)buffer_ptr,
                    (long)q_ptr->sz)) {
                status = OSAL_SUCCESS;
            }
            set_fs(fs);
            break;
        default:
            /*
             * no timeout in kernel space
             */
            return(status);
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
    mm_segment_t         fs;
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
                    0)) {
                return(status);
            }
            fs = get_fs();
            set_fs(KERNEL_DS);
            ret = OSAL_syscall3(__NR_read,
                    (long)q_ptr->fid,
                    (long)buffer_ptr,
                    (long)q_ptr->sz);
            set_fs(fs);
            if (ret > 0) {
                status = ret;
            }
            break;
        case OSAL_WAIT_FOREVER:
            if (OSAL_FAIL == _OSAL_msgQSetBlocking(q_ptr->fid,
                    1)) {
                return(status);
            }
            fs = get_fs();
            set_fs(KERNEL_DS);
            ret = OSAL_syscall3(__NR_read,
                    (long)q_ptr->fid,
                    (long)buffer_ptr,
                    (long)q_ptr->sz);
            set_fs(fs);
            if (ret > 0) {
                status = ret;
            }
            break;
        default:
            return(status);
    }
    
    return(status);
}

EXPORT_SYMBOL(OSAL_msgQCreate);
EXPORT_SYMBOL(OSAL_msgQDelete);
EXPORT_SYMBOL(OSAL_msgQSend);
EXPORT_SYMBOL(OSAL_msgQRecv);
