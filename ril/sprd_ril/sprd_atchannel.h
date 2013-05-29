/* //vendor/sprd/proprietories-source/ril/sprd_ril/sprd_atchannel.h
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

#ifndef ATCHANNEL_H
#define ATCHANNEL_H 1

#ifdef __cplusplus
extern "C" {
#endif

/* define AT_DEBUG to send AT traffic to /tmp/radio-at.log" */
#define AT_DEBUG  0

#if AT_DEBUG
extern void  AT_DUMP(const char* prefix, const char*  buff, int  len);
#else
#define  AT_DUMP(prefix,buff,len)  do{}while(0)
#endif

extern int modem;
#define RILLOGI(fmt, args...) ALOGI("[%c] " fmt, modem,  ## args)
#define RILLOGD(fmt, args...) ALOGD("[%c] " fmt, modem,  ## args)
#define RILLOGV(fmt, args...) ALOGV("[%c] " fmt, modem,  ## args)
#define RILLOGW(fmt, args...) ALOGW("[%c] " fmt, modem,  ## args)
#define RILLOGE(fmt, args...) ALOGE("[%c] " fmt, modem,  ## args)

extern int s_multiSimMode;

#define MAX_CHANNELS 4
#define MULTI_MAX_CHANNELS 3
#define AT_ERROR_GENERIC -1
#define AT_ERROR_COMMAND_PENDING -2
#define AT_ERROR_CHANNEL_CLOSED -3
#define AT_ERROR_TIMEOUT -4
#define AT_ERROR_INVALID_THREAD -5 /* AT commands may not be issued from
                                       reader thread (or unsolicited response
                                       callback */
#define AT_ERROR_INVALID_RESPONSE -6 /* eg an at_send_command_singleline that
                                        did not get back an intermediate
                                       response */



typedef enum {
    NO_RESULT,   /* no intermediate response expected */
    NUMERIC,     /* a single intermediate response starting with a 0-9 */
    SINGLELINE,  /* a single intermediate response starting with a prefix */
    MULTILINE    /* multiple line intermediate response
                    starting with a prefix */
} ATCommandType;

/** a singly-lined list of intermediate responses */
typedef struct ATLine  {
    struct ATLine *p_next;
    char *line;
} ATLine;

/** Free this with at_response_free() */
typedef struct {
    int success;              /* true if final response indicates
                                    success (eg "OK") */
    char *finalResponse;      /* eg OK, ERROR */
    ATLine  *p_intermediates; /* any intermediate responses */
} ATResponse;

/**
 * a user-provided unsolicited response handler function
 * this will be called from the reader thread, so do not block
 * "s" is the line, and "sms_pdu" is either NULL or the PDU response
 * for multi-line TS 27.005 SMS PDU responses (eg +CMT:)
 */
typedef void (*ATUnsolHandler)(const char *s, const char *sms_pdu);
//add AT channel data structure,  Data member move from atchannel.c
//#define MAX_AT_RESPONSE (8 * 1024)  //move from reference_ril.c
#define MAX_AT_RESPONSE 0x1000

/* define one channel for each open connection */
struct ATChannels {
    /* for input buffering */
    int s_fd;    /* fd of the AT channel */
    int channelID;
    char *name;
    int nolog;

    char s_ATBuffer[MAX_AT_RESPONSE+1];
    char *s_ATBufferCur;
    /* current line */
    char *line;

    ATCommandType s_type;
    char *s_responsePrefix;
    char *s_smsPDU;
    ATResponse *sp_response;
    char *p_read;
    char *p_eol;

    /* Handler */
    ATUnsolHandler s_unsolHandler;

};
struct ATChannels ATChannel[MAX_CHANNELS]; //move from Atchannel.c
void init_channels(void);
void stop_reader(void);
int start_reader(void);
struct ATChannels *at_open(int s_fd, int channelID, char *name, ATUnsolHandler h);
void at_close(struct ATChannels *ATch);

/* This callback is invoked on the command thread.
   You should reset or handshake here to avoid getting out of sync */
void at_set_on_timeout(void (*onTimeout)(void));
/* This callback is invoked on the reader thread (like ATUnsolHandler)
   when the input stream closes before you call at_close
   (not when you call at_close())
   You should still call at_close()
   It may also be invoked immediately from the current thread if the read
   channel is already closed */
void at_set_on_reader_closed(void (*onClose)(void));

int at_send_command_singleline (struct ATChannels *ATch, const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse);
int at_send_command_singleline_timeout (struct ATChannels *ATch,
                                const char *command,
                                const char *responsePrefix,
                                ATResponse **pp_outResponse,
				int timeout);

int at_send_command_numeric (struct ATChannels *ATch, const char *command,
                                 ATResponse **pp_outResponse);

int at_send_command_multiline (struct ATChannels *ATch, const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse);


int at_handshake(struct ATChannels *ATch);


int at_send_command (struct ATChannels *ATch,
                     const char *command, ATResponse **pp_outResponse);

int at_send_command_sms (struct ATChannels *ATch,
                            const char *command, const char *pdu,
                            const char *responsePrefix,
                            ATResponse **pp_outResponse);

void at_response_free(ATResponse *p_response);

typedef enum {
    CME_ERROR_NON_CME = -1,
    CME_SUCCESS = 0,
    CME_SIM_NOT_INSERTED = 10
} AT_CME_Error;

AT_CME_Error at_get_cme_error(const ATResponse *p_response);

#ifdef __cplusplus
}
#endif

#endif /*ATCHANNEL_H*/
