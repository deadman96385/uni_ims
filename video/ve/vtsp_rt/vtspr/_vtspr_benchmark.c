/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28518 $ $Date: 2014-08-27 18:43:57 +0800 (Wed, 27 Aug 2014) $
 *
 */
#ifdef VTSP_ENABLE_BENCHMARK

/*
 * Benchmark functions
 */

/*
 * Define data to be reported. Choose one:
 * #define _VTSPR_BENCHMARK_MEASURE_AVG reports the largest average value
 * #define _VTSPR_BENCHMARK_MEASURE_PEAK reports the peak value
 */
#define _VTSPR_BENCHMARK_MEASURE_PEAK

#include "osal.h"

#include "vtspr.h"
#include "_vtspr_private.h"

#if defined(VTSP_ENABLE_BENCHMARK_NETLOG)
static char _mBenchBuf[128];
#endif

/*
 * Lock
 */
void _VTSPR_benchmarkIrqLock(void)
{
#if defined(VTSP_ENABLE_NET_LOOPBACK)
    vint temp;
    /*
     * Only lock out interrupts when NET_LOOPBACK is used, because making
     * system calls when interrupts are disabled, makes the system unstable.
     */
#warning XXX Locking out interrupts for testing
#warning XXX Code below is for ARM only
    asm volatile("MRS %0, cpsr": "=r" (temp) );
    asm volatile("ORR %0, %1, #1 <<7" : "=r" (temp) : "r" (temp) );
//    asm("ORR %0, %1, #1 <<6" : "=r" (temp) : "r" (temp) );
    asm volatile("MSR cpsr, %0" : : "r" (temp));
#endif
}

/*
 * Unlock
 */
void _VTSPR_benchmarkIrqUnLock(void)
{
#if defined(VTSP_ENABLE_NET_LOOPBACK)
    vint temp;
    /*
     * Only lock out interrupts when NET_LOOPBACK is used, because making
     * system calls when interrupts are disabled, makes the system unstable.
     */
#warning XXX Locking out interrupts for testing
#warning XXX Code below is for ARM only
    asm volatile("MRS %0, cpsr": "=r" (temp) );
    asm volatile("BIC %0, %1, #1 <<7" : "=r" (temp) : "r" (temp) );
//    asm("BIC %0, %1, #1 <<6" : "=r" (temp) : "r" (temp) );
    asm volatile("MSR cpsr, %0" : : "r" (temp));
#endif
}

/*
 * ======== _VTSPR_benchmarkCompute ========
 */
