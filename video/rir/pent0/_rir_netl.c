/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <sys/types.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rir_event.h"
#include "_rir_netl.h"
#include "_rir_log.h"

char RIR_netLinkProtocolPath[256] = {0};

/*
 * Data storage for netlink.
 * Netlink can be a separate module from RIR.
 * But for now bundle it with RIR.
 */
static _RIR_Netl _RIR_NetlObj;

/*
 * Get info about wireless extension.
 */
static inline int _RIR_netlGetWirelessExtension(
    int           sock,
    int           request,
    char         *name_ptr,
    struct iwreq *req_ptr)
{
    strncpy(req_ptr->ifr_ifrn.ifrn_name,
            name_ptr, sizeof(req_ptr->ifr_ifrn.ifrn_name) - 1);

    return(ioctl(sock, request, req_ptr));
}

/*
 * ======== _RIR_netlCheckRange() ========
 *
 * Just a simple function that checks if two values are within a
 * tolerance of each other.
 *
 * Returns:
 *  1 : within tolerance range.
 *  0 : not in range, and second value is overwritten with first.
 *
 */
static inline int _RIR_netlCheckRange(
    int *val1_ptr,
    int *val2_ptr,
    int  tolerance)
{
    int diff = *val1_ptr - *val2_ptr;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff <= tolerance) {
        return (1);
    }

    *val1_ptr = *val2_ptr;
    return (0);
}

/*
 * ======== _RIR_netlGetWirelessInfo() ========
 *
 * Find wireless interface params like SSID etc.
 *
 * Returns:
 *  0  : OK
 *  -1 : Failed.
 *
 */
static int _RIR_netlGetWirelessInfo(
     void)
{

    int    sock;
    struct iwreq req;
    _RIR_NetlInfc *cur_ptr;
    struct iw_statistics stats;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return (-1);
    }

    /*
     * Find wireless params.
     */
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {

        memset((void *)&req, 0, sizeof(req));

        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWNAME,
                cur_ptr->name,
                &req)) {
            cur_ptr->params.isWireless = 0;
            cur_ptr = cur_ptr->next_ptr;
            continue;
        }

        /*
         * This is a wireless interface because above call passed.
         */
        cur_ptr->params.isWireless = 1;

        /*
         * Get some useful info.
         */
#if 0 // NOTE, This causes a crash on the nexus one.
        /*
         * ESSID
         */
        req.u.essid.pointer = (void *)cur_ptr->params.types.i802_11.essid;
        req.u.essid.length = sizeof(cur_ptr->params.types.i802_11.essid);
        req.u.essid.flags = 0;
        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWESSID,
                cur_ptr->name,
                &req)) {
            cur_ptr->params.types.i802_11.essid[0] = 0;
        }
#endif

        /*
         * Bit rate
         */
        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWRATE,
                cur_ptr->name,
                &req)) {
            cur_ptr->params.types.i802_11.bitrate = 0;
        }
        else {
            cur_ptr->params.types.i802_11.bitrate = req.u.bitrate.value;
        }

        /*
         * Power state
         */
        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWPOWER,
                cur_ptr->name,
                &req)) {
        }
        else {
            /*
             * TBD
             */
        }

        /*
         * Link quality.
         */
        req.u.data.pointer = (void *)&stats;
        req.u.data.length = sizeof(stats);
        req.u.data.flags = 1;
        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWSTATS,
                cur_ptr->name,
                &req)) {
            cur_ptr->params.types.i802_11.quality = 0;
        }
        else {
            /*
             * TBD. Find range and calculate quality as percent.
             */
            cur_ptr->params.types.i802_11.quality = stats.qual.qual;
        }

        /*
         * BSSID
         */
        if (-1 == _RIR_netlGetWirelessExtension(
                sock,
                SIOCGIWAP,
                cur_ptr->name,
                &req)) {
            cur_ptr->params.types.i802_11.bssid[0] = 0;
        }
        else {
            OSAL_memCpy(&cur_ptr->params.types.i802_11.bssid[0],
                    (unsigned char *)&req.u.ap_addr.sa_data[0],
                    sizeof(cur_ptr->params.types.i802_11.bssid));
        }

        cur_ptr = cur_ptr->next_ptr;
    }

    close(sock);
    return (0);
}
/*
 * ======== _RIR_netlChecksum() ========
 *
 * Find checksum of a ping packet.
 *
 * Returns:
 *  Checksum.
 *  0  : OK
 *  -1 : Failed.
 *
 */
