/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22553 $ $Date: 2013-10-23 15:11:27 +0800 (Wed, 23 Oct 2013) $
 */

#include <osal.h>
#include <pduconv.h>
#include <utf8_to_utf16.h>
#include "_pdu_hlpr.h"

/* 
 * ======== _PDU_pduHexCharToInt() ========
 * 
 * Private function to convert hex character to int
 *
 * Returns: 
 *    0: Failed to convert.
 *    Non-zero: converted int.
 */
int _PDU_pduHexCharToInt(
    char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }
    return 0;
}

#ifdef PDU_DEBUG
/* 
 * ======== _PDU_printAllBytes() ========
 * 
 * Private function to print all bytes of the input string
 *
 * Returns: 
 *    None
 */
void _PDU_printAllBytes(
    char *s_ptr)
{
    int len = OSAL_strlen(s_ptr);
    int x;
    for (x = 0 ; x < len ; x += 2) {
        OSAL_logMsg("%c%c ", s_ptr[x], s_ptr[x+1]);
    }
}

/* 
 * ======== _PDU_printBytes() ========
 * 
 * Private function to print the input string with length specified
 *
 * Returns: 
 *    None
 */
void _PDU_printBytes(
    unsigned char *s_ptr,
    int len)
{
    int x;
    for (x = 0 ; x < len ; x++) {
        OSAL_logMsg("%02X ", s_ptr[x]);
    }
    OSAL_logMsg("\n");
}
#endif

/* 
 * ======== _PDU_pduInitUserData() ========
 * 
 * Private function to initialize PDU_UserData
 *
 * Returns: 
 *    None
 */
void _PDU_pduInitUserData(
    PDU_UserData *userdata_ptr)
{
    userdata_ptr->dataLen       = 0;
    userdata_ptr->dataSeptets   = 0;
    userdata_ptr->data_ptr      = 0;
    userdata_ptr->hdrLen        = 0;
    userdata_ptr->hdrSeptets    = 0;
    userdata_ptr->hdr_ptr       = 0;
}

/* 
 * ======== _PDU_pduInitDecode() ========
 * 
 * Private function to initialize the PDU_Payload for decode
 *
 * Returns: 
 *    None
 */
void _PDU_pduInitDecode(
    PDU_Payload   *pdu_ptr,
    char          *data_ptr,
    int            dataLen)
{
    if (0 == dataLen) {
        /* Data length eq zero for hex string */
        pdu_ptr->length = PDU_pduHexStringToBytes(data_ptr, pdu_ptr->pduBytes);
    }
    else {
        pdu_ptr->length = dataLen;
        OSAL_memCpy(pdu_ptr->pduBytes, data_ptr, dataLen);
    }
    pdu_ptr->cur_ptr = pdu_ptr->start_ptr = pdu_ptr->pduBytes;

    _PDU_printBytes(pdu_ptr->cur_ptr, pdu_ptr->length);
    PDU_dbgPrintf("len=%d", pdu_ptr->length);
}

/*
 * ======== _PDU_pduInitEncode() ========
 * 
 * Private function to initialize the PDU_Payload for encode
 *
 * Returns: 
 *    None
 */
void _PDU_pduInitEncode(
    PDU_Payload   *pdu_ptr)
{
    pdu_ptr->length = PDU_BYTE_BUFFER_SIZE;
    pdu_ptr->cur_ptr = pdu_ptr->start_ptr = pdu_ptr->pduBytes;
}

/*
 * ======== _PDU_pduCheckLength() ========
 * 
 * Private function to check if there is enough size in the input PDU_Payload
 *
 * Returns: 
 *    -1: The size exceeds the remaining length
 *     0: Remaining length is enough
 */
int _PDU_pduCheckLength(
    PDU_Payload *pdu_ptr,
    int          size)
{
    int remaining;
    // Get the remaining length
    remaining = (pdu_ptr->cur_ptr - pdu_ptr->start_ptr);
    remaining = pdu_ptr->length - remaining;
    if (remaining < size) {
        return (-1);
    }
    // There's enough left to read 'size' number of bytes 
    return (0);
}

/*
 * ======== _PDU_pduReadByte() ========
 * 
 * Private function to read a byte from the current position of the PDU_Payload
 *
 * Returns: 
 *    -1: Failed to read.
 *     1: Read successfully.
 */
