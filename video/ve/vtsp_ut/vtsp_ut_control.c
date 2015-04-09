/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5622 $ $Date: 2008-03-27 19:14:57 -0400 (Thu, 27 Mar 2008) $
 *
 */

/* Test control functions
 * Assumes the VTSP has already been initialized
 */
#include <string.h>
#include <stdio.h>

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"

/* Define some structures for testing */
VTSP_ToneTemplate UT_myToneTemplate10 = {
    200,                            /* freq1 */
    600,                            /* freq2 */
    -10,                            /* power1 */
    -15,                            /* power2 */
    2,                              /* cadences */
    200,                            /* make1 */
    300,                            /* break1 */
    1,                              /* repeat1 */
    100,                            /* make2 */
    400,                            /* break2 */
    1,                              /* repeat2 */
    0,                              /* make3 */
    0,                              /* break3 */
    0                               /* repeat3 */
};

VTSP_ToneTemplate UT_myToneTemplate11 = {
    800,                            /* freq1 */
    0,                              /* freq2 */
    -10,                            /* power1 */
    0,                              /* power2 */
    1,                              /* cadences */
    200,                            /* make1 */
    100,                            /* break1 */
    1,                              /* repeat1 */
    0,                              /* make2 */
    0,                              /* break2 */
    0,                              /* repeat2 */
    0,                              /* make3 */
    0,                              /* break3 */
    0                               /* repeat3 */
};

VTSP_ToneTemplate UT_1KToneTemplate = {
    1000,                           /* freq1 */
    0,                              /* freq2 */
    -10*2,                          /* power1 */
    0,                              /* power2 */
    1,                              /* cadences */
    1000,                           /* make1 */
    0,                              /* break1 */
    60,                             /* repeat1 */
    0,                              /* make2 */
    0,                              /* break2 */
    0,                              /* repeat2 */
    0,                              /* make3 */
    0,                              /* break3 */
    0                               /* repeat3 */
};

UT_Return UT_updateCidSeq(
                          uvint         vector,
                          uvint         seq,
                          VTSP_CIDData *cid_ptr)
{
    
    char          time[20];
    char          num[20];
    char          name[60];
    vint          hr;
    vint          min;
    vint          day;
    vint          mon;
    
    VTSP_cidDataPackInit(cid_ptr);
    
    /* defaults */
    strcpy(time, "08301245");
    strcpy(num, "8055643424");
    strcpy(name, "D2 Tech vPort 0xd290");
    
    if (1 == vector) {
        min = seq % 60;
        hr = seq % 24;
        day = (seq % 30) + 1;
        mon = (seq % 12) + 1;
        sprintf(time, "%02d%02d%02d%02d",
                mon, day, hr, min);
        
        sprintf(num, "805%07d", seq);
        sprintf(name, "0x%08x", seq);
        
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, num, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, name, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time, cid_ptr);
    }
    else if (2 == vector) {
        UT_updateCid(cid_ptr);
    }
    else if (3 == vector) {
        min = seq % 60;
        hr = seq % 24;
        day = (seq % 30) + 1;
        mon = (seq % 12) + 1;
        sprintf(time, "%02d%02d%02d%02d",
                mon, day, hr, min);
        
        sprintf(num, "805%07d", seq);
        sprintf(name, "0x%08x-0x%08x-0x%08x", seq, seq, seq);
        
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, num, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, name, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time, cid_ptr);
    }
    else {
        /* Use fixed CID String */
        seq = vector;
        
        min = seq % 60;
        hr = seq % 24;
        day = (seq % 30) + 1;
        mon = (seq % 12) + 1;
        sprintf(time, "%02d%02d%02d%02d",
                mon, day, hr, min);
        
        sprintf(num, "805%07d", seq);
        sprintf(name, "0x%08x", seq);
        
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, num, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, name, cid_ptr);
        VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time, cid_ptr);
    }
    return (UT_PASS);
}

/* Create circular test of different CID vectors
 */
