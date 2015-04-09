/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29240 $ $Date: 2014-10-09 19:19:43 +0800 (Thu, 09 Oct 2014) $
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include <sip_sip.h>
#include <sip_hdrflds.h>
#include <sip_mem_pool.h>
#include <vier_net.h>
#include "_vpr.h"
#include "_vpr_daemon.h"

/* Global VPR_Obj pointer */
VPR_Obj *_VPR_Obj_ptr = NULL;

/*
 * ======== _VPR_allocateKernel() ========
 *
 * Private routine for allocating the VPR kernel module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
OSAL_Status _VPR_allocateKernel(void)
{
#ifdef OSAL_KERNEL_EMULATION
    vint             idx;
    /* Init voice rtp socket */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        _VPR_Obj_ptr->voiceRtpSockets[idx].id = VPR_SOCKET_ID_NONE;
        _VPR_Obj_ptr->voiceRtpSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
    /* Create FIFO for vpr voice rtp sockets */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        OSAL_snprintf(_VPR_Obj_ptr->voiceRtpSockets[idx].fifoPath,
                sizeof(_VPR_Obj_ptr->voiceRtpSockets[idx].fifoPath),
                "%s%d", VPR_VOER_READ_FIFO_NAME, idx);
        /* Delete FIFO */
        OSAL_fileFifoDelete(_VPR_Obj_ptr->voiceRtpSockets[idx].fifoPath);
        /* Create FIFO */
        if (OSAL_SUCCESS != OSAL_fileFifoCreate(
                _VPR_Obj_ptr->voiceRtpSockets[idx].fifoPath, VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm))) {
            OSAL_logMsg("%s:%d Failed creating %s FIFO", __FUNCTION__, __LINE__,
                    _VPR_Obj_ptr->voiceRtpSockets[idx].fifoPath);
            return (OSAL_FAIL);
        }
    }
