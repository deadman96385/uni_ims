/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include <sip_sip.h>
#include <sip_hdrflds.h>
#include <sip_mem_pool.h>
#include "_vpr.h"
#include "_vpr_daemon.h"
#include "_vpr_sr.h"

/*
 * ======== _VPR_processVprComm() ========
 *
 * Private function to process VPR_Comm from application processor.
 *
 * Returns:
 * OSAL_SUCCESS: VPR_Comm processed.
 * OSAL_FAIL: Error in VPR_Comm processing.
 */
static OSAL_Status _VPR_processVprComm(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    VPR_Net      *net_ptr;
    VPR_Socket   *sock_ptr;
    vint         size;

    switch (comm_ptr->type) {
        case VPR_TYPE_VTSP_CMD:
            break;
        case VPR_TYPE_VTSP_EVT:
            if (OSAL_SUCCESS != _VPR_sendVtspEvt(vpr_ptr, comm_ptr)) {
                VPR_dbgPrintf("Failed to send vtsp event to vtsp.\n");
            }
            break;
        case VPR_TYPE_ISIP:
            if (OSAL_FAIL == _VPR_srSendIsipToIsi(vpr_ptr, comm_ptr)) {
                VPR_dbgPrintf("Failed to send isip message to isi.\n");
            }
            break;
        case VPR_TYPE_NET:
            net_ptr = &comm_ptr->u.vprNet;
            switch (net_ptr->type) {
                case VPR_NET_TYPE_RTP_SEND_PKT:
                    if (OSAL_SUCCESS != _VPR_sendVideoRtp(
                            vpr_ptr, net_ptr,
                            net_ptr->u.packet.packetData,
                            net_ptr->u.packet.packetLen)) {
                        VPR_dbgPrintf("Failed to send video rtp.\n");
                        /* Return error to VIER */
                        if (OSAL_SUCCESS != _VPR_sendVideoRtpError(comm_ptr,
                                VPR_NET_STATUS_WRITE_ERROR)) {
                            VPR_dbgPrintf("Failed to send video error back.\n");
                        }
                    }
                    break;
                case VPR_NET_TYPE_RTP_RECV_PKT:
                    /* Dispatch voice rtp packet to corresponding VPR_Socket */
                    _VPR_dispatchVoiceRtp(vpr_ptr, comm_ptr);
                    break;
                case VPR_NET_TYPE_CREATE_VIDEO_RTP:
                    /* Send to VPR video thread to process */
                    size = sizeof(VPR_Comm);
                    if (OSAL_SUCCESS != OSAL_fileWrite(
                            &vpr_ptr->fd.videoTaskCmdFifoFd, comm_ptr, &size)) {
                        VPR_dbgPrintf("Failed to write video thread cmd fifo.\n");
                        if (OSAL_SUCCESS != _VPR_sendVideoRtpError(comm_ptr,
                                VPR_NET_STATUS_OPEN_ERROR)) {
                            VPR_dbgPrintf("Failed to send video error back.\n");
                        }
                    }
                    break;
                case VPR_NET_TYPE_ERROR:
                    VPR_dbgPrintf("Receive network error from voer. Type:%d\n",
                            net_ptr->u.status.evtType);
                    /* Set socket error if it's socket create error */
                    if (VPR_NET_STATUS_OPEN_ERROR == net_ptr->u.status.evtType) {
                        /* Get socket */
                        if (NULL == (sock_ptr = _VPR_getSocketById(vpr_ptr,
                                net_ptr->referenceId))) {
                            VPR_dbgPrintf("Failed to find voer socket:%d.\n",
                                    net_ptr->referenceId);
                            return (OSAL_FAIL);
                        }
                        /*
                         * There is socket error from voer.
                         */
                        sock_ptr->error = OSAL_TRUE;
                    }
                    break;
                case VPR_NET_TYPE_CLOSE:
                    /* Send to VPR video thread to process */
                    size = sizeof(VPR_Comm);
                    if (OSAL_SUCCESS != OSAL_fileWrite(
                            &vpr_ptr->fd.videoTaskCmdFifoFd, comm_ptr, &size)) {
                        VPR_dbgPrintf("Failed to write video thread cmd fifo.\n");
                        if (OSAL_SUCCESS != _VPR_sendVideoRtpError(comm_ptr,
                                VPR_NET_STATUS_CLOSE_ERROR)) {
                            VPR_dbgPrintf("Failed to send video error back.\n");
                        }
                    }
                    break;
                case VPR_NET_TYPE_CREATE_SIP:
                    if (OSAL_SUCCESS != _VPR_srCreateSipSocket(vpr_ptr,
                            comm_ptr)) {
                        VPR_dbgPrintf("Failed to create sip socket.\n");
                        /* Indicate SR there error in the socket */
                        if (OSAL_SUCCESS != _VPR_srSendSipError(comm_ptr,
                                VPR_NET_STATUS_OPEN_ERROR)) {
                            VPR_dbgPrintf("Failed to send sip error back.\n");
                        }
                    }
                    break;
                case VPR_NET_TYPE_SIP_SEND_PKT:
                    if (OSAL_SUCCESS != _VPR_srSendSip(vpr_ptr, comm_ptr)) {
                        VPR_dbgPrintf("Failed to send sip packet.\n");
                        /* Indicate SR there error in the socket */
                        if (OSAL_SUCCESS != _VPR_srSendSipError(comm_ptr,
                                VPR_NET_STATUS_WRITE_ERROR)) {
                            VPR_dbgPrintf("Failed to send sip error back.\n");
                        }
                    }
                    break;
                case VPR_NET_TYPE_SIP_RECV_PKT:
                case VPR_NET_TYPE_RTCP_EVT:
                default:
                    VPR_dbgPrintf("Invalid net type %d\n", net_ptr->type);
                    break;
            }
            break;
        case VPR_TYPE_RTCP_CMD:
            if (OSAL_SUCCESS != _VPR_sendRtcpCmd(
                    vpr_ptr, comm_ptr)) {
                VPR_dbgPrintf("Failed to send rtcp msg to vtsp.\n");
            }
            break;
        case VPR_TYPE_CSM_EVT:
            if (OSAL_SUCCESS != _VPR_sendCsmEvt(
                    vpr_ptr, comm_ptr)) {
                VPR_dbgPrintf("Failed to send csm evnet to csm input.\n");
            }
            break;
        default:
            VPR_dbgPrintf("Invalid command type %d\n", comm_ptr->type);
            break;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_processVideoVprComm() ========
 *
 * Private function to process video VPR_Comm command from which is sent
 * from VPR daemon.
 * This function is called by VPR video thread.
 *
 * Returns:
 * OSAL_SUCCESS: VPR_Comm processed.
 * OSAL_FAIL: Error in VPR_Comm processing.
 */
static OSAL_Status _VPR_processVideoVprComm(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    VPR_Net          *net_ptr;

    switch (comm_ptr->type) {
        case VPR_TYPE_NET:
            net_ptr = &comm_ptr->u.vprNet;
            switch (net_ptr->type) {
                case VPR_NET_TYPE_CREATE_VIDEO_RTP:
                    /* Create video rtp socket */
                    if (NULL == _VPR_getVideoSocketByReferenceId(
                            vpr_ptr, net_ptr->referenceId)) {
                        /*
                         * There is no corresponding socket.
                         * Let's create, bind and set options to it.
                         */
                        if (NULL == _VPR_createVideoSocket(vpr_ptr,
                                net_ptr)) {
                            VPR_dbgPrintf("Failed to create rtp socket.\n");
                            /* Send error to VIER */
                            if (OSAL_SUCCESS != _VPR_sendVideoRtpError(
                                    comm_ptr, VPR_NET_STATUS_OPEN_ERROR)) {
                                VPR_dbgPrintf(
                                        "Failed to send video error back.\n");
                            }
                        }
                    }
                    break;
                case VPR_NET_TYPE_CLOSE:
                    VPR_dbgPrintf("\n");
                    _VPR_closeVideoSocket(vpr_ptr, net_ptr);
                    break;
                default:
                    VPR_dbgPrintf("Invliad VRP_Net type:%d\n", net_ptr->type);
                    break;
            }
            break;
        default:
            VPR_dbgPrintf("Invliad VRP_Commd type:%d\n", comm_ptr->type);
            break;
    }

    return (OSAL_SUCCESS);
}

static int _VPR_videoReadSetClear(
    VPR_Obj        *vpr_ptr,
    OSAL_SelectSet *readSet_ptr)
{
    vint               idx;
    OSAL_selectSetInit(readSet_ptr);
    /* Add fd of VPR daemon command queue for select */
    OSAL_selectAddId(&vpr_ptr->fd.videoTaskCmdFifoFd, readSet_ptr);
    /*
     * Add active video command socket for select.
     */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        if (VPR_SOCKET_ID_NONE != vpr_ptr->videoRtpSockets[idx].id) {
            /* This is an active socket */
            OSAL_selectAddId(&vpr_ptr->videoRtpSockets[idx].id, readSet_ptr);
        }
    }
    return 0;
}

void _VPR_videoProcessEvt(
    VPR_Obj        *vpr_ptr,
    VPR_Comm       *comm_ptr,
    OSAL_SelectSet  readSet)
{
    OSAL_NetSockId    *sockId_ptr;
    OSAL_Boolean       boolean;
    vint               idx;
    VPR_Net           *net_ptr;
    OSAL_NetAddress   *addr_ptr;
    uint8             *buf_ptr;
    uint32            *size_ptr;
    VPR_Socket        *sock_ptr;
    vint               size;
    /*
     * Now read all ready packets.
     */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        sock_ptr = &vpr_ptr->videoRtpSockets[idx];
        sockId_ptr = &sock_ptr->id;
        if (VPR_SOCKET_ID_NONE != *sockId_ptr) {
            /* This is an active socket */
            OSAL_selectIsIdSet(sockId_ptr, &readSet, &boolean);
            if (OSAL_FALSE == boolean) {
                /*
                 * If the RTP data stream is ready. Poll it for data. If not,
                 * go to the next RTP object.
                 */
                continue;
            }
            net_ptr = &comm_ptr->u.vprNet;
            buf_ptr = net_ptr->u.packet.packetData;
            addr_ptr = &net_ptr->remoteAddress;
            size_ptr = &comm_ptr->u.vprNet.u.packet.packetLen;
            /* Construct VPR_Comm */
            if (OSAL_FAIL == _VPR_constructVideoRtpPacket(
                    NULL, 0, NULL, NULL, sock_ptr->referenceId,
                    comm_ptr)) {
                VPR_dbgPrintf("%s:%d Failed to construct video packet.\n",
                     __FUNCTION__, __LINE__);
            }
            /* There is data, receive it */
            *size_ptr = sizeof(net_ptr->u.packet.packetData);
            if (OSAL_FAIL == OSAL_netSocketReceiveFrom(
                    sockId_ptr, buf_ptr, (vint *)size_ptr, addr_ptr)) {
                return ;
            }
            /* Got video rtp packet, send to VIER */
            VPMD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
        }
    }

    /* See if it's command from VPR daemon */
    OSAL_selectIsIdSet(&vpr_ptr->fd.videoTaskCmdFifoFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read it */
        size = sizeof(VPR_Comm);
        if (OSAL_SUCCESS != OSAL_fileRead(&vpr_ptr->fd.videoTaskCmdFifoFd,
                comm_ptr, &size)) {
            VPR_dbgPrintf("%s:%d Failed to read queue.\n",
                 __FUNCTION__, __LINE__);
        }
        else {
            if (sizeof(VPR_Comm) != size) {
            VPR_dbgPrintf("%s:%d Read fifo incomplete.\n",
                 __FUNCTION__, __LINE__);
            }
            /* Process it */
            _VPR_processVideoVprComm(vpr_ptr, comm_ptr);
        }
    }
}


