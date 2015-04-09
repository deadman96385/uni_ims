/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIEVETRY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28518 $ $Date: 2014-08-27 18:43:57 +0800 (Wed, 27 Aug 2014) $
 *
 */

/* Test stream functions
 * VTSP must be initialized and started prior to running these functions.
 */

#include "osal.h"
#include "vtsp.h"
#include "vtsp_ut.h"

#if defined(OSAL_NUCLEUS)
/*
 * Needed to htonl() and htons()
 */
#include "global.h"
#endif

#if defined(OSAL_VXWORKS)
#include <netinet/in.h>
#endif

#if defined(OSAL_PTHREADS)
#include <netinet/in.h>
#endif

#define PAYLOAD_SZ  (5 * VTSP_BLOCK_MAX_SZ)

#ifdef VTSP_ENABLE_PLAY
/*
 * Define two streams for the purpose of testing 3-way calling
 */
static VTSP_Stream  stream0 = { 
    0,                          /* streamId */
    VTSP_STREAM_DIR_SENDRECV,   /* Dir */
    VTSP_STREAM_PEER_NETWORK,   /* Peer */
    VTSP_CODER_G711U,           /* Encoder */
    { 10, 10, 10000, 10, 10, 20, 30,  10},  /* encodeTime */
    {  0,  1,    13, 18,  2, 97, 97, 101},  /* encodeType
                                               G711U,G711A,CN,G729,G726_32,ILBC_20,
                                               ILBC_30,DTMFR */
    {  0,  1,    13, 18,  2, 97, 97, 101},  /* decodeType */
    0,                          /* extension */
    0,                          /* dtmfRelay */
    0,                          /* silenceComp */
    0,                          /* confMask */
    0,                          /* remoteAddr.ipv4 */
    0,                          /* remoteAddr.port */
    0,                          /* remoteAddr.port */
    0,                          /* localAddr.ipv4 */
    0,                          /* localAddr.port */
    0                           /* localAddr.port */
};

static VTSP_Stream  stream1 = { 
    1,                          /* streamId */
    VTSP_STREAM_DIR_SENDRECV,   /* Dir */
    VTSP_STREAM_PEER_NETWORK,   /* Peer */
    VTSP_CODER_G711U,           /* Encoder */
    { 10, 10, 10, 10, 10, 20, 30,  10},  /* encodeTime */
    {  0,  1, 13, 18,  2, 97, 97, 101},  /* encodeType
                                               G711U,G711A,CN,G729,G726_32,ILBC_20,
                                               ILBC_30,DTMFR */
    {  0,  1, 13, 18,  2, 97, 97, 101},  /* decodeType */
    0,                          /* extension */
    0,                          /* dtmfRelay */
    0,                          /* silenceComp */
    0,                          /* confMask */
    0,                          /* remoteIpAddr */
    0,                          /* remoteDataPort */
    0,                          /* remoteControlPort */
    0,                          /* localIpAddr */
    0,                          /* localDataPort */
    0                           /* localControlPort */
};

/*
 * ======== fill729() ========
 *
 * This function fills the specified array with a pattern. Currently this
 * pattern does not generate any recognizable audio.
 */
static vint fill729(
    void *data_ptr,
    vint  maxSz)
{
    vint    blockSize;
    uint32 *word_ptr;

    word_ptr = data_ptr;
    for (blockSize = 0; blockSize < maxSz; blockSize += 20) {
        *word_ptr++ = 0x00000000;
        *word_ptr++ = 0xffffffff;
        *word_ptr++ = 0x00000000;
        *word_ptr++ = 0xffffffff;
        *word_ptr++ = 0x00000000;
    }
    return (blockSize);
}

/*
 * ======== fill726_32K() ========
 *
 * This function fills the specified array with a pattern. Currently this
 * pattern does not generate any recognizable audio.
 */
static vint fill726_32K(
    void *data_ptr,
    vint  maxSz)
{
    vint    blockSize;
    uint32 *word_ptr;

    word_ptr = data_ptr;

    for (blockSize = 0; blockSize < maxSz; blockSize += 40) {
        *word_ptr++ = 0xfedeffde;
        *word_ptr++ = 0xef12fffe;
        *word_ptr++ = 0x2f1213ef;
        *word_ptr++ = 0x1f1123f1;
        *word_ptr++ = 0xf111fdd2;
        *word_ptr++ = 0xeffeffff;
        *word_ptr++ = 0xc1ff3ff1;
        *word_ptr++ = 0x21ee1ddf;
        *word_ptr++ = 0xef212e2f;
        *word_ptr++ = 0xefe1dfdd;
    }
    return (blockSize);
}
/*
 * ======== fillMu1K() ========
 *
 * This function fills the array specified by data_ptr with the G.711 Mu-law
 * pattern for a 1000 Hz tone. The array is filled in blocks of 8 characters. So
 * the array must be at least 8 bytes and should be a multiple of 8 bytes so
 * no discontinuities are generated.
 */
