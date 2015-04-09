/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 20271 $ $Date: 2013-03-29 17:21:28 +0800 (五, 29  3月 2013) $
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include <sip_sip.h>
#include <sip_hdrflds.h>
#include <sip_mem_pool.h>
#include "_vpr.h"

/*
 * ======== VPR_srConstructSrPacket() ========
 *
 * This function is to construct VPR command object for sip packet.
 *
 * Returns: 
 * OSAL_SUCCESS: VPR_Comm object constructed.
 * OSAL_FAIL: Error in VPR_Comm object construction.
 */
static OSAL_Status _VPR_srConstructSrPacket( 
    const uint8            *packet_ptr, 
    const uint32            packetLen, 
    const OSAL_NetAddress  *lclAddress_ptr, 
    const OSAL_NetAddress  *rmtAddress_ptr, 
    VPR_Comm               *comm_ptr,
    const OSAL_NetSockId    socketId,
    VPR_NetType             type,
    vint                    chuckNumber,
    vint                    packetEnd)
{ 
    if (VPR_NET_MAX_DATA_SIZE_OCTETS < packetLen) { 
        VPR_dbgPrintf("Packet length is too long: %d\n", packetLen);
        return (OSAL_FAIL); 
    } 
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm)); 
    comm_ptr->type = VPR_TYPE_NET; 
    comm_ptr->u.vprNet.type = type;
    comm_ptr->u.vprNet.referenceId = socketId;
    if (lclAddress_ptr) {
        comm_ptr->u.vprNet.localAddress = *lclAddress_ptr;
    }
    if (rmtAddress_ptr){
        comm_ptr->u.vprNet.remoteAddress = *rmtAddress_ptr;
    }
    comm_ptr->u.vprNet.u.packet.chunkNumber = chuckNumber;
    comm_ptr->u.vprNet.u.packet.packetEnd = packetEnd;
    comm_ptr->u.vprNet.u.packet.tosValue = 0;
    if (packet_ptr && (0 < packetLen)) {
        OSAL_memCpy(comm_ptr->u.vprNet.u.packet.packetData, packet_ptr,
                packetLen);
        comm_ptr->u.vprNet.u.packet.packetLen = packetLen;
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srSendSipPacket() ========
 *
 * Private function to send sip packet to SR.
 *
 * Returns: 
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_srSendSipPacketToSr(
    VPR_Obj         *vpr_ptr,
    VPR_SrSocket    *sock_ptr,
    const uint8     *buf_ptr,
    uint32           size,
    OSAL_NetAddress *rmtAddr_ptr)
{
    vint      sendSize;
    VPR_Comm *comm_ptr;
    vint      chunkNum;
    vint      packetEnd;

    comm_ptr = &vpr_ptr->commSrNic;
    /* Send VPR in chunks if needed */
    chunkNum = 0;
    packetEnd = 0;
    while (0 < size) {
        if (VPR_NET_MAX_DATA_SIZE_OCTETS < size) {
            sendSize = VPR_NET_MAX_DATA_SIZE_OCTETS; 
            size -= VPR_NET_MAX_DATA_SIZE_OCTETS;
        }
        else {
            sendSize = size;
            size = 0;
            packetEnd = 1;
        }

        chunkNum++;
        /* Construct sip packet */
        if (OSAL_SUCCESS != _VPR_srConstructSrPacket(
                (uint8 *)buf_ptr, sendSize, NULL, NULL, comm_ptr,
                sock_ptr->referenceId, VPR_NET_TYPE_SIP_RECV_PKT, chunkNum,
                packetEnd)) {
            VPR_dbgPrintf("Construct packet failed.\n");
            return (OSAL_FAIL);
        }
        buf_ptr += sendSize;

        /* Write to modem processor */
        if (OSAL_SUCCESS != VPMD_writeSip(comm_ptr, sizeof(VPR_Comm))) {
            VPR_dbgPrintf("Fail to write vpmd.\n");
            return (OSAL_FAIL);
        }
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srFindCallId() ========
 *
 * Private function to search a dialog id is in the dialig id list.
 *
 * Returns: 
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
OSAL_Status _VPR_srFindCallId(
    VPR_Obj *vpr_ptr,
    char    *id_ptr)
{
    vint idx;

    for (idx = 0; idx < VPR_CALL_ID_LIST_MAX_ENTRIES; idx++) {
        if (0 == OSAL_strncmp(vpr_ptr->callIdList[idx], id_ptr,
                SIP_CALL_ID_HF_STR_SIZE)) {
            /* Found it */
            VPR_dbgPrintf("Found call id %s\n", id_ptr);
            return (OSAL_SUCCESS);
        }
    }
    VPR_dbgPrintf("Cannot find call id %s\n", id_ptr);
    return (OSAL_FAIL);
}

/*
 * ======== _VPR_srAddCallId() ========
 *
 * Private function to add a dialog id is to dialig id list.
 *
 * Returns: 
 * OSAL_SUCCESS: Dialog id added.
 * OSAL_FAIL: Dialog id wasn't added.
 */
OSAL_Status _VPR_srAddCallId(
    VPR_Obj *vpr_ptr,
    char    *id_ptr)
{
    vint idx;

    /* If it is already in database, return SUCCESS. */
    if (OSAL_SUCCESS == (_VPR_srFindCallId(vpr_ptr, id_ptr))) {
        VPR_dbgPrintf("Dialog id already exists %s\n", id_ptr);
        return (OSAL_SUCCESS);
    }

    for (idx = 0; idx < VPR_CALL_ID_LIST_MAX_ENTRIES; idx++) {
        if (0 == vpr_ptr->callIdList[idx][0]) {
            /* Found an empty entry */
            OSAL_strncpy(vpr_ptr->callIdList[idx], id_ptr,
                    SIP_CALL_ID_HF_STR_SIZE);
            return (OSAL_SUCCESS);
        }
    }
            
    VPR_dbgPrintf("No free entry in call id list, list size: %d\n",
            VPR_CALL_ID_LIST_MAX_ENTRIES);
    return (OSAL_FAIL);
}

/*
 * ======== _VPR_srRemoveCallId() ========
 *
 * Private function to remove a dialog id is from dialig id list.
 *
 * Returns: 
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
OSAL_Status _VPR_srRemoveCallId(
    VPR_Obj *vpr_ptr,
    char    *id_ptr)
{
    vint idx;

    for (idx = 0; idx < VPR_CALL_ID_LIST_MAX_ENTRIES; idx++) {
        if (0 == OSAL_strncmp(vpr_ptr->callIdList[idx], id_ptr,
                SIP_CALL_ID_HF_STR_SIZE)) {
            /* Found it */
            VPR_dbgPrintf("Found call id %s\n", id_ptr);
            /* Empty the call id string */
            vpr_ptr->callIdList[idx][0] = '\0';
            return (OSAL_SUCCESS);
        }
    }
    VPR_dbgPrintf("Cannot find call id %s\n", id_ptr);
    return (OSAL_FAIL);
}

