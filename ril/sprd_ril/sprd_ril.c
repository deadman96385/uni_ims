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

#define SIM_MODE_PROPERTY  "persist.msms.phone_count"
#define RIL_SIM_POWER_OFF_PROPERTY  "sys.power.off"
#define RIL_SIM_POWER_PROPERTY  "ril.sim.power"
#define RIL_SIM_POWER_PROPERTY1  "ril.sim.power1"
#define RIL_MAIN_SIM_PROPERTY  "persist.msms.phone_default"
#define RIL_ASSERT  "ril.assert"
#define RIL_SIM_PIN_PROPERTY  "ril.sim.pin"
#define RIL_SIM_PIN_PROPERTY1  "ril.sim.pin1"
#define RIL_MODEM_RESET_PROPERTY "persist.sys.sprd.modemreset"
#define RIL_STK_PROFILE_PREFIX  "ril.stk.proflie_"
#define RIL_SIM0_STATE  "gsm.sim.state0"
#define RIL_SIM_ABSENT_PROPERTY "persist.sys.sim.absence"

int s_dualSimMode = 0;

struct ATChannels *ATch_type[MAX_CHANNELS];
static char creg_response[100] = "";
static char cgreg_response[100] = "";
static int s_channel_open = 0;
static int s_sms_ready = 0;

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
    { 0, -1, "Unsol", "/dev/CHNPTY0", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 1, -1, "Channel1", "/dev/CHNPTY1", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 2, -1, "Channel2", "/dev/CHNPTY2", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
    { 3, -1, "Channel3", "/dev/CHNPTY3", CHANNEL_IDLE, PTHREAD_MUTEX_INITIALIZER},
};

struct channel_description dual_descriptions[DUAL_MAX_CHANNELS] = {
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
static void putChannel(int channel);
static int getChannel();
static int getSmsChannel();
extern const char * requestToString(int request);
static void requestSetupDataCall(int channelID, void *data, size_t datalen, RIL_Token t);
static void getSmsState(int channelID) ;

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
    ALOGD("put pdp[%d]", cid);
    ALOGD("pdp[0].state = %d, pdp[1].state = %d,pdp[2].state = %d", pdp[0].state, pdp[1].state, pdp[2].state);
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
            ALOGD("get pdp[%d]", ret);
            ALOGD("pdp[0].state = %d, pdp[1].state = %d,pdp[2].state = %d", pdp[0].state, pdp[1].state, pdp[2].state);
            return ret;
        }
        pthread_mutex_unlock(&pdp[i].mutex);
    }
    return ret;
}

#if 0
#define WRITE_PPP_OPTION(option) write(fd, option, strlen(option))
#endif

