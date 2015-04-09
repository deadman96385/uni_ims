/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 7809 $ $Date: 2008-10-13 16:50:06 -0400 (Mon, 13 Oct 2008) $
 *
 */

/* Test stream tone functions
 * VTSP must be initialized and started prior to running these functions.
 */
#include "osal.h"
#include "vtsp.h"
#include "vtsp_ut.h"

VTSP_ToneTemplate UT_sToneCW = {
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

VTSP_ToneTemplate UT_sToneRing = {
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


UT_Return UT_printStream(
        uvint        infc,
        VTSP_Stream *stream_ptr)
{
    
    OSAL_logMsg("%s:%d infc%d streamId %08x ----- \n",
            __FILE__, __LINE__, infc, stream_ptr->streamId);
    OSAL_logMsg("%s:%d remoteIpAddr %08x\n",
            __FILE__, __LINE__, OSAL_netNtohl(stream_ptr->remoteAddr.ipv4));
    OSAL_logMsg("%s:%d remoteDataPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteAddr.port));
    OSAL_logMsg("%s:%d remoteControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteControlPort));
    OSAL_logMsg("%s:%d localIpAddr %08x\n",
            __FILE__, __LINE__, OSAL_netNtohl(stream_ptr->localAddr.ipv4));
    OSAL_logMsg("%s:%d localDataPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localAddr.port));
    OSAL_logMsg("%s:%d localControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localControlPort));
    OSAL_logMsg("%s:%d encoder %d\n",
            __FILE__, __LINE__, stream_ptr->encoder);
    OSAL_logMsg("%s:%d encodeTime %d\n", __FILE__, __LINE__,
            stream_ptr->encodeTime[stream_ptr->encoder]);
    OSAL_logMsg("%s:%d silenceComp 0x%x\n",
            __FILE__, __LINE__, stream_ptr->silenceComp);
    OSAL_logMsg("%s:%d encodeTime[CN] %d\n",
            __FILE__, __LINE__, stream_ptr->encodeTime[VTSP_CODER_CN]);
    OSAL_logMsg("%s:%d dtmfRelay 0x%x\n",
            __FILE__, __LINE__, stream_ptr->dtmfRelay);
    OSAL_logMsg("%s:%d extension 0x%x\n",
            __FILE__, __LINE__, stream_ptr->extension);
    OSAL_logMsg("%s:%d confMask 0x%x\n",
            __FILE__, __LINE__, stream_ptr->confMask);
    return (UT_PASS);
}

UT_Return UT_testStreamTone(
        vint         arg)
{
    typedef struct {
        uvint             infcId;
        VTSP_Stream       streams[2];
        VTSP_CIDData      cid;
    } UT_Phone;

    typedef struct {
        VTSP_QueryData   *vtspQuery_ptr;
        vint              vtspNumInfc;
        vint              vtspFxsFirst;
        vint              vtspFxsLast;
        vint              vtspStreams;
        vint              vtspStreamPerInfc;
        vint              vtspNumTemplateIds;
        vint              vtspMaxSequenceLen;
        vint              numFxs;
        vint              streamIndexMax;
        UT_Phone          phones[4];
    } UT_Gateway;

    UT_Gateway       *sys_ptr;
    UT_Phone         *phone_ptr;
    UT_Phone         *phoneA_ptr;
    UT_Phone         *phoneB_ptr;

    VTSP_Stream      *stream_ptr;
    VTSP_Stream      *streamA_ptr;
    VTSP_Stream      *streamB_ptr;
    vint              phoneId;
    vint              infcId;
    vint              infc;
    vint              streamId;
    vint              streamIndex;
    vint              cnameLoop;
    uint32            word;

    uvint             toneTable_ptr[30];
    vint              tableLen;
    
    /* Initialize system objects */
    sys_ptr = OSAL_memCalloc(1, sizeof(UT_Gateway), 0);

    /* Query vTSP capabilities for dynamic object allocation */
    sys_ptr->vtspQuery_ptr = VTSP_query();

    if (NULL == sys_ptr->vtspQuery_ptr) {
        OSAL_logMsg("%s: vtspQuery NULL\n", __FILE__); 
        return (UT_FAIL);
    }

    /* Configure tone Ids 5 and 6 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_sToneRing);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, 5, &UT_sToneRing);

    sys_ptr->vtspFxsFirst = sys_ptr->vtspQuery_ptr->hw.fxsFirst;
    sys_ptr->vtspFxsLast = sys_ptr->vtspQuery_ptr->hw.fxsLast;
    sys_ptr->numFxs = sys_ptr->vtspFxsLast - sys_ptr->vtspFxsFirst + 1;
    sys_ptr->vtspStreams = sys_ptr->vtspQuery_ptr->stream.numStreams;
    sys_ptr->vtspStreamPerInfc = sys_ptr->vtspQuery_ptr->stream.numPerInfc;
    sys_ptr->vtspNumTemplateIds = sys_ptr->vtspQuery_ptr->tone.numTemplateIds;
    sys_ptr->vtspMaxSequenceLen = sys_ptr->vtspQuery_ptr->tone.maxSequenceLen;

    if (sys_ptr->numFxs < 2) {
        OSAL_logMsg("%s: This test requires >= 2 infcs, only found %d.\n", 
                __FILE__, sys_ptr->numFxs);
        UT_FAILMSG;
        return (UT_FAIL);
    }

    /* Runtime alloc streams based on vTSP capabilities */
    sys_ptr->streamIndexMax = sys_ptr->numFxs * sys_ptr->vtspStreamPerInfc;

    /* Initialize phone & stream objects; N stream objects per phone */
    for (phoneId = 0, streamIndex = 0, infcId = sys_ptr->vtspFxsFirst; 
            infcId <= sys_ptr->vtspFxsLast; infcId++, phoneId++) { 
        /* Initialize VTSP infc params per phone */
        phone_ptr = &sys_ptr->phones[phoneId];
        phone_ptr->infcId = infcId;

        for (streamId = 0;
                streamId < sys_ptr->vtspStreamPerInfc; 
                streamId++, streamIndex++) { 

            /* Assign the stream object to the phone */
            stream_ptr = &sys_ptr->phones[phoneId].streams[streamId];

            /* Constant streamId does not change */
            stream_ptr->streamId             = streamId;

            /* Init the stream to defaults */
            stream_ptr->dir     = VTSP_STREAM_DIR_SENDRECV;
            stream_ptr->peer    = VTSP_STREAM_PEER_NETWORK;
            stream_ptr->encoder = VTSP_CODER_G711U;
            stream_ptr->encodeTime[VTSP_CODER_G711U] = 10;
            stream_ptr->encodeTime[VTSP_CODER_CN]    = 100;
            stream_ptr->encodeType[VTSP_CODER_G711U] = 0;
            stream_ptr->decodeType[VTSP_CODER_G711U] = 0;
            stream_ptr->decodeType[VTSP_CODER_G711U] = 13;
            stream_ptr->encodeType[VTSP_CODER_G711U] = 13;
            stream_ptr->extension   = 0;
            stream_ptr->dtmfRelay   = 0;
            stream_ptr->silenceComp = VTSP_MASK_CODER_G711U;
            stream_ptr->confMask    = 0;
            /*
             * Map port numbers so voice path of
             * (infc,streamId) connects as (0,0)<->(1,0) and (0,1)<->(1,1)
             *
             * infc   streamId   localDataPort   remoteDataPort
             *   0      0           5001            5007
             *   0      1           5003            5005
             *   1      0           5005            5003
             *   1      1           5007            5001
             */
            stream_ptr->localAddr.ipv4    = 0;  /* ANY (no bind) */
            word = 5001 + (streamIndex * 2);
            stream_ptr->localAddr.port    = OSAL_netHtons(word);
            word = 5002 + (streamIndex * 2);
            stream_ptr->localControlPort  = OSAL_netHtons(word);
            stream_ptr->remoteAddr.ipv4   = OSAL_netHtonl(0x7f000001);
            word = 5007 + (-streamIndex * 2);
            stream_ptr->remoteAddr.port   = OSAL_netHtons(word);
            word = 5007 + (-streamIndex * 2);
            stream_ptr->remoteControlPort = OSAL_netHtons(word);
        }
    }

    /* Use phone0 and phone1 for test */
    phoneA_ptr = &sys_ptr->phones[0];
    phoneB_ptr = &sys_ptr->phones[1];
    /* Use the first stream on phone0 and last stream on phone1 */
    streamA_ptr = &phoneA_ptr->streams[0];
    streamB_ptr = &phoneB_ptr->streams[1];

    OSAL_logMsg("UT_testStreamTone\nphoneA_ptr->stream_ptr=0x%p streamId=%d\n"
        "phoneB_ptr->stream_ptr=0x%p streamId=%d\n", streamA_ptr,
        streamA_ptr->streamId, streamB_ptr, streamB_ptr->streamId);

    cnameLoop = 0;
    UT_run = 1;
    while (UT_run) { 
        
        OSAL_logMsg("\n\n%s:%d Ring infc%d will use streamId%d.\n", 
                __FILE__, __LINE__, phoneA_ptr->infcId, 
                streamA_ptr->streamId);
        UT_printStream(phoneA_ptr->infcId, streamA_ptr);
        OSAL_logMsg("\n\n%s:%d Ring infc%d will use streamId%d.\n", 
                __FILE__, __LINE__, phoneB_ptr->infcId, 
                streamB_ptr->streamId);
        UT_printStream(phoneB_ptr->infcId, streamB_ptr);

        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, phoneA_ptr->infcId, 5,
                VTSP_TONE_NMAX, VTSP_TONE_TMAX);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, phoneB_ptr->infcId, 5,
                VTSP_TONE_NMAX, VTSP_TONE_TMAX);

        OSAL_logMsg("%s:%d Pausing while ringing.\n", 
                __FILE__, __LINE__);
        OSAL_taskDelay(9000);

        OSAL_logMsg("%s: Stopping ring on infc%d.\n", __FILE__, 
                phoneA_ptr->infcId);
        OSAL_logMsg("%s: Stopping ring on infc%d.\n", __FILE__, 
                phoneB_ptr->infcId);
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, phoneA_ptr->infcId);
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, phoneB_ptr->infcId);

        /* Software Gain */
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, phoneA_ptr->infcId, 0, 0);
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, phoneB_ptr->infcId, 0, 0);
        
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, phoneA_ptr->infcId, 1, 60, 800);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, phoneB_ptr->infcId, 1, 60, 800);
        OSAL_logMsg("%s:%d Pausing during tone.\n", __FILE__, __LINE__);
        OSAL_taskDelay(1000);

        OSAL_logMsg("%s: Starting infc%d streamId%d.\n", __FILE__, 
                phoneA_ptr->infcId, streamA_ptr->streamId);
        cnameLoop = _UT_updateCname(phoneA_ptr->infcId, cnameLoop);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, 
                phoneA_ptr->infcId, streamA_ptr);

        OSAL_logMsg("%s: Starting infc%d streamId%d.\n", __FILE__, 
                phoneB_ptr->infcId, streamB_ptr->streamId);
        cnameLoop = _UT_updateCname(phoneB_ptr->infcId, cnameLoop);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, 
                phoneB_ptr->infcId, streamB_ptr);

        /* Pause for RTP to stabilize */
        OSAL_taskDelay(2000);

        /* 
         * Begin tone generation to both local and remote peer
         */
        infc     = phoneA_ptr->infcId;
        streamId = streamA_ptr->streamId;

        /* Generate to local */
        UT_EXPECT5(VTSP_OK, VTSP_streamTone, phoneA_ptr->infcId,
                streamA_ptr->streamId, 1, 1, 1500);
        OSAL_taskDelay(2000);

        /* Generate to local */
        OSAL_logMsg("DIAL TONE (DT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 10, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("DIAL TONE (DT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 10;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence, infc, streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("RING BACK TONE (RBT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 11, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("RING BACK TONE (RBT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 11;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("BUSY TONE (BT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 12, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("BUSY TONE (BT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 12;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("PRIVATE DIAL TONE (PDT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 13, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("PRIVATE DIAL TONE (PDT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 13;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("SECOND DIAL TONE (SDT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 14, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("SECOND DIAL TONE (SDT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 14;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("ACCEPTANCE TONE (CPT) -> LOCAL\n");
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 15, 1, 8000);
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("ACCEPTANCE TONE (CPT) -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 15;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */

        /* Generate to local */
        OSAL_logMsg("HOLD SERVICE TONE (HST) Sequence -> LOCAL \n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 16;
        toneTable_ptr[tableLen++] = 17;
        UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
                infc,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */
        OSAL_taskDelay(6000);

        /* Generate to stream */
        OSAL_logMsg("HOLD SERVICE TONE (HST) Sequence -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 16;
        toneTable_ptr[tableLen++] = 17;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */
        OSAL_taskDelay(6000);


        
        OSAL_logMsg("INCOMING IDENTIFICATION TONE (IIT) Sequence -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 18;
        toneTable_ptr[tableLen++] = 19;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */
        OSAL_taskDelay(6000);

        OSAL_logMsg("SPECIFIC INCOMING IDENTIFICATION TONE (SIIT) Sequence -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 20;
        toneTable_ptr[tableLen++] = 21;
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,           /* numToneIds */
                0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                 /* repeat */
        OSAL_taskDelay(6000);

        OSAL_logMsg("HOWLER TONE (HOW) Sequence -> LOCAL\n");
        OSAL_logMsg("HOWLER TONE (HOW) Sequence -> STREAM\n");
        OSAL_taskDelay(200);
        tableLen = 0;
        toneTable_ptr[tableLen++] = 22; /* f1 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 22; /* f1 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 22; /* f1 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 22; /* f1 */ 
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 24; /* f3 */
        toneTable_ptr[tableLen++] = 23; /* f2 */
        toneTable_ptr[tableLen++] = 25; /* f3 long break */

        OSAL_logMsg("Test VTSP_TONE_BREAK_MIX\n");

        UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
                infc,
                (uvint *)toneTable_ptr,
                tableLen,            /* numToneIds */
                VTSP_TONE_BREAK_MIX, /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                  /* repeat */
        UT_EXPECT6(VTSP_OK, VTSP_streamToneQuadSequence,
                infc,
                streamId,
                (uvint *)toneTable_ptr,
                tableLen,            /* numToneIds */
                VTSP_TONE_BREAK_MIX, /* control, 0 or VTSP_TONE_BREAK_MIX */
                1);                  /* repeat */
        OSAL_taskDelay(100);


        /* Turn off stream tone */
        UT_EXPECT2(VTSP_OK, VTSP_streamToneQuadStop, infc, streamId);
        OSAL_taskDelay(200);

        /* Turn off local tone */
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
        OSAL_taskDelay(2000);


        OSAL_logMsg("%s: Ending infc%d streamId%d.\n", __FILE__, 
                phoneA_ptr->infcId, streamA_ptr->streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, 
                phoneA_ptr->infcId, streamA_ptr->streamId);
        OSAL_logMsg("%s: Ending infc%d streamId%d.\n", __FILE__, 
                phoneB_ptr->infcId, streamB_ptr->streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, 
                phoneB_ptr->infcId, streamB_ptr->streamId);
    }

    /* Close all streams - should already be closed, always returns OK */
    for (phoneId = 0; phoneId < sys_ptr->numFxs; phoneId++) { 
        phone_ptr = &sys_ptr->phones[phoneId];
        for (streamId = 0; 
                streamId < sys_ptr->vtspStreamPerInfc; streamId++) { 
            UT_EXPECT2(VTSP_OK, VTSP_streamEnd, 
                    phone_ptr->infcId, 
                    phone_ptr->streams[streamId].streamId);
        }
    }

    /* Dealloc all memory for shutdown */
    OSAL_memFree(sys_ptr, 0);
    
    return (UT_PASS);
}