static unsigned _RIR_netlChecksum(
    unsigned char *tmp_ptr,
    unsigned short len)
{
    unsigned long  sum;
    unsigned short data;
    for (sum = 0; len > 0; len--) {
        data = tmp_ptr[len - 1] + (tmp_ptr[len - 2] << 8);
        len--;
        sum += data;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (~sum);
}

/*
 * ======== _RIR_netlPingInit() ========
 *
 * Send ping packet from each interface, then prepare to wait for its reply.
 *
 * Returns:
 *  0 : Failed
 *  1 : Success
 *
 */
static int _RIR_netlPingInit(
    unsigned long  serverIp,
    fd_set        *fdset_ptr)
{
    unsigned long          retval;
    struct sockaddr_in     sa;
    struct sockaddr_in     la;
    struct timeval         time;
    unsigned long          timediff;
    _RIR_NetlInfc         *cur_ptr;

    gettimeofday(&time, NULL);
    timediff = (time.tv_sec - _RIR_NetlObj.timeLast.tv_sec) * 1000000 +
            time.tv_usec - _RIR_NetlObj.timeLast.tv_usec;
    if (timediff < _RIR_NETL_PING_INTERVAL_US) {
        /*
         * Dont ping more often than this time.
         */
        return (0);
    }
    memcpy(&_RIR_NetlObj.timeLast, &time, sizeof(time));

    /*
     * Prepare an ICMP packet.
     */

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(serverIp);
    sa.sin_port = 0;
    time.tv_sec = 1;
    time.tv_usec = 0;

    /*
     * Send ping for all interfaces.
     */
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr;) {

        /*
         * Very large value.
         */
        cur_ptr->params.rtPing = 10000;

        memset(&la, 0, sizeof(la));
        la.sin_family = AF_INET;
        la.sin_addr.s_addr = htonl(cur_ptr->params.addr.ipv4);
        la.sin_port = 0;
        memset(&cur_ptr->icmp, 0, sizeof(cur_ptr->icmp));
        cur_ptr->icmp.type = 8;
        cur_ptr->icmp.code = 0;
        cur_ptr->icmp.checksum = 0;
        cur_ptr->icmp.id = htons(getpid());
        cur_ptr->icmp.sequence = (unsigned short)random();
        strncpy(cur_ptr->icmp.data, cur_ptr->name,
                sizeof(cur_ptr->icmp.data) - 1);
        cur_ptr->icmp.checksum = htons(
                _RIR_netlChecksum((unsigned char *)&(cur_ptr->icmp),
                sizeof(cur_ptr->icmp)));

        /*
         * May need to close last time opened socket.
         */
        if (cur_ptr->sock >= 0) {
            close(cur_ptr->sock);
            cur_ptr->sock = -1;
        }

        /*
         * Only if interface can send IP packet
         */
        if ((cur_ptr->params.addr.ipv4) && (cur_ptr->params.up)) {

            cur_ptr->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
            if (cur_ptr->sock < 0) {
                _RIR_logMsg("%s %d: infc %s\n", __FILE__, __LINE__,
                        cur_ptr->name);
            }
            else {

                /*
                 * So the program does not hang. Nonblocking.
                 */
                retval = fcntl(cur_ptr->sock, F_GETFL);
                retval |= O_NONBLOCK;
                fcntl(cur_ptr->sock, F_SETFL, retval);

                /*
                 * bind to send from this infc. non blocking socket so our 
                 * program does not hang if there is error.
                 */
                if (0 != bind(cur_ptr->sock, (struct sockaddr *)&la,
                        sizeof(la))) {
                    close(cur_ptr->sock);
                    cur_ptr->sock = -1;
                    _RIR_logMsg("%s %d: infc %s\n", __FILE__, __LINE__,
                            cur_ptr->name);
                }
                else {
                    /*
                     * Send
                     */
                    retval = sizeof(cur_ptr->icmp);
                    if ((int)retval != sendto(cur_ptr->sock,
                            (void *)&cur_ptr->icmp, retval, 0,
                            (struct sockaddr *)&sa, sizeof(sa))) {
                        close(cur_ptr->sock);
                        cur_ptr->sock = -1;
                        _RIR_logMsg("%s %d: infc %s\n", __FILE__, __LINE__,
                                cur_ptr->name);
                    }
                    else {
                        /*
                         * Get time we sent out this packet.
                         * Also put in expected select list.
                         */
                        gettimeofday(&cur_ptr->time, NULL);
                        FD_SET(cur_ptr->sock, fdset_ptr);
                    }
                }
            }
        }
        cur_ptr = cur_ptr->next_ptr;
    }
    return (1);
}
/*
 * ======== _RIR_netlPingTest() ========
 *
 * Read ping reply and calcualte round trip latency.
 * If reply not received, then set to -1.
 *
 * Returns:
 *
 */
static void _RIR_netlPingTest(
    fd_set *fdset_ptr)
{
    _RIR_NetlInfc         *cur_ptr;
    int                     retval;
    unsigned long           sa;

    /*
     * Check if a socket is set in the FD list for any interface.
     */
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {
        if (cur_ptr->sock < 0) {
            cur_ptr = cur_ptr->next_ptr;
            continue;
        }
        if (FD_ISSET(cur_ptr->sock, fdset_ptr)) {
            memset(&sa, 0, sizeof(sa));
            memset(&_RIR_NetlObj.data, 0, sizeof(_RIR_NetlObj.data));
            /*
             * Got something.
             */
            retval = sizeof(sa);
            retval = recvfrom(cur_ptr->sock, (void *)&_RIR_NetlObj.data,
                    sizeof(_RIR_NetlObj.data), 0, (struct sockaddr *)&sa,
                    (socklen_t *)&retval);
            gettimeofday(&cur_ptr->time1, NULL);
            if (retval >= ((int)(sizeof(cur_ptr->icmp) + 20))) {
                memcpy(&cur_ptr->icmp, &_RIR_NetlObj.data[20],
                        sizeof(cur_ptr->icmp));
                if(!strncmp((char *)cur_ptr->icmp.data,
                        cur_ptr->name, sizeof(cur_ptr->icmp.data))) {
                    cur_ptr->params.rtPing = cur_ptr->time1.tv_sec -
                            cur_ptr->time.tv_sec;
                    cur_ptr->params.rtPing *= 1000;
                    cur_ptr->params.rtPing += (cur_ptr->time1.tv_usec -
                            cur_ptr->time.tv_usec) / 1000;
                    /*
                     * Round trip latency calculated in ms.
                     */
                }
            }
        }
        memset(&cur_ptr->time, 0, sizeof(cur_ptr->time));
        memset(&cur_ptr->time1, 0, sizeof(cur_ptr->time1));
        cur_ptr = cur_ptr->next_ptr;
    }
}

