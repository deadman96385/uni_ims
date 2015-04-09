/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include <osal.h>
#include <pduconv.h>
#include <utf8_to_utf16.h>
#include "_pdu_hlpr.h"
#include "pdu_hlpr.h"
#include "pdu_3gpp2.h"

static const char dtmf2Ascii[16] = 
        {'D', '1', '2', '3', '4','5','6','7','8','9','0','*','#','A','B','C'};

/* 
 * ======== _PDU_3gpp2SmsDecodeAddress() ========
 * 
 * Private function to decode 3GPP2 SMS address from PDU.
 *
 * numModeFlag: 1 means "1-bit Number mode" field will
 * be encoded into PDU.
 *
 * Returns: 
 *    PDU_ERR: Failed to decode address.
 *    PDU_OK: Decode address successfully.
 */
static PDU_Return _PDU_3gpp2SmsDecodeAddress(
    uint8        len,
    PDU_Payload *pdu_ptr,
    char        *addr_ptr,
    int          addrLen,
    int          numModeFlag)
{
    uint8 byte;
    uint8 byte2;
    uint8 bitShift;
    uint8 digitMode;
    uint8 numMode;
    uint8 numPlan;
    uint8 numType;
    uint8 numFields;
    uint8 octet;
    vint  i;

    UNUSED(numPlan);
    UNUSED(numType);
    PDU_dbgPrintf("\n");

    _PDU_pduReadByte(pdu_ptr, (char *)&byte);
    len--;

    digitMode = (byte >> 7) & 0x01;
    if (numModeFlag == 1) {
        numMode = (byte >> 6) & 0x01;
        bitShift = 2;
    }
    else {
        numMode = 0;
        bitShift = 1;
    }
    
    if (0 != digitMode) {
        numType = (byte >> 3) & 0x03;
        bitShift += 3;
        if (1 != numMode) {
            _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
            len--;
            numPlan = (((byte >> 0) & 0x03) << 1) | (byte2 >> 7);
            bitShift = 1;
            byte = byte2;
        }
    }
    else if (0 == numMode) {
        /* 4-bit DTMF */
        PDU_dbgPrintf("4-bit DTMF mode\n");
    }
    else {
        PDU_dbgPrintf("Currently doesn't support this modes:"
                "digit mode:%d, number mode:%d\n",
                digitMode, numMode);
    }

    /* Read the num_fields */
    _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
    len--;
    numFields = (byte << bitShift) | (byte2 >> (8 - bitShift));

    if (numFields > addrLen) {
        PDU_dbgPrintf("Address length too short. numFields:%d, addr len:%d\n",
                numFields, addrLen);
        return (PDU_ERR);
    }

    if ((0 == digitMode) && (0 == numMode)) {
        /* 4-bit DTMF */
        for (i = 0; i < len; i++) {
            _PDU_pduReadByte(pdu_ptr, (char *)&byte);
            octet = (byte2 << bitShift) | (byte >> (8 - bitShift));
            addr_ptr[i * 2] = dtmf2Ascii[octet >> 4];
            addr_ptr[i * 2 + 1] =  dtmf2Ascii[octet & 0x0F];
            byte2 = byte;
        }
        if ((numFields & 1) == 1) {
            octet = byte2 << bitShift;
            addr_ptr[i * 2] = dtmf2Ascii[octet >> 4];
        }
        addr_ptr[numFields] = 0;
    }
    else {
        /* Consider it as Data network address */
        for (i = 0; i < len; i++) {
            _PDU_pduReadByte(pdu_ptr, (char *)&byte);
            addr_ptr[i] = (byte2 << bitShift) | (byte >> (8 - bitShift));
            byte2 = byte;
        }
        addr_ptr[i] = (byte2 << bitShift);
    }

    PDU_dbgPrintf("Address:%s\n", addr_ptr);

    return (PDU_OK);
}

/* 
 * ======== _PDU_3gpp2AsciiToDtmf() ========
 * 
 * Private function to convert ASCII to 4-bit DTMF digit.
 * 
 * Returns: 
 *    0x0~0xF: Converted successfully.
 *    Otherwise: Failed to convert.
 */
