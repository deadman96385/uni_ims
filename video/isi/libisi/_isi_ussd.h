/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17946 $ $Date: 2012-08-21 12:22:16 -0700 (Tue, 21 Aug 2012) $
 *
 */

#ifndef __ISI_USSD_H__
#define __ISI_USSD_H__

void ISIT_protoUssd(
    ISI_Id        textId, 
    ISIP_Message *msg_ptr);

void ISIT_appUssd(
    ISID_UssdId  *text_ptr, 
    ISIP_Message *msg_ptr);


#endif