/*
 * ======== _RIR_netlFindInterfaceorAdd() ========
 *
 * Find wireless interface in interfaces linked list using its name.
 * If not found then add and init a new interface in the list.
 *
 * Returns:
 *  Pointer to the interface (found or added)
 *
 */
static _RIR_NetlInfc *_RIR_netlFindInterfaceorAdd(
    char *name_ptr,
    int   index)
{

    if (!strcmp("lo", name_ptr)) {
        /*
         * No local loopback.
         */
        return (NULL);
    }

    /*
     * Keep a linked list of all system interfaces.
     */
    _RIR_NetlInfc *last_ptr = NULL;
    _RIR_NetlInfc *cur_ptr;

    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {
        if (!strncmp(name_ptr, cur_ptr->name,
                sizeof(cur_ptr->name))) {
            /*
             * Exists.
             */
            cur_ptr->index = index;
            return (cur_ptr);
        }
        last_ptr = cur_ptr;
        cur_ptr = (_RIR_NetlInfc *)cur_ptr->next_ptr;
    }

    /*
     * Not found. Add.
     */
    cur_ptr = malloc(sizeof(_RIR_NetlInfc));
    if (NULL == cur_ptr) {
        _RIR_logMsg("%s %d: infc %s, not enough memory\n",
                __FILE__, __LINE__, name_ptr);
        return (NULL);
    }
    memset(cur_ptr, 0, sizeof(_RIR_NetlInfc));
    strncpy(cur_ptr->name, name_ptr, sizeof(cur_ptr->name) - 1);
    if (NULL != last_ptr) {
        last_ptr->next_ptr = cur_ptr;
    }
    else {
        _RIR_NetlObj.interfaces_ptr = cur_ptr;
    }

    /*
     * Init here.
     */
    cur_ptr->params.rtPing = 10000;
    cur_ptr->index         = index;
    cur_ptr->sock          = -1;

    _RIR_logMsg("%s %d: Add infc index=%d, name=%s\n", __func__, __LINE__,
            cur_ptr->index, cur_ptr->name);

    return (cur_ptr);
}

/*
 * ======== _RIR_netlFindInterfaceFromIndex() ========
 *
 * Find wireless interface from its netlink index in interfaces linked list.
 *
 * Returns:
 *  NULL : no such interface
 *  Or pointer to found interface.
 *
 */
static _RIR_NetlInfc *_RIR_netlFindInterfaceFromIndex(
    int index)
{
    /*
     * Keep a linked list of all system interfaces.
     */
    _RIR_NetlInfc *cur_ptr;
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {
        if (index == cur_ptr->index) {
            return (cur_ptr);
        }
        cur_ptr = (_RIR_NetlInfc *)cur_ptr->next_ptr;
    }
    return (NULL);
}

/*
 * ======== _RIR_netlParseLinkDel() ========
 *
 * Called on netlink link deletion event to check what happened.
 *
 * Returns:
 *
 */
void _RIR_netlParseLinkDel(
    struct nlmsghdr *h_ptr)
{
    struct ifinfomsg *info_ptr;
    _RIR_NetlInfc   *infc_ptr;
    int               index;

    /*
     * Find values and pointers.
     */
    info_ptr = NLMSG_DATA(h_ptr);
    index = info_ptr->ifi_index;

    infc_ptr = _RIR_netlFindInterfaceFromIndex(index);
    if (NULL != infc_ptr) {
        infc_ptr->params.up = 0;
    }
}

/*
 * ======== _RIR_netlParseLinkDel() ========
 *
 * Called on netlink link event to check what happened.
 *
 * Returns:
 *
 */
void _RIR_netlParseLink(
    struct nlmsghdr *h_ptr)
{
    struct ifinfomsg *info_ptr;
    struct rtattr    *a_ptr;
    char             *buf_ptr;
    int               len;
    _RIR_NetlInfc    *infc_ptr;
    int               index;
    int               ipv6Count;

    /*
     * Find values and pointers.
     */
    info_ptr = NLMSG_DATA(h_ptr);
    a_ptr = IFLA_RTA(info_ptr);
    len = IFLA_PAYLOAD(h_ptr);
    index = info_ptr->ifi_index;

    while (RTA_OK(a_ptr, len)) {
        buf_ptr = RTA_DATA(a_ptr);
        if (IFLA_IFNAME == a_ptr->rta_type) {
            /*
             * This is where interface gets added.
             */
            infc_ptr = _RIR_netlFindInterfaceorAdd(buf_ptr, index);
        }
        a_ptr = RTA_NEXT(a_ptr, len);
    }

    infc_ptr = _RIR_netlFindInterfaceFromIndex(index);
    if (NULL != infc_ptr) {
        /*
         * Find link up/down.
         */
        if ((IFF_UP & info_ptr->ifi_flags) &&
                (IFF_LOWER_UP & info_ptr->ifi_flags) &&
                (IFF_RUNNING & info_ptr->ifi_flags)) {
            /*
             * Link is fully up and running.
             */
            infc_ptr->params.up = 1;
        }
        else {
            /*
             * Link is not up.
             */
            infc_ptr->params.up = 0;
            /*
             * Clear all addresses while link down.
             */
            infc_ptr->params.addr.ipv4 = 0;
            ipv6Count = infc_ptr->params.addr.ipv6Count;
            for (index = 0; index < ipv6Count; index++) {
                OSAL_memSet(&infc_ptr->params.addr.ipv6[index], 0,
                        sizeof(_RIR_NetIpv6Addr));
                /* If find the ip, break for loop */
                infc_ptr->params.addr.ipv6Count--;
            }
        }
    }
}

