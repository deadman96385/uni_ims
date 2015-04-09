/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19214 $ $Date: 2012-12-10 01:50:08 -0800 (Mon, 10 Dec 2012) $
 */
#include <osal.h>
#include "rpm.h"
#include "_rpm.h"
#include "_rpm_utils.h"

/** Outbound IP call scheme support mode. */
static int _OutboudIpTelSupportMode = USE_OUTBOUND_IP_TEL_SUPPORT;

/**
 * ======== _RPM_utilAddressPrefixCheck() ========
 *
 * Check is there any prefix in the input address.
 * @param address address to check.
 * @return the prefix type.
 */
static int _RPM_utilAddressPrefixCheck(
    const char *address_ptr)
{
    if (0 == OSAL_strncmp(address_ptr, STRING_PREFIX_SIP, 
                sizeof(STRING_PREFIX_SIP) - 1)) {
        return (ADDRESS_PREFIX_SIP);
    }
    else if (0 == OSAL_strncmp(address_ptr, STRING_PREFIX_TEL, 
                sizeof(STRING_PREFIX_TEL) - 1)) {
        return (ADDRESS_PREFIX_TEL);
    }
    return (ADDRESS_PREFIX_NONE);
}

/**
 * ======== _RPM_utilAddressSymbolAtCheck() ========
 *
 * Check is there any symbol '@' in the input address.
 * @param address address to check.
 * @return true: with '@', false: without '@'
 */
static RPM_Return _RPM_utilAddressSymbolAtCheck(
    const char *address_ptr)
{
    if (NULL != (OSAL_strchr(address_ptr, '@'))) {
        return (RPM_RETURN_OK);
    }
    return (RPM_RETURN_ERROR);
}

/**
 * ======== _RPM_utilGetDomainFromAddress() ========
 *
 * Get the domain from the input address.
 * @param address address to check.
 * @return domain in the address.
 */
static char* _RPM_utilGetDomainFromAddress(
    const char *address_ptr)
{
    char *pos_ptr;
    char *extra_ptr;
    if (NULL != (pos_ptr = OSAL_strchr(address_ptr, '@'))) {
        pos_ptr++;
        /*
         * Also ensure that there's no ';' since some IP
         * domains might have params after it. We just
         * want to get the NULL terminated domain.
         */
        if (NULL != (extra_ptr = OSAL_strchr(pos_ptr, ';'))) {
            *extra_ptr = 0;
        }
        return (pos_ptr);
    }
    return (NULL);
}

/**
 * ======== _RPM_utilIsNumeric() ========
 *
 * Check the input string is numeric or not.
 * @param input the string to check
 * @return true: input string is numeric, false: input string is not numeric.
 */
OSAL_Boolean _RPM_utilIsNumeric(
    const char *input_ptr)
{
    vint i;

    /* Loop the string to see if every character is numeric */
    for (i = 0; i < OSAL_strlen(input_ptr); i++) {
        if (('0' > input_ptr[i]) || ('9' < input_ptr[i])) {

#if defined(PROVIDER_LGUPLUS)
            /* '#' is allowed. */
            if ('#' == input_ptr[i]) {
                continue;
            }
#endif            
            /* prefix '+' is allowed */
            if ((0 != i) || ('+' != input_ptr[i])) {
                return (OSAL_FALSE);
            }
        }
    }
    return (OSAL_TRUE);
}

/**
 * ======== RPM_utilGetNameFromAddress() ========
 *
 * Get the name from the input address.
 * @param address address to check.
 * @return name in the address.
 */