UT_Return UT_updateCid(
                       VTSP_CIDData *cid_ptr)
{
    
    char          time[10];
    static  uvint min1 = 1;
    static  uvint min2 = 0;
    static  uvint hr1 = 0;
    static  uvint hr2 = 0;
    static  uvint cidNum = 0;
    
    VTSP_cidDataPackInit(cid_ptr);
    
    /* increment minute field of CIDS time */
    min1++;
    if (min1 > 9) {
        min1 = 0;
        min2++;
        if (min2 > 5) {
            min2 = 0;
            /* increment hr field of CIDS time */
            hr1++;
            if (hr1 > 9) {
                hr1 = 0;
                hr2++;
                if (hr2 > 1) {
                    hr2 = 0;
                }
            }
        }
    }
    strcpy(time, "01020304");
    time[4] = '0' + hr2;
    time[5] = '0' + hr1;
    time[6] = '0' + min2;
    time[7] = '0' + min1;
    
    switch (cidNum) {
        default:
            cidNum = 0;
            /* fallthrough */
        case 0:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "8052528260",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "Zubair Khan",
                             cid_ptr);
            break;
        case 1:
            /* The following is a test vector copied from Teltone TLS-5 */
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "102",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "JONES JENNIFER",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, "08061203",
                             cid_ptr);
            break;
        case 2:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "7537337777",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "Abc",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time,
                             cid_ptr);
            break;
            
        case 3:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "1112232222",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "Abcdef X",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time,
                             cid_ptr);
            break;
        case 4:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "5553334444",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "AbcdefAbcdef Abcdef",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time,
                             cid_ptr);
            break;
        case 5:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "12122232323",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "AbcdefAbcdef",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time,
                             cid_ptr);
            break;
        case 6:
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "1212211211",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "XyzdefAbcdef Qxyz",
                             cid_ptr);
            VTSP_cidDataPack(VTSP_CIDDATA_FIELD_DATE_TIME, time,
                             cid_ptr);
            break;
            
    }
    cidNum++;
    
    return (UT_PASS);
    
}

/* Run through complete test of RING generation and RING events
 *
 * This test requires user interaction & user verification
 */
UT_Return UT_testRing(
                      vint infc)
{
    vint          tId;
    char          buf[10];
    
    OSAL_logMsg("%s:\n\nBeginning RING test.\n"
                "This test requires user interaction.\n"
                "Verify EVENTS are correct as they are printed.\n", __FILE__);
    
    OSAL_logMsg("\n\n%s: Playing TONE to infc%d.\n\n", __FILE__, infc);
    
    OSAL_logMsg("\n\n%s: Place infc%d in RELEASE to start.\n\n"
                "\n(press enter for next test)\n",__FILE__, infc);
    
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, VTSP_TONE_NMAX,
               VTSP_TONE_TMAX);
    D2_getLine(buf, 2);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
    
    /*
     * Test MAX_REPEAT
     */
    tId = 1;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Max Repeat Event\n", __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               1, /* # rings */
               14000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(9000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "2. RING_GENERATE ACTIVE TRAILING\n"
                "3. RING_GENERATE MAX_REPEAT.\n"
                "\n(press enter for next test)\n",__FILE__);
    
    D2_getLine(buf, 2);
    
    tId = 3;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Max Repeat Event\n", __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               1, /* # rings */
               14000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(9000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "2. RING_GENERATE ACTIVE TRAILING\n"
                "3. RING_GENERATE MAX_REPEAT.\n"
                "\n(press enter for next test)\n", __FILE__);
    
    D2_getLine(buf, 2);
    
    
    /*
     * Test MAX_TIMEOUT
     */
    tId = 1;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Max Timeout Event Prior to TRAILING EDGE\n",
                __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               1, /* # rings */
               1000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(10000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "3. RING_GENERATE MAX_TIMEOUT.\n"
                "\n(press enter for next test)\n", __FILE__);
    
    D2_getLine(buf, 2);
    
    /*
     * Test MAX_TIMEOUT
     */
    tId = 1;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Max Timeout Event After TRAILING EDGE\n",
                __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               2, /* # rings */
               8000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(12000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "2. RING_GENERATE ACTIVE TRAILING\n"
                "3. RING_GENERATE ACTIVE LEADING\n"
                "4. RING_GENERATE MAX_TIMEOUT.\n"
                "\n(press enter for next test)\n", __FILE__);
    
    D2_getLine(buf, 2);
    
    /*
     * Test HALTED from VTSP_ringStop
     */
    tId = 1;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Halted Event from VTSP_ringStop.\n",
                __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               1, /* # rings */
               14000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(2000);
    UT_EXPECT1(VTSP_OK, VTSP_ringStop, infc);
    OSAL_taskDelay(4000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "2. RING_GENERATE HALTED.\n"
                "\n(press enter for next test)\n",__FILE__);
    
    D2_getLine(buf, 2);
    
    /*
     * Test HALTED from SEIZE
     */
    tId = 1;
    OSAL_logMsg("\n\n\n%s: Ringing infc%d once with tId%d, no CID, "
                "Halted Event from USER.\n", __FILE__, infc, tId);
    
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId, /* ring template */
               3, /* # rings */
               14000, /* ms timeout */
               NULL);
    
    OSAL_taskDelay(200);
    OSAL_logMsg("\n\n\n%s: SEIZE infc%d NOW (during ring).\n", __FILE__, infc);
    OSAL_taskDelay(9000);
    OSAL_logMsg("\n\n\n%s: Verify EVENTS above are:\n"
                "1. RING_GENERATE ACTIVE LEADING\n"
                "2. RING_GENERATE HALTED.\n"
                "3. HOOK_SEIZE LEADING.\n"
                "\n\nRELEASE infc,"
                "\n(press enter for next test)\n", __FILE__);
    
    D2_getLine(buf, 2);
    
    OSAL_logMsg("\n\n\n%s: End of test\n",
                __FILE__);
    
    return (UT_PASS);
}

