
/*
 *
 * cmux.h: cmux implementation for the phoneserver
 *
 *
 */

#ifndef cmux_H

#define cmux_H

#include "adapter.h"
#include "pty.h"
#include "os_api.h"
struct cmux_ops {

	/* Operations */

	/*## operation close() */
	int (*cmux_close) (struct cmux_t * const cmux);

	/*## operation deregist_cmd_callback() */
	int (*cmux_deregist_cmd_callback) (struct cmux_t * const me);

	/*## operation free() */
	int (*cmux_free) (struct cmux_t * const cmux);

	/*## operation read() */
	int (*cmux_read) (struct cmux_t * const me, char *buf, int len);

	/*## operation regist_cmd_callback() */
	int (*cmux_regist_cmd_callback) (struct cmux_t * const me,
					 void *callback_fn, int userdata);

	/*## operation write() */
	int (*cmux_write) (struct cmux_t * const me, char *buf, int len);
};
struct cmux_t {

	/***    User explicit entries    ***/
	void *me;
	char *buffer;		/*## buffer for read tsmux */
	int muxfd;		/*## muxfd of mux dev */
	int type;		/*## mux channel type */
	int cmd_type;		/*## cmd  type */
	int userdata;		/*## userdata for  */
	int wait_resp;		/*## flag for wait_resp */
	int in_use;
	int (*callback) (AT_CMD_RSP_T * resp_req, int usdata);
	char name[30];
	struct cmux_ops *ops;
	sem cmux_lock;
	mutex mutex_timeout;
	cond cond_timeout;
	int cp_blked;
	struct pty_t *pty;
};
struct cmux_ops *cmux_get_operations(void);

#endif /*  */
