
/*
 *
 * adapter.h: adapter implementation for the phoneserver
 *
 *
 */

#ifndef adapter_H

#define adapter_H

#define AT_RESULT_OK 0
#define AT_RESULT_NG -1
#define AT_RESULT_TIMEOUT -2
#define AT_RESULT_PROGRESS 1
#define MAX_AT_CMD_LEN 4*1024

#define DEFAULT_TIMEOUT 60
#define CMGS_TIMEOUT   90 // 60
#define CMGW_TIMEOUT   90 //60
#define CGACT_TIMEOUT      40

#define TRUE 1
#define FALSE 0

//#define phoneserver_log PDEBUG
typedef enum at_cmd_enum_t {
	AT_CMD_GENERIC_CMD =  0,
	 AT_CMD_CGDCONT_READ, AT_CMD_CGDCONT_TEST, AT_CMD_CGDCONT_SET,
	AT_CMD_CGDATA_TEST, AT_CMD_CGDATA_SET, AT_CMD_CGACT_READ,
	AT_CMD_CGACT_TEST, AT_CMD_CGACT_SET, AT_CMD_CGACT_SET_0,
	AT_CMD_CGATT,AT_CMD_CGQMIN,AT_CMD_CGQREQ, AT_CMD_CGEQMIN,
	AT_CMD_CGEQREQ,AT_CMD_CGEREP,AT_CMD_CGEQNEG,AT_CMD_CGPADDR,


	AT_CMD_CCWA_READ,AT_CMD_CCWA_TEST, AT_CMD_CCWA_SET,
	AT_CMD_CCWA_IND, AT_CMD_COLP_READ, AT_CMD_COLP_TEST,
	AT_CMD_COLP_SET, AT_CMD_COLP_IND, AT_CMD_ECHUPVT_SET,
	AT_CMD_EVTS_SET, AT_CMD_CRSM,AT_CMD_CHLD, AT_CMD_CMOD,
	AT_CMD_VTS, AT_CMD_VTD, AT_CMD_CLCC,


	AT_CMD_CLIP_READ, AT_CMD_CLIP_TEST, AT_CMD_CLIP_SET,
	AT_CMD_CLIP_IND, AT_CMD_CLIR, AT_CMD_CCFC, AT_CMD_CLCK,
	AT_CMD_CSSN, AT_CMD_CUSD_READ, AT_CMD_CUSD_TEST,
	AT_CMD_CUSD_SET, AT_CMD_CUSD_IND,

	AT_CMD_CREG_READ, AT_CMD_CREG_TEST, AT_CMD_CREG_SET,
	AT_CMD_CREG_IND, AT_CMD_CGREG_READ, AT_CMD_CGREG_TEST,
	AT_CMD_CGREG_SET, AT_CMD_CGREG_IND, AT_CMD_CEREG_READ,
	AT_CMD_CEREG_TEST, AT_CMD_CEREG_SET, AT_CMD_CEREG_IND,
	AT_CMD_CSQ_TEST, AT_CMD_CSQ_ACTION, AT_CMD_CSQ_IND,
	AT_CMD_CPOL, AT_CMD_COPS, AT_CMD_ESQOPT,


	AT_CMD_EPIN_READ,AT_CMD_CPIN_READ, AT_CMD_CPIN_SET,AT_CMD_CPWD,
	AT_CMD_ECPIN2, AT_CMD_EUICC,

	AT_CMD_CMGS_TEST, AT_CMD_CMGS_SET, AT_CMD_CMGW_TEST,
	AT_CMD_CMGW_SET, AT_CMD_CMGD,AT_CMD_CSCA, AT_CMD_CNMA,
	AT_CMD_CNMI, AT_CMD_CMMS,


	AT_CMD_ATD_SET, AT_CMD_ATDT_SET,

	AT_CMD_ESATPROFILE_SET,
	AT_CMD_ESATENVECMD_SET, AT_CMD_ESATTERMINAL_SET, AT_CMD_ESATCAPREQ,


	AT_CMD_ATA_SET,
	AT_CMD_ATH_SET, AT_CMD_EBAND_SET, AT_CMD_EBAND_QUERY,AT_CMD_CEER,
	AT_CMD_CFUN, AT_CMD_CTZU, AT_CMD_CTZR, AT_CMD_CCWE, AT_CMD_CACM,
	AT_CMD_CAMM, AT_CMD_CAOC, AT_CMD_CPUC, AT_CMD_CGSN, AT_CMD_CIMI,
	AT_CMD_CGMR,
	AT_CMD_SNVM_SET,
	AT_CMD_SAUTOATT_SET,
	AT_CMD_UNKNOWN,

	AT_CMD_CCCM_IND, AT_CMD_CRING_IND, AT_CMD_CSSI_IND,
	AT_CMD_CSSU_IND, AT_CMD_CTZV_IND, AT_CMD_BUSY_IND,
	AT_CMD_CONNECT_IND, AT_CMD_NO_CARRIER_IND, AT_CMD_NO_ANSWER_IND,
	AT_CMD_NO_DIALTONE_IND, AT_CMD_RING_IND, AT_CMD_CMTI_IND,
	AT_CMD_CMT_IND, AT_CMD_CDSI_IND, AT_CMD_CDS_IND,
	AT_CMD_SIND_IND, AT_CMD_ECSQ_IND, AT_CMD_ECIND_IND,
	AT_CMD_ECEER_IND, AT_CMD_ESATCAPCNF_IND,
	AT_CMD_ESATPROCMDIND_IND, AT_CMD_ESATENDSESSIONIND_IND,
	AT_CMD_ESATSETUPCALLACKIND_IND, AT_CMD_ESIMATDISPLAY_IND,
	AT_CMD_EEMGINBFTM_IND, AT_CMD_EEMGINFOBASIC_IND,
	AT_CMD_EEMGINFOSVC_IND, AT_CMD_EEMGINFOPS_IND,
	AT_CMD_EEMGINFONC_IND,
	AT_CMD_CESQ_IND,

	AT_CMD_INVALID
} AT_CMD_ID_T;

