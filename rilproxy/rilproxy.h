
#ifndef SPRD_RIL_PROXY_H
#define SPRD_RIL_PROXY_H 

#include <stdlib.h>
#include <stdint.h>

#define RILPROXY_SOCKET_NAME      "srild"
#define LTE_RILD_SOCKET_NAME      "lrild"
#define TDG_RILD_SOCKET_NAME      "trild"

#define SSDA_MODE_PROP            "persist.radio.ssda.mode"
#define SSDA_TESTMODE_PROP        "persist.radio.ssda.testmode"

/* Constants for response types, copy from  libril/ril.cpp */
#define RESPONSE_SOLICITED         0
#define RESPONSE_UNSOLICITED       1

#define PS_TD_ENABLE               0
#define PS_LTE_ENABLE              1

typedef enum {
	ReqToUnKown = -1,
	ReqToAuto   = 0x0,     // data connection AT to target modem (TD or LTE modem)
	ReqToTDG    = 0x1,     // req only send to Td/G modem
	ReqToLTE    = 0x2,     // req only send to LTE modem
	ReqToTDG_LTE = (ReqToTDG | ReqToLTE)
    
} RILP_RequestType;

#define    RSP_FROM_TDG_RILD        ReqToTDG
#define    RSP_FROM_LTE_RILD        ReqToLTE

#define    RSP_UNSENT               0
#define    RSP_SENT_DONE            1

#define    SEND_AT_TO_LTE_LTEBGTIMER           "AT+LTEBGTIMER"
#define    SEND_AT_TO_LTE_LTESETRSRP           "AT+LTESETRSRP"
#define    SEND_AT_TO_LTE_LTENCELLINFO         "AT+LTENCELLINFO"
#define    SEND_AT_TO_LTE_CPOF                 "AT+CPOF"
#define    SEND_AT_TO_AT_RESET                 "AT+RESET"
#define    SEND_AT_TO_AT_SPATASSERT            "AT+SPATASSERT"
#define    SEND_AT_TO_AT_ARMLOG                "AT+ARMLOG"
#define    SEND_AT_TO_AT_SPDSPOP               "AT+SPDSPOP"
#define    SEND_AT_TO_AT_SPCAPLOG              "AT+SPCAPLOG"
#define    SEND_AT_TO_LTE_SBAND0               "AT+SBAND=0,12"
#define    SEND_AT_TO_LTE_SBAND1               "AT+SBAND=1,12"
//#define    SEND_AT_TO_LTE_SGMR_R               "AT+SGMR=1,0,3,3"
//#define    SEND_AT_TO_LTE_SGMR_W               "AT+SGMR=1,1,3,3"
#define    SEND_AT_TO_LTE_LTECFG               "AT+SPLTECFG"

/* RILProxy  */
typedef struct {
    int       reqId;
    int       token;        // request serial number, it is only one
    int       rspType;      // response form which rild, when it equals reqType; it is sent to RILJ.
    int       sentSate;      // For ReqToTDG_LTE, when first response is failure, sent the response at once, its other response ignore.
} RILP_RspTDG_LTE;


 void  rilproxy_init(void);
 void  rilproxy_server(void);
 void *rilproxy_client(void*);
 bool is_svlte(void);
/* Add for dual signal bar */
 void *rilproxy_lte_server();

 void set_lte_radio_on(int radioon);

#endif

