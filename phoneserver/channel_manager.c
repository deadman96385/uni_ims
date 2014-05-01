/*
 *
 * channel_manager.c: channel manager implementation for the phoneserver

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
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "channel_manager.h"
#include "os_api.h"
#include "pty.h"
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <cutils/sockets.h>
#include "version.h"
#include <private/android_filesystem_config.h>
#include "cutils/properties.h"
#include <hardware_legacy/power.h>

#define TD_SIM_NUM  "ro.modem.t.count"
#define W_SIM_NUM  "ro.modem.w.count"
#define L_SIM_NUM  "ro.modem.l.count"

#define MUX_TD_DEV  "ro.modem.t.tty"
#define MUX_W_DEV  "ro.modem.w.tty"
#define MUX_L_DEV  "ro.modem.l.tty"

#undef  PHS_LOGD
#define PHS_LOGD(x...)  ALOGD( x )

#define ANDROID_WAKE_LOCK_NAME "phoneserver-init"

const char *modem = NULL;
int multiSimMode;
struct channel_manager_t chnmng;

struct chns_config_t single_chns_data = {.pty = {
	{.dev_str = "/dev/CHNPTY0",.index = 0,.type = IND,.prority = 1}, 	/*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY1",.index = 1,.type = AT,.prority = 1},
	{.dev_str = "/dev/CHNPTY2",.index = 2,.type = AT,.prority = 1},		/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY3",.index = 3,.type = AT,.prority = 1},		/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY12",.index = 12,.type = AT,.prority = 1},	/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY13",.index = 13,.type = AT,.prority = 1},	/*## attribute at_pty */
},.mux = {
	{.dev_str = "",.index = 0,.type = INDM,.prority = 20},	/*## attribute misc_mux */
	{.dev_str = "",.index = 1,.type = SIMM,.prority = 20},	/*## attribute sim_mux */
	{.dev_str = "",.index = 2,.type = SSM,.prority = 20},	/*## attribute ss_mux */
	{.dev_str = "",.index = 3,.type = PBKM,.prority = 20},	/*## attribute pbk_mux */
	{.dev_str = "",.index = 4,.type = STMM,.prority = 20},	/*## attribute stm_mux */
	{.dev_str = "",.index = 5,.type = GSM,.prority = 20},	/*## attribute gsm_mux */
	{.dev_str = "",.index = 6,.type = CSM,.prority = 20},	/*## attribute cs_mux */
	{.dev_str = "",.index = 7,.type = PSM,.prority = 20},	/*## attribute ps_mux */
	{.dev_str = "",.index = 8,.type = NWM,.prority = 20},	/*## attribute nw_mux */
	{.dev_str = "",.index = 9,.type = STKM,.prority = 20},	/*## attribute stk_mux */
	{.dev_str = "",.index = 10,.type = SMSM,.prority = 20},	/*## attribute smsm_mux */
	//{.dev_str = "/dev/ts0710mux15",.index = 15,.type = SMSTM,.prority = 20},	/*## attribute smstm_mux */
},
};

struct chns_config_t multi_chns_data = {.pty = {
	{.dev_str = "/dev/CHNPTY0",.index = 0,.type = IND_SIM1,.prority = 1},/*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY1",.index = 1,.type = AT_SIM1,.prority = 1}, /*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY2",.index = 2,.type = AT_SIM1,.prority = 1}, /*## attribute at_pty */

	{.dev_str = "/dev/CHNPTY3",.index = 3,.type = IND_SIM2,.prority = 1}, /*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY4",.index = 4,.type = AT_SIM2,.prority = 1},	/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY5",.index = 5,.type = AT_SIM2,.prority = 1},	/*## attribute at_pty */

	{.dev_str = "/dev/CHNPTY6",.index = 6,.type = IND_SIM3,.prority = 1}, /*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY7",.index = 7,.type = AT_SIM3,.prority = 1},	/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY8",.index = 8,.type = AT_SIM3,.prority = 1},	/*## attribute at_pty */

	{.dev_str = "/dev/CHNPTY9",.index = 9,.type = IND_SIM4,.prority = 1}, /*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY10",.index = 10,.type = AT_SIM4,.prority = 1}, /*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY11",.index = 11,.type = AT_SIM4,.prority = 1}, /*## attribute at_pty */

	{.dev_str = "/dev/CHNPTY12",.index = 12,.type = AT_SIM1,.prority = 1}, /*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY13",.index = 13,.type = AT_SIM1,.prority = 1}, /*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY14",.index = 14,.type = AT_SIM2,.prority = 1}, /*## attribute at_pty */

}, .mux = {
	{.dev_str = "",.index = 0,.type = INDM_SIM1,.prority = 20}, /*## attribute misc_mux */
	{.dev_str = "",.index = 1,.type = ATM1_SIM1,.prority = 20},
	{.dev_str = "",.index = 2,.type = ATM2_SIM1,.prority = 20},

	{.dev_str = "",.index = 3,.type = INDM_SIM2,.prority = 20}, /*## attribute misc_mux */
	{.dev_str = "",.index = 4,.type = ATM1_SIM2,.prority = 20},
	{.dev_str = "",.index = 5,.type = ATM2_SIM2,.prority = 20},

	{.dev_str = "",.index = 6,.type = INDM_SIM3,.prority = 20}, /*## attribute misc_mux */
	{.dev_str = "",.index = 7,.type = ATM1_SIM3,.prority = 20},
	{.dev_str = "",.index = 8,.type = ATM2_SIM3,.prority = 20},

	{.dev_str = "",.index = 9,.type = INDM_SIM4,.prority = 20}, /*## attribute misc_mux */
	{.dev_str = "",.index = 10,.type = ATM1_SIM4,.prority = 20},
	{.dev_str = "",.index = 11,.type = ATM2_SIM4,.prority = 20},
},
};


