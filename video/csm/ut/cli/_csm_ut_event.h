/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7809 $ $Date: 2008-10-13 16:50:06 -0400 (Mon, 13 Oct 2008) $
 *
 */

#ifndef _CSM_UT_EVENT_H_
#define _CSM_UT_EVENT_H_

/* Define */
#define CSM_UT_AT_COMMAND_SIZE                  (512)

#define CSM_UT_AT_RESPONSE_WAITING              ("+CCWA:")
#define CSM_UT_AT_RESPONSE_SMS_SENT             ("+CMGS:")
#define CSM_UT_AT_RESPONSE_SMS_RECV             ("+CMT:")
#define CSM_UT_AT_RESPONSE_SMS_ERROR            ("+CMS ERROR:")
#define CSM_UT_AT_RESPONSE_USSD_SENT            ("+CUSD:")
#define CSM_UT_AT_RESPONSE_REPORT               ("+CLCC:")
#define CSM_UT_AT_RESPONSE_STR_OK               ("OK")
#define CSM_UT_AT_RESPONSE_OK                   ("0")
#define CSM_UT_AT_RESPONSE_ERROR                ("+CME ERROR:")
#define CSM_UT_AT_RESPONSE_REMOTE_DISCONNECT    ("3")
#define CSM_UT_AT_RESPONSE_CR                   ("\r")
#define CSM_UT_AT_RESPONSE_CR_LN                ("\r\n")
#define CSM_UT_AT_RESPONSE_OIP                  ("+CLIP:")
#define CSM_UT_AT_RESPONSE_OIR                  ("+CLIR:")
#define CSM_UT_AT_RESPONSE_TIP                  ("+COLP:")
#define CSM_UT_AT_RESPONSE_TIR                  ("+COLR:")
#define CSM_UT_AT_RESPONSE_CF                   ("+CCFC:")
#define CSM_UT_AT_RESPONSE_CB                   ("+CLCK:")
#define CSM_UT_AT_RESPONSE_WAITING              ("+CCWA:")
#define CSM_UT_AT_RESPONSE_SUPSRV_MO_CALL       ("+CSSI:")
#define CSM_UT_AT_RESPONSE_SUPSRV_MT_CALL       ("+CSSU:")
#define CSM_UT_AT_RESPONSE_SUPSRV_HISTORY       ("+CSSH:")
#define CSM_UT_AT_RESPONSE_CLASS                ("1")
#define CSM_UT_AT_RESPONSE_CF_TYPE_145          (145)
#define CSM_UT_AT_RESPONSE_CF_TYPE_129          (129)

/* D2 private AT */
#define CSM_UT_AT_RESPONSE_DTMF_DETECT          ("\%DTMFD")

/*
 * event filter function prototypes
 */
typedef OSAL_Boolean (*CSMUT_EventFilter) (
    CSM_OutputEvent  *response_ptr);

OSAL_TaskReturn CSMUT_eventTaskBlock(
    OSAL_TaskArg taskArg);

OSAL_Boolean CSMUT_eventRegisterFilter(
    CSM_EventType       type,
    CSMUT_EventFilter   eventFilter);

OSAL_Boolean CSMUT_eventUnRegisterFilter(
    CSM_EventType   type);

#endif // _CSM_UT_EVENT_H_
