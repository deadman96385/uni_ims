
/*
 *
 * pty.h: pty implementation for the phoneserver
 *
 *
 */

#ifndef pty_H
#define pty_H

#include "os_api.h"
#include "cmux.h"
struct pty_ops {

	/*## operation clear_wait_resp_flag() */
	int (*pty_clear_wait_resp_flag) (void *const pty);

	/*## operation enter_edit_mode() */
	int (*pty_enter_edit_mode) (void *const pty, void *callback,
				    int userdata);

	/*## operation get_at_cmd() */
	int (*pty_read) (void *const pty, char *buf, int len);

	/*## operation set_wait_resp_flag() */
	int (*pty_set_wait_resp_flag) (void *const pty);

	/*## operation write() */
	int (*pty_write) (void *const pty, char *buf, int len);
};
struct pty_t {

	/***    User explicit entries    ***/
	void *me;
	char *buffer;		/*##for reading from channel pty  ## */
	int (*edit_callback) (struct pty_t * pty, char *str, int len, int userdata);	/*## attribute edit_callback */
	int edit_mode;		/*## sms text edit_mode */
	int user_data;
	int pty_fd;		/*##  pty_fd for channel pty */
	pid_t tid;
	int used;
	int type;		/*##  type */
	char *name;
	int wait_resp;		/*## flag for wait_resp */
	sem_t receive_lock;	/*## write_lock  to avoid multi access */
	sem_t get_mux_lock;
	sem_t write_lock;	/*## write_lock  to avoid multi access */
	struct pty_ops *ops;
	struct cmux_t *mux;
#if defined CONFIG_DUAL_SIM
	int wait_flag;
	int sms;
#endif
	int cmgs_cmgw_set_result;
};
struct pty_ops *pty_get_operations(void);

#endif /*  */
