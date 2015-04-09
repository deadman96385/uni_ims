/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 21647 $ $Date: 2013-07-31 09:17:31 +0800 (Wed, 31 Jul 2013) $
 */

#ifndef _CSM_ISI_SMS_H_
#define _CSM_ISI_SMS_H_

#include "_csm_sms.h"

#ifdef __cplusplus
extern "C" {
#endif

void _CSM_isiSmsSend(
    CSM_SmsEvt      *smsEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

#ifdef __cplusplus
}
#endif

#endif /* _CSM_ISI_SMS_H_ */