int _PDU_pduReadByte(
    PDU_Payload *pdu_ptr,
    char        *byte_ptr)
{
    if (0 == _PDU_pduCheckLength(pdu_ptr, 1)) {
        *byte_ptr = *(pdu_ptr->cur_ptr);
        pdu_ptr->cur_ptr++;
        return (1);
    }
    return (-1);
}

/*
 * ======== _PDU_pduReadByte() ========
 * 
 * Private function to write a byte to the current position of the PDU_Payload
 *
 * Returns: 
 *    -1: Failed to write
 *     1: Write successfully.
 */
int _PDU_pduWriteByte(
    PDU_Payload *pdu_ptr,
    char         byte)
{
    if (0 == _PDU_pduCheckLength(pdu_ptr, 1)) {
        *pdu_ptr->cur_ptr = byte;
        pdu_ptr->cur_ptr++;
        return (1);
    }
    return (-1);
}

/*
 * ======== _PDU_pduWriteBytes() ========
 * 
 * Private function to write length of bytes to the current position of the
 * PDU_Payload.
 *
 * Returns: 
 *    -1: Failed to write
 *     1: Write successfully.
 */
int _PDU_pduWriteBytes(
    PDU_Payload  *pdu_ptr,
    unsigned char bytes[],
    int           size)
{
    int i;
    if (0 != _PDU_pduCheckLength(pdu_ptr, size)) {
        return (-1);
    }
    // Loop and write all the bytes
    for (i = 0 ; i < size ; i++) {
        _PDU_pduWriteByte(pdu_ptr, (char)bytes[i]);
    }
    return (1);
}

/*
 * ======== _PDU_pduRemaining() ========
 * 
 * Private function to get the remaining size of the PDU_Payload.
 * PDU_Payload.
 *
 * Returns: 
 *    -1: No free space in the PDU_Payload.
 *     Otherwise: The remaining length of PDU_Payload.
 */
int _PDU_pduRemaining(
    PDU_Payload *pdu_ptr)
{
     int remaining;
    // Get the remaining length
    remaining = (pdu_ptr->cur_ptr - pdu_ptr->start_ptr);
    remaining = pdu_ptr->length - remaining;
    if (remaining <= 0) {
        return (-1);
    }
    // There's enough left to read 'size' number of bytes 
    return (remaining);
}

/*
 * ======== _PDU_pduGetLength() ========
 * 
 * Private function to get the current data lenght of the PDU_Payload.
 * PDU_Payload.
 *
 * Returns: 
 *    Current data length in the PDU_Payload.
 */
int _PDU_pduGetLength(
    PDU_Payload *pdu_ptr)
{
    return (pdu_ptr->cur_ptr - pdu_ptr->start_ptr);
}

/*
 * ======== _PDU_pduHexByteToBcd() ========
 * 
 * Private function to convert hex byte to BCD
 *
 * Returns: 
 *    Converted BCD character.
 */
char _PDU_pduHexByteToBcd(
    int b)
{
    char firstDigit = 0;
    char secondDigit = 0;
    /* Only values less than 99 will be processed. */
    if (b >= 100) {
        return 0;
    }
    firstDigit = b / 10;
    secondDigit = b % 10;
    /* Swap the nibbles. for example so a '12' will be '21' */
    secondDigit <<= 4;
    return (secondDigit | firstDigit);
}

/*
 * ======== _PDU_pduBcdToChar() ========
 * 
 * Private function to convert BCD to character
 *
 * Returns: 
 *    0: Invalid input BCD
 *    Otherwise: Converted character
 */
char _PDU_pduBcdToChar(
    char b) {
    if (b < 0xa) {
        return (char)('0' + b);
    }
    
    switch (b) {
        case 0xa: return '*';
        case 0xb: return '#';
        case 0xc: return PAUSE;
        case 0xd: return WILD;
        default: return 0;
    }
}

/*
 * ======== _PDU_pduReadBCDToString() ========
 * 
 * Private function to read BCD from PDU_Payload and convert to string
 *
 * Returns: 
 *    Converted length
 */
