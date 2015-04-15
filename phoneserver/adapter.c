/*
 * Copyright (C)
 *
 *

 * ================================================================================*/
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
#include "adapter.h"
#include "at_tok.h"
#include "cmux.h"
#include "pty.h"
#include "os_api.h"
#include "ps_service.h"
#include "config.h"
#include "channel_manager.h"
#include <cutils/properties.h>

#define RIL_SIM1_ABSENT_PROPERTY  "ril.sim1.absent"
#define BLOCKED_MAX_COUNT 5

extern int multiSimMode;
static int sim1_absent =-1, sim2_exist = -1;
extern const char *modem;
extern int soc_client;

struct cmd_table {
    AT_CMD_ID_T cmd_id;
    AT_CMD_TYPE_T cmd_type;
    char *cmd_str;
    int len;
    int (*cvt_func) (AT_CMD_REQ_T * req);
    int timeout;	// timeout for response
};
struct ind_table {
    AT_CMD_ID_T cmd_id;
    char *cmd_str;
    int len;
    int (*cvt_func) (AT_CMD_IND_T * req);
};

#define AT_CMD_STR(str)  (str),sizeof((str))-1

static int tem_value = 0;
static int rsp_tem[4] = { 0 };
static int g_call_status_array[10] = {0};
sem sms_lock;

extern struct ppp_info_struct ppp_info[];

