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
/* 
 * ======== HTTP_setup() ========
 * 
 * This function is called to setup http connection parameters.
 * Must be called before any http request.
*
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
 OSAL_Status HTTP_setup(
    HTTP_Obj *obj_ptr)
{
     
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_setURL() ========
 * 
 * This function is called to setup/change http URL.
 * Must be called before any http request, but after HTTP_setup().
 * Start URL with http:// or https://, and optionally prepend
 * username and passowrd.
 * For example 
 *  http://zkhan:zkhanpwd@d2fs1.hq.d2tech.com
 *  https://zkhan:zkhanpwd@d2fs1.hq.d2tech.com
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_setURL(
    HTTP_Obj *obj_ptr,
    char     *url_ptr)
{
    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_setUserInfo() ========
 *
 * This function is called to setup/change the user info for this
 * http request. Must be called before any http request, but after HTTP_setup().
 * Returns:
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_setUserInfo(
    HTTP_Obj *obj_ptr,
    char     *username_ptr,
    char     *password_ptr,
    char     *userAgent_ptr)
{
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_delete() ========
 * 
 * Sends HTTP DELETE request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_delete(
    HTTP_Obj *obj_ptr,
    char     *url_ptr)
{

    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_get() ========
 * 
 * Sends HTTP GET request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_get(
    HTTP_Obj *obj_ptr,
    char     *url_ptr)
{

    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_put() ========
 * 
 * Sends HTTP PUT request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_put(
    HTTP_Obj *obj_ptr,
    void     *buf_ptr,
    int       bufSz,
    char     *url_ptr)
{

    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_cleanup() ========
 * 
 * Deallocates all resources allocated by HTTP_setup().
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_cleanup(
    HTTP_Obj *obj_ptr)
{
 
    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_copyHeaderValue() ========
 *
 * Strip off leading and trailing whitespace from the value in the
 * given HTTP header line and copy to the supplied buffer.
 * Set buffer to an empty string if the header value
 * consists entirely of whitespace.
 * assumption: destBuf_ptr is big enough to store the value string
 * code base is from libcurl
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status  HTTP_copyHeaderValue(
    const char  *hl_ptr,
    char        *destBuf_ptr)
{
    return (OSAL_SUCCESS);
}

void HTTP_freshBuffer(
    HTTP_Obj *obj_ptr)
{
    return ;
}
