/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7544 $ $Date: 2008-09-05 19:45:05 -0400 (Fri, 05 Sep 2008) $
 *
 */

/*
 * Test control functions
 * Assumes the VTSP has already been initialized
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"
#include "osal_net.h"
#include <stdio.h>


/*
 * The following is defined to allow cut/paste from vapp_vtspev.c
 */
#define VAPP_eventConstant UT_eventConstant

#undef VAPP_PRINT_EVENT_RTP        /* use #undef to turn off */

typedef struct {
    VTSP_EventConstant   num;
    char                *name;
} UT_EventConstants;

typedef struct {
    VTSP_EventMsgCodes   num;
    char                *name;
} UT_EventCodes;

UT_EventConstants  UT_eventConstantTable[] = {
    { VTSP_EVENT_INACTIVE, "INACTIVE" },
    { VTSP_EVENT_ACTIVE, "ACTIVE" },
    { VTSP_EVENT_COMPLETE, "COMPLETE" },
    { VTSP_EVENT_HALTED, "HALTED" },
    { VTSP_EVENT_ERROR_INFC, "INFC" },
    { VTSP_EVENT_HOOK_SEIZE, "HOOK_SEIZE" },
    { VTSP_EVENT_HOOK_RELEASE, "HOOK_RELEASE" },
    { VTSP_EVENT_HOOK_FLASH, "HOOK_FLASH" },
    { VTSP_EVENT_MAX_TIMEOUT, "MAX_TIMEOUT" },
    { VTSP_EVENT_MAX_REPEAT, "MAX_REPEAT" },
    { VTSP_EVENT_OVERFLOW, "OVERFLOW" },
    { VTSP_EVENT_RTCP_BYE, "RTCP_BYE" },
    { VTSP_EVENT_RTCP_RR, "RTCP_RR" },
    { VTSP_EVENT_RTCP_SR, "RTCP_SR" },
    { VTSP_EVENT_RTCP_SS, "RTCP_SS" },
    { VTSP_EVENT_RTCP_MR, "RTCP_MR" },
    { VTSP_EVENT_RTCP_CS, "RTCP_CS" },
    { VTSP_EVENT_RTCP_XR, "RTCP_XS" },
    { VTSP_EVENT_HOOK_POLARITY_FWD, "VTSP_EVENT_HOOK_POLARITY_FWD" },
    { VTSP_EVENT_HOOK_POLARITY_REV, "VTSP_EVENT_HOOK_POLARITY_REV" },
    { VTSP_EVENT_HOOK_POWER_DOWN, "VTSP_EVENT_HOOK_POWER_DOWN" },
    { VTSP_EVENT_HOOK_DISCONNECT, "VTSP_EVENT_HOOK_DISCONNECT" },
    { VTSP_EVENT_HOOK_PULSE, "VTSP_EVENT_HOOK_PULSE" },
    { VTSP_EVENT_ERROR_RT, "VTSP_EVENT_ERROR_RT" },
    { VTSP_EVENT_ERROR_HW_THERMAL, "VTSP_EVENT_ERROR_THERMAL" },
    { VTSP_EVENT_ERROR_HW_POWER, "VTSP_EVENT_ERROR_POWER" },
    { VTSP_EVENT_ERROR_HW_RESET, "VSTP_EVENT_ERROR_RESET" },
    { VTSP_EVENT_TONE_DIR_LOCAL, "TONE_DIR_LOCAL" },
    { VTSP_EVENT_TONE_DIR_STREAM, "TONE_DIR_STREAM" },
    { VTSP_EVENT_REC_INFC_AVAILABLE, "RESOURCE_INFC_AVAILABLE" },
    { VTSP_EVENT_REC_INFC_UNAVAILABLE, "RESOURCE_INFC_UNAVAILABLE" },
    { VTSP_EVENT_REC_STREAM_AVAILABLE, "RESOURCE_STREAM_AVAILABLE" },
    { VTSP_EVENT_REC_STREAM_UNAVAILABLE, "RESOURCE_STREAM_UNAVAILABLE" },
    { VTSP_EVENT_HW_AUDIO_ATTACH, "HW_AUDIO_ATTACHED" },
    { VTSP_EVENT_HW_AUDIO_DETACH, "HW_AUDIO_DETACHED" }
};