const struct cmd_table single_at_cmd_cvt_table[] = {
    {AT_CMD_CGDCONT_READ, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDCONT?;"),
        cvt_generic_cmd_req, 10},
    {AT_CMD_CGDCONT_READ, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDCONT?"),
        cvt_cgdcont_read_req, 5},
    {AT_CMD_CGDCONT_TEST, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDCONT=?"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGDATA_TEST, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDATA=?"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGQMIN, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGQMIN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGQREQ, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGQREQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEQMIN, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGEQMIN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEQREQ, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGEQREQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEREP, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGEREP"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGDCONT_SET, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDCONT="),
        cvt_cgdcont_set_req, 5},
    {AT_CMD_CGDATA_SET, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGDATA="),
        cvt_cgdata_set_req, 120},
    {AT_CMD_CGACT_READ, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGACT?"),
        cvt_cgact_query_cmd_req, 5},
    {AT_CMD_CGACT_SET, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGACT=1"),
        cvt_cgact_act_req, 600},
    {AT_CMD_CGACT_SET_0, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGACT=0"),
        cvt_cgact_deact_req, 50},

    {AT_CMD_CLIR, AT_CMD_TYPE_SS, AT_CMD_STR("AT+COLR"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CLIR, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CLIR"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CCFC, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CCFC"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CLCK, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CLCK"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CGATT, AT_CMD_TYPE_PS, AT_CMD_STR("AT+CGATT"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=?"),
        cvt_generic_cmd_req, 180},
    {AT_CMD_CCWA_READ, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CCWA?"),
        cvt_ccwa_cmd_req, 30},
    {AT_CMD_CCWA_TEST, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CCWA=?"),
        cvt_ccwa_cmd_req, 5},
    {AT_CMD_CCWA_SET, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CCWA="),
        cvt_ccwa_cmd_req, 50},
    {AT_CMD_COLP_READ, AT_CMD_TYPE_SS, AT_CMD_STR("AT+COLP?"),
        cvt_colp_cmd_req, 30},
    {AT_CMD_COLP_TEST, AT_CMD_TYPE_SS, AT_CMD_STR("AT+COLP=?"),
        cvt_colp_cmd_req, 5},
    {AT_CMD_COLP_SET, AT_CMD_TYPE_SS, AT_CMD_STR("AT+COLP="),
        cvt_colp_cmd_req, 50},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_CS, AT_CMD_STR("AT+ECHUPVT="),
        cvt_echupvt_set_req, 50},
    {AT_CMD_EVTS_SET, AT_CMD_TYPE_CS, AT_CMD_STR("AT+EVTS="),
        cvt_evts_set_req, 10},
    {AT_CMD_CRSM, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+CRSM"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_CMOD, AT_CMD_TYPE_CS, AT_CMD_STR("AT+CMOD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CLCC, AT_CMD_TYPE_CS, AT_CMD_STR("AT+ECTTY"),
        cvt_not_support_cmd_req, 5},


    {AT_CMD_CLIP_READ, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CLIP?"),
        cvt_clip_cmd_req, 30},
    {AT_CMD_CLIP_TEST, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CLIP=?"),
        cvt_clip_cmd_req, 5},
    {AT_CMD_CLIP_SET, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CLIP="),
        cvt_clip_cmd_req, 50},
    {AT_CMD_CSSN, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CSSN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CUSD_READ, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CUSD?"),
        cvt_cusd_cmd_req, 5},
    {AT_CMD_CUSD_TEST, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CUSD=?"),
        cvt_cusd_cmd_req, 5},
    {AT_CMD_CUSD_SET, AT_CMD_TYPE_SS, AT_CMD_STR("AT+CUSD="),
        cvt_cusd_cmd_req, 50},


    {AT_CMD_CREG_READ, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CREG?"),
        cvt_creg_cmd_req, 15},
    {AT_CMD_CREG_TEST, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CREG=?"),
        cvt_creg_cmd_req, 15},
    {AT_CMD_CREG_SET, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CREG="),
        cvt_creg_cmd_req, 15},
    {AT_CMD_CGREG_READ, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CGREG?"),
        cvt_cgreg_cmd_req, 15},
    {AT_CMD_CGREG_TEST, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CGREG=?"),
        cvt_cgreg_cmd_req, 15},
    {AT_CMD_CGREG_SET, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CGREG="),
        cvt_cgreg_cmd_req, 15},
    {AT_CMD_CEREG_READ, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CEREG?"),
        cvt_cereg_cmd_req, 15},
    {AT_CMD_CEREG_TEST, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CEREG=?"),
        cvt_cereg_cmd_req, 15},
    {AT_CMD_CEREG_SET, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CEREG="),
        cvt_cereg_cmd_req, 15},
    {AT_CMD_CSQ_TEST, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CSQ=?"),
        cvt_csq_test_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CSQ"),
        cvt_csq_action_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NW, AT_CMD_STR("AT+SPSCSQ"),
        cvt_spscsq_action_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CESQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CPOL, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CPOL"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=0"),
        cvt_cops_set_cmd_req0, 120},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=1"),
        cvt_cops_set_cmd_req1, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=2"),
        cvt_cops_set_cmd_req2, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=4"),
        cvt_cops_set_cmd_req4, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS?"),
        cvt_generic_cmd_req, 10},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS=3"),
        cvt_generic_cmd_req, 10},
    {AT_CMD_COPS, AT_CMD_TYPE_NW, AT_CMD_STR("AT+COPS"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NW, AT_CMD_STR("AT+ESQOPT=2"),
        cvt_esqopt2_cmd_req, 5},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NW, AT_CMD_STR("AT+ESQOPT="),
        cvt_esqopt01_cmd_req, 5},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NW, AT_CMD_STR("AT+ESQOPT"),
        cvt_generic_cmd_req, 5},


    {AT_CMD_EPIN_READ, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+EPIN?"),
        cvt_epin_test_req, 4},
    {AT_CMD_CPIN_READ, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+CPIN?"),
        cvt_generic_cmd_req, 4},
    {AT_CMD_CPIN_SET, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+CPIN="),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CPWD, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+CPWD"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_ECPIN2, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+ECPIN2"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_EUICC, AT_CMD_TYPE_SIM, AT_CMD_STR("AT+EUICC"),
        cvt_generic_cmd_req, 60},


    {AT_CMD_CMGS_TEST, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMGS=?"),
        cvt_cmgs_cmgw_test_req, 5},
    {AT_CMD_CMGS_SET, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMGS="),
        cvt_cmgs_cmgw_set_req, 10},
    {AT_CMD_SNVM_SET, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+SNVM=1"),
        cvt_snvm_set_req, 10},
    {AT_CMD_CMGW_TEST, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMGW=?"),
        cvt_cmgs_cmgw_test_req, 5},
    {AT_CMD_CMGW_SET, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMGW="),
        cvt_cmgs_cmgw_set_req, 10},

    {AT_CMD_CMGD, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMGD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CSCA, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CSCA"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CNMA, AT_CMD_TYPE_SMST, AT_CMD_STR("AT+CNMA"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CNMI, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CNMI"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CMMS, AT_CMD_TYPE_SMS, AT_CMD_STR("AT+CMMS"),
        cvt_generic_cmd_req, 5},

    {AT_CMD_CHLD, AT_CMD_TYPE_CS, AT_CMD_STR("AT+CHLD"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_VTS, AT_CMD_TYPE_CS, AT_CMD_STR("AT+VTS"),
        cvt_generic_cmd_req, 30},
    {AT_CMD_VTS, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+SDTMF"),
        cvt_generic_cmd_req, 30},
    {AT_CMD_VTD, AT_CMD_TYPE_CS, AT_CMD_STR("AT+VTD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_ATD_SET, AT_CMD_TYPE_CS, AT_CMD_STR("ATD"),
        cvt_generic_cmd_req, 40},
    {AT_CMD_ATA_SET, AT_CMD_TYPE_CS, AT_CMD_STR("ATA"),
        cvt_ata_cmd_req, 40},
    {AT_CMD_ATH_SET, AT_CMD_TYPE_CS, AT_CMD_STR("ATH"),
        cvt_ath_cmd_req, 40},
    {AT_CMD_CLCC, AT_CMD_TYPE_CS, AT_CMD_STR("AT+CLCC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_CS, AT_CMD_STR("AT+CHUPVT="),
	cvt_generic_cmd_req, 50},


    {AT_CMD_ESATPROFILE_SET, AT_CMD_TYPE_STK, AT_CMD_STR("AT+ESATPROFILE="),
        cvt_esatprofile_set_req, 5},
    {AT_CMD_ESATENVECMD_SET, AT_CMD_TYPE_STK, AT_CMD_STR("AT+ESATENVECMD="),
        cvt_esatenvecmd_set_req, 5},
    {AT_CMD_ESATTERMINAL_SET, AT_CMD_TYPE_STK, AT_CMD_STR("AT+ESATTERMINAL="),
        cvt_esatterminal_set_req, 5},
    {AT_CMD_ESATCAPREQ, AT_CMD_TYPE_STK, AT_CMD_STR("AT+ESATCAPREQ"),
        cvt_generic_cmd_req, 5},

    {AT_CMD_EBAND_SET, AT_CMD_TYPE_NW, AT_CMD_STR("AT+EBAND="),
        cvt_eband_set_req, 30},
    {AT_CMD_EBAND_QUERY, AT_CMD_TYPE_NW, AT_CMD_STR("AT+EBAND?"),
        cvt_eband_query_req, 5},
    {AT_CMD_CEER, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CEER"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CFUN, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CFUN"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CFUN, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+SFUN=2"),
        cvt_generic_cmd_req, 300},
    {AT_CMD_CFUN, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+SFUN=4"),
        cvt_generic_cmd_req, 120},
    {AT_CMD_CTZU, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CTZU"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CTZR, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CTZR"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CCWE, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CCWE"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CACM, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CACM"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CAMM, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CAMM"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CAOC, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CAOC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CPUC, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CPUC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGSN, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CGSN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CIMI, AT_CMD_TYPE_GEN, AT_CMD_STR("AT+CIMI"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGMR, AT_CMD_TYPE_NW, AT_CMD_STR("AT+CGMR"),
        cvt_generic_cmd_req, 5},

    {AT_CMD_UNKNOWN, AT_CMD_TYPE_GEN, AT_CMD_STR("AT"), cvt_generic_cmd_req,
        50},
    //  NULL,
};

const struct cmd_table multi_at_cmd_cvt_table[] = {
    {AT_CMD_CGDCONT_READ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGDCONT?"),
        cvt_cgdcont_read_req, 5},
    {AT_CMD_CGDCONT_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGDCONT=?"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGDATA_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGDATA=?"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGQMIN, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGQMIN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGQREQ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGQREQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEQMIN, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGEQMIN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEQREQ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGEQREQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGEREP, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGEREP"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGDCONT_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGDCONT="),
        cvt_cgdcont_set_req, 5},
    {AT_CMD_CGDATA_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGDATA="),
        cvt_cgdata_set_req, 120},
    {AT_CMD_CGACT_READ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGACT?"),
        cvt_cgact_query_cmd_req, 5},
    {AT_CMD_CGACT_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGACT=1"),
        cvt_cgact_act_req, 600},
    {AT_CMD_CGACT_SET_0, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGACT=0"),
        cvt_cgact_deact_req, 50},
    {AT_CMD_CFUN, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+SFUN=4"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CFUN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+SFUN=5"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_SAUTOATT_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+SAUTOATT="),
        cvt_generic_cmd_req, 50},

    {AT_CMD_CLIR, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+COLR"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CLIR, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CLIR"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CCFC, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CCFC"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CLCK, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CLCK"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CGATT, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CGATT"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+COPS=?"),
        cvt_generic_cmd_req, 180},
    {AT_CMD_CCWA_READ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CCWA?"),
        cvt_ccwa_cmd_req, 30},
    {AT_CMD_CCWA_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CCWA=?"),
        cvt_ccwa_cmd_req, 5},
    {AT_CMD_CCWA_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CCWA="),
        cvt_ccwa_cmd_req, 50},
    {AT_CMD_COLP_READ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+COLP?"),
        cvt_colp_cmd_req, 30},
    {AT_CMD_COLP_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+COLP=?"),
        cvt_colp_cmd_req, 5},
    {AT_CMD_COLP_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+COLP="),
        cvt_colp_cmd_req, 50},
    {AT_CMD_CLIP_READ, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CLIP?"),
        cvt_clip_cmd_req, 30},
    {AT_CMD_CLIP_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CLIP=?"),
        cvt_clip_cmd_req, 5},
    {AT_CMD_CLIP_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CLIP="),
        cvt_clip_cmd_req, 50},
    {AT_CMD_CMGS_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CMGS=?"),
        cvt_cmgs_cmgw_test_req, 5},
    {AT_CMD_CMGS_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CMGS="),
        cvt_cmgs_cmgw_set_req, 10},
    {AT_CMD_SNVM_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+SNVM=1"),
        cvt_snvm_set_req, 10},
    {AT_CMD_CMGW_TEST, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CMGW=?"),
        cvt_cmgs_cmgw_test_req, 5},
    {AT_CMD_CMGW_SET, AT_CMD_TYPE_SLOW, AT_CMD_STR("AT+CMGW="),
        cvt_cmgs_cmgw_set_req, 10},

    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ECHUPVT="),
        cvt_echupvt_set_req, 50},
    {AT_CMD_EVTS_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+EVTS="),
        cvt_evts_set_req, 10},
    {AT_CMD_CRSM, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CRSM"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_CMOD, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CMOD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CLCC, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ECTTY"),
        cvt_not_support_cmd_req, 5},
    {AT_CMD_CSSN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CSSN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CUSD_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CUSD?"),
        cvt_cusd_cmd_req, 5},
    {AT_CMD_CUSD_TEST, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CUSD=?"),
        cvt_cusd_cmd_req, 5},
    {AT_CMD_CUSD_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CUSD="),
        cvt_cusd_cmd_req, 50},
    {AT_CMD_CREG_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CREG?"),
        cvt_creg_cmd_req, 5},
    {AT_CMD_CREG_TEST, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CREG=?"),
        cvt_creg_cmd_req, 5},
    {AT_CMD_CREG_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CREG="),
        cvt_creg_cmd_req, 5},
    {AT_CMD_CGREG_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CGREG?"),
        cvt_cgreg_cmd_req, 5},
    {AT_CMD_CGREG_TEST, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CGREG=?"),
        cvt_cgreg_cmd_req, 5},
    {AT_CMD_CGREG_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CGREG="),
        cvt_cgreg_cmd_req, 5},
    {AT_CMD_CEREG_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CEREG?"),
        cvt_cereg_cmd_req, 5},
    {AT_CMD_CEREG_TEST, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CEREG=?"),
        cvt_cgreg_cmd_req, 5},
    {AT_CMD_CEREG_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CEREG="),
        cvt_cereg_cmd_req, 5},
    {AT_CMD_CSQ_TEST, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CSQ=?"),
        cvt_csq_test_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CSQ"),
        cvt_csq_action_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+SPSCSQ"),
        cvt_spscsq_action_req, 5},
    {AT_CMD_CSQ_ACTION, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CESQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CPOL, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CPOL"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS=0"),
        cvt_cops_set_cmd_req0, 120},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS=1"),
        cvt_cops_set_cmd_req1, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS=2"),
        cvt_cops_set_cmd_req2, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS=3"),
        cvt_generic_cmd_req, 10},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS=4"),
        cvt_cops_set_cmd_req4, 50},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS?"),
        cvt_generic_cmd_req, 10},
    {AT_CMD_COPS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+COPS"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESQOPT=2"),
        cvt_esqopt2_cmd_req, 5},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESQOPT="),
        cvt_esqopt01_cmd_req, 5},
    {AT_CMD_ESQOPT, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESQOPT"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_EPIN_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+EPIN?"),
        cvt_epin_test_req, 4},
    {AT_CMD_CPIN_READ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CPIN?"),
        cvt_generic_cmd_req, 4},
    {AT_CMD_CPIN_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CPIN="),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CPWD, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CPWD"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_ECPIN2, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ECPIN2"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_EUICC, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+EUICC"),
        cvt_generic_cmd_req, 60},
    {AT_CMD_CMGD, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CMGD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CSCA, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CSCA"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CNMA, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CNMA"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CNMI, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CNMI"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CMMS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CMMS"),
        cvt_generic_cmd_req, 5},

    {AT_CMD_CHLD, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CHLD"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_VTS, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+VTS"),
        cvt_generic_cmd_req, 30},
    {AT_CMD_VTD, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+VTD"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("ATDL"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_ATD_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("ATD"),
        cvt_generic_cmd_req, 40},
    {AT_CMD_ATA_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("ATA"),
        cvt_generic_cmd_req, 40},
    {AT_CMD_ATH_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("ATH"),
        cvt_generic_cmd_req, 40},
    {AT_CMD_CLCC, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CLCC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CHUPVT="),
        cvt_generic_cmd_req, 50},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+SPUSATCALLSETUP"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_ECHUPVT_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CHUP"),
        cvt_generic_cmd_req, 50},

    {AT_CMD_ESATPROFILE_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESATPROFILE="),
        cvt_esatprofile_set_req, 5},
    {AT_CMD_ESATENVECMD_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESATENVECMD="),
        cvt_esatenvecmd_set_req, 5},
    {AT_CMD_ESATTERMINAL_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESATTERMINAL="),
        cvt_esatterminal_set_req, 5},
    {AT_CMD_ESATCAPREQ, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+ESATCAPREQ"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_EBAND_SET, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+EBAND="),
        cvt_eband_set_req, 30},
    {AT_CMD_EBAND_QUERY, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+EBAND?"),
        cvt_eband_query_req, 5},
    {AT_CMD_CEER, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CEER"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CFUN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CFUN"),
        cvt_generic_cmd_req, 50},
    {AT_CMD_CFUN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+SFUN=2"),
        cvt_generic_cmd_req, 300},
    {AT_CMD_CTZU, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CTZU"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CTZR, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CTZR"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CCWE, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CCWE"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CACM, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CACM"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CAMM, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CAMM"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CAOC, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CAOC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CPUC, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CPUC"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGSN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CGSN"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CIMI, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CIMI"),
        cvt_generic_cmd_req, 5},
    {AT_CMD_CGMR, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT+CGMR"),
        cvt_generic_cmd_req, 5},

    {AT_CMD_UNKNOWN, AT_CMD_TYPE_NORMAL, AT_CMD_STR("AT"),
        cvt_generic_cmd_req, 50},
    //  NULL,
};

