#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <utils/Log.h>
#include <telephony/sprd_ril.h>
#include <sprd_atchannel.h>
#include "sprd_ril_cb.h"

#define LOG_TAG "RIL"

static void onRequest (int request, void *data, size_t datalen, RIL_Token t);

static const RIL_RadioFunctions atch_RadioFuncs = {
    RIL_VERSION,
    onRequest,
    currentState,
    onSupports,
    onCancel,
    getVersion
};

static const struct RIL_Env *atch_rilenv;
#define sOnRequestComplete(t, e, response, responselen) atch_rilenv->OnRequestComplete(t,e, response, responselen)

#define RIL_REQUEST_SEND_CMD 1

const RIL_RadioFunctions *RIL_ATCI_Init(const struct RIL_Env *env, int argc, char **argv) {
    atch_rilenv = env;
    return &atch_RadioFuncs;
}

static void onRequest (int request, void *data, size_t datalen, RIL_Token t) {

    if(request == RIL_REQUEST_SEND_CMD) {
        int i, err;
        int channelID;
        ATResponse *p_response = NULL;
        ATLine *p_cur = NULL;

        char *at_cmd = (char *)data;
        char *response[1]={NULL};
        char buf[1024] = {0};

        if(at_cmd == NULL) {
            RILLOGE("Invalid AT command");
            return;
        }

        channelID = getChannel();
        err = at_send_command_multiline(ATch_type[channelID], at_cmd, "", &p_response);

        if (err < 0 || p_response->success == 0) {
            strlcat(buf, p_response->finalResponse, sizeof(buf));
            strlcat(buf, "\r\n", sizeof(buf));
            response[0] = buf;
            sOnRequestComplete(t, RIL_E_GENERIC_FAILURE, response, sizeof(char*));
        } else {
            p_cur = p_response->p_intermediates;
            for (i=0; p_cur != NULL; p_cur = p_cur->p_next,i++) {
                strlcat(buf, p_cur->line, sizeof(buf));
                strlcat(buf, "\r\n", sizeof(buf));
            }
            strlcat(buf, p_response->finalResponse, sizeof(buf));
            strlcat(buf, "\r\n", sizeof(buf));
            response[0] = buf;
            sOnRequestComplete(t, RIL_E_SUCCESS, response, sizeof(char*));
        }
        at_response_free(p_response);
        putChannel(channelID);
    }
}

