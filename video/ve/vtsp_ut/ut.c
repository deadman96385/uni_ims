/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7809 $ $Date: 2008-10-13 16:50:06 -0400 (Mon, 13 Oct 2008) $
 *
 */

#include "osal.h"
#include "vtsp.h"
#include "vtsp_ut.h"
#include "voice_net.h"
#include "voer.h"
#include "vpr.h"
#include "tic.h"

#define D2_VPORT_REVISION D2_Release_VTSP_UT
extern char const D2_Release_VTSP_UT[];

//#define UT_FIXED_CMD    "v2"

extern int D2_getLine(
        char         *buf_ptr,
        unsigned int  max);

uvint            VAPP_run = 1;
OSAL_TaskId      UT_taskId = 0;

VTSP_Stream       UT_vtspStream0;
VTSP_Stream       UT_vtspStream1;
VTSP_Stream       UT_vtspStream2;
VTSP_Stream       UT_vtspStream3;
VTSP_StreamVideo  UT_vtspStreamv0;

/*
 * Fixed IP Addresses of boards
 * MUST BE IN HOST BYTE ORDER
 */

UT_Return UT_initStreamStruct(
        VTSP_Stream *stream_ptr,
        vint streamId)
{
    vint cnt;

    stream_ptr->streamId = streamId;
    stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
    stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
    stream_ptr->encoder = VTSP_CODER_G711U;
    stream_ptr->extension = 0;
    stream_ptr->dtmfRelay = 0;
    stream_ptr->silenceComp = 0;
    stream_ptr->confMask = 0;
    stream_ptr->remoteAddr.ipv4   = 0;
    stream_ptr->remoteAddr.port   = 0;
    stream_ptr->remoteControlPort = 0;
    stream_ptr->localAddr.ipv4    = 0;
    stream_ptr->localAddr.port    = 0;
    stream_ptr->localControlPort = 0;
    /*
     * Bug 3197
     * Clear the srtpKey to use the NULL encoding algorithm by default
     */
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;

    for (cnt = 0; cnt < VTSP_SRTP_KEY_STRING_MAX_LEN; cnt++) {
        stream_ptr->srtpSendKey[cnt] = 0;
        stream_ptr->srtpRecvKey[cnt] = 0;
    }

    stream_ptr->encodeTime[VTSP_CODER_G711U] = 20;
    stream_ptr->encodeType[VTSP_CODER_G711U] = 0;
    stream_ptr->decodeType[VTSP_CODER_G711U] = 0;

    stream_ptr->encodeTime[VTSP_CODER_G711A] = 20;
    stream_ptr->encodeType[VTSP_CODER_G711A] = 8;
    stream_ptr->decodeType[VTSP_CODER_G711A] = 8;

    stream_ptr->encodeTime[VTSP_CODER_CN] = 10000;
    stream_ptr->encodeType[VTSP_CODER_CN] = 13;
    stream_ptr->decodeType[VTSP_CODER_CN] = 13;

#ifdef VTSP_ENABLE_G729
    stream_ptr->encodeTime[VTSP_CODER_G729] = 20;
    stream_ptr->encodeType[VTSP_CODER_G729] = 18;
    stream_ptr->decodeType[VTSP_CODER_G729] = 18;
#endif

#ifdef VTSP_ENABLE_G726
    stream_ptr->encodeTime[VTSP_CODER_G726_32K] = 20;
    stream_ptr->encodeType[VTSP_CODER_G726_32K] = 2;
    stream_ptr->decodeType[VTSP_CODER_G726_32K] = 2;
#endif

#ifdef VTSP_ENABLE_ILBC
    stream_ptr->encodeTime[VTSP_CODER_ILBC_20MS] = 20;
    stream_ptr->encodeType[VTSP_CODER_ILBC_20MS] = 96;
    stream_ptr->decodeType[VTSP_CODER_ILBC_20MS] = 96;

    stream_ptr->encodeTime[VTSP_CODER_ILBC_30MS] = 30;
    stream_ptr->encodeType[VTSP_CODER_ILBC_30MS] = 97;
    stream_ptr->decodeType[VTSP_CODER_ILBC_30MS] = 97;
#endif

#ifdef VTSP_ENABLE_16K_MU
    stream_ptr->encodeTime[VTSP_CODER_16K_MU] = 10;
    stream_ptr->encodeType[VTSP_CODER_16K_MU] = 99;
    stream_ptr->decodeType[VTSP_CODER_16K_MU] = 99;
#endif

#ifdef VTSP_ENABLE_G722
    stream_ptr->encodeTime[VTSP_CODER_G722] = 10;
    stream_ptr->encodeType[VTSP_CODER_G722] = 9;
    stream_ptr->decodeType[VTSP_CODER_G722] = 9;
#endif

#ifdef VTSP_ENABLE_G722P1
    stream_ptr->encodeTime[VTSP_CODER_G722P1_20MS] = 20;
    stream_ptr->encodeType[VTSP_CODER_G722P1_20MS] = 103;
    stream_ptr->decodeType[VTSP_CODER_G722P1_20MS] = 103;
#endif

#ifdef VTSP_ENABLE_G723
    stream_ptr->encodeTime[VTSP_CODER_G723_30MS] = 30;
    stream_ptr->encodeType[VTSP_CODER_G723_30MS] = 4;
    stream_ptr->decodeType[VTSP_CODER_G723_30MS] = 4;
#endif

#ifdef VTSP_ENABLE_G711P1
    stream_ptr->encodeTime[VTSP_CODER_G711P1U] = 10;
    stream_ptr->encodeType[VTSP_CODER_G711P1U] = 97;
    stream_ptr->decodeType[VTSP_CODER_G711P1U] = 97;

    stream_ptr->encodeTime[VTSP_CODER_G711P1A] = 10;
    stream_ptr->encodeType[VTSP_CODER_G711P1A] = 96;
    stream_ptr->decodeType[VTSP_CODER_G711P1A] = 96;
#endif

#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
    stream_ptr->encodeTime[VTSP_CODER_GAMRNB_20MS_OA] = 20;
    stream_ptr->encodeType[VTSP_CODER_GAMRNB_20MS_OA] = 105;
    stream_ptr->decodeType[VTSP_CODER_GAMRNB_20MS_OA] = 105;

    stream_ptr->encodeTime[VTSP_CODER_GAMRNB_20MS_BE] = 20;
    stream_ptr->encodeType[VTSP_CODER_GAMRNB_20MS_BE] = 110;
    stream_ptr->decodeType[VTSP_CODER_GAMRNB_20MS_BE] = 110;
#endif

#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
    stream_ptr->encodeTime[VTSP_CODER_GAMRWB_20MS_OA] = 20;
    stream_ptr->encodeType[VTSP_CODER_GAMRWB_20MS_OA] = 109;
    stream_ptr->decodeType[VTSP_CODER_GAMRWB_20MS_OA] = 109;

    stream_ptr->encodeTime[VTSP_CODER_GAMRWB_20MS_BE] = 20;
    stream_ptr->encodeType[VTSP_CODER_GAMRWB_20MS_BE] = 111;
    stream_ptr->decodeType[VTSP_CODER_GAMRWB_20MS_BE] = 111;
#endif

#ifdef VTSP_ENABLE_SILK
    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_8K] = 20;
    stream_ptr->encodeType[VTSP_CODER_SILK_20MS_8K] = 106;
    stream_ptr->decodeType[VTSP_CODER_SILK_20MS_8K] = 106;

    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_16K] = 20;
    stream_ptr->encodeType[VTSP_CODER_SILK_20MS_16K] = 107;
    stream_ptr->decodeType[VTSP_CODER_SILK_20MS_16K] = 107;

    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_24K] = 20;
    stream_ptr->encodeType[VTSP_CODER_SILK_20MS_24K] = 108;
    stream_ptr->decodeType[VTSP_CODER_SILK_20MS_24K] = 108;
#endif

#ifdef VTSP_ENABLE_DTMFR
    stream_ptr->encodeTime[VTSP_CODER_TONE] = 10;
    stream_ptr->encodeType[VTSP_CODER_TONE] = 100;
    stream_ptr->decodeType[VTSP_CODER_TONE] = 100;

    stream_ptr->encodeTime[VTSP_CODER_DTMF] = 10;
    stream_ptr->encodeType[VTSP_CODER_DTMF] = 101;
    stream_ptr->decodeType[VTSP_CODER_DTMF] = 101;
#endif
    return(UT_PASS);
}

UT_Return UT_initStreamStructVideo(
        VTSP_StreamVideo *stream_ptr,
        vint streamId)
{
    vint cnt;

    stream_ptr->streamId = streamId;
    stream_ptr->dir = VTSP_STREAM_DIR_SENDRECV;
    stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
    stream_ptr->encoder = VTSP_CODER_VIDEO_H264;
    stream_ptr->remoteAddr.ipv4   = 0;
    stream_ptr->remoteAddr.port   = 0;
    stream_ptr->remoteControlPort = 0;
    stream_ptr->localAddr.ipv4    = 0;
    stream_ptr->localAddr.port    = 0;
    stream_ptr->localControlPort = 0;
    /*
     * Bug 3197
     * Clear the srtpKey to use the NULL encoding algorithm by default
     */
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;

    for (cnt = 0; cnt < VTSP_SRTP_KEY_STRING_MAX_LEN; cnt++) {
        stream_ptr->srtpSendKey[cnt] = 0;
        stream_ptr->srtpRecvKey[cnt] = 0;
    }

    stream_ptr->encodeTime[VTSP_CODER_G711U] = 0;
    stream_ptr->encodeType[VTSP_CODER_VIDEO_H264] = 99;
    stream_ptr->decodeType[VTSP_CODER_VIDEO_H264] = 99;
    stream_ptr->encodeType[VTSP_CODER_VIDEO_H263] = 100;
    stream_ptr->decodeType[VTSP_CODER_VIDEO_H263] = 100;

    return(UT_PASS);
}


UT_Return UT_quit(
        vint arg)
{
    VAPP_run = 0;
    OSAL_logMsg("%s:%d VAPP_run = %x\n", __FILE__, __LINE__, VAPP_run);
    return (UT_PASS);
}

/*
 * Test Function Table
 * --------
 *
 * First member is command string.
 * Second member is function to call.
 * Third member is description for menu.
 */
UT_TestTableItem UT_testTable[] =
{
    /* System tests */
    { "preinit",   UT_preInit,        "VTSP pre-init API test" },
    { "i",         UT_vtspInit,       "Perform regular init" },
    { "initcheck", UT_init,           "VTSP check for 2nd init API test" },
    { "shutdown",  UT_shutdown,       "VTSP shutdown API test" },
    { "q",         UT_quit,           "Quit (exit vtsp_ut)"},
    /* Control Tests */
    { "io",      UT_controlIO,        "VTSP hardware IO control" },
    { "aec",     UT_controlAEC,       "VTSP AEC control" },
    { "jb",      UT_controlJB,        "VTSP Jitter Buffer control test" },
    { "cn",      UT_controlCN,        "VTSP Comfort Noise control test" },
    { "gain1",   UT_controlGainDb,    "VTSP S/W gain control loop test" },
    /* Voice Tests */
    { "s1",      UT_streamLoopback,   "Stream test, 1 board, network loop" },
    { "s1v",     UT_streamLoopbackVideo,
            "Stream test, 1 board, network loop, video only" },
    { "s2v",     UT_streamPeerVideo,
            "Stream test, 2 boards, video only" },
    { "s2",      UT_streamPeer,       "Stream test, 2 boards" },
    { "s3",      UT_streamHold,       "Stream test, 2 boards" },
    { "v2",      UT_streamVQT,        "VQT Stream test, 2 boards, events=off" },
    { "v3",      UT_streamConfVQT,    "VQT Conf Stream, 3 boards, events=off" },
    { "v4",      UT_streamVQTall,
            "VQT Stream test, 2 boards, Multiple Coder" },
    { "jbs",     UT_streamJBstats,    "JB stats monitoring, for RFC2198 test" },
    { "vce-i0",  UT_streamVce0,       "Voice Conference Endpoint, infc0" },
    { "vcb-i0",  UT_streamVcb0,       "Voice Conference Bridge, infc0" },
    /* Tone Tests */
    { "tg",      UT_testToneGain,     "Tone generation infc gain test" },
    { "t1",      UT_testToneLocalSeq, "Tone local sequence test" },
    { "t2",      UT_streamLoopbackToneMixing,
            "Tone local sequence with mixing" },
    { "tone5ms", UT_testTone5ms,      "Tone 5ms generation" },
    { "tnet1",   UT_testStreamTone,   "Tone generation to Network" },
    { "tloc1",   UT_testToneLocal,    "Tone gen. at specified dB and frequency" },
    { "tloc2",   UT_testToneDbLocal,  "Tone gen. user freq with increasing dB" },
    { "rfc4733", UT_testRfc4733,      "Test RFC 4733 generation" },
    { "level",   UT_testLevels,       "Audio Level Test"},
    /* Net tests */
    { "netbind", UT_testBind,         "Network Bind Test"},
    { "nettx",   UT_testNetTx,        "Network TX Test"},
    { "srtp1",   UT_srtpStream,       "Test SRTP: cycle all enc & auth modes" },
    { "srtp2",   UT_srtpStreamVQT,    "Test SRTP, selectable enc & auth" },
    /*
     * Gateway features migrated from vPort 1.6
     */
    { "p1", UT_flowPlayLocal,   "Local Flow Play with Inactive Streams"},
    { "p2", UT_flowPlayStreamLocal, "Local Flow Play Test, network loopback"},
    { "p3", UT_flowPlayStreamPeer, "Peer Flow Play Test, network loopback"},
    { "rp1", UT_flowPlayRecord,  "Local Flow Play/Record Test"},
#ifdef VTSP_ENABLE_T38
    { "faxrx", UT_testfaxAns,  "T38 Unit test module (Rx side)" },
    { "faxtx", UT_testfaxCall, "T38 Unit test module (Tx side)" },
#endif
//    { "g711faxrx", UT_g711faxAns,  "G.711 Unit test module (Rx side)" },
//    { "g711faxtx", UT_g711faxCall, "G.711 Unit test module (Tx side)" },
    { "r",       UT_testRing,         "FXS Ring and CID Onhook test" },
    { "vmwi",    UT_testVmwi,         "FXS VMWI test" },
    { "cidseq",  UT_callerIdSeq,      "Caller-ID Sequence Test"},
#ifdef VTSP_ENABLE_TONE_QUAD
    { "quad",    UT_testToneQuad,     "Tone Quad generation" },
#endif
#if TIC_INFC_FXO_NUM > 0
    { "fxo-s1",  UT_streamFxoDial,    "FXO dial to stream test" },
    { "fxo-s2",  UT_streamFxoIn,      "FXO incoming to stream test" },
    { "fxo-utd", UT_testUtdConfig,    "FXO UTD test" },
    { "hook",    UT_testControlHook,  "FXS/FXO Hook Control" },
    { "dial",    UT_testFxoDial,      "FXO DTMF Dialing string" },
#ifdef VTSP_ENABLE_PULSE_GENERATE
    { "pdial", UT_testFxoPulseDial, "FXO Pulse Dialing string" },
#endif
#endif  // TIC_INFC_FXO_NUM > 0
#ifdef VTSP_ENABLE_GR909
    { "gr909", UT_gr909Diag,          "GR909 Diagnostic Test" },
#endif
    { "cTx", UT_CalibTx,              "Calibration PCM TX to Phone" },
    { "cRx", UT_CalibRx,              "Calibration Phone to PCM RX" },
#ifdef VTSP_ENABLE_NETLOG
    { "netlog", UT_Netlog,            "Turn on/off Netlog" },
#endif

};

