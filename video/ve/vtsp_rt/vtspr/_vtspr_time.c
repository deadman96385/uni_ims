/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 *
 */

/*
 * Time functions
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== VTSPR_getTime() ========
 *
 * Return the current sample time, for use in packet arrival.
 * This should return the highest resolution time in the system,
 * for the best jitter buffer receive performance.
 *
 * Ideally, this time returns a per-sample counter.
 */
OSAL_INLINE uint32 VTSPR_getTime(
    VTSPR_DSP   *dsp_ptr)
{
    /* 8 samples per 1 ms */
    return (dsp_ptr->tick1ms << 3);
}

/*
* ======== _VTSPR_time ========
* Top of loop function to track/measure time
*/
OSAL_INLINE void _VTSPR_time(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_StreamObj *stream_ptr;
    VTSPR_ChanObj   *chan_ptr;
    vint             streamId;
    uint32           streamMask;
    vint             infc;

    /* New 10 ms tick
     */
    dsp_ptr->tick10ms++;
    dsp_ptr->tick1ms += 10;

    /* Heartbeat event to VTSP
     */
    dsp_ptr->heartbeat--;
    if (0 == dsp_ptr->heartbeat) { 
        /* Generate heartbeat clock event to vtsp (10sec)
         */
        dsp_ptr->heartbeat = VTSP_HEARTBEAT_EVENT_COUNT - 1;

        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TIMER;
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.time.reason = VTSP_EVENT_ACTIVE;
        q_ptr->eventMsg.msg.time.period = VTSP_HEARTBEAT_EVENT_COUNT * 10;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
        if ((VTSP_HEARTBEAT_EVENT_COUNT - 1) == dsp_ptr->heartbeat) {
            dsp_ptr->procTickHi = 0;
        }
    }
    dsp_ptr->heartbeat1s--;
    if (0 == dsp_ptr->heartbeat1s) { 
        /* Generate 1sec heartbeat clock event to vtsp
         */
        dsp_ptr->heartbeat1s = 100 - 1;

        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TIMER;
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.time.reason = VTSP_EVENT_ACTIVE;
        q_ptr->eventMsg.msg.time.period = 1000;
        q_ptr->eventMsg.msg.time.tickAvg = dsp_ptr->procTickAvg;
        if (0 == dsp_ptr->procTickHi) {
            q_ptr->eventMsg.msg.time.tickAvg = dsp_ptr->procTickAvg;
        }
        else {
            q_ptr->eventMsg.msg.time.tickHi = dsp_ptr->procTickHi;
        }

        /* 
         * Add Rx/Tx dB for infc0 to event Msg
         * XXX: Needs modification to use all infcs
         */
        infc       = 0;
        chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        q_ptr->eventMsg.msg.time.dbRx = chan_ptr->dbRx;
        q_ptr->eventMsg.msg.time.dbTx = chan_ptr->dbTx;

        /*
         * Bitrate for entire vtspr.
         * This will give bitrate of bits per second
         * because this is 1 sec event.
         */
        q_ptr->eventMsg.msg.time.bitrate = vtspr_ptr->bitrate;
        vtspr_ptr->bitrate = 0;

        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);

        /*
         * For each actively receiving stream, send JB statistics message.
         */
        _VTSPR_FOR_ALL_INFC(infc) {
            for (streamId = _VTSP_STREAM_PER_INFC - 1;  streamId >= 0;
                    streamId--) { 
                stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc,
                        streamId);
                streamMask = stream_ptr->algStreamState;
                if (0 != (VTSPR_ALG_STREAM_T38 & streamMask)) { 
#ifdef VTSP_ENABLE_T38
                    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_T38;
                    q_ptr->eventMsg.infc = infc;
                    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                    q_ptr->eventMsg.msg.t38.reason = VTSP_EVENT_T38_STATISTIC;
                    q_ptr->eventMsg.msg.t38.streamId = streamId;
                    q_ptr->eventMsg.msg.t38.state =
                           (vint)stream_ptr->fr38Obj_ptr->status_ptr->frs_state;
                    q_ptr->eventMsg.msg.t38.pages =
                           (vint)stream_ptr->fr38Obj_ptr->status_ptr->frs_pages;
                    q_ptr->eventMsg.msg.t38.trainDown =
                           (vint)stream_ptr->fr38Obj_ptr->status_ptr->
                                                                frs_train_down;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
#endif
                }
                else if ((0 != (VTSPR_ALG_STREAM_JB & streamMask)) && 
                        ((VTSP_STREAM_DIR_SENDRECV ==
                        stream_ptr->streamParam.dir) ||
                        (VTSP_STREAM_DIR_RECVONLY ==
                        stream_ptr->streamParam.dir))) { 
                   q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_JB;
                   q_ptr->eventMsg.infc = infc;
                   q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                   q_ptr->eventMsg.msg.jb.reason = VTSP_EVENT_ACTIVE;
                   q_ptr->eventMsg.msg.jb.streamId = streamId;
                   q_ptr->eventMsg.msg.jb.level = stream_ptr->jbObj.level;
                   q_ptr->eventMsg.msg.jb.jbSize = stream_ptr->jbObj.jbSize;
                   q_ptr->eventMsg.msg.jb.drop = stream_ptr->jbObj.stats.drops;
                   q_ptr->eventMsg.msg.jb.plc = stream_ptr->jbObj.stats.plcs;
                   q_ptr->eventMsg.msg.jb.leaks = stream_ptr->jbObj.stats.leaks;
                   q_ptr->eventMsg.msg.jb.accms = stream_ptr->jbObj.stats.accms;
                   q_ptr->eventMsg.msg.jb.lastTimeStamp = stream_ptr->lastTimeStamp;
                   q_ptr->eventMsg.msg.jb.lastTimeArrival = stream_ptr->lastTimeArrival;
                   VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                }
            }
        }
    }

#ifdef VTSP_ENABLE_BENCHMARK
    _VTSPR_benchmarkCompute(vtspr_ptr);
#endif

}