static uint8 _PDU_3gpp2AsciiToDtmf(
    char c)
{
    if ('1' <= c && '9' >= c) {
        return (c - '0');
    }

    switch (c) {
        case ('0'):
            return (0x0a);
        case ('*'):
            return (0x0b);
        case ('#'):
            return (0x0c);
        case ('A'):
        case ('a'):
            return (0x0d);
        case ('B'):
        case ('b'):
            return (0x0e);
        case ('C'):
        case ('c'):
            return (0x0f);
        case ('D'):
        case ('d'):
            return (0x00);
        default:
            break;
    }

    /* should not be here */
    return (0x10);
}

/* 
 * ======== _PDU_3gpp2SmsAddressEncode() ========
 * 
 * Private function to encode 3GPP2 SMS address to PDU.
 * Input address should be normalized without scheme and domain.
 * 
 * numModeFlag: 1 means "1-bit Number mode" field will
 * be encoded into PDU.
 *
 * Returns: 
 *    0: Failed to encode sms-submit
 *    Non-zero: Ecoded pdu length when encode sms-submit successfully.
 */
static vint _PDU_3gpp2SmsAddressEncode(
    PDU_Payload *pdu_ptr,
    char        *addr_ptr, 
    int          numModeFlag)
{
    uint8 *addrLen_ptr;
    uint8 addressLen;
    uint8 lastByte;
    uint8 byte;
    uint8 bitShift;
    uint8 digitMode;
    uint8 numMode;
    vint  i;
    uint8 octet;
    char  address_ptr[PDU_BYTE_BUFFER_SIZE + 1];

    /* Let's store where we want to write the length of the destination address */
    addrLen_ptr = pdu_ptr->cur_ptr;
    _PDU_pduAdvance(pdu_ptr, 1);

    /* Assume 4-bit DTMF is supported. */
    /* Check if there is a plus */
    if (addr_ptr[0] == '+') {
        OSAL_snprintf(address_ptr, sizeof(address_ptr), "00%s", addr_ptr + 1);
    }
    else {
        OSAL_strncpy(address_ptr, addr_ptr, sizeof(address_ptr));
    }
    addressLen = OSAL_strlen(address_ptr);

    digitMode = PDU_3GPP2_DIGIT_MODE_4BIT_DTMF;
    numMode = 0;
    if (numModeFlag == 1) {
        bitShift = 6;
        byte = digitMode << 7 | numMode << 6 | addressLen >> (8 - bitShift);
    }
    else {
        bitShift = 7;
        byte = digitMode << 7 | addressLen >> (8 - bitShift);
    }
    _PDU_pduWriteByte(pdu_ptr, byte);
    lastByte = addressLen << bitShift;

    for (i = 0; i < addressLen; i += 2) {
        /* If this is the last time then just right the last odd digit. */
        if (i + 2 > addressLen) {
            octet = _PDU_3gpp2AsciiToDtmf(address_ptr[i]) << 4;
            lastByte = lastByte | octet >> (8 - bitShift);
            break;
        }
        octet = _PDU_3gpp2AsciiToDtmf(address_ptr[i]) << 4 |
                _PDU_3gpp2AsciiToDtmf(address_ptr[i + 1]);

        byte = lastByte | octet >> (8 - bitShift);
        _PDU_pduWriteByte(pdu_ptr, byte);
        lastByte = octet << bitShift;
    }
    _PDU_pduWriteByte(pdu_ptr, lastByte);

    *addrLen_ptr = pdu_ptr->cur_ptr - addrLen_ptr - 1;

#if 0 /* To be removed. */
    /* Assume 8-bit ASCII format is supported. */
    digitMode = PDU_3GPP2_DIGIT_MODE_8BIT_ASCII;
    numMode = PDU_3GPP2_NUMBER_MODE_DEFAULT; /* 1 */
    numType = PDU_3GPP2_NUMBER_TYPE_DEFAULT; /* 2 */
    bitShift = 3;
    byte = digitMode << 7 | numMode << 6 | numType << 3 | addressLen >> (8 - bitShift);
    _PDU_pduWriteByte(pdu_ptr, byte);
    totalLen++;
    lastByte = addressLen << bitShift;

    for (i = 0; i < addressLen; i++) {
        byte = lastByte | addr_ptr[i] >> (8 - bitShift);
        _PDU_pduWriteByte(pdu_ptr, byte);
        totalLen++;
        lastByte = addr_ptr[i] << bitShift;
    }
    _PDU_pduWriteByte(pdu_ptr, lastByte);
    totalLen++;
#endif

    return (*addrLen_ptr);
}

