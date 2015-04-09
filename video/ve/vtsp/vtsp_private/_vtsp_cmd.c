/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
 *
 */

#include "vtsp.h"
#include "_vtsp_private.h"

VTSP_Return _VTSP_putCmd(
    uvint   infc,
    void   *data_ptr,
    vint    video)
{
    _VTSP_CmdMsg   *cmd_ptr;

    cmd_ptr = (_VTSP_CmdMsg *)data_ptr;

    /*
     * Audio commands go to vtspr, video commands to video engine
     */
    if (0 != video) { 
        if (OSAL_SUCCESS != OSAL_msgQSend(_VTSP_object_ptr->cmdQVideo,
                (char *)cmd_ptr, sizeof(_VTSP_CmdMsg), _VTSP_Q_CMD_TIMEOUT, 
                NULL)) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_RESOURCE);
        }
    }
    else {
        if (OSAL_SUCCESS != OSAL_msgQSend(_VTSP_object_ptr->cmdQ,
                (char *)cmd_ptr, sizeof(_VTSP_CmdMsg), _VTSP_Q_CMD_TIMEOUT, 
                NULL)) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_RESOURCE);
        }
    }

    return (VTSP_OK);
}

