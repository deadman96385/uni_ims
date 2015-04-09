/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-07 17:10:49 -0500 (Wed, 07 Jul 2010) $
 */

#ifndef _SAPP_CPIM_PAGE_H_
#define _SAPP_CPIM_PAGE_H_

#define CPIM_PAGE_USE_ANONYMITY (1)

vint SAPP_cpimEncodeIm(
    const char       *to_ptr,
    const char       *from_ptr,
    ISI_MessageReport report,
    const char       *messageId_ptr,
    const char       *msg_ptr,
    char            **target_ptr,
    vint             *targetLen_ptr);

vint SAPP_cpimEncodeImNotification(
    const char       *to_ptr,
    const char       *from_ptr,
    ISI_MessageReport report,
    const char       *messageId_ptr,
    char             *target_ptr,
    vint              maxTargetLen);

vint SAPP_cpimDecode(
    SAPP_ServiceObj *service_ptr,
    char            *payload_ptr,
    vint             payloadSize,
    SAPP_Event      *evt_ptr);

vint SAPP_cpimDecodeMessageOnly(
    SAPP_ServiceObj *service_ptr,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Text       *text_ptr);

void SAPP_cpimDecodeTextPlain(
    const char      *payload_ptr,
    vint             payloadLen,
    ISIP_Text       *isi_ptr);

vint SAPP_cpimEncodeIsComposing(
    const char       *to_ptr,
    const char       *from_ptr,
    char            **target_ptr,
    vint             *targetLen_ptr);

#endif