/*
 * ======== _VPR_videoTask() ========
 *
 * This task loops forever to receive VRP_Comm from _VPR_vpmdTask()
 * and receive video rtp packet from modem network interface.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPR_videoTask(
    OSAL_TaskArg arg_ptr)
{
    VPR_Obj           *vpr_ptr;
    VPR_Comm          *comm_ptr;
    OSAL_SelectSet    readSet;
    OSAL_SelectTimeval *timeout_ptr;
    OSAL_SelectTimeval  time;
    OSAL_Boolean        isTimeout;
    vint                timeout;
    vpr_ptr  = (VPR_Obj*) arg_ptr;
    comm_ptr = &vpr_ptr->commVideoTask;

    
_VPR_VIDEO_TASK_LOOP:
    /* Clear before read */
    _VPR_videoReadSetClear(vpr_ptr,&readSet);
    isTimeout = OSAL_FALSE;

    /* Wait forever */
    timeout = 3000;

    OSAL_memSet(&time, 0, sizeof(OSAL_SelectTimeval));
    time.sec= timeout / 1000;
    time.usec= timeout % 1000;
    timeout_ptr = &time;
    if (OSAL_FAIL == OSAL_select(&readSet, NULL, timeout_ptr, &isTimeout)) {
        OSAL_logMsg("%s:%d Failed to seletec fd for video sockets.\n",
             __FUNCTION__, __LINE__);
        OSAL_taskDelay(100);
        goto _VPR_VIDEO_TASK_LOOP;
    }
    if (OSAL_TRUE == isTimeout) {
        goto _VPR_VIDEO_TASK_LOOP;
    }
    _VPR_videoProcessEvt(vpr_ptr,comm_ptr,readSet); 
    goto _VPR_VIDEO_TASK_LOOP;
    return 0;
}

