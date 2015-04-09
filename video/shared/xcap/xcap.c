/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 *
 */

#include <osal.h>
#include "_xcap_task.h"
#include "_xcap_dbg.h"
#include "xcap_resources.h"
#include "xcap.h"
#ifdef INCLUDE_GBA
#include "_xcap_xact_gaa.h"
#include "_xcap_cmd_gaa.h"
#endif
/*
 * ======== _XCAP_deallocate() ========
 *
 * Internal routine for free up the XCAP module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _XCAP_deallocate(
     XCAP_Obj *obj_ptr)
{
#ifdef INCLUDE_GBA
    if (NULL != obj_ptr->nafSession_ptr) {
        GAA_freeNafSession(obj_ptr->nafSession_ptr);
        obj_ptr->nafSession_ptr = NULL;
    }
#endif

    OSAL_msgQDelete(obj_ptr->cmdq);
    OSAL_msgQDelete(obj_ptr->evtq);
    
    return (OSAL_SUCCESS);
}

/*
 * ======== _XCAP_stop() ========
 *
 * Internal routine for stoping the XCAP module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _XCAP_stop(
     XCAP_Obj *obj_ptr)
{
    OSAL_taskDelete(obj_ptr->tid);
    
    return (OSAL_SUCCESS);
}

/*
 * ======== XCAP_allocate() ========
 *
 * Public routine for allocating the XCAP module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint XCAP_allocate(
    XCAP_Obj *obj_ptr,
    int       timeoutsec)
{
    /*
     * Create command queue
     */
    obj_ptr->cmdq = OSAL_msgQCreate(XCAP_CMD_Q_NAME,
            OSAL_MODULE_XCAP, OSAL_MODULE_XCAP, OSAL_DATA_STRUCT_XCAP_Cmd,
            _XCAP_TASK_CMD_Q_LEN,
            sizeof(XCAP_Cmd),
            0);
    if (NULL == obj_ptr->cmdq) {
        return (-1);
    }

    /*
     * Create event queue
     */
    obj_ptr->evtq = OSAL_msgQCreate(XCAP_EVT_Q_NAME,
            OSAL_MODULE_XCAP, OSAL_MODULE_XCAP, OSAL_DATA_STRUCT_XCAP_Evt,
            _XCAP_TASK_EVT_Q_LEN,
            sizeof(XCAP_Evt),
            0);
    if (NULL == obj_ptr->evtq) {
        OSAL_msgQDelete(obj_ptr->cmdq);
        return (-1);
    }
    
    obj_ptr->exit = 0;
    obj_ptr->timeoutsec = timeoutsec;
    
    return (OSAL_SUCCESS);
}

/*
 * ======== XCAP_start() ========
 *
 * Public routine for starting the XCAP module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint XCAP_start(
     XCAP_Obj *obj_ptr)
{
    /*
     * Create task
     */

    obj_ptr->tid = OSAL_taskCreate(XCAP_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            _XCAP_TASK_STACK_BYTES,
            _XCAP_task,
            (void *)obj_ptr);
    if (NULL == obj_ptr->tid) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== XCAP_destroy() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint XCAP_destroy(
    XCAP_Obj *obj_ptr)
{
    _XCAP_stop(obj_ptr);
    _XCAP_deallocate(obj_ptr);

    _XCAP_DBG(__FILE__, __LINE__);

    return (OSAL_SUCCESS);
}

/* 
 * ======== XCAP_init() ========
 */
int XCAP_init(
    XCAP_Obj *obj_ptr,
    int       timeoutsec)
{
    if (NULL == obj_ptr) {
        return (OSAL_FAIL);
    }

    XCAP_allocate(obj_ptr, timeoutsec);
    XCAP_start(obj_ptr);
    
    return (OSAL_SUCCESS);
}

/* 
 * ======== XCAP_shutdown() ========
 */
int XCAP_shutdown(
     XCAP_Obj *obj_ptr)
{
    XCAP_Cmd cmd;

    if (NULL == obj_ptr) {
        return (OSAL_FAIL);
    }

    obj_ptr->exit = 1;
    XCAP_sendCmd(obj_ptr, &cmd);
    
    return (OSAL_SUCCESS);
}

/* 
 * ======== XCAP_sendCmd() ========
 */
int XCAP_sendCmd(
     XCAP_Obj *obj_ptr,
     XCAP_Cmd *cmd_ptr)
{
    if (NULL == obj_ptr) {
        return (OSAL_FAIL);
    }
    if (NULL == cmd_ptr) {
        return (OSAL_FAIL);
    }

    if (OSAL_SUCCESS == OSAL_msgQSend(obj_ptr->cmdq,
            (char *)cmd_ptr,
            sizeof(XCAP_Cmd),
            OSAL_WAIT_FOREVER,
            NULL)) {
        _XCAP_DBG(__FILE__, __LINE__);
        return (OSAL_SUCCESS);
    }
    _XCAP_DBG(__FILE__, __LINE__);
    return (OSAL_FAIL);
}

/* 
 * ======== XCAP_getEvt() ========
 */
int XCAP_getEvt(
     XCAP_Obj *obj_ptr,
     XCAP_Evt *evt_ptr,
     int       msTimeout)
{
    if (NULL == obj_ptr) {
        return (OSAL_FAIL);
    }
    if (NULL == evt_ptr) {
        return (OSAL_FAIL);
    }

    evt_ptr->hdr_ptr = NULL;
    evt_ptr->body_ptr = NULL;
    evt_ptr->error = XCAP_EVT_ERR_LAST;
    if (sizeof(XCAP_Evt) != OSAL_msgQRecv(obj_ptr->evtq,
            (char *)evt_ptr,
            sizeof(XCAP_Evt),
            msTimeout < 0 ? OSAL_WAIT_FOREVER : msTimeout,
            NULL)) {
        return (OSAL_FAIL);
    }
    _XCAP_DBG(__FILE__, __LINE__);
    return (OSAL_SUCCESS);
}

/* 
 * ======== XCAP_disposeEvt() ========
 */
int XCAP_disposeEvt(
     XCAP_Evt *evt_ptr)
{
    if (NULL == evt_ptr) {
        return (OSAL_FAIL);
    }

    /*
     * Free body only for new http lib
     */
    if (NULL != evt_ptr->body_ptr) {
        OSAL_memFree(evt_ptr->body_ptr,
                OSAL_MEM_ARG_DYNAMIC_ALLOC);
        evt_ptr->hdr_ptr = NULL;
    }
    _XCAP_DBG(__FILE__, __LINE__);
    return (OSAL_SUCCESS);
}
