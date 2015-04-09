/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7809 $ $Date: 2008-10-13 16:50:06 -0400 (Mon, 13 Oct 2008) $
 *
 */

/*
 * Test stream functions
 * VTSP must be initialized and started prior to running these functions.
 */
#include "osal.h"
#include "vtsp.h"
#include "vtsp_ut.h"
#define UT_TIMER_SECS 190  /* was 530 for 2.4 Kbs, 190 for 14.4 or 9.6 Kbs*/
#define UT_TIMER_STEP 5

/* 
 * Define some structures for testing
 */
VTSP_ToneTemplate UT_myToneTemplateCallWait = {
    400,                            /* freq1 */
    0,                              /* freq2 */
    -20,                            /* power1 */
    0,                              /* power2 */
    1,                              /* cadences */
    200,                            /* make1 */
    0,                              /* break1 */
    1,                              /* repeat1 */
    0,                              /* make2 */
    0,                              /* break2 */
    0,                              /* repeat2 */
    0,                              /* make3 */
    0,                              /* break3 */
    0                               /* repeat3 */
};

VTSP_ToneTemplate UT_myToneTemplateRing = {
    1000,                           /* freq1 */
    2500,                           /* freq2 */
    -6,                             /* power1 */
    -6,                             /* power2 */
    3,                              /* cadences */
    0,                              /* make1 */
    1000,                           /* break1 */
    1,                              /* repeat1 */
    50,                             /* make2 */
    50,                             /* break2 */
    20,                             /* repeat2 */
    0,                              /* make3 */
    3000,                           /* break3 */
    1                               /* repeat3 */
};

/*
 * ======== UT_updateCoder() =======
 *
 * Return coder++ (next available coder).  Skip unsupported coders.
 *
 */
uvint UT_updateCoder(
    uvint        coder,
    VTSP_Stream *stream_ptr)
{
    while (1) { 
        /* Check the new coder to ensure it is supported */ 
        switch (coder) { 
            case VTSP_CODER_G711U:
                stream_ptr->encoder = VTSP_CODER_G711U;
                return (VTSP_CODER_G711U);
            case VTSP_CODER_G711A:
                stream_ptr->encoder = VTSP_CODER_G711A;
                return (VTSP_CODER_G711A);
            case VTSP_CODER_CN:
            case VTSP_CODER_DTMF:
            case VTSP_CODER_T38:
                /* skip CN, DTMF, and T38 coder by break back to loop */
                break; 
            case VTSP_CODER_16K_MU:
#ifdef VTSP_ENABLE_16K_MU
                stream_ptr->encoder = VTSP_CODER_16K_MU;
                return (VTSP_CODER_16K_MU);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_G729:
#ifdef VTSP_ENABLE_G729
                stream_ptr->encoder = VTSP_CODER_G729;
                return (VTSP_CODER_G729);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_G722:
#ifdef VTSP_ENABLE_G722
                stream_ptr->encoder = VTSP_CODER_G722;
                return (VTSP_CODER_G722);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_G722P1_20MS:
#ifdef VTSP_ENABLE_G722P1
                if (0 == (stream_ptr->extension & VTSP_MASK_EXT_G722P1_32)) {
                    stream_ptr->extension |= VTSP_MASK_EXT_G722P1_32;
                }
                else {
                    stream_ptr->extension &= ~VTSP_MASK_EXT_G722P1_32;
                }
                stream_ptr->encoder = VTSP_CODER_G722P1_20MS;
                return (VTSP_CODER_G722P1_20MS);
#else
                break; /* skip coder by break back to loop */
#endif                
            case VTSP_CODER_G726_32K:
#ifdef VTSP_ENABLE_G726
                stream_ptr->encoder = VTSP_CODER_G726_32K;
                return (VTSP_CODER_G726_32K);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_ILBC_20MS:
#ifdef VTSP_ENABLE_ILBC
                stream_ptr->encoder = VTSP_CODER_ILBC_20MS;
                return (VTSP_CODER_ILBC_20MS);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_ILBC_30MS:
#ifdef VTSP_ENABLE_ILBC
                stream_ptr->encoder = VTSP_CODER_ILBC_30MS;
                return (VTSP_CODER_ILBC_30MS);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_G723_30MS:
#ifdef VTSP_ENABLE_G723
                if (0 == (stream_ptr->extension & VTSP_MASK_EXT_G723_53)) {
                    stream_ptr->extension |= VTSP_MASK_EXT_G723_53;
                }
                else {
                    stream_ptr->extension &= ~VTSP_MASK_EXT_G723_53;
                }
                stream_ptr->encoder = VTSP_CODER_G723_30MS;
                return (VTSP_CODER_G723_30MS);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_SILK_20MS_8K:
#ifdef VTSP_ENABLE_SILK
                stream_ptr->encoder = VTSP_CODER_SILK_20MS_8K;
                return (VTSP_CODER_SILK_20MS_8K);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_SILK_20MS_16K:
#ifdef VTSP_ENABLE_SILK
                stream_ptr->encoder = VTSP_CODER_SILK_20MS_16K;
                return (VTSP_CODER_SILK_20MS_16K);
#else
                break; /* skip coder by break back to loop */
#endif
            case VTSP_CODER_SILK_20MS_24K:
#ifdef VTSP_ENABLE_SILK
                stream_ptr->encoder = VTSP_CODER_SILK_20MS_24K;
                return (VTSP_CODER_SILK_20MS_24K);
#else
                break; /* skip coder by break back to loop */
#endif
            default:
        /* Invalid coder number, start over*/
                stream_ptr->encoder = VTSP_CODER_G711U;
                return (VTSP_CODER_G711U);
        }
    coder++; /* coder not supported, try next one */
    }
}

