/* //vendor/sprd/proprietories-source/ril/sprd_ril/sprd_ril.c
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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <alloca.h>
#include "sprd_atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <getopt.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <telephony/sprd_ril.h>
#include <semaphore.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define NEW_AT
#ifdef NEW_AT
#define AT_PREFIX "+SP"
#else
#define AT_PREFIX "^"
#endif

#define VT_DCI "\"000001B000000001B5090000010000000120008440FA282C2090A21F\""

#define MO_CALL 0
#define MT_CALL 1

#define TD_SIM_NUM  "ro.modem.t.count"
#define W_SIM_NUM  "ro.modem.w.count"

#define ETH_TD  "ro.modem.t.eth"
#define ETH_W  "ro.modem.w.eth"

#define RIL_MAIN_SIM_PROPERTY  "persist.msms.phone_default"
#define RIL_SIM_POWER_OFF_PROPERTY  "sys.power.off"
#define RIL_TD_SIM_POWER_PROPERTY  "ril.t.sim.power"
#define RIL_TD_SIM_POWER_PROPERTY1  "ril.t.sim.power1"
#define RIL_TD_SIM_POWER_PROPERTY2  "ril.t.sim.power2"
#define RIL_W_SIM_POWER_PROPERTY  "ril.w.sim.power"
#define RIL_W_SIM_POWER_PROPERTY1  "ril.w.sim.power1"
#define RIL_W_SIM_POWER_PROPERTY2  "ril.w.sim.power2"
#define RIL_TD_ASSERT  "ril.t.assert"
#define RIL_W_ASSERT  "ril.w.assert"
#define RIL_TD_SIM_PIN_PROPERTY  "ril.t.sim.pin"
#define RIL_TD_SIM_PIN_PROPERTY1  "ril.t.sim.pin1"
#define RIL_TD_SIM_PIN_PROPERTY2  "ril.t.sim.pin2"
#define RIL_W_SIM_PIN_PROPERTY  "ril.w.sim.pin"
#define RIL_W_SIM_PIN_PROPERTY1  "ril.w.sim.pin1"
#define RIL_W_SIM_PIN_PROPERTY2  "ril.w.sim.pin2"
#define RIL_MODEM_RESET_PROPERTY "persist.sys.sprd.modemreset"
#define RIL_STK_PROFILE_PREFIX  "ril.stk.proflie_"
#define RIL_SIM0_STATE  "gsm.sim.state0"
#define RIL_SIM_ABSENT_PROPERTY "persist.sys.sim.absence"
#define RIL_SIM1_ABSENT_PROPERTY "ril.sim1.absent"  /* this property will be
						       set by phoneserver */
#define RIL_SIM_TYPE  "ril.ICC_TYPE"
#define RIL_SIM_TYPE1  "ril.ICC_TYPE_1"
#define RIL_SET_SPEED_MODE_COUNT  "ril.sim.speed_count"

int modem;
int s_multiSimMode = 0;
static const char * s_modem = NULL;

struct ATChannels *ATch_type[MAX_CHANNELS];
static int s_channel_open = 0;
static int s_sms_ready = 0;
static int simNum;
static int s_socket_connected = 0;

#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

struct listnode
{
    char data;
    struct listnode *next;
    struct listnode *prev;
};

struct listnode dtmf_char_list;

void list_init(struct listnode *list);
void list_add_tail(struct listnode *list, struct listnode *item);
void list_remove(struct listnode *item);

/* #define PPP_TTY_PATH "/dev/ppp0" */

enum channel_state {
    CHANNEL_IDLE,
    CHANNEL_BUSY,
};

struct channel_description
{
    int channelID;
    int fd;
    char name[128];
    char ttyName[128];
    enum channel_state state;
    pthread_mutex_t mutex;
};

struct channel_description single_descriptions[MAX_CHANNELS] = {
    { 0, -1, "Unsol", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 1, -1, "Channel1", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 2, -1, "Channel2", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 3, -1, "Channel3", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
};

struct channel_description multi_descriptions[MULTI_MAX_CHANNELS] = {
    { 0, -1, "", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 0, -1, "", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 0, -1, "", "", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
};

#define MAX_PDP 3

enum pdp_state {
    PDP_IDLE,
    PDP_BUSY,
};

struct pdp_info
{
    int cid;
    enum pdp_state state;
    pthread_mutex_t mutex;
};

struct pdp_info pdp[MAX_PDP] = {
    { -1, PDP_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { -1, PDP_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { -1, PDP_IDLE, PTHREAD_MUTEX_INITIALIZER},
};

#define FACILITY_LOCK_REQUEST "2"

enum sms_init_status {
    CSTAT_SMS = 1,
    CSTAT_PHB = 2
};

typedef enum {
    SIM_ABSENT = 0,
    SIM_NOT_READY = 1,
    SIM_READY = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
    SIM_PIN = 3,
    SIM_PUK = 4,
    SIM_NETWORK_PERSONALIZATION = 5,
    SIM_BLOCK = 6,
    SIM_PIN2 = 7,
    SIM_PUK2 = 8
} SIM_Status;

static void onRequest (int request, void *data, size_t datalen, RIL_Token t);
static RIL_RadioState currentState();
static int onSupports (int requestCode);
static void onCancel (RIL_Token t);
static const char *getVersion();
static int isRadioOn(int channelID);
static SIM_Status getSIMStatus(int channelID);
static int getCardStatus(int channelID, RIL_CardStatus_v6 **pp_card_status);
static void freeCardStatus(RIL_CardStatus_v6 *p_card_status);
static void onDataCallListChanged(void *param);
void putChannel(int channel);
int getChannel();
static int getSmsChannel();
extern const char * requestToString(int request);
static void requestSetupDataCall(int channelID, void *data, size_t datalen, RIL_Token t);
static void getSmsState(int channelID) ;
static void convertBinToHex(char *bin_ptr, int length, char *hex_ptr);
static int convertHexToBin(const char *hex_ptr, int length, char *bin_ptr);
int parsePdu(char *pdu);

/*** Static Variables ***/
static const RIL_RadioFunctions s_callbacks = {
    RIL_VERSION,
    onRequest,
    currentState,
    onSupports,
    onCancel,
    getVersion
};

#ifdef RIL_SHLIB
static const struct RIL_Env *s_rilenv;

#define RIL_onRequestComplete(t, e, response, responselen) s_rilenv->OnRequestComplete(t,e, response, responselen)
#define RIL_onUnsolicitedResponse(a,b,c) s_rilenv->OnUnsolicitedResponse(a,b,c)
#define RIL_requestTimedCallback(a,b,c) s_rilenv->RequestTimedCallback(a,b,c)
#endif

/* Last pdp fail cause, obtained by *ECAV. */
static int s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
static int call_fail_cause = CALL_FAIL_ERROR_UNSPECIFIED;

static RIL_RadioState sState = RADIO_STATE_UNAVAILABLE;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_channel_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_call_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_screen_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_sms_ready_mutex = PTHREAD_MUTEX_INITIALIZER;

static int s_port = -1;
static const char * s_device_path = NULL;
static int          s_device_socket = 0;
static sem_t w_sem;

/* trigger change to this with s_state_cond */
static int s_closed = 0;

static int sFD;     /* file desc of AT channel */

#if 0
static char sATBuffer[MAX_AT_RESPONSE+1];
static char *sATBufferCur = NULL;
#endif

/* add AT SPERROR for ussd,SPERROR is used for HTC project */
static int ussdError = 0;/* 0: no unsolicited SPERROR. 1: unsolicited SPERROR. */
static int ussdRun = 0;/* 0: ussd to end. 1: ussd to start. */

static int s_isstkcall = 0;

static const struct timeval TIMEVAL_SIMPOLL = {1,0};
static const struct timeval TIMEVAL_CALLSTATEPOLL = {0,500000};
static const struct timeval TIMEVAL_0 = {0,0};

static void pollSIMState (void *param);
static void attachGPRS(int channelID, void *data, size_t datalen, RIL_Token t);
static void detachGPRS(int channelID, void *data, size_t datalen, RIL_Token t);
static void setRadioState(int channelID, RIL_RadioState newState);

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
#define RIL_DATA_PREFER_PROPERTY  "persist.sys.dataprefer.simid"
typedef struct {
    int mcc;
    char *long_name;
    char *short_name;
} rilnet_tz_entry_t;

static rilnet_tz_entry_t rilnet_tz_entry[] = {
#include "rilnet_tz_entry.h"
};
#endif

void list_init(struct listnode *node)
{
    node->next = node;
    node->prev = node;
}

void list_add_tail(struct listnode *head, struct listnode *item)
{
    pthread_mutex_lock(&s_list_mutex);
    item->next = head;
    item->prev = head->prev;
    head->prev->next = item;
    head->prev = item;
    pthread_mutex_unlock(&s_list_mutex);
}

void list_remove(struct listnode *item)
{
    pthread_mutex_lock(&s_list_mutex);
    item->next->prev = item->prev;
    item->prev->next = item->next;
    pthread_mutex_unlock(&s_list_mutex);
}

/* Release pdp which the state is busy
 ** cid=0: pdp[0]
 ** cid=1: pdp[1]
 ** cid=2: pdp[2]
*/
static void putPDP(int cid)
{
    pthread_mutex_lock(&pdp[cid].mutex);
    if(pdp[cid].state != PDP_BUSY)
    {
        goto done1;
    }
    pdp[cid].state = PDP_IDLE;
done1:
    pdp[cid].cid = -1;
    RILLOGD("put pdp[%d]", cid);
    RILLOGD("pdp[0].state = %d, pdp[1].state = %d,pdp[2].state = %d", pdp[0].state, pdp[1].state, pdp[2].state);
    pthread_mutex_unlock(&pdp[cid].mutex);
}

/* Return pdp which the state is idle
 ** 0: pdp[0]
 ** 1: pdp[1]
 ** 2: pdp[2]
*/
static int getPDP(void)
{
    int ret = -1;
    int i;

    for (i=0; i < MAX_PDP; i++) {
        pthread_mutex_lock(&pdp[i].mutex);
        if(pdp[i].state == PDP_IDLE) {
            pdp[i].state = PDP_BUSY;
            ret = i;
            pthread_mutex_unlock(&pdp[i].mutex);
            RILLOGD("get pdp[%d]", ret);
            RILLOGD("pdp[0].state = %d, pdp[1].state = %d,pdp[2].state = %d", pdp[0].state, pdp[1].state, pdp[2].state);
            return ret;
        }
        pthread_mutex_unlock(&pdp[i].mutex);
    }
    return ret;
}

#if 0
#define WRITE_PPP_OPTION(option) write(fd, option, strlen(option))
#endif  

static void process_calls(int _calls)
{
    static int calls = 0;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    char buf[3];
    int incall_recording_status;

    if (calls && _calls == 0) {
        pthread_mutex_lock(&lock);
        RILLOGW("################## < vaudio > This is the Last PhoneCall ##################");
        /*
         * The Last PhoneCall is really terminated,
         * audio codec is freed by Modem side completely [luther.ge]
         *
         */
        incall_recording_status = open("/proc/vaudio/close", O_RDWR);
        if (incall_recording_status >= 0) {
            memset(buf, 0, sizeof buf);
            read(incall_recording_status, buf, 3);
            RILLOGW("################## < vaudio > %sincall recording ##################[%s]",
                    buf[0] == '1' ? "": "no ", buf);
            if (buf[0] == '1') {
                /* incall recording */
                write(incall_recording_status, buf, 1);
            }
            close(incall_recording_status);
        }
        pthread_mutex_unlock(&lock);
    }

    calls = _calls;
}

static inline void speaker_mute(void)
{
    RILLOGW("\n\nThere will be no call, so mute speaker now to avoid noise pop sound\n\n");
    /* Remove handsfree pop noise sound [luther.ge] */
    system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
}

static inline int all_calls(int channelID, int do_mute)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int countCalls;

    if (at_send_command_multiline(ATch_type[channelID], "AT+CLCC", "+CLCC:", &p_response) != 0 ||
            p_response->success == 0) {
        at_response_free(p_response);
        return -1;
    }

    /* total the calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next)
        countCalls++;
    at_response_free(p_response);

    if (do_mute && countCalls == 1)
        speaker_mute();

    return countCalls;
}

static void skipNextComma(char **p_cur)
{
    if (*p_cur == NULL) return;

    while (**p_cur != '\0' && **p_cur != ',') {
        (*p_cur)++;
    }

    if (**p_cur == ',') {
        (*p_cur)++;
    }
}


static void skipWhiteSpace(char **p_cur)
{
    if (*p_cur == NULL) return;

    while (**p_cur != '\0' && isspace(**p_cur)) {
        (*p_cur)++;
    }
}

static void deactivateDataConnection(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;
    const char *cid_ptr = NULL;
    int cid;
    char cmd[30];

    cid_ptr = ((const char **)data)[0];
    cid = atoi(cid_ptr);
    if(cid < 1)
        goto error1;

    RILLOGD("Try to deactivated modem ..., cid=%d", cid);
    if (pdp[cid - 1].cid == cid) {
        snprintf(cmd, sizeof(cmd), "AT+CGACT=0,%d", cid);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;

        putPDP(cid - 1);
    }

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    if (pdp[cid - 1].cid == cid)
        putPDP(cid - 1);
error1:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static int clccStateToRILState(int state, RIL_CallState *p_state)

{
    switch(state) {
        case 0: *p_state = RIL_CALL_ACTIVE;   return 0;
        case 1: *p_state = RIL_CALL_HOLDING;  return 0;
        case 2: *p_state = RIL_CALL_DIALING;  return 0;
        case 3: *p_state = RIL_CALL_ALERTING; return 0;
        case 4: *p_state = RIL_CALL_INCOMING; return 0;
        case 5: *p_state = RIL_CALL_WAITING;  return 0;
        default: return -1;
    }
}


/**
 * Note: directly modified line and has *p_call point directly into
 * modified line
 */
static int callFromCLCCLine(char *line, RIL_Call *p_call)
{
    /*+CLCC: 1,0,2,0,0,\"+18005551212\",145
     *     index,isMT,state,mode,isMpty(,number,TOA)?
     */

    int err;
    int state;
    int mode;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_call->index));
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &(p_call->isMT));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &state);
    if (err < 0) goto error;

    err = clccStateToRILState(state, &(p_call->state));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &mode);
    if (err < 0) goto error;

    p_call->isVoice = (mode==0);

    err = at_tok_nextbool(&line, &(p_call->isMpty));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(p_call->number));

        /* tolerate null here */
        if (err < 0) return 0;

        /* Some lame implementations return strings
         * like "NOT AVAILABLE" in the CLCC line
	 */
        if (p_call->number != NULL
                && 0 == strspn(p_call->number, "+0123456789*#abc")
           ) {
            p_call->number = NULL;
        }

        err = at_tok_nextint(&line, &p_call->toa);
        if (err < 0) goto error;
    }

    p_call->uusInfo = NULL;
    return 0;

error:
    RILLOGE("invalid CLCC line\n");
    return -1;
}

static int forwardFromCCFCLine(char *line, RIL_CallForwardInfo *p_forward)
{
    int err;
    int state;
    int mode;
    int i;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_forward->status));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_forward->serviceClass));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(p_forward->number));

        /* tolerate null here */
        if (err < 0) return 0;

        if (p_forward->number != NULL
                && 0 == strspn(p_forward->number, "+0123456789")
           ) {
            p_forward->number = NULL;
        }

        err = at_tok_nextint(&line, &p_forward->toa);
        if (err < 0) goto error;

        if (at_tok_hasmore(&line)) {
            for(i=0; i<2; i++ ) {
                skipNextComma(&line);
            }

            if (at_tok_hasmore(&line)) {
                err = at_tok_nextint(&line, &p_forward->timeSeconds);
                if (err < 0) goto error;
            }
        }
    }

    return 0;

error:
    RILLOGE("invalid CCFC line\n");
    return -1;
}

static void requestCallForward(int channelID, RIL_CallForwardInfo *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    int err;
    int errNum = 0xff;
    char *cmd, *line;
    int ret = -1;

    if (datalen != sizeof(*data))
        goto error1;
    if (data->serviceClass == 0) {
        if (data->status == 2) {
            ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",129",
                data->reason,
                data->status,
                data->number ? data->number : "");
        } else {
            if (data->timeSeconds != 0 && data->status == 3) {
                ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,,\"\",,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->timeSeconds);

            } else {
                ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa);
            }
        }
    } else {
        if (data->status == 2) {
            ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",129,%d",
                data->reason,
                data->status,
                data->number ? data->number : "",
                data->serviceClass);
        } else {
            if (data->timeSeconds != 0 && data->status == 3) {
                ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d,\"\",,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->serviceClass,
                        data->timeSeconds);
            } else {
                ret = asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->serviceClass);
            }
        }
    }
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error1;
    }
    err = at_send_command_multiline (ATch_type[channelID],  cmd, "+CCFC:", &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0)
        goto error;

    if (data->status == 2 ) {
        RIL_CallForwardInfo **forwardList, *forwardPool;
        int forwardCount = 0;
        int validCount = 0;
        int i;

        for (p_cur = p_response->p_intermediates
                ; p_cur != NULL
                ; p_cur = p_cur->p_next, forwardCount++
            );

        forwardList = (RIL_CallForwardInfo **)
            alloca(forwardCount * sizeof(RIL_CallForwardInfo *));

        forwardPool = (RIL_CallForwardInfo *)
            alloca(forwardCount * sizeof(RIL_CallForwardInfo));

        memset(forwardPool, 0, forwardCount * sizeof(RIL_CallForwardInfo));

        /* init the pointer array */
        for(i = 0; i < forwardCount ; i++)
            forwardList[i] = &(forwardPool[i]);

        for (p_cur = p_response->p_intermediates
                ; p_cur != NULL
                ; p_cur = p_cur->p_next
            ) {
            err = forwardFromCCFCLine(p_cur->line, forwardList[validCount]);
            forwardList[validCount]->reason = data->reason;
            if (err == 0)
                validCount++;
        }

        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                validCount ? forwardList : NULL,
                validCount * sizeof (RIL_CallForwardInfo *));
    } else
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    at_response_free(p_response);
    return;

error:
    if (data->status == 2 ) {
        if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
            line = p_response->finalResponse;
            err = at_tok_start(&line);
            if (err < 0) goto error1;
            err = at_tok_nextint(&line,&errNum);
            if (err < 0) goto error1;
            if (errNum == 70 || errNum == 3) {
                RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
                at_response_free(p_response);
                return;
            }
        }
    }
error1:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestFacilityLock(int channelID,  char **data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    int result;
    int response[2] = {0};
    int serviceClass = 0;
    int status;
    char *cmd, *line;
    int errNum = -1;
    int ret = -1;
    ATLine *p_cur;

    if (datalen != 5 * sizeof(char *))
        goto error1;

    serviceClass = atoi(data[3]);
    if (serviceClass == 0) {
        ret = asprintf(&cmd, "AT+CLCK=\"%s\",%c,\"%s\"",
                data[0], *data[1], data[2]);
    } else {
        ret = asprintf(&cmd, "AT+CLCK=\"%s\",%c,\"%s\",%s",
                data[0], *data[1], data[2], data[3]);
    }
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error1;
    }
    RILLOGD("requestFacilityLock: %s", cmd);

    if (*data[1] == '2') {
        err = at_send_command_multiline(ATch_type[channelID], cmd, "+CLCK: ",
                &p_response);
        free(cmd);
    } else {
        err = at_send_command(ATch_type[channelID],  cmd, &p_response);
        free(cmd);
        if (err < 0 || p_response->success == 0) {
            goto error;
        }
        result = 1;
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
        at_response_free(p_response);
        return;
    }

    if (err < 0 || p_response->success == 0)
        goto error;

    for (p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
        line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &status);
        if (err < 0) goto error;
        if (at_tok_hasmore(&line)) {
            err = at_tok_nextint(&line, &serviceClass);
            if (err < 0) goto error;
        }

        response[0] = status;
        response[1] |= serviceClass;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
    at_response_free(p_response);
    return;

error:
    if(strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
        line = p_response->finalResponse;
        err = at_tok_start(&line);
        if (err >= 0) {
            err = at_tok_nextint(&line, &errNum);
            if(err >= 0) {
                if(errNum == 11 || errNum == 12) {
                    setRadioState(channelID, RADIO_STATE_SIM_LOCKED_OR_ABSENT);
                } else if (errNum == 70 || errNum == 3) {
                    RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
                    at_response_free(p_response);
                    return;
                }
            }
        }
    }
error1:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestChangeFacilityLock(int channelID,  char **data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err = -1;
    int errNum = 0xff;
    int result;
    char *cmd, *line;

    if (datalen != 3 * sizeof(char *))
        goto error;

    err = asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", data[0], data[1], data[2]);
    if(err < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    RILLOGD("requestChangeFacilityLock: %s", cmd);

    err = at_send_command(ATch_type[channelID],  cmd, &p_response);
    if (err < 0 || p_response->success == 0) {
        if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
            line = p_response->finalResponse;
            err = at_tok_start(&line);
            if (err >= 0) {
                err = at_tok_nextint(&line,&errNum);
            }
        }
        free(cmd);
        goto error;
    }
    if (*data[1] == '1')
        result = 1;
    else
        result = 0;
    free(cmd);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
    at_response_free(p_response);
    return;

error:
    if (err < 0 || errNum == 0xff) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else if (errNum == 70 || errNum == 3) {
        RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    }
    at_response_free(p_response);
}



/** do post-AT+CFUN=1 initialization */
static void onRadioPowerOn(int channelID)
{
    at_send_command(ATch_type[channelID], "AT+CTZR=1", NULL);

    pollSIMState(&channelID);
}

/** do post- SIM ready initialization */
static void onSIMReady(int channelID)
{
    at_send_command_singleline(ATch_type[channelID], "AT+CSMS=1", "+CSMS:", NULL);
    /*
     * Always send SMS messages directly to the TE
     *
     * mode = 1  discard when link is reserved (link should never be
     *             reserved)
     * mt = 2    most messages routed to TE
     * bm = 2    new cell BM's routed to TE
     * ds = 1    Status reports routed to TE
     * bfr = 1   flush buffer
     */
    at_send_command(ATch_type[channelID],"AT+CNMI=3,2,2,1,1", NULL);

    getSmsState(channelID);
}