/*
 * ======== _RIR_netlParseAddrDel() ========
 *
 * Called on netlink IP address deletion event to check what happened.
 *
 * Returns:
 *
 */
void _RIR_netlParseAddrDel(
    struct nlmsghdr *h_ptr)
{
    struct ifaddrmsg    *info_ptr;
    _RIR_NetlInfc       *infc_ptr;
    int                  index;
    struct rtattr       *a_ptr;
    char                *buf_ptr;
    int                  len;
    int                  size;
    unsigned short       in6Addr[8];

    /*
     * Find values and pointers.
     */
    info_ptr = NLMSG_DATA(h_ptr);
    a_ptr    = IFA_RTA(info_ptr);
    len      = IFA_PAYLOAD(h_ptr);
    index    = info_ptr->ifa_index;
    infc_ptr = _RIR_netlFindInterfaceFromIndex(index);

    if (NULL == infc_ptr) {
        _RIR_logMsg("%s %d: Cannot find interface from index(%d).\n",
                __func__, __LINE__, index);
        return;
    }


    while(RTA_OK(a_ptr, len)) {
        buf_ptr = RTA_DATA(a_ptr);
        if ((IFA_ADDRESS == a_ptr->rta_type) &&
                (AF_INET6 == info_ptr->ifa_family)) {
            /* Convert buf_ptr Ntoh to in6Addr */
            OSAL_netIpv6Ntoh(in6Addr, (unsigned short *)buf_ptr);
            /* Look for what IP is deleted and rearrange ipv6 database. */
            for (index = 0; index < infc_ptr->params.addr.ipv6Count; index++) {
                if (0 == OSAL_memCmp(infc_ptr->params.addr.ipv6[index].addr,
                        in6Addr, sizeof(struct in6_addr))) {
                    if (index == (infc_ptr->params.addr.ipv6Count - 1)) {
                        /* the last IPv6 IP and set it to 0 */
                        OSAL_memSet(&infc_ptr->params.addr.ipv6[index], 0,
                            sizeof(_RIR_NetIpv6Addr));
                    }
                    else {
                        /* move others IP which is after found IP to front */
                        size = infc_ptr->params.addr.ipv6Count - index - 1;
                        OSAL_memCpy(&infc_ptr->params.addr.ipv6[index],
                                &infc_ptr->params.addr.ipv6[index + 1],
                                sizeof(_RIR_NetIpv6Addr) * size);
                        /* set last IP as 0 */
                        OSAL_memSet(&infc_ptr->params.addr.ipv6[
                                    infc_ptr->params.addr.ipv6Count],
                                0, sizeof(_RIR_NetIpv6Addr));
                    }
                    /* If find the ip, break for loop */
                    infc_ptr->params.addr.ipv6Count--;
                    break;
                }
            }
        }
        else if((IFA_ADDRESS == a_ptr->rta_type) &&
                (AF_INET == info_ptr->ifa_family)) {
            infc_ptr->params.addr.ipv4 = 0;
        }
        a_ptr = RTA_NEXT(a_ptr, len);
    }

}

/*
 * ======== _RIR_netInsertIpv6Addr() ========
 *
 * Insert IPv6 address into interface IPv6 pool.
 *
 * Returns:
 *
 */
void _RIR_netInsertIpv6Addr(
    _RIR_NetlInfc      *infc_ptr,
    unsigned short     *addr_ptr,
    _RIR_Ipv6AddrType   type)
{
    int             idx;
    int             count;
    OSAL_Boolean    isExist;

    isExist = OSAL_FALSE;
    count   = infc_ptr->params.addr.ipv6Count;

    /* If IPv6 pool is full, cannot put the ip to pool */
    if (_RIR_NETL_MAX_IPV6_COUNT == count) {
        OSAL_logMsg("%s:%d Cannot put the ip into IPv6 pool.\n",
                __FUNCTION__, __LINE__);
        return;
    }

    /* Check if this ip has been in IPv6 pool */
    for (idx = 0; idx < count; idx++) {
        if (0 == OSAL_memCmp(infc_ptr->params.addr.ipv6[idx].addr,
                addr_ptr, sizeof(struct in6_addr))) {
            isExist = OSAL_TRUE;
            break;
        }
    }

    /* If this IP is not in ipv6 pool, add it into pool. */
    if (OSAL_FALSE == isExist) {
        /*
         * The first ip is the heighest priority which will be reported.
         * If there is temporary ipv6 address, we should report it.
         * Therefore need to adjust the position in ipv6 pool.
         * The permanent ip is always last in pool.
         */
        if ((0 == count) || (_RIR_ADDR_TYPE_PERMANENT == type)) {
            OSAL_memCpy(infc_ptr->params.addr.ipv6[count].addr, addr_ptr,
                    sizeof(struct in6_addr));
            infc_ptr->params.addr.ipv6[count].type = type;
            infc_ptr->params.addr.ipv6Count++;
        }
        else {
            if (_RIR_ADDR_TYPE_PERMANENT ==
                    infc_ptr->params.addr.ipv6[count - 1].type) {
                OSAL_memCpy(&infc_ptr->params.addr.ipv6[count],
                        &infc_ptr->params.addr.ipv6[count - 1],
                        sizeof(_RIR_NetIpv6Addr));
                OSAL_memCpy(infc_ptr->params.addr.ipv6[count - 1].addr,
                        addr_ptr, sizeof(struct in6_addr));
                infc_ptr->params.addr.ipv6[count - 1].type = type;
                infc_ptr->params.addr.ipv6Count++;
            }
            else {
                OSAL_memCpy(infc_ptr->params.addr.ipv6[count].addr, addr_ptr,
                        sizeof(struct in6_addr));
                infc_ptr->params.addr.ipv6[count].type = type;
                infc_ptr->params.addr.ipv6Count++;
            }
        }
    }
}