/*
 * ======== _UT_updateCname() =======
 *
 * Return rtcp cname++ (next cname).
 */
uvint _UT_updateCname(
    uvint infc,
    uvint cId)
{
    switch (cId) {
        case 0:
            VTSP_rtcpCname(infc, "1");
            cId++;
            break;
        case 1:
            VTSP_rtcpCname(infc, "12");
            cId++;
            break;
        case 2:
            VTSP_rtcpCname(infc, "123");
            cId++;
            break;
        case 3:
            VTSP_rtcpCname(infc, "1234");
            cId++;
            break;
        case 4:
            VTSP_rtcpCname(infc, "");
            cId++;
            break;
        default:
            VTSP_rtcpCname(infc,
                    "1234567890123456789012345678901234567890"
                    "1234567890123456789012345678901234567890");
            cId = 0;
            break;
    }
    return (cId);
}
/*
 * ======== _UT_printTime() ========
 * 
 * Get the current time, and print the result to the console.
 */
void _UT_printTime(void)
{
    /*
     * ZK:
     * use user space time, cannot use OSAL_archCountGet() becasue that a
     * kernel space function.
     */
    OSAL_logMsg("%s:%d", __FILE__, __LINE__);
}

UT_Return UT_testStreamHold(
        vint         infc,
        VTSP_Stream *stream0_ptr,
        VTSP_Stream *stream1_ptr)
{
    uvint        coderloop;
    uvint        coder;
    uvint        streamId;
    vint         cnameLoop;         
    VTSP_Stream *stream_ptr;

    UT_run = 1;
    cnameLoop = 0;
    streamId = 0;
    
    /* Configure tone Ids 5 and 6 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_myToneTemplateRing);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 6, &UT_myToneTemplateCallWait);

    while (UT_run) { 
        stream_ptr = stream0_ptr;
        if (streamId == 1) {
            stream_ptr = stream1_ptr;
        }

        OSAL_logMsg("\n\n%s:%d Ring infc %d will use streamId %d.\n", 
                __FILE__, __LINE__, infc, streamId);

        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 5, VTSP_TONE_NMAX,
                VTSP_TONE_TMAX);
        
        OSAL_logMsg("%s:%d Pausing while ringing.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(8000);

        OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);


        /* Software Gain */
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, 0, 0, 0);
        
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, 60, 8000);
        OSAL_logMsg("%s:%d Pausing during tone.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(9000);

        OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__, 
                streamId);


        switch (cnameLoop) {
            case 0:
                VTSP_rtcpCname(infc, "1");
                cnameLoop++;
                break;
            case 1:
                VTSP_rtcpCname(infc, "12");
                cnameLoop++;
                break;
            case 2:
                VTSP_rtcpCname(infc, "123");
                cnameLoop++;
                break;
            case 3:
                VTSP_rtcpCname(infc, "1234");
                cnameLoop++;
                break;
            case 4:
                VTSP_rtcpCname(infc, "");
                cnameLoop++;
                break;
            default:
                VTSP_rtcpCname(infc,
                        "12345678901234567890123456789012345678901234567890123456789012345678901234567890");
                cnameLoop = 0;
                break;
        }
        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        for (coderloop = 0, 
                coder=VTSP_CODER_G711U; coderloop < (VTSP_ENCODER_NUM - 1); coderloop++) { 
            coder = UT_updateCoder(++coder, stream_ptr);
            OSAL_logMsg("%s:%d new encoder: %d\n", __FILE__, __LINE__, coder);
            UT_EXPECT3(VTSP_OK, VTSP_streamModifyEncoder, 
                    infc, streamId, coder);

            OSAL_taskDelay(8000);
            OSAL_logMsg("%s:%d Modify Dir to inActive\n",
                    __FILE__, __LINE__);

            stream_ptr->dir = VTSP_STREAM_DIR_INACTIVE;

            UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);

            OSAL_taskDelay(8000);

            OSAL_logMsg("%s:%d Modify Dir to SENDRECV\n",
                    __FILE__, __LINE__);

            stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
            UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);

            OSAL_taskDelay(8000);

            /*
             * Now test to see that the ssrc does not change when a stream is
             * inactive and the local ports are set to 0.
             */
            OSAL_logMsg("%s:%d Modify local ports to 0\n", __FILE__, __LINE__);
            stream_ptr->localAddr.port = 0;
            stream_ptr->localControlPort = 0;
            UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);
            OSAL_taskDelay(8000);

            OSAL_logMsg("%s:%d Restore local ports\n", __FILE__, __LINE__);
             if (streamId == 0) {
                stream_ptr->localAddr.port   = OSAL_netHtons(UT_recvPort);
                stream_ptr->localControlPort = OSAL_netHtons(UT_recvPort - 1);
            }
            else {
                stream_ptr->localAddr.port   = OSAL_netHtons(UT_recvPort + 2);
                stream_ptr->localControlPort = OSAL_netHtons(UT_recvPort + 1);
            }
            UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);
            OSAL_taskDelay(8000);
            
            coder++;
        }

        OSAL_logMsg("%s: Ending streamId %d..\n", __FILE__, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);

        if (streamId == 0) {
            streamId = 1;
        }
        else {
            streamId = 0;
        }
    }
    return (UT_PASS);
}
UT_Return UT_testStreamPeer(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint coderloop;
    uvint coder;
    uvint streamId;
    vint  cnameLoop;

    UT_run     = 1;
    cnameLoop  = 0;
    UT_encoder = VTSP_CODER_G711U;

    UT_silComp   = 0xffffffff;
    UT_dtmfRelay = 0xffffffff;
    OSAL_logMsg("\n%s:%d silComp = 0x%x, dtmfRelay = 0x%x\n", 
            __FILE__, __LINE__, UT_silComp, UT_dtmfRelay);

    /* Configure tone Ids 5 and 6 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_myToneTemplateRing);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 6, &UT_myToneTemplateCallWait);

    streamId   = stream_ptr->streamId;
    stream_ptr = UT_updateStreamData(infc, streamId);

    /* Disable SRTP */
    OSAL_logMsg("\n%s:%d XXX XXX Disable SRTP in s2 Test XXX XXX\n", 
            __FILE__, __LINE__);
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;

    UT_eventTaskDisplay = 1;
    OSAL_logMsg("\n%s:%d Enable all events printing!\n", 
            __FILE__, __LINE__);

    while (UT_run) { 
        OSAL_logMsg("\n\n%s:%d Ring infc %d will use streamId %d.\n", 
                __FILE__, __LINE__, infc, streamId);

        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 5, VTSP_TONE_NMAX,
                VTSP_TONE_TMAX);

        OSAL_logMsg("%s:%d Pausing while ringing.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(7000);

        OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);

        /* Software Gain */
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, 0, 0);
        
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, 60, 8000);
        OSAL_logMsg("%s:%d Pausing during tone.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(4000);

        OSAL_logMsg("%s: Starting infc%d streamId%d..\n", __FILE__, 
                infc, streamId);

        cnameLoop = _UT_updateCname(infc, cnameLoop);

        stream_ptr = UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

        coder = VTSP_CODER_T38;
        for (coderloop = 0; coderloop < 11; coderloop++) { 
            
            coder = UT_updateCoder(++coder, stream_ptr);
            if (UT_eventTaskDisplay) {
                OSAL_logMsg("%s:%d new encoder=%d loop=%d\n",
                        __FILE__, __LINE__, coder, coderloop);
            }
            UT_EXPECT2(VTSP_OK, VTSP_streamModify, 
                    infc, stream_ptr);

            OSAL_taskDelay(6000);
            if (UT_testTimeSec != 0) { 
                UT_testTimeSec -= 6;
                if (UT_testTimeSec <= 0) { 
                    UT_run = 0;
                    break;
                }
            }
        }

        OSAL_logMsg("%s: Ending streamId %d..\n", 
                __FILE__, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);

        streamId = (streamId + 1) % 2;
    }


    return (UT_PASS);
}

