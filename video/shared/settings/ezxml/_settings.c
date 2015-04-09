/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */

#include <osal.h>
#include "../_settings.h"
#include "_settings_parm.h"
#include <ezxml_data.h>
#include <ezxml_mem.h>
#include <ezxml.h>

/*
 * ======== _SETTINGS_cfgMemAlloc() ========
 * Allocate memory for the configuration container.
 * In ezxml, the type of container is ezxml_t
 *
 * Returns: The memory allocated for cfg container
 *
 */
void * _SETTINGS_cfgMemAlloc(
    int          cfgIndex)
{
    /* Ignore index in ezxml */
    void *cfg_ptr;

    if (NULL != (cfg_ptr = OSAL_memAlloc(sizeof(EZXML_data),
            OSAL_MEM_ARG_DYNAMIC_ALLOC))) {
        OSAL_memSet(cfg_ptr, 0, sizeof(EZXML_data));
    }

    return (cfg_ptr);
}

/*
 * ======== _SETTINGS_memFreeDoc() ========
 * Free memory for doc string and that be allocated during
 * operating of ezxml.
 *
 * Returns: None
 *
 */
void _SETTINGS_memFreeDoc(
    int          cfgIndex,
    void        *cfg_ptr)
{
    if (NULL == cfg_ptr) {
        return;
    }

    if (NULL != ((EZXML_data *)cfg_ptr)->data) {
        ezxml_free(((EZXML_data *)cfg_ptr)->data);
    }

    if (NULL != ((EZXML_data *)cfg_ptr)->doc_ptr) {
        EZXML_memFree(((EZXML_data *)cfg_ptr)->doc_ptr,
                OSAL_MEM_ARG_DYNAMIC_ALLOC);
    }

    OSAL_memFree(cfg_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
}

/*
 * ======== _SETTINGS_stringToContainer() ========
 * This function is ued to parse the string and conver to
 * it specific data sturcture then put into container.
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Get container successfully
 *    SETTINGS_RETURN_ERROR : Failed to conver string
 */
SETTINGS_Return _SETTINGS_stringToContainer(
    int         cfgIndex,
    char       *doc_ptr,
    int         docLen,
    void       *cfg_ptr)
{
    if (NULL == cfg_ptr) {
        return SETTINGS_RETURN_ERROR;
    }

    ((EZXML_data *)cfg_ptr)->data = ezxml_parse_str(doc_ptr, docLen);

    if (NULL == ((EZXML_data *)cfg_ptr)->data) {
        return SETTINGS_RETURN_ERROR;
    }

    SETTINGS_dbgPrintf("%s:%d Parse XML is OK\n", __FUNCTION__, __LINE__);

    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_loadFile() ========
 * Open the xml file and load the content to a string pointer
 *
 * filePath: specify the file path. if null, will use default path
 * cfg_ptr: the config data storage in ram after reading
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Open xml file successfully
 *    SETTINGS_RETURN_ERROR : Failed to load xml file
 */
SETTINGS_Return _SETTINGS_loadFile(
    const char  *filePath,
    void        *cfg_ptr)
{
    if (NULL == cfg_ptr) {
        return SETTINGS_RETURN_ERROR;
    }

    if (NULL == filePath) {
        filePath = (const char *)((EZXML_data *)cfg_ptr)->cfgFilePathName;
    }

    if (0 == ezxml_alloc_str(filePath, &(((EZXML_data *)cfg_ptr)->doc_ptr),
            &(((EZXML_data *)cfg_ptr)->docLen))) {
        SETTINGS_dbgPrintf("%s:%d Reading XML from file system\n",
                __FUNCTION__, __LINE__);
        return SETTINGS_RETURN_OK;
    }

    return SETTINGS_RETURN_ERROR;
}

/*
 * ======== _SETTINGS_parse() ========
 * Parse the config string and conver it to specific data
 * structure. In ezxml, it's ezxml_t.
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Open xml file successfully
 *    SETTINGS_RETURN_ERROR : Failed to load xml file
 */
SETTINGS_Return _SETTINGS_parse(
    int   cfgIndex,
    void *cfg_ptr)
{
    if (NULL == cfg_ptr) {
        return SETTINGS_RETURN_ERROR;
    }

    ((EZXML_data *)cfg_ptr)->data = ezxml_parse_str(
            ((EZXML_data *)cfg_ptr)->doc_ptr,
            ((EZXML_data *)cfg_ptr)->docLen);

    if (NULL == ((EZXML_data *)cfg_ptr)->data) {
        return SETTINGS_RETURN_ERROR;
    }

    SETTINGS_dbgPrintf("%s:%d Parse XML is OK\n", __FUNCTION__, __LINE__);

    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_getPram() ========
 * Get parameter value from container by input information.
 * This function uses nestedMode to determine which ezxml
 * function be called.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getPram(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (SETTINGS_NESTED_ONE == nestedMode) {
        return _SETTINGS_getNestedParmValue(((EZXML_data *)cfg_ptr)->data,
                tag_ptr, chdOne_ptr, parm_ptr);
    }
    else if (SETTINGS_NESTED_TWO == nestedMode) {
        return _SETTINGS_get2NestedParmValue(((EZXML_data *)cfg_ptr)->data,
                tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
    }

    return _SETTINGS_getParmValue(((EZXML_data *)cfg_ptr)->data,
            tag_ptr, parm_ptr);
}

/*
 * ======== _SETTINGS_getAttr() ========
 * Get attribute value from container by input information.
 * This function uses nestedMode to determine which ezxml
 * function be called.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _SETTINGS_getAttr(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *attr_ptr)
{
    if (SETTINGS_NESTED_ONE == nestedMode) {
        /* No requirement so far. Return null */
        return NULL;
    }
    else if (SETTINGS_NESTED_TWO == nestedMode) {
        return _SETTINGS_get2NestedTagAttribute(((EZXML_data *)cfg_ptr)->data,
                tag_ptr, chdOne_ptr, chdTwo_ptr, attr_ptr);
    }

    return _SETTINGS_getTagAttribute(((EZXML_data *)cfg_ptr)->data,
            tag_ptr, attr_ptr);
}

/*
 * ======== _SETTINGS_cfgSetDefaultPath() ========
 * For file based settting storage, this will setup the folder for the files,
 * and file name for config specified by cfgIndex/cfg_ptr. 
 * These settings are transitional, stored in cfg_ptr buffer, and freed 
 * as the same time as cfg_ptr.
 *
 * Returns:
 *    N/A
 */
void _SETTINGS_cfgSetDefaultPath(
    int         cfgIndex,
    void       *cfg_ptr,
    const char *cfgFolderPath_ptr,
    const char *cfgFileName_ptr)
{
    /* for file based config storage, this will copy the file folder/name */
    OSAL_snprintf( ((EZXML_data *)cfg_ptr)->cfgFilePathName,
            OSAL_MSG_PATH_NAME_SIZE_MAX,
            "%s/%s",
            cfgFolderPath_ptr,
            cfgFileName_ptr);
    return;
}