UT_Return UT_testCidSeq(
                        vint         infc,
                        vint         type,
                        vint         tId,
                        vint         vector,
                        VTSP_Stream *stream_ptr)
{
    VTSP_CIDData  cidMsg;
    vint          streamId;
    vint          coder;
    vint          seq;
    
    seq = vector;
    
    /* Software Gain */
    UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, 0, 0, 0);
    
    if (1 == type) {
        /* Ring with CID
         */
        while (1) {
            OSAL_logMsg("\n\n\n%s: Ringing infc%d with tId%d, "
                        "with CID Seq#%d\n", __FILE__, infc, tId, seq);
            
            UT_updateCidSeq(vector, seq, &cidMsg);
            UT_printCid(&cidMsg);
            OSAL_taskDelay(100);
            UT_EXPECT5(VTSP_OK, VTSP_ring,
                       infc,
                       tId, /* ring template */
                       2, /* # rings */
                       VTSP_RING_TMAX, /* ms timeout */
                       &cidMsg);
            
            OSAL_taskDelay(12000);
            OSAL_logMsg("%s: Verify CID Seq#%d Appears on phone.\n",
                        __FILE__, seq);
            OSAL_taskDelay(12000);
            
            seq++;
        }
    }
    
    if (2 == type) {
        /* Call-Waiting with CID
         */
        streamId = 0;
        coder = VTSP_CODER_G711U;
        
        UT_EXPECT5(VTSP_OK, VTSP_ring,
                   infc,
                   1, /* ring template */
                   1, /* # rings */
                   1000, /* ms timeout */
                   NULL);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, 60, 8000);
        OSAL_taskDelay(1000);
        OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__, streamId);
        UT_updateStreamData(infc, streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
        OSAL_logMsg("%s:%d new encoder: %d\n", __FILE__, __LINE__, coder);
        UT_EXPECT2(VTSP_OK, VTSP_streamModify,
                   infc, stream_ptr);
        while (1) {
            OSAL_logMsg("\n\n\n%s: CID/CW on infc%d "
                        "with CID Seq#%d,\n", __FILE__, infc, seq);
            
            UT_updateCidSeq(vector, seq, &cidMsg);
            UT_printCid(&cidMsg);
            /* Pause to ensure console print does not corrupt audio! */
            OSAL_taskDelay(1000);
            UT_EXPECT2(VTSP_OK, VTSP_cidOffhook,
                       infc,
                       &cidMsg);
            
            /* Pause to ensure phone times out the 3rd party caller! */
            OSAL_taskDelay(6000);
            OSAL_logMsg("%s: Verify CID Seq#%d Appears on phone.\n",
                        __FILE__, seq);
            OSAL_taskDelay(6000);
            
            seq++;
        }
    }
    
    return (UT_PASS);
}

