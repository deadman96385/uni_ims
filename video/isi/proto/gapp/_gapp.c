/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include "isi.h"
#include "isip.h"
#include "_gapp.h"
#include "_gapp_call.h"

#ifndef GAPP_DISABLE_GSM
#include "gsm.h"
#else
typedef uint32 GSM_Id;
#endif

/*
 * This table is network registration mix table. 
 *     CSM:     INACTIVE  ACTIVE  IN_PROGRESS  FAILED  UNKNOWN  ROAMING
 * GSM:
 * ----------------------------------------------------------------------
 * INACTIVE        0        1         2          3       4        5
 * ACTIVE          1        1         1          1       1        1
 * IN_PROGRESS     2        1         2          2       2        2
 * FAILED          3        1         3          3       3        3
 * UNKNOWN         4        1         4          4       4        4
 * ROAMING         5        1         5          5       5        5
 */
static const int _GAPP_regStatusTable[GAPP_REG_TABLE_SIZE]
        [GAPP_REG_TABLE_SIZE] = {
    {0, 1, 2, 3, 4, 5},
    {1, 1, 1, 1, 1, 1},
    {2, 1, 2, 2, 2, 2},
    {3, 1, 3, 3, 3, 3},
    {4, 1, 4, 4, 4, 4},
    {5, 1, 5, 5, 5, 5}};

/* 
 * ======== GAPP_getUniqueIsiId() ========
 * This function is used to get a unique ISI ID
 *
 * Return Values:
 * Nothing.
 */  
ISI_Id GAPP_getUniqueIsiId(
    ISI_Id serviceId)
{
    static uint32 uniqueId  = 0;
    uint32 id;
    uniqueId++;
    if (uniqueId > GAPP_MAX_ID) {
        uniqueId = 1;
    }
    id = (uint32)serviceId;
    id = (id << 24);
    return ((ISI_Id)(uniqueId | id));
}

#ifndef GAPP_DISABLE_GSM
/*
 * ======== _GAPP_copyToken() ========
 * This function is used to copy a token value to a target buffer and then
 * NULL terminate the string.
 *
 * Return Values:
 * Nothing
 */
static void _GAPP_copyToken(
   char *target_ptr,
   vint  maxTargetSize,
   GAPP_Token  *token_ptr)
{
    maxTargetSize--;
    maxTargetSize = ((vint)token_ptr->length > maxTargetSize) ?
            maxTargetSize : (vint)token_ptr->length;
    OSAL_memCpy(target_ptr, token_ptr->start_ptr, maxTargetSize);
    target_ptr[maxTargetSize] = 0;
    return;
}

/* 
 * ======== _GAPP_srvcIsiEvt() ========
 * This function is used by various other functions to populate a ISI event
 * for ISI Service events. These events will be passed from GAPP 
 * to the ISI module. 
 *
 * Return Values:
 * Nothing.
 */   
static void _GAPP_srvcIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_ServiceReason  reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = serviceId;
    isi_ptr->protocol = protocolId;
    isi_ptr->code = ISIP_CODE_SERVICE;
    isi_ptr->msg.service.reason = reason;
    isi_ptr->msg.service.status = status;
    return;
}

/* 
 * ======== _GAPP_textIsiEvt() ========
 * This function is used by various other functions to populate a ISI event
 * for "sms" (short message service) related events. These events will be 
 * passed from GAPP to the ISI module. 
 *
 * Return Values:
 * Nothing.
 */  
static void _GAPP_textIsiEvt(
    ISI_Id              textId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_TextReason     reason,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = textId;
    isi_ptr->code = ISIP_CODE_MESSAGE;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.message.chatId = 0;
    isi_ptr->msg.message.reason = reason;
    isi_ptr->msg.message.serviceId = serviceId;
    /* Zero the 'subject' and the 'to'. For GSM calls this is irrelevant. */
    isi_ptr->msg.message.subject[0] = 0;
    isi_ptr->msg.message.to[0] = 0;
    return;
}

/* 
 * ======== _GSM_getSmsMessageStatus() ========
 * This function is used to process SMS message status
 *
 * Return Values:
 * GAPP_OK: The result code
 * GAPP_ERR: Not processed
 */
static vint _GSM_getSmsMessageStatus(
    char      *buffer_ptr,
    vint       bufferLen,
    ISIP_Text *isi_ptr)
{
    char *pos_ptr;
    vint len;
    vint status;
    /*
     * Let's get the messageReference and the status of the
     * delivery report.
     */
    if (NULL == (pos_ptr = OSAL_strnscan(buffer_ptr, bufferLen, " "))) {
        /* Nothing there..bail */
        return (GAPP_ERR);
    }
    /* Then we have the the messageReference */
    len = pos_ptr - buffer_ptr;
    if (len >= ISI_ID_STRING_SZ) {
        /* Not enough room, bail */
        return (GAPP_ERR);
    }
    bufferLen -= len;
    /* Step off the ' ' */
    pos_ptr++;
    /* Copy the message reference */
    OSAL_memCpy(isi_ptr->messageId, buffer_ptr, len);
    isi_ptr->messageId[len] = 0;
    /* Let's get the status */
    status = OSAL_atoi(pos_ptr);
    if (0 == status) { /* not zero means fail */
        isi_ptr->report = ISI_MSG_RPT_DELIVERY_SUCCESS;
        return (GAPP_OK);
    }
    /*
     * Otherwise the report status simply does not know.  In other
     * words the status is not 'OK' which means that the carrier
     * actually has no idea whether the message coul dbe delivered or
     * not.
     */
    return (GAPP_ERR);
}

/* 
 * ======== _GAPP_smsEvent() ========
 * This function is used to process +CMT: and +CMGR results codes.
 * It will parse the result and get the originating address (the sender)
 * information and the SMS message itself.
 *
 * Return Values:
 * GAPP_OK: The result code was a +CMT: or +CMGR: was it was processed.
 * GAPP_ERR: The result code was not a +CMT: or +CMGR and should be continued
 *           to be processed.
 */
static vint _GAPP_smsEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    ISIP_Message    *isi_ptr)
{
    vint         len;
    vint         ret;
    vint         state = 0; /* 0 == init, 1 == read sms, 2 = mwi */

    /*
     * Is it an 'unsolicited' SMS result code? See if it's a new 
     * message or MWI. If so send to ISI.
     */
    ret = GAPP_ERR;
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        switch (state) {
            case 0:
                /* If we are here then we have a valid token */
                if (0 == OSAL_strncmp(GAPP_GSM_SMS_RECVD, 
                        result_ptr->token.start_ptr,
                        sizeof(GAPP_GSM_SMS_RECVD) - 1)) {
                    /* 
                     * Then it's a new SMS message 
                     */
                    /* 
                     * Start setting up the ISIP message to send for a
                     * new incoming SMS the next line will contain the SMS PDU.
                     */
                   /* 
                    * Note that the 'from field is not really needed 
                    * but it can't be NULL to satisfy ISI. 
                    */
                   OSAL_snprintf(isi_ptr->msg.message.from, 
                           ISI_ADDRESS_STRING_SZ, "%s", "in-pdu");
                   isi_ptr->msg.message.reasonDesc[0] = 0;
                   isi_ptr->msg.message.to[0] = 0;
                   isi_ptr->msg.message.subject[0] = 0;
                   isi_ptr->msg.message.messageId[0] = 0;
                   isi_ptr->msg.message.dateTime[0] = 0;
                   isi_ptr->msg.message.report = 0;
                   isi_ptr->msg.message.chatId = 0;
                   isi_ptr->msg.message.type = ISI_MSG_TYPE_PDU_3GPP;

                   state = 1;
                }
                break;
            case 1:
                /* Then this current token is the actual SMS (text) message */
                len = (result_ptr->token.length > ISI_TEXT_STRING_SZ) ? 
                        ISI_TEXT_STRING_SZ : result_ptr->token.length;
                OSAL_memCpy(isi_ptr->msg.message.message, 
                        result_ptr->token.start_ptr, len);
                /* NULL terminate */
                isi_ptr->msg.message.message[len] = 0;
                isi_ptr->msg.message.pduLen = len;
                /* Populate the rest of the ISI event */
                _GAPP_textIsiEvt(0, service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_TEXT_REASON_NEW, isi_ptr);
                ret = GAPP_OK;
                return (ret);
            case 2:
                /* 
                 * There is no MWI report of "read messages" in GSM, only 
                 * "unread message" so set the "read message" field to '0'. 
                 */
                isi_ptr->msg.event.settings.args.arg1 = 0;
            
                if (0 == result_ptr->token.length) {
                    /* Then the mailbox is empty */
                    isi_ptr->msg.event.settings.args.arg0 = 0;
                }
                else if (GAPP_MWI_STATUS_OFFSET_END <= result_ptr->token.length) {
                    /* 
                     * NULL terminate the MWI string at the end of where the 
                     * number of "unread meassages" is reported.  
                     */
                    result_ptr->token.start_ptr[GAPP_MWI_STATUS_OFFSET_END] = 0;
                    /* Advance to the start of the "unread messages" value */
                    result_ptr->token.start_ptr += GAPP_MWI_STATUS_OFFSET_START;
                    /* 
                     * convert the string representing the number of 
                     * "unread messages" to an integer.
                     */
                    isi_ptr->msg.event.settings.args.arg0 = 
                            OSAL_atoi(result_ptr->token.start_ptr);
                }
                else {
                    /* 
                     * Do a default behavior. We are not sure what the 
                     * format of the MWI report is so tell ISI that 
                     * there is '1' unread message.
                     */
                    isi_ptr->msg.event.settings.args.arg0 = 1;
                }
                /* Populate the rest of the ISI event */
                GAPP_telEvtIsiEvt(0, service_ptr->isiServiceId, 
                        service_ptr->protocolId,
                        ISIP_TEL_EVENT_REASON_NEW, ISI_TEL_EVENT_VOICEMAIL, 
                        isi_ptr);
                return (ret);
            case 3:
                /* 
                 * Let's get the messageReference and the status of the 
                 * delivery report.
                 */
                if (GAPP_OK == _GSM_getSmsMessageStatus(
                        result_ptr->token.start_ptr, result_ptr->token.length,
                        &isi_ptr->msg.message)) {
                    _GAPP_textIsiEvt(0, service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_TEXT_REASON_REPORT, isi_ptr);
                }
                return (ret);
            default:
                break;
        } /* End of switch */
    } /* End of while */    
    return (ret);
}
 
