/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28006 $ $Date: 2014-08-08 17:13:11 +0800 (Fri, 08 Aug 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include "_csm.h"
#include "_csm_utils.h"

/** Outbound IP call scheme support mode. */
static int _OutboudIpTelSupportMode = USE_OUTBOUND_IP_TEL_SUPPORT;

/**
 * ======== CSM_utilGetReasonDesc() ========
 *
 * Helper uitility to get Reason code from an ISI reason string buffer
 *
 * Returns: 
 *     vint code
 */
vint CSM_utilGetReasonCode(
    const char *reason_ptr)
{
    const char *start_ptr;
    start_ptr = OSAL_strscan(reason_ptr, CSM_UTILS_REASON_CODE_PARAM);
    if (NULL == start_ptr) {
        return (CSM_UTILS_RETURN_INVALID_CODE);
    }
    start_ptr += (sizeof(CSM_UTILS_REASON_CODE_PARAM) - 1);
    return OSAL_atoi(start_ptr);
}

/**
 * ======== CSM_utilGetReasonDesc() ========
 *
 * Helper uitility to get Reason description from an ISI reason string buffer
 *
 * Returns: 
 *     CSM_OK: function exits normally.
 *     CSM_ERR: in case of error
 */
vint CSM_utilGetReasonDesc(
    const char *reason_ptr,
    char *out_ptr,
    vint maxOutSize)
{
    const char *start_ptr;
    vint bytes;
    start_ptr = OSAL_strscan(reason_ptr, CSM_UTILS_REASON_DESC_PARAM);
    if (NULL == start_ptr) {
        *out_ptr = 0;
        return (CSM_ERR);
    }
    start_ptr += (sizeof(CSM_UTILS_REASON_DESC_PARAM) - 1);
    bytes = OSAL_snprintf(out_ptr, maxOutSize, "%s", start_ptr);
    if (bytes > maxOutSize) {
        /* Truncated! */
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/**
 * ======== _CSM_utilAddressPrefixCheck() ========
 *
 * Check is there any prefix in the input address.
 * @param address address to check.
 * @return the prefix type.
 */
static int _CSM_utilAddressPrefixCheck(
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
 * ======== _CSM_utilAddressSymbolAtCheck() ========
 *
 * Check is there any symbol '@' in the input address.
 * @param address address to check.
 * @return true: with '@', false: without '@'
 */
static vint _CSM_utilAddressSymbolAtCheck(
    const char *address_ptr)
{
    if (NULL != (OSAL_strchr(address_ptr, '@'))) {
        return (CSM_OK);
    }
    return (CSM_ERR);
}

/**
 * ======== _CSM_utilGetDomainFromAddress() ========
 *
 * Get the domain from the input address.
 * @param address address to check.
 * @return domain in the address.
 */
static char* _CSM_utilGetDomainFromAddress(
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
 * ======== _CSM_utilGetNameFromAddress() ========
 *
 * Get the name from the input address.
 * @param address address to check.
 * @return name in the address.
 */
vint CSM_utilGetNameFromAddress(
    const char *address_ptr,
    char       *out_ptr, 
    vint        maxOutSize)
{
        char       *pos_ptr;
    vint        size;
    const char *noPrefixAddress_ptr;

    /* Remove prefix header of address, if there is it */
    if (ADDRESS_PREFIX_NONE != _CSM_utilAddressPrefixCheck(address_ptr)) {
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
            return (CSM_OK);
        }
    }

    size = pos_ptr - noPrefixAddress_ptr;
    maxOutSize--;
    size = (size > maxOutSize) ? maxOutSize : size;
    OSAL_memCpy(out_ptr, noPrefixAddress_ptr, size);
    out_ptr[size] = 0;
    return (CSM_OK);

}

/**
 * ======== _CSM_utilIsNumeric() ========
 *
 * Check the input string is numeric or not.
 * @param input the string to check
 * @return true: input string is numeric, false: input string is not numeric.
 */
vint _CSM_utilIsNumeric(
    const char *input_ptr)
{
    vint i;

    /* Loop the string to see if every character is numeric */
    for (i = 0; i < OSAL_strlen(input_ptr); i++) {
        if (('0' > input_ptr[i]) || ('9' < input_ptr[i])) {
            /* prefix '+' is allowed */
            if ((0 != i) || ('+' != input_ptr[i])) {
                return (CSM_ERR);
            }
        }
    }
    return (CSM_OK);
}

/**
 * ======== _CSM_utilStripSeperators() ========
 *
 * removes any hyphens or parenthesis.
 * @param target The string to remove separators from.
 * @return The address without any separators.
 */
static vint _CSM_utilStripSeperators(
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
        return (CSM_ERR);
    }
    out_ptr[pos] = 0;
    return (CSM_OK);
}

/**
 * ======== CSM_utilOutBoundIpCallNormalize ========
 *
 * The utility to normalize the address/number for out-bound IP call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for out-bound IP call.
 * @return the normalized address/number by out-bound IP call rule.
 */
vint CSM_utilOutBoundIpCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[CSM_UTILS_STRING_SZ];
    char        strippedName[CSM_UTILS_STRING_SZ];
    const char  *noPrefixAddress_ptr;

    CSM_dbgPrintf("non-normalized address:%s\n", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _CSM_utilAddressPrefixCheck(address_ptr);
    if (prefixCheckResult == ADDRESS_PREFIX_TEL) {
        uriPrefix_ptr = STRING_PREFIX_TEL;
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
    if (CSM_OK != _CSM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        if (!domain_ptr) {
            return (CSM_ERR);
        }
        uriDomain_ptr = domain_ptr;
        _CSM_utilStripSeperators(noPrefixAddress_ptr, strippedName, 
                CSM_UTILS_STRING_SZ);
        if (CSM_OK == _CSM_utilIsNumeric(strippedName)) {
            /* Only use TEL support if the build option is set. */
            if (0 != _OutboudIpTelSupportMode) {
                uriPrefix_ptr = STRING_PREFIX_TEL;
            }
        }
    }
    else {
        uriDomain_ptr = _CSM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (CSM_ERR);
        }
        CSM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                CSM_UTILS_STRING_SZ);
        _CSM_utilStripSeperators(uriName, strippedName, CSM_UTILS_STRING_SZ);
    }

    /* Check out-bound IP call rules here. */
    if ((prefixCheckResult == ADDRESS_PREFIX_TEL) && 
            (CSM_ERR == _CSM_utilIsNumeric(strippedName))) {
        return (CSM_ERR);
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
        return (CSM_OK);
    }
}


/**
 * The utility to normalize the address/number for out-bound CS call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for out-bound CS call.
 * @return the normalized address/number by out-bound CS call rule.
 */
vint CSM_utilOutBoundCsCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[CSM_UTILS_STRING_SZ];
    char        strippedName[CSM_UTILS_STRING_SZ];
    const char *noPrefixAddress_ptr;

    CSM_dbgPrintf("non-normalized address:%s", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _CSM_utilAddressPrefixCheck(address_ptr);
    uriPrefix_ptr = STRING_PREFIX_NONE;

    /* Remove address prefix if any. */
    if (prefixCheckResult == ADDRESS_PREFIX_NONE) {
        noPrefixAddress_ptr = address_ptr;
    }
    else {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }

    /* Get name and domain from address. */
    if (CSM_OK != _CSM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        uriDomain_ptr = STRING_NO_DOMAIN;
        _CSM_utilStripSeperators(noPrefixAddress_ptr, strippedName, 
                CSM_UTILS_STRING_SZ);
    }
    else {
        uriDomain_ptr = _CSM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (CSM_ERR);
        }
        CSM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                CSM_UTILS_STRING_SZ);
        _CSM_utilStripSeperators(uriName, strippedName, CSM_UTILS_STRING_SZ);
    }

    /* Check out-bound CS call rules here. */
    if (uriDomain_ptr == STRING_NO_DOMAIN) {
        if (CSM_OK != _CSM_utilIsNumeric(strippedName)) {
            return (CSM_ERR);
        }
    }
    else {
        if (CSM_OK != _CSM_utilIsNumeric(strippedName)) {
            return (CSM_ERR);
        }
        if (0 != OSAL_strcmp(uriDomain_ptr, domain_ptr)) {
            return (CSM_ERR);
        }
    }
    OSAL_snprintf(out_ptr, maxOutLen, "%s%s", uriPrefix_ptr, strippedName);
    CSM_dbgPrintf("normalized address:%s", out_ptr);
    return (CSM_OK);
}