const struct ind_table at_ind_cvt_table[] = {
    {AT_CMD_CCWA_IND, AT_CMD_STR("+CCWA:"), cvt_generic_cmd_ind},
    {AT_CMD_CLIP_IND, AT_CMD_STR("+CLIP:"), cvt_generic_cmd_ind},
    {AT_CMD_COLP_IND, AT_CMD_STR("+COLP:"), cvt_generic_cmd_ind},
    {AT_CMD_CREG_IND, AT_CMD_STR("+CREG:"), cvt_generic_cmd_ind},
    {AT_CMD_CGREG_IND, AT_CMD_STR("+CGREG:"), cvt_generic_cmd_ind},
    {AT_CMD_CEREG_IND, AT_CMD_STR("+CEREG:"), cvt_generic_cmd_ind},
    {AT_CMD_CUSD_IND, AT_CMD_STR("+CUSD:"), cvt_generic_cmd_ind},
    {AT_CMD_CSQ_IND, AT_CMD_STR("+CSQ:"), cvt_csq_cmd_ind},
    {AT_CMD_CSQ_IND, AT_CMD_STR("+SPSCSQ:"), cvt_spscsq_cmd_ind},
    {AT_CMD_CCCM_IND, AT_CMD_STR("+CCCM:"), cvt_generic_cmd_ind},
    {AT_CMD_CRING_IND, AT_CMD_STR("+CRING:"), cvt_generic_cmd_ind},
    {AT_CMD_CSSI_IND, AT_CMD_STR("+CSSI:"), cvt_generic_cmd_ind},
    {AT_CMD_CSSU_IND, AT_CMD_STR("+CSSU:"), cvt_generic_cmd_ind},
    {AT_CMD_CTZV_IND, AT_CMD_STR("+CTZV:"), cvt_generic_cmd_ind},
    {AT_CMD_BUSY_IND, AT_CMD_STR("BUSY"), cvt_generic_cmd_ind},
    {AT_CMD_CONNECT_IND, AT_CMD_STR("CONNECT"), cvt_generic_cmd_ind},
    {AT_CMD_NO_CARRIER_IND, AT_CMD_STR("NO CARRIER"), cvt_generic_cmd_ind},
    {AT_CMD_NO_ANSWER_IND, AT_CMD_STR("NO ANSWER"), cvt_generic_cmd_ind},
    {AT_CMD_NO_DIALTONE_IND, AT_CMD_STR("NO DIALTONE"), cvt_generic_cmd_ind},
    {AT_CMD_RING_IND, AT_CMD_STR("RING"), cvt_generic_cmd_ind},
    {AT_CMD_CMTI_IND, AT_CMD_STR("+CMTI:"), cvt_generic_cmd_ind},
    {AT_CMD_CMT_IND, AT_CMD_STR("+CMT:"), cvt_generic_cmd_ind},
    {AT_CMD_CDSI_IND, AT_CMD_STR("+CDSI:"), cvt_null_cmd_ind},
    {AT_CMD_CDS_IND, AT_CMD_STR("+CDS:"), cvt_generic_cmd_ind},
    {AT_CMD_SIND_IND, AT_CMD_STR("+SIND:"), cvt_generic_cmd_ind},
    {AT_CMD_ECSQ_IND, AT_CMD_STR("+ECSQ:"), cvt_ecsq_cmd_ind},
    {AT_CMD_ECEER_IND, AT_CMD_STR("+ECEER:"), cvt_eceer_cmd_ind},
    {AT_CMD_ECIND_IND, AT_CMD_STR("+ECIND: 3"), cvt_ecind_cmd_ind},
    {AT_CMD_ECIND_IND, AT_CMD_STR("+ECIND:"), cvt_ecind0_cmd_ind},
    {AT_CMD_ESATCAPCNF_IND, AT_CMD_STR("+ESATCAPCNF"), cvt_generic_cmd_ind},
    {AT_CMD_ESATPROCMDIND_IND, AT_CMD_STR("+ESATPROCMDIND"),
        cvt_generic_cmd_ind},
    {AT_CMD_ESATENDSESSIONIND_IND, AT_CMD_STR("+ESATENDSESSIONIND"),
        cvt_generic_cmd_ind},
    {AT_CMD_ESATSETUPCALLACKIND_IND, AT_CMD_STR("+ESATSETUPCALLACKIND"),
        cvt_generic_cmd_ind},
    {AT_CMD_ESIMATDISPLAY_IND, AT_CMD_STR("+ESIMATDISPLAY"),
        cvt_generic_cmd_ind},
    {AT_CMD_EEMGINBFTM_IND, AT_CMD_STR("+EEMGINBFTM"), cvt_generic_cmd_ind},
    {AT_CMD_EEMGINFOBASIC_IND, AT_CMD_STR("+EEMGINFOBASIC"),
        cvt_generic_cmd_ind},
    {AT_CMD_EEMGINFOSVC_IND, AT_CMD_STR("+EEMGINFOSVC"),
        cvt_generic_cmd_ind},
    {AT_CMD_EEMGINFOPS_IND, AT_CMD_STR("+EEMGINFOPS"), cvt_generic_cmd_ind},
    {AT_CMD_ECIND_IND, AT_CMD_STR("+CGEV:"), cvt_null_cmd_ind},
    {AT_CMD_EEMGINFONC_IND, AT_CMD_STR("+EEMGINFONC"), cvt_generic_cmd_ind},
    {AT_CMD_CESQ_IND, AT_CMD_STR("+CESQ:"), cvt_cesq_cmd_ind}
};

int cvt_cmgs_edit_callback(pty_t * pty, char *cmd, int len, unsigned long user_data);
int cvt_cmgw_edit_callback(pty_t * pty, char *cmd, int len, unsigned long user_data);

// Returns 1 if found, 0 otherwise. needle must be null-terminated.
// strstr might not work because WebBox sends garbage before the first OKread
int findInBuf(char *buf, int len, char *needle)
{
    int i;
    int needleMatchedPos = 0;

    if (buf == NULL) {
        return 0;
    }

    if (needle[0] == '\0') {
        return 1;
    }
    for (i = 0; i < len; i++) {
        if (needle[needleMatchedPos] == buf[i]) {
            needleMatchedPos++;
            if (needle[needleMatchedPos] == '\0') {

                // Entire needle was found
                return 1;
            }
        } else {
            needleMatchedPos = 0;
        }
    }
    return 0;
}

int phoneserver_deliver_indicate(const cmux_t * cmux, char *cmd, int len)
{
    AT_CMD_IND_T cmd_ind;
    struct ind_table *item = NULL;
    int i, count;

    if (cmux == NULL || cmd == NULL) {
        return AT_RESULT_NG;
    }

    /* find command router */
    i = 0;
    count = sizeof(at_ind_cvt_table) / sizeof(at_ind_cvt_table[0]);

    for (i = 0; i < count; i++) {
        if (!strncasecmp
                (at_ind_cvt_table[i].cmd_str, cmd,
                 at_ind_cvt_table[i].len)) {
            item = (struct ind_table *)&at_ind_cvt_table[i];
            break;
        }
    }
    if (item == NULL) {
        return AT_RESULT_NG;
    }

    /* call router */
    cmd_ind.recv_cmux = (cmux_t *) cmux;
    cmd_ind.ind_str = cmd;
    cmd_ind.len = len;
    cmd_ind.ind_id = item->cmd_id;
    return item->cvt_func(&cmd_ind);
}

int phoneserver_deliver_indicate_default(const cmux_t * cmux, char *cmd,
					 int len)
{
    int ret;
    char ind_str[MAX_AT_CMD_LEN];
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;	

    memset(ind_str, 0, MAX_AT_CMD_LEN);
    ret = phoneserver_deliver_indicate(cmux, cmd, len);
    if (ret != AT_RESULT_OK) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        snprintf(ind_str, sizeof(ind_str), "%s%s%s", "\r\n", cmd, "\n");
        if(ind_pty && ind_pty->ops && len < MAX_AT_CMD_LEN)
            ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
        if(ind_eng_pty && ind_eng_pty->ops && len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));
    }
    return AT_RESULT_OK;
}

void read_message(AT_CMD_RSP_T * rsp_req, int cmd_len, int spare_len,
		  char *content)
{
    cmux_t *mux = rsp_req->recv_cmux;
    int count;
    memcpy(content, rsp_req->rsp_str + cmd_len, rsp_req->len - cmd_len);
    content += rsp_req->len;

    do {
        count = read(mux->muxfd, content, spare_len);
        if (count < 0)
            continue;
        spare_len -= count;
        content += count;
    } while (spare_len > 0 && errno == EINTR);
}

int phoneserver_deliver_at_cmd(const pty_t * pty, char *cmd, int len)
{
    AT_CMD_REQ_T cmd_req;
    int ret;
    struct cmd_table *item = NULL;
    int i, count;
    char *str_tmp;
    pty_t *ptyp = (pty_t *) pty;

    if (pty == NULL || cmd == NULL) {
        return AT_RESULT_NG;
    }

    /* deliver data for edit mode first */
    if (pty->edit_mode) {
        pty->edit_callback((pty_t *) pty, cmd, len, pty->user_data);
        ptyp->edit_mode = 0;
        return AT_RESULT_OK;
    }

    /* find command router */
    i = 0;
    if(multiSimMode == 1) {
        count = sizeof(multi_at_cmd_cvt_table) / sizeof(multi_at_cmd_cvt_table[0]);
        for (i = 0; i < count; i++) {
            if (!strncasecmp
                    (multi_at_cmd_cvt_table[i].cmd_str, cmd,
                     multi_at_cmd_cvt_table[i].len)) {
                item = (struct cmd_table *)&multi_at_cmd_cvt_table[i];
                break;
            }
        }
    } else {
        count = sizeof(single_at_cmd_cvt_table) / sizeof(single_at_cmd_cvt_table[0]);
        for (i = 0; i < count; i++) {
            if (!strncasecmp
                    (single_at_cmd_cvt_table[i].cmd_str, cmd,
                     single_at_cmd_cvt_table[i].len)) {
                item = (struct cmd_table *)&single_at_cmd_cvt_table[i];
                break;
            }
        }
    }
    if (item == NULL) {
        PHS_LOGE("\n Can not find a correct command router for cmd: %s\n", cmd);
        adapter_pty_write_error((pty_t *) pty, CME_ERROR_NOT_SUPPORT);
        return AT_RESULT_NG;
    }

    if(multiSimMode == 1) {
        switch (pty->type){
            case AT_SIM1:
                if(item->cmd_type == AT_CMD_TYPE_SLOW)
                    cmd_req.cmd_type = AT_CMD_TYPE_SLOW1;
                else if(item->cmd_type == AT_CMD_TYPE_NORMAL)
                    cmd_req.cmd_type = AT_CMD_TYPE_NORMAL1;
                else
                   PHS_LOGE("wrong cmd_type!");
                break;
            case AT_SIM2:
                if(item->cmd_type == AT_CMD_TYPE_SLOW)
                    cmd_req.cmd_type = AT_CMD_TYPE_SLOW2;
                else if(item->cmd_type == AT_CMD_TYPE_NORMAL)
                    cmd_req.cmd_type = AT_CMD_TYPE_NORMAL2;
                else
                    PHS_LOGE("wrong cmd_type!");
                break;
            case AT_SIM3:
                if(item->cmd_type == AT_CMD_TYPE_SLOW)
                    cmd_req.cmd_type = AT_CMD_TYPE_SLOW3;
                else if(item->cmd_type == AT_CMD_TYPE_NORMAL)
                    cmd_req.cmd_type = AT_CMD_TYPE_NORMAL3;
                else
                    PHS_LOGE("wrong cmd_type!");
                break;
            case AT_SIM4:
                if(item->cmd_type == AT_CMD_TYPE_SLOW)
                    cmd_req.cmd_type = AT_CMD_TYPE_SLOW4;
                else if(item->cmd_type == AT_CMD_TYPE_NORMAL)
                    cmd_req.cmd_type = AT_CMD_TYPE_NORMAL4;
                else
                    PHS_LOGE("wrong cmd_type!");
                break;
            default:
                PHS_LOGE("AT command is written to the error PTY channel!pty->type=%d\n",pty->type);
        }
    } else {
        cmd_req.cmd_type = item->cmd_type;
    }

    /* call router */
    cmd_req.recv_pty = (pty_t *) pty;
    cmd_req.cmd_str = cmd;
    cmd_req.len = len;
    cmd_req.cmd_id = item->cmd_id;
    cmd_req.timeout = item->timeout;
    ret = item->cvt_func(&cmd_req);
    if (ret < 0) {
        PHS_LOGD("\n phoneserver_deliver_at_cmd failed!\n");
        adapter_pty_write_error((pty_t *)pty,CME_ERROR_NOT_SUPPORT);
        return AT_RESULT_NG;
    } else if (ret > 0) {
        pty->ops->pty_set_wait_resp_flag((void *const)pty);
        return AT_RESULT_PROGRESS;
    } else {
        return ret;
    }
}