/*
 * ======== _GAPP_isiTelEventCallForwardCmd() ========
 * This function is the handler for the 'conditional call forwarding' 
 * telephone event.
 *
 * Return Values:
 * Nothing.
 */
static void _GAPP_isiTelEventCallForwardCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr)
{
    ISIP_TelEvent *tel_ptr;
    vint           forward;
        
    tel_ptr = &cmd_ptr->msg.event;
    

    /* 
     * Convert the enumerated ISI forward condition value to a value the GSM 
     * interface will understand.  These values are defined by the GSM AT
     * command spec.
     */
    switch (tel_ptr->settings.forward.condition) {
        case ISI_FORWARD_UNCONDITIONAL:
            forward = 0;
            break;
        case ISI_FORWARD_BUSY:
            forward = 1;
            break;
        case ISI_FORWARD_NO_REPLY:
            forward = 2;
            break;
        case ISI_FORWARD_UNREACHABLE:
            forward = 3;
            break;
        default:
            /* Then the condition is not supported */
            GAPP_telEvtIsiEvt(cmd_ptr->id, service_ptr->isiServiceId,
                    service_ptr->isiServiceId, 
                    ISIP_TEL_EVENT_REASON_ERROR, tel_ptr->evt, isi_ptr);
            return;
    }
    
    /* 
     * Cache the tel event values. These are used once we get a result 
     * back from GSM 
     */
    service_ptr->telEvt.isiId = cmd_ptr->id;
    service_ptr->telEvt.evt = tel_ptr->evt;
    
    /* 
     * In the following OSAL_snprintf calls you will see '3' and '4' 
     * hardcoded values.  These are defined by the AT command set.  
     * 3 = register the call forward setting. 
     * 4 = erase the call forward setting.
     */
    if (0 == tel_ptr->settings.forward.enable) {
        /* Then we are disabling the call forward setting */
        GSM_cmdDisableCallForwarding(&service_ptr->daemon, 
                &service_ptr->telEvt.gsmId, forward);
    }
    else {
        /* Then we are enabling */
        if (2 == forward && 0 != tel_ptr->settings.forward.timeout) {
            /* 
             * This is the 'no reply' condition which means we have
             * to include the timeout as well if it's set.
             */
            GSM_cmdEnableCallForwardingNoReply(&service_ptr->daemon,
                    &service_ptr->telEvt.gsmId, forward, tel_ptr->to, 
                    tel_ptr->settings.forward.timeout);
        }
        else {
            GSM_cmdEnableCallForwarding(&service_ptr->daemon,
                    &service_ptr->telEvt.gsmId, forward, tel_ptr->to);
        }
    }
    return;
}

/*
 * ======== _GAPP_isiTelEventDtmfCmd() ========
 * This function is the handler for DTMF related telephone events.
 *
 * Return Values:
 * Nothing.
 */
static void _GAPP_isiTelEventDtmfCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr)
{
    ISIP_TelEvent *te_ptr;
    char           digit;
    uint32         duration;
        
    te_ptr = &cmd_ptr->msg.event;
    
    /* Very the digit */
    switch (te_ptr->settings.args.arg0) {
        case 'A':
        case 'a':
            digit = 'A';
            break;
        case 'B':
        case 'b':
            digit = 'B';
            break;
        case 'C':
        case 'c':
            digit = 'C';
            break;
        case 'D':
        case 'd':
            digit = 'D';
            break;
        default:
            /* 0 - 9, *, # */
            digit = te_ptr->settings.args.arg0;
            break;
    }
    
    /* Duration is in milliseconds, convert to number of hundrediths */
    //duration = te_ptr->settings.args.arg1 / GAPP_GSM_DTMF_DURATION_DIVISOR;
    // For Nexus one we use the duration value verbatim.
    duration = te_ptr->settings.args.arg1;
    
    /* 
     * Cache the tel event values. These are used once we get a result 
     * back from GSM.
     */
    service_ptr->telEvt.isiId = cmd_ptr->id;
    service_ptr->telEvt.evt = te_ptr->evt;
    
    GSM_cmdDialDigit(&service_ptr->daemon, &service_ptr->telEvt.gsmId,
            digit, duration);
   
    return;
}

/*
 * ======== _GAPP_isiTelEventSendUSSDCmd() ========
 *
 * Return Values:
 * Nothing.
 */
static void _GAPP_isiTelEventSendUSSDCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr)
{
    ISIP_TelEvent *te_ptr;
        
    te_ptr = &cmd_ptr->msg.event;
    /* 
     * Cache the tel event values. These are used once we get a result 
     * back from GSM.
     */
    service_ptr->telEvt.isiId = cmd_ptr->id;
    service_ptr->telEvt.evt = te_ptr->evt;
    GSM_cmdSendUssd(&service_ptr->daemon, &service_ptr->telEvt.gsmId, te_ptr->to);
}

/*
 * ======== _GAPP_isiGetServiceAttribute() ========
 *
 * Return Values:
 * GAPP_ERR: Could not insert the phone num into the list. List is full.
 * GAPP_OK: User was successfully added to list.
 */
static int _GAPP_isiGetServiceAttribute(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr)
{
    ISIP_TelEvent       *te_ptr;
    ISI_SeviceAttribute  cmd;
    char                *arg1_ptr;
       char                *arg2_ptr;

    te_ptr = &cmd_ptr->msg.event;
    /* 
     * Cache the tel event values. These are used once we get a result 
     * back from GSM.
     */
    service_ptr->telEvt.isiId = cmd_ptr->id;
    service_ptr->telEvt.evt = te_ptr->evt;

    cmd = te_ptr->settings.service.cmd;
    arg1_ptr = te_ptr->settings.service.arg1;
    arg2_ptr = te_ptr->settings.service.arg2;

    GSM_cmdIsim(&service_ptr->daemon, &service_ptr->telEvt.gsmId, cmd,
            arg1_ptr, arg2_ptr);

    return (GAPP_OK);
}

/* 
 * ======== _GAPP_insertBlockedUser() ========
 * This function inserts a NULL terminated string containing a phone
 * number of a user that this user wishes to block (deny service to).
 *
 * Return Values:
 * GAPP_ERR: Could not insert the phone num into the list. List is full.
 * GAPP_OK: User was successfully added to list.
 */  