#ifdef VTSP_ENABLE_T38
/*
 * ======== UT_testfaxAns() ========
 */
UT_Return UT_testfaxAns(
        vint arg)
{

    uvint          infc;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = 0;

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    //return (UT_testT38Stream(infc, &UT_vtspStream0));
    return (UT_testT38Ans(infc, &UT_vtspStream0));

}
#endif


#ifdef VTSP_ENABLE_T38
/*
 * ======== UT_testfaxCall() ========
 */
UT_Return UT_testfaxCall(
        vint arg)
{

    uvint           infc;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = 0;

    OSAL_logMsg("%s:  Enter IP Addr of peer (172.16.0.23) : ", __FUNCTION__);
    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    return (UT_testT38Call(infc, &UT_vtspStream0));
}
#endif

/*
 * ======== UT_g711faxAns() ========
 */
UT_Return UT_g711faxAns(
        vint arg)
{
    uvint          infc;
    char           buf[10];

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    OSAL_logMsg("%s:  Telephony Infc (0=default, 0 - n) : ", __FUNCTION__);
    D2_getLine(buf, 20);
    infc = OSAL_atoi(buf);

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_startPort + UT_vtspStream2.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream2.streamId;
    UT_encoder = VTSP_CODER_G711U;
    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_encodeTime = 20;

    return (UT_g711Ans(infc, &UT_vtspStream2));
}

/*
 * ======== UT_g711faxCall() ========
 */
UT_Return UT_g711faxCall(
        vint arg)
{
    uvint          infc;
    char           buf[10];

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    OSAL_logMsg("%s:  Telephony Infc (0=default, 0 - n) : ", __FUNCTION__);
    D2_getLine(buf, 20);
    infc = OSAL_atoi(buf);

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_startPort + UT_vtspStream2.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream2.streamId;
    UT_encoder = VTSP_CODER_G711U;
    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_encodeTime = 20;

    return (UT_g711Call(infc, &UT_vtspStream2));
}

/* Print the CID Msg to the console in visible form.
 */
UT_Return UT_printCid(
        VTSP_CIDData *cid_ptr)
{
    vint           index;
    unsigned char  buf[300];
    struct privateSubjectToChangeAtAnyTimeDataFormat {
        uint16          len;
        unsigned char   data[256];
    } *data_ptr;


    data_ptr = (void *)cid_ptr;
    memset(buf, 0, sizeof(buf));
    for (index = 0; index < data_ptr->len; index++) {
        buf[index] = data_ptr->data[index];
        if (data_ptr->data[index] < ' ' ||
            data_ptr->data[index] > '~') {
            /* unprintable */
            buf[index] = ' ';
        }
    }
    OSAL_logMsg("%s: CidMsg=%s\n", __FILE__, buf);
    return (UT_PASS);
}

/*
 * ======== UT_callerIdSeq() ========
 */
UT_Return UT_callerIdSeq(
    vint arg)
{
    uvint          infc;
    uvint          word;
    uvint          cidType;
    uvint          tId;
    uvint          vector;

    char           buf[10];

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    infc = 0;
    OSAL_logMsg("%s:  Interface [%d] : ", __FUNCTION__, infc);
    D2_getLine(buf, sizeof(buf));
    word = OSAL_atoi(buf);
    infc = word;

    cidType = 1;
    OSAL_logMsg("%s:  CallerId Type 1(ONHOOK) or 2(OFFHOOK) [%d] : ",
                __FUNCTION__, cidType);
    D2_getLine(buf, sizeof(buf));
    word = OSAL_atoi(buf);
    if (0 != word) {
        cidType = word;
    }

    tId = 1;
    if (1 == cidType) {
        /* Type 1 ONHOOK - RINGING CID */
        OSAL_logMsg("%s:  Ring Template [%d] : ", __FUNCTION__, tId);
        D2_getLine(buf, sizeof(buf));
        word = OSAL_atoi(buf);
        if (0 != word) {
            tId = word;
        }
    }
    else {
        /* Type 2 OFFHOOK - CID/CW */
        UT_recvAddr.ipv4 = UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
        UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
        UT_recvPort = UT_startPort + UT_vtspStream0.streamId;
    }

    vector = 1;
    OSAL_logMsg("%s:  Test vector type 1 - n [%d] : ", __FUNCTION__, vector);
    D2_getLine(buf, sizeof(buf));
    word = OSAL_atoi(buf);
    if (0 != word) {
        vector = word;
    }

    return (UT_testCidSeq(infc, cidType, tId, vector, &UT_vtspStream0));
}

UT_Return UT_Netlog(
     vint arg)
{
    uvint           infc;
    OSAL_NetAddress remoteAddr;
    uvint           control;
    char            buf[20];

    VTSP_DebugTemplate debugConfig;

    infc = UT_getUserTestParam(UT_INFC);

    /* Turn On/Off */
    control = 0;
    OSAL_logMsg("%s: Turn On/OFF (0=default turn off, 1=turn on)", __FILE__);
    D2_getLine(buf, 5);
    control = OSAL_atoi(buf);

    if (0 == control) {
        debugConfig.control     = VTSP_TEMPL_CONTROL_DEBUG_PCMDUMP;
        debugConfig.debugEnable = 0;
        UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_DEBUG, infc, &debugConfig);
    }
    else {
        /* get remote IP */
        OSAL_logMsg("%s:  Enter IP Addr of peer (172.16.0.171) : ",
                    __FILE__);
        D2_getLine(buf, 20);
        if (OSAL_FAIL == OSAL_netStringToAddress((int8 *)buf,&remoteAddr)) {
            OSAL_netStringToAddress("172.16.0.171", &remoteAddr);
            OSAL_logMsg("%s: Using default address 172.16.0.171\n",
                        __FILE__);
        }

        debugConfig.control       = VTSP_TEMPL_CONTROL_DEBUG_PCMDUMP;
        debugConfig.debugEnable   = 1;
        debugConfig.debugRemoteIP = OSAL_netHtonl(remoteAddr.ipv4);
        UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_DEBUG, infc, &debugConfig);
    }

    return (UT_PASS);
}

UT_Return UT_gr909Diag(
        vint arg)
{
    uvint  infc;
    vint   word;
    char   buf[10];

    infc = UT_getUserTestParam(UT_INFC);

    OSAL_logMsg("HEMF    : 0\n");
    OSAL_logMsg("FEMF    : 1\n");
    OSAL_logMsg("RFAULT  : 2\n");
    OSAL_logMsg("ROFFHOOK: 3\n");
    OSAL_logMsg("REN     : 4\n");
    OSAL_logMsg("ALL     : 5\n");
    OSAL_logMsg("Choose GR909 test item: ");
    D2_getLine(buf, 2);
    word = OSAL_atoi(buf);

    UT_EXPECT3(VTSP_OK, VTSP_infcControlIO, infc, VTSP_CONTROL_INFC_IO_GR909, word);

    return (UT_PASS);
}


/*
 * ======== UT_testUtdConfig() ========
 */
UT_Return UT_testUtdConfig(
        vint infc)
{
#ifdef VTSP_ENABLE_UTD
    vint                tId;
    VTSP_UtdTemplate    utdConfig;

    infc = 2;

    OSAL_logMsg("UTD should detect BUSY/REORDER/RINGBACK\n\n\n");

    OSAL_logMsg("%s:%d FXO->Offhook\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_OFFHOOK);
    OSAL_taskDelay(500);   /* Application must wait for HOOK event */
    OSAL_logMsg("%s:%d FXO->UTD Detect ON\n", __FILE__, __LINE__);

    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, VTSP_DETECT_UTD);
    OSAL_taskDelay(3000);   /* Application must wait for HOOK event */

    OSAL_logMsg("%s:%d FXO->DIALING\n", __FILE__, __LINE__);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '0', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);

    OSAL_taskDelay(9000);   /* Application must wait for event */

    OSAL_logMsg("%s:%d FXO->ONHOOK\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(4000);   /* Application must wait for HOOK event */


    /* Now try, with mask modified to ignore all except DIALTONE */
    OSAL_logMsg("\n\n\n");
    OSAL_logMsg("UTD should detect DIALTONE only\n\n\n");
    utdConfig.control = VTSP_TEMPL_CONTROL_UTD_MASK;
    utdConfig.mask = VTSP_TEMPL_UTD_MASK_DIAL;
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, infc, &utdConfig);
    OSAL_logMsg("%s:%d FXO->UTD configured to mask 0x%x\n",
            __FILE__, __LINE__, utdConfig.mask, 0);


    OSAL_logMsg("%s:%d FXO->Offhook\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_OFFHOOK);

    OSAL_taskDelay(500);   /* Application must wait for HOOK event */
    OSAL_logMsg("%s:%d FXO->UTD Detect ON\n", __FILE__, __LINE__);

    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, VTSP_DETECT_UTD);
    OSAL_taskDelay(3000);   /* Application must wait for HOOK event */

    OSAL_logMsg("%s:%d FXO->DIALING\n", __FILE__, __LINE__);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '0', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);

    OSAL_taskDelay(9000);   /* Application must wait for event */

    OSAL_logMsg("%s:%d FXO->ONHOOK\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(4000);   /* Application must wait for HOOK event */


    /* Now try, with mask modified to detect all */
    OSAL_logMsg("\n\n\n");
    OSAL_logMsg("UTD should detect ALL\n\n\n");
    utdConfig.control = VTSP_TEMPL_CONTROL_UTD_MASK;
    utdConfig.mask = VTSP_TEMPL_UTD_MASK_ALL;
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, infc, &utdConfig);
    OSAL_logMsg("%s:%d FXO->UTD configured to mask 0x%x\n",
            __FILE__, __LINE__, utdConfig.mask);

    OSAL_logMsg("%s:%d FXO->Offhook\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_OFFHOOK);

    OSAL_taskDelay(500);   /* Application must wait for HOOK event */
    OSAL_logMsg("%s:%d FXO->UTD Detect ON\n", __FILE__, __LINE__);

    UT_EXPECT2(VTSP_OK, VTSP_detect, infc, VTSP_DETECT_UTD);
    OSAL_taskDelay(3000);   /* Application must wait for HOOK event */

    OSAL_logMsg("%s:%d FXO->DIALING\n", __FILE__, __LINE__);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '0', 20);
    UT_EXPECT3(UT_PASS, UT_playToneDigit, infc, '1', 20);

    OSAL_taskDelay(9000);   /* Application must wait for event */

    OSAL_logMsg("%s:%d FXO->ONHOOK\n", __FILE__, __LINE__);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(4000);   /* Application must wait for HOOK event */


    OSAL_logMsg("Reconfiguring UTD\n\n\n");

    /* Set a dummy tone that will "never detect" to fill templates */
    utdConfig.control        = VTSP_TEMPL_CONTROL_UTD_TONE;
    utdConfig.type           = VTSP_TEMPL_UTD_TONE_TYPE_UNK;
    utdConfig.dual.control   = VTSP_TEMPL_UTD_TONE_DUAL;
    utdConfig.dual.cadences  = 1;
    utdConfig.dual.freq1     = 1440;
    utdConfig.dual.freqDev1  = 11;
    utdConfig.dual.freq2     = 440;
    utdConfig.dual.freqDev2  = 11;
    utdConfig.dual.minMake1  = 5000;
    utdConfig.dual.maxMake1  = 0;
    utdConfig.dual.minBreak1 = 0;
    utdConfig.dual.maxBreak1 = 0;
    utdConfig.dual.minMake2  = 0;
    utdConfig.dual.maxMake2  = 0;
    utdConfig.dual.minMake3  = 0;
    utdConfig.dual.maxMake3  = 0;
    utdConfig.dual.minBreak3 = 0;
    utdConfig.dual.maxBreak3 = 0;
    utdConfig.dual.power     = -39;
    tId = 0;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 1;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 2;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 3;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 4;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 5;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 6;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 7;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 8;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
    tId = 9;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);

    utdConfig.control = VTSP_TEMPL_CONTROL_UTD_TONE;
    utdConfig.type = VTSP_TEMPL_UTD_TONE_TYPE_DIAL;
    utdConfig.dual.control = VTSP_TEMPL_UTD_TONE_DUAL;
    utdConfig.dual.cadences = 1;
    utdConfig.dual.freq1 = 350;
    utdConfig.dual.freqDev1 = 11;
    utdConfig.dual.freq2 = 440;
    utdConfig.dual.freqDev2 = 11;
    utdConfig.dual.minMake1 = 500;
    utdConfig.dual.maxMake1 = 0;
    utdConfig.dual.minBreak1 = 0;
    utdConfig.dual.maxBreak1 = 0;
    utdConfig.dual.minMake2 = 0;
    utdConfig.dual.maxMake2 = 0;
    utdConfig.dual.minMake3 = 0;
    utdConfig.dual.maxMake3 = 0;
    utdConfig.dual.minBreak3 = 0;
    utdConfig.dual.maxBreak3 = 0;
    utdConfig.dual.power = -39;
    tId = 10;
    OSAL_logMsg("Reconfiguring UTD tId %d\n", tId);
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_UTD, tId, &utdConfig);
#endif
    return (UT_PASS);
}
UT_Return _UT_configNTTTones(
    vint infc)
{
#ifdef VTSP_ENABLE_TONE_QUAD
    VTSP_ToneQuadTemplate   tQuad;
    vint tId;

    /*
     * DIAL TONE (DT)
     */
    tId = 10;
    tQuad.control = VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 440;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -22;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = -1;
    tQuad.break1           = 0;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * RING BACK TONE (RBT)
     */
    tId = 11;
    tQuad.control = VTSP_TEMPL_CONTROL_TONE_QUAD_MOD;
    tQuad.tone.modulate.carrier = 400;
    tQuad.tone.modulate.signal  = 17;
    tQuad.tone.modulate.power   = -29;
    tQuad.tone.modulate.index   = 85;
    tQuad.cadences              = 1;
    tQuad.make1                 = 1000;
    tQuad.break1                = 2000;
    tQuad.repeat1               = 0;
    tQuad.make2                 = 0;
    tQuad.break2                = 0;
    tQuad.repeat2               = 0;
    tQuad.make3                 = 0;
    tQuad.break3                = 0;
    tQuad.repeat3               = 0;
    tQuad.delta                 = 0;
    tQuad.decay                 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * BUSY TONE (BT)
     */
    tId = 12;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 440;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -29;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 500;
    tQuad.break1           = 500;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * PRIVATE DIAL TONE (PDT)
     */
    tId = 13;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -22;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 250;
    tQuad.break1           = 250;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * SECOND DIAL TONE (SDT)
     */
    tId = 14;
    tQuad.control = VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -22;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 130;
    tQuad.break1           = 120;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * ACCEPTANCE TONE (CPT)
     *   (or NOTIFICATION TONE (NFT))
     */
    tId = 15;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -26;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 2;
    tQuad.make1            = 125;
    tQuad.break1           = 125;
    tQuad.repeat1          = 2;
    tQuad.make2            = 0;
    tQuad.break2           = 500;
    tQuad.repeat2          = 1;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * HOLD SERVICE TONE (HST)
     * --------
     * TONE ID: 1 OF 2
     */
    tId = 16;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MOD;
    tQuad.tone.modulate.carrier = 400;
    tQuad.tone.modulate.signal  = 16;
    tQuad.tone.modulate.power   = -22;
    tQuad.tone.modulate.index   = 85;
    tQuad.cadences              = 1;
    tQuad.make1                 = 500;
    tQuad.break1                = 500;
    tQuad.repeat1               = 1;
    tQuad.make2                 = 0;
    tQuad.break2                = 0;
    tQuad.repeat2               = 0;
    tQuad.make3                 = 0;
    tQuad.break3                = 0;
    tQuad.repeat3               = 0;
    tQuad.delta                 = 0;
    tQuad.decay                 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* HST TONE ID: 2 OF 2 */
    tId = 17;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -22;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 500;
    tQuad.break1           = 2500;
    tQuad.repeat1          = 1;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * INCOMING IDENTIFICATION TONE (IIT)
     * --------
     * TONE ID: 1 OF 2
     */
    tId = 18;
    tQuad.control = VTSP_TEMPL_CONTROL_TONE_QUAD_MOD;
    tQuad.tone.modulate.carrier = 400;
    tQuad.tone.modulate.signal  = 16;
    tQuad.tone.modulate.power   = -25;
    tQuad.tone.modulate.index   = 85;
    tQuad.cadences              = 1;
    tQuad.make1                 = 500;
    tQuad.break1                = 1000;
    tQuad.repeat1               = 1;
    tQuad.make2                 = 0;
    tQuad.break2                = 0;
    tQuad.repeat2               = 0;
    tQuad.make3                 = 0;
    tQuad.break3                = 0;
    tQuad.repeat3               = 0;
    tQuad.delta                 = 0;
    tQuad.decay                 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* IIT TONE ID: 2 OF 2 */
    tId = 19;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -25;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 2;
    tQuad.make1            = 50;
    tQuad.break1           = 450;
    tQuad.repeat1          = 2;
    tQuad.make2            = 0;
    tQuad.break2           = 3000;
    tQuad.repeat2          = 1;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * SPECIFIC INCOMING IDENTIFICATION TONE (SIIT)
     * --------
     * TONE ID: 1 OF 2
     */
    tId = 20;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MOD;
    tQuad.tone.modulate.carrier = 400;
    tQuad.tone.modulate.signal  = 16;
    tQuad.tone.modulate.power   = -25;
    tQuad.tone.modulate.index   = 85;
    tQuad.cadences              = 1;
    tQuad.make1                 = 500;
    tQuad.break1                = 1000;
    tQuad.repeat1               = 1;
    tQuad.make2                 = 0;
    tQuad.break2                = 0;
    tQuad.repeat2               = 0;
    tQuad.make3                 = 0;
    tQuad.break3                = 0;
    tQuad.repeat3               = 0;
    tQuad.delta                 = 0;
    tQuad.decay                 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* SIIT TONE ID: 2 OF 2 */
    tId = 21;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 400;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -25;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 2;
    tQuad.make1            = 50;
    tQuad.break1           = 450;
    tQuad.repeat1          = 4;
    tQuad.make2            = 0;
    tQuad.break2           = 2000;
    tQuad.repeat2          = 1;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * HOWLER TONE (HOW)
     * --------
     * TONE ID: 1 OF 4
     */
    /* HOW TONE ID: 1 OF 4 */
    tId = 22;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 1600;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = 0;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 500;
    tQuad.break1           = 0;
    tQuad.repeat1          = 1;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* HOW TONE ID: 2 OF 4 */
    tId = 23;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 1000;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = 0;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 125;
    tQuad.break1           = 0;
    tQuad.repeat1          = 1;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* HOW TONE ID: 3 OF 4 */
    tId = 24;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 2000;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = 0;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 125;
    tQuad.break1           = 0;
    tQuad.repeat1          = 1;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
    /* HOW TONE ID: 4 OF 4 */
    tId = 25;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 2000;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = 0;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 125;
    tQuad.break1           = 2000;
    tQuad.repeat1          = 1;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * ROT
     */
    tId = 26;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 1000;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -27;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 2;
    tQuad.make1            = 200;
    tQuad.break1           = 200;
    tQuad.repeat1          = 1;
    tQuad.make2            = 200;
    tQuad.break2           = 400;
    tQuad.repeat2          = 1;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * ITC
     */
    tId = 27;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 800;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -27;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 150;
    tQuad.break1           = 150;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * PTC
     */
    tId = 28;
    tQuad.control =  VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 800;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -27;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 1000;
    tQuad.break1           = 0;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);

    /*
     * JP4
     */
    tId = 29;
    tQuad.control = VTSP_TEMPL_CONTROL_TONE_QUAD_MONO;
    tQuad.tone.quad.freq1  = 800;
    tQuad.tone.quad.freq2  = 0;
    tQuad.tone.quad.freq3  = 0;
    tQuad.tone.quad.freq4  = 0;
    tQuad.tone.quad.power1 = -27;
    tQuad.tone.quad.power2 = 0;
    tQuad.tone.quad.power3 = 0;
    tQuad.tone.quad.power4 = 0;
    tQuad.cadences         = 1;
    tQuad.make1            = 250;
    tQuad.break1           = 250;
    tQuad.repeat1          = 0;
    tQuad.make2            = 0;
    tQuad.break2           = 0;
    tQuad.repeat2          = 0;
    tQuad.make3            = 0;
    tQuad.break3           = 0;
    tQuad.repeat3          = 0;
    tQuad.delta            = 0;
    tQuad.decay            = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configToneQuad, tId, &tQuad);