static void requestSIMPower(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int onOff;
    int err;
    ATResponse *p_response = NULL;

    assert (datalen >= sizeof(int *));
    onOff = ((int *)data)[0];

    if (onOff == 0) {
        err = at_send_command(ATch_type[channelID], "AT+SFUN=3", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;
        setRadioState (channelID, RADIO_STATE_UNAVAILABLE);
    } else if (onOff > 0) {
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

static void requestRadioPower(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int onOff;
    int autoAttach = -1;
    int err, i;
    ATResponse *p_response = NULL;
    char sim_prop[PROPERTY_VALUE_MAX];
    char data_prop[PROPERTY_VALUE_MAX];
    extern int s_sim_num;

    assert (datalen >= sizeof(int *));
    onOff = ((int *)data)[0];

#if defined (RIL_SPRD_EXTENSION)
    autoAttach = ((int *)data)[1];
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(s_multiSimMode) {
        property_get(RIL_SIM1_ABSENT_PROPERTY, sim_prop, "0");
        property_get(RIL_DATA_PREFER_PROPERTY, data_prop, "0");
        RILLOGD(" requetRadioPower sim_prop = %s", sim_prop);
        RILLOGD(" requetRadioPower data_prop = %s", data_prop);
        if(!strcmp(sim_prop, "1")) {
            if (s_sim_num == 0) {
                if(!strcmp(data_prop, "1"))
                    autoAttach = 1;
                else
                    autoAttach = 0;
            } else if (s_sim_num == 1) {
                if(!strcmp(data_prop, "0"))
                    autoAttach = 1;
                else
                    autoAttach = 0;
            }
        } else {
            if (s_sim_num == 0) {
                if(!strcmp(data_prop, "0"))
                    autoAttach = 1;
                else
                    autoAttach = 0;
            } else if (s_sim_num == 1) {
                if(!strcmp(data_prop, "1"))
                    autoAttach = 1;
                else
                    autoAttach = 0;
            }
        }
    }
#endif

    if (onOff == 0) {
        /* The system ask to shutdown the radio */
        err = at_send_command(ATch_type[channelID], "AT+SFUN=5", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;

        for(i = 0; i < MAX_PDP; i++) {
            if (pdp[i].cid > 0) {
                RILLOGD("pdp[%d].state = %d", i, pdp[i].state);
                putPDP(i);
            }
        }

        setRadioState(channelID, RADIO_STATE_OFF);
    } else if (onOff > 0 && sState == RADIO_STATE_OFF) {
        if(s_multiSimMode) {
            if(autoAttach == 1)
                err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", &p_response);
            else
                err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=0", &p_response);
            if (err < 0 || p_response->success == 0)
                RILLOGE("GPRS auto attach failed!");
        } else {
            err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", &p_response);
            if (err < 0 || p_response->success == 0)
                RILLOGE("GPRS auto attach failed!");
        }
        err = at_send_command(ATch_type[channelID],  "AT+SFUN=4", &p_response);
        if (err < 0|| p_response->success == 0) {
            /* Some stacks return an error when there is no SIM,
             * but they really turn the RF portion on
             * So, if we get an error, let's check to see if it
             * turned on anyway
	     */

            if (isRadioOn(channelID) != 1) {
                goto error;
            }
        }
        setRadioState(channelID, RADIO_STATE_SIM_NOT_READY);
    }

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestOrSendDataCallList(int channelID, int cid, RIL_Token *t);

static void onDataCallListChanged(void *param)
{
    int channelID;
    channelID = getChannel();
    requestOrSendDataCallList(channelID, -1, NULL);
    putChannel(channelID);
}

static void onClass2SmsReceived(void *param)
{
    if(param == NULL)
    {
        RILLOGD("onClass2SmsReceived: param is null");
        return;
    }

    int err, skip;
    char *line;
    ATResponse *p_response = NULL;
    int sim_type, channelID;
    char *sms_pdu;
    char buf[256] = {0};
    int sms_length;
    int pdu_length;
    int simtype[4] = {0x3F007F10, 0x3F007FFF, 0x3F007FFF, 0x3F007F10};

    channelID = getChannel();
    err = at_send_command_singleline(ATch_type[channelID], "AT^CARDMODE", "^CARDMODE:", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &sim_type);
    if (err < 0) goto error;

    if(sim_type < 1 || sim_type > 4)
    {
          // 1 : sim
          // 2 : usim
          // 3 : test usim
          // 4 : test sim
          goto error;
    }
    sprintf(buf,"AT+CRSM=178,28476,%d,4,176,0,\"%x\"", *(int*)param, simtype[sim_type-1]);
    RILLOGD("buf : %s", buf);
    err = at_send_command_singleline(ATch_type[channelID], buf, "+CRSM:", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &skip);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &skip);
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &sms_pdu);
        if (err < 0) goto error;
    }
    RILLOGD("pdu : %s", sms_pdu);
    pdu_length = strlen(sms_pdu);
    if(!convertHexToBin(sms_pdu, pdu_length, buf))
    {
        sms_length = parsePdu(buf);  //sms bin length
        RILLOGD("sms binary length = %d", sms_length);
    } else {
        RILLOGD("convert class2 sms from hex to bin failed ");
        goto error;
    }
    sms_length = 2*sms_length; //sms hex length
    if(sms_length > pdu_length)
        goto error;
    sms_pdu[sms_length] = '\0';
    RILLOGD("pdu : %s", sms_pdu);
    RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS,
                &sms_pdu[2], sms_length - 2);  //skip the status
error:
    putChannel(channelID);
    at_response_free(p_response);
}

static void requestDataCallList(int channelID, void *data, size_t datalen, RIL_Token t)
{
    requestOrSendDataCallList(channelID, -1, &t);
}

static void requestOrSendDataCallList(int channelID, int cid, RIL_Token *t)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int err;
    int n = 0;
    char *out;
    int count = 0;

    err = at_send_command_multiline (ATch_type[channelID], "AT+CGACT?", "+CGACT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                    NULL, 0);
        at_response_free(p_response);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
            p_cur = p_cur->p_next)
        n++;

    RIL_Data_Call_Response_v6 *responses =
        alloca(n * sizeof(RIL_Data_Call_Response_v6));

    int i;
    for (i = 0; i < n; i++) {
        responses[i].status = -1;
        responses[i].suggestedRetryTime = -1;
        responses[i].cid = -1;
        responses[i].active = -1;
        responses[i].type = "";
        responses[i].ifname = "";
        responses[i].addresses = "";
        responses[i].dnses = "";
        responses[i].gateways = "";
    }

    RIL_Data_Call_Response_v6 *response = responses;
    for (p_cur = p_response->p_intermediates; p_cur != NULL;
            p_cur = p_cur->p_next) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->cid);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->active);
        if (err < 0)
            goto error;

        response++;
    }

    at_response_free(p_response);

    err = at_send_command_multiline (ATch_type[channelID], "AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                    NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
            p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;
        char *type;
        char *apn;
        char *address;
        char cmd[PROPERTY_VALUE_MAX] = {0};
        char eth[PROPERTY_VALUE_MAX] = {0};
        char prop[PROPERTY_VALUE_MAX] = {0};
        const int   dnslist_sz = 50;
        char*       dnslist = alloca(dnslist_sz);
        const char* separator = "";
        int nn;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cid);
        if (err < 0)
            goto error;

        for (i = 0; i < n; i++) {
            if (responses[i].cid == cid)
                break;
        }

        if (i >= n) {
            /* details for a context we didn't hear about in the last request */
            continue;
        }

        /* Assume no error */
        responses[i].status = PDP_FAIL_NONE;

        /* type */
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].type = alloca(strlen(out) + 1);
        strcpy(responses[i].type, out);

        /* APN ignored for v5 */
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        if(!strcmp(s_modem, "t")) {
            property_get(ETH_TD, eth, "veth");
        } else if(!strcmp(s_modem, "w")) {
            property_get(ETH_W, eth, "veth");
        } else {
            RILLOGE("Unknown modem type, exit");
            exit(-1);
        }

        snprintf(cmd, sizeof(cmd), "%s%d", eth, i);
        responses[i].ifname = alloca(strlen(cmd) + 1);
        strcpy(responses[i].ifname, cmd);

        snprintf(cmd, sizeof(cmd), "net.%s%d.ip", eth, i);
        property_get(cmd, prop, NULL);
        responses[i].addresses = alloca(strlen(prop) + 1);
        responses[i].gateways = alloca(strlen(prop) + 1);
        strcpy(responses[i].addresses, prop);
        strcpy(responses[i].gateways, prop);

        dnslist[0] = 0;
        for (nn = 0; nn < 2; nn++) {
            snprintf(cmd, sizeof(cmd), "net.%s%d.dns%d", eth, i, nn+1);
            property_get(cmd, prop, NULL);

            /* Append the DNS IP address */
            strlcat(dnslist, separator, dnslist_sz);
            strlcat(dnslist, prop, dnslist_sz);
            separator = " ";
        }
        responses[i].dnses = dnslist;

        RILLOGD("status=%d",responses[i].status);
        RILLOGD("suggestedRetryTime=%d",responses[i].suggestedRetryTime);
        RILLOGD("cid=%d",responses[i].cid);
        RILLOGD("active = %d",responses[i].active);
        RILLOGD("type=%s",responses[i].type);
        RILLOGD("ifname = %s",responses[i].ifname);
        RILLOGD("address=%s",responses[i].addresses);
        RILLOGD("dns=%s",responses[i].dnses);
        RILLOGD("gateways = %s",responses[i].gateways);
    }

    at_response_free(p_response);


    if(cid > 0) {
        RILLOGD("requestOrSendDataCallList is called by SetupDataCall!");
        for(i = 0; i < n; i++) {
            if(responses[i].cid == cid) {
                RIL_onRequestComplete(*t, RIL_E_SUCCESS, &responses[i],
                        sizeof(RIL_Data_Call_Response_v6));
                return;
            }
        }
        if(i >= n) {
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
            return;
        }
    }

    for(i = 0; i < n; i++) {
        if(responses[i].active == 1) {
            if (count != i) {
                responses[count].status = responses[i].status;
                responses[count].suggestedRetryTime = responses[i].suggestedRetryTime;
                responses[count].cid = responses[i].cid;
                responses[count].active = responses[i].active;
                responses[count].type = responses[i].type;
                responses[count].ifname = responses[i].ifname;
                responses[count].addresses = responses[i].addresses;
                responses[count].gateways = responses[i].gateways;
                responses[count].dnses = responses[i].dnses;
            }
            count++;
        }
    }


    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, responses,
                count * sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                responses,
                count * sizeof(RIL_Data_Call_Response_v6));

    return;

error:
    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                NULL, 0);

    at_response_free(p_response);
}

static void convertFailCause(int cause)
{
    int failCause = cause;

    switch(failCause) {
        case MN_GPRS_ERR_NO_SATISFIED_RESOURCE:
        case MN_GPRS_ERR_INSUFF_RESOURCE:
        case MN_GPRS_ERR_MEM_ALLOC:
        case MN_GPRS_ERR_LLC_SND_FAILURE:
        case MN_GPRS_ERR_OPERATION_NOT_ALLOWED:
        case MN_GPRS_ERR_SPACE_NOT_ENOUGH:
        case MN_GPRS_ERR_TEMPORARILY_BLOCKED:
            s_lastPdpFailCause = PDP_FAIL_INSUFFICIENT_RESOURCES;
            break;
        case MN_GPRS_ERR_SERVICE_OPTION_OUTOF_ORDER:
        case MN_GPRS_ERR_OUT_OF_ORDER_SERVICE_OPTION:
            s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_OUT_OF_ORDER;
            break;
        case MN_GPRS_ERR_PDP_AUTHENTICATION_FAILED:
        case MN_GPRS_ERR_AUTHENTICATION_FAILURE:
            s_lastPdpFailCause = PDP_FAIL_USER_AUTHENTICATION;
            break;
        case MN_GPRS_ERR_NO_NSAPI:
        case MN_GPRS_ERR_PDP_TYPE:
        case MN_GPRS_ERR_PDP_ID:
        case MN_GPRS_ERR_NSAPI:
        case MN_GPRS_ERR_UNKNOWN_PDP_ADDR_OR_TYPE:
        case MN_GPRS_ERR_INVALID_TI:
            s_lastPdpFailCause = PDP_FAIL_UNKNOWN_PDP_ADDRESS_TYPE;
            break;
        case MN_GPRS_ERR_SERVICE_OPTION_NOT_SUPPORTED:
        case MN_GPRS_ERR_UNSUPPORTED_SERVICE_OPTION:
        case MN_GPRS_ERR_FEATURE_NOT_SUPPORTED:
        case MN_GPRS_ERR_QOS_NOT_ACCEPTED:
        case MN_GPRS_ERR_ATC_PARAM:
        case MN_GPRS_ERR_PERMENANT_PROBLEM:
        case MN_GPRS_ERR_READ_TYPE:
        case MN_GPRS_ERR_STARTUP_FAILURE:
            s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_NOT_SUPPORTED;
            break;
        case MN_GPRS_ERR_ACTIVE_REJCET:
        case MN_GPRS_ERR_REQUEST_SERVICE_OPTION_NOT_SUBSCRIBED:
        case MN_GPRS_ERR_UNSUBSCRIBED_SERVICE_OPTION:
            s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED;
            break;
        case MN_GPRS_ERR_ACTIVATION_REJ_GGSN:
            s_lastPdpFailCause = PDP_FAIL_ACTIVATION_REJECT_GGSN;
            break;
        case MN_GPRS_ERR_ACTIVATION_REJ:
        case MN_GPRS_ERR_MODIFY_REJ:
            s_lastPdpFailCause = PDP_FAIL_ACTIVATION_REJECT_UNSPECIFIED;
            break;
        case MN_GPRS_ERR_MISSING_OR_UNKOWN_APN:
        case MN_GPRS_ERR_UNKNOWN_APN:
            s_lastPdpFailCause = PDP_FAIL_MISSING_UKNOWN_APN;
            break;
        case MN_GPRS_ERR_SAME_PDP_CONTEXT:
        case MN_GPRS_ERR_NSAPI_ALREADY_USED:
            s_lastPdpFailCause = PDP_FAIL_NSAPI_IN_USE;
            break;
        case MN_GPRS_ERR_OPERATOR_DETERMINE_BAR:
            s_lastPdpFailCause = PDP_FAIL_OPERATOR_BARRED;
            break;
        case MN_GPRS_ERR_INCORRECT_MSG:
        case MN_GPRS_ERR_SYNTACTICAL_ERROR_IN_TFT_OP:
        case MN_GPRS_ERR_SEMANTIC_ERROR_IN_PACKET_FILTER:
        case MN_GPRS_ERR_SYNTAX_ERROR_IN_PACKET_FILTER:
        case MN_GPRS_ERR_PDP_CONTEXT_WO_TFT_ALREADY_ACT:
        case MN_GPRS_ERR_CONTEXT_CAUSE_CONDITIONAL_IE_ERROR:
        case MN_GPRS_ERR_UNIMPLE_MSG_TYPE:
        case MN_GPRS_ERR_UNIMPLE_IE:
        case MN_GPRS_ERR_INCOMP_MSG_PROTO_STAT:
        case MN_GPRS_ERR_SEMANTIC_ERROR_IN_TFT_OP:
        case MN_GPRS_ERR_INCOMPAT_MSG_TYP_PROTO_STAT:
        case MN_GPRS_ERR_UNKNOWN_PDP_CONTEXT:
        case MN_GPRS_ERR_NO_PDP_CONTEXT:
        case MN_GPRS_ERR_PDP_CONTEXT_ACTIVATED:
        case MN_GPRS_ERR_INVALID_MAND_INFO:
        case MN_GPRS_ERR_PRIMITIVE:
            s_lastPdpFailCause = PDP_FAIL_PROTOCOL_ERRORS;
            break;
        case MN_GPRS_ERR_SENDER:
        case MN_GPRS_ERR_RETRYING:
        case MN_GPRS_ERR_UNKNOWN_ERROR:
        case MN_GPRS_ERR_REGULAR_DEACTIVATION:
        case MN_GPRS_ERR_REACTIVATION_REQD:
        case MN_GPRS_ERR_SM_ERR_UNSPECIFIED:
        case MN_GPRS_ERR_UNSPECIFIED:
            s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
            break;
        default:
            s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
    }
}

static void requestSetupDataCall(int channelID, void *data, size_t datalen, RIL_Token t)
{
    const char *apn = NULL;
    const char *username = NULL;
    const char *password = NULL;
    const char *authtype = NULL;
    char cmd[128] = {0};
    int err;
    char response[20];
    ATResponse *p_response = NULL;
    int index = -1;
    char qos_state[PROPERTY_VALUE_MAX];
    char *line = NULL;
    const  char *pdp_type;
    int failCause;

    apn = ((const char **)data)[2];
    username = ((const char **)data)[3];
    password = ((const char **)data)[4];
    authtype = ((const char **)data)[5];

    RILLOGD("requestSetupDataCall data[0] '%s'", ((const char **)data)[0]);
    RILLOGD("requestSetupDataCall data[1] '%s'", ((const char **)data)[1]);
    RILLOGD("requestSetupDataCall data[2] '%s'", ((const char **)data)[2]);
    RILLOGD("requestSetupDataCall data[3] '%s'", ((const char **)data)[3]);
    RILLOGD("requestSetupDataCall data[4] '%s'", ((const char **)data)[4]);
    RILLOGD("requestSetupDataCall data[5] '%s'", ((const char **)data)[5]);
    RILLOGD("requestSetupDataCall data[6] '%s'", ((const char **)data)[4]);
    RILLOGD("requestSetupDataCall data[7] '%s'", ((const char **)data)[5]);


    if (ATch_type[channelID]) {

        if (datalen > 6 * sizeof(char *)) {
            pdp_type = ((const char **)data)[6];
        } else {
            pdp_type = "IP";
        }

        index = getPDP();
        if(index < 0 || pdp[index].cid >= 0)
            goto error;

        ATch_type[channelID]->nolog = 0;

        snprintf(cmd, sizeof(cmd), "AT+CGACT=0,%d", index+1);
        at_send_command(ATch_type[channelID], cmd, NULL);

        snprintf(cmd, sizeof(cmd), "AT+CGDCONT=%d,\"%s\",\"%s\",\"\",0,0", index+1, pdp_type, apn);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err < 0 || p_response->success == 0){
            s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
            goto error;
        }

        snprintf(cmd, sizeof(cmd), "AT+CGPCO=0,\"%s\",\"%s\",%d,%d", username, password,index+1,atoi(authtype));
        at_send_command(ATch_type[channelID], cmd, NULL);

        /* Set required QoS params to default */
        property_get("persist.sys.qosstate", qos_state, "0");
        if(!strcmp(qos_state, "0")) {
            snprintf(cmd, sizeof(cmd), "AT+CGEQREQ=%d,2,0,0,0,0,2,0,\"1e4\",\"0e0\",3,0,0", index+1);
            at_send_command(ATch_type[channelID], cmd, NULL);
        }

        snprintf(cmd, sizeof(cmd), "AT+CGDATA=\"PPP\",%d", index+1);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err < 0 || p_response->success == 0) {
            if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
                line = p_response->finalResponse;
                err = at_tok_start(&line);
                if (err >= 0) {
                    err = at_tok_nextint(&line,&failCause);
                    if (err >= 0) {
                        convertFailCause(failCause);
                    } else {
                        s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
                        goto error;
                    }
                }
            } else
                s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;

            goto error;
        }

        pthread_mutex_lock(&pdp[index].mutex);
        pdp[index].cid = index + 1;
        pthread_mutex_unlock(&pdp[index].mutex);

    } else
        goto error;

    requestOrSendDataCallList(channelID, index+1, &t);

    at_response_free(p_response);

    return;
error:
    if(index >= 0)
        putPDP(index);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    if(p_response)
        at_response_free(p_response);
}

/**
 * RIL_REQUEST_LAST_PDP_FAIL_CAUSE
 *
 * Requests the failure cause code for the most recently failed PDP
 * context activate.
 *
 * See also: RIL_REQUEST_LAST_CALL_FAIL_CAUSE.
 *
 */
void requestLastDataFailCause(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int response = PDP_FAIL_ERROR_UNSPECIFIED;

    response = s_lastPdpFailCause;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
            sizeof(int));
}

void requestLastCallFailCause(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int response = CALL_FAIL_ERROR_UNSPECIFIED;

    pthread_mutex_lock(&s_call_mutex);
    switch(call_fail_cause) {
        case 1:
        case 22:
        case 28:
            response = CALL_FAIL_UNOBTAINABLE_NUMBER;
            break;
        case 0:
        case 3:
        case 16:
        case 301:
            response = CALL_FAIL_NORMAL;
            break;
        case 302:
            response = CALL_FAIL_IMEI_NOT_ACCEPTED;
            break;
        case 17:
        case 21:
            response = CALL_FAIL_BUSY;
            break;
        case 34:
        case 38:
        case 41:
        case 42:
        case 44:
        case 47:
            response = CALL_FAIL_CONGESTION;
            break;
        case 68:
            response = CALL_FAIL_ACM_LIMIT_EXCEEDED;
            break;
        case 8:
            response = CALL_FAIL_CALL_BARRED;
            break;
        case 241:
            response = CALL_FAIL_FDN_BLOCKED;
            break;
        default:
            response = CALL_FAIL_ERROR_UNSPECIFIED;
    }
    pthread_mutex_unlock(&s_call_mutex);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
            sizeof(int));
}

static void requestBasebandVersion(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    char  response[80] ;
    char* line = NULL;
    ATLine *p_cur = NULL;
    int i;

    err = at_send_command_multiline(ATch_type[channelID], "AT+CGMR", "", &p_response);
    memset(response,0,sizeof(response));
    if (err != 0 || p_response->success == 0) {
        RILLOGE("requestBasebandVersion:Send command error!");
        goto error;
    }
    for (i=0,p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next,i++) {

        line = p_cur->line;
        if(i < 2)
            continue;
        if(i < 4) {
            if(at_tok_start(&line) == 0) {
                skipWhiteSpace(&line);
                strcat(response, line);

                strcat(response,"|");
            } else
                continue;
        } else {
            skipWhiteSpace(&line);
            strcat(response, line);
        }
    }
    if(strlen(response) == 0) {
        RILLOGE("requestBasebandVersion: Parameter parse error!");
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, strlen(response));
    at_response_free(p_response);
    return;

error:
    RILLOGE("ERROR: requestBasebandVersion failed\n");
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


static void requestNetworkList(int channelID, void *data, size_t datalen, RIL_Token t)
{
    static char *statStr[] = {
        "unknown",
        "available",
        "current",
        "forbidden"
    };
    static char *actStr[] = {
        "GSM",
        "GSMCompact",
        "UTRAN"
    };
    int err, stat, act;
    char *line;
    char **responses, **cur;
    ATResponse *p_response = NULL;
    int tok = 0, count = 0, i = 0;

    err = at_send_command_singleline(ATch_type[channelID], "AT+COPS=?", "+COPS:",
            &p_response);
    if (err != 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    while (*line) {
        if(*line == '(')
            tok++;
        if(*line  == ')') {
            if(tok == 1) {
                count++;
                tok--;
            }
        }
        if(*line == '"') {
            do {
                line++;
                if(*line == 0)
                    break;
                else if(*line == '"')
                    break;
            }while(1);
        }
        if(*line != 0)
            line++;
    }
    RILLOGD("Searched available network list numbers = %d", count - 2);
    if(count <= 2)
        goto error;
    count = count - 2;
    responses = alloca(count * 5 * sizeof(char *));

    line = p_response->p_intermediates->line;
    cur = responses;

    while ( (line = strchr(line, '(')) && (i++ < count) ) {
        line++;

        err = at_tok_nextint(&line, &stat);
        if (err < 0) continue;

        cur[3] = statStr[stat];

        err = at_tok_nextstr(&line, &(cur[0]));
        if (err < 0) continue;

        err = at_tok_nextstr(&line, &(cur[1]));
        if (err < 0) continue;

        err = at_tok_nextstr(&line, &(cur[2]));
        if (err < 0) continue;

#if defined (RIL_SPRD_EXTENSION)
        err = at_tok_nextint(&line, &act);
        if (err < 0) continue;

        cur[4] = actStr[act];
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        err = at_tok_nextstr(&line, &(cur[4]));
        if (err < 0) continue;
#endif

        cur += 5;
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, responses, count * 5 * sizeof(char *));
    at_response_free(p_response);
    return;
error:

    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestQueryNetworkSelectionMode(int channelID,
                void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline(ATch_type[channelID], "AT+COPS?", "+COPS:", &p_response);
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

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    RILLOGE("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestNetworkRegistration(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    char cmd[128] = {0};
    int err;
#ifdef RIL_SPRD_EXTENSION
    RIL_NetworkList *network = (RIL_NetworkList *)data;
#else
    char *network = (char *)data;
#endif
    ATResponse *p_response = NULL;

    if (network) {
#ifdef RIL_SPRD_EXTENSION
        snprintf(cmd, sizeof(cmd), "AT+COPS=1,2,\"%s\",%d", network->operatorNumeric, network->act);
#else
        snprintf(cmd, sizeof(cmd), "AT+COPS=1,2,\"%s\"", network);
#endif
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err != 0 || p_response->success == 0)
            goto error;
    } else
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void resetModem(void * param)
{
    int channelID;

    channelID = getChannel();
    RILLOGD("resetModem channelID = %d", channelID);
    if(channelID == 0) {
        RILLOGD("resetModem channel is busy");
        RIL_requestTimedCallback (resetModem, NULL, &TIMEVAL_SIMPOLL);
    } else {
	char modemrst_property[PROPERTY_VALUE_MAX];

        memset(modemrst_property, 0, sizeof(modemrst_property));
        property_get(RIL_MODEM_RESET_PROPERTY, modemrst_property, "");
        RILLOGD("%s is %s", RIL_MODEM_RESET_PROPERTY, modemrst_property);
        property_set(RIL_MODEM_RESET_PROPERTY, "1");
        at_send_command(ATch_type[channelID], "AT+SPATASSERT=1", NULL);
    }
    putChannel(channelID);
    return;
}

static void onSimAbsent(void *param)
{
    int channelID;

    channelID = getChannel();
    setRadioState(channelID, RADIO_STATE_SIM_LOCKED_OR_ABSENT);
    putChannel(channelID);

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,
                                    NULL, 0);
    RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_DROP, NULL, 0);
}

static void onSimPresent(void *param)
{
    int channelID;

    if (sState == RADIO_STATE_SIM_LOCKED_OR_ABSENT) {
        channelID = getChannel();
        setRadioState(channelID, RADIO_STATE_SIM_NOT_READY);
        putChannel(channelID);
    }
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,
                                    NULL, 0);
}

static void sendCallStateChanged(void *param)
{
    RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
            NULL, 0);
}

