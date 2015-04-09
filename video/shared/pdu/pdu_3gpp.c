/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22547 $ $Date: 2013-10-23 11:17:36 +0800 (Wed, 23 Oct 2013) $
 */

#include <osal.h>
#include <pduconv.h>
#include <utf8_to_utf16.h>
#include "pdu_hlpr.h"
#include "_pdu_hlpr.h"
#include "pdu_3gpp.h"

/*
 * ======== PDU_3gppGetSubmitPduMessageAndAddress() ========
 * 
 * Private function to decode the submit pdu and get message and address
 *
 * Returns: 
 *    -1: Failed to decode the submit pud
 *     0: Get message and address successfully.
 */
int PDU_3gppGetSubmitPduMessageAndAddress(
    const char  *pduBuf_ptr,
    char        *address_ptr,
    char        *msg_ptr,
    int          msgLen)
{
    PDU_Payload pdu;
    char     protocolIdentifier = 0;
    char     dataCodingScheme = 0;
    char     toa = 0;
    char     len = 0;
    int      hasUserDataHeader;
    int      hasValidtyPeriod;
    char     firstByte = 0;
    char     validityPeriod;

    PDU_dbgPrintf("\n");

    _PDU_pduInitDecode(&pdu, (char *)pduBuf_ptr, 0);

    /* Skip Optional SMSC length */
    _PDU_pduAdvance(&pdu, 1);

    /* Skip First octet */
    _PDU_pduReadByte(&pdu, &firstByte);

    /* Skip TP-Message-Reference octet */
    _PDU_pduAdvance(&pdu, 1);

    // Let's parse the originating address

    // "The Address-Length field is an integer representation of
    // the number field, i.e. excludes any semi octet containing only
    // fill bits."
    _PDU_pduReadByte(&pdu, &len);

    /*
     * Read the TOA field, note that toa is not included in the length
     * that was read above
     */
    _PDU_pduReadByte(&pdu, &toa);

    if (-1 == _PDU_pduDecodeOriginatingAddress(&pdu, len, toa, address_ptr)) {
        PDU_dbgPrintf("error\n");
        return (-1);
    }

    PDU_dbgPrintf("The originating address is:%s\n",
            address_ptr);

    // TP-Protocol-Identifier (TP-PID) TS 23.040 9.2.3.9
    _PDU_pduReadByte(&pdu, &protocolIdentifier);

    // TP-Data-Coding-Scheme see TS 23.038
    _PDU_pduReadByte(&pdu, &dataCodingScheme);

    /* Get optional validity period if present */
    hasValidtyPeriod = (firstByte & 0x0C) == 0x0C;
    if (hasValidtyPeriod) {
        _PDU_pduReadByte(&pdu, &validityPeriod);
    }

    hasUserDataHeader = (firstByte & 0x40) == 0x40;

    _PDU_pduDecodeUserData(&pdu, hasUserDataHeader,
            dataCodingScheme, msg_ptr, msgLen);

    PDU_dbgPrintf("The message is:%s\n", msg_ptr);
    return 0;
}

/*
 * ======== PDU_3gppEncodeDeliverySms() ========
 * 
 * Private function to encode message to SMS-DELIVER TPDU format
 *
 * Returns: 
 *    -1: Failed to encode the SMS-DELIVER
 *    Otherwise: Encoded size of the SMS-DELIVER 
 */
