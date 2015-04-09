/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SIP_DEBUG_H_
#define _SIP_DEBUG_H_

#include "sip_port.h"

enum {
   SIP_DB_TRANS_MODULE,
   SIP_DB_ENCODE_MODULE,
   SIP_DB_DECODE_MODULE,
   SIP_DB_SDP_ENC_MODULE,
   SIP_DB_SDP_DEC_MODULE,
   SIP_DB_MEMORY_MODULE,
   SIP_DB_TU_MODULE,
   SIP_DB_TRANSPORT_MODULE,
   SIP_DB_TIMER_MODULE,
   SIP_DB_GENERAL_MODULE,
   SIP_DB_CONFIG_MODULE,
   SIP_DB_UA_MODULE,
   SIP_DB_SESSION_MODULE,
   SIP_DB_DIALOG_MODULE,
   SIP_DB_APP_MODULE,
   SIP_DB_LAST_MODULE
};

#define SIP_DB_ALL_MODULES      (255)

#define SIP_DB_TRANS_LVL_1     (1 << (2 * SIP_DB_TRANS_MODULE))
#define SIP_DB_TRANS_LVL_2     (2 << (2 * SIP_DB_TRANS_MODULE))
#define SIP_DB_TRANS_LVL_3     (3 << (2 * SIP_DB_TRANS_MODULE))

#define SIP_DB_ENCODE_LVL_1    (1 << (2 * SIP_DB_ENCODE_MODULE))
#define SIP_DB_ENCODE_LVL_2    (2 << (2 * SIP_DB_ENCODE_MODULE))
#define SIP_DB_ENCODE_LVL_3    (3 << (2 * SIP_DB_ENCODE_MODULE))

#define SIP_DB_DECODE_LVL_1    (1 << (2 * SIP_DB_DECODE_MODULE))
#define SIP_DB_DECODE_LVL_2    (2 << (2 * SIP_DB_DECODE_MODULE))
#define SIP_DB_DECODE_LVL_3    (3 << (2 * SIP_DB_DECODE_MODULE))

#define SIP_DB_SDP_DEC_LVL_1   (1 << (2 * SIP_DB_SDP_DEC_MODULE))
#define SIP_DB_SDP_DEC_LVL_2   (2 << (2 * SIP_DB_SDP_DEC_MODULE))
#define SIP_DB_SDP_DEC_LVL_3   (3 << (2 * SIP_DB_SDP_DEC_MODULE))

#define SIP_DB_SDP_ENC_LVL_1   (1 << (2 * SIP_DB_SDP_ENC_MODULE))
#define SIP_DB_SDP_ENC_LVL_2   (2 << (2 * SIP_DB_SDP_ENC_MODULE))
#define SIP_DB_SDP_ENC_LVL_3   (3 << (2 * SIP_DB_SDP_ENC_MODULE))

#define SIP_DB_MEMORY_LVL_1    (1 << (2 * SIP_DB_MEMORY_MODULE))
#define SIP_DB_MEMORY_LVL_2    (2 << (2 * SIP_DB_MEMORY_MODULE))
#define SIP_DB_MEMORY_LVL_3    (3 << (2 * SIP_DB_MEMORY_MODULE))

#define SIP_DB_TU_LVL_1        (1 << (2 * SIP_DB_TU_MODULE))
#define SIP_DB_TU_LVL_2        (2 << (2 * SIP_DB_TU_MODULE))
#define SIP_DB_TU_LVL_3        (3 << (2 * SIP_DB_TU_MODULE))

#define SIP_DB_TRANSPORT_LVL_1 (1 << (2 * SIP_DB_TRANSPORT_MODULE))
#define SIP_DB_TRANSPORT_LVL_2 (2 << (2 * SIP_DB_TRANSPORT_MODULE))
#define SIP_DB_TRANSPORT_LVL_3 (3 << (2 * SIP_DB_TRANSPORT_MODULE))

#define SIP_DB_TIMER_LVL_1     (1 << (2 * SIP_DB_TIMER_MODULE))
#define SIP_DB_TIMER_LVL_2     (2 << (2 * SIP_DB_TIMER_MODULE))
#define SIP_DB_TIMER_LVL_3     (3 << (2 * SIP_DB_TIMER_MODULE))