static void wait4android_audio_ready(const char *prefix)
{
#if 1
    char buf[128];
    static int android_modem_mode = -1;
    static int ril_audio_ipc_0 = -1, ril_audio_ipc_1 = -1;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    struct timeval tv;
    int retval;
    fd_set rfds;
#define RIL_AUDIO_MODEM_IPC_TIMEOUT   (10*1000) /* 10 seconds */
    int do_ril_sync = 1;
    ALOGW("< %s > wait4android_audio_ready start.", prefix);
    pthread_mutex_lock(&lock);
    if (android_modem_mode < 0) android_modem_mode = open("/sys/class/modem/mode", O_RDWR);
    if (android_modem_mode >= 0) {
        do {
			buf[0] = -1;
            lseek(android_modem_mode, 0, SEEK_SET);
            read(android_modem_mode, buf, sizeof(buf) - 1);
            // if (strncmp(buf, "waiting", 7)) break;
			if (strtol(buf, NULL, 10) != 4/* AudioSystem::MODE_WAITING */) break;
            ALOGW("\n\n\n\n!!!!!!!!!! My God !!! your speed is so quick !!!!!!!!!!\n\n\n\n");
            usleep(100*1000);
        } while (1);
        if ((strncmp(buf, "incall", 6) == 0) &&
                ((strncmp(prefix, "ATD", 3) == 0) || (strncmp(prefix, "ATA", 3) == 0))) do_ril_sync = 0;
        ALOGW("1111111111111111 %s %s 1111111111111111", buf, prefix);
    } else ALOGW("< %s > Failed to open /sys/class/modem/mode", prefix);
    if (do_ril_sync) {
        if (ril_audio_ipc_0 < 0) ril_audio_ipc_0 = open("/dev/pipe/ril.audio.0", O_RDWR);
        if (ril_audio_ipc_1 < 0) ril_audio_ipc_1 = open("/dev/pipe/ril.audio.1", O_RDWR);
        if (ril_audio_ipc_0 >= 0 && ril_audio_ipc_1 >= 0) {
            ALOGW("< %s > write ril ipc to /dev/pipe/ril.audio.0", prefix);
            write(ril_audio_ipc_0, "ril->audio : sync start", 24);
            FD_ZERO(&rfds);
            FD_SET(ril_audio_ipc_1, &rfds);
            tv.tv_sec = RIL_AUDIO_MODEM_IPC_TIMEOUT / 1000;
            tv.tv_usec = (RIL_AUDIO_MODEM_IPC_TIMEOUT % 1000) * 1000;
            retval = select(ril_audio_ipc_1 + 1, &rfds, NULL, NULL, &tv);
            if (retval == -1) {
                ALOGW("================== [ BUG pipe error ] ==================");
            } else if (retval) {
                ALOGW("< %s > read audio ipc from /dev/pipe/ril.audio.1", prefix);
                read(ril_audio_ipc_1, buf, sizeof(buf) - 1);
                ALOGW("< %s > %s", buf, prefix);
            } else {
                ALOGW("================== [ ril pipe %dms timeout, so force passed ] ==================", RIL_AUDIO_MODEM_IPC_TIMEOUT);
            }
        } else {
            ALOGW("< %s > Failed to open /dev/pipe/ril.audio.0 & /dev/pipe/ril.audio.1", prefix);
        }
    } else {
        ALOGW("< %s > you are ATD or ATA another call, ignore ril <-> audio sync signal", prefix);
    }
    pthread_mutex_unlock(&lock);
    ALOGW("< %s > wait4android_audio_ready done.", prefix);
#else
    char buf[16];
    int android_fd;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    android_fd = open("/sys/class/modem/mode", O_RDWR);
    pthread_mutex_lock(&lock);
    if (android_fd >= 0) {
        read(android_fd, buf, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        close(android_fd);
        if (strncmp(buf, "ringtone", 8) == 0) {
            /* in ringtone, we should do some sync with alsa */
            ALOGW("!!! syncing with android audio !!!");
            property_set("ril.modem.audio.controling", "1");
            usleep(1000);
            for (;;) {
                property_get("ril.modem.audio.controling", buf, "1");
                if (buf[0] == '0') {
                    ALOGW("--- synced ---");
                    break;
                }
                ALOGW("--- syncing ---");
                usleep(120*1000);
            }
        }
    }
    pthread_mutex_unlock(&lock);
#endif
}

static void wait4modem_audio_release(const char *prefix)
{
    static int ril_audio_ipc_2 = -1;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    ALOGW("< %s > wait4modem_audio_release start.", prefix);
    pthread_mutex_lock(&lock);
    if (ril_audio_ipc_2 < 0) ril_audio_ipc_2 = open("/dev/pipe/ril.audio.2", O_RDWR);
    if (ril_audio_ipc_2 >= 0) {
        ALOGW("< %s > write ril ipc to /dev/pipe/ril.audio.2", prefix);
        write(ril_audio_ipc_2, "ril->audio : modem has released audio codec", 44);
    } else {
        ALOGW("< %s > Failed to open /dev/pipe/ril.audio.2", prefix);
    }
    pthread_mutex_unlock(&lock);
    ALOGW("< %s > wait4modem_audio_release done.", prefix);
}

static void process_calls(int _calls)
{
    static int calls = 0;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    char buf[3];
    int incall_recording_status;

    if (calls && _calls == 0) {
        pthread_mutex_lock(&lock);
        ALOGW("################## < vaudio > This is the Last PhoneCall ##################");
        /*
         * The Last PhoneCall is really terminated,
         * audio codec is freed by Modem side completely [luther.ge]
         *
         */
        incall_recording_status = open("/proc/vaudio/close", O_RDWR);
        if (incall_recording_status >= 0) {
            memset(buf, 0, sizeof buf);
            read(incall_recording_status, buf, 3);
            ALOGW("################## < vaudio > %sincall recording ##################[%s]",
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
    ALOGW("\n\nThere will be no call, so mute speaker now to avoid noise pop sound\n\n");
    /* Remove handsfree pop noise sound [luther.ge] */
    system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
}

static inline int all_calls(int channelID, int do_mute)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int countCalls;

    if (at_send_command_multiline(ATch_type[channelID], "AT+CLCC", "+CLCC:", &p_response) != 0 ||
            p_response->success == 0)
        return -1;

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
    char *cmd;

    cid_ptr = ((const char **)data)[0];
    cid = atoi(cid_ptr);

    ALOGD("Try to deactivated modem ..., cid=%d", cid);
    if (pdp[cid - 1].cid == cid) {
        asprintf(&cmd, "AT+CGACT=0,%d", cid);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;
        free(cmd);

        putPDP(cid - 1);
    }

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    if (pdp[cid - 1].cid == cid)
        putPDP(cid - 1);
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
    ALOGE("invalid CLCC line\n");
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
    ALOGE("invalid CCFC line\n");
    return -1;
}

static void requestCallForward(int channelID, RIL_CallForwardInfo *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    int err;
    int errNum = 0xff;
    char *cmd, *line;

    if (datalen != sizeof(*data))
        goto error1;
    if (data->serviceClass == 0) {
        if (data->status == 2) {
            asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",129",
                data->reason,
                data->status,
                data->number ? data->number : "");
        } else {
            if (data->timeSeconds != 0 && data->status == 3) {
                asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,,\"\",,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->timeSeconds);

            } else {
                asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa);
            }
        }
    } else {
        if (data->status == 2) {
            asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",129,%d",
                data->reason,
                data->status,
                data->number ? data->number : "",
                data->serviceClass);
        } else {
            if (data->timeSeconds != 0 && data->status == 3) {
                asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d,\"\",,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->serviceClass,
                        data->timeSeconds);
            } else {
                asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d",
                        data->reason,
                        data->status,
                        data->number ? data->number : "",
                        data->toa,
                        data->serviceClass);
            }
        }
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
            if (errNum == 70 || errNum == 3)
                RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
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
    char *cmd, *line;

    if (datalen != 5 * sizeof(char *))
        goto error;

    serviceClass = atoi(data[3]);
    if (serviceClass == 0) {
        asprintf(&cmd, "AT+CLCK=\"%s\",%c,\"%s\"",
                data[0], *data[1], data[2]);
    } else {
        asprintf(&cmd, "AT+CLCK=\"%s\",%c,\"%s\",%s",
                data[0], *data[1], data[2], data[3]);
    }
    ALOGD("requestFacilityLock: %s", cmd);

    if (*data[1] == '2')
        err = at_send_command_singleline(ATch_type[channelID],  cmd, "+CLCK: ",
                &p_response);
    else {
        err = at_send_command(ATch_type[channelID],  cmd, &p_response);
        if (err < 0 || p_response->success == 0) {
            free(cmd);
            goto error;
        }
        result = 1;
        free(cmd);
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
        at_response_free(p_response);
        return;
    }
    free(cmd);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[0]);
    if (err < 0) goto error;
    if (at_tok_hasmore(&line)) {
        err = at_tok_nextint(&line, &response[1]);
        if (err < 0) goto error;
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
    at_response_free(p_response);
    return;

error:
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

    asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", data[0], data[1], data[2]);
    ALOGD("requestChangeFacilityLock: %s", cmd);

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
    int autoAttach;
    char prop[10];
    int err, i;
    ATResponse *p_response = NULL;

    assert (datalen >= sizeof(int *));
    onOff = ((int *)data)[0];
    autoAttach = ((int *)data)[1];

    if (onOff == 0) {
        /*clear the array to ensure strcmp(creg_response/cgreg_response, s)
         * is different between enter and exit airfly mode
         * due to modem is not stable to unsolicited +CREG +CGREG
         */

        memset(creg_response, 0, sizeof(creg_response));
        memset(cgreg_response, 0, sizeof(cgreg_response));

        /* The system ask to shutdown the radio */
        err = at_send_command(ATch_type[channelID], "AT+SFUN=5", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;

        for(i = 0; i < MAX_PDP; i++) {
            if (pdp[i].cid > 0) {
                ALOGD("pdp[%d].state = %d", i, pdp[i].state);
                putPDP(i);
            }
        }

        setRadioState(channelID, RADIO_STATE_OFF);
    } else if (onOff > 0 && sState == RADIO_STATE_OFF) {
        if(s_dualSimMode) {
            if(autoAttach == 1)
                err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", &p_response);
            else
                err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=0", &p_response);
            if (err < 0 || p_response->success == 0)
                ALOGE("GPRS auto attach failed!");
        } else {
            err = at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", &p_response);
            if (err < 0 || p_response->success == 0)
                ALOGE("GPRS auto attach failed!");
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
    int err, channelID, skip;
    char *cmd, *line;
    int index = *((int*)param);
    ATResponse *p_response = NULL;
    int sim_type;
    char *file_path, *sms_pdu;

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

    if(sim_type == 1)  //sim
        asprintf(&file_path, "3F007F10");
    else if(sim_type == 2)  //usim
	asprintf(&file_path, "3F007FFF");
    else
	goto error;

    asprintf(&cmd, "AT+CRSM=178,28476,%d,4,176,0,\"%s\"", index, file_path);
    err = at_send_command_singleline(ATch_type[channelID], cmd, "+CRSM:", &p_response);
    putChannel(channelID);
    free(cmd);
    free(file_path);
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
    sms_pdu += 1;  //skip the status byte
    RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS,
                sms_pdu, strlen(sms_pdu));
     at_response_free(p_response);
    return;
error:
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
        char *cmd;
        char prop[20];
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

        asprintf(&cmd, "veth%d", i);
        responses[i].ifname = alloca(strlen(cmd) + 1);
        strcpy(responses[i].ifname, cmd);
        free(cmd);

        asprintf(&cmd, "net.veth%d.ip", i);
        property_get(cmd, prop, NULL);
        responses[i].addresses = alloca(strlen(prop) + 1);
        responses[i].gateways = alloca(strlen(prop) + 1);
        strcpy(responses[i].addresses, prop);
        strcpy(responses[i].gateways, prop);
        free(cmd);

        dnslist[0] = 0;
        for (nn = 0; nn < 2; nn++) {
            asprintf(&cmd, "net.veth%d.dns%d", i, nn+1);
            property_get(cmd, prop, NULL);

            /* Append the DNS IP address */
            strlcat(dnslist, separator, dnslist_sz);
            strlcat(dnslist, prop, dnslist_sz);
            separator = " ";
            free(cmd);
        }
        responses[i].dnses = dnslist;

        ALOGD("status=%d",responses[i].status);
        ALOGD("suggestedRetryTime=%d",responses[i].suggestedRetryTime);
        ALOGD("cid=%d",responses[i].cid);
        ALOGD("active = %d",responses[i].active);
        ALOGD("type=%s",responses[i].type);
        ALOGD("ifname = %s",responses[i].ifname);
        ALOGD("address=%s",responses[i].addresses);
        ALOGD("dns=%s",responses[i].dnses);
        ALOGD("gateways = %s",responses[i].gateways);
    }

    at_response_free(p_response);

    for(i = 0; i < n; i++) {
        if(responses[i].active == 1)
            count++;
    }

    if(cid > 0) {
        ALOGD("requestOrSendDataCallList is called by SetupDataCall!");
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

static void requestSetupDataCall(int channelID, void *data, size_t datalen, RIL_Token t)
{
    const char *apn = NULL;
    const char *username = NULL;
    const char *password = NULL;
    const char *authtype = NULL;
    char *cmd;
    int err;
    char response[20];
    char * responseStr[3];
    ATResponse *p_response = NULL;
    int index = -1;
    char qos_state[10];
    char *line = NULL;
    const  char *pdp_type;

    apn = ((const char **)data)[2];
    username = ((const char **)data)[3];
    password = ((const char **)data)[4];
    authtype = ((const char **)data)[5];

    ALOGD("requestSetupDataCall data[0] '%s'", ((const char **)data)[0]);
    ALOGD("requestSetupDataCall data[1] '%s'", ((const char **)data)[1]);
    ALOGD("requestSetupDataCall data[2] '%s'", ((const char **)data)[2]);
    ALOGD("requestSetupDataCall data[3] '%s'", ((const char **)data)[3]);
    ALOGD("requestSetupDataCall data[4] '%s'", ((const char **)data)[4]);
    ALOGD("requestSetupDataCall data[5] '%s'", ((const char **)data)[5]);
    ALOGD("requestSetupDataCall data[6] '%s'", ((const char **)data)[4]);
    ALOGD("requestSetupDataCall data[7] '%s'", ((const char **)data)[5]);


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

        asprintf(&cmd, "AT+CGACT=0,%d", index+1);
        err = at_send_command(ATch_type[channelID], cmd, NULL);
        free(cmd);

        asprintf(&cmd, "AT+CGDCONT=%d,\"%s\",\"%s\",\"\",0,0", index+1, pdp_type, apn);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err < 0 || p_response->success == 0){
            free(cmd);
            s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
            goto error;
        }
        free(cmd);

        asprintf(&cmd, "AT+CGPCO=0,\"%s\",\"%s\",%d,%d", username, password,index+1,atoi(authtype));
        err = at_send_command(ATch_type[channelID], cmd, NULL);
        free(cmd);

        /* Set required QoS params to default */
        property_get("persist.sys.qosstate", qos_state, "0");
        if(!strcmp(qos_state, "0")) {
            asprintf(&cmd, "AT+CGEQREQ=%d,2,0,0,0,0,2,0,\"1e4\",\"0e0\",3,0,0", index+1);
            err = at_send_command(ATch_type[channelID], cmd, NULL);
            free(cmd);
        }

        asprintf(&cmd, "AT+CGDATA=\"PPP\",%d", index+1);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        free(cmd);
        if (err < 0 || p_response->success == 0) {
            if (strStartsWith(p_response->finalResponse,"+CME ERROR:")) {
                line = p_response->finalResponse;
                err = at_tok_start(&line);
                if (err >= 0)
                    err = at_tok_nextint(&line,&s_lastPdpFailCause);
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
    free(responseStr[0]);
    free(responseStr[1]);
    free(responseStr[2]);

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

    switch(s_lastPdpFailCause) {
        case 534:
            response = PDP_FAIL_OPERATOR_BARRED;  /* no retry */
            break;
        case 533:
            response = PDP_FAIL_MISSING_UKNOWN_APN; /* no retry */
            break;
        case 15:
            response = PDP_FAIL_UNKNOWN_PDP_ADDRESS_TYPE; /* no retry */
            break;
        case 149:
            response = PDP_FAIL_USER_AUTHENTICATION; /* no retry */
            break;
        case 132:
            response = PDP_FAIL_SERVICE_OPTION_NOT_SUPPORTED;  /* no retry */
            break;
        case 133:
            response = PDP_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED;    /* no retry */
            break;
        case 134:
            response = PDP_FAIL_SERVICE_OPTION_OUT_OF_ORDER;  /* no retry */
            break;
        case 21:
            response = PDP_FAIL_NSAPI_IN_USE;  /* no retry */
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 22:
        case 23:
        case 24:
        case 25:
        case 535:
            response = PDP_FAIL_PROTOCOL_ERRORS;     /* no retry */
            break;
        case 101:
        case 102:
            response = PDP_FAIL_INSUFFICIENT_RESOURCES;   /* retry */
            break;
        default:
            response = PDP_FAIL_ERROR_UNSPECIFIED;  /* retry */
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
            sizeof(int));
}

void requestLastCallFailCause(int channelID, void *data, size_t datalen, RIL_Token t)
{
    int response = CALL_FAIL_ERROR_UNSPECIFIED;

    pthread_mutex_lock(&s_call_mutex);
    switch(call_fail_cause) {
        case 1:
        case 3:
        case 22:
        case 28:
            response = CALL_FAIL_UNOBTAINABLE_NUMBER;
            break;
        case 0:
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
        ALOGE("requestBasebandVersion:Send command error!");
        goto error;
    }
    for (i=0,p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next,i++) {

        line = p_cur->line;
        if(i < 2)
            continue;
        if(i < 4) {
            if(at_tok_start(&line) == 0) {
                skipWhiteSpace(&line);
                if(i == 0)
                    strcpy(response, line);
                else
                    strcat(response, line);

                strcat(response,"|");
            } else
                continue;
        } else {
            skipWhiteSpace(&line);
            strcat(response, line);
        }
    }
    if(response == NULL) {
        ALOGE("requestBasebandVersion: Parameter parse error!");
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, strlen(response));
    at_response_free(p_response);
    return;

error:
    ALOGE("ERROR: requestBasebandVersion failed\n");
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
    ALOGD("Searched available network list numbers = %d", count - 2);
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

        err = at_tok_nextint(&line, &act);
        if (err < 0) continue;

        cur[4] = actStr[act];

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
    ALOGE("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestNetworkRegistration(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    char *network;
    char *cmd;
    int err;
    network = (char *)data;
    ATResponse *p_response = NULL;

    if (network) {
        asprintf(&cmd, "AT+COPS=1,2,\"%s\"", network);
        err = at_send_command(ATch_type[channelID], cmd, &p_response);
        if (err != 0 || p_response->success == 0)
            goto error;
    } else
        goto error;

    free(cmd);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    free(cmd);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void receiveSMS(void *param)
{
    int location = (int)param;
    char *cmd;
    int channelID;

    channelID = getChannel();
    ALOGD("[cmti]receiveSMS channelID = %d", channelID);

    if(channelID == 0) {
        ALOGD("channel is busy");
        RIL_requestTimedCallback (receiveSMS, param, &TIMEVAL_SIMPOLL);
    } else {
        asprintf(&cmd, "AT+CMGR=%d", location);
        /* request the sms in a specific location */
        at_send_command(ATch_type[channelID],  cmd, NULL);
        free(cmd);
    }
    putChannel(channelID);
    return;
}

static void resetModem(void * param)
{
    int channelID;

    channelID = getChannel();
    ALOGD("resetModem channelID = %d", channelID);
    if(channelID == 0) {
        ALOGD("resetModem channel is busy");
        RIL_requestTimedCallback (resetModem, NULL, &TIMEVAL_SIMPOLL);
    } else {
	char modemrst_property[8];

        memset(modemrst_property, 0, sizeof(modemrst_property));
        property_get(RIL_MODEM_RESET_PROPERTY, modemrst_property, "");
        ALOGD("%s is %s", RIL_MODEM_RESET_PROPERTY, modemrst_property);
        property_set(RIL_MODEM_RESET_PROPERTY, "1");
        at_send_command(ATch_type[channelID], "AT+SPATASSERT=1", NULL);
    }
    putChannel(channelID);
    return;
}

static void onSimAbsent(void *param)
{
    int channelID;
    char prop[10];

    if(sState == RADIO_STATE_SIM_READY) {
        channelID = getChannel();
        property_get(RIL_SIM_ABSENT_PROPERTY, prop, "0");
        if(!strcmp(prop, "1"))
            at_send_command(ATch_type[channelID], "AT+SPATASSERT=1", NULL);
        else
            setRadioState(channelID, RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        putChannel(channelID);
    }
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
    if (countValidCalls) {
    /* We don't seem to get a "NO CARRIER" message from
     * smd, so we're forced to poll until the call ends.
     */
#else
        if (needRepoll) {
#endif
            if (bVideoCall == 0){
                RIL_requestTimedCallback (sendCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
            } else {
                RIL_requestTimedCallback (sendVideoCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
            }
        }

        return;
error:
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        at_response_free(p_response);
}

static void requestDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    int ret;

    p_dial = (RIL_Dial *)data;

    ALOGD("requestDial isStkCall = %d", s_isstkcall);
    if (s_isstkcall == 1) {
        ALOGD(" setup STK call ");
        s_isstkcall = 0;
        wait4android_audio_ready("ATD");
        asprintf(&cmd, "AT+SPUSATCALLSETUP=1");
        at_send_command(ATch_type[channelID], cmd, NULL);
        free(cmd);
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        return;
    }

    switch (p_dial->clir) {
        case 1: clir = "I"; break;  /*invocation*/
        case 2: clir = "i"; break;  /*suppression*/
        default:
        case 0: clir = ""; break;   /*subscription default*/
    }

    asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);
    wait4android_audio_ready("ATD");
    ret = at_send_command(ATch_type[channelID], cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
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

    p_args = (RIL_SMS_WriteArgs *)data;

    length = strlen(p_args->pdu)/2;

    smsc = (const char *)(p_args->smsc);
    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    asprintf(&cmd, "AT+CMGW=%d,%d", length, p_args->status);
    asprintf(&cmd1, "%s%s", smsc, p_args->pdu);

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
    char *cmd;
    ATResponse *p_response = NULL;

    p_line = (int *)data;

    /* 3GPP 22.030 6.5.5
     * "Releases a specific active call X"
     */
    asprintf(&cmd, "AT+CHLD=7%d", p_line[0]);
    all_calls(channelID, 1);
    ret = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (ret < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
    free(cmd);
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
    ALOGE("requestSignalStrength must never return an error when radio is on");
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
    char *responseStr[4] = { NULL, NULL, NULL, NULL };
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line, *p;
    int commas;
    int skip;
    int count = 1;
    int para,i;

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
            count = 1;
            break;

        case 1: /* +CREG: <n>, <stat> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            count = 1;
            break;

        case 2: /* +CREG: <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            count = 3;
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
            count = 3;
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
            err = at_tok_nexthexint(&line, &response[3]);
            if (err < 0) goto error;
            count = 4;
            break;
        default:
            goto error;
    }

    asprintf(&responseStr[0], "%d", response[0]);
    if (response[1] != -1) {
        asprintf(&responseStr[1], "%x", response[1]);
    }
    if (response[2] != -1) {
        asprintf(&responseStr[2], "%x", response[2]);
    }

    if (count == 4) {
        response[3] = mapCgregResponse(response[3]);
        asprintf(&responseStr[3], "%d", response[3]);
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, count*sizeof(char*));
    at_response_free(p_response);
    free(responseStr[0]);
    if (responseStr[1] != NULL)
        free(responseStr[1]);

    if (responseStr[2] != NULL)
        free(responseStr[2]);

    if (responseStr[3] != NULL)
        free(responseStr[3]);

    return;
error:
    ALOGE("requestRegistrationState must never return an error when radio is on");
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

        ALOGD("Neighbor cell_id %s = %d", NeighboringCell[current].cid,
                NeighboringCell[current].rssi);

        NeighboringCellList[current] = &NeighboringCell[current];
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NeighboringCellList,
            cell_id_number * sizeof(RIL_NeighboringCell *));
    at_response_free(p_response);
    return;
error:
    ALOGD("requestNeighboaringCellIds fail");
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
    ALOGE("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);

}

static void requestSetCmms(int channelID, void *data, size_t datalen, RIL_Token t)
{
    char* cmd;
    int enable = ((int *)data)[0];

    asprintf(&cmd, "AT+CMMS=%d",enable);
    at_send_command( ATch_type[channelID], cmd, NULL);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    free(cmd);
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

    memset(&response, 0, sizeof(RIL_SMS_Response));
    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    asprintf(&cmd2, "%s%s", smsc, pdu);

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

    memset(&response, 0, sizeof(RIL_SMS_Response));
    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    /* "NULL for default SMSC" */
    if (smsc == NULL) {
        smsc= "00";
    }

    asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    asprintf(&cmd2, "%s%s", smsc, pdu);

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
        ALOGE("unsupported arg to RIL_REQUEST_SMS_ACKNOWLEDGE\n");
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

        ALOGI("Reference-ril. requestSIM_IO pin2 %s",p_args->pin2);
    }
    if (p_args->data == NULL) {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,%c,\"%s\"",
                p_args->command, p_args->fileid,
                p_args->p1, p_args->p2, p_args->p3,pad_data,p_args->path);

    } else {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                p_args->command, p_args->fileid,
                p_args->p1, p_args->p2, p_args->p3, p_args->data,p_args->path);
    }

    err = at_send_command_singleline(ATch_type[channelID], cmd, "+CRSM:", &p_response);

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
    free(cmd);

    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);

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

static void  requestScreeState(int channelID, int status, RIL_Token t)
{
    int err;
    int stat;

    pthread_mutex_lock(&s_screen_mutex);
    if (!status) {
        /* Suspend */
        at_send_command(ATch_type[channelID], "AT+CCED=2,8", NULL);
    } else {
        /* Resume */
        at_send_command(ATch_type[channelID], "AT+CCED=1,8", NULL);
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

    if ( datalen == 2*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=%s", strings[0]);
    } else if ( datalen == 3*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=%s,%s", strings[0], strings[1]);
    } else
        goto error;

    err = at_send_command(ATch_type[channelID], cmd, &p_response);

    free(cmd);

    if (err < 0 || p_response->success == 0) {
        goto error;
    } else {
        /* add for modem reboot */
        const char *pin = NULL;
        extern int s_sim_num;
        if ( datalen == sizeof(char*) ) {
            pin = strings[0];
        } else if ( datalen == 2*sizeof(char*) ) {
            pin = strings[1];
        } else
            goto out;

        if(s_dualSimMode) {
            if(s_sim_num == 0) {
                if(pin != NULL)
                    property_set(RIL_SIM_PIN_PROPERTY, pin);
            } else if (s_sim_num == 1) {
                if(pin != NULL)
                    property_set(RIL_SIM_PIN_PROPERTY1, pin);
            }
        } else {
            if(pin != NULL)
                property_set(RIL_SIM_PIN_PROPERTY, pin);
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
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;

    if ( datalen == 3*sizeof(char*) ) {
        asprintf(&cmd, "ATD**052*%s*%s*%s#",strings[0],strings[1],strings[1]);
    } else
        goto error;

    err = at_send_command(ATch_type[channelID], cmd, &p_response);

    free(cmd);

    if (err < 0 || p_response->success == 0) {
error:
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        if(getSIMStatus(channelID) == SIM_READY) {
            setRadioState(channelID, RADIO_STATE_SIM_READY);
        }
    }
    at_response_free(p_response);

}


static void  requestEnterSimPin(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;


    if ( datalen == 3*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPWD=\"SC\",\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
error:
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);

}

static void  requestEnterSimPin2(int channelID, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;

    if ( datalen == 3*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPWD=\"P2\",\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
error:
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);

}

static int setSmsBroadcastConfigValue(int value ,char * out_value)
{
    ALOGI("Reference-ril. setSmsBroadcastConfigValue value %d",value);

    if(value == 0xffff){
        return 0;
    }else{
        ALOGI("Reference-ril. setSmsBroadcastConfigValue value %d",value);
        sprintf(out_value,"%d",value);
    }
    ALOGI("Reference-ril. setSmsBroadcastConfigValue out_value %s",out_value);

    return 1;
}

static void setSmsBroadcastConfigData(int data, int idx, int isFirst, char *toStr, int* strLength, char *retStr ){
    int len = 0;
    char get_char[10] ={0};
    char comma = 0x2c;
    char quotes = 0x22;

    memset(get_char,0,10);
    if(setSmsBroadcastConfigValue( data, get_char)>0){
        ALOGI("Reference-ril. setSmsBroadcastConfigData  ");
        ALOGI("Reference-ril. setSmsBroadcastConfigData (1) ");
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
    ALOGI("Reference-ril. setSmsBroadcastConfigData  ret_char %s , len %d",retStr , *strLength);
}

static void requestSetSmsBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse    *p_response = NULL;
    int           err;
    char*         cmd;

    RIL_GSM_BroadcastSmsConfigInfo **gsmBciPtrs = ( RIL_GSM_BroadcastSmsConfigInfo* *)data;
    int  i =0 ,j=0;
    RIL_GSM_BroadcastSmsConfigInfo gsmBci;
    char pre_colon = 0x22;
    int count = datalen/sizeof(RIL_GSM_BroadcastSmsConfigInfo*);
    int channelLen =0;
    int langLen = 0;
    int len = 0;
    char *channel;
    char *lang;
    char  get_char[10] ={0};
    char comma = 0x2c;
    int enable = 0;
    char tmp[20] = {0};
    char quotes = 0x22;

    ALOGD("Reference-ril. requestSetSmsBroadcastConfig %d ,count %d", datalen,count);

    channel = malloc(datalen*16);
    lang = malloc(datalen*16);
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
        ALOGI("Reference-ril. requestSetSmsBroadcastConfig channel %s ,%d ",channel, channelLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.toServiceId,i,0,channel,&len,tmp);
        memcpy(channel+channelLen,tmp,strlen(tmp));
        channelLen += len;
        ALOGI("Reference-ril. requestSetSmsBroadcastConfig channel %s ,%d",channel, channelLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.fromCodeScheme,i,1,lang,&len,tmp);
        memcpy(lang+langLen,tmp,strlen(tmp));
        langLen += len;
        ALOGI("Reference-ril. requestSetSmsBroadcastConfig lang %s, %d",lang, langLen);

        memset(tmp, 0,20);
        setSmsBroadcastConfigData(gsmBci.toCodeScheme,i,0,lang,&len,tmp);
        memcpy(lang+langLen,tmp,strlen(tmp));
        langLen += len;
        ALOGI("Reference-ril. requestSetSmsBroadcastConfig lang %s, %d",lang,langLen);
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
    asprintf(&cmd, "AT+CSCB=%d%c%s%c%c%s%c",enable,comma,channel,quotes,comma,lang,quotes);
    ALOGI("Reference-ril. requestSetSmsBroadcastConfig cmd %s",cmd);

    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(channel);
    free(lang);
    free(cmd);
    ALOGI( "requestSetSmsBroadcastConfig err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
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
        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    }

    at_response_free(p_response);
}

static void requestSmsBroadcastActivation(int channelID,  void *data, size_t datalen, RIL_Token t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    int   *active = (int*)data;

    ALOGI("Reference-ril."
            " datalen: %d ,active %d\n", datalen,active[0]);

    asprintf(&cmd, "AT+CSCB=%d", active[0]);

    err = at_send_command(ATch_type[channelID], cmd,&p_response);
    free(cmd);
    ALOGI( "requestSmsBroadcastActivation err %d ,success %d",err,p_response->success);
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

static void requestSendUSSD(int channelID, void *data, size_t datalen, RIL_Token t)
{
    ATResponse  *p_response = NULL;
    char *ussdHexRequest;
    int err;
    char *cmd;
    int len;

    ussdRun = 1;
    ussdHexRequest = (char *)(data);
    asprintf(&cmd, "AT+CUSD=1,\"%s\",15", ussdHexRequest);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    if (err < 0 || p_response->success == 0) {
        ussdRun = 0;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        free(cmd);
        at_response_free(p_response);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        free(cmd);
        at_response_free(p_response);
    }

}

static void requestVideoPhoneDial(int channelID, void *data, size_t datalen, RIL_Token t)
{
    RIL_VideoPhone_Dial *p_dial;
    ATResponse   *p_response = NULL;
    int err;
    char *cmd;

    p_dial = (RIL_VideoPhone_Dial *)data;

#ifdef NEW_AT
    asprintf(&cmd, "ATD=%s", p_dial->address);
#else
    asprintf(&cmd, "AT^DVTDIAL=\"%s\"", p_dial->address);
#endif
    wait4android_audio_ready("ATD_VIDEO");
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
}

/* release Channel */
static void putChannel(int channel)
{
    struct channel_description *descriptions;

    if(s_dualSimMode)
        descriptions = dual_descriptions;
    else
        descriptions = single_descriptions;

    if(s_dualSimMode) {
        if(channel < 1 || channel >= DUAL_MAX_CHANNELS )
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
    ALOGD("put Channel ID '%d'", descriptions[channel].channelID);
    pthread_mutex_unlock(&descriptions[channel].mutex);
}

/* Return channel ID */
static int getChannel()
{
    int ret=0;
    int channel;
    struct channel_description *descriptions;
    int channel_num;

    if(s_dualSimMode) {
        descriptions = dual_descriptions;
        channel_num = DUAL_MAX_CHANNELS;
    } else {
        descriptions = single_descriptions;
        channel_num = MAX_CHANNELS;
    }

    if(s_dualSimMode)
        pthread_mutex_lock(&s_channel_mutex);
    for (;;) {
        if(!s_channel_open) {
            sleep(1);
            continue;
        }
        for (channel = 1; channel < channel_num; channel++) {
            pthread_mutex_lock(&descriptions[channel].mutex);
            if(descriptions[channel].state == CHANNEL_IDLE) {
                ALOGD("channel%d state: '%d' \n",
                    descriptions[channel].channelID, descriptions[channel].state);
                descriptions[channel].state = CHANNEL_BUSY;
                ALOGD("get Channel ID '%d'", descriptions[channel].channelID);
                pthread_mutex_unlock(&descriptions[channel].mutex);
                if(s_dualSimMode)
                    pthread_mutex_unlock(&s_channel_mutex);
                return channel;
            }
            pthread_mutex_unlock(&descriptions[channel].mutex);
        }
        usleep(5000);
    }
    if(s_dualSimMode)
        pthread_mutex_unlock(&s_channel_mutex);
    return ret;
}

static int getSmsChannel()
{
    int ret = 0;
    struct channel_description *descriptions;
    int channel_num;

    if(s_dualSimMode) {
        descriptions = dual_descriptions;
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
            ALOGD("channel%d state: '%d' \n",
                descriptions[channel_num].channelID, descriptions[channel_num].state);
            descriptions[channel_num].state = CHANNEL_BUSY;
            ALOGD("get Channel ID '%d'", descriptions[channel_num].channelID);
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
            ALOGD("getSimType: %s", line);
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
            else
                ret = RIL_APPTYPE_SIM;

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
static void requestSetCellBroadcastConfig(int channelID,  void *data, size_t datalen, RIL_Token t)
{

    ATResponse        *p_response = NULL;
    int                err;
    char              *cmd;
    RIL_CB_ConfigArgs *cbDataPtr = (RIL_CB_ConfigArgs *)data;

    ALOGD("Reference-ril. requestSetCellBroadcastConfig selectedId = %d", cbDataPtr->selectedId);
    if (cbDataPtr->selectedId == 1) { // Configure all IDs
        asprintf(&cmd, "AT+CSCB=%d,\"1000\",\"\"", !(cbDataPtr->bCBEnabled));
    } else if (cbDataPtr->selectedId == 2) { // Configure special IDs
        asprintf(&cmd, "AT+CSCB=%d,\"%s\",\"\"",
                 !(cbDataPtr->bCBEnabled), cbDataPtr->msgIDs);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }
    ALOGI("Reference-ril. requestSetSmsBroadcastConfig cmd %s",cmd);
    err = at_send_command(ATch_type[channelID], cmd, &p_response);
    ALOGI( "requestSetCellBroadcastConfig err %d ,success %d",err,p_response->success);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
error:
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
    RIL_CB_ConfigArgs  cbsPtr = {0};

    ALOGD("Reference-ril. requestGetCellBroadcastConfig enter");
    err = at_send_command_singleline(ATch_type[channelID], "AT+CSCB?", "+CSCB:", &p_response);
    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        line = p_response->p_intermediates->line;
        ALOGD("requestGetCellBroadcastConfig: err=%d line=%s", err, line);
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
                    cbsPtr.selectedId = 2;
                    cbsPtr.msgIDs = response;
                }
                RIL_onRequestComplete(t, RIL_E_SUCCESS, &cbsPtr, sizeof(RIL_CB_ConfigArgs));
                free(response);
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

    ALOGD("Reference-ril. requestSendEncodedUSSD enter");

    ussdRun = 1;
    asprintf(&cmd, "AT+CUSD=1,\"%s\",%d", p_ussd->encodedUSSD, p_ussd->dcsCode);
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
#endif

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
    int channelID;

    ALOGD("onRequest: %s sState=%d", requestToString(request), sState);

    channelID = getChannel();

    /* Ignore all requests except RIL_REQUEST_GET_SIM_STATUS
     * when RADIO_STATE_UNAVAILABLE.
     */
    if (sState == RADIO_STATE_UNAVAILABLE
            && !(request == RIL_REQUEST_GET_SIM_STATUS
                || request == RIL_REQUEST_GET_IMEI
                || request == RIL_REQUEST_GET_IMEISV)
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
#endif
                || request == RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
                || request == RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
                || request == RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
                || request == RIL_REQUEST_GET_SIM_STATUS
                || request == RIL_REQUEST_SIM_IO
                || request == RIL_REQUEST_BASEBAND_VERSION
                || request == RIL_REQUEST_GET_IMEI
                || request == RIL_REQUEST_GET_IMEISV)
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

                int result = getCardStatus(channelID, &p_card_status);
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
                ALOGD("Is stk call, Don't send chld=2");
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
            wait4android_audio_ready("ATA");
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
                char  cmd[12];
                int   party = ((int*)data)[0];
                p_response = NULL;

                /* Make sure that party is in a valid range.
                 * (Note: The Telephony middle layer imposes a range of 1 to 7.
                 * It's sufficient for us to just make sure it's single digit.)
		 */
                if (party > 0 && party < 10) {
                    sprintf(cmd, "AT+CHLD=2%d", party);
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
            requestSignalStrength(channelID, data, datalen, t);
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
                char *cmd;
                struct listnode *cmd_item = NULL;

                cmd_item = (struct listnode* )malloc(sizeof(struct listnode));
                if(cmd_item == NULL) {
                    ALOGE("Allocate dtmf cmd_item failed");
                    exit(-1);
                }
                cmd_item->data = ((char *)data)[0];
                list_add_tail(&dtmf_char_list, cmd_item);

                asprintf(&cmd, "AT+SDTMF=1,\"%c\",1", (int)c);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                free(cmd);
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_DTMF:
            {
                char c = ((char *)data)[0];
                char *cmd;

                asprintf(&cmd, "AT+VTS=%c", (int)c);
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                free(cmd);
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_DTMF_STOP:
            {
                char *cmd;
                struct listnode *cmd_item = NULL;
                char c;

                cmd_item = (&dtmf_char_list)->next;
                if(cmd_item != (&dtmf_char_list)) {
                    c = cmd_item->data;
                    asprintf(&cmd, "AT+VTS=%c", (int)c);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                    free(cmd);
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
                        p_response->p_intermediates->line, sizeof(char *));
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
                        p_response->p_intermediates->line, sizeof(char *));
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
                char *cmd;

                p_response = NULL;
                /* AT^SYSCONFIG=<mode>,<acqorder>,<roam>,<srvdomain>
                 * mode: 2:Auto 13:GSM ONLY 15:TDSCDMA ONLY
                 * acqorder: 3 -- no change
                 * roam: 2 -- no change
                 * srvdomain: 4 -- no change
		 */
                RIL_onUnsolicitedResponse (
                        RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                        NULL, 0);
                if(s_dualSimMode) {
                    char prop[10];
                    extern int s_sim_num;

                    if(s_sim_num == 0) {
                        property_get(RIL_MAIN_SIM_PROPERTY, prop, "0");
                        if(!strcmp(prop, "0"))
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=1", NULL);
                        else
                            at_send_command(ATch_type[channelID], "AT+SAUTOATT=0", NULL);
                        asprintf(&cmd, "AT^SYSCONFIG=%d,3,2,4", ((int *)data)[0]);
                        err = at_send_command(ATch_type[channelID], cmd, &p_response);
                        free(cmd);
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
                        ALOGD(" RIL_SIM0_STATE = %s", prop);
                        if(!strcmp(prop, "ABSENT")) {
                            asprintf(&cmd, "AT^SYSCONFIG=%d,3,2,4", ((int *)data)[0]);
                            err = at_send_command(ATch_type[channelID], cmd, &p_response);
                            free(cmd);
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
                    asprintf(&cmd, "AT^SYSCONFIG=%d,3,2,4", ((int *)data)[0]);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    free(cmd);
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
                char *cmd;
                int response = 0;

                p_response = NULL;
                asprintf(&cmd, "AT^SYSCONFIG?");
                err = at_send_command_singleline(ATch_type[channelID], cmd,
                        "^SYSCONFIG:", &p_response);
                free(cmd);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextint(&line, &response);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response,
                                sizeof(response));
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
                char *cmd;

                asprintf(&cmd, "AT+CLIR=%d", n);
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

                ALOGD("got OEM_HOOK_STRINGS: 0x%8p %lu", data, (long)datalen);


                for (i = (datalen / sizeof (char *)), cur = (const char **)data ;
                        i > 0 ; cur++, i --) {
                    ALOGD("> '%s'", *cur);
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
                char * cmd;
                p_response = NULL;
                asprintf(&cmd, "AT+CMGD=%d", ((int *)data)[0]);
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
                char *cmd;
                asprintf(&cmd, "AT+CMUT=%d", ((int *)data)[0]);
                err = at_send_command(ATch_type[channelID], cmd, NULL);
                free(cmd);
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
                char *cmd, *line;
                ATLine *p_cur;

                if (c == 0) {
                    asprintf(&cmd, "AT+CCWA=1,2");
                }else {
                    asprintf(&cmd, "AT+CCWA=1,2,%d", c);
                }
                err = at_send_command_multiline(ATch_type[channelID], cmd, "+CCWA: ",
                        &p_response);
                free(cmd);

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
                                validCount ? waitingList : NULL,
                                validCount * sizeof (RIL_CallWaitingInfo *));
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
#endif
            }
        case RIL_REQUEST_SET_CALL_WAITING:
            {
                p_response = NULL;
                int enable = ((int *)data)[0];
                int c = ((int *)data)[1];
                char *cmd;

                if (c == 0) {
                    asprintf(&cmd, "AT+CCWA=1,%d", enable);
                }else {
                    asprintf(&cmd, "AT+CCWA=1,%d,%d", enable, c);
                }

                err = at_send_command(ATch_type[channelID], cmd,
                        &p_response);
                free(cmd);
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
                char *cmd;

                p_response = NULL;
                asprintf(&cmd, "AT+SPUSATPROFILE?");
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS,
                            p_response->p_intermediates->line, sizeof(char *));
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_STK_SET_PROFILE:
            break;
        case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND:
            {
                char *cmd;

                p_response = NULL;
                asprintf(&cmd, "AT+SPUSATENVECMD=\"%s\"", (char*)(data));
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

                p_response = NULL;
                asprintf(&cmd, "AT+SPUSATTERMINAL=\"%s\"", (char*)(data));
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
                char *cmd;
                p_response = NULL;
                int value = ((int *)data)[0];
                if (value == 0) {
                    ALOGD(" cancel STK call ");
                    s_isstkcall = 0;
                    asprintf(&cmd, "AT+SPUSATCALLSETUP=0");
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    free(cmd);
                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                    }
                    at_response_free(p_response);
                } else {
                    ALOGD(" confirm STK call ");
                    s_isstkcall = 1;
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING:
            {
                char *cmd;
                int response = 0;
                p_response = NULL;
                ALOGD("[stk]send RIL_REQUEST_STK_SET_PROFILE");
                asprintf(&cmd, "AT+SPUSATPROFILE?");
                err = at_send_command(ATch_type[channelID], cmd, &p_response);
                free(cmd);
                ALOGD("[stk]RIL_REQUEST_STK_SET_PROFILE: err=%d succ=%d", err, p_response->success);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                }
                break;
            }
        case RIL_REQUEST_GET_SMSC_ADDRESS:
            {
                char *cmd;
                char *sc_line;

                p_response = NULL;
                ALOGD("[sms]RIL_REQUEST_GET_SMSC_ADDRESS");
                asprintf(&cmd, "AT+CSCA?");
                err = at_send_command_singleline(ATch_type[channelID], cmd, "+CSCA:",
                        &p_response);
                free(cmd);
                if (err >= 0 && p_response->success) {
                    char *line = p_response->p_intermediates->line;
                    err = at_tok_start(&line);
                    if (err >= 0) {
                        err = at_tok_nextstr(&line, &sc_line);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, sc_line,
                                strlen(sc_line));
                    } else {
                        ALOGD("[sms]at_tok_start fail");
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    ALOGD("[sms]RIL_REQUEST_GET_SMSC_ADDRESS fail");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }
        case RIL_REQUEST_SET_SMSC_ADDRESS:
            {
                char *cmd;

                p_response = NULL;
                ALOGD("[sms]RIL_REQUEST_SET_SMSC_ADDRESS");
                asprintf(&cmd, "AT+CSCA=\"%s\"", (char*)(data));
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
                char *cmd;

                p_response = NULL;
                asprintf(&cmd, "AT+SPSMSFULL=%d", !((int *)data)[0]);
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
                char *cmd;
                p_response = NULL;
                asprintf(&cmd, "AT"AT_PREFIX"DVTSEND=%d", ((int *)data)[0]);
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

        case RIL_REQUEST_VIDEOPHONE_CONTROL_IFRAME:
            {
                char *cmd;
                p_response = NULL;
                asprintf(&cmd, "AT"AT_PREFIX"DVTLFRAME=%d,%d", ((int *)data)[0], ((int *)data)[1]);
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
        case RIL_REQUEST_VIDEOPHONE_SET_VOICERECORDTYPE:
            {
                char *cmd;
                p_response = NULL;
                asprintf(&cmd, "AT+SPRUDLV=%d", ((int *)data)[0]);
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
                char *cmd;
                p_response = NULL;
                RIL_VideoPhone_Codec* p_codec = (RIL_VideoPhone_Codec*)data;
                asprintf(&cmd, "AT"AT_PREFIX"DVTCODEC=%d", p_codec->type);
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
        case RIL_REQUEST_VIDEOPHONE_HANGUP:
            {
                p_response = NULL;
#ifdef NEW_AT
                int reason = ((int *)data)[0];
                if (reason < 0){
                    err = at_send_command(ATch_type[channelID], "ATH", &p_response);
                } else {
                    char *cmd;
                    asprintf(&cmd, "ATH%d", ((int *)data)[0]);
                    err = at_send_command(ATch_type[channelID], cmd, &p_response);
                    free(cmd);
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
                wait4android_audio_ready("ATA_VIDEO");
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
                p_response = NULL;
                asprintf(&cmd, "AT"AT_PREFIX"DVTSTRS=\"%s\"", (char*)(data));
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
                char *cmd;
                p_response = NULL;
                int datatype = ((int*)data)[0];
                int sw = ((int*)data)[1];

                if ((datalen/sizeof(int)) >2){
                    int indication = ((int*)data)[2];
                    asprintf(&cmd, "AT"AT_PREFIX"DVTSEND=%d,%d,%d", datatype, sw, indication);
                } else {
                    asprintf(&cmd, "AT"AT_PREFIX"DVTSEND=%d,%d", datatype, sw);
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
        case RIL_REQUEST_VIDEOPHONE_RECORD_VIDEO:
            {
                char *cmd;
                p_response = NULL;
                asprintf(&cmd, "AT"AT_PREFIX"DVTRECA=%d", ((int *)data)[0]);
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
        case RIL_REQUEST_VIDEOPHONE_RECORD_AUDIO:
            {
                char *cmd;
                p_response = NULL;
                int on = ((int*)data)[0];
                int mode = ((int*)data)[1];

                if (datalen > 1) {
                    asprintf(&cmd, "AT^DAUREC=%d,%d", on, mode);
                } else {
                    asprintf(&cmd, "AT^DAUREC=%d", ((int *)data)[0]);
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
        case RIL_REQUEST_VIDEOPHONE_TEST:
            {
                char *cmd;
                p_response = NULL;
                int flag = ((int*)data)[0];
                int value = ((int*)data)[1];

                asprintf(&cmd, "AT"AT_PREFIX"DVTTEST=%d,%d", flag, value);
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
        case RIL_REQUEST_MBBMS_GSM_AUTHEN:
            {
                char *cmd;
                char *line;
                p_response = NULL;
                ALOGD("[MBBMS]send RIL_REQUEST_MBBMS_GSM_AUTHEN");
                asprintf(&cmd, "AT^MBAU=\"%s\"",(char*)(data));
                err = at_send_command_singleline(ATch_type[channelID], cmd, "^MBAU:",
                        &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {

                    line = p_response->p_intermediates->line;
                    ALOGD("[MBBMS]RIL_REQUEST_MBBMS_GSM_AUTHEN: err=%d line=%s", err, line);
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
                p_response = NULL;
                ALOGD("[MBBMS]send RIL_REQUEST_MBBMS_USIM_AUTHEN");
                asprintf(&cmd, "AT^MBAU=\"%s\",\"%s\"",((char **)(data))[0], ((char **)(data))[1]);
                err = at_send_command_singleline(ATch_type[channelID], cmd, "^MBAU:",
                        &p_response);
                free(cmd);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {

                    line = p_response->p_intermediates->line;
                    ALOGD("[MBBMS]RIL_REQUEST_MBBMS_USIM_AUTHEN: err=%d line=%s", err, line);
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
            char * str;

            ALOGD("[MBBMS]RIL_REQUEST_MBBMS_SIM_TYPE");
            app_type = getSimType(channelID);
            if(app_type == RIL_APPTYPE_UNKNOWN)
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            else if(app_type == RIL_APPTYPE_USIM)
                card_type = 1;
            else
                card_type = 0;
            asprintf(&str, "%d", card_type);
            ALOGD("[MBBMS]RIL_REQUEST_MBBMS_SIM_TYPE, card_type =%s", str);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, str, strlen(str));
            free(str);
            break;
        }
        case RIL_REQUEST_GET_REMAIN_TIMES:
            {
                char  cmd[12];
                int   type = ((int*)data)[0];
                char *line;
                int result;

                p_response = NULL;
                ALOGD("[MBBMS]send RIL_REQUEST_GET_REMAIN_TIMES, type:%d",type);
                if (type >= 0 && type < 4) {
                    sprintf(cmd, "AT+XX=%d", type);

                    err = at_send_command_singleline(ATch_type[channelID], cmd, "+XX:",
                            &p_response);

                    if (err < 0 || p_response->success == 0) {
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    } else {

                        line = p_response->p_intermediates->line;

                        ALOGD("[MBBMS]RIL_REQUEST_GET_REMAIN_TIMES: err=%d line=%s", err, line);

                        err = at_tok_start(&line);

                        if (err == 0) err = at_tok_nextint(&line, &result);
                        if (err == 0) {
                            RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
                        }
                        else {
                            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                        }
                    }
                } else {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                break;
            }

        case RIL_REQUEST_GET_SIM_CAPACITY:
            {
                char *cmd;
                char *line, *skip;
                int response[2] = {-1, -1};
                char *responseStr[2] = {NULL, NULL};
                int result = 0;

                p_response = NULL;
                asprintf(&cmd, "AT+CPMS?");
                err = at_send_command_singleline(ATch_type[channelID], cmd, "+CPMS:",
                        &p_response);
                free(cmd);

                if (err >= 0 && p_response->success) {
                    line = p_response->p_intermediates->line;
                    ALOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: err=%d line=%s", err, line);
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
                    ALOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: result=%d resp0=%d resp1=%d", result, response[0], response[1]);
                    if (result == 1) {
                        asprintf(&responseStr[0], "%d", response[0]);
                        asprintf(&responseStr[1], "%d", response[1]);
                        ALOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY: str resp0=%s resp1=%s", responseStr[0], responseStr[1]);
                        RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, 2*sizeof(char*));
                        if (responseStr[0] != NULL) {
                            free(responseStr[0]);
                        }
                        if (responseStr[1] != NULL) {
                            free(responseStr[1]);
                        }
                    } else {
                        ALOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
                        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    }
                } else {
                    ALOGD("[sms]RIL_REQUEST_GET_SIM_CAPACITY fail");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                at_response_free(p_response);
                break;
            }

        case RIL_REQUEST_MMI_ENTER_SIM:
            {
                char *cmd;

                p_response = NULL;
                ALOGD("[SIM]send RIL_REQUEST_MMI_ENTER_SIM");
                asprintf(&cmd, "ATD%s",(char*)(data));
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
        case RIL_REQUEST_SET_CELL_BROADCAST_CONFIG:
            requestSetCellBroadcastConfig(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_GET_CELL_BROADCAST_CONFIG:
            requestGetCellBroadcastConfig(channelID,data, datalen, t);
            break;

        case RIL_REQUEST_SEND_ENCODED_USSD:
            requestSendEncodedUSSD(channelID, data, datalen, t);
            break;

        case RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO:
            {
                ALOGD("RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO");
                break;
            }

        case RIL_REQUEST_GET_PHONEBOOK_ENTRY:
            {
                ALOGD("RIL_REQUEST_GET_PHONEBOOK_ENTRY");
                break;
            }

        case RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY:
            {
                ALOGD("RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY");
                break;
            }

        case RIL_REQUEST_USIM_PB_CAPA:
            {
                ALOGD("RIL_REQUEST_USIM_PB_CAPA");
                break;
            }

        case RIL_REQUEST_LOCK_INFO:
            {
                char  cmd[12] = {0};
                int   type = 0;
                char *line = NULL;
                int   result = 0;
                RIL_SIM_Lockinfo *p_lock = NULL;
                RIL_SIM_Lockinfo_Response lock_info = {0};

                ALOGD("RIL_REQUEST_LOCK_INFO");
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
                    ALOGD("RIL_REQUEST_LOCK_INFO: unsupport lock type!!");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                    return;
                }
                sprintf(cmd, "AT+XX=%d", type);
                err = at_send_command_singleline(ATch_type[channelID], cmd, "+XX:",
                                                 &p_response);
                if (err < 0 || p_response->success == 0) {
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                } else {
                    line = p_response->p_intermediates->line;
                    ALOGD("RIL_REQUEST_LOCK_INFO: err=%d line=%s", err, line);
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
                char *cmd;

                ALOGD("RIL_REQUEST_STK_SIM_INIT_EVENT");
                p_response = NULL;
                asprintf(&cmd, "AT+SPUSATCHECKFDN=1");
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
#endif
        default:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }

    putChannel(channelID);

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
        char property[10] = "";
        char *cmd;
        char prop[10] = "";
        ATResponse   *p_response1 = NULL;
        property_get(RIL_ASSERT, property, "0");
        if(!strcmp(property, "1")) {
            extern int s_sim_num;
            if(s_dualSimMode) {
                if(s_sim_num == 0)
                    property_get(RIL_SIM_PIN_PROPERTY, prop, "");
                else if (s_sim_num == 1)
                    property_get(RIL_SIM_PIN_PROPERTY1, prop, "");
                if(strlen(prop) != 4) {
                    goto out;
                } else {
                    asprintf(&cmd, "AT+CPIN=%s", prop);
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
                property_get(RIL_SIM_PIN_PROPERTY, prop, "");
                if(strlen(prop) != 4) {
                    goto out;
                } else {
                    asprintf(&cmd, "AT+CPIN=%s", prop);
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
        ALOGD("app type %d",app_type);
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
            ALOGD("pdp[%d].state = %d", i, pdp[i].state);
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

    /*power on sim card */
    if(s_dualSimMode) {
        char prop[10];
        extern int s_sim_num;
        if(s_sim_num == 0) {
            property_get(RIL_SIM_POWER_PROPERTY, prop, "0");
            if(!strcmp(prop, "0")) {
                property_set(RIL_SIM_POWER_PROPERTY, "1");
                at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
            }
        } else if (s_sim_num == 1) {
            property_get(RIL_SIM_POWER_PROPERTY1, prop, "0");
            if(!strcmp(prop, "0")) {
                property_set(RIL_SIM_POWER_PROPERTY1, "1");
                at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
            }
        }
    } else {
        char prop[10];
        property_get(RIL_SIM_POWER_PROPERTY, prop, "0");
        if(!strcmp(prop, "0")) {
            property_set(RIL_SIM_POWER_PROPERTY, "1");
            at_send_command(ATch_type[channelID], "AT+SFUN=2", NULL);
        }
    }

    /* assume radio is off on error */
    if(isRadioOn(channelID) > 0) {
        setRadioState (channelID, RADIO_STATE_SIM_NOT_READY);
    }
    putChannel(channelID);

    list_init(&dtmf_char_list);
}

static void waitForClose()
{
    pthread_mutex_lock(&s_state_mutex);

    while (s_closed == 0) {
        pthread_cond_wait(&s_state_cond, &s_state_mutex);
    }

    pthread_mutex_unlock(&s_state_mutex);
}

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
        ALOGD("[unsl] state=%d  %s", sState, s);
        return;
    }

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

    } else if (strStartsWith(s, "+CTZV:")) {
        /*NITZ time */
        char *response;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);

        if (err != 0) {
            ALOGE("invalid NITZ line %s\n", s);
        } else {
            RIL_onUnsolicitedResponse (
                    RIL_UNSOL_NITZ_TIME_RECEIVED,
                    response, strlen(response));
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
        if (strStartsWith(s,"+CREG:") && strcmp(creg_response, s)) {
            strncpy(creg_response, s, 100);
            RIL_onUnsolicitedResponse (
                    RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                    NULL, 0);
        }
        if (strStartsWith(s,"+CGREG") && strcmp(cgreg_response, s)) {
            strncpy(cgreg_response, s, 100);
            RIL_onUnsolicitedResponse (
                    RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                    NULL, 0);
        }
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
            ALOGD("The last call fail cause: %d", call_fail_cause);
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
            if(s_sms_ready == 0) {
                s_sms_ready = 1;
#if defined (RIL_SPRD_EXTENSION)
                RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_SMS_READY, NULL, 0);
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
                RIL_onUnsolicitedResponse (RIL_UNSOL_DEVICE_READY_NOTI, NULL, 0);
#endif
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
        int value;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &type);
        if (err < 0) goto out;

        if(type == 3) {
            if (at_tok_hasmore(&tmp)) {
                err = at_tok_nextint(&tmp, &value);
                if (err < 0) goto out;
            }
            if(value == 1)
                RIL_requestTimedCallback (onSimAbsent, NULL, NULL);
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
            else if(value == 5)
                RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_PB_READY, NULL, 0);
#endif
        }
    } else if (strStartsWith(s, "+CBM:")) {

        ALOGD("Reference-ril. CBM   >>>>>> sss %s ,len  %d", s,strlen(s));
        ALOGD("Reference-ril. CBM   >>>>>> %s ,len  %d", sms_pdu,strlen(sms_pdu));
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS,
                sms_pdu, strlen(sms_pdu));

    } else if (strStartsWith(s, "+CDS:")) {
        RIL_onUnsolicitedResponse (
                RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
                sms_pdu, strlen(sms_pdu));
    } else if (strStartsWith(s, "+CMGR:")) {
        if (sms_pdu != NULL) {
            RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS, sms_pdu,
                    strlen(sms_pdu));
        } else {
            ALOGD("[cmgr] sms_pdu is NULL");
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

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("sms request fail");
            goto out;
        }
        if (strcmp(response, "SM")) {
            ALOGD("sms request arrive but it is not a new sms");
            goto out;
        }

        /* Read the memory location of the sms */
        err = at_tok_nextint(&tmp, &location);
        if (err < 0) {
            ALOGD("error parse location");
            goto out;
        }
        ALOGD("[unsl]cmti: location = %d", location);
#if defined (RIL_SPRD_EXTENSION)
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM, &location, sizeof(location));
#elif defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        RIL_requestTimedCallback (onClass2SmsReceived, &location, NULL);
#endif
    } else if (strStartsWith(s, "+SPUSATENDSESSIONIND")) {
        ALOGD("[stk unsl]RIL_UNSOL_STK_SESSION_END");
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_SESSION_END, NULL, 0);
    }  else if (strStartsWith(s, "+SPUSATPROCMDIND:")) {
        char *response = NULL;
        char *tmp;

        ALOGD("[stk unsl]RIL_UNSOL_STK_PROACTIVE_COMMAND");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }

        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, response, sizeof(response));
    } else if (strStartsWith(s, "+SPUSATDISPLAY:")) {
        char *response = NULL;
        char *tmp;

        ALOGD("[stk unsl]RIL_UNSOL_STK_EVENT_NOTIFY");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_EVENT_NOTIFY, response, sizeof(response));
    } else if (strStartsWith(s, "+SPUSATSETUPCALL:")) {
        char *response = NULL;
        char *tmp;

        ALOGD("[stk unsl]RIL_UNSOL_STK_CALL_SETUP");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse (RIL_UNSOL_STK_CALL_SETUP, response, sizeof(response));
    } else if (strStartsWith(s, "+SPUSATREFRESH:")) {
        int response[3];
        char *tmp;

        ALOGD("[stk unsl]SPUSATREFRESH");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextint(&tmp, &(response[0]));
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &(response[1]));
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &(response[2]));
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        ALOGD("[stk unsl]SPUSATREFRESH result = %d %d %d", response[0], response[1], response[2]);
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_REFRESH, &response, sizeof(response));
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
            ALOGD("%s fail", s);
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
            ALOGD("%s fail", s);
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

        ALOGD("SPERROR for SS");
        if (ussdRun != 1)
            return;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &type);
        if (err < 0) goto out;

        ALOGD("SPERROR type = %d",type);
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
            ALOGE("Error code not present");
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
                ALOGD("+CUSD ussdError");
                if (!strcmp(response[0],"0")) {
                    response[0] = "4";//4:network does not support the current operation
                }
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
            ALOGD("%s fail", s);
            goto out;
        }
        if (response != NULL) {
            if (!strcmp(response, "0")) {
                static int vt_pipe = -1;
                if (vt_pipe < 0) vt_pipe = open("/dev/rpipe/ril.vt.0", O_RDWR);
                if (vt_pipe > 0) {
                    write(vt_pipe, "0", 2);
                } else {
                    ALOGE("open vt_pipe failed: %d", vt_pipe);
                    ALOGE("vt_pipe errno: %d, %s", errno, strerror(errno));
                }
            }
        }

        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_DATA,response,strlen(response) * sizeof(char*));
    } else if (strStartsWith(s, AT_PREFIX"DVTCODECRI:")) {
        int response[3];
        int index = 0;
        int iLen = 1;
        char *tmp;
        ALOGD("onUnsolicited(), "AT_PREFIX"DVTCODECRI:");

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response[0]);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }

        if (3 == response[0]){
            for (index = 1; index <= 3; index++){
                err = at_tok_nextint(&tmp, &response[index]);
                if (err < 0) {
                    ALOGD("%s fail", s);
                    goto out;
                }
            }
            iLen = 4;
        }

        ALOGD("onUnsolicited(), "AT_PREFIX"DVTCODECUP:, response[0]: %d", response[0]);
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_CODEC, &response, iLen * sizeof(response[0]));
    } else if (strStartsWith(s, AT_PREFIX"DVTSTRRI:")) {
        char *response = NULL;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_STRING,response,strlen(response) * sizeof(char*));
    } else if (strStartsWith(s, AT_PREFIX"DVTSENDRI")) {
        int response[3];
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &(response[0]));
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &(response[1]));
        if (err < 0) {
            ALOGD("%s fail", s);
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
            ALOGD("%s fail", s);
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
            ALOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_RELEASING,response,strlen(response) * sizeof(char*));
    } else if (strStartsWith(s, AT_PREFIX"DVTRECARI")) {
        int response;
        char *tmp;

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
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
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->idr);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->stat);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        err = at_tok_nextint(&tmp, &response->type);
        if (err < 0) {
            ALOGD("%s fail", s);
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
               ALOGD("%s fail", s);
               goto out;
            }
            err = at_tok_nextstr(&tmp, &response->number);
            if (err < 0) {
                ALOGD("%s fail", s);
                goto out;
            }
            err = at_tok_nextint(&tmp, &response->num_type);
            if (err < 0) {
                ALOGD("%s fail", s);
                goto out;
            }
            err = at_tok_nextint(&tmp, &response->bs_type);
            if (err < 0) {
                ALOGD("%s fail", s);
                goto out;
            }
            if (at_tok_hasmore(&tmp)) {
                err = at_tok_nextint(&tmp, &response->cause);
                if (err < 0) {
                    ALOGD("%s fail", s);
                    goto out;
                }
                ALOGD("onUnsolicited(), ^DSCI:, id: %d, idr: %d, stat: %d, type: %d, mpty: %d, number: %s, num_type: %d, bs_type: %d, cause: %d",
                        response->id, response->id, response->stat, response->type, response->mpty, response->number,
                        response->num_type, response->bs_type, response->cause);
                RIL_onUnsolicitedResponse(RIL_UNSOL_VIDEOPHONE_DSCI, response, sizeof(RIL_VideoPhone_DSCI));
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
        ALOGD("onUnsolicited(), "AT_PREFIX"VTMDSTRT:");

        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }

        ALOGD("onUnsolicited(), "AT_PREFIX"VTMDSTRT:, response[0]: %d", response);
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
            ALOGD("[sms]RIL_UNSOL_SIM_SMS_STORAGE_FULL");
            RIL_onUnsolicitedResponse (RIL_UNSOL_SIM_SMS_STORAGE_FULL, NULL, 0);
        }
    }
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    else if (strStartsWith(s, "+SPUSATDISPLAY:")) {
        char *tmp;
        char *data;
        RIL_SSReleaseComplete *response = NULL;

        ALOGD("RIL_UNSOL_RELEASE_COMPLETE_MESSAGE");
        response = (RIL_SSReleaseComplete *)malloc(sizeof(RIL_SSReleaseComplete));
        if (response == NULL) goto out;
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &data);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        response->dataLen = strlen(data);
        response->size = sizeof(RIL_SSReleaseComplete) + response->dataLen;
        response->params = RIL_PARAM_SSDI_STATUS| RIL_PARAM_SSDI_DATA;
        response->status = 0;
        response->data = data;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RELEASE_COMPLETE_MESSAGE, response,sizeof(RIL_SSReleaseComplete));
        free(response);
    } else if (strStartsWith(s, "+SPUSATDISPLAY:")) {
        int response;
        char *tmp;

        ALOGD("RIL_UNSOL_STK_SEND_SMS_RESULT");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);

        err = at_tok_nextint(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        // 0x0000: SMS SEND OK
        // 0x8016: "SMS SEND FAIL - MEMORY NOT AVAILABLE"
        // 0x802A: SMS SEND FAIL RETRY
        // any other value: SMS SEND GENERIC FAIL
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_SEND_SMS_RESULT, &response, sizeof(response));
    } else if (strStartsWith(s, "+SPUSATCALLCTRL:")) {
        char *response = NULL;
        char *tmp;

        ALOGD("RIL_UNSOL_STK_CALL_CONTROL_RESULT");
        line = strdup(s);
        tmp = line;
        at_tok_start(&tmp);
        err = at_tok_nextstr(&tmp, &response);
        if (err < 0) {
            ALOGD("%s fail", s);
            goto out;
        }
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_CALL_CONTROL_RESULT, response, sizeof(response));
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

    ALOGI("AT channel closed\n");
    if(s_dualSimMode)
        channel_nums = DUAL_MAX_CHANNELS;
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

    ALOGI("AT channel timeout; closing\n");
    if(s_dualSimMode)
        channel_nums = DUAL_MAX_CHANNELS;
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
    if(s_dualSimMode) {
        descriptions = dual_descriptions;
        channel_nums = DUAL_MAX_CHANNELS;
    } else {
        descriptions = single_descriptions;
        channel_nums = MAX_CHANNELS;
    }

    AT_DUMP("== ", "entering mainLoop()", -1 );
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);
    if(s_dualSimMode) {
        sim_num = 3*sim_num;
        sim_save = sim_num;
    }

    for (;;) {
        fd = -1;
        s_closed = 0;
        init_channels();
again:
        sim_num = sim_save;
        for(i = 0; i < channel_nums; i++)
        {
            /* open TTY device, and attach it to channel */
            if(s_dualSimMode) {
                sprintf(str,"/dev/CHNPTY%d",sim_num);
                strcpy(dual_descriptions[i].ttyName , str);
                dual_descriptions[i].channelID = sim_num;
                sprintf(str,"Channel%d",sim_num);
                strcpy(dual_descriptions[i].name , str);
            }

            fd = open (descriptions[i].ttyName, O_RDWR | O_NONBLOCK);

            if ( fd >= 0 ) {
                /* disable echo on serial ports */
                struct termios  ios;
                tcgetattr(fd, &ios);
                ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
                tcsetattr(fd, TCSANOW, &ios );
                descriptions[i].fd = fd;
                ALOGI ("AT channel [%d] open successfully, ttyName:%s", i, descriptions[i].ttyName );
            } else {
                perror("Opening AT interface. retrying...");
                sleep(1);
                goto again;
            }
            ATch_type[i] = at_open(fd, i, descriptions[i].name, onUnsolicited);


            if (ATch_type[i] ==NULL){
                ALOGE ("AT error on at_open\n");
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
        ALOGI("Re-opening after close");
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
    char phoneCount[5];

    if(0 == property_get(SIM_MODE_PROPERTY, phoneCount, "1")) {
        s_dualSimMode = 0;
    } else {
        if(!strcmp(phoneCount, "2"))
            s_dualSimMode = 1;
        else
            s_dualSimMode = 0;
    }

    s_rilenv = env;

    if(s_dualSimMode) {
        while ( -1 != (opt = getopt(argc, argv, "n:"))) {
            switch (opt) {
                case 'n':
                    s_sim_num = atoi(optarg);
                    ALOGD("sim card %d channel will be established!\n", s_sim_num);
                    break;
                default:
                    usage(argv[0]);
                    break;
            }
        }
    }
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(s_dualSimMode){
        ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, &s_sim_num);
        ALOGD("RIL enter dual sim card mode!");
    } else {
        ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, NULL);
        ALOGD("RIL enter single sim card mode!");
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
                ALOGI("Opening loopback port %d\n", s_port);
                break;

            case 'd':
                s_device_path = optarg;
                ALOGI("Opening tty device %s\n", s_device_path);
                break;

            case 's':
                s_device_path   = optarg;
                s_device_socket = 1;
                ALOGI("Opening socket %s\n", s_device_path);
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
