/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_xport.h>

#include <vpr_comm.h>
#include <sr.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#include "_sapp.h"
#include "mns/_mns_neg.h"
#include "_sapp_dialog.h"
#include "_sapp_reg.h"

static vint _SAPP_radioGetAddrString(
    OSAL_NetAddress *addr_ptr,
    char            *target_ptr)
{
    OSAL_NetAddress addr;
    
    /* 
     * Convert to string, note that the addr needs to be in network byte 
     * order.
     */
    addr.port = 0;
    OSAL_netAddrHton(&addr, addr_ptr);
    if (OSAL_SUCCESS != OSAL_netAddressToString((int8 *)target_ptr, &addr)) {
        /* There was an error, make no updates */
        return (SAPP_ERR);
    }
    SAPP_dbgPrintf("%s: IP Address %s\n", __FUNCTION__, target_ptr);
    return (SAPP_OK);
}

static void _SAPP_radioServiceUpdate(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr)
{
    tUaConfig       *config_ptr;
    char            *pos_ptr;
    vint             len;
    config_ptr = &service_ptr->sipConfig.config;

    /* 
     * Update the object that specifies what connection to use for UA API 
     * calls 
     */
    OSAL_netAddrPortCpy(&service_ptr->sipConfig.localConn.addr, 
            &service_ptr->sipInfc);
    service_ptr->sipConfig.localConn.fd = service_ptr->sipInfcFd;
  
    /* update the fqdn */ 
    if (NULL != (pos_ptr = OSAL_strchr(config_ptr->szFqdn, '@'))) {
        pos_ptr++;
        len = (pos_ptr - config_ptr->szFqdn);
        if ((OSAL_NET_SOCK_UDP_V6 == service_ptr->sipInfc.type) ||
                (OSAL_NET_SOCK_TCP_V6 == service_ptr->sipInfc.type)) {
            /* Bracket ipv6 address with [] */
            OSAL_snprintf(pos_ptr, (SIP_URI_STRING_MAX_SIZE - len),
                "[%s]", sip_ptr->hostname);
        }
        else {
            OSAL_snprintf(pos_ptr, (SIP_URI_STRING_MAX_SIZE - len),
                "%s", sip_ptr->hostname);
        }
    }
    
    /* Now set it in the SIP stack */
    if (service_ptr->sipConfig.uaId == 0) {
        /* Then we're done, there's no UA created for this service yet anyway */
        return;
    }

    /* Then UA is currently active so call UA_Modify() */
    UA_Modify(service_ptr->sipConfig.uaId, NULL, 0, 0,
            NULL, config_ptr->szFqdn, NULL, NULL, NULL, 20, 0);

    return;
}

