/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */

#ifndef _EZXML_DATA_H_
#define _EZXML_DATA_H_

#include <ezxml.h>

typedef struct EZXML_data {
    ezxml_t   data;
    char     *doc_ptr;
    int       docLen;
    char     cfgFilePathName[OSAL_MSG_PATH_NAME_SIZE_MAX];
} EZXML_data;

#endif
