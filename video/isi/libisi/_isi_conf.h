/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14352 $ $Date: 2011-03-29 06:57:46 +0800 (Tue, 29 Mar 2011) $
 *
 */

#ifndef __ISI_CONF_H__
#define __ISI_CONF_H__

void ISIF_protoMsg(
    ISI_Id        confId, 
    ISIP_Message *msg_ptr);

ISI_Return ISIF_appMsg(
    ISID_ConfId   *conf_ptr, 
    ISIP_Message  *msg_ptr);

#endif
