/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "rir_event.h"
#include "_rir.h"
#include "_rir_log.h"

/*
 * ======== _RIR_eventLog() ========
 *
 * Logs RIR events.
 * 
 * Returns:
 * 
 */
static void _RIR_eventLog(
    RIR_EventMsg *m_ptr,
    uint32        diff)
{
    switch(m_ptr->code) {
        case RIR_EVENT_MSG_CODE_CALL:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_CALL:\n");
            break;
        case RIR_EVENT_MSG_CODE_LOCALITY:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_LOCALITY:\n");
            break;
        case RIR_EVENT_MSG_CODE_QUALITY:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_QUALITY:\n");
            break;
        case RIR_EVENT_MSG_CODE_CONNECTIVITY:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_CONNECTIVITY:\n");
            break;
        case RIR_EVENT_MSG_CODE_STATE:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_STATE:\n");
            break;
        case RIR_EVENT_MSG_CODE_HANDOFF:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_HANDOFF:\n");
            break;
        case RIR_EVENT_MSG_CODE_TIME:
            break;
        case RIR_EVENT_MSG_CODE_POWER:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_POWER:\n");
            break;
        case RIR_EVENT_MSG_CODE_RESET:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_RESET:\n");
            break;
       case RIR_EVENT_MSG_CODE_SET_PROFILE:
            _RIR_logMsg("RIR_EVENT_MSG_CODE_SET_PROFILE:\n");
            break;
        default:
            _RIR_logMsg("Unknown RIR event\n");
            break;
     }
}

/*
 * ======== _RIR_eventProcessCall() ========
 *
 * Process an event related to VoIP call event class.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessCall(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    _RIR_Interface *ifc_ptr;
    int id;

    id = m_ptr->msg.call.streamId;
    if(id >= _RIR_MAX_CALLS_PER_INTERFACE) {
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }

    /*
     * For CMRS, we get IP of 0.
     */
    if (OSAL_netIsAddrZero(&m_ptr->msg.call.addr)) {
        ifc_ptr = _RIR_findInterfaceFromName("cmrs");
        if (NULL == ifc_ptr) {
            return (_RIR_EVENT_PROCESS_RET_INVALID);
        }
    }
    else {
        ifc_ptr = _RIR_findInterfaceFromIp(&m_ptr->msg.call.addr);
        if (NULL == ifc_ptr) {
            return (_RIR_EVENT_PROCESS_RET_INVALID);
        }
    }   
     
    ifc_ptr->calls[id].up = (RIR_CALL_MSG_CODE_STOP == m_ptr->msg.call.code) ? 
            0 : 1;
    
    return (_RIR_EVENT_PROCESS_RET_OTHER);
}

