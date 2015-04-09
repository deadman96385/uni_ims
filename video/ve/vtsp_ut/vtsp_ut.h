/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7474 $ $Date: 2008-08-29 13:15:22 -0400 (Fri, 29 Aug 2008) $
 *
 */


#ifndef _VTSP_UT_H_
#define _VTSP_UT_H_

#include "osal.h"
#include "vtsp.h"

#define UT_RTP_PORT_START (5001)
#undef UT_MHZ_NET_LOG

/*
 * CN enable mask for coders, including wideband
 */
#ifdef VTSP_ENABLE_STREAM_16K
#define UT_STREAM_TEST_CN_MASK \
        (VTSP_MASK_CODER_G711U | VTSP_MASK_CODER_G711A | \
         VTSP_MASK_CODER_G726_32K | VTSP_MASK_CODER_G729 | \
         VTSP_MASK_CODER_16K_MU | VTSP_MASK_CODER_G722 | \
         VTSP_MASK_CODER_G723_30MS | VTSP_MASK_CODER_G722P1_20MS | \
         VTSP_MASK_CODER_SILK_20MS_8K | VTSP_MASK_CODER_SILK_20MS_16K)
#else /* not VTSP_ENABLE_STREAM_16K */
#define UT_STREAM_TEST_CN_MASK \
        (VTSP_MASK_CODER_G711U | VTSP_MASK_CODER_G711A | \
         VTSP_MASK_CODER_G726_32K | VTSP_MASK_CODER_G729 | \
         VTSP_MASK_CODER_G723_30MS | VTSP_MASK_CODER_SILK_20MS_8K)
#endif /* end VTSP_ENABLE_STREAM_16K */

/*
 * DR enable mask for coders, including wideband
 */
#ifdef VTSP_ENABLE_STREAM_16K
#define UT_STREAM_TEST_DR_MASK \
        (VTSP_MASK_CODER_G711U | VTSP_MASK_CODER_G711A | \
         VTSP_MASK_CODER_G726_32K | VTSP_MASK_CODER_G729 | \
         VTSP_MASK_CODER_ILBC_20MS | \
         VTSP_MASK_CODER_16K_MU | VTSP_MASK_CODER_G722 | \
         VTSP_MASK_CODER_G723_30MS | VTSP_MASK_CODER_G722P1_20MS | \
         VTSP_MASK_CODER_SILK_20MS_8K | VTSP_MASK_CODER_SILK_20MS_16K)
#else /* not VTSP_ENABLE_STREAM_16K */
#define UT_STREAM_TEST_DR_MASK \
        (VTSP_MASK_CODER_G711U | VTSP_MASK_CODER_G711A | \
         VTSP_MASK_CODER_G726_32K | VTSP_MASK_CODER_G729 | \
         VTSP_MASK_CODER_ILBC_20MS | \
         VTSP_MASK_CODER_G723_30MS | VTSP_MASK_CODER_SILK_20MS_8K)
#endif /* end VTSP_ENABLE_STREAM_16K */


#define UT_FAILMSG                                      \
    (void)OSAL_logMsg("%s:%d FAIL rval=%d\n", __FILE__, __LINE__, UT_rval); \
    UT_failures++;

#define UT_EXPECT0(retval, func)                        \
    if (retval != (UT_rval = func())) { UT_FAILMSG; }

#define UT_EXPECT1(retval, func, arg1)                  \
    if (retval != (UT_rval = func(arg1))) { UT_FAILMSG; }

#define UT_EXPECT2(retval, func, arg1, arg2)            \
    if (retval != (UT_rval = func(arg1, arg2))) { UT_FAILMSG; }

#define UT_EXPECT3(retval, func, arg1, arg2, arg3)      \
    if (retval != (UT_rval = func(arg1, arg2, arg3))) { \
        UT_FAILMSG; }

#define UT_EXPECT4(retval, func, arg1, arg2, arg3, arg4)      \
    if (retval != (UT_rval = func(arg1, arg2, arg3, arg4))) { \
        UT_FAILMSG; }

#define UT_EXPECT5(retval, func, arg1, arg2, arg3, arg4, arg5)      \
    if (retval != (UT_rval = func(arg1, arg2, arg3, arg4, arg5))) { \
        UT_FAILMSG; }

#define UT_EXPECT6(retval, func, arg1, arg2, arg3, arg4, arg5, arg6)      \
    if (retval != (UT_rval = func(arg1, arg2, arg3, arg4, arg5, arg6))) { \
        UT_FAILMSG; }

typedef enum {
    UT_UNKNOWN = -2,
    UT_PASS = 1,
    UT_FAIL = -1
} UT_Return;

