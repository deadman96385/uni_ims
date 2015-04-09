/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12746 $ $Date: 2010-08-13 15:11:59 -0700 (Fri, 13 Aug 2010) $
 */

#include "sip_sip.h"

#include "sip_timers.h"
#include "sip_msgcodes.h"
#include "sip_auth.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_parser_enc.h"
#include "sip_parser_dec.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_tu.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_ua.h"
#include "sip_app.h"
#include "sip_ua_error.h"

#include "_sip_helpers.h"
#include "_sip_fsm.h"

static tpfSipDispatcher _UAE_dispatcher = NULL;

vint UAE_dispatch(
    tSipHandle  hUa,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    tSipIpcMsg ipcMsg;
    tSipHandle hContext;

    /* build up the message for Inter Process Communication */
    ipcMsg.type = eSIP_IPC_ERROR_MSG;
    ipcMsg.hTransaction = hTransaction;
    ipcMsg.hOwner = hUa;
    ipcMsg.pMsg = pMsg;
    ipcMsg.id = 0xFFFFFFFF;

    if (hUa != NULL) {
        hContext = (tSipHandle)((tUa*)hUa)->taskId;
    }
    else {
        hContext = TRANS_GetContext(hTransaction);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_3,
      "_UAE_dispatch: dispatching to hContext:%x, hTransaction %x owner:%x",
      (int)hContext, (int)hTransaction, (int)hUa);

    if ((*_UAE_dispatcher)(hContext, &ipcMsg) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "_UAE_dispatch: Could not dispatch msg to hContext:%x, hTransaction:%x owner:%x",
            (int)hContext, (int)hTransaction, (int)hUa);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

static vint _UA_error(
    tSipHandle  hOwner,
    uint32      event,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    tUa *pUa;
    /* the owner in this case is a UA */
    pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UA_error: hTransaction:%x hOwner:%x pMsg:%x)",
            (int)hTransaction, (int)hOwner, (int)pMsg);

    UA_AppEvent(pUa, NULL, eUA_NIC_ERROR, NULL, NULL);

    return SIP_OK;
}

void UAE_Entry(
    tSipHandle  hOwner,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32      id)
{

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAE_Entry: we have no transaction sending pMsg:%x to UA:%x",
            (int)pMsg, (int)hOwner, 0);

    _UA_error(hOwner, eTU_DEAD, pMsg, hTransaction);

    return;
}

vint UAE_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAE_TrafficCB:", 0, 0, 0);

    UA_NicErr(pLclConn, pRmtConn);

    return (SIP_OK);
}

void UAE_RegisterDispatcher(tpfSipDispatcher pfHandler)
{
    _UAE_dispatcher = pfHandler;
    return;
}