int PDU_3gppEncodeDeliverySms(
    char                    *from_ptr,
    char                    *msg_ptr,
    char                    *target_ptr,
    int                      maxTargetLen,
    PDU_3gppDataCodingScheme pdu_dcs)
{
    int            septets;
    unsigned char  pduBuf[PDU_BYTE_BUFFER_SIZE];
    unsigned char *addrLen_ptr;
    int            addressLen;
    int            bytes;
    int            hasUnknownChars;
    /* Since this routine is typically used for SMS over IP,
     * then we don't have a "service center" persae.
     */
    char           *serviceCenter_ptr = "0000000000";
    OSAL_TimeLocal  tm;
    PDU_Payload     pdu;
    char            byte;

    _PDU_pduInitEncode(&pdu);

    /* Let's write the encoded service center address. */
    addressLen = OSAL_strlen(serviceCenter_ptr);

    /*
     * let's store where we want to write the length of the service center
     * address
     */
    addrLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    // Right the TOA_Unknown
    byte = TOA_Unknown;
    _PDU_pduWriteByte(&pdu, byte);

    // Now let's write the service center address in BCD format
    _PDU_pduWriteStringToBCD(&pdu, serviceCenter_ptr, addressLen);

    /*
     * Write the length in bytes of the encoded Destination address
     * and TOA byte.  Note the length for this address is totally different
     * than when the originating address is encoded.
     */
    *addrLen_ptr = (addressLen / 2) + 1;

    /* Write the MTI. */
    byte = 0x04;//TP_MTI_SMS_DELIVER;
    _PDU_pduWriteByte(&pdu, byte);

    /* Let's write the encoded originating address. */

    // let's store where we want to write the length of orig address
    addrLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    byte = TOA_Unknown;
    if (*from_ptr == '+') {
        byte = TOA_International;
        from_ptr++;
    }
    /* Write the TOA. */
    _PDU_pduWriteByte(&pdu, byte);

    // Now let's write the service center address in BCD format
    addressLen = _PDU_pduWriteStringToBCD(&pdu, from_ptr,
            OSAL_strlen(from_ptr));
    /*
     * Write the length... Destination address length in BCD digits,
     * ignoring TON byte and pad
     */
    *addrLen_ptr = addressLen;

    /* Write the 'protocol' ID which is '0' */
    byte = 0x00;
    _PDU_pduWriteByte(&pdu, byte);

    /* Write the data coding type. */
    switch(pdu_dcs) {
        case PDU_3GPP_DCS_VOICEMAIL_ON:
            byte = 0xc8;
            break;
        case PDU_3GPP_DCS_VOICEMAIL_OFF: 
            byte = 0xc0;
            break;
        case PDU_3GPP_DCS_DEFAULT:
        default:
            byte = 0x00;
            break;
    }
    _PDU_pduWriteByte(&pdu, byte);

    /*
     * Write the time. year, month, day, hour, minute, second, time zone.
     * Note the nibbles are swapped
     * the following is an example of 12.06.26 @ 12:59:10
     */
    if (OSAL_FAIL == OSAL_timeLocalTime(&tm)) {
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
        _PDU_pduWriteByte(&pdu, 0x00);
    }
    else {
        byte = _PDU_pduHexByteToBcd(tm.year - 100);
        _PDU_pduWriteByte(&pdu, byte); // year
        byte = _PDU_pduHexByteToBcd(tm.mon + 1);
        _PDU_pduWriteByte(&pdu, byte); // month
        byte = _PDU_pduHexByteToBcd(tm.mday);
        _PDU_pduWriteByte(&pdu, byte);  //day
        byte = _PDU_pduHexByteToBcd(tm.hour);
        _PDU_pduWriteByte(&pdu, byte);
        byte = _PDU_pduHexByteToBcd(tm.min);
        _PDU_pduWriteByte(&pdu, byte);
        byte = _PDU_pduHexByteToBcd(tm.sec);
        _PDU_pduWriteByte(&pdu, byte);
        /* time zone. */
        _PDU_pduWriteByte(&pdu, 0x0A);
    }


    /* If it can be encoded using GSM7 then use that, otherwise use UCS-2 */
    hasUnknownChars = 0;
    bytes = ascii_to_pdu(msg_ptr, pduBuf, sizeof(pduBuf), &hasUnknownChars);
    if (0 == hasUnknownChars) {
        PDU_dbgPrintf("Encoding the PDU as GSM7\n");

        septets = count_septets(msg_ptr);

        /* Write the length of the pdu encoded message */
        _PDU_pduWriteByte(&pdu, septets);

        if (-1 == _PDU_pduWriteBytes(&pdu, pduBuf, bytes)) {
            /* No room */
            return (-1);
        }
    }
    else {
        PDU_dbgPrintf("Encoding the PDU as UCS-2, GSM7 was a no go\n");

        /* Save the location of where to write the length of the payload */
        addrLen_ptr = pdu.cur_ptr;
        _PDU_pduAdvance(&pdu, 1);

        bytes = OSAL_strlen(msg_ptr);
        /* Multiply by 2 since we are going to UCS2 */
        if ((bytes << 1) > _PDU_pduRemaining(&pdu)) {
            /* Not enough room */
            return (-1);
        }

        /* Encode using big endian */
        bytes = utf8_to_utf16((unsigned short*)pdu.cur_ptr, msg_ptr, bytes, 1);
        /*
         * The above routine returns the number of words,
         * so let's make it bytes
         */
        bytes <<= 1;
        /* Write the length of the pdu encoded message */
        *addrLen_ptr = bytes;
        _PDU_pduAdvance(&pdu, bytes);
    }

    bytes = _PDU_pduGetLength(&pdu);

    /*
     * Multiply bytes by 2 since we are about to turn the hex values into
     * hex string
     */
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (-1);
    }

    bytes = PDU_pduBytesToHexString(pdu.start_ptr, bytes, target_ptr);

    _PDU_printAllBytes(target_ptr);
    return (bytes);
}

