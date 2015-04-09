/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "_rir.h"
#include "_rir_log.h"
#include "../csm/csm_event.h"

_RIR_Obj _RIR_obj;
FILE *_RIR_log_ptr = NULL;
char *_RIR_logFileName_ptr = _RIR_obj.logfile;

/*
 * ======== _RIR_printString() ========
 *
 * Print a string in a specific format, aligning it on 16 bytes boundry.
 *
 * Returns:
 *
 */
static void _RIR_printString(
    char *str_ptr)
{
    int len;
    /*
     * Each entry is 16 char wide.
     */
    len = OSAL_strlen(str_ptr);
    if (_RIR_obj.logIndex + 21 >= (int)(sizeof(_RIR_obj.logBuf))) {
        return;
    }
    OSAL_memCpy(_RIR_obj.logBuf + _RIR_obj.logIndex, str_ptr, len);
    _RIR_obj.logIndex += 21;
}

/*
 * ======== _RIR_printObj() ========
 *
 * Print (for info) interface object for each interface.
 * Columns are interfaces, rows are interface parameters.
 *
 * Returns:
 *
 */
static void _RIR_printObj(
    void)
{
    _RIR_Interface *cur_ptr;
    char tmp[48];
    int calls;

    _RIR_obj.logIndex = 0;
    OSAL_memSet(_RIR_obj.logBuf, ' ', sizeof(_RIR_obj.logBuf));
    _RIR_obj.logBuf[sizeof(_RIR_obj.logBuf) - 1] = 0;
    _RIR_logMsg("\n\n");

    /*
     * Rows, Cols labels.
     */
    _RIR_printString("\nparams");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        _RIR_printString(cur_ptr->name);
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    /*
     * Now start printing objects of each interface in a loop.
     */
    _RIR_printString("\nnext best infc");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        _RIR_printString((_RIR_obj.bestInfc_ptr == cur_ptr) ?
                "yes" : "no");
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\ncurrent infc");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        _RIR_printString((_RIR_obj.curInfc_ptr == cur_ptr) ?
                "yes" : "no");
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\npriority");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->prio);
        _RIR_printString(tmp);
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nlink");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        _RIR_printString(cur_ptr->up ? "up" : "down");
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nipv4 address");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("");
        }
        else {
            /* ipv4 */
            OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d.%d.%d.%d",
                    (cur_ptr->addr.ipv4 >> 24) & 0xFF,
                    (cur_ptr->addr.ipv4 >> 16) & 0xFF,
                    (cur_ptr->addr.ipv4 >>  8) & 0xFF,
                    (cur_ptr->addr.ipv4 >>  0) & 0xFF);
            _RIR_printString(tmp);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nipv6 address");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("");
        }
        else {
            /* ipv6 */
            OSAL_snprintf(tmp, sizeof(tmp) - 1,
                    "%04x:%04x:%04x:%04x:",
                    cur_ptr->addr.ipv6[0], cur_ptr->addr.ipv6[1],
                    cur_ptr->addr.ipv6[2], cur_ptr->addr.ipv6[3]);
            _RIR_printString(tmp);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\n");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("");
        }
        else {
            /* ipv6 */
            OSAL_snprintf(tmp, sizeof(tmp) - 1,
                    "%04x:%04x:%04x:%04x",
                    cur_ptr->addr.ipv6[4], cur_ptr->addr.ipv6[5],
                    cur_ptr->addr.ipv6[6], cur_ptr->addr.ipv6[7]);
            _RIR_printString(tmp);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\ntype");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_OTHER == cur_ptr->type) {
            _RIR_printString("other");
        }
        else if (RIR_INTERFACE_TYPE_802_11 == cur_ptr->type) {
            _RIR_printString("wireless-802.11");
        }
        else if (RIR_INTERFACE_TYPE_802_16 == cur_ptr->type) {
            _RIR_printString("wireless-802.16");
        }
        else if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("mobile-cs");
        }
        else {
            _RIR_printString("");
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nping (usec)");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("");
        }
        else {
            if (cur_ptr->rtPing >= 10000) {
                OSAL_snprintf(tmp, sizeof(tmp) - 1, "unable to ping");
            }
            else {
                OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->rtPing);
            }
            _RIR_printString(tmp);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nESSID");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_802_11 == cur_ptr->type) {
            _RIR_printString(cur_ptr->wireless.essid);
        }
        else {
            _RIR_printString("");
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nLink quality");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if ((RIR_INTERFACE_TYPE_802_11 == cur_ptr->type)
            || (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type)) {
            OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->wireless.quality);
            _RIR_printString(tmp);
        }
        else {
            _RIR_printString("");
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nMax bitrate");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_802_11 == cur_ptr->type) {
            OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->wireless.bitrate);
            _RIR_printString(tmp);
        }
        else {
            _RIR_printString("");
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nVoIP bitrate");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) {
            _RIR_printString("");
        }
        else {
            OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->voipBitrate);
            _RIR_printString(tmp);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    _RIR_printString("\nBSSID");
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (RIR_INTERFACE_TYPE_802_11 == cur_ptr->type) {
            OSAL_snprintf(tmp, sizeof(tmp) - 1,
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    cur_ptr->wireless.bssid[0], cur_ptr->wireless.bssid[1],
                    cur_ptr->wireless.bssid[2], cur_ptr->wireless.bssid[3],
                    cur_ptr->wireless.bssid[4], cur_ptr->wireless.bssid[5]);
            _RIR_printString(tmp);
        }
        else {
            _RIR_printString("");
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    /*
     * Print quality for each call.
     */
     for (calls = 0; calls < _RIR_MAX_CALLS_PER_INTERFACE; calls++) {
        OSAL_snprintf(tmp, sizeof(tmp) - 1, "\nCall%d state", calls);
        _RIR_printString(tmp);
        for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
            _RIR_printString(cur_ptr->calls[calls].up ? "up" : "down");
            cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
        }

        OSAL_snprintf(tmp, sizeof(tmp) - 1, "\nCall%d loss", calls);
        _RIR_printString(tmp);
        for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
            if ((RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) ||
                    (!cur_ptr->calls[calls].up)) {
                _RIR_printString("");
            }
            else {
                OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->calls[calls].loss);
                _RIR_printString(tmp);
            }
            cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
        }
        OSAL_snprintf(tmp, sizeof(tmp) - 1, "\nCall%d latency", calls);
        _RIR_printString(tmp);
        for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
            if ((RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) ||
                    (!cur_ptr->calls[calls].up)) {
                _RIR_printString("");
            }
            else {
                OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->calls[calls].latency);
                _RIR_printString(tmp);
            }
            cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
        }
        OSAL_snprintf(tmp, sizeof(tmp) - 1, "\nCall%d jitter", calls);
        _RIR_printString(tmp);
        for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
            if ((RIR_INTERFACE_TYPE_CMRS == cur_ptr->type) ||
                    (!cur_ptr->calls[calls].up)) {
                _RIR_printString("");
            }
            else {
                OSAL_snprintf(tmp, sizeof(tmp) - 1, "%d", cur_ptr->calls[calls].jitter);
                _RIR_printString(tmp);
            }
            cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
        }
    }

     _RIR_obj.logBuf[_RIR_obj.logIndex] = 0;
     _RIR_logMsg("%s", _RIR_obj.logBuf);
    _RIR_logMsg("\n\n");
}

