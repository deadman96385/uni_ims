/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29578 $ $Date: 2014-10-31 11:43:18 +0800 (Fri, 31 Oct 2014) $
 */

#ifndef __MNS_SESSION_ATTR_H_
#define __MNS_SESSION_ATTR_H_

void _MNS_negGenerateSrtpKeys(
    char              *aes80_ptr,
    char              *aes32_ptr,
    tMedia            *media_ptr);

vint _MNS_sessionSetMediaAttr(
    MNS_SessionObj  *mns_ptr,
    tSession        *lclSess_ptr,
    tSession        *rmtSess_ptr,
    MNS_NegType      negType);

vint _MNS_sessionSetSessionAttr(
    tSession        *lclSess_ptr,
    tSession        *rmtSess_ptr);

void  _MNS_sessionPopulateIsiEvt(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr,
    ISIP_Call       *isi_ptr);

#endif
