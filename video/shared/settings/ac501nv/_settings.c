/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#include <osal.h>
#include <nvdata.h>
#include "_settings_parm.h"
#include "../_settings.h"
#include "nvm_stack_interface.h"

/*
 * ======== _SETTINGS_cfgMemAlloc() ========
 * Allocate memory for the configuration container. In ac501, there
 * are 4 different types of NV data structure. CSM/SAPP/MC/ISIM.
 * Using cfgIndex to indicate the type of container and allocate
 * memory for container with it's NV data structue correctly.
 *
 * Returns: The memory allocated for cfg container
 *
 */
void * _SETTINGS_cfgMemAlloc(
    int          cfgIndex)
{
    void *cfg_ptr;

    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            cfg_ptr = OSAL_memAlloc(sizeof(CSM_nvKeeper),
                    OSAL_MEM_ARG_DYNAMIC_ALLOC);
            break;
        case SETTINGS_TYPE_SAPP:
            cfg_ptr = OSAL_memAlloc(sizeof(SAPP_nvKeeper),
                    OSAL_MEM_ARG_DYNAMIC_ALLOC);
            break;
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            cfg_ptr = OSAL_memAlloc(sizeof(MC_nvKeeper),
                    OSAL_MEM_ARG_DYNAMIC_ALLOC);
            break;
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            cfg_ptr = OSAL_memAlloc(sizeof(ISIM_nvKeeper),
                    OSAL_MEM_ARG_DYNAMIC_ALLOC);
            break;
#endif
        default:
            cfg_ptr = NULL;
            break;
    }

    if (NULL != cfg_ptr) {
        switch(cfgIndex) {
            case SETTINGS_TYPE_CSM:
                OSAL_memSet(cfg_ptr, 0, sizeof(CSM_nvKeeper));
                break;
            case SETTINGS_TYPE_SAPP:
                OSAL_memSet(cfg_ptr, 0, sizeof(SAPP_nvKeeper));
                break;
#ifdef INCLUDE_MC
            case SETTINGS_TYPE_MC:
                OSAL_memSet(cfg_ptr, 0, sizeof(MC_nvKeeper));
                break;
#endif
#ifdef INCLUDE_ISIM
            case SETTINGS_TYPE_ISIM:
                OSAL_memSet(cfg_ptr, 0, sizeof(ISIM_nvKeeper));
                break;
#endif
            default:
                break;
        }
    }

    return (cfg_ptr);
}

/*
 * ======== _SETTINGS_memFreeDoc() ========
 * Free memory for container.
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

    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            if (NULL != ((CSM_nvKeeper *)cfg_ptr)->nvData) {
                OSAL_memFree(((CSM_nvKeeper *)cfg_ptr)->nvData,
                        OSAL_MEM_ARG_DYNAMIC_ALLOC);
            }
            break;
        case SETTINGS_TYPE_SAPP:
            if (NULL != ((SAPP_nvKeeper *)cfg_ptr)->nvData) {
                OSAL_memFree(((SAPP_nvKeeper *)cfg_ptr)->nvData,
                        OSAL_MEM_ARG_DYNAMIC_ALLOC);
            }
            break;
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            if (NULL != ((MC_nvKeeper *)cfg_ptr)->nvData) {
                OSAL_memFree(((MC_nvKeeper *)cfg_ptr)->nvData,
                        OSAL_MEM_ARG_DYNAMIC_ALLOC);
            }
            break;
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            if (NULL != ((ISIM_nvKeeper *)cfg_ptr)->nvData) {
                OSAL_memFree(((ISIM_nvKeeper *)cfg_ptr)->nvData,
                        OSAL_MEM_ARG_DYNAMIC_ALLOC);
            }
            break;
#endif
        default:
            break;

    }
    OSAL_memFree(cfg_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
}

/*
 * ======== _SETTINGS_stringToContainer() ========
 * NV RAM doesn't support this function
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
 * In ac501, no necessary to open and load config from 
 * a file. So do nothing in this function.
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Always return this value
 */