typedef enum at_cmd_type_t
    { AT_CMD_TYPE_CS, AT_CMD_TYPE_PS, AT_CMD_TYPE_SS, AT_CMD_TYPE_SMS,AT_CMD_TYPE_SMST,
	AT_CMD_TYPE_NW, AT_CMD_TYPE_PBK, AT_CMD_TYPE_SIM, AT_CMD_TYPE_GEN,
	AT_CMD_TYPE_STK,
	AT_CMD_TYPE_STM,
	AT_CMD_TYPE_SLOW, AT_CMD_TYPE_NORMAL,
	AT_CMD_TYPE_SLOW1, AT_CMD_TYPE_NORMAL1,
	AT_CMD_TYPE_SLOW2, AT_CMD_TYPE_NORMAL2,
	AT_CMD_TYPE_SLOW3, AT_CMD_TYPE_NORMAL3,
	AT_CMD_TYPE_SLOW4, AT_CMD_TYPE_NORMAL4,
	AT_CMD_TYPE_INVALID
} AT_CMD_TYPE_T;

typedef struct pty_t pty_t;
typedef struct cmux_t cmux_t;
typedef enum mux_type_t mux_type_t;

typedef struct at_cmd_req_t {
	pty_t *recv_pty;
	char *cmd_str;
	int len;
	AT_CMD_TYPE_T cmd_type;
	AT_CMD_ID_T cmd_id;
	int timeout;
} AT_CMD_REQ_T;
typedef struct at_cmd_rsp_t {
	cmux_t *recv_cmux;
	char *rsp_str;
	int len;
} AT_CMD_RSP_T;
typedef struct at_cmd_ind_t {
	cmux_t *recv_cmux;
	char *ind_str;
	int len;
	AT_CMD_ID_T ind_id;
} AT_CMD_IND_T;

#define CME_ERROR_NOT_SUPPORT 4

#define AT_RSP_TYPE_OK 0
#define AT_RSP_TYPE_MID 1
#define AT_RSP_TYPE_ERROR 2
#define AT_RSP_TYPE_CONNECT 3

#define ETIMEOUT -1234

extern int g_esqopt_value;

extern struct pty_t *channel_manager_get_sim1_ind_pty(void);
extern struct pty_t *channel_manager_get_sim2_ind_pty(void);
extern struct pty_t *channel_manager_get_sim3_ind_pty(void);
extern struct pty_t *channel_manager_get_sim4_ind_pty(void);

int strStartsWith(const char *line, const char *prefix);

/*
API block start
*/
cmux_t *adapter_get_cmux(int type, int wait);
void adapter_free_cmux(cmux_t * mux);
void adapter_free_cmux_for_ps(cmux_t * mux);
void adapter_wakeup_cmux(cmux_t * mux);
int adapter_cmux_register_callback(cmux_t * mux, void *fn, unsigned long user_data);
int adapter_cmux_write_for_ps(cmux_t * mux_t, char *buf, int len, int to);
int adapter_pty_write(pty_t * pty, char *buf, int len);
int adapter_pty_write_error(pty_t * pty, int error_code);
int adapter_pty_enter_editmode(pty_t * pty, void *callback, unsigned long userdata);
int adapter_pty_end_cmd(pty_t * pty);
int adapter_cmux_deregister_callback(cmux_t * mux);
int adapter_cmd_is_end(char *str, int len);
int adapter_get_rsp_type(char *str, int len);
int adapter_cmux_write(cmux_t * mux_t, char *buf, int len, int to);

