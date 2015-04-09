/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#ifndef __RIR_H_
#define __RIR_H_

#include "rir_event.h"

/*
 * This is somehow now established for end point.
 */
#define _RIR_MAX_CALLS_PER_INTERFACE (2)

/*
 * Private data for RIR.
 */
typedef enum {
    _RIR_EVENT_PROCESS_RET_INVALID = -1,
    _RIR_EVENT_PROCESS_RET_RESET = 2,
    _RIR_EVENT_PROCESS_RET_OTHER = 0,
    _RIR_EVENT_PROCESS_RET_TIMER = 1
} _RIR_EventProcessRet;


/*
 * A hysteresis based param.
 */
typedef struct
{
    int val;
    int sustain;
    int count;
} _RIR_Param;

/*
 * The world of information about an interface.
 */
typedef struct _RIR_Interface {
    char   name[128];
    char   typeName[16];
    RIR_InterfaceType type;
    int    up;
    int    prio;
    OSAL_NetAddress addr;
    OSAL_NetAddress oldAddr;
    int    rtPing;
    int    voipBitrate;
    struct {
        int streamId;
        int up;
        int jitter;
        int loss;
        int latency;
    } calls[_RIR_MAX_CALLS_PER_INTERFACE];
    struct {
        char essid[128];
        int  bitrate;
        int  quality;
        unsigned char bssid[_RIR_MAC_ADDR_BYTES];
    } wireless;

    struct {
        _RIR_Param ping;
        _RIR_Param jitter;
        _RIR_Param latency;
        _RIR_Param loss;
        _RIR_Param bitrate;
        _RIR_Param quality;
    } handoff;

    struct _RIR_Interface *next_ptr;
} _RIR_Interface;

/*
 * THE data sotrage of RIR.
 */
typedef struct {
    int shut;
    int all;
    int delayReset;
    int delayTimeMs;
    char logfile[128];
    char wmproxy[128];
    char profile[32];
    char protocols[128];
    char logBuf[2048];
    int  logIndex;
    OSAL_SelectTimeval time;
    OSAL_MsgQId     ipcId;
    OSAL_MsgQId     csmEvtInQ;
    RIR_EventMsg    msg;
    int             netlinkUp;
    int             tmrUp;
    uint32          serverIp;
    int             noInfc;
    _RIR_Interface *infc_ptr;
    _RIR_Interface *bestInfc_ptr;
    _RIR_Interface *curInfc_ptr;
} _RIR_Obj;

/*
 * Prototypes.
 */
void _RIR_netlStart(
    unsigned long pingServerIp);

void _RIR_netlStop(
    void);

void _RIR_tmrStart(
    void);

void _RIR_tmrStop(
    void);

int _RIR_parseInput(
    _RIR_Obj *obj_ptr,
    void     *cfg_ptr);

_RIR_Interface *_RIR_findInterfaceorAdd(
    _RIR_Obj *obj_ptr,
    char     *name_ptr);

_RIR_EventProcessRet _RIR_eventProcess(
    _RIR_Obj *obj_ptr);

_RIR_Interface *_RIR_findInterfaceFromIp(
    OSAL_NetAddress *addr);
    
_RIR_Interface *_RIR_findInterfaceFromName(
    char *name_ptr);

int _RIR_isMacAddrZero(
    uint8 *addr_ptr);

void _RIR_qualifyEvaluateInterfacesForBest(
    _RIR_Obj *obj_ptr);
    
void _RIR_hoEvaluateHandoffConditions(
    _RIR_Obj *obj_ptr);
    
void _RIR_hoDecrementParamsCounter(
    _RIR_Obj *obj_ptr);
    
void _RIR_notifyProtos(
    _RIR_Obj    *obj_ptr,
    RIR_Command *cmd_ptr);
#endif