SETTINGS_Return _SETTINGS_loadFile(
    const char  *filePath,
    void        *cfg_ptr)
{
    /* Do nothing for ac501 NV ram. */
    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_parse() ========
 * In ac501, config is be stored in NV RAM with specific
 * data structure. This function will read specific NV
 * data structure from NV RAM by input cfgIndex.
 *
 * Returns:
 *    SETTINGS_RETURN_OK    : Read NV data successfully
 *    SETTINGS_RETURN_ERROR : Failed to read NV data
 */
SETTINGS_Return _SETTINGS_parse(
    int   cfgIndex,
    void *cfg_ptr)
{
    int ret;
    nvm_param_t type;
    uint32 nv_size = 0;

    switch (cfgIndex) {
        case SETTINGS_TYPE_CSM:
            ((CSM_nvKeeper *)cfg_ptr)->nvData = OSAL_memAlloc(
                    sizeof(CSM_NvData), OSAL_MEM_ARG_DYNAMIC_ALLOC);
            type = NVM_PARAM_TYPE_IMS_CSM_SETTINGS;
            ret = nvm_read(type, 0, &(((CSM_nvKeeper *)cfg_ptr)->nvData),
                    &nv_size);
            break;
        case SETTINGS_TYPE_SAPP:
            ((SAPP_nvKeeper *)cfg_ptr)->nvData = OSAL_memAlloc(
                    sizeof(SAPP_NvData), OSAL_MEM_ARG_DYNAMIC_ALLOC);
            type = NVM_PARAM_TYPE_IMS_SAPP_SETTINGS;
            ret = nvm_read(type, 0, &(((SAPP_nvKeeper *)cfg_ptr)->nvData),
                    &nv_size);
            break;
#ifdef INCLUDE_MC
        case SETTINGS_TYPE_MC:
            ((MC_nvKeeper *)cfg_ptr)->nvData = OSAL_memAlloc(
                    sizeof(MC_NvData), OSAL_MEM_ARG_DYNAMIC_ALLOC);
            type = NVM_PARAM_TYPE_IMS_MC_SETTINGS;
            ret = nvm_read(type, 0, &(((MC_nvKeeper *)cfg_ptr)->nvData),
                    &nv_size);
            break;
#endif
#ifdef INCLUDE_ISIM
        case SETTINGS_TYPE_ISIM:
            ((ISIM_nvKeeper *)cfg_ptr)->nvData = OSAL_memAlloc(
                    sizeof(ISIM_NvData), OSAL_MEM_ARG_DYNAMIC_ALLOC);
            type = NVM_PARAM_TYPE_IMS_ISIM_SETTINGS;
            ret = nvm_read(type, 0, &(((ISIM_nvKeeper *)cfg_ptr)->nvData),
                    &nv_size);
            break;
#endif
#ifdef INCLUDE_GAPP
        case SETTINGS_TYPE_GAPP:
            /* XXX */
            break;
#endif
        default:
            OSAL_logMsg("%s:%d Unsupported CFG index(%d)\n",
                __FUNCTION__, __LINE__, cfgIndex);
            return SETTINGS_RETURN_ERROR;
    }

    if (OSA_FALSE == ret) {
        OSAL_logMsg("%s:%d Failed to read NV data(%d)\n",
                __FUNCTION__, __LINE__, cfgIndex);
        return SETTINGS_RETURN_ERROR;
    }

    SETTINGS_dbgPrintf("%s:%d Getting NV data(%d) is OK, size = %d\n",
                __FUNCTION__, __LINE__, cfgIndex, nv_size);

    return SETTINGS_RETURN_OK;
}

/*
 * ======== _SETTINGS_getParm() ========
 * Get parameter value from container by input information.
 * This function uses cfgIndex to determine which type of 
 * parameter will be used. CSM, SAPP, MC or ISIM.
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
    /* for NV based config storage, this done nonthing */
    return;
}