/*
 * ======== _PDU_3gppDecodeTpdu() ========
 * 
 * Private function to decode tpdu message body.
 *
 * Returns: 
 *    -1: Failed to decode the tpdu
 *    
 */
static int _PDU_3gppDecodeTpdu(
    char        rpMti,
    PDU_Payload *pdu_ptr,
    char        *errCode_ptr,
    char        *tpMti_ptr)
{
    char     mti;
    char     firstByte;

    /* Read TP MTI */
    firstByte = 0; /* Just for eliminate compile warning */
    _PDU_pduReadByte(pdu_ptr, &firstByte);

    mti = firstByte & 0x03;
    switch (mti) {
        case 0:
            /* SMS-DELIVER */
            *tpMti_ptr = PDU_3GPP_TP_MTI_SMS_DELIVER;
            break;
        case 1:
            /* SMS-SUBMIT-REPORT */
            if (PDU_3GPP_MTI_RP_ERROR_SC_TO_MS == rpMti) {
                /* Read failure cause */
                _PDU_pduReadByte(pdu_ptr, errCode_ptr);
            }
            *tpMti_ptr = PDU_3GPP_TP_MTI_SMS_SUBMIT_REPORT;
            break;
        case 2:
            /* SMS-STATUS-REPORT */
            *tpMti_ptr = PDU_3GPP_TP_MTI_SMS_STATUS_REPORT;
            break;
        default:
            /* Nothing here! let's return an error */
            return (-1);
    }

    return (0);
}

/* 
 * ======== _PDU_decodeRpData() ========
 * 
 * Private function to decode RP-DATA message.
 * 
 * Returns: 
 *    -1: Failed to decode
 *    Otherwise: Decode successully
 */
