/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "../csm/csm_event.h"

#include "_rir.h"
#include "_rir_log.h"

/*
 * ======== _RIR_hoDecrementParam() ========
 *
 * Used to decrement time of a param. When this time expires, HO occurs.
 * ge argument specifies if the comparison is greater or equal to.
 *
 * Returns:
 *
 */
static inline void _RIR_hoDecrementParam(
    _RIR_Param *param_ptr,
    int         val,
    int         ge)
{
    if ((param_ptr->val < 0) || (param_ptr->sustain < 0)) {
        /*
         * < 0 param val/sustain means not participating in handoff.
         */
        return;
    }

    if (ge) {
        if (val >= param_ptr->val) {
            if (param_ptr->count > 0) {
                param_ptr->count--;
            }
        }
        else {
            param_ptr->count = param_ptr->sustain;
        }
    }
    else {
        if (val < param_ptr->val) {
            if (param_ptr->count > 0) {
                param_ptr->count--;
            }
        }
        else {
            param_ptr->count = param_ptr->sustain;
        }
    }
}

/*
 * ======== _RIR_hoNotifyProtos() ========
 *
 * Notify protocols that a change of IP has occured.
 * List of protocol queue names comma separated in protos_ptr.
 *
 * Returns:
 *
 */
static void _RIR_hoNotifyProtos(
    _RIR_Obj    *obj_ptr,
    RIR_Command *cmd_ptr)
{
    /* Notify protocol depends on platform */
    _RIR_notifyProtos(obj_ptr, cmd_ptr);
}

/*
 * ======== _RIR_hoSwitchToBest() ========
 *
 * Used to decrement time of a param. When this time expires, HO occurs.
 * ge argument specifies if the comparison is greater or equal to.
 *
 * Returns:
 *
 */
static void _RIR_hoSwitchToBest(
    _RIR_Obj *obj_ptr)
{
    RIR_Command cmd;

    /*
     * Change of IP is immediately conveyed.
     */
    obj_ptr->curInfc_ptr = obj_ptr->bestInfc_ptr;
    _RIR_logMsg("%s %d: Current interface is now %s\n",
        __FILE__, __LINE__, obj_ptr->curInfc_ptr->name);
    OSAL_netAddrCpy(&obj_ptr->curInfc_ptr->oldAddr, &obj_ptr->curInfc_ptr->addr);

    /*
     * Notify.
     */
    cmd.infcType = obj_ptr->curInfc_ptr->type;
    OSAL_strncpy(cmd.infcName, obj_ptr->curInfc_ptr->name,
            sizeof(cmd.infcName));
    OSAL_strncpy(cmd.typeName, obj_ptr->curInfc_ptr->typeName,
            sizeof(cmd.typeName));
    OSAL_netAddrCpy(&cmd.addr, &obj_ptr->curInfc_ptr->addr);
    OSAL_memCpy(&cmd.bssid, &obj_ptr->curInfc_ptr->wireless.bssid, 
            sizeof(cmd.bssid));
    if (RIR_INTERFACE_TYPE_CMRS == obj_ptr->curInfc_ptr->type) {
        OSAL_netAddrClear(&cmd.addr);
    }
    else {
        OSAL_netAddrCpy(&obj_ptr->curInfc_ptr->addr, &obj_ptr->curInfc_ptr->oldAddr);
    }

    _RIR_hoNotifyProtos(obj_ptr, &cmd);

    _RIR_logMsg("%s %d: Current interface is now %s\n",
            __FILE__, __LINE__, obj_ptr->curInfc_ptr->name);
}

/*
 * ======== _RIR_hoEvaluateHandoffConditions() ========
 *
 * This function evaluates if handoff is required on the current interface.
 *
 * Returns:
 *
 */
