/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

#include <osal.h>
#include "xcap.h"
#include "_xcap_xact.h"
#include "_xcap_dbg.h"

/* 
 * ======== _XCAP_cmdCreateReplaceRun() ========
 * This function calls create or replace transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdCreateReplaceRun(
    XCAP_Cmd      *cmd_ptr,
    _XCAP_XactObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__); 
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
            return(_XCAP_xactCreateOrReplaceDocument(xact_ptr,
                    cmd_ptr->auid_ptr));
        case XCAP_OPERATION_TYPE_ELEMENT:
            return(_XCAP_xactCreateOrReplaceElement(xact_ptr));
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            return(_XCAP_xactCreateOrReplaceAttribute(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdFetchRun() ========
 * This function calls fetch transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdFetchRun(
    XCAP_Cmd      *cmd_ptr,
    _XCAP_XactObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__);
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
        case XCAP_OPERATION_TYPE_ELEMENT:
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            /*
             * This depends on URI.
             */
            return(_XCAP_xactFetch(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdDeleteRun() ========
 * This function calls delete transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdDeleteRun(
    XCAP_Cmd      *cmd_ptr,
    _XCAP_XactObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__); 
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
        case XCAP_OPERATION_TYPE_ELEMENT:
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            /*
             * This depends on URI.
             */
            return(_XCAP_xactDelete(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdRun() ========
 * This function calls processes a command recived from application.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
int _XCAP_cmdRun(
    XCAP_Cmd      *cmd_ptr,
    _XCAP_XactObj *xact_ptr)
{
    /*
     * Check command operation.
     */
    switch(cmd_ptr->op) {
        case XCAP_OPERATION_DELETE:
            return (_XCAP_cmdDeleteRun(cmd_ptr,
                    xact_ptr));
            break;
        case XCAP_OPERATION_CREATE_REPLACE:
            return (_XCAP_cmdCreateReplaceRun(cmd_ptr,
                    xact_ptr));
            break;
        case XCAP_OPERATION_FETCH:
            return (_XCAP_cmdFetchRun(cmd_ptr,
                    xact_ptr));
            break;
        default:
            break;
    }

    /*
     * Failure on cmd.
     */
    return (0);
}
