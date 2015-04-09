/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */

#ifndef __MNS_NEG_H_
#define __MNS_NEG_H_

#define _MNS_NEG_OCTET_ALIGN "octet-align=1"

void _MNS_negDirection(
    tSdpAttrType in,
    tSdpAttrType *out_ptr);

vint _MNS_negSecurity(
    tMedia       *rmt_ptr,
    tMedia       *lcl_ptr);

vint _MNS_negCoders(
    tMedia         *in_ptr,
    tMedia         *out_ptr,
    MNS_NegType     negType);

vint _MNS_negRtcpFb(
    tMedia       *rmt_ptr,
    tMedia       *lcl_ptr);

vint _MNS_negAnswer(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr);

vint _MNS_negOffer(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr);

#endif