pty_t *adapter_get_default_ind_pty(void);
pty_t *adapter_single_get_eng_ind_pty(void);

pty_t *adapter_get_ind_pty(mux_type_t type);
pty_t *adapter_multi_get_eng_ind_pty(mux_type_t type);

/*pty call this function to deliver a command */
int phoneserver_deliver_at_cmd(const pty_t * pty, char *cmd, int len);

/*mux call this function to deliver a response or indicator */
int phoneserver_deliver_at_rsp(const cmux_t * cmux, char *rsp, int len);

/*any others function to call this function deliver indicate */
int phoneserver_deliver_indicate(const cmux_t * cmux, char *cmd, int len);
int phoneserver_deliver_indicate_default(const cmux_t * cmux, char *cmd,
					 int len);
int cvt_generic_cmd_req(AT_CMD_REQ_T * req);
int cvt_not_support_cmd_req(AT_CMD_REQ_T * req);
int cvt_ata_cmd_req(AT_CMD_REQ_T * req);
int cvt_ata_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_generic_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_generic_cmd_ind(AT_CMD_IND_T * ind);
int cvt_null_cmd_ind(AT_CMD_IND_T * ind);
int cvt_sind_cmd_ind(AT_CMD_IND_T * ind);
int cvt_ecsq_cmd_ind(AT_CMD_IND_T * ind);
int cvt_csq_cmd_ind(AT_CMD_IND_T * ind);
int cvt_spscsq_cmd_ind(AT_CMD_IND_T * ind);
int cvt_cesq_cmd_ind(AT_CMD_IND_T * ind);

/*add by wz 09 -03 -11 */
int cvt_ccwa_cmd_req(AT_CMD_REQ_T * req);
int cvt_ccwa_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_clip_cmd_req(AT_CMD_REQ_T * req);
int cvt_clip_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_colp_cmd_req(AT_CMD_REQ_T * req);
int cvt_colp_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_creg_cmd_req(AT_CMD_REQ_T * req);
int cvt_creg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cgreg_cmd_req(AT_CMD_REQ_T * req);
int cvt_cgreg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cusd_cmd_req(AT_CMD_REQ_T * req);
int cvt_cusd_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cereg_cmd_req(AT_CMD_REQ_T * req);
int cvt_cereg_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_csq_action_req(AT_CMD_REQ_T * req);
int cvt_csq_action_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_spscsq_action_req(AT_CMD_REQ_T * req);
int cvt_spscsq_action_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_csq_test_req(AT_CMD_REQ_T * req);
int cvt_csq_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_epin_test_req(AT_CMD_REQ_T * req);
int cvt_epin_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cmgs_cmgw_test_req(AT_CMD_REQ_T * req);
int cvt_cmgs_cmgw_test_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cmgs_cmgw_set_req(AT_CMD_REQ_T * req);
int cvt_cmgs_cmgw_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_cmgs_cmgw_set_rsp1(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_echupvt_set_req(AT_CMD_REQ_T * req);
int cvt_echupvt_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_esatprofile_set_req(AT_CMD_REQ_T * req);
int cvt_esatenvecmd_set_req(AT_CMD_REQ_T * req);
int cvt_esatterminal_set_req(AT_CMD_REQ_T * req);
int cvt_atd_active_req(AT_CMD_REQ_T * req);
int cvt_atd_active_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_ath_cmd_req(AT_CMD_REQ_T * req);
int cvt_ath_cmd_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_evts_set_req(AT_CMD_REQ_T * req);
int cvt_evts_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_eband_set_req(AT_CMD_REQ_T * req);
int cvt_eband_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_eband_query_req(AT_CMD_REQ_T * req);
int cvt_eband_query_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);
int cvt_esqopt2_cmd_req(AT_CMD_REQ_T * req);
int cvt_esqopt01_cmd_req(AT_CMD_REQ_T * req);
int cvt_cgact_query_cmd_req(AT_CMD_REQ_T * req);
int cvt_cops_set_cmd_req0(AT_CMD_REQ_T * req);
int cvt_cops_set_cmd_req1(AT_CMD_REQ_T * req);
int cvt_cops_set_cmd_req2(AT_CMD_REQ_T * req);
int cvt_cops_set_cmd_req4(AT_CMD_REQ_T * req);
int cvt_ecind_cmd_ind(AT_CMD_IND_T * ind);
int cvt_ecind0_cmd_ind(AT_CMD_IND_T * ind);
int cvt_eceer_cmd_ind(AT_CMD_IND_T * ind);
int cvt_snvm_set_req(AT_CMD_REQ_T * req);
int cvt_snvm_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data);

#endif /*  */