#endif
    return(UT_PASS);
}

UT_Return UT_testToneQuad(
        vint infc)
{
    vint                    tId;
    uvint                   toneTable_ptr[30];
    vint                    tableLen;

    UT_testTimeSec = 30;

    for (tId = 1; tId < 9; tId ++) {
        OSAL_logMsg("%s: infc%d tQuadId%d\n", __FUNCTION__, infc, tId);
        OSAL_taskDelay(200);
        UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, tId, 1, 8000);
        OSAL_taskDelay(1000);
    }

    UT_testTimeSec = 30;

    _UT_configNTTTones(infc);
    /* Play NTT Tones */
    OSAL_logMsg("DIAL TONE (DT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 10, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("RING BACK TONE (RBT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 11, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("BUSY TONE (BT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 12, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("PRIVATE DIAL TONE (PDT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 13, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("SECOND DIAL TONE (SDT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 14, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("ACCEPTANCE TONE (CPT)\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 15, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("HOLD SERVICE TONE (HST) Sequence\n");
    OSAL_taskDelay(200);
    tableLen = 0;
    toneTable_ptr[tableLen++] = 16;
    toneTable_ptr[tableLen++] = 17;
    UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
            infc,
            toneTable_ptr,
            tableLen,           /* numToneIds */
            0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
            1);                 /* repeat */
    OSAL_taskDelay(6000);

    OSAL_logMsg("INCOMING IDENTIFICATION TONE (IIT) Sequence\n");
    OSAL_taskDelay(200);
    tableLen = 0;
    toneTable_ptr[tableLen++] = 18;
    toneTable_ptr[tableLen++] = 19;
    UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
            infc,
            toneTable_ptr,
            tableLen,           /* numToneIds */
            0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
            1);                 /* repeat */
    OSAL_taskDelay(6000);

    OSAL_logMsg("SPECIFIC INCOMING IDENTIFICATION TONE (SIIT) Sequence\n");
    OSAL_taskDelay(200);
    tableLen = 0;
    toneTable_ptr[tableLen++] = 20;
    toneTable_ptr[tableLen++] = 21;
    UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
            infc,
            toneTable_ptr,
            tableLen,           /* numToneIds */
            0,                  /* control, 0 or VTSP_TONE_BREAK_MIX */
            1);                 /* repeat */
    OSAL_taskDelay(6000);

    OSAL_logMsg("HOWLER TONE (HOW) Sequence\n");
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
    UT_EXPECT5(VTSP_OK, VTSP_toneQuadLocalSequence,
            infc,
            toneTable_ptr,
            tableLen,            /* numToneIds */
            VTSP_TONE_BREAK_MIX, /* control, 0 or VTSP_TONE_BREAK_MIX */
            1);                  /* repeat */
    OSAL_taskDelay(22000);

    OSAL_logMsg("ROT\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 26, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("ITC\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 27, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("PTC\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 28, 1, 8000);
    OSAL_taskDelay(6000);

    OSAL_logMsg("JP4\n");
    OSAL_taskDelay(200);
    UT_EXPECT4(VTSP_OK, VTSP_toneQuadLocal, infc, 29, 1, 8000);
    OSAL_taskDelay(6000);

    UT_EXPECT1(VTSP_OK, VTSP_toneQuadLocalStop, infc);
    return (UT_PASS);
}

/*
 * ======== UT_testFxoDial() ========
 */
UT_Return UT_testFxoDial(
        vint infc)
{
#ifdef VTSP_ENABLE_FXO
    char dialString_ptr[50];
    vint dialLen;
    vint toneId;
    vint index;

    infc = 2;
    UT_testTimeSec = 30;
    toneId = 15;

    OSAL_logMsg("%s:  Enter infc%d digit string (digits only): ",
            __FUNCTION__, infc);
    D2_getLine(dialString_ptr, sizeof(dialString_ptr));

    dialLen = OSAL_strlen(dialString_ptr);
    if (dialLen < 1) {
        return (UT_PASS);
    }

    OSAL_logMsg("%s: infc%d VTSP_INFC_OFFHOOK\n",
            __FUNCTION__, infc);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_OFFHOOK);
    OSAL_taskDelay(2000);

    for (index = 0; index < dialLen; index++) {
        /*
         * Config and Generate the DTMF tone to dial.
         * Blocks until tone done
         */
        UT_EXPECT3(UT_PASS, UT_playToneDigit,
                infc, dialString_ptr[index], toneId);
    }

    OSAL_taskDelay(5000);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, 9, 1, 10000);

    while (UT_testTimeSec) {
        OSAL_taskDelay(5000);
        UT_testTimeSec -= 5;

        OSAL_logMsg("%s: infc%d VTSP_INFC_FLASH\n",
                __FUNCTION__, infc);
        UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc,
                VTSP_CONTROL_INFC_FLASH);
    }

    OSAL_taskDelay(2000);
    OSAL_logMsg("%s: infc%d VTSP_INFC_ONHOOK\n",
            __FUNCTION__, infc);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_ONHOOK);
#endif
    return (UT_PASS);
}
#ifdef VTSP_ENABLE_PULSE_GENERATE
/*
 * ======== UT_testFxoPulseDial() ========
 */
UT_Return UT_testFxoPulseDial(
        vint infc)
{
#define _UT_DIALSTR_SZ    (35)
    char dialString_ptr[_UT_DIALSTR_SZ];
    uint8 dialNum;
    vint dialLen;
    vint index;

    infc = 2;

    OSAL_logMsg("%s:  Enter infc%d digit string (digits only): ",
            __FUNCTION__, infc);
    D2_getLine(dialString_ptr, sizeof(dialString_ptr));

    dialLen = OSAL_strlen(dialString_ptr);
    if (dialLen < 1) {
        return (UT_PASS);
    }

    /*
     * Convert the dial digit(ASCII) to pulse number
     */
    for (index=0; index < _UT_DIALSTR_SZ; index++) {
        dialNum = (uint8)(dialString_ptr[index] - '0');
        if ('*' == dialString_ptr[index]) {
            /*
             * Pause code:
             * 0xD0: The Pause code in _tic
             * 0x02: number * _TIC_PULSE_PAUSE_TIME
             */
            dialString_ptr[index] = 0xD0 | 0x02;
        }
        else if (0 == dialString_ptr[index]) {
            break;
        }
        else if ( 0 == dialNum ) {
            dialString_ptr[index] = 10;
        }
        else if ((dialNum > 0) && (dialNum <= 9)) {
            dialString_ptr[index] = dialNum;
        }
        else {
            OSAL_logMsg("%s %d: The dial string is incorrect \n",
                    __FUNCTION__, __LINE__);
            return (UT_FAIL);
        }
    }
    OSAL_logMsg("%s: infc%d VTSP_INFC_OFFHOOK\n", __FUNCTION__, infc);

    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_OFFHOOK);
    OSAL_taskDelay(1000);

    UT_EXPECT3(VTSP_OK, VTSP_infcControlIO, infc, VTSP_CONTROL_INFC_IO_PULSE_GENERATE, (int32)dialString_ptr);
    OSAL_taskDelay(30000);

    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(1000);
}
#endif

/*
 * ======== UT_streamFxoDial() ========
 */
UT_Return UT_streamFxoDial(
        vint infc)
{
#ifdef VTSP_ENABLE_FXO
    VTSP_CIDData    cidMsg;
    VTSP_Stream    *stream1_ptr;
    VTSP_Stream    *stream2_ptr;
    char            dialString_ptr[50];
    char            buf[10];
    uvint           infc1;
    uvint           infc2;
    vint            streamId1;
    vint            streamId2;
    vint            dialLen;
    vint            toneId;
    vint            index;

    infc1 = 1;
    infc2 = 2;
    streamId1 = 0;
    streamId2 = 0;
    toneId = 15;

    OSAL_logMsg("%s:  "
            "This test opens a vTSP stream between infc %d <-> infc %d\n",
            __FUNCTION__, infc1, infc2);

    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = UT_getUserTestParam(UT_SILCOMP_MASK);
    UT_dtmfRelay = UT_getUserTestParam(UT_DTMFR_ENCODE_MASK);
    UT_extension = 1;
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    OSAL_logMsg("%s:  Enter infc%d digit string (digits only): ",
            __FUNCTION__, infc2);
    D2_getLine(dialString_ptr, sizeof(dialString_ptr));

    dialLen = OSAL_strlen(dialString_ptr);
    if (dialLen < 1) {
        return (UT_PASS);
    }

    /*
     * ONHOOK, OFFHOOK, then Dial on infc2
     */
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(2000);
    OSAL_logMsg("%s: infc%d VTSP_INFC_OFFHOOK\n",
            __FUNCTION__, infc2);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_OFFHOOK);
    OSAL_taskDelay(300);    /* Application must wait for HOOK event */
    UT_EXPECT2(VTSP_OK, VTSP_detect, infc2, VTSP_DETECT_UTD);
    OSAL_taskDelay(3000);

    for (index = 0; index < dialLen; index++) {
        /*
         * Config and Generate the DTMF tone to dial.
         * Blocks until tone done
         */
        UT_EXPECT3(UT_PASS, UT_playToneDigit,
                infc2, dialString_ptr[index], toneId);
    }

    /*
     * Ring on infc1
     */
    UT_updateCid(&cidMsg);
    UT_printCid(&cidMsg);
    OSAL_taskDelay(1000);
    UT_EXPECT5(VTSP_OK, VTSP_ring,
            infc1,
            1, /* ring template */
            2, /* # rings */
            VTSP_RING_TMAX, /* ms timeout */
            &cidMsg);

    OSAL_taskDelay(5000);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc2, 9, 1, 10000);
    OSAL_taskDelay(2000);

    /*
     * Start the voice stream
     */
    /* infc1 -send-> infc2  and infc1 <-recv- infc2 */
    OSAL_strcpy(buf, "127.0.0.1");
    OSAL_netStringToAddress(buf, (struct in_addr *)&UT_sendAddr);
    UT_recvAddr = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    UT_sendPort = 5000 + infc2;
    UT_recvPort = 5000 + infc1;
    stream1_ptr = UT_updateStreamData(infc1, streamId1);
    OSAL_logMsg("%s: Starting infc %d streamId %d..\n", __FILE__,
            infc1, stream1_ptr->streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc1, stream1_ptr);

    /* infc1 -send-> infc0  and infc1 <-recv- infc0 */
    OSAL_strcpy(buf, "127.0.0.1");
    OSAL_netStringToAddress(buf, (struct in_addr *)&UT_sendAddr);
    UT_recvAddr = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    UT_sendPort = 5000 + infc1;
    UT_recvPort = 5000 + infc2;
    stream2_ptr = UT_updateStreamData(infc2, streamId2);
    OSAL_logMsg("%s: Starting infc %d streamId %d..\n", __FILE__,
            infc2, stream2_ptr->streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc2, stream2_ptr);

    if (UT_testTimeSec < 0) {
        /* Leave stream running, return to caller */
        return (UT_PASS);
    }

    /* Turn on event printing
     */
    OSAL_logMsg("%s: Event printing ON.\n", __FILE__);
    UT_eventTaskDisplay = 1;
    UT_run = 1;

    while (UT_run) {
        OSAL_taskDelay(1000);
        if (UT_testTimeSec != 0) {
            UT_testTimeSec -= 1;
            if (UT_testTimeSec <= 0) {
                break;
            }
        }
    }

    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(2000);

    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc1, stream1_ptr->streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc2, stream2_ptr->streamId);