static vint fillMu1K(
    void *data_ptr,
    vint  maxSz)
{
    vint    blockSize;
    uint32 *word_ptr;

    word_ptr = data_ptr;

    for (blockSize = 0; blockSize < maxSz; blockSize += 8) {
        *word_ptr++ = 0xb9a5a5b9;
        *word_ptr++ = 0x39252539;
    }
    return (blockSize);
}
/*
 * ======== fillA500() ========
 *
 * This function fills the array specified by data_ptr with the G.711 A-law
 * pattern for a 500 Hz tone. The array is filled in blocks of 16 characters. So
 * the array must be at least 16 bytes and should be a multiple of 16 bytes so
 * no discontinuities are generated.
 */
static vint fillA500(
    void *data_ptr,
    vint  maxSz)
{
    vint    blockSize;
    uint32 *word_ptr;

    word_ptr = data_ptr;

    for (blockSize = 0; blockSize < maxSz; blockSize += 16) {
        *word_ptr++ = 0x00363c3e;
        *word_ptr++ = 0x3c360055;
        *word_ptr++ = 0x80b6bcbe;
        *word_ptr++ = 0xbcb68055;
    }
    return (blockSize);
}
#endif
/*
 * ======== UT_flowPlayLocal() ========
 *
 * This test plays flows without any open streams. Single and conferenced flows
 * are checked as are each coder type.
 */