static int _PDU_decodeRpData(
    PDU_Payload *pdu_ptr,
    char        *target_ptr,
    int          maxTargetLen,
    char         rpMti,
    char        *smsc_ptr,
    int          maxSmscLen,
    char        *msgRef_ptr,
    char        *errCode_ptr,
    char        *tpMti_ptr)
{
    unsigned char  byte;
    char  toa;
    char  len;
    unsigned char *tpdu_ptr;

    /* Message Reference */
    _PDU_pduReadByte(pdu_ptr, msgRef_ptr);

    /* Copy smsc address len to target */
    byte = PDU_pduBytesToHexString(pdu_ptr->cur_ptr, 1, target_ptr);
    target_ptr += byte;

    /* Read RP-Origination Address */
    len = 0; /* Just for eliminate compile warning */
    _PDU_pduReadByte(pdu_ptr, &len);

    /* Copy smsc address to target */
    byte = PDU_pduBytesToHexString(pdu_ptr->cur_ptr, len, target_ptr);
    target_ptr += byte;

    /*
     * Read the TOA field, note that toa is not included in the length
     * that was read above
     */
    toa = 0; /* Just for eliminate compile warning */
    _PDU_pduReadByte(pdu_ptr, &toa);

    /* _PDU_pduDecodeOriginatingAddress takes address number length */
    len = (len - 1) * 2;
    if (-1 == _PDU_pduDecodeOriginatingAddress(pdu_ptr, len, toa, smsc_ptr)) {
        return (-1);
    }

    /* Read RP-Destination Address */
    _PDU_pduReadByte(pdu_ptr, (char *)&byte);

    /* Skip the address */
    _PDU_pduAdvance(pdu_ptr, byte);

    /* Read RP-User Data length */
     _PDU_pduReadByte(pdu_ptr, (char *)&byte);

    /* Keep start address of tpdu */
    tpdu_ptr = pdu_ptr->cur_ptr;

    if (-1 == _PDU_3gppDecodeTpdu(rpMti, pdu_ptr, errCode_ptr, tpMti_ptr)) {
        return (-1);
    }

    if (maxTargetLen < byte) {
        return (-1);
    }

    /* Convert TPDU bytes to hex-string */
    PDU_pduBytesToHexString(tpdu_ptr, byte, target_ptr);
    
    _PDU_printAllBytes(target_ptr);

    return (0);
}

/* 
 * ======== _PDU_decodeRpAck() ========
 * 
 * Private function to decode RP-ACK message.
 * 
 * Returns: 
 *    -1: Failed to decode
 *    Otherwise: Decode successully
 */
static int _PDU_decodeRpAck(
    PDU_Payload *pdu_ptr,
    char         rpMti,
    char        *msgRef_ptr,
    char        *errCode_ptr,
    char        *tpMti_ptr)
{
    char byte;

    /* Message Reference */
    _PDU_pduReadByte(pdu_ptr, msgRef_ptr);

    /* User Data IEI */
    if (-1 == _PDU_pduReadByte(pdu_ptr, &byte)) {
        /* No more data to read */
        return (0);
    }

    /* Must be 0x41 */
    if (0x41 != byte) {
        return (-1);
    }
    /* Read RP-User Data length */
     _PDU_pduReadByte(pdu_ptr, &byte);

    return (_PDU_3gppDecodeTpdu(rpMti, pdu_ptr, errCode_ptr, tpMti_ptr));
}

/* 
 * ======== _PDU_3gppDecodeRpError() ========
 * 
 * Private function to decode RP-ERROR message.
 * 
 * Returns: 
 *    -1: Failed to decode
 *    Otherwise: Decode successully
 */
static int _PDU_3gppDecodeRpError(
    PDU_Payload *pdu_ptr,
    char         rpMti,
    char        *msgRef_ptr,
    char        *errCode_ptr,
    char        *tpMti_ptr)
{
    /* Message Reference */
    _PDU_pduReadByte(pdu_ptr, msgRef_ptr);

    /* Cause */
    _PDU_pduReadByte(pdu_ptr, errCode_ptr);

    /* Must be SMS-SUBMIT-REPORT */
    *tpMti_ptr = PDU_3GPP_TP_MTI_SMS_SUBMIT_REPORT;

    return (0);
}

/* 
 * ======== _PDU_3gppEncodeRpAck() ========
 * 
 * Private function to encode RP-ACK with a message reference id and error
 * code.
 * 
 * Returns: 
 *    -1: Failed to encode
 *    Otherwise: Encoded pdu length
 */