/* 
 * This test requires user interaction & user verification
 */
UT_Return UT_testLevels(
        vint infc)
{ 
    char              buf[10];

    OSAL_logMsg("\n\n\n%s: TX Level Test, infc%d\n",
            __FILE__, infc);

    UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, 0, 0);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);

    OSAL_logMsg("\n\n\n%s: Measure for Noise Floor now, then press ENTER.\n",
            __FILE__);
    D2_getLine(buf, sizeof(buf));

    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 7, VTSP_TONE_NMAX, 
            VTSP_TONE_TMAX);
    OSAL_logMsg("\n\n\n%s: Measure for 0dB now, then press ENTER.\n",
            __FILE__);
    D2_getLine(buf, sizeof(buf));


    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 9, VTSP_TONE_NMAX, 
            VTSP_TONE_TMAX);
    OSAL_logMsg("\n\n\n%s: Measure for -6dB now, then press ENTER.\n",
            __FILE__);
    D2_getLine(buf, sizeof(buf));


    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 9, 10000, 5000);
    OSAL_logMsg("\n\n\n%s: Measure for -6dB now, then press ENTER.\n",
            __FILE__);
    D2_getLine(buf, sizeof(buf));

    OSAL_logMsg("\n\n\n%s: End of test\n",
            __FILE__);

    return (UT_PASS);
}

UT_Return UT_controlIO(
    vint arg)
{
    VTSP_AecTemplate aecConfig;
    vint             infc;
    vint             testOpt;
    vint             control;
    vint             value;

    infc = 0;
    value = 0;

    testOpt = UT_getUserTestParam(UT_CONTROL_IO);

    switch (testOpt) {
        case 0:
            control = VTSP_CONTROL_INFC_IO_OUTPUT_SPEAKER;
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSFREE_MODE;
            VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);
            break;
        case 1:
            control = VTSP_CONTROL_INFC_IO_OUTPUT_HANDSET;
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE;
            VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);
            break;
        case 2:
            control = VTSP_CONTROL_INFC_IO_OUTPUT_HEADSET;
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE;
            VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);
            break;
        case 3:
            control = VTSP_CONTROL_INFC_IO_INPUT_HANDSET_MIC;
            break;
        case 4:
            control = VTSP_CONTROL_INFC_IO_INPUT_HEADSET_MIC;
            break;
        case 5:
            control = VTSP_CONTROL_INFC_IO_MUTE_INPUT_HW;
            value = 1;
            break;
        case 6:
            control = VTSP_CONTROL_INFC_IO_MUTE_INPUT_HW;
            value = 0;
            break;
        case 7:
            control = VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_HW;
            value = 1;
            break;
        case 8:
            control = VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_HW;
            value = 0;
            break;
        case 9:
            control = VTSP_CONTROL_INFC_IO_MUTE_INPUT_SW;
            value = 1;
            break;
        case 10:
            control = VTSP_CONTROL_INFC_IO_MUTE_INPUT_SW;
            value = 0;
            break;
        case 11:
            control = VTSP_CONTROL_INFC_IO_AUDIO_ATTACH;
            value = VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_ENABLE;
            break;
        case 12:
            control = VTSP_CONTROL_INFC_IO_AUDIO_ATTACH;
            value = VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_DISABLE;
            break;
        case 13:
            value = UT_getUserTestParam(UT_GAIN);
            UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, value, value);
            return(UT_PASS);
        default:
            OSAL_logMsg("Error unknown option\n");
            return (UT_UNKNOWN);
    };

    UT_EXPECT3(VTSP_OK, VTSP_infcControlIO, infc, control, value);

    return (UT_PASS);
}

