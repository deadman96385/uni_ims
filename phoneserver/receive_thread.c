/*
 *
 * receive_thread.c: channel  implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */
#include "receive_thread.h"
#include "os_api.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#undef  PHS_LOGD
#define PHS_LOGD(x...)  ALOGD( x )

#if AT_DEBUG
void AT_DUMP(const char *prefix, const char *buff, int len)
{
	if (len < 0)
		len = strlen(buff);
	PHS_LOGD("%s len=%d /:%s", prefix, len, buff);
}

#endif /*  */

/**
 * Returns a pointer to the end of the next line
 * special-cases the "> " SMS prompt
 *
 * returns NULL if there is no complete line
 */
static char *findNextEOL(char *cur)
{
	if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {

		/* SMS prompt character...not \r terminated */
		return cur + 2;
	}
	// Find next newline
	while (*cur != '\0' && *cur != '\r' && *cur != '\n')
		cur++;
	return *cur == '\0' ? NULL : cur;
}

/**
 * Reads a line from the mux channel, returns NULL on timeout.
 * Assumes it has exclusive read access to the FD
 *
 * This line is valid only until the next call to readline
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
static char *readline(struct receive_thread_t *me)
{
	ssize_t count;
	char *p_read = NULL;
	char *p_eol = NULL;
	char *ret;

	/* this is a little odd. I use *s_ATBufferCur == 0 to
	 * mean "buffer consumed completely". If it points to a character, than
	 * the buffer continues until a \0
	 */
	if (*me->s_ATBufferCur == '\0') {
		/* empty buffer */
		me->s_ATBufferCur = me->mux->buffer;
		p_read = me->s_ATBufferCur;
	} else {		/* *s_ATBufferCur != '\0' */
		/* there's data in the buffer from the last read */

		// skip over leading newlines
		while (*me->s_ATBufferCur == '\r' || *me->s_ATBufferCur == '\n')
			me->s_ATBufferCur++;
		p_eol = findNextEOL(me->s_ATBufferCur);
		if (p_eol == NULL) {
			/* a partial line. move it up and prepare to read more */
			size_t len;
			len = strlen(me->s_ATBufferCur);
			memmove(me->mux->buffer, me->s_ATBufferCur, len + 1);
			p_read = me->mux->buffer + len;
			me->s_ATBufferCur = me->mux->buffer;
		}
		/* Otherwise, (p_eol !- NULL) there is a complete line  */
		/* that will be returned the while () loop below        */
	}
	while (p_eol == NULL) {
		if (0 == MAX_AT_RESPONSE - (p_read - me->mux->buffer)) {
			PHS_LOGE("ERROR: Input line exceeded buffer\n");
			/* ditch buffer and start over again */
			me->s_ATBufferCur = me->mux->buffer;
			*me->s_ATBufferCur = '\0';
			p_read = me->mux->buffer;
		}

		do {
			//PHS_LOGD("Before read << ");
			count =
			    read(me->mux->muxfd, p_read,
				 MAX_AT_RESPONSE - (p_read - me->mux->buffer));
			//PHS_LOGD("After read count: %d, p_read: %s<< ", count, p_read);
		} while (count < 0 && errno == EINTR);

		if (count > 0) {
			AT_DUMP("CHNMNG:readline << ", p_read, count);
			//PHS_LOGD("Rev TID [%d] :read pread= %p,count=%ld\n", me->tid, p_read,count);
			//p_read[count] = '\0';
			while (count >0) {
               		if (*p_read =='\0') {
				*p_read = ' ';
				 PHS_LOGD("\n receive thread reads in string end char!!!!!!\n");
				}
				p_read++;
				count--;
			}
			*p_read='\0';
			// skip over leading newlines
			while (*me->s_ATBufferCur == '\r'
			       || *me->s_ATBufferCur == '\n')
				me->s_ATBufferCur++;
			p_eol = findNextEOL(me->s_ATBufferCur);
			//p_read += count;
		} else if (count <= 0) {
			/* read error encountered or EOF reached */
			if (count == 0) {
				PHS_LOGE("atchannel: EOF reached");
			} else {
				PHS_LOGE("atchannel: read error %s",strerror(errno));
			}
			return NULL;
		}
	}

	/* a full line in the buffer. Place a \0 over the \r and return */
	ret = me->s_ATBufferCur;
	me->end_char = *p_eol;
	*p_eol = '\0';
	me->s_ATBufferCur = p_eol + 1;	/* this will always be <= p_read,    */

	/* and there will be a \0 at *p_read */
	//PHS_LOGD("Receive thread's TID [%d] CHNMNG:AT< %s\n", me->tid, ret);
	return ret;
}
void *receive_data(struct receive_thread_t *me)
{
	int received = 0;
	char *buffer = me->mux->buffer;
	char *atstr = NULL;
	me->s_ATBufferCur = buffer;
	struct cmux_t *mux = me->mux;
	*buffer = '\0';
	char tmp_buff[SERIAL_BUFFSIZE / 2];
	memset(buffer, 0, SERIAL_BUFFSIZE);
	me->buffer = me->mux->buffer;
	pid_t tid = gettid();
	me->tid = tid;
	PHS_LOGD
	    ("Rev TID [%d] : enter receive thread :mux=%s\n",
	     tid, me->mux->name);
	while (1) {
		//PHS_LOGD("Rev TID [%d] MUX :%s Waiting for resp  \n", tid, me->mux->name);
		atstr = readline(me);	//read a completed at response
                if (atstr != NULL) {
   			tmp_buff[0] = '\0';
			snprintf(tmp_buff, sizeof(tmp_buff), "%s%c", atstr, me->end_char);
			memset(atstr, 0, strlen(atstr));
			received = strlen(tmp_buff);
			PHS_LOGD
			    ("Rev TID [%d] :  rev data :mux=%s:input:%s\n",
			     tid, me->mux->name, tmp_buff);
			phoneserver_deliver_at_rsp(me->mux, tmp_buff, received);
		}
	}
	return NULL;
}
struct receive_thread_ops rcvops = {.receive_data = receive_data,
};
struct receive_thread_ops *receive_thread_get_operations(void)
{
	return &rcvops;
}