static vint _GAPP_insertBlockedUser(
    GAPP_BlockUsers *block_ptr,
    char            *user_ptr)
{
    vint x;
    /* Search for an empty spot in the array and insert */
    for (x = 0 ; x < block_ptr->maxUsers ; x++) {
        if (block_ptr->aUsers[x][0] == 0) {
            /* Found one */
            OSAL_snprintf(block_ptr->aUsers[x], GAPP_STRING_SZ, 
                    "%s", user_ptr);
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/* 
 * ======== _GAPP_serviceBlockUser() ========
 * This function is called when ISI has commanded GAPP to add
 * a user to a list of users used to track remote user's 
 * that we want to deny service to.
 *
 * User's that we deny service to are considered to be "blocked".
 *
 * Return Values:
 * Nothing.
 */
static void _GAPP_serviceBlockUser(
    GAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
    /* If the status is invalid from ISI then we want to block */
    if (s_ptr->status == ISIP_STATUS_TRYING) {
        _GAPP_insertBlockedUser(
                &service_ptr->blockedUsers, s_ptr->settings.uri);
    }
    else {
        GAPP_searchBlockedUser(
                &service_ptr->blockedUsers, s_ptr->settings.uri, 1);
    }
    return;
}
 
/* 
 * ======== _GAPP_serviceIdentity() ========
 * This function is called when GAPP receives a command from ISI to 
 * hide/enable the caller ID when making phone calls.
 *
 * service_ptr : A pointer to a GAPP_GsmAtServiceObj uses internally by GAPP
 *               to manage "service" settings.
 *
 * s_ptr : A pointer to the ISIP_Service object nested inside a ISIP_Message
 *         object. The ISIP_Message is the command that came from ISI.
 *
 * Return Values:
 * Nothing.
 */
static void _GAPP_serviceIdentity(
    GAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
    service_ptr->blockCid = s_ptr->settings.identityHide;
    return;
}

/* 
 * ======== _GAPP_serviceRegisterResult() ========
 * This function checks for a registration status in an unsolicited 
 * or solicited result code.  If this function successfully finds a registration
 * status and other parameters, then cache these parameters in serverice object.
 * The possible values of thevalue written in status_ptr are defined by 
 * the GSM AT command spec. 
 * Refer the the AT Command AT_CREG for more details.
 * If the result is solicited, 
 * the format is "+CREG: <n>,<stat>[,<lac>,<ci>]"
 * If the result is unsolicited,
 * the format is "+CREG: <stat>[,<lac>,<ci>]"
 *
 * Return Values:
 * GAPP_OK: find CREG parameters.
 * GAPP_ERR: The event format is not CREG.
 */
static vint _GAPP_serviceRegisterResult(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr)
{
    GAPP_Buffer buff;
    char        status;
    int         idx, comma, found;

    found = 0; // registration is not found yet.
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_SERVICE_REG_REPORT,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_SERVICE_REG_REPORT) - 1)) {

            GAPP_initBuffer(result_ptr->token.start_ptr, 
                    result_ptr->token.length, &buff);

            /* check this result event is solicited or unsolicited by comma. */
            comma = 0;
            for (idx = 0; idx < buff.length; idx++) {
                if (',' == buff.start_ptr[idx]) {
                    comma++;
                }
            }

            if ((0 == comma) || (2 == comma)) {
                found = 1;
                idx   = 0;
                if (GAPP_OK == GAPP_getToken(&buff, ":")) {
                    while (GAPP_OK == GAPP_getToken(&buff, ",")) {
                        if (0 == idx) {
                            status = *buff.token.start_ptr;
                            /* Cache modem network registration status. */
                            service_ptr->netReg->gsmState = OSAL_atoi(&status);
                            GAPP_dbgPrintf("%s %d GSM Register state is:%c\n",
                                    __FUNCTION__, __LINE__, status);
                        }
                        else if (1 == idx) {
                            /* Cache modem network registration location. */
                            _GAPP_copyToken(
                                 service_ptr->netReg->locationAreaCode,
                                 sizeof(service_ptr->netReg->locationAreaCode),
                                 &buff.token);
                            GAPP_dbgPrintf("%s %d GSM Register lac is:%s\n",
                                    __FUNCTION__, __LINE__, 
                                    service_ptr->netReg->locationAreaCode);
                        }
                        else if (2 == idx) {
                            /* Cache modem network registration cellId. */
                            _GAPP_copyToken(service_ptr->netReg->cellId,
                                    sizeof(service_ptr->netReg->cellId),
                                    &buff.token);
                            GAPP_dbgPrintf("%s %d GSM Register ci is:%s\n",
                                    __FUNCTION__, __LINE__, 
                                    service_ptr->netReg->cellId);
                        }
                        idx++;
                    }
                    break;
                }
            }
            else if ((1 == comma) || (3 == comma)) {
                found = 1;
                idx   = 0;
                if (GAPP_OK == GAPP_getToken(&buff, ":")) {
                    while (GAPP_OK == GAPP_getToken(&buff, ",")) {
                        if (1 == idx) {
                            status = *buff.token.start_ptr;
                            /* Cache modem network registration status. */
                            service_ptr->netReg->gsmState = OSAL_atoi(&status);
                            GAPP_dbgPrintf("%s %d GSM Register state is:%c\n",
                                    __FUNCTION__, __LINE__, status);
                        }
                        else if (2 == idx) {
                            /* Cache modem network registration location. */
                            _GAPP_copyToken(
                                    service_ptr->netReg->locationAreaCode,
                                    sizeof(service_ptr->netReg->locationAreaCode),
                                    &buff.token);
                            GAPP_dbgPrintf("%s %d GSM Register lac is:%s\n",
                                    __FUNCTION__, __LINE__, 
                                    service_ptr->netReg->locationAreaCode);
                        }
                        else if (3 == idx) {
                            /* Cache modem network registration cellId. */
                            _GAPP_copyToken(service_ptr->netReg->cellId,
                                    sizeof(service_ptr->netReg->cellId),
                                    &buff.token);
                            GAPP_dbgPrintf("%s %d GSM Register ci is:%s\n",
                                    __FUNCTION__, __LINE__, 
                                    service_ptr->netReg->cellId);
                        }
                        idx++;
                    }
                    break;
                }
            }
        }
    }
    if (0 == found) {
        /* This result is not registration or the format is incorrect. */
        return (GAPP_ERR);
    }
    return (GAPP_OK);    
}

/* 
 * ======== getAddress() ========
 * This function will look for an address that is in the payload of a result.
 * i.e. "something,something,something,"18055643424", something".
 * It will then copy and NULL terminate the address in the detination buffer.
 *
 * Users can specify the value delimiter in the source buffer and also the
 * number of times the delimiter is detected before the target address is found.
 * For example, for the aboce example, the delimiter_ptr would be "," and 
 * numDelimiter would be 3.
 *
 * Return Values:
 * GAPP_OK: An address was successfully parsed and copied into the destination
 *          buffer.
 * GAPP_ERR: No address was found in the source buffer.
 */
vint getAddress(
    char *source_ptr,
    vint  maxSourceLen,
    char *destination_ptr,
    vint  maxDestinationLen,
    char *delimiter_ptr,
    vint  numDelimiter)
{
    uint32      len;
    GAPP_Buffer buff;
    
    /* Init the buffer object used to parse the string */
    GAPP_initBuffer(source_ptr, maxSourceLen, &buff);
    
    /* 
     * Move down to where the address would be. i.e. the number of "," 
     * down the buffer.
     */
    while (numDelimiter != 0) { 
        if (GAPP_OK != GAPP_getToken(&buff, delimiter_ptr)) {
            /* Then no address can be found */
            return (GAPP_ERR);
        }
        numDelimiter--;
    }
    
    /* 
     * If we are here then the address is the next item wrapped 
     * in double qoutes
     */
    if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
        if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
            /* Found the address, let's copy it to the destination buffer */
            len = maxDestinationLen;
            len = (buff.token.length >= len) ? (len - 1) : buff.token.length;
            OSAL_memCpy(destination_ptr, buff.token.start_ptr, len);
            /* NULL terminate */
            destination_ptr[len] = 0;
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}
#endif
/* 
 * ======== getAddressAndIndex() ========
 * This function will look for an address and call index that is in the payload
 * of a result. i.e. "something,something,something,"18055643424", something".
 * It will then copy and NULL terminate the address in the detination buffer.
 *
 * Users can specify the value delimiter in the source buffer and also the
 * number of times the delimiter is detected before the target address is found.
 * For example, for the aboce example, the delimiter_ptr would be "," and 
 * numDelimiter would be 3.
 *
 * Return Values:
 * GAPP_OK: An address and call index were successfully parsed and copied 
 * into the destination buffer and call index pointer.
 * GAPP_ERR: No address or index was found.
 */
