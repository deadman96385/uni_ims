/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#ifndef __CSM_H_
#define __CSM_H_

#include <osal.h>
#include <csm_event.h>
#include "_csm_calls.h"
#include "_csm_sms.h"
#include "_csm_ussd.h"
#include "_csm_isi.h"
#include "fsm.h"
#include "_csm_service.h"
#include "_csm_radio_policy.h"

/* Return codes. */
#define CSM_OK                     (0)
#define CSM_ERR                    (-1)

/* Task Related settings */
#define CSM_TASK_STACK_BYTES       (16384)
#define CSM_SIP_TASK_NAME          "Csm"

/* Generally used definitions */
#define CSM_STRING_SZ              (128)
#define CSM_SCRATCH_SZ             (512)

/* xml configuration files folder path */
#ifndef CSM_CONFIG_DEFAULT_PATH
#define CSM_CONFIG_DEFAULT_PATH "//system//bin"
#endif

#ifdef VPORT_4G_PLUS_APROC
#define CSM_SAPP_XML_FILE_NAME  CSM_CONFIG_DEFAULT_PATH"/sapp_aproc.xml"
#define CSM_RIR_XML_FILE_NAME   CSM_CONFIG_DEFAULT_PATH"/rir_aproc.xml"
#define CSM_GAPP_XML_FILE_NAME   CSM_CONFIG_DEFAULT_PATH"/gapp_aproc.xml"
#else
#define CSM_SAPP_XML_FILE_NAME  CSM_CONFIG_DEFAULT_PATH"/sapp.xml"
#define CSM_RIR_XML_FILE_NAME  CSM_CONFIG_DEFAULT_PATH"/rir.xml"
#define CSM_GAPP_XML_FILE_NAME   CSM_CONFIG_DEFAULT_PATH"/gapp.xml"
#define CSM_MC_XML_FILE_NAME   CSM_CONFIG_DEFAULT_PATH"/mc.xml"
#endif

#ifndef CSM_DEBUG
#define CSM_dbgPrintf(fmt, args...)
#else
#ifdef VPORT_4G_PLUS_APROC
#define CSM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[AP: %s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define CSM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[MP:%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif
#endif

/*
 * The following defines how SMS message are handled over IP (SIP).
 * SIP_SMS_USE_PDU defines the format of SMS over SIP as PDU.
 */
#define SIP_SMS_USE_PDU (1)

/*
 * This is the CSM task object.
 */
typedef struct {
    OSAL_TaskId  tId;
    uvint        stackSz;
    uvint        pri;
    void        *func_ptr;
    char         name[16];
    void        *arg_ptr;
} CSM_TaskObj;

/*
 * This is the CSM global object.
 * Anything allocated statically must be placed in this object.
 */
typedef struct {
    struct {
        OSAL_MsgQId             inEvtQ; // Input to CSM
        OSAL_MsgQId             privateInEvtQ; // Private input to CSM
        OSAL_MsgQId             outEvtQ;  // Output to AT (CAPP)
        OSAL_MsgQId             isimOutEvtQ;  // Output to ISIM
        OSAL_MsgQId             mgtOutEvtQ;  // Output to MGT
        OSAL_MsgQGrpId          groupInEvt;
        union {
            CSM_InputEvent      publicEvt; // An area to read CMS input event
            CSM_PrivateInputEvt privateEvt; // Receive csm input event from internal(ISI)
        } inputEvt;
        CSM_OutputEvent         csmOutputEvent; // An area to construct outputs to send
    } queue;
    struct {
        CSM_ServiceEvt          service;
        CSM_CallEvt             call;
        CSM_SmsEvt              sms;
        CSM_UssdEvt             ussd;
    } evt;
    CSM_CallManager             callManager;
    CSM_SmsMngr                 smsManager;
    CSM_ServiceMngr             accountManager;
    CSM_IsiMngr                 isiManager;
#ifdef INCLUDE_SUPSRV
    SUPSRV_Mngr                 supSrvManager;
#endif
    CSM_UssdMngr                ussdManager;
    CSM_TaskObj                 task;
} CSM_GlobalObj;

/*
 * Public Methods
 */

CSM_GlobalObj* CSM_getObject(
    void);

/*
 * Private methods
 */
void _CSM_task(
    OSAL_TaskArg arg_ptr);

#endif //__CSM_H_