#endif
#ifndef OSAL_KERNEL_EMULATION    
    /* Create message queue to send event/RTP to kernel space */
    if (NULL == (_VPR_Obj_ptr->queue.vprToKernelQ = OSAL_msgQCreate(
                VPR_USER_TO_KERN_Q_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_VPR_KERNEL,
                OSAL_DATA_STRUCT_VPR_Comm,
                VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_USER_TO_KERN_Q_NAME);
        return (OSAL_FAIL);
    }

    /* Create message queue to receive cmd/RTP from kernel space */
    if (NULL == (_VPR_Obj_ptr->queue.vprFromKernelQ = OSAL_msgQCreate(
                VPR_KERN_TO_USER_Q_NAME,
                OSAL_MODULE_VPR_KERNEL, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT_VPR_Comm,
                VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_KERN_TO_USER_Q_NAME);
        return (OSAL_FAIL);
    }
#endif
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_allocateVideo() ========
 *
 * Private routine for allocating the VPR video module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
OSAL_Status _VPR_allocateVideo(void)
{
    vint             idx;
    /* Init video rtp socket */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        _VPR_Obj_ptr->videoRtpSockets[idx].id = VPR_SOCKET_ID_NONE;
        _VPR_Obj_ptr->videoRtpSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
    /* Init sr socket */
    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        _VPR_Obj_ptr->srSockets[idx].id = VPR_SOCKET_ID_NONE;
        _VPR_Obj_ptr->srSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
    /* create video command and event message queue */
    if (NULL == (_VPR_Obj_ptr->queue.videoCmdQ = OSAL_msgQCreate(
                VPR_VIDEO_CMD_QUEUE_NAME,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_CMD_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    if (NULL == (_VPR_Obj_ptr->queue.videoEvtQ = OSAL_msgQCreate(
                VPR_VIDEO_EVT_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_EVENT_NUM_MSG,
                _VTSP_Q_EVENT_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp msg Q */
    if (NULL == (_VPR_Obj_ptr->queue.videoRtcpCmdQ = OSAL_msgQCreate(
                VPR_VIDEO_RTCP_MSG_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
                _VTSP_Q_RTCP_NUM_MSG,
                _VTSP_Q_RTCP_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_RTCP_MSG_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp evt Q */
    if (NULL == (_VPR_Obj_ptr->queue.videoRtcpEvtQ = OSAL_msgQCreate(
                VPR_VIDEO_RTCP_EVT_QUEUE_NAME,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
                _VTSP_Q_RTCP_NUM_EVENT,
                _VTSP_Q_RTCP_EVENT_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create video thread command fifo */
    OSAL_fileFifoDelete(VPR_VIDEO_THREAD_CMD_FIFO_NAME);
    if (OSAL_FAIL == OSAL_fileFifoCreate(
                VPR_VIDEO_THREAD_CMD_FIFO_NAME, VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm))) {
        OSAL_logMsg("%s:%d Could not create %s fifo.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_THREAD_CMD_FIFO_NAME);
        return (OSAL_FAIL);
    }
    /* Open fifo */
    if (OSAL_FAIL == OSAL_fileOpen(
            &_VPR_Obj_ptr->fd.videoTaskCmdFifoFd, VPR_VIDEO_THREAD_CMD_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        OSAL_logMsg("%s:%d Could not open %s fifo.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_THREAD_CMD_FIFO_NAME);
        return (OSAL_FAIL);
    }
    /* Now add video command queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VPR_Obj_ptr->queue.groupQ,
            _VPR_Obj_ptr->queue.videoCmdQ)) {
        OSAL_logMsg("%s:%d Fail to add video cmd queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Now add video rtcp event queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VPR_Obj_ptr->queue.groupQ,
            _VPR_Obj_ptr->queue.videoRtcpEvtQ)) {
        OSAL_logMsg("%s:%d Fail to add video rtcp queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_allocateVpmd() ========
 *
 * Private routine for allocating the VPR vpmd module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
OSAL_Status _VPR_allocateVpmd(void)
{
/* Get voice stream device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdVoiceStreamFd = VPMD_getVoiceStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get voice stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video event device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdVideoCmdEvtFd = VPMD_getVideoCmdEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdVideoStreamFd = VPMD_getVideoStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get sip device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdSipFd = VPMD_getSipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get sip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get isip device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdIsipFd = VPMD_getIsipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get isip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (_VPR_Obj_ptr->fd.vpmdCsmEvtFd = VPMD_getCsmEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get csm event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /* create isip send Q */
    if (NULL == (_VPR_Obj_ptr->queue.isipSendQ = OSAL_msgQCreate(
                VPR_SR_ISIP_SEND_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
                VPR_MSGQ_DEPTH,
                sizeof(ISIP_Message), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_SR_ISIP_SEND_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create isip recv Q */
    if (NULL == (_VPR_Obj_ptr->queue.isipRecvQ = OSAL_msgQCreate(
                VPR_SR_ISIP_RECV_QUEUE_NAME,
                OSAL_MODULE_ISI, OSAL_MODULE_VPR, OSAL_DATA_STRUCT_ISIP_Message,
                VPR_MSGQ_DEPTH,
                sizeof(ISIP_Message), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_SR_ISIP_RECV_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create csm event send Q */
    if (NULL == (_VPR_Obj_ptr->queue.csmEvtQ = OSAL_msgQCreate(
                VPR_CSM_INPUT_EVT_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_CSM_PUBLIC,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN,
                sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_CSM_INPUT_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }
        /* Create group queue for multiple VPMD devices */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&_VPR_Obj_ptr->queue.groupQ)) {
        OSAL_logMsg("%s:%d Fail to create group queue.\n", __FUNCTION__,
                __LINE__);
        return (OSAL_FAIL);
    }
    /* Now add isip message recv queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_VPR_Obj_ptr->queue.groupQ,
            _VPR_Obj_ptr->queue.isipRecvQ)) {
        OSAL_logMsg("%s:%d Fail to add isip recv queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}


/*
 * ======== _VPR_init() ========
 *
 * Private function to initialize VPR.
 *
 * Returns:
 * OSAL_SUCCESS: VPR initialized successfully.
 * OSAL_FAIL: Error in VPR initialization.
 */
OSAL_Status _VPR_init(
    VPR_Obj **pVpr_ptr)
{
    vint             idx;
    VPR_Obj         *vpr_ptr;

    /* Allocate memory for VPR_Obj */
    if (NULL == (*pVpr_ptr = OSAL_memCalloc(1, sizeof(VPR_Obj), 0))) {
        VPR_dbgPrintf("Failed to allocate memory for global VPR_Obj\n");
        return (OSAL_FAIL);
    }

    vpr_ptr = *pVpr_ptr;

    /* Init video rtp socket */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        vpr_ptr->videoRtpSockets[idx].id = VPR_SOCKET_ID_NONE;
        vpr_ptr->videoRtpSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
#ifdef OSAL_KERNEL_EMULATION
    /* Init voice rtp socket */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        vpr_ptr->voiceRtpSockets[idx].id = VPR_SOCKET_ID_NONE;
        vpr_ptr->voiceRtpSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
#endif
    /* Init sr socket */
    for (idx = 0; idx < VPR_MAX_SR_SOCKETS; idx++) {
        vpr_ptr->srSockets[idx].id = VPR_SOCKET_ID_NONE;
        vpr_ptr->srSockets[idx].referenceId = VPR_SOCKET_ID_NONE;
    }
    /* create video command and event message queue */
    if (NULL == (vpr_ptr->queue.videoCmdQ = OSAL_msgQCreate(
                VPR_VIDEO_CMD_QUEUE_NAME,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_CMD_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    if (NULL == (vpr_ptr->queue.videoEvtQ = OSAL_msgQCreate(
                VPR_VIDEO_EVT_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_EVENT_NUM_MSG,
                _VTSP_Q_EVENT_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp msg Q */
    if (NULL == (vpr_ptr->queue.videoRtcpCmdQ = OSAL_msgQCreate(
                VPR_VIDEO_RTCP_MSG_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
                _VTSP_Q_RTCP_NUM_MSG,
                _VTSP_Q_RTCP_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_RTCP_MSG_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* create video rtcp evt Q */
    if (NULL == (vpr_ptr->queue.videoRtcpEvtQ = OSAL_msgQCreate(
                VPR_VIDEO_RTCP_EVT_QUEUE_NAME,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
                _VTSP_Q_RTCP_NUM_EVENT,
                _VTSP_Q_RTCP_EVENT_SZ, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create video thread command fifo */
    OSAL_fileFifoDelete(VPR_VIDEO_THREAD_CMD_FIFO_NAME);
    if (OSAL_FAIL == OSAL_fileFifoCreate(
                VPR_VIDEO_THREAD_CMD_FIFO_NAME, VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm))) {
        OSAL_logMsg("%s:%d Could not create %s fifo.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_THREAD_CMD_FIFO_NAME);
        return (OSAL_FAIL);
    }
    /* Open fifo */
    if (OSAL_FAIL == OSAL_fileOpen(
            &vpr_ptr->fd.videoTaskCmdFifoFd, VPR_VIDEO_THREAD_CMD_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        OSAL_logMsg("%s:%d Could not open %s fifo.\n", __FUNCTION__,
                __LINE__, VPR_VIDEO_THREAD_CMD_FIFO_NAME);
        return (OSAL_FAIL);
    }

#ifdef OSAL_KERNEL_EMULATION
    /* Create FIFO for vpr voice rtp sockets */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        OSAL_snprintf(vpr_ptr->voiceRtpSockets[idx].fifoPath,
                sizeof(vpr_ptr->voiceRtpSockets[idx].fifoPath),
                "%s%d", VPR_VOER_READ_FIFO_NAME, idx);
        /* Delete FIFO */
        OSAL_fileFifoDelete(vpr_ptr->voiceRtpSockets[idx].fifoPath);
        /* Create FIFO */
        if (OSAL_SUCCESS != OSAL_fileFifoCreate(
                vpr_ptr->voiceRtpSockets[idx].fifoPath, VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm))) {
            OSAL_logMsg("%s:%d Failed creating %s FIFO", __FUNCTION__, __LINE__,
                    vpr_ptr->voiceRtpSockets[idx].fifoPath);
            return (OSAL_FAIL);
        }
    }
#endif

    /* create isip send Q */
    if (NULL == (vpr_ptr->queue.isipSendQ = OSAL_msgQCreate(
                VPR_SR_ISIP_SEND_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
                VPR_MSGQ_DEPTH,
                sizeof(ISIP_Message), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_SR_ISIP_SEND_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create isip recv Q */
    if (NULL == (vpr_ptr->queue.isipRecvQ = OSAL_msgQCreate(
                VPR_SR_ISIP_RECV_QUEUE_NAME,
                OSAL_MODULE_ISI, OSAL_MODULE_VPR, OSAL_DATA_STRUCT_ISIP_Message,
                VPR_MSGQ_DEPTH,
                sizeof(ISIP_Message), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_SR_ISIP_RECV_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* create csm event send Q */
    if (NULL == (vpr_ptr->queue.csmEvtQ = OSAL_msgQCreate(
                VPR_CSM_INPUT_EVT_QUEUE_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_CSM_PUBLIC,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN,
                sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_CSM_INPUT_EVT_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    /* Get voice stream device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdVoiceStreamFd = VPMD_getVoiceStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get voice stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video event device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdVideoCmdEvtFd = VPMD_getVideoCmdEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdVideoStreamFd = VPMD_getVideoStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get sip device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdSipFd = VPMD_getSipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get sip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get isip device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdIsipFd = VPMD_getIsipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get isip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (vpr_ptr->fd.vpmdCsmEvtFd = VPMD_getCsmEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get csm event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Create group queue for multiple VPMD devices */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&vpr_ptr->queue.groupQ)) {
        OSAL_logMsg("%s:%d Fail to create group queue.\n", __FUNCTION__,
                __LINE__);
        return (OSAL_FAIL);
    }

    /* Now add video command queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&vpr_ptr->queue.groupQ,
            vpr_ptr->queue.videoCmdQ)) {
        OSAL_logMsg("%s:%d Fail to add video cmd queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Now add video rtcp event queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&vpr_ptr->queue.groupQ,
            vpr_ptr->queue.videoRtcpEvtQ)) {
        OSAL_logMsg("%s:%d Fail to add video rtcp queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Now add isip message recv queue. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&vpr_ptr->queue.groupQ,
            vpr_ptr->queue.isipRecvQ)) {
        OSAL_logMsg("%s:%d Fail to add isip recv queue to group queue\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

#ifndef OSAL_KERNEL_EMULATION
    /* Create message queue to send event/RTP to kernel space */
    if (NULL == (vpr_ptr->queue.vprToKernelQ = OSAL_msgQCreate(
                VPR_USER_TO_KERN_Q_NAME,
                OSAL_MODULE_VPR, OSAL_MODULE_VPR_KERNEL,
                OSAL_DATA_STRUCT_VPR_Comm,
                VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_USER_TO_KERN_Q_NAME);
        return (OSAL_FAIL);
    }

    /* Create message queue to receive cmd/RTP from kernel space */
    if (NULL == (vpr_ptr->queue.vprFromKernelQ = OSAL_msgQCreate(
                VPR_KERN_TO_USER_Q_NAME,
                OSAL_MODULE_VPR_KERNEL, OSAL_MODULE_VPR,
                OSAL_DATA_STRUCT_VPR_Comm,
                VPR_MSGQ_DEPTH,
                sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, VPR_KERN_TO_USER_Q_NAME);
        return (OSAL_FAIL);
    }
#endif

    /* Set default mode to LTE */
    vpr_ptr->networkMode = VPR_NETWORK_MODE_LTE;
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_shutdown() ========
 *
 * Private function to shutdown VPR.
 *
 * Returns:
 */
void _VPR_shutdown(
    VPR_Obj **pVpr_ptr)
{
    vint idx;

#ifdef OSAL_KERNEL_EMULATION
    /* Delelte voice VPR_Socket */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        /* Delete FIFO */
        OSAL_fileFifoDelete((*pVpr_ptr)->voiceRtpSockets[idx].fifoPath);
    }
#endif
    /* Delete all queues */
    if (NULL != (*pVpr_ptr)->queue.videoCmdQ) {
        OSAL_msgQDelete((*pVpr_ptr)->queue.videoCmdQ);
    }
    if (NULL != (*pVpr_ptr)->queue.videoEvtQ) {
        OSAL_msgQDelete((*pVpr_ptr)->queue.videoEvtQ);
    }
    if (NULL != (*pVpr_ptr)->queue.videoRtcpCmdQ) {
        OSAL_msgQDelete((*pVpr_ptr)->queue.videoRtcpCmdQ);
    }
    if (NULL != (*pVpr_ptr)->queue.videoRtcpEvtQ) {
        OSAL_msgQDelete((*pVpr_ptr)->queue.videoRtcpEvtQ);
    }
    if (NULL != (*pVpr_ptr)->queue.csmEvtQ) {
        OSAL_msgQDelete((*pVpr_ptr)->queue.csmEvtQ);
    }
    if (NULL != (*pVpr_ptr)->queue.groupQ) {
        OSAL_msgQGrpDelete(&((*pVpr_ptr)->queue.groupQ));
    }
    if (0 != (*pVpr_ptr)->fd.videoTaskCmdFifoFd) {
        OSAL_fileClose(&(*pVpr_ptr)->fd.videoTaskCmdFifoFd);
    }
    /* Delete fifo */
    OSAL_fileFifoDelete(VPR_VIDEO_THREAD_CMD_FIFO_NAME);

    /* Free memory */
    OSAL_memFree(*pVpr_ptr, 0);
    *pVpr_ptr = NULL;
}

/*
 * ======== _VPR_constructVideoRtpPacket() ========
 *
 * This function is to construct VPR command object for sending rtp packet.
 *
 * Returns:
 * OSAL_SUCCESS: VPR_Comm object constructed.
 * OSAL_FAIL: Error in VPR_Comm object construction.
 */
OSAL_Status _VPR_constructVideoRtpPacket(
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    OSAL_NetSockId          socketId,
    VPR_Comm               *comm_ptr)
{
    if (VPR_NET_MAX_DATA_SIZE_OCTETS < packetLen) {
        return (OSAL_FAIL);
    }
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm));
    comm_ptr->type = VPR_TYPE_NET;
    comm_ptr->u.vprNet.type = VPR_NET_TYPE_RTP_RECV_PKT;
    comm_ptr->u.vprNet.referenceId = socketId;
    if (lclAddress_ptr) {
        comm_ptr->u.vprNet.localAddress = *lclAddress_ptr;
    }
    if (rmtAddress_ptr){
        comm_ptr->u.vprNet.remoteAddress = *rmtAddress_ptr;
    }
    comm_ptr->u.vprNet.u.packet.chunkNumber = 1;
    comm_ptr->u.vprNet.u.packet.packetEnd = 1;
    comm_ptr->u.vprNet.u.packet.tosValue = 0;
    comm_ptr->u.vprNet.u.packet.packetLen = packetLen;
    if ((0 != packetLen) && (NULL != packet_ptr)) {
        /* Copy pakcet data */
        OSAL_memCpy(comm_ptr->u.vprNet.u.packet.packetData, packet_ptr,
            packetLen);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_getSocketByReferenceId() ========
 *
 * Private function to get vpr socket by reference id(vier socket id in
 * application processor).
 *
 * Returns:
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_Socket* _VPR_getVideoSocketByReferenceId(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId referenceId)
{
    vint idx;

    if (VPR_SOCKET_ID_NONE == referenceId) {
        return (NULL);
    }

    /* Loop to match the reference id */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        if (vpr_ptr->videoRtpSockets[idx].referenceId == referenceId) {
            return (&vpr_ptr->videoRtpSockets[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_getAvailableVideoSocket() ========
 *
 * Private function to get an available vpr video socket.
 *
 * Returns:
 * -1: No available vpr socket.
 * Otherwise: The pointer to the available vpr socket.
 */
VPR_Socket* _VPR_getAvailableVideoSocket(
    VPR_Obj       *vpr_ptr)
{
    vint idx;

    /* Loop to find an available socket */
    for (idx = 0; idx < VPR_MAX_VIDEO_STREAMS; idx++) {
        if (VPR_SOCKET_ID_NONE == vpr_ptr->videoRtpSockets[idx].id) {
            return (&vpr_ptr->videoRtpSockets[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_createVideoSocket() ========
 *
 * Private function to create socket for video rtp.
 *
 * Returns:
 * NULL: Fail to create video socket.
 * Otherwise: The pointer to the created vpr socket.
 */
VPR_Socket* _VPR_createVideoSocket(
    VPR_Obj   *vpr_ptr,
    VPR_Net   *net_ptr)
{
    VPR_Socket      *sock_ptr;
    OSAL_NetAddress *address_ptr;

    address_ptr = &net_ptr->localAddress;
    VPR_dbgPrintf("addr:%d, type:%d\n", address_ptr->ipv4, address_ptr->type);
    /* Get an available vpr socket for video */
    if (NULL == (sock_ptr = _VPR_getAvailableVideoSocket(vpr_ptr))) {
        VPR_dbgPrintf("Failed to get available video socket.\n");
        return (OSAL_FAIL);
    }

    /* Create socket */
    if (OSAL_SUCCESS != OSAL_netSocket(&sock_ptr->id, address_ptr->type)) {
        OSAL_logMsg("%s:%d Failed to create socket.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Bind it */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&sock_ptr->id, address_ptr)) {
        OSAL_logMsg("%s:%d Failed to bind ip:%d, port:%d\n",
                __FUNCTION__, __LINE__,
                address_ptr->ipv4, address_ptr->port);
        return (OSAL_FAIL);
    }

    /* Currently only set TOS */
    if (OSAL_SUCCESS !=
            OSAL_netSetSocketOptions(&sock_ptr->id, OSAL_NET_IP_TOS,
            net_ptr->u.packet.tosValue)) {
        OSAL_logMsg("%s:%d Failed to set socket options\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    sock_ptr->referenceId = net_ptr->referenceId;
    VPR_dbgPrintf("socket created:%d(Ref id:%d)\n",
            sock_ptr->id, sock_ptr->referenceId);
    return (sock_ptr);
}

/*
 * ======== _VPR_closeVideoSocket() ========
 *
 * Private function to close socket for video rtp.
 *
 * Returns:
 * OSAL_SUCCESS: Video socket closed successfully.
 * OSAL_FAIL: Cannot find the corresponding videp rtp socket.
 */
OSAL_Status _VPR_closeVideoSocket(
    VPR_Obj   *vpr_ptr,
    VPR_Net   *net_ptr)
{
    VPR_Socket      *sock_ptr;

    /* Get video rtp socket */
    if (NULL != (sock_ptr = _VPR_getVideoSocketByReferenceId(vpr_ptr,
            net_ptr->referenceId))) {
        /* There is no corresponding socket. Close it. */
        VPR_dbgPrintf("Close socket %d(Ref id=%d).\n",
                sock_ptr->id, sock_ptr->referenceId);
        OSAL_netCloseSocket(&sock_ptr->id);
        /* Clean id */
        sock_ptr->id = VPR_SOCKET_ID_NONE;
        sock_ptr->referenceId = VPR_SOCKET_ID_NONE;
        sock_ptr->ssl_ptr = NULL;
        return (OSAL_SUCCESS);
    }

    VPR_dbgPrintf("Cannot find socket. Ref id=%d\n", net_ptr->referenceId);
    return (OSAL_FAIL);
}

/*
 * ======== _VPR_sendVtspEvt() ========
 * Private function to send vtsp event message to vtsp
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_sendVtspEvt(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    uint8 *msg_ptr;

    msg_ptr = (uint8 *)&comm_ptr->u.vtspEvt;
    if (OSAL_SUCCESS != OSAL_msgQSend(vpr_ptr->queue.videoEvtQ,
            msg_ptr, _VTSP_Q_EVENT_MSG_SZ, OSAL_NO_WAIT, NULL)) {
        VPR_dbgPrintf("Failed to write vtsp event msg Q.\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_sendRtcpCmd() ========
 * Private function to send rtcp command message to vtsp
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_sendRtcpCmd(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    uint8 *msg_ptr;

    msg_ptr = (uint8 *)&comm_ptr->u.vtspRtcpCmd;
    if (OSAL_SUCCESS != OSAL_msgQSend(vpr_ptr->queue.videoRtcpCmdQ,
            msg_ptr, _VTSP_Q_RTCP_MSG_SZ, OSAL_NO_WAIT, NULL)) {
        VPR_dbgPrintf("Failed to write rtcp msg Q.\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_sendVideoRtp() ========
 *
 * Private function to send video rtp packet.
 *
 * Returns:
 * OSAL_SUCCESS: Video sent successfully.
 * OSAL_FAIL: Error on sending video rtp packet.
 */
OSAL_Status _VPR_sendVideoRtp(
    VPR_Obj           *vpr_ptr,
    VPR_Net           *net_ptr,
    uint8             *packet_ptr,
    uint32             packetLen)
{
    VPR_Socket      *sock_ptr;

    /* Get video rtp socket */
    if (NULL == (sock_ptr = _VPR_getVideoSocketByReferenceId(vpr_ptr,
            net_ptr->referenceId))) {
        /* Cannot find corresponding socket, return fail */
        return (OSAL_FAIL);
    }

    /* Then let's write it out of an existing connection if there is one. */
    if (sock_ptr->id) {
        if (OSAL_FAIL == OSAL_netSocketSendTo(&sock_ptr->id, packet_ptr,
                (vint*)&packetLen, &net_ptr->remoteAddress)) {
            return (OSAL_FAIL);
        }
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== _VPR_getSocketById() ========
 *
 * Private function to get vpr socket index by socket id.
 *
 * Returns:
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_Socket* _VPR_getSocketById(
    VPR_Obj      *vprObj_ptr,
    OSAL_NetSockId socketId)
{
#ifdef OSAL_KERNEL_EMULATION
    vint idx;

    if (NULL == vprObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        if (socketId == vprObj_ptr->voiceRtpSockets[idx].id) {
            return (&vprObj_ptr->voiceRtpSockets[idx]);
        }
    }

    return (NULL);
#else
    return (NULL);
#endif
}

/*
 * ======== _VPR_freeSocket() ========
 *
 * Private function to free a VPR_Socket
 *
 * Returns:
 * OSAL_SUCCESS: Success.
 * OSAL_FAIL: Failed.
 */
OSAL_Status _VPR_freeSocket(
    VPR_Socket *sock_ptr)
{

    if (NULL == sock_ptr) {
        return (OSAL_FAIL);
    }

    /* Close FIFO */
    OSAL_fileClose(&sock_ptr->fifoId);

    /* Clean data */
    OSAL_memSet(&sock_ptr->localAddress, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&sock_ptr->remoteAddress, 0, sizeof(OSAL_NetAddress));
    sock_ptr->id = VPR_SOCKET_ID_NONE;
    sock_ptr->referenceId = VPR_SOCKET_ID_NONE;
    return (OSAL_SUCCESS);
}

/*
 * ======== VPR_constructVoerPacket() ========
 *
 * This function is to construct VPR command object for sending rtp packet.
 *
 * Returns:
 * OSAL_SUCCESS: VPR_Comm object constructed.
 * OSAL_FAIL: Error in VPR_Comm object construction.
 */
OSAL_Status VPR_constructVoerPacket(
    OSAL_NetSockId         *socket_ptr,
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    VPR_Comm               *comm_ptr,
    OSAL_NetSockType        tos,
    VPR_NetType             type)
{
    if (VPR_NET_MAX_DATA_SIZE_OCTETS < packetLen) {
        return (OSAL_FAIL);
    }
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm));

    comm_ptr->type = VPR_TYPE_NET;
    comm_ptr->u.vprNet.type = type;
    comm_ptr->u.vprNet.referenceId = *socket_ptr;

    if (lclAddress_ptr) {
        comm_ptr->u.vprNet.localAddress = *lclAddress_ptr;
    }
    if (rmtAddress_ptr){
        comm_ptr->u.vprNet.remoteAddress = *rmtAddress_ptr;
    }
    comm_ptr->u.vprNet.u.packet.chunkNumber = 1;
    comm_ptr->u.vprNet.u.packet.packetEnd = 1;
    if (VPR_NET_TYPE_RTP_SEND_PKT == type) {
        comm_ptr->u.vprNet.u.packet.tosValue = tos;
    }
    else if (VPR_NET_TYPE_RTP_RECV_PKT == type) {
        comm_ptr->u.vprNet.u.packet.tosValue = 0;
    }
    if (0 != packetLen) {
        OSAL_memCpy(comm_ptr->u.vprNet.u.packet.packetData, packet_ptr,
                packetLen);
        comm_ptr->u.vprNet.u.packet.packetLen = packetLen;
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_dispatchVoiceRtp() ========
 *
 * Private function to dispatch received rtp packet to corresponding VPR_Socket
 *
 * Returns:
 * OSAL_SUCCESS: voice dispatched successfully.
 * OSAL_FAIL: Error on dispatching voice rtp packet.
 */
OSAL_Status _VPR_dispatchVoiceRtp(
    VPR_Obj   *vpr_ptr,
    VPR_Comm  *comm_ptr)
{
    VPR_Socket *sock_ptr = NULL;
    VPR_Net    *net_ptr;
    vint        size;

    if (VPR_TYPE_NET != comm_ptr->type) {
        VPR_dbgPrintf("Invalid VPR command type:%d\n", comm_ptr->type);
        return (OSAL_FAIL);
    }

    net_ptr = &comm_ptr->u.vprNet;

    if (VPR_NET_TYPE_RTP_RECV_PKT != net_ptr->type) {
        VPR_dbgPrintf("Invalid VPR net type:%d\n", net_ptr->type);
        return (OSAL_FAIL);
    }

#ifdef OSAL_KERNEL_EMULATION
    /* Get socket */
    if (NULL == (sock_ptr = _VPR_getSocketById(vpr_ptr,
            net_ptr->referenceId))) {
        VPR_dbgPrintf("Failed to find voer socket:%d.\n",
                net_ptr->referenceId);
        return (OSAL_FAIL);
    }

    /* Write to FIFO */
    size = sizeof(VPR_Comm);
    if (OSAL_SUCCESS != OSAL_fileWrite(&sock_ptr->fifoId, comm_ptr, &size)) {
        return (OSAL_FAIL);
    }
#else
    /* VE is kernel mode, so write to kernel space. */
    if (OSAL_SUCCESS != OSAL_msgQSend(vpr_ptr->queue.vprToKernelQ, comm_ptr,
            sizeof(VPR_Comm), OSAL_NO_WAIT, NULL)) {
        VPR_dbgPrintf("Failed to write data to kernel space.\n");
        return (OSAL_FAIL);
    }
#endif

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_allocSocket() ========
 *
 * Private function to allocate a VPR_Socket
 *
 * Returns:
 * -1: No available vpr socket.
 * Otherwise: The index of vpr socket.
 */
VPR_Socket* _VPR_allocSocket(
    VPR_Obj *vprObj_ptr)
{
#ifdef OSAL_KERNEL_EMULATION
    vint              idx;

    if (NULL == vprObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        if (-1 == vprObj_ptr->voiceRtpSockets[idx].id) {
            /* Open FIFO */
            if (OSAL_SUCCESS != OSAL_fileOpen(
                    &vprObj_ptr->voiceRtpSockets[idx].fifoId,
                    vprObj_ptr->voiceRtpSockets[idx].fifoPath,
                    OSAL_FILE_O_RDWR, 0)) {
                OSAL_logMsg("%s:%d Open %s FIFO failed.\n", __FUNCTION__,
                        __LINE__, vprObj_ptr->voiceRtpSockets[idx].fifoPath);
                return (NULL);
            }
            /* Get fd from fd and set it to socketId */
            vprObj_ptr->voiceRtpSockets[idx].id =
                    vprObj_ptr->voiceRtpSockets[idx].fifoId;
            return (&vprObj_ptr->voiceRtpSockets[idx]);
        }
    }
    return (NULL);
#else
    return (NULL);
#endif
}

/*
 * ======== _VPR_sendVideoRtpError() ========
 *
 * Private function to send error message back to ViER
 *
 * Returns:
 * OSAL_SUCCESS: success to send error message to ViER.
 * OSAL_FAIL:    fail to send error message to ViER.
 */
OSAL_Status _VPR_sendVideoRtpError(
    VPR_Comm         *comm_ptr,
    VPR_NetStatusType statusType)
{
    comm_ptr->u.vprNet.type             = VPR_NET_TYPE_ERROR;
    comm_ptr->u.vprNet.u.status.evtType = statusType;

    if (OSAL_SUCCESS != VPMD_writeVideoStream(comm_ptr, sizeof(VPR_Comm))) {
        VPR_dbgPrintf("Fail to write rtp stream msg to VPMD\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_sendCsmEvt() ========
 * Private function to send csm event to csm
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_sendCsmEvt(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr)
{
    uint8 *msg_ptr;

    msg_ptr = (uint8 *)&comm_ptr->u.csmEvt;
    if (OSAL_SUCCESS != OSAL_msgQSend(vpr_ptr->queue.csmEvtQ,
            msg_ptr, sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL)) {
        VPR_dbgPrintf("Failed to write csm event input Q.\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== VPR_setNetworkMode() ========
 *
 * This function is to set what network mode used, wifi or lte.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VPR_setNetworkMode(
    VPR_Obj  *vpr_ptr,
    VPR_NetworkMode mode)
{
    VPR_Comm *comm_ptr;

    vpr_ptr->networkMode = mode;
    comm_ptr = &vpr_ptr->commSr;

    /* Tell vier about the mode change */
    comm_ptr->type = VPR_TYPE_NETWORK_MODE;
    comm_ptr->u.networkMode = mode;

#ifndef OSAL_KERNEL_EMULATION
    /* VE is kernel mode, so notify what network mode. */
    if (OSAL_SUCCESS != OSAL_msgQSend(vpr_ptr->queue.vprToKernelQ, comm_ptr,
            sizeof(VPR_Comm), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d Fail to send message.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
#endif

    if (OSAL_SUCCESS != VPMD_writeVideoCmdEvt(comm_ptr, sizeof(VPR_Comm))) {
        VPR_dbgPrintf("Fail to write network type mode msg to VPMD\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}