#define SIP_DB_GENERAL_LVL_1   (1 << (2 * SIP_DB_GENERAL_MODULE))
#define SIP_DB_GENERAL_LVL_2   (2 << (2 * SIP_DB_GENERAL_MODULE))
#define SIP_DB_GENERAL_LVL_3   (3 << (2 * SIP_DB_GENERAL_MODULE))

#define SIP_DB_CONFIG_LVL_1    (1 << (2 * SIP_DB_CONFIG_MODULE))
#define SIP_DB_CONFIG_LVL_2    (2 << (2 * SIP_DB_CONFIG_MODULE))
#define SIP_DB_CONFIG_LVL_3    (3 << (2 * SIP_DB_CONFIG_MODULE))

#define SIP_DB_UA_LVL_1    (1 << (2 * SIP_DB_UA_MODULE))
#define SIP_DB_UA_LVL_2    (2 << (2 * SIP_DB_UA_MODULE))
#define SIP_DB_UA_LVL_3    (3 << (2 * SIP_DB_UA_MODULE))

#define SIP_DB_SESSION_LVL_1    (1 << (2 * SIP_DB_SESSION_MODULE))
#define SIP_DB_SESSION_LVL_2    (2 << (2 * SIP_DB_SESSION_MODULE))
#define SIP_DB_SESSION_LVL_3    (3 << (2 * SIP_DB_SESSION_MODULE))

#define SIP_DB_DIALOG_LVL_1    (1 << (2 * SIP_DB_DIALOG_MODULE))
#define SIP_DB_DIALOG_LVL_2    (2 << (2 * SIP_DB_DIALOG_MODULE))
#define SIP_DB_DIALOG_LVL_3    (3 << (2 * SIP_DB_DIALOG_MODULE))

#define SIP_DB_APP_LVL_1    (1 << (2 * SIP_DB_APP_MODULE))
#define SIP_DB_APP_LVL_2    (2 << (2 * SIP_DB_APP_MODULE))
#define SIP_DB_APP_LVL_3    (3 << (2 * SIP_DB_APP_MODULE))

typedef struct sDebugEntry
{
    uint32      level;
    uint32      bitLocation;
    const char *moduleName;
} tDebugEntry;

typedef void (*tpfSipDebugCB)(char *, uint32, uint32, uint32);

#ifndef SIP_DEBUG_LOG
#define SIP_DebugLog(arg,arg1,arg2,arg3,arg4)
#define SIP_DebugLogAddr(filter, addr_ptr);
#define SIP_DebugExit()
#define SIP_DebugSetLevel(module,level)
#define SIP_DebugSetLogCallBack(arg)
#define SIP_DebugGetLogCallBack() (0)
#else
void SIP_DebugSetLogCallBack(tpfSipDebugCB pfCallBack);
tpfSipDebugCB SIP_DebugGetLogCallBack(void);

/* 
 *****************************************************************************
 * ================SIP_DebugSetLevel===================
 *
 * This function sets the debug level for the module specified in 'module'.
 * The higher the level the more in-depth logging is performed.
 *
 * module = A value representing the module that is being set.  These values 
 *          are defined in sipDebug.h
 *
 * level = the level of debugging valid values are (0 - 3)
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_DebugSetLevel(
    uint32 module, 
    uint32 level);

/* 
 *****************************************************************************
 * ================SIP_DebugLog===================
 *
 * This function is used to print debug information.
 *
 * filter = This is the debugging level at which it is desirable to print the 
 * debug message.  If the filter is less than or equal to the log level set for the 
 * particular module, the messagw will be logged.
 *
 * pStr = A string to include in the logging.
 *
 * arg1 = An argument to be past to be included in the logging
 *
 * arg2 = An argument to be past to be included in the logging
 *
 * arg3 = An argument to be past to be included in the logging
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_DebugLog(
    uint32  filter,
    char   *pStr,
    uint32  arg1,
    uint32  arg2,
    uint32  arg3);

void SIP_DebugLogAddr(
    uint32 filter,
    OSAL_NetAddress *addr_ptr);
#endif

#endif
