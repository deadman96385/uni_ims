/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28271 $ $Date: 2014-08-18 15:24:33 +0800 (Mon, 18 Aug 2014) $
 */

#ifndef _CSM_SMS_H_
#define _CSM_SMS_H_

#include <osal.h>
#include <ezxml.h>
#include <csm_event.h>
#include "_csm_isi.h"

/* Max one time buffer size when calling ISI_readMessage() */
#define CSM_ISI_MAX_MESSAGE_SZ (1024)

#define CSM_SMS_SMSC_STR_SZ    (128)

#define CSM_SMS_IMDN_REPORT_NONE             (0)
#define CSM_SMS_IMDN_DELIVERY_REPORT_SUCCESS (1)
#define CSM_SMS_IMDN_DELIVERY_REPORT_FAILED  (2)
#define CSM_SMS_IMDN_READ_REPORT_SUCCESS     (4)
#define CSM_SMS_IMDN_READ_REPORT_FAILED      (8)

#define CSM_SMS_DATA_SCHEME_DEFAULT          (0)
#define CSM_SMS_DATA_SCHEME_VOICEMAIL_ON     (1)
#define CSM_SMS_DATA_SCHEME_VOICEMAIL_OFF    (2)

#define CSM_SMS_ERROR_PDU_MODE               (304)
#define CSM_SMS_ERROR_NET_SERVICE            (331)
#define CSM_SMS_ERROR_NETWORK_TIMEOUT        (332)
#define CSM_SMS_ERROR_UNKNOWN                (500)
/* The RT_ACK timer in ms when SMS SUBMIT */
#define CSM_SMS_RT_ACK_TIMER_MS      (35000)

typedef enum {
    CSM_SMS_PDU_MODE_TPDU,
    CSM_SMS_PDU_MODE_RPDU,
    CSM_SMS_PDU_MODE_TEXT, /*
                            * Indicate the message format in CSM_InputSms
                            * and CSM_OutputSms is plain text.
                            */
} CSM_SmsPduMode;

/* 
 * Top level class of the SMS Package 
 */
typedef struct {
    int             numSms;
    OSAL_TmrId      tmrId;
    CSM_IsiMngr    *isiMngr_ptr;
    char            scratch[CSM_ISI_MAX_MESSAGE_SZ + 1];
    char            smsc[CSM_SMS_SMSC_STR_SZ];
    unsigned char   msgRefId;
    vint            msgIdForWaitingAck;
                    /*
                     * Indicate the msg Id of ACK we are waiting for.
                     * -1 for not waiting
                     */
    OSAL_MsgQId     evtQId; // csm private input event queue
    CSM_SmsPduMode  pduMode;
} CSM_SmsMngr;

/* 
 * CSM SMS Manager package public methods 
 */
vint CSM_smsInit(
    CSM_SmsMngr *smsMngr_ptr,
    CSM_IsiMngr *isiMngr_ptr,
    void        *cfg_ptr);

vint CSM_smsProcessEvent(
    CSM_SmsMngr     *smsMngr_ptr,
    CSM_SmsEvt      *smsEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_smsShutdown(
    CSM_SmsMngr *smsMngr_ptr);

void CSM_smsConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputSmsEvt_ptr,
    CSM_SmsEvt         *csmSmsEvt_ptr);
/* 
 * CSM SMS Manager private methods
 */

#endif //_CSM_SMS_H_