/* 
 * ======== _PDU_3gpp2SmsEncodeDeliveryAck() ========
 * 
 * Function to encode 3GPP2 SMS Delivery Ack message
 * 
 * Returns: 
 *    0: Failed to encode Delivery ACK message.
 *    Non-zero: Ecoded pdu length when encode Delivery ACK successfully.
 */
int PDU_3gpp2SmsEncodeDeliveryAck(
    char    *smsc_ptr,
    vint     msgRef,
    char    *to_ptr, 
    char    *target_ptr,
    int      maxTargetLen)
{
    uint8         *bearerDataLen_ptr;
    int            bytes;
    PDU_Payload    pdu;
    char           byte;
        
    _PDU_pduInitEncode(&pdu);
    
    /* Tranport layer message */
    /* SMS_MSG_TYPE - SMS Point-to-Point '00000000' */
    byte = 0x00;
    _PDU_pduWriteByte(&pdu, byte);

    /* SMS parameter identifier */
    byte = 0x00;
    _PDU_pduWriteByte(&pdu, byte);

    /* SMS message parameter length - ‘00000010’ */
    byte = 0x02;
    _PDU_pduWriteByte(&pdu, byte);

    /* Teleservice Identifier - 4096 */
    byte = 0x10;
    _PDU_pduWriteByte(&pdu, byte);
    byte = 0x02;
    _PDU_pduWriteByte(&pdu, byte);
    /* Originator addr */
    _PDU_pduWriteByte(&pdu, 0x02);

    _PDU_3gpp2SmsAddressEncode(&pdu, to_ptr, 1);

    /* Bearer Data id */
    byte = 0x08;
    _PDU_pduWriteByte(&pdu, byte);

    bearerDataLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    /* Message identifier */
    _PDU_pduWriteByte(&pdu, 0);
    /* Len */
    _PDU_pduWriteByte(&pdu, 3);
    /* Msg Type - Delivery Ack */
    _PDU_pduWriteByte(&pdu, 4 << 4);
    byte = msgRef >> 4;
    _PDU_pduWriteByte(&pdu, byte);
    byte = msgRef << 4;
    _PDU_pduWriteByte(&pdu, 0);

    *bearerDataLen_ptr = pdu.cur_ptr - bearerDataLen_ptr - 1;

    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (-1);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    _PDU_printBytes((uint8 *)target_ptr, bytes);
    return (bytes);
}

/* 
 * ======== _PDU_3gpp2SmsDecodeUserData() ========
 * 
 * Private function to decode 3GPP2 SMS user data.
 * 
 * Returns: 
 *    PDU_ERR: Failed to decode user data
 *    PDU_OK: Decode user data successfully.
 */
static vint _PDU_3gpp2SmsDecodeUserData(
    uint8        len,
    PDU_Payload *pdu_ptr,
    char        *msg_ptr,
    vint         msgLen,
    vint        *msgCoding_ptr)
{
    uint8 byte;
    uint8 byte2;
    uint8 bitShift;
    uint8 msgEncoding;
    uint8 msgType = 0;
    uint8 numFields;
    vint  i;
    uint8 ascii7Bit[256];

    UNUSED(numFields);
    UNUSED(msgType);

    PDU_dbgPrintf("\n");

    _PDU_pduReadByte(pdu_ptr, (char *)&byte);
    len--;

    bitShift = 5;
    msgEncoding = byte >> 3;
    if (0x01 == msgEncoding) {
        _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
        len--;
        msgType = byte << 5 | byte2 >> 3;
        byte = byte2;
    }

    _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
    len--;
    numFields = byte << bitShift | byte2 >> (8 - bitShift);
    PDU_dbgPrintf("numFields:%d msgType:%d\n", numFields, msgType);
    byte = byte2;
    if (PDU_3GPP2_DATA_CODING_ASCII == msgEncoding) {
        /* 7-bit ascii */
        for (i = 0; i < len; i++) {
            _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
            ascii7Bit[i] = byte << bitShift | byte2 >> (8 - bitShift);
            byte = byte2;
        }
        ascii7Bit[i] = byte << bitShift;

        ansi7bitAscii_to_ascii(ascii7Bit, i, msg_ptr, msgLen);
        PDU_dbgPrintf("msg:%s\n", msg_ptr);
    }
    else if (PDU_3GPP2_DATA_CODING_UTF8 == msgEncoding) {
        /* UNICODE, consider it as UCS-2 */
        for (i = 0; i < len; i++) {
            _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
            ascii7Bit[i] = byte << bitShift | byte2 >> (8 - bitShift);
            byte = byte2;
        }
        ascii7Bit[i] = byte << bitShift;

        _PDU_printBytes(ascii7Bit, i);
        pdu_utf16_to_utf8((uint16*)ascii7Bit, i, (uint8*)msg_ptr, 256);
    }
    else {
        PDU_dbgPrintf("Currently doesn't support this coding scheme%d\n",
                msgEncoding);
        return (PDU_ERR);
    }
    *msgCoding_ptr = msgEncoding;
    return (PDU_OK);
}

