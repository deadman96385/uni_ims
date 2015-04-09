/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */


#include "sip_app.h"

#include "sip_auth.h"
#include "sip_dbase_endpt.h"
#include "sip_dbase_sys.h"
#include "sip_parser_enc.h"
#include "sip_parser_dec.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "../xport/_sip_descr.h"
#include "../xport/_sip_drvr.h"
#include "../xport/_sip_resolv.h"
#include "sip_ua_server.h"
#include "sip_ua_client.h"
#include "sip_ua_error.h"
#include "sip_ua.h"
#include "sip_timers.h"
#include "sip_dialog.h"
#include "_sip_helpers.h"
#include "_sip_rsrc.h"
#include "sip_mem_pool.h"

/* 
 *****************************************************************************
 * ================SIP_Init===================
 *
 * This function is used as a "one stop shop" to initialize the SIP stack.
 * It may be neccessary to pull module calls out of this function and move them.
 * This is perfectly allowable.  If this function is not used, the steps
 * performed in this function must still be done somewhere
 *
 * pConfig = A pointer to a config object that contains stack configuration
 *           info see sip_app.h for more.
 * RETURNS:
 *         SIP_OK:     Initialization was successful
 *         SIP_FAILED: Initialization failed possible due to memory heap 
 *                     problems
 * 
 ******************************************************************************
 */
