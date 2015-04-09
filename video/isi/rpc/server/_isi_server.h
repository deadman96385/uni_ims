/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#ifndef __ISI_SERVER_H_
#define __ISI_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define different task name and queue for isi server to running in
 * application processor.
 */
#ifdef VPORT_4G_PLUS_APROC
#define _ISI_SERVER_TASK_NAME          "aproc.isi-server"
#define _ISI_SERVER_EVT_TASK_NAME      "aproc.isi-server"
#define _ISI_CLIENT_EVT_Q_NAME         "aproc.isi-server-evt-q"
#define _ISI_SERVER_CSM_EVT_Q_NAME     "aproc.isi-server-csm-evt-q"
#else
#define _ISI_SERVER_TASK_NAME          "isi-server"
#define _ISI_SERVER_EVT_TASK_NAME      "isi-server"
#define _ISI_CLIENT_EVT_Q_NAME         "isi-server-evt-q"
#define _ISI_SERVER_CSM_EVT_Q_NAME     "isi-server-csm-evt-q"
#endif 

#define _ISI_SERVER_TASK_STACK_BYTES   (4096)
#define _ISI_CLIENT_EVT_Q_NUM          (8)
#define _ISI_SERVER_CSM_EVT_Q_NUM      (4)

#define _ISI_SERVER_DB_MAX_ENTRIES     (8)
#define _ISI_SERVER_MAX_SERVICES       (10)

#define _ISI_SERVER_VERIFY_INIT          \
        if (NULL == _ISI_ServerObj_ptr) {     \
            return;                       \
        }

typedef struct {
    ISI_IdType          type;
    ISI_Id              id;
} ISI_ServerDbEntry;

/* ISI_ServerObj */
typedef struct {
    OSAL_MsgQId         clientEvtQ; /* Q for ISI event sending to ISI client */
    OSAL_MsgQId         csmEvtQ; /* Q for ISI event sending to CSM */
    OSAL_TaskId         taskId;
    OSAL_TaskId         evtTaskId;
    ISI_Xdr             xdr;
    ISI_ServerDbEntry   db[_ISI_SERVER_DB_MAX_ENTRIES];
    ISI_FeatureType     features; /*
                                   * For storing features which will routes to
                                   * ISI client.
                                   */
    ISI_NetworksMode    networkMode; /* network mode, lte or wifi */
    /* cach what session type of LTE */
    ISI_SessionType     psMediaSessionType;
    /* cach what session type of wifi */
    ISI_SessionType     wifiMediaSessionType;
    ISI_RTMedia         psRTMediaSessionType;
    ISI_RTMedia         wifiRTMediaSessionType;
    ISI_Id              confCallId;
    /*
     * struct to store the service data that configigured by ISI Client
     * application(i.e. mCUE).
     */
    struct {
        char            provisioningData[ISI_PROVISIONING_DATA_STRING_SZ];
        char            szRealm[ISI_ADDRESS_STRING_SZ + 1];
        char            szDomain[ISI_ADDRESS_STRING_SZ + 1];
        char            szUsername[ISI_ADDRESS_STRING_SZ + 1];
        char            szPassword[ISI_ADDRESS_STRING_SZ + 1];
        char            szUri[ISI_ADDRESS_STRING_SZ + 1];
        char            szProxy[ISI_ADDRESS_STRING_SZ + 1];
        char            szRegistrar[ISI_ADDRESS_STRING_SZ + 1];
        char            szOutboundProxy[ISI_LONG_ADDRESS_STRING_SZ + 1];
        char            imConfUri[ISI_ADDRESS_STRING_SZ + 1];
    } service;
    /* Msg Q communicate with ISI client. */
    OSAL_MsgQId isiRpcWriteQ;
    OSAL_MsgQId isiRpcReadQ;
    OSAL_MsgQId isiEvtRpcWriteQ;
    OSAL_MsgQId isiEvtRpcReadQ;
} ISI_ServerObj;

/*
 * ISI event struct
 * It is used to pass from ISI_serverGetEvent() to
 * _ISI_clientGetEvent().
 */
typedef struct {
    ISI_Id       serviceId;
    ISI_Id       id;
    ISI_IdType   type;
    ISI_Event    evt;
    char         eventDesc[ISI_EVENT_DESC_STRING_SZ];
} ISI_ServerEvent;

OSAL_Boolean _ISI_serverIsEventForIsiClient(
    ISI_Id      *serviceId_ptr,
    ISI_Id       id,
    ISI_IdType   type,
    ISI_Event    evt,
    char        *eventDesc_ptr);

OSAL_Boolean _ISI_serverIsEventForBoth(
    ISI_IdType   type,
    ISI_Event    evt);

int32 _ISI_serverDaemon(
    void *arg_ptr);

int32 _ISI_serverGetEvtTask(
    void *arg_ptr);

OSAL_Status _ISI_startServerTask(
    void);

OSAL_Status _ISI_startEventTask(
    void);

void _ISI_serverProcessEvt(
    ISI_ApiName isiApi,
    ISI_Xdr *xdr_ptr);

#ifdef __cplusplus
}
#endif


#endif /* __ISI_SERVER_H_ */
