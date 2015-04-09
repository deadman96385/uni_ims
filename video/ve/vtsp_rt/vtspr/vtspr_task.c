/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */


/*
 * This file is the voice processing task
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#if defined(VTSP_ENABLE_SYSTEM_CHECK)
#include "system_check.c"
#endif

extern int VHW_isSleeping(void);
extern uint32 OSAL_archCountGet(void);
extern uint32 OSAL_archCountCyclesToUsec(vint count);

/*
 * ======== VTSPR_task() ========
 *
 * Main processing loop for voice 
 */
void VTSPR_task(
    VTSPR_Obj *vtspr_ptr)
{
    VTSPR_Queues      *q_ptr;
    VTSPR_DSP         *dsp_ptr;
    VTSPR_ChanObj     *chan_ptr;


    vint              *rx_ptr; 
    vint              *tx_ptr;
    vint               infc;  /* must be signed */
    void              *tic_ptr;
#ifdef VTSP_ENABLE_REALTIME_MISS_CHECK
    uint32             startCnt;
    uint32             stopCnt;
    uint32             diffCnt;
    uint32             timeUsecs;
#endif

    q_ptr = vtspr_ptr->q_ptr;
    dsp_ptr = vtspr_ptr->dsp_ptr;

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
    OSAL_logMsg( "        %s\n", D2_Release_VTSP_RT);


    /*
     * Until application sends 'start', all commands will be processed
     * but voice, TIC, and events will not run
     * --------
     */
#ifndef VTSP_ENABLE_AUTOSTART
    while (0 != (VTSPR_TASK_WAIT & vtspr_ptr->task10ms.taskEnable)) { 
        /*
         * Process commands
         * waiting for START
         */
        _VTSPR_recvAllCmd(vtspr_ptr, q_ptr, dsp_ptr);

        /* Yield to other tasks */
        OSAL_taskDelay(100);
    }
#else
    vtspr_ptr->autoStartCmd = 1;
#endif /* VTSP_ENABLE_AUTOSTART */
    /*
     *
     * Start the real-time PCM voice and real time loop. 
     *
     * --------
     */
    VHW_start();

    while (0 != (VTSPR_TASK_RUN & vtspr_ptr->task10ms.taskEnable)) {
#ifdef VTSP_ENABLE_REALTIME_MISS_CHECK
        /* Get current time */
        startCnt = OSAL_archCountGet();
#endif
        /*
         * Run timing function
         */
        _VTSPR_time(vtspr_ptr, q_ptr, dsp_ptr);

#ifdef VTSP_ENABLE_SYSTEM_CHECK
        if (OSAL_FALSE == _VTSPR_audioPack()) {
            COMM_fill(tx_ptr, 0xAA, VHW_NUM_PCM_CH * VHW_PCM_BUF_SZ);
        }
#endif
        /*
         * Get 10ms audio data, blocking function.
         */
        VHW_exchange(&tx_ptr, &rx_ptr);

#ifdef VTSP_ENABLE_SYSTEM_CHECK
        if (OSAL_FALSE == _VTSPR_audioPack()) {
            COMM_fill(rx_ptr, 0xAA, VHW_NUM_PCM_CH * VHW_PCM_BUF_SZ);
        }
#endif
        /* 
         * Set audio buffers.
         * Always set buffers even if FXS/FXO/AUDIO is not enabled.
         * Run Physical Interface
         */
#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_TOTAL, 1);
        _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_TIC, 1);
#endif
#if (_VTSP_INFC_NUM > 0)
        _VTSPR_FOR_ALL_INFC(infc) {
            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
            chan_ptr->tx_ptr = VHW_GETBUF(tx_ptr, infc);
            chan_ptr->rx_ptr = VHW_GETBUF(rx_ptr, infc);
            if (NULL == chan_ptr->tx_ptr || NULL == chan_ptr->rx_ptr) {
                OSAL_logMsg("\tGETBUF NULL tx_ptr=%p, rx_ptr=%p"
                        " infc=%d chan_ptr=%p\n",
                        tx_ptr,rx_ptr,infc,chan_ptr);
            }
            TIC_run(tic_ptr);
        }
#endif  /* _VTSP_INFC_NUM > 0 */

#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_TIC, 1);
        _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_VTSPR, 1);
#endif
        /*
         * Generate upstream events
         */