int _PDU_pduReadBCDToString(
    PDU_Payload *pdu_ptr,
    int          len,
    char        *out_ptr)
{
    char  byte = 0;
    int   i;
    char *cur_ptr = out_ptr;
    
    if (0 != _PDU_pduCheckLength(pdu_ptr, len)) {
        // Then the pdu payload isn't big enough for this
        return (-1);
    }
        
    for (i = 0 ; i < len ; i++) {
        char b;
        char c;
        
        _PDU_pduReadByte(pdu_ptr, &byte);

        c = _PDU_pduBcdToChar(byte & 0xf);

        if (c == 0) {
            break;
        }
        *cur_ptr = c;
        cur_ptr++;

        // TS 23.040 9.1.2.3 says
        // "if a mobile receives 1111 in a position prior to
        // the last semi-octet then processing shall commense with
        // the next semi-octet and the intervening
        // semi-octet shall be ignored"

        b = (byte >> 4) & 0xf;

        if (b == 0xf && i + 1 == len) {
            //ignore final 0xf
            break;
        }

        c = _PDU_pduBcdToChar(b);
        if (c == 0) {
            break;
        }
        *cur_ptr = c;
        cur_ptr++;
    }
    // NULL terminate the output
    *cur_ptr = 0;
    /* Return the numner of bytes read */
    return (i);
}
 
/*
 * ======== _PDU_pduAdvance() ========
 * 
 * Private function to advance size of byte of PDU_Payload
 *
 * Returns: 
 *    -1: Not enough space to advance
 *    Otherwise: Advanced size.
 */
int _PDU_pduAdvance(
    PDU_Payload *pdu_ptr,
    int          size)
{
    if (0 == _PDU_pduCheckLength(pdu_ptr, size)) {
        pdu_ptr->cur_ptr += size;
        return (size);
    }
    return (-1);
}

/*
 * ======== _PDU_pduCharToBCD() ========
 * 
 * Private function to convert character to BCD
 *
 * Returns: 
 *    -1: Invalid character
 *    Otherwise: Converted BCD
 */
int _PDU_pduCharToBCD(
    char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c == '*') {
        return 0xa;
    }
    else if (c == '#') {
        return 0xb;
    }
    else if (c == PAUSE) {
        return 0xc;
    }
    else if (c == WILD) {
        return 0xd;
    }
    else {
        return -1;
    }
}

/*
 * ======== _PDU_pduWriteStringToBCD() ========
 * 
 * Private function to convert string to BCD and write to PDU_Payload
 *
 * Returns: 
 *    -1: Failed to convert or write to PDU_Payload
 *    Otherwise: Written BCD digits
 */
int _PDU_pduWriteStringToBCD(
    PDU_Payload *pdu_ptr,
    char        *address_ptr,
    int          addressLen)
{
    char a, b;    
    int countFullBytes;
    int i;
    int curChar;
    int countBcdDigits = 0;
    
    countFullBytes = (addressLen / 2);
    
    // Let's make sure there's enough room
    if (0 != _PDU_pduCheckLength(pdu_ptr, countFullBytes + 1)) {
        // Then the pdu payload isn't big enough for this
        return (-1);
    }
    
    curChar = 0;
    for (i = 0; i < countFullBytes; i++) {
        a = _PDU_pduCharToBCD(address_ptr[curChar++]);
        b =  _PDU_pduCharToBCD(address_ptr[curChar++]) << 4;
        _PDU_pduWriteByte(pdu_ptr, (a | b));
        countBcdDigits += 2;
    }

    // The left-over octet for odd-length phone numbers should be
    // filled with 0xf.
    if (addressLen & 1) {
        a = _PDU_pduCharToBCD(address_ptr[curChar++]);
        b = (0xf << 4);
         _PDU_pduWriteByte(pdu_ptr, (a | b));
         countBcdDigits++;
    }
    return (countBcdDigits);
}

/*
 * ======== _PDU_pduDecodeAddress() ========
 * 
 * Private function to decode address from BCD format to string
 *
 * Returns: 
 *    Decoded address string length.
 */
int _PDU_pduDecodeAddress(
    PDU_Payload *pdu_ptr,
    int          len,
    char         toa,
    char        *out_ptr)
{
    if ((toa & 0xff) == TOA_International) {
        *out_ptr = '+';
        out_ptr++;
    }
    // Decode Service Center address
    return _PDU_pduReadBCDToString(pdu_ptr, len, out_ptr);
}

/*
 * ======== _PDU_pduDecodeUserDataAndHeader() ========
 * 
 * Private function to decode user data and header
 *
 * Returns: 
 *    Decoded user data bytes.
 */
