/******************************************************************************
 ** File Name:      D2idh.c                                                 *
 ** Author:                                                         *
 ** DATE:           27/06/2014                                               *
 ** Copyright:      2004 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the D2idh.     *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 27/06/2011                                                                *
 **                                                                           *
 ******************************************************************************/


/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "sci_types.h"
#include <mn_api.h>
#include <settings.h>
#include "nvm_stack_interface.h"
#include <csm_main.h>

#include "d2idh.h"
#include "csm_event.h"
#include "osal_ut.h"
#include "osal_net.h"
#include "osal_string.h"

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/



/**---------------------------------------------------------------------------*
 **                         Extern Refernce                                   *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Macro Define                                      *
 **---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
 **                         Structure Define                                  *
 **---------------------------------------------------------------------------*/
typedef struct IDH_GlobalObj{
    OSAL_MsgQId csmInputEvtQ;
    OSAL_MsgQId csmInputMsgLoopEvtQ;
    int         msgInputLoopFlag;
    OSAL_MsgQId csmOutputEvtQ;
    OSAL_MsgQId csmOutputMsgLoopEvtQ;
    int         msgOutputLoopFlag;
    char        cmdString[128];
} IDH_GlobalObj;

typedef struct IDH_EnumType{
    int index;
    char string[128];
} IDH_EnumType;

/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/
#define _D2IDH_VPMD_PARAM_NUMBER    (6)
#define _D2IDH_ECHO_SERVER_PORT     (5001)
#define _D2IDH_HTTP_PARAM_NUMBER    (7)
#define _D2IDH_HTTP_PARAM_LEN       (64)


/**---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/
int osal_utErrorsFound;
int osal_utInited;
IDH_GlobalObj *global_ptr = NULL;
static char _d2idhTestBuf[] = "This is a data for testing...";


IDH_EnumType _IDH_OutputReasonTable[] = {
    {CSM_OUTPUT_REASON_OK,
        "CSM_OUTPUT_REASON_OK"},
    {CSM_OUTPUT_REASON_ERROR,
        "CSM_OUTPUT_REASON_ERROR"},
    {CSM_OUTPUT_REASON_CALL_LIST,
        "CSM_OUTPUT_REASON_CALL_LIST"},
    {CSM_OUTPUT_REASON_DISCONNECT_EVENT,
        "CSM_OUTPUT_REASON_DISCONNECT_EVENT"},
    {CSM_OUTPUT_REASON_INCOMING_EVENT,
        "CSM_OUTPUT_REASON_INCOMING_EVENT"},
    {CSM_OUTPUT_REASON_WAITING_EVENT,
        "CSM_OUTPUT_REASON_WAITING_EVENT"},
    {CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT,
        "CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT"},
    {CSM_OUTPUT_REASON_INITIALIZING_EVENT,
        "CSM_OUTPUT_REASON_INITIALIZING_EVENT"},
    {CSM_OUTPUT_REASON_CALL_EARLY_MEDIA,
        "CSM_OUTPUT_REASON_CALL_EARLY_MEDIA"},
    {CSM_OUTPUT_REASON_SMS_SENT,
        "CSM_OUTPUT_REASON_SMS_SENT"},
    {CSM_OUTPUT_REASON_SMS_RECEIVED,
        "CSM_OUTPUT_REASON_SMS_RECEIVED"},
    {CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED,
        "CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED"},
    {CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED,
        "CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED"},
    {CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED,
        "CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED"},
    {CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION,
        "CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION"},
    {CSM_OUTPUT_REASON_SERVICE_STATE,
        "CSM_OUTPUT_REASON_SERVICE_STATE"},
    {CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE,
        "CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE"},
    {CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP,
        "CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP"},
    {CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE,
        "CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE"},
    {CSM_OUTPUT_REASON_SERVICE_SHUTDOWN,
        "CSM_OUTPUT_REASON_SERVICE_SHUTDOWN"},
    {CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT,
        "CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT"},
    {CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT,
        "CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT"},
    {CSM_OUTPUT_REASON_USSD_REQUEST_EVENT,
        "CSM_OUTPUT_REASON_USSD_REQUEST_EVENT"},
    {CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT,
        "CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT"},
    {CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION,
        "CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION"},
    {CSM_OUTPUT_REASON_CALL_EXTRA_INFO,
        "CSM_OUTPUT_REASON_CALL_EXTRA_INFO"},
    {CSM_OUTPUT_REASON_CALL_MODIFY_EVENT,
        "CSM_OUTPUT_REASON_CALL_MODIFY_EVENT"},
    {CSM_OUTPUT_REASON_CALL_INDEX,
        "CSM_OUTPUT_REASON_CALL_INDEX"},
    {CSM_OUTPUT_REASON_CALL_DTMF_DETECT,
        "CSM_OUTPUT_REASON_CALL_DTMF_DETECT"},
    {CSM_OUTPUT_REASON_CALL_MONITOR,
        "CSM_OUTPUT_REASON_CALL_MONITOR"},
    {CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY,
        "CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY"},
    {CSM_OUTPUT_REASON_SERVICE_NOTIFY,
        "CSM_OUTPUT_REASON_SERVICE_NOTIFY"},
};
/**---------------------------------------------------------------------------*
 **                         Function Define                                   *
 **---------------------------------------------------------------------------*/
#ifdef VP4G_PLUS_MODEM_TEST
extern int VPMD_testMain(int argc, char *argv_ptr[]);
#endif

#ifdef INCLUDE_SHARED_HTTP_TEST
extern int http_testMain(int argc, char *argv_ptr[]);
#endif
/*
 * ======== d2idh_init() ========
 *
 * d2idh initialization routine.
 *
 * Return:
 *   None.
 */

void d2idh_init(void)
{
    OSAL_logMsg("d2idh_init\n");
    osal_utInited = OSAL_TRUE;
    global_ptr = OSAL_memCalloc(1, sizeof(IDH_GlobalObj), 0);
    OSAL_memSet(global_ptr, 0, sizeof(IDH_GlobalObj));
    /* not sure if it is safe todo the log at system thread init time
     * OSAL_logMsg("d2idh_init.\n");
     */
    return ;
}

extern int vtsp_ut_main(int argc, char *argv_ptr[]);
/*
 * ======== d2idh_vtspUtTask() ========
 * run the vtsp
 *
 * Returns:
 *   None.
 */
static void d2idh_vtspUtTask(
    char *cmdStr)
{
    char *argv_ptr[2];
    
    /* simulate command line arguments */
    argv_ptr[0] = "vtsp_ut_main";
    argv_ptr[1] = cmdStr;
    
    vtsp_ut_main(2, argv_ptr);
}

/*
 * ======== _d2idh_osalUtTest() ========
 * Gen unit test vectors for each OSAL function
 *
 * Returns:
 *   None.
 */