static int _VPR_vpmdReadSetClear(
    VPR_Obj          *vpr_ptr,
    OSAL_SelectSet   *readSet_ptr)
{
    /*
     * Group fd set.
     */
    OSAL_selectSetInit(readSet_ptr);
    /* Add vpmd video stream fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdVideoStreamFd, readSet_ptr);
    /* Add vpmd video cmd evt fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdVideoCmdEvtFd, readSet_ptr);
    /* Add vpmd voice stream fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdVoiceStreamFd, readSet_ptr);
    /* Add vpmd sip fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdSipFd, readSet_ptr);
    /* Add vpmd isip fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdIsipFd, readSet_ptr);
    /* Add vpmd csm event fd for select */
    OSAL_selectAddId(&vpr_ptr->fd.vpmdCsmEvtFd, readSet_ptr);
    return 0;
}

void _VPR_vpmdProcessEvt(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr,
    OSAL_SelectSet   readSet)
{
    OSAL_Boolean     boolean;

    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdVideoStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdVideoCmdEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoCmdEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdVoiceStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVoiceStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
    /* See if it's sip packet from sr */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdSipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readSip(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
    /* See if it's isip message from sr */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdIsipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readIsip(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
    /* See if it's csm event from rir */
    OSAL_selectIsIdSet(&vpr_ptr->fd.vpmdCsmEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readCsmEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_NO_WAIT)) {
            /* Got VPR_Comm, process it */
            VPR_dbgPrintf("Got csm event from RIR\n");
            _VPR_processVprComm(vpr_ptr, comm_ptr);
        }
    }
}