UT_EventCodes  UT_eventCodeTable[] = {
    { VTSP_EVENT_MSG_CODE_TRACE, "EVENT TRACE" },
    { VTSP_EVENT_MSG_CODE_HOOK, "EVENT HOOK" },
    { VTSP_EVENT_MSG_CODE_TONE_GENERATE, "EVENT TONE_GENERATE" },
    { VTSP_EVENT_MSG_CODE_TONE_DETECT, "EVENT TONE_DETECT" },
    { VTSP_EVENT_MSG_CODE_DIGIT_GENERATE, "EVENT DIGIT_GENERATE" },
    { VTSP_EVENT_MSG_CODE_TIMER, "EVENT TIMER" },
    { VTSP_EVENT_MSG_CODE_BENCHMARK, "EVENT BENCHMARK" },
    { VTSP_EVENT_MSG_CODE_SHUTDOWN, "EVENT SHUTDOWN" },
    { VTSP_EVENT_MSG_CODE_ERROR, "EVENT ERROR" },
    { VTSP_EVENT_MSG_CODE_RTP, "EVENT RTP" },
    { VTSP_EVENT_MSG_CODE_JB, "EVENT JB" },
    { VTSP_EVENT_MSG_CODE_STATISTIC, "EVENT STAT" },
    { VTSP_EVENT_MSG_CODE_RTCP, "EVENT RTCP" },
    { VTSP_EVENT_MSG_CODE_RESOURCE, "EVENT RESOURCE" },
    { VTSP_EVENT_MSG_CODE_HW, "EVENT HARDWARE" },
};

/*
 * Look up event msg constant in table
 */
unsigned char *UT_eventConstant(
        vint arg)
{
    uvint entry;

    for (entry = 0; entry < sizeof(UT_eventConstantTable); entry++) {
        if (UT_eventConstantTable[entry].num == (uvint)arg) {
            return ((unsigned char *)UT_eventConstantTable[entry].name);
        }
    }

    return (NULL);
}

/*
 * Look up event msg code in table
 */
unsigned char *UT_eventCode(
    uvint arg)
{
    uvint code;

    for (code = 0; code < sizeof(UT_eventCodeTable); code++) {
        if (UT_eventCodeTable[code].num == arg) {
            return ((unsigned char *)UT_eventCodeTable[code].name);
        }
    }

    return (NULL);
}