vint SIP_Init(tSipConfig *pConfig)
{
    /* Pre-allocate whole memory for SIP stack */
    if (SIP_OK != SIP_memPoolInit()) {
        OSAL_logMsg("%s:%d SIP_FAILED\n", __FUNCTION__, __LINE__);
        return (SIP_FAILED);
    }

    SIP_initCLib(pConfig->randomGenSeed);
    SIPTIMER_Init();
    TRANS_Init();
    DIALOG_Init();
    SYSDB_Init();
    DEC_Init();
    
    UAS_SetMatch(pConfig->matchType);

    if (pConfig->pfProxy && pConfig->pProxyFqdn) {
        UAS_SetupProxy(pConfig->pfProxy, pConfig->pProxyFqdn);
        UAC_SetupProxy(pConfig->pfProxy, pConfig->pProxyFqdn);
    }

    /* Set the dispatcher used to send OSAL Queue messages between tasks */
    UAS_RegisterDispatcher(SIP_rsrcDispatcher);
    UAC_RegisterDispatcher(SIP_rsrcDispatcher);
    UAE_RegisterDispatcher(SIP_rsrcDispatcher);
    SIPTIMER_RegisterDispatcher(SIP_rsrcTmrDispatcher);

    /* Initialize the transport module */
    if (TRANSPORT_Init(UAC_TrafficCB, UAS_TrafficCB, UAE_TrafficCB, 
            pConfig->mtu) != SIP_OK) {
        DIALOG_KillModule();
        TRANS_KillModule();
        SIPTIMER_KillModule();
        SYSDB_KillModule();
        SIP_destroyCLib();
        OSAL_logMsg("%s:%d SIP_FAILED\n", __FUNCTION__, __LINE__);
        return (SIP_FAILED);
    }

    /* init the UA */
    if (UA_InitModule(pConfig->maxDialogsPerUa) != SIP_OK) {
        DIALOG_KillModule();
        TRANS_KillModule();
        TRANSPORT_KillModule();
        SIPTIMER_KillModule();    
        SYSDB_KillModule();
        SIP_destroyCLib();
        OSAL_logMsg("%s:%d SIP_FAILED\n", __FUNCTION__, __LINE__);
        return (SIP_FAILED);
    }

    
    if (SIP_OK != SIP_rsrcInit(pConfig)) {
        /* Then we failed */
        UA_KillModule(); 
        DIALOG_KillModule();
        TRANS_KillModule();
        TRANSPORT_KillModule();
        SIPTIMER_KillModule();  
        SYSDB_KillModule();
        SIP_destroyCLib();
        OSAL_logMsg("%s:%d SIP_FAILED\n", __FUNCTION__, __LINE__);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================SIP_Destroy===================
 *
 * This function is used as a "one stop shop" to destroy the SIP Stack.
 * RETURNS:
 *         SIP_OK: Always.
 * 
 ******************************************************************************
 */
vint SIP_Destroy(void)
{
    /* Stop the network interface from accepting packets */
    TRANSPORT_DescriptorSwitch(0, -1);

    /* Free all OSAL system resources used to accomidate the stack */
    SIP_rsrcDestroy();
    
    /* Free all internal objects */
    UA_KillModule(); /* Kills dialogs, timers and transports */
    DIALOG_KillModule(); 
    TRANS_KillModule(); /* Will kill some timers and transports */
    TRANSPORT_KillModule();
    SIPTIMER_KillModule();
    SYSDB_KillModule(); 
    SIP_destroyCLib();
    /* Free SIP memory pool memory */
    SIP_memPoolDestroy();
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================SIP_NetworkInterfaceSwitch()===================
 *
 * This function will disable/enable a file descriptor used for reading
 * data from the network interface thread.
 *
 * NOTE, the file descriptor must be the persistent one that is always
 * used to service the network interface thread. 
 *
 * NOTE, once this function is called by the application, the application 
 * should 'stimulate' or 'wake up' the network interface by sending it a 
 * message/signal.
 *
 * fd = The file descriptor that the user wants to enable or disable
 *
 * onOff = '1' = enable, '0' = disable
 *
 * RETURNS:
 *     SIP_OK: The file descriptor was successfully enabled/disabled
 *     SIP_NOT_FOUND: The file descriptor specified in 'fd' was invalid
 *
 ******************************************************************************
 */
vint SIP_SwitchNetworkInterface(
    OSAL_NetSockId  fd, 
    vint            onOff)
{
    return TRANSPORT_DescriptorSwitch(fd, onOff);
}

/* 
 *****************************************************************************
 * ================SIP_ReplaceUdpDefault()===================
 *
 * This function will replace the existing default UDP server interface
 * with the specified interface. Note, if there are transactions currently 
 * using the existing default interface, they wil continue to use that 
 * interface until the transaction terminates.
 *
 * newUdpFd = The file descriptor of the new UDP interface to switch to.
 *
 * newUdpPort = The port of the new interface to switch to.
 *
 * RETURNS:
 *     SIP_OK: The interface was successfully switched 
 *     SIP_FAILED: The interface was no switched.
 *
 ******************************************************************************
 */
vint SIP_ReplaceUdpDefault(
    OSAL_NetSockId    newUdpFd,
    uint16 newUdpPort)
{
    return TRANSPORT_ReplaceUdpDefault(newUdpFd, newUdpPort);
}

vint SIP_replaceServerSocket(
    OSAL_NetSockId   newFd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr)
{
    return (TRANSPORT_replaceServerSocket(newFd, pAddr, tType, nwAccess_ptr));
}


/*
 * ======== SIP_clientConnect() ========
 *
 * This function is to create a transport client and it will make connection
 * for TCP socket.
 *
 * Returns: 
 * SIP_OK: Socket connect successfully.
 * SIP_FAILED: Socket connect failed.
 */
vint SIP_clientConnect(
    tSipHandle      hUa,
    tLocalIpConn   *pLclConn,
    tTransportType  type)
{
    tUa *pUa;

    tEPDB_Entry   *pEntry;
    tUri          *pUri;

    pUa = (tUa*)hUa;
    if (pUa == NULL) {
        return (SIP_BADPARM);
    }

    /* Check if there's a proxy or outbound proxy configured.  */
    pUri = NULL;
    if ((pEntry = EPDB_Get(eEPDB_OUTBOUND_PROXY_URI, pUa->epdb)) != NULL) {
        pUri= pEntry->x.pUri;
    }
    else if ((pEntry = EPDB_Get(eEPDB_PROXY_URI, pUa->epdb)) != NULL) {
        pUri = pEntry->x.pUri;
    }
    else {
        return (SIP_FAILED);
    }

    if (NULL == TRANSPORT_ClientAlloc(pUri, pLclConn, type)) {
        return (SIP_FAILED);
    }

    return (SIP_OK);
}
    
/*
 *****************************************************************************
 * ================SIP_CloseAllConnections()===================
 *
 * This function will try to close all existing TCP/UDP connections used by SIP.
 * This functiin will not actually close the TCP/UDP connection, it just set the
 * transport descriptor to NORMAL so that the descriptor will be destroyed after
 * all the transactions which use the descriptor are all terminated.
 * This function is typically used before registration to make sure the new
 * registration will not use an bad connection which is still alive.
 *
 * RETURNS:
 *     Nothing
 *
 ******************************************************************************
 */
void SIP_CloseAllConnections(void)
{
    TRANSPORT_CloseAllConnections();
}

 /*
 *****************************************************************************
 * ================SIP_CloseConnection===================
 *
 * This function is used as a "one stop shop" to initialize the SIP stack.
 * It may be neccessary to pull module calls out of this function and move them.
 * This is perfectly allowable.  If this function is not used, the steps
 * performed in this function must still be done somewhere
 *
 * fd = The file descriptor that the user wants to close.
 *
 * RETURNS:
 *     Nothing
 *
 ******************************************************************************
 */
void SIP_CloseConnection(
    OSAL_NetSockId fd)
{
    TRANSPORT_CloseConnection(fd);
}

/*
 *****************************************************************************
 * ================SIP_getTransportType()===================
 *
 * This function is to get the transport type according to the outbound proxy or proxy
 * user configured.
 *
 * RETURNS:
 *
 ******************************************************************************
 */
tTransportType SIP_getTransportType(
    tSipHandle hUa)
{
    tUa *pUa;

    tEPDB_Entry   *pEntry;
    tUri          *pUri;
    tTransport     transport;
    tTransportType transportType;

    pUa = (tUa*)hUa;
    if (pUa == NULL) {
        return eTransportNone;
    }

    /* Check if there's a proxy or outbound proxy configured.  */
    pUri = NULL;
    if ((pEntry = EPDB_Get(eEPDB_OUTBOUND_PROXY_URI, pUa->epdb)) != NULL) {
        pUri= pEntry->x.pUri;
    }
    else if ((pEntry = EPDB_Get(eEPDB_PROXY_URI, pUa->epdb)) != NULL) {
        pUri = pEntry->x.pUri;
    }
    else {
        /* Return default transport type */
        return (eTransportUdp);
    }

    if (SIP_OK != _TRANSPORT_GetTransport(&transport, pUri, &transportType, 1)) {
        transportType = eTransportUdp;
    }

    return (transportType);
}

/* 
 *****************************************************************************
 * ================SIP_SetDebugLogLevel===================
 *
 * When debuggin is enabled by defining SIP_DEBUG_LOG in the 
 * config.mk file, the function can be used to set the debug log level for 
 * individual modules in the stack.  These enumerations are defined in sip_debug.h
 * 
 * moduleEnum = The enumeration of the module defined in sip_debug.h
 *
 * level = The level of debugging to set for the module possible values are
 *         0: debug logging off
 *         1: Errors Only
 *         2: Warnings (But not necessarily errors)
 *         3: General logging
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_SetDebugLogLevel(
    uint32 moduleEnum, 
    int    level)
{
    SIP_DebugSetLevel(moduleEnum, (uint32)level);
    return;
}

/* 
 *****************************************************************************
 * ================SIP_DebugLogApp===================
 *
 * This function provides applications the ability to print debug logging 
 * using the same mechanisms that the stack uses internally.  The log
 * levels are the same as debug log levels used within the stack
 * 
 * level = levels of debugging are...
 *         0: debug logging off
 *         1: Errors Only
 *         2: Warnings (But not necessarily errors)
 *         3: General logging
 *
 * pStr = The string to print.  This can look like a OSAL_logMsg string.
 *        i.e. "Failed: Error performing blah-blah arg1:%d arg2:%d"
 *
 * arg = arg1 - arg3 are additional arguments that can be passed in when the 
 *       pStr looks like a OSAL_logMsg function.
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_DebugLogApp(
    int      level, 
    char    *pStr, 
    uint32   arg1, 
    uint32   arg2, 
    uint32   arg3)
{
#if (SIP_DEBUG_LOG)
    uint32 lvl;
    if (level == 1)
        lvl = SIP_DB_APP_LVL_1;
    else if (level == 2)
        lvl = SIP_DB_APP_LVL_2;
    else if (level == 3)
        lvl = SIP_DB_APP_LVL_3;
    else 
        return;

    SIP_DebugLog(lvl, pStr, arg1, arg2, arg3);
#endif
    return;
}

/* 
 *****************************************************************************
 * ================SIP_SetDebugLogCallBack===================
 *
 * This function allows applications to link in a callback routine to use 
 * when the stack tries to print debug logging information.  If no function
 * is registerd with the SIP stack via this function, the stack will attempt 
 * to use OSAL_logMsg.  Otherwise the stack will perform some internal formatting
 * and then call the callback that was set using htis function.  
 * For example, let's say you have a seperate thread that you want to use to
 * logging.  Then you would regsiter a callback routine with the stack 
 * (via this function), that will perhaps dispatch the string to print
 * to the thread sepciifcally creted to perform debug loggin.
 *
 * pfCallback = A pointer to a function that will be called when the stack
 *              attempts to print.
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void SIP_SetDebugLogCallBack(
    tpfSipDebugCB pfCallback)
{
#if (SIP_DEBUG_LOG)
    SIP_DebugSetLogCallBack(pfCallback);
#endif
    return;
}

/*
 * ======== SIP_setTimers() ========
 * This function is to configure sip timers T1, T2 and T4.
 * All other timers will change as T1/T2/T4 changes.
 *
 * t1: Value of sip timer t1 in millisecond.
 * t2: Value of sip timer t2 in millisecond.
 * t4: Value of sip timer t4 in millisecond.
 *
 * Returns
 *   None.
 */
void SIP_setTimers(
    uint32 t1,
    uint32 t2,
    uint32 t4)
{
    TRANS_setTimers(t1, t2, t4);
}

#if defined(PROVIDER_CMCC)
/*
 * ======== SIP_setTregTimer() ========
 * This function is to configure sip timers Tcall which timer is defined by CMCC
 *
 * tcall: Value of sip timer Tcall in millisecond.
 *
 * Returns
 *   None.
 */
void SIP_setTcallTimer(
    uint32  tcall)
{
    TRANS_setTcallTimer(tcall);
}
#endif

/*
 * ======== SIP_updateAccessNwInfo() ========
 * Update network access info of a fd.
 *
 * Returns
 *   SIP_OK: Successul.
 *   SIP_FAILED: Failed.
 */
vint SIP_updateAccessNwInfo(
    OSAL_NetSockId  fd,
    tNwAccess      *nwAccess_ptr)
{
    return (TRANSPORT_updateAccessNwInfo(fd, nwAccess_ptr));
}