vint getAddressAndIndex(
    char *source_ptr,
    vint  maxSourceLen,
    char *destination_ptr,
    vint  maxDestinationLen,
    vint *callIdx_ptr,
    char *delimiter_ptr,
    vint  numDelimiter)
{
    uint32      len;
    GAPP_Buffer buff;
    
    /* Init the buffer object used to parse the string */
    GAPP_initBuffer(source_ptr, maxSourceLen, &buff);
    
    /* Clean the call index */
    *callIdx_ptr = 0;
    
    /* 
     * Move down to where the address would be. i.e. the number of "," 
     * down the buffer.
     */
    while (numDelimiter != 0) { 
        if (GAPP_OK != GAPP_getToken(&buff, delimiter_ptr)) {
            /* Then no address can be found */
            return (GAPP_ERR);
        }
        numDelimiter--;
    }
    
    /* 
     * If we are here then the address is the next item wrapped 
     * in double qoutes
     */
    if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
        if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
            /* Found the address, let's copy it to the destination buffer */
            len = maxDestinationLen;
            len = (buff.token.length >= len) ? (len - 1) : buff.token.length;
            OSAL_memCpy(destination_ptr, buff.token.start_ptr, len);
            /* NULL terminate */
            destination_ptr[len] = 0;
            /* Let's get the index */
            if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                if (GAPP_OK == GAPP_getToken(&buff, GAPP_GSM_CRLF)) {
                    *callIdx_ptr = (*buff.token.start_ptr) - 48;
                }
            }
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_telEvtIsiEvt() ========
 * This function is used by various other functions to populate ISI events
 * that arerelated to telephone events. These ISI events will be passed from GAPP
 * to the ISI module. 
 *
 * Return Values:
 * Nothing.
 */   
void GAPP_telEvtIsiEvt(
    ISI_Id              evtId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_TelEvtReason   reason,
    ISI_TelEvent        event,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = evtId;
    isi_ptr->protocol = protocolId;
    isi_ptr->code = ISIP_CODE_TEL_EVENT;
    isi_ptr->msg.event.serviceId = serviceId;
    isi_ptr->msg.event.callId = 0;
    isi_ptr->msg.event.reason = reason;
    isi_ptr->msg.event.evt = event;
    isi_ptr->msg.event.to[0] = 0;
    isi_ptr->msg.event.from[0] = 0;
    return;
}

/* 
 * ======== GAPP_searchBlockedUser() ========
 * This function will search to see if a particular user's callerID
 * is in the list of "Blocked" users.  If a user is found in the list
 * then that means that we should deny this user access to this device. 
 *
 * This function can also be used to search and remove a user.  For example,
 * if the user of this device wishes to allow another remote user to contact 
 * us.
 *
 * Return Values:
 * GAPP_ERR : Could not find the user in the list.
 * GAPP_OK : User was successfully found (and possibly removed).
 */
