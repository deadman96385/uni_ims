/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26717 $ $Date: 2014-06-03 16:49:18 +0800 (Tue, 03 Jun 2014) $
 */

#ifndef __CSM_UTILS_H__
#define __CSM_UTILS_H__

/* Generally used definitions */
#define CSM_UTILS_REASON_CODE_PARAM     ("CODE:")
#define CSM_UTILS_REASON_DESC_PARAM     ("DESC:")

#define CSM_UTILS_STRING_SZ (64)

#define CSM_UTILS_RETURN_INVALID_CODE (-1)

#define USE_OUTBOUND_IP_TEL_SUPPORT (0)

#define STRING_PREFIX_NONE ("")
/** The fixed value "sip:" for address prefix. */
#define STRING_PREFIX_SIP ("sip:")
    /** The fixed value "tel:" for address prefix. */
#define STRING_PREFIX_TEL ("tel:")
/** The fixed value "tel:+" for address prefix. */
/** The string length of address prefix "sip:" or "tel:". */
#define PREFIX_STRING_LENGTH (4)
/** Indicate there is no prefix in address. */
#define ADDRESS_PREFIX_NONE (0)
/** Indicate there is prefix "sip:" in address. */
#define ADDRESS_PREFIX_SIP (1)
/** Indicate there is prefix "tel:" in address. */
#define ADDRESS_PREFIX_TEL (2)
/** The string to indicate there is no domain in address.*/
#define STRING_NO_DOMAIN ("no domain")
/** The string to the regular expression in isNumeric function. */
#define NUMERIC_REGULAR_EXPRESSION ("^[+]?\\d+$")

/* 
 * CSM Utility helper public methods 
 */
vint CSM_utilGetReasonCode(
    const char *reason_ptr);

vint CSM_utilOutBoundIpCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

vint CSM_utilOutBoundCsCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

vint CSM_utilInBoundIpCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen);

vint CSM_utilInBoundCsCallNormalize(
    const char *uid_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

vint CSM_utilInBoundIpSmsNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen);

vint CSM_utilUrlDecode(
    const char *instr_ptr,
    char *outstr_ptr);

vint _CSM_utilGetValue(
    const char *target_ptr,
    const char *key_ptr,
    char      **value_pPtr,
    uvint      *size_ptr);

vint CSM_utilGetNameFromAddress(
    const char *address_ptr,
    char       *out_ptr, 
    vint        maxOutSize);

#endif