UT_Return UT_testStreamPeerVideo(
        vint         infc,
        VTSP_StreamVideo *stream_ptr)
{
    /* For preventing the compiler error */
#ifdef _VTSP_STREAM_VIDEO_H_
    uvint streamId;

    streamId   = stream_ptr->streamId;
    stream_ptr = UT_updateStreamDataVideo(infc, streamId);
    UT_run     = 1;

    /* Disable SRTP */
    OSAL_logMsg("\n%s:%d XXX XXX Disable SRTP in s2 Test XXX XXX\n",
            __FILE__, __LINE__);
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;

    UT_eventTaskDisplay = 1;
    OSAL_logMsg("\n%s:%d Enable all events printing!\n",
            __FILE__, __LINE__);

    OSAL_logMsg("%s: Starting infc%d streamId%d..\n", __FILE__,
            infc, streamId);

    UT_EXPECT2(VTSP_OK, VTSP_streamVideoStart, infc, stream_ptr);
    
    while (UT_run) {


        if (UT_testTimeSec >= 0) { 
            UT_testTimeSec -= 1;
            if (UT_testVideoKeyIntSec > 0) {
                if (0 == (UT_testTimeSec % UT_testVideoKeyIntSec)) {
                    VTSP_streamVideoRequestKeyFrame(infc, streamId);
                    OSAL_logMsg("%s: requesting key frame infc%d streamId%d..\n", __FILE__,
                                infc, streamId);
                }
            }
            if (UT_testTimeSec <= 0) {
                UT_run = 0;
                break;
            }
        }
        else {
            return (UT_PASS);
        }
        OSAL_taskDelay(1000);

    }
    
    OSAL_logMsg("%s: Ending streamId %d..\n",
            __FILE__, streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamVideoEnd, infc, stream_ptr->streamId);

    OSAL_taskDelay(1000);

#endif
    return (UT_PASS);
}