UT_Return UT_processEvent(
    VTSP_EventMsg  *event_ptr)
{
    unsigned char  *code_ptr = NULL;
    unsigned char  *reason_ptr = NULL;
    unsigned char  *edge_ptr = NULL;
    uvint           parg1 = 0;
    uvint           parg2 = 0;
    uvint           parg3 = 0;
    uvint           parg4 = 0;
    uvint           parg5 = 0;

    code_ptr = UT_eventCode(event_ptr->code);

    switch (event_ptr->code) {
        case VTSP_EVENT_MSG_CODE_HOOK:
            reason_ptr = UT_eventConstant(event_ptr->msg.hook.reason);
            edge_ptr = UT_eventConstant(event_ptr->msg.hook.edgeType);
            break;

        case VTSP_EVENT_MSG_CODE_TONE_DETECT:
            reason_ptr = UT_eventConstant(event_ptr->msg.toneDetect.reason);
            edge_ptr = UT_eventConstant(event_ptr->msg.toneDetect.edgeType);
            parg3 = event_ptr->msg.toneDetect.tone;
            parg4 = event_ptr->msg.toneDetect.detect;
            break;
        case VTSP_EVENT_MSG_CODE_TONE_GENERATE:
            reason_ptr = UT_eventConstant(event_ptr->msg.toneGenerate.reason);
            edge_ptr = UT_eventConstant(event_ptr->msg.toneGenerate.direction);
            parg3 = event_ptr->msg.toneGenerate.streamId;
            break;

        case VTSP_EVENT_MSG_CODE_SHUTDOWN:
//        case VTSP_EVENT_MSG_CODE_ERROR:
            reason_ptr = UT_eventConstant(event_ptr->msg.debug.arg1);
            parg2 = event_ptr->msg.debug.arg2;
            parg3 = event_ptr->msg.debug.arg3;
            parg4 = event_ptr->msg.debug.arg4;
            break;

        case VTSP_EVENT_MSG_CODE_RTP:
#ifdef VAPP_PRINT_EVENT_RTP
            reason_ptr = UT_eventConstant(event_ptr->msg.rtp.reason);
            OSAL_logMsg("infc %d: %s streamId%d ssrc=0x%x\n", event_ptr->infc,
                    reason_ptr, event_ptr->msg.rtp.streamId,
                    event_ptr->msg.rtp.ssrc);
            switch (event_ptr->msg.rtp.reason) {
                case VTSP_EVENT_RTCP_RR:
                    OSAL_logMsg(
                            "  frac_cum_lost=0x%x hi_seq=0x%x jit=%d\n",
                            event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg3,
                            event_ptr->msg.rtp.arg4);
                    OSAL_logMsg(
                            "  rx_pkts=%d rx_bytes=%d\n",
                            event_ptr->msg.rtp.arg5, event_ptr->msg.rtp.arg6);
                    break;
                case VTSP_EVENT_RTCP_SR:
                    OSAL_logMsg(
                            "  ntp_hi=0x%x ntp_lo=0x%x rtp_ts=0x%x "
                            "tx_pkts=%d tx_bytes=%d\n",
                            event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg2,
                            event_ptr->msg.rtp.arg3, event_ptr->msg.rtp.arg4,
                            event_ptr->msg.rtp.arg5);
                    break;
                case VTSP_EVENT_RTCP_SS:
                    OSAL_logMsg(
                            "  flags=0x%x beg_end_seq=0x%x "
                            "lost_pkt=0x%x dup_pkt=%d\n",
                            event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg2,
                            event_ptr->msg.rtp.arg3, event_ptr->msg.rtp.arg4);
                    OSAL_logMsg(
                            "  mean_jit=%d min_jit=%d max_jit=%d dev_jit=%d\n",
                            event_ptr->msg.rtp.arg5, event_ptr->msg.rtp.arg6,
                            event_ptr->msg.rtp.arg7, event_ptr->msg.rtp.arg8);
                    break;
                case VTSP_EVENT_RTCP_MR:
                    OSAL_logMsg(
                            "  loss_rate=0x%x discard_rate=%d "
                            "end_sys_delay=%d\n", event_ptr->msg.rtp.arg1,
                            event_ptr->msg.rtp.arg2, event_ptr->msg.rtp.arg3);
                    OSAL_logMsg(
                            "  RERL=%d jb_nom=%d jb_max=%d jb_abs_max=%d\n",
                            event_ptr->msg.rtp.arg4, event_ptr->msg.rtp.arg5,
                            event_ptr->msg.rtp.arg6, event_ptr->msg.rtp.arg7);
                    break;
                case VTSP_EVENT_RTCP_CS:
                    OSAL_logMsg(
                            "  enc_bytes=0x%x dec_bytes=0x%x "
                            "enc_pkts=%d dec_pkts=%d\n",
                            event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg2,
                            event_ptr->msg.rtp.arg3, event_ptr->msg.rtp.arg4);
                    OSAL_logMsg(
                            "  cn_enc=%d cn_dec=%d plc_runs=%d nse_runs=%d\n",
                            event_ptr->msg.rtp.arg5, event_ptr->msg.rtp.arg6,
                            event_ptr->msg.rtp.arg7, event_ptr->msg.rtp.arg8);
                    OSAL_logMsg(
                            "  ticks=%d\n",
                            event_ptr->msg.rtp.arg9);
                    break;
            }
            return (UT_PASS);
#else
            reason_ptr = UT_eventConstant(event_ptr->msg.rtp.arg1);
            parg2 = event_ptr->msg.rtp.streamId;
            parg3 = event_ptr->msg.rtp.arg1;
            parg4 = event_ptr->msg.rtp.arg2;
#endif
            break;

        case VTSP_EVENT_MSG_CODE_JB:
            parg1 = event_ptr->msg.jb.streamId;
            parg2 = event_ptr->msg.jb.level;
            parg3 = event_ptr->msg.jb.drop;
            parg4 = event_ptr->msg.jb.plc;
#ifndef VAPP_MGMT_PRINT_ALL
            return (0); /* Do NOT print these events */
#endif
            break;

        case VTSP_EVENT_MSG_CODE_RTCP:
            reason_ptr = VAPP_eventConstant(event_ptr->msg.rtcp.reason);
            parg2 = event_ptr->msg.rtcp.ssrc;
            parg3 = event_ptr->msg.rtcp.arg1;
            parg4 = event_ptr->msg.rtcp.arg2;
            break;

        case VTSP_EVENT_MSG_CODE_TIMER:
#ifndef VAPP_MGMT_PRINT_ALL
            return (0); /* Do NOT print these events */
#endif
            break;
        case VTSP_EVENT_MSG_CODE_EC:
            reason_ptr = UT_eventConstant(event_ptr->msg.echoCanc.reason);
            parg2 = event_ptr->msg.echoCanc.status;
            parg3 = event_ptr->msg.echoCanc.rerl;
            break;
        case VTSP_EVENT_MSG_CODE_BENCHMARK:
#ifdef VTSP_ENABLE_BENCHMARK_1
            {
                uint32 parg1 = event_ptr->msg.benchmark.vtspr;
#ifdef VTSP_ENABLE_G729
                uint32 parg2 = event_ptr->msg.benchmark.g729Encode0;
                uint32 parg3 = event_ptr->msg.benchmark.g729Decode0;
                uint32 parg4 = event_ptr->msg.benchmark.g729Encode1;
                uint32 parg5 = event_ptr->msg.benchmark.g729Decode1;
#endif
#ifdef VTSP_ENABLE_ECSR
                uint32 parg6 = event_ptr->msg.benchmark.ecsr;
#endif
#ifdef VTSP_ENABLE_DTMF
                uint32 parg8 = event_ptr->msg.benchmark.dtmf;
#endif
                uint32 parg9 = event_ptr->msg.benchmark.tic;
                uint32 parg10 = event_ptr->msg.benchmark.total;
                uint32 parg11 = event_ptr->msg.benchmark.nfe;
                uint32 parg12 = event_ptr->msg.benchmark.dcrmPeer;
                uint32 parg13 = event_ptr->msg.benchmark.dcrmNear;
                uint32 parg14 = event_ptr->msg.benchmark.jbGet;
                uint32 parg15 = event_ptr->msg.benchmark.jbPut;
                uint32 parg17 = event_ptr->msg.benchmark.events;
                uint32 parg18 = event_ptr->msg.benchmark.cmd;
#ifdef VTSP_ENABLE_G711P1
                uint32 parg19 = event_ptr->msg.benchmark.g711p1Encode0;
                uint32 parg20 = event_ptr->msg.benchmark.g711p1Decode0;
                uint32 parg21 = event_ptr->msg.benchmark.g711p1Encode1;
                uint32 parg22 = event_ptr->msg.benchmark.g711p1Decode1;
#endif
#ifdef VTSP_ENABLE_ILBC
                uint32 parg23 = event_ptr->msg.benchmark.ilbcEncode0;
                uint32 parg24 = event_ptr->msg.benchmark.ilbcDecode0;
                uint32 parg25 = event_ptr->msg.benchmark.ilbcEncode1;
                uint32 parg26 = event_ptr->msg.benchmark.ilbcDecode1;
#endif
#ifdef VTSP_ENABLE_G722P1
                uint32 parg27 = event_ptr->msg.benchmark.g722p1Encode0;
                uint32 parg28 = event_ptr->msg.benchmark.g722p1Decode0;
                uint32 parg29 = event_ptr->msg.benchmark.g722p1Encode1;
                uint32 parg30 = event_ptr->msg.benchmark.g722p1Decode1;
#endif
#ifdef VTSP_ENABLE_G723
                uint32 parg31 = event_ptr->msg.benchmark.g723Encode0;
                uint32 parg32 = event_ptr->msg.benchmark.g723Decode0;
                uint32 parg33 = event_ptr->msg.benchmark.g723Encode1;
                uint32 parg34 = event_ptr->msg.benchmark.g723Decode1;
#endif
#ifdef VTSP_ENABLE_G722
                uint32 parg35 = event_ptr->msg.benchmark.g722Encode0;
                uint32 parg36 = event_ptr->msg.benchmark.g722Decode0;
                uint32 parg37 = event_ptr->msg.benchmark.g722Encode1;
                uint32 parg38 = event_ptr->msg.benchmark.g722Decode1;
#endif
#ifdef VTSP_ENABLE_G726
                uint32 parg39 = event_ptr->msg.benchmark.g726Encode0;
                uint32 parg40 = event_ptr->msg.benchmark.g726Decode0;
                uint32 parg41 = event_ptr->msg.benchmark.g726Encode1;
                uint32 parg42 = event_ptr->msg.benchmark.g726Decode1;
#endif
#ifdef VTSP_ENABLE_AEC
                uint32 parg43 = event_ptr->msg.benchmark.aecComputeRout;
                uint32 parg44 = event_ptr->msg.benchmark.aecComputeSout;
#endif
#ifdef VTSP_ENABLE_SILK_20MS
                uint32 parg45 = event_ptr->msg.benchmark.silkEncode0;
                uint32 parg46 = event_ptr->msg.benchmark.silkDecode0;
                uint32 parg47 = event_ptr->msg.benchmark.silkEncode1;
                uint32 parg48 = event_ptr->msg.benchmark.silkDecode1;
#endif
#ifdef VTSP_ENABLE_GAMRWB
                uint32 parg49 = event_ptr->msg.benchmark.gamrwbEncode0;
                uint32 parg50 = event_ptr->msg.benchmark.gamrwbDecode0;
                uint32 parg51 = event_ptr->msg.benchmark.gamrwbEncode1;
                uint32 parg52 = event_ptr->msg.benchmark.gamrwbDecode1;
#endif
                OSAL_logMsg("\ntotal-%dkHz,\tvtspr-%dkHz,\ttic-%dkHz\n",
                          parg10, parg1, parg9);
#ifdef VTSP_ENABLE_G729
                OSAL_logMsg("g729enc0-%dkHz,\tg729dec0-%dkHz,\n",
                       parg2, parg3);
                OSAL_logMsg("g729enc1-%dkHz,\tg729dec1-%dkHz\n",
                       parg4, parg5);
#endif
                OSAL_logMsg("nfe-%dkHz,\tdcrmPeer-%dkHz,\t"
                        "dcrmNeer-%dkHz\n", parg11, parg12, parg13);
                OSAL_logMsg("jbGet-%dkHz,\tjbPut-%dkHz,\t", parg14, parg15);

                OSAL_logMsg("events-%dkHz\tcmd-%dkHz\n",
                        parg17, parg18);
#ifdef VTSP_ENABLE_G711P1
                OSAL_logMsg("g711p1enc0-%dkHz,\tg711p1dec0-%dkHz,\n",
                       parg19, parg20);
                OSAL_logMsg("g711p1enc1-%dkHz,\tg711p1dec1-%dkHz\n",
                       parg21, parg22);
#endif
#ifdef VTSP_ENABLE_ILBC
                OSAL_logMsg("ilbcenc0-%dkHz,\tilbcdec0-%dkHz,\n",
                       parg23, parg24);
                OSAL_logMsg("ilbcenc1-%dkHz,\tilbcdec1-%dkHz\n",
                       parg25, parg26);
#endif
#ifdef VTSP_ENABLE_G722P1
                OSAL_logMsg("g722p1enc0-%dkHz,\tg722p1dec0-%dkHz,\n",
                       parg27, parg28);
                OSAL_logMsg("g722p1enc1-%dkHz,\tg722p1dec1-%dkHz\n",
                       parg29, parg30);
#endif
#ifdef VTSP_ENABLE_G723
                OSAL_logMsg("g723enc0-%dkHz,\tg723dec0-%dkHz,\n",
                       parg31, parg32);
                OSAL_logMsg("g723enc1-%dkHz,\tg723dec1-%dkHz\n",
                       parg33, parg34);
#endif
#ifdef VTSP_ENABLE_G722
                OSAL_logMsg("g722enc0-%dkHz,\tg722dec0-%dkHz,\n",
                       parg35, parg36);
                OSAL_logMsg("g722enc1-%dkHz,\tg722dec1-%dkHz\n",
                       parg37, parg38);
#endif
#ifdef VTSP_ENABLE_G726
                OSAL_logMsg("g726enc0-%dkHz,\tg726dec0-%dkHz,\n",
                       parg39, parg40);
                OSAL_logMsg("g726enc1-%dkHz,\tg726dec1-%dkHz\n",
                       parg41, parg42);
#endif
#ifdef VTSP_ENABLE_AEC
                OSAL_logMsg("aecRout-%dkHz,\taecSout-%dkHz,\n",
                       parg43, parg44);
#endif
#ifdef VTSP_ENABLE_SILK_20MS
                OSAL_logMsg("silkEnc0-%dkHz,\tsilkDec0-%dkHz,\n",
                       parg45, parg46);
                OSAL_logMsg("silkEnc1-%dkHz,\tsilkDec1-%dkHz\n",
                       parg47, parg48);
#endif
#ifdef VTSP_ENABLE_GAMRWB
                OSAL_logMsg("gamrwbenc0-%dkHz,\tgamrwbdec0-%dkHz,\n",
                       parg49, parg50);
                OSAL_logMsg("gamrwbenc1-%dkHz,\tgamrwbdec1-%dkHz\n",
                       parg51, parg52);
#endif
                if (parg10 >= 250000) {
                    OSAL_logMsg("Total: %dkHz\n", parg10);
                }

            }
#endif
            return (0);
            break;
        case VTSP_EVENT_MSG_CODE_RESOURCE:
            reason_ptr = UT_eventConstant(event_ptr->msg.resource.reason);
            parg2 = event_ptr->msg.resource.streamId;
            break;
        case VTSP_EVENT_MSG_CODE_HW:
            reason_ptr = UT_eventConstant(event_ptr->msg.hw.reason);
            break;
        default:
            parg1 = event_ptr->msg.debug.arg1;
            parg2 = event_ptr->msg.debug.arg2;
            parg3 = event_ptr->msg.debug.arg3;
            parg4 = event_ptr->msg.debug.arg4;
            break;
    }

    if (0 == UT_eventTaskDisplay) {
        return (UT_PASS);
    }

    if (code_ptr) {
        OSAL_logMsg("%s: %d ", __FILE__, __LINE__);
        OSAL_logMsg("infc %d: %s ",
                event_ptr->infc, code_ptr);
    }
    else {
        OSAL_logMsg("%s: %d ", __FILE__, __LINE__);
        OSAL_logMsg("infc %d: 0x%x ",
                event_ptr->infc, event_ptr->code);
    }

    if (reason_ptr && edge_ptr) {
        if (VTSP_EVENT_MSG_CODE_TONE_DETECT == event_ptr->code) {
            if (VTSP_DETECT_DTMF == parg4) {
                /* print the DTMF digit */
                OSAL_logMsg("%s %s '%c' %x(DTMF) \n",
                        reason_ptr, edge_ptr, parg3, parg4);
            }
            else if (VTSP_DETECT_UTD == parg4) {
                /* print the UTD tId */
                OSAL_logMsg("%s %s %d(tId) 0x%x(UTD) \n",
                        reason_ptr, edge_ptr, parg3, parg4);
            }
            else if (VTSP_DETECT_FMTD == parg4) {
                /* print the FMTD */
                OSAL_logMsg("%s %s 0x%x 0x%x(FMTD) \n",
                        reason_ptr, edge_ptr, parg3, parg4);
            }
            else {
                /* print the generic tone detect */
                OSAL_logMsg("%s %s 0x%x(tone) 0x%x(detect)\n",
                        reason_ptr, edge_ptr, parg3, parg4);
            }
        }
        else if (VTSP_EVENT_MSG_CODE_TONE_GENERATE == event_ptr->code) {
            /* print other tone detect */
            if (VTSP_EVENT_TONE_DIR_STREAM
                    == event_ptr->msg.toneGenerate.direction) {
                OSAL_logMsg("%s %s streamId=%d\n", reason_ptr,
                        edge_ptr, parg3);
            }
            else {
                OSAL_logMsg("%s %s\n", reason_ptr, edge_ptr);
            }
        }
        else {
            if (VTSP_EVENT_HOOK_PULSE == event_ptr->msg.hook.reason) {
                parg3 = event_ptr->msg.hook.digit;
            }
            /* print other tone detect */
            OSAL_logMsg("%s %s 0x%x 0x%x\n",
                    reason_ptr, edge_ptr, parg3, parg4);
        }
    }
    else if (reason_ptr) {
        OSAL_logMsg("%s 0x%x 0x%x 0x%x\n",
                reason_ptr, parg2, parg3, parg4);
    }
    else {
        OSAL_logMsg("0x%x 0x%x 0x%x 0x%x\n",
                parg1, parg2, parg3, parg4);
    }

    if (0 != parg5) {
        OSAL_logMsg("infc %d:   arg5=0x%x\n",
                event_ptr->infc, parg5);
    }

    return (UT_PASS);
}

