/* //vendor/sprd/proprietories-source/ril/sprd_ril/sprd_atchannel.c
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

#include "sprd_atchannel.h"
#include "at_tok.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define LOG_NDEBUG 1
#define LOG_TAG "AT"
#include <utils/Log.h>

#ifdef HAVE_ANDROID_OS
/* for IOCTL's */
#include <linux/omap_csmi.h>
#endif /*HAVE_ANDROID_OS*/

#include "misc.h"

#ifdef HAVE_ANDROID_OS
#define USE_NP 1
#endif /* HAVE_ANDROID_OS */


#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

#define HANDSHAKE_RETRY_COUNT 8
#define HANDSHAKE_TIMEOUT_MSEC 250

static pthread_mutex_t mutex[MAX_CHANNELS];
static pthread_cond_t cond[MAX_CHANNELS];
static pthread_t s_tid_reader;
static fd_set ATch_fd;
static int open_ATch;

//struct ATChannels ATChannel[MAX_CHANNELS];  //move to Atchannel.h
//static int s_ackPowerIoctl; /* true if TTY has android byte-count handshake for low power*/

#if AT_DEBUG
void  AT_DUMP(const char*  prefix, const char*  buff, int  len)
{
    if (len < 0)
        len = strlen(buff);
    RILLOGD("%.*s", len, buff);
}
#endif

/*
 * for current pending command
 * these are protected by s_commandmutex
 */

//static pthread_mutex_t s_commandmutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t s_commandcond = PTHREAD_COND_INITIALIZER;

static void (*s_onTimeout)(void) = NULL;
static void (*s_onReaderClosed)(void) = NULL;
static int s_readerClosed;

static void onReaderClosed();
static int writeCtrlZ (struct ATChannels *ATch, const char *s);
static int writeline (struct ATChannels *ATch, const char *s);

#ifndef USE_NP
static void setTimespecRelative(struct timespec *p_ts, long long msec)
{
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *) NULL);

    /* what's really funny about this is that I know
       pthread_cond_timedwait just turns around and makes this
       a relative time again */
    p_ts->tv_sec = tv.tv_sec + (msec / 1000);
    p_ts->tv_nsec = (tv.tv_usec + (msec % 1000) * 1000L ) * 1000L;
}
#endif /*USE_NP*/

static void sleepMsec(long long msec)
{
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep (&ts, &ts);
    } while (err < 0 && errno == EINTR);
}


/** add an intermediate response to sp_response*/
static void addIntermediate(struct ATChannels *ATch)
{
    ATLine *p_new;

    p_new = (ATLine  *) malloc(sizeof(ATLine));

    p_new->line = strdup(ATch->line);

    /* note: this adds to the head of the list, so the list
       will be in reverse order of lines received. the order is flipped
       again before passing on to the command issuer */
    p_new->p_next = ATch->sp_response->p_intermediates;
    ATch->sp_response->p_intermediates = p_new;
}