/*
 * ======== _RIR_netlParseAddr() ========
 *
 * Called on netlink IP address assign event to check what happened.
 *
 * Returns:
 *
 */
void _RIR_netlParseAddr(
    struct nlmsghdr *h_ptr)
{
    struct ifaddrmsg    *info_ptr;
    struct rtattr       *a_ptr;
    char                *buf_ptr;
    int                  len;
    _RIR_NetlInfc       *infc_ptr;
    unsigned long        ipv4;
    int                  buflen;
    int                  index;
    unsigned short       in6Addr[OSAL_NET_IPV6_WORD_SZ];

    /*
     * Find values and pointers.
     */
    info_ptr = NLMSG_DATA(h_ptr);
    a_ptr    = IFA_RTA(info_ptr);
    len      = IFA_PAYLOAD(h_ptr);
    index    = info_ptr->ifa_index;

    infc_ptr = _RIR_netlFindInterfaceFromIndex(index);
    if (NULL == infc_ptr) {
        _RIR_logMsg("%s %d: Cannot find interface from index(%d).\n",
                __func__, __LINE__, index);
        return;
    }

    while(RTA_OK(a_ptr, len)) {
        buf_ptr = RTA_DATA(a_ptr);
        buflen = RTA_PAYLOAD(a_ptr);
        if ((IFA_ADDRESS == a_ptr->rta_type) &&
                (AF_INET6 == info_ptr->ifa_family)) {
            if (!IN6_IS_ADDR_LINKLOCAL((struct in6_addr *)buf_ptr)) {
                OSAL_netIpv6Ntoh(in6Addr, (unsigned short *)buf_ptr);
                /* Put the new IPv6 address to ipv6 pool */
                if (0 != (info_ptr->ifa_flags & IFA_F_TEMPORARY)) {
                    _RIR_netInsertIpv6Addr(infc_ptr, in6Addr,
                            _RIR_ADDR_TYPE_TEMPORARY);
                }
                else {
                    _RIR_netInsertIpv6Addr(infc_ptr, in6Addr,
                            _RIR_ADDR_TYPE_PERMANENT);
                }
            }
        }
        else if((IFA_ADDRESS == a_ptr->rta_type) &&
                (AF_INET == info_ptr->ifa_family)) {
            memcpy(&ipv4, buf_ptr, buflen);
            infc_ptr->params.addr.ipv4 = OSAL_netNtohl(ipv4);
        }
        a_ptr = RTA_NEXT(a_ptr, len);
    }
}

/*
 * ======== _RIR_netlSendMsg() ========
 *
 * Called to send netlink message.
 *
 * Returns:
 *
 */
void _RIR_netlSendMsg(
    int msgType)
{

    struct sockaddr_nl   sa;       /* the remote(kernel) side of the communication */
    struct iovec         iov;      /* IO vector for sendmsg */
    struct msghdr        rtnl_msg; /* for sending and receiving */
    struct {
        struct nlmsghdr hdr;
        struct rtgenmsg rtg;
    } req; /* describes the rtnetlink packet itself */

    /* Send request msg */
    memset(&sa, 0, sizeof(sa));
    memset(&iov, 0, sizeof(iov));
    memset(&req, 0, sizeof(req));
    memset(&rtnl_msg, 0, sizeof(rtnl_msg));

    /* Construct request */
    req.hdr.nlmsg_len    = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    req.hdr.nlmsg_type   = msgType;
    req.hdr.nlmsg_flags  = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
    req.hdr.nlmsg_seq    = 1;
    req.hdr.nlmsg_pid    = 0;
    req.rtg.rtgen_family = AF_PACKET; /*  no preferred AF, we will get *all* interfaces */

    sa.nl_family         = AF_NETLINK; /* fill-in kernel address (destination of our message) */
    iov.iov_base         = &req;
    iov.iov_len          = req.hdr.nlmsg_len;
    rtnl_msg.msg_iov     = &iov;
    rtnl_msg.msg_iovlen  = 1;
    rtnl_msg.msg_name    = &sa;
    rtnl_msg.msg_namelen = sizeof(sa);

    sendmsg(_RIR_NetlObj.fd, (struct msghdr *) &rtnl_msg, 0);
}

/*
 * ======== _RIR_netlRecvMsg() ========
 *
 * Called to get netlink message data.
 *
 * Returns:
 *  1: recevie message type NLMSG_DONE.
 *  0: receive others message type.
 */
