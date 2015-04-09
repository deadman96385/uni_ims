/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */

#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#ifndef SETTINGS_DEBUG
#define SETTINGS_dbgPrintf(fmt, args...)
#else
#define SETTINGS_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

#include "settings.h"

void * _SETTINGS_cfgMemAlloc(
    int          cfgIndex);

void _SETTINGS_memFreeDoc(
    int          cfgIndex,
    void         *cfg_ptr);

SETTINGS_Return _SETTINGS_stringToContainer(
    int         cfgIndex,
    char       *doc_ptr,
    int         docLen,
    void       *cfg_ptr);

SETTINGS_Return _SETTINGS_loadFile(
    const char  *filePath,
    void        *cfg_ptr);

SETTINGS_Return _SETTINGS_parse(
    int   cfgIndex,
    void *cfg_ptr);

char* _SETTINGS_getPram(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getAttr(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *attr_ptr);

void _SETTINGS_cfgSetDefaultPath(
    int         cfgIndex,
    void       *cfg_ptr,
    const char *cfgFolderPath_ptr,
    const char *cfgFileName_ptr);

#endif