UT_Return UT_flowPlayLocal(
        vint         infc)
{
#ifdef VTSP_ENABLE_PLAY
    vint            iloop;
    vint            flag;
    vint            streamId1;
    vint            streamId2;
    uvint           keyMask;
    uvint           key1;
    uvint           key2;
    vint            flowId1;
    vint            flowId2;
    vint            blockSize;
    vint            maxSz;
    vint            retVal;
    uint32          control;
    uint32          timeout;
    uint8           data_ptr[PAYLOAD_SZ];
    VTSP_QueryData *vtspQueryData_ptr;
    char            buf[12];

    UT_run = 1;
    OSAL_logMsg("%s:%d Entering Flow Test 1.\n", __FILE__, __LINE__);

    vtspQueryData_ptr = VTSP_query();
    maxSz = vtspQueryData_ptr->flow.payloadMaxSz;
    OSAL_logMsg("%s:%d Max payload size %d key Mask %x.\n", 
            __FILE__, __LINE__, maxSz, vtspQueryData_ptr->flow.keyMask);

    keyMask = vtspQueryData_ptr->flow.keyMask;
    /*
     * Setting control will allow DTMF digits to terminate flows.
     */
    control = 0;
    /*
     * The stream ID variables allow the flows to be associated with different
     * streams.
     */
    streamId1 = 0;
    streamId2 = 1;
    /*
     * The key variables specifies a key for each open flow. key1 is associated
     * with the flow associated with streamId1. key2 is associated with
     * streamId2.
     */
    key1 = 0;
    key2 = 0;
    /*
     * The flag variable is used to toggle between closing flows or aborting
     * flows.
     */
    flag = 0;
    /*
     * Timeout is used for non-CN VTSP_flowPlay calls. It is set to a value that
     * allows a full payload of G.711 to be played. 
     */
    timeout = 10 * (maxSz/VTSP_BLOCK_G711_10MS_SZ + 1);

    OSAL_logMsg("\n\n%s:%d Ring infc %d.\n", __FILE__, __LINE__, infc);

    UT_EXPECT5(VTSP_OK, VTSP_ring, 
            infc, 
            1, /* ring template */ 
            3, /* # rings */
            14000, /* ms timeout */
            &UT_cidObj);
    OSAL_logMsg("%s:%d Pausing while ringing.\n", __FILE__, __LINE__);
    OSAL_taskDelay(8000);

    OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
    UT_EXPECT1(VTSP_OK, VTSP_ringStop, infc);

    OSAL_logMsg("\n\n%s:%d Flow Test with Inactive Streams\n"
                "This is a local interface test\n"
                "\nOptions: \n"
                " c for continuous test\n"
                " s for single shot\n"
                "   DEFAULT - single shot\n"
                "(enter option now)\n",
                __FILE__, __LINE__, 0, 0);
    D2_getLine(buf, 2);

    while (UT_run) { 
        OSAL_taskDelay(2000);

        OSAL_logMsg("\n%s:%d Single Flow - on Stream 0..1KHz MuLaw\n",
                __FILE__, __LINE__);
        OSAL_logMsg("%s:%d 1KHz - CN - 1KHz\n", __FILE__, __LINE__);
        if ((flowId1 = VTSP_flowOpen(infc, streamId1, VTSP_FLOW_DIR_LOCAL_PLAY,
                key1, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %x..\n", __FILE__, 
                    __LINE__, flowId1);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 40; iloop++) {
            if (iloop == 10) {
                data_ptr[0] = 30;
                if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId1,
                        VTSP_CODER_CN, 1, data_ptr, 200, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
            else {
                if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                    if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                            VTSP_CODER_G711U, blockSize, data_ptr, control,
                            VTSP_TIMEOUT_FOREVER))) {
                        continue;
                    } else {
                        OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                        break;
                    }
                }
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId1);
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId1, timeout);
        }
        key1 = (key1 + 1) & keyMask;

        OSAL_taskDelay(5000);

        OSAL_logMsg("\n%s:%d Single Flow - on Stream 1.. 500 Hz Alaw\n",
                __FILE__, __LINE__);
        OSAL_logMsg("%s:%d 500 Hz - CN - 500Hz\n", __FILE__, __LINE__);
        if ((flowId2 = VTSP_flowOpen(infc, streamId2, VTSP_FLOW_DIR_LOCAL_PLAY,
                key2, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId2);
            goto flowCleanup;
        }

        for (iloop = 0; iloop < 40; iloop++) {
            if (iloop == 10) {
                data_ptr[0] = 20;
                if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId2,
                        VTSP_CODER_CN, 1, data_ptr, 200, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
            else {
                if ((blockSize = fillA500(data_ptr, PAYLOAD_SZ)) > 0) {
                    if (VTSP_OK == (retVal = VTSP_flowPlay(flowId2,
                            VTSP_CODER_G711A, blockSize, data_ptr, control,
                            VTSP_TIMEOUT_FOREVER))) {
                        continue;
                    } else {
                        OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                                __FILE__, __LINE__, retVal);
                        break;
                    }
                }
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Aborting Flows. %x \n", 
                    __FILE__, __LINE__, flowId2);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId2, timeout);
        }
        else {
            OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                    __FILE__, __LINE__, flowId2);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId2);
        }
        key2 = (key2 + 1) & keyMask;
        OSAL_taskDelay(5000);

        OSAL_logMsg("\n%s:%d Dual Flows\n", __FILE__, __LINE__);
        OSAL_logMsg("%s:%d 1KHz+CN the CN+500 Hz\n", __FILE__, __LINE__);
        if ((flowId1 = VTSP_flowOpen(infc, streamId1, VTSP_FLOW_DIR_LOCAL_PLAY,
                key1, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId1);
            goto flowCleanup;
        }
        if ((flowId2 = VTSP_flowOpen(infc, streamId2, VTSP_FLOW_DIR_LOCAL_PLAY,
                key2, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId2);
            goto flowCleanup;
        }
        /*
         * Generate CN on flowId 2
         */
        data_ptr[0] = 20;
        if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId2, VTSP_CODER_CN,
                1, data_ptr, 250, control, VTSP_TIMEOUT_FOREVER))) {
        } else {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
        }

        for (iloop = 0; iloop < 50; iloop++) {
            if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                        VTSP_CODER_G711U, blockSize, data_ptr, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        data_ptr[0] = 30;
        if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId1, VTSP_CODER_CN,
                1, data_ptr, 250, control, VTSP_TIMEOUT_FOREVER))) {
        } else {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                __FILE__, __LINE__, retVal);
        }
        for (iloop = 0; iloop < 50; iloop++) {
            if ((blockSize = fillA500(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId2,
                        VTSP_CODER_G711A, blockSize, data_ptr, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flows. %x %x \n", 
                __FILE__, __LINE__, flowId1, flowId2);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId1);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId2, timeout);
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x %x\n", 
                __FILE__, __LINE__, flowId1, flowId2);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId1, timeout);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId2);
        }
        OSAL_taskDelay(5000);
        key1 = (key1 + 1) & keyMask;
        key2 = (key2 + 1) & keyMask;
        /*
         * Now test all coders by switching dynamically
         */

        OSAL_logMsg("\n%s:%d Single Flow Mu-A-729-726..\n", __FILE__, 
                __LINE__, infc, streamId1);
        OSAL_logMsg("%s: infc %d streamId %d, key %d \n", __FILE__, 
                infc, streamId1, key1);
        if ((flowId1 = VTSP_flowOpen(infc, streamId1, VTSP_FLOW_DIR_LOCAL_PLAY,
                key1, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId1);
            goto flowCleanup;
        }
        OSAL_taskDelay(2000);

        OSAL_logMsg("%s:%d Play G.711 MULAW\n", __FILE__, __LINE__);
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                        VTSP_CODER_G711U, blockSize, data_ptr, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        OSAL_logMsg("%s:%d Play G.711 ALAW..\n", __FILE__, __LINE__);
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = fillA500(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                        VTSP_CODER_G711U, blockSize, data_ptr, control,
                        VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        OSAL_logMsg("%s:%d Play G.729..\n", __FILE__, __LINE__);
        for (iloop = 0; iloop < 25; iloop++) {
            if ((blockSize = fill729(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1, VTSP_CODER_G729,
                        blockSize, data_ptr, control, VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        OSAL_logMsg("%s:%d Play G.726_32K..\n", __FILE__, __LINE__);
        for (iloop = 0; iloop < 50; iloop++) {
            if ((blockSize = fill726_32K(data_ptr, PAYLOAD_SZ)) > 0) {
                if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                                VTSP_CODER_G726_32K,
                        blockSize, data_ptr, control, VTSP_TIMEOUT_FOREVER))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId1);
            flag = 1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId1, VTSP_TIMEOUT_FOREVER);
            flag = 0;
        }
        OSAL_taskDelay(5000);
        key1 = (key1 + 1) & keyMask;
        if (buf[0] != 'c') {
            OSAL_logMsg("%s:%d Exiting\n", __FILE__, __LINE__);
            goto flowCleanup;
        }
    }

flowCleanup:
    /* OSAL_memFree(data_ptr, 0); */
#endif
    return (UT_PASS);
}

/*
 * ======== UT_flowPlayStreamLocal() ========
 *
 * This test check that a local flow can override a stream.
 */

UT_Return UT_flowPlayStreamLocal(
        vint         infc)
{
#ifdef VTSP_ENABLE_PLAY
    vint            iloop;
    vint            flag;
    vint            streamId1;
    vint            streamId2;
    vint            flowId1;
    uvint           keyMask;
    uvint           key1;
    vint            blockSize;
    vint            maxSz;
    vint            retVal;
    uint32          control;
    uint32          timeout;
    VTSP_Stream    *stream_ptr;
    uint8           data_ptr[PAYLOAD_SZ];
    VTSP_QueryData *vtspQueryData_ptr;
    char            buf[12];

    UT_run = 1;
    OSAL_logMsg("%s:%d Entering Flow Play Test 2.\n", __FILE__, __LINE__);

    vtspQueryData_ptr = VTSP_query();
    maxSz = vtspQueryData_ptr->flow.payloadMaxSz;
    keyMask = vtspQueryData_ptr->flow.keyMask;
    OSAL_logMsg("%s:%d Max payload size %d mask %x.\n", 
            __FILE__, __LINE__, maxSz, keyMask);

    key1 = 0;
    stream_ptr = &stream0;
    control = VTSP_FLOW_STOP_DTMF_2;
    streamId1 = stream_ptr->streamId;
    streamId2 = 1;
    flag = 0;
    /*
     * Timeout allows silence packet to play.
     */
    timeout = 2050;

    OSAL_logMsg("\n\n%s:%d Ring infc %d.\n", __FILE__, __LINE__, infc);

    UT_EXPECT5(VTSP_OK, VTSP_ring, 
            infc, 
            1, /* ring template */ 
            3, /* # rings */
            14000, /* ms timeout */
            &UT_cidObj);
    OSAL_logMsg("%s:%d Pausing while ringing.\n", __FILE__, __LINE__);
    OSAL_taskDelay(8000);

    OSAL_logMsg("%s: Stopping ring..\n", __FILE__, 0, 0, 0);
    UT_EXPECT1(VTSP_OK, VTSP_ringStop, infc);
    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, VTSP_DETECT_DTMF);

    OSAL_logMsg("\n\n%s:%d Flow Test with Active Streams\n"
                "Entering a DTMF 2 will terminate the flow.\n"
                "This is a local interface test\n"
                "\nOptions: \n"
                " c for continuous test\n"
                " s for single shot\n"
                "   DEFAULT - single shot\n"
                "(enter option now)\n", __FILE__, __LINE__);
    D2_getLine(buf, 2);

    while (UT_run) { 
        OSAL_taskDelay(2000);
        OSAL_logMsg("\n%s:%d Stream and Flow on different Streams..\n",
                __FILE__, __LINE__);
        OSAL_logMsg("%s:%dTalking on phone will be mixed with flow\n",
                __FILE__, __LINE__);

        OSAL_logMsg("%s:%d Starting stream on %d %d..\n", __FILE__, 
                __LINE__, infc, streamId1);

        stream_ptr->remoteAddr.ipv4  = htonl(0x7f000001);
        stream_ptr->remoteAddr.port  = htons(5001);
        stream_ptr->localAddr.ipv4   = htonl(0x7f000001);
        stream_ptr->localAddr.port   = htons(5001);    
#if 0
        stream_ptr->remoteIpAddr  = htonl(0x7f000001);
        stream_ptr->localIpAddr  = htonl(0x7f000001);
        stream_ptr->remoteDataPort  = htons(5001);
        stream_ptr->localDataPort  = htons(5001);
#endif
        stream_ptr->encoder     = VTSP_CODER_G729;
        stream_ptr->silenceComp = 0;
        stream_ptr->dtmfRelay   = 0;
        stream_ptr->extension   = 0;

        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

        OSAL_taskDelay(2000);

        OSAL_logMsg("\n%s:%d Starting flow on %d %d..\n", __FILE__, 
                __LINE__, infc, streamId2);
        if ((flowId1 = VTSP_flowOpen(infc, streamId2, VTSP_FLOW_DIR_LOCAL_PLAY,
                key1, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId1);
            goto flowCleanup;
        }

        for (iloop = 0; iloop < 80; iloop++) {
            if (iloop == 20) {
                data_ptr[0] = 30;
                if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId1,
                        VTSP_CODER_CN, 1, data_ptr, 200, control,
                        timeout))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
            else {
                if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                    if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                            VTSP_CODER_G711U, blockSize, data_ptr, control,
                            timeout))) {
                            continue;
                    } else {
                        OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                        break;
                    }
                }
            }
        }
        OSAL_logMsg("%s:%d Closing stream. %d \n", 
                __FILE__, __LINE__, streamId1);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId1);
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x \n", 
                __FILE__, __LINE__, flowId1, 0);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId1, VTSP_TIMEOUT_NO_WAIT);
        }
        OSAL_taskDelay(2000);
        key1 = (key1 + 1) & keyMask;

        OSAL_logMsg("\n%s:%d Stream on Flow on same Stream..\n",
                __FILE__, __LINE__);
        OSAL_logMsg("%s:%dTalking on phone will be stopped by flow\n",
                __FILE__, __LINE__);
        OSAL_logMsg("\n%s:%d Starting stream on %d %d..\n",
                __FILE__, __LINE__, infc, streamId1);

        stream_ptr->remoteAddr.ipv4  = htonl(0x7f000001);
        stream_ptr->localAddr.ipv4   = htonl(0x7f000001);
        stream_ptr->remoteAddr.port  = htons(5001);
        stream_ptr->localAddr.port    = htons(5001);

        stream_ptr->encoder     = VTSP_CODER_G729;
        stream_ptr->silenceComp = 0;
        stream_ptr->dtmfRelay   = 0;
        stream_ptr->extension   = 0;

        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

        OSAL_logMsg("\n%s:%d Starting flow on %d %d..\n",
                __FILE__, __LINE__, infc, streamId1);
        if ((flowId1 = VTSP_flowOpen(infc, streamId1, VTSP_FLOW_DIR_LOCAL_PLAY,
                key1, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n",
                    __FILE__, __LINE__, flowId1);
            goto flowCleanup;
        }
        OSAL_taskDelay(2000);

        for (iloop = 0; iloop < 80; iloop++) {
            if (iloop == 20) {
                data_ptr[0] = 30;
                if (VTSP_OK == (retVal = VTSP_flowPlaySil(flowId1,
                        VTSP_CODER_CN, 1, data_ptr, 200, control,
                        timeout))) {
                    continue;
                } else {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    break;
                }
            }
            else {
                if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                    if (VTSP_OK == (retVal = VTSP_flowPlay(flowId1,
                            VTSP_CODER_G711U, blockSize, data_ptr, control,
                            timeout))) {
                        continue;
                    } else {
                        OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                        break;
                    }
                }
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId1);
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x \n", 
                __FILE__, __LINE__, flowId1);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId1, VTSP_TIMEOUT_NO_WAIT);
        }
        OSAL_logMsg("%s:%d Talking on stream is now heard after flow closes. %d \n", 
                __FILE__, __LINE__, streamId1);
        OSAL_taskDelay(2000);
        key1 = (key1 + 1) & keyMask;

        OSAL_logMsg("%s:%d Closing stream. %d \n", 
                __FILE__, __LINE__, streamId1);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);

        if (buf[0] != 'c') {
            OSAL_logMsg("%s:%d Exiting\n", __FILE__, __LINE__);
            goto flowCleanup;
        }
    }

