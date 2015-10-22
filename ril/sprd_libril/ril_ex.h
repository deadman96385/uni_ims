/*
* Copyright (C) 2014 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef RIL_EX_H_INCLUDED
#define RIL_EX_H_INCLUDED

#include <telephony/sprd_ril.h>
#include <telephony/record_stream.h>
#include <binder/Parcel.h>


namespace android {

typedef struct {
    int requestNumber;
    void (*dispatchFunction) (Parcel &p, struct RequestInfo *pRI);
    int(*responseFunction) (Parcel &p, void *response, size_t responselen);
} CommandInfo;

typedef struct RequestInfo {
    int32_t token;      //this is not RIL_Token
    CommandInfo *pCI;
    struct RequestInfo *p_next;
    char cancelled;
    char local;         // responses to local commands do not go back to command process
} RequestInfo;

extern int modem;
#define RILLOGI(fmt, args...) ALOGI("[%c] " fmt, modem,  ## args)
#define RILLOGD(fmt, args...) ALOGD("[%c] " fmt, modem,  ## args)
#define RILLOGV(fmt, args...) ALOGV("[%c] " fmt, modem,  ## args)
#define RILLOGW(fmt, args...) ALOGW("[%c] " fmt, modem,  ## args)
#define RILLOGE(fmt, args...) ALOGE("[%c] " fmt, modem,  ## args)

extern RIL_RadioFunctions s_callbacks;
extern "C" const char *requestToString(int request);
extern "C" void invalidCommandBlock (RequestInfo *pRI);
extern "C" const char *callStateToString(RIL_CallState s);
extern "C" void stripNumberFromSipAddress(const char *sipAddress, char *number, int len);

// request, response, and unsolicited msg print macro
#define PRINTBUF_SIZE 8096
/* Negative values for private RIL errno's */
#define RIL_ERRNO_INVALID_RESPONSE -1
#define RILC_LOG 1

#if RILC_LOG
    extern char printBuf[PRINTBUF_SIZE];
#endif

#if RILC_LOG
    #define startRequest           sprintf(printBuf, "(")
    #define closeRequest           sprintf(printBuf, "%s)", printBuf)
    #define printRequest(token, req)           \
            RILLOGD("[%04d]> %s %s", token, requestToString(req), printBuf)

    #define startResponse           sprintf(printBuf, "%s {", printBuf)
    #define closeResponse           sprintf(printBuf, "%s}", printBuf)
    #define printResponse           RILLOGD("%s", printBuf)

    #define clearPrintBuf           printBuf[0] = 0
    #define removeLastChar          printBuf[strlen(printBuf)-1] = 0
    #define appendPrintBuf(x...)    sprintf(printBuf, x)
#else
    #define startRequest
    #define closeRequest
    #define printRequest(token, req)
    #define startResponse
    #define closeResponse
    #define printResponse
    #define clearPrintBuf
    #define removeLastChar
    #define appendPrintBuf(x...)
#endif

}

#define NUM_ELEMS_SOCKET(a)     (sizeof (a) / sizeof (a)[0])

void rilEventAddWakeup_helper(struct ril_event *ev);
void listenCallback_helper(int fd, short flags, void *param);
int blockingWrite_helper(int fd, const void* data, size_t len);

enum SocketWakeType {DONT_WAKE, WAKE_PARTIAL};

typedef struct SocketListenParam {
    RIL_SOCKET_ID socket_id;
    int fdListen;
    int fdCommand;
    char* processName;
    struct ril_event* commands_event;
    struct ril_event* listen_event;
    void (*processCommandsCallback)(int fd, short flags, void *param);
    RecordStream *p_rs;
    RIL_SOCKET_TYPE type;
} SocketListenParam;

#endif