struct chns_config_t single_chns_data_l = {.pty = {
	{.dev_str = "/dev/CHNPTY0",.index = 0,.type = IND,.prority = 1}, 	/*## attribute ind_pty */
	{.dev_str = "/dev/CHNPTY1",.index = 1,.type = AT, .prority = 1},	/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY2",.index = 2,.type = AT, .prority = 1},	/*## attribute at_pty */
	{.dev_str = "/dev/CHNPTY3",.index = 3,.type = AT, .prority = 1},	/*## attribute at_pty */
},.mux = {
	{.dev_str = "",.index = 0,.type = INDM,.prority = 20},	/*## attribute misc_mux */
	{.dev_str = "",.index = 1,.type = SIMM,.prority = 20},	/*## attribute sim_mux */
	{.dev_str = "",.index = 2,.type = SSM,.prority = 20},	/*## attribute ss_mux */
	{.dev_str = "",.index = 3,.type = PBKM,.prority = 20},	/*## attribute pbk_mux */
	{.dev_str = "",.index = 4,.type = STMM,.prority = 20},	/*## attribute stm_mux */
	{.dev_str = "",.index = 5,.type = GSM,.prority = 20},	/*## attribute gsm_mux */
	{.dev_str = "",.index = 6,.type = CSM,.prority = 20},	/*## attribute cs_mux */
	{.dev_str = "",.index = 7,.type = PSM,.prority = 20},	/*## attribute ps_mux */
	{.dev_str = "",.index = 8,.type = NWM,.prority = 20},	/*## attribute nw_mux */
	{.dev_str = "",.index = 9,.type = STKM,.prority = 20},	/*## attribute stk_mux */
	{.dev_str = "",.index = 10,.type = SMSM,.prority = 20},	/*## attribute smsm_mux */
},
};


/*
 * get_pty - get a pty master/slave pair and chown the slave side
 * to the uid given.  Assumes slave_name points to >= 16 bytes of space.
 */