RPM_Return RPM_utilGetNameFromAddress(
    const char *address_ptr,
    char       *out_ptr, 
    vint        maxOutSize)
{
    char       *pos_ptr;
    vint        size;
    const char *noPrefixAddress_ptr;

    /* Remove prefix header of address, if there is it */
    if (ADDRESS_PREFIX_NONE != _RPM_utilAddressPrefixCheck(address_ptr)) {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }
    else {
        noPrefixAddress_ptr = address_ptr;
    }

    /*
    * example 1: "123@domain"
    * => Can find "@" and then we can get the size of numbers before "@".
    * example 2: "+123456"
    * => Can't find "@" nor ";". The whole string is the numbers.
    * example 3: "+123456;parameters"
    * => Can't fine "@" but ";" and then we cat get the size of number 
    * before ";".
    */
    if (NULL == (pos_ptr = OSAL_strchr(noPrefixAddress_ptr, '@'))) {       
        if (NULL == (pos_ptr = OSAL_strchr(noPrefixAddress_ptr, ';'))) {
            size = OSAL_strlen(noPrefixAddress_ptr);
            OSAL_memCpy(out_ptr, noPrefixAddress_ptr, size);
            out_ptr[size] = 0;
            return (RPM_RETURN_OK);
        }
    }

    size = pos_ptr - noPrefixAddress_ptr;
    maxOutSize--;
    size = (size > maxOutSize) ? maxOutSize : size;
    OSAL_memCpy(out_ptr, noPrefixAddress_ptr, size);
    out_ptr[size] = 0;
    return (RPM_RETURN_OK);
}

/**
 * ======== _RPM_utilStripSeperators() ========
 *
 * removes any hyphens or parenthesis.
 * @param target The string to remove separators from.
 * @return The address without any separators.
 */
static RPM_Return _RPM_utilStripSeperators(
    const char *string_ptr,
    char *out_ptr,
    vint maxOutSize)
{
    char v;
    vint x;
    vint len = OSAL_strlen(string_ptr);
    vint pos = 0;

    for (x = 0 ; (x < len) && (pos < maxOutSize) ; x++) {
        v = string_ptr[x];
        switch (v) {
        case '-':
        case '(':
        case ')':
        case ' ':
            break;
        default:
            out_ptr[pos++] = v;
            break;
        }
    }
    /* We are done.  Make sure the out buffer is zero'd. */
    if (pos == maxOutSize) {
        return (RPM_RETURN_ERROR);
    }
    out_ptr[pos] = 0;
    return (RPM_RETURN_OK);
}

/**
 * ======== RPM_utilOutBoundIpCallNormalize ========
 *
 * The utility to normalize the address/number for out-bound IP call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for out-bound IP call.
 * @return the normalized address/number by out-bound IP call rule.
 */