void _RIR_hoEvaluateHandoffConditions(
    _RIR_Obj *obj_ptr)
{
    char *ho_ptr = NULL;
    _RIR_Interface *cur_ptr = obj_ptr->curInfc_ptr;
    RIR_Command cmd;

    /*
     * If there is no current interface then simply go to best interface.
     */
    if (NULL == cur_ptr) {
        if (NULL != obj_ptr->bestInfc_ptr) {
            /*
             * Since current is not assigned yet, assign now.
             */
            _RIR_hoSwitchToBest(obj_ptr);
            return;
        }
    }

    if (NULL == cur_ptr) {
        if (0 == obj_ptr->noInfc) {
            OSAL_netAddrClear(&cmd.addr);
            OSAL_memSet(cmd.bssid, 0, sizeof(cmd.bssid));
            _RIR_hoNotifyProtos(obj_ptr, &cmd);
            obj_ptr->noInfc = 1;
        }
        return;
    }

    obj_ptr->noInfc = 0;

    /*
     * Now evaluate conditions.
     */

    /*
     * Change of IP is immediately conveyed for same interface.
     */
    if (!OSAL_netIsAddrEqual(&cur_ptr->addr, &cur_ptr->oldAddr)) {
        cmd.infcType = cur_ptr->type;
        OSAL_strncpy(cmd.infcName, cur_ptr->name, sizeof(cmd.infcName));
        OSAL_strncpy(cmd.typeName, cur_ptr->typeName, sizeof(cmd.typeName));
        OSAL_netAddrCpy(&cmd.addr, &cur_ptr->addr);
        OSAL_memCpy(&cmd.bssid, &cur_ptr->wireless.bssid, sizeof(cmd.bssid));

        _RIR_hoNotifyProtos(obj_ptr, &cmd);
        _RIR_logMsg("%s %d: IP2IP handoff on same interface %s\n",
                __FILE__, __LINE__, cur_ptr->name);
        OSAL_netAddrCpy(&cur_ptr->oldAddr, &cur_ptr->addr);
        return;
    }

    /*
     * Check if any param counter has reached 0.
     * If it has then go to next best interface.
     */

    if (NULL == obj_ptr->bestInfc_ptr) {
        /*
         * Cant do much for intra interface switch if no best interface.
         */
        return;
    }

    cur_ptr = obj_ptr->curInfc_ptr;
    if (NULL == cur_ptr) {
        return;
    }

    /*
     * This is the first, most important criteria.
     */
    if (OSAL_netIsAddrZero(&cur_ptr->addr) || (0 == cur_ptr->up)) {
        ho_ptr = "link";
    }

    /*
     * No other criteria for accept-all mode.
     */
    if (!obj_ptr->all) {
        /*
         * Other criteria.
         */
        if (0 == cur_ptr->handoff.bitrate.count) {
            cur_ptr->handoff.bitrate.count = cur_ptr->handoff.bitrate.sustain;
            ho_ptr = "bitrate";
        }
        if (0 == cur_ptr->handoff.loss.count) {
            cur_ptr->handoff.loss.count = cur_ptr->handoff.loss.sustain;
            ho_ptr = "loss";
        }
        if (0 == cur_ptr->handoff.latency.count) {
            cur_ptr->handoff.latency.count = cur_ptr->handoff.latency.sustain;
            ho_ptr = "latency";
        }
        if (0 == cur_ptr->handoff.jitter.count) {
            cur_ptr->handoff.jitter.count = cur_ptr->handoff.jitter.sustain;
            ho_ptr = "jitter";
        }
        if (0 == cur_ptr->handoff.ping.count) {
            cur_ptr->handoff.ping.count = cur_ptr->handoff.ping.sustain;
            ho_ptr = "ping";
        }
        if (0 == cur_ptr->handoff.quality.count) {
            cur_ptr->handoff.quality.count = cur_ptr->handoff.quality.sustain;
            ho_ptr = "quality";
        }
    }

    /*
     * Handoff if required.
     */
    if (NULL != ho_ptr) {
        _RIR_logMsg("%s %d: Handoff (leaving infc %s) based on %s\n",
                __FILE__, __LINE__, cur_ptr->name, ho_ptr);
        _RIR_hoSwitchToBest(obj_ptr);
    }
}

/*
 * ======== _RIR_hoDecrementParamsCounter() ========
 *
 * This function looks at parameters participating in handoff, and if they are
 * below specified range, it will decrement their timer. If timer expires, handoff happens.
 *
 * Returns:
 *
 */
void _RIR_hoDecrementParamsCounter(
    _RIR_Obj *obj_ptr)
{
    _RIR_Interface *cur_ptr;
    int calls;
    int jitter = 0;
    int loss = 0;
    int latency = 0;

    /*
     * This is for current interface only.
     * Called every one second.
     */
    cur_ptr = obj_ptr->curInfc_ptr;
    if (NULL == cur_ptr) {
        return;
    }

    /*
     * Check values, then decrement below range.
     * When count reaches 0, handoff happens.
     */

    _RIR_hoDecrementParam(&cur_ptr->handoff.ping, cur_ptr->rtPing, 1);

    if (
            (RIR_INTERFACE_TYPE_802_11 ==  cur_ptr->type) ||
            (RIR_INTERFACE_TYPE_802_16 ==  cur_ptr->type) ||
            (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type)) {
        _RIR_hoDecrementParam(&cur_ptr->handoff.quality,
                cur_ptr->wireless.quality, 0);
        _RIR_hoDecrementParam(&cur_ptr->handoff.bitrate,
                cur_ptr->wireless.bitrate, 0);
    }

    /*
     * For all calls up, if there is a problem with a call, handoff.
     * In future handoff will occur for the call in trouble only.
     */
    for (calls = 0; calls < _RIR_MAX_CALLS_PER_INTERFACE; calls++) {
        if (cur_ptr->calls[calls].jitter > jitter) {
            jitter = cur_ptr->calls[calls].jitter;
        }
        if (cur_ptr->calls[calls].loss > loss) {
            loss = cur_ptr->calls[calls].loss;
        }
        if (cur_ptr->calls[calls].latency > latency) {
            latency = cur_ptr->calls[calls].latency;
        }
    }

    _RIR_hoDecrementParam(&cur_ptr->handoff.jitter, jitter, 1);
    _RIR_hoDecrementParam(&cur_ptr->handoff.loss, loss, 1);
    _RIR_hoDecrementParam(&cur_ptr->handoff.latency, latency, 1);
}
