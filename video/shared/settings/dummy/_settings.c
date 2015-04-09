/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#include <osal.h>
#include "_settings_parm.h"
#include "../_settings.h"

/*
 * ======== _SETTINGS_cfgMemAlloc() ========
 *
 * Returns: The memory allocated for cfg container
 *
 */
void * _SETTINGS_cfgMemAlloc(
    int          cfgIndex)
{
    return NULL;
}

/*
 * ======== _SETTINGS_memFreeDoc() ========
 *
 * Returns: None
 *
 */
void _SETTINGS_memFreeDoc(
    int          cfgIndex,
    void        *cfg_ptr)
{
    return;
}

/*
 * ======== _SETTINGS_stringToContainer() ========
 * dummy doesn't support this function
 *
 * Returns:
 *    SETTINGS_RETURN_ERROR    : Always return this value
 */
SETTINGS_Return _SETTINGS_stringToContainer(
    int         cfgIndex,
    char       *doc_ptr,
    int         docLen,
    void       *cfg_ptr)
{
    return SETTINGS_RETURN_ERROR;
}

/*
 * ======== _SETTINGS_loadFile() ========
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Always return this value
 */
SETTINGS_Return _SETTINGS_loadFile(
    const char  *filePath,
    void        *cfg_ptr)
{
    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_parse() ========
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Always return this value
 */
SETTINGS_Return _SETTINGS_parse(
    int   cfgIndex,
    void *cfg_ptr)
{
    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_getParm() ========
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
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
    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            return _SETTINGS_getCsmParmValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
        case SETTINGS_TYPE_SAPP:
            return _SETTINGS_getSappParmValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            return _SETTINGS_getMcParmValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            return _SETTINGS_getIsimParmValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
#ifdef INCLUDE_GAPP
        case SETTINGS_TYPE_GAPP:
            return _SETTINGS_getGappParmValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
        default:
            break;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getAttr() ========
 * Get attribute value from container by input information.
 * This function uses cfgIndex to determine which type of
 * attribute will be used. CSM, SAPP, MC or ISIM.
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
    const char *parm_ptr)
{
    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            return _SETTINGS_getCsmAttrValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
        case SETTINGS_TYPE_SAPP:
            return _SETTINGS_getSappAttrValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            return _SETTINGS_getMcAttrValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            return _SETTINGS_getIsimAttrValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
#ifdef INCLUDE_GAPP
        case SETTINGS_TYPE_GAPP:
            return _SETTINGS_getGappAttrValue(nestedMode, cfg_ptr,
                    tag_ptr, chdOne_ptr, chdTwo_ptr, parm_ptr);
#endif
        default:
            break;
    }

    return NULL;
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
    /* for dummy config storage, this done nonthing */
    return;
}