vint GAPP_searchBlockedUser(
    GAPP_BlockUsers *block_ptr,
    char            *uri_ptr,
    vint             shouldRemove)
{
    vint x;
    /* See if the URI exists in the list */
    for (x = 0 ; x < block_ptr->maxUsers ; x++) {
        if (OSAL_strncmp(block_ptr->aUsers[x], uri_ptr, GAPP_STRING_SZ) == 0) {
            /* Found it */
            if (shouldRemove == 1) {
                block_ptr->aUsers[x][0] = 0;
            }
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_sendEvent() ========
 *
 * This function sends events to ISI.
 *
 * Returns: 
 * GAPP_OK  : Event sent.
 * GAPP_ERR : Error.
 */   
vint GAPP_sendEvent(
    GAPP_Event *evt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvtQId,
            (char *)&evt_ptr->isiMsg, sizeof(ISIP_Message), OSAL_WAIT_FOREVER,
            NULL)) {
        GAPP_dbgPrintf("%s: ERROR to send event.\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    return (GAPP_OK);
}

/* 
 * ======== GAPP_getToken() ========
 * This function is used search and return tokens in result code strings
 * that are received from the GSM module.
 *
 * Return Values: 
 * GAPP_OK: A token was returned and the details are in buff_ptr->token
 * GAPP_ERR: There is no token to process.  The escape sequence
 *         specified in esc_ptr was not found.
 */
vint GAPP_getToken(
    GAPP_Buffer  *buff_ptr, 
    const char   *esc_ptr)
{
    GAPP_Token  *token_ptr;
    char      *bufEnd;
    uint16     holeLen; 
    uint16     inc;
    char       currc;
    
    token_ptr = &buff_ptr->token;
    bufEnd  = buff_ptr->start_ptr + buff_ptr->length;
    
    OSAL_memSet(token_ptr, 0, sizeof(GAPP_Token));

    for (; buff_ptr->curr_ptr < bufEnd ; buff_ptr->curr_ptr++) {
        if ( !(*buff_ptr->curr_ptr == ' ') )
            break;
    }
   
    if (buff_ptr->curr_ptr >= bufEnd) {
        buff_ptr->isEnd = OSAL_TRUE;
        return (GAPP_ERR);
    }
   
    token_ptr->start_ptr = buff_ptr->curr_ptr;
    holeLen = 0;
    while (buff_ptr->curr_ptr < bufEnd) { 
        currc = *buff_ptr->curr_ptr;
        
        /* If we've got the CRLF then inc = 2; */
        if (0 == OSAL_strncmp(GAPP_GSM_CRLF, buff_ptr->curr_ptr,
                sizeof(GAPP_GSM_CRLF) - 1)) {
            inc = 2;
        }
        else {
            inc = 1;
        }

        /* if the character is a delimiter then break the loop */
        if (NULL != OSAL_strchr(esc_ptr, currc)) {
            token_ptr->dmtr_ptr  = buff_ptr->curr_ptr;
            buff_ptr->curr_ptr += inc;
            break;
        }

        /* if the character is not a delimiter but whitespace */
        if (currc == ' ') {
            holeLen++;
        }
        else {
           /* Increase length by number of whitespaces and by inc */
            token_ptr->length += holeLen + inc;
            holeLen = 0;
        }
        buff_ptr->curr_ptr += inc; 
    }

    if (buff_ptr->curr_ptr >= bufEnd) {
        buff_ptr->isEnd = OSAL_TRUE;
    }
    return (GAPP_OK);
}

/* 
 * ======== GAPP_initBuffer() ========
 * This function is used to initialize an GAPP_Buffer.  This object is 
 * used to track the state of a buffer when parsing.
 *
 * Return Values:  
 * Nothing.
 */
void GAPP_initBuffer(
    char        *result_ptr,
    vint         resultLen,
    GAPP_Buffer *buff_ptr)
{
    buff_ptr->isEnd = OSAL_FALSE;
    if (NULL == result_ptr) {
        /* Then attempt to reset the state of this parsing mechanism */
        result_ptr = buff_ptr->start_ptr;
        /* Leave the length field alone */
    }
    else {
        buff_ptr->length = resultLen;
    }
    buff_ptr->start_ptr = buff_ptr->curr_ptr = result_ptr;
    return;
}

/* 
 * ======== GAPP_chkResult() ========
 * This function is used to check for a particular result code value
 * specified by compare_ptr within the whole result code string
 * specified in result_ptr.
 *
 * Return Values: 
 * GAPP_OK: The result code in compare_ptr was found.
 * GAPP_ERR: The result code in compare_ptr did not exist in result_ptr.
 */
vint GAPP_chkResult(
    GAPP_Buffer  *result_ptr,
    const char   *compare_ptr,
    vint          compareSize)
{
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(compare_ptr, result_ptr->token.start_ptr,
                compareSize)) {
            return (GAPP_OK);
        }
        if (0 == OSAL_strncmp(compare_ptr, GAPP_GSM_OK, compareSize)) {
            if (0 == OSAL_strncmp("0", result_ptr->token.start_ptr, 1)) {
                return (GAPP_OK);
            }
        }
    }
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_initService() ========
 * This function initializes the GAPP service object used 
 * to manage the GSM service. 
 *
 * Return Values:  
 * Nothing.
 */
void GAPP_initService(
    GAPP_ServiceObj *service_ptr,
    GAPP_GsmObj     *gsm_ptr,
    ISI_Id           isiServiceId,
    vint             protocolId)
{
    vint x;
    
    service_ptr->isiServiceId = isiServiceId;
    service_ptr->protocolId = protocolId;
    service_ptr->blockCid = 0;
    service_ptr->reg.regGsmId = 0;
    service_ptr->reg.unregGsmId = 0;
    service_ptr->reg.isRegistered = OSAL_FALSE;
    service_ptr->sms.gsmId = 0;
    service_ptr->sms.isConstructed = OSAL_FALSE;
    service_ptr->telEvt.gsmId = 0;
    service_ptr->networkOperator[0] = 0;
    
   /* Do not clear ringTemplate or vdi values in the service object */
    
    for (x = 0 ; x < GAPP_CALL_NUM; x++) {
        service_ptr->aCall[x].idx = x + 1;
        GAPP_callInit(service_ptr, &service_ptr->aCall[x]);
        
    }
    
    /* Clear the bits that represent if a call is active */
    service_ptr->callBits.active = 0;
    /* Clear the bits that represent if a call is on hold */
    service_ptr->callBits.hold = 0;
    /* Clear the bits that represent if a call is ringing */
    service_ptr->callBits.ring = 0;
    /* Clear the bits that represent if a call is in conference */
    service_ptr->callBits.conf = 0;

    /* Init the database used for 'blocked users' */
    service_ptr->blockedUsers.maxUsers = GAPP_MAX_BLOCKED_USERS;
    for (x = 0 ; x < GAPP_MAX_BLOCKED_USERS ; x++) {
        service_ptr->blockedUsers.aUsers[x][0] = 0;
    }

    /* Service needs handle to proxy network registration. */
    service_ptr->netReg = &gsm_ptr->proxyCmdMngr.networkReg;

    /* Extended Dial command enabled. */
    service_ptr->extDialCmdEnabled = gsm_ptr->proxyCmdMngr.extDialCmdEnabled;
    return;
}
#ifndef GAPP_DISABLE_GSM
/* 
 * ======== GAPP_isiServiceCmd() ========
 * This function is the entry point for command from ISI related to the
 * "service".
 *
 * Return Values:
 * Nothing.
 */
void GAPP_isiServiceCmd(
    GAPP_GsmObj     *gsm_ptr, 
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr, 
    ISIP_Message    *isi_ptr)
{
    ISI_Id id;
    char  *str_ptr;
    
    id = cmd_ptr->id;

    switch (cmd_ptr->msg.service.reason) {
        case ISIP_SERVICE_REASON_CREATE:
            /* init the local object used to manage the gsm service */
            GAPP_initService(service_ptr, gsm_ptr, 
                    id, cmd_ptr->protocol);
            GAPP_dbgPrintf("%s %d GSM_init() passed\n", __FUNCTION__, __LINE__);
            /* Tell ISI the service was successfully created */
            _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                    ISIP_SERVICE_REASON_CREATE,
                    ISIP_STATUS_DONE, isi_ptr);
            /* Set media profiles if Ext Dial command enabled. */
            if (gsm_ptr->proxyCmdMngr.extDialCmdEnabled) {
                GSM_cmdSetMediaProfiles(&service_ptr->daemon, NULL);
            }

            break;
        
        case ISIP_SERVICE_REASON_DESTROY:
            if (id == service_ptr->isiServiceId) {
                _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DESTROY,
                        ISIP_STATUS_DONE, isi_ptr);
                service_ptr->isiServiceId = 0;
                return;
            }
            _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DESTROY,
                    ISIP_STATUS_INVALID, isi_ptr);
            break;
    
        case ISIP_SERVICE_REASON_ACTIVATE:
            if (id != service_ptr->isiServiceId) {
                _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE,
                        ISIP_STATUS_INVALID, isi_ptr);
                return;
            }
            service_ptr->reg.isRegistered = OSAL_FALSE;
            
            GSM_cmdRegister(&service_ptr->daemon,
                    &service_ptr->reg.regGsmId);
            
            break;
            
        case ISIP_SERVICE_REASON_DEACTIVATE:
            if (id != service_ptr->isiServiceId) {
                _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE,
                        ISIP_STATUS_FAILED, isi_ptr);
                return;
            }
            /* De-register from the network */
            GSM_cmdDeregister(&service_ptr->daemon,
                    &service_ptr->reg.unregGsmId);
            _GAPP_srvcIsiEvt(id, service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, isi_ptr);
             service_ptr->reg.isRegistered = OSAL_FALSE;
            break;
        case ISIP_SERVICE_REASON_BLOCKUSER:
            _GAPP_serviceBlockUser(service_ptr, &cmd_ptr->msg.service);
            break;
        case ISIP_SERVICE_REASON_IDENTITY:
            _GAPP_serviceIdentity(service_ptr, &cmd_ptr->msg.service);
            break;
        case ISIP_SERVICE_REASON_SERVER:
            str_ptr = cmd_ptr->msg.service.settings.server;
            if (ISI_SERVER_TYPE_PROXY == cmd_ptr->msg.service.server) {
                if (str_ptr[0] == 0) {
                    /* Then clear the value */
                    service_ptr->networkOperator[0] = 0;
                }
                else {
                    OSAL_snprintf(service_ptr->networkOperator, 
                            GAPP_STRING_SZ, "%s", str_ptr);
                }
            }
            if (ISI_SERVER_TYPE_OUTBOUND_PROXY == cmd_ptr->msg.service.server) {
                /* 
                 * Set the address (phone number) of the PBX that's FMC 
                 * enabled.  This is considered the 'outbound' device because
                 * it's the next hop the request should go to to make a call.
                 */
                if (str_ptr[0] == 0) {
                    /* Then clear the value */
                    service_ptr->fmcServer[0] = 0;
                }
                else {
                    OSAL_snprintf(service_ptr->fmcServer, 
                            GAPP_STRING_SZ, "%s", str_ptr);
                }
            }
            break;
        case ISIP_SERVICE_REASON_AUTH:
            str_ptr = cmd_ptr->msg.service.settings.credentials.password;
            if (str_ptr[0] == 0) {
                /* Then clear the value */
                service_ptr->fmcDisaPassword[0] = 0;
            }
            else {
                OSAL_snprintf(service_ptr->fmcDisaPassword, 
                        GAPP_STRING_SZ, "%s", str_ptr);
            }
            break;
        case ISIP_SERVICE_REASON_URI:
        case ISIP_SERVICE_REASON_CODERS:
        case ISIP_SERVICE_REASON_HANDOFF:
        case ISIP_SERVICE_REASON_NET:
        case ISIP_SERVICE_REASON_PORT:
        default:
            break;
    } /* End of switch */
    return;
}

/* 
 * ======== GAPP_isiSmsCmd() ========
 * This function is the entry point for commands from ISI related to 
 * SMS or text messaging.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_isiSmsCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr, 
    ISIP_Message    *isi_ptr)
{
    vint   len;
    vint   smsLen;
    vint   reports;
    vint   requestReport = 0;

    switch (cmd_ptr->msg.message.reason) {
        case ISIP_TEXT_REASON_NEW:
            smsLen = GAPP_IM_MAX_SZ;
            len = OSAL_snprintf(service_ptr->sms.msg, smsLen, "%s",
                    cmd_ptr->msg.message.message); 
            if (len > smsLen) {
                /* Then the message was truncated, return an error */
                _GAPP_textIsiEvt(cmd_ptr->id, service_ptr->isiServiceId, 
                        service_ptr->protocolId,
                        ISIP_TEXT_REASON_ERROR, isi_ptr);
                return;
            }
            
            /* Cache the ISI ID for this command */
            service_ptr->sms.isiId = cmd_ptr->id;

            reports = cmd_ptr->msg.message.report;
            if ((0 != (reports & ISI_MSG_RPT_DELIVERY_SUCCESS)) ||
                    (0 != (reports & ISI_MSG_RPT_DELIVERY_FAILED)) ||
                    (0 != (reports & ISI_MSG_RPT_DELIVERY_FORBIDDEN)) ||
                    (0 != (reports & ISI_MSG_RPT_DELIVERY_ERROR))) {
                /*
                 * The users requests any report then let's make sure that
                 * request to told to RIL
                 */
                requestReport = 1;
            }
            
            if (-1 == GSM_cmdSendSms(&service_ptr->daemon, 
                    &service_ptr->sms.gsmId, cmd_ptr->msg.message.to, 
                    service_ptr->sms.msg, requestReport)) {
                /* 
                 * Could not send it 
                 * Then the message was truncated, return an error
                 */
                _GAPP_textIsiEvt(cmd_ptr->id, service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_TEXT_REASON_ERROR, isi_ptr);
                return;
            }
            service_ptr->sms.isConstructed = OSAL_TRUE;
            //service_ptr->sms.isConstructed = OSAL_FALSE;
            break;
        case ISIP_TEXT_REASON_INVALID:
        case ISIP_TEXT_REASON_ERROR:
        case ISIP_TEXT_REASON_COMPLETE:
        default:
            break;
    } /* End of switch */
    return;
}