/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesError[] = {
    "ERROR",
    "+CMS ERROR:",
    "+CME ERROR:",
    "NO CARRIER", /* sometimes! */
    "NO ANSWER",
    "NO DIALTONE",
};
static int isFinalResponseError(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesError) ; i++) {
        if (strStartsWith(line, s_finalResponsesError[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesSuccess[] = {
    "OK",
    "CONNECT"       /* some stacks start up data on another channel */
};

static int isFinalResponseSuccess(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesSuccess) ; i++) {
        if (strStartsWith(line, s_finalResponsesSuccess[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static int isFinalResponse(const char *line)
{
    return isFinalResponseSuccess(line) || isFinalResponseError(line);
}


/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char * s_smsUnsoliciteds[] = {
    "+CMT:",
    "+CDS:",
    "+CBM:",
    "+CMGR:"
};

static int isSMSUnsolicited(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_smsUnsoliciteds) ; i++) {
        if (strStartsWith(line, s_smsUnsoliciteds[i])) {
            return 1;
        }
    }

    return 0;
}


/** assumes s_commandmutex is held */
static void handleFinalResponse(struct ATChannels* ATch)
{
    ATch->sp_response->finalResponse = strdup(ATch->line);
    pthread_cond_signal(&cond[ATch->channelID]);
}

static void handleUnsolicited(struct ATChannels* ATch)
{
    char *line = ATch->line;
    if (ATch->s_unsolHandler != NULL) {
        ATch->s_unsolHandler(line, NULL);
    }
}

static void processLine(struct ATChannels *ATch)
{
    pthread_mutex_lock(&mutex[ATch->channelID]);

    if (ATch->sp_response == NULL) {
        /* no command pending */
        handleUnsolicited(ATch);
    } else if (isFinalResponseSuccess(ATch->line)) {
        ATch->sp_response->success = 1;
        handleFinalResponse(ATch);
    } else if (isFinalResponseError(ATch->line)) {
        ATch->sp_response->success = 0;
        handleFinalResponse(ATch);
    } else if (ATch->s_smsPDU != NULL && 0 == strcmp(ATch->line, "> ")) {
        // See eg. TS 27.005 4.3
        // Commands like AT+CMGS have a "> " prompt
        writeCtrlZ(ATch, ATch->s_smsPDU);
        ATch->s_smsPDU = NULL;
    } else switch (ATch->s_type) {
        case NO_RESULT:
            handleUnsolicited(ATch);
            break;
        case NUMERIC:
            if (ATch->sp_response->p_intermediates == NULL
                && isdigit(ATch->line[0])
            ) {
                addIntermediate(ATch);
            } else {
                /* either we already have an intermediate response or
                   the line doesn't begin with a digit */
                handleUnsolicited(ATch);
            }
            break;
        case SINGLELINE:
            if (ATch->sp_response->p_intermediates == NULL
                && strStartsWith (ATch->line, ATch->s_responsePrefix)
            ) {
                addIntermediate(ATch);
            } else {
                /* we already have an intermediate response */
                handleUnsolicited(ATch);
            }
            break;
        case MULTILINE:
            if (strStartsWith (ATch->line, ATch->s_responsePrefix) ||
                strchr(ATch->line, ':') == NULL) {
                addIntermediate(ATch);
            } else {
                handleUnsolicited(ATch);
            }
        break;

        default: /* this should never be reached */
            RILLOGE("Unsupported AT command type %d\n", ATch->s_type);
            handleUnsolicited(ATch);
        break;
    }

    pthread_mutex_unlock(&mutex[ATch->channelID]);
}


/**
 * Returns a pointer to the end of the next line
 * special-cases the "> " SMS prompt
 *
 * returns NULL if there is no complete line
 */
static char * findNextEOL(char *cur)
{
    if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
        /* SMS prompt character...not \r terminated */
        return cur+2;
    }

    // Find next newline
    while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

    return *cur == '\0' ? NULL : cur;
}


/**
 * Reads a line from the AT channel, returns NULL on timeout.
 * Assumes it has exclusive read access to the FD
 *
 * This line is valid only until the next call to readline
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */

static const char *readline(struct ATChannels *ATch)
{
    ssize_t count;
    char *p_read = NULL;
    char *p_eol = NULL;

    /* this is a little odd. I use *s_ATBufferCur == 0 to
     * mean "buffer consumed completely". If it points to a character, than
     * the buffer continues until a \0
     */
    if (*ATch->s_ATBufferCur == '\0') {
        /* empty buffer */
        ATch->s_ATBufferCur = ATch->s_ATBuffer;
        *ATch->s_ATBufferCur = '\0';
        p_read = ATch->s_ATBuffer;
    } else {   /* *s_ATBufferCur != '\0' */
        /* there's data in the buffer from the last read */

        // skip over leading newlines
        while (*ATch->s_ATBufferCur == '\r' || *ATch->s_ATBufferCur == '\n')
            ATch->s_ATBufferCur++;

        p_eol = findNextEOL(ATch->s_ATBufferCur);

        if (p_eol == NULL) {
            /* a partial line. move it up and prepare to read more */
            size_t len;

            len = strlen(ATch->s_ATBufferCur);

            memmove(ATch->s_ATBuffer, ATch->s_ATBufferCur, len + 1);
            p_read = ATch->s_ATBuffer + len;
            ATch->s_ATBufferCur = ATch->s_ATBuffer;
        }
        /* Otherwise, (p_eol !- NULL) there is a complete line  */
        /* that will be returned the while () loop below        */
    }

    while (p_eol == NULL) {
        if (0 == MAX_AT_RESPONSE - (p_read - ATch->s_ATBuffer)) {
            RILLOGE("ERROR: Input line exceeded buffer\n");
            /* ditch buffer and start over again */
            ATch->s_ATBufferCur = ATch->s_ATBuffer;
            *ATch->s_ATBufferCur = '\0';
            p_read = ATch->s_ATBuffer;
        }

        do {
            count = read(ATch->s_fd, p_read,
                            MAX_AT_RESPONSE - (p_read - ATch->s_ATBuffer));
        } while (count < 0 && errno == EINTR);

        if (count > 0) {
            AT_DUMP( "<< ", p_read, count );

            p_read[count] = '\0';

            // skip over leading newlines
            while (*ATch->s_ATBufferCur == '\r' || *ATch->s_ATBufferCur == '\n')
                ATch->s_ATBufferCur++;

            p_eol = findNextEOL(ATch->s_ATBufferCur);
            p_read += count;
        } else if (count <= 0) {
            /* read error encountered or EOF reached */
            if(count == 0) {
                RILLOGD("atchannel: EOF reached");
            } else {
                //RILLOGD("atchannel: read error %s", strerror(errno));
            }
            return NULL;
        }
    }

    /* a full line in the buffer. Place a \0 over the \r and return */

    ATch->line = ATch->s_ATBufferCur;
    *p_eol = '\0';
    ATch->s_ATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
                              /* and there will be a \0 at *p_read */
    if (!ATch->nolog) {
        if (!ATch->name)
            RILLOGD("AT< %s\n", ATch->line);
        else
            RILLOGD("%s: AT< %s\n", ATch->name, ATch->line);
    }

    return ATch->line;
}


static void onReaderClosed()
{
    if (s_onReaderClosed != NULL && s_readerClosed == 0) {
        int channel;
        int channel_nums;

        if(s_multiSimMode)
            channel_nums = MULTI_MAX_CHANNELS;
        else
            channel_nums = MAX_CHANNELS;

        for (channel = 0; channel < channel_nums; channel++)
        {
        pthread_mutex_lock(&mutex[channel]);
        s_readerClosed = 1;
        pthread_cond_signal(&cond[channel]);
        pthread_mutex_unlock(&mutex[channel]);
        }

        s_onReaderClosed();
    }
}


static void *readerLoop(void *arg)
{
    int i = 0;
    int ret;
    fd_set rfds;
    int channel_nums;

    if(s_multiSimMode)
            channel_nums = MULTI_MAX_CHANNELS;
    else
            channel_nums = MAX_CHANNELS;

    for (;;) {
        do {
            rfds = ATch_fd;
            ret = select(open_ATch + 1, &rfds, NULL, NULL, 0);
        } while(ret == -1 && errno == EINTR);
	if (ret > 0) {
            for (i = 0; i < channel_nums; i++) {
                if (ATChannel[i].s_fd != -1 &&
                    FD_ISSET(ATChannel[i].s_fd, &rfds)) {
                    struct ATChannels *ATch = &ATChannel[i];
                    while(1) {
                        if (!readline(ATch))
                            break;
                        if (ATch->line == NULL)
                            break;
                        if (isSMSUnsolicited(ATch->line)) {
                            char *line1;
                            const char *line2;

                            // The scope of string returned by 'readline()' is valid only
                            // till next call to 'readline()' hence making a copy of line
                            // before calling readline again.
                            line1 = strdup(ATch->line);
                            fcntl(ATch->s_fd, F_SETFL, O_RDWR);
                            line2 = readline(ATch);
                            fcntl(ATch->s_fd, F_SETFL, O_RDWR | O_NONBLOCK);

                            if (line2 == NULL)
                                break;

                            if (ATch->s_unsolHandler != NULL)
                                ATch->s_unsolHandler (line1, line2);
                            free(line1);
                        } else {
                            processLine(ATch);
                        }
                    }
                 }
            }
        }
    }

    onReaderClosed();

    return NULL;
}

/**
 * Sends string s to the radio with a \r appended.
 * Returns AT_ERROR_* on error, 0 on success
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
static int writeline (struct ATChannels *ATch, const char *s)
{
    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;

    if (ATch->s_fd < 0 || s_readerClosed > 0) {
        return AT_ERROR_CHANNEL_CLOSED;
    }

    if (!ATch->nolog) {
        if (ATch->name)
            RILLOGD("%s: AT> %s\n", ATch->name, s);
        else
            RILLOGD("AT> %s\n", s);
    }

    AT_DUMP( ">> ", s, strlen(s) );

    /* the main string */
    while (cur < len) {
        do {
            written = write (ATch->s_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }

        cur += written;
    }

    /* the \r  */

    do {
        written = write (ATch->s_fd, "\r" , 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) {
        return AT_ERROR_GENERIC;
    }

    return 0;
}
static int writeCtrlZ (struct ATChannels *ATch, const char *s)
{
    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;

    if (ATch->s_fd < 0 || s_readerClosed > 0) {
        return AT_ERROR_CHANNEL_CLOSED;
    }

    RILLOGD("AT> %s^Z\n", s);

    AT_DUMP( ">* ", s, strlen(s) );

    /* the main string */
    while (cur < len) {
        do {
            written = write (ATch->s_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }

        cur += written;
    }

    /* the ^Z  */

    do {
        written = write (ATch->s_fd, "\032" , 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) {
        return AT_ERROR_GENERIC;
    }

    return 0;
}

static void clearPendingCommand(struct ATChannels *ATch)
{
    ATResponse *sp_response = ATch->sp_response;

    if (sp_response != NULL) {
        at_response_free(sp_response);
    }

    ATch->sp_response = NULL;
    ATch->s_responsePrefix = NULL;
    ATch->s_smsPDU = NULL;
}

void init_channels(void)
{
    int i;
    int channel_nums;

    if(s_multiSimMode)
            channel_nums = MULTI_MAX_CHANNELS;
    else
            channel_nums = MAX_CHANNELS;

    FD_ZERO(&ATch_fd);
    for (i = 0; i < channel_nums; i++) {
	ATChannel[i].s_fd = -1;
    }
    open_ATch = -1;
}

/**
 * Starts AT handler on stream "fd'
 * returns 0 on success, -1 on error
 */
struct ATChannels *at_open(int fd, int channelID, char *name, ATUnsolHandler h)
{
    struct ATChannels *ATch;
    int i= channelID;
    int channel_nums;

    if(s_multiSimMode)
            channel_nums = MULTI_MAX_CHANNELS;
    else
            channel_nums = MAX_CHANNELS;

    ATch = &ATChannel[i];

    if (i == channel_nums) {
        RILLOGE ("channelID exceeded MAX_CHANNELS in at_open\n");
        return NULL;
    }
    if (name)
        ATch->name = strdup(name);

    ATch->s_fd = fd;
    ATch->s_responsePrefix = NULL;
    ATch->channelID = channelID;
    ATch->s_smsPDU = NULL;
    ATch->s_unsolHandler = h;
    ATch->sp_response = NULL;
    ATch->nolog = 1;
    memset(ATch->s_ATBuffer, 0, sizeof(ATch->s_ATBuffer));
    ATch->s_ATBufferCur = ATch->s_ATBuffer;

    FD_SET(fd, &ATch_fd);
    open_ATch = fd > open_ATch ? fd : open_ATch;
    pthread_mutex_init(&mutex[i], NULL);
    pthread_cond_init(&cond[i], NULL);
    return ATch;
}

int start_reader(void)
{
    int ret;
    pthread_t tid;
    pthread_attr_t attr;

    s_readerClosed = 0;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&s_tid_reader, &attr, readerLoop, &attr);

    if (ret < 0) {
        perror ("pthread_create");
        return -1;
    }
    return 0;
}

/* FIXME is it ok to call this from the reader and the command thread? */
void at_close(struct ATChannels *ATch)
{//change
    if (ATch->s_fd >= 0) {
        close(ATch->s_fd);
    }
    FD_CLR(ATch->s_fd, &ATch_fd);
    if (ATch->name)
        free(ATch->name);

    ATch->s_fd = -1;
}

void stop_reader(void)
{
        int channel;
        int channel_nums;

    if(s_multiSimMode)
            channel_nums = MULTI_MAX_CHANNELS;
    else
            channel_nums = MAX_CHANNELS;

        for (channel = 0; channel < channel_nums; channel++)
        {
        pthread_mutex_lock(&mutex[channel]);
        s_readerClosed = 1;
        pthread_cond_signal(&cond[channel]);
        pthread_mutex_unlock(&mutex[channel]);
        }

    /* the reader thread should eventually die */
}

static ATResponse * at_response_new()
{
    return (ATResponse *) calloc(1, sizeof(ATResponse));
}

void at_response_free(ATResponse *p_response)
{
    ATLine *p_line;

    if (p_response == NULL) return;

    p_line = p_response->p_intermediates;

    while (p_line != NULL) {
        ATLine *p_toFree;

        p_toFree = p_line;
        p_line = p_line->p_next;

        free(p_toFree->line);
        free(p_toFree);
    }

    free (p_response->finalResponse);
    free (p_response);
}

/**
 * The line reader places the intermediate responses in reverse order
 * here we flip them back
 */
static void reverseIntermediates(struct ATChannels *ATch)
{
    ATLine *pcur,*pnext;

    pcur = ATch->sp_response->p_intermediates;
    ATch->sp_response->p_intermediates = NULL;

    while (pcur != NULL) {
        pnext = pcur->p_next;
        pcur->p_next = ATch->sp_response->p_intermediates;
        ATch->sp_response->p_intermediates = pcur;
        pcur = pnext;
    }
}

/**
 * Internal send_command implementation
 * Doesn't lock or call the timeout callback
 *
 * timeoutMsec == 0 means infinite timeout
 */

static int at_send_command_full_nolock (struct ATChannels *ATch,
                    const char *command, ATCommandType type,
                    const char *responsePrefix, const char *smspdu,
                    long long timeoutMsec, ATResponse **pp_outResponse)
{
    int err = 0;
#ifndef USE_NP
    struct timespec ts;
#endif /*USE_NP*/

    if(ATch->sp_response != NULL) {
        err = AT_ERROR_COMMAND_PENDING;
        goto error;
    }

    ATch->s_type = type;
    ATch->s_responsePrefix = (char *)responsePrefix;
    ATch->s_smsPDU = (char *)smspdu;
    ATch->sp_response = at_response_new();
    if(ATch->sp_response == NULL) {
        err = AT_ERROR_GENERIC;
        goto error;
    }

    err = writeline(ATch, command);
    if (err < 0) {
        goto error;
    }

#ifndef USE_NP
    if (timeoutMsec != 0) {
        setTimespecRelative(&ts, timeoutMsec);
    }
#endif /*USE_NP*/

    while (ATch->sp_response->finalResponse == NULL &&
           s_readerClosed == 0) {
        if (timeoutMsec != 0) {
#ifdef USE_NP
            err = pthread_cond_timeout_np(&cond[ATch->channelID], &mutex[ATch->channelID], timeoutMsec);
#else
            err = pthread_cond_timedwait(&cond[ATch->channelID], &mutex[ATch->channelID], &ts);
#endif /*USE_NP*/
        } else {
            err = pthread_cond_wait(&cond[ATch->channelID], &mutex[ATch->channelID]);
        }

        if (err == ETIMEDOUT) {
            err = AT_ERROR_TIMEOUT;
            goto error;
        }
    }

    if (pp_outResponse == NULL) {
        at_response_free(ATch->sp_response);
    } else {
        /* line reader stores intermediate responses in reverse order */
        reverseIntermediates(ATch);
        *pp_outResponse = ATch->sp_response;
    }

    ATch->sp_response = NULL;

    if(s_readerClosed > 0) {
        err = AT_ERROR_CHANNEL_CLOSED;
        goto error;
    }

    err = 0;
error:
    clearPendingCommand(ATch);

    return err;
}

/**
 * Internal send_command implementation
 *
 * timeoutMsec == 0 means infinite timeout
 */
static int at_send_command_full (struct ATChannels *ATch,
                    const char *command, ATCommandType type,
                    const char *responsePrefix, const char *smspdu,
                    long long timeoutMsec, ATResponse **pp_outResponse)
{
    int err;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&mutex[ATch->channelID]);


    err = at_send_command_full_nolock(ATch, command, type,
                    responsePrefix, smspdu,
                    timeoutMsec, pp_outResponse);

    pthread_mutex_unlock(&mutex[ATch->channelID]);

    if (err == AT_ERROR_TIMEOUT && s_onTimeout != NULL) {
        s_onTimeout();
    }

    return err;
}


/**
 * Issue a single normal AT command with no intermediate response expected
 *
 * "command" should not include \r
 * pp_outResponse can be NULL
 *
 * if non-NULL, the resulting ATResponse * must be eventually freed with
 * at_response_free
 */
int at_send_command (struct ATChannels *ATch, const char *command,
                     ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (ATch, command, NO_RESULT, NULL,
                                    NULL, 0, pp_outResponse);

    return err;
}


int at_send_command_singleline (struct ATChannels *ATch, const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (ATch, command, SINGLELINE, responsePrefix,
                                    NULL, 0, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_numeric (struct ATChannels *ATch, const char *command,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (ATch, command, NUMERIC, NULL,
                                    NULL, 0, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_sms (struct ATChannels *ATch, const char *command,
                                const char *pdu,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (ATch, command, SINGLELINE, responsePrefix,
                                    pdu, 0, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_multiline (struct ATChannels *ATch, const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (ATch, command, MULTILINE, responsePrefix,
                                    NULL, 0, pp_outResponse);

    return err;
}


/** This callback is invoked on the command thread */
void at_set_on_timeout(void (*onTimeout)(void))
{
    s_onTimeout = onTimeout;
}

/**
 *  This callback is invoked on the reader thread (like ATUnsolHandler)
 *  when the input stream closes before you call at_close
 *  (not when you call at_close())
 *  You should still call at_close()
 */

void at_set_on_reader_closed(void (*onClose)(void))
{
    s_onReaderClosed = onClose;
}

/**
 * Periodically issue an AT command and wait for a response.
 * Used to ensure channel has start up and is active
 */

int at_handshake(struct ATChannels *ATch)
{
    int i;
    int err = 0;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&mutex[ATch->channelID]);

    for (i = 0 ; i < HANDSHAKE_RETRY_COUNT ; i++) {
        /* some stacks start with verbose off */
        err = at_send_command_full_nolock (ATch, "ATE0Q0V1", NO_RESULT,
                    NULL, NULL, HANDSHAKE_TIMEOUT_MSEC, NULL);
        if (err == 0) {
            break;
        }
    }

#if 0
    if (err == 0) {
        /* pause for a bit to let the input buffer drain any unmatched OK's
           (they will appear as extraneous unsolicited responses) */

        sleepMsec(HANDSHAKE_TIMEOUT_MSEC);
    }
#endif
    pthread_mutex_unlock(&mutex[ATch->channelID]);

    return err;
}

/**
 * Returns error code from response
 * Assumes AT+CMEE=1 (numeric) mode
 */
AT_CME_Error at_get_cme_error(const ATResponse *p_response)
{
    int ret;
    int err;
    char *p_cur;

    if (p_response->success > 0) {
        return CME_SUCCESS;
    }

    if (p_response->finalResponse == NULL
        || !strStartsWith(p_response->finalResponse, "+CME ERROR:")
    ) {
        return CME_ERROR_NON_CME;
    }

    p_cur = p_response->finalResponse;
    err = at_tok_start(&p_cur);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    err = at_tok_nextint(&p_cur, &ret);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    return (AT_CME_Error) ret;
}