/*
 * ======== _RIR_eventProcessLocality() ========
 *
 * Process an event related to locality event class.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessLocality(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{

    _RIR_Interface *ifc_ptr;
    ifc_ptr = _RIR_findInterfaceFromName(m_ptr->msg.locality.infc);
    if (NULL == ifc_ptr) {
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }

    switch(m_ptr->msg.locality.code) {
        case RIR_LOCALITY_MSG_CODE_ESSID:
            OSAL_strncpy(ifc_ptr->wireless.essid, m_ptr->msg.locality.u.essid,
                    sizeof(ifc_ptr->wireless.essid));
            return (_RIR_EVENT_PROCESS_RET_OTHER);
        case RIR_LOCALITY_MSG_CODE_BSSID:
            OSAL_memCpy(ifc_ptr->wireless.bssid, m_ptr->msg.locality.u.bssid,
                    sizeof(ifc_ptr->wireless.bssid));
            return (_RIR_EVENT_PROCESS_RET_OTHER);
        default:
            break;
    }
    return (_RIR_EVENT_PROCESS_RET_INVALID);
}

/*
 * ======== _RIR_eventProcessQuality() ========
 *
 * Process an event related to call quality event class.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessQuality(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    _RIR_Interface *ifc_ptr;
    int id;

    id = m_ptr->msg.quality.streamId;
    if(id >= _RIR_MAX_CALLS_PER_INTERFACE) {
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }

    ifc_ptr = _RIR_findInterfaceFromIp(&m_ptr->msg.quality.addr);
    if (NULL == ifc_ptr) {
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }
    
    ifc_ptr->calls[id].jitter = m_ptr->msg.quality.jitter;
    ifc_ptr->calls[id].latency = m_ptr->msg.quality.latency;
    ifc_ptr->calls[id].loss = m_ptr->msg.quality.loss;
    ifc_ptr->voipBitrate = m_ptr->msg.quality.bitrate;
        
    return (_RIR_EVENT_PROCESS_RET_OTHER);
}

/*
 * ======== _RIR_eventProcessConnectivity() ========
 *
 * Process an event related to connectivity event class.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessConnectivity(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    _RIR_Interface *ifc_ptr;
    ifc_ptr = _RIR_findInterfaceFromName(m_ptr->msg.connectivity.infc);
    if (NULL == ifc_ptr) {
        _RIR_logMsg("_RIR_EVENT_PROCESS_RET_INVALID null interface:\n");
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }
    
    switch (m_ptr->msg.connectivity.code) {
        case RIR_CONNECITVITY_MSG_CODE_QUALITY:
            _RIR_logMsg("RIR_CONNECITVITY_MSG_CODE_QUALITY:\n");
            if ((RIR_INTERFACE_TYPE_802_11 == ifc_ptr->type)
                    || (RIR_INTERFACE_TYPE_CMRS == ifc_ptr->type)) {
                ifc_ptr->wireless.quality = m_ptr->msg.connectivity.u.quality;
                return (_RIR_EVENT_PROCESS_RET_OTHER);
            }
            break;
            
        case RIR_CONNECITVITY_MSG_CODE_BITRATE:
            _RIR_logMsg("RIR_CONNECITVITY_MSG_CODE_BITRATE:\n");
            if (RIR_INTERFACE_TYPE_802_11 == ifc_ptr->type) {
                ifc_ptr->wireless.bitrate = m_ptr->msg.connectivity.u.bitrate;
                return (_RIR_EVENT_PROCESS_RET_OTHER);
            }
            break;
            
        case RIR_CONNECITVITY_MSG_CODE_PING:
            _RIR_logMsg("RIR_CONNECITVITY_MSG_CODE_PING:\n"); 
            ifc_ptr->rtPing = m_ptr->msg.connectivity.u.rtPing;
            return (_RIR_EVENT_PROCESS_RET_OTHER);

        case RIR_CONNECITVITY_MSG_CODE_LINK:
            _RIR_logMsg("RIR_CONNECITVITY_MSG_CODE_LINK:\n"); 
            ifc_ptr->type = m_ptr->msg.connectivity.type; 
            ifc_ptr->up = m_ptr->msg.connectivity.u.link;
            return (_RIR_EVENT_PROCESS_RET_OTHER);

        case RIR_CONNECITVITY_MSG_CODE_IPADDR:
            OSAL_netAddrCpy(&ifc_ptr->addr, &m_ptr->msg.connectivity.u.addr);
            return (_RIR_EVENT_PROCESS_RET_OTHER);

        default:
            _RIR_logMsg("RIR_CONNECITVITY CODE UNKNOWN:\n");
            break;
    }
    
    return (_RIR_EVENT_PROCESS_RET_INVALID);
}

/*
 * ======== _RIR_eventProcessState() ========
 *
 * Process an event related to module state class.
 * This class is for up/down events of different drivers/modules.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessState(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    
    switch(m_ptr->msg.state.code) {
        case RIR_STATE_MSG_CODE_NETLINK:
            obj_ptr->netlinkUp = m_ptr->msg.state.up;
            _RIR_logMsg("Netlink UP=%d\n", obj_ptr->netlinkUp);
            return (_RIR_EVENT_PROCESS_RET_TIMER);
        case RIR_STATE_MSG_CODE_TIME:
            obj_ptr->tmrUp = m_ptr->msg.state.up;
            _RIR_logMsg("Timer UP=%d\n", obj_ptr->tmrUp);
            return (_RIR_EVENT_PROCESS_RET_TIMER);
        default:
            break;
    }
    
    return (_RIR_EVENT_PROCESS_RET_INVALID);
}

/*
 * ======== _RIR_eventSetProfile() ========
 *
 * Process an event related interface profile change.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventSetProfile(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    /* Check for valid profile name */

    /* Copy profile name to RIR object */
    OSAL_strncpy(obj_ptr->profile, m_ptr->msg.profile.name, 
            sizeof(obj_ptr->profile));

    _RIR_logMsg("%s - set to profile=%s", __FUNCTION__, obj_ptr->profile);

    return (_RIR_EVENT_PROCESS_RET_INVALID);
}