int phoneserver_deliver_at_rsp(const cmux_t * cmux, char *rsp, int len)
{
    int ret = AT_RESULT_NG;
    AT_CMD_RSP_T rsp_req;
    pid_t tid;
    tid = gettid();

    if (cmux == NULL || rsp == NULL) {
        PHS_LOGE("cmux is NULL || rsp is NULL\n");
        return AT_RESULT_NG;
    }
    
    if(multiSimMode == 1) {
        if ((cmux->type == INDM_SIM1)	|| (cmux->type == INDM_SIM2) ||
                (cmux->type == INDM_SIM3) || (cmux->type == INDM_SIM4) ) {
            PHS_LOGD("[%d] INDM\n", tid);
            return phoneserver_deliver_indicate_default(cmux, rsp, len);
        }
    } else {
        if (cmux->type == INDM) {
            PHS_LOGD("[%d] INDM\n", tid);
            return phoneserver_deliver_indicate_default(cmux, rsp, len);
        }
    }

    /* here shall lock*/
    if (cmux->callback) {
        rsp_req.recv_cmux = (cmux_t *) cmux;
        rsp_req.rsp_str = rsp;
        rsp_req.len = len;
        ret = cmux->callback(&rsp_req, cmux->userdata);
    } else
        PHS_LOGE("[%d] No Callback function.\n", tid);

    if (ret == AT_RESULT_OK) {
        return ret;
    }
    PHS_LOGD("[%d] IND\n", tid);
    return phoneserver_deliver_indicate(cmux, rsp, len);
}

#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

/** returns 1 if line starts with prefix, 0 if it does not */
int strStartsWith(const char *line, const char *prefix)
{
    for (; *line != '\0' && *prefix != '\0'; line++, prefix++) {
        if (*line != *prefix) {
            return 0;
        }
    }
    return *prefix == '\0';
}

/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char *s_finalResponsesError[] = { "ERROR", "+CMS ERROR:", "+CME ERROR:",
    //"NO CARRIER",	/* sometimes! */
    "NO ANSWER", "NO DIALTONE",
};

static int isFinalResponseError(const char *line)
{
    size_t i;
    for (i = 0; i < NUM_ELEMS(s_finalResponsesError); i++) {
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
static const char *s_finalResponsesSuccess[] = { "OK"
    //    "CONNECT"       /* some stacks start up data on another channel */
};

static int isFinalResponseSuccess(const char *line)
{
    size_t i;
    for (i = 0; i < NUM_ELEMS(s_finalResponsesSuccess); i++) {
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

int cvt_cops_set_cmd_req0(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cops_set_cmd_req1(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cops_set_cmd_req2(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cops_set_cmd_req4(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_not_support_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }
    adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
    return AT_RESULT_OK;
}

int cvt_generic_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int i;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);

    if (strStartsWith(req->cmd_str, "at+cfun=0")
            || strStartsWith(req->cmd_str, "AT+CFUN=0")
            || strStartsWith(req->cmd_str, "at+sfun=5")
            || strStartsWith(req->cmd_str, "AT+SFUN=5")) {
       int maxPDPNum = getMaxPDPNum();
        for(i = 0; i < maxPDPNum; i++)
            ppp_info[i].state = PPP_STATE_IDLE;
    }

    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgact_query_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int g_esqopt_value = 0;
int cvt_esqopt2_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

#ifdef PDP_DEACT_WHILE_ESQOPT
    g_esqopt_value = 2;
#endif
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_esqopt01_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

#ifdef PDP_DEACT_WHILE_ESQOPT
    g_esqopt_value = 0;
#endif
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_ata_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_ata_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_ata_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    ret =
        phoneserver_deliver_indicate(rsp->recv_cmux, rsp->rsp_str,
                rsp->len);
    if (ret == AT_RESULT_OK) {
        return AT_RESULT_OK;
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {

        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_generic_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    ret =
        phoneserver_deliver_indicate(rsp->recv_cmux, rsp->rsp_str,
                rsp->len);
    if (ret == AT_RESULT_OK) {
        return AT_RESULT_OK;
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    }
    return AT_RESULT_OK;
}

pty_t *adapter_get_ind_pty(mux_type_t type)
{
    pty_t *ind_pty=NULL;
    if (type == INDM_SIM1 || type == ATM1_SIM1 || type == ATM2_SIM1)
        ind_pty = channel_manager_get_sim1_ind_pty();
    else if (type == INDM_SIM2 || type == ATM1_SIM2 || type == ATM2_SIM2)
        ind_pty = channel_manager_get_sim2_ind_pty();
    else if (type == INDM_SIM3 || type == ATM1_SIM3 || type == ATM2_SIM3)
        ind_pty = channel_manager_get_sim3_ind_pty();
    else if (type == INDM_SIM4 || type == ATM1_SIM4 || type == ATM2_SIM4)
        ind_pty = channel_manager_get_sim4_ind_pty();
    else
        PHS_LOGE("get_ind_pty error\n");
    return ind_pty;
}

pty_t *adapter_multi_get_eng_ind_pty(mux_type_t type)
{
    pty_t *ind_pty = NULL;
    if (type == INDM_SIM1 || type == ATM1_SIM1 || type == ATM2_SIM1)
        ind_pty = channel_manager_multi_get_eng_ind_pty();
    return ind_pty;
}

pty_t *adapter_get_default_ind_pty(void)
{
    return channel_manager_get_default_ind_pty();
}

pty_t *adapter_single_get_eng_ind_pty(void)
{
    return channel_manager_single_get_eng_ind_pty();
}

int cvt_generic_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }

    memset(ind_str, 0, MAX_AT_CMD_LEN);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        snprintf(ind_str, sizeof(ind_str), "%s%s%s", "\r\n", ind->ind_str, "\n");
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    }else
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);

    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));

    return AT_RESULT_OK;
}

int cvt_ecind_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    char ind_str[MAX_AT_CMD_LEN];
    char *tmp;
    int err;
    int skip, sim_status;
    pid_t pid1, pid2;
    char prop[PROPERTY_VALUE_MAX];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }

    memset(ind_str, 0, MAX_AT_CMD_LEN);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        snprintf(ind_str, sizeof(ind_str), "%s%s%s", "\r\n", ind->ind_str, "\n");
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
        if(multiSimMode == 1) {
            property_get(RIL_SIM1_ABSENT_PROPERTY, prop, "0");
            if(strcmp(prop, "0") == 0) {
                tmp = ind->ind_str;
                err = at_tok_start(&tmp, ':');
                if (err < 0) {
                    PHS_LOGD("parse cmd error\n");
                    return AT_RESULT_OK;
                }
                /*skip type value */
                err = at_tok_nextint(&tmp, &skip);
                if (err < 0) {
                    return AT_RESULT_OK;
                }
                if (0 == at_tok_nextint(&tmp, &sim_status)) {
                    if(sim_status == 1) {   /* sim is absent */
                        if((mux_type)(ind->recv_cmux->type) == INDM_SIM1) {
                            sim1_absent = 1;
                            if(sim2_exist == 1) {
                                /*restart rild*/
                                property_set(RIL_SIM1_ABSENT_PROPERTY, "1");
                                property_set("ctl.stop", "ril-daemon");
                                property_set("ctl.stop", "ril-daemon1");
                                property_set("ctl.start", "ril-daemon");
                                property_set("ctl.start", "ril-daemon1");
                            }
                        }
                    } else if(sim_status == 0 || sim_status == 3) {  /* sim is exist */
                        if((mux_type)(ind->recv_cmux->type) == INDM_SIM2) {
                            sim2_exist = 1;
                            if(sim1_absent == 1) {
                                /*restart rild*/
                                property_set(RIL_SIM1_ABSENT_PROPERTY, "1");
                                property_set("ctl.stop", "ril-daemon");
                                property_set("ctl.stop", "ril-daemon1");
                                property_set("ctl.start", "ril-daemon");
                                property_set("ctl.start", "ril-daemon1");
                            }
                        }
                    }
                }
            }
    	}
#endif
    } else
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);

    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));

    return AT_RESULT_OK;
}

int cvt_ecind0_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int err;
    int call_id;
    int call_status;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }

    memset(ind_str, 0, MAX_AT_CMD_LEN);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        snprintf(ind_str, sizeof(ind_str), "%s%s%s", "\r\n", ind->ind_str, "\n");
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));

    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_OK;
    }

    /*skip type value */
    err = at_tok_nextint(&at_in_str, &call_status);
    if (err < 0 || call_status !=0) {
        return AT_RESULT_OK;
    }
    /*Call status */
    err = at_tok_nextint(&at_in_str, &call_status);
    if (err < 0) {
        return AT_RESULT_OK;
    }

    err = at_tok_nextint(&at_in_str, &call_id);
    if (err < 0) {
        return AT_RESULT_OK;
    }

    PHS_LOGD("Call status notify:call_id = [%d],call_status=[%d]\n",call_id,call_status);

    if(call_id >=0 && call_id < 8)
    {
        g_call_status_array[call_id] = call_status;
    }
    return AT_RESULT_OK;
}