#endif /* VTSP_ENABLE_FXO */
    return (UT_PASS);
}

/*
 * ======== UT_streamFxoIn() ========
 */
UT_Return UT_streamFxoIn(
        vint infc)
{
#ifdef VTSP_ENABLE_FXO
    VTSP_Stream    *stream1_ptr;
    VTSP_Stream    *stream2_ptr;
    char            buf[10];
    uvint           infc1;
    uvint           infc2;
    vint            streamId1;
    vint            streamId2;
    vint            fxoStream;

    infc1 = 0;
    infc2 = 2;
    streamId1 = 0;
    streamId2 = 0;

    /*
     * Optionally, this test can stream between infc1 and infc2
     * Currently, disabled.
     */
    fxoStream = 1;

    /*
     * Get test parameters from the user
     */
    OSAL_logMsg("%s:  "
            "This test opens a vTSP stream between infc %d <-> infc %d\n",
            __FUNCTION__, infc1, infc2);

    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = UT_getUserTestParam(UT_SILCOMP_MASK);
    UT_dtmfRelay = UT_getUserTestParam(UT_DTMFR_ENCODE_MASK);
    UT_extension = 1;
    OSAL_logMsg("%s: Event printing ON.\n", __FILE__);
    UT_eventTaskDisplay = 1;
    /*
     * FXO go ONHOOK on infc2, and set up the stream
     */
    OSAL_logMsg("%s: infc%d VTSP_INFC_ONHOOK\n",
            __FUNCTION__, infc2);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(500);
    UT_EXPECT2(VTSP_OK, VTSP_detect, infc2, VTSP_DETECT_CALLERID_ONHOOK);
    OSAL_logMsg("%s: infc%d Please dial in within the next 10 seconds\n",
            __FUNCTION__, infc2);
    OSAL_taskDelay(1000);

    /*
     * Start the voice stream
     */
    /* infc1 -send-> infc2  and infc1 <-recv- infc2 */
    if (fxoStream) {
        OSAL_strcpy(buf, "127.0.0.1");
        OSAL_netStringToAddress(buf, (struct in_addr *)&UT_sendAddr);
        UT_recvAddr = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
        UT_sendPort = 5000 + infc2;
        UT_recvPort = 5000 + infc1;
        stream1_ptr = UT_updateStreamData(infc1, streamId1);
        OSAL_logMsg("%s: Starting infc %d streamId %d..\n", __FILE__,
                infc1, stream1_ptr->streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc1, stream1_ptr);

        /* infc1 -send-> infc0  and infc1 <-recv- infc0 */
        OSAL_strcpy(buf, "127.0.0.1");
        OSAL_netStringToAddress(buf, (struct in_addr *)&UT_sendAddr);
        UT_recvAddr = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
        UT_sendPort = 5000 + infc1;
        UT_recvPort = 5000 + infc2;
        stream2_ptr = UT_updateStreamData(infc2, streamId2);
        OSAL_logMsg("%s: Starting infc %d streamId %d..\n", __FILE__,
                infc2, stream2_ptr->streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc2, stream2_ptr);
    }

    /* Wait for incoming call */
    OSAL_taskDelay(20000);

    /* Go OFFHOOK */
    OSAL_logMsg("%s: infc%d VTSP_INFC_OFFHOOK\n",
            __FUNCTION__, infc2);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_OFFHOOK);
    OSAL_taskDelay(2000);                /* Need time to wait for SEIZE event */
    UT_EXPECT2(VTSP_OK, VTSP_detect,
            infc2, VTSP_DETECT_CALLERID_OFFHOOK);
    OSAL_taskDelay(60000);

    /* Go ONHOOK to finish */
    OSAL_logMsg("%s: infc%d VTSP_INFC_ONHOOK\n",
            __FUNCTION__, infc2);
    UT_EXPECT2(VTSP_OK, VTSP_infcControlLine, infc2, VTSP_CONTROL_INFC_ONHOOK);
    OSAL_taskDelay(500);

    if (fxoStream) {
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc1, stream1_ptr->streamId);
        UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc2, stream2_ptr->streamId);
    }
#endif
    return (UT_PASS);
}

/*
 * ======== UT_testControlHook() ========
 */
UT_Return UT_testControlHook(
        vint infc)
{
#ifdef VTSP_ENABLE_FXO
    vint control;
    vint word;
    char buf[10];

    OSAL_logMsg("\n\n%s: Beginning Hook Control test.\n", __FILE__);

    infc = 2;
    OSAL_logMsg("%s:  Interface [%d] : ",
            __FUNCTION__, infc, 0, 0);
    D2_getLine(buf, sizeof(buf));
    word = OSAL_atoi(buf);
    infc = word;

    OSAL_logMsg("%s:\n"
            " VTSP_INFC_POWER_DOWN = 4\n"
            " VTSP_INFC_POLARITY_FWD = 5\n"
            " VTSP_INFC_POLARITY_REV = 6\n",
            __FUNCTION__);

    OSAL_logMsg(
            " VTSP_INFC_ONHOOK = 7\n"
            " VTSP_INFC_OFFHOOK = 8\n"
            " VTSP_INFC_FLASH = 9\n");

    for (control = 0; control != -1; ) {
        OSAL_logMsg("%s:  Enter infc%d control enum [-1 = end]: ",
                __FUNCTION__, infc);
        D2_getLine(buf, 20);
        control = OSAL_atoi(buf);

        if (-1 == control) {
            break;
        }
        UT_EXPECT2(VTSP_OK, VTSP_infcControlLine,
                infc, control);
        OSAL_taskDelay(200);
    }
#endif /* VTSP_ENABLE_FXO */
    return (UT_PASS);
}

/*
 * Update global stream structure
 * from global variables
 */
