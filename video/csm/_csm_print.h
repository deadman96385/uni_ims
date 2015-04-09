/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 21647 $ $Date: 2013-07-31 09:17:31 +0800 (Wed, 31 Jul 2013) $
 */

#ifndef _CSM_ISI_PRINT_H_
#define _CSM_ISI_PRINT_H_

#include <csm_event.h>
#include "_csm_event.h"

#define CSM_ISI_PRINT_OUTPUT_BUFFER_MAX_SIZE (4095)

char* CSM_isiPrintReturnString(
    ISI_Return r);

void CSM_isiPrintEvent(
    ISI_Id     serviceId,
    ISI_Id     id,
    ISI_IdType idType,
    ISI_Event  event,
    char      *eventDesc_ptr);

void CSM_isiPrintTelEvt(
    ISI_TelEvent event,
    char        *from_ptr,
    char        *dateTime_ptr,
    uint32       arg0,
    uint32       arg1);

void CSM_isiPrintCallerId(
    char *from_ptr,
    char *subject_ptr);

char *CSM_getCallReasonString(
    CSM_CallReason reason);

void CSM_isiPrintIm(
    char  *from_ptr,
    ISI_Id messageId,
    char  *subject_ptr,
    char  *message_ptr,
    char  *dateTime_ptr,
    int    report,
    char  *reportId_ptr);

#endif // _CSM_ISI_PRINT_H_
