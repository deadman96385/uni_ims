/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef __RIR_NETL_H_
#define __RIR_NETL_H_

/*
 * How often to ping and how long to wait for reply.
 */
#define _RIR_NETL_PING_INTERVAL_US 5000000
#define _RIR_NETL_PING_TIMEOUT_US  1000000


/*
 * This must be a integer factor of _RIR_NETL_PING_TIMEOUT_US, less than it.
 */
#define _RIR_NETL_HEARTBEAT_US     100000

/*
 * Define in usec, event generation threshold.
 */
#define _RIR_NETL_PING_TOLERANCE    1000
#define _RIR_NETL_QUALITY_TOLERANCE 5
#define _RIR_NETL_BITRATE_TOLERANCE 10000

#define _RIR_NETL_MAX_IPV6_COUNT    8

/*
 * Define IPv6 type, Permanent or Temporary address.
 */
typedef enum {
    _RIR_ADDR_TYPE_NONE,
    _RIR_ADDR_TYPE_PERMANENT,
    _RIR_ADDR_TYPE_TEMPORARY
} _RIR_Ipv6AddrType;

typedef struct {
    unsigned short      addr[8];
    _RIR_Ipv6AddrType   type;
} _RIR_NetIpv6Addr;

/*
 * ICMP packet. For pinging.
 */
typedef struct {
    unsigned char type;
    unsigned char  code;
    unsigned short checksum;
    unsigned short id;
    unsigned short sequence;
    char data[8];
} _RIR_NetlIcmp;

/*
 * Some interface params on which events will be generated.
 */
typedef struct {
    int                     isWireless;
    int                     up;
    int                     rtPing;
    struct {
        unsigned long       ipv4;
        _RIR_NetIpv6Addr    ipv6[_RIR_NETL_MAX_IPV6_COUNT];
        int                 ipv6Count;
    } addr;
    union {
        struct {
            char            essid[64];
            int             quality;
            int             bitrate;
            unsigned char   bssid[_RIR_MAC_ADDR_BYTES];
        } i802_11;
        struct {
            int             bitrate;
        } i802_1;
    } types;
} _RIR_NetlInfcParams;

typedef struct _RIR_NetlInfc {
    char name[32];
    int  index;
    int sock;
    struct timeval time;
    struct timeval time1;
    _RIR_NetlInfcParams params;
    _RIR_NetlInfcParams paramsCache;
    _RIR_NetlIcmp icmp;
    struct _RIR_NetlInfc *next_ptr;
} _RIR_NetlInfc;
/*
 * The data storage for an interface.
 */
typedef struct {
    int                 fd;
    struct sockaddr_nl  addr;
    int                 shut;
    unsigned long       pingIp;
    char                data[4096];
    _RIR_NetlInfc     *interfaces_ptr;
    struct timeval      timeLast;
    RIR_EventMsg        evt;
} _RIR_Netl;

#endif