UT_Return UT_controlAEC(
    vint arg)
{
    VTSP_AecTemplate aecConfig;
    vint             infc;
    vint             testOpt;

    infc = 0;

    testOpt = UT_getUserTestParam(UT_CONTROL_AEC);

    switch (testOpt) {
        case 0:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_NORMAL;
            break;
        case 1:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_BYPASS;
            break;
        case 2:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_BYPASS_AGC;
            break;
        case 3:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE;
            break;
        case 4:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSFREE_MODE;
            break;
        case 5:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_TAIL_LENGTH;
            aecConfig.tailLen = UT_getUserTestParam(UT_AEC_TAIL_LEN);
            break;
        case 6:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_FREEZE;
            break;
        case 7:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_COMFORT_NOISE;
            break;
        case 8:
            aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HALF_DUPLEX;
            break;
        default:
            OSAL_logMsg("Error unknown option\n");
            return (UT_UNKNOWN);
    };
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_AEC, infc, &aecConfig);

    return (UT_PASS);
}


UT_Return UT_controlGainDb(
    vint arg)
{
    vint infc;
    vint gain;
    vint duration;
    vint up;

    infc = 0;
    gain = -20;
    up = 1;

    duration = UT_getUserTestParam(UT_TEST_LENGTH) * 1000; /* test time in ms */

    OSAL_logMsg("Testing S/W gain between -20 and +20 dB\n");

    while (1) {
        /* change gain from -20 to 20 and back */
        if (20 == gain) {
            up = 0;
        }
        else if (-20 == gain) {
            up = 1;
        }
        if (up) {
            gain++;
        }
        else {
            gain--;
        }

        /* call API */
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, gain, gain);
        
        /* sleep 500ms, update timer, and break when test done */
        OSAL_taskDelay(500);
        duration -= 500;
        if (duration < 0) {
            break;
        }
    }

    return (UT_PASS);
}

UT_Return UT_controlJB(
    vint arg)
{
    VTSP_JbTemplate jbConfig;
    vint infc;

    infc = 0;

    jbConfig.streamId  = UT_getUserTestParam(UT_STREAM_ID);
    jbConfig.control   = UT_getUserTestParam(UT_JB_MODE);
    jbConfig.initLevel = UT_getUserTestParam(UT_JB_INIT_LEVEL);
    jbConfig.maxLevel  = UT_getUserTestParam(UT_JB_MAX_LEVEL);

    OSAL_logMsg("UT_controlJB: stream=%d, mode=%d, init=%d, max=%d\n", 
            jbConfig.streamId, jbConfig.control, jbConfig.initLevel, 
            jbConfig.maxLevel);

    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_JB, infc, &jbConfig);

    return (UT_PASS);
}

UT_Return UT_controlCN(
    vint arg)
{
    VTSP_CnTemplate cnConfig;
    vint infc;

    infc = 0;

    cnConfig.control      = VTSP_TEMPL_CONTROL_CN_POWER_ATTEN;    
    cnConfig.cnPwrAttenDb =  UT_getUserTestParam(UT_CN_PWR_ATTEN);

    VTSP_config(VTSP_TEMPL_CODE_CN, infc, &cnConfig);

    OSAL_logMsg("UT_controlCN: atten pwr=%d\n", cnConfig.cnPwrAttenDb);

    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_CN, infc, &cnConfig);

    return (UT_PASS);
}

/* Run through all tones
 * Then changes gain on the test tone
 *
 * This test requires user interaction & user verification
 */