/*
 * ======== _RIR_arrangeInterfacesByPrio() ========
 *
 * Re-order the linked list to put the highest prio interface first,
 * then in decending order.
 *
 * Returns:
 *
 */
static void _RIR_arrangeInterfacesByPrio(
    void)
{
    _RIR_Interface *cur_ptr;
    _RIR_Interface *nxt_ptr;
    _RIR_Interface *tmp_ptr;
    _RIR_Interface *last_ptr;

    last_ptr = NULL;

    /*
     * Simple sort.
     */
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        nxt_ptr = cur_ptr->next_ptr;
        if (NULL == nxt_ptr) {
            break;
        }
        if (cur_ptr->prio < nxt_ptr->prio) {
            tmp_ptr = nxt_ptr->next_ptr;
            nxt_ptr->next_ptr = cur_ptr;
            cur_ptr->next_ptr = tmp_ptr;
            if (NULL == last_ptr) {
                _RIR_obj.infc_ptr = nxt_ptr;
            }
            else {
                last_ptr->next_ptr = nxt_ptr;
            }
            cur_ptr = _RIR_obj.infc_ptr;
            last_ptr = NULL;
            continue;
        }
        last_ptr = cur_ptr;
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }
}

/*
 * ======== _RIR_findInterfaceFromIp() ========
 *
 * Get an interface object from interfaces linked list based on IP address.
 *
 * Returns:
 *  Pointer to interface, or NULL if not found.
 */