/* 
 * ======== _PDU_3gpp2SmsDecodeCauseCode() ========
 * 
 * Private function to decode 3GPP2 SMS Cause Code
 * 
 * Returns: 
 *    PDU_ERR: Failed to decode Cause Code
 *    PDU_OK: Decode Cause Code successfully.
 */
static PDU_Return _PDU_3gpp2SmsDecodeCauseCode(
    uint8         len,
    PDU_Payload  *pdu_ptr,
    int8         *replySeq_ptr,
    uint8        *causeCode_ptr)
{
    char  byte;

    if (2 < len) {
        PDU_dbgPrintf("Invalid length: %d\n", len);
        return (PDU_ERR);
    }

    _PDU_pduReadByte(pdu_ptr, &byte);
    *replySeq_ptr = byte >> 2;

    if (0 == len) {
        /* No cause code. */
        *causeCode_ptr = 0;
    }
    else {
        /* With cause code. */
        _PDU_pduReadByte(pdu_ptr, (char *)causeCode_ptr);
    }

    return (PDU_OK);
}

/* 
 * ======== _PDU_3gpp2SmsDecodeBearerData() ========
 * 
 * Private function to decode 3GPP2 SMS bearer data.
 * 
 * Returns: 
 *    PDU_ERR: Failed to decode bearer data
 *    PDU_OK: Decode bearer data successfully.
 */
static PDU_Return _PDU_3gpp2SmsDecodeBearerData(
    uint8               len,
    PDU_Payload        *pdu_ptr,
    char               *msg_ptr,
    vint                msgLen,
    int                *msgCoding_ptr,
    PDU_3gpp2TsMsgType *msgType_ptr,
    uint16             *msgId_ptr,
    int                *numOfMsg_ptr,
    char               *from_ptr,
    int                 maxFromLen)
{
    uint8  subParamId;
    uint8  subParamLen;
    uint8  msgType;
    uint8  byte1, byte2;
    uint8  headerInd;

    UNUSED(headerInd);
    UNUSED(msgType);

    msgType = 0;
    *numOfMsg_ptr = -1;
    while (0 < len) {
        /* Read parameter id */
        _PDU_pduReadByte(pdu_ptr, (char *)&subParamId);
        _PDU_pduReadByte(pdu_ptr, (char *)&subParamLen);
        len -= 2;
        PDU_dbgPrintf("sub parameter id:0x%02X len:%d\n",
                subParamId, subParamLen);

        switch (subParamId) {
            case PDU_3GPP2_SUBPARAM_MSG_ID:
                PDU_dbgPrintf("Message identifier\n");
                _PDU_pduReadByte(pdu_ptr, (char *)&byte1);
                msgType = byte1 >> 4;
                _PDU_pduReadByte(pdu_ptr, (char *)&byte2);
                *msgId_ptr = (byte1 << 4 | byte2 >> 4) << 8;
                _PDU_pduReadByte(pdu_ptr, (char *)&byte1);
                *msgId_ptr |= (byte2 << 4 | byte1 >> 4);
                headerInd = (byte1 >> 3) & 0x01;
                PDU_dbgPrintf("Message type:%d msg id:%d,"
                        " header indicator:%d\n",
                        msgType, *msgId_ptr, headerInd);
                break;
            case PDU_3GPP2_SUBPARAM_USER_DATA:
                PDU_dbgPrintf("User data\n");
                if (PDU_OK != _PDU_3gpp2SmsDecodeUserData(
                        subParamLen, pdu_ptr, msg_ptr, msgLen, msgCoding_ptr)) {
                    return (PDU_ERR);
                }
                break;
            case PDU_3GPP2_SUBPARAM_CB_NUM_ID:
                PDU_dbgPrintf("Call Back Number\n");
                _PDU_3gpp2SmsDecodeAddress(subParamLen, pdu_ptr, from_ptr, 
                    maxFromLen, 0);
                break;
            case PDU_3GPP2_SUBPARAM_NUM_MSG_ID:
                /* Number of Messages */
                PDU_dbgPrintf("Number of Messages.\n");
                _PDU_pduReadByte(pdu_ptr, (char *)&byte1);
                /* Convert 4-bit BCD to integer. */
                *numOfMsg_ptr = (byte1 >> 4) * 10 + (byte1 & 0x0F);
                PDU_dbgPrintf("Number of Messages: %d\n", *numOfMsg_ptr);
                break;
            default:
                if (subParamLen > len) {
                    PDU_dbgPrintf("Parameter data length is not correct\n");
                    return (PDU_ERR);
                }
                _PDU_pduAdvance(pdu_ptr, subParamLen);
                break;
        }
        len -= subParamLen;
    }

    *msgType_ptr = (PDU_3gpp2TsMsgType)msgType;
    return (PDU_OK);
}

