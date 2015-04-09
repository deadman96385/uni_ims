/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_sip.h"
#include "sip_hdrflds.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_debug.h"
#include "sip_port.h"
#include "sip_xport.h"
#include "_sip_descr.h"
#include "_sip_drvr.h"
#include "_sip_resolv.h"
#include <ims_net.h>

/* NOTE: *********************************************
 * This file is based on the logic in 4.1 of rfc3263 *
 */

/* 
 *****************************************************************************
 * ================_TRANSPORT_DnsARec()===================
 *
 * This function performs DNS A record address lookups.
 * 
 * pDomainName = A pointer to a string containing a domain to look up
 *
 * pDnsBuff = A buffer used for sending/recv'ing DNS packets
 *
 * dnsBuffSize = The max size of the buffer described above.
 *
 * pAddr = A pointer to an array of tAddrSet objects.
 *
 * addrSize = The max size (# of cells) in the pAddr array
 *
 * port = the port number applicable to this set of addresses
 *
 * RETURNS:
 *      The number of resolved addresses
 *
 ******************************************************************************
 */
static vint _TRANSPORT_DnsARec(
    char            *pDomainName,
    char            *pDnsBuff,
    vint             dnsBuffSize,
    OSAL_NetAddress *pAddr,
    vint             addrSize,
    uint16           port)
{
    char                *pIp;
    char                *pIpLast;
    vint                 x;
    OSAL_NetAddress      addr;
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
           "_TRANSPORT_DnsARec: lookup for %s", (int)pDomainName, 0, 0);

    if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)pDomainName, (int8 *)pDnsBuff, 
                dnsBuffSize, 2, 4, OSAL_NET_RESOLVE_A)) {
        return (SIP_FAILED);
    } 
    
    pIp = pIpLast = pDnsBuff;
    x = 0;
    while ((NULL != (pIp = OSAL_strscan(pIp, "\n"))) && (x < addrSize)) {
        *pIp = 0;
        pIp++;

        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "DNS 'A' Response recv'd:%s\n",
                (int)pIpLast, 0, 0);

        if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)pIpLast, &addr)) {
            pAddr->ipv4 = addr.ipv4;
            pAddr->port = port;
            pAddr++;
            x++;
        }
        pIpLast = pIp;
    }
    /* Return the number of resolved addresses */
    return (x);
}

/*
 *****************************************************************************
 * ================_TRANSPORT_DnsAAAARec()===================
 *
 * This function performs DNS AAAA record address lookups.
 *
 * pDomainName = A pointer to a string containing a domain to look up
 *
 * pDnsBuff = A buffer used for sending/recv'ing DNS packets
 *
 * dnsBuffSize = The max size of the buffer described above.
 *
 * pAddr = A pointer to an array of tAddrSet objects.
 *
 * addrSize = The max size (# of cells) in the pAddr array
 *
 * port = the port number applicable to this set of addresses
 *
 * RETURNS:
 *      The number of resolved addresses
 *
 ******************************************************************************
 */
static vint _TRANSPORT_DnsAAAARec(
    char            *pDomainName,
    char            *pDnsBuff,
    vint             dnsBuffSize,
    OSAL_NetAddress *pAddr,
    vint             addrSize,
    uint16           port)
{
    char                *pIp;
    char                *pIpLast;
    vint                 x;
    OSAL_NetAddress      addr;
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
           "_TRANSPORT_DnsAAAARec: lookup for %s", (int)pDomainName, 0, 0);

    if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)pDomainName, (int8 *)pDnsBuff, 
                dnsBuffSize, 2, 4, OSAL_NET_RESOLVE_AAAA)) {
        return (SIP_FAILED);
    } 

    pIp = pIpLast = pDnsBuff;
    x = 0;
    while ((NULL != (pIp = OSAL_strscan(pIp, "\n"))) && (x < addrSize)) {
        *pIp = 0;
        pIp++;
        addr.type = OSAL_NET_SOCK_UDP_V6;
        if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)pIpLast, &addr)) {
            OSAL_memCpy(pAddr->ipv6, addr.ipv6, sizeof(addr.ipv6));
            pAddr->port = port;
            pAddr->type = OSAL_NET_SOCK_UDP_V6;
            pAddr++;
            x++;
        }
        pIpLast = pIp;
    }
    /* Return the number of resolved addresses */
    return (x);
}