void _VTSPR_benchmarkCompute(
    VTSPR_Obj *vtspr_ptr)
{
    VTSPR_Benchmark      *benchmark_ptr;
    VTSPR_Queues         *q_ptr;
    _VTSPR_BenchmarkType  ctr;
    vint                  benchmarkCtr;
    uvint                 delta;
    uint32                cyclesPerSec;


    if (NULL == vtspr_ptr->benchmark_ptr) {
        /*
         * First time,
         * malloc all the benchmarks
         */
        if (NULL == (vtspr_ptr->benchmark_ptr =
                    OSAL_memCalloc(1, sizeof(VTSPR_Benchmark), 0))) {
                _VTSP_TRACE(__FILE__, __LINE__);
                return;
        }
        /*
         * Init variables
         */
        benchmark_ptr = vtspr_ptr->benchmark_ptr;
        for (ctr = 0; ctr < _VTSPR_BENCHMARK_NUM; ctr++) {
            benchmark_ptr->measureStart[ctr] = 0;
            benchmark_ptr->measureStop[ctr] = 0;
            benchmark_ptr->measureSum[ctr] = 0;
            benchmark_ptr->measureTicAvg[ctr] = 0;
            benchmark_ptr->measureTicHi[ctr] = 0;
            benchmark_ptr->measureTicPeak[ctr] = 0;
        }
        benchmark_ptr->ctr = 0;

        return;
    }

    /*
     * Init temp variables
     */
    benchmark_ptr = vtspr_ptr->benchmark_ptr;
    q_ptr = vtspr_ptr->q_ptr;

    /*
     * Compute the deltas of start and stop for each component
     * and record peak
     */
    for (ctr = 0; ctr < _VTSPR_BENCHMARK_NUM; ctr++) {
        delta = OSAL_archCountDelta(benchmark_ptr->measureStart[ctr],
                benchmark_ptr->measureStop[ctr]);
        if (benchmark_ptr->measureTicPeak[ctr] < delta) {
            benchmark_ptr->measureTicPeak[ctr] = delta;
        }
        benchmark_ptr->measureStart[ctr] = benchmark_ptr->measureStop[ctr] = 0;
        benchmark_ptr->measureSum[ctr] += delta;
    }

    /*
     * Compute the benchmarks
     * Then reinit variables
     */
    benchmarkCtr = benchmark_ptr->ctr++;
    if (0 == (benchmarkCtr & 0x7f)) {
        /*
         * 128 count, 1.28 sec
         * Perform one second averaging
         */
        for (ctr = 0; ctr < _VTSPR_BENCHMARK_NUM; ctr++) {
            benchmark_ptr->measureTicAvg[ctr] =
                    benchmark_ptr->measureSum[ctr] >> 7;
            if (benchmark_ptr->measureTicHi[ctr] <
                    benchmark_ptr->measureTicAvg[ctr]) {
                benchmark_ptr->measureTicHi[ctr] =
                        benchmark_ptr->measureTicAvg[ctr];
            }
            benchmark_ptr->measureSum[ctr] = 0;
        }
    }

#if defined(VTSP_ENABLE_BENCHMARK_NETLOG)
    if (0 == (benchmarkCtr & 0x07f)) {
        static OSAL_NetSockId fd = -1;
        OSAL_NetAddress addr;
        int size;

        if (-1 == fd) {
            if (OSAL_FAIL == OSAL_netSocket(&fd, OSAL_NET_SOCK_UDP)) {
                fd = -1;
                OSAL_logMsg("%s %d - ERROR: fail to open socket to send BENCHMARK data", __FILE__, __LINE__);
                return;
            }
        }
        /*
         * Output data:
         * Total, VTSPR, TIC, ECSR, ENCODE, DECODE
         * avg / hi / peak
         */
        sprintf(_mBenchBuf, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_TOTAL]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_VTSPR]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_TIC]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_ECSR]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_G729_ENCODE_ID0]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_G729_DECODE_ID0]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_G723_ENCODE_ID0] / 3),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicAvg[_VTSPR_BENCHMARK_G723_DECODE_ID0] / 3),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_TOTAL]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_VTSPR]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_TIC]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_ECSR]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_G729_ENCODE_ID0]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_G729_DECODE_ID0]),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_G723_ENCODE_ID0] / 3),
                OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[_VTSPR_BENCHMARK_G723_DECODE_ID0] / 3));

        size = strlen(_mBenchBuf);
        addr.ipv4 = OSAL_netHtonl(0xAC1000AB); /* 172.16.0.171 */
        addr.port = OSAL_netHtons(22345);
        OSAL_netSocketSendTo(&fd, _mBenchBuf, &size, &addr);
    }