/*
 * ======== _RIR_eventProcessReset() ========
 *
 * Process an event related interface profile change.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
static _RIR_EventProcessRet _RIR_eventProcessReset(
    _RIR_Obj *obj_ptr,
    RIR_EventMsg *m_ptr)
{
    obj_ptr->delayReset = m_ptr->msg.reset.isBootUp;

    return (_RIR_EVENT_PROCESS_RET_RESET);
}

/*
 * ======== _RIR_eventProcess() ========
 *
 * Get an event from the OSAL queue to the RIR then process it.
 * 
 * Returns:
 * See _RIR_EventProcessRet
 * 
 */
_RIR_EventProcessRet _RIR_eventProcess(
    _RIR_Obj *obj_ptr)
{
    RIR_EventMsg *msg_ptr = &(obj_ptr->msg);
    OSAL_SelectTimeval now;
     
    if (sizeof(RIR_EventMsg) != OSAL_msgQRecv(obj_ptr->ipcId, (char *)msg_ptr,
            sizeof(RIR_EventMsg), 100, NULL)) {
        /*
         * Bad event.
         */
        return (_RIR_EVENT_PROCESS_RET_INVALID);
    }
    
    /*
     * Find how much delay event has.
     * TBD: Do something about late events.
     */
    OSAL_selectGetTime(&now);
    now.sec -= msg_ptr->ticksec;
    now.usec -= msg_ptr->tickusec;
    
    now.sec *= 1000000;
    now.sec += now.usec;
    
    /*
     * Got an event.
     * Process.
     */
     
     _RIR_eventLog(msg_ptr, now.sec);
     
     switch(msg_ptr->code) {
         case RIR_EVENT_MSG_CODE_CALL:
             return(_RIR_eventProcessCall(obj_ptr, msg_ptr));
            
         case RIR_EVENT_MSG_CODE_LOCALITY:
             return(_RIR_eventProcessLocality(obj_ptr, msg_ptr));
            
         case RIR_EVENT_MSG_CODE_QUALITY:
             return(_RIR_eventProcessQuality(obj_ptr, msg_ptr));
            
         case RIR_EVENT_MSG_CODE_CONNECTIVITY:
             return(_RIR_eventProcessConnectivity(obj_ptr, msg_ptr));
            
         case RIR_EVENT_MSG_CODE_STATE:
             return(_RIR_eventProcessState(obj_ptr, msg_ptr));

         case RIR_EVENT_MSG_CODE_TIME:
             obj_ptr->time.sec = msg_ptr->ticksec;
             obj_ptr->time.usec = msg_ptr->tickusec;
             return(_RIR_EVENT_PROCESS_RET_TIMER);
             
         case RIR_EVENT_MSG_CODE_RESET:
             return (_RIR_eventProcessReset(obj_ptr, msg_ptr));

         case RIR_EVENT_MSG_CODE_SET_PROFILE:
             return (_RIR_eventSetProfile(obj_ptr, msg_ptr));

         case RIR_EVENT_MSG_CODE_HANDOFF:
         case RIR_EVENT_MSG_CODE_POWER:
         default:
             break;
     }
     
    return (_RIR_EVENT_PROCESS_RET_INVALID);
}
