/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include "http.h"

#ifndef HTTP_DBG_LOG
#define HTTP_dbgPrintf(x, ...)
#else
#define HTTP_dbgPrintf OSAL_logMsg
#endif

/*
 * ======== HTTP_getParameterValue() ========
 * This function is used to search for parameters within a header line of http headers
 *
 * Return Values:
 *  OSAL_SUCCESS or OSAL_FAIL
 *
 */
OSAL_Status HTTP_getParameterValue(
    char         *headerLine_ptr,
    const char   *pName_ptr,
    char        **value_ptr,
    vint         *valueLen_ptr)
{
    char *end_ptr;
    char *p_ptr;
    vint len;
    OSAL_Boolean withQuote = OSAL_FALSE;
    OSAL_Boolean withEscape = OSAL_FALSE;
    OSAL_Boolean isNotEnd = OSAL_TRUE;

    /*
     * search for the parameter name specified in pName_ptr.
     * If the pName_ptr is found copy a pointer to the value.
     */

    /* Get the length of the parameter name to look for */
    len = OSAL_strlen(pName_ptr);

    /* Search for the parameter name. */
    if ((p_ptr = OSAL_strncasescan(headerLine_ptr, OSAL_strlen(headerLine_ptr),
            pName_ptr))) {
        /* Found it, Now get the value */
        p_ptr += len;
        if ('=' != *p_ptr) {
            return (OSAL_FAIL);
        }
        p_ptr += 1;
        if ('\"' == *p_ptr) {
            withQuote = OSAL_TRUE;
            p_ptr += 1;
        }
        end_ptr = *value_ptr = p_ptr;
        /* Go until the end */
        for (; (*end_ptr) && isNotEnd; end_ptr++) {
            switch (*end_ptr) {
                case '\\':
                    if (OSAL_FALSE == withEscape) {
                        /* possibly the start of an escaped quote */
                        withEscape = OSAL_TRUE;
                        continue;
                    }
                    break;
                case ',':
                    if (!withQuote) {
                        /*
                         * this signals the end of the content if we didn't get a
                         * starting quote and then we do "sloppy" parsing
                         */
                        isNotEnd = OSAL_FALSE; /* the end */
                        continue;
                    }
                    break;
                case '\r':
                case '\n':
                    isNotEnd = OSAL_FALSE; /* the end */
                    continue;
                case '\"':
                    if (!withEscape && withQuote) {
                        isNotEnd = OSAL_FALSE; /* the end */
                        continue;
                    }
                    break;
            }
            withEscape = OSAL_FALSE;
        }
        *valueLen_ptr = (end_ptr - p_ptr);
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

void HTTP_hexDump(
    const void *buf,
    int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (i != 0 && i % 32 == 0) OSAL_logMsg("\n");
        if (i % 4 == 0) OSAL_logMsg(" ");
        OSAL_logMsg("%02x", ((const unsigned char *)buf)[i]);
    }
#if 1
    if (i % 32 != 0) OSAL_logMsg("\n");
#endif

    return;
}


/*
 * ======== HTTP_urlToHostPortPath() ========
 *
 * Convert url into host/port/path.
 * based on yahoo2 httplib.
 * caller provide buffers with enough size
 *
 * Returns:
 *  OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_urlToHostPortPath(
    const char *url,
    char *host,
    vint *port,
    char *path,
    vint *ssl)
{
    const char *urlpos = NULL;
    char *slash = NULL;
    char *colon = NULL;
    char numBuffer[8] = {0};

    /*
     * http://hostname
     * http://hostname/
     * http://hostname/path
     * http://hostname/path:foo
     * http://hostname:port
     * http://hostname:port/
     * http://hostname:port/path
     * http://hostname:port/path:foo
     * and https:// variants of the above
     */

    if (OSAL_strscan(url, "http://") == url) {
        urlpos = &url[7];
    } else if (OSAL_strscan(url, "https://") == url) {
        urlpos = &url[8];
        *ssl = 1;
    } else {
        HTTP_dbgPrintf("%s: Weird url - unknown protocol: %s", __FUNCTION__, url);
        return OSAL_FAIL;
    }

    slash = strchr(urlpos, '/');
    colon = strchr(urlpos, ':');

    if (!colon || (slash && (slash < colon))) {
        if (*ssl)
            *port = 443;
        else
            *port = 80;
    } else {
        if (slash) {
            OSAL_memCpy(numBuffer, colon+1, slash-(colon+1));
        } else {
            OSAL_strcpy(numBuffer, colon+1);
        }
        *port = OSAL_atoi(numBuffer);
    }

    if (!slash) {
        OSAL_strcpy(path, "/");
    } else {
        OSAL_strcpy(path, slash);
    }
    if (!colon) {
        OSAL_strncpy(host, urlpos, (slash-urlpos)+1); /* 1 for ending null */
        host[slash-urlpos] = '\0';
    } else {
        OSAL_strncpy(host, urlpos, (colon-urlpos)+1);
        host[colon-urlpos] = '\0';
    }
    return OSAL_SUCCESS;
}


