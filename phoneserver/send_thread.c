/*
 *
 * send_ thread.c: channel implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#include "send_thread.h"
#include "os_api.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "config.h"


#undef  PHS_LOGD
#define PHS_LOGD(x...)  ALOGD( x )

/*## operation deliver_cmd_req(char*,pty_type) */
static void send_thread_deliver_cmd_req(struct send_thread_t *const me,
					char *cmd_str, int len)
{
	phoneserver_deliver_at_cmd(me->pty, cmd_str, len);
}

/**
 * Returns a pointer to the end of the next line

 * returns NULL if there is no complete line
 */
static char *findNextEOL(char *cur)
{
	// Find next newline
	while (*cur != '\0' && *cur != '\r' && *cur != '\n' && *cur != 0x1a
	       && *cur != '\032') {
		cur++;
	}
	return *cur == '\0' ? NULL : cur;
}

/**
 * Reads a line from the pty channel
 * This line is valid only until the next call to readline
 */
static char *readline(struct send_thread_t *me)
{
	ssize_t count;
	char *p_read = NULL;
	char *p_eol = NULL;
	char *ret;
	int res;
	fd_set rfds;

	/* this is a little odd. I use *s_ATBufferCur == 0 to
	 * mean "buffer consumed completely". If it points to a character, than
	 * the buffer continues until a \0
	 */
	if (*me->s_ATBufferCur == '\0') {
		/* empty buffer */
		me->s_ATBufferCur = me->pty->buffer;
		p_read = me->s_ATBufferCur;
	} else {		/* *s_ATBufferCur != '\0' */

		/* there's data in the buffer from the last read */

		// skip over leading newlines
		while (*me->s_ATBufferCur == '\r' || *me->s_ATBufferCur == '\n')
			me->s_ATBufferCur++;
		PHS_LOGD("Send thread's TID [%d] CHNMNG:findNextEOL:\n", me->tid);
		p_eol = findNextEOL(me->s_ATBufferCur);
		PHS_LOGD("Send thread's TID [%d] CHNMNG:end findNextEOL:\n",
		       me->tid);
		if (p_eol == NULL) {
			/* a partial line. move it up and prepare to read more */
			size_t len;
			len = strlen(me->s_ATBufferCur);
			memmove(me->pty->buffer, me->s_ATBufferCur, len + 1);
			p_read = me->pty->buffer + len;
			me->s_ATBufferCur = me->pty->buffer;
		}

		/* Otherwise, (p_eol !- NULL) there is a complete line  */
		/* that will be returned the while () loop below        */
	}
	while (p_eol == NULL) {
		if (0 == MAX_AT_RESPONSE - (p_read - me->pty->buffer)) {
			PHS_LOGE("\n ERROR: pty Input line exceeded buffer\n");

			/* ditch buffer and start over again */
			me->s_ATBufferCur = me->pty->buffer;
			*me->s_ATBufferCur = '\0';
			p_read = me->pty->buffer;
		}

		do {
			count = -1;
			FD_ZERO(&rfds);
			FD_SET(me->pty->pty_fd, &rfds);
			res = select(me->pty->pty_fd + 1, &rfds, NULL, NULL, NULL);
			if ( res > 0) {
				if(FD_ISSET(me->pty->pty_fd, &rfds))
					count =
			    		read(me->pty->pty_fd, p_read,
				 	MAX_AT_RESPONSE - (p_read - me->pty->buffer));
			}
		} while (count < 0 && errno == EINTR);

		if (count > 0) {
			AT_DUMP("CHNMNG:pty readline << ", p_read, count);
			p_read[count] = '\0';

			// skip over leading newlines
			while (*me->s_ATBufferCur == '\r'
			       || *me->s_ATBufferCur == '\n')
				me->s_ATBufferCur++;
			PHS_LOGD("Send thread's TID [%d] CHNMNG:findNextEOL:\n",
			       me->tid);
			p_eol = findNextEOL(me->s_ATBufferCur);
			PHS_LOGD
			    ("Send thread's TID [%d] CHNMNG:end findNextEOL:\n",
			     me->tid);
			p_read += count;
		} else if (count <= 0) {

			/* read error encountered or EOF reached */
			if (count == 0) {
				PHS_LOGE("atchannel: EOF reached");
			} else {
				PHS_LOGE("atchannel: read error %s", strerror(errno));
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
	//PHS_LOGD("Send thread's TID [%d] CHNMNG:AT> %s\n", me->tid, ret);
	return ret;
}
void *send_data(struct send_thread_t *me)
{
	int received = 0;
	char *buffer = me->pty->buffer;
	char *atstr = NULL;
	me->s_ATBufferCur = buffer;
	*buffer = '\0';
	char tmp_buff[SERIAL_BUFFSIZE / 8];
	pid_t tid = gettid();
	me->pty->tid = tid;
	me->tid = tid;
	PHS_LOGD("Send thread's TID [%d] enter send data thread :pty=%s\n", tid,
	       me->pty->name);
	memset(buffer, 0, SERIAL_BUFFSIZE);
	while (1) {

		//PDEBUG("Waiting for command\n");
		memset(buffer, 0, SERIAL_BUFFSIZE);
		atstr = readline(me);	//read a completed at response
		if (atstr != NULL) { 
			tmp_buff[0] = '\0';
			snprintf(tmp_buff, sizeof(tmp_buff), "%s%c", atstr, me->end_char);
			memset(atstr, 0, strlen(atstr));
			received = strlen(tmp_buff);
			PHS_LOGD
			    ("Send thread's TID [%d] PS_PTY : %s Received %d bytes command[%s]\n",
		    	 tid, me->pty->name, received, tmp_buff);

			//mutex_lock(&me->pty->receive_lock);  //get channel lock
			me->ops->send_thread_deliver_cmd_req(me, tmp_buff, received);
		}
	}
	return NULL;
}
struct send_thread_ops sndthreadops = {
	/*## operation deliver_cmd_req(char*,pty_type) */
	.send_thread_deliver_cmd_req = send_thread_deliver_cmd_req,
	.send_data = send_data,
};
struct send_thread_ops *send_thread_get_operations(void)
{
	return &sndthreadops;
}
