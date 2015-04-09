/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_RESOLV_H_
#define _SIP_RESOLV_H_

vint TRANSPORT_GetHostByUri (
    tTransport      *pTrans,
    tUri            *pUri,
    vint             useNaptr,
    tTransportType  *pTransport);

vint _TRANSPORT_GetTransport (
    tTransport     *pTrans,
    tUri           *pUri,
    tTransportType *pTransport,
    vint            useNaptr);

#endif