/**
 * The utility to normalize the address/number for in-bound IP call.
 * @param domain account domain for comparison.
 * @param inputAddress The address/number to normalize for in-bound IP call.
 * @return the normalized address/number by in-bound IP call rule.
 */
vint CSM_utilInBoundIpCallNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char       *uriPrefix_ptr;
    char        uriName[CSM_UTILS_STRING_SZ];
    const char *noPrefixAddress_ptr;

    CSM_dbgPrintf("non-normalized address:%s", address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _CSM_utilAddressPrefixCheck(address_ptr);
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
    if (CSM_OK != _CSM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        return (CSM_ERR);
    }
    else {
        uriDomain_ptr = _CSM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (CSM_ERR);
        }
        CSM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                CSM_UTILS_STRING_SZ);
    }

    /* Check in-bound IP call rules here. */
    if ((prefixCheckResult == ADDRESS_PREFIX_TEL)) {
        if (CSM_OK != _CSM_utilIsNumeric(uriName)) {
            return (CSM_ERR);
        }
        OSAL_snprintf(out_ptr, maxOutLen, "%s%s", uriPrefix_ptr, uriName);
        /* There is no 'alpha' or display name. */
        *display_ptr = 0;
        CSM_dbgPrintf("normalized address:%s", out_ptr);
        return (CSM_OK);
    }
    else if (prefixCheckResult == ADDRESS_PREFIX_SIP) {
        CSM_dbgPrintf("uriDomain:%s domain:%s", uriDomain_ptr, domain_ptr);
        if (0 == OSAL_strcmp(uriDomain_ptr, domain_ptr)) {
            if (CSM_OK == _CSM_utilIsNumeric(uriName)) {
                // Note: as of 6/13 we will include the FQDN.  
                // Better call log experience.
                //OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
                //*display_ptr = 0;
                OSAL_snprintf(display_ptr, maxDisplayLen, "%s%s@%s", 
                        uriPrefix_ptr, uriName, uriDomain_ptr);
                OSAL_snprintf(out_ptr, maxOutLen, "%s%s@%s", uriPrefix_ptr, 
                        uriName, uriDomain_ptr);
            }
            else {
                /*
                 * Not numeric, let's set both the address & alpha to 
                 * just the name.
                 * Note: as of 6/13 we will include the FQDN.  
                 * Better call log experience.
                 */
                //OSAL_snprintf(display_ptr, maxDisplayLen, "%s", uriName);
                //OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
                OSAL_snprintf(display_ptr, maxDisplayLen, "%s%s@%s", 
                        uriPrefix_ptr, uriName, uriDomain_ptr);
                OSAL_snprintf(out_ptr, maxOutLen, "%s%s@%s", uriPrefix_ptr, 
                        uriName, uriDomain_ptr);
            }
            CSM_dbgPrintf("normalized address:%s", out_ptr);
            return (CSM_OK);
        }
        else {
            /* 
             * Not numeric, let's set both the address & alpha to 
             * the same thing. 
             */
            OSAL_snprintf(display_ptr, maxDisplayLen, "%s%s@%s", 
                    uriPrefix_ptr, uriName, uriDomain_ptr);
            OSAL_snprintf(out_ptr, maxOutLen, "%s%s@%s", uriPrefix_ptr, 
                    uriName, uriDomain_ptr);
            CSM_dbgPrintf("normalized address:%s", out_ptr);
            return (CSM_OK);
        }
    }
    else {
        return (CSM_ERR);
    }
}