VTSP_StreamVideo *UT_updateStreamDataVideo(
        vint infc,
        vint streamId)
{
    VTSP_StreamVideo   *stream_ptr;
    vint                offset;
    vint                cnt;
    int8                ipStr[OSAL_NET_IPV6_STR_MAX];

    offset = 0;
    /* Default to streamIndex = 0 */
    stream_ptr = &UT_vtspStreamv0;
#if defined(OSAL_THREADX)
    UT_encoder = VTSP_CODER_VIDEO_H264;
#else
    UT_encoder =  UT_getUserTestParam(UT_ENCODER_VIDEO);
#endif
    OSAL_netAddrCpy(&stream_ptr->remoteAddr, &UT_sendAddr);
    stream_ptr->remoteAddr.port   = OSAL_netHtons((UT_sendPort + offset));
    stream_ptr->remoteControlPort = OSAL_netHtons((UT_sendPort + 100));
    OSAL_netAddrCpy(&stream_ptr->localAddr, &UT_recvAddr);
    stream_ptr->localAddr.port    = OSAL_netHtons((UT_recvPort + offset));
    stream_ptr->localControlPort  = OSAL_netHtons((UT_recvPort + 100));

    stream_ptr->encoder = UT_encoder;

    OSAL_logMsg("%s:%d infc%d streamId %08x ----- \n",
            __FILE__, __LINE__, infc, stream_ptr->streamId);
    OSAL_netAddressToString(ipStr, &stream_ptr->remoteAddr);
    OSAL_logMsg("%s:%d remoteIpAddr %s\n", __FILE__, __LINE__, ipStr);
    OSAL_logMsg("%s:%d remoteDataPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteAddr.port));
    OSAL_logMsg("%s:%d remoteControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteControlPort));
    OSAL_netAddressToString(ipStr, &stream_ptr->localAddr);
    OSAL_logMsg("%s:%d localIpAddr %s\n", __FILE__, __LINE__, ipStr);
    OSAL_logMsg("%s:%d localDataPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localAddr.port));
    OSAL_logMsg("%s:%d localControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localControlPort));
    OSAL_logMsg("%s:%d encoder %d\n",
            __FILE__, __LINE__, stream_ptr->encoder);
    /*
     * Bug 3197
     * Clear the srtp keys to use the NULL encoding algorithm by default
     */
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;
    for (cnt = 0; cnt < VTSP_SRTP_KEY_STRING_MAX_LEN; cnt++) {
        stream_ptr->srtpSendKey[cnt] = 0;
        stream_ptr->srtpRecvKey[cnt] = 0;
    }

    OSAL_taskDelay(200);   /* Delay so printfs do not disrupt voice */

    return (stream_ptr);
}

/*
 * Update global stream structure
 * from global variables
 */
VTSP_Stream *UT_updateStreamData(
        vint infc,
        vint streamId)
{
    VTSP_Stream *stream_ptr;
    vint         streamIndex;
    vint         offset;
    vint         cnt;
    int8         ipStr[OSAL_NET_IPV6_STR_MAX];

    /* XXX: should use VTSP_query to find #streams/infc here.
     */
    streamIndex  = (infc * 2) + streamId;

    offset = 0;
    /* Default to streamIndex = 0 */
    stream_ptr = &UT_vtspStream0;
    if (streamIndex == 1) {
        stream_ptr = &UT_vtspStream1;
    }
    else if (streamIndex == 2) {
        stream_ptr = &UT_vtspStream2;
    }
    else if (streamIndex == 3) {
        stream_ptr = &UT_vtspStream3;
    }

    OSAL_netAddrCpy(&stream_ptr->remoteAddr, &UT_sendAddr);
    stream_ptr->remoteAddr.port   = OSAL_netHtons((UT_sendPort + offset));
    stream_ptr->remoteControlPort = OSAL_netHtons((UT_sendPort + 100));
    OSAL_netAddrCpy(&stream_ptr->localAddr, &UT_recvAddr);
    stream_ptr->localAddr.port    = OSAL_netHtons((UT_recvPort + offset));
    stream_ptr->localControlPort  = OSAL_netHtons((UT_recvPort + 100));

    stream_ptr->encoder = UT_encoder;
    stream_ptr->silenceComp = UT_silComp;
    stream_ptr->dtmfRelay = UT_dtmfRelay;
    stream_ptr->extension = UT_extension;
    stream_ptr->confMask = UT_confMask;
    stream_ptr->encodeTime[VTSP_CODER_G711U] = UT_encodeTime;
    stream_ptr->encodeTime[VTSP_CODER_G711A] = UT_encodeTime;
#ifdef VTSP_ENABLE_G729
    stream_ptr->encodeTime[VTSP_CODER_G729] = UT_encodeTime;
#endif
#ifdef VTSP_ENABLE_G726
    stream_ptr->encodeTime[VTSP_CODER_G726_32K] = UT_encodeTime;
#endif
#ifdef VTSP_ENABLE_G723
    stream_ptr->encodeTime[VTSP_CODER_G723_30MS] = 30;
#endif
#ifdef VTSP_ENABLE_G722P1
    stream_ptr->encodeTime[VTSP_CODER_G722P1_20MS] = 20;
#endif
#ifdef VTSP_ENABLE_G722
    stream_ptr->encodeTime[VTSP_CODER_G722] = UT_encodeTime;
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
    stream_ptr->encodeTime[VTSP_CODER_GAMRNB_20MS_OA] = 20;
    stream_ptr->encodeTime[VTSP_CODER_GAMRNB_20MS_BE] = 20;
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
    stream_ptr->encodeTime[VTSP_CODER_GAMRWB_20MS_OA] = 20;
    stream_ptr->encodeTime[VTSP_CODER_GAMRWB_20MS_BE] = 20;
#endif
#ifdef VTSP_ENABLE_SILK
    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_8K] = 20;

    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_16K] = 20;

    stream_ptr->encodeTime[VTSP_CODER_SILK_20MS_24K] = 20;
#endif
#ifdef VTSP_ENABLE_G711P1
    stream_ptr->encodeTime[VTSP_CODER_G711P1U] = UT_encodeTime;
    stream_ptr->encodeTime[VTSP_CODER_G711P1A] = UT_encodeTime;
#endif
    stream_ptr->encodeTime[VTSP_CODER_DTMF] = UT_encodeTime;
    stream_ptr->encodeTime[VTSP_CODER_CN] = UT_encodeTime_cn;

    OSAL_logMsg("%s:%d infc%d streamId %08x ----- \n",
            __FILE__, __LINE__, infc, stream_ptr->streamId);
    OSAL_netAddressToString(ipStr, &stream_ptr->remoteAddr);
    OSAL_logMsg("%s:%d remoteIpAddr %s\n", __FILE__, __LINE__, ipStr);
    OSAL_logMsg("%s:%d remoteAddr.port %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteAddr.port));
    OSAL_logMsg("%s:%d remoteControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->remoteControlPort));
    OSAL_netAddressToString(ipStr, &stream_ptr->localAddr);
    OSAL_logMsg("%s:%d localAddr.ipv4 %s\n", __FILE__, __LINE__, ipStr);
    OSAL_logMsg("%s:%d localAddr.port %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localAddr.port));
    OSAL_logMsg("%s:%d localControlPort %d\n",
            __FILE__, __LINE__, OSAL_netNtohs(stream_ptr->localControlPort));
    OSAL_logMsg("%s:%d encoder %d\n",
            __FILE__, __LINE__, stream_ptr->encoder);
    OSAL_logMsg("%s:%d encodeTime %d\n",
            __FILE__, __LINE__, stream_ptr->encodeTime[stream_ptr->encoder]);
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
    /*
     * Bug 3197
     * Clear the srtp keys to use the NULL encoding algorithm by default
     */
    stream_ptr->srtpSecurityType = VTSP_SRTP_SECURITY_SERVICE_NONE;
    for (cnt = 0; cnt < VTSP_SRTP_KEY_STRING_MAX_LEN; cnt++) {
        stream_ptr->srtpSendKey[cnt] = 0;
        stream_ptr->srtpRecvKey[cnt] = 0;
    }

    OSAL_taskDelay(200);   /* Delay so printfs do not disrupt voice */

    return (stream_ptr);
}

static vint UT_dtmfTable[16][2] =
{
    /* 0 */ {941, 1336},
    /* 1 */ {697, 1209},
    /* 2 */ {697, 1336},
    /* 3 */ {697, 1477},
    /* 4 */ {770, 1209},
    /* 5 */ {770, 1336},
    /* 6 */ {770, 1477},
    /* 7 */ {852, 1209},
    /* 8 */ {852, 1336},
    /* 9 */ {852, 1477},
    /* *(10) */ {941, 1209},
    /* #(11) */ {941, 1336}
};

/*
 * Display menu option and accept user input
 */
uint32 UT_getUserTestParam(UT_TestOption opt)
{
    char            buf[128];
    vint            word;
    vint            getWord;
    OSAL_NetAddress sendAddr;

    word = 0;
    getWord = 1;

    switch(opt) {
        case UT_INFC:
            OSAL_logMsg("%s:  Telephony Infc (0=default, 0 - n) : ", __FILE__);
            break;
        case UT_STREAM_ID:
            OSAL_logMsg("%s:  StreamId (0=default, 0 - 1) : ", __FILE__);
            break;
        case UT_IP_ADDRESS:
            getWord = 0;
            OSAL_logMsg("%s:  Enter IP Addr of peer (172.16.0.23) : ",
                    __FILE__);
            D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
            if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL)
            {
                OSAL_netStringToAddress("172.16.0.23", &sendAddr);
                OSAL_logMsg("%s: Using default address 172.16.0.23\n",__FILE__);
            }
            OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
            /* config receive address as ANY */
            UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
            OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
            UT_recvAddr.type = UT_sendAddr.type;
            break;
        case UT_IP_PORT:
            getWord = 0;
            OSAL_logMsg("%s:  Enter IP Port of peer @ %s (%d-%d) : ",
                    __FILE__, buf, UT_startPort, UT_endPort);
            D2_getLine(buf, 10);
            word = OSAL_atoi(buf);
            if (0 == word) {
                word = UT_startPort;
            }
            break;
        case UT_SILCOMP_MASK:
            OSAL_logMsg("%s: Silence Compression mask (0=no CN/VAD, >0=all): ",
                    __FILE__);
            getWord = 0;
            D2_getLine(buf, 5);
            word = OSAL_atoi(buf);
            if (word) {
                word = 0xffffffff;
            }
            break;
        case UT_DTMFR_ENCODE_MASK:
            OSAL_logMsg("%s:  DTMF Relay encode mask (0=no DTMFR, 1=all): ",
                    __FILE__);
            getWord = 0;
            D2_getLine(buf, 5);
            word = OSAL_atoi(buf);
            if (word) {
                word = 0xffffffff;
            }
            break;
        case UT_EXTENSION_WORD:
            getWord = 0;
            if (VTSP_CODER_G723_30MS == UT_encoder) {
                OSAL_logMsg("%s: 'extension' word (0=G723 6.3K, 2=G723 5.3K): ",
                        __FILE__);
            }
            else if(VTSP_CODER_G722P1_20MS == UT_encoder) {
                OSAL_logMsg("%s:'extension' word (0=G722P1 24K, 3=G722P1 32K):",
                        __FILE__);
            }
            else if ((VTSP_CODER_GAMRNB_20MS_OA == UT_encoder) ||
                    (VTSP_CODER_GAMRNB_20MS_BE == UT_encoder)) {
                OSAL_logMsg("%s:'extension' word:\n"
                        "0 = GAMRNB_MR475(default)\n"
                        "1 = GAMRNB_MR515\n"
                        "2 = GAMRNB_MR59\n"
                        "3 = GAMRNB_MR67\n"
                        "4 = GAMRNB_MR74\n"
                        "5 = GAMRNB_MR795\n"
                        "6 = GAMRNB_MR102\n"
                        "7 = GAMRNB_MR122\n"
                        ,__FILE__);
            }
            else if ((VTSP_CODER_GAMRWB_20MS_OA == UT_encoder) ||
                    (VTSP_CODER_GAMRWB_20MS_BE == UT_encoder)) {
                OSAL_logMsg("%s:'extension' word:\n"
                        "0 = GAMRWB_MR660(default)\n"
                        "1 = GAMRWB_MR885\n"
                        "2 = GAMRWB_MR1265\n"
                        "3 = GAMRWB_MR1425\n"
                        "4 = GAMRWB_MR1585\n"
                        "5 = GAMRWB_MR1825\n"
                        "6 = GAMRWB_MR1985\n"
                        "7 = GAMRWB_MR2305\n"
                        "8 = GAMRWB_MR2385\n"
                        ,__FILE__);
            }
            else {
                OSAL_logMsg("%s: 'extension' word (0=normal,1=fax clear chan):",
                __FILE__);
            }
            D2_getLine(buf, 5);
            word = OSAL_atoi(buf);
            if ((VTSP_CODER_G723_30MS == UT_encoder) && (2 == word)) {
                word = VTSP_MASK_EXT_G723_53;
            }
            if ((VTSP_CODER_G722P1_20MS == UT_encoder) && (3 == word)) {
                word = VTSP_MASK_EXT_G722P1_32;
            }
            if ((VTSP_CODER_GAMRNB_20MS_OA == UT_encoder) ||
                    (VTSP_CODER_GAMRNB_20MS_BE == UT_encoder)) {
                word = 1 << (16 + word);
            }
            if ((VTSP_CODER_GAMRWB_20MS_OA == UT_encoder) ||
                    (VTSP_CODER_GAMRWB_20MS_BE == UT_encoder)) {
                word = 1 << (16 + word);
            }

            break;
        case UT_CONFERENCE_MASK:
                OSAL_logMsg("%s:  'confMask' word (0=no mix, 1=encode mix): ",
                    __FILE__);
            break;
        case UT_CODER_ENCODE_TIME:
                OSAL_logMsg("%s:Encode Time for coder (10 / 20 / 30 / 40 ms): ",
                        __FILE__);
            break;
        case UT_CN_ENCODE_TIME:
            OSAL_logMsg("%s:  Encode Time for CN (10 ~ 100000 ms): ", __FILE__);
            break;
        case UT_ECHO_CANCELLER:
            OSAL_logMsg("%s:  Echo canceller (1=enable, 0=disable): ",
                    __FILE__);
        case UT_EC_BYPASS:
            OSAL_logMsg("%s:  Run EC Bypass test? (0=no, 1=yes): ", __FILE__);
            break;
        case UT_STREAM_DIRECTION:
            OSAL_logMsg("%s:  Stream direction test (1=enable, 0=disable): ",
                    __FILE__);
            break;
        case UT_TEST_LENGTH:
            OSAL_logMsg("\n\n%s:Length of test(sec)(0=forever, -1=background):",
                    __FILE__);
            break;
        case UT_TEST_VIDEO_KEYINT:
            OSAL_logMsg("\n\n%s:interval of key frames(sec)(auto <= 0):",
                    __FILE__);
            break;
        case UT_FMTD_GBL_PWR_INFC:
        case UT_FMTD_GBL_PWR_PEER:
            getWord = 0;
            if (UT_FMTD_GBL_PWR_PEER == opt) {
                OSAL_logMsg("%s:  FMTD Min Power PEER ( -33 to -53 dB [-33]): ",                        __FILE__);
            }
            else {
                OSAL_logMsg("%s:  FMTD Min Power INFC ( -33 to -53 dB [-33]): ",                        __FILE__);
            }
            D2_getLine(buf, 5);
            word = OSAL_atoi(buf);
            if (-33 < word) {
                word = -33;
            }
            else if (-53 > word)  {
                word = -53;
            }
            OSAL_logMsg("%s: Using level: %d\n", __FILE__, word);
            break;
        case UT_TONE_POWER:
             OSAL_logMsg("%s: Test Tone power (in 0.5dB): ", __FILE__);
            break;

        case UT_TONE_FREQ:
             OSAL_logMsg("%s: Test Tone frequency (in Hz): ", __FILE__);
        case UT_RTP_REDUN_LEVEL:
             OSAL_logMsg("%s: RFC2198 RTP Redundant Level (0 = disable): ",
                    __FILE__);
            break;
        case UT_RTP_REDUN_HOP:
             OSAL_logMsg("%s: RFC2198 RTP Delayed Hop: ", __FILE__);
            break;
        case UT_JB_FIXED_LEVEL:
             OSAL_logMsg("%s: JB Fixed Level: ", __FILE__);
            break;
        case UT_ENCODER_VIDEO:
            OSAL_logMsg("\n%s:  Enter encoder to use.\n"
                    "#define VTSP_CODER_H264            (0)\n"
                    "#define VTSP_CODER_H263            (1)\n"
                    "Enter encoder # (0 - 1): ",
                    __FILE__);
            break;

        case UT_ENCODER:
            OSAL_logMsg("%s:  Enter encoder to use.\n"
                    "#define VTSP_CODER_G711U           (0)\n"
                    "#define VTSP_CODER_G711A           (1)\n",
                    __FILE__);
            OSAL_logMsg(
                    "#define VTSP_CODER_CN              (2)\n"
#ifdef VTSP_ENABLE_G729
                    "#define VTSP_CODER_G729            (3)\n"
#endif
#ifdef VTSP_ENABLE_G726
                    "#define VTSP_CODER_G726_32K        (4)\n"
#endif
                    );
            OSAL_logMsg("\n"
#ifdef VTSP_ENABLE_ILBC
                    "#define VTSP_CODER_ILBC_20MS       (5)\n"
#endif
#ifdef VTSP_ENABLE_ILBC
                    "#define VTSP_CODER_ILBC_30MS       (6)\n"
#endif
#ifdef VTSP_ENABLE_G723
                    "#define VTSP_CODER_G723_30MS       (7)\n"
#endif
#ifdef VTSP_ENABLE_16K_MU
                    "#define VTSP_CODER_16K_MU          (10)\n"
#endif

#ifdef VTSP_ENABLE_G722
                    "#define VTSP_CODER_G722            (11)\n"
#endif
#ifdef VTSP_ENABLE_G722P1
                    "#define VTSP_CODER_G722P1_20MS     (12)\n\n"
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
                    "#define VTSP_CODER_GAMRNB_20MS_OA  (14)\n"
                    "#define VTSP_CODER_GAMRNB_20MS_BE  (15)\n"
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
                    "#define VTSP_CODER_GAMRWB_20MS_OA  (16)\n"
                    "#define VTSP_CODER_GAMRWB_20MS_BE  (17)\n"
#endif
#ifdef VTSP_ENABLE_G711P1
                    "#define VTSP_CODER_G711P1U         (18)\n\n"
#endif
#ifdef VTSP_ENABLE_G711P1
                    "#define VTSP_CODER_G711P1A         (19)\n\n"
#endif
#ifdef VTSP_ENABLE_SILK
                    "#define VTSP_CODER_SILK_20MS_8K    (20)\n\n"
                    "#define VTSP_CODER_SILK_20MS_16K   (21)\n\n"
                    "#define VTSP_CODER_SILK_20MS_24K   (22)\n\n"
#endif
                    "Enter encoder # (0 - 20): ");
            break;
        case UT_CONTROL_IO:
            OSAL_logMsg("Select an IO control option.\n"
                    "Audio out to Speaker             (0)\n"
                    "Audio out to Handset             (1)\n"
                    "Audio out to Headset             (2)\n"
                    "Audio in from Handset Mic        (3)\n"
                    "Audio in from Headset Mic        (4)\n"
                    "Mute Input    (H/W)              (5)\n"
                    "Unmute Input  (H/W)              (6)\n"
                    "Mute Output   (H/W)              (7)\n"
                    "Unmute Output (H/W)              (8)\n"
                    "Mute Input    (S/W)              (9)\n"
                    "Unmute Input  (S/W)             (10)\n"
                    "Voice Driver Enable             (11)\n"
                    "Voice Driver Disable            (12)\n"
                    "Gain Input / Output (Software)  (13)\n");
            break;
        case UT_CONTROL_AEC:
            OSAL_logMsg("Select an AEC control option.\n"
                    "Normal AEC                       (0)\n"
                    "Bypass AEC                       (1)\n"
                    "Bypass AGC                       (2)\n"
                    "Handset Mode                     (3)\n"
                    "Handsfree Mode                   (4)\n"
                    "Set Tail Length                  (5)\n"
                    "Freeze AEC                       (6)\n"
                    "Mix CN to Sout during FAR_ACTIVE (7)\n"
                    "Half-Duplex Mode                 (8)\n");
            break;
        case UT_GAIN:
            OSAL_logMsg("Select an integer gain from -40dB to +40dB.\n");
            break;
        case UT_AEC_TAIL_LEN:
            OSAL_logMsg("Select a multiple of 8 from 8ms to 128ms.\n");
            break;
        case UT_JB_MODE:
            OSAL_logMsg("Select an JB control option.\n"
                    "VTSP_TEMPL_CONTROL_JB_VOICE        (0)\n"
                    "VTSP_TEMPL_CONTROL_JB_FIXED        (1)\n"
                    "VTSP_TEMPL_CONTROL_JB_FREEZE       (2)\n");
            break;
        case UT_JB_INIT_LEVEL:
            OSAL_logMsg("Select a JB initLevel from 0ms to 700ms.\n");
            break;
        case UT_JB_MAX_LEVEL:
            OSAL_logMsg("Select a JB maxLevel from 0ms to 700ms.\n");
            break;
        case UT_CN_PWR_ATTEN:
            OSAL_logMsg("%s: CN Power Attenuation (0 to -35 dB): ", __FILE__);
            break;
        case UT_DTMFR_RATE:
            getWord = 0;
            OSAL_logMsg("%s: Enter DTMFR Bit Rate\n"
                    "    0 = telephone-event/8000\n"
                    "    1 = telephone-event/16000\n", __FILE__);
            D2_getLine(buf, 5);
            word = OSAL_atoi(buf);
            if (1 == word) {
                UT_extension |= VTSP_MASK_EXT_DTMFR_16K;
            }
            else {
                UT_extension |= VTSP_MASK_EXT_DTMFR_8K;
            }
            break;
        default:;
    }
    if (getWord) {
        D2_getLine(buf, 5);
        word = OSAL_atoi(buf);
    }
    return (word);
}

/*
 * ======== UT_playToneDigit() ========
 */
UT_Return UT_playToneDigit(
    vint infc,
    char digitChar,
    vint tId)
{
    VTSP_ToneTemplate  tone;
    vint               index;
    vint               tMake;
    vint               tBreak;

    index = digitChar - '0';
    if ('#' == digitChar) { /* # */
        index = 11;
    }
    if ('*' == digitChar) {  /* * */
        index = 10;
    }
    if ((index < 0) || (index >= 12)) {
        return (UT_FAIL);
    }

    tMake  = 300;
    tBreak = 300;

    tone.freq1 = UT_dtmfTable[index][0];
    tone.freq2 = UT_dtmfTable[index][1];
    tone.power1 = (-10) * 2;
    tone.power2 = (-10) * 2;
    tone.cadences = 1;
    tone.make1 = tMake;
    tone.break1 = tBreak;
    tone.repeat1 = 1;
    tone.make2 = 0;
    tone.break2 = 0;
    tone.repeat2 = 0;
    tone.make3 = 0;
    tone.break3 = 0;
    tone.repeat3 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId, &tone);

    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, 1, tMake + tBreak);

    /* Pause until tone done; normally would wait for VTSP Event */
    OSAL_taskDelay(tMake + tBreak);

    return (UT_PASS);
}

UT_Return UT_testToneLocal(
    vint arg)
{
    VTSP_ToneTemplate       tone;
    vint                    tId;
    vint                    freq;
    vint                    power;

    freq =  UT_getUserTestParam(UT_TONE_FREQ);
    power = UT_getUserTestParam(UT_TONE_POWER);
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    /*
     * TONE
     */
    OSAL_memSet(&tone, 0, sizeof(VTSP_ToneTemplate));

    tId = 10;
    tone.freq1 = freq;
    tone.freq2 = 0;
    tone.power1 = power;
    tone.power2 = 0;
    tone.cadences = 1;
    tone.make1 = (vint)VTSP_TONE_TMAX;
    tone.break1 = 0;
    tone.repeat1 = 1;

    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId, &tone);

    OSAL_logMsg("\n\nPlaying tone\n");
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, 0, tId, VTSP_TONE_NMAX, VTSP_TONE_TMAX);

    OSAL_taskDelay(UT_testTimeSec * 1000);

    UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, 0);

    return (UT_PASS);
}