/* 
 * ======== GAPP_isiTelEventCmd() ========
 * This function processes an ISI TEL event command
 *
 * Return Values:
 *    none
 */
void GAPP_isiTelEventCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr)
{
    ISIP_TelEvent   *te_ptr;
    ISI_TelEvent     evt;
    te_ptr = &cmd_ptr->msg.event;
    
    /* We only want the "new" reason */
    if (ISIP_TEL_EVENT_REASON_NEW != te_ptr->reason) {
        /* Then we are not interested in this event. Tell ISI about error. */
        GAPP_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }
    evt = te_ptr->evt;
    if (ISI_TEL_EVENT_CALL_FORWARD == evt) {
        /* Currently we only support DTMF digits */
        _GAPP_isiTelEventCallForwardCmd(service_ptr, cmd_ptr, isi_ptr);
    }
    else if (ISI_TEL_EVENT_DTMF_OOB == evt || ISI_TEL_EVENT_DTMF == evt) {
        _GAPP_isiTelEventDtmfCmd(service_ptr, cmd_ptr, isi_ptr);
    }
    else if (ISI_TEL_EVENT_SEND_USSD == evt) {
        _GAPP_isiTelEventSendUSSDCmd(service_ptr, cmd_ptr, isi_ptr);
    }
    else if (ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE == evt) {
        _GAPP_isiGetServiceAttribute(service_ptr, cmd_ptr, isi_ptr);
    }
    else {
        /* Then the event is not supported return an error */
        GAPP_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
    }
    return;
}

/* 
 * ======== _GAPP_smsCmtiUnsolicitedEvent() ========
 * This function is used to check for and process +CMTI result codes.
 * These result codes indicate that there is an SMS in the SIM memory.
 * This routine will issue a command to retrieve the SMS and then delete
 * id.
 *
 * Return Values:
 * GAPP_OK: The result code was a +CMTI: was it was processed.
 * GAPP_ERR: The result code was no a +CMTI: and should be continued
 *           to be processed.
 */  
static vint _GAPP_smsCmtiUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    ISIP_Message    *isi_ptr)
{
    GAPP_Buffer  buff;
    vint         ret;
    char         number[16];
    GSM_Id       gsmId;
    
    /* 
     * Is it an 'unsolicited' SMS 'CMTI' result code?.  If so then issue a 
     * command to retrieve the SMS at the specified memory index.
     */
    ret = GAPP_ERR;
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_SMS_MEM_RECVD, 
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_SMS_MEM_RECVD) - 1)) {
            /* Then it's a new SMS message in memory */
            ret = GAPP_OK;
            GAPP_initBuffer(result_ptr->token.start_ptr, 
                result_ptr->token.length, &buff);
            /* Get the index into memory */
            
            if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                if (OSAL_TRUE == buff.isEnd) {
                    /* Then there's nothing here */
                    continue;
                }
                /* Get the next token */
                if (GAPP_OK == GAPP_getToken(&buff, GAPP_GSM_CRLF)) {
                    /* Copy the value into a buffer */
                    _GAPP_copyToken(number, sizeof(number), &buff.token);
                    GAPP_dbgPrintf("%s %d GSM SMS in memory at: %s\n",
                                __FUNCTION__, __LINE__, number);
                    
                    /* Get the SMS message */
                    GSM_cmdReadSmsMem(&service_ptr->daemon, 
                            &service_ptr->smsRead.gsmId, number);
                    
                    /* Now delete the SMS from memory */
                    GSM_cmdDeleteSmsMem(&service_ptr->daemon, &gsmId, number);
                }
            }
        }
    }
    return (ret);
}


/* 
 * ======== GAPP_serviceResultEvent() ========
 * This function processes a result code from the GSM that is related to
 * a previous GSM AT command.  This function checks is a result code
 * belongs to a previously issue AT command related to the GMS service
 * and if it does then it processes it and returns an "OK" indicating
 * that the result code was processed and no further processing is needed.
 *
 * Return Values:
 * GAPP_OK: The result code belonged to a service command and was processed
 * GAPP_ERR: The result code was not processed, the code calling this
 *         function should continue to process this result code.
 */
vint GAPP_serviceResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    GAPP_Event      *evt_ptr)
{
    int  regStatus;

    if (service_ptr->reg.regGsmId == gsmId) {
        /* Then this is a result code for a registration command */
        if (GAPP_OK  == _GAPP_serviceRegisterResult(service_ptr, result_ptr)) {
            regStatus = service_ptr->netReg->gsmState;
            if ((1 == regStatus) || (5 == regStatus) || 
                    (2 == regStatus)) {
                /* Registration was success, tell ISI */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_DONE,
                        &evt_ptr->isiMsg);
                service_ptr->reg.isRegistered = OSAL_TRUE;
            }
            else if ((3 == regStatus)) {
                /* Then there was a permissions (authentication) issue */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                        &evt_ptr->isiMsg);
                GAPP_sendEvent(evt_ptr);

                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
            }
            else {
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
            }
            /* return "OK" indicating that this function processed it */
            return (GAPP_OK);
        }
    }
    else if (service_ptr->reg.unregGsmId == gsmId) {
        /* Then this is a result code for unregistering */
        if (GAPP_OK  == _GAPP_serviceRegisterResult(service_ptr, result_ptr)) {
            regStatus = service_ptr->netReg->gsmState;
            if ((1 != regStatus) && (5 != regStatus)) {
                /* then we are not registered */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
            }
            else {
                 _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_FAILED,
                        &evt_ptr->isiMsg);
            }
            /* return "OK" indicating that this function processed it */
            return (GAPP_OK);
        }
    }

    /* XXX add other service related handling here */

    /* 
     * returning Error indicates that this function did not process the result
     * and that the calling code should continue to attempt to process this 
     * result code.
     */
    return (GAPP_ERR);
}

/*
 * ======== _GAPP_serviceSignalStrengthResult() ========
 *
 * Return Values:
 * GAPP_OK: A signal strength was found and was populated in rssi_ptr.
 * GAPP_ERR: No signal strength was found.
 */
static vint _GAPP_serviceSignalStrengthResult(
    GAPP_Buffer   *result_ptr,
    vint           *rssi_ptr)
{
    GAPP_Buffer buff;
    vint        rssi;

    rssi = -1;
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_SIGNAL_REPORT,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_SIGNAL_REPORT) - 1)) {
            /* Then we found "+CSEQ:".  Let's process this */

            GAPP_initBuffer(result_ptr->token.start_ptr,
                result_ptr->token.length, &buff);
            /*
             * Get the end or next comma.
             */
            if (GAPP_OK == GAPP_getToken(&buff, ":")) {
                if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                    rssi = OSAL_atoi(buff.token.start_ptr);
                    GAPP_dbgPrintf("%s %d GSM Signal strength is:%d\n",
                            __FUNCTION__, __LINE__, rssi);
                }
            }
        }
    } /* End of while */
    if (-1 == rssi) {
        /* Then we found no status at all !*/
        return (GAPP_ERR);
    }
    *rssi_ptr = rssi;
    return (GAPP_OK);
}

/* 
 * ======== GAPP_serviceUnsolicitedEvent() ========
 * This function processes an unsolicited result code.  
 * Unsolicited result codes are events from the GSM module 
 * that were NOT stimulated due to a previous AT command.  They are
 * events from GSM that came from the GMS network.
 *
 * Return Values:
 * GAPP_OK: The result code event pertains to a service and was processed.
 *        No further processing of the event is needed.
 * GAPP_ERR: The result code event is not related to a service and was NOT
 *         processed the calling code should continue to process the
 *         result code event.
 */