static void sendVideoCallStateChanged(void *param)
{
#if defined (RIL_SPRD_EXTENSION)
    RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VIDEOCALL_STATE_CHANGED,
            NULL, 0);
#endif
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

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pp_calls,
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

static void requestDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    int err;
    int ret;

    p_dial = (RIL_Dial *)data;

    RILLOGD("requestDial isStkCall = %d", s_isstkcall);
    if (s_isstkcall == 1) {
        RILLOGD(" setup STK call ");
        s_isstkcall = 0;
//      wait4android_audio_ready("ATD");
        err = at_send_command(ATch_type[channelID], "AT+SPUSATCALLSETUP=1", NULL);
        if (err != 0) goto error;
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        return;
    }

    switch (p_dial->clir) {
        case 1: clir = "I"; break;  /*invocation*/
        case 2: clir = "i"; break;  /*suppression*/
        default:
        case 0: clir = ""; break;   /*subscription default*/
    }

    ret = asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
//  wait4android_audio_ready("ATD");
    err = at_send_command(ATch_type[channelID], cmd, NULL);
    free(cmd);
    if (err != 0) goto error;

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestEccDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    char *category = NULL;
    int ret, err;

    p_dial = (RIL_Dial *)data;

    switch (p_dial->clir) {
        case 1: clir = "I"; break;  /*invocation*/
        case 2: clir = "i"; break;  /*suppression*/
        default:
        case 0: clir = ""; break;   /*subscription default*/
    }

    category = strchr(p_dial->address, '/');
    if(category)
        *category = '@';

    ret = asprintf(&cmd, "ATD%s,#%s;", p_dial->address, clir);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
//  wait4android_audio_ready("ATD");
    err = at_send_command(ATch_type[channelID], cmd, NULL);
    free(cmd);
    if (err != 0) goto error;

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestWriteSmsToSim(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_SMS_WriteArgs *p_args;
    char *cmd, *cmd1;
    int length;
    int err;
    ATResponse *p_response = NULL;
    const char *smsc;
    char *line = NULL;
    int errorNum;
    int ret;

    p_args = (RIL_SMS_WriteArgs *)data;

    length = strlen(p_args->pdu)/2;

    smsc = (const char *)(p_args->smsc);
    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    ret = asprintf(&cmd, "AT+CMGW=%d,%d", length, p_args->status);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error1;
    }
    ret = asprintf(&cmd1, "%s%s", smsc, p_args->pdu);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        free(cmd);
        cmd1 = NULL;
        goto error1;
    }

    err = at_send_command_sms(ATch_type[channelID], cmd, cmd1, "+CMGW:", &p_response);
    free(cmd);
    free(cmd1);

    if (err != 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;
error:
    if (strStartsWith(p_response->finalResponse,"+CMS ERROR:")) {
        line = p_response->finalResponse;
        err = at_tok_start(&line);
        if (err < 0)
            goto error1;
        err = at_tok_nextint(&line, &errorNum);
        if (err < 0)
            goto error1;
        if(errorNum != 322)
            goto error1;
#if defined (RIL_SPRD_EXTENSION)
        RIL_onRequestComplete(t, RIL_E_SMS_SAVE_FAIL_FULL, NULL, 0);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
#endif
        at_response_free(p_response);
        return;
    }
error1:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}
static void requestHangup(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int *p_line;
    int ret;
    char cmd[20] = {0};
    ATResponse *p_response = NULL;

    p_line = (int *)data;

    /* 3GPP 22.030 6.5.5
     * "Releases a specific active call X"
     */
    snprintf(cmd, sizeof(cmd), "AT+CHLD=7%d", p_line[0]);
    all_calls(channelID, 1);
    ret = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (ret < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

static void requestSignalStrength(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    RIL_SignalStrength_v6 response_v6;
    char *line;

    memset(&response_v6, -1, sizeof(response_v6));
    err = at_send_command_singleline(ATch_type[channelID], "AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response_v6.GW_SignalStrength.signalStrength));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response_v6.GW_SignalStrength.bitErrorRate));
    if (err < 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response_v6, sizeof(RIL_SignalStrength_v6));

    at_response_free(p_response);
    return;

error:
    RILLOGE("requestSignalStrength must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestSsSignalStrength(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    RIL_SignalStrength_v6 response_v6;
    char *line;
    int csq, rat;

    memset(&response_v6, -1, sizeof(response_v6));
    err = at_send_command_singleline(ATch_type[channelID], "AT+SPSCSQ", "+SPSCSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &csq);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response_v6.GW_SignalStrength.bitErrorRate));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &rat);
    if (err < 0) goto error;

    if(rat == 0) {
        /* convert GSM rssi to asu*/
        if(csq <= 5 || csq == 99)
            csq = 1;
        else if(csq <= 7)
            csq = 4;
        else if(csq <= 9)
            csq = 7;
        else if(csq <= 12)
            csq = 10;
        else if(csq > 12)
            csq = 14;
    } else if(rat == 1) {
        /* convert UMTS rssi to asu*/
        if(csq <= 1 || csq == 99)
            csq = 1;
        else if(csq <= 4)
            csq = 4;
        else if(csq <= 7)
            csq = 7;
        else if(csq <= 11)
            csq = 10;
        else if(csq > 11)
            csq = 14;
    } else
        goto error;

    response_v6.GW_SignalStrength.signalStrength = csq;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response_v6, sizeof(RIL_SignalStrength_v6));
    at_response_free(p_response);
    return;

error:
    RILLOGE("requestSsSignalStrength must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


/* Mapping <Act> response to following data:
 * Ref:\frameworks\base\telephony\java\android\telephony\ServiceState.java
 *   public static final int RADIO_TECHNOLOGY_UNKNOWN = 0;
 *   public static final int RADIO_TECHNOLOGY_GPRS = 1;
 *   public static final int RADIO_TECHNOLOGY_EDGE = 2;
 *   public static final int RADIO_TECHNOLOGY_UMTS = 3;
 *   public static final int RADIO_TECHNOLOGY_IS95A = 4;
 *   public static final int RADIO_TECHNOLOGY_IS95B = 5;
 *   public static final int RADIO_TECHNOLOGY_1xRTT = 6;
 *   public static final int RADIO_TECHNOLOGY_EVDO_0 = 7;
 *   public static final int RADIO_TECHNOLOGY_EVDO_A = 8;
 *   public static final int RADIO_TECHNOLOGY_HSDPA = 9;
 *   public static final int RADIO_TECHNOLOGY_HSUPA = 10;
 *   public static final int RADIO_TECHNOLOGY_HSPA = 11;
*/
static int mapCgregResponse(int in_response)
{
    int out_response = 0;

    switch(in_response) {
        case 0:
            out_response = 1;    /* GPRS */
            break;
        case 3:
            out_response = 2;    /* EDGE */
            break;
        case 2:
            out_response = 3;    /* TD */
            break;
        case 4:
            out_response = 9;    /* HSDPA */
            break;
        case 5:
            out_response = 10;   /* HSUPA */
            break;
        case 6:
            out_response = 11;   /* HSPA */
            break;
        case 15:
            out_response = 15;   /* HSPA+ */
            break;
        default:
            out_response = 0;    /* UNKNOWN */
            break;
    }
    return out_response;
}

static void requestRegistrationState(int channelID, int request, void *data,
                                        size_t datalen, RIL_Token t)
{
    int err;
    int response[4] = { -1, -1, -1, -1 };
    char *responseStr[15] = {NULL};
    char res[5][20];
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line, *p;
    int commas;
    int skip;
    int i;


    if (request == RIL_REQUEST_VOICE_REGISTRATION_STATE) {
        cmd = "AT+CREG?";
        prefix = "+CREG:";
    } else if (request == RIL_REQUEST_DATA_REGISTRATION_STATE) {
        cmd = "AT+CGREG?";
        prefix = "+CGREG:";
    } else {
        assert(0);
        goto error;
    }

    err = at_send_command_singleline(ATch_type[channelID], cmd, prefix, &p_response);

    if (err != 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    /* Ok you have to be careful here
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here
     *
     * finally, a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     */

    /* count number of commas */
    commas = 0;
    for (p = line ; *p != '\0' ;p++) {
        if (*p == ',') commas++;
    }

    switch (commas) {
        case 0: /* +CREG: <stat> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            break;

        case 1: /* +CREG: <n>, <stat> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            break;

        case 2: /* +CREG: <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            break;
        case 3: /* +CREG: <n>, <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            break;
            /* special case for CGREG, there is a fourth parameter
             * that is the network type (unknown/gprs/edge/umts)
             */
        case 4: /* +CGREG: <n>, <stat>, <lac>, <cid>, <networkType> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[3]);
            if (err < 0) goto error;
            break;
        default:
            goto error;
    }

#if defined (RIL_SPRD_EXTENSION)
    sprintf(res[0], "%d", response[0]);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(8 == response[0])
        sprintf(res[0], "14");
    else
        sprintf(res[0], "%d", response[0]);
#endif
    responseStr[0] = res[0];

    if (response[1] != -1) {
        sprintf(res[1], "%x", response[1]);
        responseStr[1] = res[1];
    }

    if (response[2] != -1) {
        sprintf(res[2], "%x", response[2]);
        responseStr[2] = res[2];
    }

    if (response[3] != -1) {
        response[3] = mapCgregResponse(response[3]);
        sprintf(res[3], "%d", response[3]);
        responseStr[3] = res[3];
    }

    if (request == RIL_REQUEST_VOICE_REGISTRATION_STATE) {
        sprintf(res[4], "0");
        responseStr[7] = res[4];
        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, 15*sizeof(char*));
    } else if (request == RIL_REQUEST_DATA_REGISTRATION_STATE) {
        sprintf(res[4], "3");
        responseStr[5] = res[4];
        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, 6*sizeof(char*));
    }
    at_response_free(p_response);
    return;
error:
    RILLOGE("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestNeighboaringCellIds(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_NeighboringCell *NeighboringCell;
    RIL_NeighboringCell **NeighboringCellList;
    int err = 0;
    int cell_id_number;
    int current = 0;
    char *line;
    ATLine *p_cur;

    err = at_send_command_singleline(ATch_type[channelID], "AT+Q2GNCELL",
		    "+Q2GNCELL:", &p_response);
    if (err != 0 || p_response->success == 0)
        goto error;

    p_cur = p_response->p_intermediates;
    line = p_cur->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    if(at_tok_hasmore(&line)) {
        char *sskip = NULL;
	int skip;

        err = at_tok_nextstr(&line, &sskip);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &cell_id_number);
        if (err < 0 || cell_id_number == 0)
            goto error;
    } else {
        at_response_free(p_response);
        err = at_send_command_singleline(ATch_type[channelID], "AT+Q3GNCELL",
		    "+Q3GNCELL:", &p_response);
        if (err != 0 || p_response->success == 0)
            goto error;

        p_cur = p_response->p_intermediates;
        line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        if(at_tok_hasmore(&line)) {
            char *sskip = NULL;
	    int skip;

            err = at_tok_nextstr(&line, &sskip);
            if (err < 0) goto error;

            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;

            err = at_tok_nextint(&line, &cell_id_number);
            if (err < 0 || cell_id_number == 0)
                goto error;
        } else
            goto error;
    }

    NeighboringCellList = (RIL_NeighboringCell **) alloca(cell_id_number *
            sizeof(RIL_NeighboringCell *));

    NeighboringCell = (RIL_NeighboringCell *) alloca(cell_id_number *
            sizeof(RIL_NeighboringCell));

    for (current = 0; at_tok_hasmore(&line), current < cell_id_number;
            current++) {

        err = at_tok_nextstr(&line, &(NeighboringCell[current].cid));
        if (err < 0)
            goto error;

	err = at_tok_nextint(&line, &(NeighboringCell[current].rssi));
        if (err < 0)
            goto error;

        RILLOGD("Neighbor cell_id %s = %d", NeighboringCell[current].cid,
                NeighboringCell[current].rssi);

        NeighboringCellList[current] = &NeighboringCell[current];
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NeighboringCellList,
            cell_id_number * sizeof(RIL_NeighboringCell *));
    at_response_free(p_response);
    return;
error:
    RILLOGD("requestNeighboaringCellIds fail");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestOperator(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;

    err = at_send_command_multiline(ATch_type[channelID],
            "AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?",
            "+COPS:", &p_response);

    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     */

    if (err != 0) goto error;

    for (i = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next, i++
        ) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        /* If we're unregistered, we may just get
         * a "+COPS: 0" response
	 */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        /* a "+COPS: 0, n" response is also possible */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));
        if (err < 0) goto error;
    }

    if (i != 3) {
        /* expect 3 lines exactly */
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;
error:
    RILLOGE("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);

}

static void requestSetCmms(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char cmd[20] = {0};
    int enable = ((int *)data)[0];

    snprintf(cmd, sizeof(cmd), "AT+CMMS=%d",enable);
    at_send_command( ATch_type[channelID], cmd, NULL);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}


static void requestSetSpeedMode(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char cmd[20] = {0};
    int enable = ((int *)data)[0];
    char prop[PROPERTY_VALUE_MAX] = {0};
    int fd = -1;
    int n = -1;
    int lockFd = -1;
    int speedCount = 0;
    int i;

    RILLOGD("requestSetSpeedMode  enable = %d", enable);
    for(i=0; i<3; i++) {
        lockFd = open("/data/local/tmp/rilLock", O_RDWR | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR);
        RILLOGD("requestSetSpeedMode  lockFd = %d", lockFd);
        if (lockFd >= 0) {
            fd = open("/sys/module/ipc_sdio/parameters/sdio_tx_wait_time", O_RDWR);
            if (fd >= 0) {
                if (enable == 1) {
                    property_get(RIL_SET_SPEED_MODE_COUNT, prop, "0");
                    RILLOGD("requestSetSpeedMode   prop = %s", prop);
					speedCount = atoi(prop);
                    if (speedCount == 0) {
                        property_set(RIL_SET_SPEED_MODE_COUNT, "1");
                        n = write(fd, "0", 2);
                        if (n < 0) {
                            perror("write");
                            goto out;
                        }
                    } else if(speedCount == 1) {
                       property_set(RIL_SET_SPEED_MODE_COUNT, "2");
                       goto out;
                    } else if(speedCount == 2) {
                       property_set(RIL_SET_SPEED_MODE_COUNT, "3");
                       goto out;
                    }
                } else {
                    property_get(RIL_SET_SPEED_MODE_COUNT, prop, "0");
                    RILLOGD("requestSetSpeedMode   prop = %s", prop);
					speedCount = atoi(prop);
                    if (speedCount == 1) {
                        property_set(RIL_SET_SPEED_MODE_COUNT, "0");
                        n = write(fd, "10", 3);
                        if (n < 0) {
                            perror("write");
                            goto out;
                        }
                    } else if(speedCount == 2) {
                        property_set(RIL_SET_SPEED_MODE_COUNT, "1");
                        goto out;
                    } else if(speedCount == 3) {
                        property_set(RIL_SET_SPEED_MODE_COUNT, "2");
                        goto out;
                    }
                }
                snprintf(cmd, sizeof(cmd), "AT+SPTEST=16,1,%d",enable);
                at_send_command( ATch_type[channelID], cmd, NULL);
            }
            break;
        } else {
            RILLOGD("requestSetSpeedMode  errno = %d", errno);
            usleep(500000);
        }
    }
out:
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    if (lockFd >= 0) {
        close(lockFd);
        RILLOGD("requestSetSpeedMode  delete rilLock");
        unlink("/data/local/tmp/rilLock");
    }
    if (fd >= 0)
        close(fd);
}

static void requestSendSMS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err;
    const char *smsc;
    const char *pdu;
    int tpLayerLength;
    char *cmd1, *cmd2;
    RIL_SMS_Response response;
    ATResponse *p_response = NULL;
    char * line;
    int ret;

    memset(&response, 0, sizeof(RIL_SMS_Response));
    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    ret = asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd1 = NULL;
        goto error1;
    }
    ret = asprintf(&cmd2, "%s%s", smsc, pdu);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        free(cmd1);
        cmd2 = NULL;
        goto error1;
    }

    err = at_send_command_sms(ATch_type[channelID], cmd1, cmd2, "+CMGS:", &p_response);
    free(cmd1);
    free(cmd2);
    if (err != 0 || p_response->success == 0) goto error;

    /* FIXME fill in messageRef and ackPDU */

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if(err < 0)
        goto error1;
    err = at_tok_nextint(&line, &response.messageRef);
    if(err < 0)
        goto error1;
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(RIL_SMS_Response));
    at_response_free(p_response);
    return;
error:
    line = p_response->finalResponse;
    err = at_tok_start(&line);
    if (err < 0)
        goto error1;
    err = at_tok_nextint(&line, &response.errorCode);
    if (err < 0)
        goto error1;
    if((response.errorCode != 313) && (response.errorCode != 512))
        goto error1;
    if (response.errorCode == 313) {
        RIL_onRequestComplete(t, RIL_E_SMS_SEND_FAIL_RETRY, NULL, 0);
    } else if (response.errorCode == 512) {
        RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
    }
    at_response_free(p_response);
    return;
error1:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)
#define BIT4 (1 << 4)
#define BIT5 (1 << 5)
#define BIT6 (1 << 6)
#define BIT7 (1 << 7)

#define	TP_VPF_NOT_PRESENT	0x00
#define	TP_VPF_ENHANCE		0x01
#define	TP_VPF_RELATIVE		0x10
#define	TP_VPF_ABSOLUTE		0x11

int is_long_sms(char *pdu, int *max_num, int *seq_num)
{
    char *p = pdu;
    int tp_vp, tp_udhl;
    char elem_id;
    int elem_length;
    int addr_length;

    //first octet
    if(!(*p & BIT6)) {
        return 0;
    } else {
        switch(*p & (BIT4 | BIT3)) {
            case TP_VPF_RELATIVE:
                tp_vp = 1;
                break;
            case TP_VPF_ENHANCE:
            case TP_VPF_ABSOLUTE:
                tp_vp = 7;
                break;
            case TP_VPF_NOT_PRESENT:
            default:
                tp_vp = 0;
        }
        //TP_DA
        p += 2;
        //TP_PID
        addr_length =  (*p + 1)/2;
        p += (addr_length + 2);
        //TP_VP
        p += 2;
        //TP_UDL
        p += tp_vp;
        //TP_UDHL
        p += 1;
        tp_udhl = *p;
        //the first element
        p += 1;
        while(tp_udhl >= 5) {
            elem_id = *p;
            if(0x00 != elem_id && 0x08 != elem_id) {
                //element identifier octet
                p += 1;
                elem_length = *p;
                //the next element identifier octet
                p += (elem_length + 1);
            } else {
                //element identifier octet
                p += 1;
                elem_length = *p;
                if(0x00 == elem_id)
                    p += 2;
                else if(0x08 == elem_id)
                    p += 3;
                //max number of concatenated sms
                *max_num = *p;
                //sequence number of current sms
                *seq_num = *(p+1);
                return 1;
            }
            tp_udhl -= (elem_length + 2);
        }
        return 0;
    }
}

int parsePdu(char *pdu)
{
    char *p = pdu + 1; // Skip status
    int smsc_len;
    int tp_udl = 0;
    int addr_length;
    int sms_length;
    int dcs_type; // 0000 0000 7bit, 0000 0100 8bit, 0000 1000 16bit

    if(*p == 0xFF)
        smsc_len = 0;
    else
        smsc_len = *p;

    //TPDU
    //TP_OA: just skip the first octet
    p += smsc_len + 2;

    //TP_PID
    //TP_DCS
    addr_length =  (*p + 1) >> 1;
    p += (addr_length + 3);

    dcs_type = *p & 0x0C;
    RILLOGD("dcs_type : %d", dcs_type);

    //TP_TS
    //TP_UDL
    p += 8;

    switch(dcs_type)
    {
         case 0:
             tp_udl = *p - ((*p)>>3);
             RILLOGD("gsm7bit : %d", tp_udl);
             break;
         case 4:
         case 8:
             tp_udl = *p;
             RILLOGD("8bit : %d", tp_udl);
             break;
         default :
             RILLOGD("DCS is error, this code might be never called");
    }
    sms_length = 15 + smsc_len + addr_length + tp_udl;

    /**************************************************************/
    /*  SCA  *  PDU  *  OA  *  PID  *  DCS  *  TS  *  UDL  *  UD  */
    /*  1~10 *   1   * 2~12 *   1   *   1   *   7  *   1   * 0~140*/
    /**************************************************************/
    return sms_length;
}

static void requestSendSSSMS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err, count = 0;
    const char *smsc;
    const char *pdu;
    int tpLayerLength;
    char *cmd1, *cmd2;
    RIL_SMS_Response response;
    ATResponse *p_response = NULL;
    char * line;
    int max_num, seq_num;
    char *pdu_bin;
    int ret;

    memset(&response, 0, sizeof(RIL_SMS_Response));
    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    pdu_bin = (char *)malloc(tpLayerLength);
    if(!convertHexToBin(pdu, strlen(pdu), pdu_bin)) {
        if(is_long_sms(pdu_bin, &max_num, &seq_num)) {
            RILLOGD("is a long sms, max_num = %d, seq_num = %d", max_num, seq_num);
            if(seq_num == 1) {
                at_send_command( ATch_type[channelID], "AT+CMMS=1", NULL);
            } else if(seq_num == max_num) {
                at_send_command( ATch_type[channelID], "AT+CMMS=0", NULL);
            }
        } else {
            RILLOGD("is not a long sms");
        }
    }
    free(pdu_bin);

    ret = asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd1 = NULL;
        goto error2;
    }
    ret = asprintf(&cmd2, "%s%s", smsc, pdu);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        free(cmd1);
        cmd2 = NULL;
        goto error2;
    }

retry:
    err = at_send_command_sms(ATch_type[channelID], cmd1, cmd2, "+CMGS:", &p_response);
    if (err != 0 || p_response->success == 0)
        goto error;

    /* FIXME fill in messageRef and ackPDU */

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if(err < 0)
        goto error1;
    err = at_tok_nextint(&line, &response.messageRef);
    if(err < 0)
        goto error1;

    free(cmd1);
    free(cmd2);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(RIL_SMS_Response));
    at_response_free(p_response);
    return;
error:
    line = p_response->finalResponse;
    err = at_tok_start(&line);
    if (err < 0)
        goto error1;
    err = at_tok_nextint(&line, &response.errorCode);
    if (err < 0)
        goto error1;
    if(response.errorCode == 98 || response.errorCode == 47
        || response.errorCode == 42 || response.errorCode == 41
        || response.errorCode == 27)
        if(count < 3) {
            count++;
            goto retry;
        }
error1:
    free(cmd1);
    free(cmd2);
error2:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    return;
}

