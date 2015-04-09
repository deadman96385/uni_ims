/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#ifndef _PDU_3GPP2_H_
#define _PDU_3GPP2_H_

#define PDU_3GPP2_PARAM_ID_TELESERVICE    (0) /* '00000000' */
#define PDU_3GPP2_PARAM_LEN_REPLY         (1)
#define PDU_3GPP2_PARAM_ID_ORIG_ADDR      (2) /* '00000100' */
#define PDU_3GPP2_PARAM_ID_DEST_ADDR      (4) /* '00000100' */
#define PDU_3GPP2_PARAM_ID_REPLY          (6)
#define PDU_3GPP2_PARAM_ID_CAUSE_CODES    (7) /* '00000111' */
#define PDU_3GPP2_PARAM_ID_BEARER_DATA    (8) /* '00001000' */

#define PDU_3GPP2_PARAM_LEN_TELESERVICE   (2)

#define PDU_3GPP2_TELESERVICE_ID_IMSST_B0 (0x10) /* 4014: 0x1012 */
#define PDU_3GPP2_TELESERVICE_ID_IMSST_B1 (0x12)
#define PDU_3GPP2_TELESERVICE_ID_CDMA_B0  (0x10) /* 4098: 0x1002 */
#define PDU_3GPP2_TELESERVICE_ID_CDMA_B1  (0x02)
#define PDU_3GPP2_TELESERVICE_ID_VM       (4099) /* Voice Mail Notification*/
#define PDU_3GPP2_TELESERVICE_ID_PAGING   (4097) /* Cellular Paging Teleservice */

#define PDU_3GPP2_SUBPARAM_MSG_ID          (0)
#define PDU_3GPP2_SUBPARAM_USER_DATA       (1)
#define PDU_3GPP2_SUBPARAM_LEN_MSG_ID      (3)
#define PDU_3GPP2_SUBPARAM_PRIORITY_ID     (8)
#define PDU_3GPP2_SUBPARAM_NUM_MSG_ID      (11)
#define PDU_3GPP2_SUBPARAM_LANG_ID         (13)
#define PDU_3GPP2_SUBPARAM_CB_NUM_ID       (14)

#define PDU_3GPP2_SUBPARAM_LEN_PRIORITY_ID (1)
#define PDU_3GPP2_SUBPARAM_LEN_LANG_ID     (1)

#define PDU_3GPP2_MSG_TYPE_SUBMIT          (2) 
#define PDU_3GPP2_DIGIT_MODE_4BIT_DTMF     (0)
#define PDU_3GPP2_DIGIT_MODE_8BIT_ASCII    (1)
#define PDU_3GPP2_NUMBER_MODE_DEFAULT      (1)
#define PDU_3GPP2_NUMBER_TYPE_DEFAULT      (2)

#define PDU_3GPP2_DATA_CODING_ASCII        (2)
#define PDU_3GPP2_DATA_CODING_UTF8         (4)
#define PDU_3GPP2_DATA_CODING_RESERVED     (16)

#define PDU_3GPP2_REPLYSEQ_INVALID         (255)
/* Transport layer message type. */
typedef enum {
    PDU_3GPP2_TL_MSG_TYPE_P2P = 0, /* '00000000' */
    PDU_3GPP2_TL_MSG_TYPE_BROADCAST = 1, /* '00000001' */
    PDU_3GPP2_TL_MSG_TYPE_ACK = 2, /* '00000010' */
} PDU_3gpp2TlMsgType;

/* Teleservice message type. */
typedef enum
{
    PDU_3GPP2_TS_MSG_TYPE_NONE = 0,
    PDU_3GPP2_TS_MSG_TYPE_DELIVER = 1, /* MT */
    PDU_3GPP2_TS_MSG_TYPE_SUBMIT = 2, /* MO */
    PDU_3GPP2_TS_MSG_TYPE_CANCELLATION = 3, /* MO */
    PDU_3GPP2_TS_MSG_TYPE_DELIVERY_ACK = 4, /* MT */
    PDU_3GPP2_TS_MSG_TYPE_USER_ACK = 5, /* MO/MT */
} PDU_3gpp2TsMsgType;

PDU_Return PDU_3gpp2DecodeSms(
    char     *payload_ptr,
    int       payloadLen,
    char     *msg_ptr,
    int       maxMsgLen,
    int      *msgCoding_ptr,
    char     *from_ptr,
    int       maxFromLen,
    int      *msgType_ptr,
    uint16   *msgId_ptr,
    int8     *replySeq_ptr,
    uint8    *causeCode_ptr,
    int      *numOfMsg_ptr);

int PDU_3gpp2EncodeSubmit(
    char        *msg_ptr,
    char        *to_ptr,
    char        *smsc_ptr,
    uint16       msgRef,
    char        *target_ptr,
    int          maxTargetLen,
    OSAL_Boolean bearerReplyOption,
    char        *callBackNumber_ptr);

int PDU_3gpp2EncodeSmsAck(
    char  *to_ptr,
    uint16 msgId,
    uint8  causeCode,
    char  *target_ptr,
    int    maxTargetLen);

#endif