int _RIR_netlRecvMsg(
    void)
{
    struct sockaddr_nl   addr;
    socklen_t            addrlen;
    struct nlmsghdr     *msg_ptr;
    int                  ret;
    int                  len;
    _RIR_Netl *n_ptr = &_RIR_NetlObj;

    ret = 0;

    /*
     * Get data from netlink.
     */
    addrlen = sizeof(struct sockaddr_nl);
    if ((len = recvfrom(n_ptr->fd, n_ptr->data, sizeof(n_ptr->data),
            0, (struct sockaddr *)&addr, &addrlen)) <= 0) {
        _RIR_logMsg("%s %d\n", __FILE__, __LINE__);
        return ret;
    }

    for (msg_ptr = (struct nlmsghdr *)n_ptr->data;
            NLMSG_OK(msg_ptr, (unsigned int)len);
            msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
        switch(msg_ptr->nlmsg_type) {
            case RTM_NEWLINK:
                _RIR_netlParseLink(msg_ptr);
                break;
            case RTM_DELLINK:
                _RIR_netlParseLinkDel(msg_ptr);
                break;
            case RTM_NEWADDR:
                _RIR_netlParseAddr(msg_ptr);
                break;
            case RTM_DELADDR:
                _RIR_netlParseAddrDel(msg_ptr);
                break;
            case NLMSG_DONE:
                ret = 1;
                break;
            default:
                break;
        }
    }

    return ret;
}

/*
 * ======== _RIR_netlGetAllNetworkInterfaces() ========
 *
 * Called at init to find all interfaces that exist in the system
 * at that time.  Then this adds those interfaces to the linked list.
 *
 * Returns:
 *
 */
void _RIR_netlGetAllNetworkInterfaces(
    void)
{
    int  exit;

    /* Query all system network interface */
    _RIR_netlSendMsg(RTM_GETLINK);

    /* Receive response */
    exit = 0;
    while (!exit) {
        exit = _RIR_netlRecvMsg();
    }

    /* Query all system network interface ip address */
    _RIR_netlSendMsg(RTM_GETADDR);

    /* Receive response */
    exit = 0;
    while (!exit) {
        exit = _RIR_netlRecvMsg();
    }
}

/*
 * ======== _RIR_netlSendMcmEvent() ========
 *
 * Send an event to RIR.
 *
 * Returns:
 * -1 : Failed
 *  0 : Success
 */
static int _RIR_netlSendMcmEvent(
    RIR_EventMsg *evt_ptr)
{
    struct timeval  time;
    int             fid;
    char            name[256];
    int             ret = 0;

    /*
     * Time this event was sent.
     */
    gettimeofday(&time, NULL);
    evt_ptr->ticksec = time.tv_sec;
    evt_ptr->tickusec = time.tv_usec;

    if (0 == RIR_netLinkProtocolPath[0]) {
        snprintf(name, sizeof(name), "%s/%s-%08x", OSAL_IPC_FOLDER,
                RIR_EVENT_QUEUE_NAME, sizeof(RIR_EventMsg));
    }
    else {
        snprintf(name, sizeof(name), "%s/%s-%08x", RIR_netLinkProtocolPath,
                RIR_EVENT_QUEUE_NAME, sizeof(RIR_EventMsg));
    }
    fid = open(name, O_RDWR);
    if (fid >= 0) {
        if (sizeof(RIR_EventMsg) != write(fid, (void *)evt_ptr,
                sizeof(RIR_EventMsg))) {
            ret = -1;
        }
    }
    else {
        ret = -1;
    }
    /* close fifo fd */
    if (fid >= 0) {
        close(fid);
    }

    return (ret);
}

/*
 * ======== _RIR_netlGenerateEvents() ========
 *
 * Compare network interface params cache with latest params, and if they
 * are out of tolerance, generate event to RIR.
 *
 * Returns:
 *  -1 : Some failure.
 *   0 : Success
 *
 */
