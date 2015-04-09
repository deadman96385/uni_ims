/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef __VCE_PRIVATE_H_
#define __VCE_PRIVATE_H_

#include <osal.h>
#include <h264.h>

#include "_vce_struct.h"

#define _VCE_TRACE(x, y) OSAL_logMsg("%s %d\n", x, y)

/* Task related settings. */
#define VCE_VCI_INFC_TASK_NAME     "Vce-Vci-Evt"

/*
 * VCE - VCE interface methods.
 */
vint VCE_init(
    int w,
    int h);

void VCE_shutdown(
    void);

/*
 * Task Related
 */
void _VCE_task(
    _VCE_Obj   *vce_ptr);

void _VCE_processEvt(
    _VCE_Obj   *vce_ptr,
    VCE_Event   event,
    vint        codecType,
    char        eventDesc[VCI_EVENT_DESC_STRING_SZ]);

vint _VCE_display_init(
    CODEC_Ptr   codec_ptr);

vint _VCE_camera_init(
    CODEC_Ptr   codec_ptr,
    int         w,
    int         h);

#endif /* End __VCE_PRIVATE_H_ */
