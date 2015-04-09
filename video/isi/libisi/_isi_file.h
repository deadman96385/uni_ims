/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-07 17:10:49 -0500 (Wed, 07 Jul 2010) $
 *
 */

#ifndef __ISI_FILE_H__
#define __ISI_FILE_H__

void ISIFT_protoMsg(
    ISI_Id        serviceId, 
    ISIP_Message *msg_ptr);

void ISIFT_appMsg(
    ISID_FileId  *file_ptr,
    ISIP_Message *msg_ptr);

#endif