/* 
 * ======== PDU_3gpp2DecodeSms() ========
 * 
 * This function is to decode a 3GPP2 SMS PDU body which contains
 * Transport Layer Message.
 * 
 * Returns: 
 *   PDU_3GPP2_TS_MSG_TYPE_NONE: Failed to decode the PDU.
 *   Otherwise: Decode successfully.
 */
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
    int      *numOfMsg_ptr)
{
    PDU_Payload    pdu;
    char           tlMsgType;
    char           paramId;
    char           paramLen;
    char           done;
    char           byte1, byte2;
    uint16         tService = 0;

    UNUSED(tService);
    PDU_dbgPrintf("\n");

    _PDU_pduInitDecode(&pdu, payload_ptr, payloadLen);

    _PDU_pduReadByte(&pdu, &tlMsgType);
    PDU_dbgPrintf("msgType:0x%02X\n", tlMsgType);
    done = 0;
    *msgType_ptr = PDU_3GPP2_TS_MSG_TYPE_NONE;
    *replySeq_ptr = -1;
    *msgCoding_ptr = PDU_3GPP2_DATA_CODING_ASCII;
    while (!done) {
        /* Read parameter id */
        _PDU_pduReadByte(&pdu, &paramId);
        _PDU_pduReadByte(&pdu, &paramLen);
        PDU_dbgPrintf("parameter id:0x%02X len:%d\n",
                paramId, paramLen);

        switch (paramId) {
            case PDU_3GPP2_PARAM_ID_TELESERVICE:
                PDU_dbgPrintf("Teleservice id\n");
                _PDU_pduReadByte(&pdu, &byte1);
                _PDU_pduReadByte(&pdu, &byte2);
                tService = byte1 << 8 | byte2;
                PDU_dbgPrintf("Teleservice:0x%02X\n",
                        tService);
                break;
            case PDU_3GPP2_PARAM_ID_ORIG_ADDR:
                PDU_dbgPrintf("Originating address\n");
                _PDU_3gpp2SmsDecodeAddress(paramLen, &pdu, from_ptr, maxFromLen,
                    1);
                break;
            case PDU_3GPP2_PARAM_ID_DEST_ADDR:
                /* Destination address, don't case. */
                if (paramLen > _PDU_pduRemaining(&pdu)) {
                    PDU_dbgPrintf("Incorrect parameter data length\n");
                    return (PDU_ERR);
                }
                _PDU_pduAdvance(&pdu, paramLen);
                break;
            case PDU_3GPP2_PARAM_ID_REPLY:
                PDU_dbgPrintf("Bearer Reply Option\n");
                /* Reply Sequence */
                _PDU_pduReadByte(&pdu, &byte1);
                *replySeq_ptr = byte1 >> 2;
                break;
            case PDU_3GPP2_PARAM_ID_CAUSE_CODES:
                PDU_dbgPrintf("Cause Code\n");
                if (PDU_ERR == _PDU_3gpp2SmsDecodeCauseCode(
                        paramLen, &pdu, replySeq_ptr, (uint8 *)causeCode_ptr)) {
                    PDU_dbgPrintf("Cause code decode error\n");
                }
                break;
            case PDU_3GPP2_PARAM_ID_BEARER_DATA:
                PDU_dbgPrintf("Bearer Data\n");
                if (PDU_ERR == _PDU_3gpp2SmsDecodeBearerData(
                        paramLen, &pdu, msg_ptr, maxMsgLen, msgCoding_ptr,
                        (PDU_3gpp2TsMsgType *)msgType_ptr, msgId_ptr,
                        numOfMsg_ptr, from_ptr, maxFromLen)) {
                    PDU_dbgPrintf("Bearer Data decode error\n");
                }
                break;
            case (0x0d):
                /* Must be 0d0a in the end of message body, then we are done */
                done = 1;
                break;
            default:
                if (paramLen > _PDU_pduRemaining(&pdu)) {
                    PDU_dbgPrintf("Parameter data length is not correct\n");
                    return (PDU_ERR);
                }
                _PDU_pduAdvance(&pdu, paramLen);
                break;
        }
        if (0 >= _PDU_pduRemaining(&pdu)) {
            done = 1;
        }
    }
    if (tService == PDU_3GPP2_TELESERVICE_ID_VM || 
        tService == PDU_3GPP2_TELESERVICE_ID_PAGING) {
        *msgCoding_ptr = tService;
    }

    return (PDU_OK);
}