/*
 * ======== _VPR_vpmdTask() ========
 *
 * This task is to wait on all VPMD's fds, receive data from VPMD
 * and process the data.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPR_vpmdTask(
    OSAL_TaskArg arg_ptr)
{

    VPR_Obj         *vpr_ptr;
    VPR_Comm        *comm_ptr;
    OSAL_SelectSet   readSet;
    OSAL_SelectTimeval *timeout_ptr;
    OSAL_SelectTimeval  time;
    OSAL_Boolean        isTimeout;
    vint                timeout;
    vpr_ptr = (VPR_Obj*) arg_ptr;
    comm_ptr = &vpr_ptr->commVpmdTask;
    
_VPR_VPMD_TASK_LOOP:
    /* Clear before read*/
    _VPR_vpmdReadSetClear(vpr_ptr,&readSet);

    /* Wait forever */
    timeout = 3000;
    OSAL_memSet(&time, 0, sizeof(OSAL_SelectTimeval));
    time.sec= timeout / 1000;
    time.usec= timeout % 1000;
    timeout_ptr = &time;

    if (OSAL_FAIL == OSAL_select(&readSet, NULL, timeout_ptr, &isTimeout)) {
        OSAL_logMsg("%s:%d Failed to seletec fd for video sockets.\n",
             __FUNCTION__, __LINE__);
        OSAL_taskDelay(100);
        goto _VPR_VPMD_TASK_LOOP;
    }
    if (OSAL_TRUE == isTimeout) {
        goto _VPR_VPMD_TASK_LOOP;
    }
    _VPR_vpmdProcessEvt(vpr_ptr,comm_ptr,readSet);
    goto _VPR_VPMD_TASK_LOOP;
    return 0;
}

