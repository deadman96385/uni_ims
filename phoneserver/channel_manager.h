
/*
 *
 * channel_manager.h: channel manager implementation for the phoneserver
 *
 *
 */

#ifndef channel_manager_H

#define channel_manager_H

/* link itsAdapter */
#include "adapter.h"
/* link itsCmux */
#include "cmux.h"
/* link itsCmux_config */
#include "config.h"
/* link itsPty */
#include "pty.h"
/* link itsReceive_thread */
#include "receive_thread.h"
/* link itsRedirect_table */
//#include "redirect_table.h"
/* link itsSend_thread */
#include "send_thread.h"
#include "os_api.h"
struct chnmng_ops {

	/* Operations */
	/*## operation free_cmux(cmux_struct) */
	void (*channel_manager_free_cmux) (void *const chnmng,
					   struct cmux_t * cmux);

	/*## operation get_cmux(cmd_type) */
	struct cmux_t *(*channel_manager_get_cmux) (void *const chnmng,
						    const AT_CMD_TYPE_T type,
						    int block);
};
struct channel_manager_t {
	void *me;
	char itsBuffer[MULTI_CHN_NUM][4 + SERIAL_BUFFSIZE];	/*## link itsBuffer PHS_MUX_NUM+PTY_CHN_NUM */
	struct cmux_t itsCmux[MUX_NUM];	/*## link itsCmux  11 */
	struct chns_config_t *itschns_config;	/*## link itsCmux_config */
	struct pty_t itsPty[MULTI_PTY_CHN_NUM];	/*## link itsPty */
	struct pty_t itsMngPty;
	struct receive_thread_t itsReceive_thread[MULTI_PHS_MUX_NUM];	/*## link itsReceive_thread  5 */
	struct send_thread_t itsSend_thread[MULTI_PTY_CHN_NUM];	/*## link itsSend_thread  3 */
	sem get_mux_lock;
	sem array_lock;
	int block_count;
	struct chnmng_ops *ops;
	sem gsm_sem;
	sem csm_sem;
	sem psm_sem;
	sem miscm_sem;
	sem indm_sem;

	pty_t *gsm_wait_array[GSM_WAIT_NUM];
	pty_t *csm_wait_array[CSM_WAIT_NUM];
	pty_t *psm_wait_array[PSM_WAIT_NUM];
	pty_t *stm_wait_array[STMM_WAIT_NUM];
	pty_t *ssm_wait_array[GSM_WAIT_NUM];
	pty_t *smsm_wait_array[GSM_WAIT_NUM];
	pty_t *smstm_wait_array[GSM_WAIT_NUM];
	pty_t *stkm_wait_array[GSM_WAIT_NUM];
	pty_t *pbkm_wait_array[GSM_WAIT_NUM];
	pty_t *nwm_wait_array[GSM_WAIT_NUM];
	pty_t *simm_wait_array[GSM_WAIT_NUM];

	pty_t *slow1_wait_array[SLOW1_WAIT_NUM];
	pty_t *normal1_wait_array[NORMAL1_WAIT_NUM];
	pty_t *slow2_wait_array[SLOW2_WAIT_NUM];
	pty_t *normal2_wait_array[NORMAL2_WAIT_NUM];
	pty_t *slow3_wait_array[SLOW3_WAIT_NUM];
	pty_t *normal3_wait_array[NORMAL3_WAIT_NUM];
	pty_t *slow4_wait_array[SLOW4_WAIT_NUM];
	pty_t *normal4_wait_array[NORMAL4_WAIT_NUM];

};

/* Operations */
struct pty_t *channel_manager_get_default_ind_pty(void);
struct pty_t *channel_manager_single_get_eng_ind_pty(void);
struct pty_t *channel_manager_multi_get_eng_ind_pty(void);

/*## operation free_cmux(cmux_struct) */
void channel_manager_free_cmux(const struct cmux_t *cmux);

/*## operation get_cmux(cmd_type) */
struct cmux_t *channel_manager_get_cmux(const AT_CMD_TYPE_T type, int block);

#endif /*  */