UT_Return UT_testStreamPeerToneMixing(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint          UT_myToneLocalSequence[20];
    vint           mixSelect;
    vint           delay;
    uvint          tId1;
    uvint          tId2;
    uvint          numToneIds;
    uint32         repeat;
    uvint          coderloop;
    uvint          coder;
    uvint          streamId;

    /* User defined tone Ids */
    tId1 = 10;
    tId2 = 29;
    numToneIds = 4;
    UT_myToneLocalSequence[0] = tId1;
    UT_myToneLocalSequence[1] = tId2;
    UT_myToneLocalSequence[2] = tId1;
    UT_myToneLocalSequence[3] = tId2;

    /* Configure tone Ids 5 and 6 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_myToneTemplateRing);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 6, &UT_myToneTemplateCallWait);

    UT_run = 1;

    streamId = stream_ptr->streamId;

    while (UT_run) { 

        OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__, 
                streamId);

        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        for (coderloop = 0, 
                coder=VTSP_CODER_G711U; coderloop < 8; coderloop++) { 

            coder = UT_updateCoder(++coder, stream_ptr);

            OSAL_logMsg("%s:%d new encoder: %d\n",
                    __FILE__, __LINE__, coder);
            UT_EXPECT3(VTSP_OK, VTSP_streamModifyEncoder, infc, 
                    streamId, coder);

            OSAL_taskDelay(2000);

            OSAL_logMsg("%s:\n\n\n\nContinuing TONE test.\n"
                "Tone local sequence with mixing\n",
                __FILE__);

            mixSelect   = VTSP_TONE_BREAK_MIX;
            delay       = 10000;
            repeat      = 3;
            UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, 
                infc,
                UT_myToneLocalSequence,
                numToneIds,
                mixSelect,
                repeat);
            OSAL_taskDelay(delay);
            
            OSAL_logMsg("%s:\n\n\n\nContinuing TONE test.\n"
                "Tone local sequence with no mixing\n",
                __FILE__);

            mixSelect   = 0;
            delay       = 10000;
            repeat      = 3;
            UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, 
                infc,
                UT_myToneLocalSequence,
                numToneIds,
                mixSelect,
                repeat);
            OSAL_taskDelay(delay);
            
            OSAL_taskDelay(2000);
        }

        OSAL_logMsg("%s: Ending streamId %d..\n", 
                __FILE__, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    }


    return (UT_PASS);
}


/* 
 * This is a stream test designed to run for VQT only.
 * For VQT there must be:
 *  - no osal msgs
 *  - no coder changes
 *
 */

UT_Return UT_testStreamVQT(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    VTSP_JbTemplate jbConfig;
    uvint           streamId;
    vint            timer1;

    UT_run = 1;
    timer1 = 0;

    streamId = stream_ptr->streamId;

    /* Turn off event printing
     */
    OSAL_logMsg("%s: Event printing OFF.\n", __FILE__);
    UT_eventTaskDisplay = 1;

    OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__, 
            streamId);
    UT_updateStreamData(infc, streamId);
    OSAL_logMsg("%s: Encoder = %d.\n", __FILE__, 
            stream_ptr->encoder);

    if (UT_streamJBFixed) { 
        OSAL_logMsg("%s: ====> JB FIXED MODE TEST.\n", __FILE__);
        OSAL_logMsg("%s: JB will alternate between FIXED mode "
                "and VOICE mode\n", __FILE__);
        OSAL_logMsg("%s: every 2 minutes.  'F' is printed for Fixed mode,\n",
                __FILE__);
        OSAL_logMsg("%s: and 'v' is printed for Voice mode\n", 
                __FILE__);
        jbConfig.control = VTSP_TEMPL_CONTROL_JB_VOICE;
        jbConfig.streamId = stream_ptr->streamId;
    }
    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

    if (UT_testTimeSec < 0) { 
        /* Leave stream running, return to caller */
        return (UT_PASS);
    }
    
    while (UT_run) { 
        OSAL_taskDelay(1000);
        if (UT_testTimeSec != 0) { 
            UT_testTimeSec -= 1;
            if (UT_testTimeSec <= 0) { 
                break;
            }
        }
        if (UT_streamJBFixed) { 
            timer1--;
            if (timer1 <= 0) { 
                if (VTSP_TEMPL_CONTROL_JB_VOICE == jbConfig.control) {
                    /* Switch to FIXED mode */
                    OSAL_logMsg("JB FIXED mode\n");
                    jbConfig.control = VTSP_TEMPL_CONTROL_JB_FIXED;
                    jbConfig.maxLevel = 700;
                    jbConfig.initLevel = 450;
                }
                else {
                    /* Switch to VOICE mode */
                    OSAL_logMsg("JB VOICE mode\n");
                    jbConfig.control = VTSP_TEMPL_CONTROL_JB_VOICE;
                    jbConfig.maxLevel = 200;
                    jbConfig.initLevel = 0;
                }
                VTSP_config(VTSP_TEMPL_CODE_JB, infc, &jbConfig);
                timer1 = 60 * 2;
            }
        }
    }

    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, stream_ptr->streamId);


    return (UT_PASS);
}

