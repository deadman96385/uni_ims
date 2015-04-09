/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12065 $ $Date: 2010-05-17 16:29:49 -0400 (Mon, 17 May 2010) $
 *
 */

/*
 * This file is the voice processing task
 */
#include "_ve_private.h"

/*
 * ======== VE_taskEncode() ========
 *
 * Main processing loop for voice
 */
void _VE_taskEncode(
    _VE_Obj *ve_ptr)
{
    _VE_Queues      *q_ptr;
    _VE_Dsp         *dsp_ptr;

    /*
     * Wait till main task starts.
     */
    while (0 != (_VE_TASK_WAIT & ve_ptr->taskEncode.taskEnable)) {
        /* Yield to other tasks */
        OSAL_taskDelay(100);
    }

    q_ptr = ve_ptr->q_ptr;
    dsp_ptr = ve_ptr->dsp_ptr;

    OSAL_logMsg("%s:%d Encoder RUNNING\n", __FILE__, __LINE__);

    while (0 != (_VE_TASK_RUN & ve_ptr->taskEncode.taskEnable)) {
        /* Check for active streams */
        if (1 == _VE_videoStreamIsActive(dsp_ptr)) {
            /*
             * Get video data, blocking function.
             * Then preview it.
             */
            VCD_videoIn(&dsp_ptr->pic);
            if (0 != dsp_ptr->pic.size) {
                /*
                 * If a picture received, draw it in preview.
                 */
                dsp_ptr->pic.id = VIDEO_CALLID_CAMERA;
                VDD_videoOut(&dsp_ptr->pic);
            }

            /*
             * The encoder may need to stop in case a new command needs to
             *  update the objects, but dont stop the preview.
             */
            if (OSAL_FAIL == OSAL_semAcquire(ve_ptr->encStopSem, 100)) {
                _VE_TRACE(__FILE__, __LINE__);
                continue;
            }
            
            _VE_videoStreamEncode(ve_ptr, q_ptr, dsp_ptr);

            _VE_videoStreamEncodeSendData(ve_ptr, q_ptr, dsp_ptr);

            if (OSAL_FAIL == OSAL_semGive(ve_ptr->encStopSem)) {
                _VE_TRACE(__FILE__, __LINE__);
                continue;
            }
        }
        else {
            /* Wait till streams are active */
            OSAL_taskDelay(100);
        }
    }

    /*
     * Shut down encoders.
     */
    _VE_videoStreamShutdownEncoders(ve_ptr, q_ptr, dsp_ptr);

    /*
     * Exit spawned task
     */
    ve_ptr->taskEncode.taskEnable = _VE_TASK_FINISHED;
    OSAL_logMsg("%s:%d finished\n", __FILE__, __LINE__);
    OSAL_semGive(ve_ptr->taskEncode.finishSemId);
}

/*
 * ======== VE_task() ========
 *
 * Main processing loop for voice 
 */
void _VE_task(
    _VE_Obj *ve_ptr)
{
    _VE_Queues      *q_ptr;
    _VE_Dsp         *dsp_ptr;

    q_ptr = ve_ptr->q_ptr;
    dsp_ptr = ve_ptr->dsp_ptr;

    /* 
     * Big D2 Banner with version information.
     */
    OSAL_logMsg("\n"
            "             D2 Technologies, Inc.\n"
            "      _   _  __                           \n"
            "     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
            "    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
            "                                _/        \n"
            "\n"
            "        Unified Communication Products\n");
    OSAL_logMsg(
            "               www.d2tech.com\n");


    ve_ptr->taskEncode.taskEnable |= _VE_TASK_RUN;
    ve_ptr->taskEncode.taskEnable &= ~_VE_TASK_WAIT;
    ve_ptr->taskMain.taskEnable |= _VE_TASK_RUN;
    ve_ptr->taskMain.taskEnable &= ~_VE_TASK_WAIT;

    OSAL_logMsg("%s:%d RUNNING\n", __FILE__, __LINE__);

    /*
     * ToNet Event to application prior to exit, on globalQ and each infcQ
     */
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_SHUTDOWN_VIDEO;
    q_ptr->eventMsg.tick = 0;
    q_ptr->eventMsg.msg.shutdown.reason = VTSP_EVENT_ACTIVE;
    q_ptr->eventMsg.msg.shutdown.status = 0;
    q_ptr->eventMsg.infc = VTSP_INFC_VIDEO;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_VIDEO);

    while (0 != (_VE_TASK_RUN & ve_ptr->taskMain.taskEnable)) {
        /* Check for active streams */
        if (1 == _VE_videoStreamIsActive(dsp_ptr)) {
            /* Check for incoming RTP data */
            if (VTSP_OK != _VE_rtpRecv(ve_ptr, q_ptr, dsp_ptr)) {
                _VTSP_TRACE(__FILE__, __LINE__);
            }
 
            /*
             * Generate Stream Events
             */
            _VE_genEventStream(ve_ptr, q_ptr, dsp_ptr);

            /*
             * Do STUN processing
             */
            if (VTSP_OK != _VE_stunProcess(q_ptr, ve_ptr->net_ptr)) {
                _VE_TRACE(__FILE__, __LINE__);
            }
            
            /* Decode packets as needed */
            _VE_videoStreamDecode(ve_ptr, q_ptr, dsp_ptr);
            _VE_videoStreamDecodeDrawData(ve_ptr, q_ptr, dsp_ptr);

            /*
             * Get any RTCP packets from network.
             */
            if (VTSP_OK != _VE_rtcpRecv(q_ptr, dsp_ptr, ve_ptr->net_ptr)) {
                _VE_TRACE(__FILE__, __LINE__);
            }
        }
        else {
            /* Wait till streams are active */
            OSAL_taskDelay(100);
        }
        /*
         *
         * Run all commands sent down from user level
         */
        _VE_recvAllCmd(ve_ptr, q_ptr, dsp_ptr);
    }
    OSAL_logMsg("%s:%d", __FUNCTION__, __LINE__);
    /*
     * Shutdown video
     * --------
     */
    /*
     * Shutdown RTP streams and interfaces.
     */
    _VE_rtpShutdown(&ve_ptr->net_ptr->rtpObj[0]);

    /*
     * Shut down decoders.
     */
    _VE_videoStreamShutdownDecoders(ve_ptr, q_ptr, dsp_ptr);

    /*
     * ToNet Event to application prior to exit, on globalQ and each infcQ
     */
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_SHUTDOWN_VIDEO;
    q_ptr->eventMsg.tick = 0;
    q_ptr->eventMsg.msg.shutdown.reason = VTSP_EVENT_HALTED;
    q_ptr->eventMsg.msg.shutdown.status = 0;
    q_ptr->eventMsg.infc = VTSP_INFC_VIDEO;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_VIDEO);

    /*
     * Exit spawned task
     */
    ve_ptr->taskMain.taskEnable = _VE_TASK_FINISHED;
    OSAL_logMsg("%s:%d finished\n", __FILE__, __LINE__);
    OSAL_semGive(ve_ptr->taskMain.finishSemId);
}


