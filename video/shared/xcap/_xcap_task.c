/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 *
 */

#include <osal.h>
#include "xcap.h"
#include "xcap_resources.h"
#include "_xcap_dbg.h"
#include "_xcap_xact.h"
#include "_xcap_cmd.h"
#include "_xcap_task.h"
#ifdef INCLUDE_GBA
#include "_xcap_xact_gaa.h"
#include "_xcap_cmd_gaa.h"
#endif

/*
 * ======== _XCAP_doXact() ========
 *
 * Do one xcap transaction requested by the cmd_ptr.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static vint _XCAP_doXact(
    XCAP_Obj    *obj_ptr,
    XCAP_Cmd    *cmd_ptr)
{
    XCAP_Evt         evt;
    _XCAP_XactObj    xobj;

    xobj.uri_ptr = cmd_ptr->uri_ptr;


    /* setup the user info */
    xobj.username_ptr = cmd_ptr->username_ptr;
    xobj.password_ptr = cmd_ptr->password_ptr;
    xobj.userAgent_ptr = cmd_ptr->userAgent_ptr;
    xobj.x3gpp_ptr = cmd_ptr->x3gpp_ptr;
    xobj.infcAddr_ptr = cmd_ptr->infcAddr_ptr;
    /*
     * Common stuff.
     * ETag conditional.
     */
    if (NULL != cmd_ptr->etag_ptr) {
        if (XCAP_CONDITION_IF_MATCH == cmd_ptr->cond) {
            xobj.etag_ptr = cmd_ptr->etag_ptr;
            xobj.ifnm_ptr = XCAP_IF_MATCH;
        }
        else if (XCAP_CONDITION_IF_MATCH == cmd_ptr->cond) {
            xobj.etag_ptr = cmd_ptr->etag_ptr;
            xobj.ifnm_ptr = XCAP_IF_NONE_MATCH;
        }
    }
    /*
     * A doc location (can be NULL for get and delete ops)
     */
    xobj.src_ptr = cmd_ptr->src_ptr;
    xobj.srcSz = cmd_ptr->srcSz;

    /*
     * Process cmd_ptr->
     */
    xobj.reply.hdr_ptr = NULL;
    xobj.reply.body_ptr = NULL;
    xobj.reply.hdrBufSz = 0;
    xobj.reply.bodyBufSz = 0;
    evt.error = XCAP_EVT_ERR_NONE;


    if (!_XCAP_xactInit(&xobj, obj_ptr->timeoutsec)) {
        evt.error = XCAP_EVT_ERR_HTTP;
        _XCAP_DBG(__FILE__, __LINE__);
    }
    else if(!_XCAP_cmdRun(cmd_ptr, &xobj)) {
        evt.error = XCAP_EVT_ERR_NET;
        _XCAP_DBG(__FILE__, __LINE__);
    }

    /*
     * Put header and body in event.
     * Note: Since body and header can be large XML and not XML docs, it is
     * better to allocate them on heap and then let the application free the
     * memory after use, but to save copy, never pass data.
     */
    evt.hdr_ptr = xobj.reply.hdr_ptr;
    evt.body_ptr = xobj.reply.body_ptr;

    /*
     * If already failed, then send error.
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(obj_ptr->evtq,
            (char *)&evt,
            sizeof(XCAP_Evt),
            OSAL_WAIT_FOREVER,
            NULL)) {
        _XCAP_DBG(__FILE__, __LINE__);
        /*
         * Since failed to send event, save memory leak by freeing the event.
         */
        OSAL_memFree(evt.hdr_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        OSAL_memFree(evt.body_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    }
    if (evt.error != XCAP_EVT_ERR_HTTP) {
        _XCAP_xactShutdown(&xobj);
    }

    return (1);
}

#ifdef INCLUDE_GBA
/*
 * ======== _XCAP_doXactGaa() ========
 *
 * Do one xcap transaction requested by the cmd_ptr with GAA authentication.
 *
 * Returns:
 *  0 failed, 1 success.
 */