typedef enum {
    UT_EVENT_MASK_TIC           = 0x0001,
    UT_EVENT_MASK_TONE          = 0x0002,
    UT_EVENT_MASK_T38           = 0x0004,
    UT_EVENT_MASK_T38_STATS     = 0x0008,
    UT_EVENT_MASK_MANAGE        = 0x0010,
    UT_EVENT_MASK_RTP           = 0x0020,
    UT_EVENT_MASK_RTP_FULL      = 0x0040,
    UT_EVENT_MASK_JB            = 0x0080,
    UT_EVENT_MASK_JB_FULL       = 0x0100,
    UT_EVENT_MASK_RTCP          = 0x0200,
    UT_EVENT_MASK_TIMER_1       = 0x0400,
    UT_EVENT_MASK_TIMER_10      = 0x0800,
    UT_EVENT_MASK_EC            = 0x1000
} UT_EventMask;

typedef enum {
    UT_INFC = 0,
    UT_STREAM_ID,
    UT_IP_ADDRESS,
    UT_IP_PORT,
    UT_SILCOMP_MASK,
    UT_DTMFR_ENCODE_MASK,
    UT_EXTENSION_WORD,
    UT_CONFERENCE_MASK,
    UT_CODER_ENCODE_TIME,
    UT_CN_ENCODE_TIME,
    UT_ECHO_CANCELLER,
    UT_STREAM_DIRECTION,
    UT_TEST_LENGTH,
    UT_TEST_VIDEO_KEYINT,
    UT_ENCODER,
    UT_ENCODER_VIDEO,
    UT_EC_BYPASS,
    UT_FMTD_GBL_PWR_INFC,
    UT_FMTD_GBL_PWR_PEER,
    UT_TONE_POWER,
    UT_TONE_FREQ,
    UT_CONTROL_IO,
    UT_CONTROL_AEC,
    UT_JB_MODE,
    UT_JB_INIT_LEVEL,
    UT_JB_MAX_LEVEL,
    UT_GAIN,
    UT_AEC_TAIL_LEN,
    UT_CN_PWR_ATTEN,
    UT_RTP_REDUN_LEVEL,
    UT_RTP_REDUN_HOP,
    UT_JB_FIXED_LEVEL,
    UT_DTMFR_RATE
/* add more as needed */
} UT_TestOption;

typedef UT_Return (UT_TestFunc)(vint arg);

typedef struct {
    unsigned char   cmd[20];
    UT_TestFunc    *func_ptr;
    unsigned char   desc[50];
} UT_TestTableItem;

/*
 * Externs
 */
extern int32            UT_rval;

extern uint32           UT_silComp;
extern uint32           UT_dtmfRelay;
extern uint32           UT_encoder;

extern uint32           UT_eventMask;

extern OSAL_NetAddress  UT_sendAddr;
extern OSAL_NetAddress  UT_recvAddr;
extern uint32           UT_sendPort;
extern uint32           UT_recvPort;
extern uint32           UT_confMask;

extern uint64           UT_failures;
extern vint             UT_run;
extern vint             UT_testTimeSec;
extern uvint            UT_newStream;
extern VTSP_Context    *UT_vtsp_ptr;
extern uvint            UT_detect;
extern uvint            UT_extension;
extern vint             UT_startPort;
extern vint             UT_endPort;
extern vint             UT_testVideoKeyIntSec;

extern VTSP_CIDData     UT_cidObj;
extern uvint            UT_eventTaskDisplay;
extern vint             UT_streamDir;
extern vint             UT_streamEC;
extern vint             UT_streamJBFixed;
extern uvint            UT_encodeTime;
extern uvint            UT_encodeTime_cn;
extern int16            UT_dtmfThreshold;
extern int16            UT_vadThreshold;

extern int              UT_sockFd;

/*
 * External function
 */
int D2_getLine(
    char *buf,
    unsigned int max);

/*
 * Prototypes
 */
int UT_main(
    int argc,
    char *argv[]);

UT_Return UT_testStreamTone(
        vint arg);

UT_Return UT_testTone5ms(
    vint infc);

UT_Return UT_testToneQuad(
    vint infc);

UT_Return UT_streamPeer(
    vint arg);

UT_Return UT_streamLoopback(
    vint arg);

UT_Return UT_streamLoopbackVideo(
        vint arg);

UT_Return UT_streamPeerVideo(
        vint arg);

UT_Return UT_streamLoopbackToneMixing(
    vint arg);

UT_Return UT_streamVQTInfc2(
vint arg);

UT_Return UT_streamVce0(
    vint arg);

UT_Return UT_streamVce1(
    vint arg);

UT_Return UT_streamVcb0(
    vint arg);

UT_Return UT_streamJBstats(
        vint arg);

UT_Return UT_streamVQT(
    vint arg);

UT_Return UT_streamVQTall(
    vint arg);

UT_Return UT_streamConfVQT(
    vint arg);

UT_Return UT_streamHold(
    vint arg);

UT_Return UT_vtspInit(
    vint arg);

UT_Return UT_testControlHook(
        vint infc);

UT_Return UT_testNetTx(
        vint arg);

UT_Return UT_testBind(
        vint infc);

UT_Return UT_initStreamStruct(
        VTSP_Stream *stream_ptr,
        vint streamId);

UT_Return UT_testToneToNet(
    vint arg);

UT_Return UT_testToneDbToNet(
    vint arg);

UT_Return UT_testToneLocal(
    vint arg);