static void requestSMSAcknowledge(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int ackSuccess;
    int err;

    ackSuccess = ((int *)data)[0];

    if (ackSuccess == 1) {
        err = at_send_command(ATch_type[channelID], "AT+CNMA=1", NULL);
    } else if (ackSuccess == 0)  {
        err = at_send_command(ATch_type[channelID], "AT+CNMA=2", NULL);
    } else {
        RILLOGE("unsupported arg to RIL_REQUEST_SMS_ACKNOWLEDGE\n");
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

static void requestResetRadio(int channelID, RIL_Token t)
{
    int onOff, err;
    RIL_RadioState currentState;
    ATResponse *p_response = NULL;

    err = at_send_command( ATch_type[channelID], "AT+CFUN=0",  &p_response);
    if (err < 0 || p_response->success == 0) goto error;

    setRadioState(channelID, RADIO_STATE_OFF);

    err = at_send_command( ATch_type[channelID], "AT+CFUN=1", &p_response);
    if (err < 0|| p_response->success == 0)
    {
        /* Some stacks return an error when there is no SIM, but they really turn the RF portion on
         * So, if we get an error, let's check to see if it turned on anyway
         */
        if (isRadioOn(channelID) != 1)  goto error;
    }

    setRadioState(channelID, RADIO_STATE_SIM_NOT_READY);

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

static void  requestSIM_IO(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    char *cmd = NULL;
    RIL_SIM_IO_v6 *p_args;
    char *line;
    char pad_data = '0';


    memset(&sr, 0, sizeof(sr));

    p_args = (RIL_SIM_IO_v6 *)data;

    /* FIXME handle pin2 */

    if(p_args->pin2 != NULL){

        RILLOGI("Reference-ril. requestSIM_IO pin2 %s",p_args->pin2);
    }
    if (p_args->data == NULL) {
        err = asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,%c,\"%s\"",
                p_args->command, p_args->fileid,
                p_args->p1, p_args->p2, p_args->p3,pad_data,p_args->path);

    } else {
        err = asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                p_args->command, p_args->fileid,
                p_args->p1, p_args->p2, p_args->p3, p_args->data,p_args->path);
    }
    if(err < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }

    err = at_send_command_singleline(ATch_type[channelID], cmd, "+CRSM:", &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw1));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw2));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(sr.simResponse));
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestGetIMEISV(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;
    char *line;
    int sv;
    char response[10];

    err = at_send_command_singleline(ATch_type[channelID], "AT+SGMR=0,0,2", "+SGMR:",&p_response);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &sv);
    if (err < 0) goto error;

    if(sv >= 0  && sv < 10)
        snprintf(response, sizeof(response), "0%d", sv);
    else
        snprintf(response, sizeof(response), "%d", sv);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void onQuerySignalStrength(void *param)
{
    int channelID;
    ATResponse *p_response = NULL;
    int err;
    RIL_SignalStrength_v6 response_v6;
    char *line;

    RILLOGE("query signal strength when screen on");
    channelID = getChannel();
    memset(&response_v6, -1, sizeof(response_v6));
    err = at_send_command_singleline(ATch_type[channelID], "AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response_v6.GW_SignalStrength.signalStrength));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response_v6.GW_SignalStrength.bitErrorRate));
    if (err < 0) goto error;

    RIL_onUnsolicitedResponse(
              RIL_UNSOL_SIGNAL_STRENGTH,
              &response_v6, sizeof(RIL_SignalStrength_v6));
    putChannel(channelID);
    at_response_free(p_response);
    return;

error:
    RILLOGE("onQuerySignalStrength fail");
    putChannel(channelID);
    at_response_free(p_response);
}

static void  requestScreeState(int channelID, int status, RIL_Token t)
{
    int err;
    int stat;

    pthread_mutex_lock(&s_screen_mutex);
    if (!status) {
        /* Suspend */
        at_send_command(ATch_type[channelID], "AT+CCED=2,8", NULL);
        at_send_command(ATch_type[channelID], "AT+CREG=1", NULL);
        at_send_command(ATch_type[channelID], "AT+CGREG=1", NULL);
        at_send_command(ATch_type[channelID], "AT*FDY=1,5", NULL);
    } else {
        /* Resume */
        at_send_command(ATch_type[channelID], "AT+CCED=1,8", NULL);
        at_send_command(ATch_type[channelID], "AT+CREG=2", NULL);
        at_send_command(ATch_type[channelID], "AT+CGREG=2", NULL);
        at_send_command(ATch_type[channelID], "AT*FDY=1,8", NULL);
        if (sState == RADIO_STATE_SIM_READY) {
            RIL_requestTimedCallback (onQuerySignalStrength, NULL, NULL);
        }
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                NULL, 0);
    }
    pthread_mutex_unlock(&s_screen_mutex);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

static void  requestVerifySimPin(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;
    char *cpinLine;
    char *cpinResult;
    int ret;
    char sim_prop[20];

    if ( datalen == 2*sizeof(char*) ) {
        ret = asprintf(&cmd, "AT+CPIN=%s", strings[0]);
    } else if ( datalen == 3*sizeof(char*) ) {
        err = at_send_command_singleline(ATch_type[channelID], "AT+CPIN?", "+CPIN:", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;

        cpinLine = p_response->p_intermediates->line;
        err = at_tok_start (&cpinLine);
        if(err < 0) goto error;
        err = at_tok_nextstr(&cpinLine, &cpinResult);
        if(err < 0) goto error;
        if ((0 == strcmp(cpinResult, "READY")) || (0 == strcmp(cpinResult, "SIM PIN"))) {
            ret = asprintf(&cmd, "ATD**05*%s*%s*%s#",strings[0],strings[1],strings[1]);
        } else {
            ret = asprintf(&cmd, "AT+CPIN=%s,%s", strings[0], strings[1]);
        }
        at_response_free(p_response);
    } else
        goto error;

    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    } else {
        /* add for modem reboot */
        const char *pin = NULL;
        extern int s_sim_num;
        if ( datalen == 2*sizeof(char*) ) {
            pin = strings[0];
        } else if ( datalen == 3*sizeof(char*) ) {
            pin = strings[1];
        } else
            goto out;

        if(s_multiSimMode) {
            if(s_sim_num == 0) {
                if(pin != NULL) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY);
                    }
                    property_set(sim_prop, pin);
                }
            } else if (s_sim_num == 1) {
                if(pin != NULL) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY1);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY1);
                    }
                    property_set(sim_prop, pin);
                }
            } else if (s_sim_num == 2) {
                if(pin != NULL) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY2);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY2);
                    }
                    property_set(sim_prop, pin);
                }
            }
        } else {
            if(pin != NULL) {
                if(!strcmp(s_modem, "t")) {
                    strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY);
                } else if(!strcmp(s_modem, "w")) {
                    strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY);
                }
                property_set(sim_prop, pin);
            }
        }
out:
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        if(getSIMStatus(channelID) == SIM_READY) {
            setRadioState(channelID, RADIO_STATE_SIM_READY);
        }
        at_response_free(p_response);
        return;
    }
error:
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    at_response_free(p_response);
}

static void  requestVerifySimPuk2(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse *p_response = NULL;
    int err;
    char *cmd = NULL;
    const char **strings = (const char**)data;
    int ret;

    if (datalen == 3*sizeof(char*) ) {
        ret = asprintf(&cmd, "ATD**052*%s*%s*%s#",strings[0],strings[1],strings[1]);
    } else
        goto error;

    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    if(getSIMStatus(channelID) == SIM_READY) {
        setRadioState(channelID, RADIO_STATE_SIM_READY);
    }
    return;
error:
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    at_response_free(p_response);
}


static void  requestEnterSimPin(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int err;
    char *cmd = NULL;
    const char **strings = (const char**)data;
    int ret;

    if ( datalen == 3*sizeof(char*) ) {
        ret = asprintf(&cmd, "AT+CPWD=\"SC\",\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    at_response_free(p_response);
}

static void  requestEnterSimPin2(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int err;
    char *cmd = NULL;
    const char **strings = (const char**)data;
    int ret;

    if ( datalen == 3*sizeof(char*) ) {
        ret = asprintf(&cmd, "AT+CPWD=\"P2\",\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    at_response_free(p_response);
}

static int setSmsBroadcastConfigValue(int value ,char * out_value)
{
    RILLOGI("Reference-ril. setSmsBroadcastConfigValue value %d",value);

    if(value == 0xffff){
        return 0;
    }else{
        RILLOGI("Reference-ril. setSmsBroadcastConfigValue value %d",value);
        sprintf(out_value,"%d",value);
    }
    RILLOGI("Reference-ril. setSmsBroadcastConfigValue out_value %s",out_value);

    return 1;
}

static void setSmsBroadcastConfigData(int data, int idx, int isFirst, char *toStr, int* strLength, char *retStr ){
    int len = 0;
    char get_char[10] ={0};
    char comma = 0x2c;
    char quotes = 0x22;

    memset(get_char,0,10);
    if(setSmsBroadcastConfigValue( data, get_char)>0){
        RILLOGI("Reference-ril. setSmsBroadcastConfigData  ");
        RILLOGI("Reference-ril. setSmsBroadcastConfigData (1) ");
        if(idx==0 && isFirst){
            retStr[len] = quotes;
            len += 1;
        }else{
            retStr[0] = comma;
            len += 1;
        }
        memcpy(retStr+len,get_char,strlen(get_char));
        len += strlen(get_char);
    }
    *strLength = len;
    RILLOGI("Reference-ril. setSmsBroadcastConfigData  ret_char %s , len %d",retStr , *strLength);
}

static void requestSetSmsBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse    *p_response = NULL;
    int           err;
    char*         cmd;
    int ret = -1;

    RIL_GSM_BroadcastSmsConfigInfo **gsmBciPtrs = ( RIL_GSM_BroadcastSmsConfigInfo* *)data;
    RIL_GSM_BroadcastSmsConfigInfo gsmBci;
    int enable = 0;
#if defined (RIL_SPRD_EXTENSION)
    int  i =0 ,j=0;
    char pre_colon = 0x22;
    int count = datalen/sizeof(RIL_GSM_BroadcastSmsConfigInfo*);
    int channelLen =0;
    int langLen = 0;
    int len = 0;
    char *channel;
    char *lang;
    char  get_char[10] ={0};
    char comma = 0x2c;
    char tmp[20] = {0};
    char quotes = 0x22;

    RILLOGD("Reference-ril. requestSetSmsBroadcastConfig %d ,count %d", datalen,count);

    channel = alloca(datalen*16);
    lang = alloca(datalen*16);
    memset(channel,0,datalen*16);
    memset(lang,0,datalen*16);

    for(i=0;i<count;i++){
        gsmBci =*(RIL_GSM_BroadcastSmsConfigInfo *)(gsmBciPtrs[i]);
        if(i == 0){
            enable = gsmBci.selected;
        }
        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.fromServiceId,i,1,channel,&len,tmp);
        memcpy(channel+channelLen,tmp,strlen(tmp));
        channelLen += len;
        RILLOGI("Reference-ril. requestSetSmsBroadcastConfig channel %s ,%d ",channel, channelLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.toServiceId,i,0,channel,&len,tmp);
        memcpy(channel+channelLen,tmp,strlen(tmp));
        channelLen += len;
        RILLOGI("Reference-ril. requestSetSmsBroadcastConfig channel %s ,%d",channel, channelLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.fromCodeScheme,i,1,lang,&len,tmp);
        memcpy(lang+langLen,tmp,strlen(tmp));
        langLen += len;
        RILLOGI("Reference-ril. requestSetSmsBroadcastConfig lang %s, %d",lang, langLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.toCodeScheme,i,0,lang,&len,tmp);
        memcpy(lang+langLen,tmp,strlen(tmp));
        langLen += len;
        RILLOGI("Reference-ril. requestSetSmsBroadcastConfig lang %s, %d",lang,langLen);
    }
    if(langLen == 0)
    {
        sprintf(lang,"%c",quotes);
    }
    if(channelLen == 1)
    {
        sprintf(channel+channelLen,"%c",quotes);
    }
    if(channelLen == 0) {
        sprintf(channel,"%c",quotes);
    }
    ret = asprintf(&cmd, "AT+CSCB=%d%c%s%c%c%s%c",enable,comma,channel,quotes,comma,lang,quotes);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    RILLOGI("Reference-ril. requestSetSmsBroadcastConfig cmd %s",cmd);

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    RILLOGI( "requestSetSmsBroadcastConfig err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    gsmBci =*(RIL_GSM_BroadcastSmsConfigInfo *)(gsmBciPtrs[0]);
    enable = gsmBci.selected;
    RILLOGD("requestSetSmsBroadcastConfig enable = %d", enable);
    RILLOGD("requestSetSmsBroadcastConfig fromServiceId = %d", gsmBci.fromServiceId);
    RILLOGD("requestSetSmsBroadcastConfig toServiceId = %d", gsmBci.toServiceId);
    if (gsmBci.fromServiceId == 0 && gsmBci.toServiceId == 999) {
        ret = asprintf(&cmd, "AT+CSCB=%d,\"1000\",\"\"", !enable);
    }
    if (gsmBci.fromServiceId == gsmBci.toServiceId) {
        ret = asprintf(&cmd, "AT+CSCB=%d,\"%d\",\"\"", !enable, gsmBci.fromServiceId);
    }
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    RILLOGI("requestSetSmsBroadcastConfig cmd %s",cmd);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    RILLOGI( "requestSetSmsBroadcastConfig err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
#endif
}

static void requestGetSmsBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char* response;

    err = at_send_command_singleline(ATch_type[channelID], "AT+CSCB=?", "+CSCB:", &p_response);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        response = p_response->p_intermediates->line;
        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, strlen(response) + 1);
    }
    at_response_free(p_response);
}

static void requestSmsBroadcastActivation(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char cmd[20] = {0};
    int   *active = (int*)data;

    RILLOGI("Reference-ril."
            " datalen: %d ,active %d\n", datalen,active[0]);
#if defined (RIL_SPRD_EXTENSION)
    snprintf(cmd, sizeof(cmd), "AT+CSCB=%d", active[0]);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    snprintf(cmd, sizeof(cmd), "AT+CSCB=%d", !active[0]);
#endif

    err = at_send_command(ATch_type[channelID], cmd,&p_response);
    RILLOGI( "requestSmsBroadcastActivation err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
error:
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

static void convertBinToHex(char *bin_ptr, int length, char *hex_ptr)
{
    int i;
    unsigned char tmp;

    if (bin_ptr == NULL || hex_ptr == NULL) {
        return;
    }
    for (i=0; i<length; i++) {
        tmp = (unsigned char)((bin_ptr[i] & 0xf0)>>4);
        if (tmp <= 9) {
            *hex_ptr = (unsigned char)(tmp + '0');
        } else {
            *hex_ptr = (unsigned char)(tmp + 'A' - 10);
        }
        hex_ptr++;
        tmp = (unsigned char)(bin_ptr[i] & 0x0f);
        if (tmp <= 9) {
            *hex_ptr = (unsigned char)(tmp + '0');
        } else {
            *hex_ptr = (unsigned char)(tmp + 'A' - 10);
        }
        hex_ptr++;
    }
}

static int convertHexToBin(const char *hex_ptr, int length, char *bin_ptr)
{
    char *dest_ptr = bin_ptr;
    int i;
    char ch;

    if (hex_ptr == NULL || bin_ptr == NULL) {
        return -1;
    }

    for(i = 0; i < length; i += 2) {
        ch = hex_ptr[i];
        if(ch >= '0' && ch <= '9')
            *dest_ptr = (char)((ch - '0') << 4);
        else if(ch >= 'a' && ch <= 'f')
            *dest_ptr = (char)((ch - 'a' + 10) << 4);
        else if(ch >= 'A' && ch <= 'F')
            *dest_ptr = (char)((ch - 'A' + 10) << 4);
        else
            return -1;

        ch = hex_ptr[i+1];
        if(ch >= '0' && ch <= '9')
            *dest_ptr |= (char)(ch - '0');
        else if(ch >= 'a' && ch <= 'f')
            *dest_ptr |= (char)(ch - 'a' + 10);
        else if(ch >= 'A' && ch <= 'F')
            *dest_ptr |= (char)(ch - 'A' + 10);
        else
            return -1;

        dest_ptr++;
    }
    return 0;
}

static void requestSendUSSD(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse  *p_response = NULL;
    char *ussdHexRequest;
    int err;
    char *cmd;
    int len;
    int ret;

    ussdRun = 1;
    ussdHexRequest = (char *)(data);
    ret = asprintf(&cmd, "AT+CUSD=1,\"%s\",15", ussdHexRequest);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        ussdRun = 0;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

static void requestVideoPhoneDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_VideoPhone_Dial *p_dial;
    ATResponse   *p_response = NULL;
    int err;
    char *cmd;
    int ret;

    p_dial = (RIL_VideoPhone_Dial *)data;

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

/* release Channel */
void putChannel(int channel)
{
    struct channel_description *descriptions;

    if(s_multiSimMode)
        descriptions = multi_descriptions;
    else
        descriptions = single_descriptions;

    if(s_multiSimMode) {
        if(channel < 1 || channel >= MULTI_MAX_CHANNELS )
            return;
    } else {
        if(channel < 1 || channel >= MAX_CHANNELS)
            return;
    }

    pthread_mutex_lock(&descriptions[channel].mutex);
    if(descriptions[channel].state != CHANNEL_BUSY)
    {
        goto done1;
    }
    descriptions[channel].state = CHANNEL_IDLE;
done1:
    RILLOGD("put Channel ID '%d'", descriptions[channel].channelID);
    pthread_mutex_unlock(&descriptions[channel].mutex);
}

/* Return channel ID */
int getChannel()
{
    int ret=0;
    int channel;
    struct channel_description *descriptions;
    int channel_num;

    if(s_multiSimMode) {
        descriptions = multi_descriptions;
        channel_num = MULTI_MAX_CHANNELS;
    } else {
        descriptions = single_descriptions;
        channel_num = MAX_CHANNELS;
    }

    if(s_multiSimMode)
        pthread_mutex_lock(&s_channel_mutex);
    for (;;) {
        if(!s_channel_open) {
            sleep(1);
            continue;
        }
        for (channel = 1; channel < channel_num; channel++) {
            pthread_mutex_lock(&descriptions[channel].mutex);
            if(descriptions[channel].state == CHANNEL_IDLE) {
                RILLOGD("channel%d state: '%d' \n",
                    descriptions[channel].channelID, descriptions[channel].state);
                descriptions[channel].state = CHANNEL_BUSY;
                RILLOGD("get Channel ID '%d'", descriptions[channel].channelID);
                pthread_mutex_unlock(&descriptions[channel].mutex);
                if(s_multiSimMode)
                    pthread_mutex_unlock(&s_channel_mutex);
                return channel;
            }
            pthread_mutex_unlock(&descriptions[channel].mutex);
        }
        usleep(5000);
    }
    if(s_multiSimMode)
        pthread_mutex_unlock(&s_channel_mutex);
    return ret;
}

static int getSmsChannel()
{
    int ret = 0;
    struct channel_description *descriptions;
    int channel_num;

    if(s_multiSimMode) {
        descriptions = multi_descriptions;
        channel_num = 2;
    } else {
        descriptions = single_descriptions;
        channel_num = 3;
    }

    for (;;) {
        if(!s_channel_open) {
            sleep(1);
            continue;
        }
        pthread_mutex_lock(&descriptions[channel_num].mutex);
        if(descriptions[channel_num].state == CHANNEL_IDLE) {
            RILLOGD("channel%d state: '%d' \n",
                descriptions[channel_num].channelID, descriptions[channel_num].state);
            descriptions[channel_num].state = CHANNEL_BUSY;
            RILLOGD("get Channel ID '%d'", descriptions[channel_num].channelID);
            pthread_mutex_unlock(&descriptions[channel_num].mutex);
            return channel_num;
        }
        pthread_mutex_unlock(&descriptions[channel_num].mutex);
        usleep(5000);
    }

    return ret;
}

static RIL_AppType getSimType(int channelID)
{
           int err;
           ATResponse *p_response = NULL;
           char *line;
            RIL_AppType ret = RIL_APPTYPE_UNKNOWN ;
            int skip, card_type;

            err = at_send_command_singleline(ATch_type[channelID], "AT+EUICC?", "+EUICC:",
                                             &p_response);
            if (err < 0 || p_response->success == 0)
                goto error;

            line = p_response->p_intermediates->line;
            RILLOGD("getSimType: %s", line);
            err = at_tok_start(&line);
            if(err < 0) goto error;
            err = at_tok_nextint(&line, &skip);
            if(err < 0) goto error;
            err = at_tok_nextint(&line, &skip);
            if(err < 0) goto error;
            err = at_tok_nextint(&line, &card_type);
            if(err < 0) goto error;
            if(card_type == 1)
                ret = RIL_APPTYPE_USIM;
            else if(card_type == 0)
                ret = RIL_APPTYPE_SIM;
            else
                ret = RIL_APPTYPE_UNKNOWN;

            at_response_free(p_response);
            return ret;
error:
            at_response_free(p_response);
            return RIL_APPTYPE_UNKNOWN;
}


static void getSmsState(int channelID)
{
    ATResponse *p_response = NULL;
    int err, err_num;
    char * line;

    err = at_send_command_singleline(ATch_type[channelID], "AT+CSCA?", "+CSCA:", &p_response);
    if (err != 0 || p_response->success == 0) {
        line = p_response->finalResponse;
        err = at_tok_start(&line);
        if (err < 0)
            goto error;
        err = at_tok_nextint(&line, &err_num);
        if (err < 0)
            goto error;
        if(err_num != 313) {
            goto out;
        } else
            goto error;
    }
out:
    pthread_mutex_lock(&s_sms_ready_mutex);
    if(s_sms_ready == 0) {
        s_sms_ready = 1;
#if defined (RIL_SPRD_EXTENSION)
            RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_SMS_READY, NULL, 0);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            RIL_onUnsolicitedResponse (RIL_UNSOL_DEVICE_READY_NOTI, NULL, 0);
#endif
    }
    pthread_mutex_unlock(&s_sms_ready_mutex);
error:
    at_response_free(p_response);
}

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static char* strReplace(char **str, char flag)
{
    char **tmp = NULL;
    char **src = NULL;
    char *ret = NULL;
    int count = 0;
    char buf[512] = {0};

    if (*str == NULL)
        return NULL;

    tmp = str;
    if (flag == ',') {
        if (!strchr(*str, ',') && !strchr(*str, ';')) {
            return *str;
        }
        while(**tmp != '\0') {
            if (**tmp == ',') {
                buf[count++] = 'P';
                (*tmp)++;
                continue;
            }
            if (**tmp == ';') {
               buf[count++] = 'W';
               (*tmp)++;
               continue;
            }
            buf[count++] = **tmp;
            (*tmp)++;
        }
        buf[count] = '\0';
    } else if (flag == 'P') {
        if (!strchr(*str, 'P') && !strchr(*str, 'W')) {
            return *str;
        }
        while(**tmp != '\0') {
            if (**tmp == 'P') {
                buf[count++] = ',';
                (*tmp)++;
                continue;
            }
            if (**tmp == 'W') {
               buf[count++] = ';';
               (*tmp)++;
               continue;
            }
            buf[count++] = **tmp;
            (*tmp)++;
        }
        buf[count] = '\0';
    }
    memcpy(*str, buf, strlen(buf)+1);
    ret = *str;
    return ret;
}

static void requestSetCellBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{

    ATResponse        *p_response = NULL;
    int                err;
    char              *cmd;
    RIL_CB_ConfigArgs *cbDataPtr = (RIL_CB_ConfigArgs *)data;
    int ret;

    RILLOGD("Reference-ril. requestSetCellBroadcastConfig selectedId = %d", cbDataPtr->selectedId);
    if (cbDataPtr->selectedId == 1) { // Configure all IDs
        ret = asprintf(&cmd, "AT+CSCB=%d,\"1000\",\"\"", !(cbDataPtr->bCBEnabled));
    } else if (cbDataPtr->selectedId == 2) { // Configure special IDs
        ret = asprintf(&cmd, "AT+CSCB=%d,\"%s\",\"\"",
                 !(cbDataPtr->bCBEnabled), cbDataPtr->msgIDs);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    RILLOGI("Reference-ril. requestSetSmsBroadcastConfig cmd %s",cmd);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    RILLOGI( "requestSetCellBroadcastConfig err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    free(cmd);
    at_response_free(p_response);
}

static void requestGetCellBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int         err;
    char       *response;
    char *line = NULL;
    int   result = 0;
    RIL_CB_ConfigArgs  cbsPtr;
    int i, commas = 0;
    char *p, *str;
    char tmp[5];

    RILLOGD("Reference-ril. requestGetCellBroadcastConfig enter");
    memset(&cbsPtr, 0, sizeof(RIL_CB_ConfigArgs));
    err = at_send_command_singleline(ATch_type[channelID], "AT+CSCB?", "+CSCB:", &p_response);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        line = p_response->p_intermediates->line;
        RILLOGD("requestGetCellBroadcastConfig: err=%d line=%s", err, line);
        err = at_tok_start(&line);
        if (err == 0) err = at_tok_nextint(&line, &result);
        if (err == 0) {
            err = at_tok_nextstr(&line, &response);
            if (err == 0) {
                cbsPtr.bCBEnabled = result;
                cbsPtr.msgIdMaxCount = 1000;
                if (0 == strcmp(response, "1000")) {
                    cbsPtr.selectedId = 1;
                } else {
                    if(*response == '\0') {
                        str = NULL;
                        cbsPtr.msgIdCount = 0;
                    } else {
                        for (p = response; *p != '\0' ; p++) {
                            if (*p == ',') commas++;
                        }
                        str = (char*)alloca(4*(commas + 1) + 1);
                        for(i = 0; i < (commas + 1); i++) {
                            at_tok_nextint(&response, &result);
                            if(i == 0) {
                                sprintf(str, "%04x", result);
                            } else {
                                sprintf(tmp, "%04x", result);
                                strcat(str, tmp);
                            }
                        }
                        cbsPtr.msgIdCount = commas + 1;
                    }
                    cbsPtr.selectedId = 2;
                    cbsPtr.msgIDs = str;
                }
                RIL_onRequestComplete(t, RIL_E_SUCCESS, &cbsPtr, sizeof(RIL_CB_ConfigArgs));
            }
        } else {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        }
    }
    at_response_free(p_response);
}

static void requestSendEncodedUSSD(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse  *p_response = NULL;
    int err;
    char *cmd;
    RIL_EncodedUSSD *p_ussd = (RIL_EncodedUSSD *)data;
    int ret;

    RILLOGD("Reference-ril. requestSendEncodedUSSD enter");

    ussdRun = 1;
    ret = asprintf(&cmd, "AT+CUSD=1,\"%s\",%d", p_ussd->encodedUSSD, p_ussd->dcsCode);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (err < 0 || p_response->success == 0) {
        ussdRun = 0;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    free(cmd);
    at_response_free(p_response);
}

static void requestGetPhonebookStorageInfo(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char cmd[20] = {0};
    char *line;
    int response[5] = {0};
    char* storage_info[4] = {"SM","SN","FD","ON"};//ADN:SM,SDN:SN,FDN:FD,MSISDN:ON
    int fileid = ((int *)data)[0];
    int fileidIndex = 0;

    switch(fileid)
    {
        case 0x6F3A : //EF_ADN
            fileidIndex = 0;
        break;

        case 0x6F49 : //EF_SDN
            fileidIndex = 1;
        break;

        case 0x6F3B : //EF_FDN
            fileidIndex = 2;
        break;

        case 0x6F40 : //EF_MSISDN
            fileidIndex = 3;
        break;

        default:
            RILLOGD("requestGetPhonebookStorageInfo invalid fileid");
        break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CPBS=\"%s\"",storage_info[fileidIndex]);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    err = at_send_command_singleline(ATch_type[channelID], "AT+CPBS?", "+CPBS:", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

/*    response[0] - Total slot count.
                 The number of the total phonebook memory.
                 according to Phone book storage.
   response[1] - Used slot count.
                 The number of the used phonebook memory.
   response[2] - First index.
   response[3] - Maximum byte of phonebook entry text field.
   resopnse[4] - Maximum byte of phonebook entry number field.
*/

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[0]);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[1]);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[2]);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[3]);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[4]);
    if (err < 0) goto error;

    RILLOGD("CONTACT+requestGetPhonebookStorageInfo");
    RILLOGD("Total = %d", response[0]);
    RILLOGD("Used slot count = %d", response[1]);
    RILLOGD("First index = %d", response[2]);
    RILLOGD("Max Length String = %d", response[3]);
    RILLOGD("Max Length Num = %d", response[4]);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestGetPhonebookEntry(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    int count;
    int i;
    char cmd[30] = {0};
    char *line;
    RIL_SIM_PB *p_args;
    RIL_SIM_PB_Response *pbResponse;
    int response[2] = {0};
    char* storage_info[4] = {"SM","SN","FD","ON"};//ADN:SM,SDN:SN,FDN:FD,MSISDN:ON
    int fileidIndex = 0;												 // CONTACT_Init_JEM #2 

    p_args = (RIL_SIM_PB*)data;

    switch(p_args->fileid)
    {
        case 0x6F3A : //EF_ADN
            fileidIndex = 0;
        break;

        case 0x6F49 : //EF_SDN
            fileidIndex = 1;
        break;

        case 0x6F3B : //EF_FDN
            fileidIndex = 2;
        break;

        case 0x6F40 : //EF_MSISDN
            fileidIndex = 3;
        break;

        default:
            RILLOGD("requestGetPhonebookStorageInfo invalid fileid");
        break;
    }

    pbResponse = (RIL_SIM_PB_Response*)alloca(sizeof(RIL_SIM_PB_Response));
    memset(pbResponse, 0, sizeof(RIL_SIM_PB_Response));

    snprintf(cmd, sizeof(cmd), "AT+CPBS=\"%s\"",storage_info[fileidIndex]);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    snprintf(cmd, sizeof(cmd), "AT^SCPBR=%d",p_args->index);
    err = at_send_command_singleline(ATch_type[channelID], cmd, "^SCPBR:", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[0]);
    if (err < 0) goto error;
    pbResponse->recordIndex = response[0];

    err = at_tok_nextint(&line, &response[1]);
    if (err < 0) goto error;
    pbResponse->nextIndex = response[1];

    for (count=0; count<NUM_OF_NUMBER; count++) {
        if (at_tok_hasmore(&line)) {
            err = at_tok_nextstr(&line, &pbResponse->numbers[count]);
            if (err < 0) goto error;
            if (strReplace(&pbResponse->numbers[count],'P') == NULL)
                goto error;
            err = at_tok_nextint(&line, &pbResponse->dataTypeNumbers[count]);
            if (err < 0) goto error;
            pbResponse->lengthNumbers[count] = strlen(pbResponse->numbers[count]);
        } else {
            for (i=0; i<2; i++) {
                skipNextComma(&line);
            }
        }
    }

    for (count=0; count<NUM_OF_ALPHA; count++) {
        if (at_tok_hasmore(&line)) {
            err = at_tok_nextstr(&line, &pbResponse->alphaTags[count]);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &pbResponse->dataTypeAlphas[count]);
            if (err < 0) goto error;
            pbResponse->lengthAlphas[count] = strlen(pbResponse->alphaTags[count]);
        } else {
            for (i=0; i<2; i++) {
                skipNextComma(&line);
            }
        }
    }

    RILLOGD("CONTACT+requestGetPhonebookEntry");
    RILLOGD("Index = %d", pbResponse->recordIndex);
    RILLOGD("Next Index = %d", pbResponse->nextIndex);
    RILLOGD("alphaTags = %s", pbResponse->alphaTags[0]);
    RILLOGD("dataTypeAlphas = %d", pbResponse->dataTypeAlphas[0]);
    RILLOGD("lengthAlphas = %d", pbResponse->lengthAlphas[0]);
    RILLOGD("numbers = %s", pbResponse->numbers[0]);
    RILLOGD("dataTypeNumbers = %d", pbResponse->dataTypeNumbers[0]);
    RILLOGD("lengthNumbers = %d", pbResponse->lengthNumbers[0]);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pbResponse, sizeof(RIL_SIM_PB_Response));
    at_response_free(p_response);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestAccessPhonebookEntry(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *cmd;
    char *line;
    RIL_SIM_ACCESS_PB *p_args;
    int operindex = 0;
    char* storage_info[4] = {"SM","SN","FD","ON"};//ADN:SM,SDN:SN,FDN:FD,MSISDN:ON
    int fileidIndex = 0;
    int ret;

    p_args = (RIL_SIM_ACCESS_PB*)data;

    switch(p_args->fileid)
    {
        case 0x6F3A : //EF_ADN
            fileidIndex = 0;
        break;

        case 0x6F49 : //EF_SDN
            fileidIndex = 1;
        break;

        case 0x6F3B : //EF_FDN
            fileidIndex = 2;
        break;

        case 0x6F40 : //EF_MSISDN
            fileidIndex = 3;
        break;

        default:
            RILLOGD("requestGetPhonebookStorageInfo invalid fileid");
        break;
    }

    ret = asprintf(&cmd, "AT+CPBS=\"%s\"",storage_info[fileidIndex]);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    ret = asprintf(&cmd, "AT^SCPBW=%d,\"%s\",%d,\"%s\",%d,\"%s\",%d,\"%s\",%d,\
                   \"%s\",%d,\"%s\",%d,\"%s\",%d,\"%s\",%d",
                   p_args->index,
                   p_args->number? strReplace(&p_args->number,',') : "",
                   p_args->number? 0 : 0x20,
                   p_args->anr? strReplace(&p_args->anr,',') : "",
                   p_args->anr? 0 : 0x20,
                   p_args->anrA? strReplace(&p_args->anrA,',') : "",
                   p_args->anrA? 0 : 0x20,
                   p_args->anrB? strReplace(&p_args->anrB,',') : "",
                   p_args->anrB? 0 : 0x20,
                   p_args->anrC? strReplace(&p_args->anrC,',') : "",
                   p_args->anrC? 0 : 0x20,
                   p_args->alphaTag? p_args->alphaTag : "",
                   p_args->alphaTag? p_args->alphaTagDCS : 0x20,
                   p_args->email? p_args->email : "",
                   p_args->email? 0 : 0x20,
                   p_args->sne? p_args->sne : "",
                   p_args->sne? p_args->sneDCS : 0x20);
    if(ret < 0) {
        RILLOGE("Failed to allocate memory");
        cmd = NULL;
        goto error;
    }
    err = at_send_command_singleline(ATch_type[channelID], cmd, "^SCPBW:", &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &operindex);
    if (err < 0) goto error;

    RILLOGD("CONTACT+requestAccessPhonebookEntry");
    RILLOGD("Index = %d", operindex);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &operindex, sizeof(operindex));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestUsimPbCapa(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_Usim_PB_Capa *pbCapa;
    int err;
    int dataLen;
    int fileId;
    char cmd[30] = {0};
    char *line;
    int response[MAX_DATA_LEN] = {0};

    pbCapa = (RIL_Usim_PB_Capa *)alloca(sizeof(RIL_Usim_PB_Capa));
    memset(pbCapa, 0, sizeof(RIL_Usim_PB_Capa));
    for (fileId=0; fileId<MAX_3GPP_TYPE; fileId++) {
        memset(response, 0, sizeof(response));
        snprintf(cmd, sizeof(cmd), "AT+CPBCAPA=%d", fileId+1);
        err = at_send_command_singleline(ATch_type[channelID], cmd, "+CPBCAPA:", &p_response);
        if (err < 0 || p_response->success == 0) {
            goto error;
        }

        line = p_response->p_intermediates->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        for (dataLen=0; dataLen<MAX_DATA_LEN; dataLen++) {
            err = at_tok_nextint(&line, &response[dataLen]);
            if (err < 0) goto error;
            pbCapa->response[fileId][dataLen] = response[dataLen];
        }
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, pbCapa, sizeof(RIL_Usim_PB_Capa));
    at_response_free(p_response);
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

#endif

static void requestSendAT(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char *at_cmd = (char *)data;
    int i, err;
    ATResponse *p_response = NULL;
    char buf[1024] = {0};
    ATLine *p_cur = NULL;

    if(at_cmd == NULL) {
        RILLOGE("Invalid AT command");
        return;
    }

    err = at_send_command_multiline(ATch_type[channelID], at_cmd, "", &p_response);
    if (err < 0 || p_response->success == 0) {
        strlcat(buf, p_response->finalResponse, sizeof(buf));
        strlcat(buf, "\r\n", sizeof(buf));
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, buf, strlen(buf)+1);
    } else {
        p_cur = p_response->p_intermediates;
        for (i=0; p_cur != NULL; p_cur = p_cur->p_next,i++) {
            strlcat(buf, p_cur->line, sizeof(buf));
            strlcat(buf, "\r\n", sizeof(buf));
        }
        strlcat(buf, p_response->finalResponse, sizeof(buf));
        strlcat(buf, "\r\n", sizeof(buf));
        RIL_onRequestComplete(t, RIL_E_SUCCESS, buf, strlen(buf)+1);
    }
    at_response_free(p_response);
}