/*
 * ======== HTTP_getKeyValuePair() ========
 * This function is used to search for parsing key-value within a header line
 * of http headers
 *
 * Return Values:
 *  OSAL_SUCCESS or OSAL_FAIL
 *
 */
OSAL_Status HTTP_getKeyValuePair(
    char  *str,
    char **keyStr_ptr,
    vint  *keyLen_ptr,
    char **valueStr_ptr,
    vint  *valueLen_ptr,
    char **endStr_ptr /* starting of next key pair */
    )
{
    OSAL_Boolean withQuote = OSAL_FALSE;
    OSAL_Boolean withEscape = OSAL_FALSE;
    OSAL_Boolean isNotEnd = OSAL_TRUE;

    /* parse key=value and copy pointers to the starting addr of key/value. */
    *keyStr_ptr = str;
    *keyLen_ptr = 0;
    while (*str && (*str != '=')) {
        str += 1;
    }
    if('=' != *str) {
        /* something wrong */
        return OSAL_FAIL;
    }
    *keyLen_ptr = (str - *keyStr_ptr);

    /* parsed key, now get the content value */
    str += 1;
    *valueStr_ptr = str;
    *valueLen_ptr = 0;
    if ('\"' == *str) {
        withQuote = OSAL_TRUE;
        str += 1;
    }
    for (; (*str) && isNotEnd; str++) {
        switch (*str) {
            case '\\':
                if (OSAL_FALSE == withEscape) {
                    /* possibly the start of an escaped quote */
                    withEscape = OSAL_TRUE;
                    continue;
                }
                break;
            case ',':
                if (!withQuote) {
                    /*
                     * this signals the end of the content if we didn't get a
                     * starting quote and then we do "sloppy" parsing
                     */
                    isNotEnd = OSAL_FALSE; /* the end */
                    continue;
                }
                break;
            case '\r':
            case '\n':
                isNotEnd = OSAL_FALSE; /* the end */
                continue;
            case '\"':
                if (!withEscape && withQuote) {
                    isNotEnd = OSAL_FALSE; /* the end */
                    continue;
                }
                break;
        }
        withEscape = OSAL_FALSE;
    }
    *valueLen_ptr = (str - *valueStr_ptr);
    *endStr_ptr = str+1;
    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_getKeyValuePair() ========
 * This function is used to search for parsing key-value within a header line
 * of http headers
 *
 * Return Values:
 *  OSAL_SUCCESS or OSAL_FAIL
 *
 */
OSAL_Status HTTP_decodeKeyValuePair(
    char    *keyStr,
    vint    keyLen,
    char    *valueStr,
    vint    valueLen,
    char    *token,
    char    *buffer,
    vint    bufferLen
    )
{
    if ((0 == OSAL_strncmp(keyStr, token, keyLen)) &&
            keyLen == OSAL_strlen(token)) {
        if (valueLen > (bufferLen-1)) {
            HTTP_dbgPrintf("%s: !! params %s value len exceed buffer %d", __FUNCTION__,
                    token, valueLen);
            valueLen = bufferLen-1;
        }
        OSAL_memCpy(buffer, valueStr, valueLen);
        buffer[valueLen] = '\0';
        return (OSAL_SUCCESS);
    } else {
        return (OSAL_FAIL);
    }
}
