/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 20271 $ $Date: 2013-03-29 17:21:28 +0800 (五, 29  3月 2013) $
 */

VPR_SrSocket* _VPR_srGetSocketById(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId socketId);

OSAL_Status _VPR_srAllocSocket(
    VPR_Obj         *vpr_ptr,
    OSAL_NetSockId   socketId,
    OSAL_NetSockType type);

OSAL_Status _VPR_srFreeSocket(
    VPR_Obj        *vpr_ptr,
    OSAL_NetSockId socketId);

VPR_SrSocket* _VPR_srGetSocketByReferenceId(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId referenceId);

OSAL_Status _VPR_srProcessReceivedSipPacket(
    VPR_Obj        *vpr_ptr,
    OSAL_NetSockId socketId,
    tSipIntMsg    *pMsg,
    char          *buf_ptr,
    vint           bufLen);

OSAL_Status _VPR_srSendSip(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_srSendSipError(
    VPR_Comm         *comm_ptr,
    VPR_NetStatusType statusType);

OSAL_Status _VPR_srCreateSipSocket(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_srSendIsipToIsi(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_srAddCallId(
    VPR_Obj  *vpr_ptr,
    char     *id_ptr);

OSAL_Status _VPR_srRemoveCallId(
    VPR_Obj  *vpr_ptr,
    char     *id_ptr);

OSAL_Status _VPR_srSendRegSecCfg(
    char           *preconfiguredRoute,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc);