void _RIR_netlGenerateEvents(
    int up)
{
    _RIR_NetlInfc    *cur_ptr;
    _RIR_NetlInfcParams   *params_ptr;
    _RIR_NetlInfcParams   *paramsC_ptr;
    RIR_EventMsg      *evt_ptr = &_RIR_NetlObj.evt;

    /*
     * For all interfaces.
     */
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {
        /*
         * For all interfaces, check differences
         */
        params_ptr = &cur_ptr->params;
        paramsC_ptr = &cur_ptr->paramsCache;

        /*
         * Generate events.
         */
        strncpy(evt_ptr->msg.connectivity.infc,
                cur_ptr->name, sizeof(evt_ptr->msg.connectivity.infc) - 1);
        evt_ptr->msg.connectivity.type = RIR_INTERFACE_TYPE_INVALID;
        if (params_ptr->isWireless) {
           evt_ptr->msg.connectivity.type = RIR_INTERFACE_TYPE_802_11;
        }
        else {
           evt_ptr->msg.connectivity.type = RIR_INTERFACE_TYPE_OTHER;
        }

        /*
         * 0 tolernace means that if up -> down, or down -> generate event.
         */
        if (!_RIR_netlCheckRange(
                &paramsC_ptr->up,
                &params_ptr->up,
                0)) {
            evt_ptr->code = RIR_EVENT_MSG_CODE_CONNECTIVITY;
            evt_ptr->msg.connectivity.code = RIR_CONNECITVITY_MSG_CODE_LINK;
            evt_ptr->msg.connectivity.u.link = params_ptr->up;
            _RIR_netlSendMcmEvent(evt_ptr);
        }

        if (!_RIR_netlCheckRange(
                &paramsC_ptr->rtPing,
                &params_ptr->rtPing,
                _RIR_NETL_PING_TOLERANCE)) {
            evt_ptr->code = RIR_EVENT_MSG_CODE_CONNECTIVITY;
            evt_ptr->msg.connectivity.code = RIR_CONNECITVITY_MSG_CODE_PING;
            evt_ptr->msg.connectivity.u.rtPing = params_ptr->rtPing;
            _RIR_netlSendMcmEvent(evt_ptr);
        }

        if (params_ptr->isWireless) {
            /*
             * BSSID is a locality event.
             */
            if (OSAL_memCmp(
                    paramsC_ptr->types.i802_11.bssid,
                    params_ptr->types.i802_11.bssid,
                    sizeof(params_ptr->types.i802_11.bssid))) {
                OSAL_memCpy(paramsC_ptr->types.i802_11.bssid,
                        params_ptr->types.i802_11.bssid,
                        sizeof(paramsC_ptr->types.i802_11.bssid));
                evt_ptr->code = RIR_EVENT_MSG_CODE_LOCALITY;
                evt_ptr->msg.locality.code = RIR_LOCALITY_MSG_CODE_BSSID;
                OSAL_memCpy(evt_ptr->msg.locality.u.bssid,
                        params_ptr->types.i802_11.bssid,
                        sizeof(evt_ptr->msg.locality.u.bssid));
                _RIR_netlSendMcmEvent(evt_ptr);
            }
        }

        if (0 != OSAL_memCmp(&paramsC_ptr->addr, &params_ptr->addr,
                sizeof(params_ptr->addr))) {
            evt_ptr->code = RIR_EVENT_MSG_CODE_CONNECTIVITY;
            evt_ptr->msg.connectivity.code = RIR_CONNECITVITY_MSG_CODE_IPADDR;
            evt_ptr->msg.connectivity.u.addr.ipv4 = params_ptr->addr.ipv4;
            OSAL_memCpy(evt_ptr->msg.connectivity.u.addr.ipv6,
                    params_ptr->addr.ipv6[0].addr, sizeof(_RIR_NetIpv6Addr));

            /* Check if IPv4 or first IPv6 IP have been changed */
            if (0 != OSAL_memCmp(paramsC_ptr->addr.ipv6[0].addr,
                    params_ptr->addr.ipv6[0].addr, sizeof(_RIR_NetIpv6Addr))) {
                if ((OSAL_netIpv6IsAddrZero(params_ptr->addr.ipv6[0].addr)) &&
                        (0 != params_ptr->addr.ipv4)) {
                    evt_ptr->msg.connectivity.u.addr.type = OSAL_NET_SOCK_UDP;
                }
                else {
                    evt_ptr->msg.connectivity.u.addr.type =
                            OSAL_NET_SOCK_UDP_V6;
                }
                _RIR_netlSendMcmEvent(evt_ptr);
            }
            else if (paramsC_ptr->addr.ipv4 != params_ptr->addr.ipv4) {
                evt_ptr->msg.connectivity.u.addr.type = OSAL_NET_SOCK_UDP;
                _RIR_netlSendMcmEvent(evt_ptr);
            }

            /* Copy current addr information to cache */
            OSAL_memCpy(&paramsC_ptr->addr, &params_ptr->addr,
                sizeof(params_ptr->addr));
        }

        /*
         * Wireless events.
         */
        if (params_ptr->isWireless) {

            if (!_RIR_netlCheckRange(
                    &paramsC_ptr->types.i802_11.bitrate,
                    &params_ptr->types.i802_11.bitrate,
                    _RIR_NETL_BITRATE_TOLERANCE)) {
                evt_ptr->code = RIR_EVENT_MSG_CODE_CONNECTIVITY;
                evt_ptr->msg.connectivity.code = RIR_CONNECITVITY_MSG_CODE_BITRATE;
                evt_ptr->msg.connectivity.u.bitrate = params_ptr->types.i802_11.bitrate;
                _RIR_netlSendMcmEvent(evt_ptr);
            }

            if (!_RIR_netlCheckRange(
                    &paramsC_ptr->types.i802_11.quality,
                    &params_ptr->types.i802_11.quality,
                    _RIR_NETL_QUALITY_TOLERANCE)) {
                evt_ptr->code = RIR_EVENT_MSG_CODE_CONNECTIVITY;
                evt_ptr->msg.connectivity.code = RIR_CONNECITVITY_MSG_CODE_QUALITY;
                evt_ptr->msg.connectivity.u.quality = params_ptr->types.i802_11.quality;
                _RIR_netlSendMcmEvent(evt_ptr);
            }

            strncpy(evt_ptr->msg.locality.infc,
                    cur_ptr->name, sizeof(evt_ptr->msg.locality.infc) - 1);

            /*
             * ESSID is a locality event.
             */
            if (strncmp(
                    paramsC_ptr->types.i802_11.essid,
                    params_ptr->types.i802_11.essid,
                    sizeof(params_ptr->types.i802_11.essid))) {
                if (params_ptr->types.i802_11.essid[0] != 0) {
                    strncpy(paramsC_ptr->types.i802_11.essid,
                            params_ptr->types.i802_11.essid,
                            sizeof(paramsC_ptr->types.i802_11.essid) - 1);
                    evt_ptr->code = RIR_EVENT_MSG_CODE_LOCALITY;
                    evt_ptr->msg.locality.code = RIR_LOCALITY_MSG_CODE_ESSID;
                    strncpy(evt_ptr->msg.locality.u.essid,
                            params_ptr->types.i802_11.essid,
                            sizeof(evt_ptr->msg.locality.u.essid) - 1);
                    _RIR_netlSendMcmEvent(evt_ptr);
                }
            }
        }

        cur_ptr = cur_ptr->next_ptr;
    }

    /*
     * Tell RIR that Netlink is up.
     */
    if (up >= 0) {
        evt_ptr->code = RIR_EVENT_MSG_CODE_STATE;
        evt_ptr->msg.state.code = RIR_STATE_MSG_CODE_NETLINK;
        evt_ptr->msg.state.up = up;
        _RIR_netlSendMcmEvent(evt_ptr);
    }
}