/* 
 * This is a stream test designed to run for VQT only.
 * For VQT there must be:
 *  - no osal msgs
 *  - no coder changes
 *
 */
UT_Return UT_testStreamVQTall(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uint32          encodeTime;
    uint32          encoder;
    uint32          extension;

    encoder = VTSP_CODER_G711U;
    encodeTime = 10;
    extension = 0;

    /*
     * Print an important note
     */
    OSAL_logMsg("%s: %d Can set time using:\n    date MMDDhhmmCCYY\n\n",
            __FILE__, __LINE__);
    UT_eventTaskDisplay = 0;


    /* 
     * Turn off event printing, and start the stream.
     */
    OSAL_logMsg("%s: Event printing OFF.\n", __FILE__);
    UT_eventTaskDisplay = 0;

    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
    OSAL_taskDelay(1000);

    while (1) {
        /*
         * Print the time, and start with new parameters.
         */
        _UT_printTime();
        OSAL_taskDelay(100);
        stream_ptr->encoder = encoder;
        stream_ptr->encodeTime[encoder] = encodeTime;
        stream_ptr->extension = extension;
        OSAL_logMsg("%s: encoder = %d, encodeTime = %d, extension = %d\n",
                __FILE__, encoder, encodeTime, extension);
        OSAL_taskDelay(100);
        UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);

        /*
         * Sleep for 10 minutes
         */
        OSAL_taskDelay(600000);

        encodeTime += 10;
        if (encodeTime > 60) {
            /*
             * Repeat the test, incrementing the coders
             */
            encodeTime = 10;
            encoder = UT_updateCoder(++encoder, stream_ptr);
        }
        if (VTSP_CODER_G723_30MS == encoder) {
            /* 
             * G723 always uses 30 ms encodeTime
             * Test the two rates
             */
            if (10 == encodeTime) {
                extension = 0;
            }
            else if (20 == encodeTime) {
                /* second run through, use 5.3k rate */
                extension = VTSP_MASK_EXT_G723_53;
            }
            else {
                encodeTime = 10;
                encoder = UT_updateCoder(++encoder, stream_ptr);
            }
        }
        if (VTSP_CODER_G722P1_20MS == encoder) {
            /* 
             * G723 always uses 30 ms encodeTime
             * Test the two rates
             */
            if (VTSP_MASK_EXT_G722P1_32 == extension) {
                /* end of test */
                break;
            }
            if (10 == encodeTime) {
                extension = 0;
            }
            else if (20 == encodeTime) {
                /* second run through, use 32k rate */
                extension = VTSP_MASK_EXT_G722P1_32;
            }
            else {
                encodeTime = 10;
                encoder = UT_updateCoder(++encoder, stream_ptr);
            }
        }
    }

    OSAL_logMsg("%s: VQT all test is finished!\n\n",
            __FILE__);
    _UT_printTime();

    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, stream_ptr->streamId);
    OSAL_taskDelay(1000);

    return (UT_PASS);
}
#ifdef VTSP_ENABLE_T38
UT_Return UT_testT38Ans(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint streamId;
    vint testt;

    UT_run = 1;

    while (UT_run == 1) {
        streamId = stream_ptr->streamId;
    
        UT_EXPECT1(UT_PASS, UT_updateCid, &UT_cidObj);
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
    
        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        OSAL_taskDelay(400);
    
        OSAL_logMsg("%s:%d new encoder: %d\n",
                __FILE__, __LINE__, VTSP_CODER_T38);
        stream_ptr->encoder = VTSP_CODER_T38;
        stream_ptr->extension = 0;
        stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
        stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
        UT_EXPECT2(VTSP_OK, VTSP_streamModify, 
                infc, stream_ptr);
        
        for (testt=UT_TIMER_SECS;testt>0; testt-=UT_TIMER_STEP) {
            OSAL_taskDelay(UT_TIMER_STEP*1000);
            if (testt<=(3*UT_TIMER_STEP)) {
                OSAL_logMsg("%s:%d T38Ans() testt=%d\n",
                        __FILE__, __LINE__, testt);
            }
            if (UT_run!=1) break;
        }
        OSAL_logMsg("%s: streamEnd at time %d: infc=%d, streamId=%d\n",
                   __FILE__, testt, infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    }
    return (UT_PASS);
}
#endif


#ifdef VTSP_ENABLE_T38
UT_Return UT_testT38Call(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint streamId;
    vint testt;

    UT_run = 1;

    while (1 == UT_run) {
        streamId = stream_ptr->streamId;

        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        OSAL_taskDelay(4000);
        OSAL_logMsg("%s:%d new encoder: %d\n",
                __FILE__, __LINE__, VTSP_CODER_T38);
        stream_ptr->encoder = VTSP_CODER_T38;
        stream_ptr->extension = 0;
        stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
        stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
        UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);
        
        OSAL_taskDelay(4400);
        for (testt=UT_TIMER_SECS;testt>0; testt-=UT_TIMER_STEP) {
            OSAL_taskDelay(UT_TIMER_STEP*1000);
            if (testt <= (3*UT_TIMER_STEP)) {
                OSAL_logMsg("%s:%d T38Call() testt=%d\n",
                       __FILE__, __LINE__, testt);
            }
            if (UT_run != 1) break;
        }
        OSAL_logMsg("%s: streamEnd at time %d: infc=%d, streamId=%d\n",
                   __FILE__, testt, infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    }
    return (UT_PASS);
}
#endif

UT_Return UT_g711Ans(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint streamId;
    vint testt;

    UT_run = 1;

    while(UT_run == 1) {
        streamId = stream_ptr->streamId;
    
        UT_EXPECT1(UT_PASS, UT_updateCid, &UT_cidObj);
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
    
        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        OSAL_taskDelay(4000);
    
        OSAL_logMsg("%s:%d new encoder: %d\n",
                __FILE__, __LINE__, VTSP_CODER_G711U);
        stream_ptr->encoder = VTSP_CODER_G711U;
        stream_ptr->extension = 0;
        stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
        stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
        UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);
        
        OSAL_taskDelay(4400);
        for (testt=UT_TIMER_SECS;testt>0; testt-=UT_TIMER_STEP) {
            OSAL_taskDelay(UT_TIMER_STEP*1000);
            if(testt<=(3*UT_TIMER_STEP)) {
                OSAL_logMsg("%s:%d T38Ans() testt=%d\n",
                       __FILE__, __LINE__, testt);
            }
            if(UT_run!=1) break;
        }
        OSAL_logMsg("%s: streamEnd at time %d: infc=%d, streamId=%d\n",
                   __FILE__, testt, infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    }
    return (UT_PASS);
}


UT_Return UT_g711Call(
        vint         infc,
        VTSP_Stream *stream_ptr)
{
    uvint streamId;
    vint testt;

    UT_run = 1;

    while(UT_run == 1) {
        streamId = stream_ptr->streamId;

        UT_updateStreamData(infc, streamId);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, 60, 8000);
        OSAL_logMsg("%s:%d Pausing during tone.\n", __FILE__, __LINE__);
        OSAL_taskDelay(9000);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        OSAL_taskDelay(4000);
        OSAL_logMsg("%s:%d new encoder: %d\n",
                __FILE__, __LINE__, VTSP_CODER_G711U);
        stream_ptr->encoder = VTSP_CODER_G711U;
        stream_ptr->extension = 0;
        stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
        stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
        UT_EXPECT2(VTSP_OK, VTSP_streamModify, infc, stream_ptr);
        
        OSAL_taskDelay(4400);
        for (testt=UT_TIMER_SECS;testt>0; testt-=UT_TIMER_STEP) {
            OSAL_taskDelay(UT_TIMER_STEP*1000);
            if (testt<=(3*UT_TIMER_STEP)) {
                OSAL_logMsg("%s:%d Call() testt=%d\n",
                       __FILE__, __LINE__, testt);
            }
            if(UT_run!=1) break;
        }
        OSAL_logMsg("%s: streamEnd at time %d: infc=%d, streamId=%d\n",
                   __FILE__, testt, infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    }
    return (UT_PASS);
}

UT_Return UT_testSrtpStream(
    vint         infc,
    VTSP_Stream *stream_ptr)
{
    uvint coderloop;
    uvint coder;
    uvint streamId;
    vint  cnameLoop;
    vint  count;
    vint  securityType;

char defaultKey[30] = {
    0xe1, 0xf9, 0x7a, 0x0d, 0x3e, 0x01, 0x8b, 0xe0,
    0xd6, 0x4f, 0xa3, 0x2c, 0x06, 0xde, 0x41, 0x39,
    0x0e, 0xc6, 0x75, 0xad, 0x49, 0x8a, 0xfe, 0xeb,
    0xb6, 0x96, 0x0b, 0x3a, 0xab, 0xe6};

    securityType = VTSP_SRTP_SECURITY_SERVICE_CONF_AUTH_32;

    UT_run     = 1;
    cnameLoop  = 0;
    UT_encoder = VTSP_CODER_G711U;
//    securityType = VTSP_SRTP_SECURITY_SERVICE_NONE;
   
    /* Configure tone Ids 5 and 6 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_myToneTemplateRing);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 6, &UT_myToneTemplateCallWait);

    /* 
     * XXX a9206 WB platform can't determine when is silence XXX
     * Do not test a9206 with silence compression enabled.
     */
    UT_silComp   = 0xffffffff;
    UT_dtmfRelay = 0xffffffff;
    OSAL_logMsg("\n%s:%d XXX XXX silComp = 0x%x, dtmfRelay = 0x%x XXX XXX\n", 
            __FILE__, __LINE__, UT_silComp, UT_dtmfRelay);

            
    streamId   = stream_ptr->streamId;
    stream_ptr = UT_updateStreamData(infc, streamId);

        
    UT_eventTaskDisplay = 1;
    OSAL_logMsg("\n%s:%d XXX XXX Disable all events printing! XXX XXX\n", 
            __FILE__, __LINE__);


    while (UT_run) { 

        OSAL_logMsg("\n\n%s:%d Ring infc %d will use streamId %d.\n", 
                __FILE__, __LINE__, infc, streamId);

        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 5, VTSP_TONE_NMAX,
                VTSP_TONE_TMAX);

        OSAL_logMsg("%s:%d Pausing while ringing.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(7000);
        OSAL_logMsg("%s: Stopping ring..\n", __FILE__);
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);


        /* Software Gain */
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, 0, 0);
        
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, 60, 8000);
        OSAL_logMsg("%s:%d Pausing during tone.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(4000);

        OSAL_logMsg("%s: Starting infc%d streamId%d..\n", __FILE__, 
                infc, streamId);

        cnameLoop = _UT_updateCname(infc, cnameLoop);

        stream_ptr = UT_updateStreamData(infc, streamId);
        
        stream_ptr->srtpSecurityType = securityType++;
        for (count = 0; count < VTSP_SRTP_KEY_STRING_MAX_LEN; count++) {
            stream_ptr->srtpSendKey[count] = defaultKey[count];
            stream_ptr->srtpRecvKey[count] = defaultKey[count];
        }
        
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

        coder = VTSP_CODER_G711U;
        
        OSAL_logMsg("%s:%d Stream security policy=%d\n",
            __FILE__, __LINE__, stream_ptr->srtpSecurityType);
                
        for (coderloop = 0; coderloop < 11; coderloop++) { 
            coder = UT_updateCoder(VTSP_CODER_G711U, stream_ptr);
            if (UT_eventTaskDisplay) {
                OSAL_logMsg("%s:%d new encoder=%d loop=%d\n",
                        __FILE__, __LINE__, coder, coderloop);
            }
            
            UT_EXPECT2(VTSP_OK, VTSP_streamModify, 
                infc, stream_ptr);         
            
            OSAL_taskDelay(600);
        }

        OSAL_logMsg("%s: Ending streamId %d..\n", 
                __FILE__, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);

        if (VTSP_SRTP_SECURITY_SERVICE_CONF_AUTH_32 == stream_ptr->srtpSecurityType) {
            UT_run = 0;
        }
            
        if (streamId == 0) {
            streamId = 1;
        }
        else {
            streamId = 0;
        }
    }
    return (UT_PASS);
}