void SAPP_radioInterfaceUpdate(
    SAPP_SipObj         *sip_ptr,
    SAPP_ServiceObj     *service_ptr,
    SAPP_Event          *evt_ptr,
    OSAL_NetAddress      address,
    char                *infcName)
{
    vint             y;
    char             ipAddr[MAX_IPV6_STR_LEN];
    SAPP_CallObj    *call_ptr;

    if (OSAL_netIsAddrZero(&address)) {
        /*
        * No need to send sip packet over network if there is no network.
        * Close socket for not sending packet to network.
        */
        SAPP_sipServiceTransportSwitch(service_ptr, 0);
        /* Let's take care of the service now */
        SAPP_regNoNet(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
        service_ptr->isNoNet = OSAL_TRUE;
        return;
    }
    else {
        SAPP_sipServiceTransportSwitch(service_ptr, 1);
        /* IP network */
        SAPP_regYesNet(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
        service_ptr->isNoNet = OSAL_FALSE;
    }

    if (SAPP_OK != _SAPP_radioGetAddrString(&address, ipAddr)) {
        /* can't convert to string, so ignore */
        SAPP_dbgPrintf("%s: Failed to convert ip addr. Ignore addr:%x\n",
                __FUNCTION__, address.ipv4);
        return;
    }

    OSAL_netAddrCpy(&service_ptr->sipInfc, &address);
    OSAL_netAddrCpy(&service_ptr->artpInfc.nextInfc, &address);
    OSAL_netAddrCpy(&service_ptr->vrtpInfc.nextInfc, &address);

    /* Determine network mode */
    if (0 == OSAL_strncmp(infcName, SAPP_NETWORK_INFC_NAME_LTE,
            OSAL_strlen(SAPP_NETWORK_INFC_NAME_LTE))) {
        SAPP_dbgPrintf("%s 4G mode\n", __FUNCTION__);
        service_ptr->networkMode = SAPP_NETWORK_MODE_4G;

        /* Update IM transport protocol */
        sip_ptr->msrpUseTls = sip_ptr->msrpUseTlsFor4g;
        
    }
    else if (0 == OSAL_strncmp(infcName, SAPP_NETWORK_INFC_NAME_WIFI,
            OSAL_strlen(SAPP_NETWORK_INFC_NAME_WIFI))) {
        SAPP_dbgPrintf("%s Wifi mode\n", __FUNCTION__);
        service_ptr->networkMode = SAPP_NETWORK_MODE_WIFI;

        /* Update IM transport protocol */
        sip_ptr->msrpUseTls = sip_ptr->msrpUseTlsForWifi;
    }
    else {
        SAPP_dbgPrintf("%s Unknown interface name:%s\n", __FUNCTION__,
                infcName);
    }

    /* Set network mode for SR */
    if (OSAL_SUCCESS != SR_setNetworkMode(infcName)) {
        SAPP_dbgPrintf("%s: Set network mode failed. Mode:%s\n",
            __FUNCTION__, infcName);
    }

    OSAL_snprintf(sip_ptr->hostname, SAPP_STRING_SZ, "%s", ipAddr);
    SAPP_dbgPrintf("%s: Updating all services with ip addr:%s\n",
            __FUNCTION__, ipAddr);
            
    _SAPP_radioServiceUpdate(sip_ptr, service_ptr, evt_ptr);
    /* Now update all the calls for this service */
    call_ptr = &service_ptr->sipConfig.aCall[0];

    for (y = 0 ; y < SAPP_CALL_NUM ; y++) {
        if (0 != call_ptr->isiCallId) {
            /* Update MNS session */
            MNS_ip2ipUpdate(&call_ptr->mnsSession, &address);
        }
        call_ptr++;
    }
    return;
}


/*
 * ======== _SAPP_radioUpdateCallModify() ========
 *
 * This function is called to re-INVITE w/ current session.
 *
 * Returns:
 *  None.
 */
static vint _SAPP_radioUpdateCallModify(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr)
{
    ISIP_Message     cmd;

    /* Modify call only when MNS is in active state */
    if (!MNS_isSessionActive(&call_ptr->mnsSession)) {
        return (SAPP_ERR);
    }

    /* Perform a reinvite to notify the far end of your change */
    if (SIP_OK != UA_ModifyCall(
            service_ptr->sipConfig.uaId, call_ptr->dialogId, NULL, NULL, 0,
            &call_ptr->mnsSession.session, &service_ptr->sipConfig.localConn)) {

        SAPP_dbgPrintf("%s: Could not send re-invite\n", __FUNCTION__);
        return (SAPP_ERR);
    }

    SAPP_dbgPrintf("%s: Ip2Ip handover re-INVITE sent.\n", __FUNCTION__);
    /* Set cmd to MODIFY and run MNS */
    cmd.msg.call.reason = ISIP_CALL_REASON_MODIFY;
    /* Run MNS to set correct state */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession,
            &cmd);
    call_ptr->modify = SAPP_MODIFY_OTHER;

    return (SAPP_OK);
}

static void _SAPP_radioCallIsiEvt(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    SAPP_Event      *evt_ptr,
    ISIP_CallReason  reason,
    vint             status)
{
    ISIP_Message    *isi_ptr;

    isi_ptr = &evt_ptr->isiMsg;
    /* Let's write an event to ISI which indicates the new RTP session info */
    SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
            service_ptr->protocolId, call_ptr->isiCallId,
            reason, (ISIP_Status)status, isi_ptr);

     /* Populate the session part of the ISI event */
    _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
           &isi_ptr->msg.call);

    SAPP_sendEvent(evt_ptr);
    /* Reset it for next time */
    isi_ptr->code = ISIP_CODE_INVALID;
    return;
}

/*
 * ======== SAPP_radioUpdateCall() ========
 *
 * This function is called to perform ip to ip handover.
 * This function should be called after service activated with new radio
 * interface.
 *
 * Returns:
 *  None.
 */
void SAPP_radioUpdateCall(
    SAPP_SipObj         *sip_ptr,
    SAPP_ServiceObj     *service_ptr,
    SAPP_Event          *evt_ptr)
{
    vint          i;
    SAPP_CallObj *call_ptr;

    /* Now update all the calls for this service */
    call_ptr = &service_ptr->sipConfig.aCall[0];

    for (i = 0 ; i < SAPP_CALL_NUM ; i++) {
        if (0 != call_ptr->isiCallId) {
            /* Try to Re-INVITE */
            if (SAPP_OK == _SAPP_radioUpdateCallModify(service_ptr, call_ptr)) {
                /* 
                 * Tell ISI about the change in address so the RTP stream
                 * get updated.
                 */
                _SAPP_radioCallIsiEvt(service_ptr, call_ptr, evt_ptr,
                        ISIP_CALL_REASON_MODIFY, ISIP_STATUS_INVALID);
            }
            else {
                /* Failed to modify call. Hangup the call */
                _SAPP_sipHungUp(service_ptr, call_ptr->dialogId, NULL);
                /* Populate ISI event */
                _SAPP_radioCallIsiEvt(service_ptr, call_ptr, evt_ptr,
                        ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID);
                /* Destroy the call */
                SAPP_sipDestroyCall(call_ptr);
            }
        }
        call_ptr++;
    }
    return;
}