RPM_Return RPM_utilOutBoundIpNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[RPM_ADDRESS_STR_LEN];
    char        strippedName[RPM_ADDRESS_STR_LEN];
    const char  *noPrefixAddress_ptr;
    _RPM_Obj    *RPM_objPtr = _RPM_getObject();

    RPM_dbgPrintf("non-normalized address:%s\n", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _RPM_utilAddressPrefixCheck(address_ptr);
    if (prefixCheckResult == ADDRESS_PREFIX_TEL) {
        uriPrefix_ptr = STRING_PREFIX_TEL;
    }
    else if (prefixCheckResult == ADDRESS_PREFIX_SIP) {
        uriPrefix_ptr = STRING_PREFIX_SIP;
    }
    else {
        uriPrefix_ptr = STRING_PREFIX_NONE;
    }

    if (prefixCheckResult == ADDRESS_PREFIX_NONE) {
        /* Remove address prefix if any. */
        noPrefixAddress_ptr = address_ptr;
        if (NULL != OSAL_strchr(address_ptr, '+')) {
            /* check from RCS Provisioning settings */
            if (RPM_URL_FORMAT_SIP_URI == RPM_objPtr->urlFmt.intUrlFmt) {
                uriPrefix_ptr = STRING_PREFIX_SIP;
            }
            else {
                uriPrefix_ptr = STRING_PREFIX_TEL;
            }
        }
        else {
            if (RPM_URL_FORMAT_SIP_URI == RPM_objPtr->urlFmt.natUrlFmt) {
                uriPrefix_ptr = STRING_PREFIX_SIP;
            }
            else {
                uriPrefix_ptr = STRING_PREFIX_TEL;
            }
        }
    }
    else {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }

    /* Get name and domain from address. */
    if (RPM_RETURN_OK != _RPM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        if (!domain_ptr) {
            return (RPM_RETURN_ERROR);
        }
        uriDomain_ptr = domain_ptr;
        _RPM_utilStripSeperators(noPrefixAddress_ptr, strippedName, 
                RPM_ADDRESS_STR_LEN);
        if (OSAL_TRUE == _RPM_utilIsNumeric(strippedName)) {
            /* Only use TEL support if the build option is set. */
            if (0 != _OutboudIpTelSupportMode) {
                uriPrefix_ptr = STRING_PREFIX_TEL;
            }
        }
    }
    else {
        uriDomain_ptr = _RPM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (RPM_RETURN_ERROR);
        }
        RPM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                RPM_ADDRESS_STR_LEN);
        _RPM_utilStripSeperators(uriName, strippedName, RPM_ADDRESS_STR_LEN);
    }

    /* Check out-bound IP call rules here. */
    if ((prefixCheckResult == ADDRESS_PREFIX_TEL) && 
            (OSAL_FALSE == _RPM_utilIsNumeric(strippedName))) {
        return (RPM_RETURN_ERROR);
    }
    else {
        /* Don't include a domain if using a "tel:" prefix. */
        if (uriPrefix_ptr == STRING_PREFIX_TEL) {
            OSAL_snprintf(out_ptr, maxOutLen, "%s%s", uriPrefix_ptr, 
                    strippedName);
        }
        else {
            OSAL_snprintf(out_ptr, maxOutLen, "%s%s@%s", uriPrefix_ptr, 
                    strippedName, uriDomain_ptr);
        }
        OSAL_logMsg("%s %d: normalized address:%s", __FUNCTION__, __LINE__, 
                out_ptr);
        return (RPM_RETURN_OK);
    }
}


/**
 * The utility to normalize the address/number for out-bound CS call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for out-bound CS call.
 * @return the normalized address/number by out-bound CS call rule.
 */
RPM_Return RPM_utilOutBoundCsNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[RPM_ADDRESS_STR_LEN];
    char        strippedName[RPM_ADDRESS_STR_LEN];
    const char *noPrefixAddress_ptr;

    RPM_dbgPrintf("non-normalized address:%s", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _RPM_utilAddressPrefixCheck(address_ptr);
    uriPrefix_ptr = STRING_PREFIX_NONE;

    /* Remove address prefix if any. */
    if (prefixCheckResult == ADDRESS_PREFIX_NONE) {
        noPrefixAddress_ptr = address_ptr;
    }
    else {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }

    /* Get name and domain from address. */
    if (RPM_RETURN_OK != _RPM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        uriDomain_ptr = STRING_NO_DOMAIN;
        _RPM_utilStripSeperators(noPrefixAddress_ptr, strippedName, 
                RPM_ADDRESS_STR_LEN);
    }
    else {
        uriDomain_ptr = _RPM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (RPM_RETURN_ERROR);
        }
        RPM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                RPM_ADDRESS_STR_LEN);
        _RPM_utilStripSeperators(uriName, strippedName, RPM_ADDRESS_STR_LEN);
    }

    /* Check out-bound CS call rules here. */
    if (uriDomain_ptr == STRING_NO_DOMAIN) {
        if (OSAL_FALSE == _RPM_utilIsNumeric(strippedName)) {
            return (RPM_RETURN_ERROR);
        }
    }
    else {
        if (OSAL_FALSE == _RPM_utilIsNumeric(strippedName)) {
            return (RPM_RETURN_ERROR);
        }
        if (0 != OSAL_strcmp(uriDomain_ptr, domain_ptr)) {
            return (RPM_RETURN_ERROR);
        }
    }
    OSAL_snprintf(out_ptr, maxOutLen, "%s%s", uriPrefix_ptr, strippedName);
    RPM_dbgPrintf("normalized address:%s", out_ptr);
    return (RPM_RETURN_OK);
}