UT_Return UT_testSrtpStreamVQT(
    vint         infc,
    VTSP_Stream *stream_ptr)
{
    uvint streamId;
    vint  count;
    vint  securityType;

    char defaultKey[30] = {0xe1, 0xf9, 0x7a, 0x0d, 0x3e, 0x01, 0x8b, 0xe0,
    0xd6, 0x4f, 0xa3, 0x2c, 0x06, 0xde, 0x41, 0x39,
    0x0e, 0xc6, 0x75, 0xad, 0x49, 0x8a, 0xfe, 0xeb,
    0xb6, 0x96, 0x0b, 0x3a, 0xab, 0xe6};

    securityType = VTSP_SRTP_SECURITY_SERVICE_CONF_AUTH_32;

    UT_run     = 1;
    
    UT_encoder = VTSP_CODER_G711U;
//    securityType = VTSP_SRTP_SECURITY_SERVICE_NONE;

    streamId = stream_ptr->streamId;

    /* Turn off event printing
     */
    OSAL_logMsg("%s: Event printing OFF.\n", __FILE__);
    UT_eventTaskDisplay = 1;

    OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__, 
            streamId);
    UT_updateStreamData(infc, streamId);
    OSAL_logMsg("%s: Encoder = %d.\n", __FILE__, 
            stream_ptr->encoder);

    stream_ptr->srtpSecurityType = securityType;
    for (count = 0; count < VTSP_SRTP_KEY_STRING_MAX_LEN; count++) {
        stream_ptr->srtpSendKey[count] = defaultKey[count];
        stream_ptr->srtpRecvKey[count] = defaultKey[count];
    }

    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);

    if (UT_testTimeSec < 0) { 
        /* Leave stream running, return to caller */
        return (UT_PASS);
    }

    while (UT_run) { 
        OSAL_taskDelay(1000);
        if (UT_testTimeSec != 0) { 
            UT_testTimeSec -= 1;
            if (UT_testTimeSec <= 0) { 
                break;
            }
        }
    }

    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, stream_ptr->streamId);

    return (UT_PASS);
}

