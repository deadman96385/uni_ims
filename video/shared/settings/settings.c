/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */

#include <osal.h>
#include "_settings.h"
#include "settings.h"

/*
 * this path can optionally be set via parameter to control where
 * the config files will exist
 */
char *SETTINGS_cfgFolderPath_ptr=NULL;

/*
 * ======== SETTINGS_cfgGetDefaultName() ========
 * Get the file name for the configuration index.
 *
 * Returns:
 *
 */
static char *_SETTINGS_cfgGetDefaultName(
    int cfgIndex)
{
    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            return SETTINGS_CSM_XML_FILE_NAME;
        case SETTINGS_TYPE_SAPP:
            return SETTINGS_SAPP_XML_FILE_NAME;
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            return SETTINGS_MC_XML_FILE_NAME;
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            return SETTINGS_ISIM_XML_FILE_NAME;
#endif
#ifdef INCLUDE_MGT
        case SETTINGS_TYPE_MGT:
            return SETTINGS_MGT_XML_FILE_NAME;
#endif
#ifdef INCLUDE_GAPP
        case SETTINGS_TYPE_GAPP:
            return SETTINGS_GAPP_XML_FILE_NAME;
#endif
#ifdef INCLUDE_RIR
        case SETTINGS_TYPE_RIR:
            return SETTINGS_RIR_XML_FILE_NAME;
#endif
        case SETTINGS_TYPE_LAST:
            return NULL;
   }
   
   return NULL;
}

/*
 * ======== SETTINGS_cfgMemAlloc() ========
 * Allocate memory for the configuration container.
 *
 * Returns:
 *
 */
void * SETTINGS_cfgMemAlloc(
    int         cfgIndex)
{
    void *cfg_ptr;
    
    cfg_ptr = _SETTINGS_cfgMemAlloc(cfgIndex);

    if (NULL == SETTINGS_cfgFolderPath_ptr) {
        SETTINGS_cfgFolderPath_ptr = SETTINGS_CONFIG_DEFAULT_FOLDER;
    }
    _SETTINGS_cfgSetDefaultPath(cfgIndex, cfg_ptr, SETTINGS_cfgFolderPath_ptr,
            _SETTINGS_cfgGetDefaultName(cfgIndex));

    return cfg_ptr;
}

/*
 * ======== SETTINGS_memFreeDoc() ========
 * Free memory for doc string and that be allocated during
 * operation.
 *
 * Returns: None
 *
 */
void SETTINGS_memFreeDoc(
    int         cfgIndex,
    void       *cfg_ptr)
{
    _SETTINGS_memFreeDoc(cfgIndex, cfg_ptr);
}

/*
 * ======== SETTINGS_stringToContainer() ========
 * This function is ued to parse the string and conver to 
 * it specific data sturcture then put into container.
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Get container successfully
 *    SETTINGS_RETURN_ERROR : Failed to conver string
 */
SETTINGS_Return SETTINGS_stringToContainer(
    int         cfgIndex,
    char       *doc_ptr,
    int         docLen,
    void       *cfg_ptr)
{
    return _SETTINGS_stringToContainer(cfgIndex, doc_ptr, docLen, cfg_ptr);
}

/*
 * ======== SETTINGS_getContainer() ========
 * This function is ued to open the configuration file,
 * parse the contect and conver to specific data sturcture
 * then put into container.
 *
 * filePath: specify the file path. if null, will use default path
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Get container successfully
 *    SETTINGS_RETURN_ERROR : Failed to get container
 */
SETTINGS_Return SETTINGS_getContainer(
    int         cfgIndex,
    const char *filePath,
    void       *cfg_ptr)
{

    /* Open file */
    if (SETTINGS_RETURN_OK != _SETTINGS_loadFile(filePath, cfg_ptr)) {
        return SETTINGS_RETURN_ERROR;
    }
    /* Parse cfg and get container. */
    if (SETTINGS_RETURN_OK != _SETTINGS_parse(cfgIndex, cfg_ptr)) {
        return SETTINGS_RETURN_ERROR;
    }

    return SETTINGS_RETURN_OK;
}

/*
 * ======== SETTINGS_getParmValue() ========
 * Get parameter value from container by input information
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* SETTINGS_getParmValue(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    return _SETTINGS_getPram(cfgIndex, nestedMode, cfg_ptr, tag_ptr,
            chdOne_ptr, chdTwo_ptr, parm_ptr);
}

/*
 * ======== SETTINGS_getParmValue() ========
 * Get attribute value from container by input information
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* SETTINGS_getAttrValue(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *attr_ptr)
{
    return _SETTINGS_getAttr(cfgIndex, nestedMode, cfg_ptr, tag_ptr,
            chdOne_ptr, chdTwo_ptr, attr_ptr);
}

