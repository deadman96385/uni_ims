/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7544 $ $Date: 2008-09-06 07:45:05 +0800 (Sat, 06 Sep 2008) $
 *
 */
#ifndef __VTSPR_STUN_H_
#define __VTSPR_STUN_H_

#include "osal.h"
#include <vtsp.h>
#include "../vtsp/vtsp_private/_vtsp_private.h"
#include "vtspr.h"

/*
 * Prototypes
 */
vint _VTSPR_stunProcess(
    VTSPR_Queues     *q_ptr,
    VTSPR_NetObj     *net_ptr);

#endif /* __VTSPR_STUN_H_ */

