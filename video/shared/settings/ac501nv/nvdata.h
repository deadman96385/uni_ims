/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28218 $ $Date: 2014-08-15 13:49:15 +0800 (Fri, 15 Aug 2014) $
 */

#ifndef _NVDATA_H_
#define _NVDATA_H_

#include "ims_nv_item.h"

#define NVDATA_MAX_NUMBER_LENGTH  8

typedef struct CSM_nvKeeper {
    CSM_NvData  *nvData;
    char         dataHolder[NVDATA_MAX_NUMBER_LENGTH];
} CSM_nvKeeper;

typedef struct SAPP_nvKeeper {
    SAPP_NvData *nvData;
    char         dataHolder[NVDATA_MAX_NUMBER_LENGTH];
} SAPP_nvKeeper;

typedef struct MC_nvKeeper {
    MC_NvData *nvData;
    char       dataHolder[NVDATA_MAX_NUMBER_LENGTH];
} MC_nvKeeper;

typedef struct ISIM_nvKeeper {
    ISIM_NvData *nvData;
    char         dataHolder[NVDATA_MAX_NUMBER_LENGTH];
} ISIM_nvKeeper;

#endif