/*
 * ======== _VPR_queueProcessEvt() ========
 *
 * This task is to receive command or event from queue and process it.
 *
 * Returns:
 * None
 */
OSAL_Status _VPR_queueProcessEvt(
    VPR_Obj  *vpr_ptr,
    OSAL_MsgQId   fromq,
    VPR_Comm     *comm_ptr)
{
    if (vpr_ptr->queue.videoRtcpEvtQ == fromq) {
        VPR_dbgPrintf("Got video rtcp event.\n");
        /* It's a video rtcp event from vtsp, send to vier */
        comm_ptr->type = VPR_TYPE_RTCP_EVT;
        /* Write to VIER */
        if (OSAL_SUCCESS != VPMD_writeVideoCmdEvt(comm_ptr,
                sizeof(VPR_Comm))) {
            VPR_dbgPrintf("Fail to write rtcp msg to VPMD\n");
            return (OSAL_FAIL);
        }
    }
    else if (vpr_ptr->queue.videoCmdQ == fromq) {
        VPR_dbgPrintf("Got video cmd.\n");
        /* It's a vtsp command rtcp event, send to vier. */
        comm_ptr->type = VPR_TYPE_VTSP_CMD;
        /* Write to VIER */
        if (OSAL_SUCCESS != VPMD_writeVideoCmdEvt(comm_ptr,
                sizeof(VPR_Comm))) {
            VPR_dbgPrintf("Fail to write video cmd to VPMD\n");
            return (OSAL_FAIL);
        }
    }
    else if (vpr_ptr->queue.isipRecvQ == fromq) {
        /* It's a vtsp command rtcp event, send to vier. */
        comm_ptr->type = VPR_TYPE_ISIP;
        /* Write to SR */
        if (OSAL_SUCCESS != VPMD_writeIsip(comm_ptr,
                sizeof(VPR_Comm))) {
            VPR_dbgPrintf("Fail to write isip to VPMD\n");
            return (OSAL_FAIL);
        }
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_queueRecvTask() ========
 *
 * This task is to receive command or event from VTSP and process it.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPR_queueRecvTask(
    OSAL_TaskArg arg_ptr)
{
    VPR_Obj    *vpr_ptr = (VPR_Obj*) arg_ptr;
    VPR_Comm   *comm_ptr = &vpr_ptr->commQueueRecv;
    OSAL_MsgQId fromq;

_VPR_VTSP_TASK_LOOP:
    /* Clear before read */
    OSAL_memSet(comm_ptr, sizeof(VPR_Comm), 0);
    /*
     * Check if there are any messages to process, and if yes, get a message.
     */
    while (0 >= OSAL_msgQGrpRecv(&vpr_ptr->queue.groupQ,
            (char *)comm_ptr, sizeof(VPR_Comm), OSAL_WAIT_FOREVER,
            &fromq, NULL)) {
        OSAL_taskDelay(100);
    }
    if (OSAL_SUCCESS != _VPR_queueProcessEvt(vpr_ptr,fromq,comm_ptr)) {
        VPR_dbgPrintf("Failed _VPR_queueProcessEvt\n");
        OSAL_taskDelay(100);
    }
    goto _VPR_VTSP_TASK_LOOP;

    return 0;
}

#ifndef OSAL_KERNEL_EMULATION

void _VPR_kernProcessEvt(
    VPR_Comm     *comm_ptr)
{
    /* Send command to VOER */
    if (OSAL_SUCCESS != VPMD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm))) {
        VPR_dbgPrintf("VPMD FAIL to write voice stream.\n");
    }
}