static int get_pty(master_fdp, slave_fdp, slave_name, uid)
int *master_fdp;
int *slave_fdp;
char *slave_name;
int uid;
{
    int i, mfd, sfd = -1;
    char pty_name[20];
    struct termios tios;

#ifdef TIOCGPTN
    /*
     * Try the unix98 way first.
     */
    PHS_LOGD("CHNMNG:to open /dev/ptmx");
    mfd = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
    if (mfd >= 0) {
        int ptn, rett = 0;

        //grantpt(mfd);
        //unlockpt(mfd);
        PHS_LOGD("CHNMNG:/dev/ptmx opened");
        rett = ioctl(mfd, TIOCGPTN, &ptn);
        PHS_LOGD("CHNMNG:/dev/ptmx opened rett=%x ptn=%d", rett, ptn);
        if (rett >= 0) {
            sprintf(pty_name, "/dev/pts/%d", ptn);
            if (chmod(pty_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
                PHS_LOGE
                    ("CHNMNG: Couldn't change pty slave %s's mode",
                     pty_name);
            }
#ifdef TIOCSPTLCK
            ptn = 0;
            if ((rett = ioctl(mfd, TIOCSPTLCK, &ptn)) < 0) {
                PHS_LOGE
                    ("CHNMNG: Couldn't unlock pty slave %s rett=%x",
                     pty_name, rett);
            }
#endif
            if ((sfd = open(pty_name, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
                PHS_LOGE
                    ("CHNMNG: Couldn't open pty slave %s sfd=%x",
                     pty_name, sfd);
            }
        } else {
            PHS_LOGE("CHNMNG:fail to get pty number");
        }
    }
#endif /* TIOCGPTN */
    if (sfd < 0) {
        /* the old way - scan through the pty name space */
        PHS_LOGD("CHNMNG: begin scan pty name space");
        for (i = 0; i < 64; ++i) {
            sprintf(pty_name, "/dev/pty%c%x", 'p' + i / 16, i % 16);
            mfd = open(pty_name, O_RDWR | O_NONBLOCK, 0);
            if (mfd >= 0) {
                pty_name[5] = 't';
                sfd = open(pty_name, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
                if (sfd >= 0) {
                    fchown(sfd, uid, -1);
                    fchmod(sfd, S_IRUSR | S_IWUSR);
                    break;
                }
                close(mfd);
            }
        }
    }
    if (sfd < 0)
        return 0;
    strncpy(slave_name, pty_name, 16);
    *master_fdp = mfd;
    *slave_fdp = sfd;
    if (tcgetattr(sfd, &tios) == 0) {
        tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
        tios.c_cflag |= CS8 | CREAD | CLOCAL;
        tios.c_iflag = IGNPAR;
        tios.c_oflag = 0;
        tios.c_lflag = 0;
        if (tcsetattr(sfd, TCSAFLUSH, &tios) < 0) {
            PHS_LOGE("CHNMNG couldn't set attributes on pty");
        }
    } else
        PHS_LOGE("CHNMNG couldn't get attributes on pty");
    PHS_LOGD("CHNMNG get pty OK mpty=%d", mfd);
    return 1;
}

// get master pty
static int create_communication_channel(char *slave_name)
{
    int pty_master = -1;
    int pty_slave = -1;
    char pty_name[16];
    if (!get_pty(&pty_master, &pty_slave, &pty_name[0], getuid())) {
        PHS_LOGE("CHNMNG: Couldn't allocate pseudo-tty");
        return -1;
    }
    //create symlink for Application,link pty_name to slave_name
    unlink(slave_name);
    if (symlink(pty_name, slave_name) != 0) {
        PHS_LOGE("CHNMNG: Can't create symbolic link %s -> %s\n",
                slave_name, pty_name);
    }
    return pty_master;
}

/*cmux0-9:
  cmux0  indicator,misc at; (P)
  cmux1  vt data
  cmux2-5 ppp

  cmux6  cs's AT (P)
  cmux7  ps's AT (P)
  cmux8  general (P)
  cmux9  cmmb
  cmux10  cp log

  AT_CMD_TYPE_CS,
  AT_CMD_TYPE_PS,
  AT_CMD_TYPE_SS,
  AT_CMD_TYPE_SMS,
  AT_CMD_TYPE_NW,
  AT_CMD_TYPE_PBK,
  AT_CMD_TYPE_SIM,
  AT_CMD_TYPE_GEN,
  AT_CMD_TYPE_INVALID
*/
static cmux_t *find_type_cmux(struct channel_manager_t *const me, mux_type type)
{
    cmux_t *mux = NULL;
    int i;

    sem_lock(&me->get_mux_lock);
    if(!strcmp(modem, "l")) {
        for (i = 0; i < LTE_MUX_CHN_NUM; i++) {
            PHS_LOGI("LTE find_type_cmux  i = %d, me->itsCmux[i].type = %d, me->itsCmux[i].in_use = %d\n" ,i, me->itsCmux[i].type, me->itsCmux[i].in_use);
            if (me->itsCmux[i].type == (int)type && me->itsCmux[i].in_use == 0) {
                PHS_LOGI("LTE find_type_cmux type = %d\n",type);
                mux = &me->itsCmux[i];
                mux->in_use = 1;
                break;
            }
        }

    } else {
        for (i = 0; i < MUX_NUM; i++) {
            PHS_LOGI("find_type_cmux  i = %d, me->itsCmux[i].type = %d, me->itsCmux[i].in_use = %d\n" ,i, me->itsCmux[i].type, me->itsCmux[i].in_use);
            if (me->itsCmux[i].type == (int)type && me->itsCmux[i].in_use == 0) {
                PHS_LOGI(" find_type_cmux type = %d\n",type);
                mux = &me->itsCmux[i];
                mux->in_use = 1;
                break;
            }
        }
    }
    sem_unlock(&me->get_mux_lock);
    return mux;
}

static cmux_t *single_find_cmux(struct channel_manager_t *const me, AT_CMD_TYPE_T type)
{
    cmux_t *mux = NULL;

    switch (type) {
        case AT_CMD_TYPE_GEN:
            PHS_LOGI("TYPE: GSM\n");
            return find_type_cmux(me, GSM);

        case AT_CMD_TYPE_SS:
            PHS_LOGI("TYPE: SSM\n");
            return find_type_cmux(me, SSM);

        case AT_CMD_TYPE_SMS:
            PHS_LOGI("TYPE: SMSM\n");
            return find_type_cmux(me, SMSM);

        case AT_CMD_TYPE_SMST:
            PHS_LOGI("TYPE: SMSTM\n");
            return find_type_cmux(me, STKM);

        case AT_CMD_TYPE_NW:
            PHS_LOGI("TYPE:NWM\n");
            return find_type_cmux(me, NWM);

        case AT_CMD_TYPE_PBK:
            PHS_LOGI("TYPE:PBKM\n");
            return find_type_cmux(me, PBKM);

        case AT_CMD_TYPE_SIM:
            PHS_LOGI("TYPE:SIMM\n");
            return find_type_cmux(me, SIMM);

        case AT_CMD_TYPE_STK:
            PHS_LOGI("TYPE:STKM\n");
            return find_type_cmux(me, STKM);

        case AT_CMD_TYPE_CS:
            PHS_LOGI("TYPE:CSM\n");
            return find_type_cmux(me, CSM);

        case AT_CMD_TYPE_PS:
            PHS_LOGI("TYPE:PSM\n");
            return find_type_cmux(me, PSM);

        case AT_CMD_TYPE_STM:
            return find_type_cmux(me, STMM);

        default:
            PHS_LOGI(" CHNMNG single_find_cmux invalid cmd type \n");
    }
    return mux;
}

static cmux_t *multi_find_cmux(struct channel_manager_t *const me, AT_CMD_TYPE_T type)
{
    cmux_t *mux = NULL;

    switch (type) {
        case AT_CMD_TYPE_SLOW1:
		PHS_LOGI("TYPE: AT_CMD_TYPE_SLOW1\n");
		return find_type_cmux(me, ATM1_SIM1);

        case AT_CMD_TYPE_NORMAL1:
		PHS_LOGI("TYPE: AT_CMD_TYPE_NORMAL1\n");
		return find_type_cmux(me, ATM2_SIM1);

        case AT_CMD_TYPE_SLOW2:
		PHS_LOGI("TYPE: AT_CMD_TYPE_SLOW2\n");
		return find_type_cmux(me, ATM1_SIM2);

        case AT_CMD_TYPE_NORMAL2:
		PHS_LOGI("TYPE: AT_CMD_TYPE_NORMAL2\n");
		return find_type_cmux(me, ATM2_SIM2);

        case AT_CMD_TYPE_SLOW3:
		PHS_LOGI("TYPE: AT_CMD_TYPE_SLOW3\n");
		return find_type_cmux(me, ATM1_SIM3);

        case AT_CMD_TYPE_NORMAL3:
		PHS_LOGI("TYPE: AT_CMD_TYPE_NORMAL3\n");
		return find_type_cmux(me, ATM2_SIM3);

        case AT_CMD_TYPE_SLOW4:
		PHS_LOGI("TYPE: AT_CMD_TYPE_SLOW4\n");
		return find_type_cmux(me, ATM1_SIM4);

        case AT_CMD_TYPE_NORMAL4:
		PHS_LOGI("TYPE: AT_CMD_TYPE_NORMAL4\n");
		return find_type_cmux(me, ATM2_SIM4);

        default:
            PHS_LOGI(" CHNMNG multi_find_cmux invalid cmd type \n");
    }
    return mux;
}

pty_t *find_pty(struct channel_manager_t * const me, pid_t tid)
{
    int i = 0;
    int pty_chn_num;

    if(multiSimMode == 1)
        pty_chn_num = MULTI_PTY_CHN_NUM;
    else
        pty_chn_num = SINGLE_PTY_CHN_NUM;

    for (; i < pty_chn_num; i++) {
        if (me->itsSend_thread[i].tid == tid)
            break;
    }
    if(i >= pty_chn_num)
        return NULL;

    if(multiSimMode == 1)
        return me->itsSend_thread[i].pty;
    else
        return &me->itsPty[i];
}

static pty_t **single_get_mux_wait_array(struct channel_manager_t *const me,
				  AT_CMD_TYPE_T type, int *array_size)
{
    switch (type) {
        case AT_CMD_TYPE_GEN:
            *array_size = GSM_WAIT_NUM;
            return me->gsm_wait_array;

        case AT_CMD_TYPE_SS:
            *array_size = GSM_WAIT_NUM;
            return me->ssm_wait_array;

        case AT_CMD_TYPE_SMS:
            *array_size = GSM_WAIT_NUM;
            return me->smsm_wait_array;

        case AT_CMD_TYPE_SMST:
            *array_size = GSM_WAIT_NUM;
            return me->smstm_wait_array;
        case AT_CMD_TYPE_NW:
            *array_size = GSM_WAIT_NUM;
            return me->nwm_wait_array;

        case AT_CMD_TYPE_PBK:
            *array_size = GSM_WAIT_NUM;
            return me->pbkm_wait_array;

        case AT_CMD_TYPE_SIM:
            *array_size = GSM_WAIT_NUM;
            return me->simm_wait_array;

        case AT_CMD_TYPE_STK:
            *array_size = GSM_WAIT_NUM;
            return me->stkm_wait_array;
        case AT_CMD_TYPE_CS:
            *array_size = CSM_WAIT_NUM;
            return me->csm_wait_array;
        case AT_CMD_TYPE_PS:
            *array_size = PSM_WAIT_NUM;
            return me->psm_wait_array;
        case AT_CMD_TYPE_STM:
            *array_size = STMM_WAIT_NUM;
            return me->stm_wait_array;
        default:
            PHS_LOGI(" CHNMNG single_get_mux_wait_array invalid cmd type \n");
            return NULL;
    }
}

static pty_t **multi_get_mux_wait_array(struct channel_manager_t *const me,
				  AT_CMD_TYPE_T type, int *array_size)
{
    switch (type) {
        case AT_CMD_TYPE_SLOW1:
            *array_size = SLOW1_WAIT_NUM;
            return me->slow1_wait_array;
        case AT_CMD_TYPE_NORMAL1:
            *array_size = NORMAL1_WAIT_NUM;
            return me->normal1_wait_array;

        case AT_CMD_TYPE_SLOW2:
            *array_size = SLOW2_WAIT_NUM;
            return me->slow2_wait_array;
        case AT_CMD_TYPE_NORMAL2:
            *array_size = NORMAL2_WAIT_NUM;
            return me->normal2_wait_array;

        case AT_CMD_TYPE_SLOW3:
            *array_size = SLOW3_WAIT_NUM;
            return me->slow3_wait_array;
        case AT_CMD_TYPE_NORMAL3:
            *array_size = NORMAL3_WAIT_NUM;
            return me->normal3_wait_array;

        case AT_CMD_TYPE_SLOW4:
            *array_size = SLOW4_WAIT_NUM;
            return me->slow4_wait_array;
        case AT_CMD_TYPE_NORMAL4:
            *array_size = NORMAL4_WAIT_NUM;
            return me->normal4_wait_array;

        default:
            PHS_LOGE(" CHNMNG multi_get_mux_wait_array invalid cmd type ");
            return NULL;
    }
}

static void remove_wait_array(pty_t ** wait_array, pty_t * pty, int array_size)
{
    int i;

    for (i = 0; i < (array_size - 1); i++) {
        wait_array[i] = wait_array[i + 1];
    }
    wait_array[array_size - 1] = NULL;
}
static int add_wait_array(pty_t ** wait_array, pty_t * pty, int array_size)
{
    int i;
    for (i = 0; i < array_size; i++) {
        if (NULL == wait_array[i]) {
            wait_array[i] = pty;
            break;
        }
    }
    return i;
}

static void print_array(pty_t ** pty, int array_size)
{
    int i;
    for (i = 0; i < array_size; i++) {
        if (pty[i] != NULL)
            PHS_LOGI("[%d]:%d ", i, pty[i]->tid);
    }
    //PHS_LOGI("\n");
}

/*## operation getdfcmux(cmd_type) */
static cmux_t *chnmng_get_cmux(void *const chnmng, const AT_CMD_TYPE_T type,
			       int block)
{
    cmux_t *mux = NULL;
    pid_t tid;
    pty_t *pty = NULL;
    pty_t **wait_array = NULL;
    int array_size = 0;
    int not_found = 0;
    tid = gettid();
    struct channel_manager_t *me = (struct channel_manager_t *)chnmng;
    PHS_LOGI("Send thread TID [%d] CHNMNG enter channel_manager_get_cmux ",
            tid);
    pty = find_pty(me, tid);
    int index = 0;

    //find free cmux
    PHS_LOGI("Before add wait array pty=%p", pty);
    if(multiSimMode == 1)
        wait_array = multi_get_mux_wait_array(me, type, &array_size);
    else
        wait_array = single_get_mux_wait_array(me, type, &array_size);
    if (block) {
        sem_lock(&me->array_lock);
        PHS_LOGI("Before add wait array");
        print_array(wait_array, array_size);
        index = add_wait_array(wait_array, pty, array_size);
        PHS_LOGI("After add wait array");
        print_array(wait_array, array_size);
        sem_unlock(&me->array_lock);
    } else {
        if(multiSimMode == 1)
            mux = multi_find_cmux(me, type);
        else
            mux = single_find_cmux(me, type);
    }

    while (mux == NULL && block) {
        if (index == 0) {
            if(multiSimMode == 1)
                mux = multi_find_cmux(me, type);
            else
                mux = single_find_cmux(me, type);
        }
        if (mux)
            break;
        PHS_LOGI
            ("Send thread TID [%d] CHNMNG block at channel_manager_get_cmux ",
             tid);
        sem_lock(&pty->get_mux_lock);
        if (mux == 0) {
            if(multiSimMode == 1)
                mux = multi_find_cmux(me, type);
            else
                mux = single_find_cmux(me, type);
        }
    }
    if (mux) {
        mux->pty = pty;
        mux->cmd_type = type;
        PHS_LOGI
            ("Send thread TID [%d] CHNMNG Leave channel_manager_get_cmux:%s ",
             tid, mux->name);
    }
    return mux;
}

void select_send_thread_run(struct channel_manager_t *const me,
			    AT_CMD_TYPE_T type)
{
    int array_size;
    pty_t **wait_array = NULL;

    PHS_LOGI("Enter select_send_thread_run");
    if(multiSimMode == 1)
        wait_array = multi_get_mux_wait_array(me, type, &array_size);
    else
        wait_array = single_get_mux_wait_array(me, type, &array_size);

    pty_t *pty = wait_array[0];
    if (pty != NULL) {
        PHS_LOGI("select thread tid [%d] run...", wait_array[0]->tid);
        sem_unlock(&pty->get_mux_lock);
    }
    PHS_LOGI("Leave select_send_thread_run");
}

/*## operation free_cmux(cmux_struct) */
static void chnmng_free_cmux(void *const chnmng, struct cmux_t *cmux)
{
    PHS_LOGI("CHNMNG enter channel_manager_free_cmux cmux=%s ", cmux->name);
    int array_size = 0;
    int type = cmux->cmd_type;
    struct channel_manager_t *me = (struct channel_manager_t *)chnmng;
    pty_t **wait_array = NULL;
    pty_t *pty = NULL;
    pid_t tid = gettid();

    if(multiSimMode == 1)
        wait_array = multi_get_mux_wait_array(me, type, &array_size);
    else
        wait_array = single_get_mux_wait_array(me, type, &array_size);

    sem_lock(&me->array_lock);
    pty = wait_array[0];
    if (pty != NULL) {
        PHS_LOGI("CHNMNG enter channel_manager_free_cmux pty=%p ", pty);
        if (pty == cmux->pty) {
            PHS_LOGI("[%d] Before remove wait array", tid);
            print_array(wait_array, array_size);
            remove_wait_array(wait_array, pty, array_size);
            PHS_LOGI("[%d] After remove wait array", tid);
            print_array(wait_array, array_size);
        } else {
            PHS_LOGI("free cmux error");
        }
    }
    sem_lock(&me->get_mux_lock);
    cmux->ops->cmux_free(cmux);
    select_send_thread_run(me, cmux->cmd_type);
    sem_unlock(&me->get_mux_lock);
    sem_unlock(&me->array_lock);
    PHS_LOGI("CHNMNG Leave channel_manager_free_cmux cmux=%s ", cmux->name);
}

/*## operation free_cmux(cmux_struct) */
void channel_manager_free_cmux(const struct cmux_t *cmux)
{
    chnmng_free_cmux(chnmng.me, (struct cmux_t *)cmux);
}

/*## operation get_cmux(cmd_type) */
struct cmux_t *channel_manager_get_cmux(const AT_CMD_TYPE_T type, int block)
{
    return chnmng_get_cmux(chnmng.me, type, block);
}

struct pty_t *channel_manager_get_default_ind_pty(void)
{
    return &(chnmng.itsPty[0]);
}

struct pty_t *channel_manager_single_get_eng_ind_pty(void)
{
    return &(chnmng.itsPty[4]);
}

struct pty_t *channel_manager_get_sim1_ind_pty(void)
{
    return &(chnmng.itsPty[0]);
}

struct pty_t *channel_manager_get_sim2_ind_pty(void)
{
    return &(chnmng.itsPty[3]);
}

struct pty_t *channel_manager_get_sim3_ind_pty(void)
{
    return &(chnmng.itsPty[6]);
}

struct pty_t *channel_manager_get_sim4_ind_pty(void)
{
    return &(chnmng.itsPty[9]);
}

struct pty_t *channel_manager_multi_get_eng_ind_pty(void)
{
    return &(chnmng.itsPty[12]);
}

struct chnmng_ops chnmng_operaton = {
    .channel_manager_free_cmux = chnmng_free_cmux,
    .channel_manager_get_cmux = chnmng_get_cmux,
};

void chnmng_buffer_Init(struct channel_manager_t *const me)
{
    memset(me->itsBuffer, 0, sizeof(me->itsBuffer));
}

char *chnmng_find_buffer(struct channel_manager_t *const me)
{
    int chn_num;
    char *ret = NULL;
    int i = 0;

    if(multiSimMode == 1)
        chn_num = MULTI_CHN_NUM;
    else
        chn_num = SINGLE_CHN_NUM;

    for (i = 0; i < chn_num; i++) {
        if (me->itsBuffer[i][0] == 0) {
            me->itsBuffer[i][0] = 1;
            ret = &me->itsBuffer[i][4];
            break;
        }
    }
    PHS_LOGD("chnmng_find_buffer");
    return ret;
}

/*## operation initialize all cmux objects*/
static void chnmng_cmux_Init(struct channel_manager_t *const me)
{
    char prop[PROPERTY_VALUE_MAX] = {0};
    thread_sched_param sched;
    int tid = 0;
    int policy = 0;
    int index;
    char muxname[20] = {0};
    int i = 0;
    int fd;
    int phs_mux_num;
    int size;
    int chn_num = MUX_NUM;
    struct chns_config_t chns_data;
    struct termios ser_settings;

    if(!strcmp(modem, "t")) {
        property_get(MUX_TD_DEV, prop, "/dev/ts0710mux");
    } else if(!strcmp(modem, "w")) {
        property_get(MUX_W_DEV, prop, "/dev/ts0710mux");
    } else if(!strcmp(modem, "l")) {
        property_get(MUX_L_DEV, prop, "/dev/sdiomux");
        chn_num = LTE_MUX_CHN_NUM ;
    } else {
        PHS_LOGE("Wrong modem parameter");
        exit(-1);
    }

    PHS_LOGD("cmux_Init: mux device is %s", prop);
    memset(me->itsCmux, 0, sizeof(struct cmux_t) * chn_num);

    for (i = 0; i < chn_num; i++) {
        snprintf(muxname, sizeof(muxname), "%s%d", prop, i);
        me->itsCmux[i].type = RESERVE;
        me->itsCmux[i].ops = cmux_get_operations();
        me->itsCmux[i].ops->cmux_free(&me->itsCmux[i]);
        PHS_LOGD("CHNMNG: open mux:%s",muxname);
retry:
        fd = open(muxname, O_RDWR);
        if(fd < 0) {
			PHS_LOGE("Phoneserver exit: open %s failed, errno = %d (%s)\n ", muxname, errno, strerror(errno));
			if (errno == EAGAIN)
			   goto retry;
			exit(-1);
        }
    }

    if(multiSimMode == 1) {
        phs_mux_num = MULTI_PHS_MUX_NUM;
        chns_data = multi_chns_data;
    } else {
        phs_mux_num = SINGLE_PHS_MUX_NUM;
        chns_data = single_chns_data;
    }

    if(!strcmp(modem, "l")) {
        property_get(MUX_L_DEV, prop, "/dev/sdiomux");
        phs_mux_num = LTE_MUX_CHN_NUM ;
        chns_data = single_chns_data_l;
    }

    for (i = 0; i < phs_mux_num; i++) {
        index = chns_data.mux[i].index;
        me->itsCmux[i].buffer = chnmng_find_buffer(&chnmng);
        snprintf(muxname, sizeof(muxname), "%s%d", prop, index);
        size = sizeof(me->itsCmux[i].name);
        strncpy(me->itsCmux[i].name, muxname, size);
        me->itsCmux[i].name[size - 1] = '\0';
        me->itsCmux[i].type = chns_data.mux[i].type;
        me->itsCmux[i].muxfd =open(me->itsCmux[i].name, O_RDWR);
        if(me->itsCmux[i].muxfd < 0) {
            PHS_LOGE("Phoneserver exit: open mux:%s failed, errno = %d (%s)", me->itsCmux[i].name, errno, strerror(errno));
            exit(1);
        }
        if(isatty(me->itsCmux[i].muxfd)) {
             tcgetattr(me->itsCmux[i].muxfd, &ser_settings);
             cfmakeraw(&ser_settings);
             tcsetattr(me->itsCmux[i].muxfd, TCSANOW, &ser_settings);
        }
        PHS_LOGD("CHNMNG: open mux:%s fd=%d",	me->itsCmux[i].name, me->itsCmux[i].muxfd);
        sem_init(&me->itsReceive_thread[i].resp_cmd_lock, 0, 1);
        sem_init(&me->itsCmux[i].cmux_lock, 0, 0);
        cond_init(&me->itsCmux[i].cond_timeout, NULL);
        mutex_init(&me->itsCmux[i].mutex_timeout, NULL);
        me->itsReceive_thread[i].mux = &me->itsCmux[i];
        me->itsReceive_thread[i].ops = receive_thread_get_operations();
    }
    PHS_LOGD("CHNMNG: cmux_Init done");
}

/*## operation initialize all pty objects */
static void chnmng_pty_Init(struct channel_manager_t *const me)
{
    int pty_chn_num;
    int i = 0;
    char *buff = 0;
    thread_sched_param sched;
    int tid = 0;
    int policy = 0;
    int index;
    struct chns_config_t chns_data;
    char pre_ptyname[20] = {0};
    char ptyname[20] = {0};
    int size;

    memset(&me->itsPty, 0, sizeof(struct pty_t) * MULTI_PTY_CHN_NUM);

    if(!strcmp(modem, "t")) {
        strcpy(pre_ptyname, "/dev/CHNPTYT");
    } else if(!strcmp(modem, "w")) {
         strcpy(pre_ptyname, "/dev/CHNPTYW");
    } else if(!strcmp(modem, "l")) {
         strcpy(pre_ptyname, "/dev/CHNPTYL");
    } else {
        PHS_LOGE("Wrong modem parameter");
	exit(-1);
    }

    if(multiSimMode == 1) {
        pty_chn_num = MULTI_PTY_CHN_NUM;
        chns_data = multi_chns_data;
    } else {
        pty_chn_num = SINGLE_PTY_CHN_NUM;
        chns_data = single_chns_data;
    }

    if(!strcmp(modem, "l")) {
        pty_chn_num = LTE_PTY_CHN_NUM ;
        chns_data = single_chns_data_l;
    } 

    /*set attris to default value */
    for (i = 0; i < pty_chn_num; i++) {
        me->itsPty[i].ops = pty_get_operations();
        sem_init(&me->itsPty[i].write_lock, 0, 1);
        sem_init(&me->itsPty[i].receive_lock, 0, 1);
        sem_init(&me->itsPty[i].get_mux_lock, 0, 0);
        me->itsPty[i].ops->pty_clear_wait_resp_flag(&me->itsPty[i]);
        me->itsPty[i].type = RESERVE;
    }
    for (i = 0; i < pty_chn_num; i++) {
        index = chns_data.pty[i].index;
        snprintf(ptyname, sizeof(ptyname), "%s%d", pre_ptyname, index);
        size = sizeof(me->itsPty[i].name);
        strncpy(me->itsPty[i].name, ptyname, size);
        me->itsPty[i].name[size - 1] = '\0';
        me->itsPty[i].type = chns_data.pty[i].type;
        me->itsPty[i].pty_fd =create_communication_channel(me->itsPty[i].name);
        buff = chnmng_find_buffer(&chnmng);
        if (buff == NULL) {
            PHS_LOGE("ERROR chnmng_pty_Init no buffer");
        }
        me->itsPty[i].buffer = buff;
        sem_init(&me->itsSend_thread[i].req_cmd_lock, 0, 1);
        me->itsSend_thread[i].pty = &me->itsPty[i];
        me->itsSend_thread[i].ops = send_thread_get_operations();
    }
}

void chnmng_start_thread(struct channel_manager_t *const me)
{
    int phs_mux_num;
    int pty_chn_num;
    int i = 0;
    int tid = 0;
    int policy = 0;
    thread_sched_param sched;
    struct chns_config_t chns_data;

    if(multiSimMode == 1) {
        phs_mux_num = MULTI_PHS_MUX_NUM;
        pty_chn_num = MULTI_PTY_CHN_NUM;
        chns_data = multi_chns_data;
    } else {
        phs_mux_num = SINGLE_PHS_MUX_NUM;
        pty_chn_num = SINGLE_PTY_CHN_NUM;
        chns_data = single_chns_data;
    }

    if(!strcmp(modem, "l")) {
        phs_mux_num = LTE_MUX_CHN_NUM;
        pty_chn_num = LTE_PTY_CHN_NUM;
        chns_data = single_chns_data_l;
    } 

    for (i = 0; i < phs_mux_num; i++) {  //receive thread
        tid =thread_creat(&me->itsReceive_thread[i].thread, NULL,
                (void *)me->itsReceive_thread[i].ops->receive_data,
                (void *)&(me->itsReceive_thread[i]));
        if (tid < 0) {
            PHS_LOGE("ERROR chnmng_mux pthread_create \n ");
            break;
        }
        thread_getschedparam((int)&me->itsReceive_thread[i].thread, &policy, &sched);
        PHS_LOGD("chnmng_mux thread: policy=%d", policy);
        if (policy != SCHED_OTHER) {
            PHS_LOGD("chnmng_mux thread: policy=%d", policy);
            sched.sched_priority = chns_data.mux[i].prority;
            thread_setschedparam((int) &me->itsReceive_thread[i].thread, policy, &sched );
        }
    }

    for (i = 0; i < pty_chn_num; i++) {  //send thread
        tid =thread_creat(&me->itsSend_thread[i].thread, NULL,
                (void *)me->itsSend_thread[i].ops->send_data,
                (void *)&(me->itsSend_thread[i]));
        if (tid < 0) {
            PHS_LOGE("ERROR chnmng_pty pthread_create");
            break;
        }
        thread_getschedparam((int)&me->itsSend_thread[i].thread, &policy, &sched);
        PHS_LOGD("chnmng_pty thread: policy=%d", policy);
        if (policy != SCHED_OTHER) {
            PHS_LOGD("chnmng_pty thread: policy=%d!", policy);
            sched.sched_priority = chns_data.pty[i].prority;
            thread_setschedparam((int)&me->itsSend_thread[i].thread, policy, &sched );
        }
    }
}

static void get_partial_wakeLock() {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, ANDROID_WAKE_LOCK_NAME);
}

static void release_wakeLock() {
     release_wake_lock(ANDROID_WAKE_LOCK_NAME);
}

/*## operation initialize all channel manager's objects  according to phone server configuration  file*/
static void channel_manager_init(void)
{
    chnmng.me = &chnmng;
    chnmng.ops = &chnmng_operaton;
    sem_init(&chnmng.get_mux_lock, 0, 1);

    sem_init(&chnmng.array_lock, 0, 1);
    chnmng.block_count = 0;

    get_partial_wakeLock();
    chnmng_buffer_Init(chnmng.me);
    chnmng_cmux_Init(chnmng.me);
    chnmng_pty_Init(chnmng.me);

    setuid(AID_SYSTEM); /* switch user to system  */

    chnmng_start_thread(chnmng.me);
    release_wakeLock();
}

extern void ps_service_init(void);
extern sem sms_lock;

static void usage(const char *argv)
{
    PHS_LOGE("Usage: %s -m <modem>", argv);
    PHS_LOGE("modem: t (td modem)");
    PHS_LOGE("modem: w (wcdma modem)");
    PHS_LOGE("modem: l (lte modem)");
    exit(-1);
}

int soc_client = -1;
static void *detect_at_no_response(void *par)
{
    int soc_fd;
    char socket_name[10];

    if(!strcmp(modem, "t")) {
        strcpy(socket_name, "phstd");
    } else if(!strcmp(modem, "w")) {
         strcpy(socket_name, "phsw");
    } else if(!strcmp(modem, "l")) {
         strcpy(socket_name, "phsl");
    } else {
        PHS_LOGE("Wrong modem parameter");
        exit(-1);
    }
    soc_fd = socket_local_server(socket_name,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (soc_fd < 0) {
        PHS_LOGE("%s: cannot create socket %s", __func__, socket_name);
        exit(-1);
    }

   for(;;) {
        PHS_LOGI("%s: waiting for socket client ...", __func__);
        if ( (soc_client = accept(soc_fd, NULL, NULL)) == -1)
        {
            PHS_LOGE("%s: accept error", __func__);
            continue;
        }
        PHS_LOGD("%s: accept soc_client=%d", __func__, soc_client);
    }
    close(soc_client);
}

int main(int argc, char *argv[])
{
    char prop[PROPERTY_VALUE_MAX];
    pthread_t tid;
    int ret;

    PHS_LOGD("chnmng start phone server");
    PHS_LOGD("Phoneserver version: %s ",version_string);

    if (0 == strcmp(argv[1], "-m") && (argc > 2)) {
        modem = argv[2];
    } else {
        usage(argv[0]);
    }

    if(!strcmp(modem, "t")) {
        property_get(TD_SIM_NUM, prop, "");
    } else if(!strcmp(modem, "w")) {
        property_get(W_SIM_NUM, prop, "");
    } else if(!strcmp(modem, "l")) {
        property_get(L_SIM_NUM, prop, "");
    } else {
	usage(argv[0]);
    }

    PHS_LOGD("Current modem is %s, Current sim is %s", modem, prop);

    if(strcmp(prop, "1"))
        multiSimMode = 1;
    else
        multiSimMode = 0;

    sem_init(&sms_lock, 0, 1);

    ret = pthread_create(&tid, NULL, (void*)detect_at_no_response, NULL);
    if(ret < 0) 
        PHS_LOGE("create detect_at_no_response thread failed");

    ps_service_init();
    channel_manager_init();

    while(1)
        pause();
}