UT_Return UT_testToneDbLocal(
    vint arg)
{
    VTSP_ToneTemplate tone;
    vint              tId;
    vint              freq;
    vint              power;

    freq =  UT_getUserTestParam(UT_TONE_FREQ);

    /*
     * TONE
     */
    OSAL_memSet(&tone, 0, sizeof(VTSP_ToneTemplate));

    tId = 10;
    tone.freq1 = freq;
    tone.freq2 = 0;
    tone.power2 = 0;
    tone.cadences = 1;
    tone.make1 = (vint)VTSP_TONE_TMAX;
    tone.break1 = 0;
    tone.repeat1 = 1;

    for (power = -40; power <= 0; power +=1) {
        tone.power1 = power << 1; /* 1dB steps */

        UT_EXPECT2(VTSP_OK, VTSP_configTone, tId, &tone);

        OSAL_logMsg("\n\nPlaying tone @ %ddB\n", power);
        UT_EXPECT4(VTSP_OK, VTSP_toneLocal, 0, tId, VTSP_TONE_NMAX,
                VTSP_TONE_TMAX);

        OSAL_taskDelay(1500);

        UT_EXPECT1(VTSP_OK, VTSP_toneLocalStop, 0);

        OSAL_taskDelay(500);
    }

    return (UT_PASS);
}


UT_Return UT_testTone5ms(
        vint infc)
{
    VTSP_ToneTemplate       tone;
    VTSP_Stream            *stream_ptr;
    vint                    streamId;
    vint                    tId;
    uvint                   toneTable_ptr[30];
    vint                    delay;
    vint                    mixSelect;
    vint                    repeat;
    vint                    numToneIds;
    char                    buf[20];
    OSAL_NetAddress         sendAddr;

    UT_testTimeSec = 30;

    /*
     * TONE
     */
    tId = 10;
    tone.freq1 = 400;
    tone.freq2 = 0;
    tone.power1 = -2;
    tone.power2 = 0;
    tone.cadences = 1;
    tone.make1 = 135;
    tone.break1 = 115;
    tone.repeat1 = 1;
    tone.make2 = 0;
    tone.break2 = 0;
    tone.repeat2 = 0;
    tone.make3 = 0;
    tone.break3 = 0;
    tone.repeat3 = 0;
    UT_EXPECT2(VTSP_OK, VTSP_configTone, tId, &tone);

    OSAL_logMsg("\n\nPlaying tId%d\n", tId);
    OSAL_taskDelay(500);
    UT_EXPECT4(VTSP_OK, VTSP_toneLocal, infc, tId, 1, 2000);

    /*
     * TONE SEQ (No Mix)
     */
    mixSelect   = 0;
    delay       = 5000;
    repeat      = 1;
    infc        = 0;
    toneTable_ptr[0] = 10;
    toneTable_ptr[1] = 10;
    toneTable_ptr[2] = 10;
    toneTable_ptr[3] = 10;
    toneTable_ptr[4] = 10;
    numToneIds  = 5;
    OSAL_logMsg("\n\nPlaying tId%d in VTSP_toneLocalSequence\n", tId);
    OSAL_taskDelay(500);
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, toneTable_ptr, numToneIds,
            mixSelect, repeat);
    OSAL_taskDelay(delay);

    /*
     * TONE SEQ (With Mix)
     */
    /* Set up stream to remote loopback server */
    infc = 0;
    streamId = 0;
    OSAL_strcpy(buf, "172.16.60.99");
    OSAL_netStringToAddress((int8 *)buf, &sendAddr);
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;
    UT_sendPort = 5001;
    UT_recvPort = 5001;
    stream_ptr = UT_updateStreamData(infc, streamId);
    OSAL_logMsg("%s: Starting infc %d streamId %d..\n", __FILE__,
            infc, stream_ptr->streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
    /* Start Tone */
    mixSelect   = VTSP_TONE_BREAK_MIX;
    delay       = 5000;
    repeat      = 1;
    infc        = 0;
    toneTable_ptr[0] = 10;
    toneTable_ptr[1] = 10;
    toneTable_ptr[2] = 10;
    toneTable_ptr[3] = 10;
    toneTable_ptr[4] = 10;
    numToneIds  = 5;
    OSAL_logMsg("\n\nPlaying tId%d in VTSP_toneLocalSequence + MIX\n", tId);
    OSAL_taskDelay(500);
    UT_EXPECT5(VTSP_OK, VTSP_toneLocalSequence, infc, toneTable_ptr, numToneIds,
            mixSelect, repeat);
    OSAL_taskDelay(delay);
    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);

    return (UT_PASS);
}

/*
 * ======== UT_netTx() ========
 */
int UT_netTx(
        int *arg)
{
    OSAL_NetSockId     socketFd;
    int                sockOpt;
    int                retVal;
    uvint              index;
    int                recvSize;
    int                sendSize;
    char               sbuf[96];
    char               rbuf[1025];
    uint16             seqn = 0xd2d2;
    uint32             ts = 0xd2d2d2d2;
    uint32             UT_netTxEnable;
    OSAL_NetAddress    sendAddr;
    OSAL_NetAddress    recvAddr;
    int8               ipStr[OSAL_NET_IPV6_STR_MAX];


    OSAL_taskDelay(100);
    OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);

    OSAL_memSet(&sendAddr, 0, sizeof(OSAL_NetAddress));
    OSAL_netAddrCpy(&sendAddr, &UT_sendAddr);
    sendAddr.port = OSAL_netHtons(UT_sendPort);
    OSAL_netAddressToString(ipStr, &sendAddr);
    if (OSAL_netIsAddrZero(&sendAddr) || !sendAddr.port) {
        OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    OSAL_logMsg("%s:%d sending to IP %s, port %d\n", __FILE__, __LINE__,
           ipStr, sendAddr.port);
    OSAL_taskDelay(100);

    OSAL_memSet(&recvAddr, 0, sizeof(OSAL_NetAddress));
    recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    recvAddr.port = OSAL_netHtons(UT_sendPort);
    OSAL_logMsg("%s:%d recving on 0x%0x port %d\n", __FILE__, __LINE__,
           recvAddr.ipv4, recvAddr.port);
    if (!recvAddr.port) {
        return (UT_FAIL);
    }
    OSAL_taskDelay(100);
    /* Create fake RTP header for easy pkt sniff */
    sbuf[0]  = 0x80;
    sbuf[1]  = 0x00;
    sbuf[2]  = 0xf6;
    sbuf[3]  = 0xcb;
    sbuf[4]  = 0x00;
    sbuf[5]  = 0x0d;
    sbuf[6]  = 0x1a;
    sbuf[7]  = 0x60;
    sbuf[8]  = 0x00;
    sbuf[9]  = 0x02;
    sbuf[10] = 0xd5;
    sbuf[11] = 0xf0;

    for (index = 12; index < sizeof(sbuf); index++) {
        sbuf[index] = index;
    }

    if (OSAL_netSocket(&socketFd, OSAL_NET_SOCK_UDP) == OSAL_FAIL)
    {
        OSAL_logMsg("%s:%d Open Failed = %d\n", __FILE__, __LINE__, socketFd);
        return (UT_FAIL);
    }
    sockOpt = 1;
    if ((retVal = OSAL_netSetSocketOptions(&socketFd, OSAL_NET_SOCK_REUSE,
            sockOpt)) == OSAL_FAIL) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d setsockopt(REUSE_ADDR) Failed = %d\n",
            __FILE__, __LINE__, retVal);
    }

    if ((retVal = OSAL_netBindSocket(&socketFd, &recvAddr))
         == OSAL_FAIL) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d bind Failed = %d\n", __FILE__, __LINE__, retVal);
        return (UT_FAIL);
    }

    UT_netTxEnable = 1;
    while (UT_netTxEnable) {
        OSAL_taskDelay(9);  /* not 10, due to avg o/s round-up latency */
        sbuf[2] = OSAL_netHtons(seqn) >> 8;
        sbuf[3] = OSAL_netHtons(seqn) & 0xff;
        sbuf[4] = (OSAL_netHtonl(ts) >> 24) & 0xff;
        sbuf[5] = (OSAL_netHtonl(ts) >> 16) & 0xff;
        sbuf[6] = (OSAL_netHtonl(ts) >> 8) & 0xff;
        sbuf[7] = OSAL_netHtonl(ts) & 0xff;
        seqn++;
        ts += 80;
        sendSize = sizeof(sbuf);
        if ((retVal = OSAL_netSocketSendTo(&socketFd, (void *)sbuf, &sendSize,
                        &sendAddr)) == OSAL_FAIL)
        {
            OSAL_logMsg("%s:%d sendto Failed with %d\n",
                __FILE__, __LINE__, retVal);
            OSAL_taskDelay(1000);
        }
        recvSize = sizeof(rbuf);
        if ((retVal = OSAL_netSocketReceiveFrom(&socketFd, (void *)rbuf,
                &recvSize, &recvAddr)) == OSAL_FAIL) {
            /* do nothing; sink all pkts if recvd, otherwise no error */
        }
    }

    if ((retVal = OSAL_netCloseSocket(&socketFd)) == OSAL_FAIL) {
        OSAL_logMsg("%s:%d close Failed = %d\n", __FILE__,
                __LINE__, retVal);
        return (UT_FAIL);
    }

    return (UT_PASS);
}

/*
 * ======== UT_testNetTx() ========
 */
UT_Return UT_testNetTx(
        vint arg)
{
    char            buf[OSAL_NET_IPV6_STR_MAX];
    uint32          word;
    vint            peerNum;
    OSAL_TaskArg    taskArg;
    int8            ipStr[OSAL_NET_IPV6_STR_MAX];

    taskArg = 0;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    OSAL_logMsg("\n%s:%d This test will spawn 4 network transmitters.\n",
            __FILE__, __LINE__);

    for (peerNum = 0; peerNum < 4; peerNum++) {
        UT_getUserTestParam(UT_IP_ADDRESS);

        OSAL_netAddressToString(ipStr, &UT_sendAddr);
        OSAL_logMsg("%s:  Enter IP Port of peer %d @ %s (%d) : ",
                __FUNCTION__, peerNum, ipStr, UT_startPort);
        D2_getLine(buf, 20);
        word = OSAL_atoi(buf);
        if (!word) {
            word = UT_startPort;
        }
        UT_sendPort = word;

        OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
        OSAL_taskCreate("utNetTx",
                249,     /* XXX Warning, this must be > CpmacNet Task */
                8000, (OSAL_TaskPtr)UT_netTx, taskArg);
        OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
        OSAL_taskDelay(500);
    }

    return (UT_PASS);
}

/*
 * ======== UT_testBind() ========
 */
#define UT_TEST_SIZE   10
UT_Return UT_testBind(
        vint arg)
{
    OSAL_NetSockId     socketFd;
    int                retVal;
    int                iloop;
    int                oloop;
    int                recvSize, sendSize;
    char               buffer[UT_TEST_SIZE];
    VTSP_Version       vtspVersion;
    OSAL_NetAddress     sendAddr;
    OSAL_NetAddress     recvAddr;

    for (oloop = 0; oloop < 10; oloop++) {
        OSAL_memSet(&sendAddr, 0, sizeof(OSAL_NetAddress));
        OSAL_netStringToAddress("172.16.0.171", &sendAddr);
        sendAddr.port = OSAL_netHtons(5001);

        OSAL_memSet(&recvAddr, 0, sizeof(OSAL_NetAddress));
        recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
        recvAddr.port = OSAL_netHtons(5001);

        OSAL_memSet(buffer, 0xff, 10);

        if ((retVal = VOICE_NET_SOCKET(&socketFd,OSAL_NET_SOCK_UDP)) == OSAL_FAIL) {
            OSAL_logMsg("%s:%d Open Failed = %d\n", __FILE__, __LINE__,
                    socketFd);
            return (UT_FAIL);
        }

        if ((retVal = VOICE_NET_BIND_SOCKET(&socketFd, &recvAddr)) == OSAL_FAIL)
        {
            OSAL_logMsg("%s:%d bind Failed = %d\n", __FILE__, __LINE__,
                    retVal);
            return (UT_FAIL);
        }

        for (iloop = 0; iloop < 10; iloop++) {
            OSAL_logMsg("%s:%d sending %d\n", __FILE__, __LINE__, iloop);
            sendSize = recvSize = UT_TEST_SIZE;
            if ((retVal = VOICE_NET_SOCKET_SEND_TO(&socketFd, buffer, &sendSize,
                    &sendAddr)) != OSAL_SUCCESS || UT_TEST_SIZE!= sendSize) {
                OSAL_logMsg("%s:%d sendto Failed = %d\n", __FILE__,
                        __LINE__, retVal);
                return (UT_FAIL);
            }

            if ((retVal = VOICE_NET_SOCKET_RECEIVE_FROM(&socketFd, buffer,
                    &recvSize, &recvAddr)) < 0) {
                OSAL_logMsg("%s:%d recvfrom Failed = %d\n", __FILE__,
                        __LINE__, retVal);
                return (UT_FAIL);
            }
        }

        if ((retVal = VOICE_NET_CLOSE_SOCKET(&socketFd)) != OSAL_SUCCESS) {
            OSAL_logMsg("%s:%d close Failed = %d\n", __FILE__,__LINE__, retVal);
            return (UT_FAIL);
        }
    }

    if (VTSP_OK != VTSP_getVersion(&vtspVersion)) {
        return(UT_FAIL);
    }

    OSAL_logMsg("%s:%d: %s ", __FILE__, __LINE__,
            vtspVersion.string_ptr);
    OSAL_logMsg(" %c-%d-%d-%d\n", vtspVersion.type, vtspVersion.major,
            vtspVersion.minor, vtspVersion.point);

    return (UT_PASS);
}

/*
 * ======== UT_streamLoopback() ========
 */