int _PDU_pduDecodeUserDataAndHeader(
    PDU_Payload  *pdu_ptr, 
    int           hasUserDataHeader, 
    int           dataInSeptets, 
    PDU_UserData *data_ptr)
{
    char byte = 0;
    int userDataLength;
    int headerSeptets = 0;
    int userDataHeaderLength;
    int headerBits;
    int count;
    int bytesRead = 0;
    
    _PDU_pduReadByte(pdu_ptr, &byte);
    bytesRead++;
    
    userDataLength = (int) (byte & 0xff);
    
    if (hasUserDataHeader) {
        PDU_dbgPrintf("User data header not supported\n");
        _PDU_pduReadByte(pdu_ptr, &byte);
        bytesRead++;
        userDataHeaderLength = byte & 0xff;
        bytesRead += _PDU_pduAdvance(pdu_ptr, userDataHeaderLength);
        
        headerBits = (userDataHeaderLength + 1) * 8;
        headerSeptets = headerBits / 7;
        headerSeptets += (headerBits % 7) > 0 ? 1 : 0;
        // Clear data header info
        data_ptr->hdrLen = 0;
        data_ptr->hdr_ptr = 0;
        data_ptr->hdrSeptets = 0;
    }

    /*
     * Here we just create the user data length to be the remainder of
     * the pdu minus the user data hearder. This is because the count
     * could mean the number of uncompressed sepets if the userdata is
     * encoded in 7-bit.
     */
    data_ptr->data_ptr = pdu_ptr->cur_ptr;
    data_ptr->dataLen = _PDU_pduRemaining(pdu_ptr);
    if (dataInSeptets) {
        // Return the number of septets
        count = userDataLength - headerSeptets;
        // If count < 0, return 0 (means UDL was probably incorrect)
        data_ptr->dataSeptets = (count < 0) ? 0 : count;
    }
    else {
        // Set the number of octets
        data_ptr->dataSeptets = data_ptr->dataLen;
    }
    return (bytesRead);
}

/*
 * ======== _PDU_pduDecodeUserData() ========
 * 
 * Private function to decode user data 
 *
 * Returns: 
 *    -1: Failed to decode user data
 *    Otherwise: Decoded user data bytes.
 */
int _PDU_pduDecodeUserData(
    PDU_Payload  *pdu_ptr,
    int           hasUserDataHeader,
    char          dataCodingScheme,
    char         *msg_ptr,
    int           msgLen)
{
    int          userDataCompressed = 0;
    int          encodingType;
    char         msgBody[PDU_BYTE_BUFFER_SIZE];
    PDU_UserData userData;
    int          bytesRead;
    
    encodingType = ENCODING_UNKNOWN;

    // Look up the data encoding scheme
    if ((dataCodingScheme & 0x80) == 0) {
        // Bits 7..4 == 0xxx
        userDataCompressed = (0 != (dataCodingScheme & 0x20));

        if (userDataCompressed) {
            PDU_dbgPrintf("Unsupported SMS data coding scheme:%d\n",
                    dataCodingScheme & 0xff);
        }
        else {
            switch ((dataCodingScheme >> 2) & 0x3) {
                case 0: // GSM 7 bit default alphabet
                    encodingType = ENCODING_7BIT;
                    break;

                case 2: // UCS 2 (16bit)
                    encodingType = ENCODING_16BIT;
                    break;

                case 1: // 8 bit data
                case 3: // reserved
                    PDU_dbgPrintf("Unsupported SMS data coding scheme:%d\n",
                            dataCodingScheme & 0xff);
                    encodingType = ENCODING_8BIT;
                    break;
            }
        }
    }
    else if ((dataCodingScheme & 0xf0) == 0xf0) {
            userDataCompressed = 0;

        if (0 == (dataCodingScheme & 0x04)) {
            // GSM 7 bit default alphabet
            encodingType = ENCODING_7BIT;
        } else {
            // 8 bit data
            encodingType = ENCODING_8BIT;
        }
    }
    else if ((dataCodingScheme & 0xF0) == 0xC0
            || (dataCodingScheme & 0xF0) == 0xD0
            || (dataCodingScheme & 0xF0) == 0xE0) {
        // 3GPP TS 23.038 V7.0.0 (2006-03) section 4

        // 0xC0 == 7 bit, don't store
        // 0xD0 == 7 bit, store
        // 0xE0 == UCS-2, store

        if ((dataCodingScheme & 0xF0) == 0xE0) {
            encodingType = ENCODING_16BIT;
        } else {
            encodingType = ENCODING_7BIT;
        }

        userDataCompressed = 0;

        // bit 0x04 reserved

        if ((dataCodingScheme & 0x03) == 0x00) {
        } else {
            PDU_dbgPrintf("MWI for fax, email, or other :%d\n",
                    dataCodingScheme & 0xff);
        }
    }
    else {
        PDU_dbgPrintf("Unsupported SMS data coding scheme:%d\n",
                dataCodingScheme & 0xff);
    }

    // set both the user data and the user data header.
    _PDU_pduInitUserData(&userData);
    bytesRead = _PDU_pduDecodeUserDataAndHeader(pdu_ptr, hasUserDataHeader,
            encodingType == ENCODING_7BIT, &userData);

    switch (encodingType) {
        case ENCODING_UNKNOWN:
        case ENCODING_8BIT:
            PDU_dbgPrintf("8 Bit or 'unknown' encoding scheme not support."
                    "dataLen:%d\n",
                    userData.dataLen);
            _PDU_printBytes(userData.data_ptr, userData.dataLen);
            break;
            
        case ENCODING_7BIT:
            PDU_dbgPrintf(
                    "7 Bit encoding scheme. dataSeptets:%d\n",
                    userData.dataSeptets);
            if (0 < userData.dataLen) {
                _PDU_printBytes(userData.data_ptr, userData.dataSeptets);
                pdu_to_ascii(userData.data_ptr, userData.dataSeptets, msgBody,
                        sizeof(msgBody));
                _PDU_pduAdvance(pdu_ptr, userData.dataLen);
                bytesRead += userData.dataLen;
                msgLen = (userData.dataSeptets > (msgLen - 1)) ?
                        msgLen - 1 : userData.dataSeptets;
                OSAL_memCpy(msg_ptr, msgBody, msgLen);
                msg_ptr[msgLen] = 0;
                PDU_dbgPrintf("SMS message body (raw): %s",
                        msg_ptr);
                return (bytesRead);
            }
            break;

        case ENCODING_16BIT:
            PDU_dbgPrintf(
                    "16 Bit encoding scheme. dataLen:%d\n",
                    userData.dataLen);
            if (0 < userData.dataLen) {
                _PDU_printBytes(userData.data_ptr, userData.dataLen);
                pdu_utf16_to_utf8((unsigned short*)userData.data_ptr,
                        userData.dataLen, (unsigned char*)msg_ptr, msgLen);
                _PDU_pduAdvance(pdu_ptr, userData.dataLen);
                bytesRead += (userData.dataLen);
                PDU_dbgPrintf("SMS message body (raw): %s",
                        msg_ptr);
                return (bytesRead);
            }
            break;
        default:
            break;
    }
    return (-1);
}

