/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include "xcap.h"
#include "_xcap_xact_gaa.h"
#include "_xcap_dbg.h"

/* 
 * ======== _XCAP_cmdGaaGaaCreateReplaceRun() ========
 * This function calls create or replace transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdGaaCreateReplaceRun(
    XCAP_Cmd         *cmd_ptr,
    _XCAP_XactGaaObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__); 
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
            return(_XCAP_xactGaaCreateOrReplaceDocument(xact_ptr,
                    cmd_ptr->auid_ptr));
        case XCAP_OPERATION_TYPE_ELEMENT:
            return(_XCAP_xactGaaCreateOrReplaceElement(xact_ptr));
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            return(_XCAP_xactGaaCreateOrReplaceAttribute(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdGaaFetchRun() ========
 * This function calls fetch transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdGaaFetchRun(
    XCAP_Cmd         *cmd_ptr,
    _XCAP_XactGaaObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__);
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
        case XCAP_OPERATION_TYPE_ELEMENT:
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            /*
             * This depends on URI.
             */
            return(_XCAP_xactGaaFetch(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdGaaDeleteRun() ========
 * This function calls delete transaction functions.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_cmdGaaDeleteRun(
    XCAP_Cmd         *cmd_ptr,
    _XCAP_XactGaaObj *xact_ptr)
{
    _XCAP_DBG(__FILE__, __LINE__); 
    switch(cmd_ptr->opType) {
        case XCAP_OPERATION_TYPE_DOCUMENT:
        case XCAP_OPERATION_TYPE_ELEMENT:
        case XCAP_OPERATION_TYPE_ATTRIBUTE:
            /*
             * This depends on URI.
             */
            return(_XCAP_xactGaaDelete(xact_ptr));
        default:
            break;
    }
    return (0);
}

/* 
 * ======== _XCAP_cmdGaaRun() ========
 * This function calls processes a command recived from application.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
int _XCAP_cmdGaaRun(
    XCAP_Cmd         *cmd_ptr,
    _XCAP_XactGaaObj *xact_ptr)
{
    /*
     * Check command operation.
     */
    switch(cmd_ptr->op) {
        case XCAP_OPERATION_DELETE:
            return (_XCAP_cmdGaaDeleteRun(cmd_ptr,
                    xact_ptr));
            break;
        case XCAP_OPERATION_CREATE_REPLACE:
            return (_XCAP_cmdGaaCreateReplaceRun(cmd_ptr,
                    xact_ptr));
            break;
        case XCAP_OPERATION_FETCH:
            return (_XCAP_cmdGaaFetchRun(cmd_ptr,
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