UT_Return UT_testToneGain(
        vint infc)
{ 
    vint              tId;
    vint              gainTx;
    vint              gainRx;
    VTSP_ToneTemplate tone;
    VTSP_FmtdTemplate fmtdConfig;

    OSAL_logMsg("\n\n\n%s(%d): Enabling DTMF,FMTD detect on infc%d\n",
            __FILE__, __LINE__, infc);

    fmtdConfig.control = VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_INFC;
    fmtdConfig.powerMin = -33;
    VTSP_config(VTSP_TEMPL_CODE_FMTD, 0, &fmtdConfig);
    
    fmtdConfig.control = VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_PEER;
    fmtdConfig.powerMin = -33;
    VTSP_config(VTSP_TEMPL_CODE_FMTD, 0, &fmtdConfig);
    
    OSAL_logMsg("\n\n\n%s: Enabling DTMF,FMTD detect on infc%d\n",
            __FILE__, infc);

    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, 
            VTSP_DETECT_DTMF | VTSP_DETECT_FMTD);

    for (tId = 0; tId < 12; tId++) { 
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
        OSAL_logMsg("\n\n\n%s: infc%d tId %d\n",
                __FILE__, infc, tId);
        OSAL_taskDelay(400);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, 1, 
                2000);
        OSAL_taskDelay(3000);
    }

    /* Configure 1Khz -10dB tone
     *
     * typedef struct {
     *     uvint   freq1;
     *     uvint   freq2;
     *     uvint   power1;
     *     uvint   power2;
     *     uvint   cadences;
     *     uvint   make1;
     *     uvint   break1;
     *     uvint   repeat1;
     *     uvint   make2;
     *     uvint   break2;
     *     uvint   repeat2;
     *     uvint   make3;
     *     uvint   break3;
     *     uvint   repeat3;
     * } VTSP_ToneTemplate;
     */
    tone.freq1 = 1000;
    tone.freq2 = 0;
    tone.power1 = -40;
    tone.power2 = 0;
    tone.cadences = 1;
    tone.make1 = 1000;
    tone.break1 = 0;
    tone.repeat1 = 1;
    tone.make2 = 0;
    tone.break2 = 0;
    tone.repeat2 = 0;
    tone.make3 = 0;
    tone.break3 = 0;
    tone.repeat3 = 0;

    tId = 9;

    OSAL_logMsg("\n%s: Configure and play tone Id = %d\n",
            __FILE__, tId);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId, &tone);
    OSAL_taskDelay(100);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, 1, 
            1000);
    OSAL_taskDelay(1500);

    /* 
     * Tone audio power should change from
     * 1Khz -20dB
     * to
     * 1Kz 0dB
     */
    for (gainTx = -40, gainRx = -40; gainTx <= 40; gainTx++, gainRx++) { 
        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
        UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, gainTx, gainRx);
        OSAL_logMsg("\n%s: Configure infc%d gainTx=%d gainRx=%d\n",
                __FILE__, infc, gainTx, gainRx);
        OSAL_taskDelay(100);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, 1, 
                400);
        OSAL_taskDelay(400);
    }

    gainTx = 0;
    gainRx = 0;
    OSAL_logMsg("\n\n\n%s: infc%d gainTx=%d gainRx=%d\n",
            __FILE__, infc, gainTx, gainRx);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
    UT_EXPECT3(VTSP_OK, VTSP_infcControlGain, infc, gainTx, gainRx);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, VTSP_TONE_NMAX, 
            3000);

    OSAL_logMsg("\n\n\n%s: End of test\n",
            __FILE__);

    return (UT_PASS);
}

/* 
 * Test VTSP_toneLocalSequence()
 *
 * This test requires user interaction & user verification
 */