/* 
 * ======== PDU_3gpp2EncodeSubmit() ========
 * 
 * This function is to encode plain text to 3GPP2 sms submit transport
 * layer data.
 * 
 * Returns: 
 *    0: Failed to encode sms-submit
 *    Non-zero: Ecoded pdu length when encode sms-submit successfully.
 */
int PDU_3gpp2EncodeSubmit(
    char        *msg_ptr,
    char        *to_ptr,
    char        *smsc_ptr,
    uint16       msgRef,
    char        *target_ptr,
    int          maxTargetLen,
    OSAL_Boolean bearerReplyOption,
    char        *callBackNumber_ptr)
{
    PDU_Payload    pdu;
    unsigned char  pduBuf[PDU_BYTE_BUFFER_SIZE];
    char           byte;
    int            bytes;
    uint8         *bearerDataLen_ptr;
    uint8         *userDataLen_ptr;
    int            hasUnknownChars;
    int            septets;
    uint8          msgEncode;
    unsigned char  ucs2[PDU_BYTE_BUFFER_SIZE];
    uint8          lastByte;
    vint           i;

    _PDU_pduInitEncode(&pdu);

    /* Encode Tranport layer message */

    /* SMS_MSG_TYPE - SMS Point-to-Point '00000000' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_TL_MSG_TYPE_P2P);
    /* Parameter ID - Teleservice:'00000000' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_TELESERVICE);
    /* Parameter Length: 2 */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_LEN_TELESERVICE);
    /* Then Parameter Data */
    /*
     * Teleservice Identifier - CDMA Cellular Messaging Teleservice: 4098
     * XXX Need inter-op
     */ 
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_TELESERVICE_ID_CDMA_B0);
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_TELESERVICE_ID_CDMA_B1);

    /* Parameter ID - Destination Address: '00000100' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_DEST_ADDR);
    
    /* Encode destination address. */
    _PDU_3gpp2SmsAddressEncode(&pdu, to_ptr, 1);

    /* Include Bearer Reply Option if requested */
    if (bearerReplyOption) {
        /* Bearer Reply Option Id */
        _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_REPLY);
        /* Parameter length */
        _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_LEN_REPLY);
        /* REPLY_SEQ 6 bits and 2 bits reserved */
        _PDU_pduWriteByte(&pdu, msgRef << 2);
    }

    /* Paramter ID - Bearer Data: '00001000' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_BEARER_DATA);

    bearerDataLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    /* Encode Bearer Data */
    /* Bearer subparamter: Message identifier */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_SUBPARAM_MSG_ID);
    /* Len */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_SUBPARAM_LEN_MSG_ID);
    /* Msg Type - Submit */
    _PDU_pduWriteByte(&pdu, (PDU_3GPP2_MSG_TYPE_SUBMIT << 4) +
            ((msgRef >> 12) & 0xF));
    _PDU_pduWriteByte(&pdu, msgRef >> 4);
    _PDU_pduWriteByte(&pdu, msgRef << 4);

    /* User data subparameter id */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_SUBPARAM_USER_DATA);
    userDataLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    if (160 < OSAL_strlen(msg_ptr)) {
        /* XXX: No suport large SMS currently */
        msg_ptr[160] = 0;
    }

    /* If it can be encoded using ANSI then use that, otherwise use UCS-2 */
    hasUnknownChars = 0;
    bytes = ascii_to_7bitAsciiAnsi(msg_ptr, pduBuf, sizeof(pduBuf),
            &hasUnknownChars);
    PDU_dbgPrintf("7-bit ASCII msg:");
    _PDU_printBytes(pduBuf, bytes);
    PDU_dbgPrintf("\n");
    if (0 == hasUnknownChars) {
         PDU_dbgPrintf("Encoding the PDU as 7-bit ASCII\n");

        septets = OSAL_strlen(msg_ptr);

        /* Encoding + num fields */
        msgEncode = 2;
        byte = msgEncode << 3 | septets >> 5;
        _PDU_pduWriteByte(&pdu, byte);
        lastByte = septets << 3;

        for (i = 0; i < bytes; i++) {
            byte = lastByte | pduBuf[i] >> 5;
            _PDU_pduWriteByte(&pdu, byte);
            lastByte = pduBuf[i] << 3;
        }
        if ((septets % 8) - 5 < 0) {
            _PDU_pduWriteByte(&pdu, lastByte);
        }
    }
    else {
        PDU_dbgPrintf("Encoding as UNICODE UCS-2\n");
        bytes = OSAL_strlen(msg_ptr);
        /* Multiply by 2 since we are going to UCS2 */
        if ((bytes << 1) > _PDU_pduRemaining(&pdu)) {
            /* Not enough room */
            return (0);
        }

        /* Encode using big endian */
        bytes = utf8_to_utf16((uint16*)ucs2, msg_ptr, bytes, 0);
        /* The above routine returns the number of words, so let's make it bytes */
        PDU_dbgPrintf("UCS2 msg:");
        _PDU_printBytes((uint8 *)ucs2, bytes << 1);
        /* Encoding + num fields */
        msgEncode = 4;
        byte = msgEncode << 3 | bytes >> 5;
        _PDU_pduWriteByte(&pdu, byte);
        lastByte = bytes << 3;

        for (i = 0; i < (bytes << 1); i++) {
            byte = lastByte | ucs2[i] >> 5;
            _PDU_pduWriteByte(&pdu, byte);
            lastByte = ucs2[i] << 3;
        }
        _PDU_pduWriteByte(&pdu, lastByte);
    }

    *userDataLen_ptr =  pdu.cur_ptr - userDataLen_ptr - 1;
    *bearerDataLen_ptr = pdu.cur_ptr - bearerDataLen_ptr - 1;

    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (0);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    _PDU_printBytes((uint8 *)target_ptr, bytes);
    return (bytes);
}