static int _PDU_3gppEncodeRpAck(
    int          msgRef,
    char        *target_ptr,
    int          maxTargetLen,
    PDU_Payload *rpDataPdu_ptr)
{
    int            bytes;

    PDU_Payload    pdu;
    char           byte;

    _PDU_pduInitEncode(&pdu);

    /* MTI */
    byte = PDU_3GPP_MTI_RP_ACK_MS_TO_SC;
    _PDU_pduWriteByte(&pdu, byte);

    byte = msgRef;
    /* Message reference */
    _PDU_pduWriteByte(&pdu, byte);

    /* RP-User Data IEI */
    byte = 0x41;
    _PDU_pduWriteByte(&pdu, byte);

    /* User data length */
    byte = _PDU_pduGetLength(rpDataPdu_ptr);
    _PDU_pduWriteByte(&pdu, byte);
    /* Write TPDU */
    if (-1 == _PDU_pduWriteBytes(&pdu, rpDataPdu_ptr->start_ptr, byte)) {
        /* No free pdu space */
        return (-1);
    }

    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (-1);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    return (bytes);
}

/* 
 * ======== _PDU_3gppEncodceRpError() ========
 * 
 * Private function to encode RP-ERROR with a message reference id and error
 * code.
 * 
 * Returns: 
 *    -1: Failed to encode
 *    Otherwise: Encoded pdu length
 */
static int _PDU_3gppEncodceRpError(
    int          msgRef,
    int          errCode, 
    char        *target_ptr,
    int          maxTargetLen,
    PDU_Payload *rpDataPdu_ptr)
{
    int            bytes;
    PDU_Payload    pdu;
    char           byte;

    _PDU_pduInitEncode(&pdu);

    /* MTI */
    byte = PDU_3GPP_MTI_RP_ERROR_MS_TO_SC;
    _PDU_pduWriteByte(&pdu, byte);

    byte = msgRef;
    /* Message reference */
    _PDU_pduWriteByte(&pdu, byte);

    /* RP-Cause Data IEI */
    byte = 0x42;
    _PDU_pduWriteByte(&pdu, byte);

    /* RP-Cause length */
    byte = 0x01;
    _PDU_pduWriteByte(&pdu, byte);

    /* RP-Cause */
    byte = errCode;
    _PDU_pduWriteByte(&pdu, byte);
    /* User data length */
    byte = _PDU_pduGetLength(rpDataPdu_ptr);
    _PDU_pduWriteByte(&pdu, byte);
    /* Write TPDU */
    if (-1 == _PDU_pduWriteBytes(&pdu, rpDataPdu_ptr->start_ptr, byte)) {
        /* No free pdu space */
        return (-1);
    }

    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (-1);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    return (bytes);
}

/* 
 * ======== PDU_3gppDecodeSms() ========
 * 
 * This function is to decode a SMS PDU body which contains the RP message.
 * 
 * Parameters:
 *   pud_ptr: Pointer to a char that contains the pdu body.
 *   pduLen : Length of pdu body.
 *   target_ptr: A char pointer to target pdu.
 *   maxTargetLen: The maximum length of the target pdu.
 *   smsc_ptr: A pointer to a char which the decoded SMSC number will be
 *             written to if pdu decoded successfully.
 *   tpMti_ptr: Pointer to a PDU_3GPP_TP_MTI. The SMS type indication 
 *             will be written to this variable if decoded successfully.
 *   msgRef_ptr: Pointer to a int which will be written to the MR(message 
 *            reference) if pdu decoded successfully and contains MR.
 *   errCode_ptr: Pointer to a int which the error code will be written to if
 *            pdu decoded successfully and contains error code.
 *
 * Returns: 
 *   -1: Failed to decode the PDU.
 *    0: Decode successfully.
 */