#endif

    if (0 == (benchmarkCtr & 0x3ff)) {
        /*
         * 1024 count, 10.24 sec
         * For each benchmark, report either the largest average
         * or the peak value in units of kHz.
         */

        for (ctr = 0; ctr < _VTSPR_BENCHMARK_NUM; ctr++) {
#ifdef _VTSPR_BENCHMARK_MEASURE_AVG
            cyclesPerSec =
                    OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicHi[ctr]);
#elif defined(_VTSPR_BENCHMARK_MEASURE_PEAK)
            cyclesPerSec =
                    OSAL_archCountCyclesToKHz(benchmark_ptr->measureTicPeak[ctr]);
#endif

            switch(ctr) {
                case _VTSPR_BENCHMARK_VTSPR:
                    q_ptr->eventMsg.msg.benchmark.vtspr = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G729_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g729Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G729_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g729Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G729_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g729Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G729_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g729Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G711P1_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g711p1Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G711P1_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g711p1Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G711P1_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g711p1Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G711P1_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g711p1Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_ILBC_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.ilbcEncode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_ILBC_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.ilbcDecode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_ILBC_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.ilbcEncode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_ILBC_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.ilbcDecode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722P1_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g722p1Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722P1_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g722p1Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722P1_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g722p1Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722P1_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g722p1Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G723_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g723Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G723_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g723Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G723_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g723Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G723_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g723Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g722Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g722Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g722Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G722_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g722Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G726_ENCODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g726Encode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G726_DECODE_ID0:
                    q_ptr->eventMsg.msg.benchmark.g726Decode0 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G726_ENCODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g726Encode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_G726_DECODE_ID1:
                    q_ptr->eventMsg.msg.benchmark.g726Decode1 = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_GAMRNB_ENCODE_ID0:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrnbEncode0 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRNB_DECODE_ID0:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrnbDecode0 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRNB_ENCODE_ID1:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrnbEncode1 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRNB_DECODE_ID1:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrnbDecode1 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID0:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrwbEncode0 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRWB_DECODE_ID0:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrwbDecode0 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID1:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrwbEncode1 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_GAMRWB_DECODE_ID1:
                    /* Divided by 2 due to 20ms a cycle*/
                    q_ptr->eventMsg.msg.benchmark.gamrwbDecode1 = cyclesPerSec / 2;
                    break;
                case _VTSPR_BENCHMARK_AEC_COMPUTE_ROUT:
                    q_ptr->eventMsg.msg.benchmark.aecComputeRout = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_AEC_COMPUTE_SOUT:
                    q_ptr->eventMsg.msg.benchmark.aecComputeSout = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_ECSR:
                    q_ptr->eventMsg.msg.benchmark.ecsr = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_FMTD:
                    q_ptr->eventMsg.msg.benchmark.fmtd = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_DTMF:
                    q_ptr->eventMsg.msg.benchmark.dtmf = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_T38:
                    q_ptr->eventMsg.msg.benchmark.t38 = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_TIC:
                    q_ptr->eventMsg.msg.benchmark.tic = cyclesPerSec;
                    break;
                case _VTSPR_BENCHMARK_RTP_XMIT:
                    q_ptr->eventMsg.msg.benchmark.rtpXmit = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_RTP_RECV:
                    q_ptr->eventMsg.msg.benchmark.rtpRecv = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_NFE:
                    q_ptr->eventMsg.msg.benchmark.nfe = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_DCRM_NEAR:
                    q_ptr->eventMsg.msg.benchmark.dcrmNear = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_DCRM_PEER:
                    q_ptr->eventMsg.msg.benchmark.dcrmPeer = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_JB_GET:
                    q_ptr->eventMsg.msg.benchmark.jbGet = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_JB_PUT:
                    q_ptr->eventMsg.msg.benchmark.jbPut = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_CIDS:
                    q_ptr->eventMsg.msg.benchmark.cids = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_EVENTS:
                    q_ptr->eventMsg.msg.benchmark.events = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_CMD:
                    q_ptr->eventMsg.msg.benchmark.cmd = cyclesPerSec;
                    break;

                case _VTSPR_BENCHMARK_TOTAL:
                    q_ptr->eventMsg.msg.benchmark.total = cyclesPerSec;
                    break;
                default:
                    break;
            }
            benchmark_ptr->measureTicHi[ctr] = 0;
            benchmark_ptr->measureTicPeak[ctr] = 0;
        }
        /*
         * Send the event
         */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_BENCHMARK;
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.tick = vtspr_ptr->dsp_ptr->tick1ms;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
    }
}

/*
 * ======== _VTSPR_benchmarkStart ========
 */
void _VTSPR_benchmarkStart(
    VTSPR_Obj *vtspr_ptr,
    vint       component,
    vint       option)
{
    VTSPR_Benchmark *benchmark_ptr;

    benchmark_ptr = vtspr_ptr->benchmark_ptr;

    if (NULL == benchmark_ptr) {
        return;
    }

    if (0 < option) {
        _VTSPR_benchmarkIrqLock();
    }
    benchmark_ptr->measureStart[component] = OSAL_archCountGet();

}

/*
 * ======== _VTSPR_benchmarkStop ========
 */
void _VTSPR_benchmarkStop(
    VTSPR_Obj *vtspr_ptr,
    vint       component,
    vint       option)
{
    VTSPR_Benchmark *benchmark_ptr;

    benchmark_ptr = vtspr_ptr->benchmark_ptr;

    if (NULL == benchmark_ptr) {
        return;
    }

    benchmark_ptr->measureStop[component] = OSAL_archCountGet();
    if (0 < option) {
        _VTSPR_benchmarkIrqUnLock();
    }
}

/*
 * ======== _VTSPR_benchmarkReset ========
 */
void _VTSPR_benchmarkReset(
    VTSPR_Obj *vtspr_ptr,
    vint       component,
    vint       option)
{
    VTSPR_Benchmark *benchmark_ptr;

    benchmark_ptr = vtspr_ptr->benchmark_ptr;

    if (NULL == benchmark_ptr) {
        return;
    }

    benchmark_ptr->measureStart[component] = 0;
    benchmark_ptr->measureStop[component] = 0;

}


#endif /* VTSP_ENABLE_BENCHMARK */