int cvt_eceer_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int err;
    int call_id;
    int call_status;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }
    memset(ind_str, 0, MAX_AT_CMD_LEN);
    snprintf(ind_str, sizeof(ind_str), "%s%s%s", "\r\n", ind->ind_str, "\n");

    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    /*skip cause value */
    err = at_tok_nextint(&at_in_str, &call_status);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &call_id);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    PHS_LOGD("Call error code notify:call_id = [%d]\n",call_id);

    if(call_id >=0 && call_id < 8)
    {
        if(g_call_status_array[call_id] == 4 || g_call_status_array[call_id] == 5)
        {
            return AT_RESULT_OK;
        }
    }

    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));
    return AT_RESULT_OK;
}

int cvt_csq_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int err;
    int rssi=12;
    int ber=0;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }
    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    /*skip cause value */
    err = at_tok_nextint(&at_in_str, &rssi);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &ber);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if (rssi <= 31) {
        rssi = rssi;
    } else if(rssi >= 100 && rssi < 103) {
        rssi = 0;
    } else if(rssi >= 103 && rssi < 165) {
        rssi = ((rssi - 103) + 1)/ 2;    //add 1 for compensation
    } else if (rssi >= 165 && rssi <= 191) {
        rssi = 31;
    } else {
        rssi = 99;
    }

    if(ber > 99) ber = 99;
    snprintf(ind_str, sizeof(ind_str), "\r\n+CSQ: %d,%d\r\n", rssi,ber);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));
    return AT_RESULT_OK;
}

int cvt_cesq_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int err;
    int rxlev = 0, ber = 0, rscp = 0, ecno = 0, rsrq = 0, rsrp = 0;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];


    PHS_LOGD("cvt_cesq_cmd_ind enter\n");
    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }
    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &rxlev);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if (rxlev <= 61) {
        rxlev = (rxlev+2)/2;
    } else if(rxlev > 61 && rxlev<= 63){
        rxlev = 31;
    } else if(rxlev >= 100 && rxlev < 103){
        rxlev = 0;
    } else if(rxlev >= 103 && rxlev < 165) {
        rxlev = ((rxlev - 103) + 1)/2;    //add 1 for compensation
    } else if (rxlev >= 165 && rxlev <= 191) {
        rxlev = 31;
    }

    err = at_tok_nextint(&at_in_str, &ber);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &rscp);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if (rscp <= 31) {
        rscp = rscp;
    } else if(rscp >= 100 && rscp < 103) {
        rscp = 0;
    } else if(rscp >= 103 && rscp < 165) {
        rscp = ((rscp - 103) + 1)/ 2;    //add 1 for compensation
    } else if (rscp >= 165 && rscp <= 191) {
        rscp = 31;
    }

    err = at_tok_nextint(&at_in_str, &ecno);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &rsrq);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &rsrp);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if (rsrp == 255) {
        rsrp = -255;
    } else {
        rsrp -= 140;
    }

    snprintf(ind_str, sizeof(ind_str), "\r\n+CESQ: %d,%d,%d,%d,%d,%d\r\n",
             rxlev, ber, rscp, ecno, rsrq, rsrp);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));
    return AT_RESULT_OK;
}

int cvt_spscsq_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int err;
    int rssi=12;
    int ber=0, rat;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
        ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
        ind_eng_pty = adapter_single_get_eng_ind_pty();
    }
    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    /*skip cause value */
    err = at_tok_nextint(&at_in_str, &rssi);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &ber);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &rat);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if (rssi <= 31) {
        rssi = rssi;
    } else if(rssi >= 100 && rssi < 103) {
        rssi = 0;
    } else if(rssi >= 103 && rssi < 165) {
        rssi = ((rssi - 103) + 1)/ 2;    //add 1 for compensation
    } else if (rssi >= 165 && rssi <= 191) {
        rssi = 31;
    } else {
        rssi = 99;
    }

    if(ber > 99) ber = 99;
    snprintf(ind_str, sizeof(ind_str), "\r\n+SPSCSQ: %d,%d,%d\r\n", rssi,ber,rat);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    if(ind_eng_pty != NULL)
        if (ind_eng_pty->ops && ind->len < MAX_AT_CMD_LEN)
            ind_eng_pty->ops->pty_write(ind_eng_pty, ind_str, strlen(ind_str));
    return AT_RESULT_OK;
}

int cvt_ecsq_cmd_ind(AT_CMD_IND_T * ind)
{
    pty_t *ind_pty = NULL;
    int err;
    int rssi=12;
    int ber=0;
    char *at_in_str;
    char ind_str[MAX_AT_CMD_LEN];

    if(multiSimMode == 1) {
        ind_pty = adapter_get_ind_pty((mux_type)(ind->recv_cmux->type));
    } else {
        ind_pty = adapter_get_default_ind_pty();
    }
    at_in_str = ind->ind_str;
    err = at_tok_start(&at_in_str, ':');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    /*skip cause value */
    err = at_tok_nextint(&at_in_str, &rssi);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&at_in_str, &ber);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    if(rssi > 100 && rssi <163)
    {
        rssi =  (rssi -100) /2;
    }
    else if( rssi > 162)
    {
        rssi = 31;
    }

    if(ber > 99) ber = 99;
    snprintf(ind_str, sizeof(ind_str), "\r\n+ECSQ: %d,%d\r\n",rssi,ber);
    if (ind_pty && ind_pty->ops && ind->len < MAX_AT_CMD_LEN) {
        ind_pty->ops->pty_write(ind_pty, ind_str, strlen(ind_str));
    } else {
        PHS_LOGE("ind string size > %d\n", MAX_AT_CMD_LEN);
    }
    return AT_RESULT_OK;
}

int cvt_null_cmd_ind(AT_CMD_IND_T __attribute__((unused)) * ind)
{
    return AT_RESULT_OK;
}

int cvt_sind_cmd_ind(AT_CMD_IND_T __attribute__((unused)) * ind)
{
    return AT_RESULT_OK;
}

cmux_t *adapter_get_cmux(int type, int wait)
{

    return channel_manager_get_cmux(type, wait);

}

void adapter_free_cmux(cmux_t * mux)
{
    pid_t tid;
    tid = gettid();
    int seconds = time((time_t *) NULL);

    if(mux == NULL) {
        PHS_LOGD("In adapter_free_cmux mux is NULL\n");
        return;
    }

    mutex_lock(&mux->mutex_timeout);
    channel_manager_free_cmux(mux);
    mux->cp_blked = 0;
    PHS_LOGD("[%d] before signal (%d)\n", tid, seconds);
    thread_cond_signal(&mux->cond_timeout);
    PHS_LOGD("[%d] after signal (%d)\n", tid, seconds);
    mutex_unlock(&mux->mutex_timeout);
}

void adapter_free_cmux_for_ps(cmux_t * mux)
{
    if(mux == NULL) {
        PHS_LOGE("In adapter_free_cmux_for_ps: mux is NULL\n");
        return;
    }
    channel_manager_free_cmux(mux);
}

void adapter_wakeup_cmux(cmux_t * mux)
{
    pid_t tid;
    tid = gettid();
    int seconds = time((time_t *) NULL);

    if(mux == NULL) {
        PHS_LOGE("In adapter_wakeup_cmux mux is NULL\n");
        return;
    }
    mutex_lock(&mux->mutex_timeout);
    mux->cp_blked = 0;
    PHS_LOGD("[%d] before signal (%d)\n", tid, seconds);
    thread_cond_signal(&mux->cond_timeout);
    PHS_LOGD("[%d] after signal (%d)\n", tid, seconds);
    mutex_unlock(&mux->mutex_timeout);
}

int adapter_cmux_register_callback(cmux_t * mux, void *fn,
		unsigned long user_data)
{
    if(mux == NULL || mux->ops == NULL || fn == NULL) {
        PHS_LOGE("In adapter_cmux_register_callback mux is NULL\n");
        return AT_RESULT_NG;
    }
    int ret = mux->ops->cmux_regist_cmd_callback(mux, fn, user_data);
    return ret;
}

int adapter_cmux_write(cmux_t * mux, char *buf, int __attribute__((unused))len,
		int to)
{
    int ret, res;
    pid_t tid;
    tid = gettid();
    int seconds;
    struct timespec timeout;
    char str[MAX_AT_CMD_LEN];
    char block_str[20] = {0};

    memset(str, 0, MAX_AT_CMD_LEN);

    if(mux == NULL || mux->ops == NULL || buf == NULL) {
        PHS_LOGE("In adapter_cmux_write mux is NULL\n");
        return AT_RESULT_NG;
    }
    snprintf(str, sizeof(str),"%s", buf);

    mutex_lock(&mux->mutex_timeout);
    ret = mux->ops->cmux_write(mux, str, strlen(str));
    if(to < 0)
    {
        timeout.tv_sec = time(NULL) - to;
    } else {
        timeout.tv_sec = time(NULL) + to;
    }
    timeout.tv_nsec = 0;
    seconds = time((time_t *) NULL);
    PHS_LOGD("[%d] before timeout (%d)\n", tid, seconds);
    int err = thread_cond_timedwait(&mux->cond_timeout, &mux->mutex_timeout,
            &timeout);
    if (err == ETIMEDOUT) {
        mux->cp_blked += 1;
        PHS_LOGE("mux %s command %s time out",mux->name,str);
        if(mux->cp_blked > BLOCKED_MAX_COUNT) {
            mux->cp_blked = 0;
            if(!strcmp(modem, "t")) {
                ALOGD("TD modem AT no response");
                strcpy(block_str, "TD Modem Blocked");
            } else if(!strcmp(modem, "w")) {
                ALOGD("W modem AT no response");
                strcpy(block_str, "W Modem Blocked");
            } else if(!strcmp(modem, "l")) {
                ALOGD("L modem AT no response");
                strcpy(block_str, "L Modem Blocked");
            } else if(!strcmp(modem, "tl")) {
                ALOGD("TL modem AT no response");
                strcpy(block_str, "TL Modem Blocked");
            } else if(!strcmp(modem, "lf")) {
                ALOGD("LF modem AT no response");
                strcpy(block_str, "LF Modem Blocked");
            } else {
                PHS_LOGE("Wrong modem parameter2");
	        exit(-1);
            }
            if(soc_client > 0) {
                res = write(soc_client, block_str, strlen(block_str)+1);
                ALOGD("write %d bytes to soc_client:%d to info modem is blocked",
                                     res, soc_client);
            }
        }
        seconds = time((time_t *) NULL);
        PHS_LOGD("[%d] timeout (%d)\n", tid, seconds);
        adapter_cmux_deregister_callback(mux);
        if(to >= 0)
        {
            if (mux->pty) {
                adapter_pty_write_error(mux->pty,
                        CME_ERROR_NOT_SUPPORT);
                adapter_pty_end_cmd(mux->pty);
            } else {
                PHS_LOGE("adapter_cmux_write: pty is NULL\n");
            }
        } else {
            ret = AT_RESULT_TIMEOUT;
        }

        channel_manager_free_cmux(mux);
    }
    seconds = time((time_t *) NULL);
    PHS_LOGD("[%d] after timeout (%d)\n", tid, seconds);
    mutex_unlock(&mux->mutex_timeout);

    return ret;
}

