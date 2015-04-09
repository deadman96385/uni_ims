/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19348 $ $Date: 2012-12-14 15:02:57 -0800 (Fri, 14 Dec 2012) $
 */

#ifndef _CSM_USSD_H_
#define _CSM_USSD_H_

#include <osal.h>
#include <csm_event.h>
#include "_csm_isi.h"

typedef enum {
    CSM_USSD_STATE_NONE  = 0,
    CSM_USSD_STATE_ACTIVE,
} CSM_UssdState;


/* 
 * Top level class of the ussd Package 
 */
typedef struct {
    CSM_UssdState  state;
    CSM_IsiMngr   *isiMngr_ptr;
    char           remoteAddress[CSM_EVENT_STRING_SZ + 1];
    ISI_Id         serviceId;
    char           buff[CSM_USSD_STRING_SZ + 1];
} CSM_UssdMngr;

/* 
 * CSM USSD Manager package public methods 
 */
vint CSM_ussdInit(
    CSM_UssdMngr       *ussdMngr_ptr,
    CSM_IsiMngr        *isiMngr_ptr);

vint CSM_ussdProcessEvent(
    CSM_UssdMngr       *ussdMngr_ptr,
    CSM_UssdEvt        *ussdEvt_ptr,
    CSM_OutputEvent    *csmOutput_ptr);

vint CSM_ussdShutdown(
    CSM_UssdMngr       *ussdMngr_ptr);

void CSM_ussdConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputUssdEvt_ptr,
    CSM_UssdEvt        *csmUssdEvt_ptr);

#endif //_CSM_USSD_H_
