/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#ifndef _RIR_EVENT_H_
#define _RIR_EVENT_H_

#include <osal.h>

/*
 * This is for modules to include.
 */

/*
 * RIR event queue name and its depth.
 */
#ifdef VPORT_4G_PLUS_APROC
#define RIR_EVENT_QUEUE_NAME "mcm-event-aproc"
#else
#define RIR_EVENT_QUEUE_NAME "mcm-event"
#endif
#define RIR_EVENT_QUEUE_DEPTH  16

/* Network type name */
#define RIR_NETWORK_TYPE_NAME_WIFI  "wifi"
#define RIR_NETWORK_TYPE_NAME_LTE   "lte"

/*
 * MAC address are 48 bits in length.
 */
#define _RIR_MAC_ADDR_BYTES (6)

/*
 * Define event classes.
 */
typedef enum {
    RIR_EVENT_MSG_CODE_INVALID = 0,
    RIR_EVENT_MSG_CODE_CALL,
    RIR_EVENT_MSG_CODE_POWER,
    RIR_EVENT_MSG_CODE_LOCALITY,
    RIR_EVENT_MSG_CODE_QUALITY,
    RIR_EVENT_MSG_CODE_CONNECTIVITY,
    RIR_EVENT_MSG_CODE_HANDOFF,
    RIR_EVENT_MSG_CODE_TIME,
    RIR_EVENT_MSG_CODE_STATE,
    RIR_EVENT_MSG_CODE_RESET,
    RIR_EVENT_MSG_CODE_SET_PROFILE
} RIR_EventMsgCodes;

/*
 * Event class connectivity events.
 */
typedef enum {
    RIR_CONNECTIVITY_MSG_CODE_INVALID = 0,
    RIR_CONNECITVITY_MSG_CODE_QUALITY,
    RIR_CONNECITVITY_MSG_CODE_BITRATE,
    RIR_CONNECITVITY_MSG_CODE_PING,
    RIR_CONNECITVITY_MSG_CODE_LINK,
    RIR_CONNECITVITY_MSG_CODE_IPADDR
} RIR_ConnectivityMsgCodes;

/*
 * Event class VoIP call events.
 */
typedef enum {
    RIR_CALL_MSG_CODE_INVALID = 0,
    RIR_CALL_MSG_CODE_MODIFY,
    RIR_CALL_MSG_CODE_STOP,
    RIR_CALL_MSG_CODE_START
} RIR_CallMsgCodes;

/*
 * Event class locality events.
 */
typedef enum {
    RIR_LOCALITY_MSG_CODE_INVALID = 0,
    RIR_LOCALITY_MSG_CODE_ESSID,
    RIR_LOCALITY_MSG_CODE_BSSID
} RIR_LocalityMsgCodes;

/*
 * Event class module state events.
 */
typedef enum {
    RIR_STATE_MSG_CODE_INVALID = 0,
    RIR_STATE_MSG_CODE_TIME,
    RIR_STATE_MSG_CODE_NETLINK
} RIR_StateMsgCodes;

/*
 * Types of interface we support.
 * Each interface driver must include type to RIR
 * for RIR to use that interface properly.
 */
typedef enum {
    RIR_INTERFACE_TYPE_INVALID = 0,
    RIR_INTERFACE_TYPE_OTHER,
    RIR_INTERFACE_TYPE_802_11,
    RIR_INTERFACE_TYPE_802_16,
    RIR_INTERFACE_TYPE_CMRS
} RIR_InterfaceType;

/*
 * How a event is sent in a message:
 */
typedef struct {
    RIR_EventMsgCodes code;
    unsigned long tickusec;
    unsigned long ticksec;
    union {
        struct {
            RIR_CallMsgCodes code;
            int streamId;
            OSAL_NetAddress addr;
        } call;
        struct {
            int state;
        } power;
        struct {
            int state;
        } handoff;
        struct {
            char infc[32];
            RIR_LocalityMsgCodes code;
            union {
                char essid[64];
                unsigned char bssid[_RIR_MAC_ADDR_BYTES];
            } u;
        } locality;
        struct {
            int streamId;
            OSAL_NetAddress addr;
            int jitter;
            int loss;
            int latency;
            int bitrate;
        } quality;
        struct {
            char infc[32];
            RIR_ConnectivityMsgCodes code;
            RIR_InterfaceType type;
            union {
                int quality;
                int bitrate;
                OSAL_NetAddress addr;
                int rtPing;
                int link;
            } u;
        } connectivity;
        struct {
            RIR_StateMsgCodes code;
            int up;
            char commandQ[64];
        } state;
        struct {
            int count;
        } time;
        struct {
            char name[32];
        } profile;
        struct {
            int isBootUp;
        } reset;
    } msg;
} RIR_EventMsg;

/*
 * RIR Message to protocols.
 */
typedef struct {
    OSAL_NetAddress   addr;
    int8              infcName[16];
    int8              typeName[16];
    RIR_InterfaceType infcType;
    int8              pProxy[128];
    unsigned char     bssid[_RIR_MAC_ADDR_BYTES];
} RIR_Command;


#endif