vint GAPP_serviceUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    vint           sigStrength, regStatus;
    char           scratch[PRXY_AT_COMMAND_SIZE];
    GAPP_Buffer    buff;

    /* Init scratch and buff. */
    OSAL_memSet(scratch, 0, sizeof(scratch));
    GAPP_initBuffer(result_ptr->start_ptr, result_ptr->length, &buff);
    
    /* It's an 'unsolicited' result code. */
    if (GAPP_OK == _GAPP_serviceRegisterResult(service_ptr, result_ptr)) {
        regStatus = service_ptr->netReg->gsmState;
        if (OSAL_TRUE == service_ptr->reg.isRegistered) {
            if (3 == regStatus) {
                /* 
                 * We were denied access to the network, tell ISI about the
                 *  permission problem 
                 */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                        &evt_ptr->isiMsg);
                GAPP_sendEvent(evt_ptr);
            }
            if ((1 != regStatus) && (5 != regStatus) && 
                    (2 != regStatus)) {
                /* Then it failed */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE,
                        ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
                service_ptr->reg.isRegistered = OSAL_FALSE;
            }
        }
        else {
            if ((1 == regStatus) || (5 == regStatus)) {
                /* Registration was success, tell ISI */
                _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_DONE,
                        &evt_ptr->isiMsg);
                service_ptr->reg.isRegistered = OSAL_TRUE;
            }
        }

        /* Re-contruct AT result. */
        while (GAPP_OK == GAPP_getToken(&buff, GAPP_GSM_CRLF)) {
            /* If token is CRLF, fill CRLF. */
            if (0 == buff.token.length) {
                OSAL_snprintf(scratch + OSAL_strlen(scratch), 
                        (PRXY_AT_COMMAND_SIZE - OSAL_strlen(scratch)), 
                        "%s", GAPP_GSM_CRLF);
                continue;
            }
            /* If token is CREG, mix registration status and 
             * contruct CREG result.
             */
            if (0 == OSAL_strncmp(GAPP_GSM_SERVICE_REG_REPORT,
                    buff.token.start_ptr,
                    sizeof(GAPP_GSM_SERVICE_REG_REPORT) - 1)) {
                /* mix registration status. */
                regStatus = _GAPP_mixNetRegStatus(service_ptr);
                /* contruct CREG AT result. */
                if ((2 == service_ptr->netReg->mode) && 
                        ((1 == regStatus) || (5 == regStatus))) {
                    /* lac and cid is required for mode 2 */
                    OSAL_snprintf(scratch + OSAL_strlen(scratch), 
                            (PRXY_AT_COMMAND_SIZE - OSAL_strlen(scratch)), 
                            "%s %d,%s,%s%s",
                            GAPP_GSM_SERVICE_REG_REPORT, regStatus,
                            service_ptr->netReg->locationAreaCode,
                            service_ptr->netReg->cellId, GAPP_GSM_CRLF);
                }
                else {
                    /* No lac and cid is needed */
                    OSAL_snprintf(scratch + OSAL_strlen(scratch), 
                            (PRXY_AT_COMMAND_SIZE - OSAL_strlen(scratch)), 
                            "%s %d%s",
                            GAPP_GSM_SERVICE_REG_REPORT, regStatus, 
                            GAPP_GSM_CRLF);
                }
            }
            else {
                /* If token is not CREG and there is other AT result 
                 * that we do not care, copy it to scratch.
                 */
                if ((PRXY_AT_COMMAND_SIZE - OSAL_strlen(scratch)) > 
                        (vint)(buff.token.length + 1)) {
                    OSAL_snprintf(scratch + OSAL_strlen(scratch),
                            (buff.token.length + 1), "%s", 
                            buff.token.start_ptr);
                    OSAL_snprintf(scratch + OSAL_strlen(scratch), 
                            (PRXY_AT_COMMAND_SIZE - OSAL_strlen(scratch)), 
                            "%s", GAPP_GSM_CRLF);
                }
                else {
                    return (GAPP_ERR);
                }
            }
        }

        /* Write back to result */
        result_ptr->length = OSAL_snprintf(result_ptr->start_ptr,
                PRXY_AT_COMMAND_SIZE, "%s", scratch);

        return (GAPP_OK);
    }
    else if (GAPP_OK == _GAPP_serviceSignalStrengthResult(
            result_ptr, &sigStrength)) {
        return (GAPP_OK);
    }
    /* XXX add more unsolicited events here */
    return (GAPP_ERR);
}

/*
 * ======== _GAPP_smsGetMessageRef() ========
 * SMS status reports will have a "message reference" in them that represents
 * the SMS that the status report is for.  This routine helps get the reference
 * value.  In the GSM world this refernece value is a byte 0 - 255.
 *
 * Return Values:
 *  The message reference value if it exists or '0' if it doesn't.
 */
static vint _GAPP_smsGetMessageRef(
    GAPP_Token *token_ptr)
{
    char *pos_ptr;
    int messageRef = 0;
    if (NULL != (pos_ptr = OSAL_strncasescan(token_ptr->start_ptr,
            token_ptr->length, GAPP_GSM_SMS_SENT))) {
        pos_ptr += (sizeof(GAPP_GSM_SMS_SENT) - 1);
        if (0 > (messageRef = OSAL_atoi(pos_ptr))) {
            messageRef = 0;
        }
    }
    else {
        /* Must be failure response */
        return (-1);
    }
    return (messageRef);
}

/*
 * ======== GAPP_smsResultEvent() ========
 * This function processes a result code from the GSM that is related to
 * a previous GSM AT command for SMS (or text messaging).  
 * This function checks is a result code belongs to an SMS related command
 * and if it does then it processes it and returns an "OK" indicating
 * that the result code was processed and no further processing is needed.
 *
 * Return Values:
 * GAPP_OK: The result code belonged to an SMS related command and was processed
 * GAPP_ERR: The result code was not processed, the code calling this
 *         function should continue to process this result code.
 */