/*
 * ======== _PDU_pduDecodeOriginatingAddress() ========
 * 
 * Private function to decode originating address
 *
 * Returns: 
 *    -1: Failed to decode originating address
 *    Otherwise: Decoded originating address bytes
 */
int _PDU_pduDecodeOriginatingAddress(
    PDU_Payload *pdu_ptr, 
    int          len, 
    char         toa,
    char        *out_ptr)
{
    char  ton;
    int   countSeptets;
    char  addr[PDU_BYTE_BUFFER_SIZE];
    int   byteLen;
    unsigned char  lastByte;
    int   bytesRead;
    unsigned char *lastByte_ptr;
    int   addrLen;
            
    ton = 0x7 & (toa >> 4);

    // TOA must have its high bit set
    if ((toa & 0x80) != 0x80) {
        return (-1);
    }

    if (ton == TON_ALPHANUMERIC) {
        // An alphanumeric address
        countSeptets = len * 4 / 7;
        addrLen = pdu_to_ascii(pdu_ptr->cur_ptr, countSeptets, addr,
                sizeof(addr));
        if (0 != addrLen) {
            // Copy the address to the target
            strcpy(out_ptr, addr);
        }
        byteLen = (len + 1) / 2; // XXX this might be wrong
        bytesRead = _PDU_pduAdvance(pdu_ptr, byteLen);
    }
    else {
        // TS 23.040 9.1.2.5 says
        // that "the MS shall interpret reserved values as 'Unknown'
        // but shall store them exactly as received"
        byteLen = (len + 1) / 2;
        lastByte_ptr = &pdu_ptr->cur_ptr[byteLen - 1];
        lastByte = *lastByte_ptr;

        if ((len & 1) == 1) {
            // Make sure the final unused BCD digit is 0xf
            pdu_ptr->cur_ptr[byteLen - 1] |= 0xf0;
        }
        
        bytesRead = _PDU_pduDecodeAddress(pdu_ptr, byteLen, toa, out_ptr);

        // And restore origBytes
        *lastByte_ptr = lastByte;
    }
    return (bytesRead);
}
