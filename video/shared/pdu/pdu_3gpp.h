/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22547 $ $Date: 2013-10-23 11:17:36 +0800 (Wed, 23 Oct 2013) $
 */

#ifndef _PDU_3GPP_H_
#define _PDU_3GPP_H_

/*
 * Enumeration of TP-MTI(Message-Type-Indication).
 */
typedef enum {
    PDU_3GPP_TP_MTI_SMS_DELIVER = 0,
    PDU_3GPP_TP_MTI_SMS_DELIVER_REPORT,
    PDU_3GPP_TP_MTI_SMS_STATUS_REPORT,
    PDU_3GPP_TP_MTI_SMS_COMMAND,
    PDU_3GPP_TP_MTI_SMS_SUBMIT,
    PDU_3GPP_TP_MTI_SMS_SUBMIT_REPORT,
} PDU_3gppTpMti;

/*
 * Enumeration of RP-MTI(Message-Type-Indication).
 */
typedef enum {
    PDU_3GPP_MTI_RP_DATA_MS_TO_SC = 0,
    PDU_3GPP_MTI_RP_DATA_SC_TO_MS = 1,
    PDU_3GPP_MTI_RP_ACK_MS_TO_SC= 2,
    PDU_3GPP_MTI_RP_ACK_SC_TO_MS = 3,
    PDU_3GPP_MTI_RP_ERROR_MS_TO_SC = 4,
    PDU_3GPP_MTI_RP_ERROR_SC_TO_MS = 5,
    PDU_3GPP_MTI_RP_SMMA_MS_TO_SC = 6,
} PDU_3gppRpMti;

typedef enum {
    PDU_3GPP_DCS_DEFAULT         = 0,
    PDU_3GPP_DCS_VOICEMAIL_ON    = 1,
    PDU_3GPP_DCS_VOICEMAIL_OFF   = 2
} PDU_3gppDataCodingScheme;

int PDU_3gppEncodeSms(
    char  *to_ptr, 
    char  *msg_ptr,
    char  *target_ptr,
    int    maxTargetLen,
    int    requestReport);

int PDU_3gppDecodeSmsReport(
    char *data_ptr,
    char *sc_ptr,
    char *address_ptr,
    int  *messageRef_ptr,
    int  *status_ptr);

int PDU_3gppEncodeDeliverySms(
    char                 *from_ptr,
    char                 *msg_ptr,
    char                 *target_ptr,
    int                   maxTargetLen,
    PDU_3gppDataCodingScheme  pdu_dcs);

int PDU_3gppGetSubmitPduMessageAndAddress(
    const char  *pduBuf_ptr,
    char        *address_ptr,
    char        *msg_ptr,
    int          msgLen);

int PDU_3gppDecodeSms(
    char        *pdu_ptr,
    int          pduLen,
    char        *target_ptr,
    int          maxTargetLen,
    char        *smsc_ptr,
    int          maxSmscLen,
    char        *type_ptr,
    char        *msgRef_ptr,
    char        *errCode_ptr);

int PDU_3gppEncodeDeliverReport(
    char *smsc_ptr,
    char  msgRef,
    char  errorCode,
    char *target_ptr,
    int   maxTargetLen);

int PDU_3gppEncodeSubmit(
    char         *tpdu_ptr,
    int           tpduLen,
    char         *smsc_ptr,
    char          msgRef,
    char         *target_ptr,
    int           maxTargetLen);

int PDU_3gppEncodeSubmitTPDU(
    char          msgRef,
    char         *dstAddress,
    char         *userData,
    char         *outPtr,
    int           maxOutLen);

#endif
