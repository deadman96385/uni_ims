
/*
 *
 * ps_service.h: cmux implementation for the phoneserver
 *
 *
 */

#ifndef PS_SERVICE_H
#define PS_SERVICE_H
#include "adapter.h"
//#include "config.h"

#define IP_ADD_SIZE 16
#define MAX_PPP_NUM 12
#define MAX_CMD 50

typedef enum {
    UNKNOWN = 0,
    IPV4    = 1,
    IPV6    = 2,
    IPV4V6  = 3
}IP_TYPE;

struct ppp_info_struct {
    char dns1addr[IP_ADD_SIZE]; /* IPV4 Primary MS DNS entries */
    char dns2addr[IP_ADD_SIZE]; /* IPV4 secondary MS DNS entries */
    char userdns1addr[IP_ADD_SIZE]; /* IPV4 Primary MS DNS entries */
    char userdns2addr[IP_ADD_SIZE]; /* IPV4 secondary MS DNS entries */
    char ipladdr[IP_ADD_SIZE]; /* IPV4 address local */
    char ipraddr[IP_ADD_SIZE]; /* IPV4 address remote */
    char ipv6dns1addr[IP_ADD_SIZE*4]; /* IPV6 Primary MS DNS entries */
    char ipv6dns2addr[IP_ADD_SIZE*4]; /* IPV6 secondary MS DNS entries */
    char ipv6userdns1addr[IP_ADD_SIZE*4]; /* IPV6 Primary MS DNS entries */
    char ipv6userdns2addr[IP_ADD_SIZE*4]; /* IPV6 secondary MS DNS entries */
    char ipv6laddr[IP_ADD_SIZE*4]; /* IPV6 address local */
    char ipv6raddr[IP_ADD_SIZE*4]; /* IPV6 address remote */
    IP_TYPE ip_state;
    int state;
    cmux_t *cmux;
    pty_t *pty;
    mutex mutex_timeout;
    cond cond_timeout;
    int cid;
    int manual_dns;
    int error_num;
};
#define PPP_STATE_IDLE  1
#define PPP_STATE_ACTING 2
#define PPP_STATE_CONNECT 3
#define PPP_STATE_ESTING 4
#define PPP_STATE_ACTIVE 5
#define PPP_STATE_DESTING 6
#define PPP_STATE_DEACTING 7
#define PPP_STATE_ACT_ERROR 8
#define PPP_STATE_EST_ERROR 9
#define PPP_STATE_EST_UP_ERROR 10


#define PDP_DEACT_TIMEOUT 150
#define PDP_ACT_TIMEOUT 150
#define PDP_QUERY_TIMEOUT 30
#define PPP_UP_TIMEOUT 130
#define PPP_DOWN_TIMEOUT 30

void ps_service_init(void);
int cvt_cgdcont_read_req(AT_CMD_REQ_T * req);
int cvt_cgdcont_set_req(AT_CMD_REQ_T * req);
int cvt_cgdata_set_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgact_deact_req(AT_CMD_REQ_T * req);
int cvt_cgact_act_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgdcont_read_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgact_deact_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgdata_set_req(AT_CMD_REQ_T * req);
int cvt_cgact_act_req(AT_CMD_REQ_T * req);
int cvt_cgact_deact_rsp2(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgact_deact_rsp1(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgdcont_set_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_sipconfig_rsp(AT_CMD_RSP_T * rsp, int user_data);
int cvt_cgcontrdp_rsp(AT_CMD_RSP_T * rsp, int user_data);

 /* SPRD : for svlte & csfb @{ */
int isSvLte(void);
int isLte(void);
int getMaxPDPNum(void);
#endif /*  */