UT_Return UT_testToneLocalSeq(
        vint infc)
{
    char    buf[10];
    vint    mixSelect;
    vint    delay;
    uvint   tId1;
    uvint   tId2;
    uvint   tId3;
    uvint   numToneIds;
    uint32  repeat;
    uvint   UT_myToneLocalSequence[20];

    /* User defined tone Ids */
    tId1 = 10;
    tId2 = 29;
    tId3 = 28;
    numToneIds = 4;
    UT_myToneLocalSequence[0] = tId1;
    UT_myToneLocalSequence[1] = tId2;
    UT_myToneLocalSequence[2] = tId1;
    UT_myToneLocalSequence[3] = tId2;

    /* Configure tone Ids 10 and 11 */
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId1, &UT_myToneTemplate10);
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId2, &UT_myToneTemplate11);

    OSAL_logMsg("\n\n\n%s: Start tone local generation tests on infc%d\n"
            "1) tone sequence, with mixing, and no mixing\n"
            "\n(enter to continue)\n",
            __FILE__, infc);
    D2_getLine(buf, 2);

    mixSelect   = VTSP_TONE_BREAK_MIX;
    delay       = 4000;
    repeat      = 1;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    mixSelect   = 0;
    delay       = 4000;
    repeat      = 1;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    OSAL_logMsg("%s:\n\n\n\nContinuing TONE test.\n"
            "2) tone local tests\n"
            "\n(enter to continue)\n",
            __FILE__);
    D2_getLine(buf, 2);

    OSAL_logMsg("%s: Play unconfigured TONE tId%d.\n"
            "Should hear silence, if this test is run after VTSP init.\n",
            __FILE__, tId3);
    delay = 6000;
    repeat = 5;
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId3, repeat, VTSP_TONE_TMAX);
    OSAL_taskDelay(delay);

    OSAL_logMsg("%s: Play TONE tId%d. ", __FILE__, tId2);
    OSAL_taskDelay(200);
    delay = 3000;
    repeat = 5;
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId2, repeat, VTSP_TONE_TMAX);
    OSAL_taskDelay(delay);

    OSAL_logMsg("%s:\n\n\n\nContinuing TONE test.\n"
            "3) tone sequence, with repeats\n"
            "\n(enter to continue)\n",
            __FILE__);
    D2_getLine(buf, 2);

    mixSelect   = VTSP_TONE_BREAK_MIX;
    delay       = 10000;
    repeat      = 3;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    mixSelect   = 0;
    delay       = 10000;
    repeat      = 3;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    OSAL_logMsg("%s:\n\n\n\nContinuing TONE test.\n"
            "4) tone sequence interrupt\n"
            "\n(enter to continue)\n",
            __FILE__);
    D2_getLine(buf, 2);

    mixSelect   = VTSP_TONE_BREAK_MIX;
    delay       = 1000;
    repeat      = 3;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc); 
    OSAL_taskDelay(delay);

    mixSelect   = 0;
    delay       = 1400;
    repeat      = 3;
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, UT_myToneLocalSequence,
            numToneIds, mixSelect, repeat);
    OSAL_taskDelay(delay);

    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc); 
    OSAL_taskDelay(delay);

    delay = 1800;
    repeat = 5;
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId1, repeat, VTSP_TONE_TMAX);
    OSAL_taskDelay(delay);

    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc); 
    OSAL_taskDelay(delay);
    
    delay = 750;
    repeat = 5;
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId2, repeat, VTSP_TONE_TMAX);
    OSAL_taskDelay(delay);

    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc); 
    OSAL_taskDelay(delay);

    OSAL_logMsg("\n\n\n%s: End of test\n", __FILE__);
    return (UT_PASS);
}

/*
 * Test Visual Message Waiting Indication (VMWI). This test turns on the
 * "Indicator On" message and then turns it off. This test requires user
 * interaction.
 */