vint CSM_utilInBoundIpSmsNormalize(
    const char *domain_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen,
    char       *display_ptr,
    vint        maxDisplayLen)
{
    int         prefixCheckResult;
    const char *uriDomain_ptr;
    char        uriName[CSM_UTILS_STRING_SZ];
    const char *noPrefixAddress_ptr;

    CSM_dbgPrintf("%s %d: non-normalized address:%s", __FUNCTION__, __LINE__, 
            address_ptr);

    /* Check address prefix. */
    prefixCheckResult = _CSM_utilAddressPrefixCheck(address_ptr);

    /* Remove address prefix if any. */
    if (prefixCheckResult == ADDRESS_PREFIX_NONE) {
        noPrefixAddress_ptr = address_ptr;
    }
    else {
        noPrefixAddress_ptr = address_ptr + PREFIX_STRING_LENGTH;
    }

    /* Get name and domain from address. */
    if (CSM_OK != _CSM_utilAddressSymbolAtCheck(noPrefixAddress_ptr)) {
        return (CSM_ERR);
    }
    else {
        uriDomain_ptr = _CSM_utilGetDomainFromAddress(noPrefixAddress_ptr);
        if (!uriDomain_ptr) {
            return (CSM_ERR);
        }
        CSM_utilGetNameFromAddress(noPrefixAddress_ptr, uriName, 
                CSM_UTILS_STRING_SZ);
    }

    /* Check in-bound IP call rules here. */
    if ((prefixCheckResult == ADDRESS_PREFIX_TEL)) {
        if (CSM_OK != _CSM_utilIsNumeric(uriName)) {
            return (CSM_ERR);
        }
        OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
        /* There is no 'alpha' or display name. */
        *display_ptr = 0;
        CSM_dbgPrintf("normalized address:%s", out_ptr);
        return (CSM_OK);
    }
    else if (prefixCheckResult == ADDRESS_PREFIX_SIP) {
        CSM_dbgPrintf("uriDomain:%s domain:%s", uriDomain_ptr, domain_ptr);
        if (0 == OSAL_strcmp(uriDomain_ptr, domain_ptr)) {
            if (CSM_OK == _CSM_utilIsNumeric(uriName)) {
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
            CSM_dbgPrintf("normalized address:%s", out_ptr);
            return (CSM_OK);
        }
        else {
            /* 
             * Not numeric, let's set both the address & alpha to the same
             * thing. 
             */
            OSAL_snprintf(out_ptr, maxOutLen, "%s", uriName);
            *display_ptr = 0;
            CSM_dbgPrintf("normalized address:%s", out_ptr);
            return (CSM_OK);
        }
    }
    else {
        return (CSM_ERR);
    }
}

/**
 * The utility to normalize the address/number for in-bound CS call.
 *
 * @param uid account user id in String.
 * @param inputAddress The address/number to normalize for in-bound CS call.
 *
 * @return the normalized address/number by in-bound CS call rule.
 */
vint CSM_utilInBoundCsCallNormalize(
    const char *uid_ptr,
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutLen)
{
    /* In current design, just return the input string. */
    CSM_dbgPrintf("non-normalized address:%s", address_ptr);
    OSAL_snprintf(out_ptr, maxOutLen, "%s", address_ptr);
    CSM_dbgPrintf("normalized address:%s", out_ptr);
    return (CSM_OK);
}

/**
 * ======== CSM_utilUrlDecode ========
 *
 *
 * The utility to decode ulrencoded string
 *
 * @param instr input String to decode.
 * @param outstr the result buffer, assumed len >= strlen(instr)
 *
 * @return CSM_OK.
 */
vint CSM_utilUrlDecode(
    const char *instr_ptr,
    char *outstr_ptr)
{
    int ipos = 0, bpos = 0;
    char entity[3] = { 0, 0, 0 };
    unsigned dec;

    while (instr_ptr[ipos]) {
        while (instr_ptr[ipos] && instr_ptr[ipos] != '%')
            if (instr_ptr[ipos] == '+') {
                outstr_ptr[bpos++] = ' ';
                ipos++;
            } else
                outstr_ptr[bpos++] = instr_ptr[ipos++];
        if (!instr_ptr[ipos])
            break;

        if (instr_ptr[ipos + 1] && instr_ptr[ipos + 2]) {
            ipos++;
            entity[0] = instr_ptr[ipos++];
            entity[1] = instr_ptr[ipos++];
            dec = OSAL_htoi(entity);
            outstr_ptr[bpos++] = (char)dec;
        } else {
            outstr_ptr[bpos++] = instr_ptr[ipos++];
        }
    }
    outstr_ptr[bpos] = '\0';

    return (CSM_OK);
}

/*
 * ======== _CSM_utilGetValue() ========
 * This function is to parse the target string and find "key:value" pair and
 * return the pointer of value and the size of value. Return CSM_ERR if key is
 * not found.
 * For example: target_ptr: "key1:value1", key2:value2"
 * key: "key1"
 * value_ptr will point to "value1", key2:value2" and the size will be 6.
 *
 * Return:
 *  CSM_OK: key and value found. value_ptr and size_ptr is updated.
 *  CSM_ERR: key is not found.
 */
vint _CSM_utilGetValue(
    const char *target_ptr,
    const char *key_ptr,
    char      **value_pPtr,
    uvint      *size_ptr)
{
    char *pos_ptr;
    char *value_ptr;
    char *end_ptr;

    if (NULL != (pos_ptr = OSAL_strscan(target_ptr, key_ptr))) {
        /* Found the key, then ":". */
        if (NULL != (value_ptr = OSAL_strscan(pos_ptr, ":"))) {
            value_ptr += 1; /* Advance : */
            *value_pPtr = value_ptr;
            /* Then find the end of value. */
            if (NULL != (end_ptr = OSAL_strscan(value_ptr, ","))) {
                *size_ptr = end_ptr - value_ptr;
                return (CSM_OK);
            }
            else {
                /* No more key:value, get the strlen. */
                *size_ptr = OSAL_strlen(value_ptr);
                return (CSM_OK);
            }
        }
    }

    return (CSM_ERR);

}