flowCleanup:

#endif
    return (UT_PASS);
}
/*
 * ======== UT_flowPlayRecord() ========
 *
 * This routine is used to test the play and record to a local interface.
 */
UT_Return UT_flowPlayRecord(
        vint         infc)
{
#ifdef VTSP_ENABLE_PLAY
#ifdef VTSP_ENABLE_RECORD
    vint            iloop;
    vint            flag;
    vint            streamId;
    uvint           keyMask;
    uvint           key;
    uvint           coder;
    uvint           lastCoder;
    vint            flowId;
    vint            blockSize;
    vint            maxSz;
    vint            retVal;
    uint32          control;
    uint32          timeout;
    uint32          duration;
    uint8           data_ptr[PAYLOAD_SZ];
    VTSP_QueryData *vtspQueryData_ptr;
    char            buf[12];

    UT_run = 1;
    OSAL_logMsg("%s:%d Entering Play/Record Test 1.\n", __FILE__, __LINE__);

    vtspQueryData_ptr = VTSP_query();
    maxSz = vtspQueryData_ptr->flow.payloadMaxSz;
    keyMask = vtspQueryData_ptr->flow.keyMask;
    OSAL_logMsg("%s:%d Max payload size %d key Mask %x.\n", 
            __FILE__, __LINE__, maxSz, keyMask);

    lastCoder = VTSP_CODER_UNAVAIL;
    control = 0;
    streamId = 0;
    key = 0;
    flag = 0;
    flowId = -1;
    timeout = VTSP_TIMEOUT_FOREVER;

    OSAL_logMsg("\n\n%s:%d Ring infc %d.\n", __FILE__, __LINE__, infc);

    OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
    UT_EXPECT1(VTSP_OK, VTSP_ringStop, infc);

    OSAL_logMsg("\n\n%s:%d Flow Record/Play Test \n"
                "This is a local interface test\n"
                "\nOptions: \n"
                " c for continuous test\n"
                " s for single shot\n"
                "   DEFAULT - single shot\n"
                "(enter option now)\n", __FILE__, __LINE__);
    D2_getLine(buf, 2);

    while (UT_run) { 
        OSAL_taskDelay(2000);

        /*
         * Play/Record on stream 0 using G.711U.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using G711U on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_G711U)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 15, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
        /*
         * Play/Record on stream 0 using G.711A.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using G711A on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_G711A)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId, 0);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 15, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                    __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
#ifdef VTSP_ENABLE_G726
        /*
         * Play/Record on stream 0 using G.726_32K.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using G726_32K on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_G726_32K)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n", __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 15, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 50; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
#endif
#ifdef VTSP_ENABLE_G729
        /*
         * Play/Record on stream 0 using G.729.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using G729 on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_G729)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 15, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 50; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
#endif
#ifdef VTSP_ENABLE_ILBC
        /*
         * Play/Record on stream 0 using ILBC.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using ILBC 20ms on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_ILBC_20MS)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId, 0);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 12, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
            flag = 1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                    __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
            flag = 0;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
        /*
         * Play/Record on stream 0 using ILBC 30ms.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using ILBC 30ms on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_ILBC_30MS)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId, 0);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                VTSP_CODER_CN, 1, data_ptr, 12, control, timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
            flag = 1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
            flag = 0;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
#endif
#ifdef VTSP_ENABLE_G723
        /*
         * Play/Record on stream 0 using G723 30ms.
         */
        OSAL_logMsg("\n%s:%d Play/Record Using G723 30ms on stream %d\n",
                __FILE__, __LINE__, streamId);
        if ((flowId = VTSP_flowOpen(infc, streamId,
                (VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_LOCAL_RECORD),
                key, VTSP_CODER_G723_30MS)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d\n", __FILE__, 
                    __LINE__, flowId, 0);
            goto flowCleanup;
        }
        OSAL_logMsg("\n%s:%d Flow Open %x \n",
                __FILE__, __LINE__, flowId);
        data_ptr[0] = 80;
        if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                        VTSP_CODER_CN, 1, data_ptr, 12, control,
                        timeout))) {
            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                    __FILE__, __LINE__, retVal);
            goto flowCleanup;
        }
        for (iloop = 0; iloop < 100; iloop++) {
            if ((blockSize = VTSP_flowRecord(flowId, maxSz, &coder, data_ptr,
                            &duration, timeout)) > 0) {
                if (coder != lastCoder) {
                    OSAL_logMsg("%s:%d Coder Change new %d old %d\n", 
                            __FILE__, __LINE__, coder, lastCoder);
                    lastCoder = coder;
                }
                retVal = VTSP_flowPlay(flowId, coder, blockSize,
                        data_ptr, control, timeout);
                if (retVal != VTSP_OK) {
                    OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                            __FILE__, __LINE__, retVal);
                    goto flowCleanup;
                }
            }
            else {
                OSAL_logMsg("%s:%d Flow Record Failed. %d\n", 
                        __FILE__, __LINE__, blockSize);
                goto flowCleanup;
            }
        }
        if (0 == flag) {
            OSAL_logMsg("%s:%d Closing Flow. %x\n", __FILE__, __LINE__, flowId);
            UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
            flowId = -1;
            flag = 1;
        }
        else {
            OSAL_logMsg("%s:%d Aborting Flow. %x\n", 
                __FILE__, __LINE__, flowId);
            UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
            flowId = -1;
            flag = 0;
        }
        key = (key + 1) & keyMask;
        OSAL_taskDelay(2000);