vint GAPP_smsResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    ISIP_Message    *isi_ptr)
{
    vint messageRef;
    if (service_ptr->sms.gsmId == gsmId) {
        /* Then that result event belongs to the sms text message handling */
        if (OSAL_TRUE == service_ptr->sms.isConstructed) {
            /* 
             * Then we are waiting to send the text message.  
             * Verify that we have the "prompt" which indicates 
             * that we can send the text message.
             */
            if (GAPP_OK == GAPP_chkResult(result_ptr,
                    GAPP_GSM_SERVICE_SMS_PROMPT,
                    sizeof(GAPP_GSM_SERVICE_SMS_PROMPT) - 1)) {
                /* 
                 * If we are here then we have the prompt. Issue the text 
                 * message terminated with a Control-Z 
                 * (a.k.a. ASCII "SUBSTITUTE" character).
                 */
                GSM_cmdWriteSms(&service_ptr->daemon, &service_ptr->sms.gsmId,
                        service_ptr->sms.msg);
            }
            else {
                /* We failed, tell ISI */
                _GAPP_textIsiEvt(service_ptr->sms.isiId,
                        service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_TEXT_REASON_ERROR, isi_ptr);
            }
            /* Change the state */
            service_ptr->sms.isConstructed = OSAL_FALSE;
            return (GAPP_OK);
        }
            
        /* Then this must be the result to sending the sms (text) message */
        if (GAPP_OK != GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) { 
            /* The sms (text) message was NOT sent.  Tell ISI */
            _GAPP_textIsiEvt(service_ptr->sms.isiId,
                    service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_TEXT_REASON_ERROR, isi_ptr);
            return (GAPP_OK);
        }
        /* Otherwise, all is well, the token will point to the result line */
        if (-1 == (messageRef =
                _GAPP_smsGetMessageRef(&result_ptr->token))) {
            /* The sms (text) message was NOT sent.  Tell ISI */
            GAPP_dbgPrintf("Failed to send SMS\n");
            _GAPP_textIsiEvt(service_ptr->sms.isiId,
                    service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_TEXT_REASON_ERROR, isi_ptr);
            return (GAPP_OK);
        }
        OSAL_snprintf(isi_ptr->msg.message.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ, "%d", messageRef);
        _GAPP_textIsiEvt(service_ptr->sms.isiId,
                service_ptr->isiServiceId,
                service_ptr->protocolId,
                ISIP_TEXT_REASON_COMPLETE, isi_ptr);
        return (GAPP_OK);
    }
    else if (service_ptr->smsRead.gsmId == gsmId) {
        return _GAPP_smsEvent(service_ptr, result_ptr, isi_ptr);
    }
    /* 
     * Returning Error indicates that this function did not process the result
     * and that the calling code should continue to attempt to process this 
     * result code.
     */
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_telEvtResultEvent() ========
 * This function processes a result code from the GSM that is related to
 * a previous GSM AT command for telephone events. Curently the only
 * telephone event supported is for setting "conditional call forwarding".  
 * This function checks is a result code belongs to a previously issued AT
 * command and if it does then it processes it and returns an "OK" indicating
 * that the result code was processed and no further processing is needed.
 *
 * Return Values:
 * GAPP_OK: The result code belonged to a telephone event related command and 
 *          was processed.
 * GAPP_ERR: The result code was not processed, the code calling this
 *           function should continue to process this result code.
 */
vint GAPP_telEvtResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    ISIP_Message    *isi_ptr)
{
    if (service_ptr->telEvt.gsmId == gsmId) {
        /* Then this result belongs to a tel event */
        if (GAPP_OK != GAPP_chkResult(result_ptr, GAPP_GSM_ERROR,
                sizeof(GAPP_GSM_ERROR) - 1)) {
            /* Coppy result to reasonDesc for pass to ISI */
            OSAL_snprintf(isi_ptr->msg.event.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "%s", result_ptr->start_ptr);
            GAPP_telEvtIsiEvt(service_ptr->telEvt.isiId,
                    service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_TEL_EVENT_REASON_COMPLETE,
                    service_ptr->telEvt.evt, isi_ptr);
        }
        else {
            GAPP_telEvtIsiEvt(service_ptr->telEvt.isiId,
                    service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_TEL_EVENT_REASON_ERROR,
                    service_ptr->telEvt.evt, isi_ptr);
        }
        return (GAPP_OK);
    }
    /* 
     * Returning Error indicates that this function did not process the result
     * and that the calling code should continue to attempt to process this 
     * result code.
     */
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_smsUnsolicitedEvent() ========
 * This function processes an unsolicited result code related to SMS
 * (text messaging & MWI). Unsolicited result codes are events from the GSM 
 * module that were NOT stimulated due to a previous AT command.  They are
 * events from GSM that came from the GSM network.  For example, if someone 
 * sends this endpoint an SMS (text message) then we will get an unsolicited 
 * result code event that indicates that a new SMS message was received.
 *
 * Return Values:
 * GAPP_OK: The result code event pertains to SMS and was processed.
 *        No further processing of the event is needed.
 * GAPP_ERR: The result code event is not related to SMS and was NOT
 *         processed. The calling code should continue to process the
 *         result code event.
 */
vint GAPP_smsUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    ISIP_Message    *isi_ptr)
{
    /* Check if it's a +CMTI: AKA an SMS sitting in memory */
    if (GAPP_OK == _GAPP_smsCmtiUnsolicitedEvent(service_ptr, result_ptr,
            isi_ptr)) {
        /* Then it's been processed */
        return (GAPP_OK);
    }
    /* Reset the buffer and look for +CMT: */
    GAPP_initBuffer(NULL, 0, result_ptr);
    if (GAPP_OK == _GAPP_smsEvent(service_ptr, result_ptr, isi_ptr)) {
        return (GAPP_OK);
    }
    return (GAPP_ERR);
}

/* 
 * ======== GAPP_networkRegSolicitedEvent ========
 * This function process network registration solicited event.
 *
 * Return Values:
 * GAPP_ERR: The result is not result of network registration
 * GAPP_OK:  Return OK, after getting the mixing result of 
 * network registration and re-contruct AT result.
 */
vint GAPP_networkRegSolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    PRXY_NetRegState  regStatus;
    char              scratch[PRXY_AT_COMMAND_SIZE];


    if (GAPP_OK != _GAPP_serviceRegisterResult(service_ptr, result_ptr)) {
        return (GAPP_ERR);
    }
    regStatus = service_ptr->netReg->gsmState;

    /* tell ISI about GSM network registration status. */
    if (OSAL_TRUE == service_ptr->reg.isRegistered) {
        if (3 == regStatus) {
            /* 
             * We were denied access to the network, tell ISI about the
             *  permission problem 
             */
            _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            GAPP_sendEvent(evt_ptr);
        }
        if ((1 != regStatus) && (5 != regStatus) && 
                (2 != regStatus)) {
            /* Then it failed */
            _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            service_ptr->reg.isRegistered = OSAL_FALSE;
        }
    }
    else {
        if ((1 == regStatus) || (5 == regStatus)) {
            /* Registration was success, tell ISI */
            _GAPP_srvcIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_DONE,
                    &evt_ptr->isiMsg);
            service_ptr->reg.isRegistered = OSAL_TRUE;
        }
    }


    /* mix registration status. */
    regStatus = _GAPP_mixNetRegStatus(service_ptr);
    /* contruct AT result. */
    if ((2 == service_ptr->netReg->mode) && 
            ((PRXY_NET_REG_STATE_ACTIVE == regStatus) || 
            (PRXY_NET_REG_STATE_ROAMING == regStatus))) {
        /* Lac and cid is required for mode 2 */
        OSAL_snprintf(scratch, PRXY_AT_COMMAND_SIZE, "%s %d,%d,%s,%s",
                GAPP_GSM_SERVICE_REG_REPORT, service_ptr->netReg->mode,
                regStatus, service_ptr->netReg->locationAreaCode,
                service_ptr->netReg->cellId);
    }
    else {
        /* No lac and cid is needed */
        OSAL_snprintf(scratch, PRXY_AT_COMMAND_SIZE, "%s %d,%d",
            GAPP_GSM_SERVICE_REG_REPORT,
            service_ptr->netReg->mode, regStatus);
    }

    result_ptr->length = OSAL_snprintf(result_ptr->start_ptr,
            PRXY_AT_COMMAND_SIZE, "%s%s%s%s", scratch, GAPP_GSM_CRLF,
            GAPP_GSM_OK, GAPP_GSM_CR);

    return (GAPP_OK);
}
#endif

/* 
 * ======== _GAPP_mixNetRegStatus() ========
 * This function mix network registration CSM state and modem state.
 *
 * Return Values:
 * The result is the status of network registration after mix modem and IMS.
 */
vint _GAPP_mixNetRegStatus(    
    GAPP_ServiceObj *service_ptr)
{
    return (_GAPP_regStatusTable[service_ptr->netReg->gsmState]
            [service_ptr->netReg->csmState]);
}

/* 
 * ======== GAPP_processCsmRegResponse ========
 * This function process CSM network registration state and contruct AT.
 *
 * Return Values:
 * GAPP_ERR: If IMS network registration status is not changed.
 * GAPP_OK:  Return OK, after getting the mixing result of 
 * network registration and re-contruct AT result.
 */
vint GAPP_processCsmRegOutputEvent(
    GAPP_ServiceObj *service_ptr,
    char            *out_ptr)
{
    int  regStatus;

    PRXY_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
    if (OSAL_FALSE == service_ptr->netReg->isCsmRegChange) {
        /* return ERR, do not need to process this response. */
        return (GAPP_ERR);
    }
    else {
        service_ptr->netReg->isCsmRegChange = OSAL_FALSE;
    }

    /* mix registration status. */
    regStatus = _GAPP_mixNetRegStatus(service_ptr);
    /* contruct AT result. */
    if (service_ptr->netReg->isEmergency) {
            OSAL_snprintf(out_ptr, PRXY_AT_COMMAND_SIZE, "%s %d%s", 
                    GAPP_GSM_EMGCY_SERVICE_REG_REPORT, regStatus,
                    GAPP_GSM_CRLF);
    }
    else {
        if ((2 == service_ptr->netReg->mode) &&
                ((1 == regStatus) || (5 == regStatus))) {
            /* lac and cid are required */
            OSAL_snprintf(out_ptr, PRXY_AT_COMMAND_SIZE,
                    "%s %d,%d,%s,%s%s",
                    GAPP_GSM_SERVICE_REG_REPORT,
                    service_ptr->netReg->mode, regStatus,
                    service_ptr->netReg->locationAreaCode,
                    service_ptr->netReg->cellId,
                    GAPP_GSM_CRLF);
        }
        else {
            /* No need lac and cid */
            OSAL_snprintf(out_ptr, PRXY_AT_COMMAND_SIZE, "%s %d,%d%s", 
                    GAPP_GSM_SERVICE_REG_REPORT, 
                    service_ptr->netReg->mode, regStatus,
                    GAPP_GSM_CRLF);
        }
    }

    return (GAPP_OK);
}