UT_Return UT_streamLoopback(
        vint arg)
{
    uvint            infc;
    VTSP_AecTemplate aecConfig;
    UT_Return        retVal;

    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    infc = 0;

    OSAL_logMsg("%s:  IP Addr of destination: 127.0.0.1\n", __FUNCTION__);

    /* only test by ipv4 */
    UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_sendAddr.type = OSAL_NET_SOCK_UDP;
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_recvAddr.type = OSAL_NET_SOCK_UDP;

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    OSAL_logMsg("----\nDisabling AEC\n----\n");
    aecConfig.control = VTSP_TEMPL_CONTROL_AEC_BYPASS;
    VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);

    retVal = UT_testStreamPeer(infc, &UT_vtspStream0);

    OSAL_logMsg("----\nTest Complete. Re-enabling AEC\n----\n");
    aecConfig.control = VTSP_TEMPL_CONTROL_AEC_NORMAL;
    VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);

    return (retVal);
}

/*
 * ======== UT_streamLoopbackVideo() ========
 */
UT_Return UT_streamLoopbackVideo(
        vint arg)
{
    uvint          infc;
    infc = VTSP_INFC_VIDEO;
#if defined(OSAL_THREADX)
    UT_testTimeSec = 10000;
    UT_testVideoKeyIntSec = 0;

    UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_sendAddr.type = OSAL_NET_SOCK_UDP;
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_recvAddr.type = OSAL_NET_SOCK_UDP;

#else
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);
    UT_testVideoKeyIntSec = UT_getUserTestParam(UT_TEST_VIDEO_KEYINT);

    OSAL_logMsg("%s:  IP Addr of destination: 127.0.0.1", __FUNCTION__);

    /* only test by ipv4 */
    UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_sendAddr.type = OSAL_NET_SOCK_UDP;
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_recvAddr.type = OSAL_NET_SOCK_UDP;
#endif
    UT_sendPort = UT_startPort + UT_vtspStreamv0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStreamv0.streamId;

    return (UT_testStreamPeerVideo(infc, &UT_vtspStreamv0));
}

/*
 * ======== UT_streamPeerVideo() ========
 */
UT_Return UT_streamPeerVideo(
        vint arg)
{
    uvint          infc;

    UT_getUserTestParam(UT_IP_ADDRESS);
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);
    UT_testVideoKeyIntSec = UT_getUserTestParam(UT_TEST_VIDEO_KEYINT);

    infc = VTSP_INFC_VIDEO;

    OSAL_logMsg("%s:  IP Addr of destination: 127.0.0.1", __FUNCTION__);

    UT_sendPort = UT_startPort + UT_vtspStreamv0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStreamv0.streamId;

    return (UT_testStreamPeerVideo(infc, &UT_vtspStreamv0));
}


/*
 * ======== UT_streamLoopbackToneMixing() ========
 */
UT_Return UT_streamLoopbackToneMixing(
        vint arg)
{

    uvint          infc;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = 0;

    OSAL_logMsg("%s:  IP Addr of destination: 127.0.0.1", __FUNCTION__);

    /* only test by ipv4 */
    UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_sendAddr.type = OSAL_NET_SOCK_UDP;
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_recvAddr.type = OSAL_NET_SOCK_UDP;

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    return (UT_testStreamPeerToneMixing(infc, &UT_vtspStream0));

}

/*
 * ======== UT_streamHold() ========
 */
UT_Return UT_streamHold(
        vint arg)
{

    uvint          infc;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = 0;

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    return (UT_testStreamHold(infc, &UT_vtspStream0, &UT_vtspStream1));

}

/*
 * ======== UT_streamPeer() ========
 */
UT_Return UT_streamPeer(
        vint arg)
{
    uvint          infc;

    infc = 0;
    UT_getUserTestParam(UT_IP_ADDRESS);
    UT_getUserTestParam(UT_SILCOMP_MASK);
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

#ifdef VTSP_ENABLE_DTMFR
    /* Enable DTMFR */
    UT_dtmfRelay = 255;
#endif

    OSAL_logMsg("%s:%d: testStreamPeer()\n", __FUNCTION__, __LINE__);
    return (UT_testStreamPeer(infc, &UT_vtspStream0));

}


/*
 * ======== UT_srtpStream() ========
 */
UT_Return UT_srtpStream(
        vint arg)
{
    uvint          infc;

    infc = 0;
    UT_getUserTestParam(UT_IP_ADDRESS);

//    UT_silComp = 0;
    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

#ifdef VTSP_ENABLE_DTMFR
    /* Enable DTMFR */
    UT_dtmfRelay = 0xffffffff;
#endif

    OSAL_logMsg("%s:%d: testSrtpStream()\n", __FUNCTION__, __LINE__);
    return (UT_testSrtpStream(infc, &UT_vtspStream0));

}

/*
 * ======== UT_streamConfVQT() ========
 */
UT_Return UT_streamConfVQT(
    vint arg)
{
    uvint             infc;
    char              buf[10];
    vint              word;
    OSAL_NetAddress     sendAddr;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = 0;

    OSAL_logMsg("%s:  In this test you are peerA, "
            "with streams A<->B and A<->C.\n", __FUNCTION__);

    OSAL_logMsg("%s:  Enter IP Addr of peerB (172.16.0.23) : ", __FUNCTION__);
    D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
    if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL) {
        OSAL_netStringToAddress("172.16.0.23", &sendAddr);
        OSAL_logMsg("%s: Invalid IP address.\n",__FILE__);
        return (UT_FAIL);
    }
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    OSAL_logMsg("%s:  Enter IP Port of peerB @ %s (%d-%d) : ",
            __FUNCTION__, buf, UT_startPort, UT_endPort);
    D2_getLine(buf, 20);
    word = OSAL_atoi(buf);
    UT_sendPort  = word;
    UT_recvPort  = UT_sendPort;
    UT_encoder   = UT_getUserTestParam(UT_ENCODER);
    UT_silComp   = UT_getUserTestParam(UT_SILCOMP_MASK);
    UT_dtmfRelay = UT_getUserTestParam(UT_DTMFR_ENCODE_MASK);
    UT_extension = UT_getUserTestParam(UT_EXTENSION_WORD);
    UT_confMask  = UT_getUserTestParam(UT_CONFERENCE_MASK);

    /* Update vars to stream object */
    UT_updateStreamData(infc, UT_vtspStream0.streamId);

    OSAL_logMsg("%s:  Enter IP Addr of peerC (172.16.0.23) : ", __FUNCTION__);
    D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
    if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL) {
        OSAL_netStringToAddress("172.16.0.23", &sendAddr);
        OSAL_logMsg("%s: Invalid IP address.\n",__FILE__);
        return (UT_FAIL);
    }
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    OSAL_logMsg("%s:  Enter IP Port of peerC @ %s (%d-%d) : ",
            __FUNCTION__, buf, UT_startPort, UT_endPort);
    D2_getLine(buf, 20);
    word = OSAL_atoi(buf);
    UT_sendPort = word;
    UT_recvPort = UT_sendPort;
    UT_encoder  = UT_getUserTestParam(UT_ENCODER);
    UT_confMask = UT_getUserTestParam(UT_CONFERENCE_MASK);

    /* Update vars to stream object */
    UT_updateStreamData(infc, UT_vtspStream1.streamId);

    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    return (UT_testStreamConfVQT(infc, &UT_vtspStream0, &UT_vtspStream1));
}

/*
 * ======== UT_streamVce0() ========
 */
UT_Return UT_streamVce0(
    vint arg)
{
    uvint             infc;
    char              buf[10];
    vint              word;
    vint              streamId;
    VTSP_Stream      *stream_ptr;
    OSAL_NetAddress     sendAddr;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    infc = arg;
    streamId = 0;
    OSAL_logMsg("%s:  Telephony Infc %d\n", __FUNCTION__, infc);
    OSAL_logMsg("%s:  StreamId %d\n", __FUNCTION__, streamId);

    OSAL_logMsg("%s:  Enter IP Addr of conference bridge (172.16.0.23) : ",
            __FUNCTION__);
    D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
    if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL) {
        OSAL_netStringToAddress("172.16.0.23", &sendAddr);
        OSAL_logMsg("%s: Invalid IP address.\n",__FILE__);
        return (UT_FAIL);
    }
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    UT_sendPort = UT_startPort + 2*infc;    /* default */
    OSAL_logMsg("%s:  Enter RTP Port of conference bridge @ %s (%d) : ",
            __FUNCTION__, buf, UT_sendPort);
    D2_getLine(buf, 20);
    word = OSAL_atoi(buf);
    if (!word) {
        word = UT_sendPort;
    }
    UT_sendPort = word;
    UT_recvPort = word;

    UT_encoder = UT_getUserTestParam(UT_ENCODER);

    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_confMask = 0;
    UT_testTimeSec = -1;

    stream_ptr = UT_updateStreamData(infc, streamId);

    return (UT_testStreamVQT(infc, stream_ptr));
}

/*
 * ======== UT_streamVcb0() ========
 */
UT_Return UT_streamVcb0(
    vint arg)
{
    uvint             infc;
    char              buf[10];
    vint              word;
    vint              streamId;
    VTSP_Stream      *stream_ptr;
    OSAL_NetAddress     sendAddr;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    OSAL_logMsg("%s:  In this test you are the conference bridge, "
            "with 2 streams, A<->B and A<->C.\n", __FUNCTION__);
    /*
     * Set stream 0.
     */
    infc = arg;
    streamId = 0;
    OSAL_logMsg("%s:  Telephony Infc %d\n", __FUNCTION__, infc);
    OSAL_logMsg("%s:  StreamId %d\n", __FUNCTION__, streamId);

    OSAL_logMsg("%s:  Enter IP Addr of peerB (172.16.0.23) : ", __FUNCTION__);
    D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
    if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL) {
        OSAL_netStringToAddress("172.16.0.23", &sendAddr);
        OSAL_logMsg("%s: Invalid IP address.\n",__FILE__);
        return (UT_FAIL);
    }
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    UT_sendPort = UT_startPort + 2*(infc * 2 + streamId);
    OSAL_logMsg("%s:  Enter IP Port of peerB @ %s (%d) : ",
            __FUNCTION__, buf, UT_sendPort);
    D2_getLine(buf, 20);
    word = OSAL_atoi(buf);
    if (0 == word) {
        word = UT_sendPort;
    }
    UT_sendPort = word;
    UT_recvPort = UT_sendPort;
    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_confMask = (1<< (!streamId));
    /* Update parameters to stream */
    stream_ptr = UT_updateStreamData(infc, streamId);
    /* Run voice stream */
    UT_testTimeSec = -1;
    UT_testStreamVQT(infc, stream_ptr);

    /*
     * Set stream 1.
     */
    streamId = 1;
    OSAL_logMsg("%s:  Telephony Infc %d\n", __FUNCTION__, infc);
    OSAL_logMsg("%s:  StreamId %d\n", __FUNCTION__, streamId);

    OSAL_logMsg("%s:  Enter IP Addr of peerC (172.16.0.23) : ", __FUNCTION__);
    D2_getLine(buf, OSAL_NET_IPV6_STR_MAX);
    if(OSAL_netStringToAddress((int8 *)buf,&sendAddr) == OSAL_FAIL) {
        OSAL_netStringToAddress("172.16.0.23", &sendAddr);
        OSAL_logMsg("%s: Invalid IP address.\n",__FILE__);
        return (UT_FAIL);
    }
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    /* config receive address as ANY */
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    UT_sendPort = UT_startPort + 2*(infc * 2 + streamId);
    OSAL_logMsg("%s:  Enter IP Port of peerC @ %s (%d) : ",
            __FUNCTION__, buf, UT_sendPort);
    D2_getLine(buf, 20);
    word = OSAL_atoi(buf);
    if (0 == word) {
        word = UT_sendPort;
    }
    UT_sendPort = word;
    UT_recvPort = UT_sendPort;

    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_confMask = (1 << (!streamId));
    /* Update parameters to stream */
    stream_ptr = UT_updateStreamData(infc, streamId);
    /* Run voice stream */
    UT_testTimeSec = -1;
    UT_testStreamVQT(infc, stream_ptr);

    return (UT_PASS);
}

/*
 * ======== UT_streamJBstats() ========
 */
UT_Return UT_streamJBstats(
    vint arg)
{
    uvint             infc;
    VTSP_Stream      *stream_ptr;
    VTSP_RtpTemplate  rtpConfig;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);
    infc = rtpConfig.streamId = 0;

    /* only test by ipv4 */
    UT_sendAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_sendAddr.type = OSAL_NET_SOCK_UDP;
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    UT_recvAddr.type = OSAL_NET_SOCK_UDP;

    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    rtpConfig.rdnLevel = UT_getUserTestParam(UT_RTP_REDUN_LEVEL);
    rtpConfig.rdnHop   = UT_getUserTestParam(UT_RTP_REDUN_HOP);

    UT_dtmfRelay = 0x0;
    UT_extension = 0;

    rtpConfig.control = VTSP_TEMPL_CONTROL_RTP_REDUNDANT;
    UT_EXPECT3(VTSP_OK, VTSP_config, VTSP_TEMPL_CODE_RTP, infc, &rtpConfig);

    stream_ptr = UT_updateStreamData(infc, rtpConfig.streamId);
    stream_ptr->rdnDynType = 99;
    return (UT_testStreamVQT(infc, stream_ptr));
}

/*
 * ======== UT_streamVQT() ========
 */
UT_Return UT_streamVQT(
    vint arg)
{
    uvint             infc;
    vint              streamId;
    VTSP_Stream      *stream_ptr;

#if defined(OSAL_THREADX)
    OSAL_NetAddress   sendAddr;
#endif

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    infc = 0;
#if !defined(OSAL_THREADX)
    streamId = UT_getUserTestParam(UT_STREAM_ID);

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = UT_getUserTestParam(UT_IP_PORT);
    UT_recvPort = UT_sendPort;
    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = UT_getUserTestParam(UT_SILCOMP_MASK);
    UT_dtmfRelay = UT_getUserTestParam(UT_DTMFR_ENCODE_MASK);
    UT_extension = UT_getUserTestParam(UT_EXTENSION_WORD);
    UT_encodeTime = UT_getUserTestParam(UT_CODER_ENCODE_TIME);
    UT_encodeTime_cn = UT_getUserTestParam(UT_CN_ENCODE_TIME);
    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);
#else
    /* For threadx, fix command to test loopback */
    streamId = 0;
    OSAL_netStringToAddress("127.0.0.1", &sendAddr);
    OSAL_netAddrCpy(&UT_sendAddr, &sendAddr);
    UT_recvAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_ANY);
    OSAL_netIpv6AddrAnyInit(UT_recvAddr.ipv6);
    UT_recvAddr.type = UT_sendAddr.type;

    UT_sendPort = 5001;
    UT_recvPort = UT_sendPort;
    UT_encoder = 14;    // VTSP_CODER_GAMRNB_20MS_OA
#if 0
    UT_silComp = 0xffffffff;  // enable CN
#else
    UT_silComp = 0;
#endif
    UT_dtmfRelay = 0;
    UT_extension = 1 << (16 + 0);   // 0 is GAMRNB_MR475
    UT_encodeTime = 20;
    UT_encodeTime_cn = 0;
    UT_testTimeSec = 0; // forever
#endif

    stream_ptr = UT_updateStreamData(infc, streamId);

    return (UT_testStreamVQT(infc, stream_ptr));
}

/*
 * ======== UT_srtpStreamVQT() ========
 */