UT_Return UT_testToneDbLocal(
    vint arg);

UT_Return UT_testRfc4733(
    vint arg);

UT_Return UT_srtpStream(
    vint arg);

UT_Return UT_srtpStreamVQT(
        vint arg);

void UT_printRetVal(
        int v);

UT_Return UT_preInit(
    vint arg);

UT_Return UT_shutdown(
    vint arg);

UT_Return UT_init(
    vint arg);

UT_Return UT_ticflash(
    vint arg);

VTSP_Stream *UT_updateStreamData(
        vint infc,
    vint streamId);

VTSP_StreamVideo *UT_updateStreamDataVideo(
        vint infc,
    vint streamId);

UT_Return UT_printEvent(
    VTSP_EventMsg  *event_ptr);

UT_Return UT_showInfo(
    void);

UT_Return UT_event(
    void);

UT_Return UT_testToneGain(
    vint infc);

UT_Return UT_testToneLocalSeq(
    vint infc);

UT_Return UT_testStreamPeerToneMixing(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_stream1(
    vint *arg);

UT_Return UT_testStreamVQTInfc2(
    vint         infc1,
    VTSP_Stream *stream1_ptr,
    vint         infc2,
    VTSP_Stream *stream2_ptr);

UT_Return UT_testStreamVQT(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_testStreamVQTall(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_testStreamHold(
    vint         infc,
    VTSP_Stream *stream0_ptr,
    VTSP_Stream *stream1_ptr);

UT_Return UT_testStreamPeer(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_testStreamPeerVideo(
        vint         infc,
        VTSP_StreamVideo *stream_ptr);

UT_Return UT_testStreamConf(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_testStreamConfVQT(
    vint         infc,
    VTSP_Stream *stream1_ptr,
    VTSP_Stream *stream2_ptr);

UT_Return UT_testRelay(
    vint infc);

UT_Return UT_testLevels(
    vint infc);

uint32 UT_getUserTestParam(
    UT_TestOption opt);

UT_Return UT_processEvent(
    VTSP_EventMsg  *event_ptr);

UT_Return _UT_configNTTTones(
    vint infc);

uvint _UT_updateCname(
    uvint infc,
    uvint cId);

UT_Return UT_testSrtpStream(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_testSrtpStreamVQT(
    vint         infc,
    VTSP_Stream *stream_ptr);

UT_Return UT_controlIO(
    vint arg);

UT_Return UT_controlAEC(
    vint arg);

UT_Return UT_controlJB(
    vint arg);

UT_Return UT_controlCN(
    vint arg);

UT_Return UT_controlGainDb(
    vint arg);

/*
 * Functions migrated from vPort 1.6
 */
UT_Return UT_updateCid(
    VTSP_CIDData *cid_ptr);
UT_Return UT_printCid(
    VTSP_CIDData *cid_ptr);
UT_Return UT_testRing(
    vint infc);
UT_Return UT_testVmwi(
    vint infc);
UT_Return UT_testToneGain(
    vint infc);
UT_Return UT_testToneLocal(
    vint infc);
UT_Return UT_CalibTx(
    vint infc);
UT_Return UT_CalibRx(
    vint infc);
UT_Return UT_testT38Ans(
        vint         infc,
        VTSP_Stream *stream_ptr);
UT_Return UT_testT38Call(
        vint         infc,
        VTSP_Stream *stream_ptr);
UT_Return UT_g711Ans(
        vint         infc,
        VTSP_Stream *stream_ptr);
UT_Return UT_g711Call(
        vint         infc,
        VTSP_Stream *stream_ptr);
UT_Return UT_testStreamConf(
        vint         infc,
        VTSP_Stream *stream_ptr);
UT_Return UT_testStreamConfVQT(
        vint         infc,
        VTSP_Stream *stream1_ptr,
        VTSP_Stream *stream2_ptr);
UT_Return UT_flowPlayLocal(
        vint         infc);
UT_Return UT_flowPlayStreamLocal(
        vint         infc);
UT_Return UT_flowPlayStreamPeer(
        vint         infc);
UT_Return UT_flowPlayRecord(
        vint         infc);
#ifdef VTSP_ENABLE_T38
UT_Return UT_testfaxAns(
        vint arg);
UT_Return UT_testfaxCall(
        vint arg);
#endif
UT_Return UT_g711faxAns(
        vint arg);
UT_Return UT_g711faxCall(
        vint arg);
UT_Return UT_testFxoDial(
        vint infc);
#ifdef VTSP_ENABLE_PULSE_GENERATE
UT_Return UT_testFxoPulseDial(
        vint infc);
#endif
UT_Return UT_streamFxoDial(
        vint infc);
UT_Return UT_streamFxoIn(
        vint infc);
UT_Return UT_testUtdConfig(
        vint infc);
UT_Return UT_callerIdSeq(
        vint infc);
UT_Return UT_testCidSeq(
    vint         infc,
    vint         type,
    vint         tId,
    vint         vector,
    VTSP_Stream *stream_ptr);
UT_Return UT_Netlog(
     vint arg);

#endif