UT_Return UT_testVmwi(
                      vint infc)
{
    VTSP_CIDData        cidMsg;
    VTSP_RingTemplate   vmwi_ringTemplate;
    vint                tId;
    char                buf[10];
    char                vi[2];
    
    
    OSAL_logMsg("\n\n%s: Place infc%d in RELEASE to start\n", __FILE__, infc);
    OSAL_logMsg("%s: Press enter to begin test\n", __FILE__);
    
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 1, VTSP_TONE_NMAX,
               VTSP_TONE_TMAX);
    D2_getLine(buf, 2);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
    
    OSAL_taskDelay(500);
    
    /*
     * Configure a custom ring template. VMWI uses a short ring template.
     * Note, parameters below are specified to VTSP using ms.
     */
    vmwi_ringTemplate.cadences = 1;
    vmwi_ringTemplate.make1 = 200;
    vmwi_ringTemplate.break1 = (6000 - 200);
    vmwi_ringTemplate.make2 = 0;
    vmwi_ringTemplate.break2 = 0;
    vmwi_ringTemplate.make3 = 0;
    vmwi_ringTemplate.break3 = 0;
    vmwi_ringTemplate.cidBreakNum = 1;
    
    tId = 7;
    UT_EXPECT2(VTSP_OK, VTSP_configRing, tId, &vmwi_ringTemplate);
    
    OSAL_logMsg("\n\n\n%s: Sending infc%d with tId%d, with VMWI-CID\n",
                __FILE__, infc, tId);
    
    OSAL_taskDelay(500);
    
    /*
     * Build the CID message, and send the short ring with the data.
     */
    VTSP_cidDataPackInit(&cidMsg);
    VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NUMBER, "8055643424",
                     &cidMsg);
    VTSP_cidDataPack(VTSP_CIDDATA_FIELD_NAME, "D2 Technologies",
                     &cidMsg);
    /*
     * Format for VMI data is length followed by "Indicator On" Message (0xFF)
     */
    vi[0] = 1;
    vi[1] = 0xff;
    VTSP_cidDataPack(VTSP_CIDDATA_FIELD_VI, vi,
                     &cidMsg);
    UT_printCid(&cidMsg);
    OSAL_taskDelay(100);
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId,            /* ring template */
               2,              /* # rings */
               VTSP_RING_TMAX, /* ms timeout */
               &cidMsg);
    
    OSAL_taskDelay(15000);
    OSAL_logMsg("\n\n\n%s: Verify VMWI-CID Appears on phone.\n", __FILE__);
    D2_getLine(buf, 2);
    
    
    OSAL_logMsg("\n\n\n%s: (enter) to test turning the VWI off\n", __FILE__);
    D2_getLine(buf, 2);
    
    /*
     * Build new CID message, and to turn the message waiting indicator off.
     */
    VTSP_cidDataPackInit(&cidMsg);
    
    /*
     * Format for VMI data is length followed by "Indicator Off" Message (0x0)
     */
    vi[0] = 1;
    vi[1] = 0;
    VTSP_cidDataPack(VTSP_CIDDATA_FIELD_VI, vi,
                     &cidMsg);
    
    UT_printCid(&cidMsg);
    OSAL_taskDelay(100);
    UT_EXPECT5(VTSP_OK, VTSP_ring,
               infc,
               tId,            /* ring template */
               2,              /* # rings */
               VTSP_RING_TMAX, /* ms timeout */
               &cidMsg);
    
    OSAL_taskDelay(15000);
    OSAL_logMsg("\n\n%s: Verify VWI is turned off.\n", __FILE__);
    D2_getLine(buf, 2);
    
    OSAL_logMsg("\n%s: End of test\n", __FILE__);
    
    
    return (UT_PASS);
}

UT_Return UT_CalibTx(
                     vint infc)
{
    vint              tId1;
    
    infc = 0;
    tId1 = 11;
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId1, &UT_1KToneTemplate);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId1, VTSP_TONE_NMAX,
               VTSP_TONE_TMAX);
    OSAL_logMsg("Sending -10 dbm 1K signal to PCM Tx \n Check the dbm"
                " from the Malden ... ");
    OSAL_taskDelay(60000);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
    return (UT_PASS);
}

UT_Return UT_CalibRx(
                     vint infc)
{
    infc = 0;
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 0, VTSP_TONE_NMAX,
               VTSP_TONE_TMAX);
    OSAL_logMsg("Playing silence to PCM Tx ... ");
    OSAL_logMsg("Please give 1KHZ 0 0dbm tone from Melden to PCM "
                "Rx (FXS port) ..., Check Rx RMS value: ");
    OSAL_taskDelay(60000);
    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, infc);
    return (UT_PASS);
}