int adapter_cmux_write_for_ps(cmux_t * mux, char *buf,
		int __attribute__((unused)) len, int to)
{
    int ret, res;
    pid_t tid;
    tid = gettid();
    int seconds;
    struct timespec timeout;
    char str[MAX_AT_CMD_LEN];
    char block_str[20] = {0};

    memset(str, 0, MAX_AT_CMD_LEN);

    if(mux == NULL || mux->ops == NULL || buf == NULL) {
        PHS_LOGE("In adapter_cmux_write_for_ps mux is NULL\n");
        return AT_RESULT_NG;
    }

    snprintf(str, sizeof(str),"%s", buf);

    mutex_lock(&mux->mutex_timeout);
    ret = mux->ops->cmux_write(mux, str, strlen(str));
    timeout.tv_sec = time(NULL) + to;
    timeout.tv_nsec = 0;
    seconds = time((time_t *) NULL);
    PHS_LOGD("[%d] before timeout (%d)\n", tid, seconds);
    int err = thread_cond_timedwait(&mux->cond_timeout, &mux->mutex_timeout,
            &timeout);
    if (err == ETIMEDOUT) {
        mux->cp_blked += 1;
        PHS_LOGE("mux %s command %s time out",mux->name,str);
        if(mux->cp_blked > BLOCKED_MAX_COUNT) {
            mux->cp_blked = 0;
            if(!strcmp(modem, "t")) {
                ALOGD("TD modem AT no response");
                strcpy(block_str, "TD Modem Blocked");
            } else if(!strcmp(modem, "w")) {
                ALOGD("W modem AT no response");
                strcpy(block_str, "W Modem Blocked");
            } else if(!strcmp(modem, "l")) {
                ALOGD("L modem AT no response");
                strcpy(block_str, "L Modem Blocked");
            } else if(!strcmp(modem, "tl")) {
                ALOGD("TL modem AT no response");
                strcpy(block_str, "TL Modem Blocked");
            } else if(!strcmp(modem, "lf")) {
                ALOGD("LF modem AT no response");
                strcpy(block_str, "LF Modem Blocked");
            } else {
                PHS_LOGE("Wrong modem parameter1");
	        exit(-1);
            }
            if(soc_client > 0) {
                res = write(soc_client, block_str, strlen(block_str)+1);
                ALOGD("write %d bytes to soc_client:%d to info modem is blocked",
                                     res, soc_client);
            }
        }
        seconds = time((time_t *) NULL);
        PHS_LOGD("[%d] timeout (%d)\n", tid, seconds);
        adapter_cmux_deregister_callback(mux);
        ret = AT_RESULT_TIMEOUT;
    }
    seconds = time((time_t *) NULL);
    PHS_LOGD("[%d] after timeout (%d)\n", tid, seconds);
    mutex_unlock(&mux->mutex_timeout);
    return ret;
}

int adapter_pty_write(pty_t * pty, char *buf, int __attribute__((unused)) len)
{
    int ret = 0;
    char str[MAX_AT_CMD_LEN];

    memset(str, 0, MAX_AT_CMD_LEN);
    if(pty == NULL || pty->ops ==NULL || buf == NULL)
        return -1;
    snprintf(str, sizeof(str),"%s%s%s", "\r\n", buf, "\n");
    ret = pty->ops->pty_write(pty, str, strlen(str));
    return ret;
}

int adapter_pty_enter_editmode(pty_t * pty, void *callback,
		unsigned long userdata)
{
    if(pty && pty->ops)
        return pty->ops->pty_enter_edit_mode(pty, callback, userdata);
    else {
        PHS_LOGE("In adapter_pty_enter_editmode pty is NULL\n");
        return -1;
    }
}

int adapter_pty_write_error(pty_t * pty, int __attribute__((unused)) error_code)
{
    char str[20];

    snprintf(str, sizeof(str), "%s","\r\nERROR\r\n");

    if (pty)
        return pty->ops->pty_write(pty, str, strlen(str));
    else {
        PHS_LOGE("In adapter_pty_write_error pty is NULL\n");
        return -1;
    }
}

int adapter_pty_end_cmd(pty_t * pty)
{
    if(pty && pty->ops)
        return pty->ops->pty_clear_wait_resp_flag(pty);
    else {
        PHS_LOGE("In adapter_pty_end_cmd pty is NULL\n");
        return -1;
    }
}

int adapter_cmux_deregister_callback(cmux_t * mux)
{
    if(mux && mux->ops)
        return mux->ops->cmux_deregist_cmd_callback(mux);
    else {
        PHS_LOGE("In adapter_cmux_deregister_callback mux is NULL\n");
        return -1;
    }
}

int adapter_cmd_is_end(char *str, int __attribute__((unused)) len)
{
    return isFinalResponse(str);
}

int adapter_get_rsp_type(char *str, int __attribute__((unused)) len)
{

#if 0
    if (isFinalResponseSuccess(str)) {
        return AT_RSP_TYPE_OK;
    }
    if (isFinalResponseError(str)) {
        return AT_RSP_TYPE_ERROR;
    }
#endif /*  */
    if (strStartsWith(str, "CONNECT"))
        return AT_RSP_TYPE_CONNECT;
    if (isFinalResponseError(str)) {
        return AT_RSP_TYPE_ERROR;
    }
    if (isFinalResponseSuccess(str)) {
        return AT_RSP_TYPE_OK;
    }
    return AT_RSP_TYPE_MID;
}

int cvt_ccwa_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_ccwa_cmd_rsp,
            (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_ccwa_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CCWA")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_clip_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_clip_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_clip_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CLIP")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_colp_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_colp_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_colp_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+COLP")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_creg_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_creg_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_creg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CREG")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cgreg_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cgreg_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgreg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CGREG")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cereg_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cereg_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cereg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CEREG")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cusd_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cusd_cmd_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cusd_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CUSD")) {
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_csq_action_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_csq_action_rsp,
          (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_csq_action_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    char *input;
    int len;
    int rssi_3g = 0, rssi_2g = 0, ber;
    char rsp_str[MAX_AT_CMD_LEN];

    memset(rsp_str, 0, MAX_AT_CMD_LEN);
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    input = rsp->rsp_str;
    len = rsp->len;
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(input, len, "+CSQ")) {

        /*get the 3g rssi value and then convert into 2g rssi */
        if (0 == at_tok_start(&input, ':')) {
            if(0 == at_tok_nextint(&input, &rssi_3g)) {
                if (rssi_3g <= 31) {
                    rssi_2g = rssi_3g;
                } else if (rssi_3g >= 100 && rssi_3g < 103) {
                    rssi_2g = 0;
                } else if(rssi_3g >= 103 && rssi_3g < 165) {
                    rssi_2g = ((rssi_3g - 103) + 1) / 2;
                } else if (rssi_3g >= 165 && rssi_3g <= 191) {
                    rssi_2g = 31;
                } else {
                    rssi_2g = 99;
                }

                if(0 == at_tok_nextint(&input, &ber)) {
                    if(ber > 99) ber = 99;
                }
                snprintf(rsp_str, sizeof(rsp_str), "+CSQ:%d,%d\r", rssi_2g, ber);
                adapter_pty_write((pty_t *) user_data, rsp_str,
                        strlen(rsp_str));
            }
        }
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_spscsq_action_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_spscsq_action_rsp,
          (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_spscsq_action_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    char *input;
    int len;
    int rssi_3g = 0, rssi_2g = 0, ber, rat;
    char rsp_str[MAX_AT_CMD_LEN];

    memset(rsp_str, 0, MAX_AT_CMD_LEN);
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    input = rsp->rsp_str;
    len = rsp->len;
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(input, len, "+SPSCSQ")) {

        /*get the 3g rssi value and then convert into 2g rssi */
        if (0 == at_tok_start(&input, ':')) {
            ret = at_tok_nextint(&input, &rssi_3g);

            if (rssi_3g <= 31) {
                rssi_2g = rssi_3g;
            } else if (rssi_3g >= 100 && rssi_3g < 103) {
                rssi_2g = 0;
            } else if(rssi_3g >= 103 && rssi_3g < 165) {
                rssi_2g = ((rssi_3g - 103) + 1) / 2;
            } else if (rssi_3g >= 165 && rssi_3g <= 191) {
                rssi_2g = 31;
            } else {
                rssi_2g = 99;
            }

            ret = at_tok_nextint(&input, &ber);
            if(ber > 99)
            {
                ber = 99;
            }

            ret = at_tok_nextint(&input, &rat);

            snprintf(rsp_str, sizeof(rsp_str), "+SPSCSQ:%d,%d,%d\r", rssi_2g, ber, rat);
        }
        adapter_pty_write((pty_t *) user_data, rsp_str,
                strlen(rsp_str));
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_csq_test_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_csq_test_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_csq_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    char tmp[200] = { "+CSQ:(0-31,100-191,199),(0-7,99)" };
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CSQ")) {

        //sprintf (tmp, "+EPIN:%d,%d,%d,%d\r\n", rsp->data[0], rsp->data[1], rsp->data[2], rsp->data[3] );
        memset(rsp->rsp_str, 0, rsp->len);
        memcpy(rsp->rsp_str, tmp, strlen(tmp));
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
    }

    else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    }

    else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

typedef struct epin_test_rsp_t {
    pty_t *pty;
    cmux_t *mux;
    int rsp_index;
    int rsp[4];
} EPIN_TEST_RSP_T;
EPIN_TEST_RSP_T epin_rsp;