/*
 * ======== _VPR_recvKernVe() ========
 *
 * This task is to receive command or event from VTSP and process it.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPR_recvKernVe(
    OSAL_TaskArg arg_ptr)
{
#if 0
    VPR_Obj    *vpr_ptr = (VPR_Obj*) arg_ptr;
    VPR_Comm   *comm_ptr = &vpr_ptr->commSend;
    vint        size;
_VPR_RECV_KERNEL_VE_LOOP:
    /* Clear before read */
    OSAL_memSet(comm_ptr, sizeof(VPR_Comm), 0);
    
    /*
     * Check if there are any messages to process.
     */
    if (0 >= OSAL_msgQRecv(vpr_ptr->queue.vprFromKernelQ, comm_ptr,
            sizeof(VPR_Comm), OSAL_WAIT_FOREVER, NULL)) {
        OSAL_taskDelay(100);
        goto _VPR_RECV_KERNEL_VE_LOOP;
    }
    _VPR_kernProcessEvt(comm_ptr);

    goto _VPR_RECV_KERNEL_VE_LOOP;

    return (0);
#endif
    VPR_Obj    *vpr_ptr = (VPR_Obj*) arg_ptr;
    VPR_Comm   *comm_ptr = &vpr_ptr->commSend;
    vint        size;

_VPR_RECV_KERNEL_VE_LOOP:
    /* Clear before read */
    OSAL_memSet(comm_ptr, sizeof(VPR_Comm), 0);

    /*
     * Check if there are any messages to process.
     */
    if (0 >= OSAL_msgQRecv(vpr_ptr->queue.vprFromKernelQ, comm_ptr,
            sizeof(VPR_Comm), OSAL_WAIT_FOREVER, NULL)) {
        OSAL_taskDelay(100);
        goto _VPR_RECV_KERNEL_VE_LOOP;
    }

    /* Send command to VOER */
    if (OSAL_SUCCESS != VPMD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm))) {
        VPR_dbgPrintf("VPMD FAIL to write voice stream.\n");
    }

    goto _VPR_RECV_KERNEL_VE_LOOP;

    return (0);

}
#endif

/*
 * ======== _VPR_daemonStop() ========
 *
 * Private function to stop VPR daemon.
 *
 * Returns:
 * None.
 */
void _VPR_daemonStop(VPR_Obj *VPR_Obj_ptr)
{
    vint idx;

    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        VPR_Obj_ptr->videoRtpSockets[idx].id = -1;
        VPR_Obj_ptr->videoRtpSockets[idx].referenceId = -1;
    }
#ifdef OSAL_KERNEL_EMULATION
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        VPR_Obj_ptr->voiceRtpSockets[idx].id = -1;
        VPR_Obj_ptr->voiceRtpSockets[idx].referenceId = -1;
    }
#endif
    OSAL_taskDelete(VPR_Obj_ptr->vprQueueRecvTaskId);
    OSAL_taskDelete(VPR_Obj_ptr->vprVideoTaskId);
#ifndef OSAL_KERNEL_EMULATION
    OSAL_taskDelete(VPR_Obj_ptr->vprKernTaskId);
#endif
}

/*
 * ======== _VPR_startIsipTask() ========
 *
 * This task is to receive command or event from VTSP/ISI and process it.
 *
 * Returns:
 * OSAL_SUCCESS: Task started successfully.
 * OSAL_FAIL: Task started failed.
 */