/*
 *****************************************************************************
 * ================_TRANSPORT_DnsNaptrRec()===================
 *
 * This function performs DNS NAPTR record address lookups.
 * And parse the result and return a highest priority transport type
 *
 * pDomainName = A pointer to a string containing a domain to look up
 *
 * pDnsBuffer = A buffer used for sending/recv'ing DNS packets
 *
 * dnsBuffSize = The max size of the buffer described above.
 *
 * pTType = A pointer to an array of tTransportType objects.
 *
 * tTypeSize = The max size (# of cells) in the pTType array
 *
 * RETURNS:
 *      The number of resolved addresses
 *
 ******************************************************************************
 */
static vint _TRANSPORT_DnsNaptrRec(
    char            *pDomainName,
    char            *pDnsBuffer,
    vint             dnsBuffSize,
    tTransportType  *pTType,
    vint             tTypeSize)
{
    tDnsAns        dnsAns[MAX_DNS_NAPTR_ANS + 1];
    char          *now_ptr;
    int            i, swap, cnt;
    tTransportType tType;

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
           "_TRANSPORT_DnsNaptrRec: lookup for %s", (int)pDomainName, 0, 0);

    if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)pDomainName, (int8 *)pDnsBuffer,
                dnsBuffSize, 2, 4, OSAL_NET_RESOLVE_NAPTR)) {
        return (SIP_FAILED);
    }

    /* Parse DNS NAPTR records, order:preference:flag:service\n ... */
    now_ptr = pDnsBuffer;
    i = 0;
    cnt = 0;

    while ((0 != *pDnsBuffer) && (cnt < MAX_DNS_NAPTR_ANS)) {
        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
        dnsAns[cnt].u.naptr.order = OSAL_atoi(pDnsBuffer);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
        dnsAns[cnt].u.naptr.preference = OSAL_atoi(pDnsBuffer);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
         OSAL_strncpy(dnsAns[cnt].u.naptr.flag, pDnsBuffer,
                 MAX_FLAG_STR_LEN);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, "\n"))) {
            break;
        }
        *now_ptr = 0;
        OSAL_strncpy(dnsAns[cnt].u.naptr.service, pDnsBuffer,
                MAX_DOMAIN_NAME_LEN);
        pDnsBuffer = now_ptr + 1;

        cnt++;
    }

    /* Sort list of records based on order */
    do {
        swap = 0;
        for (i = 0; i < (cnt - 1); i++) {
            if (dnsAns[i].u.naptr.order >
                    dnsAns[i + 1].u.naptr.order) {
                dnsAns[cnt] = dnsAns[i];
                dnsAns[i] = dnsAns[i + 1];
                dnsAns[i + 1] = dnsAns[cnt];
                swap = 1;
            }
        }
    } while (swap);

    /* Convert service to transport type and copy to pTType */
    for (i = 0; i < cnt && i < tTypeSize; i++) {
        if (0 == OSAL_strcasecmp("SIP+D2U", dnsAns[i].u.naptr.service)) {
            tType = eTransportUdp;
        }
        else if (0 == OSAL_strcasecmp("SIP+D2T", dnsAns[i].u.naptr.service)) {
            tType = eTransportTcp;
        }
        else if (0 == OSAL_strcasecmp("SIPS+D2T", dnsAns[i].u.naptr.service)) {
            tType = eTransportTls;
        }
        else {
            tType = eTransportNone;
        }
        pTType[i] = tType;
    }
    /* Return the number of resolved addresses */
    return (i);
}

/* 
 *****************************************************************************
 * ================_TRANSPORT_GetTransport()===================
 *
 * This function determines what the transport type should be for the URI
 * specified in pUri.  these rules are defined in RFC3263
 *
 * pUri = A pointer to a URI
 *
 * pTransport = A pointer to a tTransportType.  This value will be populated
 *              with the transport type that the URI should use for SIP
 *              message transmission.
 *
 * RETURNS:
 *      SIP_OK: determined what type of transport to use
 *      SIP_NOT_FOUND: Couldn't determine the transport to use.
 *                     A NAPTR lookup should be performed after this function.
 *
 ******************************************************************************
 */