_RIR_Interface *_RIR_findInterfaceFromIp(
    OSAL_NetAddress *addr_ptr)
{
    _RIR_Interface *cur_ptr;

    /*
     * Bad IP for IP interfaces.
     */
    if (OSAL_netIsAddrZero(addr_ptr)) {
        return (NULL);
    }

    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (OSAL_netIsAddrEqual(addr_ptr, &cur_ptr->addr)) {
            /*
             * Exists.
             */
            return (cur_ptr);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    return (NULL);
}

/*
 * ======== _RIR_findInterfaceorAdd() ========
 *
 * Get an interface object from interfaces linked list based on its name,
 * and if not found, add it to that list, and init it.
 *
 * Returns:
 *  Pointer to the found or added interface.
 */
_RIR_Interface *_RIR_findInterfaceorAdd(
    _RIR_Obj *obj_ptr,
    char     *name_ptr)
{
    int calls;

    /*
     * Keep a linked list of all system interfaces.
     */
    _RIR_Interface *last_ptr = NULL;
    _RIR_Interface *cur_ptr;
    for (cur_ptr = obj_ptr->infc_ptr; cur_ptr; ) {
        if (!OSAL_strncmp(name_ptr, cur_ptr->name, sizeof(cur_ptr->name))) {
            /*
             * Exists.
             */
            return (cur_ptr);
        }
        last_ptr = cur_ptr;
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    /*
     * Not found. Add.
     */
    cur_ptr = OSAL_memAlloc(sizeof(_RIR_Interface), 0);
    if (NULL == cur_ptr) {
        _RIR_logMsg("%s %d: infc %s, not enough memory\n",
                __FILE__, __LINE__, name_ptr);
        return (NULL);
    }
    OSAL_memSet(cur_ptr, 0, sizeof(_RIR_Interface));
    OSAL_strncpy(cur_ptr->name, name_ptr, sizeof(cur_ptr->name));
    if (NULL != last_ptr) {
        last_ptr->next_ptr = cur_ptr;
    }
    else {
        obj_ptr->infc_ptr = cur_ptr;
    }

    /*
     * Init it.
     */
    cur_ptr->rtPing = 10000;

    for (calls = 0; calls < _RIR_MAX_CALLS_PER_INTERFACE; calls++) {
        cur_ptr->calls[calls].streamId = -1;
        cur_ptr->calls[calls].streamId = -1;
    }

    return (cur_ptr);

}

/*
 * ======== _RIR_findInterfaceFromName() ========
 *
 * Get an interface object from interfaces linked list based on its name.
 *
 * Returns:
 *  Pointer to interface, or NULL if not found.
 */
_RIR_Interface *_RIR_findInterfaceFromName(
    char *name_ptr)
{
    _RIR_Interface *cur_ptr;

    if (NULL == name_ptr) {
        return (NULL);
    }
    if (0 == name_ptr[0]) {
        return (NULL);
    }
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        if (!OSAL_strncmp(name_ptr, cur_ptr->name, sizeof(cur_ptr->name))) {
            /*
             * Exists.
             */
            return (cur_ptr);
        }
        cur_ptr = (_RIR_Interface *)cur_ptr->next_ptr;
    }

    /*
     * Add interface if it does not exists in accept-all operation mode.
     */
    if (_RIR_obj.all) {
        return (_RIR_findInterfaceorAdd(&_RIR_obj, name_ptr));
    }

    return (NULL);
}

/*
 * ======== _RIR_task() ========
 *
 * The task that runs RIR.
 *
 * Returns:
 *  0.
 */
OSAL_TaskReturn _RIR_task(
    OSAL_TaskArg arg)
{
    _RIR_Interface *last_ptr = NULL;
    _RIR_Interface *cur_ptr;
    int ret;

    /*
     * Repeat till shut command.
     */
    while (!_RIR_obj.shut) {
        /*
         * Get event.
         */
        ret = _RIR_eventProcess(&_RIR_obj);
        if (-1 == ret) {
            continue;
        }

        if (1 == ret) {
            /*
             * Run timer based hysteresis.
             */
            _RIR_hoDecrementParamsCounter(&_RIR_obj);
        }

        /*
         * Till netlink is fully up, do nothing.
         * This will supress transients during init.
         */
        if (_RIR_obj.netlinkUp && _RIR_obj.tmrUp) {
            /*
             * If change of IP, notify protocols.
             */
            _RIR_qualifyEvaluateInterfacesForBest(&_RIR_obj);
            _RIR_hoEvaluateHandoffConditions(&_RIR_obj);
        }

        /*
         * Ifc info. Dont print on timer event, as that wastes file size.
         */
        if (1 != ret) {
            _RIR_printObj();
        }
    }

    /*
     * Shutdown and free.
     */
    for (cur_ptr = _RIR_obj.infc_ptr; cur_ptr; ) {
        last_ptr = cur_ptr;
        cur_ptr = cur_ptr->next_ptr;
        OSAL_memFree(last_ptr, 0);
    }

    _RIR_obj.shut = 0;

    return ((OSAL_TaskReturn)0);
}

/*
 * ======== _RIR_isMacAddrZero() ========
 *
 * Check if MAC address is zero
 *
 * Returns:
 *   1: zero
 *   0: not zero
 */

int _RIR_isMacAddrZero(
    uint8 *addr_ptr)
{
    vint i;

    for (i = 0; i < _RIR_MAC_ADDR_BYTES; i++) {
        if (0 != addr_ptr[i]) {
            return (0);
        }
    }

    return (1);
}

/*
 * ======== RIR_init() ========
 *
 * Inits RIR.
 *
 * Returns:
 *  0.
 */
int RIR_init(
    void *cfg_ptr)
{
    _RIR_obj.shut = 0;

    OSAL_memSet(&_RIR_obj, 0, sizeof(_RIR_obj));

    if (0 != _RIR_parseInput(&_RIR_obj, cfg_ptr)) {
        OSAL_logMsg("Input file cannot be parsed.\n");
        return (-1);
    }
    else {
        OSAL_logMsg("logfile = %s\n", _RIR_obj.logfile);

        /*
         * Prio is a fixed criteria.
         */
        if (!_RIR_obj.all) {
            _RIR_arrangeInterfacesByPrio();
        }
    }

    /*
     * Open log file.
     */
    _RIR_log_ptr = fopen(_RIR_obj.logfile, "a+");
    if (NULL == _RIR_log_ptr) {
        OSAL_logMsg("Log file cannot be opened, so logging to console\n");
    }

    if (0 == (_RIR_obj.ipcId = OSAL_msgQCreate(RIR_EVENT_QUEUE_NAME,
            OSAL_MODULE_RIR, OSAL_MODULE_RIR, OSAL_DATA_STRUCT_RIR_EventMsg,
            RIR_EVENT_QUEUE_DEPTH, sizeof(RIR_EventMsg), 0))) {
        OSAL_logMsg("%s:%d Message queue failure.\n", __FUNCTION__, __LINE__);
        if (NULL != _RIR_log_ptr) {
            fclose(_RIR_log_ptr);
        }
        return (-1);
    }

    /*
     * Send new IP info.
     */
    if (0 == (_RIR_obj.csmEvtInQ = OSAL_msgQCreate(CSM_INPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_RIR, OSAL_MODULE_CSM_PUBLIC,
            OSAL_DATA_STRUCT_CSM_InputEvent,
            CSM_INPUT_EVENT_MSGQ_LEN, sizeof(CSM_InputEvent), 0))) {
        OSAL_msgQDelete(_RIR_obj.ipcId);
        OSAL_logMsg("%s:%d Message queue failure.\n", __FUNCTION__, __LINE__);
        if (NULL != _RIR_log_ptr) {
            fclose(_RIR_log_ptr);
        }
        return (-1);
    }

    if (NULL == OSAL_taskCreate("rir", OSAL_TASK_PRIO_NRT, 4096,
            _RIR_task, (OSAL_TaskArg)&_RIR_obj)) {
        OSAL_logMsg("Task creation failure.\n");
        OSAL_msgQDelete(_RIR_obj.csmEvtInQ);
        OSAL_msgQDelete(_RIR_obj.ipcId);
        if (NULL != _RIR_log_ptr) {
            fclose(_RIR_log_ptr);
        }
        return (-1);
    }

    OSAL_logMsg("RIR running.\n");

    _RIR_tmrStart();
    _RIR_netlStart(_RIR_obj.serverIp);
    return (0);
}

/*
 * ======== RIR_shutdown() ========
 *
 * Shuts down RIR.
 *
 * Returns:
 */
void RIR_shutdown()
{
    if (!_RIR_obj.shut) {
        /* RIR is not running */
        return;
    }

    _RIR_tmrStop();
    _RIR_netlStop();

    _RIR_obj.shut = 1;
    while (_RIR_obj.shut) {
        OSAL_taskDelay(100);
    }

    OSAL_msgQDelete(_RIR_obj.ipcId);
    OSAL_msgQDelete(_RIR_obj.csmEvtInQ);
    if (NULL != _RIR_log_ptr) {
        fclose(_RIR_log_ptr);
    }

    OSAL_logMsg("RIR finished.\n");
}

