/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7544 $ $Date: 2008-09-06 07:45:05 +0800 (Sat, 06 Sep 2008) $
 *
 */

#include "osal.h"

#include "vtsp.h"
#include "_vtsp_private.h"


/*
 * ======== _VTSP_isToneTemplIdValid() ========
 *
 *  check templateId variable is valid for the infc
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_ARG if invalid
 */
VTSP_Return _VTSP_isToneTemplIdValid(
    uvint infc, 
    uvint id)
{
    if (id < _VTSP_object_ptr->config.tone.numTemplateIds) {
        return (VTSP_OK);
    }

    return (VTSP_E_ARG);
}


/*
 * ======== _VTSP_isRingTemplIdValid() ========
 *
 *  check templateId variable is valid for the infc
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_ARG if invalid
 */
VTSP_Return _VTSP_isRingTemplIdValid(
    uvint infc, 
    uvint id)
{

    if (id < _VTSP_object_ptr->config.ring.numTemplateIds) {
        return (VTSP_OK);
    }

    return (VTSP_E_ARG);
}

/*
 * ======== _VTSP_putCidData() ========
 *
 *  put cid obj into ipc
 *
 */
VTSP_Return _VTSP_putCidData(
    uvint           infc, 
    VTSP_CIDData   *cid_ptr)
{
    OSAL_Boolean    timeo;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VTSP_object_ptr->infc[infc].cidQ,
            (char *)cid_ptr, 
            sizeof(_VTSP_CIDData),
            _VTSP_Q_CMD_TIMEOUT,
            &timeo)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}

/*
 * ======== _VTSP_cidFormat() ========
 *
 * Format cid data for protocol
 *
 */
VTSP_Return _VTSP_cidFormat(
    VTSP_CIDData   *cid_ptr)
{
    _VTSP_CIDData  *obj_ptr;
    char           *dst_ptr;

    obj_ptr = (_VTSP_CIDData *)cid_ptr;
    dst_ptr = (char *)&obj_ptr->data[obj_ptr->len];

    /* 
     * Check for region support, and format CID Data for this region 
     */
    switch (_VTSP_object_ptr->cidFormat) { 
        case VTSP_TEMPL_CID_FORMAT_JP:
        case VTSP_TEMPL_CID_FORMAT_DATA_JP:
            /* Add formatting for NTT */
            break;
        case VTSP_TEMPL_CID_FORMAT_UK_DTMF:
            /* DTMF protocol format */
            /* Add string end byte */
            *dst_ptr++ = 'C';
            obj_ptr->len++;
            break;
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_getCidData() ========
 *
 *  get cid obj from ipc
 *
 */
VTSP_Return _VTSP_getCidData(
    uvint           infc, 
    uint8          *cidData_ptr)
{

    if (0 >= OSAL_msgQRecv(_VTSP_object_ptr->infc[infc].cidQ,
            (char *)cidData_ptr,
            sizeof(_VTSP_CIDData), 
            0,
            NULL)) { 
        return (VTSP_E_RESOURCE);
    }
    return (VTSP_OK);
}