/**
 * The utility to normalize the address/number for in-bound IP call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for in-bound IP call.
 * @return the normalized address/number by in-bound IP call rule.
 */
RPM_Return RPM_utilInBoundIpNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen)
{
#ifdef RPM_NORMALIZE_INBOUND_TO_URI
    /* Just copy from address_ptr. */
    OSAL_snprintf(out_ptr, maxOutLen, "%s", address_ptr);
    *display_ptr = 0;
    RPM_dbgPrintf("normalized address:%s", out_ptr);
    return (RPM_RETURN_OK);
#else

    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[RPM_ADDRESS_STR_LEN];
    const char *noPrefixAddress_ptr;

    RPM_dbgPrintf("non-normalized address:%s", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _RPM_utilAddressPrefixCheck(address_ptr);
    if (prefixCheckResult == ADDRESS_PREFIX_TEL) {
        uriPrefix_ptr = STRING_PREFIX_NONE;
    }
    else {
        uriPrefix_ptr = STRING_PREFIX_SIP;
    }

    /* Remove address prefix if any. */
    if (prefixCheckResult == ADDRESS_PREFIX_NONE) {
        noPrefixAddress_ptr = address_ptr;
    }
    else {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }

    /* Get name and domain from address. */
    uriDomain_ptr = _RPM_utilGetDomainFromAddress(noPrefixAddress_ptr);
    RPM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
            RPM_ADDRESS_STR_LEN);

    /* Check in-bound IP call rules here. */
    if ((prefixCheckResult == ADDRESS_PREFIX_TEL)) {
        if (OSAL_FALSE == _RPM_utilIsNumeric(uriName)) {
            return (RPM_RETURN_ERROR);
        }
        OSAL_snprintf(out_ptr, maxOutLen, "%s%s", uriPrefix_ptr, uriName);
        /* There is no 'alpha' or display name. */
        *display_ptr = 0;
        RPM_dbgPrintf("normalized address:%s", out_ptr);
        return (RPM_RETURN_OK);
    }
    else if (prefixCheckResult == ADDRESS_PREFIX_SIP) {
        RPM_dbgPrintf("uriDomain:%s domain:%s", uriDomain_ptr, domain_ptr);
        if (!uriDomain_ptr) {
            return (RPM_RETURN_ERROR);
        }
        if (0 == OSAL_strcmp(uriDomain_ptr, domain_ptr)) {
            if (OSAL_TRUE == _RPM_utilIsNumeric(uriName)) {
                OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
                *display_ptr = 0;
            }
            else {
                /*
                 * Not numeric, let's set both the address & alpha to just 
                 * the name.
                 */
                OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
                *display_ptr = 0;
            }
            RPM_dbgPrintf("normalized address:%s", out_ptr);
            return (RPM_RETURN_OK);
        }
        else {
            /* 
             * Not numeric, let's set both the address & alpha to the same
             * thing. 
             */
            OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
            *display_ptr = 0;
            RPM_dbgPrintf("normalized address:%s", out_ptr);
            return (RPM_RETURN_OK);
        }
    }
    else {
        return (RPM_RETURN_ERROR);
    }
#endif
}

/**
 * The utility to normalize the address/number for in-bound CS call.
 *
 * @param inputAddress The address/number to normalize for in-bound CS call.
 *
 * @return the normalized address/number by in-bound CS call rule.
 */
RPM_Return RPM_utilInBoundCsNormalize(
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    /* In current design, just return the input string. */
    RPM_dbgPrintf("non-normalized address:%s", address_ptr);
    OSAL_snprintf(out_ptr, maxOutLen, "%s", address_ptr);
    RPM_dbgPrintf("normalized address:%s", out_ptr);
    return (RPM_RETURN_OK);
}

