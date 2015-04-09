/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19328 $ $Date: 2012-12-14 08:48:07 +0800 (Fri, 14 Dec 2012) $
 */


#ifndef _CSM_ISI_TEL_H_
#define _CSM_ISI_TEL_H_

#include <osal.h>

/*
 * Private methods for CSM ISI Tel sub package
 */
void _CSM_isiTelTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          messageId,
    ISI_Event       event,
    const char     *desc_ptr);

#endif /* _CSM_ISI_TEL_H_ */