OSAL_Status _VPR_startIsipTask(VPR_Obj *VPR_Obj_ptr)
{
    /* Create VPR queue recv task to receive message from VTSP and ISI */
    if (0 == (VPR_Obj_ptr->vprQueueRecvTaskId = OSAL_taskCreate(
            VPR_QUEUE_RECV_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_queueRecvTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_QUEUE_RECV_TASK_NAME);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_startVideoTask() ========
 *
 * This task is to create VPR video task
 *
 * Returns:
 * OSAL_SUCCESS: Task started successfully.
 * OSAL_FAIL: Task started failed.
 */
OSAL_Status _VPR_startVideoTask(VPR_Obj *VPR_Obj_ptr)
{
    /* Create VPR video task */
    if (0 == (VPR_Obj_ptr->vprVideoTaskId = OSAL_taskCreate(
            VPR_VIDEO_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_videoTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_VIDEO_TASK_NAME);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_startVpmdTask() ========
 *
 * This task is to create VPR vpmd task
 *
 * Returns:
 * OSAL_SUCCESS: Task started successfully.
 * OSAL_FAIL: Task started failed.
 */
OSAL_Status _VPR_startVpmdTask(VPR_Obj *VPR_Obj_ptr)
{
    /* Create VPR vpmd task */
    if (0 == (VPR_Obj_ptr->vprVpmdTaskId = OSAL_taskCreate(
            VPR_VPMD_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_vpmdTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_VPMD_TASK_NAME);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_startKernelTask() ========
 *
 * This task is to receive voice message from kernel space 
 *
 * Returns:
 * OSAL_SUCCESS: Task started successfully.
 * OSAL_FAIL: Task started failed.
 */
OSAL_Status _VPR_startKernelTask(VPR_Obj *VPR_Obj_ptr)
{
#ifndef OSAL_KERNEL_EMULATION   
    /* Create a task to receive voice message from kernel space */
    if (0 == (VPR_Obj_ptr->vprKernTaskId = OSAL_taskCreate(
            VPR_RECV_KERN_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_recvKernVe,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_RECV_KERN_TASK_NAME);
        return (OSAL_FAIL);
    }
#endif
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_daemonGo() ========
 *
 * Private function to start VPR daemon. This function creates VPR daemon task.
 *
 * Returns:
 * OSAL_SUCCESS: VPR daemon started successfully.
 * OSAL_FAIL: VPR daemon started failed.
 */
OSAL_Status _VPR_daemonGo(VPR_Obj *VPR_Obj_ptr)
{
    /* Create VPR queue recv task to receive message from VTSP and ISI */
    if (0 == (VPR_Obj_ptr->vprQueueRecvTaskId = OSAL_taskCreate(
            VPR_QUEUE_RECV_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_queueRecvTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_QUEUE_RECV_TASK_NAME);
        return (OSAL_FAIL);
    }

    /* Create VPR video task */
    if (0 == (VPR_Obj_ptr->vprVideoTaskId = OSAL_taskCreate(
            VPR_VIDEO_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_videoTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_VIDEO_TASK_NAME);
        return (OSAL_FAIL);
    }

    /* Create VPR vpmd task */
    if (0 == (VPR_Obj_ptr->vprVpmdTaskId = OSAL_taskCreate(
            VPR_VPMD_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_vpmdTask,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_VPMD_TASK_NAME);
        return (OSAL_FAIL);
    }

#ifndef OSAL_KERNEL_EMULATION
    /* Create a task to receive voice message from kernel space */
    if (0 == (VPR_Obj_ptr->vprKernTaskId = OSAL_taskCreate(
            VPR_RECV_KERN_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPR_TASK_STACK_BYTES,
            _VPR_recvKernVe,
            (void *)VPR_Obj_ptr))) {
        _VPR_daemonStop(VPR_Obj_ptr);
        VPR_dbgPrintf("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_RECV_KERN_TASK_NAME);
        return (OSAL_FAIL);
    }
#endif
    return (OSAL_SUCCESS);
}
