/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19328 $ $Date: 2012-12-13 16:48:07 -0800 (Thu, 13 Dec 2012) $
 */

#ifndef _CSM_ISI_USSD_H_
#define _CSM_ISI_USSD_H_

#include "_csm_ussd.h"

#ifdef __cplusplus
extern "C" {
#endif

void _CSM_isiUssdSend(
    CSM_UssdEvt        *ussdEvt_ptr, 
    ISI_UssdType        type, 
    char               *remoteAddress, 
    ISI_Id              serviceId,
    CSM_OutputEvent    *csmOutput_ptr);

void _CSM_isiUssdTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          messageId,
    ISI_Event       event,
    const char     *desc_ptr);


#ifdef __cplusplus
}
#endif

#endif /* _CSM_ISI_USSD_H_ */
