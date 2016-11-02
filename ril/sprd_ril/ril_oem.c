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
