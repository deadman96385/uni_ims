/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19050 $ $Date: 2012-11-29 15:10:16 -0800 (Thu, 29 Nov 2012) $
 */

#ifndef _RPM_UTILS_H_
#define _RPM_UTILS_H_

/* Generally used definitions */
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
 * RPM Utility helper methods 
 */
RPM_Return RPM_utilGetNameFromAddress(
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutSize);

RPM_Return RPM_utilOutBoundIpNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

RPM_Return RPM_utilOutBoundCsNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

RPM_Return RPM_utilInBoundIpNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen);

RPM_Return RPM_utilInBoundCsNormalize(
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen);

#endif // _RPM_UTILS_H_
