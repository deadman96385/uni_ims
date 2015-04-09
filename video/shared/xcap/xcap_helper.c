/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19200 $ $Date: 2012-12-10 12:41:44 +0800 (Mon, 10 Dec 2012) $
 *
 */

#include <osal.h>
#include "xcap_resources.h"

/* 
 * ======== XCAP_helperMakeUri() ========
 * XXX: Make this function efficient and shorten its length.
 */
int XCAP_helperMakeUri(
    char *username_ptr,
    char *password_ptr,
    char *root_ptr,
    char *auid_ptr,
    char *folder_ptr,
    char *xui_ptr,
    char *doc_ptr,
    char *node_ptr,
    char *dst_ptr,
    int   dstSz)
{
    int    len;
    int    ch;
    int    https;
    int    auth;
    int    node;

    /*
     * NULL check all required.
     */
    if ((NULL == root_ptr) || (NULL == dst_ptr) || (NULL == xui_ptr)) {
        return (0);
    }

    dst_ptr[0] = 0;

    /*
     * Type check.
     */

    /*
     * Default doc if none given
     */
    if (NULL == doc_ptr) {
        doc_ptr = XCAP_INDEX_XML;
    }

    /*
     * Default folder is users
     */
    if ((XCAP_USERS != folder_ptr) && (XCAP_GLOBAL != folder_ptr)) {
        return (0);
    }

    /*
     * Only allow selective applications.
     */
    if ((XCAP_RLS_SERVICES != auid_ptr) && (XCAP_PRES_RULES != auid_ptr) && 
            (XCAP_RESOURCE_LISTS != auid_ptr) && 
            (XCAP_ETSI_SIMSERVS_AUID != auid_ptr)&&
            (XCAP_SIMSERVS_AUID != auid_ptr)) {
        return (0);
    }

    /*
     * prototcol check http or https.
     */
    if (0 == OSAL_strncmp(root_ptr,
            XCAP_HTTPS, OSAL_strlen(XCAP_HTTPS))) {
        https = 1;
    }
    else if (0 == OSAL_strncmp(root_ptr,
            XCAP_HTTP, OSAL_strlen(XCAP_HTTP))) {
        https = 0;
    }
    else {
        return (0);
    }

    /*
     * check if username, password need to be added.
     */
    auth = (NULL != username_ptr && 0 != *username_ptr) && 
            (NULL != password_ptr && 0 != *password_ptr);
    
    /*
     * check if node needs to be added.
     */
    node = (NULL != node_ptr && 0 != *node_ptr);

    /*
     * Store in dst_ptr, all the way to node separator.
     * Skip node separator when node selector is not given (NULL)
     */
    if (!node) {
        if (auth) {
            if (https) {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s:%s@%s/%s/%s/%s/%s",
                        XCAP_HTTPS,
                        username_ptr,
                        password_ptr,
                        root_ptr + OSAL_strlen(XCAP_HTTPS),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            else {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s:%s@%s/%s/%s/%s/%s",
                        XCAP_HTTP,
                        username_ptr,
                        password_ptr,
                        root_ptr + OSAL_strlen(XCAP_HTTP),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
        }
        else {
            if (https) {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s/%s/%s/%s/%s",
                        XCAP_HTTPS,
                        root_ptr + OSAL_strlen(XCAP_HTTPS),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            else {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s/%s/%s/%s/%s",
                        XCAP_HTTP,
                        root_ptr + OSAL_strlen(XCAP_HTTP),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            
        }
        if (len >= dstSz) {
            /*
             * Buffer too small.
             */
            return (0);
        }
        dst_ptr[len] = 0;
        return (len);
    }
    else {
        if (auth) {
            if (https) {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s:%s@%s/%s/%s/%s/%s/~~/",
                        XCAP_HTTPS,
                        username_ptr,
                        password_ptr,
                        root_ptr + OSAL_strlen(XCAP_HTTPS),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            else {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s:%s@%s/%s/%s/%s/%s/~~/",
                        XCAP_HTTP,
                        username_ptr,
                        password_ptr,
                        root_ptr + OSAL_strlen(XCAP_HTTP),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
        }
        else {
            if (https) {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s/%s/%s/%s/%s/~~/",
                        XCAP_HTTPS,
                        root_ptr + OSAL_strlen(XCAP_HTTPS),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            else {
                len = OSAL_snprintf(dst_ptr,
                        dstSz,
                        "%s%s/%s/%s/%s/%s/~~/",
                        XCAP_HTTP,
                        root_ptr + OSAL_strlen(XCAP_HTTP),
                        auid_ptr,
                        folder_ptr,
                        xui_ptr,
                        doc_ptr);
            }
            
        }
        
        if (len >= dstSz) {
            /*
             * Buffer too small.
             */
            return (0);
        }
        dst_ptr[len] = 0;

        /*
         * Escape encode node selector and concatenate to URI.
         */
        while (*node_ptr) {
            if ((len + 4) > dstSz) {
                /*
                 * Buffer too small.
                 */
                return (0);
            }
            ch = *node_ptr++;
            /*
             * XXX: Add more later when required as in RFC 4825
             */
            switch (ch) {
                case '[':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '5';
                    dst_ptr[len + 2] = 'B';
                    len += 3;
                    break;
                case '~':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '7';
                    dst_ptr[len + 2] = 'E';
                    len += 3;
                    break;
                case '\'':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '2';
                    dst_ptr[len + 2] = '7';
                    len += 3;
                    break;
                case '?':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '3';
                    dst_ptr[len + 2] = 'F';
                    len += 3;
                    break;
                case ']':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '5';
                    dst_ptr[len + 2] = 'D';
                    len += 3;
                    break;
                case '"':
                    dst_ptr[len + 0] = '%';
                    dst_ptr[len + 1] = '2';
                    dst_ptr[len + 2] = '2';
                    len += 3;
                    break;
                default:
                    dst_ptr[len + 0] = ch;
                    len += 1;
                    break;
            }
        }

        dst_ptr[len] = 0;
        return (len);
    }

}