/*** Callback methods from the RIL library to us ***/

/**
 * Call from RIL to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void
onRequest (int request, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response;
    int err;
    int channelID = -1;

    RILLOGD("onRequest: %s sState=%d", requestToString(request), sState);

#if defined (RIL_SPRD_EXTENSION)
    channelID = getChannel();
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(request != RIL_REQUEST_OEM_HOOK_RAW)
        channelID = getChannel();
#endif

    /* Ignore all requests except RIL_REQUEST_GET_SIM_STATUS
     * when RADIO_STATE_UNAVAILABLE.
     */
    if (sState == RADIO_STATE_UNAVAILABLE
            && !(request == RIL_REQUEST_GET_SIM_STATUS
                || request == RIL_REQUEST_GET_IMEI
                || request == RIL_REQUEST_GET_IMEISV
                || (request == RIL_REQUEST_DIAL && s_isstkcall))
       ) {
        RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        putChannel(channelID);
        return;
    }

    /* Ignore all non-power requests when RADIO_STATE_OFF
     * (except RIL_REQUEST_GET_SIM_STATUS)
     */
    if (sState == RADIO_STATE_OFF
            && !(request == RIL_REQUEST_RADIO_POWER
#if defined (RIL_SPRD_EXTENSION)
                || request == RIL_REQUEST_SIM_POWER
                || request == RIL_REQUEST_GET_REMAIN_TIMES
#endif
                || request == RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
                || request == RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
                || request == RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
                || request == RIL_REQUEST_GET_SIM_STATUS
                || request == RIL_REQUEST_SIM_IO
                || request == RIL_REQUEST_SET_SMSC_ADDRESS
                || request == RIL_REQUEST_GET_SMSC_ADDRESS
                || request == RIL_REQUEST_BASEBAND_VERSION
                || request == RIL_REQUEST_ENTER_SIM_PIN
                || request == RIL_REQUEST_GET_IMEI
                || request == RIL_REQUEST_GET_IMEISV
                || request == RIL_REQUEST_SCREEN_STATE
                || request == RIL_REQUEST_SEND_AT
                || request == RIL_REQUEST_SET_SPEED_MODE
                || request == RIL_REQUEST_DELETE_SMS_ON_SIM
                || request == RIL_REQUEST_GET_IMSI
                || request == RIL_REQUEST_QUERY_FACILITY_LOCK
                || request == RIL_REQUEST_SET_FACILITY_LOCK
                || request == RIL_REQUEST_ENTER_SIM_PIN2
                || request == RIL_REQUEST_ENTER_SIM_PUK
                || request == RIL_REQUEST_ENTER_SIM_PUK2
                || request == RIL_REQUEST_CHANGE_SIM_PIN
                || request == RIL_REQUEST_CHANGE_SIM_PIN2
                || (request == RIL_REQUEST_DIAL && s_isstkcall))
       ) {
        RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        putChannel(channelID);
        return;
    }

    switch (request) {
        case RIL_REQUEST_BASEBAND_VERSION:
            requestBasebandVersion(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_QUERY_FACILITY_LOCK:
            {
                char *lockData[4];
                lockData[0] = ((char **)data)[0];
                lockData[1] = FACILITY_LOCK_REQUEST;
                lockData[2] = ((char **)data)[1];
                lockData[3] = ((char **)data)[2];
                requestFacilityLock(channelID, lockData, datalen + sizeof(char *), t);
                break;
            }
        case RIL_REQUEST_SET_FACILITY_LOCK:
            requestFacilityLock(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_CHANGE_BARRING_PASSWORD:
            requestChangeFacilityLock(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:
        {
            requestCallForward(channelID, data, datalen, t);
            break;
        }

        case RIL_REQUEST_SET_CALL_FORWARD:
            requestCallForward(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_GET_SIM_STATUS:
            {
                RIL_CardStatus_v6 *p_card_status;
                char *p_buffer;
                int buffer_size;

                sem_wait(&w_sem);
                int result = getCardStatus(channelID, &p_card_status);
                sem_post(&w_sem);
                if (result == RIL_E_SUCCESS) {
                    p_buffer = (char *)p_card_status;
                    buffer_size = sizeof(*p_card_status);
                } else {
                    p_buffer = NULL;
                    buffer_size = 0;
                }
                RIL_onRequestComplete(t, result, p_buffer, buffer_size);
                freeCardStatus(p_card_status);
                break;
            }
        case RIL_REQUEST_GET_CURRENT_CALLS:
            requestGetCurrentCalls(channelID, data, datalen, t, 0);
            break;
        case RIL_REQUEST_DIAL:
            requestDial(channelID, data, datalen, t);
            break;
#if defined (RIL_SPRD_EXTENSION) || (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_REQUEST_DIAL_EMERGENCY_CALL:
            requestEccDial(channelID, data, datalen, t);
            break;
#endif
        case RIL_REQUEST_HANGUP:
            requestHangup(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
            /* 3GPP 22.030 6.5.5
             * "Releases all held calls or sets User Determined User Busy
             *  (UDUB) for a waiting call."
	     */
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CHLD=0", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
            /* 3GPP 22.030 6.5.5
             * "Releases all active calls (if any exist) and accepts
             *  the other (held or waiting) call."
	     */
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CHLD=1", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
            /* 3GPP 22.030 6.5.5
             * "Places all active calls (if any exist) on hold and accepts
             *  the other (held or waiting) call."
	     */
            if (s_isstkcall == 1) {
                RILLOGD("Is stk call, Don't send chld=2");
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                break;
            }
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CHLD=2", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        case RIL_REQUEST_ANSWER:
            p_response = NULL;
//          wait4android_audio_ready("ATA");
            err = at_send_command(ATch_type[channelID], "ATA", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            break;
        case RIL_REQUEST_CONFERENCE:
            /* 3GPP 22.030 6.5.5
             * "Adds a held call to the conversation"
	     */
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CHLD=3", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            break;
        case RIL_REQUEST_UDUB:
            /* user determined user busy */
            /* sometimes used: ATH */
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CHLD=0", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            break;

        case RIL_REQUEST_SEPARATE_CONNECTION:
            {
                char  cmd[20] = {0};
                int   party = ((int*)data)[0];
                p_response = NULL;

                /* Make sure that party is in a valid range.
                 * (Note: The Telephony middle layer imposes a range of 1 to 7.
                 * It's sufficient for us to just make sure it's single digit.)
		 */
                if (party > 0 && party < 10) {
                    snprintf(cmd, sizeof(cmd), "AT+CHLD=2%d", party);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                    at_response_free(p_response);
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            }
            break;

        case RIL_REQUEST_SIGNAL_STRENGTH:
#if defined (RIL_SPRD_EXTENSION)
            requestSignalStrength(channelID, data, datalen, t);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            requestSsSignalStrength(channelID, data, datalen, t);
#endif
            break;
        case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS:
	    requestNeighboaringCellIds(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_VOICE_REGISTRATION_STATE:
        case RIL_REQUEST_DATA_REGISTRATION_STATE:
            requestRegistrationState(channelID, request, data, datalen, t);
            break;
        case RIL_REQUEST_OPERATOR:
            requestOperator(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_RADIO_POWER:
            requestRadioPower(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_DTMF_START:
            {
                char c = ((char *)data)[0];
                char cmd[30] = {0};
                struct listnode *cmd_item = NULL;

                cmd_item = (struct listnode* )malloc(sizeof(struct listnode));
                if(cmd_item == NULL) {
                    RILLOGE("Allocate dtmf cmd_item failed");
                    exit(-1);
                }
                cmd_item->data = ((char *)data)[0];
                list_add_tail(&dtmf_char_list, cmd_item);

                sprintf(cmd, "AT+SDTMF=1,\"%c\",0", (int)c);
                err = at_send_command(ATch_type[channelID], cmd, NULL);
                if (err < 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                sprintf(cmd, "AT+EVTS=1,%c", (int)c);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_DTMF:
            {
                char c = ((char *)data)[0];
                char cmd[20] = {0};

                snprintf(cmd, sizeof(cmd), "AT+VTS=%c", (int)c);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_DTMF_STOP:
            {
                char cmd[20] = {0};
                struct listnode *cmd_item = NULL;
                char c;

                cmd_item = (&dtmf_char_list)->next;
                if(cmd_item != (&dtmf_char_list)) {
                    c = cmd_item->data;
                    err = at_send_command(ATch_type[channelID], "AT+SDTMF=0", NULL);
                    if (err < 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                    snprintf(cmd, sizeof(cmd), "AT+EVTS=0,%c", (int)c);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                    at_response_free(p_response);
                    list_remove(cmd_item);
                    free(cmd_item);
                    break;
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    break;
	        }
            }
        case RIL_REQUEST_SEND_SMS:
#if defined (RIL_SPRD_EXTENSION)
            requestSendSMS(channelID, data, datalen, t);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            requestSendSSSMS(channelID, data, datalen, t);
#endif
            break;
        case RIL_REQUEST_SETUP_DATA_CALL:
            requestSetupDataCall(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_DEACTIVATE_DATA_CALL:
            deactivateDataConnection(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE:
            requestLastDataFailCause(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_LAST_CALL_FAIL_CAUSE:
            requestLastCallFailCause(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_SMS_ACKNOWLEDGE:
            requestSMSAcknowledge(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_GET_IMSI:
            p_response = NULL;
            err = at_send_command_numeric(ATch_type[channelID], "AT+CIMI", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                        p_response->p_intermediates->line, strlen(p_response->p_intermediates->line)+1);
            }
            at_response_free(p_response);
            getSmsState(channelID);
            break;

        case RIL_REQUEST_GET_IMEI:
            p_response = NULL;
            err = at_send_command_numeric(ATch_type[channelID], "AT+CGSN", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                        p_response->p_intermediates->line, strlen(p_response->p_intermediates->line)+1);
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_GET_IMEISV:
            requestGetIMEISV(channelID, data,datalen,t);
            break;

        case RIL_REQUEST_SIM_IO:
            requestSIM_IO(channelID, data,datalen,t);
            break;

        case RIL_REQUEST_SEND_USSD:
            requestSendUSSD(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_CANCEL_USSD:
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "AT+CUSD=2", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:
            requestNetworkList(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
            {
                char cmd[30] = {0};

                p_response = NULL;
                /* AT^SYSCONFIG=<mode>,<acqorder>,<roam>,<srvdomain>
                 * mode: 2:Auto 13:GSM ONLY 14:WCDMA_ONLY 15:TDSCDMA ONLY
                 * acqorder: 3 -- no change
                 * roam: 2 -- no change
                 * srvdomain: 4 -- no change
		 */
                RIL_onUnsolicitedResponse (
                        RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                        NULL, 0);
                // transfer rilconstan to at
                int type = 0;
                switch(((int *)data)[0]) {
                case 0: //NETWORK_MODE_WCDMA_PREF
                    type = 2;
                    break;
                case 1://NETWORK_MODE_GSM_ONLY
                    type = 13;
                    break;
                case 2://NETWORK_MODE_WCDMA_ONLY or TDSCDMA ONLY
                    if(!strcmp(s_modem, "t")) {
                        type = 15;
                    } else {
                        type = 14;
                    }
                    break;
                }
                if (0 == type) {
                    RILLOGE("set prefeered network failed, type incorrect: %d", ((int *)data)[0]);
                    break;
                }
                if(s_multiSimMode) {
                    char prop[PROPERTY_VALUE_MAX];
                    extern int s_sim_num;

                    if(s_sim_num == 0) {
                        property_get(RIL_MAIN_SIM_PROPERTY, prop, "0");
                        if(!strcmp(prop, "0"))
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", NULL);
                        else
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=0", NULL);
                        snprintf(cmd, sizeof(cmd), "AT^SYSCONFIG=%d,3,2,4", type);
                        err = at_send_command(ATch_type[channelID], cmd, &p_response);
                        if (err < 0 || p_response->success == 0) {
                            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                        } else {
                            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                        }
                    } else if (s_sim_num == 1) {
                        property_get(RIL_MAIN_SIM_PROPERTY, prop, "0");
                        if(!strcmp(prop, "1"))
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", NULL);
                        else
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=0", NULL);
                        property_get(RIL_SIM0_STATE, prop, "ABSENT");
                        RILLOGD(" RIL_SIM0_STATE = %s", prop);
                        if(!strcmp(prop, "ABSENT")) {
                            snprintf(cmd, sizeof(cmd), "AT^SYSCONFIG=%d,3,2,4", type);
                            err = at_send_command(ATch_type[channelID], cmd, &p_response);
                            if (err < 0 || p_response->success == 0) {
                                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                            } else {
                                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                            }
                        } else {
                            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                        }
                    }
                } else {
                    snprintf(cmd, sizeof(cmd), "AT^SYSCONFIG=%d,3,2,4", type);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                }
                if(p_response)
                    at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE:
            {
                int response = 0;

                p_response = NULL;
                err = at_send_command_singleline(ATch_type[channelID], "AT^SYSCONFIG?",
                        "^SYSCONFIG:", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response);
                        // transfer at to rilconstant
                        int type = 0;
                        switch(response) {
                        case 2:
                            type = 0;//NETWORK_MODE_WCDMA_PREF
                            break;
                        case 13:
                            type = 1;//NETWORK_MODE_GSM_ONLY
                            break;
                        case 14:
                        case 15:
                            type = 2;//NETWORK_MODE_WCDMA_ONLY or TDSCDMA ONLY
                            break;
                        }
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, &type,
                                sizeof(type));
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
            at_send_command(ATch_type[channelID], "AT+COPS=0", NULL);
            at_send_command(ATch_type[channelID], "AT+CGAUTO=1", NULL);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:
            requestNetworkRegistration(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_DATA_CALL_LIST:
            requestDataCallList(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_GET_CLIR:
            {
                p_response = NULL;
                int response[2] = {1, 1};
                char *line = NULL;
                int errNum = 0xff;

                err = at_send_command_singleline(ATch_type[channelID], "AT+CLIR?",
                        "+CLIR: ", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;

                    err = at_tok_start(&line);

                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response[0]);

                        if (err >= 0)
                            err = at_tok_nextint(&line, &response[1]);
                    }
                    if (err >= 0) {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    if (err >= 0) {
                        if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
                            line = p_response->finalResponse;
                            err = at_tok_start(&line);
                            if (err >= 0) {
                                err = at_tok_nextint(&line,&errNum);
                            }
                        }
                    }
                    if (err >= 0 && (errNum == 70 || errNum == 3)) {
                        RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_SET_CLIR:
            {
                p_response = NULL;
                int n = ((int *)data)[0];
                char cmd[20] = {0};

                snprintf(cmd, sizeof(cmd), "AT+CLIR=%d", n);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:
            requestQueryNetworkSelectionMode(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_OEM_HOOK_RAW:
            /* echo back data */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
            break;

        case RIL_REQUEST_OEM_HOOK_STRINGS:
            {
                int i;
                const char ** cur;

                RILLOGD("got OEM_HOOK_STRINGS: 0x%8p %lu", data, (long)datalen);


                for (i = (datalen / sizeof (char *)), cur = (const char **)data ;
                        i > 0 ; cur++, i --) {
                    RILLOGD("> '%s'", *cur);
                }

                /* echo back strings */
                RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
                break;
            }
        case RIL_REQUEST_WRITE_SMS_TO_SIM:
            requestWriteSmsToSim(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_DELETE_SMS_ON_SIM:
            {
                char cmd[20] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", ((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_ENTER_SIM_PIN:
        case RIL_REQUEST_ENTER_SIM_PIN2:
        case RIL_REQUEST_ENTER_SIM_PUK:
            requestVerifySimPin(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_ENTER_SIM_PUK2:
            requestVerifySimPuk2(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_CHANGE_SIM_PIN:
            requestEnterSimPin(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_CHANGE_SIM_PIN2:
            requestEnterSimPin2(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_SET_MUTE:
            {
                char cmd[20] = {0};
                snprintf(cmd, sizeof(cmd), "AT+CMUT=%d", ((int *)data)[0]);
                at_send_command(ATch_type[channelID], cmd, NULL);
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                break;
            }
        case  RIL_REQUEST_GET_MUTE:
            {
                p_response = NULL;
                int response = 0;
                err = at_send_command_singleline(ATch_type[channelID], "AT+CMUT?",
                        "+CMUT: ", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
                                sizeof(response));
                    }
                }
                else
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

                at_response_free(p_response);
                break;
            }
        case  RIL_REQUEST_QUERY_CLIP:
            {
                p_response = NULL;
                int response[2] = {0, 0};

                err = at_send_command_singleline(ATch_type[channelID], "AT+CLIP?",
                        "+CLIP: ", &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response[0]);
                        if (err >= 0) {
                            err = at_tok_nextint(&line, &response[1]);
                        }
                    }

                    if (err >= 0) {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
                                sizeof(response));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }

                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_QUERY_CALL_WAITING:
            {
#if defined (RIL_SPRD_EXTENSION)
                p_response = NULL;
                int c = ((int *)data)[0];
                int errNum = 0xff;
                char cmd[30] = {0};
                char *line;
                ATLine *p_cur;

                if (c == 0) {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,2");
                }else {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,2,%d", c);
                }
                err = at_send_command_multiline(ATch_type[channelID], cmd, "+CCWA: ",
                        &p_response);
                if (err >= 0 && p_response->success) {
                    int waitingCount = 0;
                    int validCount = 0;
                    int i;
                    RIL_CallWaitingInfo **waitingList, *waitingPool;

                    for (p_cur = p_response->p_intermediates
                            ; p_cur != NULL
                            ; p_cur = p_cur->p_next, waitingCount++
                        );

                    waitingList = (RIL_CallWaitingInfo **)
                        alloca(waitingCount * sizeof(RIL_CallWaitingInfo *));

                    waitingPool = (RIL_CallWaitingInfo *)
                        alloca(waitingCount * sizeof(RIL_CallWaitingInfo));

                    memset(waitingPool, 0, waitingCount * sizeof(RIL_CallWaitingInfo));

                    /* init the pointer array */
                    for(i = 0; i < waitingCount ; i++)
                        waitingList[i] = &(waitingPool[i]);

                    for (p_cur = p_response->p_intermediates
                            ; p_cur != NULL
                            ; p_cur = p_cur->p_next
                        ) {
                        line = p_cur->line;
                        err = at_tok_start(&line);
                        if (err >= 0) {
                            err = at_tok_nextint(&line, &(waitingList[validCount]->status));
                            if (err >= 0)
                                err = at_tok_nextint(&line, &(waitingList[validCount]->serviceClass));
                        }

                        if (err == 0)
                            validCount++;
                    }
                    if (err >= 0) {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                                validCount ? waitingPool : NULL,
                                validCount * sizeof (RIL_CallWaitingInfo));
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    if (err >= 0) {
                        if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
                            line = p_response->finalResponse;
                            err = at_tok_start(&line);
                            if (err >= 0) {
                                err = at_tok_nextint(&line,&errNum);
                            }
                        }
                    }
                    if (err >= 0 && (errNum == 70 || errNum == 3)) {
                        RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                p_response = NULL;
                int c = ((int *)data)[0];
                int errNum = 0xff;
                char cmd[30] = {0};
                char *line;
                ATLine *p_cur;
                int mode, serviceClass;
                int response[2] = {0, 0};

                if (c == 0) {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,2");
                }else {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,2,%d", c);
                }
                err = at_send_command_multiline(ATch_type[channelID], cmd, "+CCWA: ",
                        &p_response);
                if (err >= 0 && p_response->success) {
                    for (p_cur = p_response->p_intermediates
                            ; p_cur != NULL
                            ; p_cur = p_cur->p_next
                        ) {
                        line = p_cur->line;
                        err = at_tok_start(&line);
                        if (err >= 0) {
                            err = at_tok_nextint(&line, &mode);
                            if (err >= 0) {
                                err = at_tok_nextint(&line, &serviceClass);
                                if(err >= 0) {
                                    response[0] = mode;
                                    response[1] |= serviceClass;
                                }
                            }
                        }
                    }
                    if(response[1] == 0)
                        response[1] = 7;
                    RIL_onRequestComplete(t, RIL_E_SUCCESS,
                               response, sizeof (response));
                } else {
                    if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
                        line = p_response->finalResponse;
                        err = at_tok_start(&line);
                        if (err >= 0) {
                            err = at_tok_nextint(&line,&errNum);
                            if (err >= 0 && (errNum == 70 || errNum == 3))
                                RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
                            else
                                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                        } else
                            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
#endif
            }
        case RIL_REQUEST_SET_CALL_WAITING:
            {
                p_response = NULL;
                int enable = ((int *)data)[0];
                int c = ((int *)data)[1];
                char cmd[40] = {0};

                if (c == 0) {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,%d", enable);
                }else {
                    snprintf(cmd, sizeof(cmd), "AT+CCWA=1,%d,%d", enable, c);
                }

                err = at_send_command(ATch_type[channelID], cmd,
                        &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_RESET_RADIO:
            requestResetRadio(channelID, t);
            break;
        case RIL_REQUEST_SCREEN_STATE:
            requestScreeState(channelID, ((int*)data)[0], t);
            break;
        case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION:
            requestSmsBroadcastActivation(channelID,data, datalen, t);
            break;
        case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG:
            requestSetSmsBroadcastConfig(channelID,data, datalen, t);
            break;
        case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG:
            requestGetSmsBroadcastConfig(channelID,data, datalen, t);
            break;
        case RIL_REQUEST_STK_GET_PROFILE:
            {
                char * line;

                p_response = NULL;
                err = at_send_command(ATch_type[channelID], "AT+SPUSATPROFILE?", &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, line, strlen(line)+1);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_STK_SET_PROFILE:
            break;
        case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND:
            {
                char *cmd;
                int ret;

                p_response = NULL;
                ret = asprintf(&cmd, "AT+SPUSATENVECMD=\"%s\"", (char*)(data));
                if(ret < 0) {
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
        case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE:
            {
                char *cmd;
                int ret;

                p_response = NULL;
                ret = asprintf(&cmd, "AT+SPUSATTERMINAL=\"%s\"", (char*)(data));
                if(ret < 0) {
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
        case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM:
            {
                p_response = NULL;
                int value = ((int *)data)[0];
                if (value == 0) {
                    RILLOGD(" cancel STK call ");
                    s_isstkcall = 0;
                    err = at_send_command(ATch_type[channelID], "AT+SPUSATCALLSETUP=0", &p_response);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                    at_response_free(p_response);
                } else {
                    RILLOGD(" confirm STK call ");
                    s_isstkcall = 1;
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING:
            {
                int response = 0;
                p_response = NULL;
                RILLOGD("[stk]send RIL_REQUEST_STK_SET_PROFILE");
                err = at_send_command(ATch_type[channelID], "AT+SPUSATPROFILE?", &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    RILLOGD("[stk]RIL_REQUEST_STK_SET_PROFILE: err=%d", err);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_GET_SMSC_ADDRESS:
            {
                char *sc_line;
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                char  sc_temp[50] = {0};
                char ch;
                unsigned int i,j = 0;
#endif

                p_response = NULL;
                RILLOGD("[sms]RIL_REQUEST_GET_SMSC_ADDRESS");
                err = at_send_command_singleline(ATch_type[channelID], "AT+CSCA?", "+CSCA:",
                        &p_response);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
#if defined (RIL_SPRD_EXTENSION)
                        err = at_tok_nextstr(&line, &sc_line);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, sc_line,
                                strlen(sc_line));
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                        line++;
                        for ( i = 0; i < strlen(line); i++ )
                        {
                            ch = *(line+i);
                            if( ch == '\"' || ch == ','){
                                sc_temp[j] = ch;
                                if( ch == ',' ){
                                    i++;
                                    j++;
                                    while(*(line+i) != '\0'){
                                        sc_temp[j] = *(line+i);
                                        i++;
                                        j++;
                                    }
                                    break;
                                }
                                j++;
                            }
                            else{
                                if(ch >= '0' && ch <= '9')
                                    sc_temp[j] = (char)((ch - '0') << 4);
                                else if(ch >= 'a' && ch <= 'f')
                                    sc_temp[j] = (char)((ch - 'a' + 10) << 4);
                                else if(ch >= 'A' && ch <= 'F')
                                    sc_temp[j] = (char)((ch - 'A' + 10) << 4);


                                ch = *(line+i+1);
                                if(ch >= '0' && ch <= '9')
                                    sc_temp[j] |= (char)(ch - '0');
                                else if(ch >= 'a' && ch <= 'f')
                                    sc_temp[j] |= (char)(ch - 'a' + 10);
                                else if(ch >= 'A' && ch <= 'F')
                                    sc_temp[j] |= (char)(ch - 'A' + 10);
                                i++;
                                j++;
                            }

                        }
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, sc_temp,
                                strlen(sc_temp));
#endif
                    } else {
                        RILLOGD("[sms]at_tok_start fail");
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    RILLOGD("[sms]RIL_REQUEST_GET_SMSC_ADDRESS fail");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_SET_SMSC_ADDRESS:
            {
                char *cmd;
                int ret;
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                char str_temp[50] = {0};
                char *cp;
                unsigned int i;
#endif
                p_response = NULL;
                RILLOGD("[sms]RIL_REQUEST_SET_SMSC_ADDRESS (%s)", (char*)(data));
#if defined (RIL_SPRD_EXTENSION)
                ret = asprintf(&cmd, "AT+CSCA=\"%s\"", (char*)(data));
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                cp = str_temp;
                for(i = 0 ; i< strlen((char*)data); i++)
                {
                    if( *((char*)data + i) != '\"' && *((char*)data+i) != ',' && *((char*)data+i) != '\0'){
                        cp += sprintf(cp,"%X",*((char*)data+i));
                    }
                    else if( *((char*)data+i) == ','){
                        break;
                    }
                }

                ret = asprintf(&cmd, "AT+CSCA=\"%s\"", str_temp);
#endif
                if(ret < 0) {
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
        case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS:
            {
                char cmd[30] = {0};

                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT+SPSMSFULL=%d", !((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

#if defined (RIL_SPRD_EXTENSION)
        case RIL_REQUEST_GET_CURRENT_VIDEOCALLS:
            requestGetCurrentCalls(channelID, data, datalen, t, 1);
            break;
        case RIL_REQUEST_END_ALL_CONNECTIONS:
            p_response = NULL;
            err = at_send_command(ATch_type[channelID], "ATH", &p_response);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        case RIL_REQUEST_SIM_POWER:
            requestSIMPower(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_GPRS_ATTACH:
            attachGPRS(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_GPRS_DETACH:
            detachGPRS(channelID, data, datalen, t);
            break;
       case RIL_REQUEST_SET_CMMS:
            requestSetCmms(channelID, data, datalen, t);
            break;
       case RIL_REQUEST_SET_SPEED_MODE:
            requestSetSpeedMode(channelID, data, datalen, t);
            break;
        case  RIL_REQUEST_QUERY_COLP:
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
        case  RIL_REQUEST_QUERY_COLR:
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
        case RIL_REQUEST_VIDEOPHONE_CONTROL_AUDIO:
            {
                char cmd[30] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d", ((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_VIDEOPHONE_CONTROL_IFRAME:
            {
                char cmd[50] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTLFRAME=%d,%d", ((int *)data)[0], ((int *)data)[1]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_VIDEOPHONE_SET_VOICERECORDTYPE:
            {
                char cmd[30] = {0};
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT+SPRUDLV=%d", ((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_VIDEOPHONE_DIAL:
            requestVideoPhoneDial(channelID, data, datalen, t);
            break;
        case RIL_REQUEST_VIDEOPHONE_CODEC:
            {
                char cmd[30] = {0};
                p_response = NULL;
                RIL_VideoPhone_Codec* p_codec = (RIL_VideoPhone_Codec*)data;
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
        case RIL_REQUEST_VIDEOPHONE_HANGUP:
            {
                p_response = NULL;
#ifdef NEW_AT
                int reason = ((int *)data)[0];
                if (reason < 0){
                    err = at_send_command(ATch_type[channelID], "ATH", &p_response);
                } else {
                    char cmd[20] = {0};
                    snprintf(cmd, sizeof(cmd), "ATH%d", ((int *)data)[0]);
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
        case RIL_REQUEST_VIDEOPHONE_ANSWER:
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
        case RIL_REQUEST_VIDEOPHONE_FALLBACK:
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
        case RIL_REQUEST_VIDEOPHONE_STRING:
            {
                char *cmd;
                int ret;
                p_response = NULL;
                ret = asprintf(&cmd, "AT"AT_PREFIX"DVTSTRS=\"%s\"", (char*)(data));
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
        case RIL_REQUEST_VIDEOPHONE_LOCAL_MEDIA:
            {
                char cmd[50] = {0};
                p_response = NULL;
                int datatype = ((int*)data)[0];
                int sw = ((int*)data)[1];

                if ((datalen/sizeof(int)) >2){
                    int indication = ((int*)data)[2];
                    snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d,%d", datatype, sw, indication);
                } else {
                    snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTSEND=%d,%d", datatype, sw);
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
        case RIL_REQUEST_VIDEOPHONE_RECORD_VIDEO:
            {
                char cmd[30];
                p_response = NULL;
                snprintf(cmd, sizeof(cmd), "AT"AT_PREFIX"DVTRECA=%d", ((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_VIDEOPHONE_RECORD_AUDIO:
            {
                char cmd[40];
                p_response = NULL;
                int on = ((int*)data)[0];
                int mode = ((int*)data)[1];

                if (datalen > 1) {
                    snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d,%d", on, mode);
                } else {
                    snprintf(cmd, sizeof(cmd), "AT^DAUREC=%d", ((int *)data)[0]);
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
        case RIL_REQUEST_VIDEOPHONE_TEST:
            {
                char cmd[40];
                p_response = NULL;
                int flag = ((int*)data)[0];
                int value = ((int*)data)[1];

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
        case RIL_REQUEST_MBBMS_GSM_AUTHEN:
            {
                char *cmd;
                char *line;
                int ret;
                p_response = NULL;
                RILLOGD("[MBBMS]send RIL_REQUEST_MBBMS_GSM_AUTHEN");
                ret = asprintf(&cmd, "AT^MBAU=\"%s\"",(char*)(data));
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
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, line, strlen(line));
                    }
                    else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_MBBMS_USIM_AUTHEN:
            {
                char *cmd;
                char *line;
                int ret;
                p_response = NULL;
                RILLOGD("[MBBMS]send RIL_REQUEST_MBBMS_USIM_AUTHEN");
                ret = asprintf(&cmd, "AT^MBAU=\"%s\",\"%s\"",((char **)(data))[0], ((char **)(data))[1]);
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
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, line, strlen(line));
                    }
                    else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_MBBMS_SIM_TYPE:
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
            RIL_onRequestComplete(t, RIL_E_SUCCESS, str, strlen(str));
            break;
        }
        case RIL_REQUEST_GET_REMAIN_TIMES:
            {
                char  cmd[20] = {0};
                int   type = ((int*)data)[0];
                char *line;
                int result;

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
                break;
            }
        case RIL_REQUEST_GET_SIM_CAPACITY:
            {
                char *line, *skip;
                int response[2] = {-1, -1};
                char *responseStr[2] = {NULL, NULL};
                char res[2][20];
                int i, result = 0;

                for(i = 0; i < 2; i++)
                    responseStr[i] = res[i];

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
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, 2*sizeof(char*));
                    } else {
                        RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    RILLOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_MMI_ENTER_SIM:
            {
                char *cmd;
                int ret;

                p_response = NULL;
                RILLOGD("[SIM]send RIL_REQUEST_MMI_ENTER_SIM");
                ret = asprintf(&cmd, "ATD%s",(char*)(data));
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
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        case RIL_REQUEST_GET_CELL_BROADCAST_CONFIG:
            requestGetCellBroadcastConfig(channelID,data, datalen, t);
            break;

        case RIL_REQUEST_SEND_ENCODED_USSD:
            requestSendEncodedUSSD(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO:
            requestGetPhonebookStorageInfo(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_GET_PHONEBOOK_ENTRY:
            requestGetPhonebookEntry(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY:
            requestAccessPhonebookEntry(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_USIM_PB_CAPA:
            requestUsimPbCapa(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_LOCK_INFO:
            {
                char  cmd[20] = {0};
                int   type = -1;
                char *line = NULL;
                int   result = 0;
                RIL_SIM_Lockinfo *p_lock = NULL;
                RIL_SIM_Lockinfo_Response lock_info;

                RILLOGD("RIL_REQUEST_LOCK_INFO");
                memset(&lock_info, 0, sizeof(RIL_SIM_Lockinfo_Response));
                p_lock = (RIL_SIM_Lockinfo *)data;
                lock_info.lock_type = p_lock->lock_type;
                p_response = NULL;
                switch (p_lock->lock_type){
                  case 3: // LOCK_SIM
                    type = 0;
                    lock_info.lock_key = 1; // PIN
                    break;
                  case 9: // LOCK_PIN2
                    type = 1;
                    lock_info.lock_key = 3; // PIN2
                    break;
                  case 10: // LOCK_PUK2
                    type = 3;
                    lock_info.lock_key = 4; // PUK2
                    break;
                  default:
                    RILLOGD("RIL_REQUEST_LOCK_INFO: unsupport lock type!!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                if(type == -1)
                    break;
                snprintf(cmd, sizeof(cmd), "AT+XX=%d", type);
                err = at_send_command_singleline(ATch_type[channelID], cmd, "+XX:",
                                                 &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    RILLOGD("RIL_REQUEST_LOCK_INFO: err=%d line=%s", err, line);
                    err = at_tok_start(&line);
                    if (err == 0) err = at_tok_nextint(&line, &result);
                    if (err == 0) {
                        lock_info.num_lock_type = 1;
                        lock_info.num_of_retry = result;
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, &lock_info, sizeof(RIL_SIM_Lockinfo_Response));
                    }
                    else {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                }
                break;
            }

        case RIL_REQUEST_STK_SIM_INIT_EVENT:
            {
                RILLOGD("RIL_REQUEST_STK_SIM_INIT_EVENT");
                p_response = NULL;
                err = at_send_command(ATch_type[channelID], "AT+SPUSATCHECKFDN=1", &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
#endif
        case RIL_REQUEST_SEND_AT:
            requestSendAT(channelID,data, datalen, t);
            break;
        default:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }

#if defined (RIL_SPRD_EXTENSION)
    putChannel(channelID);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    if(request != RIL_REQUEST_OEM_HOOK_RAW)
        putChannel(channelID);
#endif
}

/**
 * Synchronous call from the RIL to us to return current radio state.
 * RADIO_STATE_UNAVAILABLE should be the initial state.
 */
static RIL_RadioState
currentState()
{
    return sState;
}
/**
 * Call from RIL to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */

static int
onSupports (int requestCode)
{
    /* @@@ todo */

    return 1;
}

static void onCancel (RIL_Token t)
{
    /* @@@ todo */

}

static const char * getVersion(void)
{
    pthread_mutex_lock(&s_sms_ready_mutex);
    s_socket_connected = 1;
    pthread_mutex_unlock(&s_sms_ready_mutex);
    return "android reference-ril 1.0";
}

static void
setRadioState(int channelID, RIL_RadioState newState)
{
    RIL_RadioState oldState;

    pthread_mutex_lock(&s_state_mutex);

    oldState = sState;

    if (s_closed > 0) {
        /* If we're closed, the only reasonable state is
         * RADIO_STATE_UNAVAILABLE
         * This is here because things on the main thread
         * may attempt to change the radio state after the closed
         * event happened in another thread
	 */
        newState = RADIO_STATE_UNAVAILABLE;
    }

    if (sState != newState || s_closed > 0) {
        sState = newState;

        pthread_cond_broadcast (&s_state_cond);
    }

    pthread_mutex_unlock(&s_state_mutex);


    /* do these outside of the mutex */
    if (sState != oldState) {
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                NULL, 0);

        /* FIXME onSimReady() and onRadioPowerOn() cannot be called
         * from the AT reader thread
         * Currently, this doesn't happen, but if that changes then these
         * will need to be dispatched on the request thread
         */
        if (sState == RADIO_STATE_SIM_READY) {
            onSIMReady(channelID);
        } else if (sState == RADIO_STATE_SIM_NOT_READY) {
            onRadioPowerOn(channelID);
        }
    }
}

/** Returns SIM_NOT_READY on error */
static SIM_Status
getSIMStatus(int channelID)
{
    ATResponse *p_response = NULL;
    int err;
    int ret;
    char *cpinLine;
    char *cpinResult;

    if (sState == RADIO_STATE_UNAVAILABLE) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_send_command_singleline(ATch_type[channelID], "AT+CPIN?", "+CPIN:", &p_response);

    if (err != 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            ret = SIM_ABSENT;
            goto done;

        default:
            ret = SIM_NOT_READY;
            goto done;
    }

    /* CPIN? has succeeded, now look at the result */

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    if (0 == strcmp (cpinResult, "SIM PIN")) {
        /* add for modem reboot */
        char sim_prop[PROPERTY_VALUE_MAX];
        char prop[PROPERTY_VALUE_MAX];
        char *cmd;
        int res;
        ATResponse   *p_response1 = NULL;

        if(!strcmp(s_modem, "t")) {
            strcpy(sim_prop, RIL_TD_ASSERT);
        } else if(!strcmp(s_modem, "w")) {
            strcpy(sim_prop, RIL_W_ASSERT);
        }
        property_get(sim_prop, prop, "0");
        if(!strcmp(prop, "1")) {
            extern int s_sim_num;
            if(s_multiSimMode) {
                if(s_sim_num == 0) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY);
                    }
                    property_get(sim_prop, prop, "");
                } else if (s_sim_num == 1) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY1);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY1);
                    }
                    property_get(sim_prop, prop, "");
                } else if (s_sim_num == 2) {
                    if(!strcmp(s_modem, "t")) {
                        strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY2);
                    } else if(!strcmp(s_modem, "w")) {
                        strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY2);
                    }
                    property_get(sim_prop, prop, "");
                }
                if(strlen(prop) != 4) {
                    goto out;
                } else {
                    res = asprintf(&cmd, "AT+CPIN=%s", prop);
                    if(res < 0) {
                        RILLOGE("Failed to allocate memory");
                        cmd = NULL;
                        goto out;
                    }
                    err = at_send_command(ATch_type[channelID], cmd, &p_response1);
                    free(cmd);
                    if(err < 0 || p_response1->success == 0) {
                        at_response_free(p_response1);
                        goto out;
                    }
                    at_response_free(p_response1);
                    ret = SIM_NOT_READY;
                    goto done;
                }
            } else {
                if(!strcmp(s_modem, "t")) {
                    strcpy(sim_prop, RIL_TD_SIM_PIN_PROPERTY);
                } else if(!strcmp(s_modem, "w")) {
                    strcpy(sim_prop, RIL_W_SIM_PIN_PROPERTY);
                }
                property_get(sim_prop, prop, "");
                if(strlen(prop) != 4) {
                    goto out;
                } else {
                    res = asprintf(&cmd, "AT+CPIN=%s", prop);
                    if(res < 0) {
                        RILLOGE("Failed to allocate memory");
                        cmd = NULL;
                        goto out;
                    }
                    err = at_send_command(ATch_type[channelID], cmd, &p_response1);
                    free(cmd);
                    if(err < 0 || p_response1->success == 0) {
                        at_response_free(p_response1);
                        goto out;
                    }
                    at_response_free(p_response1);
                    ret = SIM_NOT_READY;
                    goto done;
                }
            }
        }
out:
        ret = SIM_PIN;
        goto done;
    } else if (0 == strcmp (cpinResult, "SIM PUK")) {
        ret = SIM_PUK;
        goto done;
    } else if (0 == strcmp (cpinResult, "SIM PIN2")) {
        ret = SIM_PIN2;
        goto done;
    } else if (0 == strcmp (cpinResult, "SIM PUK2")) {
        ret = SIM_PUK2;
        goto done;
    } else if (0 == strcmp (cpinResult, "BLOCK")) {
        ret = SIM_BLOCK;
        goto done;
    } else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
        return SIM_NETWORK_PERSONALIZATION;
    } else if (0 != strcmp (cpinResult, "READY"))  {
        /* we're treating unsupported lock types as "sim absent" */
        ret = SIM_ABSENT;
        goto done;
    }

    at_response_free(p_response);
    p_response = NULL;
    cpinResult = NULL;

    ret = SIM_READY;

done:
    at_response_free(p_response);
    return ret;
}


/**
 * Get the current card status.
 *
 * This must be freed using freeCardStatus.
 * @return: On success returns RIL_E_SUCCESS
 */
static int getCardStatus(int channelID, RIL_CardStatus_v6 **pp_card_status)
{
    static RIL_AppStatus app_status_array[] = {
        /* SIM_ABSENT = 0 */
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        /* SIM_NOT_READY = 1 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_DETECTED, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        /* SIM_READY = 2 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_READY,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        /* SIM_PIN = 3 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_PIN, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        /* SIM_PUK = 4 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_PUK, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_ENABLED_BLOCKED, RIL_PINSTATE_UNKNOWN },
        /* SIM_NETWORK_PERSONALIZATION = 5 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
            NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        /* SIM_BLOCK = 6 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_BLOCKED, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        /* SIM_PIN2 = 7 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_ENABLED_NOT_VERIFIED },
        /* SIM_PUK2 = 8 */
        { RIL_APPTYPE_SIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_UNKNOWN,
            NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_ENABLED_BLOCKED }
    };
    RIL_CardState card_state;
    int num_apps;

    int sim_status = getSIMStatus(channelID);
    if (sim_status == SIM_ABSENT) {
        card_state = RIL_CARDSTATE_ABSENT;
        num_apps = 0;
    } else {
        card_state = RIL_CARDSTATE_PRESENT;
        num_apps = 1;
    }

    /* Allocate and initialize base card status. */
    RIL_CardStatus_v6 *p_card_status = malloc(sizeof(RIL_CardStatus_v6));
    p_card_status->card_state = card_state;
    p_card_status->universal_pin_state = RIL_PINSTATE_UNKNOWN;
    p_card_status->gsm_umts_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->cdma_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->ims_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->num_applications = num_apps;

    RIL_AppType app_type = getSimType( channelID);



    /* Initialize application status */
    unsigned int i;
    for (i = 0; i < RIL_CARD_MAX_APPS; i++) {
        p_card_status->applications[i] = app_status_array[SIM_ABSENT];
    }
    for(i = 0; i < sizeof(app_status_array)/sizeof(RIL_AppStatus); i++) {

        app_status_array[i].app_type = app_type;
        RILLOGD("app type %d",app_type);
    }
    /* Pickup the appropriate application status
     * that reflects sim_status for gsm.
     */
    if (num_apps != 0) {
        /* Only support one app, gsm */
        p_card_status->num_applications = 1;
        p_card_status->gsm_umts_subscription_app_index = 0;

        /* Get the correct app status */
        p_card_status->applications[0] = app_status_array[sim_status];
    }

    *pp_card_status = p_card_status;
    return RIL_E_SUCCESS;
}

/**
 * Free the card status returned by getCardStatus
 */
static void freeCardStatus(RIL_CardStatus_v6 *p_card_status)
{
    free(p_card_status);
}

/**
 * SIM ready means any commands that access the SIM will work, including:
 *  AT+CPIN, AT+CSMS, AT+CNMI, AT+CRSM
 *  (all SMS-related commands)
 */

static void pollSIMState (void *param)
{
    ATResponse *p_response;
    int ret;
    int channelID;

    if(param != NULL)
        channelID = *((int *)param);
    else
        channelID = getChannel();


    if (sState != RADIO_STATE_SIM_NOT_READY) {
        /* no longer valid to poll */
        if(param == NULL)
            putChannel(channelID);
        return;
    }

    switch(getSIMStatus(channelID)) {
        case SIM_ABSENT:
        case SIM_PIN:
        case SIM_PUK:
        case SIM_NETWORK_PERSONALIZATION:
        default:
            setRadioState(channelID, RADIO_STATE_SIM_LOCKED_OR_ABSENT);
            if(param == NULL)
                putChannel(channelID);
            return;

        case SIM_NOT_READY:
            if(param == NULL)
                putChannel(channelID);
            RIL_requestTimedCallback (pollSIMState, NULL, &TIMEVAL_SIMPOLL);
            return;

        case SIM_READY:
        case SIM_PIN2:
        case SIM_PUK2:
            setRadioState(channelID, RADIO_STATE_SIM_READY);
            if(param == NULL)
                putChannel(channelID);
            return;
    }
}

static void attachGPRS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int ret;
    int err;

    err = at_send_command(ATch_type[channelID], "AT+CGATT=1", &p_response);
    if (err < 0 || p_response->success == 0) {
        at_response_free(p_response);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;
}

static void detachGPRS(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int ret;
    int err, i;

    for(i = 0; i < MAX_PDP; i++) {
        if (pdp[i].cid > 0) {
            RILLOGD("pdp[%d].state = %d", i, pdp[i].state);
            putPDP(i);
        }
    }

    err = at_send_command(ATch_type[channelID], "AT+SGFD", &p_response);
    if (err < 0 || p_response->success == 0) {
        at_response_free(p_response);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;
}

/** returns 1 if on, 0 if off, and -1 on error */
static int isRadioOn(int channelID)
{
    ATResponse *p_response = NULL;
    int err;
    char *line;
    int ret;

    err = at_send_command_singleline(ATch_type[channelID], "AT+CFUN?", "+CFUN:", &p_response);

    if (err < 0 || p_response->success == 0) {
        /* assume radio is off */
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &ret);
    if (err < 0) goto error;

    at_response_free(p_response);

    if(ret == 0 || ret == 2)
        return 0;
    else if(ret == 1)
        return 1;
    else
        return -1;
error:
    at_response_free(p_response);
    return -1;
}

/**
 * Initialize everything that can be configured while we're still in
 * AT+CFUN=0
 */
static void initializeCallback(void *param)
{
    ATResponse *p_response = NULL;
    ATResponse *p_response1 = NULL;
    int err;
    int channelID;
    channelID = getChannel();
    setRadioState (channelID, RADIO_STATE_OFF);

    /* note: we don't check errors here. Everything important will
       be handled in onATTimeout and onATReaderClosed */

    /*  atchannel is tolerant of echo but it must */
    /*  have verbose result codes */
    at_send_command(ATch_type[channelID], "ATE0Q0V1", NULL);
#if 0
    /* use CRING instead of RING */
    at_send_command(ATch_type[channelID], "AT+CRC=1", NULL);

    /* use +CLIP: to indicate CLIP */
    at_send_command(ATch_type[channelID], "AT+CLIP=1", NULL);

    /* use +COLP: to indicate COLP */
    /* set it 0 to disable subscriber info and avoid cme err 512 */
    at_send_command(ATch_type[channelID], "AT+COLP=0", NULL);

    /* use +CCWA: to indicate waiting call */
    at_send_command(ATch_type[channelID], "AT+CCWA=1,1", NULL);

    /* use +CTZU: timezone update */
    at_send_command(ATch_type[channelID], "AT+CTZU=1", NULL);

    /* use +CTZR: timezone reporting */
    at_send_command(ATch_type[channelID], "AT+CTZR=1", NULL);

    /* configure message format as PDU mode*/
    /* FIXME: TEXT mode support!! */
        at_send_command(ATch_type[channelID], "AT+CMGF=0", NULL);
#endif
    /*  No auto-answer */
    at_send_command(ATch_type[channelID], "ATS0=0", NULL);

    /*  Extended errors */
    at_send_command(ATch_type[channelID], "AT+CMEE=1", NULL);

    /*  Network registration events */
    err = at_send_command(ATch_type[channelID], "AT+CREG=2", &p_response);

    /* some handsets -- in tethered mode -- don't support CREG=2 */
    if (err < 0 || p_response->success == 0) {
        at_send_command(ATch_type[channelID], "AT+CREG=1", NULL);
    }

    at_response_free(p_response);

    /*  GPRS registration events */
    at_send_command(ATch_type[channelID], "AT+CGREG=2", NULL);

    /*  Signal strength unsolicited switch
     *  < +CSQ: rssi,ber> will unsolicited
     * */
    at_send_command(ATch_type[channelID], "AT+CCED=1,8", NULL);

    /*  Call Waiting notifications */
    at_send_command(ATch_type[channelID], "AT+CCWA=1", NULL);

    /*  Alternating voice/data off */
    at_send_command(ATch_type[channelID], "AT+CMOD=0", NULL);

    /*  Not muted */
    at_send_command(ATch_type[channelID], "AT+CMUT=0", NULL);

    /*  +CSSU unsolicited supp service notifications
     *  CSSU,CSSI
     */
    at_send_command(ATch_type[channelID], "AT+CSSN=1,1", NULL);

    /*  no connected line identification */
    at_send_command(ATch_type[channelID], "AT+COLP=0", NULL);

    /*  HEX character set */
    at_send_command(ATch_type[channelID], "AT+CSCS=\"HEX\"", NULL);

    /*  USSD unsolicited */
    at_send_command(ATch_type[channelID], "AT+CUSD=1", NULL);

    /*  Enable +CGEV GPRS event notifications, but don't buffer */
    at_send_command(ATch_type[channelID], "AT+CGEREP=1,0", NULL);

    /*  SMS PDU mode */
    at_send_command(ATch_type[channelID], "AT+CMGF=0", NULL);

    /* set DTMF tone duration to minimum value */
    at_send_command(ATch_type[channelID], "AT+VTD=1", NULL);

    /* following is videophone h324 initialization */
    at_send_command(ATch_type[channelID], "AT+CRC=1", NULL);

    at_send_command(ATch_type[channelID], "AT^DSCI=1", NULL);
    at_send_command(ATch_type[channelID], "AT"AT_PREFIX"DVTTYPE=1", NULL);
    at_send_command(ATch_type[channelID], "AT+SPVIDEOTYPE=3", NULL);
    at_send_command(ATch_type[channelID], "AT+SPDVTDCI="VT_DCI, NULL);
    at_send_command(ATch_type[channelID], "AT+SPDVTTEST=2,650", NULL);

    /*power on sim card */
    if(s_multiSimMode) {
        char sim_prop[PROPERTY_VALUE_MAX];
        char prop[PROPERTY_VALUE_MAX];
        extern int s_sim_num;

        if(s_sim_num == 0) {
            if(!strcmp(s_modem, "t")) {
                strcpy(sim_prop, RIL_TD_SIM_POWER_PROPERTY);
            } else if(!strcmp(s_modem, "w")) {
                strcpy(sim_prop, RIL_W_SIM_POWER_PROPERTY);
            }
            property_get(sim_prop, prop, "0");
            if(!strcmp(prop, "0")) {
                property_set(sim_prop, "1");
                at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
            }
        } else if (s_sim_num == 1) {
            if(!strcmp(s_modem, "t")) {
                strcpy(sim_prop, RIL_TD_SIM_POWER_PROPERTY1);
            } else if(!strcmp(s_modem, "w")) {
                strcpy(sim_prop, RIL_W_SIM_POWER_PROPERTY1);
            }
            property_get(sim_prop, prop, "0");
            if(!strcmp(prop, "0")) {
                property_set(sim_prop, "1");
                at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
            }
        } else if (s_sim_num == 2) {
            if(!strcmp(s_modem, "t")) {
                strcpy(sim_prop, RIL_TD_SIM_POWER_PROPERTY2);
            } else if(!strcmp(s_modem, "w")) {
                strcpy(sim_prop, RIL_W_SIM_POWER_PROPERTY2);
            }
            property_get(sim_prop, prop, "0");
            if(!strcmp(prop, "0")) {
                property_set(sim_prop, "1");
                at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
            }
        }
    } else {
        char sim_prop[PROPERTY_VALUE_MAX];
        char prop[PROPERTY_VALUE_MAX];

        if(!strcmp(s_modem, "t")) {
            strcpy(sim_prop, RIL_TD_SIM_POWER_PROPERTY);
        } else if(!strcmp(s_modem, "w")) {
            strcpy(sim_prop, RIL_W_SIM_POWER_PROPERTY);
        }
        property_get(sim_prop, prop, "0");
        if(!strcmp(prop, "0")) {
            property_set(sim_prop, "1");
            at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
        }
    }

    /* assume radio is off on error */
    if(isRadioOn(channelID) > 0) {
        setRadioState (channelID, RADIO_STATE_SIM_NOT_READY);
    }
    putChannel(channelID);

    list_init(&dtmf_char_list);
    sem_post(&w_sem);
}

static void waitForClose()
{
    pthread_mutex_lock(&s_state_mutex);

    while (s_closed == 0) {
        pthread_cond_wait(&s_state_cond, &s_state_mutex);
    }

    pthread_mutex_unlock(&s_state_mutex);
}

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
static void onNitzReceived(void *param)
{
    int i, err;
    char nitz_str[128];
    ATResponse *p_response = NULL;
    char *line;
    int channelID, mcc;
    char *raw_str = (char *)param;

    if(!raw_str) {
        RILLOGE("nitz received, but raw str is NULL");
        return;
    }

    channelID = getChannel();
    err = at_send_command_singleline(ATch_type[channelID], "AT+CCED=0,1", "+CCED:", &p_response);
    putChannel(channelID);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }
    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &mcc);
    if (err < 0) goto error;

    for(i = 0; i < (int)NUM_ELEMS(rilnet_tz_entry); i++) {
        if(mcc == rilnet_tz_entry[i].mcc) {
            RILLOGD("nitz matched mcc = %d", mcc);
            if(*(rilnet_tz_entry[i].long_name) == '\0') {
                snprintf(nitz_str, sizeof(nitz_str), "%s,%d", raw_str, mcc);
                RILLOGD("long_name is empty, nitz_str = %s", nitz_str);
            } else {
                snprintf(nitz_str, sizeof(nitz_str), "%s,%s", raw_str, rilnet_tz_entry[i].long_name);
                RILLOGD("nitz_str = %s", nitz_str);
            }
            break;
        }
    }
    if(i >= (int)NUM_ELEMS(rilnet_tz_entry))
        snprintf(nitz_str, sizeof(nitz_str), "%s", raw_str);

    RIL_onUnsolicitedResponse (
            RIL_UNSOL_NITZ_TIME_RECEIVED,
            nitz_str, strlen(nitz_str));
    free(raw_str);
    return;
error:
    free(raw_str);
    at_response_free(p_response);
}
#endif

/**
 * Called by atchannel when an unsolicited line appears
 * This is called on atchannel's reader thread. AT commands may
 * not be issued here
 */
static void onUnsolicited (const char *s, const char *sms_pdu)
{
    char *line = NULL;
    int err;

    /* Ignore unsolicited responses until we're initialized.
     * This is OK because the RIL library will poll for initial state
     */
    if (sState == RADIO_STATE_UNAVAILABLE) {
        RILLOGD("[unsl] state=%d  %s", sState, s);
        return;
    }

#if defined (RIL_SPRD_EXTENSION)
    if (strStartsWith(s, "+CSQ:")) {
        RIL_SignalStrength_v6 response_v6;
        char *tmp;

        memset(&response_v6, -1, sizeof(RIL_SignalStrength_v6));
        line = strdup(s);
        tmp = line;

        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &(response_v6.GW_SignalStrength.signalStrength));
        if (err < 0) goto out;

        err = at_tok_nextint(&tmp, &(response_v6.GW_SignalStrength.bitErrorRate));
        if (err < 0) goto out;

        RIL_onUnsolicitedResponse(
                RIL_UNSOL_SIGNAL_STRENGTH,
                &response_v6, sizeof(RIL_SignalStrength_v6));
    } else
#endif
    if (strStartsWith(s, "+CTZV:")) {
        /*NITZ time */
        char *response;
        char *tmp;
        char *raw_str;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err != 0) {
            RILLOGE("invalid NITZ line %s\n", s);
        } else {
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            raw_str = (char *)malloc(strlen(response) + 1);
            memcpy(raw_str, response, strlen(response) + 1);
            RIL_requestTimedCallback (onNitzReceived, raw_str, NULL);
#elif defined (RIL_SPRD_EXTENSION)
            RIL_onUnsolicitedResponse (
                    RIL_UNSOL_NITZ_TIME_RECEIVED,
                    response, strlen(response));
#endif
        }
    } else if (strStartsWith(s,"+CRING:")
            || strStartsWith(s,"RING")
            || strStartsWith(s,"NO CARRIER")
            || strStartsWith(s,"+CCWA")
            ) {
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
                NULL, 0);
    } else if (strStartsWith(s,"+CREG:")
            || strStartsWith(s,"+CGREG:")
            ) {
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                NULL, 0);
    } else if (strStartsWith(s,"^CEND:")) {
        char *p;
        char *tmp;
        int commas;
        int skip;
        int end_status;
        int cc_cause;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        /* count number of commas */
        commas = 0;
        for (p = tmp; *p != '\0' ;p++) {
            if (*p == ',') commas++;
        }
        err = at_tok_nextint(&tmp, &skip);
        if (err < 0) goto out;

        skipNextComma(&tmp);

        err = at_tok_nextint(&tmp, &end_status);
        if (err < 0) goto out;

        err = at_tok_nextint(&tmp, &cc_cause);
        if (err < 0) goto out;

        if(commas == 3) {
            pthread_mutex_lock(&s_call_mutex);
            call_fail_cause = cc_cause;
            pthread_mutex_unlock(&s_call_mutex);
            RILLOGD("The last call fail cause: %d", call_fail_cause);
        }
        if(commas == 4) { /*GPRS reply 5 parameters*/
            if(end_status != 29) {
                RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);
                RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                        NULL, 0);
            }
        }
    } else if (strStartsWith(s, "+SIND:")) {
        char *tmp;
        int sms_rd;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &sms_rd);
        if (err < 0) goto out;

        if(sms_rd == 12) {
            pthread_mutex_lock(&s_sms_ready_mutex);
            if(s_socket_connected) {
                if(s_sms_ready == 0) {
                    s_sms_ready = 1;
#if defined (RIL_SPRD_EXTENSION)
                    RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_SMS_READY, NULL, 0);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                    RIL_onUnsolicitedResponse (RIL_UNSOL_DEVICE_READY_NOTI, NULL, 0);
#endif
                }
            }
            pthread_mutex_unlock(&s_sms_ready_mutex);
        }
    } else if (strStartsWith(s, "+CMT:")) {
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_NEW_SMS,
                sms_pdu, strlen(sms_pdu));

    } else if (strStartsWith(s, "+ECIND:")) {
        char *tmp;
        int type;
        int value = 0, cause = -1;
        int card_type;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &type);
        if (err < 0) goto out;

        if(type == 3) {
            if (at_tok_hasmore(&tmp)) {
                err = at_tok_nextint(&tmp, &value);
                if (err < 0) goto out;
                if(value == 1) {
                    if (at_tok_hasmore(&tmp)) {
                        err = at_tok_nextint(&tmp, &cause);
                        if (err < 0) goto out;
                        if(cause == 2)
                            RIL_requestTimedCallback (onSimAbsent, NULL, NULL);
                    }
                } else if (value == 100) {
                    RIL_requestTimedCallback (onSimPresent, NULL, NULL);
                } else if (value == 0 || value == 2) {
                    if(sState == RADIO_STATE_OFF) {
                        RILLOGD("RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED  RADIO_STATE_OFF");
                        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,NULL, 0);
                    }
                }
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                else if(value == 5) {
                    RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_PB_READY, NULL, 0);
                } else if(value == 6) {
                    extern int s_sim_num;
                    if (at_tok_hasmore(&tmp)) {
                        char prop[5];
                        err = at_tok_nextint(&tmp, &card_type);
                        if (err < 0) goto out;
                        if(card_type == 0) {
                            if(s_sim_num == 0) {
                                property_get(RIL_SIM_TYPE, prop, "0");
                                if(!strcmp(prop, "0"))
                                    property_set(RIL_SIM_TYPE, "1");
                            } else if(s_sim_num == 1) {
                                property_get(RIL_SIM_TYPE1, prop, "0");
                                if(!strcmp(prop, "0"))
                                    property_set(RIL_SIM_TYPE1, "1");
                            }
                        } else if(card_type == 1) {
                            if(s_sim_num == 0) {
                                property_get(RIL_SIM_TYPE, prop, "0");
                                if(!strcmp(prop, "0"))
                                    property_set(RIL_SIM_TYPE, "2");
                            } else if(s_sim_num == 1) {
                                property_get(RIL_SIM_TYPE1, prop, "0");
                                if(!strcmp(prop, "0"))
                                    property_set(RIL_SIM_TYPE1, "2");
                            }
                        } else {
                            if(s_sim_num == 0)
                                property_set(RIL_SIM_TYPE, "0");
                            else if(s_sim_num == 1)
                                property_set(RIL_SIM_TYPE1, "0");
                        }
                    }
                }
#endif
            }
        }
    } else if (strStartsWith(s, "+CBM:")) {
        char *pdu_bin = NULL;

        RILLOGD("Reference-ril. CBM   >>>>>> sss %s ,len  %d", s,strlen(s));
        RILLOGD("Reference-ril. CBM   >>>>>> %s ,len  %d", sms_pdu,strlen(sms_pdu));
#if defined (RIL_SPRD_EXTENSION)
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS,
                sms_pdu, strlen(sms_pdu));
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        pdu_bin = (char *)alloca(strlen(sms_pdu)/2);
        memset(pdu_bin, 0, strlen(sms_pdu)/2);
        if(!convertHexToBin(sms_pdu, strlen(sms_pdu), pdu_bin)) {
            RIL_onUnsolicitedResponse (
                    RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS,
                    pdu_bin, strlen(sms_pdu)/2);
        } else
            RILLOGE("Convert hex to bin failed for SMSCB");
#endif
    } else if (strStartsWith(s, "+CDS:")) {
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
                sms_pdu, strlen(sms_pdu));
    } else if (strStartsWith(s, "+CMGR:")) {
        if (sms_pdu != NULL) {
            RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS, sms_pdu,
                    strlen(sms_pdu));
        } else {
            RILLOGD("[cmgr] sms_pdu is NULL");
        }
    } else if (strStartsWith(s, "+CGEV:")) {
        /* Really, we can ignore NW CLASS and ME CLASS events here,
         * but right now we don't since extranous
         * RIL_UNSOL_DATA_CALL_LIST_CHANGED calls are tolerated
         */
        /* can't issue AT commands here -- call on main thread */
        RIL_requestTimedCallback (onDataCallListChanged, NULL, NULL);
    } else if (strStartsWith(s, "+CMTI:")) {
        /* can't issue AT commands here -- call on main thread */
        int location;
        char *response = NULL;
        char *tmp;
        int *p_index;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("sms request fail");
            goto out;
        }
        if (strcmp(response, "SM")) {
            RILLOGD("sms request arrive but it is not a new sms");
            goto out;
        }

        /* Read the memory location of the sms */
        err = at_tok_nextint(&tmp, &location);
        if (err < 0) {
            RILLOGD("error parse location");
            goto out;
        }
        RILLOGD("[unsl]cmti: location = %d", location);
#if defined (RIL_SPRD_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM, &location, sizeof(location));
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        p_index = (int *)malloc(sizeof(int));
        memcpy(p_index, &location, sizeof(int));
        RIL_requestTimedCallback (onClass2SmsReceived, p_index, NULL);
#endif
    } else if (strStartsWith(s, "+SPUSATENDSESSIONIND")) {
        RILLOGD("[stk unsl]RIL_UNSOL_STK_SESSION_END");
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_SESSION_END, NULL, 0);
    }  else if (strStartsWith(s, "+SPUSATPROCMDIND:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("[stk unsl]RIL_UNSOL_STK_PROACTIVE_COMMAND");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
    } else if (strStartsWith(s, "+SPUSATDISPLAY:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("[stk unsl]RIL_UNSOL_STK_EVENT_NOTIFY");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
#if defined (RIL_SPRD_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_EVENT_NOTIFY, response, strlen(response) + 1);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
#endif
    } else if (strStartsWith(s, "+SPUSATSETUPCALL:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("[stk unsl]RIL_UNSOL_STK_CALL_SETUP");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
#if defined (RIL_SPRD_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_CALL_SETUP, response, strlen(response) + 1);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
#endif
    } else if (strStartsWith(s, "+SPUSATREFRESH:")) {
        char *tmp;
        int result = 0;
        RIL_SimRefreshResponse_v7 *response = NULL;

        RILLOGD("[stk unsl]SPUSATREFRESH");
        response = (RIL_SimRefreshResponse_v7 *)alloca(sizeof(RIL_SimRefreshResponse_v7));
        if (response == NULL) goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextint(&tmp, &result);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->ef_id);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextstr(&tmp, &response->aid);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        response->result = result;
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_REFRESH, response, sizeof(RIL_SimRefreshResponse_v7));
    } else if (strStartsWith(s, "+CSSI:")) {
        RIL_SuppSvcNotification *response = NULL;
        int code = 0;
        char *tmp;

        response = (RIL_SuppSvcNotification *)malloc(sizeof(RIL_SuppSvcNotification));
        if (response == NULL)
            goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextint(&tmp, &code);
        if (err < 0) {
            RILLOGD("%s fail", s);
            free(response);
            goto out;
        }
        response->notificationType = MO_CALL;
        response->code = code;
        response->index = 0;
        response->type = 0;
        response->number = NULL;

        RIL_onUnsolicitedResponse (RIL_UNSOL_SUPP_SVC_NOTIFICATION, response,sizeof(RIL_SuppSvcNotification));
        free(response);
    } else if (strStartsWith(s, "+CSSU:")) {
        RIL_SuppSvcNotification *response = NULL;
        int code = 0;
        char *tmp;

        response = (RIL_SuppSvcNotification *)malloc(sizeof(RIL_SuppSvcNotification));
        if (response == NULL)
            goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextint(&tmp, &code);
        if (err < 0) {
            RILLOGD("%s fail", s);
            free(response);
            goto out;
        }
        response->notificationType = MT_CALL;
        response->code = code;
        response->index = 0;
        response->type = 0;
        response->number = NULL;

        RIL_onUnsolicitedResponse (RIL_UNSOL_SUPP_SVC_NOTIFICATION, response,sizeof(RIL_SuppSvcNotification));
        free(response);
    } else if (strStartsWith(s, "+SPERROR:")) {/*for SS */
        int type;
        char *tmp;

        RILLOGD("SPERROR for SS");
        if (ussdRun != 1)
            return;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &type);
        if (err < 0) goto out;

        RILLOGD("SPERROR type = %d",type);
        if (type == 5) {/* 5: for SS */
            ussdError = 1;
        }

    } else if (strStartsWith(s, "+CUSD:")) {
        char *response[3] = { NULL, NULL, NULL};
        char *tmp;
        char *buf = NULL;

        ussdRun = 0;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &(response[0]));
        if (err < 0) {
            RILLOGE("Error code not present");
            goto out;
        }

        err = at_tok_nextstr(&tmp, &(response[1]));
        if (err == 0) {
            /* Convert the response, which is in the GSM 03.38 [25]
             * default alphabet, to UTF-8
             * Since the GSM alphabet characters contain a character
             * above 2048 (the Euro symbol)
             * the string can theoretically expand by 3/2 in length
             * when converted to UTF-8, so we allocate a new buffer, twice
             * the size of the one holding the hex string.*/
            at_tok_nextstr(&tmp, &(response[2]));
            RIL_onUnsolicitedResponse(
                    RIL_UNSOL_ON_USSD,
                    &response,3*sizeof(char*));
        } else {
            if (ussdError == 1) {/* for ussd */
                RILLOGD("+CUSD ussdError");
                response[0] = "4";//4:network does not support the current operation
                ussdError = 0;
            }

            RIL_onUnsolicitedResponse(
                    RIL_UNSOL_ON_USSD,
                    &response[0], 1*sizeof(char*));
        }
    }
#if defined (RIL_SPRD_EXTENSION)
    else if (strStartsWith(s, AT_PREFIX"DTVTDATA:")) {
        char *response = NULL;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        if (response != NULL) {
            if (!strcmp(response, "0")) {
                static int vt_pipe = -1;
                if (vt_pipe < 0) vt_pipe = open("/dev/rpipe/ril.vt.0", O_RDWR);
                if (vt_pipe > 0) {
                    write(vt_pipe, "0", 2);
                } else {
                    RILLOGE("open vt_pipe failed: %d", vt_pipe);
                    RILLOGE("vt_pipe errno: %d, %s", errno, strerror(errno));
                }
            }
        }

        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_DATA, response, strlen(response) + 1);
    } else if (strStartsWith(s, AT_PREFIX"DVTCODECRI:")) {
        int response[4];
        int index = 0;
        int iLen = 1;
        char *tmp;
        RILLOGD("onUnsolicited(), "AT_PREFIX"DVTCODECRI:");

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response[0]);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        if (3 == response[0]){
            for (index = 1; index <= 3; index++){
                err = at_tok_nextint(&tmp, &response[index]);
                if (err < 0) {
                    RILLOGD("%s fail", s);
                    goto out;
                }
            }
            iLen = 4;
        }

        RILLOGD("onUnsolicited(), "AT_PREFIX"DVTCODECUP:, response[0]: %d", response[0]);
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_CODEC, &response, iLen * sizeof(response[0]));
    } else if (strStartsWith(s, AT_PREFIX"DVTSTRRI:")) {
        char *response = NULL;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_STRING,response,strlen(response) + 1);
    } else if (strStartsWith(s, AT_PREFIX"DVTSENDRI")) {
        int response[3];
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &(response[0]));
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &(response[1]));
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &(response[2]));
        if (err < 0) {
            RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_REMOTE_MEDIA,&response,sizeof(response[0])*2);
        } else {
            RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_REMOTE_MEDIA,&response,sizeof(response));
        }
    } else if (strStartsWith(s, AT_PREFIX"DVTMMTI")) {
        int response;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_MM_RING,&response,sizeof(response));
    } else if (strStartsWith(s, AT_PREFIX"DVTRELEASING")) {
        char *response = NULL;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_RELEASING, response, strlen(response)+1);
    } else if (strStartsWith(s, AT_PREFIX"DVTRECARI")) {
        int response;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_RECORD_VIDEO,&response,sizeof(response));
    }
#endif
    else if (strStartsWith(s, "^DSCI:")) {
        RIL_VideoPhone_DSCI *response = NULL;
        response = (RIL_VideoPhone_DSCI *)alloca(sizeof(RIL_VideoPhone_DSCI));
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response->id);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->idr);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->stat);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->type);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        if(response->type == 0) {
            RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
                NULL, 0);
            goto out;
        } else if(response->type == 1) {
#if defined (RIL_SPRD_EXTENSION)
            RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_VIDEOCALL_STATE_CHANGED,
                NULL, 0);

            err = at_tok_nextint(&tmp, &response->mpty);
            if (err < 0) {
               RILLOGD("%s fail", s);
               goto out;
            }
            err = at_tok_nextstr(&tmp, &response->number);
            if (err < 0) {
                RILLOGD("%s fail", s);
                goto out;
            }
            err = at_tok_nextint(&tmp, &response->num_type);
            if (err < 0) {
                RILLOGD("%s fail", s);
                goto out;
            }
            err = at_tok_nextint(&tmp, &response->bs_type);
            if (err < 0) {
                RILLOGD("%s fail", s);
                goto out;
            }
            if (at_tok_hasmore(&tmp)) {
                err = at_tok_nextint(&tmp, &response->cause);
                if (err < 0) {
                    RILLOGD("%s fail", s);
                    goto out;
                }
                if (at_tok_hasmore(&tmp)) {
                    err = at_tok_nextint(&tmp, &response->location);
                    if (err < 0) {
                        RILLOGD("%s fail", s);
                        goto out;
                    }
                    RILLOGD("onUnsolicited(), ^DSCI:, id: %d, idr: %d, stat: %d, type: %d, mpty: %d, number: %s, num_type: %d, bs_type: %d, cause: %d, location: %d",
                            response->id, response->id, response->stat, response->type, response->mpty, response->number,
                            response->num_type, response->bs_type, response->cause, response->location);
                    RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_DSCI, response, sizeof(RIL_VideoPhone_DSCI));
                } else {
                    response->location = 0;
                    RILLOGD("onUnsolicited(), ^DSCI:, id: %d, idr: %d, stat: %d, type: %d, mpty: %d, number: %s, num_type: %d, bs_type: %d, cause: %d",
                            response->id, response->id, response->stat, response->type, response->mpty, response->number,
                            response->num_type, response->bs_type, response->cause);
                    RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_DSCI, response, sizeof(RIL_VideoPhone_DSCI));
                }
            }
#endif
        }
    }
#if defined (RIL_SPRD_EXTENSION)
    else if (strStartsWith(s,AT_PREFIX"DVTRING:")
            || strStartsWith(s,AT_PREFIX"DVTCLOSED")) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VIDEOCALL_STATE_CHANGED,
            NULL, 0);
    } else if (strStartsWith(s, AT_PREFIX"VTMDSTRT")) {
        int response;
        int index = 0;
        char *tmp;
        RILLOGD("onUnsolicited(), "AT_PREFIX"VTMDSTRT:");

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        RILLOGD("onUnsolicited(), "AT_PREFIX"VTMDSTRT:, response[0]: %d", response);
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_MEDIA_START, &response, sizeof(response));
    }
#endif
    else if (strStartsWith(s, "^SMOF:")) {
	char *tmp;
        int value;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &value);
        if (err < 0) goto out;

        if(value == 2) {
            RILLOGD("[sms]RIL_UNSOL_SIM_SMS_STORAGE_FULL");
            RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_SMS_STORAGE_FULL, NULL, 0);
        }
    }
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    else if (strStartsWith(s, "+SPUSATSMS:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("STK_SEND_SMS");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
    } else if (strStartsWith(s, "+SPUSATSS:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("STK_SEND_SS");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
    } else if (strStartsWith(s, "+SPUSATUSSD:")) {
        char *response = NULL;
        char *tmp;

        RILLOGD("STK_SEND_USSD");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }

        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, strlen(response) + 1);
    } else if (strStartsWith(s, "+SPUSATDISPLAY:")) {
        char *tmp;
        char *data;
        RIL_SSReleaseComplete *response = NULL;

        RILLOGD("RIL_UNSOL_RELEASE_COMPLETE_MESSAGE");
        response = (RIL_SSReleaseComplete *)alloca(sizeof(RIL_SSReleaseComplete));
        if (response == NULL) goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &data);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        response->dataLen = strlen(data);
        response->size = sizeof(RIL_SSReleaseComplete) + response->dataLen;
        response->params = RIL_PARAM_SSDI_STATUS| RIL_PARAM_SSDI_DATA;
        response->status = 0;
        response->data = data;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RELEASE_COMPLETE_MESSAGE, response,sizeof(RIL_SSReleaseComplete));
    } else if (strStartsWith(s, "+SPUSATSMSERR:")) {
        int response;
        char *tmp;

        RILLOGD("RIL_UNSOL_STK_SEND_SMS_RESULT");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        // 0x0000: SMS SEND OK
        // 0x8016: "SMS SEND FAIL - MEMORY NOT AVAILABLE"
        // 0x802A: SMS SEND FAIL RETRY
        // any other value: SMS SEND GENERIC FAIL
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_SEND_SMS_RESULT, &response, sizeof(response));
    } else if (strStartsWith(s, "+SPUSATCALLCTRL:")) {
        char *tmp;
        RIL_StkCallControlResult *response = NULL;;

        RILLOGD("RIL_UNSOL_STK_CALL_CONTROL_RESULT");
        response = (RIL_StkCallControlResult *)alloca(sizeof(RIL_StkCallControlResult));
        if (response == NULL) goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextint(&tmp, &response->call_type);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->result);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->is_alpha);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->alpha_len);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextstr(&tmp, &response->alpha_data);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->pre_type);
        if (err < 0) {
           RILLOGD("%s fail", s);
           goto out;
        }
        err = at_tok_nextint(&tmp, &response->ton);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->npi);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->num_len);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextstr(&tmp, &response->number);
        if (err < 0) {
            RILLOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_CALL_CONTROL_RESULT, response, sizeof(RIL_StkCallControlResult));
    } else if (strStartsWith(s, "+SPSCSQ:")) {
        RIL_SignalStrength_v6 response_v6;
        char *tmp;
        int csq, rat;

        memset(&response_v6, -1, sizeof(RIL_SignalStrength_v6));
        line = strdup(s);
        tmp = line;

        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &csq);
        if (err < 0) goto out;

        err = at_tok_nextint(&tmp, &(response_v6.GW_SignalStrength.bitErrorRate));
        if (err < 0) goto out;

        err = at_tok_nextint(&tmp, &rat);
        if (err < 0) goto out;

        if(rat == 0) {
            /* convert GSM rssi to asu*/
            if(csq <= 5 || csq == 99)
                csq = 1;
            else if(csq <= 7)
                csq = 4;
            else if(csq <= 9)
                csq = 7;
            else if(csq <= 12)
                csq = 10;
            else if(csq > 12)
                csq = 14;
        } else if(rat == 1) {
            /* convert UMTS rssi to asu*/
            if(csq <= 1 || csq == 99)
                csq = 1;
            else if(csq <= 4)
                csq = 4;
            else if(csq <= 7)
                csq = 7;
            else if(csq <= 11)
                csq = 10;
            else if(csq > 11)
                csq = 14;
        } else
            goto out;

        response_v6.GW_SignalStrength.signalStrength = csq;

        RIL_onUnsolicitedResponse(
                RIL_UNSOL_SIGNAL_STRENGTH,
                &response_v6, sizeof(RIL_SignalStrength_v6));
    }
#endif
out:
    free(line);
}

/* Called on command or reader thread */
static void onATReaderClosed()
{
    int i=0;
    int channelID;
    int channel_nums;

    RILLOGI("AT channel closed\n");
    if(s_multiSimMode)
        channel_nums = MULTI_MAX_CHANNELS;
    else
        channel_nums = MAX_CHANNELS;

    for(i = 0; i < channel_nums; i++) {
        at_close(ATch_type[i]);
    }
    stop_reader();
    s_closed = 1;
    channelID = getChannel();

    setRadioState (channelID, RADIO_STATE_UNAVAILABLE);
    putChannel(channelID);
}

/* Called on command thread */
static void onATTimeout()
{
    int  i;
    int channelID;
    int channel_nums;

    RILLOGI("AT channel timeout; closing\n");
    if(s_multiSimMode)
        channel_nums = MULTI_MAX_CHANNELS;
    else
        channel_nums = MAX_CHANNELS;

    for(i = 0; i < channel_nums; i++) {
        at_close(ATch_type[i]);
    }
    stop_reader();

    s_closed = 1;

    /* FIXME cause a radio reset here */
    channelID = getChannel();

    setRadioState (channelID, RADIO_STATE_UNAVAILABLE);
    putChannel(channelID);
}

static void usage(char *s)
{
#ifdef RIL_SHLIB
    fprintf(stderr, "reference-ril requires: -p <tcp port> or -d /dev/tty_device\n");
#else
    fprintf(stderr, "usage: %s [-p <tcp port>] [-d /dev/tty_device]\n", s);
    exit(-1);
#endif
}


static void *
mainLoop(void *param)
{
    int fd,  i;
    struct ATChannels *ret;
    int sim_num = 0;
    char str[15];
    int sim_save = 0;
    int channel_nums;
    struct channel_description *descriptions;

    if(param)
        sim_num= *((int*)param);

    if(s_multiSimMode) {
        descriptions = multi_descriptions;
        channel_nums = MULTI_MAX_CHANNELS;
    } else {
        descriptions = single_descriptions;
        channel_nums = MAX_CHANNELS;
    }

    AT_DUMP("== ", "entering mainLoop()", -1 );
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);
    if(s_multiSimMode) {
        sim_num = 3*sim_num;
        sim_save = sim_num;
    }

    for (;;) {
        fd = -1;
        s_closed = 0;
        init_channels();
        sem_wait(&w_sem);
again:
        sim_num = sim_save;
        for(i = 0; i < channel_nums; i++)
        {
            if(!strcmp(s_modem, "t")) {
                sprintf(str,"/dev/CHNPTYT%d",sim_num);
            } else if(!strcmp(s_modem, "w")) {
                sprintf(str,"/dev/CHNPTYW%d",sim_num);
            } else {
                RILLOGE("Invalid tty device");
                exit(-1);
            }
            strcpy(descriptions[i].ttyName , str);

            /* open TTY device, and attach it to channel */
            if(s_multiSimMode) {
                descriptions[i].channelID = sim_num;
                sprintf(str,"Channel%d",sim_num);
                strcpy(descriptions[i].name , str);
            }

            fd = open (descriptions[i].ttyName, O_RDWR | O_NONBLOCK);

            if ( fd >= 0 ) {
                /* disable echo on serial ports */
                struct termios  ios;
                tcgetattr(fd, &ios);
                ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
                tcsetattr(fd, TCSANOW, &ios );
                descriptions[i].fd = fd;
                RILLOGI ("AT channel [%d] open successfully, ttyName:%s", i, descriptions[i].ttyName );
            } else {
                perror("Opening AT interface. retrying...");
                sleep(1);
                goto again;
            }
            ATch_type[i] = at_open(fd, i, descriptions[i].name, onUnsolicited);


            if (ATch_type[i] ==NULL){
                RILLOGE ("AT error on at_open\n");
                return 0;
            }
            ATch_type[i]->nolog = 0;

            sim_num++;
        }

        s_channel_open = 1;

        start_reader();

        RIL_requestTimedCallback(initializeCallback, NULL, &TIMEVAL_0);

        /* Give initializeCallback a chance to dispatched, since
         * we don't presently have a cancellation mechanism */
        sleep(1);

        waitForClose();
        RILLOGI("Re-opening after close");
    }
}

#ifdef RIL_SHLIB

pthread_t s_tid_mainloop;
int s_sim_num;

const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
    int ret;
    int fd = -1;
    int opt;
    pthread_attr_t attr;
    char phoneCount[PROPERTY_VALUE_MAX];
    char prop[PROPERTY_VALUE_MAX];

    s_rilenv = env;

    while ( -1 != (opt = getopt(argc, argv, "m:n:"))) {
        switch (opt) {
            case 'm':
                s_modem = optarg;
                break;
            case 'n':
                s_sim_num = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    modem = *s_modem;
    RILLOGD("rild connect %s modem, current is rild%d\n", s_modem, s_sim_num);

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
    } else {
        usage(argv[0]);
    }

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    sem_init(&w_sem, 0, 1);
    if(s_multiSimMode) {
        simNum = s_sim_num;
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        property_get(RIL_SIM1_ABSENT_PROPERTY, prop, "0");
        if(!strcmp(prop, "1")) {
            simNum = s_sim_num>0 ? 0 : 1;
        }
#endif
        ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, &simNum);
        RILLOGD("RIL enter multi sim card mode!");
    } else {
        ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, NULL);
        RILLOGD("RIL enter single sim card mode!");
    }
    return &s_callbacks;
}
#else /* RIL_SHLIB */
int main (int argc, char **argv)
{
    int ret;
    int fd = -1;
    int opt;

    while ( -1 != (opt = getopt(argc, argv, "p:d:"))) {
        switch (opt) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port == 0) {
                    usage(argv[0]);
                }
                RILLOGI("Opening loopback port %d\n", s_port);
                break;

            case 'd':
                s_device_path = optarg;
                RILLOGI("Opening tty device %s\n", s_device_path);
                break;

            case 's':
                s_device_path   = optarg;
                s_device_socket = 1;
                RILLOGI("Opening socket %s\n", s_device_path);
                break;

            default:
                usage(argv[0]);
        }
    }

    if (s_port < 0 && s_device_path == NULL) {
        usage(argv[0]);
    }

    RIL_register(&s_callbacks);

    mainLoop(NULL);

    return 0;
}

#endif /* RIL_SHLIB */
