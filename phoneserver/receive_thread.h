/*
 *
 * receive_thread.h: channel mux implementation for the phoneserver 

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */
#ifndef receive_thread_H

#define receive_thread_H
#include "os_api.h"
#include "cmux.h"
/*----------------------------------------------------------------------------*/
/* receive_thread.h                                                                  */
/*----------------------------------------------------------------------------*/
struct receive_thread_ops {

/* Operations */

/*## operation get_at_cmd() */
	void (*receive_thread_deliver_cmd_resp) (void *const me, char *cmd_str,
						 int len);
	void *(*receive_data) (void *const me);
};
struct receive_thread_t {

    /***    User explicit entries    ***/
	void *me;
	int prority;
	char *s_ATBufferCur;
	char *buffer;
	sem_t resp_cmd_lock;	/*## write_lock  to avoid multi access */
	struct cmux_t *mux;	/*## attribute cmux */
	char end_char;
	thread_t thread;
	pid_t tid;
	struct receive_thread_ops *ops;
};
struct receive_thread_ops *receive_thread_get_operations(void);

#endif /*  */
