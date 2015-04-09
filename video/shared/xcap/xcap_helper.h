/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief Generic helper functions.
 *  
 * API in this file is used to do operations on data.
 */

#ifndef _XCAP_HELPER_H_
#define _XCAP_HELPER_H_

/*
 * Function prototypes.
 */

/** \fn int XCAP_helperMakeUri(
 *          char *username_ptr,
 *          char *password_ptr,
 *          char *root_ptr,
 *          char *auid_ptr,
 *          char *folder_ptr,
 *          char *xui_ptr,
 *          char *doc_ptr,
 *          char *node_ptr,
 *          char *dst_ptr,
 *          int   dstSz)
 * \brief URI construction helper function.
 *
 * This function creates a HTTP URI from various XCAP path elements.\n
 * Note: node URI is automatically escaped, hence [uri="something"] will come
 * out as %5buri=%22something%22%5d.
 * @param username_ptr Pointer to a NULL terminated string containing password
 * (optional, can be NULL)
 * @param password_ptr Pointer to a NULL terminated string containing username
 * (optional, can be NULL)
 * @param root_ptr Pointer to a NULL terminated string containing XCAP root
 * (mandatory)
 * @param auid_ptr Set to one of #XCAP_RLS_SERVICES, #XCAP_PRES_RULES, or 
 * #XCAP_RESOURCE_LISTS
 * (mandatory)
 * @param folder_ptr Set to #XCAP_USERS or #XCAP_GLOBAL
 * (mandatory)
 * @param xui_ptr Pointer to a NULL terminated string containing XUI
 * (mandatory)
 * @param doc_ptr Pointer to NULL terminated string containing name of document,
 * or set to NULL to use index.xml
 * (optional, can be NULL)
 * @param node_ptr Pointer to NULL terminated string containg location of
 * node with in the document, or set to NULL for no node
 * (optional, can be NULL)
 * @param dst_ptr A buffer where URI will be constructed
 * (mandatory)
 * @param dstSz Size of buffer pointed by dst_ptr, if size is small function
 * will return with a failure
 * @return 0: failed, >0: passed (string is NULL terminated) and return value 
 * is length of string
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
    int   dstSz);

#endif
