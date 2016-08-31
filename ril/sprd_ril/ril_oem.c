#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <telephony/sprd_ril.h>
#include <telephony/sprd_ril_oem.h>
#include <sprd_atchannel.h>
#include "sprd_ril_cb.h"
#include "ril_call_blacklist.h"

#define LOG_TAG "RIL"

// {for get sim lock infors,like dummy,white list}
#define REQUEST_SIMLOCK_WHITE_LIST_PS 1
#define REQUEST_SIMLOCK_WHITE_LIST_PN 2
#define REQUEST_SIMLOCK_WHITE_LIST_PU 3
#define REQUEST_SIMLOCK_WHITE_LIST_PP 4
#define REQUEST_SIMLOCK_WHITE_LIST_PC 5

#if defined (RIL_SUPPORTED_OEM_PROTOBUF)

#define PROP_BUILD_TYPE "ro.build.type"

extern int s_isuserdebug;

static void requestSIMPower(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int *payload = (int*)(((OemRequest*)data)->payload);
    int onOff = payload[0];

    if (onOff == 0) {
        err = at_send_command(ATch_type[channelID], "AT+SFUN=3", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;
        setRadioState (channelID, RADIO_STATE_UNAVAILABLE);
    } else if (onOff > 0) {
        /* SPRD : for svlte & csfb @{ */
        setTestMode(channelID);
        /* @} */

        err = at_send_command(ATch_type[channelID], "AT+SFUN=2", &p_response);
        if (err < 0|| p_response->success == 0)
        goto error;
    }
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestGetRemainTimes(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char  cmd[20] = {0};
    char *line;
    int result, err;
    ATResponse *p_response = NULL;

    int *payload = (int*)(((OemRequest*)data)->payload);
    int type = payload[0];

    p_response = NULL;
    RILLOGD("[MBBMS]send RIL_REQUEST_GET_REMAIN_TIMES, type:%d",type);
    if (type >= 0 && type < 4) {
        snprintf(cmd, sizeof(cmd), "AT+XX=%d", type);
        err = at_send_command_singleline(ATch_type[channelID], cmd, "+XX:",
                &p_response);
        if (err < 0 || p_response->success == 0) {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        } else {
            line = p_response->p_intermediates->line;

            RILLOGD("[MBBMS]RIL_REQUEST_GET_REMAIN_TIMES: err=%d line=%s", err, line);

            err = at_tok_start(&line);
            if (err == 0) {
                err = at_tok_nextint(&line, &result);
                if (err == 0) {
                    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                    resp->funcId = OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES;
                    resp->subFuncId = 0;
                    resp->payload = (void*)(&result);
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, sizeof(result));
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            }
        }
        at_response_free(p_response);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

static void requestVideoPhoneDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_VideoPhone_Dial *p_dial;
    ATResponse   *p_response = NULL;
    int err;
    char *cmd;
    int ret;

    OemRequest *req = (OemRequest *)data;
    p_dial = (RIL_VideoPhone_Dial *)(req->payload);

#ifdef NEW_AT
    ret = asprintf(&cmd, "ATD=%s", p_dial->address);
#else
    ret = asprintf(&cmd, "AT^DVTDIAL=\"%s\"", p_dial->address);
#endif

    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

//  wait4android_audio_ready("ATD_VIDEO");
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

static void requestGetCurrentCalls(int channelID, void *data, size_t datalen, RIL_Token t, int bVideoCall)
{
    int err;
    ATResponse *p_response;
    ATLine *p_cur;
    int countCalls;
    int countValidCalls;
    RIL_Call *p_calls;
    RIL_Call **pp_calls;
    int i;
    int needRepoll = 0;

    err = at_send_command_multiline (ATch_type[channelID],"AT+CLCC", "+CLCC:", &p_response);
    if (err != 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        at_response_free(p_response);
        return;
    }

    /* count the calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
        countCalls++;
    }

    process_calls(countCalls);

    /* yes, there's an array of pointers and then an array of structures */

    pp_calls = (RIL_Call **)alloca(countCalls * sizeof(RIL_Call *));
    p_calls = (RIL_Call *)alloca(countCalls * sizeof(RIL_Call));
    memset (p_calls, 0, countCalls * sizeof(RIL_Call));

    /* init the pointer array */
    for(i = 0; i < countCalls ; i++) {
        pp_calls[i] = &(p_calls[i]);
    }

    for (countValidCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
        err = callFromCLCCLine(p_cur->line, p_calls + countValidCalls);

        if (err != 0) {
            continue;
        }
#if 0
        if (p_calls[countValidCalls].state != RIL_CALL_ACTIVE
                && p_calls[countValidCalls].state != RIL_CALL_HOLDING
           ) {
            needRepoll = 1;
        }
#endif
        countValidCalls++;
    }

    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
    resp->funcId = OEM_REQ_FUNCTION_ID_VIDEOPHONE;
    resp->subFuncId = OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS;
    resp->payload = (void*)pp_calls;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
            countValidCalls * sizeof (RIL_Call *));

    at_response_free(p_response);
#ifdef POLL_CALL_STATE
    if (countValidCalls)
    /* We don't seem to get a "NO CARRIER" message from
     * smd, so we're forced to poll until the call ends.
     */
#else
    if (needRepoll)
#endif
    {
        if (bVideoCall == 0) {
            RIL_requestTimedCallback (sendCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
        } else {
            RIL_requestTimedCallback (sendVideoCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
        }
    }
    return;
}

static void requestVideoPhone(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;

    OemRequest *videoPhoneReq = (OemRequest *)data;
    RILLOGD("Video Phone subFuncId: %d", videoPhoneReq->subFuncId);
    switch(videoPhoneReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_DIAL :
            requestVideoPhoneDial(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_CODEC:
            {
                char cmd[30] = {0};
                p_response = NULL;

                RIL_VideoPhone_Codec* p_codec = (RIL_VideoPhone_Codec*)(videoPhoneReq->payload);

                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTCODEC=%d", p_codec->type);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_HANGUP:
            {
                p_response = NULL;
#ifdef NEW_AT
                int reason = ((int*)(videoPhoneReq->payload))[0];
                if (reason < 0){
                    err = at_send_command(ATch_type[channelID], "ATH", &p_response);
                } else {
                    char cmd[20] = {0};
                    snprintf(cmd, sizeof(cmd), "ATH%d", reason);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                }
#else
                err = at_send_command(ATch_type[channelID], "AT^DVTEND", &p_response);
#endif

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_ANSWER:
            {
                p_response = NULL;
//              wait4android_audio_ready("ATA_VIDEO");
#ifdef NEW_AT
                err = at_send_command(ATch_type[channelID], "ATA", &p_response);
#else
                err = at_send_command(ATch_type[channelID], "AT^DVTANS", &p_response);
#endif

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_FALLBACK:
            {
                p_response = NULL;
                err = at_send_command(ATch_type[channelID], "AT"AT_PREFIX"DVTHUP", &p_response);

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_STRING:
            {
                char *cmd;
                int ret;
                p_response = NULL;
                char *str = (char*)(videoPhoneReq->payload);
                ret = asprintf(&cmd, "AT"AT_PREFIX"DVTSTRS=\"%s\"", str);
                if(ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                free(cmd);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_LOCAL_MEDIA:
            {
                char cmd[50] = {0};
                int *payload = (int*)(videoPhoneReq->payload);
                int datatype = payload[0];
                int sw = payload[1];

                if ((datalen/sizeof(int)) >2){
                    int indication = payload[2];
                    snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d,%d", datatype, sw, indication);
                } else {
                    snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d", datatype, sw);
                }
                p_response = NULL;
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_VIDEO:
            {
                char cmd[30];
                p_response = NULL;
                int payload = ((int*)(videoPhoneReq->payload))[0];
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTRECA=%d", payload);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_AUDIO:
            {
                char cmd[40];
                p_response = NULL;
                int *payload = (int*)(videoPhoneReq->payload);
                int on, mode;
                on = payload[0];
                if (datalen > 1) {
                    mode = payload[1];
                    snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d,%d", on, mode);
                } else {
                    snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d", on);
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_TEST:
            {
                char cmd[40];
                p_response = NULL;
                int *payload = (int*)(videoPhoneReq->payload);
                int flag = payload[0];
                int value = payload[1];

                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTTEST=%d,%d", flag, value);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS:
            requestGetCurrentCalls(channelID, data, datalen, t, 1);
            break;
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_IFRAME:
            {
                char cmd[50] = {0};
                p_response = NULL;
                int *payload = (int*)(videoPhoneReq->payload);
                int flag = payload[0];
                int value = payload[1];

                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTLFRAME=%d,%d", flag, value);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_SET_VOICERECORDTYPE:
            {
                char cmd[30] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT+SPRUDLV=%d", ((int*)(videoPhoneReq->payload))[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestQueryCOLRCOLP(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;

    OemRequest *queryReq = (OemRequest *)data;
    switch(queryReq->subFuncId) {
        case  OEM_REQ_SUBFUNCID_QUERY_COLP:
            {
                p_response = NULL;
                int response[2] = {0, 0};

                err = at_send_command_singleline(ATch_type[channelID], "AT+COLP?",
                        "+COLP: ", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response[0]);
                        if (err >= 0)
                            err = at_tok_nextint(&line, &response[1]);
                    }
                    if (err >= 0) {
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_QUERY_COLP_COLR;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_QUERY_COLP;
                        resp->payload = (void*)(&response[1]);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
                                sizeof(response[1]));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case  OEM_REQ_SUBFUNCID_QUERY_COLR:
            {
                p_response = NULL;
                int response[2] = {0, 0};

                err = at_send_command_singleline(ATch_type[channelID], "AT+COLR?",
                        "+COLR: ", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response[0]);
                        if (err >= 0)
                            err = at_tok_nextint(&line, &response[1]);
                    }
                    if (err >= 0) {
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_QUERY_COLP_COLR;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_QUERY_COLR;
                        resp->payload = (void*)(&response[1]);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
                                sizeof(response[1]));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestMBBMS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;

    OemRequest *mbbmsReq = (OemRequest *)data;
    switch(mbbmsReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_MBBMS_GSM_AUTHEN :
            {
                char *cmd;
                char *line;
                int ret;
                int err;
                p_response = NULL;

                char *str = (char*)(mbbmsReq->payload);
                RILLOGD("[MBBMS]send RIL_REQUEST_MBBMS_GSM_AUTHEN");
                ret = asprintf(&cmd, "AT^MBAU=\"%s\"",str);
                if(ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
                }
                err = at_send_command_singleline(ATch_type[channelID], cmd, "^MBAU:",
                        &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    RILLOGD("[MBBMS]RIL_REQUEST_MBBMS_GSM_AUTHEN: err=%d line=%s", err, line);
                    err = at_tok_start(&line);
                    if (err == 0) {
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_MBBMS;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_MBBMS_GSM_AUTHEN;
                        resp->payload = (void*)line;
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, strlen(line));
                    }
                    else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_MBBMS_USIM_AUTHEN :
            {
                char *cmd;
                char *line;
                int ret, err;
                p_response = NULL;
                RILLOGD("[MBBMS]send RIL_REQUEST_MBBMS_USIM_AUTHEN");
                char **payload = (char**)(mbbmsReq->payload);
                ret = asprintf(&cmd, "AT^MBAU=\"%s\",\"%s\"",payload[0], payload[1]);
                if(ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
                }
                err = at_send_command_singleline(ATch_type[channelID], cmd, "^MBAU:",
                        &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {

                    line = p_response->p_intermediates->line;
                    RILLOGD("[MBBMS]RIL_REQUEST_MBBMS_USIM_AUTHEN: err=%d line=%s", err, line);
                    err = at_tok_start(&line);
                    if (err == 0) {
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_MBBMS;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_MBBMS_USIM_AUTHEN;
                        resp->payload = (void*)line;
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, strlen(line));
                    }
                    else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_MBBMS_SIM_TYPE :
            {
                RIL_AppType app_type = RIL_APPTYPE_UNKNOWN;
                int card_type = 0;
                char str[15];

                RILLOGD("[MBBMS]RIL_REQUEST_MBBMS_SIM_TYPE");
                app_type = getSimType(channelID);
                if(app_type == RIL_APPTYPE_UNKNOWN)
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                else if(app_type == RIL_APPTYPE_USIM)
                    card_type = 1;
                else
                    card_type = 0;
                snprintf(str, sizeof(str), "%d", card_type);
                RILLOGD("[MBBMS]RIL_REQUEST_MBBMS_SIM_TYPE, card_type =%s", str);
                OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                resp->funcId = OEM_REQ_FUNCTION_ID_MBBMS;
                resp->subFuncId = OEM_REQ_SUBFUNCID_MBBMS_SIM_TYPE;
                resp->payload = (void*)str;
                RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, strlen(str));
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestSetCmms(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char cmd[20] = {0};
    OemRequest *req = (OemRequest *)data;
    int enable = ((int*)(req->payload))[0];

    snprintf(cmd, sizeof(cmd), "AT+CMMS=%d",enable);
    at_send_command( ATch_type[channelID], cmd, NULL);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

static void requestCallCsFallBack(int channelID, void *data, size_t datalen, RIL_Token t)
{
    OemRequest *callCSFBReq = (OemRequest *)data;
    switch(callCSFBReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_CALL_CSFALLBACK_ACCEPT:
            requestCallCsFallBackAccept(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_CALL_CSFALLBACK_REJECT:
            requestCallCsFallBackReject(channelID, data, datalen, t);
            break;
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestPriorityNetwork(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    OemRequest *pNReq = (OemRequest *)data;
    switch(pNReq->subFuncId) {
        if (sState == RADIO_STATE_OFF
                   && !(pNReq->subFuncId == OEM_REQ_SUBFUNCID_SET_PRIORITY_NETWORK_MODE)
           ) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }
        case OEM_REQ_SUBFUNCID_SET_PRIORITY_NETWORK_MODE:
            {
                if (s_testmode != 4 && s_testmode != 5
                        && s_testmode != 6 && s_testmode != 7 && s_testmode != 8) {
                    RILLOGE("no need set priority");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    break;
                }
                char cmd[30] = {0};

                p_response = NULL;
                /* AT^SYSCONFIG=<mode>,<acqorder>,<roam>,<srvdomain>
                 * mode: 17 -- LTE
                 * acqorder: 5:4G priority  6:2/3G priority
                 * roam: 2 -- no change
                 * srvdomain: 4 -- no change
                */

                // transfer rilconstan to at
                int order = 0;
                int type = ((int *)(pNReq->payload))[0];
                switch(type) {
                case 0: //4G priority
                    order = 5;
                    break;
                case 1://2/3G priority
                    order = 6;
                    break;
                }
                if (0 == order) {
                    RILLOGE("set priority network failed, order incorrect: %d", type);
                    break;
                }

                snprintf(cmd, sizeof(cmd), "AT^SYSCONFIG=17,%d,2,4", order);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                if(p_response)
                    at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_GET_PRIORITY_NETWORK_MODE:
            {
                p_response = NULL;
                int response[2] = {0, 0};

                err = at_send_command_singleline(ATch_type[channelID], "AT^SYSCONFIG?",
                        "^SYSCONFIG:", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response[0]);
                        if(err >= 0)
                        err = at_tok_nextint(&line, &response[1]);
                        // transfer at to rilconstant
                        int order = -1;
                        switch(response[1]) {
                            case 5:
                            order = 0; //4G priority network mode
                            break;
                            case 6:
                            order = 1;//2/3G priority network mode
                            break;
                        }
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_PRIORITY_NETWORK;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_GET_PRIORITY_NETWORK_MODE;
                        resp->payload = (void*)(&order);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
                                sizeof(order));
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestSwitch(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    OemRequest *switchReq = (OemRequest *)data;
    switch(switchReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_SWITCH_BAND_INFO_REPORT :
            {
                RILLOGD("enter to handle event: RIL_REQUEST_SWITCH_BAND_INFO_REPORT");
                p_response = NULL;
                int n = ((int*)(switchReq->payload))[0];
                char cmd[20] = {0};
                RILLOGD("RIL_REQUEST_SWITCH_BAND_INFO_REPORT to data value: %d", n);

                snprintf(cmd, sizeof(cmd), "AT+SPCLB=%d", n);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);

                if (err < 0 || p_response->success == 0) {
                    RILLOGD("response of RIL_REQUEST_SWITCH_BAND_INFO_REPORT: generic failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("response of RIL_REQUEST_SWITCH_BAND_INFO_REPORT: success!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_SWITCH_3_WIRE :
            {
                RILLOGD("enter to handle event: RIL_REQUEST_SWITCH_3_WIRE");
                p_response = NULL;
                int n = ((int*)(switchReq->payload))[0];
                char cmd[20] = {0};

                if (n == 0) {
                    n = 2; //close 3_wire coexistance(AT+SPCLB=2).
                } else if (n == 1) {
                    n = 3; //open 3_wire coexistance(AT+SPCLB=3).
                }
                RILLOGD("RIL_REQUEST_SWITCH_3_WIRE to data value: %d", n);
                snprintf(cmd, sizeof(cmd), "AT+SPCLB=%d", n);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);

                if (err < 0 || p_response->success == 0) {
                    RILLOGD("response of RIL_REQUEST_SWITCH_3_WIRE: generic failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("response of RIL_REQUEST_SWITCH_3_WIRE: success!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }

                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_SWITCH_BT :
            {
                RILLOGD("enter to handle event: RIL_REQUEST_SWITCH_BT");
                p_response = NULL;
                int n = ((int*)(switchReq->payload))[0];
                char cmd[20] = {0};

                if (n == 0) {
                    n = 6; //close BT coexistance(AT+SPCLB=6).
                } else if (n == 1) {
                    n = 7; //open BT coexistance(AT+SPCLB=7).
                }
                RILLOGD("RIL_REQUEST_SWITCH_BT to data value: %d", n);
                snprintf(cmd, sizeof(cmd), "AT+SPCLB=%d", n);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);

                if (err < 0 || p_response->success == 0) {
                    RILLOGD("response of RIL_REQUEST_SWITCH_BT: generic failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("response of RIL_REQUEST_SWITCH_BT: success!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }

                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_SWITCH_WIFI :
            {
                RILLOGD("enter to handle event: RIL_REQUEST_SWITCH_WIFI");
                p_response = NULL;
                int n = ((int*)(switchReq->payload))[0];
                char cmd[20] = {0};

                if (n == 0) {
                    n = 4; //close WIFI coexistance(AT+SPCLB=4).
                } else if (n == 1) {
                    n = 5; //open WIFI coexistance(AT+SPCLB=5).
                }
                RILLOGD("RIL_REQUEST_SWITCH_WIFI to data value: %d", n);
                snprintf(cmd, sizeof(cmd), "AT+SPCLB=%d", n);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);

                if (err < 0 || p_response->success == 0) {
                    RILLOGD("response of RIL_REQUEST_SWITCH_WIFI: generic failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("response of RIL_REQUEST_SWITCH_WIFI: success!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }

                at_response_free(p_response);
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestInitISIM(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err, response = 0;
    ATResponse *p_response = NULL;
    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));

    //SPRD: For WIFI get BandInfo report from modem, BRCM4343+9620, Zhanlei Feng added. 2014.06.20 END
    char *line;
    err = at_send_command_singleline(ATch_type[channelID], "AT+ISIM=1", "+ISIM:", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) {
        goto error;
    }
    err = at_tok_nextint(&line, &response);
    if (err < 0) {
        goto error;
    }

    resp->funcId = OEM_REQ_FUNCTION_ID_INIT_ISIM;
    resp->subFuncId = 0;
    resp->payload = (void*)(&response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, sizeof(int));
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    RILLOGE("INITISIM must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestIMS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    OemRequest *imsReq = (OemRequest *)data;

    switch(imsReq->subFuncId) {
        if (sState == RADIO_STATE_OFF
                   && (imsReq->subFuncId == OEM_REQ_SUBFUNCID_DISABLE_IMS)
           ) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }
        /* SPRD: add for VoLTE to handle Voice call Availability
         * AT+CAVIMS=<state>
         * state: integer type.The UEs IMS voice call availability status
         * 0, Voice calls with the IMS are not available.
         * 1, Voice calls with the IMS are available.
         * {@*/
        case OEM_REQ_SUBFUNCID_SET_IMS_VOICE_CALL_AVAILABILITY:
            {
                char cmd[20] = {0};
                p_response = NULL;
                int state = ((int*)(imsReq->payload))[0];

                snprintf(cmd, sizeof(cmd), "AT+CAVIMS=%d", state);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RILLOGD("SET_IMS_VOICE_CALL_AVAILABILITY:%d",state);
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("SET_IMS_VOICE_CALL_AVAILABILITY failure!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                if (p_response) {
                    at_response_free(p_response);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_GET_IMS_VOICE_CALL_AVAILABILITY:
            {
                p_response = NULL;
                int state = 0;

                err = at_send_command_singleline(ATch_type[channelID], "AT+CAVIMS?",
                        "+CAVIMS:", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &state);
                        RILLOGD("GET_IMS_VOICE_CALL_AVAILABILITY:%d",state);
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_IMS;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_GET_IMS_VOICE_CALL_AVAILABILITY;
                        resp->payload = (void*)(&state);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
                                sizeof(state));
                    }
                } else {
                    RILLOGD("GET_IMS_VOICE_CALL_AVAILABILITY failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPU:
            {
                char cmd[100] = {0};
                const char *impu = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS impu = \"%s\"", impu);
                snprintf(cmd, sizeof(cmd), "AT+IMPU=\"%s\"", impu);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPI :
            {
                char cmd[100] = {0};
                const char *impi = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS impi = \"%s\"", impi);
                snprintf(cmd, sizeof(cmd), "AT+IMPI=\"%s\"", impi);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_DOMAIN :
            {
                char cmd[100] = {0};
                const char *domain = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS domain = \"%s\"", domain);
                snprintf(cmd, sizeof(cmd), "AT+DOMAIN=\"%s\"", domain);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMEI :
            {
                char cmd[100] = {0};
                const char *impi = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS_IMEI instanceId = \"%s\"", impi);
                snprintf(cmd, sizeof(cmd), "AT+INSTANCEID=\"%s\"", impi);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_XCAP :
            {
                char cmd[100] = {0};
                const char *xcap = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS_XCAP xcap = \"%s\"", xcap);
                snprintf(cmd, sizeof(cmd), "AT+XCAPRTURI=\"%s\"", xcap);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_REGISTER_IMS_BSF :
            {
                char cmd[100] = {0};
                const char *bsf = (const char *)(imsReq->payload);

                RILLOGE("RIL_REQUEST_REGISTER_IMS_BSF bsf = \"%s\"", bsf);
                snprintf(cmd, sizeof(cmd), "AT+BSF=\"%s\"", bsf);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_DISABLE_IMS :
            {
                err = at_send_command(ATch_type[channelID], "AT+IMSEN=0" , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_SET_IMS_SMSC :
            {
                char *cmd;
                int ret;
                p_response = NULL;
                char *bsf = (const char *)(imsReq->payload);

                RILLOGD("[sms]RIL_REQUEST_SET_IMS_SMSC (%s)", bsf);
                ret = asprintf(&cmd, "AT+PSISMSC=\"%s\"", bsf);
                if (ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_SET_INITIAL_ATTACH_IMS_APN :
            {
                RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN");
                char cmd[128] = {0};
                char qos_state[PROPERTY_VALUE_MAX] = {0};
                int initial_attach_id = 11;
                RIL_InitialAttachApn *initialAttachIMSApn = NULL;
                p_response = NULL;
                if (data != NULL) {
                    initialAttachIMSApn = (RIL_InitialAttachApn *) (imsReq->payload);

                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN apn = %s",initialAttachIMSApn->apn);
                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN protocol = %s",initialAttachIMSApn->protocol);
                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN authtype = %d",initialAttachIMSApn->authtype);
                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN username = %s",initialAttachIMSApn->username);
                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN password = %s",initialAttachIMSApn->password);

                    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=%d,\"%s\",\"%s\",\"\",0,0",
                            initial_attach_id, initialAttachIMSApn->protocol,initialAttachIMSApn->apn);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);

                    snprintf(cmd, sizeof(cmd), "AT+CGPCO=0,\"%s\",\"%s\",%d,%d",
                            initialAttachIMSApn->username, initialAttachIMSApn->password,
                            initial_attach_id, initialAttachIMSApn->authtype);
                    err = at_send_command(ATch_type[channelID], cmd, NULL);

                    /* Set required QoS params to default */
                    property_get("persist.sys.qosstate", qos_state, "0");
                    if (!strcmp(qos_state, "0")) {
                        snprintf(cmd, sizeof(cmd),"AT+CGEQREQ=%d,2,0,0,0,0,2,0,\"1e4\",\"0e0\",3,0,0",initial_attach_id);
                        err = at_send_command(ATch_type[channelID], cmd, NULL);
                    }
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    at_response_free(p_response);
                } else {
                    RILLOGD("RIL_REQUEST_SET_INITIAL_ATTACH_IMS_APN data is null");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                break;
            }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestInitialGroupCall(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;
    const char *numbers = NULL;
    char cmd[PROPERTY_VALUE_MAX] = {0};
    OemRequest *req = (OemRequest*)data;
    char *str = (char*)(req->payload);
    numbers = (char*)strdup((char *)str);
    if (s_isuserdebug) {
        RILLOGE("requestInitialGroupCall numbers = \"%s\"", numbers);
    }
    snprintf(cmd, sizeof(cmd), "AT+CGU=1,\"%s\"", numbers);
    if (numbers != NULL) {
        free(numbers);
    }
    err = at_send_command(ATch_type[channelID], cmd , NULL);
    if (err < 0) {
        goto error;
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestAddGroupCall(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;
    const char *numbers = NULL;
    char cmd[PROPERTY_VALUE_MAX] = {0};
    OemRequest *req = (OemRequest*)data;
    char *str = (char*)(req->payload);
    numbers = (char*)strdup((char *)str);
    if (s_isuserdebug) {
        RILLOGE("requestAddGroupCall numbers = \"%s\"", numbers);
    }
    snprintf(cmd, sizeof(cmd), "AT+CGU=4,\"%s\"", numbers);
    if (numbers != NULL) {
        free(numbers);
    }
    err = at_send_command(ATch_type[channelID], cmd , NULL);
    if (err < 0) {
        goto error;
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestGetCurrentCallsVoLTE(int channelID, void *data, size_t datalen, RIL_Token t, int bVideoCall)
{
    int err;
    ATResponse *p_response;
    ATLine *p_cur;
    int countCalls;
    int countValidCalls;
    RIL_Call_VoLTE *p_calls;
    RIL_Call_VoLTE **pp_calls;
    int i;
    int needRepoll = 0;

    err = at_send_command_multiline (ATch_type[channelID],"AT+CLCCS", "+CLCCS:", &p_response);
    if (err != 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        at_response_free(p_response);
        return;
    }

    /* count the calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
        countCalls++;
    }

    process_calls(countCalls);

    /* yes, there's an array of pointers and then an array of structures */

    pp_calls = (RIL_Call_VoLTE **)alloca(countCalls * sizeof(RIL_Call_VoLTE *));
    p_calls = (RIL_Call_VoLTE *)alloca(countCalls * sizeof(RIL_Call_VoLTE));
    RIL_Call_VoLTE * p_t_calls = (RIL_Call_VoLTE *)alloca(countCalls * sizeof(RIL_Call_VoLTE));
    memset (p_calls, 0, countCalls * sizeof(RIL_Call_VoLTE));

    /* init the pointer array */
    for (i = 0; i < countCalls ; i++) {
        pp_calls[i] = &(p_calls[i]);
    }
    int groupCallIndex = 8;

    for (countValidCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
        err = callFromCLCCLineVoLTE(p_cur->line, p_calls + countValidCalls);
        p_t_calls = p_calls + countValidCalls;
        if (p_t_calls->mpty == 2) {
            if (groupCallIndex != 8) {
                p_t_calls->index = groupCallIndex;
            }
            groupCallIndex --;
        }

        if (err != 0) {
            continue;
        }
#if 0
        if (p_calls[countValidCalls].state != RIL_CALL_ACTIVE
                && p_calls[countValidCalls].state != RIL_CALL_HOLDING
           ) {
            needRepoll = 1;
        }
#endif
        countValidCalls++;
    }

    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
    resp->funcId = OEM_REQ_FUNCTION_ID_VOLTE;
    resp->subFuncId = OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE;
    resp->payload = (void*)pp_calls;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp,
            countValidCalls * sizeof (RIL_Call_VoLTE *));

    at_response_free(p_response);
#ifdef POLL_CALL_STATE
    if (countValidCalls)
    /* We don't seem to get a "NO CARRIER" message from
     * smd, so we're forced to poll until the call ends.
     */
#else
    if (needRepoll)
#endif
    {
        if (bVideoCall == 0) {
            RIL_requestTimedCallback (sendCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
        } else {
            RIL_requestTimedCallback (sendVideoCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
        }
    }
    return;
}

static void requestVolte(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    char versionStr[PROPERTY_VALUE_MAX];
    ATResponse *p_response = NULL;
    OemRequest *volteReq = (OemRequest *)data;
    property_get(PROP_BUILD_TYPE, versionStr, "user");
    if(strstr(versionStr, "userdebug")) {
        s_isuserdebug = 1;
    }
    switch (volteReq->subFuncId) {
        if (sState == RADIO_STATE_OFF
                   && !(volteReq->subFuncId == OEM_REQ_SUBFUNCID_VOLTE_SET_CONFERENCE_URI
                       || volteReq->subFuncId == OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE)
           ) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }
        case OEM_REQ_SUBFUNCID_VOLTE_CALL_FALL_BACK_TO_VOICE :
            {
                char cmd[30] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT+CCMMD=1,1,\"m=audio\"");
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_FALL_BACK_TO_VOICE failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_FALL_BACK_TO_VOICE success!");
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                if (p_response) {
                    at_response_free(p_response);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_VOLTE_INITIAL_GROUP_CALL :
            requestInitialGroupCall(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_VOLTE_ADD_TO_GROUP_CALL :
            requestAddGroupCall(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_VOLTE_SET_CONFERENCE_URI :
            {
                char cmd[100] = {0};
                const char *uri = (const char *)(volteReq->payload);

                RILLOGE("RIL_REQUEST_VOLTE_SET_CONFERENCE_URI uri = \"%s\"", uri);
                snprintf(cmd, sizeof(cmd), "AT+CONFURI=0,\"%s\"", uri);
                err = at_send_command(ATch_type[channelID], cmd , NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_VOLTE_CALL_REQUEST_MEDIA_CHANGE :
            {
                char cmd[30] = {0};
                p_response = NULL;
                int isVideo = ((int *)(volteReq->payload))[0];
                if (isVideo) {
                    snprintf(cmd, sizeof(cmd), "AT+CCMMD=1,2,\"m=audio\"");
                } else {
                    snprintf(cmd, sizeof(cmd), "AT+CCMMD=1,2,\"m=video\"");
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_REQUEST_MEDIA_CHANGE failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_REQUEST_MEDIA_CHANGE:%d",isVideo);
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                if (p_response) {
                    at_response_free(p_response);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_VOLTE_CALL_RESPONSE_MEDIA_CHANGE :
            {
                char cmd[20] = {0};
                p_response = NULL;
                int isAccept = ((int *)(volteReq->payload))[0];
                if (isAccept) {
                    snprintf(cmd, sizeof(cmd), "AT+CCMMD=1,3");
                } else {
                    snprintf(cmd, sizeof(cmd), "AT+CCMMD=1,4");
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_RESPONSE_MEDIA_CHANGE failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RILLOGD("RIL_REQUEST_VOLTE_CALL_RESPONSE_MEDIA_CHANGE:%d",isAccept);
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                if (p_response) {
                    at_response_free(p_response);
                }
                break;
            }
        case OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE :
            requestGetCurrentCallsVoLTE(channelID, data, datalen, t, 0);
            break;
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}
static void requestGetSimCapacity(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    char *line, *skip;
    int response[2] = {-1, -1};
    char *responseStr[2] = {NULL, NULL};
    char res[2][20] = {0};
    int i, result = 0;
    int err;

    for(i = 0; i < 2; i++) {
        responseStr[i] = res[i];
    }

    p_response = NULL;
    err = at_send_command_singleline(ATch_type[channelID], "AT+CPMS?", "+CPMS:",
            &p_response);
    if (err >= 0 && p_response->success) {
        line = p_response->p_intermediates->line;
        RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: err=%d line=%s", err, line);
        err = at_tok_start(&line);
        if (err >= 0) {
            err = at_tok_nextstr(&line, &skip);
            if (err == 0) {
                err = at_tok_nextint(&line, &response[0]);
                if (err == 0) {
                    err = at_tok_nextint(&line, &response[1]);
                    if (err == 0 && response[0] != -1 && response[1] != -1) {
                        result = 1;
                    }
                }
            }
        }
        RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: result=%d resp0=%d resp1=%d", result, response[0], response[1]);
        if (result == 1) {
            sprintf(res[0], "%d", response[0]);
            sprintf(res[1], "%d", response[1]);

            RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: str resp0=%s resp1=%s", res[0], res[1]);

            OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
            resp->funcId = OEM_REQ_FUNCTION_ID_GET_SIM_CAPACITY;
            resp->subFuncId = 0;
            resp->payload = (void*)responseStr;
            RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, 2*sizeof(char*));
        } else {
            RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        }
    } else {
        RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    at_response_free(p_response);
}



//SPRD add for simlock status begin
static void requestGetSimLockStatus(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err,skip,status;
    char *cmd, *line;
    int ret = -1;

    OemRequest *simlockReq = (OemRequest*)data;
    int fac = ((int*)(simlockReq->payload))[0];
    int ck_type = ((int*)(simlockReq->payload))[1];

    RILLOGD("data[0] = %d, data[1] = %d", fac, ck_type);

    ret = asprintf(&cmd, "AT+SPSMPN=%d,%d", fac, ck_type);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    RILLOGD("requestGetSimLockStatus: %s", cmd);

    err = at_send_command_singleline(ATch_type[channelID], cmd, "+SPSMPN:", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &skip);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &status);
    if (err < 0) goto error;

    RILLOGD("requestGetSimLockStatus fac = %d status = %d ", fac, status);

    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
    resp->funcId = OEM_REQ_FUNCTION_ID_SIMLOCK;
    resp->subFuncId = OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS;
    resp->payload = (void*)(&status);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, sizeof(status));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestGetSimLockDummys(int channelID, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err,i;
    char *line;
    int dummy[8];
    RILLOGD(" requestGetSimLockDummys");
    err = at_send_command_singleline(ATch_type[channelID],"AT+SPSLDUM?","+SPSLDUM:",&p_response);
    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    for(i = 0;i < 8;i++)
    {
        err = at_tok_nextint(&line, &dummy[i]);
        RILLOGD(" requestGetSimLockDummys dummy[%d] = ",dummy[i]);
        if (err < 0) goto error;
    }

    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
    resp->funcId = OEM_REQ_FUNCTION_ID_SIMLOCK;
    resp->subFuncId = OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS;
    resp->payload = (void*)dummy;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, sizeof(dummy));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestGetSimLockWhiteList(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err,i;
    int result;
    int status;
    char *cmd, *line,*mcc,*mnc,*plmn,*type_ret,*numlocks_ret;
    int errNum = -1;
    int ret = -1;
    int type,type_back,numlocks,mnc_digit;

    OemRequest *simlockReq = (OemRequest*)data;
    type = ((int*)(simlockReq->payload))[0];

    RILLOGD("requestGetSimLockWhiteList");
    ret = asprintf(&cmd, "AT+SPSMNW=%d,\"%s\",%d", type, "12345678", 1);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    err = at_send_command_singleline(ATch_type[channelID], cmd, "+SPSMNW:", &p_response);
    RILLOGD(" requestGetSimLockWhiteList cmd = %s",cmd);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &type_ret);
    if (err < 0) goto error;
    type_back = atoi(type_ret);
    RILLOGD(" requestGetSimLockWhiteList type_back = %d tpye_ret = %s",type_back,type_ret);

    err = at_tok_nextstr(&line, &numlocks_ret);
    if (err < 0 ) goto error;
    numlocks = atoi(numlocks_ret);
    RILLOGD(" requestGetSimLockWhiteList numlocks = %d numlocks_ret = %s",numlocks,numlocks_ret);
    if (numlocks < 0) goto error;

    switch (type_back) {
        case REQUEST_SIMLOCK_WHITE_LIST_PS:
        {
            char *imsi_len,*imsi_val[8];
            int imsi_index;
            plmn = (char*)alloca(sizeof(char)*numlocks*(19+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(19+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&imsi_len);
                strcat(plmn,imsi_len);
                if (err < 0) goto error;
                strcat(plmn,",");

                for(imsi_index = 0;imsi_index < 8;imsi_index++)
                {
                    err = at_tok_nextstr(&line,&imsi_val[imsi_index]);
                    if (err < 0) goto error;
                    int toFillLen = strlen(imsi_val[imsi_index]);
                    if (toFillLen == 1) {
                        strcat(plmn,"0");
                    }
                    strcat(plmn,imsi_val[imsi_index]);
                }

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PN:
            plmn = (char*)alloca(sizeof(char)*numlocks*(6+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(6+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        case REQUEST_SIMLOCK_WHITE_LIST_PU:
        {
            char *network_subset1,*network_subset2;
            plmn = (char*)alloca(sizeof(char)*numlocks*(8+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(8+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);

                err = at_tok_nextstr(&line,&network_subset1);
                if (err < 0) goto error;
                strcat(plmn,network_subset1);

                err = at_tok_nextstr(&line,&network_subset2);
                if (err < 0) goto error;
                strcat(plmn,network_subset2);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PP:
        {
            char *gid1;
            plmn = (char*)alloca(sizeof(char)*numlocks*(10+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(10+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid1);
                if (err < 0) goto error;
                strcat(plmn,gid1);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PC:
        {
            char *gid1,*gid2;
            plmn = (char*)alloca(sizeof(char)*numlocks*(14+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(14+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;
                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid1);
                if (err < 0) goto error;
                strcat(plmn,gid1);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid2);
                if (err < 0) goto error;
                strcat(plmn,gid2);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        default:
            goto error;
            break;
    }
    RILLOGD("telefk requestGetSimLockWhiteList plmn = %s",plmn);

    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
    resp->funcId = OEM_REQ_FUNCTION_ID_SIMLOCK;
    resp->subFuncId = OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST;
    resp->payload = (void*)plmn;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, strlen(plmn));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;

}

static void requestSimlock(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    OemRequest *simlockReq = (OemRequest *)data;
    switch(simlockReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES :
            {
                char cmd[20] = {0};
                int fac = ((int*)(simlockReq->payload))[0];
                int ck_type = ((int*)(simlockReq->payload))[1];
                char *line;
                int result[2] = {0,0};

                p_response = NULL;
                ALOGD("[MBBMS]send RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES, fac:%d,ck_type:%d",fac,ck_type);
                sprintf(cmd, "AT+SPSMPN=%d,%d", fac, ck_type);
                err = at_send_command_singleline(ATch_type[channelID], cmd, "+SPSMPN:", &p_response);

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    ALOGD("[MBBMS]RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES: err=%d line=%s", err, line);

                    err = at_tok_start(&line);

                    if (err == 0) {
                        err = at_tok_nextint(&line, &result[0]);
                        if (err == 0) {
                            at_tok_nextint(&line, &result[1]);
                            err = at_tok_nextint(&line, &result[1]);
                        }
                    }
                    if (err == 0) {
                        OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                        resp->funcId = OEM_REQ_FUNCTION_ID_SIMLOCK;
                        resp->subFuncId = OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES;
                        resp->payload = (void*)result;
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, sizeof(result));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS :
            requestGetSimLockStatus(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS :
            requestGetSimLockDummys(channelID, t);
            break;
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST :
            requestGetSimLockWhiteList(channelID, data, datalen, t);
            break;
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}
//SPRD add for simlock infors end

void requestOemHookRaw(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    OemRequest *req = (OemRequest *)data;
    RILLOGD("OEM_HOOK_RAW funcId: %d", req->funcId);
    switch (req->funcId) {
        if (sState == RADIO_STATE_UNAVAILABLE
                && !(req->funcId == OEM_REQ_FUNCTION_ID_SIM_POWER
                   || req->funcId == OEM_REQ_FUNCTION_ID_SIMLOCK
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
                   || req->funcId == OEM_REQ_FUNCTION_ID_CALL_BLACKLIST
#endif
                   )
       ) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }

        if (sState == RADIO_STATE_OFF
                && !(req->funcId == OEM_REQ_FUNCTION_ID_SIM_POWER
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
                   || req->funcId == OEM_REQ_FUNCTION_ID_CALL_BLACKLIST
#endif
                   || req->funcId == OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES
                   || req->funcId == OEM_REQ_FUNCTION_ID_SIMLOCK
                   || req->funcId == OEM_REQ_FUNCTION_ID_CSFALLBACK
                   || req->funcId == OEM_REQ_FUNCTION_ID_PRIORITY_NETWORK
                   || req->funcId == OEM_REQ_FUNCTION_ID_INIT_ISIM
                   || req->funcId == OEM_REQ_FUNCTION_ID_IMS
                   || req->funcId == OEM_REQ_FUNCTION_ID_VOLTE)
           ) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }

#if defined (RIL_SUPPORT_CALL_BLACKLIST)
        case OEM_REQ_FUNCTION_ID_CALL_BLACKLIST :
            requestCallBlackList(data, datalen, t);
            break;
#endif
        case OEM_REQ_FUNCTION_ID_SIM_POWER :
            requestSIMPower(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES :
            requestGetRemainTimes(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_STK_DIAL :
            {
                RILLOGD("requestDial isStkCall = %d", s_isstkcall);
                if (s_isstkcall == 1) {
                    RILLOGD(" setup STK call ");
                    s_isstkcall = 0;
            //      wait4android_audio_ready("ATD");
                    err = at_send_command(ATch_type[channelID], "AT+SPUSATCALLSETUP=1", NULL);
                    if (err != 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                break;
            }
        case OEM_REQ_FUNCTION_ID_VIDEOPHONE :
            requestVideoPhone(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_QUERY_COLP_COLR:
            requestQueryCOLRCOLP(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_MMI_ENTER_SIM:
            {
                char *cmd;
                int ret;
                char *str = (char*)(req->payload);

                p_response = NULL;
                RILLOGD("[SIM]send RIL_REQUEST_MMI_ENTER_SIM");
                ret = asprintf(&cmd, "ATD%s", str);
                if(ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
                    break;
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_FUNCTION_ID_MBBMS :
            requestMBBMS(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_GET_SIM_CAPACITY :
            requestGetSimCapacity(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_END_ALL_CONNECTIONS :
            {
                p_response = NULL;
                err = at_send_command(ATch_type[channelID], "ATH", &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_FUNCTION_ID_SET_CMMS :
            requestSetCmms(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_CSFALLBACK:
            requestCallCsFallBack(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_PRIORITY_NETWORK :
            requestPriorityNetwork(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_GET_BAND_INFO :
            {
                RILLOGD("enter to handle event: RIL_REQUEST_GET_BAND_INFO");
                p_response = NULL;
                char* line = NULL;
                err = at_send_command_singleline(ATch_type[channelID], "AT+SPCLB?","+SPCLB:", &p_response);

                if (err < 0 || p_response->success == 0) {
                    RILLOGD("response of RIL_REQUEST_GET_BAND_INFO: generic failure!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    RILLOGD("response of RIL_REQUEST_GET_BAND_INFO: %s", line);
                    //TODO: check the string of line, which is number.
                    OemRequest *resp = (OemRequest*)alloca(sizeof(OemRequest));
                    resp->funcId = OEM_REQ_FUNCTION_ID_GET_BAND_INFO;
                    resp->subFuncId = 0;
                    resp->payload = (void*)line;
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, resp, strlen(line));
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_FUNCTION_ID_SWITCH :
            requestSwitch(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_STOP_QUERY_AVAILABLE_NETWORKS :
            stopQueryNetwork(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_INIT_ISIM :
            requestInitISIM(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_IMS :
            requestIMS(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_VOLTE :
            requestVolte(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_ENABLE_BROADCAST_SMS :
            {
                char *cmd;
                int pri = ((int*)(req->payload))[0];
                int sec = ((int*)(req->payload))[1];
                int test = ((int*)(req->payload))[2];
                int cmas = ((int*)(req->payload))[3];
                int ret;
                RILLOGI("Reference-ril. requestEnableBroadcastSms %d ,%d ,%d ,%d", pri, sec, test, cmas);
                p_response = NULL;
                ret = asprintf(&cmd, "AT+SPPWS=%d,%d,%d,%d", pri, sec, test, cmas);
                if (ret < 0) {
                    RILLOGE("Failed to allocate memory");
                    cmd = NULL;
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
                }
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        //add for get simlock infors
        case OEM_REQ_FUNCTION_ID_SIMLOCK :
            requestSimlock(channelID, data, datalen, t);
            break;
        default :
            /* echo back data */
            requestSendAT(channelID, data, datalen, t);
            break;
    }
}
#endif

static void requestGetRemainTimes(int channelID, void *data,
                                      size_t datalen, RIL_Token t) {
    char  cmd[20] = {0};
    char *line;
    int result, err;
    ATResponse *p_response = NULL;

    int type = ((int *)data)[2];

    p_response = NULL;
    RILLOGD("[MBBMS]send RIL_REQUEST_GET_REMAIN_TIMES, type:%d",type);
    if (type >= 0 && type < 4) {
        snprintf(cmd, sizeof(cmd), "AT+XX=%d", type);
        err = at_send_command_singleline(ATch_type[channelID], cmd, "+XX:",
                &p_response);
        if (err < 0 || p_response->success == 0) {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        } else {
            line = p_response->p_intermediates->line;

            RILLOGD("[MBBMS]RIL_REQUEST_GET_REMAIN_TIMES: err=%d line=%s", err, line);

            err = at_tok_start(&line);
            if (err == 0) {
                err = at_tok_nextint(&line, &result);
                if (err == 0) {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            }
        }
        at_response_free(p_response);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

static void requestVideoPhone(int channelID, void *data, size_t datalen, RIL_Token t) {
    int err;
    ATResponse *p_response = NULL;

    OemRequest *videoPhoneReq = (OemRequest *)data;
    int len = videoPhoneReq->len;
    RILLOGD("Video Phone subFuncId: %d", videoPhoneReq->subFuncId);
    switch(videoPhoneReq->subFuncId) {
//        case OEM_REQ_SUBFUNCID_VIDEOPHONE_DIAL :
//            requestVideoPhoneDial(channelID, data, datalen, t);
//            break;
//        case OEM_REQ_SUBFUNCID_VIDEOPHONE_CODEC:
//            {
//                char cmd[30] = {0};
//                p_response = NULL;
//                int type = ((int*)data)[2];
//                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTCODEC=%d", type);
//                err = at_send_command(ATch_type[channelID], cmd, &p_response);
//                if (err < 0 || p_response->success == 0) {
//                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
//                } else {
//                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
//                }
//                at_response_free(p_response);
//                break;
//            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_HANGUP:
            {
                p_response = NULL;
#ifdef NEW_AT
                int reason = ((int*)data)[2];
                if (reason < 0){
                    err = at_send_command(ATch_type[channelID], "ATH", &p_response);
                } else {
                    char cmd[20] = {0};
                    snprintf(cmd, sizeof(cmd), "ATH%d", reason);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                }
#else
                err = at_send_command(ATch_type[channelID], "AT^DVTEND", &p_response);
#endif

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_ANSWER:
            {
                p_response = NULL;
//              wait4android_audio_ready("ATA_VIDEO");
#ifdef NEW_AT
                err = at_send_command(ATch_type[channelID], "ATA", &p_response);
#else
                err = at_send_command(ATch_type[channelID], "AT^DVTANS", &p_response);
#endif

                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_FALLBACK: {
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT"AT_PREFIX"DVTHUP", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_STRING: {
            char *cmd;
            int ret;
            p_response = NULL;
            char *payload = (char*)data;
            char *str = &payload[8];
            ret = asprintf(&cmd, "AT"AT_PREFIX"DVTSTRS=\"%s\"", str);
            if(ret < 0) {
                RILLOGE("Failed to allocate memory");
                cmd = NULL;
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                break;
            }
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            free(cmd);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_LOCAL_MEDIA: {
            char cmd[50] = {0};
            int datatype = ((int*)data)[2];
            int sw = ((int*)data)[3];
            if (datalen > 16){
                int indication = ((int*)data)[4];
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d,%d", datatype, sw, indication);
            } else {
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d", datatype, sw);
            }
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_VIDEO: {
            char cmd[30];
            p_response = NULL;
            int payload = ((int*)data)[2];
            snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTRECA=%d", payload);
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_AUDIO: {
            char cmd[40];
            p_response = NULL;
            int on, mode;
            on = ((int*)data)[2];
            if (datalen > 12) {
                mode = ((int*)data)[3];
                snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d,%d", on, mode);
            } else {
                snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d", on);
            }
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_TEST: {
            char cmd[40];
            p_response = NULL;
            int flag = ((int*)data)[2];
            int value = ((int*)data)[3];

            snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTTEST=%d,%d", flag, value);
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
//        case OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS:
//            requestGetCurrentCalls(channelID, data, datalen, t, 1);
//            break;
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_IFRAME: {
            char cmd[50] = {0};
            p_response = NULL;
            int flag = ((int*)data)[2];
            int value = ((int*)data)[3];

            snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTLFRAME=%d,%d", flag, value);
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_SET_VOICERECORDTYPE: {
            char cmd[30] = {0};
            p_response = NULL;
            snprintf(cmd, sizeof(cmd), "AT+SPRUDLV=%d", ((int*)data)[2]);
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestQueryCOLRCOLP(int channelID, void *data, size_t datalen, RIL_Token t) {
    int err;
    ATResponse *p_response = NULL;

    OemRequest *queryReq = (OemRequest *)data;
    switch(queryReq->subFuncId) {
        case  OEM_REQ_SUBFUNCID_QUERY_COLP: {
            p_response = NULL;
            int response[2] = {0, 0};

            err = at_send_command_singleline(ATch_type[channelID], "AT+COLP?",
                    "+COLP: ", &p_response);
            if (err >= 0 && p_response->success) {
                char *line = p_response->p_intermediates->line;
                err = at_tok_start(&line);
                if (err >= 0) {
                    err = at_tok_nextint(&line, &response[0]);
                    if (err >= 0)
                        err = at_tok_nextint(&line, &response[1]);
                }
                if (err >= 0) {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response[1],
                            sizeof(response[1]));
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            } else {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case  OEM_REQ_SUBFUNCID_QUERY_COLR: {
            p_response = NULL;
            int response[2] = {0, 0};

            err = at_send_command_singleline(ATch_type[channelID], "AT+COLR?",
                    "+COLR: ", &p_response);
            if (err >= 0 && p_response->success) {
                char *line = p_response->p_intermediates->line;
                err = at_tok_start(&line);
                if (err >= 0) {
                    err = at_tok_nextint(&line, &response[0]);
                    if (err >= 0)
                        err = at_tok_nextint(&line, &response[1]);
                }
                if (err >= 0) {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response[1],
                            sizeof(response[1]));
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            } else {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        default :
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

static void requestGetSimLockStatus(int channelID, void *data, size_t datalen, RIL_Token t) {
    ATResponse *p_response = NULL;
    int err,skip,status;
    char *cmd, *line;
    int ret = -1;

    int fac = ((int *)(data))[2];
    int ck_type = ((int *)(data))[3];

    RILLOGD("data[0] = %d, data[1] = %d", fac, ck_type);

    ret = asprintf(&cmd, "AT+SPSMPN=%d,%d", fac, ck_type);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    RILLOGD("requestGetSimLockStatus: %s", cmd);

    err = at_send_command_singleline(ATch_type[channelID], cmd, "+SPSMPN:", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &skip);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &status);
    if (err < 0) goto error;

    RILLOGD("requestGetSimLockStatus fac = %d status = %d ", fac, status);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &status, sizeof(status));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestGetSimLockDummys(int channelID, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err,i;
    char *line;
    int dummy[8];
    RILLOGD(" requestGetSimLockDummys");
    err = at_send_command_singleline(ATch_type[channelID],"AT+SPSLDUM?","+SPSLDUM:",&p_response);
    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    for(i = 0;i < 8;i++)
    {
        err = at_tok_nextint(&line, &dummy[i]);
        RILLOGD(" requestGetSimLockDummys dummy[%d] = ",dummy[i]);
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, dummy, sizeof(dummy));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestGetSimLockWhiteList(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err,i;
    int result;
    int status;
    char *cmd, *line,*mcc,*mnc,*plmn,*type_ret,*numlocks_ret;
    int errNum = -1;
    int ret = -1;
    int type,type_back,numlocks,mnc_digit;

    type = ((int*)data)[2];

    RILLOGD("requestGetSimLockWhiteList");
    ret = asprintf(&cmd, "AT+SPSMNW=%d,\"%s\",%d", type, "12345678", 1);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    err = at_send_command_singleline(ATch_type[channelID], cmd, "+spsmnw:", &p_response);
    RILLOGD(" requestGetSimLockWhiteList cmd = %s",cmd);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &type_ret);
    if (err < 0) goto error;
    type_back = atoi(type_ret);
    RILLOGD(" requestGetSimLockWhiteList type_back = %d tpye_ret = %s",type_back,type_ret);

    err = at_tok_nextstr(&line, &numlocks_ret);
    if (err < 0 ) goto error;
    numlocks = atoi(numlocks_ret);
    RILLOGD(" requestGetSimLockWhiteList numlocks = %d numlocks_ret = %s",numlocks,numlocks_ret);
    if (numlocks < 0) goto error;

    switch (type_back) {
        case REQUEST_SIMLOCK_WHITE_LIST_PS:
        {
            char *imsi_len,*imsi_val[8];
            int imsi_index;
            plmn = (char*)alloca(sizeof(char)*numlocks*(19+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(19+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&imsi_len);
                strcat(plmn,imsi_len);
                if (err < 0) goto error;
                strcat(plmn,",");

                for(imsi_index = 0;imsi_index < 8;imsi_index++)
                {
                    err = at_tok_nextstr(&line,&imsi_val[imsi_index]);
                    if (err < 0) goto error;
                    int toFillLen = strlen(imsi_val[imsi_index]);
                    if (toFillLen == 1) {
                        strcat(plmn,"0");
                    }
                    strcat(plmn,imsi_val[imsi_index]);
                }

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PN:
            plmn = (char*)alloca(sizeof(char)*numlocks*(6+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(6+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        case REQUEST_SIMLOCK_WHITE_LIST_PU:
        {
            char *network_subset1,*network_subset2;
            plmn = (char*)alloca(sizeof(char)*numlocks*(8+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(8+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);

                err = at_tok_nextstr(&line,&network_subset1);
                if (err < 0) goto error;
                strcat(plmn,network_subset1);

                err = at_tok_nextstr(&line,&network_subset2);
                if (err < 0) goto error;
                strcat(plmn,network_subset2);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PP:
        {
            char *gid1;
            plmn = (char*)alloca(sizeof(char)*numlocks*(10+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(10+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;

                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid1);
                if (err < 0) goto error;
                strcat(plmn,gid1);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        case REQUEST_SIMLOCK_WHITE_LIST_PC:
        {
            char *gid1,*gid2;
            plmn = (char*)alloca(sizeof(char)*numlocks*(14+1)+5);
            memset(plmn, 0, sizeof(char)*numlocks*(14+1)+5);
            strcat(plmn,type_ret);
            strcat(plmn,",");
            strcat(plmn,numlocks_ret);
            strcat(plmn,",");
            for(i = 0;i < numlocks;i++)
            {
                err = at_tok_nextstr(&line,&mcc);
                if (err < 0) goto error;
                //add for test sim card:mcc=001
                if (strlen(mcc) == 1) {
                    strcat(plmn,"00");
                } else if (strlen(mcc) == 2) {
                    strcat(plmn,"0");
                }
                strcat(plmn,mcc);

                err = at_tok_nextstr(&line,&mnc);
                if (err < 0) goto error;
                err = at_tok_nextint(&line, &mnc_digit);
                if (err < 0) goto error;
                int toFillLen = mnc_digit - strlen(mnc);
                if (toFillLen == 1) {
                    strcat(plmn,"0");
                } else if (toFillLen == 2) {
                    strcat(plmn,"00");
                }
                strcat(plmn,mnc);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid1);
                if (err < 0) goto error;
                strcat(plmn,gid1);
                strcat(plmn,",");

                err = at_tok_nextstr(&line,&gid2);
                if (err < 0) goto error;
                strcat(plmn,gid2);

                if ((i + 1) < numlocks) strcat(plmn,",");
            }
            break;
        }
        default:
            goto error;
            break;
    }
    RILLOGD("telefk requestGetSimLockWhiteList plmn = %s",plmn);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, plmn, strlen(plmn)+1);
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;

}

void requestSimLock(int channelID, void *data, size_t datalen, RIL_Token t) {
    int err;
    ATResponse *p_response = NULL;

    OemRequest *simlockReq = (OemRequest *)data;
    switch (simlockReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES: {
            char cmd[20] = {0};
            int fac = ((int*)data)[2];
            int ck_type = ((int*)data)[3];
            char *line;
            int result[2] = {0,0};

            p_response = NULL;
            ALOGD("[MBBMS]send RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES, fac:%d,ck_type:%d",fac,ck_type);
            sprintf(cmd, "AT+SPSMPN=%d,%d", fac, ck_type);
            err = at_send_command_singleline(ATch_type[channelID], cmd, "+SPSMPN:", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                line = p_response->p_intermediates->line;
                ALOGD("[MBBMS]RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES: err=%d line=%s", err, line);

                err = at_tok_start(&line);

                if (err == 0) {
                    err = at_tok_nextint(&line, &result[0]);
                    if (err == 0) {
                        at_tok_nextint(&line, &result[1]);
                        err = at_tok_nextint(&line, &result[1]);
                    }
                }

                if (err == 0) {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS:
            requestGetSimLockStatus(channelID, data, datalen, t);
            break;
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS :
            requestGetSimLockDummys(channelID, t);
            break;
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST :
            requestGetSimLockWhiteList(channelID, data, datalen, t);
            break;
        default:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void requestOemHookRaw(int channelID, void *data, size_t datalen, RIL_Token t) {
    ATResponse *p_response = NULL;
    int err;
    OemRequest *req = (OemRequest *)data;
    RILLOGD("OEM_HOOK_RAW funcId: %d", req->funcId);
    switch (req->funcId) {
        if (sState == RADIO_STATE_UNAVAILABLE) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }

        if (sState == RADIO_STATE_OFF
                && !(req->funcId == OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES
                   || req->funcId == OEM_REQ_FUNCTION_ID_SIMLOCK)) {
            RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
            break;
        }

        case OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES :
            requestGetRemainTimes(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_VIDEOPHONE :
            requestVideoPhone(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_QUERY_COLP_COLR:
            requestQueryCOLRCOLP(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_MMI_ENTER_SIM: {
            char *cmd;
            int ret;
            char *payload = (char *)data;
            char *str = &payload[8];
            p_response = NULL;
            RILLOGD("[SIM]send RIL_REQUEST_MMI_ENTER_SIM");
            ret = asprintf(&cmd, "ATD%s", str);
            if(ret < 0) {
                RILLOGE("Failed to allocate memory");
                cmd = NULL;
                RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
                break;
            }
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            free(cmd);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        case OEM_REQ_FUNCTION_ID_SIMLOCK:
            requestSimLock(channelID, data, datalen, t);
            break;
        case OEM_REQ_FUNCTION_ID_ENABLE_BROADCAST_SMS: {
            char *cmd;
            int pri = ((int *)data)[2];
            int sec = ((int *)data)[3];
            int test = ((int *)data)[4];
            int cmas = ((int *)data)[5];
            int ret;
            RILLOGI("Reference-ril. requestEnableBroadcastSms %d ,%d ,%d ,%d", pri, sec, test, cmas);
            p_response = NULL;
            ret = asprintf(&cmd, "AT+SPPWS=%d,%d,%d,%d", pri, sec, test, cmas);
            if (ret < 0) {
                RILLOGE("Failed to allocate memory");
                cmd = NULL;
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                break;
            }
            err = at_send_command(ATch_type[channelID], cmd, &p_response);
            free(cmd);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }
        default:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}