int PDU_3gppDecodeSms(
    char        *pdu_ptr,
    int          pduLen,
    char        *target_ptr,
    int          maxTargetLen,
    char        *smsc_ptr,
    int          maxSmscLen,
    char        *type_ptr,
    char        *msgRef_ptr,
    char        *errCode_ptr)
{
    PDU_Payload    pdu;
    char           rpMti;

    if (0 == pdu_ptr || 0 == smsc_ptr || 0 == type_ptr || 
            0 == msgRef_ptr) {
        return (-1);
    }

    _PDU_pduInitDecode(&pdu, pdu_ptr, pduLen);

    /* Read MTI */
    rpMti = 0; /* Just for eliminate compile warning */
    _PDU_pduReadByte(&pdu, &rpMti);

    if ((PDU_3GPP_MTI_RP_DATA_MS_TO_SC == rpMti) ||
            (PDU_3GPP_MTI_RP_DATA_SC_TO_MS == rpMti)) {
        /* RP-DATA */
        if (-1 == _PDU_decodeRpData(&pdu, target_ptr, maxTargetLen,
                rpMti, smsc_ptr, maxSmscLen,
                msgRef_ptr, errCode_ptr, type_ptr)) {
            /* Failed to decode RP data */
            return (-1);
        }
    }
    else if ((PDU_3GPP_MTI_RP_ERROR_MS_TO_SC == rpMti) ||
            (PDU_3GPP_MTI_RP_ERROR_SC_TO_MS == rpMti)) {
        /* RP-ERROR */
        if (-1 == _PDU_3gppDecodeRpError(&pdu, rpMti, 
                msgRef_ptr, errCode_ptr, type_ptr)) {
            /* Failed to decode RP error */
            return (-1);
        }
    }
    else if ((PDU_3GPP_MTI_RP_ACK_MS_TO_SC == rpMti) ||
            (PDU_3GPP_MTI_RP_ACK_SC_TO_MS == rpMti)) {
        /* RP-ACK */
        if (-1 == _PDU_decodeRpAck(&pdu, rpMti,
                msgRef_ptr, errCode_ptr, type_ptr)) {
            /* Failed to decode RP ack */
            return (-1);
        }
    }

    return (0);
}

/* 
 * ======== PDU_3gppEncodeDeliverReport() ========
 * 
 * This function is to encode a deliver report to pdu
 * 
 * Parameters:
 *   smsc_ptr: A pointer to a char which contains the SMSC number.
 *   msgRef:   Message reference which will be encoded to the deliver report
 *   errCode: Error code which will be encoded to deliver report if it's 
 *            non-zero.
 *   target_ptr: A char pointer to target pdu.
 *   maxTargetLen: The maximum length of the target pdu.
 *
 * Returns: 
 *    0: Failed to encode deliver report
 *    Non-zero: Ecoded pdu length when encode deliver report successfully.
 */
int PDU_3gppEncodeDeliverReport(
    char *smsc_ptr,
    char  msgRef,
    char  errCode,
    char *target_ptr,
    int   maxTargetLen)
{
    PDU_Payload    pdu;
    char           byte;
    int            bytes;

    _PDU_pduInitEncode(&pdu);

    /* MTI = SMS-DELIVER-REPORT */
    byte = 0x00;
    _PDU_pduWriteByte(&pdu, byte);

    /* TP-Parameter-Indicator */
    byte = 0x00;
    _PDU_pduWriteByte(&pdu, byte);

    if (0 == errCode) {
        /* Encode to RP-ACK */
        bytes = _PDU_3gppEncodeRpAck(
                msgRef, target_ptr, maxTargetLen, &pdu);
    }
    else {
        /* Encode to RP-ERROR */
        bytes = _PDU_3gppEncodceRpError(
                msgRef, errCode, target_ptr, maxTargetLen, &pdu);
    }

    _PDU_printBytes((unsigned char *)target_ptr, bytes);

    return (bytes);
}