/* 
 * ======== PDU_3gpp2EncodeSmsAck() ========
 * 
 * This function is to encode 3GPP2 SMS Acknowledge Message
 * transport layer data.
 * 
 * Returns: 
 *    0: Failed to encode SMS Acknowledgement Message
 *    Non-zero: Ecoded pdu length when encode SMS Acknowledgement Message
 *              successfully.
 */
int PDU_3gpp2EncodeSmsAck(
    char  *to_ptr,
    uint16 msgId,
    uint8  causeCode,
    char  *target_ptr,
    int    maxTargetLen)
{
    PDU_Payload    pdu;
    int            bytes;

    _PDU_pduInitEncode(&pdu);

    /* Encode Tranport layer message */

    /* SMS_MSG_TYPE - SMS ACK */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_TL_MSG_TYPE_ACK);

    /* Parameter ID - Destination Address: '00000100' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_DEST_ADDR);
    
    /* Encode destination address. */
    _PDU_3gpp2SmsAddressEncode(&pdu, to_ptr, 1);

    /* Encode Cause Codes. */
    /* Parameter ID - Destination Address: '00000100' */
    _PDU_pduWriteByte(&pdu, PDU_3GPP2_PARAM_ID_CAUSE_CODES);
    /* Parameter Length: 1 or 2(with error) */
    if (0 != causeCode) {
        _PDU_pduWriteByte(&pdu, 2);
        /* Msg Id and error class. XXX Assume error class is temporary. */
        _PDU_pduWriteByte(&pdu, (msgId << 2) | 2); 
        /* Cause code. */
        _PDU_pduWriteByte(&pdu, causeCode);
    }
    else {
        _PDU_pduWriteByte(&pdu, 1);
        /* Msg Id */
        _PDU_pduWriteByte(&pdu, msgId << 2); 
    }
    
    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (0);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    _PDU_printBytes((uint8 *)target_ptr, bytes);
    return (bytes);
}