vint _TRANSPORT_GetTransport (
    tTransport     *pTrans,
    tUri           *pUri,
    tTransportType *pTransport,
    vint            useNaptr)
{
    char          *pDnsBuffer;
    tTransportType tTypeSet[MAX_DNS_NAPTR_ANS]; /* array of tTransportType */

    pDnsBuffer = pTrans->dnsScratch.buffer;


    if (pUri->transport != eTransportNone) {
        *pTransport = pUri->transport;
        return (SIP_OK);
    }
    else if ((pUri->host.addressType == eNwAddrIPv4) ||
            (pUri->host.addressType == eNwAddrIPv6)) {
        if (pUri->scheme == eURI_SCHEME_SIPS) {
            *pTransport = eTransportTls;
        }
        else {
            *pTransport = eTransportUdp;
        }
        return (SIP_OK);
    }
    else if (pUri->host.addressType == eNwAddrDomainName) {
        if (pUri->host.port) {
            if (pUri->scheme == eURI_SCHEME_SIPS) {
                *pTransport = eTransportTls;
            }
            else {
                *pTransport = eTransportUdp;
            }
            return (SIP_OK);
        }
    } /* end of else if */

    /* No transport found, check if need to do NAPTR */
    if (useNaptr) {
        /* If use NAPTR then do DNS NAPTR query */
        if (0 < _TRANSPORT_DnsNaptrRec(pUri->host.x.domainName, pDnsBuffer,
                    MAX_DNS_BUFFER_SIZE_BYTES, tTypeSet, MAX_DNS_NAPTR_ANS)) {
            /* Now we only get the first priority transport type */
            *pTransport = tTypeSet[0];
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                       "_TRANSPORT_DnsNaptrRec: transport type=%d", (int)*pTransport, 0, 0);
            return (SIP_OK);
        }
    }

    /* No transport found */
    return (SIP_NOT_FOUND);
}

/*
 *****************************************************************************
 * ================_TRANSPORT_GetSrv()===================
 *
 * This function that performs DNS SRV record address lookups.
 *
 * pTrans = A pointer to the tTransport object making the DNS SRV look up
 *
 * pDomainName = A pointer to a string containing a domain
 *
 * tTransport  = Transport type TCP, UDP, or TCP/TLS
 *
 * RETURNS:
 *      SIP_OK: Has resolved some IP addresses from SRV table
 *      SIP_FAILED: Couldn't resolve any IP addresses.
 *
 ******************************************************************************
 */