/*
 * ======== _VPR_srGetSocketById() ========
 *
 * Private function to get vpr socket index by socket id.
 *
 * Returns: 
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_SrSocket* _VPR_srGetSocketById(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId socketId)
{
    vint idx;

    if (NULL == vpr_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        if (socketId == vpr_ptr->srSockets[idx].id) {
            return (&vpr_ptr->srSockets[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_srAllocSocket() ========
 *
 * Private function to allocate a VPR_SrSocket for SR
 *
 * Returns: 
 * -1: No available vpr socket.
 * Otherwise: The index of vpr socket.
 */
OSAL_Status _VPR_srAllocSocket(
    VPR_Obj         *vpr_ptr,
    OSAL_NetSockId   socketId,
    OSAL_NetSockType type)
{
    vint             idx;

    if (NULL == vpr_ptr) {
        return (OSAL_FAIL);
    }

    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        if (VPR_SOCKET_ID_NONE == vpr_ptr->srSockets[idx].id) {
            /* Get fd from qId and set it to socketId */
            vpr_ptr->srSockets[idx].id = socketId;
            vpr_ptr->srSockets[idx].localAddress.type = type;
            return (OSAL_SUCCESS);
        }
    }
    return (OSAL_FAIL);
}

/*
 * ======== _VPR_srFreeSocket() ========
 *
 * Private function to free a VPR_SrSocket 
 *
 * Returns:
 * OSAL_SUCCESS: Success.
 * OSAL_FAIL: Failed.
 */