UT_Return UT_srtpStreamVQT(
    vint arg)
{

    uvint             infc;
    vint              streamId;
    VTSP_Stream      *stream_ptr;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    infc = 0;
    streamId = 0;

    UT_getUserTestParam(UT_IP_ADDRESS);
    UT_sendPort = UT_getUserTestParam(UT_IP_PORT);
    UT_recvPort = UT_sendPort;

    UT_encoder = UT_getUserTestParam(UT_ENCODER);
    UT_silComp = 0;
    UT_dtmfRelay = 0;
    UT_extension = 0;
    UT_encodeTime = 10;
    UT_encodeTime_cn = 200;

    UT_testTimeSec = UT_getUserTestParam(UT_TEST_LENGTH);

    stream_ptr = UT_updateStreamData(infc, streamId);

    return (UT_testSrtpStreamVQT(infc, stream_ptr));
}

/*
 * ======== UT_streamVQTall() ========
 */
UT_Return UT_streamVQTall(
    vint arg)
{

    uvint             infc;
    vint              streamId;
    VTSP_Stream      *stream_ptr;

    OSAL_logMsg("\n%s:%d\n", __FILE__, __LINE__);

    infc = 0; //UT_getUserTestParam(UT_INFC);
    streamId = 0; // UT_getUserTestParam(UT_STREAM_ID);

    UT_getUserTestParam(UT_IP_ADDRESS);

    UT_sendPort = 5001; //UT_getUserTestParam(UT_IP_PORT);
    UT_recvPort = UT_sendPort;

    /* Start with the these settings, which are adjusted during the test. */
    UT_encoder = 0;
    UT_silComp = 0xffffffff;
    UT_dtmfRelay = 0xffffffff;
    UT_extension = 0;
    UT_encodeTime = 10;
    UT_encodeTime_cn = 500;

    stream_ptr = UT_updateStreamData(infc, streamId);

    return (UT_testStreamVQTall(infc, stream_ptr));
}

UT_Return UT_testRfc4733(
    vint arg)
{
    VTSP_Stream *stream_ptr;
    uvint        infc;
    uvint        streamId = 0;
    VTSP_Rfc4733Event  event;
    VTSP_Rfc4733Tone   tone;

    OSAL_logMsg("%s: Event printing ON.\n", __FILE__);

    infc = 0;
    UT_eventTaskDisplay = 1;
    UT_getUserTestParam(UT_IP_ADDRESS);
    UT_encodeTime = UT_getUserTestParam(UT_CODER_ENCODE_TIME);
    UT_getUserTestParam(UT_DTMFR_RATE);
    UT_sendPort = UT_startPort + UT_vtspStream0.streamId;
    UT_recvPort = UT_startPort + UT_vtspStream0.streamId;

    UT_silComp = 0xffffffff;
    UT_dtmfRelay = 0xffffffff;

    stream_ptr = UT_updateStreamData(infc, streamId);

    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream_ptr);
    OSAL_taskDelay(2000);

   /*
     * Perform the following three tests for Sending RFC2833
     * DTMF relay generation
     * 1) Send Fixed length DTMF digits; 1, 2 and 3 end packets
     * 2) Send a DTMF digit, and then stop it
     * 3) Send a DTMF digit, then send a new DTMF digit, then stop it
     */

    /*
     * RFC4733 Test 1
     */
    OSAL_logMsg("%s: RFC4733 Test 1 - DTMF.\n", __FILE__);

    event.eventNum   = 1;   /* digit 1 */
    event.volume     = 11;  /* volume of 11 */
    event.duration   = 140; /* duration of 100 ms */
    event.endPackets = 3;   /* use 1 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(500);

    event.eventNum   = 2;   /* digit 2 */
    event.volume     = 12;  /* volume of 12 */
    event.duration   = 250; /* duration of 100 ms */
    event.endPackets = 3;   /* use 2 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(500);

    event.eventNum   = 3;   /* digit 3 */
    event.volume     = 13;  /* volume of 13 */
    event.duration   = 100; /* duration of 100 ms */
    event.endPackets = 2;   /* use 3 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);

    OSAL_taskDelay(2000);
    /*
     * RFC4733 Test 2
     */
    OSAL_logMsg("%s: RFC4733 Test 2 - DTMF.\n", __FILE__);

    event.eventNum   = 10;  /* digit 10 */
    event.volume     = 20;  /* volume of 20 */
    event.duration   = 8000; /* duration of 8000 ms */
    event.endPackets = 3;   /* use 3 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(3000);

    event.eventNum   = 0; /* digit 0, should be ignored */
    event.volume     = 0; /* volume of 0, should be ignored */
    event.duration   = 0; /* duration of 0 ms, should end current digit */
    event.endPackets = 1; /* use 1 end packets, should be ignored */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);

    OSAL_taskDelay(2000);
    /*
     * RFC4733 Test 3
     */
    OSAL_logMsg("%s: RFC4733 Test 3 - DTMF .\n", __FILE__);

    event.eventNum   = 11;  /* digit 11 */
    event.volume     = 31;  /* volume of 31 */
    event.duration   = 800; /* duration of 8000 ms */
    event.endPackets = 3;   /* use 3 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(1000);
    event.eventNum   = 12;  /* digit 12 */
    event.volume     = 32;  /* volume of 32 */
    event.duration   = 800; /* duration of 8000 ms */
    event.endPackets = 3;   /* use 3 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(1000);

    event.eventNum   = 0; /* digit 0, should be ignored */
    event.volume     = 0; /* volume of 0, should be ignored */
    event.duration   = 0; /* duration of 0 ms, should end current digit */
    event.endPackets = 1; /* use 1 end packets, should be ignored */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_EVENT, &event);
    OSAL_taskDelay(2000);
    /*
     * Perform the following three tests for Sending RFC4733
     * Tone relay generation
     * 1) Send 1 sec, 1kHz Tone
     * 2)
     * 3)
     */
    /*
     * RFC4733 Test 4 - 1k Tone
     */
    OSAL_logMsg("%s: RFC4733 Test 4 - Tones.\n", __FILE__);

    tone.modFreq    = 0;    /* No AM XXX requires GENF */
    tone.divMod3    = 0;    /* No AM */
    tone.freq1      = 1000; /* Freq 1,  1000 Hz */
    tone.freq2      = 0;    /* Freq 2,  0 Hz */
    tone.freq3      = 0;    /* Freq 3,  0 Hz */
    tone.freq4      = 0;    /* Freq 4,  0 Hz */
    tone.volume     = 31;   /* volume of 31 */
    tone.duration   = 1000; /* duration of 1000 ms */
    tone.endPackets = 3;    /* use 3 end packets */
    UT_EXPECT4(VTSP_OK, VTSP_streamSendRfc4733, infc, streamId,
            VTSP_RFC4733_TYPE_TONE, &tone);



    OSAL_logMsg("%s: Test finished.\n", __FILE__);
    OSAL_taskDelay(500);

    /*
     * End the test
     */
    OSAL_taskDelay(2000);
    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, streamId);
    OSAL_logMsg("%s: Event printing OFF.\n", __FILE__);
    UT_eventTaskDisplay = 0;

    return (UT_PASS);
}

/*
 * ======== UT_vtspInit() ========
 */
UT_Return UT_vtspInit(
    vint arg)
{
    VTSP_QueryData   *vtspConfig_ptr;
    VTSP_TaskConfig   taskConfig;
    VTSP_Version      versionData;

    OSAL_memSet(&taskConfig, 0, sizeof(VTSP_TaskConfig));

    taskConfig.rtcpInternalPort = OSAL_netHtons(2049);

    if (NULL == UT_vtsp_ptr) {

        if (VTSP_OK != VTSP_init(&UT_vtsp_ptr, NULL, &taskConfig)) {
            UT_FAILMSG;
            return (UT_FAIL);
        }

        if (NULL == UT_vtsp_ptr) {
            UT_FAILMSG;
            return (UT_FAIL);
        }

        if (UT_PASS != UT_event()) {
            UT_FAILMSG;
            return (UT_FAIL);
        }

        vtspConfig_ptr = VTSP_query();

        OSAL_logMsg("%s:%d  vtspConfig_ptr->encoder.types 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->encoder.types);
        OSAL_logMsg("%s:%d  vtspConfig_ptr->encoder.silenceComp 0x%x\n",
                __FILE__, __LINE__,
                vtspConfig_ptr->encoder.silenceComp);
        OSAL_logMsg("%s:%d  vtspConfig_ptr->encoder.dtmfRelay 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->encoder.dtmfRelay);
        OSAL_logMsg("%s:%d  vtspConfig_ptr->decoder.types 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->decoder.types);
        OSAL_logMsg("%s:%d  vtspConfig_ptr->decoder.dtmfRelay 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->decoder.dtmfRelay);

        OSAL_logMsg("%s:%d  vtspConfig_ptr->hw.numInfc 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->hw.numInfc);
        OSAL_logMsg("%s:%d  vtspConfig_ptr->hw.audio 0x%x\n",
                __FILE__, __LINE__, vtspConfig_ptr->hw.numAudio);

        OSAL_taskDelay(100);    /* cleans up printing */
        UT_EXPECT1(VTSP_OK, VTSP_getVersion, &versionData);
        OSAL_taskDelay(100);    /* cleans up printing */
        OSAL_logMsg("%s: VTSP_getVersion reports v%d.%d.%d\n\n",
            __FILE__, versionData.major, versionData.minor, versionData.point);

        if (VTSP_OK != VTSP_start()) {
            UT_FAILMSG;
            return (UT_FAIL);
        }

        /*
         * NOTE.
         * The # of ports depends on the # of streams in the system.
         * Must use VTSP query config data to configure # of ports.
         *
         */
        UT_startPort = UT_RTP_PORT_START;
        UT_endPort = UT_startPort + vtspConfig_ptr->stream.numStreams - 1;

        /*
         * Wake the VHW audio driver
         */
        if (VTSP_OK != VTSP_infcControlIO(0, VTSP_CONTROL_INFC_IO_AUDIO_ATTACH,
                VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_ENABLE)) {
            OSAL_logMsg("%s: FAILED to 'attach' the vtsp\n", __FUNCTION__);
            return (UT_FAIL);
        }
    }



    return (UT_PASS);
}

/*
 * ======== UT_menu() ========
 */
OSAL_TaskReturn UT_menu(
        char *varg)
{
    vint              item;
    vint              itemMax;
    UT_TestTableItem *item_ptr;
    UT_TestFunc      *func_ptr;
    int32             arg;
    vint              printMenu;
    char              buf[10];

    UT_EXPECT1(UT_PASS, UT_vtspInit, 0);

    /* Init the stream structures */
    UT_initStreamStruct(&UT_vtspStream0, 0);
    UT_initStreamStruct(&UT_vtspStream1, 1);
    UT_initStreamStruct(&UT_vtspStream2, 0);
    UT_initStreamStruct(&UT_vtspStream3, 1);
    UT_initStreamStructVideo(&UT_vtspStreamv0, 0);

    printMenu = 1;

    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(UT_testTable)/sizeof(UT_testTable[0]);
    while (VAPP_run) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n\n"
                    "=====================\n"
                    "D2 Tech VTSP Sys Test %s\n"
                    "=====================\n"
                    "Command  Description\n"
                    "-------  -----------\n",
                    D2_VPORT_REVISION);

            for (item = 0; item < itemMax; item++) {
                item_ptr = &UT_testTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\n\n== VTSP_UT == Cmd: ");

#if  defined(VTSP_UT_FIXED_CMD)
        OSAL_taskDelay(2000);
        OSAL_strcpy(buf, varg);
#elif defined(UT_FIXED_CMD)
        OSAL_taskDelay(2000);
        OSAL_strcpy(buf, UT_FIXED_CMD);
#else
        D2_getLine(buf, 2 * sizeof(UT_testTable[0].cmd));
#endif

        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &UT_testTable[item];
            if (0 == OSAL_strncmp(buf, (char *)item_ptr->cmd, sizeof(buf))) {
                func_ptr = item_ptr->func_ptr;
                break;
            }
        }

        if (NULL != func_ptr) {
            OSAL_logMsg("\n\n\n"
                    "==============================================\n"
                    "%s\n"
                    "==============================================\n\n\n",
                    item_ptr->desc);
            /*
             * In the future the arg will be passed from the user's cmd line
             * after parsing the cmd line for white space, etc.
             */
            arg = 0;
            func_ptr(arg);
            printMenu = 0;
        }
        else {
            /* Unknown cmd, print help */
            printMenu = 1;
        }
    }
    OSAL_logMsg("\n%s:%d  UT_menu return\n\n", __FILE__, __LINE__);

    return (UT_PASS);
}


/*
 * ======== UT_main() ========
 */
int UT_main(
        int argc, char *argv[])
{
    char               rbuf[1025];
    int                socketFd;
    OSAL_NetAddress    recvAddr;
    int                recvSize;
    int                retVal;
    int                bytes;
    OSAL_TaskArg       taskArg;

    OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
    OSAL_logMsg("%s: D2 vPort %s\n\n", __FUNCTION__, D2_VPORT_REVISION);

    /* Start VPR */
    if (OSAL_FAIL == VPR_startDaemon()) {
        OSAL_logMsg("%s:%d VPR start failed.\n", __FILE__, __LINE__);
        exit(98);
        return (0);
    }

#ifdef VTSP_UT_FIXED_CMD
    taskArg = (OSAL_TaskArg) argv[1];
#else
    taskArg = NULL;
#endif
    UT_taskId = OSAL_taskCreate("utMain",
            OSAL_TASK_PRIO_NRT,
            8000, (OSAL_TaskPtr)UT_menu, taskArg);

    if (NULL == UT_taskId) {
        OSAL_logMsg("\n%s:%d  taskCreate fail\n\n", __FUNCTION__, __LINE__);
    }
    socketFd = 0;

    while (VAPP_run) {
        OSAL_taskDelay(500);
        if (socketFd) {
            OSAL_memSet(rbuf, 0, sizeof(rbuf));
            recvSize = sizeof(rbuf);
            if ((retVal = OSAL_netSocketReceiveFrom(&socketFd, (void *)rbuf,
                            (vint *)&recvSize, (OSAL_NetAddress *)&recvAddr))
                == OSAL_FAIL)
            {
                uint32 *word_ptr;
                for (bytes = 0; bytes < retVal; bytes += 4) {
                    word_ptr = (uint32 *)&rbuf[bytes];
                    OSAL_logMsg("%d\n%x", bytes, *(word_ptr + 0));
                }
            }
            else {
                /* No data from socket */
            }
        }
    }

    OSAL_taskDelay(1000);
    /* Stop VPR */
    if (OSAL_SUCCESS != VPR_stopDaemon()) {
        OSAL_logMsg("Failed to stop VPR.\n");
    }

    OSAL_logMsg("\n%s:%d  UT_main shutdown()\n\n", __FUNCTION__, __LINE__);
    //exit(98);

    return (0);
}

/*
 * ======== main ========
 * Unix main
 */
#if defined(OSAL_VXWORKS) || defined(OSAL_THREADX)
int vtsp_ut_main(int argc, char *argv_ptr[])
#else
OSAL_ENTRY
#endif
{

    OSAL_logMsg(
"\n"
"    ----------------------- VTSP Unit Test -----------------------\n"
"    COPYRIGHT 2004-2014 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED.\n"
"    %s\n", D2_VPORT_REVISION);

    OSAL_taskDelay(100);    /* cleans up printing */

    UT_main(argc, argv_ptr);

    OSAL_logMsg("%s exit\n", __func__);
    return (0);
}
#if (!defined(OSAL_VXWORKS)) && (!defined(OSAL_THREADX))
OSAL_EXIT
#endif
