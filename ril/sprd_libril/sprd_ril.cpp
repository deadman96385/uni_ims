/* //vendor/sprd/proprietories-source/ril/libril/sprd_ril.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "RILC"

#include <hardware_legacy/power.h>
#include <telephony/sprd_thread_pool.h>
#include <telephony/sprd_ril.h>
#include <telephony/ril_cdma_sms.h>
#include <cutils/sockets.h>
#include <cutils/jstring.h>
#include <telephony/record_stream.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <pthread.h>
#include <binder/Parcel.h>
#include <cutils/jstring.h>

#include <sys/types.h>
#include <sys/limits.h>
#include <pwd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <alloca.h>
#include <sys/un.h>
#include <assert.h>
#include <netinet/in.h>
#include <cutils/properties.h>

#include <ril_event.h>

namespace android {

#define PHONE_PROCESS "radio"

#if 0
#define SOCKET_NAME_RIL "rild"
#endif
#define SOCKET_NAME_RIL_DEBUG "rild-debug"


int modem;
#define RILLOGI(fmt, args...) ALOGI("[%c] " fmt, modem,  ## args)
#define RILLOGD(fmt, args...) ALOGD("[%c] " fmt, modem,  ## args)
#define RILLOGV(fmt, args...) ALOGV("[%c] " fmt, modem,  ## args)
#define RILLOGW(fmt, args...) ALOGW("[%c] " fmt, modem,  ## args)
#define RILLOGE(fmt, args...) ALOGE("[%c] " fmt, modem,  ## args)

#define ANDROID_WAKE_LOCK_NAME "radio-interface"


#define PROPERTY_RIL_IMPL "gsm.version.ril-impl"

// match with constant in RIL.java
#define MAX_COMMAND_BYTES (8 * 1024)

// Basically: memset buffers that the client library
// shouldn't be using anymore in an attempt to find
// memory usage issues sooner.
#define MEMSET_FREED 1

#define NUM_ELEMS(a)     (sizeof (a) / sizeof (a)[0])

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Constants for response types */
#define RESPONSE_SOLICITED 0
#define RESPONSE_UNSOLICITED 1

/* Negative values for private RIL errno's */
#define RIL_ERRNO_INVALID_RESPONSE -1

// request, response, and unsolicited msg print macro
#define PRINTBUF_SIZE 8096

// Enable RILC log
#define RILC_LOG 1
#define THR_MAX            3
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

enum WakeType {DONT_WAKE, WAKE_PARTIAL};

typedef struct {
    int requestNumber;
    void (*dispatchFunction) (Parcel &p, struct RequestInfo *pRI);
    int(*responseFunction) (Parcel &p, void *response, size_t responselen);
} CommandInfo;

typedef struct {
    int requestNumber;
    int (*responseFunction) (Parcel &p, void *response, size_t responselen);
    WakeType wakeType;
} UnsolResponseInfo;

typedef struct RequestInfo {
    int32_t token;      //this is not RIL_Token
    CommandInfo *pCI;
    struct RequestInfo *p_next;
    char cancelled;
    char local;         // responses to local commands do not go back to command process
} RequestInfo;

typedef struct UserCallbackInfo {
    RIL_TimedCallback p_callback;
    void *userParam;
    struct ril_event event;
    struct UserCallbackInfo *p_next;
} UserCallbackInfo;


/*******************************************************************/

RIL_RadioFunctions s_callbacks = {0, NULL, NULL, NULL, NULL, NULL};
static int s_registerCalled = 0;

static pthread_t s_tid_dispatch;
static pthread_t s_tid_reader;
static int s_started = 0;

static int s_fdListen = -1;
static int s_fdCommand = -1;
static int s_fdDebug = -1;

/* Command thread local data */
struct commthread_data_t {
    pthread_mutex_t threadmutex;
    RecordStream *p_rs;
    char *buffer;
    size_t buflen;
};

threadpool_t *threadpool_d;
static pthread_t s_tid_sms;
static pthread_t s_tid_local;

static int s_fdWakeupRead;
static int s_fdWakeupWrite;

static struct ril_event s_commands_event;
static struct ril_event s_wakeupfd_event;
static struct ril_event s_listen_event;
static struct ril_event s_wake_timeout_event;
static struct ril_event s_debug_event;


static const struct timeval TIMEVAL_WAKE_TIMEOUT = {1,0};

static pthread_mutex_t s_pendingRequestsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_startupMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_startupCond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t s_dispatchMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_dispatchCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_listMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_localDispatchMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_localDispatchCond = PTHREAD_COND_INITIALIZER;

static RequestInfo *s_pendingRequests = NULL;

static RequestInfo *s_toDispatchHead = NULL;
static RequestInfo *s_toDispatchTail = NULL;

static UserCallbackInfo *s_last_wake_timeout_info = NULL;

static void *s_lastNITZTimeData = NULL;
static size_t s_lastNITZTimeDataSize;

#define TD_SIM_NUM  "ro.modem.t.count"
#define W_SIM_NUM  "ro.modem.w.count"
#define L_SIM_NUM  "ro.modem.l.count"
#define TL_SIM_NUM  "ro.modem.tl.count"
#define LF_SIM_NUM  "ro.modem.lf.count"

static int s_multiSimMode;
const char * s_modem = NULL;
static int s_sim_num;


#if RILC_LOG
    static char printBuf[PRINTBUF_SIZE];
#endif

struct listnode
{
    commthread_data_t *user_data;
    struct listnode *next;
    struct listnode *prev;
};

#define list_for_each(node, list) \
    for (node = (list)->next; node != (list); node = node->next)

struct listnode sms_cmd_list;
struct listnode other_cmd_list;
struct listnode local_cmd_list;

void list_init(struct listnode *list);
void list_add_tail(struct listnode *list, struct listnode *item);
void list_add_head(struct listnode *list, struct listnode *item);
void list_remove(struct listnode *item);

/*******************************************************************/

static void dispatchVoid (Parcel& p, RequestInfo *pRI);
static void dispatchString (Parcel& p, RequestInfo *pRI);
static void dispatchStrings (Parcel& p, RequestInfo *pRI);
static void dispatchInts (Parcel& p, RequestInfo *pRI);
static void dispatchDial (Parcel& p, RequestInfo *pRI);
static void dispatchSIM_IO (Parcel& p, RequestInfo *pRI);
static void dispatchSIM_APDU (Parcel& p, RequestInfo *pRI);
static void dispatchCallForward(Parcel& p, RequestInfo *pRI);
static void dispatchRaw(Parcel& p, RequestInfo *pRI);
static void dispatchSmsWrite (Parcel &p, RequestInfo *pRI);
static void dispatchDataCall (Parcel& p, RequestInfo *pRI);
static void dispatchVoiceRadioTech (Parcel& p, RequestInfo *pRI);
static void dispatchSetInitialAttachApn (Parcel& p, RequestInfo *pRI);
static void dispatchCdmaSubscriptionSource (Parcel& p, RequestInfo *pRI);

static void dispatchCdmaSms(Parcel &p, RequestInfo *pRI);
static void dispatchImsSms(Parcel &p, RequestInfo *pRI);
static void dispatchImsCdmaSms(Parcel &p, RequestInfo *pRI, uint8_t retry, int32_t messageRef);
static void dispatchImsGsmSms(Parcel &p, RequestInfo *pRI, uint8_t retry, int32_t messageRef);
static void dispatchCdmaSmsAck(Parcel &p, RequestInfo *pRI);
static void dispatchGsmBrSmsCnf(Parcel &p, RequestInfo *pRI);
static void dispatchCdmaBrSmsCnf(Parcel &p, RequestInfo *pRI);
static void dispatchRilCdmaSmsWriteArgs(Parcel &p, RequestInfo *pRI);
static void dispatchNVReadItem(Parcel &p, RequestInfo *pRI);
static void dispatchNVWriteItem(Parcel &p, RequestInfo *pRI);
static void dispatchUiccSubscripton(Parcel &p, RequestInfo *pRI);
static void dispatchSimAuthentication(Parcel &p, RequestInfo *pRI);
static void dispatchDataProfile(Parcel &p, RequestInfo *pRI);
static void dispatchRadioCapability(Parcel &p, RequestInfo *pRI);
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static void dispatchImsSendSms(Parcel& p, RequestInfo *pRI);
static void dispatchSetUiccSub(Parcel& p, RequestInfo *pRI);
static void dispatchSetCBConf(Parcel& p, RequestInfo *pRI);
static void dispatchSendUssd(Parcel& p, RequestInfo *pRI);
static void dispatchGetPB(Parcel& p, RequestInfo *pRI);
static void dispatchAccessPB(Parcel& p, RequestInfo *pRI);
static void dispatchEccDial(Parcel& p, RequestInfo *pRI);
#endif
#if defined (RIL_SPRD_EXTENSION)
static void dispatchNetworkList (Parcel &p, RequestInfo *pRI);
static void dispatchVideoPhoneInit(Parcel& p, RequestInfo *pRI);
static void dispatchVideoPhoneDial(Parcel& p, RequestInfo *pRI);
static void dispatchVideoPhoneCodec(Parcel& p, RequestInfo *pRI);
#endif


static int responseInts(Parcel &p, void *response, size_t responselen);
static int responseStrings(Parcel &p, void *response, size_t responselen);
static int responseString(Parcel &p, void *response, size_t responselen);
static int responseVoid(Parcel &p, void *response, size_t responselen);
static int responseCallList(Parcel &p, void *response, size_t responselen);
static int responseSMS(Parcel &p, void *response, size_t responselen);
static int responseSIM_IO(Parcel &p, void *response, size_t responselen);
static int responseCallForwards(Parcel &p, void *response, size_t responselen);
static int responseDataCallList(Parcel &p, void *response, size_t responselen);
static int responseSetupDataCall(Parcel &p, void *response, size_t responselen);
static int responseRaw(Parcel &p, void *response, size_t responselen);
static int responseSsn(Parcel &p, void *response, size_t responselen);
static int responseSimStatus(Parcel &p, void *response, size_t responselen);
static int responseGsmBrSmsCnf(Parcel &p, void *response, size_t responselen);
static int responseCdmaBrSmsCnf(Parcel &p, void *response, size_t responselen);
static int responseCdmaSms(Parcel &p, void *response, size_t responselen);
static int responseCellList(Parcel &p, void *response, size_t responselen);
static int responseCdmaInformationRecords(Parcel &p,void *response, size_t responselen);
static int responseRilSignalStrength(Parcel &p,void *response, size_t responselen);
static int responseCallRing(Parcel &p, void *response, size_t responselen);
static int responseCdmaSignalInfoRecord(Parcel &p,void *response, size_t responselen);
static int responseCdmaCallWaiting(Parcel &p,void *response, size_t responselen);
static int responseSimRefresh(Parcel &p, void *response, size_t responselen);
static int responseCellInfoList(Parcel &p, void *response, size_t responselen);
static int responseHardwareConfig(Parcel &p, void *response, size_t responselen);
static int responseDcRtInfo(Parcel &p, void *response, size_t responselen);
static int responseRadioCapability(Parcel &p, void *response, size_t responselen);
static int responseSSData(Parcel &p, void *response, size_t responselen);
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static int responseImsSendSms(Parcel &p, void *response, size_t responselen);
static int responseDataCallProfile(Parcel &p, void *response, size_t responselen);
static int responseGetUiccSub(Parcel &p, void *response, size_t responselen);
static int responseGetCBConf(Parcel &p, void *response, size_t responselen);
static int responseGetPB(Parcel &p, void *response, size_t responselen);
static int responseSS(Parcel &p, void *response, size_t responselen);
static int responseLockInfo(Parcel &p, void *response, size_t responselen);
static int responsemsg(Parcel &p, void *response, size_t responselen);
static int responseCCresult(Parcel &p, void *response, size_t responselen);
#endif
#if defined (RIL_SPRD_EXTENSION)
static int responseDSCI(Parcel &p, void *response, size_t responselen);
static int responseCallCsFallBack(Parcel &p, void *response, size_t responselen);
#endif

static int decodeVoiceRadioTechnology (RIL_RadioState radioState);
static int decodeCdmaSubscriptionSource (RIL_RadioState radioState);
static RIL_RadioState processRadioState(RIL_RadioState newRadioState);

static bool isServiceTypeCfQuery(RIL_SsServiceType serType, RIL_SsRequestType reqType);

extern "C" const char * requestToString(int request);
extern "C" const char * failCauseToString(RIL_Errno);
extern "C" const char * callStateToString(RIL_CallState);
extern "C" const char * radioStateToString(RIL_RadioState);

#ifdef RIL_SHLIB
extern "C" void RIL_onUnsolicitedResponse(int unsolResponse, void *data,
                                size_t datalen);

extern "C" void
RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response,
                                size_t responselen);
#endif

#if defined(ANDROID_MULTI_SIM)
#define RIL_UNSOL_RESPONSE(a, b, c, d) RIL_onUnsolicitedResponse((a), (b), (c), (d))
#define CALL_ONREQUEST(a, b, c, d, e) s_callbacks.onRequest((a), (b), (c), (d), (e))
#define CALL_ONSTATEREQUEST(a) s_callbacks.onStateRequest(a)
#else
#define RIL_UNSOL_RESPONSE(a, b, c, d) RIL_onUnsolicitedResponse((a), (b), (c))
#define CALL_ONREQUEST(a, b, c, d, e) s_callbacks.onRequest((a), (b), (c), (d))
#define CALL_ONSTATEREQUEST(a) s_callbacks.onStateRequest()
#endif

static UserCallbackInfo * internalRequestTimedCallback
    (RIL_TimedCallback callback, void *param, const struct timeval *relativeTime);

static void internalRemoveTimedCallback(void *callbackInfo);

/** Index == requestNumber */
static CommandInfo s_commands[] = {
#include "sprd_ril_commands.h"
};

static UnsolResponseInfo s_unsolResponses[] = {
#include "sprd_ril_unsol_commands.h"
};

/* For older RILs that do not support new commands RIL_REQUEST_VOICE_RADIO_TECH and
   RIL_UNSOL_VOICE_RADIO_TECH_CHANGED messages, decode the voice radio tech from
   radio state message and store it. Every time there is a change in Radio State
   check to see if voice radio tech changes and notify telephony
 */
int voiceRadioTech = -1;

/* For older RILs that do not support new commands RIL_REQUEST_GET_CDMA_SUBSCRIPTION_SOURCE
   and RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED messages, decode the subscription
   source from radio state and store it. Every time there is a change in Radio State
   check to see if subscription source changed and notify telephony
 */
int cdmaSubscriptionSource = -1;

/* For older RILs that do not send RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, decode the
   SIM/RUIM state from radio state and store it. Every time there is a change in Radio State,
   check to see if SIM/RUIM status changed and notify telephony
 */
int simRuimStatus = -1;

static char *
strdupReadString(Parcel &p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p.readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}

static status_t
readStringFromParcelInplace(Parcel &p, char *str, size_t maxLen) {
    size_t s16Len;
    const char16_t *s16;

    s16 = p.readString16Inplace(&s16Len);
    if (s16 == NULL) {
        return NO_MEMORY;
    }
    size_t strLen = strnlen16to8(s16, s16Len);
    if ((strLen + 1) > maxLen) {
        return NO_MEMORY;
    }
    if (strncpy16to8(str, s16, strLen) == NULL) {
        return NO_MEMORY;
    } else {
        return NO_ERROR;
    }
}

static void writeStringToParcel(Parcel &p, const char *s) {
    char16_t *s16;
    size_t s16_len;
    s16 = strdup8to16(s, &s16_len);
    p.writeString16(s16, s16_len);
    free(s16);
}


static void
memsetString (char *s) {
    if (s != NULL) {
        memset (s, 0, strlen(s));
    }
}

void   nullParcelReleaseFunction (const uint8_t* data, size_t dataSize,
                                    const size_t* objects, size_t objectsSize,
                                        void* cookie) {
    // do nothing -- the data reference lives longer than the Parcel object
}

/**
 * To be called from dispatch thread
 * Issue a single local request, ensuring that the response
 * is not sent back up to the command process
 */
