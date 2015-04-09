/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7778 $ $Date: 2008-10-06 09:42:56 -0400 (Mon, 06 Oct 2008) $
 *
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"

void UT_printRetVal(
        int v)
{
    OSAL_logMsg("[%d] ", v);
    switch (v) {
        case VTSP_OK:
           OSAL_logMsg("Operation successfully completed.\n");
           break;
        case VTSP_E_INIT:
           OSAL_logMsg("Initialization software error or software requires re-initialization.\n");
           break;
        case VTSP_E_HW:
           OSAL_logMsg("General Hardware error.\n");
           break;
        case VTSP_E_CONFIG:
           OSAL_logMsg("Configuration error.\n");
           break;
        case VTSP_E_SHUTDOWN:
           OSAL_logMsg("Shutdown error, or re-initialization error.\n");
           break;
        case VTSP_E_TEMPL_NUM:
           OSAL_logMsg("Template number used was invalid.\n");
           break;
        case VTSP_E_TEMPL:
           OSAL_logMsg("Template structure used was invalid.\n");
           break;
        case VTSP_E_TIMEOUT:
           OSAL_logMsg("A blocking function called with a timeout has timed out.\n");
           break;
        case VTSP_E_NO_MSG:
           OSAL_logMsg("No message available from the event function.\n");
           break;
        case VTSP_E_INFC:
           OSAL_logMsg("Physical interface number used was invalid.\n");
           break;
        case VTSP_E_INFC_HW:
           OSAL_logMsg("Physical interface hardware is not correct for desired operation (such as an FXS-only operation attempted on an FXO).\n");
           break;
        case VTSP_E_STREAM_NUM:
           OSAL_logMsg("Stream number is invalid.\n");
           break;
        case VTSP_E_STREAM_DATA:
           OSAL_logMsg("Stream data structure is invalid, or stream is ended.\n");
           break;
        case VTSP_E_STREAM_NA:
           OSAL_logMsg("Stream modification for this modification type is not applicable.\n");
           break;
        case VTSP_E_RESOURCE:
           OSAL_logMsg("All existing resources are in use, or the requested resource is in use.\n");
           break;
        case VTSP_E_FIFO_ERROR:
           OSAL_logMsg("Voice payload FIFO buffer error.\n");
           break;
        case VTSP_E_JB_ERROR:
           OSAL_logMsg("Voice payload jitter buffer error.\n");
           break;
    }
}