OSAL_Status _VPR_srFreeSocket(
    VPR_Obj        *vpr_ptr,
    OSAL_NetSockId socketId)
{
    VPR_SrSocket  *sock_ptr;

    if (NULL == vpr_ptr) {
        return (OSAL_FAIL);
    }

    if (NULL == (sock_ptr = _VPR_srGetSocketById(vpr_ptr, socketId))) {
        return (OSAL_FAIL);
    }

    /* Clean data */
    OSAL_memSet(&sock_ptr->localAddress, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&sock_ptr->remoteAddress, 0, sizeof(OSAL_NetAddress));
    sock_ptr->id = VPR_SOCKET_ID_NONE;
    sock_ptr->referenceId = VPR_SOCKET_ID_NONE;
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srGetSocketByAddress() ========
 *
 * Private function to get vpr socket by local address
 *
 * Returns: 
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_SrSocket* _VPR_srGetSocketByAddress(
    VPR_Obj         *vpr_ptr,
    OSAL_NetAddress *addr_ptr)
{
    vint idx;

    if (NULL == addr_ptr) {
        return (NULL);
    }

    /* Loop to match the reference id */
    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        if (0 != addr_ptr->port) {
            if (OSAL_netIsAddrPortEqual(&vpr_ptr->srSockets[idx].localAddress,
                    addr_ptr)) {
                /* Return an available vpr socket */
                if (VPR_SOCKET_ID_NONE == vpr_ptr->srSockets[idx].referenceId) {
                    return (&vpr_ptr->srSockets[idx]);
                }
            }
        }
        else {
            /* If port is zero, only match ip address */
            if (OSAL_netIsAddrEqual(&vpr_ptr->srSockets[idx].localAddress,
                    addr_ptr)) {
                /* Return an available vpr socket */
                if (VPR_SOCKET_ID_NONE == vpr_ptr->srSockets[idx].referenceId) {
                    return (&vpr_ptr->srSockets[idx]);
                }
            }
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_srGetAvailableSocket() ========
 *
 * Private function to get an available vpr sr socket.
 *
 * Returns: 
 * -1: No available vpr socket.
 * Otherwise: The pointer to the available vpr socket.
 */
VPR_SrSocket* _VPR_srGetAvailableSocket(
    VPR_Obj       *vpr_ptr)
{
    vint idx;

    /* Loop to find an available socket */
    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        if (VPR_SOCKET_ID_NONE == vpr_ptr->srSockets[idx].id) {
            return (&vpr_ptr->srSockets[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_srGetSocketByReferenceId() ========
 *
 * Private function to get vpr socket by reference id(vier socket id in
 * application processor).
 *
 * Returns: 
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_SrSocket* _VPR_srGetSocketByReferenceId(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId referenceId)
{
    vint idx;

    if (VPR_SOCKET_ID_NONE == referenceId) {
        return (NULL);
    }

    /* Loop to match the reference id */
    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        if (vpr_ptr->srSockets[idx].referenceId == referenceId) {
            return (&vpr_ptr->srSockets[idx]);
        }
    }

    return (NULL);
}


/*
 * ======== _VPR_srCreateSipSocket() ========
 * Private function to find or create sip socket in VPR for SR.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_srCreateSipSocket(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    VPR_Net         *net_ptr;
    VPR_SrSocket    *sock_ptr;
    OSAL_NetAddress *addr_ptr;

    VPR_dbgPrintf("\n");

    /* Find VPR_SrSocket */
    net_ptr = &comm_ptr->u.vprNet;
    if (NULL != (sock_ptr = _VPR_srGetSocketByReferenceId(vpr_ptr,
            net_ptr->referenceId))) {
        /* Found by reference Id, should not be here */
        VPR_dbgPrintf("!!!Found VPR_SrSocket by reference id. Ref id:%d, "
                "socket id:%d\n", net_ptr->referenceId, sock_ptr->id);
        return (OSAL_SUCCESS);
    }

    addr_ptr = &net_ptr->localAddress;
    if (NULL == (sock_ptr = _VPR_srGetSocketByAddress(vpr_ptr, addr_ptr))) {
        /*
         * Cannot find VPR_SrSocket via address, create one.
         */
        if (NULL == (sock_ptr = _VPR_srGetAvailableSocket(vpr_ptr))) {
            /* No free VPR_SrSocket */
            VPR_dbgPrintf("No free VPR_SrSocket for SR\n");
            return (OSAL_FAIL);
        }
        if (OSAL_FAIL == OSAL_netSocket(&sock_ptr->id,
                net_ptr->localAddress.type)) {
            VPR_dbgPrintf("Fail to create socket.\n");
            return (OSAL_FAIL);
        }
        sock_ptr->referenceId = net_ptr->referenceId;
        /* Then bind it */
        if (OSAL_SUCCESS != OSAL_netBindSocket(&sock_ptr->id,
                &net_ptr->localAddress)) {
            VPR_dbgPrintf("Failed to bind ip:%d, port:%d\n",
                    net_ptr->localAddress.ipv4,
                    net_ptr->localAddress.port);
            /* Free VPR_SrSocket */
            _VPR_srFreeSocket(vpr_ptr, sock_ptr->id);
            return (OSAL_FAIL);
        }
        /* Get port we bound */
        if (OSAL_FAIL == OSAL_netGetSocketAddress(&sock_ptr->id,
                &sock_ptr->localAddress)) {
            VPR_dbgPrintf("Failed to get address and port\n");
            /* Free VPR_SrSocket */
            _VPR_srFreeSocket(vpr_ptr, sock_ptr->id);
        }
    }
    else {
        /* Then we found the socket via address */
        sock_ptr->referenceId = net_ptr->referenceId;
        VPR_dbgPrintf("Found VPR_SrSocket via address. Ref id:%d, "
                "socket id:%d\n", net_ptr->referenceId, sock_ptr->id);
    }
        
    /* Send to SR to notify the port, reuse comm_ptr */
    if (OSAL_SUCCESS ==_VPR_srConstructSrPacket(
            NULL, 0, &sock_ptr->localAddress, NULL, comm_ptr,
            sock_ptr->referenceId, net_ptr->type, 1, 1)) {
        VPMD_writeSip(comm_ptr, sizeof(VPR_Comm));
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srSendIsipToIsi() ========
 * Private function to send isip message to ISI.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_srSendIsipToIsi(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    ISIP_Message *msg_ptr; 

    msg_ptr = &comm_ptr->u.isipMsg;

    /* See if it's system start event */
    if ((ISIP_CODE_SYSTEM == msg_ptr->code) &&
            (ISIP_SYSTEM_REASON_START == msg_ptr->msg.system.reason)) {
        /* Modify the ipc name so that isi could send message to vpr */
        OSAL_strncpy(msg_ptr->msg.system.protocolIpc,
                VPR_SR_ISIP_RECV_QUEUE_NAME,
                sizeof(msg_ptr->msg.system.protocolIpc));
    }

    /* Send to ISI */
    return (OSAL_msgQSend(vpr_ptr->queue.isipSendQ, msg_ptr,
            sizeof(ISIP_Message), OSAL_NO_WAIT, NULL));
}

/*
 * ======== _VPR_srCachePacket() ========
 * Private function to cache sip packet to buffer.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_srCachePacket(
    VPR_SrSocket *sock_ptr,
    uint8        *data_ptr,
    vint          packetLen)
{
    /* Check if buffer size is available */
    if (VPR_PACKET_BUFFER_SIZE_MAX <
            (sock_ptr->packetBuffer.length + packetLen)) {
        VPR_dbgPrintf("Packet size too large %d.\n",
                sock_ptr->packetBuffer.length + packetLen);
        return (OSAL_FAIL);
    }
    /* Copy data */
    OSAL_memCpy(sock_ptr->packetBuffer.data + sock_ptr->packetBuffer.length,
            data_ptr, packetLen);
    sock_ptr->packetBuffer.length += packetLen;

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srSendSip() ========
 * Private function to send SR sip packet through VPR.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_srSendSip(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    VPR_Net      *net_ptr;
    VPR_SrSocket *sock_ptr;
    uint8        *data_ptr;
    vint          dataLen;
    OSAL_Status   ret;

    /* Find VPR_SrSocket */
    net_ptr = &comm_ptr->u.vprNet;
    if (NULL == (sock_ptr = _VPR_srGetSocketByReferenceId(vpr_ptr,
            net_ptr->referenceId))) {
        /* Cannot find VPR_SrSocket */
        VPR_dbgPrintf("Failed to find VPR_SrSocket. Ref id:%d\n",
                net_ptr->referenceId);
        return (OSAL_FAIL);
    }

    /* Cache data if it's not end packet */
    if (!net_ptr->u.packet.packetEnd) {
        /* Still has packet(s), cache it */
        if (OSAL_SUCCESS != _VPR_srCachePacket(sock_ptr,
                net_ptr->u.packet.packetData,
                net_ptr->u.packet.packetLen)) {
            return (OSAL_FAIL);
        }
        /* It's not the end of packet, don't send it now */
        return (OSAL_SUCCESS);
    }
    else {
        /* It's end of packet */
        if (1 == net_ptr->u.packet.chunkNumber) {
            /* There is only one chunk, just send it */
            data_ptr = net_ptr->u.packet.packetData;
            dataLen = net_ptr->u.packet.packetLen;
        }
        else {
            /* More than one chunk, assemble it and send */
            if (OSAL_SUCCESS != _VPR_srCachePacket(sock_ptr,
                    net_ptr->u.packet.packetData,
                    net_ptr->u.packet.packetLen)) {
                return (OSAL_FAIL);
            }
            data_ptr = sock_ptr->packetBuffer.data;
            dataLen = sock_ptr->packetBuffer.length;
        }
    }

    /* Use if remote address exists to determine calling send or sendto */
    if (OSAL_TRUE == OSAL_netIsAddrZero(&net_ptr->remoteAddress)) {
        /* See if it's SSL */
        if (NULL != sock_ptr->ssl_ptr) {
            /* It's ssl over tcp */
            ret = OSAL_netSslSend(sock_ptr->ssl_ptr,
                    data_ptr, &dataLen);
        }
        else {
            /* It's TCP */
            ret = OSAL_netSocketSend(&sock_ptr->id, data_ptr, &dataLen);
        }
    }
    else {
        /* It's UDP */
        ret = OSAL_netSocketSendTo(&sock_ptr->id,
                data_ptr, &dataLen, &net_ptr->remoteAddress);
    }

    /* Data sent, clear buffer */
    sock_ptr->packetBuffer.length = 0;
    return (ret);
}

/*
 * ======== _VPR_srSendSipError() ========
 *
 * Private function to send error message back to SR to indicate an error of sip
 * packet processing.
 *
 * Returns:
 * OSAL_SUCCESS: success to send error message to SR
 * OSAL_FAIL:    fail to send error message to SR
 */
OSAL_Status _VPR_srSendSipError(
    VPR_Comm         *comm_ptr,
    VPR_NetStatusType statusType)
{
    comm_ptr->u.vprNet.type             = VPR_NET_TYPE_ERROR;
    comm_ptr->u.vprNet.u.status.evtType = statusType;

    if (OSAL_SUCCESS != VPMD_writeSip(comm_ptr, sizeof(VPR_Comm))) {
        VPR_dbgPrintf("Fail to write sip msg to VPMD\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_srProcessReceivedSipPacket() ========
 *
 * Private function to process received sip packet.
 * This function decode the sip packet and see if the sip packet is for MSAPP or
 * ASAPP.
 * If it's for ASAPP then send it to SR and return OSAL_SUCCESS.
 * If it's not for ASAPP then return OSAL_FAIL.
 *
 * Returns: 
 * OSAL_SUCCESS: Sip packet is redirected to ASAPP, no need to send to MASAPP.
 * OSAL_FAIL: Sip packet is for MSAPP.
 */
OSAL_Status _VPR_srProcessReceivedSipPacket(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId socketId,
    tSipIntMsg    *sipMsg_ptr,
    char          *buf_ptr,
    vint           bufLen)
{ 
    VPR_SrSocket *sock_ptr;
    tSdpMedia    *sdpMedia_ptr;
    tSdpMsg      *sdp_ptr;
    vint          redirect;

    /* Search call id to see if it's a call id for MSASPP */
    if (OSAL_SUCCESS == _VPR_srFindCallId(vpr_ptr, sipMsg_ptr->szCallId)) {
        /* Found call id, it's for MSAPP, no need to redirect. */
        VPR_dbgPrintf("Found callId for MSAPP %s\n", sipMsg_ptr->szCallId);
        return (OSAL_FAIL);
    }

    redirect = 0; 
    /* Then see if it's an IM/FT request */
    if ((eSIP_REQUEST == sipMsg_ptr->msgType)) {
        switch (sipMsg_ptr->method) {
            case eSIP_INVITE:
                /* Loop sdp */
                sdp_ptr = sipMsg_ptr->pSessDescr;
                while (NULL != sdp_ptr) {
                    /* Loop media to find msrp media type */
                    sdpMedia_ptr = &sipMsg_ptr->pSessDescr->media;
                    while (NULL != sdpMedia_ptr) {
                        if (eSdpMediaMsMessage == sdpMedia_ptr->mediaType) {
                            /* Found it */
                            VPR_dbgPrintf("Found msrp media.\n");
                            redirect = 1;
                            break;
                        }
                        sdpMedia_ptr = sdpMedia_ptr->next;
                    }
                    if (1 == redirect) {
                        /* We got the msrp */
                        break;
                    }
                    sdp_ptr = sdp_ptr->next;
                }
                break;
            case eSIP_NOTIFY:
                /* 
                 * If call id in NOTIFY is not found for MSAPP then it's for
                 * ASAPP.
                 */
            case eSIP_BYE:
            case eSIP_ACK:
            case eSIP_CANCEL:
            case eSIP_PRACK:
            case eSIP_UPDATE:
                /* Call id doesn't match, shouldn't be for MSAPP */
                redirect = 1;
                break;
            case eSIP_MESSAGE:
                /* Check content type */
                if (eCONTENT_TYPE_3GPPSMS == sipMsg_ptr->ContentType) {
                    /* 3GPP SMS, send to MSAPP */
                    redirect = 0;
                }
                break;
            case eSIP_OPTIONS:
                /*
                 * Consider OPTIONS for capability exchange, redirect to ASAPP.
                 */
                redirect = 1;
                break;
            case eSIP_INFO:
                /* Currently only USSD uses INFO, don't redirect to ASAPP. */
                redirect = 0;
                break;
            case eSIP_ERROR:
                /* Let MSAPP handle ERROR, fall through. */
            default:
                /* Other requests consider them for MSAPP now. */
                redirect = 0;
                break;
        }
    }
    else {
        /* It's a response, send to both sapp */
        redirect = 1;
        VPR_dbgPrintf("It's a response.\n");
    }

    if (0 == redirect) {
        VPR_dbgPrintf("No redirect.\n");
        return (OSAL_FAIL);
    } 

    /*
     * We are here to rediect sip pakcet.
     * Check if there is ASAPP socket associated with this VPR_SrSocket
     */
    if (NULL != (sock_ptr = _VPR_srGetSocketById(vpr_ptr, socketId))) {
        /* Found VPR_SrSocket */
        if (VPR_SOCKET_ID_NONE != sock_ptr->referenceId) {
            /*
             * There is an ASAPP socket associated to this socket,
             * see if we need to send the sip packet to asapp.
             */
            if (OSAL_SUCCESS ==  _VPR_srSendSipPacketToSr(vpr_ptr, sock_ptr,
                    (uint8 *)buf_ptr, bufLen, NULL)) {
                VPR_dbgPrintf("Redirected.\n");
            }
            else {
                VPR_dbgPrintf("Fail to send sip packet.\n");
            }

            /*
             * Send response and ACK to both SAPP. It will be ignored if the
             * message is not for itself.
             */
            if ((eSIP_RESPONSE == sipMsg_ptr->msgType) ||
                    (eSIP_ACK == sipMsg_ptr->method)) {
                /* Also send to MSAPP */
                return (OSAL_FAIL);
            }
            return (OSAL_SUCCESS);
        }
        else {
            /* There is no ASAPP socket associated to this VPR_SrSocket */
            VPR_dbgPrintf("No associated ASAPP socket to redirect.\n");
            return (OSAL_FAIL);
        }
    }
    else {
        VPR_dbgPrintf("Fail to find VPR_SrSocket %d for SR.\n", socketId);
        return (OSAL_FAIL);
    }
}

/*
 * ======== _VPR_srSendRegSecCfg() ========
 *
 * Private function to send IPSec/Route configuration to SR.
 *
 * Returns:
 * OSAL_SUCCESS: success to send
 * OSAL_FAIL:    fail to send
 */
OSAL_Status _VPR_srSendRegSecCfg(
    char           *preconfiguredRoute,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc)
{
    VPR_Comm    comm;
    int         idx;
    int         numHfs;

    comm.type = VPR_TYPE_NET;
    comm.u.vprNet.type = VPR_NET_TYPE_REG_SEC_CFG;

    /* Copy route path */
    OSAL_strncpy(comm.u.vprNet.u.regsecData.preconfiguredRoute,
            preconfiguredRoute,
            sizeof(comm.u.vprNet.u.regsecData.preconfiguredRoute));

    /* Copy secSrvHfs */
    if (VPR_SEC_SRV_HF_MAX_NUM < secSrvHfNum) {
        numHfs = VPR_SEC_SRV_HF_MAX_NUM;
    }
    else {
        numHfs = secSrvHfNum;
    }
    for (idx = 0; idx < numHfs; idx++) {
        OSAL_strncpy(comm.u.vprNet.u.regsecData.secSrvHfs[idx],
                &secSrvHfs[idx * secSrvStrlen],
                sizeof(comm.u.vprNet.u.regsecData.secSrvHfs[idx]));
    }

    /* Copy IPSec SA data */
    OSAL_memCpy(&comm.u.vprNet.u.regsecData.inboundSAc, inboundSAc,
            sizeof(OSAL_IpsecSa));
    OSAL_memCpy(&comm.u.vprNet.u.regsecData.inboundSAs, inboundSAs,
            sizeof(OSAL_IpsecSa));
    OSAL_memCpy(&comm.u.vprNet.u.regsecData.outboundSAc, outboundSAc,
            sizeof(OSAL_IpsecSa));

    VPMD_writeSip(&comm, sizeof(VPR_Comm));

    return (OSAL_SUCCESS);
}