static void
issueLocalRequest(int request, void *data, int len) {
    RequestInfo *pRI;
    int ret;

    pRI = (RequestInfo *)calloc(1, sizeof(RequestInfo));

    pRI->local = 1;
    pRI->token = 0xffffffff;        // token is not used in this context

    if (request < 1
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
        || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
        || (request > RIL_SPRD_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#else
        || (request > RIL_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#endif
        || (request > RIL_OEM_REQUEST_LAST)
#else
#if defined (RIL_SPRD_EXTENSION)
        || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
        || (request > RIL_SPRD_REQUEST_LAST)
#else
        || request >= (int)NUM_ELEMS(s_commands)
#endif
#endif
    ) {
        RILLOGE("invalid request %d", request);
        exit(-1);
    }

#if defined (RIL_SPRD_EXTENSION)
    if(request > RIL_SPRD_REQUEST_BASE && request <= RIL_SPRD_REQUEST_LAST)
        request = request - RIL_SPRD_REQUEST_BASE + RIL_REQUEST_LAST;
#endif

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(request > RIL_OEM_REQUEST_BASE && request <= RIL_OEM_REQUEST_LAST)
#if defined (RIL_SPRD_EXTENSION)
        request = request - RIL_OEM_REQUEST_BASE +
                             RIL_REQUEST_LAST + RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE;
#else
        request = request - RIL_OEM_REQUEST_BASE + RIL_REQUEST_LAST;
#endif
#endif

    pRI->pCI = &(s_commands[request]);

    ret = pthread_mutex_lock(&s_pendingRequestsMutex);
    assert (ret == 0);

    pRI->p_next = s_pendingRequests;
    s_pendingRequests = pRI;

    ret = pthread_mutex_unlock(&s_pendingRequestsMutex);
    assert (ret == 0);

    RILLOGD("C[locl]> %s", requestToString(request));

    s_callbacks.onRequest(request, data, len, pRI);
}



static int
processCommandBuffer(void *buffer, size_t buflen) {
    Parcel p;
    status_t status;
    int32_t request;
    int32_t token;
    RequestInfo *pRI;
    int ret;

    p.setData((uint8_t *) buffer, buflen);

    // status checked at end
    status = p.readInt32(&request);
    status = p.readInt32 (&token);
    RILLOGD("PCB request code %d token %d", request, token);
    if (status != NO_ERROR) {
        RILLOGE("invalid request block");
        return 0;
    }

    if (request < 1
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
        || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
        || (request > RIL_SPRD_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#else
        || (request > RIL_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#endif
        || (request > RIL_OEM_REQUEST_LAST)
#else
#if defined (RIL_SPRD_EXTENSION)
        || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
        || (request > RIL_SPRD_REQUEST_LAST)
#else
        || request >= (int32_t)NUM_ELEMS(s_commands)
#endif
#endif
    ) {
        RILLOGE("unsupported request code %d token %d", request, token);
        // FIXME this should perhaps return a response
        return 0;
    }

#if defined (RIL_SPRD_EXTENSION)
    if(request > RIL_SPRD_REQUEST_BASE && request <= RIL_SPRD_REQUEST_LAST)
        request = request - RIL_SPRD_REQUEST_BASE + RIL_REQUEST_LAST;
#endif

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(request > RIL_OEM_REQUEST_BASE && request <= RIL_OEM_REQUEST_LAST)
#if defined (RIL_SPRD_EXTENSION)
        request = request - RIL_OEM_REQUEST_BASE + RIL_REQUEST_LAST
                             + RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE;
#else
        request = request - RIL_OEM_REQUEST_BASE + RIL_REQUEST_LAST;
#endif
#endif

    pRI = (RequestInfo *)calloc(1, sizeof(RequestInfo));

    pRI->token = token;
    pRI->pCI = &(s_commands[request]);

    ret = pthread_mutex_lock(&s_pendingRequestsMutex);
    assert (ret == 0);

    pRI->p_next = s_pendingRequests;
    s_pendingRequests = pRI;

    ret = pthread_mutex_unlock(&s_pendingRequestsMutex);
    assert (ret == 0);

/*    sLastDispatchedToken = token; */
    pRI->pCI->dispatchFunction(p, pRI);

    return 0;
}

static void
invalidCommandBlock (RequestInfo *pRI) {
    RILLOGE("invalid command block for token %d request %s",
                pRI->token, requestToString(pRI->pCI->requestNumber));
}

/** Callee expects NULL */
static void
dispatchVoid (Parcel& p, RequestInfo *pRI) {
    clearPrintBuf;
    printRequest(pRI->token, pRI->pCI->requestNumber);
    s_callbacks.onRequest(pRI->pCI->requestNumber, NULL, 0, pRI);
}

/** Callee expects const char * */
static void
dispatchString (Parcel& p, RequestInfo *pRI) {
    status_t status;
    size_t datalen;
    size_t stringlen;
    char *string8 = NULL;

    string8 = strdupReadString(p);

    startRequest;
    appendPrintBuf("%s%s", printBuf, string8);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, string8,
                       strlen(string8)+1, pRI);

#ifdef MEMSET_FREED
    memsetString(string8);
#endif

    free(string8);
    return;
}

/** Callee expects const char ** */
static void
dispatchStrings (Parcel &p, RequestInfo *pRI) {
    int32_t countStrings;
    status_t status;
    size_t datalen;
    char **pStrings;

    status = p.readInt32 (&countStrings);

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    if (countStrings == 0) {
        // just some non-null pointer
        pStrings = (char **)alloca(sizeof(char *));
        datalen = 0;
    } else if (((int)countStrings) == -1) {
        pStrings = NULL;
        datalen = 0;
    } else {
        datalen = sizeof(char *) * countStrings;

        pStrings = (char **)alloca(datalen);

        for (int i = 0 ; i < countStrings ; i++) {
            pStrings[i] = strdupReadString(p);
            appendPrintBuf("%s%s,", printBuf, pStrings[i]);
        }
    }
    removeLastChar;
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, pStrings, datalen, pRI);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
#ifdef MEMSET_FREED
            memsetString (pStrings[i]);
#endif
            free(pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, datalen);
#endif
    }

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchRadioCapability(Parcel &p, RequestInfo *pRI){
    RIL_RadioCapability rc;
    int32_t t;
    status_t status;

    memset (&rc, 0, sizeof(RIL_RadioCapability));

    status = p.readInt32(&t);
    rc.version = (int)t;
    if (status != NO_ERROR) {
        goto invalid;
    }

    status = p.readInt32(&t);
    rc.session= (int)t;
    if (status != NO_ERROR) {
        goto invalid;
    }

    status = p.readInt32(&t);
    rc.phase= (int)t;
    if (status != NO_ERROR) {
        goto invalid;
    }

    status = p.readInt32(&t);
    rc.rat = (int)t;
    if (status != NO_ERROR) {
        goto invalid;
    }

    status = readStringFromParcelInplace(p, rc.logicalModemUuid, sizeof(rc.logicalModemUuid));
    if (status != NO_ERROR) {
        goto invalid;
    }

    status = p.readInt32(&t);
    rc.status = (int)t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%s [version:%d, session:%d, phase:%d, rat:%d, \
            logicalModemUuid:%s, status:%d", printBuf, rc.version, rc.session,
            rc.phase, rc.rat, rc.logicalModemUuid, rc.session);

    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    CALL_ONREQUEST(pRI->pCI->requestNumber,
                &rc,
                sizeof(RIL_RadioCapability),
                pRI, pRI->socket_id);
    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

/** Callee expects const int * */
static void
dispatchInts (Parcel &p, RequestInfo *pRI) {
    int32_t count;
    status_t status;
    size_t datalen;
    int *pInts;

    status = p.readInt32 (&count);

    if (status != NO_ERROR || count == 0) {
        goto invalid;
    }

    datalen = sizeof(int) * count;
    pInts = (int *)alloca(datalen);

    startRequest;
    for (int i = 0 ; i < count ; i++) {
        int32_t t;

        status = p.readInt32(&t);
        pInts[i] = (int)t;
        appendPrintBuf("%s%d,", printBuf, t);

        if (status != NO_ERROR) {
            goto invalid;
        }
   }
   removeLastChar;
   closeRequest;
   printRequest(pRI->token, pRI->pCI->requestNumber);

   s_callbacks.onRequest(pRI->pCI->requestNumber, const_cast<int *>(pInts),
                       datalen, pRI);

#ifdef MEMSET_FREED
    memset(pInts, 0, datalen);
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}


/**
 * Callee expects const RIL_SMS_WriteArgs *
 * Payload is:
 *   int32_t status
 *   String pdu
 */
static void
dispatchSmsWrite (Parcel &p, RequestInfo *pRI) {
    RIL_SMS_WriteArgs args;
    int32_t t;
    status_t status;

    memset (&args, 0, sizeof(args));

    status = p.readInt32(&t);
    args.status = (int)t;

    args.pdu = strdupReadString(p);

    if (status != NO_ERROR || args.pdu == NULL) {
        goto invalid;
    }

    args.smsc = strdupReadString(p);

    startRequest;
    appendPrintBuf("%s%d,%s,smsc=%s", printBuf, args.status,
        (char*)args.pdu,  (char*)args.smsc);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &args, sizeof(args), pRI);

#ifdef MEMSET_FREED
    memsetString (args.pdu);
#endif

    free (args.pdu);

#ifdef MEMSET_FREED
    memset(&args, 0, sizeof(args));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

/**
 * Callee expects const RIL_Dial *
 * Payload is:
 *   String address
 *   int32_t clir
 */
static void
dispatchDial (Parcel &p, RequestInfo *pRI) {
    RIL_Dial dial;
    RIL_UUS_Info uusInfo;
    int32_t sizeOfDial;
    int32_t t;
    int32_t uusPresent;
    status_t status;

    memset (&dial, 0, sizeof(dial));

    dial.address = strdupReadString(p);

    status = p.readInt32(&t);
    dial.clir = (int)t;

    if (status != NO_ERROR || dial.address == NULL) {
        goto invalid;
    }

    if (s_callbacks.version < 3) { // Remove when partners upgrade to version 3
        uusPresent = 0;
        sizeOfDial = sizeof(dial) - sizeof(RIL_UUS_Info *);
    } else {
        status = p.readInt32(&uusPresent);

        if (status != NO_ERROR) {
            goto invalid;
        }

        if (uusPresent == 0) {
            dial.uusInfo = NULL;
        } else {
            int32_t len;

            memset(&uusInfo, 0, sizeof(RIL_UUS_Info));

            status = p.readInt32(&t);
            uusInfo.uusType = (RIL_UUS_Type) t;

            status = p.readInt32(&t);
            uusInfo.uusDcs = (RIL_UUS_DCS) t;

            status = p.readInt32(&len);
            if (status != NO_ERROR) {
                goto invalid;
            }

            // The java code writes -1 for null arrays
            if (((int) len) == -1) {
                uusInfo.uusData = NULL;
                len = 0;
            } else {
                uusInfo.uusData = (char*) p.readInplace(len);
            }

            uusInfo.uusLength = len;
            dial.uusInfo = &uusInfo;
        }
        sizeOfDial = sizeof(dial);
    }

    startRequest;
    appendPrintBuf("%snum=%s,clir=%d", printBuf, dial.address, dial.clir);
    if (uusPresent) {
        appendPrintBuf("%s,uusType=%d,uusDcs=%d,uusLen=%d", printBuf,
                dial.uusInfo->uusType, dial.uusInfo->uusDcs,
                dial.uusInfo->uusLength);
    }
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &dial, sizeOfDial, pRI);

#ifdef MEMSET_FREED
    memsetString (dial.address);
#endif

    free (dial.address);

#ifdef MEMSET_FREED
    memset(&uusInfo, 0, sizeof(RIL_UUS_Info));
    memset(&dial, 0, sizeof(dial));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

/**
 * Callee expects const RIL_SIM_IO *
 * Payload is:
 *   int32_t command
 *   int32_t fileid
 *   String path
 *   int32_t p1, p2, p3
 *   String data
 *   String pin2
 *   String aidPtr
 */
static void
dispatchSIM_IO (Parcel &p, RequestInfo *pRI) {
    union RIL_SIM_IO {
        RIL_SIM_IO_v6 v6;
        RIL_SIM_IO_v5 v5;
    } simIO;

    int32_t t;
    int size;
    status_t status;

    memset (&simIO, 0, sizeof(simIO));

    // note we only check status at the end

    status = p.readInt32(&t);
    simIO.v6.command = (int)t;

    status = p.readInt32(&t);
    simIO.v6.fileid = (int)t;

    simIO.v6.path = strdupReadString(p);

    status = p.readInt32(&t);
    simIO.v6.p1 = (int)t;

    status = p.readInt32(&t);
    simIO.v6.p2 = (int)t;

    status = p.readInt32(&t);
    simIO.v6.p3 = (int)t;

    simIO.v6.data = strdupReadString(p);
    simIO.v6.pin2 = strdupReadString(p);
    simIO.v6.aidPtr = strdupReadString(p);

    startRequest;
    appendPrintBuf("%scmd=0x%X,efid=0x%X,path=%s,%d,%d,%d,%s,pin2=%s,aid=%s", printBuf,
        simIO.v6.command, simIO.v6.fileid, (char*)simIO.v6.path,
        simIO.v6.p1, simIO.v6.p2, simIO.v6.p3,
        (char*)simIO.v6.data,  (char*)simIO.v6.pin2, simIO.v6.aidPtr);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }

    size = (s_callbacks.version < 6) ? sizeof(simIO.v5) : sizeof(simIO.v6);
    s_callbacks.onRequest(pRI->pCI->requestNumber, &simIO, size, pRI);

#ifdef MEMSET_FREED
    memsetString (simIO.v6.path);
    memsetString (simIO.v6.data);
    memsetString (simIO.v6.pin2);
    memsetString (simIO.v6.aidPtr);
#endif

    free (simIO.v6.path);
    free (simIO.v6.data);
    free (simIO.v6.pin2);
    free (simIO.v6.aidPtr);

#ifdef MEMSET_FREED
    memset(&simIO, 0, sizeof(simIO));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

/**
 * Callee expects const RIL_SIM_APDU *
 * Payload is:
 *   int32_t sessionid
 *   int32_t cla
 *   int32_t instruction
 *   int32_t p1, p2, p3
 *   String data
 */
static void
dispatchSIM_APDU (Parcel &p, RequestInfo *pRI) {
    int32_t t;
    status_t status;
    RIL_SIM_APDU apdu;

    memset (&apdu, 0, sizeof(RIL_SIM_APDU));

    // Note we only check status at the end. Any single failure leads to
    // subsequent reads filing.
    status = p.readInt32(&t);
    apdu.sessionid = (int)t;

    status = p.readInt32(&t);
    apdu.cla = (int)t;

    status = p.readInt32(&t);
    apdu.instruction = (int)t;

    status = p.readInt32(&t);
    apdu.p1 = (int)t;

    status = p.readInt32(&t);
    apdu.p2 = (int)t;

    status = p.readInt32(&t);
    apdu.p3 = (int)t;

    apdu.data = strdupReadString(p);

    startRequest;
    appendPrintBuf("%ssessionid=%d,cla=%d,ins=%d,p1=%d,p2=%d,p3=%d,data=%s",
        printBuf, apdu.sessionid, apdu.cla, apdu.instruction, apdu.p1, apdu.p2,
        apdu.p3, (char*)apdu.data);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, &apdu, sizeof(RIL_SIM_APDU), pRI, pRI->socket_id);

#ifdef MEMSET_FREED
    memsetString(apdu.data);
#endif
    free(apdu.data);

#ifdef MEMSET_FREED
    memset(&apdu, 0, sizeof(RIL_SIM_APDU));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}


/**
 * Callee expects const RIL_CallForwardInfo *
 * Payload is:
 *  int32_t status/action
 *  int32_t reason
 *  int32_t serviceCode
 *  int32_t toa
 *  String number  (0 length -> null)
 *  int32_t timeSeconds
 */
static void
dispatchCallForward(Parcel &p, RequestInfo *pRI) {
    RIL_CallForwardInfo cff;
    int32_t t;
    status_t status;

    memset (&cff, 0, sizeof(cff));

    // note we only check status at the end

    status = p.readInt32(&t);
    cff.status = (int)t;

    status = p.readInt32(&t);
    cff.reason = (int)t;

    status = p.readInt32(&t);
    cff.serviceClass = (int)t;

    status = p.readInt32(&t);
    cff.toa = (int)t;

    cff.number = strdupReadString(p);

    status = p.readInt32(&t);
    cff.timeSeconds = (int)t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    // special case: number 0-length fields is null

    if (cff.number != NULL && strlen (cff.number) == 0) {
        cff.number = NULL;
    }

    startRequest;
    appendPrintBuf("%sstat=%d,reason=%d,serv=%d,toa=%d,%s,tout=%d", printBuf,
        cff.status, cff.reason, cff.serviceClass, cff.toa,
        (char*)cff.number, cff.timeSeconds);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &cff, sizeof(cff), pRI);

#ifdef MEMSET_FREED
    memsetString(cff.number);
#endif

    free (cff.number);

#ifdef MEMSET_FREED
    memset(&cff, 0, sizeof(cff));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}


static void
dispatchRaw(Parcel &p, RequestInfo *pRI) {
    int32_t len;
    status_t status;
    const void *data;

    status = p.readInt32(&len);

    if (status != NO_ERROR) {
        goto invalid;
    }

    // The java code writes -1 for null arrays
    if (((int)len) == -1) {
        data = NULL;
        len = 0;
    }

    data = p.readInplace(len);

    startRequest;
    appendPrintBuf("%sraw_size=%d", printBuf, len);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, const_cast<void *>(data), len, pRI);

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static status_t
constructCdmaSms(Parcel &p, RequestInfo *pRI, RIL_CDMA_SMS_Message& rcsm) {
    int32_t  t;
    uint8_t ut;
    status_t status;
    int32_t digitCount;
    int digitLimit;

    memset(&rcsm, 0, sizeof(rcsm));

    status = p.readInt32(&t);
    rcsm.uTeleserviceID = (int) t;

    status = p.read(&ut,sizeof(ut));
    rcsm.bIsServicePresent = (uint8_t) ut;

    status = p.readInt32(&t);
    rcsm.uServicecategory = (int) t;

    status = p.readInt32(&t);
    rcsm.sAddress.digit_mode = (RIL_CDMA_SMS_DigitMode) t;

    status = p.readInt32(&t);
    rcsm.sAddress.number_mode = (RIL_CDMA_SMS_NumberMode) t;

    status = p.readInt32(&t);
    rcsm.sAddress.number_type = (RIL_CDMA_SMS_NumberType) t;

    status = p.readInt32(&t);
    rcsm.sAddress.number_plan = (RIL_CDMA_SMS_NumberPlan) t;

    status = p.read(&ut,sizeof(ut));
    rcsm.sAddress.number_of_digits= (uint8_t) ut;

    digitLimit= MIN((rcsm.sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
        status = p.read(&ut,sizeof(ut));
        rcsm.sAddress.digits[digitCount] = (uint8_t) ut;
    }

    status = p.readInt32(&t);
    rcsm.sSubAddress.subaddressType = (RIL_CDMA_SMS_SubaddressType) t;

    status = p.read(&ut,sizeof(ut));
    rcsm.sSubAddress.odd = (uint8_t) ut;

    status = p.read(&ut,sizeof(ut));
    rcsm.sSubAddress.number_of_digits = (uint8_t) ut;

    digitLimit= MIN((rcsm.sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
        status = p.read(&ut,sizeof(ut));
        rcsm.sSubAddress.digits[digitCount] = (uint8_t) ut;
    }

    status = p.readInt32(&t);
    rcsm.uBearerDataLen = (int) t;

    digitLimit= MIN((rcsm.uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
        status = p.read(&ut, sizeof(ut));
        rcsm.aBearerData[digitCount] = (uint8_t) ut;
    }

    if (status != NO_ERROR) {
        return status;
    }

    startRequest;
    appendPrintBuf("%suTeleserviceID=%d, bIsServicePresent=%d, uServicecategory=%d, \
            sAddress.digit_mode=%d, sAddress.Number_mode=%d, sAddress.number_type=%d, ",
            printBuf, rcsm.uTeleserviceID,rcsm.bIsServicePresent,rcsm.uServicecategory,
            rcsm.sAddress.digit_mode, rcsm.sAddress.number_mode,rcsm.sAddress.number_type);
    closeRequest;

    printRequest(pRI->token, pRI->pCI->requestNumber);

    return status;
}

static void
dispatchCdmaSms(Parcel &p, RequestInfo *pRI) {
    RIL_CDMA_SMS_Message rcsm;

    ALOGD("dispatchCdmaSms");
    if (NO_ERROR != constructCdmaSms(p, pRI, rcsm)) {
        goto invalid;
    }

    s_callbacks.onRequest(pRI->pCI->requestNumber, &rcsm, sizeof(rcsm),pRI);

#ifdef MEMSET_FREED
    memset(&rcsm, 0, sizeof(rcsm));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchImsCdmaSms(Parcel &p, RequestInfo *pRI, uint8_t retry, int32_t messageRef) {
    RIL_IMS_SMS_Message rism;
    RIL_CDMA_SMS_Message rcsm;

    ALOGD("dispatchImsCdmaSms: retry=%d, messageRef=%d", retry, messageRef);

    if (NO_ERROR != constructCdmaSms(p, pRI, rcsm)) {
        goto invalid;
    }
    memset(&rism, 0, sizeof(rism));
    rism.tech = RADIO_TECH_3GPP2;
    rism.retry = retry;
    rism.messageRef = messageRef;
    rism.message.cdmaMessage = &rcsm;

    s_callbacks.onRequest(pRI->pCI->requestNumber, &rism,
            sizeof(RIL_RadioTechnologyFamily)+sizeof(uint8_t)+sizeof(int32_t)
            +sizeof(rcsm),pRI);

#ifdef MEMSET_FREED
    memset(&rcsm, 0, sizeof(rcsm));
    memset(&rism, 0, sizeof(rism));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchImsGsmSms(Parcel &p, RequestInfo *pRI, uint8_t retry, int32_t messageRef) {
    RIL_IMS_SMS_Message rism;
    int32_t countStrings;
    status_t status;
    size_t datalen;
    char **pStrings;
    ALOGD("dispatchImsGsmSms: retry=%d, messageRef=%d", retry, messageRef);

    status = p.readInt32 (&countStrings);

    if (status != NO_ERROR) {
        goto invalid;
    }

    memset(&rism, 0, sizeof(rism));
    rism.tech = RADIO_TECH_3GPP;
    rism.retry = retry;
    rism.messageRef = messageRef;

    startRequest;
    //appendPrintBuf("%sformat=%d,", printBuf, rism.format);
    if (countStrings == 0) {
        // just some non-null pointer
        pStrings = (char **)alloca(sizeof(char *));
        datalen = 0;
    } else if (((int)countStrings) == -1) {
        pStrings = NULL;
        datalen = 0;
    } else {
        datalen = sizeof(char *) * countStrings;

        pStrings = (char **)alloca(datalen);

        for (int i = 0 ; i < countStrings ; i++) {
            pStrings[i] = strdupReadString(p);
            appendPrintBuf("%s%s,", printBuf, pStrings[i]);
        }
    }
    removeLastChar;
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    rism.message.gsmMessage = pStrings;
    s_callbacks.onRequest(pRI->pCI->requestNumber, &rism,
            sizeof(RIL_RadioTechnologyFamily)+sizeof(uint8_t)+sizeof(int32_t)
            +datalen, pRI);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
#ifdef MEMSET_FREED
            memsetString (pStrings[i]);
#endif
            free(pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, datalen);
#endif
    }

#ifdef MEMSET_FREED
    memset(&rism, 0, sizeof(rism));
#endif
    return;
invalid:
    ALOGE("dispatchImsGsmSms invalid block");
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchImsSms(Parcel &p, RequestInfo *pRI) {
    int32_t  t;
    status_t status = p.readInt32(&t);
    RIL_RadioTechnologyFamily format;
    uint8_t retry;
    int32_t messageRef;

    ALOGD("dispatchImsSms");
    if (status != NO_ERROR) {
        goto invalid;
    }
    format = (RIL_RadioTechnologyFamily) t;

    // read retry field
    status = p.read(&retry,sizeof(retry));
    if (status != NO_ERROR) {
        goto invalid;
    }
    // read messageRef field
    status = p.read(&messageRef,sizeof(messageRef));
    if (status != NO_ERROR) {
        goto invalid;
    }

    if (RADIO_TECH_3GPP == format) {
        dispatchImsGsmSms(p, pRI, retry, messageRef);
    } else if (RADIO_TECH_3GPP2 == format) {
        dispatchImsCdmaSms(p, pRI, retry, messageRef);
    } else {
        ALOGE("requestImsSendSMS invalid format value =%d", format);
    }

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchCdmaSmsAck(Parcel &p, RequestInfo *pRI) {
    RIL_CDMA_SMS_Ack rcsa;
    int32_t  t;
    status_t status;
    int32_t digitCount;

    memset(&rcsa, 0, sizeof(rcsa));

    status = p.readInt32(&t);
    rcsa.uErrorClass = (RIL_CDMA_SMS_ErrorClass) t;

    status = p.readInt32(&t);
    rcsa.uSMSCauseCode = (int) t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%suErrorClass=%d, uTLStatus=%d, ",
            printBuf, rcsa.uErrorClass, rcsa.uSMSCauseCode);
    closeRequest;

    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &rcsa, sizeof(rcsa),pRI);

#ifdef MEMSET_FREED
    memset(&rcsa, 0, sizeof(rcsa));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchGsmBrSmsCnf(Parcel &p, RequestInfo *pRI) {
    int32_t t;
    status_t status;
    int32_t num;

    status = p.readInt32(&num);
    if (status != NO_ERROR) {
        goto invalid;
    }

    {
        RIL_GSM_BroadcastSmsConfigInfo gsmBci[num];
        RIL_GSM_BroadcastSmsConfigInfo *gsmBciPtrs[num];

        startRequest;
        for (int i = 0 ; i < num ; i++ ) {
            gsmBciPtrs[i] = &gsmBci[i];

            status = p.readInt32(&t);
            gsmBci[i].fromServiceId = (int) t;

            status = p.readInt32(&t);
            gsmBci[i].toServiceId = (int) t;

            status = p.readInt32(&t);
            gsmBci[i].fromCodeScheme = (int) t;

            status = p.readInt32(&t);
            gsmBci[i].toCodeScheme = (int) t;

            status = p.readInt32(&t);
            gsmBci[i].selected = (uint8_t) t;

            appendPrintBuf("%s [%d: fromServiceId=%d, toServiceId =%d, \
                  fromCodeScheme=%d, toCodeScheme=%d, selected =%d]", printBuf, i,
                  gsmBci[i].fromServiceId, gsmBci[i].toServiceId,
                  gsmBci[i].fromCodeScheme, gsmBci[i].toCodeScheme,
                  gsmBci[i].selected);
        }
        closeRequest;

        if (status != NO_ERROR) {
            goto invalid;
        }

        s_callbacks.onRequest(pRI->pCI->requestNumber,
                              gsmBciPtrs,
                              num * sizeof(RIL_GSM_BroadcastSmsConfigInfo *),
                              pRI);

#ifdef MEMSET_FREED
        memset(gsmBci, 0, num * sizeof(RIL_GSM_BroadcastSmsConfigInfo));
        memset(gsmBciPtrs, 0, num * sizeof(RIL_GSM_BroadcastSmsConfigInfo *));
#endif
    }

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void
dispatchCdmaBrSmsCnf(Parcel &p, RequestInfo *pRI) {
    int32_t t;
    status_t status;
    int32_t num;

    status = p.readInt32(&num);
    if (status != NO_ERROR) {
        goto invalid;
    }

    {
        RIL_CDMA_BroadcastSmsConfigInfo cdmaBci[num];
        RIL_CDMA_BroadcastSmsConfigInfo *cdmaBciPtrs[num];

        startRequest;
        for (int i = 0 ; i < num ; i++ ) {
            cdmaBciPtrs[i] = &cdmaBci[i];

            status = p.readInt32(&t);
            cdmaBci[i].service_category = (int) t;

            status = p.readInt32(&t);
            cdmaBci[i].language = (int) t;

            status = p.readInt32(&t);
            cdmaBci[i].selected = (uint8_t) t;

            appendPrintBuf("%s [%d: service_category=%d, language =%d, \
                  entries.bSelected =%d]", printBuf, i, cdmaBci[i].service_category,
                  cdmaBci[i].language, cdmaBci[i].selected);
        }
        closeRequest;

        if (status != NO_ERROR) {
            goto invalid;
        }

        s_callbacks.onRequest(pRI->pCI->requestNumber,
                              cdmaBciPtrs,
                              num * sizeof(RIL_CDMA_BroadcastSmsConfigInfo *),
                              pRI);

#ifdef MEMSET_FREED
        memset(cdmaBci, 0, num * sizeof(RIL_CDMA_BroadcastSmsConfigInfo));
        memset(cdmaBciPtrs, 0, num * sizeof(RIL_CDMA_BroadcastSmsConfigInfo *));
#endif
    }

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchRilCdmaSmsWriteArgs(Parcel &p, RequestInfo *pRI) {
    RIL_CDMA_SMS_WriteArgs rcsw;
    int32_t  t;
    uint32_t ut;
    uint8_t  uct;
    status_t status;
    int32_t  digitCount;

    memset(&rcsw, 0, sizeof(rcsw));

    status = p.readInt32(&t);
    rcsw.status = t;

    status = p.readInt32(&t);
    rcsw.message.uTeleserviceID = (int) t;

    status = p.read(&uct,sizeof(uct));
    rcsw.message.bIsServicePresent = (uint8_t) uct;

    status = p.readInt32(&t);
    rcsw.message.uServicecategory = (int) t;

    status = p.readInt32(&t);
    rcsw.message.sAddress.digit_mode = (RIL_CDMA_SMS_DigitMode) t;

    status = p.readInt32(&t);
    rcsw.message.sAddress.number_mode = (RIL_CDMA_SMS_NumberMode) t;

    status = p.readInt32(&t);
    rcsw.message.sAddress.number_type = (RIL_CDMA_SMS_NumberType) t;

    status = p.readInt32(&t);
    rcsw.message.sAddress.number_plan = (RIL_CDMA_SMS_NumberPlan) t;

    status = p.read(&uct,sizeof(uct));
    rcsw.message.sAddress.number_of_digits = (uint8_t) uct;

    for(digitCount = 0 ; digitCount < RIL_CDMA_SMS_ADDRESS_MAX; digitCount ++) {
        status = p.read(&uct,sizeof(uct));
        rcsw.message.sAddress.digits[digitCount] = (uint8_t) uct;
    }

    status = p.readInt32(&t);
    rcsw.message.sSubAddress.subaddressType = (RIL_CDMA_SMS_SubaddressType) t;

    status = p.read(&uct,sizeof(uct));
    rcsw.message.sSubAddress.odd = (uint8_t) uct;

    status = p.read(&uct,sizeof(uct));
    rcsw.message.sSubAddress.number_of_digits = (uint8_t) uct;

    for(digitCount = 0 ; digitCount < RIL_CDMA_SMS_SUBADDRESS_MAX; digitCount ++) {
        status = p.read(&uct,sizeof(uct));
        rcsw.message.sSubAddress.digits[digitCount] = (uint8_t) uct;
    }

    status = p.readInt32(&t);
    rcsw.message.uBearerDataLen = (int) t;

    for(digitCount = 0 ; digitCount < RIL_CDMA_SMS_BEARER_DATA_MAX; digitCount ++) {
        status = p.read(&uct, sizeof(uct));
        rcsw.message.aBearerData[digitCount] = (uint8_t) uct;
    }

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%sstatus=%d, message.uTeleserviceID=%d, message.bIsServicePresent=%d, \
            message.uServicecategory=%d, message.sAddress.digit_mode=%d, \
            message.sAddress.number_mode=%d, \
            message.sAddress.number_type=%d, ",
            printBuf, rcsw.status, rcsw.message.uTeleserviceID, rcsw.message.bIsServicePresent,
            rcsw.message.uServicecategory, rcsw.message.sAddress.digit_mode,
            rcsw.message.sAddress.number_mode,
            rcsw.message.sAddress.number_type);
    closeRequest;

    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &rcsw, sizeof(rcsw),pRI);

#ifdef MEMSET_FREED
    memset(&rcsw, 0, sizeof(rcsw));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;

}

// For backwards compatibility in RIL_REQUEST_SETUP_DATA_CALL.
// Version 4 of the RIL interface adds a new PDP type parameter to support
// IPv6 and dual-stack PDP contexts. When dealing with a previous version of
// RIL, remove the parameter from the request.
static void dispatchDataCall(Parcel& p, RequestInfo *pRI) {
    // In RIL v3, REQUEST_SETUP_DATA_CALL takes 6 parameters.
    const int numParamsRilV3 = 6;

    // The first bytes of the RIL parcel contain the request number and the
    // serial number - see processCommandBuffer(). Copy them over too.
    int pos = p.dataPosition();

    int numParams = p.readInt32();
    if (s_callbacks.version < 4 && numParams > numParamsRilV3) {
      Parcel p2;
      p2.appendFrom(&p, 0, pos);
      p2.writeInt32(numParamsRilV3);
      for(int i = 0; i < numParamsRilV3; i++) {
        p2.writeString16(p.readString16());
      }
      p2.setDataPosition(pos);
      dispatchStrings(p2, pRI);
    } else {
      p.setDataPosition(pos);
      dispatchStrings(p, pRI);
    }
}

// For backwards compatibility with RILs that dont support RIL_REQUEST_VOICE_RADIO_TECH.
// When all RILs handle this request, this function can be removed and
// the request can be sent directly to the RIL using dispatchVoid.
static void dispatchVoiceRadioTech(Parcel& p, RequestInfo *pRI) {
    RIL_RadioState state = s_callbacks.onStateRequest();

    if ((RADIO_STATE_UNAVAILABLE == state) || (RADIO_STATE_OFF == state)) {
        RIL_onRequestComplete(pRI, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
    }

    // RILs that support RADIO_STATE_ON should support this request.
    if (RADIO_STATE_ON == state) {
        dispatchVoid(p, pRI);
        return;
    }

    // For Older RILs, that do not support RADIO_STATE_ON, assume that they
    // will not support this new request either and decode Voice Radio Technology
    // from Radio State
    voiceRadioTech = decodeVoiceRadioTechnology(state);

    if (voiceRadioTech < 0)
        RIL_onRequestComplete(pRI, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onRequestComplete(pRI, RIL_E_SUCCESS, &voiceRadioTech, sizeof(int));
}

// For backwards compatibility in RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE:.
// When all RILs handle this request, this function can be removed and
// the request can be sent directly to the RIL using dispatchVoid.
static void dispatchCdmaSubscriptionSource(Parcel& p, RequestInfo *pRI) {
    RIL_RadioState state = s_callbacks.onStateRequest();

    if ((RADIO_STATE_UNAVAILABLE == state) || (RADIO_STATE_OFF == state)) {
        RIL_onRequestComplete(pRI, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
    }

    // RILs that support RADIO_STATE_ON should support this request.
    if (RADIO_STATE_ON == state) {
        dispatchVoid(p, pRI);
        return;
    }

    // For Older RILs, that do not support RADIO_STATE_ON, assume that they
    // will not support this new request either and decode CDMA Subscription Source
    // from Radio State
    cdmaSubscriptionSource = decodeCdmaSubscriptionSource(state);

    /* Add for bug279146 Start */
    /* Don't support for cdma,so force to -1*/
    cdmaSubscriptionSource = -1;
    /* Add for bug279146 End   */

    if (cdmaSubscriptionSource < 0)
        RIL_onRequestComplete(pRI, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onRequestComplete(pRI, RIL_E_SUCCESS, &cdmaSubscriptionSource, sizeof(int));
}

static void dispatchSetInitialAttachApn(Parcel &p, RequestInfo *pRI)
{
    RIL_InitialAttachApn pf;
    int32_t  t;
    status_t status;

    memset(&pf, 0, sizeof(pf));

    pf.apn = strdupReadString(p);
    pf.protocol = strdupReadString(p);

    status = p.readInt32(&t);
    pf.authtype = (int) t;

    pf.username = strdupReadString(p);
    pf.password = strdupReadString(p);

    startRequest;
    appendPrintBuf("%sapn=%s, protocol=%s, authtype=%d, username=%s, password=%s",
            printBuf, pf.apn, pf.protocol, pf.authtype, pf.username, pf.password);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }
    s_callbacks.onRequest(pRI->pCI->requestNumber, &pf, sizeof(pf), pRI);

#ifdef MEMSET_FREED
    memsetString(pf.apn);
    memsetString(pf.protocol);
    memsetString(pf.username);
    memsetString(pf.password);
#endif

    free(pf.apn);
    free(pf.protocol);
    free(pf.username);
    free(pf.password);

#ifdef MEMSET_FREED
    memset(&pf, 0, sizeof(pf));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static void dispatchImsSendSms(Parcel& p, RequestInfo *pRI)
{
    /*FIXME*/
}

static void dispatchSetUiccSub(Parcel& p, RequestInfo *pRI)
{
    /*FIXME*/
}

static void dispatchSetCBConf(Parcel& p, RequestInfo *pRI)
{
    RIL_CB_ConfigArgs cbs;
    int32_t t;
    status_t status;

    memset(&cbs, 0, sizeof(cbs));

    /* note we only check status at the end */

    status = p.readInt32(&t);
    cbs.bCBEnabled = (int)t;

    status = p.readInt32(&t);
    cbs.selectedId = (int)t;

    status = p.readInt32(&t);
    cbs.msgIdMaxCount = (int)t;

    status = p.readInt32(&t);
    cbs.msgIdCount = (int)t;

    cbs.msgIDs = strdupReadString(p);

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%sbCBEnabled=%d,selectedId=%d,msgIdMaxCount=%d,msgIdCount=%d,%s", printBuf,
        cbs.bCBEnabled, cbs.selectedId, cbs.msgIdMaxCount, cbs.msgIdCount,
        (char*)cbs.msgIDs);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &cbs, sizeof(cbs), pRI);

#ifdef MEMSET_FREED
    memsetString(cbs.msgIDs);
#endif

    free(cbs.msgIDs);

#ifdef MEMSET_FREED
    memset(&cbs, 0, sizeof(cbs));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchSendUssd(Parcel& p, RequestInfo *pRI)
{
    RIL_EncodedUSSD ussd;
    int32_t t;
    status_t status;

    memset(&ussd, 0, sizeof(ussd));

    /* note we only check status at the end */
    ussd.encodedUSSD = strdupReadString(p);

    status = p.readInt32(&t);
    ussd.ussdLength = (int)t;

    status = p.readInt32(&t);
    ussd.dcsCode = (int)t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%sencodedUSSD=%s,ussdLength=%d,dcsCode=%d", printBuf,
        ussd.encodedUSSD, ussd.ussdLength, ussd.dcsCode);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &ussd, sizeof(ussd), pRI);

#ifdef MEMSET_FREED
    memsetString(ussd.encodedUSSD);
#endif

    free(ussd.encodedUSSD);

#ifdef MEMSET_FREED
    memset(&ussd, 0, sizeof(ussd));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchGetPB(Parcel& p, RequestInfo *pRI)
{
    RIL_SIM_PB simPB;
    int32_t t;
    int size;
    status_t status;

    memset (&simPB, 0, sizeof(simPB));

    // note we only check status at the end

    status = p.readInt32(&t);
    simPB.command = (int)t;

    status = p.readInt32(&t);
    simPB.fileid = (int)t;

    status = p.readInt32(&t);
    simPB.index = (int)t;

    simPB.pin2 = strdupReadString(p);

    startRequest;
    appendPrintBuf("%scommand=%d,fileid=%d,index=%d,pin2=%s", printBuf,
        simPB.command, simPB.fileid, simPB.index, (char*)simPB.pin2);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }

    s_callbacks.onRequest(pRI->pCI->requestNumber, &simPB, sizeof(simPB), pRI);

#ifdef MEMSET_FREED
    memsetString (simPB.pin2);
#endif

    free (simPB.pin2);

#ifdef MEMSET_FREED
    memset(&simPB, 0, sizeof(simPB));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchAccessPB(Parcel& p, RequestInfo *pRI)
{
    RIL_SIM_ACCESS_PB simAccessPB;
    int32_t t;
    int size;
    status_t status;

    memset (&simAccessPB, 0, sizeof(simAccessPB));

    // note we only check status at the end

    status = p.readInt32(&t);
    simAccessPB.command = (int)t;

    status = p.readInt32(&t);
    simAccessPB.fileid = (int)t;

    status = p.readInt32(&t);
    simAccessPB.index = (int)t;

    simAccessPB.alphaTag = strdupReadString(p);

    status = p.readInt32(&t);
    simAccessPB.alphaTagDCS = (int)t;

    status = p.readInt32(&t);
    simAccessPB.alphaTagLength = (int)t;

    simAccessPB.number = strdupReadString(p);
    simAccessPB.email = strdupReadString(p);

    status = p.readInt32(&t);
    simAccessPB.emailLength = (int)t;

    simAccessPB.anr = strdupReadString(p);
    simAccessPB.anrA = strdupReadString(p);
    simAccessPB.anrB = strdupReadString(p);
    simAccessPB.anrC = strdupReadString(p);
    simAccessPB.sne = strdupReadString(p);

    status = p.readInt32(&t);
    simAccessPB.sneLength = (int)t;

    status = p.readInt32(&t);
    simAccessPB.sneDCS = (int)t;

    simAccessPB.pin2 = strdupReadString(p);

    startRequest;
    appendPrintBuf("%scommand=%d,fileid=%d,index=%d,alphaTag=%s,alphaTagDCS=%d,alphaTagLength=%d,number=%s,email=%s,emailLength=%d,anr=%s,anrA=%s,anrB=%s,anrC=%s,sne=%s,sneLength=%d,sneDCS=%d,pin2=%s", 
    printBuf,
    simAccessPB.command,
    simAccessPB.fileid,
    simAccessPB.index,
    (char*)simAccessPB.alphaTag,
    simAccessPB.alphaTagDCS,
    simAccessPB.alphaTagLength,
    (char*)simAccessPB.number,
    (char*)simAccessPB.email,
    simAccessPB.emailLength,
    (char*)simAccessPB.anr,
    (char*)simAccessPB.anrA,
    (char*)simAccessPB.anrB,
    (char*)simAccessPB.anrC,
    (char*)simAccessPB.sne,
    simAccessPB.sneLength,
    simAccessPB.sneDCS,
    (char*)simAccessPB.pin2);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }

    s_callbacks.onRequest(pRI->pCI->requestNumber, &simAccessPB, sizeof(simAccessPB), pRI);

#ifdef MEMSET_FREED
    memsetString (simAccessPB.alphaTag);
    memsetString (simAccessPB.number);
    memsetString (simAccessPB.email);
    memsetString (simAccessPB.anr);
    memsetString (simAccessPB.anrA);
    memsetString (simAccessPB.anrB);
    memsetString (simAccessPB.anrC);
    memsetString (simAccessPB.sne);
    memsetString (simAccessPB.pin2);
#endif

    free (simAccessPB.alphaTag);
    free (simAccessPB.number);
    free (simAccessPB.email);
    free (simAccessPB.anr);
    free (simAccessPB.anrA);
    free (simAccessPB.anrB);
    free (simAccessPB.anrC);
    free (simAccessPB.sne);
    free (simAccessPB.pin2);

#ifdef MEMSET_FREED
    memset(&simAccessPB, 0, sizeof(simAccessPB));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}
#endif

#if defined (RIL_SPRD_EXTENSION)
/**
 * Callee expects const RIL_NetworkList *
 * Payload is:
 *   String operatorNumeric
 *   int32_t act
 */
static void
dispatchNetworkList (Parcel &p, RequestInfo *pRI) {
    RIL_NetworkList list;
    int32_t t;
    status_t status;

    memset (&list, 0, sizeof(list));

    /* note we only check status at the end */

    list.operatorNumeric = strdupReadString(p);

    status = p.readInt32(&t);
    list.act = (int)t;

    startRequest;
    appendPrintBuf("%soperatorNumeric=%s,AcT=%d", printBuf,
        (char*)list.operatorNumeric, list.act);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        /* Mod for bug267572 Start */
        //goto invalid;
        RILLOGD("set default AcT = -1");
        list.act = -1;
        /* Mod for bug267572 End   */
    }

       s_callbacks.onRequest(pRI->pCI->requestNumber, &list, sizeof(list), pRI);

#ifdef MEMSET_FREED
    memsetString (list.operatorNumeric);
#endif

    free (list.operatorNumeric);

#ifdef MEMSET_FREED
    memset(&list, 0, sizeof(list));
#endif

    return;
//invalid:
//    invalidCommandBlock(pRI);
//    return;
}

static void dispatchVideoPhoneInit(Parcel& p, RequestInfo *pRI){
    RIL_VideoPhone_Init param;
    int32_t t;
    status_t status;

    memset(&param, 0, sizeof(param));

    status = p.readInt32(&t);
    param.crc = (int)t;

    status += p.readInt32(&t);
    param.dcpi = (int)t;

    status += p.readInt32(&t);
    param.dsci = (int)t;

    status += p.readInt32(&t);
    param.dvtchl = (int)t;

    status += p.readInt32(&t);
    param.dvtable = (int)t;

    status += p.readInt32(&t);
    param.dvttype = (int)t;

    status += p.readInt32(&t);
    param.dvtdci = (int)t;

    status += p.readInt32(&t);
    param.creg = (int)t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%scrc=%d,dcpi=%d,dsci=%d,dvtchl=%d,dvtable=%d,dvttype=%d,dvtdci=%d,creg=%d",
        printBuf, param.crc, param.dcpi, param.dsci, param.dvtchl, param.dvtable, param.dvttype, param.dvtdci, param.creg);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &param, sizeof(param), pRI);

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchVideoPhoneDial(Parcel& p, RequestInfo *pRI){
    RIL_VideoPhone_Dial dial;
    int32_t t;
    status_t status;

    memset (&dial, 0, sizeof(dial));

    dial.address = strdupReadString(p);
    dial.sub_address = strdupReadString(p);

    status = p.readInt32(&t);
    dial.clir = (int)t;

    if (status != NO_ERROR || dial.address == NULL) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%saddress=%s,sub_address=%s,clir=%d", printBuf, dial.address, dial.sub_address, dial.clir);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &dial, sizeof(dial), pRI);

#ifdef MEMSET_FREED
    memsetString (dial.address);
    memsetString (dial.sub_address);
#endif

    free (dial.address);
    free (dial.sub_address);

#ifdef MEMSET_FREED
    memset(&dial, 0, sizeof(dial));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchVideoPhoneCodec(Parcel& p, RequestInfo *pRI){
    RIL_VideoPhone_Codec codec;
    int32_t t;
    status_t status;

    memset (&codec, 0, sizeof(codec));

    status = p.readInt32(&t);
    codec.type = (int)t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%stype=%d", printBuf, codec.type);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    s_callbacks.onRequest(pRI->pCI->requestNumber, &codec, sizeof(codec), pRI);

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}
#endif

static void dispatchNVReadItem(Parcel &p, RequestInfo *pRI) {
    RIL_NV_ReadItem nvri;
    int32_t  t;
    status_t status;

    memset(&nvri, 0, sizeof(nvri));

    status = p.readInt32(&t);
    nvri.itemID = (RIL_NV_Item) t;

    if (status != NO_ERROR) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%snvri.itemID=%d, ", printBuf, nvri.itemID);
    closeRequest;

    printRequest(pRI->token, pRI->pCI->requestNumber);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &nvri, sizeof(nvri), pRI, pRI->socket_id);

#ifdef MEMSET_FREED
    memset(&nvri, 0, sizeof(nvri));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchNVWriteItem(Parcel &p, RequestInfo *pRI) {
    RIL_NV_WriteItem nvwi;
    int32_t  t;
    status_t status;

    memset(&nvwi, 0, sizeof(nvwi));

    status = p.readInt32(&t);
    nvwi.itemID = (RIL_NV_Item) t;

    nvwi.value = strdupReadString(p);

    if (status != NO_ERROR || nvwi.value == NULL) {
        goto invalid;
    }

    startRequest;
    appendPrintBuf("%snvwi.itemID=%d, value=%s, ", printBuf, nvwi.itemID,
            nvwi.value);
    closeRequest;

    printRequest(pRI->token, pRI->pCI->requestNumber);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &nvwi, sizeof(nvwi), pRI, pRI->socket_id);

#ifdef MEMSET_FREED
    memsetString(nvwi.value);
#endif

    free(nvwi.value);

#ifdef MEMSET_FREED
    memset(&nvwi, 0, sizeof(nvwi));
#endif

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchUiccSubscripton(Parcel &p, RequestInfo *pRI) {
    RIL_SelectUiccSub uicc_sub;
    status_t status;
    int32_t  t;
    memset(&uicc_sub, 0, sizeof(uicc_sub));

    status = p.readInt32(&t);
    if (status != NO_ERROR) {
        goto invalid;
    }
    uicc_sub.slot = (int) t;

    status = p.readInt32(&t);
    if (status != NO_ERROR) {
        goto invalid;
    }
    uicc_sub.app_index = (int) t;

    status = p.readInt32(&t);
    if (status != NO_ERROR) {
        goto invalid;
    }
    uicc_sub.sub_type = (RIL_SubscriptionType) t;

    status = p.readInt32(&t);
    if (status != NO_ERROR) {
        goto invalid;
    }
    uicc_sub.act_status = (RIL_UiccSubActStatus) t;

    startRequest;
    appendPrintBuf("slot=%d, app_index=%d, act_status = %d", uicc_sub.slot, uicc_sub.app_index,
            uicc_sub.act_status);
    RLOGD("dispatchUiccSubscription, slot=%d, app_index=%d, act_status = %d", uicc_sub.slot,
            uicc_sub.app_index, uicc_sub.act_status);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &uicc_sub, sizeof(uicc_sub), pRI, pRI->socket_id);

#ifdef MEMSET_FREED
    memset(&uicc_sub, 0, sizeof(uicc_sub));
#endif
    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchSimAuthentication(Parcel &p, RequestInfo *pRI)
{
    RIL_SimAuthentication pf;
    int32_t  t;
    status_t status;

    memset(&pf, 0, sizeof(pf));

    status = p.readInt32(&t);
    pf.authContext = (int) t;
    pf.authData = strdupReadString(p);
    pf.aid = strdupReadString(p);

    startRequest;
    appendPrintBuf("authContext=%d, authData=%s, aid=%s", pf.authContext, pf.authData, pf.aid);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    if (status != NO_ERROR) {
        goto invalid;
    }
    CALL_ONREQUEST(pRI->pCI->requestNumber, &pf, sizeof(pf), pRI, pRI->socket_id);

#ifdef MEMSET_FREED
    memsetString(pf.authData);
    memsetString(pf.aid);
#endif

    free(pf.authData);
    free(pf.aid);

#ifdef MEMSET_FREED
    memset(&pf, 0, sizeof(pf));
#endif

    return;
invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchDataProfile(Parcel &p, RequestInfo *pRI) {
    int32_t t;
    status_t status;
    int32_t num;

    status = p.readInt32(&num);
    if (status != NO_ERROR) {
        goto invalid;
    }

    {
        RIL_DataProfileInfo dataProfiles[num];
        RIL_DataProfileInfo *dataProfilePtrs[num];

        startRequest;
        for (int i = 0 ; i < num ; i++ ) {
            dataProfilePtrs[i] = &dataProfiles[i];

            status = p.readInt32(&t);
            dataProfiles[i].profileId = (int) t;

            dataProfiles[i].apn = strdupReadString(p);
            dataProfiles[i].protocol = strdupReadString(p);
            status = p.readInt32(&t);
            dataProfiles[i].authType = (int) t;

            dataProfiles[i].user = strdupReadString(p);
            dataProfiles[i].password = strdupReadString(p);

            status = p.readInt32(&t);
            dataProfiles[i].type = (int) t;

            status = p.readInt32(&t);
            dataProfiles[i].maxConnsTime = (int) t;
            status = p.readInt32(&t);
            dataProfiles[i].maxConns = (int) t;
            status = p.readInt32(&t);
            dataProfiles[i].waitTime = (int) t;

            status = p.readInt32(&t);
            dataProfiles[i].enabled = (int) t;

            appendPrintBuf("%s [%d: profileId=%d, apn =%s, protocol =%s, authType =%d, \
                  user =%s, password =%s, type =%d, maxConnsTime =%d, maxConns =%d, \
                  waitTime =%d, enabled =%d]", printBuf, i, dataProfiles[i].profileId,
                  dataProfiles[i].apn, dataProfiles[i].protocol, dataProfiles[i].authType,
                  dataProfiles[i].user, dataProfiles[i].password, dataProfiles[i].type,
                  dataProfiles[i].maxConnsTime, dataProfiles[i].maxConns,
                  dataProfiles[i].waitTime, dataProfiles[i].enabled);
        }
        closeRequest;
        printRequest(pRI->token, pRI->pCI->requestNumber);

        if (status != NO_ERROR) {
            goto invalid;
        }
        CALL_ONREQUEST(pRI->pCI->requestNumber,
                              dataProfilePtrs,
                              num * sizeof(RIL_DataProfileInfo *),
                              pRI, pRI->socket_id);

#ifdef MEMSET_FREED
        memset(dataProfiles, 0, num * sizeof(RIL_DataProfileInfo));
        memset(dataProfilePtrs, 0, num * sizeof(RIL_DataProfileInfo *));
#endif
    }

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static int
blockingWrite(int fd, const void *buffer, size_t len) {
    size_t writeOffset = 0;
    const uint8_t *toWrite;

    toWrite = (const uint8_t *)buffer;

    while (writeOffset < len) {
        ssize_t written;
        do {
            written = write (fd, toWrite + writeOffset,
                                len - writeOffset);
        } while (written < 0 && ((errno == EINTR) || (errno == EAGAIN)));

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            RILLOGE ("RIL Response: unexpected error on write errno:%d", errno);
            close(fd);
            return -1;
        }
    }

    return 0;
}

static int
sendResponseRaw (const void *data, size_t dataSize) {
    int fd = s_fdCommand;
    int ret;
    uint32_t header;

    if (s_fdCommand < 0) {
        return -1;
    }

    if (dataSize > MAX_COMMAND_BYTES) {
        RILLOGE("RIL: packet larger than %u (%u)",
                MAX_COMMAND_BYTES, (unsigned int )dataSize);

        return -1;
    }

    pthread_mutex_lock(&s_writeMutex);

    header = htonl(dataSize);

    ret = blockingWrite(fd, (void *)&header, sizeof(header));

    if (ret < 0) {
        pthread_mutex_unlock(&s_writeMutex);
        RILLOGE("RIL:  blockingWrite header error");
        return ret;
    }

    ret = blockingWrite(fd, data, dataSize);

    if (ret < 0) {
        pthread_mutex_unlock(&s_writeMutex);
        RILLOGE("RIL:  blockingWrite data error");
        return ret;
    }

    pthread_mutex_unlock(&s_writeMutex);

    return 0;
}

static int
sendResponse (Parcel &p) {
    printResponse;
    return sendResponseRaw(p.data(), p.dataSize());
}

/** response is an int* pointing to an array of ints*/

static int
responseInts(Parcel &p, void *response, size_t responselen) {
    int numInts;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    if (responselen % sizeof(int) != 0) {
        RILLOGE("responseInts: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof(int));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    int *p_int = (int *) response;

    numInts = responselen / sizeof(int);
    p.writeInt32 (numInts);

    /* each int*/
    startResponse;
    for (int i = 0 ; i < numInts ; i++) {
        appendPrintBuf("%s%d,", printBuf, p_int[i]);
        p.writeInt32(p_int[i]);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

/** response is a char **, pointing to an array of char *'s
    The parcel will begin with the version */
static int responseStringsWithVersion(int version, Parcel &p, void *response, size_t responselen) {
    p.writeInt32(version);
    return responseStrings(p, response, responselen);
}

/** response is a char **, pointing to an array of char *'s */
static int responseStrings(Parcel &p, void *response, size_t responselen) {
    int numStrings;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    if (responselen % sizeof(char *) != 0) {
        RILLOGE("responseStrings: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof(char *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (response == NULL) {
        p.writeInt32 (0);
    } else {
        char **p_cur = (char **) response;

        numStrings = responselen / sizeof(char *);
        p.writeInt32 (numStrings);

        /* each string*/
        startResponse;
        for (int i = 0 ; i < numStrings ; i++) {
            appendPrintBuf("%s%s,", printBuf, (char*)p_cur[i]);
            writeStringToParcel (p, p_cur[i]);
        }
        removeLastChar;
        closeResponse;
    }
    return 0;
}


/**
 * NULL strings are accepted
 * FIXME currently ignores responselen
 */
static int responseString(Parcel &p, void *response, size_t responselen) {
    /* one string only */
    startResponse;
    appendPrintBuf("%s%s", printBuf, (char*)response);
    closeResponse;

    writeStringToParcel(p, (const char *)response);

    return 0;
}

static int responseVoid(Parcel &p, void *response, size_t responselen) {
    startResponse;
    removeLastChar;
    return 0;
}

static int responseCallList(Parcel &p, void *response, size_t responselen) {
    int num;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof (RIL_Call *) != 0) {
        RILLOGE("responseCallList: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof (RIL_Call *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    /* number of call info's */
    num = responselen / sizeof(RIL_Call *);
    p.writeInt32(num);

    for (int i = 0 ; i < num ; i++) {
        RIL_Call *p_cur = ((RIL_Call **) response)[i];
        /* each call info */
        p.writeInt32(p_cur->state);
        p.writeInt32(p_cur->index);
        p.writeInt32(p_cur->toa);
        p.writeInt32(p_cur->isMpty);
        p.writeInt32(p_cur->isMT);
        p.writeInt32(p_cur->als);
        p.writeInt32(p_cur->isVoice);
        p.writeInt32(p_cur->isVoicePrivacy);
        writeStringToParcel(p, p_cur->number);
        p.writeInt32(p_cur->numberPresentation);
        writeStringToParcel(p, p_cur->name);
        p.writeInt32(p_cur->namePresentation);
        // Remove when partners upgrade to version 3
        if ((s_callbacks.version < 3) || (p_cur->uusInfo == NULL || p_cur->uusInfo->uusData == NULL)) {
            p.writeInt32(0); /* UUS Information is absent */
        } else {
            RIL_UUS_Info *uusInfo = p_cur->uusInfo;
            p.writeInt32(1); /* UUS Information is present */
            p.writeInt32(uusInfo->uusType);
            p.writeInt32(uusInfo->uusDcs);
            p.writeInt32(uusInfo->uusLength);
            p.write(uusInfo->uusData, uusInfo->uusLength);
        }
        appendPrintBuf("%s[id=%d,%s,toa=%d,",
            printBuf,
            p_cur->index,
            callStateToString(p_cur->state),
            p_cur->toa);
        appendPrintBuf("%s%s,%s,als=%d,%s,%s,",
            printBuf,
            (p_cur->isMpty)?"conf":"norm",
            (p_cur->isMT)?"mt":"mo",
            p_cur->als,
            (p_cur->isVoice)?"voc":"nonvoc",
            (p_cur->isVoicePrivacy)?"evp":"noevp");
        appendPrintBuf("%s%s,cli=%d,name='%s',%d]",
            printBuf,
            p_cur->number,
            p_cur->numberPresentation,
            p_cur->name,
            p_cur->namePresentation);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseSMS(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof (RIL_SMS_Response) ) {
        RILLOGE("invalid response length %d expected %d",
                (int)responselen, (int)sizeof (RIL_SMS_Response));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_SMS_Response *p_cur = (RIL_SMS_Response *) response;

    p.writeInt32(p_cur->messageRef);
    writeStringToParcel(p, p_cur->ackPDU);
    p.writeInt32(p_cur->errorCode);

    startResponse;
    appendPrintBuf("%s%d,%s,%d", printBuf, p_cur->messageRef,
        (char*)p_cur->ackPDU, p_cur->errorCode);
    closeResponse;

    return 0;
}

static int responseDataCallListV4(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_Data_Call_Response_v4) != 0) {
        RILLOGE("responseDataCallListV4: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_Data_Call_Response_v4));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    // Write version
    p.writeInt32(4);

    int num = responselen / sizeof(RIL_Data_Call_Response_v4);
    p.writeInt32(num);

    RIL_Data_Call_Response_v4 *p_cur = (RIL_Data_Call_Response_v4 *) response;
    startResponse;
    int i;
    for (i = 0; i < num; i++) {
        p.writeInt32(p_cur[i].cid);
        p.writeInt32(p_cur[i].active);
        writeStringToParcel(p, p_cur[i].type);
        // apn is not used, so don't send.
        writeStringToParcel(p, p_cur[i].address);
        appendPrintBuf("%s[cid=%d,%s,%s,%s],", printBuf,
            p_cur[i].cid,
            (p_cur[i].active==0)?"down":"up",
            (char*)p_cur[i].type,
            (char*)p_cur[i].address);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseDataCallListV6(Parcel &p, void *response, size_t responselen)
{
   if (response == NULL && responselen != 0) {
        RLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_Data_Call_Response_v6) != 0) {
        RLOGE("responseDataCallListV6: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_Data_Call_Response_v6));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    // Write version
    p.writeInt32(6);

    int num = responselen / sizeof(RIL_Data_Call_Response_v6);
    p.writeInt32(num);

    RIL_Data_Call_Response_v6 *p_cur = (RIL_Data_Call_Response_v6 *) response;
    startResponse;
    int i;
    for (i = 0; i < num; i++) {
        p.writeInt32((int)p_cur[i].status);
        p.writeInt32(p_cur[i].suggestedRetryTime);
        p.writeInt32(p_cur[i].cid);
        p.writeInt32(p_cur[i].active);
        writeStringToParcel(p, p_cur[i].type);
        writeStringToParcel(p, p_cur[i].ifname);
        writeStringToParcel(p, p_cur[i].addresses);
        writeStringToParcel(p, p_cur[i].dnses);
        writeStringToParcel(p, p_cur[i].gateways);
        appendPrintBuf("%s[status=%d,retry=%d,cid=%d,%s,%s,%s,%s,%s,%s],", printBuf,
            p_cur[i].status,
            p_cur[i].suggestedRetryTime,
            p_cur[i].cid,
            (p_cur[i].active==0)?"down":"up",
            (char*)p_cur[i].type,
            (char*)p_cur[i].ifname,
            (char*)p_cur[i].addresses,
            (char*)p_cur[i].dnses,
            (char*)p_cur[i].gateways);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseDataCallListV9(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL && responselen != 0) {
        RLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_Data_Call_Response_v9) != 0) {
        RLOGE("responseDataCallListV9: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_Data_Call_Response_v9));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    // Write version
    p.writeInt32(10);

    int num = responselen / sizeof(RIL_Data_Call_Response_v9);
    p.writeInt32(num);

    RIL_Data_Call_Response_v9 *p_cur = (RIL_Data_Call_Response_v9 *) response;
    startResponse;
    int i;
    for (i = 0; i < num; i++) {
        p.writeInt32((int)p_cur[i].status);
        p.writeInt32(p_cur[i].suggestedRetryTime);
        p.writeInt32(p_cur[i].cid);
        p.writeInt32(p_cur[i].active);
        writeStringToParcel(p, p_cur[i].type);
        writeStringToParcel(p, p_cur[i].ifname);
        writeStringToParcel(p, p_cur[i].addresses);
        writeStringToParcel(p, p_cur[i].dnses);
        writeStringToParcel(p, p_cur[i].gateways);
        writeStringToParcel(p, p_cur[i].pcscf);
        appendPrintBuf("%s[status=%d,retry=%d,cid=%d,%s,%s,%s,%s,%s,%s,%s],", printBuf,
            p_cur[i].status,
            p_cur[i].suggestedRetryTime,
            p_cur[i].cid,
            (p_cur[i].active==0)?"down":"up",
            (char*)p_cur[i].type,
            (char*)p_cur[i].ifname,
            (char*)p_cur[i].addresses,
            (char*)p_cur[i].dnses,
            (char*)p_cur[i].gateways,
            (char*)p_cur[i].pcscf);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseDataCallList(Parcel &p, void *response, size_t responselen)
{
    // Write version
    p.writeInt32(s_callbacks.version);

    if (s_callbacks.version < 5) {
        RLOGD("responseDataCallList: v4");
        return responseDataCallListV4(p, response, responselen);
    } else if (responselen % sizeof(RIL_Data_Call_Response_v6) == 0) {
        return responseDataCallListV6(p, response, responselen);
    } else if (responselen % sizeof(RIL_Data_Call_Response_v9) == 0) {
        return responseDataCallListV9(p, response, responselen);
    } else {
        if (response == NULL && responselen != 0) {
            RILLOGE("invalid response: NULL");
            return RIL_ERRNO_INVALID_RESPONSE;
        }

        if (responselen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("invalid response length %d expected multiple of %d",
                    (int)responselen, (int)sizeof(RIL_Data_Call_Response_v11));
            return RIL_ERRNO_INVALID_RESPONSE;
        }

        int num = responselen / sizeof(RIL_Data_Call_Response_v11);
        p.writeInt32(num);

        RIL_Data_Call_Response_v11 *p_cur = (RIL_Data_Call_Response_v11 *) response;
        startResponse;
        int i;
        for (i = 0; i < num; i++) {
            p.writeInt32((int)p_cur[i].status);
            p.writeInt32(p_cur[i].suggestedRetryTime);
            p.writeInt32(p_cur[i].cid);
            p.writeInt32(p_cur[i].active);
            writeStringToParcel(p, p_cur[i].type);
            writeStringToParcel(p, p_cur[i].ifname);
            writeStringToParcel(p, p_cur[i].addresses);
            writeStringToParcel(p, p_cur[i].dnses);
            writeStringToParcel(p, p_cur[i].gateways);
            writeStringToParcel(p, p_cur[i].pcscf);
            p.writeInt32(p_cur[i].mtu);
            appendPrintBuf("%s[status=%d,retry=%d,cid=%d,%s,%s,%s,%s,%s,%s,%s,mtu=%d],", printBuf,
                p_cur[i].status,
                p_cur[i].suggestedRetryTime,
                p_cur[i].cid,
                (p_cur[i].active==0)?"down":"up",
                (char*)p_cur[i].type,
                (char*)p_cur[i].ifname,
                (char*)p_cur[i].addresses,
                (char*)p_cur[i].dnses,
                (char*)p_cur[i].gateways,
                (char*)p_cur[i].pcscf,
                p_cur[i].mtu);
        }
        removeLastChar;
        closeResponse;
    }

    return 0;
}

static int responseSetupDataCall(Parcel &p, void *response, size_t responselen)
{
    if (s_callbacks.version < 5) {
        return responseStringsWithVersion(s_callbacks.version, p, response, responselen);
    } else {
        return responseDataCallList(p, response, responselen);
    }
}

static int responseRaw(Parcel &p, void *response, size_t responselen) {
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL with responselen != 0");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    // The java code reads -1 size as null byte array
    if (response == NULL) {
        p.writeInt32(-1);
    } else {
        p.writeInt32(responselen);
        p.write(response, responselen);
    }

    return 0;
}


static int responseSIM_IO(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof (RIL_SIM_IO_Response) ) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_SIM_IO_Response));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_SIM_IO_Response *p_cur = (RIL_SIM_IO_Response *) response;
    p.writeInt32(p_cur->sw1);
    p.writeInt32(p_cur->sw2);
    writeStringToParcel(p, p_cur->simResponse);

    startResponse;
    appendPrintBuf("%ssw1=0x%X,sw2=0x%X,%s", printBuf, p_cur->sw1, p_cur->sw2,
        (char*)p_cur->simResponse);
    closeResponse;


    return 0;
}

static int responseCallForwards(Parcel &p, void *response, size_t responselen) {
    int num;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_CallForwardInfo *) != 0) {
        RILLOGE("responseCallForwards: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_CallForwardInfo *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    /* number of call info's */
    num = responselen / sizeof(RIL_CallForwardInfo *);
    p.writeInt32(num);

    startResponse;
    for (int i = 0 ; i < num ; i++) {
        RIL_CallForwardInfo *p_cur = ((RIL_CallForwardInfo **) response)[i];

        p.writeInt32(p_cur->status);
        p.writeInt32(p_cur->reason);
        p.writeInt32(p_cur->serviceClass);
        p.writeInt32(p_cur->toa);
        writeStringToParcel(p, p_cur->number);
        p.writeInt32(p_cur->timeSeconds);
        appendPrintBuf("%s[%s,reason=%d,cls=%d,toa=%d,%s,tout=%d],", printBuf,
            (p_cur->status==1)?"enable":"disable",
            p_cur->reason, p_cur->serviceClass, p_cur->toa,
            (char*)p_cur->number,
            p_cur->timeSeconds);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseSsn(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_SuppSvcNotification)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_SuppSvcNotification));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_SuppSvcNotification *p_cur = (RIL_SuppSvcNotification *) response;
    p.writeInt32(p_cur->notificationType);
    p.writeInt32(p_cur->code);
    p.writeInt32(p_cur->index);
    p.writeInt32(p_cur->type);
    writeStringToParcel(p, p_cur->number);

    startResponse;
    appendPrintBuf("%s%s,code=%d,id=%d,type=%d,%s", printBuf,
        (p_cur->notificationType==0)?"mo":"mt",
         p_cur->code, p_cur->index, p_cur->type,
        (char*)p_cur->number);
    closeResponse;

    return 0;
}

static int responseCellList(Parcel &p, void *response, size_t responselen) {
    int num;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof (RIL_NeighboringCell *) != 0) {
        RILLOGE("responseCellList: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof (RIL_NeighboringCell *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    /* number of records */
    num = responselen / sizeof(RIL_NeighboringCell *);
    p.writeInt32(num);

    for (int i = 0 ; i < num ; i++) {
        RIL_NeighboringCell *p_cur = ((RIL_NeighboringCell **) response)[i];

        p.writeInt32(p_cur->rssi);
        writeStringToParcel (p, p_cur->cid);

        appendPrintBuf("%s[cid=%s,rssi=%d],", printBuf,
            p_cur->cid, p_cur->rssi);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

/**
 * Marshall the signalInfoRecord into the parcel if it exists.
 */
static void marshallSignalInfoRecord(Parcel &p,
            RIL_CDMA_SignalInfoRecord &p_signalInfoRecord) {
    p.writeInt32(p_signalInfoRecord.isPresent);
    p.writeInt32(p_signalInfoRecord.signalType);
    p.writeInt32(p_signalInfoRecord.alertPitch);
    p.writeInt32(p_signalInfoRecord.signal);
}

static int responseCdmaInformationRecords(Parcel &p,
            void *response, size_t responselen) {
    int num;
    char* string8 = NULL;
    int buffer_lenght;
    RIL_CDMA_InformationRecord *infoRec;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof (RIL_CDMA_InformationRecords)) {
        RILLOGE("responseCdmaInformationRecords: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof (RIL_CDMA_InformationRecords *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_CDMA_InformationRecords *p_cur =
                             (RIL_CDMA_InformationRecords *) response;
    num = MIN(p_cur->numberOfInfoRecs, RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);

    startResponse;
    p.writeInt32(num);

    for (int i = 0 ; i < num ; i++) {
        infoRec = &p_cur->infoRec[i];
        p.writeInt32(infoRec->name);
        switch (infoRec->name) {
            case RIL_CDMA_DISPLAY_INFO_REC:
            case RIL_CDMA_EXTENDED_DISPLAY_INFO_REC:
                if (infoRec->rec.display.alpha_len >
                                         CDMA_ALPHA_INFO_BUFFER_LENGTH) {
                    RILLOGE("invalid display info response length %d \
                          expected not more than %d\n",
                         (int)infoRec->rec.display.alpha_len,
                         CDMA_ALPHA_INFO_BUFFER_LENGTH);
                    return RIL_ERRNO_INVALID_RESPONSE;
                }
                string8 = (char*) malloc((infoRec->rec.display.alpha_len + 1)
                                                             * sizeof(char) );
                for (int i = 0 ; i < infoRec->rec.display.alpha_len ; i++) {
                    string8[i] = infoRec->rec.display.alpha_buf[i];
                }
                string8[(int)infoRec->rec.display.alpha_len] = '\0';
                writeStringToParcel(p, (const char*)string8);
                free(string8);
                string8 = NULL;
                break;
            case RIL_CDMA_CALLED_PARTY_NUMBER_INFO_REC:
            case RIL_CDMA_CALLING_PARTY_NUMBER_INFO_REC:
            case RIL_CDMA_CONNECTED_NUMBER_INFO_REC:
                if (infoRec->rec.number.len > CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                    RILLOGE("invalid display info response length %d \
                          expected not more than %d\n",
                         (int)infoRec->rec.number.len,
                         CDMA_NUMBER_INFO_BUFFER_LENGTH);
                    return RIL_ERRNO_INVALID_RESPONSE;
                }
                string8 = (char*) malloc((infoRec->rec.number.len + 1)
                                                             * sizeof(char) );
                for (int i = 0 ; i < infoRec->rec.number.len; i++) {
                    string8[i] = infoRec->rec.number.buf[i];
                }
                string8[(int)infoRec->rec.number.len] = '\0';
                writeStringToParcel(p, (const char*)string8);
                free(string8);
                string8 = NULL;
                p.writeInt32(infoRec->rec.number.number_type);
                p.writeInt32(infoRec->rec.number.number_plan);
                p.writeInt32(infoRec->rec.number.pi);
                p.writeInt32(infoRec->rec.number.si);
                break;
            case RIL_CDMA_SIGNAL_INFO_REC:
                p.writeInt32(infoRec->rec.signal.isPresent);
                p.writeInt32(infoRec->rec.signal.signalType);
                p.writeInt32(infoRec->rec.signal.alertPitch);
                p.writeInt32(infoRec->rec.signal.signal);

                appendPrintBuf("%sisPresent=%X, signalType=%X, \
                                alertPitch=%X, signal=%X, ",
                   printBuf, (int)infoRec->rec.signal.isPresent,
                   (int)infoRec->rec.signal.signalType,
                   (int)infoRec->rec.signal.alertPitch,
                   (int)infoRec->rec.signal.signal);
                removeLastChar;
                break;
            case RIL_CDMA_REDIRECTING_NUMBER_INFO_REC:
                if (infoRec->rec.redir.redirectingNumber.len >
                                              CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                    RILLOGE("invalid display info response length %d \
                          expected not more than %d\n",
                         (int)infoRec->rec.redir.redirectingNumber.len,
                         CDMA_NUMBER_INFO_BUFFER_LENGTH);
                    return RIL_ERRNO_INVALID_RESPONSE;
                }
                string8 = (char*) malloc((infoRec->rec.redir.redirectingNumber
                                          .len + 1) * sizeof(char) );
                for (int i = 0;
                         i < infoRec->rec.redir.redirectingNumber.len;
                         i++) {
                    string8[i] = infoRec->rec.redir.redirectingNumber.buf[i];
                }
                string8[(int)infoRec->rec.redir.redirectingNumber.len] = '\0';
                writeStringToParcel(p, (const char*)string8);
                free(string8);
                string8 = NULL;
                p.writeInt32(infoRec->rec.redir.redirectingNumber.number_type);
                p.writeInt32(infoRec->rec.redir.redirectingNumber.number_plan);
                p.writeInt32(infoRec->rec.redir.redirectingNumber.pi);
                p.writeInt32(infoRec->rec.redir.redirectingNumber.si);
                p.writeInt32(infoRec->rec.redir.redirectingReason);
                break;
            case RIL_CDMA_LINE_CONTROL_INFO_REC:
                p.writeInt32(infoRec->rec.lineCtrl.lineCtrlPolarityIncluded);
                p.writeInt32(infoRec->rec.lineCtrl.lineCtrlToggle);
                p.writeInt32(infoRec->rec.lineCtrl.lineCtrlReverse);
                p.writeInt32(infoRec->rec.lineCtrl.lineCtrlPowerDenial);

                appendPrintBuf("%slineCtrlPolarityIncluded=%d, \
                                lineCtrlToggle=%d, lineCtrlReverse=%d, \
                                lineCtrlPowerDenial=%d, ", printBuf,
                       (int)infoRec->rec.lineCtrl.lineCtrlPolarityIncluded,
                       (int)infoRec->rec.lineCtrl.lineCtrlToggle,
                       (int)infoRec->rec.lineCtrl.lineCtrlReverse,
                       (int)infoRec->rec.lineCtrl.lineCtrlPowerDenial);
                removeLastChar;
                break;
            case RIL_CDMA_T53_CLIR_INFO_REC:
                p.writeInt32((int)(infoRec->rec.clir.cause));

                appendPrintBuf("%scause%d", printBuf, infoRec->rec.clir.cause);
                removeLastChar;
                break;
            case RIL_CDMA_T53_AUDIO_CONTROL_INFO_REC:
                p.writeInt32(infoRec->rec.audioCtrl.upLink);
                p.writeInt32(infoRec->rec.audioCtrl.downLink);

                appendPrintBuf("%supLink=%d, downLink=%d, ", printBuf,
                        infoRec->rec.audioCtrl.upLink,
                        infoRec->rec.audioCtrl.downLink);
                removeLastChar;
                break;
            case RIL_CDMA_T53_RELEASE_INFO_REC:
                // TODO(Moto): See David Krause, he has the answer:)
                RILLOGE("RIL_CDMA_T53_RELEASE_INFO_REC: return INVALID_RESPONSE");
                return RIL_ERRNO_INVALID_RESPONSE;
            default:
                RILLOGE("Incorrect name value");
                return RIL_ERRNO_INVALID_RESPONSE;
        }
    }
    closeResponse;

    return 0;
}

static int responseRilSignalStrength(Parcel &p,
                    void *response, size_t responselen) {
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen >= sizeof (RIL_SignalStrength_v5)) {
        RIL_SignalStrength_v10 *p_cur = ((RIL_SignalStrength_v10 *) response);

        p.writeInt32(p_cur->GW_SignalStrength.signalStrength);
        p.writeInt32(p_cur->GW_SignalStrength.bitErrorRate);
        p.writeInt32(p_cur->CDMA_SignalStrength.dbm);
        p.writeInt32(p_cur->CDMA_SignalStrength.ecio);
        p.writeInt32(p_cur->EVDO_SignalStrength.dbm);
        p.writeInt32(p_cur->EVDO_SignalStrength.ecio);
        p.writeInt32(p_cur->EVDO_SignalStrength.signalNoiseRatio);
        if (responselen >= sizeof (RIL_SignalStrength_v6)) {
            /*
             * Fixup LTE for backwards compatibility
             */
            if (s_callbacks.version <= 6) {
                // signalStrength: -1 -> 99
                if (p_cur->LTE_SignalStrength.signalStrength == -1) {
                    p_cur->LTE_SignalStrength.signalStrength = 99;
                }
                // rsrp: -1 -> INT_MAX all other negative value to positive.
                // So remap here
                if (p_cur->LTE_SignalStrength.rsrp == -1) {
                    p_cur->LTE_SignalStrength.rsrp = INT_MAX;
                } else if (p_cur->LTE_SignalStrength.rsrp < -1) {
                    p_cur->LTE_SignalStrength.rsrp = -p_cur->LTE_SignalStrength.rsrp;
                }
                // rsrq: -1 -> INT_MAX
                if (p_cur->LTE_SignalStrength.rsrq == -1) {
                    p_cur->LTE_SignalStrength.rsrq = INT_MAX;
                }
                // Not remapping rssnr is already using INT_MAX

                // cqi: -1 -> INT_MAX
                if (p_cur->LTE_SignalStrength.cqi == -1) {
                    p_cur->LTE_SignalStrength.cqi = INT_MAX;
                }
            }
            p.writeInt32(p_cur->LTE_SignalStrength.signalStrength);
            p.writeInt32(p_cur->LTE_SignalStrength.rsrp);
            p.writeInt32(p_cur->LTE_SignalStrength.rsrq);
            p.writeInt32(p_cur->LTE_SignalStrength.rssnr);
            p.writeInt32(p_cur->LTE_SignalStrength.cqi);
            if (responselen >= sizeof (RIL_SignalStrength_v10)) {
                p.writeInt32(p_cur->TD_SCDMA_SignalStrength.rscp);
            } else {
                p.writeInt32(INT_MAX);
            }
        } else {
            p.writeInt32(99);
            p.writeInt32(INT_MAX);
            p.writeInt32(INT_MAX);
            p.writeInt32(INT_MAX);
            p.writeInt32(INT_MAX);
            p.writeInt32(INT_MAX);
        }

        startResponse;
        appendPrintBuf("%s[signalStrength=%d,bitErrorRate=%d,\
                CDMA_SS.dbm=%d,CDMA_SSecio=%d,\
                EVDO_SS.dbm=%d,EVDO_SS.ecio=%d,\
                EVDO_SS.signalNoiseRatio=%d,\
                LTE_SS.signalStrength=%d,LTE_SS.rsrp=%d,LTE_SS.rsrq=%d,\
                LTE_SS.rssnr=%d,LTE_SS.cqi=%d,TDSCDMA_SS.rscp=%d]",
                printBuf,
                p_cur->GW_SignalStrength.signalStrength,
                p_cur->GW_SignalStrength.bitErrorRate,
                p_cur->CDMA_SignalStrength.dbm,
                p_cur->CDMA_SignalStrength.ecio,
                p_cur->EVDO_SignalStrength.dbm,
                p_cur->EVDO_SignalStrength.ecio,
                p_cur->EVDO_SignalStrength.signalNoiseRatio,
                p_cur->LTE_SignalStrength.signalStrength,
                p_cur->LTE_SignalStrength.rsrp,
                p_cur->LTE_SignalStrength.rsrq,
                p_cur->LTE_SignalStrength.rssnr,
                p_cur->LTE_SignalStrength.cqi,
                p_cur->TD_SCDMA_SignalStrength.rscp);
        closeResponse;

    } else {
        RILLOGE("invalid response length");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    return 0;
}

static int responseCallRing(Parcel &p, void *response, size_t responselen) {
    if ((response == NULL) || (responselen == 0)) {
        return responseVoid(p, response, responselen);
    } else {
        return responseCdmaSignalInfoRecord(p, response, responselen);
    }
}

static int responseCdmaSignalInfoRecord(Parcel &p, void *response, size_t responselen) {
    if (response == NULL || responselen == 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof (RIL_CDMA_SignalInfoRecord)) {
        RILLOGE("invalid response length %d expected sizeof (RIL_CDMA_SignalInfoRecord) of %d\n",
            (int)responselen, (int)sizeof (RIL_CDMA_SignalInfoRecord));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;

    RIL_CDMA_SignalInfoRecord *p_cur = ((RIL_CDMA_SignalInfoRecord *) response);
    marshallSignalInfoRecord(p, *p_cur);

    appendPrintBuf("%s[isPresent=%d,signalType=%d,alertPitch=%d\
              signal=%d]",
              printBuf,
              p_cur->isPresent,
              p_cur->signalType,
              p_cur->alertPitch,
              p_cur->signal);

    closeResponse;
    return 0;
}

static int responseCdmaCallWaiting(Parcel &p, void *response,
            size_t responselen) {
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen < sizeof(RIL_CDMA_CallWaiting_v6)) {
        RILLOGW("Upgrade to ril version %d\n", RIL_VERSION);
    }

    RIL_CDMA_CallWaiting_v6 *p_cur = ((RIL_CDMA_CallWaiting_v6 *) response);

    writeStringToParcel(p, p_cur->number);
    p.writeInt32(p_cur->numberPresentation);
    writeStringToParcel(p, p_cur->name);
    marshallSignalInfoRecord(p, p_cur->signalInfoRecord);

    if (responselen >= sizeof(RIL_CDMA_CallWaiting_v6)) {
        p.writeInt32(p_cur->number_type);
        p.writeInt32(p_cur->number_plan);
    } else {
        p.writeInt32(0);
        p.writeInt32(0);
    }

    startResponse;
    appendPrintBuf("%snumber=%s,numberPresentation=%d, name=%s,\
            signalInfoRecord[isPresent=%d,signalType=%d,alertPitch=%d\
            signal=%d,number_type=%d,number_plan=%d]",
            printBuf,
            p_cur->number,
            p_cur->numberPresentation,
            p_cur->name,
            p_cur->signalInfoRecord.isPresent,
            p_cur->signalInfoRecord.signalType,
            p_cur->signalInfoRecord.alertPitch,
            p_cur->signalInfoRecord.signal,
            p_cur->number_type,
            p_cur->number_plan);
    closeResponse;

    return 0;
}

static int responseSimRefresh(Parcel &p, void *response, size_t responselen) {
    if (response == NULL || responselen == 0) {
        RILLOGE("responseSimRefresh: invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    if (s_callbacks.version == 7) {
        RIL_SimRefreshResponse_v7 *p_cur = ((RIL_SimRefreshResponse_v7 *) response);
        p.writeInt32(p_cur->result);
        p.writeInt32(p_cur->ef_id);
        writeStringToParcel(p, p_cur->aid);

        appendPrintBuf("%sresult=%d, ef_id=%d, aid=%s",
                printBuf,
                p_cur->result,
                p_cur->ef_id,
                p_cur->aid);
    } else {
        int *p_cur = ((int *) response);
        p.writeInt32(p_cur[0]);
        p.writeInt32(p_cur[1]);
        writeStringToParcel(p, NULL);

        appendPrintBuf("%sresult=%d, ef_id=%d",
                printBuf,
                p_cur[0],
                p_cur[1]);
    }
    closeResponse;

    return 0;
}

static int responseCellInfoList(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL && responselen != 0) {
        RLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_CellInfo) != 0) {
        RLOGE("responseCellInfoList: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_CellInfo));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    int num = responselen / sizeof(RIL_CellInfo);
    p.writeInt32(num);

    RIL_CellInfo *p_cur = (RIL_CellInfo *) response;
    startResponse;
    int i;
    for (i = 0; i < num; i++) {
        appendPrintBuf("%s[%d: type=%d,registered=%d,timeStampType=%d,timeStamp=%lld", printBuf, i,
            p_cur->cellInfoType, p_cur->registered, p_cur->timeStampType, p_cur->timeStamp);
        p.writeInt32((int)p_cur->cellInfoType);
        p.writeInt32(p_cur->registered);
        p.writeInt32(p_cur->timeStampType);
        p.writeInt64(p_cur->timeStamp);
        switch(p_cur->cellInfoType) {
            case RIL_CELL_INFO_TYPE_GSM: {
                appendPrintBuf("%s GSM id: mcc=%d,mnc=%d,lac=%d,cid=%d,", printBuf,
                    p_cur->CellInfo.gsm.cellIdentityGsm.mcc,
                    p_cur->CellInfo.gsm.cellIdentityGsm.mnc,
                    p_cur->CellInfo.gsm.cellIdentityGsm.lac,
                    p_cur->CellInfo.gsm.cellIdentityGsm.cid);
                appendPrintBuf("%s gsmSS: ss=%d,ber=%d],", printBuf,
                    p_cur->CellInfo.gsm.signalStrengthGsm.signalStrength,
                    p_cur->CellInfo.gsm.signalStrengthGsm.bitErrorRate);

                p.writeInt32(p_cur->CellInfo.gsm.cellIdentityGsm.mcc);
                p.writeInt32(p_cur->CellInfo.gsm.cellIdentityGsm.mnc);
                p.writeInt32(p_cur->CellInfo.gsm.cellIdentityGsm.lac);
                p.writeInt32(p_cur->CellInfo.gsm.cellIdentityGsm.cid);
                p.writeInt32(p_cur->CellInfo.gsm.signalStrengthGsm.signalStrength);
                p.writeInt32(p_cur->CellInfo.gsm.signalStrengthGsm.bitErrorRate);
                break;
            }
            case RIL_CELL_INFO_TYPE_WCDMA: {
                appendPrintBuf("%s WCDMA id: mcc=%d,mnc=%d,lac=%d,cid=%d,psc=%d,", printBuf,
                    p_cur->CellInfo.wcdma.cellIdentityWcdma.mcc,
                    p_cur->CellInfo.wcdma.cellIdentityWcdma.mnc,
                    p_cur->CellInfo.wcdma.cellIdentityWcdma.lac,
                    p_cur->CellInfo.wcdma.cellIdentityWcdma.cid,
                    p_cur->CellInfo.wcdma.cellIdentityWcdma.psc);
                appendPrintBuf("%s wcdmaSS: ss=%d,ber=%d],", printBuf,
                    p_cur->CellInfo.wcdma.signalStrengthWcdma.signalStrength,
                    p_cur->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate);

                p.writeInt32(p_cur->CellInfo.wcdma.cellIdentityWcdma.mcc);
                p.writeInt32(p_cur->CellInfo.wcdma.cellIdentityWcdma.mnc);
                p.writeInt32(p_cur->CellInfo.wcdma.cellIdentityWcdma.lac);
                p.writeInt32(p_cur->CellInfo.wcdma.cellIdentityWcdma.cid);
                p.writeInt32(p_cur->CellInfo.wcdma.cellIdentityWcdma.psc);
                p.writeInt32(p_cur->CellInfo.wcdma.signalStrengthWcdma.signalStrength);
                p.writeInt32(p_cur->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate);
                break;
            }
            case RIL_CELL_INFO_TYPE_CDMA: {
                appendPrintBuf("%s CDMA id: nId=%d,sId=%d,bsId=%d,long=%d,lat=%d", printBuf,
                    p_cur->CellInfo.cdma.cellIdentityCdma.networkId,
                    p_cur->CellInfo.cdma.cellIdentityCdma.systemId,
                    p_cur->CellInfo.cdma.cellIdentityCdma.basestationId,
                    p_cur->CellInfo.cdma.cellIdentityCdma.longitude,
                    p_cur->CellInfo.cdma.cellIdentityCdma.latitude);

                p.writeInt32(p_cur->CellInfo.cdma.cellIdentityCdma.networkId);
                p.writeInt32(p_cur->CellInfo.cdma.cellIdentityCdma.systemId);
                p.writeInt32(p_cur->CellInfo.cdma.cellIdentityCdma.basestationId);
                p.writeInt32(p_cur->CellInfo.cdma.cellIdentityCdma.longitude);
                p.writeInt32(p_cur->CellInfo.cdma.cellIdentityCdma.latitude);

                appendPrintBuf("%s cdmaSS: dbm=%d ecio=%d evdoSS: dbm=%d,ecio=%d,snr=%d", printBuf,
                    p_cur->CellInfo.cdma.signalStrengthCdma.dbm,
                    p_cur->CellInfo.cdma.signalStrengthCdma.ecio,
                    p_cur->CellInfo.cdma.signalStrengthEvdo.dbm,
                    p_cur->CellInfo.cdma.signalStrengthEvdo.ecio,
                    p_cur->CellInfo.cdma.signalStrengthEvdo.signalNoiseRatio);

                p.writeInt32(p_cur->CellInfo.cdma.signalStrengthCdma.dbm);
                p.writeInt32(p_cur->CellInfo.cdma.signalStrengthCdma.ecio);
                p.writeInt32(p_cur->CellInfo.cdma.signalStrengthEvdo.dbm);
                p.writeInt32(p_cur->CellInfo.cdma.signalStrengthEvdo.ecio);
                p.writeInt32(p_cur->CellInfo.cdma.signalStrengthEvdo.signalNoiseRatio);
                break;
            }
            case RIL_CELL_INFO_TYPE_LTE: {
                appendPrintBuf("%s LTE id: mcc=%d,mnc=%d,ci=%d,pci=%d,tac=%d", printBuf,
                    p_cur->CellInfo.lte.cellIdentityLte.mcc,
                    p_cur->CellInfo.lte.cellIdentityLte.mnc,
                    p_cur->CellInfo.lte.cellIdentityLte.ci,
                    p_cur->CellInfo.lte.cellIdentityLte.pci,
                    p_cur->CellInfo.lte.cellIdentityLte.tac);

                p.writeInt32(p_cur->CellInfo.lte.cellIdentityLte.mcc);
                p.writeInt32(p_cur->CellInfo.lte.cellIdentityLte.mnc);
                p.writeInt32(p_cur->CellInfo.lte.cellIdentityLte.ci);
                p.writeInt32(p_cur->CellInfo.lte.cellIdentityLte.pci);
                p.writeInt32(p_cur->CellInfo.lte.cellIdentityLte.tac);

                appendPrintBuf("%s lteSS: ss=%d,rsrp=%d,rsrq=%d,rssnr=%d,cqi=%d,ta=%d", printBuf,
                    p_cur->CellInfo.lte.signalStrengthLte.signalStrength,
                    p_cur->CellInfo.lte.signalStrengthLte.rsrp,
                    p_cur->CellInfo.lte.signalStrengthLte.rsrq,
                    p_cur->CellInfo.lte.signalStrengthLte.rssnr,
                    p_cur->CellInfo.lte.signalStrengthLte.cqi,
                    p_cur->CellInfo.lte.signalStrengthLte.timingAdvance);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.signalStrength);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.rsrp);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.rsrq);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.rssnr);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.cqi);
                p.writeInt32(p_cur->CellInfo.lte.signalStrengthLte.timingAdvance);
                break;
            }
            case RIL_CELL_INFO_TYPE_TD_SCDMA: {
                appendPrintBuf("%s TDSCDMA id: mcc=%d,mnc=%d,lac=%d,cid=%d,cpid=%d,", printBuf,
                    p_cur->CellInfo.tdscdma.cellIdentityTdscdma.mcc,
                    p_cur->CellInfo.tdscdma.cellIdentityTdscdma.mnc,
                    p_cur->CellInfo.tdscdma.cellIdentityTdscdma.lac,
                    p_cur->CellInfo.tdscdma.cellIdentityTdscdma.cid,
                    p_cur->CellInfo.tdscdma.cellIdentityTdscdma.cpid);
                appendPrintBuf("%s tdscdmaSS: rscp=%d],", printBuf,
                    p_cur->CellInfo.tdscdma.signalStrengthTdscdma.rscp);

                    p.writeInt32(p_cur->CellInfo.tdscdma.cellIdentityTdscdma.mcc);
                    p.writeInt32(p_cur->CellInfo.tdscdma.cellIdentityTdscdma.mnc);
                    p.writeInt32(p_cur->CellInfo.tdscdma.cellIdentityTdscdma.lac);
                    p.writeInt32(p_cur->CellInfo.tdscdma.cellIdentityTdscdma.cid);
                    p.writeInt32(p_cur->CellInfo.tdscdma.cellIdentityTdscdma.cpid);
                    p.writeInt32(p_cur->CellInfo.tdscdma.signalStrengthTdscdma.rscp);
                    break;
            }
        }
        p_cur += 1;
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseHardwareConfig(Parcel &p, void *response, size_t responselen)
{
   if (response == NULL && responselen != 0) {
       RLOGE("invalid response: NULL");
       return RIL_ERRNO_INVALID_RESPONSE;
   }

   if (responselen % sizeof(RIL_HardwareConfig) != 0) {
       RLOGE("responseHardwareConfig: invalid response length %d expected multiple of %d",
          (int)responselen, (int)sizeof(RIL_HardwareConfig));
       return RIL_ERRNO_INVALID_RESPONSE;
   }

   int num = responselen / sizeof(RIL_HardwareConfig);
   int i;
   RIL_HardwareConfig *p_cur = (RIL_HardwareConfig *) response;

   p.writeInt32(num);

   startResponse;
   for (i = 0; i < num; i++) {
      switch (p_cur[i].type) {
         case RIL_HARDWARE_CONFIG_MODEM: {
            writeStringToParcel(p, p_cur[i].uuid);
            p.writeInt32((int)p_cur[i].state);
            p.writeInt32(p_cur[i].cfg.modem.rat);
            p.writeInt32(p_cur[i].cfg.modem.maxVoice);
            p.writeInt32(p_cur[i].cfg.modem.maxData);
            p.writeInt32(p_cur[i].cfg.modem.maxStandby);

            appendPrintBuf("%s modem: uuid=%s,state=%d,rat=%08x,maxV=%d,maxD=%d,maxS=%d", printBuf,
               p_cur[i].uuid, (int)p_cur[i].state, p_cur[i].cfg.modem.rat,
               p_cur[i].cfg.modem.maxVoice, p_cur[i].cfg.modem.maxData, p_cur[i].cfg.modem.maxStandby);
            break;
         }
         case RIL_HARDWARE_CONFIG_SIM: {
            writeStringToParcel(p, p_cur[i].uuid);
            p.writeInt32((int)p_cur[i].state);
            writeStringToParcel(p, p_cur[i].cfg.sim.modemUuid);

            appendPrintBuf("%s sim: uuid=%s,state=%d,modem-uuid=%s", printBuf,
               p_cur[i].uuid, (int)p_cur[i].state, p_cur[i].cfg.sim.modemUuid);
            break;
         }
      }
   }
   removeLastChar;
   closeResponse;
   return 0;
}

static int responseRadioCapability(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof (RIL_RadioCapability) ) {
        RLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_SIM_IO_Response));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_RadioCapability *p_cur = (RIL_RadioCapability *) response;
    p.writeInt32(p_cur->version);
    p.writeInt32(p_cur->session);
    p.writeInt32(p_cur->phase);
    p.writeInt32(p_cur->rat);
    writeStringToParcel(p, p_cur->logicalModemUuid);
    p.writeInt32(p_cur->status);

    startResponse;
    appendPrintBuf("%s[version=%d,session=%d,phase=%d,\
            rat=%s,logicalModemUuid=%s,status=%d]",
            printBuf,
            p_cur->version,
            p_cur->session,
            p_cur->phase,
            p_cur->rat,
            p_cur->logicalModemUuid,
            p_cur->status);
    closeResponse;
    return 0;
}

static int responseSSData(Parcel &p, void *response, size_t responselen) {
    RLOGD("In responseSSData");
    int num;

    if (response == NULL && responselen != 0) {
        RLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_SIM_IO_Response));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_StkCcUnsolSsResponse)) {
        RLOGE("invalid response length %d, expected %d",
               (int)responselen, (int)sizeof(RIL_StkCcUnsolSsResponse));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    RIL_StkCcUnsolSsResponse *p_cur = (RIL_StkCcUnsolSsResponse *) response;
    p.writeInt32(p_cur->serviceType);
    p.writeInt32(p_cur->requestType);
    p.writeInt32(p_cur->teleserviceType);
    p.writeInt32(p_cur->serviceClass);
    p.writeInt32(p_cur->result);

    if (isServiceTypeCfQuery(p_cur->serviceType, p_cur->requestType)) {
        RLOGD("responseSSData CF type, num of Cf elements %d", p_cur->cfData.numValidIndexes);
        if (p_cur->cfData.numValidIndexes > NUM_SERVICE_CLASSES) {
            RLOGE("numValidIndexes is greater than max value %d, "
                  "truncating it to max value", NUM_SERVICE_CLASSES);
            p_cur->cfData.numValidIndexes = NUM_SERVICE_CLASSES;
        }
        /* number of call info's */
        p.writeInt32(p_cur->cfData.numValidIndexes);

        for (int i = 0; i < p_cur->cfData.numValidIndexes; i++) {
             RIL_CallForwardInfo cf = p_cur->cfData.cfInfo[i];

             p.writeInt32(cf.status);
             p.writeInt32(cf.reason);
             p.writeInt32(cf.serviceClass);
             p.writeInt32(cf.toa);
             writeStringToParcel(p, cf.number);
             p.writeInt32(cf.timeSeconds);
             appendPrintBuf("%s[%s,reason=%d,cls=%d,toa=%d,%s,tout=%d],", printBuf,
                 (cf.status==1)?"enable":"disable", cf.reason, cf.serviceClass, cf.toa,
                  (char*)cf.number, cf.timeSeconds);
             RLOGD("Data: %d,reason=%d,cls=%d,toa=%d,num=%s,tout=%d],", cf.status,
                  cf.reason, cf.serviceClass, cf.toa, (char*)cf.number, cf.timeSeconds);
        }
    } else {
        p.writeInt32 (SS_INFO_MAX);

        /* each int*/
        for (int i = 0; i < SS_INFO_MAX; i++) {
             appendPrintBuf("%s%d,", printBuf, p_cur->ssInfo[i]);
             RLOGD("Data: %d",p_cur->ssInfo[i]);
             p.writeInt32(p_cur->ssInfo[i]);
        }
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static bool isServiceTypeCfQuery(RIL_SsServiceType serType, RIL_SsRequestType reqType) {
    if ((reqType == SS_INTERROGATION) &&
        (serType == SS_CFU ||
         serType == SS_CF_BUSY ||
         serType == SS_CF_NO_REPLY ||
         serType == SS_CF_NOT_REACHABLE ||
         serType == SS_CF_ALL ||
         serType == SS_CF_ALL_CONDITIONAL)) {
        return true;
    }
    return false;
}

static void triggerEvLoop() {
    int ret;
    if (!pthread_equal(pthread_self(), s_tid_dispatch)) {
        /* trigger event loop to wakeup. No reason to do this,
         * if we're in the event loop thread */
         do {
            ret = write (s_fdWakeupWrite, " ", 1);
         } while (ret < 0 && errno == EINTR);
    }
}

static void rilEventAddWakeup(struct ril_event *ev) {
    ril_event_add(ev);
    triggerEvLoop();
}

static void sendSimStatusAppInfo(Parcel &p, int num_apps, RIL_AppStatus appStatus[]) {
        p.writeInt32(num_apps);
        startResponse;
        for (int i = 0; i < num_apps; i++) {
            p.writeInt32(appStatus[i].app_type);
            p.writeInt32(appStatus[i].app_state);
            p.writeInt32(appStatus[i].perso_substate);
            writeStringToParcel(p, (const char*)(appStatus[i].aid_ptr));
            writeStringToParcel(p, (const char*)
                                          (appStatus[i].app_label_ptr));
            p.writeInt32(appStatus[i].pin1_replaced);
            p.writeInt32(appStatus[i].pin1);
            p.writeInt32(appStatus[i].pin2);
            appendPrintBuf("%s[app_type=%d,app_state=%d,perso_substate=%d,\
                    aid_ptr=%s,app_label_ptr=%s,pin1_replaced=%d,pin1=%d,pin2=%d],",
                    printBuf,
                    appStatus[i].app_type,
                    appStatus[i].app_state,
                    appStatus[i].perso_substate,
                    appStatus[i].aid_ptr,
                    appStatus[i].app_label_ptr,
                    appStatus[i].pin1_replaced,
                    appStatus[i].pin1,
                    appStatus[i].pin2);
        }
        closeResponse;
}

static int responseSimStatus(Parcel &p, void *response, size_t responselen) {
    int i;

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen == sizeof (RIL_CardStatus_v6)) {
        RIL_CardStatus_v6 *p_cur = ((RIL_CardStatus_v6 *) response);

        p.writeInt32(p_cur->card_state);
        p.writeInt32(p_cur->universal_pin_state);
        p.writeInt32(p_cur->gsm_umts_subscription_app_index);
        p.writeInt32(p_cur->cdma_subscription_app_index);
        p.writeInt32(p_cur->ims_subscription_app_index);

        sendSimStatusAppInfo(p, p_cur->num_applications, p_cur->applications);
    } else if (responselen == sizeof (RIL_CardStatus_v5)) {
        RIL_CardStatus_v5 *p_cur = ((RIL_CardStatus_v5 *) response);

        p.writeInt32(p_cur->card_state);
        p.writeInt32(p_cur->universal_pin_state);
        p.writeInt32(p_cur->gsm_umts_subscription_app_index);
        p.writeInt32(p_cur->cdma_subscription_app_index);
        p.writeInt32(-1);

        sendSimStatusAppInfo(p, p_cur->num_applications, p_cur->applications);
    } else {
        RILLOGE("responseSimStatus: A RilCardStatus_v6 or _v5 expected\n");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    return 0;
}

static int responseGsmBrSmsCnf(Parcel &p, void *response, size_t responselen) {
    int num = responselen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);
    p.writeInt32(num);

    startResponse;
    RIL_GSM_BroadcastSmsConfigInfo **p_cur =
                (RIL_GSM_BroadcastSmsConfigInfo **) response;
    for (int i = 0; i < num; i++) {
        p.writeInt32(p_cur[i]->fromServiceId);
        p.writeInt32(p_cur[i]->toServiceId);
        p.writeInt32(p_cur[i]->fromCodeScheme);
        p.writeInt32(p_cur[i]->toCodeScheme);
        p.writeInt32(p_cur[i]->selected);

        appendPrintBuf("%s [%d: fromServiceId=%d, toServiceId=%d, \
                fromCodeScheme=%d, toCodeScheme=%d, selected =%d]",
                printBuf, i, p_cur[i]->fromServiceId, p_cur[i]->toServiceId,
                p_cur[i]->fromCodeScheme, p_cur[i]->toCodeScheme,
                p_cur[i]->selected);
    }
    closeResponse;

    return 0;
}

static int responseCdmaBrSmsCnf(Parcel &p, void *response, size_t responselen) {
    RIL_CDMA_BroadcastSmsConfigInfo **p_cur =
               (RIL_CDMA_BroadcastSmsConfigInfo **) response;

    int num = responselen / sizeof (RIL_CDMA_BroadcastSmsConfigInfo *);
    p.writeInt32(num);

    startResponse;
    for (int i = 0 ; i < num ; i++ ) {
        p.writeInt32(p_cur[i]->service_category);
        p.writeInt32(p_cur[i]->language);
        p.writeInt32(p_cur[i]->selected);

        appendPrintBuf("%s [%d: srvice_category=%d, language =%d, \
              selected =%d], ",
              printBuf, i, p_cur[i]->service_category, p_cur[i]->language,
              p_cur[i]->selected);
    }
    closeResponse;

    return 0;
}

static int responseCdmaSms(Parcel &p, void *response, size_t responselen) {
    int num;
    int digitCount;
    int digitLimit;
    uint8_t uct;
    void* dest;

    RILLOGD("Inside responseCdmaSms");

    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_CDMA_SMS_Message)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof(RIL_CDMA_SMS_Message));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_CDMA_SMS_Message *p_cur = (RIL_CDMA_SMS_Message *) response;
    p.writeInt32(p_cur->uTeleserviceID);
    p.write(&(p_cur->bIsServicePresent),sizeof(uct));
    p.writeInt32(p_cur->uServicecategory);
    p.writeInt32(p_cur->sAddress.digit_mode);
    p.writeInt32(p_cur->sAddress.number_mode);
    p.writeInt32(p_cur->sAddress.number_type);
    p.writeInt32(p_cur->sAddress.number_plan);
    p.write(&(p_cur->sAddress.number_of_digits), sizeof(uct));
    digitLimit= MIN((p_cur->sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
        p.write(&(p_cur->sAddress.digits[digitCount]),sizeof(uct));
    }

    p.writeInt32(p_cur->sSubAddress.subaddressType);
    p.write(&(p_cur->sSubAddress.odd),sizeof(uct));
    p.write(&(p_cur->sSubAddress.number_of_digits),sizeof(uct));
    digitLimit= MIN((p_cur->sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
        p.write(&(p_cur->sSubAddress.digits[digitCount]),sizeof(uct));
    }

    digitLimit= MIN((p_cur->uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
    p.writeInt32(p_cur->uBearerDataLen);
    for(digitCount =0 ; digitCount < digitLimit; digitCount ++) {
       p.write(&(p_cur->aBearerData[digitCount]), sizeof(uct));
    }

    startResponse;
    appendPrintBuf("%suTeleserviceID=%d, bIsServicePresent=%d, uServicecategory=%d, \
            sAddress.digit_mode=%d, sAddress.number_mode=%d, sAddress.number_type=%d, ",
            printBuf, p_cur->uTeleserviceID,p_cur->bIsServicePresent,p_cur->uServicecategory,
            p_cur->sAddress.digit_mode, p_cur->sAddress.number_mode,p_cur->sAddress.number_type);
    closeResponse;

    return 0;
}

static int responseDcRtInfo(Parcel &p, void *response, size_t responselen)
{
    int num = responselen / sizeof(RIL_DcRtInfo);
    if ((responselen % sizeof(RIL_DcRtInfo) != 0) || (num != 1)) {
        RLOGE("responseDcRtInfo: invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_DcRtInfo));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    RIL_DcRtInfo *pDcRtInfo = (RIL_DcRtInfo *)response;
    p.writeInt64(pDcRtInfo->time);
    p.writeInt32(pDcRtInfo->powerState);
    appendPrintBuf("%s[time=%d,powerState=%d]", printBuf,
        pDcRtInfo->time,
        pDcRtInfo->powerState);
    closeResponse;

    return 0;
}

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static int responseImsSendSms(Parcel &p, void *response, size_t responselen)
{
    /*FIXME*/
    return 0;
}

static int responseDataCallProfile(Parcel &p, void *response, size_t responselen)
{
    /*FIXME*/
    return 0;
}

static int responseGetUiccSub(Parcel &p, void *response, size_t responselen)
{
    /*FIXME*/
    return 0;
}

static int responseGetCBConf(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL) {
        RILLOGE("responseGetCBConf: invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_CB_ConfigArgs)) {
        RILLOGE("responseGetCBConf: invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_CB_ConfigArgs));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_CB_ConfigArgs *p_cur = (RIL_CB_ConfigArgs *) response;
    p.writeInt32(p_cur->bCBEnabled);
    p.writeInt32(p_cur->selectedId);
    p.writeInt32(p_cur->msgIdMaxCount);
    p.writeInt32(p_cur->msgIdCount);
    writeStringToParcel(p, p_cur->msgIDs);

    startResponse;
    appendPrintBuf("%sbCBEnabled=%d, selectedId=%d,msgIdMaxCount=%d,msgIdCount=%d,msgIDs=%s",
                   printBuf, p_cur->bCBEnabled, p_cur->selectedId, p_cur->msgIdMaxCount,
                   p_cur->msgIdCount,p_cur->msgIDs);
    closeResponse;

    return 0;
}

static int responseGetPB(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen % sizeof(RIL_SIM_PB_Response) != 0) {
        RILLOGE("invalid response length %d expected multiple of %d",
                (int)responselen, (int)sizeof(RIL_SIM_PB_Response));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    int num = responselen / sizeof(RIL_SIM_PB_Response);
    p.writeInt32(num);

    RIL_SIM_PB_Response *p_cur = (RIL_SIM_PB_Response *) response;
    startResponse;
    int i,j;
    for (i = 0; i < num; i++) {
        for (j = 0; j < NUM_OF_ALPHA; j++) {
            p.writeInt32(p_cur[i].lengthAlphas[j]);
        }

        for (j = 0; j < NUM_OF_ALPHA; j++) {
            p.writeInt32(p_cur[i].dataTypeAlphas[j]);
        }

        for (j = 0; j < NUM_OF_ALPHA; j++) {
            writeStringToParcel(p, p_cur[i].alphaTags[j]);
        }

        for (j = 0; j < NUM_OF_NUMBER; j++) {
            p.writeInt32(p_cur[i].lengthNumbers[j]);
        }

        for (j = 0; j < NUM_OF_NUMBER; j++) {
            p.writeInt32(p_cur[i].dataTypeNumbers[j]);
        }

        for (j = 0; j < NUM_OF_NUMBER; j++) {
            writeStringToParcel(p, p_cur[i].numbers[j]);
        }

        p.writeInt32(p_cur[i].recordIndex);
        p.writeInt32(p_cur[i].nextIndex);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseSS(Parcel &p, void *response, size_t responselen)
{
    /*FIXME*/
    return 0;
}

static int responsemsg(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    if (responselen != sizeof(RIL_SSReleaseComplete)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_SSReleaseComplete));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_SSReleaseComplete *p_cur = (RIL_SSReleaseComplete *) response;
    p.writeInt32(p_cur->size);
    p.writeInt32(p_cur->dataLen);
    p.writeInt32(p_cur->params);
    p.writeInt32(p_cur->status);
    writeStringToParcel(p, p_cur->data);

    startResponse;
    appendPrintBuf("%ssize=%d, dataLen=%d,params=%d,status=%d,data=%s",
                   printBuf, p_cur->size, p_cur->dataLen, p_cur->params,
                   p_cur->status,p_cur->data);
    closeResponse;

    return 0;
}

static int responseCCresult(Parcel &p, void *response, size_t responselen)
{
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    if (responselen != sizeof(RIL_StkCallControlResult)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_StkCallControlResult));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_StkCallControlResult *p_cur = (RIL_StkCallControlResult *) response;
    p.writeInt32(p_cur->call_type);
    p.writeInt32(p_cur->result);
    p.writeInt32(p_cur->is_alpha);
    p.writeInt32(p_cur->alpha_len);
    writeStringToParcel(p, p_cur->alpha_data);
    p.writeInt32(p_cur->pre_type);
    p.writeInt32(p_cur->ton);
    p.writeInt32(p_cur->npi);
    p.writeInt32(p_cur->num_len);
    writeStringToParcel(p, p_cur->number);

    startResponse;
    appendPrintBuf("%scall_type=%d, result=%d,is_alpha=%d,alpha_len=%d,alpha_data=%s \
                    pre_type=%d,ton=%d,npi=%d,num_len=%d,number=%s",
                   printBuf, p_cur->call_type, p_cur->result, p_cur->is_alpha,
                   p_cur->alpha_len,p_cur->alpha_data,p_cur->pre_type,p_cur->ton,
                   p_cur->npi,p_cur->num_len,p_cur->number);
    closeResponse;

    return 0;
}

static int responseLockInfo(Parcel &p, void *response, size_t responselen)
{
    /*FIXME*/
    return 0;
}
#endif

#if defined (RIL_SPRD_EXTENSION)
static int responseDSCI(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_VideoPhone_DSCI)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_VideoPhone_DSCI));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_VideoPhone_DSCI *p_cur = (RIL_VideoPhone_DSCI *) response;
    p.writeInt32(p_cur->id);
    p.writeInt32(p_cur->idr);
    p.writeInt32(p_cur->stat);
    p.writeInt32(p_cur->type);
    p.writeInt32(p_cur->mpty);
    writeStringToParcel(p, p_cur->number);
    p.writeInt32(p_cur->num_type);
    p.writeInt32(p_cur->bs_type);
    p.writeInt32(p_cur->cause);
    p.writeInt32(p_cur->location);

    startResponse;
    appendPrintBuf("%sstatus=%d, type=%s,number=%s,cause=%d,location=%d", printBuf,
        p_cur->stat,(p_cur->type==0)?"voice":"video", p_cur->number, p_cur->cause, p_cur->location);
    closeResponse;

    return 0;
}

/** add for LTE-CSFB to handle CS fall back of MT call  */
static int responseCallCsFallBack(Parcel &p, void *response, size_t responselen) {
    if (response == NULL) {
        RILLOGE("invalid response: NULL");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    if (responselen != sizeof(RIL_CALL_CSFALLBACK)) {
        RILLOGE("invalid response length was %d expected %d",
                (int)responselen, (int)sizeof (RIL_CALL_CSFALLBACK));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    RIL_CALL_CSFALLBACK *p_cur = (RIL_CALL_CSFALLBACK *) response;
    p.writeInt32(p_cur->id);
    writeStringToParcel(p, p_cur->number);

    startResponse;
    appendPrintBuf("responseCallCsFallBack: id=%d, number=%s", p_cur->id, p_cur->number);
    closeResponse;
    return 0;
}
#endif

/**
 * A write on the wakeup fd is done just to pop us out of select()
 * We empty the buffer here and then ril_event will reset the timers on the
 * way back down
 */
static void processWakeupCallback(int fd, short flags, void *param) {
    char buff[16];
    int ret;

    RILLOGV("processWakeupCallback");

    /* empty our wakeup socket out */
    do {
        ret = read(s_fdWakeupRead, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}

static void onCommandsSocketClosed() {
    int ret;
    RequestInfo *p_cur;

    /* mark pending requests as "cancelled" so we dont report responses */

    ret = pthread_mutex_lock(&s_pendingRequestsMutex);
    assert (ret == 0);

    p_cur = s_pendingRequests;

    for (p_cur = s_pendingRequests
            ; p_cur != NULL
            ; p_cur  = p_cur->p_next
    ) {
        p_cur->cancelled = 1;
    }

    ret = pthread_mutex_unlock(&s_pendingRequestsMutex);
    assert (ret == 0);
}


void CommandThread(void *arg) {
    pid_t tid;
    tid = gettid();
    commthread_data_t *user_data = (commthread_data_t *)arg;
    processCommandBuffer(user_data->buffer, user_data->buflen);
    RILLOGI("-->CommandThread [%d] free one command\n",tid);
    free(user_data->buffer);
    free(user_data);

}

void list_init(struct listnode *node)
{
    node->next = node;
    node->prev = node;
}

void list_add_tail(struct listnode *head, struct listnode *item)
{
    pthread_mutex_lock(&s_listMutex);
    item->next = head;
    item->prev = head->prev;
    head->prev->next = item;
    head->prev = item;
    pthread_mutex_unlock(&s_listMutex);
}

void list_add_head(struct listnode *head, struct listnode *item)
{
    pthread_mutex_lock(&s_listMutex);
    item->next = head->next;
    item->prev = head;
    head->next->prev = item;
    head->next = item;
    pthread_mutex_unlock(&s_listMutex);
}
void list_remove(struct listnode *item)
{
    pthread_mutex_lock(&s_listMutex);
    item->next->prev = item->prev;
    item->prev->next = item->next;
    pthread_mutex_unlock(&s_listMutex);
}

static void processCommandsCallback(int fd, short flags, void *param) {
    commthread_data_t *user_data = NULL;
    struct listnode * cmd_item = NULL;
    struct listnode * cmd = NULL;
    int ret = 0;
    RecordStream *p_rs= (RecordStream *)param;
    Parcel p;
    status_t status;
    int32_t request;
    int32_t token;
    CommandInfo *pCI;
    char prop[10];

    RILLOGI("enter processCommandsCallback\n");

    assert(fd == s_fdCommand);

    for (;;) {
         void *p_record;
         size_t recordlen;

        /* loop until EAGAIN/EINTR, end of stream, or other error */
        ret = record_stream_get_next(p_rs, &p_record, &recordlen);

        if (ret == 0 && p_record == NULL) {
            record_stream_free(p_rs);
            RILLOGE("PCC end of stream");
            exit(0);
            /* end-of-stream
             * restart rild
             * */
            break;
        } else if (ret < 0) {
            break;
        } else if (ret == 0) { /* && p_record != NULL */
            cmd_item = (struct listnode* )malloc(sizeof(struct listnode));
            if(cmd_item == NULL) {
                RILLOGE("Can not allocate memory for cmd_item");
                exit(-1);
            }
            user_data =(commthread_data_t *)malloc(sizeof(commthread_data_t));
            if(user_data == NULL) {
                RILLOGE("Can not allocate memory for user_data");
                free(cmd_item);
                exit(-1);
            }
            user_data->p_rs = p_rs;
            user_data->buffer =(char *)malloc(recordlen) ;
            if(user_data->buffer == NULL) {
                RILLOGE("Can not allocate memory for user_data buffer");
                free(cmd_item);
                free(user_data);
                exit(-1);
            }
            user_data->buflen = recordlen;

            memcpy(user_data->buffer,p_record,recordlen);

            cmd_item->user_data = user_data;

            p.setData((uint8_t *) (user_data->buffer), user_data->buflen);
            status = p.readInt32(&request);
            status = p.readInt32 (&token);
            if (status != NO_ERROR) {
                RILLOGE("invalid request block");
                free(cmd_item);
                free(user_data->buffer);
                free(user_data);
                return;
            }

            if (request < 1
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
                || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
                || (request > RIL_SPRD_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#else
                || (request > RIL_REQUEST_LAST && request < RIL_OEM_REQUEST_BASE)
#endif
                || (request > RIL_OEM_REQUEST_LAST)
#else
#if defined (RIL_SPRD_EXTENSION)
                || (request > RIL_REQUEST_LAST && request < RIL_SPRD_REQUEST_BASE)
                || (request > RIL_SPRD_REQUEST_LAST)
#else
                || request >= (int32_t)NUM_ELEMS(s_commands)
#endif
#endif
            ) {
                RILLOGE("unsupported request code %d token %d", request, token);
                free(cmd_item);
                free(user_data->buffer);
                free(user_data);
                return;
            }

#if defined (RIL_SPRD_EXTENSION)
            if(request > RIL_SPRD_REQUEST_BASE && request <= RIL_SPRD_REQUEST_LAST)
                request = request - RIL_SPRD_REQUEST_BASE + RIL_REQUEST_LAST;
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            if(request > RIL_OEM_REQUEST_BASE && request <= RIL_OEM_REQUEST_LAST)
#if defined (RIL_SPRD_EXTENSION)
                request = request - RIL_OEM_REQUEST_BASE + RIL_REQUEST_LAST
                                     + RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE;
#else
                request = request - RIL_OEM_REQUEST_BASE + RIL_REQUEST_LAST;
#endif
#endif

            pCI = &(s_commands[request]);

            if(s_multiSimMode) {
                if(pCI->requestNumber == RIL_REQUEST_SEND_SMS
                        || pCI->requestNumber == RIL_REQUEST_QUERY_FACILITY_LOCK
                        || pCI->requestNumber == RIL_REQUEST_SET_FACILITY_LOCK
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
                        || pCI->requestNumber == RIL_REQUEST_SET_CALL_FORWARD
                        || pCI->requestNumber == RIL_REQUEST_GET_CLIR
                        || pCI->requestNumber == RIL_REQUEST_SET_CLIR
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CALL_WAITING
                        || pCI->requestNumber == RIL_REQUEST_SET_CALL_WAITING
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CLIP
#if defined (RIL_SPRD_EXTENSION)
                        || pCI->requestNumber == RIL_REQUEST_SET_CMMS
                        || pCI->requestNumber == RIL_REQUEST_QUERY_COLP
                        || pCI->requestNumber == RIL_REQUEST_QUERY_COLR
#endif
                        || pCI->requestNumber == RIL_REQUEST_RADIO_POWER
                        || pCI->requestNumber == RIL_REQUEST_SETUP_DATA_CALL
                        || pCI->requestNumber == RIL_REQUEST_DEACTIVATE_DATA_CALL) {
                    list_add_tail(&sms_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_dispatchMutex);
                    pthread_cond_signal(&s_dispatchCond);
                    pthread_mutex_unlock(&s_dispatchMutex);
                } else if(pCI->requestNumber == RIL_REQUEST_DTMF
                    || pCI->requestNumber == RIL_REQUEST_DTMF_START
                    || pCI->requestNumber == RIL_REQUEST_DTMF_STOP) {
                    pthread_mutex_lock(&s_listMutex);
                    for(cmd = (&sms_cmd_list)->next; cmd != (&sms_cmd_list); cmd = cmd->next) {
                        p.setData((uint8_t *) (cmd->user_data->buffer), cmd->user_data->buflen);
                        status = p.readInt32(&request);
                        status = p.readInt32 (&token);
                        pCI = &(s_commands[request]);
                        if(pCI->requestNumber != RIL_REQUEST_DTMF
                            && pCI->requestNumber != RIL_REQUEST_DTMF_START
                            && pCI->requestNumber != RIL_REQUEST_DTMF_STOP) {
                            cmd_item->next = cmd;
                            cmd_item->prev = cmd->prev;
                            cmd->prev->next = cmd_item;
                            cmd->prev= cmd_item;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&s_listMutex);
                    if(cmd == (&sms_cmd_list))
                        list_add_tail(&sms_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_dispatchMutex);
                    pthread_cond_signal(&s_dispatchCond);
                    pthread_mutex_unlock(&s_dispatchMutex);
                } else if(pCI->requestNumber == RIL_REQUEST_SCREEN_STATE) {
                    list_add_tail(&local_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_localDispatchMutex);
                    pthread_cond_signal(&s_localDispatchCond);
                    pthread_mutex_unlock(&s_localDispatchMutex);
                } else {
                    list_add_tail(&other_cmd_list, cmd_item);
                }
            } else {
                if(pCI->requestNumber == RIL_REQUEST_SIM_IO
                        || pCI->requestNumber == RIL_REQUEST_QUERY_FACILITY_LOCK
                        || pCI->requestNumber == RIL_REQUEST_SET_FACILITY_LOCK
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
                        || pCI->requestNumber == RIL_REQUEST_SET_CALL_FORWARD
                        || pCI->requestNumber == RIL_REQUEST_GET_CLIR
                        || pCI->requestNumber == RIL_REQUEST_SET_CLIR
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CALL_WAITING
                        || pCI->requestNumber == RIL_REQUEST_SET_CALL_WAITING
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CLIP
#if defined (RIL_SPRD_EXTENSION)
                        || pCI->requestNumber == RIL_REQUEST_QUERY_COLP
                        || pCI->requestNumber == RIL_REQUEST_QUERY_COLR
                        || pCI->requestNumber == RIL_REQUEST_SEND_SMS
                        || pCI->requestNumber == RIL_REQUEST_SET_CMMS
#endif
                        || pCI->requestNumber == RIL_REQUEST_QUERY_CLIP) {
                    list_add_tail(&sms_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_dispatchMutex);
                    pthread_cond_signal(&s_dispatchCond);
                    pthread_mutex_unlock(&s_dispatchMutex);
                } else if(pCI->requestNumber == RIL_REQUEST_DTMF
                    || pCI->requestNumber == RIL_REQUEST_DTMF_START
                    || pCI->requestNumber == RIL_REQUEST_DTMF_STOP) {
                    pthread_mutex_lock(&s_listMutex);
                    for(cmd = (&sms_cmd_list)->next; cmd != (&sms_cmd_list); cmd = cmd->next) {
                        p.setData((uint8_t *) (cmd->user_data->buffer), cmd->user_data->buflen);
                        status = p.readInt32(&request);
                        status = p.readInt32 (&token);
                        pCI = &(s_commands[request]);
                        if(pCI->requestNumber != RIL_REQUEST_DTMF
                            && pCI->requestNumber != RIL_REQUEST_DTMF_START
                            && pCI->requestNumber != RIL_REQUEST_DTMF_STOP) {
                            cmd_item->next = cmd;
                            cmd_item->prev = cmd->prev;
                            cmd->prev->next = cmd_item;
                            cmd->prev= cmd_item;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&s_listMutex);
                    if(cmd == (&sms_cmd_list))
                        list_add_tail(&sms_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_dispatchMutex);
                    pthread_cond_signal(&s_dispatchCond);
                    pthread_mutex_unlock(&s_dispatchMutex);
                } else if(pCI->requestNumber == RIL_REQUEST_SCREEN_STATE) {
                    list_add_tail(&local_cmd_list, cmd_item);
                    pthread_mutex_lock(&s_localDispatchMutex);
                    pthread_cond_signal(&s_localDispatchCond);
                    pthread_mutex_unlock(&s_localDispatchMutex);
                } else {
                    list_add_tail(&other_cmd_list, cmd_item);
                }
            }
        }

        for (cmd_item = (&other_cmd_list)->next; cmd_item != (&other_cmd_list);
                cmd_item = (&other_cmd_list)->next) {
            do {
                RILLOGI("PCC  alloc one command p_record\n");
                ret = thread_pool_dispatch(threadpool_d, CommandThread, cmd_item->user_data);
                if(!ret) {
                    RILLOGE("dispatch a new thread unsuccess\n");
                    sleep(1);
                }
            } while(!ret);
            list_remove(cmd_item);
            free(cmd_item);
        }
    }
}

static void *
smsDispatch(void *param) {
    struct listnode * cmd_item;
    pid_t tid;
    tid = gettid();

    while(1) {
        pthread_mutex_lock(&s_dispatchMutex);
        pthread_cond_wait(&s_dispatchCond, &s_dispatchMutex);
        pthread_mutex_unlock(&s_dispatchMutex);

        for (cmd_item = (&sms_cmd_list)->next; cmd_item != (&sms_cmd_list);
                cmd_item = (&sms_cmd_list)->next) {
            processCommandBuffer(cmd_item->user_data->buffer,
                cmd_item->user_data->buflen);
            RILLOGI("-->SmsDispatch [%d] free one command\n",tid);
            list_remove(cmd_item);  /* remove list node first, then free it */
            free(cmd_item->user_data->buffer);
            free(cmd_item->user_data);
            free(cmd_item);
        }
    }
    return NULL;
}

static void *
localDispatch(void *param) {
    struct listnode * cmd_item;
    pid_t tid;
    tid = gettid();

    while(1) {
        pthread_mutex_lock(&s_localDispatchMutex);
        pthread_cond_wait(&s_localDispatchCond, &s_localDispatchMutex);
        pthread_mutex_unlock(&s_localDispatchMutex);

        for (cmd_item = (&local_cmd_list)->next; cmd_item != (&local_cmd_list);
                cmd_item = (&local_cmd_list)->next) {
            processCommandBuffer(cmd_item->user_data->buffer,
                cmd_item->user_data->buflen);
            RILLOGI("-->localDispatch [%d] free one command\n",tid);
            list_remove(cmd_item);  /* remove list node first, then free it */
            free(cmd_item->user_data->buffer);
            free(cmd_item->user_data);
            free(cmd_item);
        }
    }
    return NULL;
}

static void onNewCommandConnect() {
    // Inform we are connected and the ril version
    int rilVer = s_callbacks.version;
    RIL_onUnsolicitedResponse(RIL_UNSOL_RIL_CONNECTED,
                                    &rilVer, sizeof(rilVer));

    // implicit radio state changed
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                    NULL, 0);

    // unsolicited sim status changed
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,
                                    NULL, 0);

    // Send last NITZ time data, in case it was missed
    if (s_lastNITZTimeData != NULL) {
        sendResponseRaw(s_lastNITZTimeData, s_lastNITZTimeDataSize);

        free(s_lastNITZTimeData);
        s_lastNITZTimeData = NULL;
    }

    // Get version string
    if (s_callbacks.getVersion != NULL) {
        const char *version;
        version = s_callbacks.getVersion();
        RILLOGI("RIL Daemon version: %s\n", version);

        property_set(PROPERTY_RIL_IMPL, version);
    } else {
        RILLOGI("RIL Daemon version: unavailable\n");
        property_set(PROPERTY_RIL_IMPL, "unavailable");
    }

}

static void listenCallback (int fd, short flags, void *param) {
    int ret;
    int err;
    int is_phone_socket;
    RecordStream *p_rs;

    struct sockaddr_un peeraddr;
    socklen_t socklen = sizeof (peeraddr);

    struct ucred creds;
    socklen_t szCreds = sizeof(creds);

    struct passwd *pwd = NULL;

    assert (s_fdCommand < 0);
    assert (fd == s_fdListen);

    s_fdCommand = accept(s_fdListen, (sockaddr *) &peeraddr, &socklen);

    if (s_fdCommand < 0 ) {
        RILLOGE("Error on accept() errno:%d", errno);
        /* start listening for new connections again */
        rilEventAddWakeup(&s_listen_event);
	      return;
    }

    /* check the credential of the other side and only accept socket from
     * phone process
     */
    errno = 0;
    is_phone_socket = 0;

    err = getsockopt(s_fdCommand, SOL_SOCKET, SO_PEERCRED, &creds, &szCreds);

    if (err == 0 && szCreds > 0) {
        errno = 0;
        pwd = getpwuid(creds.uid);
        if (pwd != NULL) {
            if (strcmp(pwd->pw_name, PHONE_PROCESS) == 0) {
                is_phone_socket = 1;
            } else {
                RILLOGE("RILD can't accept socket from process %s", pwd->pw_name);
            }
        } else {
            RILLOGE("Error on getpwuid() errno: %d", errno);
        }
    } else {
        RILLOGD("Error on getsockopt() errno: %d", errno);
    }

    if ( !is_phone_socket ) {
      RILLOGE("RILD must accept socket from %s", PHONE_PROCESS);

      close(s_fdCommand);
      s_fdCommand = -1;

      onCommandsSocketClosed();

      /* start listening for new connections again */
      rilEventAddWakeup(&s_listen_event);

      return;
    }

#if 0
    ret = fcntl(s_fdCommand, F_SETFL, O_NONBLOCK);

    if (ret < 0) {
        RILLOGE ("Error setting O_NONBLOCK errno:%d", errno);
    }
#endif

    RILLOGI("libril: new connection");

    p_rs = record_stream_new(s_fdCommand, MAX_COMMAND_BYTES);

    ril_event_set (&s_commands_event, s_fdCommand, 1,
        processCommandsCallback, p_rs);

    rilEventAddWakeup (&s_commands_event);

    onNewCommandConnect();
}

static void freeDebugCallbackArgs(int number, char **args) {
    for (int i = 0; i < number; i++) {
        if (args[i] != NULL) {
            free(args[i]);
        }
    }
    free(args);
}

static void debugCallback (int fd, short flags, void *param) {
    int acceptFD, option;
    struct sockaddr_un peeraddr;
    socklen_t socklen = sizeof (peeraddr);
    int data;
    unsigned int qxdm_data[6];
    const char *deactData[1] = {"1"};
    char *actData[1];
    RIL_Dial dialData;
    int hangupData[1] = {1};
    int number;
    char **args;

    acceptFD = accept (fd,  (sockaddr *) &peeraddr, &socklen);

    if (acceptFD < 0) {
        RILLOGE ("error accepting on debug port: %d\n", errno);
        return;
    }

    if (recv(acceptFD, &number, sizeof(int), 0) != sizeof(int)) {
        RILLOGE ("error reading on socket: number of Args: \n");
        close(acceptFD);
        return;
    }
    args = (char **) malloc(sizeof(char*) * number);

    for (int i = 0; i < number; i++) {
        int len;
        if (recv(acceptFD, &len, sizeof(int), 0) != sizeof(int)) {
            RILLOGE ("error reading on socket: Len of Args: \n");
            freeDebugCallbackArgs(i, args);
            close(acceptFD);
            return;
        }
        // +1 for null-term
        args[i] = (char *) malloc((sizeof(char) * len) + 1);
        if (recv(acceptFD, args[i], sizeof(char) * len, 0)
            != (int)sizeof(char) * len) {
            RILLOGE ("error reading on socket: Args[%d] \n", i);
            freeDebugCallbackArgs(i, args);
            close(acceptFD);
            return;
        }
        char * buf = args[i];
        buf[len] = 0;
    }

    switch (atoi(args[0])) {
        case 0:
            RILLOGI ("Connection on debug port: issuing reset.");
            issueLocalRequest(RIL_REQUEST_RESET_RADIO, NULL, 0);
            break;
        case 1:
            RILLOGI ("Connection on debug port: issuing radio power off.");
            data = 0;
            issueLocalRequest(RIL_REQUEST_RADIO_POWER, &data, sizeof(int));
            // Close the socket
            close(s_fdCommand);
            s_fdCommand = -1;
            break;
        case 2:
            RILLOGI ("Debug port: issuing unsolicited voice network change.");
            RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                      NULL, 0);
            break;
        case 3:
            RILLOGI ("Debug port: QXDM log enable.");
            qxdm_data[0] = 65536;     // head.func_tag
            qxdm_data[1] = 16;        // head.len
            qxdm_data[2] = 1;         // mode: 1 for 'start logging'
            qxdm_data[3] = 32;        // log_file_size: 32megabytes
            qxdm_data[4] = 0;         // log_mask
            qxdm_data[5] = 8;         // log_max_fileindex
            issueLocalRequest(RIL_REQUEST_OEM_HOOK_RAW, qxdm_data,
                              6 * sizeof(int));
            break;
        case 4:
            RILLOGI ("Debug port: QXDM log disable.");
            qxdm_data[0] = 65536;
            qxdm_data[1] = 16;
            qxdm_data[2] = 0;          // mode: 0 for 'stop logging'
            qxdm_data[3] = 32;
            qxdm_data[4] = 0;
            qxdm_data[5] = 8;
            issueLocalRequest(RIL_REQUEST_OEM_HOOK_RAW, qxdm_data,
                              6 * sizeof(int));
            break;
        case 5:
            RILLOGI("Debug port: Radio On");
            data = 1;
            issueLocalRequest(RIL_REQUEST_RADIO_POWER, &data, sizeof(int));
            sleep(2);
            // Set network selection automatic.
            issueLocalRequest(RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, NULL, 0);
            break;
        case 6:
            RILLOGI("Debug port: Setup Data Call, Apn :%s\n", args[1]);
            actData[0] = args[1];
            issueLocalRequest(RIL_REQUEST_SETUP_DATA_CALL, &actData,
                              sizeof(actData));
            break;
        case 7:
            RILLOGI("Debug port: Deactivate Data Call");
            issueLocalRequest(RIL_REQUEST_DEACTIVATE_DATA_CALL, &deactData,
                              sizeof(deactData));
            break;
        case 8:
            RILLOGI("Debug port: Dial Call");
            dialData.clir = 0;
            dialData.address = args[1];
            issueLocalRequest(RIL_REQUEST_DIAL, &dialData, sizeof(dialData));
            break;
        case 9:
            RILLOGI("Debug port: Answer Call");
            issueLocalRequest(RIL_REQUEST_ANSWER, NULL, 0);
            break;
        case 10:
            RILLOGI("Debug port: End Call");
            issueLocalRequest(RIL_REQUEST_HANGUP, &hangupData,
                              sizeof(hangupData));
            break;
        default:
            RILLOGE ("Invalid request");
            break;
    }
    freeDebugCallbackArgs(number, args);
    close(acceptFD);
}


static void userTimerCallback (int fd, short flags, void *param) {
    UserCallbackInfo *p_info;

    p_info = (UserCallbackInfo *)param;

    p_info->p_callback(p_info->userParam);


    // FIXME generalize this...there should be a cancel mechanism
    if (s_last_wake_timeout_info != NULL && s_last_wake_timeout_info == p_info) {
        s_last_wake_timeout_info = NULL;
    }

    free(p_info);
}


static void *
eventLoop(void *param) {
    int ret;
    int filedes[2];

    ril_event_init();

    pthread_mutex_lock(&s_startupMutex);

    s_started = 1;
    pthread_cond_broadcast(&s_startupCond);

    pthread_mutex_unlock(&s_startupMutex);

    ret = pipe(filedes);

    if (ret < 0) {
        RILLOGE("Error in pipe() errno:%d", errno);
        return NULL;
    }

    s_fdWakeupRead = filedes[0];
    s_fdWakeupWrite = filedes[1];

    fcntl(s_fdWakeupRead, F_SETFL, O_NONBLOCK);

    ril_event_set (&s_wakeupfd_event, s_fdWakeupRead, true,
                processWakeupCallback, NULL);

    rilEventAddWakeup (&s_wakeupfd_event);

    // Only returns on error
    ril_event_loop();
    RILLOGE ("error in event_loop_base errno:%d", errno);
    // kill self to restart on error
    kill(0, SIGKILL);

    return NULL;
}

extern "C" void
RIL_startEventLoop(void) {
    int ret;
    pthread_attr_t attr;

    /* spin up eventLoop thread and wait for it to get started */
    s_started = 0;
    pthread_mutex_lock(&s_startupMutex);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&s_tid_dispatch, &attr, eventLoop, NULL);

    while (s_started == 0) {
        pthread_cond_wait(&s_startupCond, &s_startupMutex);
    }

    pthread_mutex_unlock(&s_startupMutex);

    if (ret < 0) {
        RILLOGE("Failed to create dispatch thread errno:%d", errno);
        return;
    }
}

// Used for testing purpose only.
extern "C" void RIL_setcallbacks (const RIL_RadioFunctions *callbacks) {
    memcpy(&s_callbacks, callbacks, sizeof (RIL_RadioFunctions));
}

extern "C" void
RIL_register (const RIL_RadioFunctions *callbacks, int argc, char ** argv) {
    int ret;
    int flags;
    char s_name_ril[20] = {0};
    char s_name_ril_debug[20] = {0};
    pthread_attr_t attr;
    int opt;
    char phoneCount[PROPERTY_VALUE_MAX];
    int count;

    optind = 0;  // reset getopt
    while ( -1 != (opt = getopt(argc, argv, "m:n:"))) {
        switch (opt) {
            case 'm':
                s_modem = optarg;
                break;
            case 'n':
                s_sim_num = atoi(optarg);
                modem = *optarg;
                break;
            default:
                RILLOGE("libril parse parameter error");
                break;
        }
    }

    //modem = *s_modem;
    RILLOGD("it's %s modem rild%d libril", s_modem, s_sim_num);

    if(!strcmp(s_modem, "t")) {
        property_get(TD_SIM_NUM, phoneCount, "");
        if(strcmp(phoneCount, "1"))
            s_multiSimMode = 1;
        else
            s_multiSimMode = 0;
    } else if(!strcmp(s_modem, "w")) {
        property_get(W_SIM_NUM, phoneCount, "");
        if(strcmp(phoneCount, "1"))
            s_multiSimMode = 1;
        else
            s_multiSimMode = 0;
    } else if(!strcmp(s_modem, "l")) {
        property_get(L_SIM_NUM, phoneCount, "");
        if(strcmp(phoneCount, "1"))
            s_multiSimMode = 1;
        else
            s_multiSimMode = 0;
    } else if(!strcmp(s_modem, "tl")) {
        property_get(TL_SIM_NUM, phoneCount, "");
        if(strcmp(phoneCount, "1"))
            s_multiSimMode = 1;
        else
            s_multiSimMode = 0;
    } else if(!strcmp(s_modem, "lf")) {
        property_get(LF_SIM_NUM, phoneCount, "");
        if(strcmp(phoneCount, "1"))
            s_multiSimMode = 1;
        else
            s_multiSimMode = 0;
    } else {
        RILLOGE("Invalid modem type");
        exit(-1);
    }

    if (callbacks == NULL) {
        RILLOGE("RIL_register: RIL_RadioFunctions * null");
        return;
    }
    if (callbacks->version < RIL_VERSION_MIN) {
        RILLOGE("RIL_register: version %d is to old, min version is %d",
             callbacks->version, RIL_VERSION_MIN);
        return;
    }
    if (callbacks->version > RIL_VERSION) {
        RILLOGE("RIL_register: version %d is too new, max version is %d",
             callbacks->version, RIL_VERSION);
        return;
    }
    RILLOGE("RIL_register: RIL version %d", callbacks->version);

    if (s_registerCalled > 0) {
        RILLOGE("RIL_register has been called more than once. "
                "Subsequent call ignored");
        return;
    }

    memcpy(&s_callbacks, callbacks, sizeof (RIL_RadioFunctions));

    s_registerCalled = 1;

    // Little self-check

    for (int i = 0; i <= RIL_REQUEST_LAST; i++) {         
        assert(i == s_commands[i].requestNumber);
    }

#if defined (RIL_SPRD_EXTENSION)
    for (int i = RIL_REQUEST_LAST + 1; i <= RIL_REQUEST_LAST +
               RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE; i++) {
        assert(i - RIL_REQUEST_LAST ==
                        s_commands[i].requestNumber - RIL_SPRD_REQUEST_BASE);
    }
#endif

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
    count = RIL_REQUEST_LAST + RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE;
#else
    count = RIL_REQUEST_LAST;
#endif
    for (int i = count + 1; i <= count + RIL_OEM_REQUEST_LAST - RIL_OEM_REQUEST_BASE; i++) {
        assert(i - count == s_commands[i].requestNumber - RIL_OEM_REQUEST_BASE);
    }
#endif

    for (int i = 0; i <= RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE; i++) {
        assert(i == s_unsolResponses[i].requestNumber - RIL_UNSOL_RESPONSE_BASE);
    }

#if defined (RIL_SPRD_EXTENSION)
    count = RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE;
    for (int i = count + 1; i <= count + RIL_SPRD_UNSOL_RESPONSE_LAST
              - RIL_SPRD_UNSOL_RESPONSE_BASE + 1; i++) {
        assert(i - count - 1 == s_unsolResponses[i].requestNumber
                        - RIL_SPRD_UNSOL_RESPONSE_BASE);
    }
#endif

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
    count = RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE
                      + RIL_SPRD_UNSOL_RESPONSE_LAST - RIL_SPRD_UNSOL_RESPONSE_BASE + 1;
#else
    count = RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE;
#endif
    for (int i = count + 1; i <= count + RIL_OEM_UNSOL_LAST
              - RIL_OEM_UNSOL_RESPONSE_BASE + 1; i++) {
        assert(i - count - 1 == s_unsolResponses[i].requestNumber
                        - RIL_OEM_UNSOL_RESPONSE_BASE);
    }
#endif

    // New rild impl calls RIL_startEventLoop() first
    // old standalone impl wants it here.

    list_init(&sms_cmd_list);
    list_init(&local_cmd_list);
    list_init(&other_cmd_list);

    threadpool_d = thread_pool_init(THR_MAX, 10000);
    if(threadpool_d->thr_max==THR_MAX){
        RILLOGI("Ril.cpp: %d CommandThread create",threadpool_d->thr_max);
    }

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&s_tid_sms, &attr, smsDispatch, NULL);
    if (ret < 0) {
        RILLOGE("Failed to create sms dispatch thread errno:%d", errno);
        return;
    }

    ret = pthread_create(&s_tid_local, &attr, localDispatch, NULL);
    if (ret < 0) {
        RILLOGE("Failed to create local dispatch thread errno:%d", errno);
        return;
    }

    if (s_started == 0) {
        RIL_startEventLoop();
    }

    // start listen socket

#if 0
    ret = socket_local_server (SOCKET_NAME_RIL,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    if (ret < 0) {
        RILLOGE("Unable to bind socket errno:%d", errno);
        exit (-1);
    }
    s_fdListen = ret;

#else
    if(s_sim_num > 0) {
#if 0
        snprintf(s_name_ril, sizeof(s_name_ril), "%srild%d", s_modem, s_sim_num);
        snprintf(s_name_ril_debug, sizeof(s_name_ril_debug), "%srild-debug%d", s_modem, s_sim_num);
#else
        snprintf(s_name_ril, sizeof(s_name_ril), "rild%d", s_sim_num+1);
        snprintf(s_name_ril_debug, sizeof(s_name_ril_debug), "rild-debug%d", s_sim_num+1);
#endif
    } else {
#if 0
        snprintf(s_name_ril, sizeof(s_name_ril), "%srild", s_modem);
        snprintf(s_name_ril_debug, sizeof(s_name_ril_debug), "%srild-debug", s_modem);
#else
        snprintf(s_name_ril, sizeof(s_name_ril), "rild");
        snprintf(s_name_ril_debug, sizeof(s_name_ril_debug), "rild-debug");
#endif
    }

    s_fdListen = android_get_control_socket(s_name_ril);
    if (s_fdListen < 0) {
        RILLOGE("Failed to get socket ' %s '", s_name_ril);
        exit(-1);
    }

    ret = listen(s_fdListen, 4);

    if (ret < 0) {
        RILLOGE("Failed to listen on control socket '%d': %s",
             s_fdListen, strerror(errno));
        exit(-1);
    }
#endif


    /* note: non-persistent so we can accept only one connection at a time */
    ril_event_set (&s_listen_event, s_fdListen, false,
                listenCallback, NULL);

    rilEventAddWakeup (&s_listen_event);

#if 0
    // start debug interface socket

    s_fdDebug = socket_local_server (SOCKET_NAME_RIL_DEBUG,
                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (s_fdDebug < 0) {
        RILLOGE("Failed to get socket '" SOCKET_NAME_RIL_DEBUG "' errno:%d", errno);
        exit(-1);
    }

    ret = listen(s_fdDebug, 4);

    if (ret < 0) {
        RILLOGE("Failed to listen on ril debug socket '%d': %s",
             s_fdDebug, strerror(errno));
        exit(-1);
    }

    ril_event_set (&s_debug_event, s_fdDebug, true,
                debugCallback, NULL);

    rilEventAddWakeup (&s_debug_event);
#endif

#if 1
    /* start debug interface socket */

    ret = socket_local_server (s_name_ril_debug,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    if (ret < 0) {
        RILLOGE("Unable to bind socket errno:%d", errno);
        exit (-1);
    }

    s_fdDebug = ret;

    ril_event_set (&s_debug_event, s_fdDebug, true,
                debugCallback, NULL);

     rilEventAddWakeup(&s_debug_event);
#endif

}

static int
checkAndDequeueRequestInfo(struct RequestInfo *pRI) {
    int ret = 0;

    if (pRI == NULL) {
        return 0;
    }

    pthread_mutex_lock(&s_pendingRequestsMutex);

    for(RequestInfo **ppCur = &s_pendingRequests
        ; *ppCur != NULL
        ; ppCur = &((*ppCur)->p_next)
    ) {
        if (pRI == *ppCur) {
            ret = 1;

            *ppCur = (*ppCur)->p_next;
            break;
        }
    }

    pthread_mutex_unlock(&s_pendingRequestsMutex);

    return ret;
}

extern "C" void
RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
    RequestInfo *pRI;
    int ret;
    size_t errorOffset;

    pRI = (RequestInfo *)t;

    if (!checkAndDequeueRequestInfo(pRI)) {
        RILLOGE ("RIL_onRequestComplete: invalid RIL_Token");
        return;
    }

    if (pRI->local > 0) {
        // Locally issued command...void only!
        // response does not go back up the command socket
        RILLOGD("C[locl]< %s", requestToString(pRI->pCI->requestNumber));

        goto done;
    }

    appendPrintBuf("[%04d]< %s",
        pRI->token, requestToString(pRI->pCI->requestNumber));

    if (pRI->cancelled == 0) {
        Parcel p;

        p.writeInt32 (RESPONSE_SOLICITED);
        p.writeInt32 (pRI->token);
        errorOffset = p.dataPosition();

        p.writeInt32 (e);

        if (response != NULL) {
            // there is a response payload, no matter success or not.
            ret = pRI->pCI->responseFunction(p, response, responselen);

            /* if an error occurred, rewind and mark it */
            if (ret != 0) {
                p.setDataPosition(errorOffset);
                p.writeInt32 (ret);
            }
        }

        if (e != RIL_E_SUCCESS) {
            appendPrintBuf("%s fails by %s", printBuf, failCauseToString(e));
        }

        if (s_fdCommand < 0) {
            RILLOGD ("RIL onRequestComplete: Command channel closed");
        }
        sendResponse(p);
    }

done:
    free(pRI);
}

static void
grabPartialWakeLock() {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, ANDROID_WAKE_LOCK_NAME);
}

static void
releaseWakeLock() {
    release_wake_lock(ANDROID_WAKE_LOCK_NAME);
}

/**
 * Timer callback to put us back to sleep before the default timeout
 */
static void
wakeTimeoutCallback (void *param) {
    // We're using "param != NULL" as a cancellation mechanism
    if (param == NULL) {
        //RILLOGD("wakeTimeout: releasing wake lock");

        releaseWakeLock();
    } else {
        //RILLOGD("wakeTimeout: releasing wake lock CANCELLED");
    }
}

static int
decodeVoiceRadioTechnology (RIL_RadioState radioState) {
    switch (radioState) {
        case RADIO_STATE_SIM_NOT_READY:
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_SIM_READY:
            return RADIO_TECH_UMTS;

        case RADIO_STATE_RUIM_NOT_READY:
        case RADIO_STATE_RUIM_READY:
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_NV_NOT_READY:
        case RADIO_STATE_NV_READY:
            return RADIO_TECH_1xRTT;

        default:
            RILLOGD("decodeVoiceRadioTechnology: Invoked with incorrect RadioState");
            return -1;
    }
}

static int
decodeCdmaSubscriptionSource (RIL_RadioState radioState) {
    switch (radioState) {
        case RADIO_STATE_SIM_NOT_READY:
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_SIM_READY:
        case RADIO_STATE_RUIM_NOT_READY:
        case RADIO_STATE_RUIM_READY:
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
            return CDMA_SUBSCRIPTION_SOURCE_RUIM_SIM;

        case RADIO_STATE_NV_NOT_READY:
        case RADIO_STATE_NV_READY:
            return CDMA_SUBSCRIPTION_SOURCE_NV;

        default:
            RILLOGD("decodeCdmaSubscriptionSource: Invoked with incorrect RadioState");
            return -1;
    }
}

static int
decodeSimStatus (RIL_RadioState radioState) {
   switch (radioState) {
       case RADIO_STATE_SIM_NOT_READY:
       case RADIO_STATE_RUIM_NOT_READY:
       case RADIO_STATE_NV_NOT_READY:
       case RADIO_STATE_NV_READY:
           return -1;
       case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
       case RADIO_STATE_SIM_READY:
       case RADIO_STATE_RUIM_READY:
       case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
           return radioState;
       default:
           RILLOGD("decodeSimStatus: Invoked with incorrect RadioState");
           return -1;
   }
}

static bool is3gpp2(int radioTech) {
    switch (radioTech) {
        case RADIO_TECH_IS95A:
        case RADIO_TECH_IS95B:
        case RADIO_TECH_1xRTT:
        case RADIO_TECH_EVDO_0:
        case RADIO_TECH_EVDO_A:
        case RADIO_TECH_EVDO_B:
        case RADIO_TECH_EHRPD:
            return true;
        default:
            return false;
    }
}

/* If RIL sends SIM states or RUIM states, store the voice radio
 * technology and subscription source information so that they can be
 * returned when telephony framework requests them
 */
static RIL_RadioState
processRadioState(RIL_RadioState newRadioState) {

    if((newRadioState > RADIO_STATE_UNAVAILABLE) && (newRadioState < RADIO_STATE_ON)) {
        int newVoiceRadioTech;
        int newCdmaSubscriptionSource;
        int newSimStatus;

        /* This is old RIL. Decode Subscription source and Voice Radio Technology
           from Radio State and send change notifications if there has been a change */
        newVoiceRadioTech = decodeVoiceRadioTechnology(newRadioState);
        if(newVoiceRadioTech != voiceRadioTech) {
            voiceRadioTech = newVoiceRadioTech;
            RIL_onUnsolicitedResponse (RIL_UNSOL_VOICE_RADIO_TECH_CHANGED,
                        &voiceRadioTech, sizeof(voiceRadioTech));
        }
        if(is3gpp2(newVoiceRadioTech)) {
            newCdmaSubscriptionSource = decodeCdmaSubscriptionSource(newRadioState);
            if(newCdmaSubscriptionSource != cdmaSubscriptionSource) {
                cdmaSubscriptionSource = newCdmaSubscriptionSource;
                RIL_onUnsolicitedResponse (RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED,
                        &cdmaSubscriptionSource, sizeof(cdmaSubscriptionSource));
            }
        }
        newSimStatus = decodeSimStatus(newRadioState);
        if(newSimStatus != simRuimStatus) {
            simRuimStatus = newSimStatus;
            RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
        }

        /* Send RADIO_ON to telephony */
        newRadioState = RADIO_STATE_ON;
    }

    return newRadioState;
}

extern "C"
void RIL_onUnsolicitedResponse(int unsolResponse, void *data,
                                size_t datalen)
{
    int unsolResponseIndex = -1;
    int ret;
    int64_t timeReceived = 0;
    bool shouldScheduleTimeout = false;
    RIL_RadioState newState;

    if (s_registerCalled == 0) {
        // Ignore RIL_onUnsolicitedResponse before RIL_register
        RILLOGW("RIL_onUnsolicitedResponse called before RIL_register");
        return;
    }

    if (unsolResponse < RIL_UNSOL_RESPONSE_BASE
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#if defined (RIL_SPRD_EXTENSION)
        || (unsolResponse > RIL_UNSOL_LAST
                 && unsolResponse < RIL_SPRD_UNSOL_RESPONSE_BASE)
        || (unsolResponse > RIL_SPRD_UNSOL_RESPONSE_LAST
                 && unsolResponse < RIL_OEM_UNSOL_RESPONSE_BASE)
#else
        || (unsolResponse > RIL_UNSOL_LAST
                 && unsolResponse < RIL_OEM_UNSOL_RESPONSE_BASE)
#endif
        || (unsolResponse > RIL_OEM_UNSOL_LAST)
#else
#if defined (RIL_SPRD_EXTENSION)
        || (unsolResponse > RIL_UNSOL_LAST
                 && unsolResponse < RIL_SPRD_UNSOL_RESPONSE_BASE)
        || (unsolResponse > RIL_SPRD_UNSOL_RESPONSE_LAST)
#endif
#endif
    ) {
        RILLOGE("unsupported unsolicited response code %d", unsolResponse);
        return;
    }

    if(unsolResponse <= RIL_UNSOL_LAST)
        unsolResponseIndex = unsolResponse - RIL_UNSOL_RESPONSE_BASE;
#if defined (RIL_SPRD_EXTENSION)
    else if(unsolResponse >= RIL_SPRD_UNSOL_RESPONSE_BASE
                  && unsolResponse <= RIL_SPRD_UNSOL_RESPONSE_LAST)
        unsolResponseIndex = unsolResponse - RIL_SPRD_UNSOL_RESPONSE_BASE
                                                + RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE + 1;
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    else if(unsolResponse >= RIL_OEM_UNSOL_RESPONSE_BASE
                  && unsolResponse <= RIL_OEM_UNSOL_LAST)
#if defined (RIL_SPRD_EXTENSION)
    unsolResponseIndex = unsolResponse - RIL_OEM_UNSOL_RESPONSE_BASE
                                                + RIL_SPRD_UNSOL_RESPONSE_LAST - RIL_SPRD_UNSOL_RESPONSE_BASE
                                                + RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE + 2;
#else
    unsolResponseIndex = unsolResponse - RIL_OEM_UNSOL_RESPONSE_BASE
                                                + RIL_UNSOL_LAST - RIL_UNSOL_RESPONSE_BASE + 1;
#endif
#endif

    /* Grab a wake lock if needed for this reponse,
     * as we exit we'll either release it immediately
     * or set a timer to release it later.
     */
    switch (s_unsolResponses[unsolResponseIndex].wakeType) {
        case WAKE_PARTIAL:
            grabPartialWakeLock();
            shouldScheduleTimeout = true;
        break;

        case DONT_WAKE:
        default:
            // No wake lock is grabed so don't set timeout
            shouldScheduleTimeout = false;
            break;
    }

    // Mark the time this was received, doing this
    // after grabing the wakelock incase getting
    // the elapsedRealTime might cause us to goto
    // sleep.
    if (unsolResponse == RIL_UNSOL_NITZ_TIME_RECEIVED) {
        timeReceived = elapsedRealtime();
    }

    appendPrintBuf("[UNSL]< %s", requestToString(unsolResponse));

    Parcel p;

    p.writeInt32 (RESPONSE_UNSOLICITED);
    p.writeInt32 (unsolResponse);

    ret = s_unsolResponses[unsolResponseIndex]
                .responseFunction(p, data, datalen);
    if (ret != 0) {
        // Problem with the response. Don't continue;
        goto error_exit;
    }

    // some things get more payload
    switch(unsolResponse) {
        case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED:
            newState = processRadioState(s_callbacks.onStateRequest());
            p.writeInt32(newState);
            appendPrintBuf("%s {%s}", printBuf,
                radioStateToString(s_callbacks.onStateRequest()));
        break;


        case RIL_UNSOL_NITZ_TIME_RECEIVED:
            // Store the time that this was received so the
            // handler of this message can account for
            // the time it takes to arrive and process. In
            // particular the system has been known to sleep
            // before this message can be processed.
            p.writeInt64(timeReceived);
        break;
    }

    ret = sendResponse(p);
    if (ret != 0 && unsolResponse == RIL_UNSOL_NITZ_TIME_RECEIVED) {

        // Unfortunately, NITZ time is not poll/update like everything
        // else in the system. So, if the upstream client isn't connected,
        // keep a copy of the last NITZ response (with receive time noted
        // above) around so we can deliver it when it is connected

        if (s_lastNITZTimeData != NULL) {
            free (s_lastNITZTimeData);
            s_lastNITZTimeData = NULL;
        }

        s_lastNITZTimeData = malloc(p.dataSize());
        s_lastNITZTimeDataSize = p.dataSize();
        memcpy(s_lastNITZTimeData, p.data(), p.dataSize());
    }

    // For now, we automatically go back to sleep after TIMEVAL_WAKE_TIMEOUT
    // FIXME The java code should handshake here to release wake lock

    if (shouldScheduleTimeout) {
        // Cancel the previous request
        if (s_last_wake_timeout_info != NULL) {
            s_last_wake_timeout_info->userParam = (void *)1;
        }

        s_last_wake_timeout_info
            = internalRequestTimedCallback(wakeTimeoutCallback, NULL,
                                            &TIMEVAL_WAKE_TIMEOUT);
    }

    // Normal exit
    return;

error_exit:
    if (shouldScheduleTimeout) {
        releaseWakeLock();
    }
}

/** FIXME generalize this if you track UserCAllbackInfo, clear it
    when the callback occurs
*/
static UserCallbackInfo *
internalRequestTimedCallback (RIL_TimedCallback callback, void *param,
                                const struct timeval *relativeTime)
{
    struct timeval myRelativeTime;
    UserCallbackInfo *p_info;

    p_info = (UserCallbackInfo *) malloc (sizeof(UserCallbackInfo));

    p_info->p_callback = callback;
    p_info->userParam = param;

    if (relativeTime == NULL) {
        /* treat null parameter as a 0 relative time */
        memset (&myRelativeTime, 0, sizeof(myRelativeTime));
    } else {
        /* FIXME I think event_add's tv param is really const anyway */
        memcpy (&myRelativeTime, relativeTime, sizeof(myRelativeTime));
    }

    ril_event_set(&(p_info->event), -1, false, userTimerCallback, p_info);

    ril_timer_add(&(p_info->event), &myRelativeTime);

    triggerEvLoop();
    return p_info;
}

static void
internalRemoveTimedCallback(void *callbackInfo)
{
    UserCallbackInfo *p_info;
    p_info = (UserCallbackInfo *)callbackInfo;
    RILLOGI("remove timer callback event");
    if(p_info) {
        ril_timer_delete(&(p_info->event));
        free(p_info);
    }
}

extern "C" void
RIL_requestTimedCallback (RIL_TimedCallback callback, void *param,
                                const struct timeval *relativeTime) {
    internalRequestTimedCallback (callback, param, relativeTime);
}

extern "C" void
RIL_removeTimedCallback (void *callbackInfo) {
    internalRemoveTimedCallback(callbackInfo);
}

const char *
failCauseToString(RIL_Errno e) {
    switch(e) {
        case RIL_E_SUCCESS: return "E_SUCCESS";
        case RIL_E_RADIO_NOT_AVAILABLE: return "E_RADIO_NOT_AVAILABLE";
        case RIL_E_GENERIC_FAILURE: return "E_GENERIC_FAILURE";
        case RIL_E_PASSWORD_INCORRECT: return "E_PASSWORD_INCORRECT";
        case RIL_E_SIM_PIN2: return "E_SIM_PIN2";
        case RIL_E_SIM_PUK2: return "E_SIM_PUK2";
        case RIL_E_REQUEST_NOT_SUPPORTED: return "E_REQUEST_NOT_SUPPORTED";
        case RIL_E_CANCELLED: return "E_CANCELLED";
        case RIL_E_OP_NOT_ALLOWED_DURING_VOICE_CALL: return "E_OP_NOT_ALLOWED_DURING_VOICE_CALL";
        case RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW: return "E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW";
        case RIL_E_SMS_SEND_FAIL_RETRY: return "E_SMS_SEND_FAIL_RETRY";
        case RIL_E_SIM_ABSENT:return "E_SIM_ABSENT";
        case RIL_E_ILLEGAL_SIM_OR_ME:return "E_ILLEGAL_SIM_OR_ME";
        case RIL_E_SIM_BUSY: return "E_SIM_BUSY";
        case RIL_E_DIAL_MODIFIED_TO_USSD: return "E_DIAL_MODIFIED_TO_USSD";
        case RIL_E_DIAL_MODIFIED_TO_SS: return "E_DIAL_MODIFIED_TO_SS";
        case RIL_E_DIAL_MODIFIED_TO_DIAL: return "E_DIAL_MODIFIED_TO_DIAL";
        case RIL_E_USSD_MODIFIED_TO_DIAL: return "E_USSD_MODIFIED_TO_DIAL";
        case RIL_E_USSD_MODIFIED_TO_SS: return "E_USSD_MODIFIED_TO_SS";
        case RIL_E_USSD_MODIFIED_TO_USSD: return "E_USSD_MODIFIED_TO_USSD";
        case RIL_E_SS_MODIFIED_TO_DIAL: return "E_SS_MODIFIED_TO_DIAL";
        case RIL_E_SS_MODIFIED_TO_USSD: return "E_SS_MODIFIED_TO_USSD";
        case RIL_E_SS_MODIFIED_TO_SS: return "E_SS_MODIFIED_TO_SS";
        case RIL_E_SUBSCRIPTION_NOT_SUPPORTED: return "E_SUBSCRIPTION_NOT_SUPPORTED";
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_E_OPER_NOT_ALLOWED: return "E_OPER_NOT_ALLOWED";
        case RIL_E_MEMORY_ERROR: return "E_MEMORY_ERROR";
        case RIL_E_INVALID_INDEX: return "E_INVALID_INDEX";
        case RIL_E_TEXT_STR_TOO_LONG: return "E_TEXT_STR_TOO_LONG";
        case RIL_E_DIAL_STR_TOO_LONG: return "E_DIAL_STR_TOO_LONG";
        case RIL_E_INVALID_CHARACTERS_IN_TEXT_STR: return "E_INVALID_CHARACTERS_IN_TEXT_STR";
        case RIL_E_INVALID_CHARACTERS_IN_DIAL_STR: return "E_INVALID_CHARACTERS_IN_DIAL_STR";
#endif
#if defined (RIL_SPRD_EXTENSION)
        case RIL_E_SMS_SAVE_FAIL_FULL: return "E_SMS_SAVE_FAIL_FULL";
#endif
#ifdef FEATURE_MULTIMODE_ANDROID
        case RIL_E_SUBSCRIPTION_NOT_AVAILABLE:return "E_SUBSCRIPTION_NOT_AVAILABLE";
        case RIL_E_MODE_NOT_SUPPORTED:return "E_MODE_NOT_SUPPORTED";
#endif
        default: return "<unknown error>";
    }
}

const char *
radioStateToString(RIL_RadioState s) {
    switch(s) {
        case RADIO_STATE_OFF: return "RADIO_OFF";
        case RADIO_STATE_UNAVAILABLE: return "RADIO_UNAVAILABLE";
        case RADIO_STATE_SIM_NOT_READY: return "RADIO_SIM_NOT_READY";
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT: return "RADIO_SIM_LOCKED_OR_ABSENT";
        case RADIO_STATE_SIM_READY: return "RADIO_SIM_READY";
        case RADIO_STATE_RUIM_NOT_READY:return"RADIO_RUIM_NOT_READY";
        case RADIO_STATE_RUIM_READY:return"RADIO_RUIM_READY";
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:return"RADIO_RUIM_LOCKED_OR_ABSENT";
        case RADIO_STATE_NV_NOT_READY:return"RADIO_NV_NOT_READY";
        case RADIO_STATE_NV_READY:return"RADIO_NV_READY";
        case RADIO_STATE_ON:return"RADIO_ON";
        default: return "<unknown state>";
    }
}

const char *
callStateToString(RIL_CallState s) {
    switch(s) {
        case RIL_CALL_ACTIVE : return "ACTIVE";
        case RIL_CALL_HOLDING: return "HOLDING";
        case RIL_CALL_DIALING: return "DIALING";
        case RIL_CALL_ALERTING: return "ALERTING";
        case RIL_CALL_INCOMING: return "INCOMING";
        case RIL_CALL_WAITING: return "WAITING";
        default: return "<unknown state>";
    }
}

const char *
requestToString(int request) {
/*
 cat libs/telephony/ril_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'


 cat libs/telephony/ril_unsol_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{RIL_([^,]+),([^}]+).+/case RIL_\1: return "\1";/'

*/
    switch(request) {
        case RIL_REQUEST_GET_SIM_STATUS: return "GET_SIM_STATUS";
        case RIL_REQUEST_ENTER_SIM_PIN: return "ENTER_SIM_PIN";
        case RIL_REQUEST_ENTER_SIM_PUK: return "ENTER_SIM_PUK";
        case RIL_REQUEST_ENTER_SIM_PIN2: return "ENTER_SIM_PIN2";
        case RIL_REQUEST_ENTER_SIM_PUK2: return "ENTER_SIM_PUK2";
        case RIL_REQUEST_CHANGE_SIM_PIN: return "CHANGE_SIM_PIN";
        case RIL_REQUEST_CHANGE_SIM_PIN2: return "CHANGE_SIM_PIN2";
        case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION: return "ENTER_NETWORK_DEPERSONALIZATION";
        case RIL_REQUEST_GET_CURRENT_CALLS: return "GET_CURRENT_CALLS";
        case RIL_REQUEST_DIAL: return "DIAL";
        case RIL_REQUEST_GET_IMSI: return "GET_IMSI";
        case RIL_REQUEST_HANGUP: return "HANGUP";
        case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND: return "HANGUP_WAITING_OR_BACKGROUND";
        case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: return "HANGUP_FOREGROUND_RESUME_BACKGROUND";
        case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE: return "SWITCH_WAITING_OR_HOLDING_AND_ACTIVE";
        case RIL_REQUEST_CONFERENCE: return "CONFERENCE";
        case RIL_REQUEST_UDUB: return "UDUB";
        case RIL_REQUEST_LAST_CALL_FAIL_CAUSE: return "LAST_CALL_FAIL_CAUSE";
        case RIL_REQUEST_SIGNAL_STRENGTH: return "SIGNAL_STRENGTH";
        case RIL_REQUEST_VOICE_REGISTRATION_STATE: return "VOICE_REGISTRATION_STATE";
        case RIL_REQUEST_DATA_REGISTRATION_STATE: return "DATA_REGISTRATION_STATE";
        case RIL_REQUEST_OPERATOR: return "OPERATOR";
        case RIL_REQUEST_RADIO_POWER: return "RADIO_POWER";
        case RIL_REQUEST_DTMF: return "DTMF";
        case RIL_REQUEST_SEND_SMS: return "SEND_SMS";
        case RIL_REQUEST_SEND_SMS_EXPECT_MORE: return "SEND_SMS_EXPECT_MORE";
        case RIL_REQUEST_SETUP_DATA_CALL: return "SETUP_DATA_CALL";
        case RIL_REQUEST_SIM_IO: return "SIM_IO";
        case RIL_REQUEST_SEND_USSD: return "SEND_USSD";
        case RIL_REQUEST_CANCEL_USSD: return "CANCEL_USSD";
        case RIL_REQUEST_GET_CLIR: return "GET_CLIR";
        case RIL_REQUEST_SET_CLIR: return "SET_CLIR";
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: return "QUERY_CALL_FORWARD_STATUS";
        case RIL_REQUEST_SET_CALL_FORWARD: return "SET_CALL_FORWARD";
        case RIL_REQUEST_QUERY_CALL_WAITING: return "QUERY_CALL_WAITING";
        case RIL_REQUEST_SET_CALL_WAITING: return "SET_CALL_WAITING";
        case RIL_REQUEST_SMS_ACKNOWLEDGE: return "SMS_ACKNOWLEDGE";
        case RIL_REQUEST_GET_IMEI: return "GET_IMEI";
        case RIL_REQUEST_GET_IMEISV: return "GET_IMEISV";
        case RIL_REQUEST_ANSWER: return "ANSWER";
        case RIL_REQUEST_DEACTIVATE_DATA_CALL: return "DEACTIVATE_DATA_CALL";
        case RIL_REQUEST_QUERY_FACILITY_LOCK: return "QUERY_FACILITY_LOCK";
        case RIL_REQUEST_SET_FACILITY_LOCK: return "SET_FACILITY_LOCK";
        case RIL_REQUEST_CHANGE_BARRING_PASSWORD: return "CHANGE_BARRING_PASSWORD";
        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE: return "QUERY_NETWORK_SELECTION_MODE";
        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: return "SET_NETWORK_SELECTION_AUTOMATIC";
        case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL: return "SET_NETWORK_SELECTION_MANUAL";
        case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS : return "QUERY_AVAILABLE_NETWORKS ";
        case RIL_REQUEST_DTMF_START: return "DTMF_START";
        case RIL_REQUEST_DTMF_STOP: return "DTMF_STOP";
        case RIL_REQUEST_BASEBAND_VERSION: return "BASEBAND_VERSION";
        case RIL_REQUEST_SEPARATE_CONNECTION: return "SEPARATE_CONNECTION";
        case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE: return "SET_PREFERRED_NETWORK_TYPE";
        case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE: return "GET_PREFERRED_NETWORK_TYPE";
        case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS: return "GET_NEIGHBORING_CELL_IDS";
        case RIL_REQUEST_SET_MUTE: return "SET_MUTE";
        case RIL_REQUEST_GET_MUTE: return "GET_MUTE";
        case RIL_REQUEST_QUERY_CLIP: return "QUERY_CLIP";
        case RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE: return "LAST_DATA_CALL_FAIL_CAUSE";
        case RIL_REQUEST_DATA_CALL_LIST: return "DATA_CALL_LIST";
        case RIL_REQUEST_RESET_RADIO: return "RESET_RADIO";
        case RIL_REQUEST_OEM_HOOK_RAW: return "OEM_HOOK_RAW";
        case RIL_REQUEST_OEM_HOOK_STRINGS: return "OEM_HOOK_STRINGS";
        case RIL_REQUEST_WRITE_SMS_TO_SIM: return "WRITE_SMS_TO_SIM";
        case RIL_REQUEST_DELETE_SMS_ON_SIM: return "DELETE_SMS_ON_SIM";
        case RIL_REQUEST_SET_BAND_MODE: return "SET_BAND_MODE";
        case RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE: return "QUERY_AVAILABLE_BAND_MODE";
        case RIL_REQUEST_STK_GET_PROFILE: return "STK_GET_PROFILE";
        case RIL_REQUEST_STK_SET_PROFILE: return "STK_SET_PROFILE";
        case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND: return "STK_SEND_ENVELOPE_COMMAND";
        case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE: return "STK_SEND_TERMINAL_RESPONSE";
        case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM: return "STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM";
        case RIL_REQUEST_SCREEN_STATE: return "SCREEN_STATE";
        case RIL_REQUEST_EXPLICIT_CALL_TRANSFER: return "EXPLICIT_CALL_TRANSFER";
        case RIL_REQUEST_SET_LOCATION_UPDATES: return "SET_LOCATION_UPDATES";
        case RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE:return"CDMA_SET_SUBSCRIPTION_SOURCE";
        case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE:return"CDMA_SET_ROAMING_PREFERENCE";
        case RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE:return"CDMA_QUERY_ROAMING_PREFERENCE";
        case RIL_REQUEST_SET_TTY_MODE:return"SET_TTY_MODE";
        case RIL_REQUEST_QUERY_TTY_MODE:return"QUERY_TTY_MODE";
        case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE:return"CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE";
        case RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE:return"CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE";
        case RIL_REQUEST_CDMA_FLASH:return"CDMA_FLASH";
        case RIL_REQUEST_CDMA_BURST_DTMF:return"CDMA_BURST_DTMF";
        case RIL_REQUEST_CDMA_SEND_SMS:return"CDMA_SEND_SMS";
        case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE:return"CDMA_SMS_ACKNOWLEDGE";
        case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION:return"GSM_SMS_BROADCAST_ACTIVATION";
        case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG:return"GSM_GET_BROADCAST_SMS_CONFIG";
        case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG:return"GSM_SET_BROADCAST_SMS_CONFIG";
        case RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG:return "CDMA_GET_BROADCAST_SMS_CONFIG";
        case RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG:return "CDMA_SET_BROADCAST_SMS_CONFIG";
        case RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION:return "CDMA_SMS_BROADCAST_ACTIVATION";
        case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY: return"CDMA_VALIDATE_AND_WRITE_AKEY";
        case RIL_REQUEST_CDMA_SUBSCRIPTION: return"CDMA_SUBSCRIPTION";
        case RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM: return "CDMA_WRITE_SMS_TO_RUIM";
        case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM: return "CDMA_DELETE_SMS_ON_RUIM";
        case RIL_REQUEST_DEVICE_IDENTITY: return "DEVICE_IDENTITY";
        case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE: return "EXIT_EMERGENCY_CALLBACK_MODE";
        case RIL_REQUEST_GET_SMSC_ADDRESS: return "GET_SMSC_ADDRESS";
        case RIL_REQUEST_SET_SMSC_ADDRESS: return "SET_SMSC_ADDRESS";
        case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS: return "REPORT_SMS_MEMORY_STATUS";
        case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING: return "REPORT_STK_SERVICE_IS_RUNNING";
        case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE: return "CDMA_GET_SUBSCRIPTION_SOURCE";
        case RIL_REQUEST_ISIM_AUTHENTICATION: return "ISIM_AUTHENTICATION";
        case RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU: return "RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU";
        case RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS: return "RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS";
        case RIL_REQUEST_VOICE_RADIO_TECH: return "VOICE_RADIO_TECH";
        case RIL_REQUEST_GET_CELL_INFO_LIST: return"GET_CELL_INFO_LIST";
        case RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE: return"SET_UNSOL_CELL_INFO_LIST_RATE";
        case RIL_REQUEST_SET_INITIAL_ATTACH_APN: return "RIL_REQUEST_SET_INITIAL_ATTACH_APN";
        case RIL_REQUEST_IMS_REGISTRATION_STATE: return "IMS_REGISTRATION_STATE";
        case RIL_REQUEST_IMS_SEND_SMS: return "IMS_SEND_SMS";
        case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC: return "SIM_TRANSMIT_APDU_BASIC";
        case RIL_REQUEST_SIM_OPEN_CHANNEL: return "SIM_OPEN_CHANNEL";
        case RIL_REQUEST_SIM_CLOSE_CHANNEL: return "SIM_CLOSE_CHANNEL";
        case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL: return "SIM_TRANSMIT_APDU_CHANNEL";
        case RIL_REQUEST_GET_RADIO_CAPABILITY: return "RIL_REQUEST_GET_RADIO_CAPABILITY";
        case RIL_REQUEST_SET_RADIO_CAPABILITY: return "RIL_REQUEST_SET_RADIO_CAPABILITY";
        case RIL_REQUEST_SET_UICC_SUBSCRIPTION: return "SET_UICC_SUBSCRIPTION";
        case RIL_REQUEST_ALLOW_DATA: return "ALLOW_DATA";
        case RIL_REQUEST_GET_HARDWARE_CONFIG: return "GET_HARDWARE_CONFIG";
        case RIL_REQUEST_SIM_AUTHENTICATION: return "SIM_AUTHENTICATION";
        case RIL_REQUEST_GET_DC_RT_INFO: return "GET_DC_RT_INFO";
        case RIL_REQUEST_SET_DC_RT_INFO_RATE: return "SET_DC_RT_INFO_RATE";
        case RIL_REQUEST_SET_DATA_PROFILE: return "SET_DATA_PROFILE";
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_REQUEST_IMS_REGISTRATION_STATE: return "IMS_REGISTRATION_STATE";
        case RIL_REQUEST_IMS_SEND_SMS: return "IMS_SEND_SMS";
        case RIL_REQUEST_GET_DATA_CALL_PROFILE: return "GET_DATA_CALL_PROFILE";
        case RIL_REQUEST_SET_UICC_SUBSCRIPTION: return "SET_UICC_SUBSCRIPTION";
        case RIL_REQUEST_SET_DATA_SUBSCRIPTION: return "SET_DATA_SUBSCRIPTION";
        case RIL_REQUEST_GET_UICC_SUBSCRIPTION: return "GET_UICC_SUBSCRIPTION";								        
        case RIL_REQUEST_GET_DATA_SUBSCRIPTION: return "GET_DATA_SUBSCRIPTION";
        case RIL_REQUEST_SET_SUBSCRIPTION_MODE: return "SET_SUBSCRIPTION_MODE";
        case RIL_REQUEST_SET_TRANSMIT_POWER: return "SET_TRANSMIT_POWER";
        case RIL_REQUEST_SETUP_QOS: return "SETUP_QOS";
        case RIL_REQUEST_RELEASE_QOS: return "RELEASE_QOS";
        case RIL_REQUEST_GET_QOS_STATUS: return "GET_QOS_STATUS";
        case RIL_REQUEST_MODIFY_QOS: return "MODIFY_QOS";
        case RIL_REQUEST_SUSPEND_QOS: return "SUSPEND_QOS";
        case RIL_REQUEST_RESUME_QOS: return "RESUME_QOS";
#endif
#if defined (RIL_SPRD_EXTENSION)
        case RIL_REQUEST_QUERY_COLP: return "QUERY_COLP";
        case RIL_REQUEST_QUERY_COLR: return "QUERY_COLR";
        case RIL_REQUEST_VIDEOPHONE_DIAL: return "VIDEOPHONE_DIAL";
        case RIL_REQUEST_VIDEOPHONE_CODEC: return "VIDEOPHONE_CODEC";
        case RIL_REQUEST_VIDEOPHONE_HANGUP: return "VIDEOPHONE_HANGUP";
        case RIL_REQUEST_VIDEOPHONE_ANSWER: return "VIDEOPHONE_ANSWER";
        case RIL_REQUEST_VIDEOPHONE_FALLBACK: return "VIDEOPHONE_FALLBACK";
        case RIL_REQUEST_VIDEOPHONE_STRING: return "VIDEOPHONE_STR";
        case RIL_REQUEST_VIDEOPHONE_LOCAL_MEDIA: return "VIDEOPHONE_LOCAL_MEDIA";
        case RIL_REQUEST_VIDEOPHONE_RECORD_VIDEO: return "VIDEOPHONE_RECORD_VIDEO";
        case RIL_REQUEST_VIDEOPHONE_RECORD_AUDIO: return "VIDEOPHONE_RECORD_AUDIO";
        case RIL_REQUEST_VIDEOPHONE_TEST: return "VIDEOPHONE_TEST";
        case RIL_REQUEST_GET_CURRENT_VIDEOCALLS: return "GET_CURRENT_VIDEOCALLS";
        case RIL_REQUEST_VIDEOPHONE_CONTROL_IFRAME: return "VIDEOPHONE_CONTROL_IFRAME";
        case RIL_REQUEST_MBBMS_GSM_AUTHEN: return "MBBMS_GSM_AUTHEN";
        case RIL_REQUEST_MBBMS_USIM_AUTHEN: return "MBBMS_USIM_AUTHEN";
        case RIL_REQUEST_MBBMS_SIM_TYPE: return "MBBMS_SIM_TYPE";
        case RIL_REQUEST_GPRS_ATTACH: return "GPRS_ATTACH";
        case RIL_REQUEST_GPRS_DETACH: return "GPRS_DETACH";
        case RIL_REQUEST_GET_SIM_CAPACITY: return "GET_SIM_CAPACITY";
        case RIL_REQUEST_MMI_ENTER_SIM: return "MMI_ENTER_SIM";
        case RIL_REQUEST_END_ALL_CONNECTIONS: return "RIL_REQUEST_END_ALL_CONNECTIONS";
        case RIL_REQUEST_GET_REMAIN_TIMES: return "RIL_REQUEST_GET_REMAIN_TIMES";
        case RIL_REQUEST_SET_CMMS: return "SET_CMMS";
        case RIL_REQUEST_SIM_POWER: return "SIM_POWER";
        case RIL_REQUEST_DIAL_EMERGENCY_CALL: return "DIAL_EMERGENCY_CALL";
        case RIL_REQUEST_SEND_AT: return "SEND_AT";
        case RIL_REQUEST_SET_SPEED_MODE: return "SET_SPEED_MODE";
        case RIL_REQUEST_SET_SIM_SLOT_CFG: return "SET_SIM_SLOT_CFG"; //SPRD:added for choosing WCDMA SIM
        case RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES: return "SIMLOCK_REMAIN_TIMES";
        case RIL_REQUEST_CALL_CSFALLBACK_ACCEPT: return "RIL_REQUEST_CALL_CSFALLBACK_ACCEPT"; //SPRD:add for LTE-CSFB to handle CS fall back of MT call
        case RIL_REQUEST_CALL_CSFALLBACK_REJECT: return "RIL_REQUEST_CALL_CSFALLBACK_REJECT"; //SPRD:add for LTE-CSFB to handle CS fall back of MT call
        case RIL_REQUEST_SET_PRIORITY_NETWORK_MODE: return "RIL_REQUEST_SET_PRIORITY_NETWORK_MODE"; //SPRD:add for priority network mode
        case RIL_REQUEST_GET_PRIORITY_NETWORK_MODE: return "RIL_REQUEST_GET_PRIORITY_NETWORK_MODE"; //SPRD:add for priority network mode
        ////SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 START
        case RIL_REQUEST_GET_BAND_INFO: return "GET_BAND_INFO";
        case RIL_REQUEST_SWITCH_BAND_INFO_REPORT: return "SWITCH_BAND_INFO_REPORT";
        case RIL_REQUEST_SWITCH_3_WIRE: return "SWITCH_3_WIRE";
        case RIL_REQUEST_SWITCH_BT: return "SWITCH_BT";
        case RIL_REQUEST_SWITCH_WIFI: return "SWITCH_WIFI";
        ////SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 END
        /*SPRD: for stop query available networks@{*/
        case RIL_REQUEST_STOP_QUERY_AVAILABLE_NETWORKS: return "RIL_REQUEST_STOP_QUERY_AVAILABLE_NETWORKS";
        /*@}*/
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_REQUEST_SET_CELL_BROADCAST_CONFIG: return "SET_CELL_BROADCAST_CONFIG";
        case RIL_REQUEST_GET_CELL_BROADCAST_CONFIG: return "GET_CELL_BROADCAST_CONFIG";
        case RIL_REQUEST_CRFM_LINE_SMS_COUNT_MSG: return "CRFM_LINE_SMS_COUNT_MSG";
        case RIL_REQUEST_CRFM_LINE_SMS_READ_MSG: return "CRFM_LINE_SMS_READ_MSG";
        case RIL_REQUEST_SEND_ENCODED_USSD: return "SEND_ENCODED_USSD";
        case RIL_REQUEST_SET_PDA_MEMORY_STATUS: return "SET_PDA_MEMORY_STATUS";
        case RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO: return "GET_PHONEBOOK_STORAGE_INFO";
        case RIL_REQUEST_GET_PHONEBOOK_ENTRY: return "GET_PHONEBOOK_ENTRY";
        case RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY: return "ACCESS_PHONEBOOK_ENTRY";
        case RIL_REQUEST_DIAL_VIDEO_CALL: return "DIAL_VIDEO_CALL";
        case RIL_REQUEST_CALL_DEFLECTION: return "CALL_DEFLECTION";	
        case RIL_REQUEST_READ_SMS_FROM_SIM: return "READ_SMS_FROM_SIM";
        case RIL_REQUEST_USIM_PB_CAPA: return "USIM_PB_CAPA";
        case RIL_REQUEST_LOCK_INFO: return "LOCK_INFO";
        case RIL_REQUEST_SEND_MOBILE_TRACKER_SMS: return "SEND_MOBILE_TRACKER_SMS";
        case RIL_REQUEST_DIAL_EMERGENCY_CALL: return "DIAL_EMERGENCY_CALL";
        case RIL_REQUEST_GET_STOREAD_MSG_COUNT: return "GET_STOREAD_MSG_COUNT";	
        case RIL_REQUEST_STK_SIM_INIT_EVENT: return "STK_SIM_INIT_EVENT";
        case RIL_REQUEST_GET_LINE_ID: return "GET_LINE_ID";	
        case RIL_REQUEST_SET_LINE_ID: return "SET_LINE_ID";
        case RIL_REQUEST_GET_SERIAL_NUMBER: return "GET_SERIAL_NUMBER";
        case RIL_REQUEST_GET_MANUFACTURE_DATE_NUMBER: return "GET_MANUFACTURE_DATE_NUMBER";
        case RIL_REQUEST_GET_BARCODE_NUMBER: return "GET_BARCODE_NUMBER";
        case RIL_REQUEST_UICC_GBA_AUTHENTICATE_BOOTSTRAP: return "UICC_GBA_AUTHENTICATE_BOOTSTRAP";
        case RIL_REQUEST_UICC_GBA_AUTHENTICATE_NAF: return "UICC_GBA_AUTHENTICATE_NAF";			
        case RIL_REQUEST_SIM_APDU: return "SIM_APDU";
        case RIL_REQUEST_SIM_OPEN_CHANNEL: return "SIM_OPEN_CHANNEL";
        case RIL_REQUEST_SIM_CLOSE_CHANNEL: return "SIM_CLOSE_CHANNEL";
        case RIL_REQUEST_SIM_TRANSMIT_CHANNEL: return "SIM_TRANSMIT_CHANNEL";
        case RIL_REQUEST_SIM_AUTH: return "SIM_AUTH";
#endif
        case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED: return "UNSOL_RESPONSE_RADIO_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED: return "UNSOL_RESPONSE_CALL_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_NEW_SMS: return "UNSOL_RESPONSE_NEW_SMS";
        case RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT: return "UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT";
        case RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM: return "UNSOL_RESPONSE_NEW_SMS_ON_SIM";
        case RIL_UNSOL_ON_USSD: return "UNSOL_ON_USSD";
        case RIL_UNSOL_ON_USSD_REQUEST: return "UNSOL_ON_USSD_REQUEST(obsolete)";
        case RIL_UNSOL_NITZ_TIME_RECEIVED: return "UNSOL_NITZ_TIME_RECEIVED";
        case RIL_UNSOL_SIGNAL_STRENGTH: return "UNSOL_SIGNAL_STRENGTH";
        case RIL_UNSOL_STK_SESSION_END: return "UNSOL_STK_SESSION_END";
        case RIL_UNSOL_STK_PROACTIVE_COMMAND: return "UNSOL_STK_PROACTIVE_COMMAND";
        case RIL_UNSOL_STK_EVENT_NOTIFY: return "UNSOL_STK_EVENT_NOTIFY";
        case RIL_UNSOL_STK_CALL_SETUP: return "UNSOL_STK_CALL_SETUP";
        case RIL_UNSOL_SIM_SMS_STORAGE_FULL: return "UNSOL_SIM_SMS_STORAGE_FULL";
        case RIL_UNSOL_SIM_REFRESH: return "UNSOL_SIM_REFRESH";
        case RIL_UNSOL_DATA_CALL_LIST_CHANGED: return "UNSOL_DATA_CALL_LIST_CHANGED";
        case RIL_UNSOL_CALL_RING: return "UNSOL_CALL_RING";
        case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED: return "UNSOL_RESPONSE_SIM_STATUS_CHANGED";
        case RIL_UNSOL_RESPONSE_CDMA_NEW_SMS: return "UNSOL_NEW_CDMA_SMS";
        case RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS: return "UNSOL_NEW_BROADCAST_SMS";
        case RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL: return "UNSOL_CDMA_RUIM_SMS_STORAGE_FULL";
        case RIL_UNSOL_RESTRICTED_STATE_CHANGED: return "UNSOL_RESTRICTED_STATE_CHANGED";
        case RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE: return "UNSOL_ENTER_EMERGENCY_CALLBACK_MODE";
        case RIL_UNSOL_CDMA_CALL_WAITING: return "UNSOL_CDMA_CALL_WAITING";
        case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS: return "UNSOL_CDMA_OTA_PROVISION_STATUS";
        case RIL_UNSOL_CDMA_INFO_REC: return "UNSOL_CDMA_INFO_REC";
        case RIL_UNSOL_OEM_HOOK_RAW: return "UNSOL_OEM_HOOK_RAW";
        case RIL_UNSOL_RINGBACK_TONE: return "UNSOL_RINGBACK_TONE";
        case RIL_UNSOL_RESEND_INCALL_MUTE: return "UNSOL_RESEND_INCALL_MUTE";
        case RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED: return "UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED";
        case RIL_UNSOL_CDMA_PRL_CHANGED: return "UNSOL_CDMA_PRL_CHANGED";
        case RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE: return "UNSOL_EXIT_EMERGENCY_CALLBACK_MODE";
        case RIL_UNSOL_RIL_CONNECTED: return "UNSOL_RIL_CONNECTED";
        case RIL_UNSOL_VOICE_RADIO_TECH_CHANGED: return "UNSOL_VOICE_RADIO_TECH_CHANGED";
        case RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED: return "UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED";
        case RIL_UNSOL_SRVCC_STATE_NOTIFY: return "UNSOL_SRVCC_STATE_NOTIFY";
        case RIL_UNSOL_HARDWARE_CONFIG_CHANGED: return "HARDWARE_CONFIG_CHANGED";
        case RIL_UNSOL_DC_RT_INFO_CHANGED: return "UNSOL_DC_RT_INFO_CHANGED";
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED: return "UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_DATA_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_DATA_NETWORK_STATE_CHANGED";
        case RIL_UNSOL_ON_SS: return "UNSOL_ON_SS";
        case RIL_UNSOL_STK_CC_ALPHA_NOTIFY: return "UNSOL_STK_CC_ALPHA_NOTIFY";
        case RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED: return "UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED";
        case RIL_UNSOL_QOS_STATE_CHANGED_IND: return "UNSOL_QOS_STATE_CHANGED_IND";
#endif
#if defined (RIL_SPRD_EXTENSION)
        case RIL_UNSOL_VIDEOPHONE_DATA: return "UNSOL_VIDEOPHONE_DATA";
        case RIL_UNSOL_VIDEOPHONE_CODEC: return "UNSOL_VIDEOPHONE_CODEC";
        case RIL_UNSOL_VIDEOPHONE_DCPI: return "UNSOL_VIDEOPHONE_DCPI";
        case RIL_UNSOL_VIDEOPHONE_DSCI: return "UNSOL_VIDEOPHONE_DSCI";
        case RIL_UNSOL_VIDEOPHONE_STRING: return "UNSOL_VIDEOPHONE_STR";
        case RIL_UNSOL_VIDEOPHONE_REMOTE_MEDIA: return "UNSOL_VIDEOPHONE_REMOTE_MEDIA";
        case RIL_UNSOL_VIDEOPHONE_MM_RING: return "UNSOL_VIDEOPHONE_MM_RING";
        case RIL_UNSOL_VIDEOPHONE_RELEASING: return "UNSOL_VIDEOPHONE_RELEASING";
        case RIL_UNSOL_VIDEOPHONE_RECORD_VIDEO: return "UNSOL_VIDEOPHONE_RECORD_VIDEO";
        case RIL_UNSOL_VIDEOPHONE_MEDIA_START: return "UNSOL_VIDEOPHONE_MEDIA_START";
        case RIL_UNSOL_RESPONSE_VIDEOCALL_STATE_CHANGED: return "UNSOL_RESPONSE_VIDEOCALL_STATE_CHANGED";
        case RIL_UNSOL_ON_STIN: return "UNSOL_ON_STIN";
        case RIL_UNSOL_SIM_SMS_READY: return "UNSOL_SIM_SMS_READY";
        case RIL_UNSOL_SIM_DROP: return "UNSOL_SIM_DROP";
        case RIL_UNSOL_SIM_PS_REJECT: return "UNSOL_SIM_PS_REJECT";
        case RIL_UNSOL_LTE_READY: return "UNSOL_LTE_READY";
        case RIL_UNSOL_SVLTE_USIM_READY: return "UNSOL_SVLTE_USIM_READY";
        case RIL_UNSOL_FDN_ENABLE: return "UNSOL_FDN_ENABLE";
        case RIL_UNSOL_CALL_CSFALLBACK: return "UNSOL_RIL_CALL_CSFALLBACK";//SPRD:add for LTE-CSFB to handle CS fall back of MT call
        //SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 START
        case RIL_UNSOL_BAND_INFO: return "UNSOL_BAND_INFO";
        //SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 END
        case RIL_UNSOL_CALL_CSFALLBACK_FINISH: return "UNSOL_RIL_CALL_CSFALLBACK";//SPRD:add for LTE-CSFB to handle CS fall back of MT call
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_UNSOL_RESPONSE_NEW_CB_MSG: return "UNSOL_RESPONSE_NEW_CB_MSG";
        case RIL_UNSOL_RELEASE_COMPLETE_MESSAGE: return "UNSOL_RELEASE_COMPLETE_MESSAGE";
        case RIL_UNSOL_STK_SEND_SMS_RESULT: return "UNSOL_STK_SEND_SMS_RESULT";
        case RIL_UNSOL_STK_CALL_CONTROL_RESULT: return "UNSOL_STK_CALL_CONTROL_RESULT";
        case RIL_UNSOL_DUN_CALL_STATUS: return "UNSOL_DUN_CALL_STATUS";
        case RIL_UNSOL_RESPONSE_LINE_SMS_COUNT: return "UNSOL_RESPONSE_LINE_SMS_COUNT";
        case RIL_UNSOL_RESPONSE_LINE_SMS_READ: return "UNSOL_RESPONSE_LINE_SMS_READ";
        case RIL_UNSOL_O2_HOME_ZONE_INFO: return "UNSOL_O2_HOME_ZONE_INFO";
        case RIL_UNSOL_DEVICE_READY_NOTI: return "UNSOL_DEVICE_READY_NOTI";
        case RIL_UNSOL_GPS_NOTI: return "UNSOL_GPS_NOTI";
        case RIL_UNSOL_AM: return "UNSOL_AM";
        case RIL_UNSOL_DUN_PIN_CONTROL_SIGNAL: return "UNSOL_DUN_PIN_CONTROL_SIGNAL";
        case RIL_UNSOL_DATA_SUSPEND_RESUME: return "UNSOL_DATA_SUSPEND_RESUME";
        case RIL_UNSOL_SAP: return "UNSOL_SAP";
        case RIL_UNSOL_RESPONSE_NO_NETWORK_RESPONSE: return "UNSOL_RESPONSE_NO_NETWORK_RESPONSE";
        case RIL_UNSOL_SIM_SMS_STORAGE_AVAILALE: return "UNSOL_SIM_SMS_STORAGE_AVAILALE";
        case RIL_UNSOL_HSDPA_STATE_CHANGED: return "UNSOL_HSDPA_STATE_CHANGED";
        case RIL_UNSOL_WB_AMR_STATE: return "UNSOL_WB_AMR_STATE";
        case RIL_UNSOL_TWO_MIC_STATE: return "UNSOL_TWO_MIC_STATE";
        case RIL_UNSOL_DHA_STATE: return "UNSOL_DHA_STATE";
        case RIL_UNSOL_UART: return "UNSOL_UART";
        case RIL_UNSOL_SIM_PB_READY: return "UNSOL_SIM_PB_READY";	
#endif

        case RIL_REQUEST_SHUTDOWN: return "SHUTDOWN";
        case RIL_UNSOL_RADIO_CAPABILITY: return "RIL_UNSOL_RADIO_CAPABILITY";
        default: return "<unknown request>";
    }
}

} /* namespace android */
