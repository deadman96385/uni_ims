/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

//#define DEBUG 
//#define VERBOSE_DEBUG

#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_debug.h"

#ifdef SIP_DEBUG_LOG    /* Entire FILE encapsulated under this */

static tpfSipDebugCB _SIP_pfDebugCallBack = 0;


/*
 * Do not edit the message level in this table.  Instead, see example of
 * calls made to
 * SIP_SetDebugLogCallBack() in _SAPP_sipInit() in file sapp_main.c
 */
static tDebugEntry _SIP_debugLevels[] =
{
    {   DEFAULT_MSG_LEVEL,  SIP_DB_TRANS_MODULE,        "xact"       },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_ENCODE_MODULE,       "enc"        },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_DECODE_MODULE,       "dec"        },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_SDP_ENC_MODULE,      "sdpEnc"     },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_SDP_DEC_MODULE,      "sdpDec"     },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_MEMORY_MODULE,       "mem"        },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_TU_MODULE,           "tu"         },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_TRANSPORT_MODULE,    "xport"      },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_TIMER_MODULE,        "Timer"      },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_GENERAL_MODULE,      "Gen"        },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_CONFIG_MODULE,       "Config"     },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_UA_MODULE,           "UA"         },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_SESSION_MODULE,      "Session"    },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_DIALOG_MODULE,       "Dialog"     },
    {   DEFAULT_MSG_LEVEL,  SIP_DB_APP_MODULE,          "App"        },
};

void SIP_DebugSetLogCallBack(tpfSipDebugCB pfCallBack)
{
    _SIP_pfDebugCallBack = pfCallBack;
}

tpfSipDebugCB SIP_DebugGetLogCallBack(void)
{
    return(_SIP_pfDebugCallBack);
}

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
    uint32 level)
{
#ifdef VERBOSE_DEBUG 
    OSAL_logMsg("changing debug level for module %d from %d to %d\n", 
        (int)module, (int)_SIP_debugLevels[module].level, (int)level);
#endif

    if (module >= SIP_DB_LAST_MODULE) {
        /* then set them all */
        int x;
        int tableSize = sizeof(_SIP_debugLevels)/sizeof(_SIP_debugLevels[0]);
        for (x = 0 ; x < tableSize ; x++) {
            if (level > 3) level = 3;
            _SIP_debugLevels[x].level = level;
        }
    }
    else {
#ifdef VERBOSE_DEBUG 
        if (level != _SIP_debugLevels[module].level) 
            OSAL_logMsg("changing debug level for module %s from %d to %d\n", 
                _SIP_debugLevels[module].moduleName,
                (int)_SIP_debugLevels[module].level, (int)level);
#endif
        if (level > 3) 
            level = 3;
        _SIP_debugLevels[module].level = level;
    }
}


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
 * arg1 = An argument to be passed to be included in the logging
 *
 * arg2 = An argument to be passed to be included in the logging
 *
 * arg3 = An argument to be passed to be included in the logging
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_DebugLog(
    uint32 filter,
    char   *pStr,
    uint32  arg1,
    uint32  arg2,
    uint32  arg3)
{
    int cnt = 0;
    uint32 value;

    if (!_SIP_pfDebugCallBack) {
        return;
    }

    /* check if log is allowed */
    while (filter) {
        value = filter & 0x000000ffL;
        if (value && (value <= _SIP_debugLevels[cnt].level)) {
            _SIP_pfDebugCallBack(pStr, arg1, arg2, arg3);
            return;
        }
        filter >>= 2;
        cnt++;
    } /* end of while loop */
    return;
}

/* 
 * ================SIP_DebugLogAddr===================
 *
 * This function is used to print debug on IP address
 *
 * filter = This is the debugging level at which it is desirable to print the 
 * debug message.  If the filter is less than or equal to the log level set for the 
 * particular module, the messagw will be logged.
 *
 * addr_ptr = A pointer of OSAL_NetAddres.
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_DebugLogAddr(
    uint32 filter,
    OSAL_NetAddress *addr_ptr)
{
    char str[OSAL_NET_IPV6_STR_MAX];

    OSAL_netAddressToString((int8 *)str, addr_ptr);
    SIP_DebugLog(filter,"Ip:%s, port:%d", (uint32)str, 
            OSAL_netNtohs(addr_ptr->port), 0);
}
#endif