static vint _TRANSPORT_GetSrv(
    tTransport     *pTrans,
    char           *pDomainName,
    tTransportType  transport)
{
    char    *pDnsName;
    char    *pDnsBuffer;
    tDnsAns *dnsAn_ptr[MAX_DNS_SRV_ANS + 1];
    char    *now_ptr;
    int      i, x, swap, cnt;
    
    pDnsName = pTrans->dnsScratch.domain;
    pDnsBuffer = pTrans->dnsScratch.buffer;
    
    /* 
     * first construct the SRV string
     */
    switch (transport) {
    case eTransportTcp:
        OSAL_strncpy(pDnsName, "_sip._tcp.", MAX_DOMAIN_NAME_LEN);
        x = 10;
        break;
    case eTransportTls:
        OSAL_strncpy(pDnsName, "_sips._tcp.", MAX_DOMAIN_NAME_LEN);
        x = 11;
        break;
    default:
        OSAL_strncpy(pDnsName, "_sip._udp.", MAX_DOMAIN_NAME_LEN);
        x = 10;
        break;
    }

    /* 
     * now concatenate the domain name.  
     * Make sure we don't overflow any buffers  
     */
    if (NULL == (now_ptr = OSAL_strscan(pDomainName, "."))) {
        return (SIP_FAILED);
    }
    OSAL_strncpy(pDnsName + x, now_ptr + 1, MAX_DOMAIN_NAME_LEN - x);
    
    if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)pDnsName, (int8 *)pDnsBuffer,
            MAX_DNS_BUFFER_SIZE_BYTES, 2, 4, OSAL_NET_RESOLVE_SRV)) { 
        /* priority:weight:port:target */
        return (SIP_FAILED);
    }

    /* Parse DNS SRV records, priority:weight:port:target\n ... */
    now_ptr = pDnsBuffer;
    i = 0;
    cnt = 0;
    x = 0;

    while (0 != *pDnsBuffer) {
        dnsAn_ptr[cnt] = &pTrans->dnsScratch.ans[cnt];
        
        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
        dnsAn_ptr[cnt]->u.srv.priority = OSAL_atoi(pDnsBuffer);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
        dnsAn_ptr[cnt]->u.srv.weight = OSAL_atoi(pDnsBuffer);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, ":"))) {
            break;
        }
        *now_ptr = 0;
        dnsAn_ptr[cnt]->u.srv.port = OSAL_atoi(pDnsBuffer);
        pDnsBuffer = now_ptr + 1;

        if (NULL == (now_ptr = OSAL_strscan(pDnsBuffer, "\n"))) {
            break;
        }
        *now_ptr = 0;
        OSAL_strncpy(dnsAn_ptr[cnt]->u.srv.target, pDnsBuffer,
                MAX_DOMAIN_NAME_LEN);
        pDnsBuffer = now_ptr + 1;

        cnt++;
    }

    /* Sort list of records based on priority */
    do {
        swap = 0;
        for (i = 0; i < (cnt - 1); i++) {
            if (dnsAn_ptr[i]->u.srv.priority >
                    dnsAn_ptr[i + 1]->u.srv.priority) {
                dnsAn_ptr[cnt] = dnsAn_ptr[i];
                dnsAn_ptr[i] = dnsAn_ptr[i + 1];
                dnsAn_ptr[i + 1] = dnsAn_ptr[cnt];
                swap = 1;
            }
        }
    } while (swap);

    /* Sort list of records based on weight if priority are the same*/
    do {
        swap = 0;
        for (i = 0; i < (cnt - 1); i++) {
            if (dnsAn_ptr[i]->u.srv.priority == dnsAn_ptr[i + 1]->u.srv.priority &&
                    dnsAn_ptr[i]->u.srv.weight < dnsAn_ptr[i + 1]->u.srv.weight) {
                dnsAn_ptr[cnt] = dnsAn_ptr[i];
                dnsAn_ptr[i] = dnsAn_ptr[i + 1];
                dnsAn_ptr[i + 1] = dnsAn_ptr[cnt];
                swap = 1;
            }
        }
    } while (swap);

    /* Resolve everything */
    for (i = 0, x = 0; i < cnt && x < MAX_DNS_IP_ADDRESSES; i++) {
        if (!OSAL_netIsAddrIpv6(&pTrans->lclConn.addr)) {
            /* ipv4 */
            x += _TRANSPORT_DnsARec(
                    dnsAn_ptr[i]->u.srv.target,
                    pDnsBuffer,
                    MAX_DNS_BUFFER_SIZE_BYTES,
                    &pTrans->rmtConn.addrSet[x],
                    (MAX_DNS_IP_ADDRESSES - x),
                    OSAL_netHtons(dnsAn_ptr[i]->u.srv.port));
        }
        else {
            /* Query ipv6 address */
            x += _TRANSPORT_DnsAAAARec(
                    dnsAn_ptr[i]->u.srv.target,
                    pDnsBuffer,
                    MAX_DNS_BUFFER_SIZE_BYTES,
                    &pTrans->rmtConn.addrSet[x],
                    (MAX_DNS_IP_ADDRESSES - x),
                    OSAL_netHtons(dnsAn_ptr[i]->u.srv.port));
        }
    }

    if (x == 0) {
        /* Then we have no addresses, return failure */
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================_TRANSPORT_GetAddrPort()===================
 *
 * This function determines what the IP Address and port should be for the URI
 * specified in pUri.  
 *
 * If successfull them the pPort param will contain an IP port number
 * and the Array of IP addresses 'Addr' will contain the IP addresses that
 * the uri 'owns' or belongs to. 
 *
 * RETURNS:
 *      SIP_OK: IP Addresses and IP port was successfully determined
 *      SIP_FAILED: Could not resolve the domain name via DNS A Record lookup
 *      SIP_NOT_SUPPORTED: The transport type is not currently supported
 *
 ******************************************************************************
 */
static vint _TRANSPORT_GetAddrPort(
    tTransport     *pTrans,
    tUri           *pUri,
    tTransportType  transport)
{
    tNetworkAddrType type;
    uint16           port;
    OSAL_NetAddress *pAddr;

    pAddr = &pTrans->rmtConn.addrSet[0];
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "_TRANSPORT_GetAddrPort: type= %d, port= %d",
            (int)(pUri->host.addressType), pUri->host.port, 0);

    /* This logic is right out of RFC3263 */

    /* 
     * First check and see if the 'maddr' parameter is populated.
     * If so, then use it as the target address. The RFC says that
     * 'maddr' will override any other IP address or domain names
     */
    if (eNwAddrIPv6 == pUri->host.addressType) {
        if (!OSAL_netIpv6IsAddrZero(pUri->maddr.v6)) {
            OSAL_netIpv6Hton(pAddr->ipv6, pUri->maddr.v6);
            port = pUri->host.port;
            if (port == 0) {
                if (transport == eTransportUdp || transport == eTransportTcp) {
                    port = SIP_DEFAULT_IPADDR_PORT;
                }
                else {
                    port = SIP_DEFAULT_TLS_IPADDR_PORT;
                }
            }
            pAddr->port = OSAL_netHtons(port);
            return (SIP_OK);
        }
    }
    else if (pUri->maddr.v4.ul != 0) {
        /* then we have an 'maddr' parameter */
        pAddr->ipv4 = OSAL_netHtonl(pUri->maddr.v4.ul);
        port = pUri->host.port;
        if (port == 0) {
            if (transport == eTransportUdp || transport == eTransportTcp) {
                port = SIP_DEFAULT_IPADDR_PORT;
            }
            else {
                port = SIP_DEFAULT_TLS_IPADDR_PORT;
            }
        }
        pAddr->port = OSAL_netHtons(port);
        return (SIP_OK);
    }

    type = pUri->host.addressType;
    if (type == eNwAddrIPv6) {
        if (!OSAL_netIpv6IsAddrZero(pUri->host.x.ip.v6)) {
            /* Place in network byte order */
            OSAL_netIpv6Hton(pAddr->ipv6, pUri->host.x.ip.v6);
            pAddr->type = OSAL_NET_SOCK_UDP_V6;
            port = pUri->host.port;
            if (port == 0) {
                if (transport == eTransportUdp || transport == eTransportTcp) {
                    port = SIP_DEFAULT_IPADDR_PORT;
                }
                else {
                    port = SIP_DEFAULT_TLS_IPADDR_PORT;
                }
            }
            /* Place in network byte order */
            pAddr->port = OSAL_netHtons(port);
            return (SIP_OK);
        }
    }
    else if (type == eNwAddrIPv4) {
        if (pUri->host.x.ip.v4.ul != OSAL_NET_INADDR_ANY) {
            /* Place in network byte order */
            pAddr->ipv4 = OSAL_netHtonl(pUri->host.x.ip.v4.ul);
            port = pUri->host.port;
            if (port == 0) {
                if (transport == eTransportUdp || transport == eTransportTcp) {
                    port = SIP_DEFAULT_IPADDR_PORT;
                }
                else {
                    port = SIP_DEFAULT_TLS_IPADDR_PORT;
                }
            }
            /* Place in network byte order */
            pAddr->port = OSAL_netHtons(port);
            return (SIP_OK);
        }
    }
    else if (type == eNwAddrDomainName) {
        port = pUri->host.port;
        if (port != 0) {
            /* perform dns lookups and use those addresses */
            if (!OSAL_netIsAddrIpv6(&pTrans->lclConn.addr)) {
                if (0 == _TRANSPORT_DnsARec(
                        pUri->host.x.domainName,
                        pTrans->dnsScratch.buffer,
                        MAX_DNS_BUFFER_SIZE_BYTES,
                        &pTrans->rmtConn.addrSet[0],
                        MAX_DNS_IP_ADDRESSES,
                        OSAL_netHtons(port))) {
                    return (SIP_FAILED);
                }
            }
            else {
                /* ipv6 */
                if (0 == _TRANSPORT_DnsAAAARec(
                        pUri->host.x.domainName,
                        pTrans->dnsScratch.buffer,
                        MAX_DNS_BUFFER_SIZE_BYTES,
                        &pTrans->rmtConn.addrSet[0],
                        MAX_DNS_IP_ADDRESSES,
                        OSAL_netHtons(port))) {
                    return (SIP_FAILED);
                }
                return (SIP_OK);
            }
            return (SIP_OK);
        }
        
        if (transport != eTransportNone) {
            /* Then we already know the transport type but not 
             * the port.  So try an SRV lookup with the specified 
             * transport type
             */

            if (_TRANSPORT_GetSrv(pTrans, pUri->host.x.domainName,
                    transport) == SIP_OK) {
                return (SIP_OK);
            }
            /* Otherwise we still don't have any addresses! Continue */
            if (transport == eTransportUdp || transport == eTransportTcp) {
                port = SIP_DEFAULT_IPADDR_PORT;
            }
            else {
                port = SIP_DEFAULT_TLS_IPADDR_PORT;
            }
            if (!OSAL_netIsAddrIpv6(&pTrans->lclConn.addr)) {
                if (0 == _TRANSPORT_DnsARec(
                        pUri->host.x.domainName,
                        pTrans->dnsScratch.buffer,
                        MAX_DNS_BUFFER_SIZE_BYTES,
                        &pTrans->rmtConn.addrSet[0],
                        MAX_DNS_IP_ADDRESSES,
                        OSAL_netHtons(port))) {
                    return (SIP_FAILED);
                }
            }
            else {
                /* Query ipv6 address */
                if (0 == _TRANSPORT_DnsAAAARec(
                        pUri->host.x.domainName,
                        pTrans->dnsScratch.buffer,
                        MAX_DNS_BUFFER_SIZE_BYTES,
                        &pTrans->rmtConn.addrSet[0],
                        MAX_DNS_IP_ADDRESSES,
                        OSAL_netHtons(port))) {
                    return (SIP_FAILED);
                }
                return (SIP_OK);
            }
            return (SIP_OK);
        }
        /* Later there should be an else here to handle Naptr entries */
    }
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "_TRANSPORT_GetAddrPort: unsupported addr type= %d (port= %d)",
                (int)type, pUri->host.port, 0);
    return (SIP_NOT_SUPPORTED);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_GetHostByUri()===================
 *
 * This function will determine the transport type, IP addresses and port 
 * pairs for a URI specified pUri.
 *
 * pUri = The URI in question.
 *
 * useNaptr = TRUE: Attempt to resolve via naptr records FALSE: Don't    
 *
 * pTransport = A pointer to a tTrasnportType variable. this will be populated 
 *              with the transport that should be used if the function is
 *              successfull.
 *
 * Addr = An array to place groups of resolved IP addresses, please note that 
 *        The array past in here should be at least as big as 
 *        MAX_DNS_IP_ADDRESSES defined in sip_sip.h.
 *
 * RETURNS:
 *      SIP_OK: has resolved some IP addresses
 *      SIP_NO_MEM: Couldn't resolve any IP addresses.
 *
 ******************************************************************************
 */
vint TRANSPORT_GetHostByUri (
    tTransport     *pTrans,
    tUri           *pUri,
    vint            useNaptr,
    tTransportType *pTransport)
{
    tTransportType transport;

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3, "TRANSPORT_GetHostByUri: useNaptr=%d",
            useNaptr, 0, 0);

    transport = eTransportNone;
    
    /* Get the transport per the rules in RFC3263 */
    if (_TRANSPORT_GetTransport(pTrans, pUri, &transport, useNaptr) != SIP_OK) {
        /* We don't know the transport, then use udp as a default */
            transport = eTransportUdp;
    }
    /* Get the IP address and IP Port per the rules in RFC3263 */
    if (_TRANSPORT_GetAddrPort(pTrans, pUri, transport) != SIP_OK) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_GetHostByUri: GetAddrPort() failed", 0, 0, 0);
        return (SIP_FAILED);
    }
    /* Addr and port should be set by now so just set the transport */
    *pTransport = transport;
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSPORT_GetHostByUri: returning rmtAddr type:%d, addr:",
            (int)pTrans->rmtConn.addrSet[0].type, 0, 0);
    SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->rmtConn.addrSet[0]);

    return (SIP_OK);
}