#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_EVENTS, 1);
#endif
        _VTSPR_genEventFxo(vtspr_ptr, q_ptr, dsp_ptr);
        _VTSPR_genEventFxs(vtspr_ptr, q_ptr, dsp_ptr);
        _VTSPR_genEventFx(vtspr_ptr, q_ptr, dsp_ptr);
        _VTSPR_genEventHs(vtspr_ptr, q_ptr, dsp_ptr);
        _VTSPR_genEventStream(vtspr_ptr, q_ptr, dsp_ptr);
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_EVENTS, 1);
#endif
        /* If VHW is detached skip processing buffers */
        if (!VHW_isSleeping()) {
            /*
             * Get all of the RTP data from network.
             */
            if (VTSP_OK != _VTSPR_rtpRecv(vtspr_ptr, q_ptr, dsp_ptr, 
                    &vtspr_ptr->net_ptr->rtpObj[0])) { 
                _VTSP_TRACE(__FILE__, __LINE__);
            }

            if (VTSP_OK != _VTSPR_stunProcess(q_ptr, vtspr_ptr->net_ptr)) { 
                _VTSP_TRACE(__FILE__, __LINE__);
            }

            /*
             * Process audio
             */
#ifndef VTSP_ENABLE_MP_LITE
            _VTSPR_audioRxFormatHs(vtspr_ptr, dsp_ptr);
            _VTSPR_audioRxFormat(vtspr_ptr, dsp_ptr);

            _VTSPR_audioRemoveEchoFx(vtspr_ptr, dsp_ptr);
            _VTSPR_audioRemoveEchoHs(vtspr_ptr, dsp_ptr);

            _VTSPR_audioPreFilter(dsp_ptr);

            _VTSPR_audioNear(vtspr_ptr, dsp_ptr);
#endif
            _VTSPR_audioStreamDecode(vtspr_ptr, q_ptr, dsp_ptr);

#ifndef VTSP_ENABLE_MP_LITE
            _VTSPR_audioConfToLocal(vtspr_ptr, dsp_ptr);
#endif

            _VTSPR_audioConfToPeer(vtspr_ptr, q_ptr, dsp_ptr);
#ifdef VTSP_ENABLE_T38
            _VTSPR_processT38(vtspr_ptr, dsp_ptr);
#endif

#ifdef VTSP_ENABLE_CIDS
            /*
             * Run CIDS after processing audio.
             * This allows CIDS to overwrite all other audio output.
             *
             * This runs both onhook and offhook CallerId Send.
             */
            _VTSPR_callerIdSend(vtspr_ptr, q_ptr, dsp_ptr);
#endif
#ifndef VTSP_ENABLE_MP_LITE
            _VTSPR_audioTxDcrm(vtspr_ptr, dsp_ptr);
            _VTSPR_audioRoutFx(dsp_ptr);
            _VTSPR_audioRoutHs(dsp_ptr);
            _VTSPR_audioTxFormat(vtspr_ptr, dsp_ptr);
            _VTSPR_audioTxFormatHs(vtspr_ptr, dsp_ptr);
#endif

            /*
             * Get any RTCP packets from network.
             */
            if (VTSP_OK != _VTSPR_rtcpRecv(q_ptr, dsp_ptr,
                        vtspr_ptr->net_ptr)) { 
                _VTSP_TRACE(__FILE__, __LINE__);
            }
#ifdef VTSP_ENABLE_REALTIME_MISS_CHECK
            /* Get current time */
            stopCnt = OSAL_archCountGet();
            diffCnt = OSAL_archCountDelta(startCnt, stopCnt);
            timeUsecs = OSAL_archCountCyclesToUsec(diffCnt);
            if (timeUsecs > 10000) {
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_ERROR;
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.msg.debug.arg1 = timeUsecs;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            }
#endif
        }
#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_VTSPR, 1);
        _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_TOTAL, 1);
#endif
       /*
        * Run all commands sent down from user level
        */
        _VTSPR_recvAllCmd(vtspr_ptr, q_ptr, dsp_ptr);
    }

    /*
     * Shutdown voice 
     * --------
     */
    /*
     * Shutdown RTP streams and interfaces.
     */
    _VTSPR_rtpShutdown(&vtspr_ptr->net_ptr->rtpObj[0]);

    /*
     * ToNet Event to application prior to exit, on globalQ and each infcQ
     */
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_SHUTDOWN;
    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
    q_ptr->eventMsg.msg.shutdown.reason = VTSP_EVENT_HALTED;
    q_ptr->eventMsg.msg.shutdown.status = 0;
    q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
    _VTSPR_FOR_ALL_INFC(infc) {
        /*
         * ToNet shutdown msg to each infc eventQ 
         */
        q_ptr->eventMsg.infc = infc;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
    }

    TIC_shutdown();

    /* 
     * Continue to run for 1/10 heartbeat event time (1 sec)
     * to allow application to recv event
     * and to allow TIC to process shutdown cmds
     */
    dsp_ptr->heartbeat = (VTSP_HEARTBEAT_EVENT_COUNT / 10);
    while (dsp_ptr->heartbeat-- != 0) { 
        /*
         * should zero tx_ptr here to send silence to phone
         */
        _VTSPR_FOR_ALL_INFC(infc) {
            tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
            TIC_run(tic_ptr);
        }
    }

    VHW_shutdown();

#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
    /* If VE use external codec, VE should shutdown it. */
    DSP_shutdown();
#endif

    /*
     * Exit spawned task
     */
    vtspr_ptr->task10ms.taskEnable = VTSPR_TASK_FINISHED;
    OSAL_logMsg("%s:%d finished\n", __FILE__, __LINE__);
    OSAL_semGive(vtspr_ptr->task10ms.finishSemId);

    OSAL_memSet(vtspr_ptr->dsp_ptr, 0, sizeof(VTSPR_DSP));
    OSAL_memFree(vtspr_ptr->dsp_ptr, 0);

    OSAL_memSet(vtspr_ptr->q_ptr, 0, sizeof(VTSPR_Queues));
    OSAL_memFree(vtspr_ptr->q_ptr, 0);


    OSAL_memSet(vtspr_ptr->net_ptr, 0, sizeof(VTSPR_NetObj));
    OSAL_memFree(vtspr_ptr->net_ptr, 0);
}