/* 
 * ======== PDU_3gppEncodeSubmit() ========
 * 
 * This function is to encode sms submit tpdu to RP message
 * 
 * Parameters:
 *   tpdu_ptr: A pointer to a char which contains the the TPDU
 *            excluding SMSC address.
 *   tpduLen:  The length of tpdu body.
 *   smsc_ptr: A char pointer which contains the number of smsc
 *   msgRef:   Message reference for this SMS.
 *   target_ptr: A char pointer to target pdu.
 *   maxTargetLen: The maximum length of the target pdu.
 *   encodedLen: The encoded pdu length
 *
 * Returns: 
 *    0: Failed to encode sms-submit
 *    Non-zero: Ecoded pdu length when encode sms-submit successfully.
 */
int PDU_3gppEncodeSubmit(
    char         *tpdu_ptr,
    int           tpduLen,
    char         *smsc_ptr,
    char          msgRef,
    char         *target_ptr,
    int           maxTargetLen)
{
    PDU_Payload    pdu;
    char           byte;
    int            hasPlus;
    char          *address_ptr;
    unsigned char *addrLen_ptr;
    int            addressLen;
    int            bytes;
    int            smscAddrLen;
    char          *pduToEncode_ptr;
    int            pduLenToEncode;

    _PDU_pduInitEncode(&pdu);

    _PDU_printBytes((unsigned char *)tpdu_ptr, tpduLen);

    /* Check if there is a plus */
    if (0 != (address_ptr = strchr(smsc_ptr, '+'))) {
        hasPlus = 1;
        address_ptr++;
    }
    else {
        address_ptr = smsc_ptr;
        hasPlus = 0;
    }

    /* Advance smsc in tdpu */
    smscAddrLen = tpdu_ptr[0];
    pduToEncode_ptr = tpdu_ptr + smscAddrLen + 1;
    pduLenToEncode = tpduLen - smscAddrLen - 1;
    addressLen = OSAL_strlen(address_ptr);

    /* Write MTI for RP-DATA */
    byte = PDU_3GPP_MTI_RP_DATA_MS_TO_SC;
    _PDU_pduWriteByte(&pdu, byte);

    /* Message reference */
    _PDU_pduWriteByte(&pdu, msgRef);

    /* Originator address, 0 for MS side */
    byte = 0;
    _PDU_pduWriteByte(&pdu, byte);

    /* Destinator address element, smsc address */

    /* Store where we want to write the length of the destination address */
    addrLen_ptr = pdu.cur_ptr;
    _PDU_pduAdvance(&pdu, 1);

    /* Now write the toa */
    if (1 == hasPlus) {
        _PDU_pduWriteByte(&pdu, TOA_International);
    }
    else {
        _PDU_pduWriteByte(&pdu, TOA_Unknown);
    }

    /* Now let's write the destination address in BCD format */
    byte = _PDU_pduWriteStringToBCD(&pdu, address_ptr, addressLen);

    /* 
     * Write the length... Destination address length in BCD digits, 
     * including TOA byte. Get actual wrote length.
     */
    *addrLen_ptr = pdu.cur_ptr - addrLen_ptr - 1;

    /* User data length */
    byte = (char)pduLenToEncode;
    _PDU_pduWriteByte(&pdu, byte);

    /* Write TPDU */
    if (-1 == _PDU_pduWriteBytes(&pdu, (unsigned char *)pduToEncode_ptr,
            pduLenToEncode)) {
        /* No free pdu space */
        return (-1);
    }

    /* Write to target */
    bytes = _PDU_pduGetLength(&pdu);
    if ((bytes << 1) > maxTargetLen) {
        /* Can't encode this */
        return (-1);
    }

    OSAL_memCpy(target_ptr, pdu.start_ptr, bytes);

    _PDU_printBytes((unsigned char *)target_ptr, bytes);
    return (bytes);
}
/* 
 * ======== PDU_3gppEncodeSubmitTPDU() ========
 * 
 * This function is to encode data to a TPDU
 * 
 * Parameters:
 *   msgRef:   Message reference for this SMS.
 *   dstAddress: A char pointer which contains the number of destination address
 *   userData:   User data
 *   outPtr: A char pointer to out buffer
 *   maxOutLen: The maximum length of out buffer.
 *
 * Returns: 
 *    0: Failed to encode sms-submit TPDU
 *    Non-zero: Ecoded pdu length when encode sms-submit successfully.
 */