int cvt_epin_test_req(AT_CMD_REQ_T * req)
{
    char *cmd_str = "AT+XX=0;+XX=2;+XX=1;+XX=3\r";
    cmux_t *mux;
    int i = 0;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    epin_rsp.pty = req->recv_pty;
    epin_rsp.mux = mux;
    epin_rsp.rsp_index = 0;
    adapter_cmux_register_callback(mux, cvt_epin_test_rsp,
		    (unsigned long) &epin_rsp);
    adapter_cmux_write(mux, cmd_str, strlen(cmd_str), req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_epin_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    int epin = 0;
    char *input;
    int len;
    char rsp_str[MAX_AT_CMD_LEN];

    memset(rsp_str, 0, MAX_AT_CMD_LEN);
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    len = rsp->len;
    input = rsp->rsp_str;
    if (user_data != (unsigned long) &epin_rsp) {
        return AT_RESULT_NG;
    }
    if (rsp->recv_cmux != epin_rsp.mux) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(input, len, "+XX")) {
        ret = at_tok_start(&input, ':');
        if(ret < 0)
            return AT_RESULT_NG;
        if (0 == at_tok_nextint(&input, &epin)) {
            if (epin_rsp.rsp_index < 4) {
                epin_rsp.rsp[epin_rsp.rsp_index] = epin;
                epin_rsp.rsp_index++;
            }
        }
        if (epin_rsp.rsp_index == 4) {
            snprintf(rsp_str, sizeof(rsp_str), "+EPIN:%d,%d,%d,%d\r",
                    epin_rsp.rsp[0], epin_rsp.rsp[1],
                    epin_rsp.rsp[2], epin_rsp.rsp[3]);
            adapter_pty_write(epin_rsp.pty, rsp_str,
                    strlen(rsp_str));
        }
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write(epin_rsp.pty, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd(epin_rsp.pty);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_test_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cmgs_cmgw_test_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cmgs_cmgw_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty,  rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CMGS")) {

        ;
    } else if (findInBuf(rsp->rsp_str, rsp->len, "+CMGW")) {
        ;

    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_set_rsp2(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cmgs_cmgw_recovery(cmux_t *mux,pty_t *pty)
{
    char escape=0x1B;
    char *at_str="AT\r";

    adapter_cmux_register_callback(mux,cvt_cmgs_cmgw_set_rsp1, 0);
    adapter_cmux_write_for_ps(mux, &escape, 1, 5);
    adapter_cmux_register_callback(mux,cvt_cmgs_cmgw_set_rsp1, 0);
    adapter_cmux_write_for_ps(mux, at_str, strlen(at_str), 5);
    adapter_pty_write_error(pty, CME_ERROR_NOT_SUPPORT);
    adapter_pty_end_cmd(pty);
    adapter_free_cmux_for_ps(mux);
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_edit_callback(pty_t * pty, char *cmd, int len, unsigned long user_data)
{
    int ret=AT_RESULT_OK;

    cmux_t *mux = (cmux_t *) (user_data);
    PHS_LOGD("\n enter cvt_cmgs_cmgw_edit_callback!\n");
    pty->cmgs_cmgw_set_result = 0;
    adapter_cmux_register_callback(mux, cvt_cmgs_cmgw_set_rsp,
		(unsigned long) pty);
    ret = adapter_cmux_write_for_ps(mux, cmd, len, CMGS_TIMEOUT);
    PHS_LOGD("\n cvt_cmgs_cmgw_edit_callback:ret=%d", ret);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("\n Send SMS error: timeout");
        adapter_pty_write_error(pty, CME_ERROR_NOT_SUPPORT);
        adapter_pty_end_cmd(pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }

    if(pty->cmgs_cmgw_set_result == 4)
    {
        adapter_pty_end_cmd(pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }
    PHS_LOGD("\n Send sms failure\n");
    //	cvt_cmgs_cmgw_recovery(mux,pty);
#if 0
    /* add for SMS not resend */

    cmgs_cmgw_set_result = 0;
    sleep(15);

    PHS_LOGD("\n Send sms failure");

    adapter_cmux_register_callback(mux, cvt_cmgs_cmgw_set_rsp2,
            &cmgs_cmgw_set_result);
    adapter_cmux_write_for_ps(mux, cmgs_cmgw_cmd,strlen(cmgs_cmgw_cmd), 10);

    if(cmgs_cmgw_set_result != 1)
    {

        PHS_LOGD("\n reSend sms failure");
        cvt_cmgs_cmgw_recovery(mux,pty);
        return AT_RESULT_OK;
    }

    adapter_cmux_register_callback(mux, cvt_cmgs_cmgw_set_rsp, pty);
    cmgs_cmgw_set_result = 0;
    ret = adapter_cmux_write_for_ps(mux, cmd, len, CMGS_TIMEOUT);
    PHS_LOGD("\n resend SMS:ret=%d", ret);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("\n reSend SMS error");
        cvt_cmgs_cmgw_recovery(mux,pty);
        return AT_RESULT_OK;
    }

    if(cmgs_cmgw_set_result == 3)
    {
        adapter_pty_write(pty, "+CMS ERROR: 313\r" , strlen( "+CMS ERROR: 313\r"));
    }
#endif
    adapter_pty_end_cmd(pty);
    adapter_free_cmux_for_ps(mux);
    sem_unlock(&sms_lock);
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);

    sem_lock(&sms_lock);
    req->recv_pty->cmgs_cmgw_set_result = 0;
    adapter_cmux_register_callback(mux, cvt_cmgs_cmgw_set_rsp2,
           (unsigned long) req->recv_pty);
    adapter_cmux_write_for_ps(mux, req->cmd_str, req->len, req->timeout);

    if(req->recv_pty->cmgs_cmgw_set_result == 1)
    {
        adapter_pty_enter_editmode(req->recv_pty, (void *)cvt_cmgs_cmgw_edit_callback,
               (unsigned long) mux);
        adapter_pty_write(req->recv_pty, "> ", strlen("> "));
        return AT_RESULT_PROGRESS;
    }else if(req->recv_pty->cmgs_cmgw_set_result == 2){
        adapter_pty_end_cmd(req->recv_pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    } else {
        adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
        adapter_pty_end_cmd(req->recv_pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }
}

int cvt_cmgs_cmgw_set_rsp1(AT_CMD_RSP_T * rsp,
		unsigned long __attribute__((unused)) user_data)
{
    int ret;
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_set_rsp2(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    pty_t *pty = (pty_t *) user_data;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, ">")) {
        //*(int *)user_data = 1;
        pty->cmgs_cmgw_set_result = 1;
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        //*(int *)user_data = 2;
        pty->cmgs_cmgw_set_result = 2;
        adapter_pty_write(pty, rsp->rsp_str , rsp->len);
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cmgs_cmgw_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    pty_t *pty = (pty_t *) user_data;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "+CMGS")) {
        adapter_pty_write(pty, rsp->rsp_str, rsp->len);
    } else if (findInBuf(rsp->rsp_str, rsp->len, "+CMGW")) {
        adapter_pty_write(pty, rsp->rsp_str, rsp->len);
    } else if(strStartsWith(rsp->rsp_str, "+CMS ERROR:" )) {
        pty->cmgs_cmgw_set_result = 3;
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write(pty, rsp->rsp_str, rsp->len); //add for SMS not resend
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        pty->cmgs_cmgw_set_result = 4;
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_esatprofile_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    char *at_in_str;
    char *data;
    char at_cmd_str[MAX_AT_CMD_LEN];
    int err;

    memset(at_cmd_str, 0, MAX_AT_CMD_LEN);
    PHS_LOGD("enter cvt_esatprofile_set_req  \n");
    if (req == NULL) {
        PHS_LOGD("leave cvt_esatprofile_set_req AT_RESULT_NG\n");
        return AT_RESULT_NG;
    }
    at_in_str = req->cmd_str;
    at_in_str[req->len - 1] = 0;

    do {
        err = at_tok_start(&at_in_str, '=');
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }

        /*get L2P */
        err = at_tok_nextstr(&at_in_str, &data);
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }
        snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+ESATPROFILE=\"%s\"\r", data);
        mux = adapter_get_cmux(req->cmd_type, TRUE);
        adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
               (unsigned long) req->recv_pty);
        adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                req->timeout);
        PHS_LOGD("leave cvt_esatprofile_set_req:AT_RESULT_PROGRESS\n");
        return AT_RESULT_PROGRESS;
    } while (0);
    PHS_LOGD("cvt_esatprofile_set_req:parse cmd error \n");
    adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
    adapter_pty_end_cmd(req->recv_pty);
    return AT_RESULT_NG;
}

int cvt_esatenvecmd_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    char *at_in_str;
    char *data;
    char at_cmd_str[MAX_AT_CMD_LEN];
    int err;

    memset(at_cmd_str, 0 ,MAX_AT_CMD_LEN);
    PHS_LOGD("enter cvt_esatenvecmd_set_req  \n");
    if (req == NULL) {
        PHS_LOGD("leave cvt_esatenvecmd_set_req AT_RESULT_NG\n");
        return AT_RESULT_NG;
    }
    at_in_str = req->cmd_str;
    at_in_str[req->len - 1] = 0;

    do {
        err = at_tok_start(&at_in_str, '=');
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }

        /*get L2P */
        err = at_tok_nextstr(&at_in_str, &data);
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }
        snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+ESATENVECMD=\"%s\"\r", data);
        mux = adapter_get_cmux(req->cmd_type, TRUE);
        adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
               (unsigned long) req->recv_pty);
        adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                req->timeout);
        PHS_LOGD("leave cvt_esatenvecmd_set_req:AT_RESULT_PROGRESS\n");
        return AT_RESULT_PROGRESS;
    } while (0);
    PHS_LOGD("cvt_esatenvecmd_set_req:parse cmd error \n");
    adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
    adapter_pty_end_cmd(req->recv_pty);
    return AT_RESULT_NG;
}

