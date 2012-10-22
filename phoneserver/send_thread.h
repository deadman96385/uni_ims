/*
 *
 * send_thread.h: channel  implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#ifndef send_thread_H

#define send_thread_H

#include "os_api.h"
#include "pty.h"
struct send_thread_ops {

	/*## operation deliver_cmd_req(char*,pty_type) */
	void (*send_thread_deliver_cmd_req) (void *const me, char *cmd_str,
					     int len);

	/*## operation send_at_cmd() */
	void *(*send_data) (void *const me);
};
struct send_thread_t {
	void *me;

	//  int adp_send_handler;             /*## attribute adp_send_handler */
	int prority;
	struct pty_t *pty;	/*## attribute pty */
	thread_t thread;
	sem_t req_cmd_lock;
	char *s_ATBufferCur;
	struct send_thread_ops *ops;
	char end_char;
	pid_t tid;
};
struct send_thread_ops *send_thread_get_operations(void);

#endif /*  */
