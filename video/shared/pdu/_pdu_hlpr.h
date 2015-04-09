/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22553 $ $Date: 2013-10-23 15:11:27 +0800 (Wed, 23 Oct 2013) $
 */

#ifndef __PDU_HLPR_H_
#define __PDU_HLPR_H_

#define PAUSE   (',')
#define WAIT    (';')
#define WILD    ('N')

#ifndef PDU_DEBUG
#define PDU_dbgPrintf(x, ...)
#else
#define PDU_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

/*
 * TOA = TON + NPI
 * See TS 24.008 section 10.5.4.7 for details.
 * These are the only really useful TOA values
 */
#define TOA_International (0x91)
#define TOA_Unknown       (0x81)

/** Unknown encoding scheme (see TS 23.038) */
#define ENCODING_UNKNOWN  (0)
/** 7-bit encoding scheme (see TS 23.038) */
#define ENCODING_7BIT     (1)
/** 8-bit encoding scheme (see TS 23.038) */
#define ENCODING_8BIT     (2)
/** 16-bit encoding scheme (see TS 23.038) */
#define ENCODING_16BIT    (3)

#define TON_UNKNOWN       (0)

#define TON_INTERNATIONAL (1)

#define TON_NATIONAL      (2)

#define TON_NETWORK       (3)

#define TON_SUBSCRIBER    (4)

#define TON_ALPHANUMERIC  (5)

#define TON_APPREVIATED   (6)


/* Masks for the Message Type Indicator (TP-MTI) fields in the PDU */
/** Message Type bits (TP-MTI) for SMS-DELIVER. */
#define TP_MTI_SMS_DELIVER 0x00;
/** Message Type bits (TP-MTI) for SMS SUBMIT . */
#define TP_MTI_SMS_SUBMIT 0x01
/** Message Type bits (TP-MTI) for SMS-STATUS-REPORT. */
#define TP_MTI_SMS_STATUS_REPORT 0x02;

/* Masks for the TP-DCS (Data Coding Scheme) field */
/** TP-DCS text is compressed. */
#define TP_DCS_TEXT_COMPRESSED 0x20;
/** TP-DCS text is uncompressed. */
#define TP_DCS_TEXT_UNCOMPRESSED 0x00;
/** TP-DCS Message Class has meaning. */
#define TP_DCS_MESSAGE_CLASS_HAS_MEANING 0x10;
/** TP-DCS Message Class has no meaning. */
#define TP_DCS_MESSAGE_CLASS_HAS_NO_MEANING 0x00;

 /** TP-DCS Alphabet default. */
#define TP_DCS_ALPHABET_DEFAULT 0x00;
/** TP-DCS Message Class ME specific. */
#define TP_DCS_MESSAGE_CLASS_ME_SPECIFIC 0x01;
/** TP-DCS Message Class TE specific. */
#define TP_DCS_MESSAGE_CLASS_TE_SPECIFIC 0x03;

#define PDU_BYTE_BUFFER_SIZE (512)

#define UNUSED(_param) ((void)(_param))

/*
 * Enumeration of RP-MTI(Message-Type-Indication).
 */
typedef enum {
    PDU_MTI_RP_DATA_MS_TO_SC = 0,
    PDU_MTI_RP_DATA_SC_TO_MS = 1,
    PDU_MTI_RP_ACK_MS_TO_SC= 2,
    PDU_MTI_RP_ACK_SC_TO_MS = 3,
    PDU_MTI_RP_ERROR_MS_TO_SC = 4,
    PDU_MTI_RP_ERROR_SC_TO_MS = 5,
    PDU_MTI_RP_SMMA_MS_TO_SC = 6,
} PDU_RpMti;

typedef struct {
    unsigned char  pduBytes[PDU_BYTE_BUFFER_SIZE];
    unsigned char *start_ptr;
    int            length;
    unsigned char *cur_ptr;
} PDU_Payload;

typedef struct {
    unsigned char *hdr_ptr;
    int            hdrLen;
    int            hdrSeptets;
    unsigned char *data_ptr;
    int            dataLen;
    int            dataSeptets;
} PDU_UserData;

typedef struct {
    // TP-Service-Centre-Time-Stamp
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int timezoneOffset;
    char tzByte;
} PDU_Time;

int _PDU_pduHexCharToInt(
    char c);

int PDU_pduHexStringToBytes(
    char          *in_ptr,
    unsigned char *out_ptr);

#ifdef PDU_DEBUG
void _PDU_printBytes(
    unsigned char *s_ptr,
    int len);

void _PDU_printAllBytes(
    char *s_ptr);

#else
#define _PDU_printBytes(...)
#define _PDU_printAllBytes(...)
#endif

int _PDU_pduHexCharToInt(
    char c);

int _PDU_pduHexCharToInt(
    char c);

void _PDU_pduInitDecode(
    PDU_Payload   *pdu_ptr,
    char          *data_ptr,
    int            dataLen);

void _PDU_pduInitEncode(
    PDU_Payload   *pdu_ptr);

int _PDU_pduHexCharToInt(
    char c);

int _PDU_pduReadByte(
    PDU_Payload *pdu_ptr,
    char        *byte_ptr);

int _PDU_pduWriteByte(
    PDU_Payload *pdu_ptr,
    char         byte);

int _PDU_pduWriteBytes(
    PDU_Payload  *pdu_ptr,
    unsigned char bytes[],
    int           size);

int _PDU_pduRemaining(
    PDU_Payload *pdu_ptr);

int _PDU_pduGetLength(
    PDU_Payload *pdu_ptr);

char _PDU_pduHexByteToBcd(
    int b);

int _PDU_pduHexCharToInt(
    char c);

int _PDU_pduReadBCDToString(
    PDU_Payload *pdu_ptr,
    int          len,
    char        *out_ptr);

int _PDU_pduAdvance(
    PDU_Payload *pdu_ptr,
    int          size);

int _PDU_pduHexCharToInt(
    char c);

int _PDU_pduWriteStringToBCD(
    PDU_Payload *pdu_ptr,
    char        *address_ptr,
    int          addressLen);

int _PDU_pduDecodeAddress(
    PDU_Payload *pdu_ptr,
    int          len,
    char         toa,
    char        *out_ptr);

int _PDU_pduDecodeUserDataAndHeader(
    PDU_Payload  *pdu_ptr, 
    int           hasUserDataHeader, 
    int           dataInSeptets, 
    PDU_UserData *data_ptr);

int _PDU_pduDecodeUserData(
    PDU_Payload  *pdu_ptr,
    int           hasUserDataHeader,
    char          dataCodingScheme,
    char         *msg_ptr,
    int           msgLen);

int _PDU_pduDecodeOriginatingAddress(
    PDU_Payload *pdu_ptr, 
    int          len, 
    char         toa,
    char        *out_ptr);

#endif
