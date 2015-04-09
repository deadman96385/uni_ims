/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17946 $ $Date: 2012-08-22 03:22:16 +0800 (Wed, 22 Aug 2012) $
 *
 */

#ifndef __ISI_TEXT_H__
#define __ISI_TEXT_H__

void ISIT_protoMsg(
    ISI_Id        serviceId, 
    ISIP_Message *msg_ptr);

void ISIT_appMsg(
    ISID_TextId  *text_ptr, 
    ISIP_Message *msg_ptr);

void ISIT_chatMsg(
    ISID_ServiceId   *service_ptr,
    ISIP_Text        *m_ptr,
    ISI_Id           *isiId_ptr);

#endif