int PDU_3gppEncodeSubmitTPDU(
    char          msgRef,
    char         *dstAddress,
    char         *userData,
    char         *outPtr,
    int           maxOutLen)
{
    PDU_Payload    pdu;
    int            septets;
    unsigned char  pduBuf[PDU_BYTE_BUFFER_SIZE];
    char           byte;
    int            bytes;
    int            hasUnknownChars;
    unsigned char *addrLen_ptr;

    
    _PDU_pduInitEncode(&pdu);

    /* No SMSC*/
    _PDU_pduWriteByte(&pdu, 0);
    
    /* Write the Message Type Indicator. */
    byte = TP_MTI_SMS_SUBMIT;
    _PDU_pduWriteByte(&pdu, byte);

    /* Write the Message Reference. */
    _PDU_pduWriteByte(&pdu, msgRef);

    /* Write the Length of Destination Address. */
    _PDU_pduWriteByte(&pdu, (char) OSAL_strlen(dstAddress));

    /* Write the Type of Destination Address. */
    if (0 != strchr(dstAddress, '+')) {
        _PDU_pduWriteByte(&pdu, TOA_International);
    }
    else {
        _PDU_pduWriteByte(&pdu, TOA_Unknown);
    }

    /* Write the value of Destination Address. */
    _PDU_pduWriteStringToBCD(&pdu, dstAddress, OSAL_strlen(dstAddress));

    /* Write Protocol Identifier. */
    _PDU_pduWriteByte(&pdu, 0);

    /* If it can be encoded using GSM7 then use that, otherwise use UCS-2 */
    hasUnknownChars = 0;
    bytes = ascii_to_pdu(userData, pduBuf, sizeof(pduBuf), &hasUnknownChars);
    if (0 == hasUnknownChars) {
        PDU_dbgPrintf("Encoding the PDU as GSM7\n");
        
        /* Write Data coding scheme. GSM 7 bit default alphabet*/
        _PDU_pduWriteByte(&pdu, 0);

        septets = count_septets(userData);

        /* Write the length of the pdu encoded message */
        _PDU_pduWriteByte(&pdu, septets);

        if (-1 == _PDU_pduWriteBytes(&pdu, pduBuf, bytes)) {
            /* No room */
            return (-1);
        }
    }
    else {
        PDU_dbgPrintf("Encoding the PDU as UCS-2, GSM7 was a no go\n");
        /* Write Data coding scheme. UCS2 format*/
        _PDU_pduWriteByte(&pdu, 8);
        
        /* Save the location of where to write the length of the payload */
        addrLen_ptr = pdu.cur_ptr;
        _PDU_pduAdvance(&pdu, 1);

        bytes = OSAL_strlen(userData);
        /* Multiply by 2 since we are going to UCS2 */
        if ((bytes << 1) > _PDU_pduRemaining(&pdu)) {
            /* Not enough room */
            return (-1);
        }

        /* Encode using big endian */
        bytes = utf8_to_utf16((unsigned short*)pdu.cur_ptr, userData, bytes, 1);
        /*
         * The above routine returns the number of words,
         * so let's make it bytes
         */
        bytes <<= 1;
        /* Write the length of the pdu encoded message */
        *addrLen_ptr = bytes;
        _PDU_pduAdvance(&pdu, bytes);
    }

    bytes = _PDU_pduGetLength(&pdu);

    /*
     * Multiply bytes by 2 since we are about to turn the hex values into
     * hex string
     */
    if ((bytes << 1) > maxOutLen) {
        /* Can't encode this */
        return (-1);
    }

    bytes = PDU_pduBytesToHexString(pdu.start_ptr, bytes, outPtr);

    _PDU_printAllBytes(outPtr);
    return bytes;
}