static void _d2idh_osalUtTask(
    uvint mask)
{
    int   passed;
    int   tests;
    int   i;
    char *failLog[128];

    passed = 0;
    tests = 0;
    if (mask & 0x01) {
        if (UT_PASS == do_test_mem()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Memory unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x02) {
        if (UT_PASS == do_test_task()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Task unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x04) {
        if (UT_PASS == do_test_msg()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Message unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x010) {
        if (UT_PASS == do_test_timer()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Timer unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x020) {
        if (UT_PASS == do_test_sem()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Semaphore unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x080) {
        if (UT_PASS == do_test_stresstask()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Task Stress unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x0100) {
        if (UT_PASS == do_test_dns()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "DNS unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x0200) {
        if (UT_PASS == do_test_aton()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "ATON/NTOA unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x0400) {
        if (UT_PASS == do_test_ipsec()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "IPsec unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x0800) {
        if (UT_PASS == do_test_crypto()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Crypto unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x1000) {
        /* XXX File is not support. */
    }
    if (mask & 0x2000) {
        if (UT_PASS == do_test_fifo()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Fifo unit test failed\n";
        }
        tests++;
    }
    if (mask & 0x4000) {
        if (UT_PASS == do_test_net()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Net unit test failed\n";
        }
        tests++;
    }    
    OSAL_logMsg("All OSAL unit tests completed.\n");
    OSAL_logMsg("%d/%d tests passed\n", passed, tests);

    for (i = 0; i < (tests-passed); i++) {
        OSAL_logMsg("%s\n", failLog[i]);
    }

}
void d2idh_displayCsmEvent(
    CSM_OutputEvent event)
{
    switch (event.type) {
        case CSM_EVENT_TYPE_CALL:
            OSAL_logMsg("call output reason: %s\n", 
                _IDH_OutputReasonTable[event.evt.call.reason].string);
            OSAL_logMsg("call reasonDesc: %s\n", event.evt.call.reasonDesc);
            break;
        case CSM_EVENT_TYPE_SERVICE:
            OSAL_logMsg("service output reason: %s\n", 
                _IDH_OutputReasonTable[event.evt.service.reason].string);
            OSAL_logMsg("servcie reasonDesc: %s\n", event.evt.service.reasonDesc);
            OSAL_logMsg("servcie state: %d\n", event.evt.service.state);
            OSAL_logMsg("servcie erroeCode: %d\n", event.evt.service.errorCode);
            OSAL_logMsg("servcie isEmergency: %d\n", event.evt.service.isEmergency);
            break;
        case CSM_EVENT_TYPE_RADIO:
            /* go through */
            break;
        case CSM_EVENT_TYPE_SUPSRV:
            OSAL_logMsg("supsrv output reason: %s\n", 
                _IDH_OutputReasonTable[event.evt.supSrv.reason].string);
            OSAL_logMsg("supsrv reasonDesc: %s\n", event.evt.supSrv.reasonDesc);
            OSAL_logMsg("supsrv cmdType: %d\n", event.evt.supSrv.cmdType);
            OSAL_logMsg("supsrv mode: %d\n", event.evt.supSrv.mode);
            OSAL_logMsg("supsrv queryEnb: %d\n", event.evt.supSrv.queryEnb);
            OSAL_logMsg("supsrv prov: %d\n", event.evt.supSrv.prov);
            OSAL_logMsg("supsrv errorCode: %d\n", event.evt.supSrv.errorCode);
            break;
        case CSM_EVENT_TYPE_USSD:
            OSAL_logMsg("ussd output reason: %s\n", 
                _IDH_OutputReasonTable[event.evt.ussd.reason].string);
            OSAL_logMsg("ussd reasonDesc: %s\n", event.evt.ussd.reasonDesc);
            OSAL_logMsg("ussd encType: %d\n", event.evt.ussd.encType);
            OSAL_logMsg("ussd message: %s\n", event.evt.ussd.message);
            OSAL_logMsg("ussd errorCode: %d\n", event.evt.ussd.errorCode);
            break;
        default :
            OSAL_logMsg("unknown event type\n");
            break;
    }
}
static void _d2idh_msgOutputReceiver(
    char         *cmdAction_ptr)
{
    CSM_OutputEvent  event;
    OSAL_Boolean     timeout;
    if (0 == global_ptr->csmOutputMsgLoopEvtQ) {
        if (0 == (global_ptr->csmOutputMsgLoopEvtQ = OSAL_msgQCreate(
                CSM_OUTPUT_EVENT_QUEUE_NAME,
                OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_OSAL_UT,
                OSAL_DATA_STRUCT_CSM_OutputEvent,
                CSM_OUTPUT_EVENT_MSGQ_LEN,
                (int32)sizeof(CSM_OutputEvent),(int32) 0))) {
            OSAL_logMsg("create csm output queue fail\n");
            return;
        }
        else {
            OSAL_logMsg("create csm output loop queue ok\n");
        }
    }
    else {
        OSAL_logMsg("already create csm output loop queue\n");
          /* queue already create */  
    }
    while (global_ptr->msgOutputLoopFlag) {
        if (OSAL_msgQRecv((OSAL_MsgQId *)global_ptr->csmOutputMsgLoopEvtQ, 
            (char *)&event, sizeof(CSM_OutputEvent),
                OSAL_WAIT_FOREVER, &timeout) > 0) {
            d2idh_displayCsmEvent(event);      
        }
    }
    OSAL_logMsg("output recevier: exit.\n");
    return;

}
static void _d2idh_msgInputReceiver(
    char         *cmdAction_ptr)
{
    CSM_InputEvent  event;
    OSAL_Boolean     timeout;
    if (0 == global_ptr->csmInputMsgLoopEvtQ) {
        if (0 == (global_ptr->csmInputMsgLoopEvtQ = OSAL_msgQCreate(
                CSM_INPUT_EVENT_QUEUE_NAME,
                OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_OSAL_UT,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN,
                (int32)sizeof(CSM_InputEvent),(int32) 0))) {
            OSAL_logMsg("create csm input queue fail\n");
            return;
        }
        else {
            OSAL_logMsg("create csm input loop queue ok\n");
        }
    }
    else {
        OSAL_logMsg("already create csm input loop queue\n");
          /* queue already create */
    }
    while (global_ptr->msgInputLoopFlag) {
        if (OSAL_msgQRecv((OSAL_MsgQId *)global_ptr->csmInputMsgLoopEvtQ, 
            (char *)&event, sizeof(CSM_InputEvent), OSAL_WAIT_FOREVER, &timeout) > 0) {
            OSAL_logMsg("Type: %d\n",event.type);
            OSAL_logMsg("reason: %d\n",event.evt.radio.reason);
            OSAL_logMsg("network: %d\n",event.evt.radio.networkType);
            OSAL_logMsg("address: %s\n",event.evt.radio.address);
            //d2idh_displayCsmEvent(event);      
        }
    }
    OSAL_logMsg("input recevier exit.\n");
    return;

}

/*
 * ======== _d2idh_vpmdUnitTest() ========
 * Gen unit test vectors for VPMD/VPAD function
 *
 * Returns:
 *   None.
 */
static void _d2idh_vpmdUnitTest(
        char *cmd_ptr)
{
    int   argc;
    char *argv_ptr[_D2IDH_VPMD_PARAM_NUMBER];
    char  vpmdParam[_D2IDH_VPMD_PARAM_NUMBER][32];
    char *temp_ptr;
   
    if (NULL == cmd_ptr) {
        return ;
    }
    temp_ptr = NULL;
    temp_ptr = OSAL_strtok(cmd_ptr, ",");
    argc = 1;
    while ((temp_ptr != NULL) && (argc < _D2IDH_VPMD_PARAM_NUMBER)) {
        OSAL_snprintf(vpmdParam[argc], 32, "%s", temp_ptr);
        argv_ptr[argc] = vpmdParam[argc];
        temp_ptr = strtok(NULL, ",");
        argc++;
    }
    OSAL_logMsg("d2idh_at: argv_ptr address=0x%08x\n", (int)argv_ptr); 
    /* start the vpmd test. */
#ifdef VP4G_PLUS_MODEM_TEST
    VPMD_testMain(argc, (int)argv_ptr);
#else
    OSAL_logMsg("Need to enable VP4G_PLUS_MODEM_TEST before the test\n");
#endif
}



/*
 * ======== _d2idh_httpUnitTest() ========
 * Gen unit test vectors for http function
 *
 * Returns:
 *   None.
 */
static void _d2idh_httpUnitTest(
        char *cmd_ptr)
{
    int   argc;
    char *argv_ptr[_D2IDH_HTTP_PARAM_NUMBER];
    char  httpParam[_D2IDH_HTTP_PARAM_NUMBER][_D2IDH_HTTP_PARAM_LEN];
    char *temp_ptr;
   
    if (NULL == cmd_ptr) {
        return ;
    }
    temp_ptr = NULL;
    temp_ptr = OSAL_strtok(cmd_ptr, ",");
    argc = 1;
    while ((temp_ptr != NULL) && (argc < _D2IDH_HTTP_PARAM_NUMBER)) {
        OSAL_snprintf(httpParam[argc], _D2IDH_HTTP_PARAM_LEN, "%s", temp_ptr);
        argv_ptr[argc] = httpParam[argc];
        temp_ptr = strtok(NULL, ",");
        argc++;
    }
    OSAL_logMsg("d2idh_at: argv_ptr address=0x%08x\n", (int)argv_ptr); 
    /* start the vpmd test. */
#ifdef INCLUDE_HTTP
    HTTP_allocate();
    HTTP_start();
    OSAL_taskDelay(3*1000);
#endif
#ifdef INCLUDE_SHARED_HTTP_TEST
    http_testMain(argc, (int)argv_ptr);
#else
    OSAL_logMsg("Need to enable SHARED_HTTP_TEST before the test\n");
#endif
#ifdef INCLUDE_HTTP
    HTTP_destroy();
#endif
}
/*
 * ======== d2idh_bindV4UdpSocket() ========
 *
 * Real network interface bind UDP ipv4 test.
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_bindV4UdpSocket(
    char         *ipAddr_ptr)
{
    OSAL_NetAddress         addr;
    OSAL_NetSockId          socketFd;
    OSAL_NetSockType        type;

    type = OSAL_NET_SOCK_UDP;

    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("Cannot create UDP server socket.\n");
        return (UT_FAIL);
    }      

    if (OSAL_SUCCESS != OSAL_netStringToAddress((int8 *)ipAddr_ptr, &addr)) {
        OSAL_logMsg("%s is not valid ip address string\n", ipAddr_ptr);
        return (-1);
    }

    addr.port = 0;

    OSAL_logMsg("%s %d\n", __FUNCTION__, __LINE__);
    /* Bind Rx socket. */  
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &addr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind UDP server socket.\n");
        return (-1);
    }

    /* Get the port. */
    if (OSAL_SUCCESS != OSAL_netGetSocketAddress(&socketFd, &addr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Failed to get bound address!\n");
        return (-1);
    }

    OSAL_logMsg("Bind success! Socket:%x Port:%d\n",
            socketFd, OSAL_netNtohs(addr.port));
    return (socketFd);
}

/*
 * ======== d2idh_sendV4UdpPacket() ========
 *
 * Real network interface UDP ipv4 send to echo server test.
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_sendV4UdpPacket(
    OSAL_NetSockId  *sockId_ptr,
    OSAL_NetAddress *rmtAddr_ptr)
{
    OSAL_NetAddress rmtAddr;
    char           *buf_ptr;
    vint            size;

    buf_ptr = _d2idhTestBuf;
    size = sizeof(_d2idhTestBuf);

    rmtAddr = *rmtAddr_ptr;
    OSAL_logMsg("sockId:%x, to addr:%x port:%d size:%d\n",
            *sockId_ptr, rmtAddr.ipv4, rmtAddr.port, size);
    /* Send it. */
    if (OSAL_SUCCESS != OSAL_netSocketSendTo(sockId_ptr,
            buf_ptr, &size, &rmtAddr)) {
        OSAL_logMsg("Send failed.\n");
        return (-1);
    }

    return (0);
}


/*
 * ======== d2idh_recvV4UdpPacket() ========
 *
 * Real network interface UDP ipv4 receive from echo server test.
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_recvV4UdpPacket(
    OSAL_NetSockId  *sockId_ptr,
    OSAL_NetAddress *rmtAddr_ptr)
{
    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  timeout;
    OSAL_Boolean        isTimedOut;
    vint                size;
    char                buf[1024];


    timeout.sec = 1;
    timeout.usec = 0;
    /* Select Fd and read file. */
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(sockId_ptr, &fdSet);

    if (OSAL_FAIL ==
            OSAL_select(&fdSet, NULL, &timeout, &isTimedOut)) {
        OSAL_logMsg("Socket select failed.\n");
        return (-1);
    }
    if (isTimedOut == OSAL_TRUE) {
         OSAL_logMsg("Select timeout\n");
        return (-1);
    }

   if (OSAL_SUCCESS != OSAL_netSocketReceiveFrom(sockId_ptr,
            (void *)buf, &size, rmtAddr_ptr)) {
        OSAL_logMsg("Recvfrom failed.\n");
        return (-1);
    }

    /* Compare data. */
    if (NULL != OSAL_memCmp(buf, _d2idhTestBuf, sizeof(_d2idhTestBuf))) {
        OSAL_logMsg("Data content is not equal\n");
        return (-1);
    }
    return (0);
}

/* XXX Temp define sci_getdnsbynetid() here. */
void sci_getdnsbynetid(TCPIP_IPADDR_T* dns1_ptr, TCPIP_IPADDR_T* dns2_ptr,TCPIP_NETID_T netid);

/*
 * ======== d2idh_getDnsServer() ========
 *
 * Get DNS server test 
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_getDnsServer(
    TCPIP_NETID_T netid)
{
    TCPIP_IPADDR_T  dnsAddr1 = 0;
    TCPIP_IPADDR_T  dnsAddr2 = 0;

    /* Get DNS server. XXX need to handle ipv6 case. */
    sci_getdnsbynetid(&dnsAddr1, &dnsAddr2, netid);

    if (0 == dnsAddr1) {
        return (-1);
    }
    OSAL_logMsg("DNS 1:0x%0x, DNS 2:0x%x\n", dnsAddr1, dnsAddr2);

    return (0);
}

/*
 * ======== d2idh_getHostByName() ========
 *
 * DNS query test
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_getHostByName(
    char *hostname_ptr)
{
    struct sci_hostent *host_ptr = NULL;
    unsigned int hostip = 0;
    int i;

    if (NULL == (host_ptr = sci_gethostbyname(hostname_ptr))) {
        OSAL_logMsg("Failed to query ip address of %s\n", hostname_ptr);
        return (-1);
    }

    for (i = 0; i < 5; i++) {
        OSAL_memCpy(&hostip, host_ptr->h_addr_list[i], host_ptr->h_length);
        OSAL_logMsg("Ip address %d: %s\n", i, inet_ntoa(hostip));
    }

    return (0);
}

/*
 * ======== d2idh_getNetId() ========
 *
 * Get netid test.
 *
 * Return:
 *   -1: Fail.
 *   Otherwise: Pass and return the netid.
 */
static int d2idh_getNetId(
    char *ipAddr_ptr)
{
    int32                   ret;
    MN_GPRS_PDP_ADDR_T      ipAddr;
    OSAL_NetAddress         addr;

    OSAL_logMsg("Look up netid for %s\n", ipAddr_ptr);
    if (OSAL_SUCCESS != OSAL_netStringToAddress((int8 *)ipAddr_ptr, &addr)) {
        OSAL_logMsg("%s is not valid ip address string\n", ipAddr_ptr);
        return (-1);
    }

    if (OSAL_netIsAddrLoopback(&addr)) {
        /* Return loopback netid. */
        return (9);
    }

    if (OSAL_netIsAddrIpv6(&addr)) {
        /* ipv6 */
        ipAddr.length = sizeof(addr.ipv6);
        OSAL_memCpy(ipAddr.value_arr, addr.ipv6,
                ipAddr.length);
    }
    else {
        /* ipv4 */
        ipAddr.length = sizeof(addr.ipv4);
        OSAL_memCpy(ipAddr.value_arr, &addr.ipv4,
                ipAddr.length);
    }
    /* Look up netid from ip address. */
    if ((ret = MN_getNetIdByIpAddr(MN_DUAL_SYS_1, ipAddr)) < 0) {
        OSAL_logMsg("Failed to get net id. ret:%d.\n", ret);
        return (-1);
    }
    OSAL_logMsg("Netid:%d\n", ret);
    return (ret);
}

/*
 * ======== d2idh_sslClient() ========
 *
 * Function to create TLS client socket, send data to server.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static int d2idh_sslClient(
    char *rmAddr_ptr,
    char *ipAddr_ptr)
{
    int32              ret = OSAL_FAIL;
    OSAL_NetAddress    addr;
    OSAL_NetAddress    rmAddr;
    OSAL_NetSockId     socketFd;
    OSAL_NetSslId      sslId;
    uint16             serverPort;
    int8    ipBuf[46];

    serverPort = 10000;
    OSAL_logMsg("%s:%d SSL Unit Test Starting...\n", __FILE__, __LINE__);
    /* Create client socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, OSAL_NET_SOCK_TCP)) {
        OSAL_logMsg("Cannot create TCP client socket.\n");
        return (OSAL_FAIL);
    }
    /* Bind to the local end. */
    OSAL_netStringToAddress(ipAddr_ptr, &addr);
    addr.port = OSAL_netHtons(5001);
    addr.type = OSAL_NET_SOCK_TCP;
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &addr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind socket.\n");
        return (OSAL_FAIL);
    }    
    /* Connet to server. */
    OSAL_netStringToAddress(rmAddr_ptr, &rmAddr);
    rmAddr.port = OSAL_netHtons(serverPort);
    rmAddr.type = OSAL_NET_SOCK_TCP;
    if (OSAL_SUCCESS != OSAL_netConnectSocket(&socketFd, &rmAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot connect to TCP server %s.\n", rmAddr_ptr);
        return (OSAL_FAIL);
    }

    /* SSL initialization.*/
    OSAL_logMsg("%s:%d Init SSL.\n", __FILE__, __LINE__);
    if (OSAL_SUCCESS != OSAL_netSslInit()) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to init SSL.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    OSAL_logMsg("%s:%d SSL Create.\n", __FILE__, __LINE__);
    OSAL_memSet(&sslId, 0, sizeof(OSAL_NetSslId));
    sslId.sockId = socketFd;
    sslId.port = serverPort;
    OSAL_strcpy(sslId.ipStr, rmAddr_ptr);
    if (OSAL_SUCCESS !=
        OSAL_netSsl(&sslId, OSAL_NET_SSL_METHOD_CLIENT_SSLV3)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Failed to create SSL.\n");
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS !=
            OSAL_netSslSetSocket(&socketFd, &sslId)) {
        OSAL_netSslClose(&sslId);
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Failed to set SSL.\n");
        return (OSAL_FAIL);
    }
    /* Connect to the remote party. */
    if (OSAL_SUCCESS !=
            OSAL_netSslConnect(&sslId)) {
        OSAL_netSslClose(&sslId);
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Failed to connect SSL server.\n");
        return (OSAL_FAIL);
    }
    OSAL_logMsg("OK to connect SSL server.\n");

    /* Close SSL connection */
    OSAL_netSslClose(&sslId);
    /* Close client socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("Fails to close TCP client socket.\n");
        return (UT_FAIL);
    }
}

/*
 * ======== d2idh_realNetworkTests() ========
 *
 * Real network tests
 *
 * Return:
 *   None.
 */
void d2idh_realNetworkTests(
    char *cmd_ptr)
{
    int             totalTest = 0;
    int             errorCount = 0;
    char           *ipAddr_ptr;
    char           *rmtAddr_ptr;
    char           *pos_ptr;
    OSAL_NetSockId  sockId;
    OSAL_NetAddress rmtAddr;
    OSAL_NetAddress lclAddr;
    TCPIP_NETID_T   netid;
    
    ipAddr_ptr = cmd_ptr;
    /* Search for ',' to see if remote echo server address is given. */
    if (NULL != (pos_ptr = OSAL_strscan(cmd_ptr, ","))) {
        /* We have ',', there must be remote ip address. */
        rmtAddr_ptr = pos_ptr + 1;
        /* NULL terminate first ip address. */
        *pos_ptr = '\0';
        OSAL_logMsg("local ip:%s remote ip:%s\n", ipAddr_ptr, rmtAddr_ptr);
    }
    else {
        rmtAddr_ptr = NULL;
    }
    
    /* Test 1 Get netid /d2idh_bindV4UdpSocket*/
    totalTest++;
    if (0 == (netid = d2idh_getNetId(ipAddr_ptr))) {
        errorCount++;
    }

    /* Test 2.1 Bind V4 UDP socket. */
    totalTest++;
    if (-1 == (sockId = d2idh_bindV4UdpSocket(ipAddr_ptr))) {
        errorCount++;
        goto _D2IDH_REAL_NW_TEST_3;
    }

    /* Test 2.2 Send packet to itself. */
    /* Get the local ip address and port. */
    if (OSAL_SUCCESS != OSAL_netGetSocketAddress(&sockId, &lclAddr)) {
        OSAL_logMsg("Failed to get bound address!\n");
        errorCount++;
        goto _D2IDH_REAL_NW_TEST_3;
    }

    if (-1 == d2idh_sendV4UdpPacket(&sockId, &lclAddr)) {
        OSAL_logMsg("Failed to send packet to itself.\n");
        errorCount++;
        goto _D2IDH_REAL_NW_TEST_3;
    }
    /* Test 2.3 Select and received packet from echo server. */
    if (-1 == d2idh_recvV4UdpPacket(&sockId, &lclAddr)) {
        OSAL_logMsg("Failed to receive packet from itself.\n");
        errorCount++;
        goto _D2IDH_REAL_NW_TEST_3;
    }
    OSAL_logMsg("Send and receive to itself test passed!\n");

    if (NULL != rmtAddr_ptr) {
        /* We have remote ip address for echo test. */
        /* Convert string to OSAL_NetAddress. */    
        if (OSAL_SUCCESS != OSAL_netStringToAddress((int8 *)rmtAddr_ptr,
                &rmtAddr)) {
            OSAL_logMsg("%s is not valid ip address string\n", rmtAddr_ptr);
            errorCount++;
            goto _D2IDH_REAL_NW_TEST_3;
        }
        rmtAddr.port = OSAL_netHtons(_D2IDH_ECHO_SERVER_PORT);
        rmtAddr.type = OSAL_NET_SOCK_UDP;

        /* Test 2.4 Send packet to echo server. */
        if (-1 == d2idh_sendV4UdpPacket(&sockId, &rmtAddr)) {
            OSAL_logMsg("Failed to send packet to echo server.\n");
            errorCount++;
            goto _D2IDH_REAL_NW_TEST_3;
        }
        /* Test 2.5 Select and received packet from echo server. */
        if (-1 == d2idh_recvV4UdpPacket(&sockId, &rmtAddr)) {
            OSAL_logMsg("Failed to receive packet from echo server.\n");
            errorCount++;
            goto _D2IDH_REAL_NW_TEST_3;
        }
        OSAL_logMsg("Send and receive to echo server test passed!\n");
    }

_D2IDH_REAL_NW_TEST_3:
    /* Test 3 Get dns server. */
    totalTest++;
    if (-1 == d2idh_getDnsServer(netid)) {
        errorCount++;
    }

    /* Test 4 DNS query test. */
    totalTest++;
    if (-1 == d2idh_getHostByName("www.google.com")) {
        errorCount++;
    }

    /* Close socket. */
    if (sockId >= 0) {
        OSAL_netCloseSocket(&sockId);
    }

    if (0 == errorCount) {
        OSAL_logMsg("Real network interface tests %d/%d passed!\n",
                totalTest, totalTest);
    }
    else {
        OSAL_logMsg("Real network interface tests failed! "
                "Error count:%d, total Tests:%d\n",
                errorCount, totalTest);
    }
    d2idh_sslClient(rmtAddr_ptr, ipAddr_ptr);
}
/*
 * ======== d2idh_writeCsmEvent() ========
 *
 * write event to csm 
 *
 * Return:
 *   -1: Fail.
 *    0: Pass.
 */
static int d2idh_writeCsmEvent(
    CSM_InputEvent *csmEvt_ptr)
{
    if (0 == global_ptr->csmInputEvtQ) { 
        if (0 == (global_ptr->csmInputEvtQ = OSAL_msgQCreate(
                CSM_INPUT_EVENT_QUEUE_NAME,
                OSAL_MODULE_OSAL_UT, OSAL_MODULE_CSM_PUBLIC,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN,
                sizeof(CSM_InputEvent), 0))) {
            return (-1);
        }
    }
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->csmInputEvtQ,
            (void *)csmEvt_ptr, sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL)) {
        return (-1);
    }
    return (0);
    
}

/*
 * ======== d2idh_registerIMS() ========
 *
 * prepare registration event and send to csm 
 *
 * Return:
 *   None.
 *    
 */
void d2idh_registerIMS(
    char *ipAddr_ptr)
{

    CSM_InputEvent *event = OSAL_memAlloc(sizeof(CSM_InputEvent), 1);
    OSAL_memSet(event, 0, sizeof(CSM_InputEvent));
    /* Send the IP change event to CSM_InputEvent */
    event->type = CSM_EVENT_TYPE_RADIO;
    event->evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE;
    event->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE;
    OSAL_strcpy(event->evt.radio.address, ipAddr_ptr); 
    OSAL_strncpy(event->evt.radio.infcName, "rmnet0", 128); 
    if (-1 == d2idh_writeCsmEvent(event)) {
        OSAL_logMsg("d2idh_writeCsmEvent fail\n");
    }
}

void d2idh_displayUsage(void)
{
    OSAL_logMsg("Usage: \n");
    OSAL_logMsg("%-20s%-40s%s","1.Osal unit test:","at+spd2cmd=1,\"[mask]\"",
        "Refer to OSAL_Reference_Manual.pdf for usage of [mask].\n");
    OSAL_logMsg("%-20s%-40s%s","2.Network test:","at+spd2cmd=2,\"[ip addr]\"",
        "Have a real ip address and perform some tests.\n");
    OSAL_logMsg("%-20s%-40s%s","3.NV ram test: ","at+spd2cmd=3,\"[module]\"",
        "Testing for reading config from NV RAM.(module=CSM|SAPP|MC|ISIM)\n");
    OSAL_logMsg("%-20s%-40s%s","4.VPAMD test: ","at+spd2cmd=4,\"\"",
        "TODO:How to use.\n");
    OSAL_logMsg("%-20s%-40s%s","5.IMS reg test: ","at+spd2cmd=5,\"[ip addr]\"",
        "You have to have real ip address first.\n");  
    OSAL_logMsg("%-20s%-40s%s","6.CSM cmd","at+spd2cmd=6,\"csm output init/shutdown\"",
        "Create a task to recv output event.\n"); 
    OSAL_logMsg("%-20s%-40s%s","9.Dial","at+spd2cmd=9,\"phone number,[session]\"",
        "session=1(video), 0(audio)\n");
    OSAL_logMsg("%-20s%-40s%s","10.Answer","at+spd2cmd=9,\"\"",
        "answer a call\n");
}
/*
=======
 * ======== d2idh_nvRamCsm() ========
 *
 * Reading all CSM cfg from NV RAM and print
 *
 * Return:
 *   None.
 */
void _d2idh_nvRamCsm(
)
{
    char *value_ptr;
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_CSM);

    OSAL_logMsg("NV RAM test for CSM\n");

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
            "", cfg_ptr)) {
        OSAL_logMsg("ERROR reading CSM cfg from NV RAM\n");
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        return;
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_NAT_URL_FMT))) {
        OSAL_logMsg("NatUrlFmt : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_INT_URL_FMR))) {
        OSAL_logMsg("IntUrlFmt : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_SIGNALLING))) {
        OSAL_logMsg("psSignalling : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_SIGNALLING))) {
        OSAL_logMsg("wifiSignalling : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_MEDIA))) {
        OSAL_logMsg("psMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_MEDIA))) {
        OSAL_logMsg("wifiMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_RT_MEDIA))) {
        OSAL_logMsg("psRTMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_RT_MEDIA))) {
        OSAL_logMsg("wifiRTMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_SIP_PORT))) {
        OSAL_logMsg("Sip_Port : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_AUDIO_PORT))) {
        OSAL_logMsg("Audio_Port : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_AUDIO_PORT_POOL_SIZE))) {
        OSAL_logMsg("Audio_Port_Pool_Size : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_VIDEO_PORT))) {
        OSAL_logMsg("Video_Port : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_VIDEO_PORT_POOL_SIZE))) {
        OSAL_logMsg("Video_Port_Pool_Size : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_SIP_PROTECTED_PORT))) {
        OSAL_logMsg("Sip_Protected_Port : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL,
            SETTINGS_PARM_SIP_PROTECTED_PORT_POOL_SIZE))) {
        OSAL_logMsg("Sip_Protected_Port_Pool_Size : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_IPSEC_SPI))) {
        OSAL_logMsg("Ipsec_Spi : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL,
            SETTINGS_PARM_IPSEC_SPI_POOL_SIZE))) {
        OSAL_logMsg("Ipsec_Spi_Pool_Size : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_AUDIO_CONF_SERVER))) {
        OSAL_logMsg("AUDIO_Conf_Server : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_VIDEO_CONF_SERVER))) {
        OSAL_logMsg("VIDEO_Conf_Server : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_RCS_PROVISIONING_ENABLED))) {
        OSAL_logMsg("Rcs_Provisioning_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_SERVER))) {
        OSAL_logMsg("Server : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_USER))) {
        OSAL_logMsg("User : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_PASSWORD))) {
        OSAL_logMsg("Password : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_URI))) {
        OSAL_logMsg("Uri : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_TIMEOUT))) {
        OSAL_logMsg("Timeout : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SMS, NULL, NULL,
            SETTINGS_PARM_SMS_PDU_FMT))) {
        OSAL_logMsg("PduFmt : %s\n", value_ptr);
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
}

/*
 * ======== d2idh_nvRamSapp() ========
 *
 * Reading all SAPP cfg from NV RAM and print
 *
 * Return:
 *   None.
 */
void _d2idh_nvRamSapp(
)
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_SAPP);
    char *value_ptr;

    OSAL_logMsg("NV RAM test for SAPP\n");

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
            "", cfg_ptr)) {
        OSAL_logMsg("ERROR reading SAPP cfg from NV RAM\n");
        SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
        return;
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_IP_VOICE_CALL))) {
        OSAL_logMsg("Reg Ip_Voice_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_IP_VOICE_CALL))) {
        OSAL_logMsg("Exc Ip_Voice_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_IP_VIDEO_CALL))) {
        OSAL_logMsg("Reg Ip_Video_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_IP_VIDEO_CALL))) {
        OSAL_logMsg("Exc Ip_Video_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_SMS_OVER_IP))) {
        OSAL_logMsg("Reg Sms_Over_Ip : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_SMS_OVER_IP))) {
        OSAL_logMsg("Exc Sms_Over_Ip : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_CHAT))) {
        OSAL_logMsg("Reg Chat : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_CHAT))) {
        OSAL_logMsg("Exc Chat : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_DISCOVERY_VIA_PRESENCE))) {
        OSAL_logMsg("Reg Discover_Via_Presence : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_DISCOVERY_VIA_PRESENCE))) {
        OSAL_logMsg("Exc Discover_Via_Presence : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_MESSAGING))) {
        OSAL_logMsg("Reg Messaging : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_MESSAGING))) {
        OSAL_logMsg("Exc Messaging : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER))) {
        OSAL_logMsg("Reg File_Transfer : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER))) {
        OSAL_logMsg("Exc File_Transfer : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_IMAGE_SHARE))) {
        OSAL_logMsg("Reg Image_Share : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_IMAGE_SHARE))) {
        OSAL_logMsg("Exc Image_Share : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_VIDEO_SHARE))) {
        OSAL_logMsg("Reg Video_Share : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_VIDEO_SHARE))) {
        OSAL_logMsg("Exc Video_Share : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL))) {
        OSAL_logMsg("Reg Video_Share_Without_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL))) {
        OSAL_logMsg("Exc Video_Share_Without_Call : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_SOCIAL_PRESENCE))) {
        OSAL_logMsg("Reg Social_Presence : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_SOCIAL_PRESENCE))) {
        OSAL_logMsg("Exc Social_Presence : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_GEOLOCATION_PUSH))) {
        OSAL_logMsg("Reg Geolocation_Push : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_GEOLOCATION_PUSH))) {
        OSAL_logMsg("Exc Geolocation_Push : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_GEOLOCATION_PULL))) {
        OSAL_logMsg("Reg Geolocation_Pull : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_GEOLOCATION_PULL))) {
        OSAL_logMsg("Exc Geolocation_Pull : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_HTTP))) {
        OSAL_logMsg("Reg File_Transfer_Http : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_HTTP))) {
        OSAL_logMsg("Exc File_Transfer_Http : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL))) {
        OSAL_logMsg("Reg File_Transfer_Thumbnail : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL))) {
        OSAL_logMsg("Exc File_Transfer_Thumbnail : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_STORE_FWD))) {
        OSAL_logMsg("Reg File_Transfer_Store_Forward : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_FILE_TRANSFER_STORE_FWD))) {
        OSAL_logMsg("Exc File_Transfer_Store_Forward : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_CS))) {
        OSAL_logMsg("Reg Rcs_Telephony_Cs : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_CS))) {
        OSAL_logMsg("Exc Rcs_Telephony_Cs : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_VOLTE))) {
        OSAL_logMsg("Reg Rcs_Telephony_Volte : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_VOLTE))) {
        OSAL_logMsg("Exc Rcs_Telephony_Volte : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_REG_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_VOHSPA))) {
        OSAL_logMsg("Reg Rcs_Telephony_Vohspa : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_PARM_RCS_TELEPHONY_VOHSPA))) {
        OSAL_logMsg("Exc Rcs_Telephony_Vohspa : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        OSAL_logMsg("this : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        OSAL_logMsg("isi : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL,SETTINGS_PARM_AUDIO))) {
        OSAL_logMsg("audio : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_STREAM))) {
        OSAL_logMsg("stream : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RING_TEMPLATE))) {
        OSAL_logMsg("Ring_Template : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_EXPIRE_SEC))) {
        OSAL_logMsg("Reg_Expire_Sec : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MWI_EXPIRE_SEC))) {
        OSAL_logMsg("Mwi_Expire_Sec : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_NAT_KEEP_ALIVE_SEC))) {
        OSAL_logMsg("Nat_Keep_Alive_Sec : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_KEEP_ALIVE_ENABLED))) {
        OSAL_logMsg("Keep_Alive_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_RETRY_BASE_TIME))) {
        OSAL_logMsg("RegRetryBaseTime : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_RETRY_MAX_TIME))) {
        OSAL_logMsg("RegRetryMaxTime : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MTU))) {
        OSAL_logMsg("Mtu : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_SESSION_TIMER))) {
        OSAL_logMsg("Session_Timer : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_FORCE_MT_SESSION_TIMER))) {
        OSAL_logMsg("Force_MT_Session_Timer : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_PRACK_ENABLED))) {
        OSAL_logMsg("Prack_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_CPIM_ENABLED))) {
        OSAL_logMsg("Cpim_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_IPSEC_ENABLED))) {
        OSAL_logMsg("Ipsec_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_ISIM_ENABLED))) {
        OSAL_logMsg("Isim_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_EVENT_ENABLED))) {
        OSAL_logMsg("Reg_Event_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MWI_EVENT_ENABLED))) {
        OSAL_logMsg("Mwi_Event_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_UA_NAME))) {
        OSAL_logMsg("Ua_Name : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_Q_VALUE))) {
        OSAL_logMsg("Q-Value : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_PRECONDITION_ENABLED))) {
        OSAL_logMsg("Precondition_Enabled : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_PRESENCE_EXPIRE_SEC))) {
        OSAL_logMsg("Presence_Expire_Sec : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_PATH))) {
        OSAL_logMsg("File_Path : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_PREPEND))) {
        OSAL_logMsg("File_Prepend : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_FLOW_CTRL_DEPTH))) {
        OSAL_logMsg("File_Flow_Ctrl_Depth : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_HANDOFF, NULL, SETTINGS_PARM_VDN))) {
        OSAL_logMsg("Vdn : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_BLACK_LIST))) {
        OSAL_logMsg("Black_List : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_WHITE_LIST))) {
        OSAL_logMsg("White_List : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_TIMEOUT))) {
        OSAL_logMsg("Timeout : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            NULL, NULL, SETTINGS_ATTR_ID))) {
        OSAL_logMsg("protocol id : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_ATTR_CAP_DISCOVERY))) {
        OSAL_logMsg("cap-discovery : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_ATTR_COMMON_STACK))) {
        OSAL_logMsg("common-stack : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_MEDIA))) {
        OSAL_logMsg("psMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_MEDIA))) {
        OSAL_logMsg("wifiMedia : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T1))) {
        OSAL_logMsg("Timer_T1 : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T2))) {
        OSAL_logMsg("Timer_T2 : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T4))) {
        OSAL_logMsg("Timer_T4 : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_TCALL))) {
        OSAL_logMsg("Timer_Tcall : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_SRVCC_CAPABILITIES,
            SETTINGS_PARM_SRVCC_ALERTING))) {
        OSAL_logMsg("SRVCC Alerting : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_SRVCC_CAPABILITIES,
            SETTINGS_PARM_SRVCC_MID_CALL))) {
        OSAL_logMsg("SRVCC Mid-Call : %s\n", value_ptr);
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
}

#ifdef INCLUDE_MC
/*
 * ======== d2idh_nvRamMc() ========
 *
 * Reading all MC cfg from NV RAM and print
 *
 * Return:
 *   None.
 */
void _d2idh_nvRamMc(
)
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_MC);
    char *value_ptr;

    OSAL_logMsg("NV RAM test for MC\n");

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_MC,
            "", cfg_ptr)) {
        OSAL_logMsg("ERROR reading MC cfg from NV RAM\n");
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        return;
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        OSAL_logMsg("this : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        OSAL_logMsg("isi : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL,SETTINGS_PARM_AUDIO))) {
        OSAL_logMsg("audio : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_STREAM))) {
        OSAL_logMsg("stream : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RTP_PORT))) {
        OSAL_logMsg("Rtp_Port : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_TONE_AUTO_CALLPROGRESS))) {
        OSAL_logMsg("Tone_Auto_Callprogress : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_TIMER_RTP_INACTIVITY))) {
        OSAL_logMsg("Timer_Rtp_Inactivity : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RTCP_INTERVAL))) {
        OSAL_logMsg("Rtcp_Interval : %s\n", value_ptr);
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
}
#endif

#ifdef INCLUDE_ISIM
/*
 * ======== d2idh_nvRamIsim() ========
 *
 * Reading all ISIM cfg from NV RAM and print
 *
 * Return:
 *   None.
 */
void _d2idh_nvRamIsim(
)
{
    char *value_ptr;
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_ISIM);

    OSAL_logMsg("NV RAM test for ISIM\n");

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_ISIM,
            "", cfg_ptr)) {
        OSAL_logMsg("ERROR reading ISIM cfg from NV RAM\n");
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return;
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_PCSCF))) {
        OSAL_logMsg("Pcscf : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMPU))) {
        OSAL_logMsg("Impu : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMPI))) {
        OSAL_logMsg("Impi : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_DOMAIN))) {
        OSAL_logMsg("Domain : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_PASSWORD))) {
        OSAL_logMsg("Password : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_SMSC))) {
        OSAL_logMsg("Smsc : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMEI_URI))) {
        OSAL_logMsg("Imei_Uri : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_KI))) {
        OSAL_logMsg("Aka_Ki : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_OP))) {
        OSAL_logMsg("Aka_Op : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_OPC))) {
        OSAL_logMsg("Aka_Opc : %s\n", value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_INSTANCE_ID))) 
        OSAL_logMsg("Instance_Id : %s\n", value_ptr);{
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AUDIO_CONF_SERVER)))
        OSAL_logMsg("Audio_Conf_Server : %s\n", value_ptr);{
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_VIDEO_CONF_SERVER)))
        OSAL_logMsg("Video_Conf_Server : %s\n", value_ptr);{
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
}
#endif

/*
 * ======== d2idh_nvRamTester() ========
 *
 * NV RAM reading test
 *
 * Return:
 *   None.
 */
void d2idh_nvRamTester(
    char *name_ptr)
{
    if (0 == strcmp(name_ptr, "CSM")) {
        _d2idh_nvRamCsm();
    }
    else if (0 == strcmp(name_ptr, "SAPP")) {
        _d2idh_nvRamSapp();
    }
#ifdef INCLUDE_MC
    else if (0 == strcmp(name_ptr, "MC")) {
        _d2idh_nvRamMc();
    }
#endif
#ifdef INCLUDE_ISIM
    else if (0 == strcmp(name_ptr, "ISIM")) {
        _d2idh_nvRamIsim();
    }
#endif
    else {
        OSAL_logMsg("%s is not supported\n", name_ptr);
    }

    return;
}

/*
 * ======== d2idh_csmControlCenter() ========
 *
 * csm relative control
 *
 * Return:
 *   None.
 */

void d2idh_csmControlCenter(
    char *control_ptr)
{
    if (0 == OSAL_strcmp((const char *)control_ptr, "csm init")) {
        void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_CSM);
        if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
                "", cfg_ptr)) {
            OSAL_logMsg("ERROR reading CSM cfg from NV RAM\n");
            SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
            return;
        }
        EZXML_init();
        if (-1 == CSM_init(cfg_ptr)) {
            OSAL_logMsg("d2idh_at: csm init fail.\n");
        }
        else {
            OSAL_logMsg("d2idh_at: csm init ok.\n");
        }
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        return;
    }                
    else if (0 == OSAL_strcmp((const char *)control_ptr, "csm shutdown")) {
        OSAL_logMsg("d2idh_at: csm shutdown.\n");
        /* EZXML will be destoryed in CSM_shutdown */
        CSM_shutdown();
        return;
    }
    else if (0 == OSAL_strcmp((const char *)control_ptr, "csm output init")) {
        global_ptr->msgOutputLoopFlag = 1;
        OSAL_taskCreate("d2idhOutputMsgReceiver", OSAL_TASK_PRIO_NRT, 
            (uint32) 8000, (OSAL_TaskPtr)_d2idh_msgOutputReceiver,
                    (OSAL_TaskArg)OSAL_atoi((const char *)control_ptr));
        return;
    }
    else if (0 == OSAL_strcmp((const char *)control_ptr, "csm output shutdown")) {
        global_ptr->msgOutputLoopFlag = 0;
        return;
    }
    else if (0 == OSAL_strcmp((const char *)control_ptr, "csm input init")) {
        global_ptr->msgInputLoopFlag = 1;
        OSAL_taskCreate("d2idhInputMsgReceiver", OSAL_TASK_PRIO_NRT, (uint32) 8000,
                    (OSAL_TaskPtr)_d2idh_msgInputReceiver,
                    (OSAL_TaskArg)OSAL_atoi((const char *)control_ptr));
        return;
    }
    else if (0 == OSAL_strcmp((const char *)control_ptr, "csm input shutdown")) {
        global_ptr->msgInputLoopFlag = 0;
        return;
    }
}


/*
 * ======== _d2idh_callDial() ========
 *
 * csm relative control
 *
 * Return:
 *   None.
 */
void _d2idh_callDial(
    char *cmd_ptr)
{
    char *end_ptr;
    char buf[64] = {0};
    char session[64] = {0};
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_DIAL;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_EVENT_CALL_REASON_AT_CMD_DIAL", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';
    end_ptr = OSAL_strscan(cmd_ptr,",");
    OSAL_memCpy(session, end_ptr + 1, 1);
    OSAL_memCpy(buf,cmd_ptr,end_ptr - cmd_ptr);
    buf[end_ptr - cmd_ptr] = '\0';
    OSAL_logMsg("number = %s, sessionType = %s\n",buf, session);
    if (OSAL_atoi(session) == 1) {
        OSAL_logMsg("Dial video call\n");
        callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_VIDEO | 
            CSM_CALL_SESSION_TYPE_AUDIO;
    }
    else {
        OSAL_logMsg("Dial audio call\n");
        callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_AUDIO;
    }   
    OSAL_snprintf(callEvt_ptr->u.remoteAddress, CSM_EVENT_STRING_SZ, "%s", buf);
    d2idh_writeCsmEvent(&csmEvt_ptr); 
}
void _d2idh_callAnswer(void)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_ANSWER;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_EVENT_CALL_REASON_AT_CMD_ANSWER", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';
    d2idh_writeCsmEvent(&csmEvt_ptr); 
    
}

/*
 * ======== d2idh_at() ========
 *
 * D2 IDH backdoor entry point.
 *
 * Return:
 *   None.
 */
void d2idh_at(uint32 cmd_idx, uint8 *cmd_str_ptr)
{
    OSAL_strcpy(global_ptr->cmdString, cmd_str_ptr);
    switch (cmd_idx) {
        case (1):
            OSAL_logMsg("d2idh_at: OSAL UT. args:%s, init:%d\n", global_ptr->cmdString,
                    osal_utInited);
            /* Spwan osal ut task. */
            OSAL_taskCreate("osalUtmain", OSAL_TASK_PRIO_NIC, (uint32) 8000,
                        (OSAL_TaskPtr)_d2idh_osalUtTask,
                        (OSAL_TaskArg)OSAL_atoi((const char *)global_ptr->cmdString));
            break;
        case (2):
            OSAL_logMsg("d2idh_at: real network test.\n");
            /* Spwan osal ut task. */
            OSAL_taskCreate("d2idhRealTest", OSAL_TASK_PRIO_NIC, (uint32) 8000,
                        (OSAL_TaskPtr)d2idh_realNetworkTests,
                        (OSAL_TaskArg)(const char *)global_ptr->cmdString);
            break;
        case (3):
            OSAL_logMsg("d2idh_at: NV RAM test\n");
            d2idh_nvRamTester((char *)global_ptr->cmdString);
            break;
        case (4):
            OSAL_logMsg("d2idh_at: VPMD UT. args:%s\n", global_ptr->cmdString);
            /* Spwan vpmd ut task. */
            OSAL_taskCreate("vpmdUT", OSAL_TASK_PRIO_NRT, (uint32) 8000,
                        (OSAL_TaskPtr)_d2idh_vpmdUnitTest,
                        (OSAL_TaskArg)global_ptr->cmdString);
            break;
        case (5):
            OSAL_logMsg("d2idh_at: IMS registration.\n");
            d2idh_registerIMS((char *)global_ptr->cmdString);
            break;
        case (6):
            OSAL_logMsg("d2idh_at: create csm relative command.\n");
            d2idh_csmControlCenter((char *)global_ptr->cmdString);
            break; 
        case (7): {
            OSAL_logMsg("d2idh_at: Module Init CSM\n");
            if (0 == OSAL_strcmp((const char *)global_ptr->cmdString, "vport4g init")) {
                OSAL_logMsg("d2idh_at: vport4g init.\n");
                if (-1 == CSM_vport4gInit()) {
                    OSAL_logMsg("d2idh_at: vport4g init fail.\n");
                }
                else {
                    OSAL_logMsg("d2idh_at: vport4g init ok.\n");
                }
            }

            if (0 == OSAL_strcmp((const char *)global_ptr->cmdString, 
                    "vport4g shutdown")) {
                OSAL_logMsg("d2idh_at: vport4g shutdown.\n");
                CSM_vport4gShutdown();
            }
        }
            break;
        case (8):
            OSAL_logMsg("d2idh_at: UT main START! CMD = %s\n", global_ptr->cmdString);
            /* Spwan vtsp ut task. */
            OSAL_taskCreate("vtspUT", OSAL_TASK_PRIO_NRT, (uint32) 8000,
                        (OSAL_TaskPtr)d2idh_vtspUtTask,
                        (OSAL_TaskArg)global_ptr->cmdString);
            break;
        case (9):
            OSAL_logMsg("d2idh_at: dial\n");
            _d2idh_callDial((char *)global_ptr->cmdString);
            break;
        case (10):
            OSAL_logMsg("d2idh_at: answer\n");
            _d2idh_callAnswer();
            break;
        case (11):
            OSAL_logMsg("d2idh_at: HTTP UT. args:%s\n", global_ptr->cmdString);
            /* Spwan vpmd ut task. */
            OSAL_taskCreate("httpUT", OSAL_TASK_PRIO_NRT, (uint32) 8000,
                        (OSAL_TaskPtr)_d2idh_httpUnitTest,
                        (OSAL_TaskArg)global_ptr->cmdString);
            break;
        default:
            OSAL_logMsg("d2idh_at: Unknown cmd:%d\n", cmd_idx);
            d2idh_displayUsage();
            break;
    }
}