/*
 * ======== _RIR_netlTask() ========
 *
 * One task for all simple sequential coding.
 * This task runs all of netlink using combinations of select,
 * and timeouts to avoid mutex locking.
 *
 * Returns:
 *  NULL.
 */
void *_RIR_netlTask(
    void *arg_ptr)
{
    _RIR_Netl *n_ptr = (_RIR_Netl *)arg_ptr;
    fd_set          rfds;
    struct timeval  time;
    unsigned long   diff;
    struct timeval  times;
    struct timeval  timee;
    int             test;
    int             first = 1;


    diff = 0;
    while (!n_ptr->shut) {

        /*
         * Add netlink FD to set
         */

        FD_ZERO(&rfds);
        FD_SET(n_ptr->fd, &rfds);

        /*
         * Send ping from active interfaces, add their FD to set.
         */
        test = 0;
        if (0 != n_ptr->pingIp) {
            test = _RIR_netlPingInit(n_ptr->pingIp, &rfds);
        }

        /*
         * Timeout of select.
         */
        gettimeofday(&times, NULL);
        do {
            /*
             * Select till timeout.
             */
            time.tv_sec = _RIR_NETL_HEARTBEAT_US / 1000000 ;
            time.tv_usec = _RIR_NETL_HEARTBEAT_US - time.tv_sec * 1000000;

            select(FD_SETSIZE, &rfds, NULL, NULL, &time);

            if (FD_ISSET(n_ptr->fd, &rfds)) {
                /* receive netlink message data */
                _RIR_netlRecvMsg();
            }

            /*
             * Check ping FIDs.
             */
            if (test) {
                _RIR_netlPingTest(&rfds);
            }

            /*
             * Check if a second has elapsed for all ping to come back.
             */
            gettimeofday(&timee, NULL);
            timee.tv_sec -= times.tv_sec;
            timee.tv_usec -= times.tv_usec;
            diff = timee.tv_sec * 1000000 + timee.tv_usec;
        } while (diff < _RIR_NETL_PING_TIMEOUT_US);

        /*
         * Get info for wireless interfaces.
         */
        if (0 != _RIR_netlGetWirelessInfo()) {
            continue;
        }

        /*
         * Now generate netlink based events.
         */
        if (first) {
            _RIR_netlGenerateEvents(1);
            first = 0;
        }
        else {
            _RIR_netlGenerateEvents(-1);
        }
    }

    _RIR_netlGenerateEvents(0);

    n_ptr->shut = 0;
    return (NULL);
}

/*
 * ======== _RIR_netlStart() ========
 *
 * main() function for netlink.
 *
 * Returns:
 *
 */
void _RIR_netlStart(
    unsigned long pingServerIp)
{
    _RIR_Netl *n_ptr = &_RIR_NetlObj;
    pthread_t tid;

    memset(&_RIR_NetlObj, 0, sizeof(_RIR_NetlObj));
    gettimeofday(&_RIR_NetlObj.timeLast, NULL);
    _RIR_NetlObj.pingIp = pingServerIp;

    /*
     * Netlink socket.
     */
    if ((n_ptr->fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        _RIR_logMsg("%s %d\n", __FILE__, __LINE__);
        return;
    }

    /*
     * We are interested in
     * Link changes
     * IPV4 address changes.
     */
    n_ptr->addr.nl_family = AF_NETLINK;
    n_ptr->addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR |
            RTMGRP_IPV6_IFADDR;
    if (bind(n_ptr->fd, (struct sockaddr *)&n_ptr->addr,
            sizeof(n_ptr->addr)) < 0) {
        _RIR_logMsg("%s %d\n", __FILE__, __LINE__);
        close(n_ptr->fd);
        return;
    }

    /*
     * Init interfaces link layer.
     */
    _RIR_netlGetAllNetworkInterfaces();

    /*
     * Go!
     */
    if (0 != pthread_create(&tid, NULL, _RIR_netlTask, (void *)n_ptr)) {
        _RIR_logMsg("%s %d\n", __FILE__, __LINE__);
        close(n_ptr->fd);
        return;
    }
}

/*
 * ======== _RIR_netlStop() ========
 *
 * Exit function for netlink.
 *
 * Returns:
 *
 */
void _RIR_netlStop(
    void)
{
    _RIR_NetlInfc *cur_ptr;
    _RIR_NetlInfc *last_ptr;

    /*
     * Tell netlink task to exit.
     */
    _RIR_NetlObj.shut = 1;
    while (_RIR_NetlObj.shut) {
        sleep(1);
    }

    /*
     * Free up memory.
     */
    for (cur_ptr = _RIR_NetlObj.interfaces_ptr; cur_ptr; ) {
        last_ptr = cur_ptr;
        cur_ptr = (_RIR_NetlInfc *)cur_ptr->next_ptr;
        if (NULL != last_ptr) {
            free(last_ptr);
        }
    }

    /*
     * close netlink socket
     */
    if (_RIR_NetlObj.fd >= 0) {
        close(_RIR_NetlObj.fd);
        _RIR_NetlObj.fd = -1;
    }
}