static vint _XCAP_doXactGaa(
    XCAP_Obj        *obj_ptr,
    XCAP_Cmd        *cmd_ptr,
    GAA_NafSession  *nafSession_ptr)
{
    _XCAP_XactGaaObj    xobj;
    XCAP_Evt         evt;

    xobj.uri_ptr = cmd_ptr->uri_ptr;

    /* setup the custom info */
    xobj.userAgent_ptr = cmd_ptr->userAgent_ptr;
    xobj.x3gpp_ptr = cmd_ptr->x3gpp_ptr;
    xobj.infcAddr_ptr = cmd_ptr->infcAddr_ptr;

    /*
     * Common stuff.
     * ETag conditional.
     */
    if (NULL != cmd_ptr->etag_ptr) {
        if (XCAP_CONDITION_IF_MATCH == cmd_ptr->cond) {
            xobj.etag_ptr = cmd_ptr->etag_ptr;
            xobj.ifnm_ptr = XCAP_IF_MATCH;
        }
        else if (XCAP_CONDITION_IF_MATCH == cmd_ptr->cond) {
            xobj.etag_ptr = cmd_ptr->etag_ptr;
            xobj.ifnm_ptr = XCAP_IF_NONE_MATCH;
        }
    }
    /*
     * A doc location (can be NULL for get and delete ops)
     */
    xobj.src_ptr = cmd_ptr->src_ptr;
    xobj.srcSz = cmd_ptr->srcSz;

    /*
     * Process cmd_ptr->
     */
    xobj.reply.hdr_ptr = NULL;
    xobj.reply.body_ptr = NULL;
    xobj.reply.hdrBufSz = 0;
    xobj.reply.bodyBufSz = 0;
    evt.error = XCAP_EVT_ERR_NONE;

    xobj.isSessionStarted = OSAL_FALSE;

    if (!_XCAP_xactGaaInit(&xobj, obj_ptr->timeoutsec)) {
        evt.error = XCAP_EVT_ERR_HTTP;
        _XCAP_DBG(__FILE__, __LINE__);
    }
    else if(!_XCAP_cmdGaaRun(cmd_ptr, &xobj)) {
        evt.error = XCAP_EVT_ERR_NET;
        _XCAP_DBG(__FILE__, __LINE__);
    }

    /*
     * Put header and body in event.
     * Note: Since body and header can be large XML and not XML docs, it is
     * better to allocate them on heap and then let the application free the
     * memory after use, but to save copy, never pass data.
     */
    evt.hdr_ptr = xobj.reply.hdr_ptr;
    evt.body_ptr = xobj.reply.body_ptr;

    /*
     * If already failed, then send error.
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(obj_ptr->evtq,
            (char *)&evt,
            sizeof(XCAP_Evt),
            OSAL_WAIT_FOREVER,
            NULL)) {
        _XCAP_DBG(__FILE__, __LINE__);
        /*
         * Since failed to send event, save memory leak by freeing the event.
         */
        OSAL_memFree(evt.hdr_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        OSAL_memFree(evt.body_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    }
    if (evt.error != XCAP_EVT_ERR_HTTP) {
        _XCAP_xactGaaShutdown(&xobj);
    }

    return (1);
}
#endif

 /*
 * ======== _XCAP_prepare() ========
 *
 * Internal routine for doing the XCAP startup actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _XCAP_prepare(
     XCAP_Obj        *obj_ptr)
{
    /* at task thread time and all modules are allocated */
    
#ifdef INCLUDE_GBA
    /* reuse the session */
    obj_ptr->nafSession_ptr = GAA_newNafSession(XCAP_APP_NAME);
#endif

    return (OSAL_SUCCESS);
}

/*
 * ======== _XCAP_task() ========
 *
 * This is one and only task of XCAP. It receives sequentially, commands from
 * application, processes these commands, then returns events sequentially.
 *
 * Returns:
 *  Never returns.
 *
 */
OSAL_TaskReturn _XCAP_task(
    OSAL_TaskArg arg_ptr)
{
    XCAP_Obj        *obj_ptr = (XCAP_Obj *)arg_ptr;
    int              len;
    XCAP_Cmd         cmd;


    /*
     * Define xobj on stack because we only allow one transaction at a time.
     */

    _XCAP_prepare(obj_ptr);

_XCAP_TASK_LOOP:
    /*
     * Receive a valid command.
     */
    len = OSAL_msgQRecv(obj_ptr->cmdq,
            (char *)&cmd,
            sizeof(cmd),
            OSAL_WAIT_FOREVER,
            NULL);

    /*
     * Check if exit command.
     */
    if (obj_ptr->exit) {
        goto _XCAP_TASK_EXIT;
    }

    /*
     * validate, nothing can be done without location uri.
     */
    if ((len != sizeof(cmd)) || (NULL == cmd.uri_ptr)) {
        OSAL_taskDelay(100);
        goto _XCAP_TASK_LOOP;
    }

#ifdef INCLUDE_GBA
    if (NULL == obj_ptr->nafSession_ptr) {
        _XCAP_doXact(obj_ptr, &cmd);
    } else {
        _XCAP_doXactGaa(obj_ptr, &cmd, obj_ptr->nafSession_ptr);
    }
#else
    _XCAP_doXact(obj_ptr, &cmd);
#endif

    goto _XCAP_TASK_LOOP;

_XCAP_TASK_EXIT:

    _XCAP_deallocate(obj_ptr);

    _XCAP_DBG(__FILE__, __LINE__);

    return (0);
}