#endif
        if (buf[0] != 'c') {
            OSAL_logMsg("%s:%d Exiting\n", __FILE__, __LINE__);
            goto flowCleanup;
        }
    }

flowCleanup:
    if (flowId >= 0) {
        UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
    }
    /* OSAL_memFree(data_ptr, 0); */
#endif
#endif
    return (UT_PASS);
}

/*
 * ======== UT_flowPlayStreamPeer() ========
 *
 * This routine is used to test play to peer. Two streams are opened. The first
 * stream is SENDRECV and the second is RECVONLY. A flow is attached to the
 * first stream. The stream should send the flow and not the data from the local
 * interface. When the second stream is closed, the audio should stop.
 */
UT_Return UT_flowPlayStreamPeer(
        vint         infc)
{
#ifdef VTSP_ENABLE_PLAY
    vint            iloop;
    vint            flag;
    vint            streamId0;
    vint            streamId1;
    vint            flowId;
    uvint           keyMask;
    uvint           key;
    vint            blockSize;
    vint            maxSz;
    vint            retVal;
    vint            streamCoder;
    vint            streamSid;
    vint            appCoder;
    uint32          control;
    uint32          timeout;
    VTSP_Stream    *stream0_ptr;
    VTSP_Stream    *stream1_ptr;
    uint8           data_ptr[PAYLOAD_SZ];
    VTSP_QueryData *vtspQueryData_ptr;
    char            buf[12];

    UT_run = 1;
    OSAL_logMsg("%s:%d Entering Play Test 3.\n", __FILE__, __LINE__);

    /*
     * Get the configured sizes for maximum payload size and the key mask.
     */
    vtspQueryData_ptr = VTSP_query();
    maxSz = vtspQueryData_ptr->flow.payloadMaxSz;
    keyMask = vtspQueryData_ptr->flow.keyMask;

    /*
     * Set some local variables.
     */
    key = 0;
    stream0_ptr = &stream0;
    stream1_ptr = &stream1;
    control = VTSP_FLOW_STOP_DTMF_2;
    streamId0 = stream0_ptr->streamId;
    streamId1 = stream1_ptr->streamId;
    flag = 0;
    streamCoder = 0;
    streamSid = 0;
    appCoder = 0;

    /*
     * Set flowId to an illegal value so that cleanup will know whether to close
     * the flow on exit.
     */
    flowId = -1;

    /*
     * Timeout is used for non-CN VTSP_flowPlay calls. It is set to a value that
     * allows a full payload of G.711 to be played. 
     */
    timeout = 10 * (maxSz/VTSP_BLOCK_G711_10MS_SZ + 2);

    OSAL_logMsg("\n\n%s:%d Ring infc %d.\n", __FILE__, __LINE__, infc);

    UT_EXPECT5(VTSP_OK, VTSP_ring, 
            infc, 
            1, /* ring template */ 
            3, /* # rings */
            14000, /* ms timeout */
            &UT_cidObj);
    OSAL_logMsg("%s:%d Pausing while ringing.\n", 
            __FILE__, __LINE__, 0, 0);
    OSAL_taskDelay(8000);
    OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
    UT_EXPECT1(VTSP_OK, VTSP_ringStop, infc);
    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, VTSP_DETECT_DTMF);

    OSAL_logMsg("\n\n%s:%d Flow Test with Active Streams\n"
                "This is a Peer interface test\n"
                "\nOptions: \n"
                " c for continuous test\n"
                " s for single shot\n"
                "   DEFAULT - single shot\n"
                "(enter option now)\n", __FILE__, __LINE__);
    D2_getLine(buf, 2);

    while (UT_run) { 
        stream0_ptr->remoteAddr.ipv4  = htonl(0x7f000001);
        stream0_ptr->localAddr.ipv4   = htonl(0x7f000001);
        stream0_ptr->remoteAddr.port  = htons(5002);
        stream0_ptr->localAddr.port   = htons(5001);
        stream0_ptr->dir         = VTSP_STREAM_DIR_SENDRECV;

        stream0_ptr->silenceComp = 0;
        stream0_ptr->dtmfRelay   = 0;
        stream0_ptr->extension   = 0;

        stream1_ptr->remoteAddr.ipv4  = htonl(0x7f000001);
        stream1_ptr->localAddr.ipv4   = htonl(0x7f000001);
        stream1_ptr->remoteAddr.port  = htons(5001);
        stream1_ptr->localAddr.port   = htons(5002);
        stream1_ptr->dir         = VTSP_STREAM_DIR_RECVONLY;

        stream1_ptr->silenceComp = 0;
        stream1_ptr->dtmfRelay   = 0;
        stream1_ptr->extension   = 0;

        switch (streamCoder) {
            case 0:
                OSAL_logMsg("%s:%d Stream Coder Type G711U SID %d\n", __FILE__, 
                        __LINE__, streamSid);
                stream0_ptr->encoder     = VTSP_CODER_G711U;
                stream1_ptr->encoder     = VTSP_CODER_G711U;
                stream0_ptr->silenceComp = streamSid;
                streamCoder++;
                break;
            case 1:
                OSAL_logMsg("%s:%d Stream Coder Type G711A SID %d\n", __FILE__, 
                        __LINE__, streamSid);
                stream0_ptr->encoder     = VTSP_CODER_G711A;
                stream1_ptr->encoder     = VTSP_CODER_G711A;
                stream0_ptr->silenceComp = streamSid;
                streamCoder++;
                break;
            case 2:
                OSAL_logMsg("%s:%d Stream Coder Type G726_32K SID %d\n", __FILE__, 
                        __LINE__, streamSid);
                stream0_ptr->encoder     = VTSP_CODER_G726_32K;
                stream1_ptr->encoder     = VTSP_CODER_G726_32K;
                stream0_ptr->silenceComp = streamSid;
                streamCoder++;
                break;
            default:
                OSAL_logMsg("%s:%d Stream Coder Type G729 SID %d\n", __FILE__, 
                        __LINE__, streamSid);
                stream0_ptr->encoder     = VTSP_CODER_G729;
                stream1_ptr->encoder     = VTSP_CODER_G729;
                stream0_ptr->silenceComp = streamSid;
                streamCoder = 0;
                if (streamSid != 0) {
                    streamSid = 0;
                }
                else {
                    streamSid = 0x1f;
                }
                break;
        }
        switch (appCoder) {
            case 0:
                OSAL_logMsg("%s:%d Application Coder Type G711U\n", __FILE__, 
                        __LINE__);
                break;
            case 1:
                OSAL_logMsg("%s:%d Application Coder Type G711A\n", __FILE__, 
                        __LINE__);
                break;
            default:
                OSAL_logMsg("%s:%d Application Coder Type CN\n", __FILE__, 
                        __LINE__);
                break;
        }

        OSAL_taskDelay(2000);
        OSAL_logMsg("%s:%d Starting stream on %d %d..\n", __FILE__, 
                __LINE__, infc, streamId0);


        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream0_ptr);

        OSAL_logMsg("%s:%d Starting stream on %d %d..\n", __FILE__, 
                __LINE__, infc, streamId1);

        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream1_ptr);

        OSAL_taskDelay(2000);

        OSAL_logMsg("%s:%d Starting flow on %d %d..\n", __FILE__, 
                __LINE__, infc, streamId0);
        if ((flowId = VTSP_flowOpen(infc, streamId0, VTSP_FLOW_DIR_PEER_PLAY,
                key, 0)) < 0) {
            OSAL_logMsg("%s:%d  flow open failed %d..\n", __FILE__, 
                    __LINE__, flowId);
            goto flowCleanup;
        }

        for (iloop = 0; iloop < 360; iloop++) {
            switch (appCoder) {
                case 0:
                    if ((blockSize = fillMu1K(data_ptr, PAYLOAD_SZ)) > 0) {
                        if (VTSP_OK != (retVal = VTSP_flowPlay(flowId,
                                VTSP_CODER_G711U, blockSize, data_ptr, control,
                                timeout))) {
                            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                                    __FILE__, __LINE__, retVal);
                        }
                    }
                    break;
                case 1:
                    if ((blockSize = fillA500(data_ptr, PAYLOAD_SZ)) > 0) {
                        if (VTSP_OK != (retVal = VTSP_flowPlay(flowId,
                                VTSP_CODER_G711A, blockSize, data_ptr, control,
                                timeout))) {
                            OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                                    __FILE__, __LINE__, retVal);
                        }
                    }
                    break;
                default:
                    data_ptr[0] = 30;
                    if (VTSP_OK != (retVal = VTSP_flowPlaySil(flowId,
                            VTSP_CODER_CN, 1, data_ptr, 5, control,
                            timeout))) {
                        OSAL_logMsg("%s:%d Flow Play Failed. %d\n", 
                        __FILE__, __LINE__, retVal);
                    }
                    break;
            }
            if (40 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - INACTIVE SILENCE\n", 
                    __FILE__, __LINE__, infc, streamId1);
                VTSP_streamModifyDir(infc, streamId1, VTSP_STREAM_DIR_INACTIVE);
            }
            else if (80 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - SENDONLY SILENCE\n", 
                    __FILE__, __LINE__, infc, streamId1);
                VTSP_streamModifyDir(infc, streamId1, VTSP_STREAM_DIR_SENDONLY);
            }
            else if (120 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - SENDRECV TONE\n", 
                    __FILE__, __LINE__, infc, streamId1);
                VTSP_streamModifyDir(infc, streamId1, VTSP_STREAM_DIR_SENDRECV);
            }
            else if (160 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - RECVONLY TONE\n", 
                    __FILE__, __LINE__, infc, streamId1);
                VTSP_streamModifyDir(infc, streamId1, VTSP_STREAM_DIR_RECVONLY);
            }
            else if (200 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - INACTIVE SILENCE\n", 
                    __FILE__, __LINE__, infc, streamId0);
                VTSP_streamModifyDir(infc, streamId0, VTSP_STREAM_DIR_INACTIVE);
            }
            else if (240 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - SENDONLY TONE\n", 
                    __FILE__, __LINE__, infc, streamId0);
                VTSP_streamModifyDir(infc, streamId0, VTSP_STREAM_DIR_SENDONLY);
            }
            else if (280 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - SENDRECV TONE\n", 
                    __FILE__, __LINE__, infc, streamId0);
                VTSP_streamModifyDir(infc, streamId0, VTSP_STREAM_DIR_SENDRECV);
            }
            else if (320 == iloop) {
                OSAL_logMsg("%s:%d infc=%d stream=%d"
                        " Direction - RECVONLY SILENCE\n", 
                    __FILE__, __LINE__, infc, streamId0);
                VTSP_streamModifyDir(infc, streamId0, VTSP_STREAM_DIR_RECVONLY);
            }
        }
        switch (flag) {
            case 0:
                OSAL_logMsg("%s:%d Closing streams. %d  %d\n", 
                        __FILE__, __LINE__, streamId0, streamId1);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId0);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);
                OSAL_taskDelay(1000);
                OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                    __FILE__, __LINE__, flowId);
                UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
                flowId = -1;
                flag = 1;
                break;
            case 1:
                OSAL_logMsg("%s:%d Closing Flow. %x \n", 
                    __FILE__, __LINE__, flowId);
                UT_EXPECT1(VTSP_OK, VTSP_flowClose, flowId);
                OSAL_taskDelay(1000);
                OSAL_logMsg("%s:%d Closing streams. %d  %d\n", 
                        __FILE__, __LINE__, streamId0, streamId1);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId0);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);
                flowId = -1;
                flag = 2;
                break;
            case 2:
                OSAL_logMsg("%s:%d Closing streams. %d  %d\n", 
                        __FILE__, __LINE__, streamId0, streamId1);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId0);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);
                OSAL_taskDelay(1000);
                OSAL_logMsg("%s:%d Aborting Flow. %x \n", 
                        __FILE__, __LINE__, flowId);
                UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
                flowId = -1;
                flag = 3;
                break;
            default:
                OSAL_logMsg("%s:%d Aborting Flow. %x \n", 
                        __FILE__, __LINE__, flowId);
                UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
                OSAL_taskDelay(1000);
                OSAL_logMsg("%s:%d Closing streams. %d  %d\n", 
                        __FILE__, __LINE__, streamId0, streamId1);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId0);
                UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId1);
                flowId = -1;
                flag = 0;
                break;
        }
        OSAL_taskDelay(1000);
        key = (key + 1) & keyMask;
        if (streamCoder == 0) {
            appCoder = (appCoder >= 2) ? (0) : (appCoder + 1);
        }
        if ((streamCoder == 0) && (appCoder == 0) && (buf[0] != 'c')) {
            OSAL_logMsg("%s:%d Exiting\n", __FILE__, __LINE__, 0, 0);
            goto flowCleanup;
        }
    }

flowCleanup:
    if (flowId >= 0) {
        UT_EXPECT2(VTSP_OK, VTSP_flowAbort, flowId, timeout);
    }
    /* OSAL_memFree(data_ptr, 0); */
#endif
    return (UT_PASS);
}
