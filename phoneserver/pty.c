/*
 *
 * pty.c: channel pty implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "pty.h"
#include "os_api.h"

/* Operations */

/*## operation clear_wait_resp_flag() */
int pty_clear_wait_resp_flag(void *const pty)
{
	struct pty_t *me = (struct pty_t *)pty;
	if (me->wait_resp)
		me->wait_resp = 0;
	else {
		PHS_LOGD
		    ("PS_PTY also be cleared before pty_clear_wait_resp_flag pty:%s\n",
		     me->name);
	}
	return 0;
}

/*## operation enter_edit_mode() */
int pty_enter_edit_mode(void *const pty, void *callback, unsigned long userdata)
{
	int ret = 0;
	struct pty_t *me = (struct pty_t *)pty;
	if (!me->edit_mode) {
		me->edit_callback = callback;
		me->user_data = userdata;
		me->edit_mode = 1;
	} else {
		PHS_LOGD("PS_PTY error reenter pty_enter_edit_mode pty:%s\n",
		       me->name);
	}
	return ret;
}

/*## operation get_at_cmd() */
int pty_read(void *const pty, char *buf, int len)
{
	int ret = 0;
	struct pty_t *me = (struct pty_t *)pty;
	if (me->pty_fd > 0) {
		while (len) {
			ret = read(me->pty_fd, buf, len);
			if (ret > 0) {
				len -= ret;
				buf += ret;
			} else if (ret < 0) {
				PHS_LOGE("PS_PTY  ERROR read error:%s\n",
				       me->name);
				return ret;
			}
		}
	}
	return ret;
}

/*## operation set_wait_resp_flag() */
int pty_set_wait_resp_flag(void *const pty)
{
	struct pty_t *me = (struct pty_t *)pty;
	if (me->wait_resp) {
		PHS_LOGD("PS_PTY  ERROR reenter pty_set_wait_resp_flag pty:%s\n",
		       me->name);
	} else {
		me->wait_resp = 1;
	}
	return 0;
}

/*## operation write() */
int pty_write(void *const pty, char *buf, int len)
{
	int ret = 0;
	struct pty_t *me = (struct pty_t *)pty;

	sem_lock(&me->write_lock);
	PHS_LOGD("pty_write get lock\n");
	if (me->pty_fd > 0) {
		PHS_LOGD("PS_PTY:%s   write :%s\n", me->name, buf);
		while (len) {
			ret = write(me->pty_fd, buf, len);
			if (ret > 0) {
				len -= ret;
				buf += ret;
			} else if (ret <= 0) {
				break;
			}
		}
	}
	sem_unlock(&me->write_lock);
	PHS_LOGD("pty_write free lock\n");
	return ret;
}
struct pty_ops ptyops = {
/* Operations */

	/*## operation clear_wait_resp_flag() */
	.pty_clear_wait_resp_flag = pty_clear_wait_resp_flag,
	/*## operation enter_edit_mode() */
	.pty_enter_edit_mode = pty_enter_edit_mode,
	/*## operation get_at_cmd() */
	.pty_read = pty_read,
	/*## operation set_wait_resp_flag() */
	.pty_set_wait_resp_flag = pty_set_wait_resp_flag,
	/*## operation write() */
	.pty_write = pty_write,
};
struct pty_ops *pty_get_operations(void)
{
	return &ptyops;
}
