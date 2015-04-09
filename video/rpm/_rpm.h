/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef __RPM_H_
#define __RPM_H_

#include <osal.h>
#include "rpm.h"
#include "radio_fsm/rfsm.h"

/*
 * Private constant definitions
 */
#ifndef RPM_DEBUG
#define RPM_dbgPrintf(fmt, args...)
#else
#define RPM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

/*
 * Private RPM Structures
 */
typedef struct {
    RFSM_Context             radioFsm;
    RPM_notifyImsRadioChange imsChangeCb;
    RPM_notifySrvccChange    srvccChangeCb;
    RPM_notifyEmerRegChange  eRegRequiredCb;
    RPM_notifySupsrvChange   supsrvChangeCb;
    vint                     isEmergencyFailoverToCs;
    vint                     isEmergencyRegRequired;
    RPM_UrlFmt               urlFmt;
} _RPM_Obj;

_RPM_Obj* _RPM_getObject(
    void);

#endif // __RPM_H_