int cvt_esatterminal_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    char *at_in_str;
    char *data;
    char at_cmd_str[MAX_AT_CMD_LEN];
    int err;

    memset(at_cmd_str, 0, MAX_AT_CMD_LEN);
    PHS_LOGD("enter cvt_esatterminal_set_req  \n");
    if (req == NULL) {
        PHS_LOGD("leave cvt_esatterminal_set_req AT_RESULT_NG\n");
        return AT_RESULT_NG;
    }
    at_in_str = req->cmd_str;
    at_in_str[req->len - 1] = 0;

    do {
        err = at_tok_start(&at_in_str, '=');
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }

        /*get L2P */
        err = at_tok_nextstr(&at_in_str, &data);
        if (err < 0) {
            PHS_LOGD("parse cmd error\n");
            break;
        }
        snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+ESATTERMINAL=\"%s\"\r", data);
        mux = adapter_get_cmux(req->cmd_type, TRUE);
        adapter_cmux_register_callback(mux, cvt_generic_cmd_rsp,
               (unsigned long) req->recv_pty);
        adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                req->timeout);
        PHS_LOGD("leave cvt_esatterminal_set_req:AT_RESULT_PROGRESS\n");
        return AT_RESULT_PROGRESS;
    } while (0);
    PHS_LOGD("cvt_esatterminal_set_req:parse cmd error \n");
    adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
    adapter_pty_end_cmd(req->recv_pty);
    return AT_RESULT_NG;
}

int cvt_echupvt_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int ret;
    char tmp[200] = { 0 };
    char *str_tmp;
    char *str = 0;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    str_tmp = req->cmd_str;
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    ret = at_tok_start(&str_tmp, '=');
    if(ret < 0)
        return AT_RESULT_NG;
    ret = at_tok_nextstr(&str_tmp, &str);
    if(ret < 0)
        return AT_RESULT_NG;
    snprintf(tmp, sizeof(tmp),"%s%s", "AT+CHUPVT=", str);
    adapter_cmux_register_callback(mux, cvt_echupvt_set_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, tmp, strlen(tmp), req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_echupvt_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    /*just get "ok/... rsp" */
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_atd_active_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL || req->recv_pty->type != STMAT) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_atd_active_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, "ATD*99***04#\r", strlen("ATD*99***04#\r"),
            req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_atd_active_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (strStartsWith(rsp->rsp_str, "CONNECT")) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, "CONNECT\r",
                strlen("CONNECT\r"));
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    }  else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_ath_cmd_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    if (req->recv_pty->type != STMAT) {
        mux = adapter_get_cmux(req->cmd_type, TRUE);
        adapter_cmux_register_callback(mux, cvt_ath_cmd_rsp,
               (unsigned long) req->recv_pty);
        adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    } else {
        mux = adapter_get_cmux(AT_CMD_TYPE_STM, TRUE);
        adapter_cmux_register_callback(mux, cvt_ath_cmd_rsp,
               (unsigned long) req->recv_pty);
        adapter_cmux_write(mux, "AT+CGACT=0,4\r",
                strlen("AT+CGACT=0,4\r"), req->timeout);
    } 
    return AT_RESULT_PROGRESS;
}

int cvt_ath_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_evts_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int ret;
    char tmp[200] = { 0 };
    char *str_tmp;
    int p1 = 0;
    int err;
    char *p2=NULL;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    str_tmp = req->cmd_str;
    err = at_tok_start(&str_tmp, '=');
    if(err < 0)
    {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&str_tmp, &p1);
    if(err < 0)
    {
        return AT_RESULT_NG;
    }

    err = at_tok_nextstr(&str_tmp, &p2);
    if(err < 0)
    {
        return AT_RESULT_NG;
    }
    memset(tmp, 0, 200);
    if(p1 == 0)
    {
        snprintf(tmp, sizeof(tmp), "%s%d\r", "AT+EVTS=", p1);
    }
    else if(p1== 1 && p2 != NULL)
    {
        p2[1] = '\0';
        snprintf(tmp, sizeof(tmp),"%s%d,%s\r", "AT+EVTS=", p1, p2);
    }
    else
    {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_evts_set_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, tmp, strlen(tmp), req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_evts_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    /*just get "ok/... rsp" */
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_eband_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int ret;
    char tmp[30] = { 0 };
    char *str_tmp;
    int p1 = 0;

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    str_tmp = req->cmd_str;
    memset(tmp, 0, 30);
    if (!strncasecmp("AT+EBAND=?", str_tmp, strlen("AT+EBAND=?"))) {

        adapter_pty_write(req->recv_pty, "+EBAND:(0-3)\r",
                strlen("+EBAND:(0-3)\r"));
        adapter_pty_write(req->recv_pty, "OK\r", strlen("OK\r"));
        adapter_pty_end_cmd(req->recv_pty);
        return AT_RESULT_OK;

    }
    ret = at_tok_start(&str_tmp, '=');
    if(ret < 0)
        return AT_RESULT_NG;
    ret = at_tok_nextint(&str_tmp, &p1);
    if(ret < 0)
        return AT_RESULT_NG;

    switch (p1) {
        case 0:
            snprintf(tmp, sizeof(tmp), "%s%s\r", "AT^SYSCONFIG=", "2,2,2,3");
            break;
        case 1:
            snprintf(tmp, sizeof(tmp), "%s%s\r", "AT^SYSCONFIG=", "13,1,2,3");
            break;
        case 2:
            snprintf(tmp, sizeof(tmp), "%s%s\r", "AT^SYSCONFIG=", "15,2,2,3");
            break;
        case 3:
            snprintf(tmp, sizeof(tmp), "%s%s\r", "AT^SYSCONFIG=", "2,1,2,3");
            break;
        default:
            adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
            adapter_pty_end_cmd(req->recv_pty);
            return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_eband_set_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, tmp, strlen(tmp), req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_eband_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_eband_query_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    if (req == NULL) {
        return AT_RESULT_NG;
    }
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_eband_query_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write(mux, "AT^SYSCONFIG?\r", strlen("AT^SYSCONFIG?\r"),
            req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_eband_query_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    char tmp[20];

    memset(tmp, 0, 20);
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (strStartsWith(rsp->rsp_str, "^SYSCONFIG:2,1")) {
        snprintf(tmp, sizeof(tmp), "%s\r", "+EBAND:3");
    } else if (strStartsWith(rsp->rsp_str, "^SYSCONFIG:2")) {
        snprintf(tmp, sizeof(tmp), "%s\r", "+EBAND:0");
    } else if (strStartsWith(rsp->rsp_str, "^SYSCONFIG:15")) {
        snprintf(tmp, sizeof(tmp), "%s\r", "+EBAND:2");
    } else if (strStartsWith(rsp->rsp_str, "^SYSCONFIG:13")) {
        snprintf(tmp, sizeof(tmp), "%s\r", "+EBAND:1");
    } else if (strStartsWith(rsp->rsp_str, "^SYSCONFIG")) {
        snprintf(tmp, sizeof(tmp), "%s\r", "+EBAND:0");
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    } else {
        return AT_RESULT_NG;
    }

    adapter_pty_write((pty_t *) user_data, tmp, strlen(tmp));
    return AT_RESULT_OK;

}

int cvt_snvm_edit_callback(pty_t * pty, char *cmd, int len, unsigned long user_data);

int cvt_snvm_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);

    sem_lock(&sms_lock);
    req->recv_pty->cmgs_cmgw_set_result = 0;
    adapter_cmux_register_callback(mux, cvt_snvm_set_rsp,
           (unsigned long) req->recv_pty);
    adapter_cmux_write_for_ps(mux, req->cmd_str, req->len, req->timeout);

    if(req->recv_pty->cmgs_cmgw_set_result == 1)
    {
        adapter_pty_enter_editmode(req->recv_pty, (void *)cvt_snvm_edit_callback,
               (unsigned long) mux);
        adapter_pty_write(req->recv_pty, "> ", strlen("> "));
        return AT_RESULT_PROGRESS;
    }else if(req->recv_pty->cmgs_cmgw_set_result == 2){
        adapter_pty_end_cmd(req->recv_pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    } else {
        adapter_pty_write_error(req->recv_pty, CME_ERROR_NOT_SUPPORT);
        adapter_pty_end_cmd(req->recv_pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }
}

int cvt_snvm_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    pty_t *ind_pty = NULL;
    pty_t *ind_eng_pty = NULL;
    int ret;
    pty_t *pty = (pty_t *) user_data;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (findInBuf(rsp->rsp_str, rsp->len, "NO CARRIER")) {
        if(multiSimMode == 1) {
            ind_pty = adapter_get_ind_pty((mux_type)(rsp->recv_cmux->type));
            ind_eng_pty = adapter_multi_get_eng_ind_pty((mux_type)(rsp->recv_cmux->type));
        } else {
            ind_pty = adapter_get_default_ind_pty();
            ind_eng_pty = adapter_single_get_eng_ind_pty();
        }
        if(ind_pty && ind_pty->ops)
            ind_pty->ops->pty_write(ind_pty, rsp->rsp_str, rsp->len);
        if(ind_eng_pty && ind_eng_pty->ops)
            ind_eng_pty->ops->pty_write(ind_eng_pty, rsp->rsp_str, rsp->len);
    }
    if (findInBuf(rsp->rsp_str, rsp->len, ">")) {
        //*(int *)user_data = 1;
        pty->cmgs_cmgw_set_result = 1;
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        //*(int *)user_data = 2;
        pty->cmgs_cmgw_set_result = 2;
        adapter_pty_write(pty, rsp->rsp_str , rsp->len);
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    } else {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_snvm_edit_callback(pty_t * pty, char *cmd, int len, unsigned long user_data)
{
    int ret = AT_RESULT_OK;

    cmux_t *mux = (cmux_t *) (user_data);
    PHS_LOGD("\n enter cvt_snvm_edit_callback!\n");
    pty->cmgs_cmgw_set_result = 0;
    adapter_cmux_register_callback(mux, cvt_snvm_set_rsp, (unsigned long)pty);
    ret = adapter_cmux_write_for_ps(mux, cmd, len, CMGS_TIMEOUT);
    PHS_LOGD("\n cvt_snvm_edit_callback:ret=%d", ret);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("\n Send SNVM error: timeout");
        adapter_pty_write_error(pty, CME_ERROR_NOT_SUPPORT);
        adapter_pty_end_cmd(pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }

    if(pty->cmgs_cmgw_set_result == 2)
    {
        adapter_pty_end_cmd(pty);
        adapter_free_cmux_for_ps(mux);
        sem_unlock(&sms_lock);
        return AT_RESULT_OK;
    }
    PHS_LOGD("\n Send snvm failure\n");
    adapter_pty_end_cmd(pty);
    adapter_free_cmux_for_ps(mux);
    sem_unlock(&sms_lock);
    return AT_RESULT_OK;
}

